/*
 *
 *        _/_/_/    _/_/   _/    _/ _/_/_/    _/_/
 *       _/   _/ _/    _/ _/_/  _/ _/   _/ _/    _/
 *      _/_/_/  _/_/_/_/ _/  _/_/ _/   _/ _/_/_/_/
 *     _/      _/    _/ _/    _/ _/   _/ _/    _/
 *    _/      _/    _/ _/    _/ _/_/_/  _/    _/
 *
 *  ***********************************************
 *                   PandA Project
 *   URL: https://github.com/ferrandi/PandA-bambu
 *            Politecnico di Milano - DEIB
 *             System Architectures Group
 *  ***********************************************
 *   Copyright (C) 2025-2026 Politecnico di Milano
 *
 * Part of the PandA Project, under the Apache License v2.0 with LLVM Exceptions.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 *
 */
/*
 * @author Tommaso Fellegara <tommaso.fellegara@polimi.it>
 *
 */
#ifndef NDEBUG
#define NDEBUG
#endif
// #undef NDEBUG
#include "PointerResolutionPass.hpp"
#include "llvm/ADT/SmallPtrSet.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/IR/CFG.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/PassManager.h"
#include "llvm/IR/Verifier.h"
#include "llvm/Pass.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Transforms/Utils/ValueMapper.h"
#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/Instruction.h>
#include <map>
#include <memory>
#include <set>

#include "debug_print.hpp"

#define PASS_PREFIX "[PTR_RES] "

#if PANDA_LLVM_CLANG_MAJOR > 9
#define make_unique std::make_unique
#else
#define make_unique llvm::make_unique
#endif

#define CREATE_FATAL_REPORT(msg) (llvm::Twine(msg) + " (" + __func__ + ":" + llvm::Twine(__LINE__) + ")")
#define REPORT_FATAL_ERROR_WITH_REPORT(msg)         \
   do                                               \
   {                                                \
      report_fatal_error(CREATE_FATAL_REPORT(msg)); \
   } while(false)

#define REPORT_WITH_PRINT(v, msg)          \
   do                                      \
   {                                       \
      (v)->print(llvm::errs());            \
      llvm::errs() << "\n";                \
      REPORT_FATAL_ERROR_WITH_REPORT(msg); \
   } while(false)

using namespace llvm;

namespace
{
   void printModuleOnFile(Module& M, const std::string& outPath)
   {
      std::error_code EC;
      raw_fd_ostream OS(outPath, EC,
#if PANDA_LLVM_CLANG_MAJOR >= 10
                        sys::fs::OF_None
#else
                        sys::fs::F_None
#endif
      );
      M.print(OS, nullptr);
   }

   Value* handleGEPInChain(Value* ptr, GetElementPtrInst* GEP, IRBuilder<>& b)
   {
      SmallVector<Value*, 4> idxs(GEP->idx_begin(), GEP->idx_end());
      if(GEP->isInBounds())
         return b.CreateInBoundsGEP(GEP->getSourceElementType(), ptr, idxs);
      return b.CreateGEP(GEP->getSourceElementType(), ptr, idxs);
   }

   Value* handleBitCastInChain(Value* ptr, BitCastInst* BC, IRBuilder<>& b)
   {
      return b.CreateBitCast(ptr, BC->getDestTy());
   }

   template <typename MemInst>
   void copyMemoryProperties(const MemInst* From, MemInst* To)
   {
#if PANDA_LLVM_CLANG_MAJOR >= 10
      To->setAlignment(From->getAlign());
#else
      To->setAlignment(From->getAlignment());
#endif

      To->setVolatile(From->isVolatile());

      if(From->isAtomic())
      {
         To->setOrdering(From->getOrdering());
#if PANDA_LLVM_CLANG_MAJOR <= 4
         To->setSynchScope(From->getSynchScope());
#else
         To->setSyncScopeID(From->getSyncScopeID());
#endif
      }
   }

   Value* handleLoadInChain(Value* ptr, LoadInst* LD, IRBuilder<>& b)
   {
      auto* newLD = b.CreateLoad(LD->getType(), ptr);
      copyMemoryProperties(LD, newLD);
      return newLD;
   }

   void handleStoreInChain(Value* ptr, StoreInst* ST, IRBuilder<>& b)
   {
      auto* newST = b.CreateStore(ST->getValueOperand(), ptr);
      copyMemoryProperties(ST, newST);
   }

   inline bool areInstsInSameBB(Instruction* I1, Instruction* I2)
   {
      return I1->getParent() == I2->getParent();
   }

   bool isPointerUsedAsPayloadInStore(SmallVector<Instruction*, 4>& chain)
   {
      // For a store, make sure the chain feeds the *address*, not the stored value
      // (e.g. reject `store %select_derived_ptr, ptr %other`).
      assert(chain.size() >= 2);
      auto* lastPtr = chain[chain.size() - 2];
      auto* ST = cast<StoreInst>(chain.back());
      return ST->getValueOperand() == lastPtr;
   }

   Instruction* findSingleMemOpThroughChain(Value* root, SmallVector<Instruction*, 4>& chain)
   {
      LLVM_DEBUG(dbgs() << "  Finding singleMemOp through single use chain:");
      Value* cur = root;
      while(true)
      {
         LLVM_DEBUG(cur->print(dbgs()); dbgs() << " ;");
         if(!cur->hasOneUse())
            return nullptr;

         auto* user = cast<Instruction>(*cur->user_begin());

         if(isa<LoadInst>(user) || isa<StoreInst>(user))
         {
            chain.push_back(user);
            return user;
         }

         if(isa<GetElementPtrInst>(user) || isa<BitCastInst>(user))
         {
            chain.push_back(user);
            cur = user;
            continue;
         }

         return nullptr; // phi, call, another select, etc. — chain breaks
      }
   }

   inline bool isInstUnreachable(Instruction& I)
   {
      return isa<llvm::UnreachableInst>(I);
   }

   size_t numPredecessorsBB(BasicBlock* BB)
   {
      size_t NumPreds = 0;
      for (BasicBlock *Pred : predecessors(BB))
         ++NumPreds;

      return NumPreds;
   }

   BasicBlock *getMergeBB(SwitchInst *SW) {
      assert(SW->getNumCases() > 0);
      auto it = SW->case_begin();
      BasicBlock *Succ = (*it).getCaseSuccessor();

      // Case A: switch -> middle_bb -> merge_bb
      if (Succ->size() == 1) {
         if (auto *Br = dyn_cast<BranchInst>(Succ->getTerminator())) {
            if (Br->isUnconditional())
               return Br->getSuccessor(0);
         }
      }

      // Case B: switch -> merge_bb
      return Succ;
   }

   /// Returns true if the BB is inserted in the BBs
   bool insertIfAbsent(std::vector<BasicBlock*>& BBs, BasicBlock* BB)
   {
      if(llvm::is_contained(BBs, BB))
         return false;
      BBs.push_back(BB);
      return true;
   }

   bool isSwitchDiamond(SwitchInst* SW, BasicBlock* &MergeBB) {
      auto* BB0 = SW->getParent();
      auto* DefBB = SW->getDefaultDest();
      MergeBB = getMergeBB(SW);

      // the default case should be a basic block with only 1 instruction
      // that is the unreachable intrinsic of LLVM
      if(DefBB->size() != 1 || !isInstUnreachable(DefBB->front()))
         return false;

      std::vector<BasicBlock*> BBs;
      // All the BBs should be different
      if (!insertIfAbsent(BBs, BB0) || !insertIfAbsent(BBs, DefBB) || !insertIfAbsent(BBs, MergeBB))
         return false;

      for (BasicBlock *Succ : successors(BB0)) {
         if(Succ == SW->getDefaultDest() || Succ == MergeBB)
            continue;

         // All the BBs should be different
         if(!insertIfAbsent(BBs, Succ))
            return false;

         // Enforce that the cases blocks have a single predecessor
         if (numPredecessorsBB(Succ) != 1)
            return false;

         // Enforce only 1 instruction that is a branch unconditional
         if (Succ->size() != 1)
            return false;

         auto *Br = dyn_cast<BranchInst>(Succ->getTerminator());
         if (!Br || !Br->isUnconditional())
            return false;

         if (MergeBB != Br->getSuccessor(0))
            return false;
      }

      if (numPredecessorsBB(MergeBB) != SW->getNumCases())
         return false;

      return true;
   }

   SmallVector<PHINode*, 4> collectPhis(BasicBlock* BB)
   {
      assert(BB);
      SmallVector<PHINode*, 4> phis;
      for(auto& I : *BB)
      {
         if(auto* phi = dyn_cast<PHINode>(&I))
         {
            phis.push_back(phi);
         }
         else
         {
            break;
         }
      }
      return phis;
   }

   bool allPhisAreOnPointers(const SmallVector<PHINode*, 4>& phis)
   {
      return llvm::all_of(phis, [](const PHINode* phi) {
         return phi->getType()->isPointerTy();
      });
   }

   void splitSwitchChain(SwitchInst* SW, BasicBlock* MergeBB, SmallVector<Instruction*, 4>& chain)
   {
      auto* rootPhi = cast<PHINode>(chain[0]);
      auto* memInst = chain.back();
      assert(isa<LoadInst>(memInst) || isa<StoreInst>(memInst));
      LLVMContext& ctx = SW->getContext();
      Function* f = SW->getFunction();
      // This is done so the split is where the load/store is located
      IRBuilder<> b(memInst);

      auto* bbBeforeMemInst = memInst->getParent();
      auto* bbFromMemInstOn = memInst->getParent()->splitBasicBlock(memInst);

      bbBeforeMemInst->getTerminator()->eraseFromParent();
      b.SetInsertPoint(bbBeforeMemInst);
      auto* newSW = b.CreateSwitch(SW->getCondition(), SW->getDefaultDest(), SW->getNumCases());

      PHINode* mergeLoadPhi = nullptr;
      if(isa<LoadInst>(memInst))
      {
         auto* LD = cast<LoadInst>(memInst);
         b.SetInsertPoint(bbFromMemInstOn, bbFromMemInstOn->getFirstInsertionPt());
         mergeLoadPhi = b.CreatePHI(LD->getType(), SW->getNumCases());
         LD->replaceAllUsesWith(mergeLoadPhi);
      }

      for(auto& caseIt : SW->cases())
      {
         auto* caseValue = caseIt.getCaseValue();
         auto* caseBB = caseIt.getCaseSuccessor() == MergeBB ? SW->getParent() : caseIt.getCaseSuccessor();
         Value* ptr = rootPhi->getIncomingValueForBlock(caseBB);
         auto* newBB = BasicBlock::Create(ctx, "", f, bbFromMemInstOn);
         newSW->addCase(caseValue, newBB);
         b.SetInsertPoint(newBB);
         for(size_t idxInst = 1; idxInst < chain.size(); idxInst++)
         {
            auto* inst = chain[idxInst];
            if(auto* GEP = dyn_cast<GetElementPtrInst>(inst))
            {
               ptr = handleGEPInChain(ptr, GEP, b);
            }
            else if(auto* BC = dyn_cast<BitCastInst>(inst))
            {
               ptr = handleBitCastInChain(ptr, BC, b);
            }
            else if(auto* LD = dyn_cast<LoadInst>(inst))
            {
               auto* newLoad = handleLoadInChain(ptr, LD, b);
               mergeLoadPhi->addIncoming(newLoad, newBB);
            }
            else if(auto* ST = dyn_cast<StoreInst>(inst))
            {
               handleStoreInChain(ptr, ST, b);
            }
            else
            {
               llvm_unreachable("Inside the chain there could be only GetElementPtr/BitCast/LoadInst/StoreInst");
            }
         }
         b.CreateBr(bbFromMemInstOn);
      }
   }

   void eraseInstWithRevIters(SmallVector<Instruction*, 4>::reverse_iterator begin, SmallVector<Instruction*, 4>::reverse_iterator end)
   {
      for(auto it = begin; it != end; ++it)
      {
         (*it)->eraseFromParent();
      }
   }

   struct SwitchChainToMemOp
   {
      SwitchInst* SW;
      BasicBlock* MergeBB;
      SmallVector<SmallVector<Instruction*, 4>, 4> chains;

      SwitchChainToMemOp(SwitchInst* SW, BasicBlock* MergeBB, SmallVector<SmallVector<Instruction*, 4>, 4> chains)
         : SW(SW), MergeBB(MergeBB), chains(chains)
      {}
   };

   /// This pass is done to recognize this pattern and move the load/store
   /// towards their specific memory:
   ///               bb0
   ///             /  |  \
   ///            |  bb2  bb3
   ///             \  |  /
   ///            bb_merge
   /// In bb0 there is a switch, in bb2 and bb3 there is only a br to bb_merge
   /// and in bb_merge there is a phi on the memories that are used in a load/store.
   /// The pass splits the bb_merge in proximity of load/store and creates as many bbs
   /// as there were in the switch with the specific memory used. The bb2, bb3 are destroyed
   /// and also the phi on the GEPs, then the bb0 and bb_merge are reassembled
   /// The pass does not use the Alias Analysis because I suppose that all the GEPs
   /// are on different memories. To be fair AA should be used
   bool runOnSwitchPointers(Module& M)
   {
      LLVM_DEBUG(dbgs() << PASS_PREFIX "Running switch on pointers\n");
      bool changed = false;
      SmallVector<SwitchChainToMemOp, 4> switchChains;
      for(auto& F : M)
      {
         if(F.isDeclaration())
            continue;

         for(auto& BB : F)
         {
            auto* term = BB.getTerminator();
            if(auto* SW = dyn_cast<SwitchInst>(term))
            {
               LLVM_DEBUG({
                  dbgs() << "Processing the switch\n";
                  SW->print(dbgs());
                  dbgs() << "\n";
                  dbgs() << "  Inserted chains\n";
               });
               BasicBlock* MergeBB = nullptr;
               LLVM_DEBUG(dbgs() << "  Checking if the switch is in a diamond config\n");
               if(!isSwitchDiamond(SW, MergeBB))
                  continue;

               LLVM_DEBUG(dbgs() << "  Collecting phis\n");
               SmallVector<PHINode*, 4> MergePhis = collectPhis(MergeBB);
               // there should be at least one phi and all phis should be on pointers
               if(MergePhis.empty() || !allPhisAreOnPointers(MergePhis))
                  continue;

               LLVM_DEBUG(dbgs() << "  Collecting the chains\n");
               bool woErrs = true;
               SmallVector<SmallVector<Instruction*, 4>, 4> chainsToProcess;
               for(PHINode* phi : MergePhis)
               {
                  SmallVector<Instruction*, 4> chain;
                  chain.push_back(phi);
                  auto* finalMemInst = findSingleMemOpThroughChain(phi, chain);

                  if(!finalMemInst)
                  {
                     LLVM_DEBUG(dbgs() << "    finalMemInst not found\n");
                     woErrs = false;
                     break;
                  }

                  if(isa<StoreInst>(finalMemInst) && isPointerUsedAsPayloadInStore(chain))
                  {
                     LLVM_DEBUG(dbgs() << "    finalMemInst is a store, but the address is in the payload\n");
                     woErrs = false;
                     break;
                  }

                  if(!areInstsInSameBB(phi, finalMemInst))
                  {
                     LLVM_DEBUG(dbgs() << "    phi and finalMemInst are not in the same BB\n");
                     woErrs = false;
                     break;
                  }

                  LLVM_DEBUG({
                     dbgs() << "    ";
                     chain.front()->print(dbgs());
                     dbgs() << "  =>";
                     chain.back()->print(dbgs());
                     dbgs() << "\n";
                  });
                  chainsToProcess.push_back(chain);
               }

               if(woErrs)
               {
                  switchChains.push_back(SwitchChainToMemOp(SW, MergeBB, chainsToProcess));
               }
            }
         }
      }

      LLVM_DEBUG(dbgs() << PASS_PREFIX"Processing the switchChains\n");
      for(auto& switchChain : switchChains)
      {
         auto* SW = switchChain.SW;
         for(SmallVector<Instruction*, 4>& chain : switchChain.chains)
         {
            changed = true;
            splitSwitchChain(SW, switchChain.MergeBB, chain);
         }
      }

      LLVM_DEBUG(dbgs() << PASS_PREFIX"Erasing the switchChains\n");
      for(auto& switchChain : switchChains)
      {
         auto* SW = switchChain.SW;
         auto* MergeBB = switchChain.MergeBB;
         for(SmallVector<Instruction*, 4>& chain : switchChain.chains)
         {
            eraseInstWithRevIters(chain.rbegin(), chain.rend());
         }

         SmallVector<BasicBlock*, 4> CaseBBs;
         SmallPtrSet<BasicBlock*, 4> SeenCaseBBs;
         for(auto& caseIt : SW->cases())
         {
            auto* caseBB = caseIt.getCaseSuccessor();
            if(caseBB != MergeBB && SeenCaseBBs.insert(caseBB).second)
               CaseBBs.push_back(caseBB);
         }

         for(auto instIt = MergeBB->begin(); instIt != MergeBB->end();)
         {
            Instruction &I = *(instIt++);
            I.moveBefore(SW);
         }
         SW->eraseFromParent();

         for(auto* CaseBB : CaseBBs)
            CaseBB->eraseFromParent();
         MergeBB->eraseFromParent();
      }

      return changed;
   }

   inline bool isSelectOnPointers(SelectInst* select)
   {
      return select->getType()->isPointerTy();
   }

   /// Two pointers points to the same memory if the result of the
   /// Alias Analysis on the pointers is MustAlias or PartialAlias
   bool arePointersToSameMemory(AAResults& AA, Value* ptr1, Value* ptr2)
   {
      assert(ptr1 && ptr2 && "One of the two pointers is nullptr");
#if PANDA_LLVM_CLANG_MAJOR < 12
      MemoryLocation loc1(ptr1);
      MemoryLocation loc2(ptr2);
      AliasResult aliasResult = AA.alias(loc1, loc2);
#else
      AliasResult aliasResult = AA.alias(ptr1, ptr2);
#endif
      return aliasResult == AliasResult::MustAlias || aliasResult == AliasResult::PartialAlias;
   }

   void splitSelectChain(SmallVector<Instruction*, 4>& chain)
   {
      auto* select = cast<SelectInst>(chain[0]);
      auto* memInst = chain.back();
      assert(isa<LoadInst>(memInst) || isa<StoreInst>(memInst));
      LLVMContext& ctx = select->getContext();
      Function* f = select->getParent()->getParent();
      // This is done so the split is where the load/store is located
      IRBuilder<> b(memInst);

      auto* bbBeforeMemInst = memInst->getParent();
      auto* bbFromMemInstOn = memInst->getParent()->splitBasicBlock(memInst);
      auto* trueBB = BasicBlock::Create(ctx, "", f, bbFromMemInstOn);
      auto* falseBB = BasicBlock::Create(ctx, "", f, bbFromMemInstOn);

      bbBeforeMemInst->getTerminator()->eraseFromParent();
      b.SetInsertPoint(bbBeforeMemInst);
      b.CreateCondBr(select->getCondition(), trueBB, falseBB);

      PHINode* phi = nullptr;
      if(isa<LoadInst>(memInst))
      {
         auto* LD = cast<LoadInst>(memInst);
         b.SetInsertPoint(bbFromMemInstOn, bbFromMemInstOn->getFirstInsertionPt());
         phi = b.CreatePHI(LD->getType(), 2);
         LD->replaceAllUsesWith(phi);
      }

      for(size_t i = 0; i < 2; i++)
      {
         auto* BB = i % 2 == 0 ? trueBB : falseBB;
         auto* ptr = i % 2 == 0 ? select->getTrueValue() : select->getFalseValue();
         b.SetInsertPoint(BB);
         for(size_t idxInst = 1; idxInst < chain.size(); idxInst++)
         {
            auto* inst = chain[idxInst];
            if(auto* GEP = dyn_cast<GetElementPtrInst>(inst))
            {
               ptr = handleGEPInChain(ptr, GEP, b);
            }
            else if(auto* BC = dyn_cast<BitCastInst>(inst))
            {
               ptr = handleBitCastInChain(ptr, BC, b);
            }
            else if(auto* LD = dyn_cast<LoadInst>(inst))
            {
               auto* newLoad = handleLoadInChain(ptr, LD, b);
               phi->addIncoming(newLoad, BB);
            }
            else if(auto* ST = dyn_cast<StoreInst>(inst))
            {
               handleStoreInChain(ptr, ST, b);
            }
            else
            {
               llvm_unreachable("Inside the chain there could be only GetElementPtr/BitCast/LoadInst/StoreInst");
            }
         }
         b.CreateBr(bbFromMemInstOn);
      }
   }

   /// This pass is done to split this type of operation:
   /// %ptr = select %2, %ptr1, %ptr2
   /// load/store on %ptr
   /// but if %ptr1 and %ptr2 are pointers to different memories, the select
   /// should be split and also the load/store and it becomes
   /// br %2, bb1, bb2
   /// bb1:
   /// load/store on %ptr1
   /// br merge
   /// bb2:
   /// load/store on %ptr2
   /// br merge
   /// merge:
   /// (optional phi if there is a load)
   /// The pass manages also when there is a bitcast or and GEPs between the
   /// select and the load/store
   bool runOnSelectPointers(Module& M, llvm::function_ref<llvm::AAResults&(llvm::Function&)> GetAA)
   {
      bool changed = false;
      SmallVector<SmallVector<Instruction*, 4>, 4> chainsToProcess;
      for(auto& F : M)
      {
         if(F.isDeclaration())
            continue;

         auto& AA = GetAA(F);
         for(auto& BB : F)
         {
            for(auto& I : BB)
            {
               if(auto* select = dyn_cast<SelectInst>(&I))
               {
                  Value* trueVal = select->getTrueValue();
                  Value* falseVal = select->getFalseValue();
                  if(!isSelectOnPointers(select) || arePointersToSameMemory(AA, trueVal, falseVal))
                     continue;

                  SmallVector<Instruction*, 4> chain;
                  chain.push_back(select);
                  auto* finalMemInst = findSingleMemOpThroughChain(select, chain);

                  if(!finalMemInst)
                     continue;

                  if(isa<StoreInst>(finalMemInst) && isPointerUsedAsPayloadInStore(chain))
                     continue;

                  if(!areInstsInSameBB(select, finalMemInst))
                     continue;

                  chainsToProcess.emplace_back(chain);
               }
            }
         }
      }

      for(SmallVector<Instruction*, 4>& chain : chainsToProcess)
      {
         changed = true;
         splitSelectChain(chain);
      }

      for(SmallVector<Instruction*, 4>& chain : chainsToProcess)
      {
         eraseInstWithRevIters(chain.rbegin(), chain.rend());
      }

      return changed;
   }
} // namespace

namespace llvm
{
   bool PointerResolutionPass::exec(Module& M, llvm::function_ref<llvm::AAResults&(llvm::Function&)> GetAA)
   {
      LLVM_DEBUG(dbgs() << "Started Pass: POINTER RESOLUTION\n");
      LLVM_DEBUG(printModuleOnFile(M, outdirNameCmd + "/before_pointer_resolution.ll"));
      // M.print(dbgs(), nullptr);
      bool changed = false;

      changed |= runOnSelectPointers(M, GetAA);
      LLVM_DEBUG(dbgs() << "[PTR_RES] Ending run on select\n");
      changed |= runOnSwitchPointers(M);

      LLVM_DEBUG(printModuleOnFile(M, outdirNameCmd + "/after_pointer_resolution.ll"));
      assert(!llvm::verifyModule(M, &llvm::errs()));
      LLVM_DEBUG(dbgs() << "Ended Pass: POINTER RESOLUTION\n");
      return changed;
   }

   bool PointerResolutionPass::runOnModule(Module& M)
   {
#if PANDA_LLVM_CLANG_MAJOR < 13
      auto GetAA = [&](llvm::Function& F) -> llvm::AAResults& {
         return getAnalysis<llvm::AAResultsWrapperPass>(F).getAAResults();
      };
      return exec(M, GetAA);
#else
      REPORT_FATAL_ERROR_WITH_REPORT("Call to runOnModule not expected with current LLVM version");
      return false;
#endif
   }

   StringRef PointerResolutionPass::getPassName() const
   {
      return "POINTER-RESOLUTION";
   }

   void PointerResolutionPass::getAnalysisUsage(AnalysisUsage& AU) const
   {
      AU.addRequired<llvm::AAResultsWrapperPass>();
      AU.setPreservesAll();
   }

#if PANDA_LLVM_CLANG_MAJOR >= 13
   PreservedAnalyses PointerResolutionPass::run(Module& M, ModuleAnalysisManager& MAM)
   {
      MAM.invalidate(M, llvm::PreservedAnalyses::none());
      auto& FAM = MAM.getResult<FunctionAnalysisManagerModuleProxy>(M).getManager();
      auto GetAA = [&](llvm::Function& F) -> llvm::AAResults& { return FAM.getResult<llvm::AAManager>(F); };

      const auto changed = exec(M, GetAA);
      return (changed ? llvm::PreservedAnalyses::none() : llvm::PreservedAnalyses::all());
   }
#endif

   char PointerResolutionPass::ID = 0;
} // namespace llvm
