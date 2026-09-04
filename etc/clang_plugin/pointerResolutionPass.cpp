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
#include "pointerResolutionPass.hpp"
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
#if LLVM_VERSION_MAJOR >= 10
                        sys::fs::OF_None
#else
                        sys::fs::F_None
#endif
      );
      M.print(OS, nullptr);
   }

   bool hasUsersOutsideBlock(const Instruction& inst, const BasicBlock* block)
   {
      return std::any_of(inst.user_begin(), inst.user_end(), [block](const User* user) {
         const auto* userInst = dyn_cast<Instruction>(user);
         return !userInst || userInst->getParent() != block;
      });
   }

   bool isUsedByTerminatorInBlock(const Instruction& inst, const BasicBlock* block)
   {
      return std::any_of(inst.user_begin(), inst.user_end(), [block](const User* user) {
         const auto* userInst = dyn_cast<Instruction>(user);
         return userInst && userInst->isTerminator() && userInst->getParent() == block;
      });
   }

   void populateUsersRecursively(BasicBlock* BB, Instruction* inst, std::vector<Instruction*>& instsToClone)
   {
      for(auto it = inst->op_begin(), e = inst->op_end(); it != e; ++it)
      {
         if(auto* operandInst = dyn_cast<Instruction>(*it))
         {
            if(BB != operandInst->getParent())
            {
               continue;
            }
            if(std::find(instsToClone.begin(), instsToClone.end(), operandInst) == instsToClone.end())
            {
               populateUsersRecursively(BB, operandInst, instsToClone);
               instsToClone.push_back(operandInst);
            }
         }
      }

      instsToClone.push_back(inst);

      for(auto* user : inst->users())
      {
         if(auto* userInst = dyn_cast<Instruction>(user))
         {
            if(std::find(instsToClone.begin(), instsToClone.end(), userInst) == instsToClone.end())
            {
               instsToClone.push_back(userInst);
               populateUsersRecursively(BB, userInst, instsToClone);
            }
         }
      }
   }

   void populatePhiNode(PHINode* phi, std::vector<Instruction*>& instsToClone)
   {
      for(auto* user : phi->users())
      {
         if(auto* userInst = dyn_cast<Instruction>(user))
         {
            assert(userInst->getParent() == phi->getParent() && "A PHI node should not have users outside its block");
            if(std::find(instsToClone.begin(), instsToClone.end(), userInst) == instsToClone.end())
            {
               populateUsersRecursively(phi->getParent(), userInst, instsToClone);
            }
         }
      }
   }

   void populateInstsToClone(BasicBlock* BB, std::vector<Instruction*>& instsToClone)
   {
      for(auto it = BB->begin(), e = BB->end(); it != e; ++it)
      {
         auto* inst = &*it;
         if(auto* phi = dyn_cast<PHINode>(inst))
         {
            if(phi->getType()->isPointerTy())
            {
               populatePhiNode(phi, instsToClone);
            }
         }
      }
   }

   void processBlock(BasicBlock* BB)
   {
      // TODO: not considering the fact that the same predecessor appears two times in BB (two edges from the same
      // block to BB)
      // TODO: consider cloning only the necessary instructions and not the whole block
      LLVM_DEBUG(dbgs() << "[PTR_RES] Processing the basic block "; BB->print(dbgs()));
      SmallVector<BasicBlock*, 8> predecessorBlocks;
      SmallVector<BasicBlock*, 8> successorBlocks;
      SmallVector<BasicBlock*, 8> newPredBBs;
      SmallVector<std::unique_ptr<ValueToValueMapTy>, 8> newBBValueMaps;
      std::vector<Instruction*> instsToClone;

      for(auto* predBB : llvm::predecessors(BB))
      {
         predecessorBlocks.push_back(predBB);
      }

      for(auto* succBB : llvm::successors(BB))
      {
         successorBlocks.push_back(succBB);
      }

      LLVM_DEBUG(dbgs() << "[PTR_RES] Creating the newBB\n");
      auto* newBB = BasicBlock::Create(BB->getContext(), "", BB->getParent(), BB);
      auto* bbBranch = BB->getTerminator();
      auto* cloneBranch = bbBranch->clone();
#if LLVM_VERSION_MAJOR >= 16
      cloneBranch->insertInto(newBB, newBB->end());
#else
      newBB->getInstList().push_back(cloneBranch);
#endif

      populateInstsToClone(BB, instsToClone);

      for(auto* predBB : predecessorBlocks)
      {
         auto vMap = make_unique<ValueToValueMapTy>();
         LLVM_DEBUG(dbgs() << "[PTR_RES] Mapping the phis\n");
         for(auto it = BB->begin(), e = BB->end(); it != e; ++it)
         {
            auto& inst = *it;
            auto* phiInst = dyn_cast<PHINode>(&inst);
            if(phiInst)
            {
               auto* incomingValue = phiInst->getIncomingValueForBlock(predBB);
               (*vMap)[phiInst] = incomingValue;
            }
         }

         LLVM_DEBUG(dbgs() << "[PTR_RES] Creating a new BB\n");
         auto* newPredBB = BasicBlock::Create(BB->getContext(), "", BB->getParent(), newBB);
         LLVM_DEBUG(dbgs() << "[PTR_RES] Creating terminator\n");
         BranchInst::Create(newBB, newPredBB);

         LLVM_DEBUG(dbgs() << "[PTR_RES] Cloning instructions\n");
         auto instIt = BB->getFirstNonPHI()->getIterator();
         const auto termIt = BB->getTerminator()->getIterator();
         for(; instIt != termIt; ++instIt)
         {
            auto* inst = &*instIt;
            auto* clone = inst->clone();
            clone->insertBefore(newPredBB->getTerminator());
            RemapInstruction(clone, *vMap, RF_IgnoreMissingLocals | RF_NoModuleLevelChanges);
            (*vMap)[inst] = clone;
         }

         LLVM_DEBUG(dbgs() << "[PTR_RES] Created the following block: "; newPredBB->print(dbgs()); dbgs() << "\n");
         newPredBBs.push_back(newPredBB);
         newBBValueMaps.push_back(std::move(vMap));
      }

      LLVM_DEBUG(dbgs() << "[PTR_RES] Adding the PHIs of newBB\n");
      auto instIt = BB->begin();
      const auto termIt = BB->getTerminator()->getIterator();
      for(; instIt != termIt; ++instIt)
      {
         auto* inst = &*instIt;
         if(hasUsersOutsideBlock(*inst, BB) || isUsedByTerminatorInBlock(*inst, BB))
         {
            auto* joinPhi = PHINode::Create(inst->getType(), predecessorBlocks.size(), "", newBB->getTerminator());
            for(size_t i = 0; i < newBBValueMaps.size(); ++i)
            {
               auto& vMap = *newBBValueMaps[i];
               joinPhi->addIncoming(vMap[inst], newPredBBs[i]);
            }
            inst->replaceAllUsesWith(joinPhi);
         }
      }
      LLVM_DEBUG(dbgs() << "[PTR_RES] The newBB is "; newBB->print(dbgs()); dbgs() << "\n");

      LLVM_DEBUG(dbgs() << "[PTR_RES] Fixing the predecessor terminator\n");
      for(size_t i = 0; i < predecessorBlocks.size(); ++i)
      {
         auto* predBB = predecessorBlocks[i];
         auto* newBB = newPredBBs[i];
         auto* predBBbranch = predBB->getTerminator();
         for(unsigned i = 0; i < predBBbranch->getNumSuccessors(); i++)
         {
            if(predBBbranch->getSuccessor(i) == BB)
            {
               predBBbranch->setSuccessor(i, newBB);
            }
         }
      }

      BB->replaceAllUsesWith(newBB);
      BB->eraseFromParent();
      LLVM_DEBUG(dbgs() << "[PTR_RES] End of processing the block\n\n");
   }

   bool isBlockToProcess(BasicBlock& BB)
   {
      std::vector<PHINode*> phis;
      for(auto it = BB.begin(), e = BB.end(); it != e; ++it)
      {
         auto& inst = *it;
         auto* phiInst = dyn_cast<PHINode>(&inst);
         if(phiInst)
         {
            phis.push_back(phiInst);
         }
      }
      std::vector<PHINode*> pointerPhis;
      std::copy_if(phis.begin(), phis.end(), std::back_inserter(pointerPhis), [](PHINode* phiInst) {
         return phiInst->getNumIncomingValues() > 1 && phiInst->getType()->isPointerTy();
      });
      return !pointerPhis.empty() &&
             // avoid splitting pure return-join blocks
             !isa<ReturnInst>(BB.getTerminator()) &&
             // require at least one non-PHI instruction before the terminator
             BB.getFirstNonPHI() != BB.getTerminator() &&
             // no successor of the block is the block itself
             std::all_of(successors(&BB).begin(), successors(&BB).end(),
                         [&BB](BasicBlock* succBB) { return succBB != &BB; }) &&
             // all the users of the pointer PHIs are instructions in the same block
             std::all_of(pointerPhis.begin(), pointerPhis.end(), [](PHINode* phiInst) {
                return std::all_of(phiInst->user_begin(), phiInst->user_end(), [phiInst](const User* u) {
                   const auto* userInst = dyn_cast<Instruction>(u);
                   return userInst && userInst->getParent() == phiInst->getParent();
                });
             });
   }
} // namespace

namespace llvm
{
   bool PointerResolutionPass::exec(Module& M)
   {
      LLVM_DEBUG(dbgs() << "[PTR_RES] Starting pointer resolution pass\n");
      LLVM_DEBUG(printModuleOnFile(M, outdirNameCmd + "/before_pointer_resolution.ll"));
      // M.print(dbgs(), nullptr);
      bool changed = false;

      for(auto& F : M)
      {
         bool bbProcessed = false;
         do
         {
            bbProcessed = false;
            for(auto& BB : F)
            {
               if(isBlockToProcess(BB))
               {
                  processBlock(&BB);
                  bbProcessed = true;
                  break;
               }
            }
            changed |= bbProcessed;
         } while(bbProcessed);
      }

      LLVM_DEBUG(printModuleOnFile(M, outdirNameCmd + "/after_pointer_resolution.ll"));
      assert(!llvm::verifyModule(M, &llvm::errs()));
      LLVM_DEBUG(dbgs() << "[PTR_RES] Ending pointer resolution pass\n");
      return changed;
   }

   bool PointerResolutionPass::runOnModule(Module& M)
   {
#if LLVM_VERSION_MAJOR < 13
      return exec(M);
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
   }

#if LLVM_VERSION_MAJOR >= 13
   PreservedAnalyses PointerResolutionPass::run(Module& M, ModuleAnalysisManager& MAM)
   {
      const auto changed = exec(M);
      return (changed ? PreservedAnalyses::none() : PreservedAnalyses::all());
   }
#endif

   char PointerResolutionPass::ID = 0;
} // namespace llvm
