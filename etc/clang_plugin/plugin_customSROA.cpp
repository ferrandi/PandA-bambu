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
/**
 * @file plugin_customSROA.cpp
 * @brief Implementation of the Custom Scalar Replacement of Aggregates Pass
 * Aiming to introduce memory partitioning into the PandA Framework.
 *
 * @author Andrea Mannarino <mannarinoandrea98@gmail.com>
 * @author Tommaso Fellegara <tommaso.fellegara@polimi.it>
 *
 */
#ifndef NDEBUG
#define NDEBUG
#endif
// #undef NDEBUG
#include "arrPart.hpp"
#include "canonicalizeHLSStreamGEPPass.hpp"
#include "loopUnrollArrPartPass.hpp"
#include "plugin_scalarizeArrayOfFifo.hpp"
#include "llvm/IR/PatternMatch.h"
#include "llvm/Transforms/Scalar/EarlyCSE.h"
#include "llvm/Transforms/Scalar/SCCP.h"
#include <algorithm>
#include <llvm/ADT/STLExtras.h>
#include <llvm/ADT/StringMap.h>
#include <llvm/Analysis/ValueTracking.h>
#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/Constant.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/Instruction.h>
#include <llvm/IR/Operator.h>
#include <llvm/Support/ErrorHandling.h>
#include <llvm/Transforms/InstCombine/InstCombine.h>
#include <llvm/Transforms/Scalar.h>
#include <llvm/Transforms/Scalar/ADCE.h>
#include <llvm/Transforms/Scalar/CorrelatedValuePropagation.h>
#include <llvm/Transforms/Scalar/DCE.h>
#include <llvm/Transforms/Scalar/IndVarSimplify.h>
#include <llvm/Transforms/Scalar/JumpThreading.h>
#include <llvm/Transforms/Scalar/LoopStrengthReduce.h>
#include <llvm/Transforms/Scalar/SROA.h>
#include <llvm/Transforms/Scalar/SimplifyCFG.h>
#include <llvm/Transforms/Utils/LoopSimplify.h>
#include <llvm/Transforms/Utils/Mem2Reg.h>

#include "llvm/ADT/DenseMap.h"
#include "llvm/Analysis/CallGraph.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Verifier.h"
#include <llvm/ADT/StringExtras.h>
#include <llvm/ADT/StringSet.h>
#include <llvm/Analysis/AliasAnalysis.h>
#include <llvm/Analysis/AssumptionCache.h>
#include <llvm/Analysis/LoopInfo.h>

#include <llvm/Analysis/ScalarEvolution.h>
#include <llvm/Analysis/TargetTransformInfo.h>
#include <llvm/IR/LegacyPassManager.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/PassManager.h>
#include <llvm/InitializePasses.h>
#include <llvm/Pass.h>
#include <llvm/PassRegistry.h>
#include <llvm/Support/CommandLine.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Transforms/IPO.h>
#include <llvm/Transforms/IPO/DeadArgumentElimination.h>
#include <llvm/Transforms/IPO/GlobalDCE.h>
#include <llvm/Transforms/IPO/Internalize.h>
#include <llvm/Transforms/IPO/SCCP.h>
#include <llvm/Transforms/Utils/Local.h>
#include <vector>

#if LLVM_VERSION_MAJOR >= 16
#include <llvm/IRPrinter/IRPrintingPasses.h>
#else
#include <llvm/IR/IRPrintingPasses.h>
#endif
#include <map>

#include <pugixml.hpp>

#if LLVM_VERSION_MAJOR >= 13
#include <llvm/Passes/PassBuilder.h>
#include <llvm/Passes/PassPlugin.h>
#else
#include <llvm/Transforms/IPO/PassManagerBuilder.h>
#endif

// If the function is a pointer
#if LLVM_VERSION_MAJOR >= 10
#define GET_ARG_PT(fn, i) fn->getArg(i)
#define GET_ARG_OPERAND_NO(inst, use) (inst->getArgOperandNo(use))
#define CALL_INST_ARG_SIZE(inst) (inst->arg_size())
#else
#define GET_ARG_PT(fn, i) (&*std::next(fn->arg_begin(), i))
#define GET_ARG_OPERAND_NO(inst, use) (use - inst->arg_begin())
#define CALL_INST_ARG_SIZE(inst) (inst->getNumArgOperands())
#endif

#if LLVM_VERSION_MAJOR <= 9
#define GET_ALIGN(alloca) alloca->getAlignment()
#elif LLVM_VERSION_MAJOR == 10
#define GET_ALIGN(alloca) llvm::MaybeAlign(alloca->getAlignment())
#else
#define GET_ALIGN(alloca) alloca->getAlign()
#endif

#if LLVM_VERSION_MAJOR >= 11
#include "llvm/Analysis/ScalarEvolutionExpressions.h"
#endif

#if LLVM_VERSION_MAJOR <= 10
#include <llvm/Analysis/OrderedBasicBlock.h>
#endif
#include "debug_print.hpp"
#include <string>

#define CREATE_FATAL_REPORT(msg) (llvm::Twine(msg) + " (" + __func__ + ":" + llvm::Twine(__LINE__) + ")")
#define REPORT_FATAL_ERROR_WITH_REPORT(msg)         \
   do                                               \
   {                                                \
      report_fatal_error(CREATE_FATAL_REPORT(msg)); \
   } while(false)

#define REPORT_WITH_PRINT(v, msg)          \
   do                                      \
   {                                       \
      llvm::errs() << "[UNEXPECTED] ";     \
      v->print(llvm::errs());              \
      llvm::errs() << "\n";                \
      REPORT_FATAL_ERROR_WITH_REPORT(msg); \
   } while(false)

using namespace llvm;

namespace
{
   bool isCallToLifetimeStartOrEnd(const CallInst* callInst)
   {
      const Function* fn = callInst->getCalledFunction();
      if(!fn)
         return false;

      Intrinsic::ID id = fn->getIntrinsicID();
      return id == Intrinsic::lifetime_start || id == Intrinsic::lifetime_end;
   }

   bool isCallToBambuCsroaPartition(const CallInst* callInst)
   {
      const Function* fn = callInst->getCalledFunction();
      if(!fn)
         return false;

      return fn->getName() == BAMBU_CSROA_PARTITION_FUN_NAME;
   }

   /**
    * @brief Get the size of the dimension of the array type
    *
    * @param t: the array type
    * @param dim: the 0 indexed dimension
    */
   uint64_t getSizeDim(ArrayType* t, uint64_t dim)
   {
      for(; dim > 0; dim--)
      {
         t = cast<ArrayType>(t->getElementType());
      }
      return t->getNumElements();
   }

   /**
    * @brief Get the number of dimensions of the array type
    *
    * @param t: the array type
    */
   uint64_t getNumDimsArray(ArrayType* t)
   {
      uint64_t dims = 0;
      while(t && t->getNumContainedTypes() > 0)
      {
         dims += 1;
         t = dyn_cast<ArrayType>(t->getElementType());
      }
      return dims;
   }

   void printValueDebug(const char* pref, Value* v)
   {
      LLVM_DEBUG({
         dbgs() << pref;
         v->print(dbgs());
         dbgs() << "\n";
      });
   }

   void printValueDebug(const char* pref, StringRef valuePrint)
   {
      LLVM_DEBUG({ dbgs() << pref << valuePrint << "\n"; });
   }

   /**
    * @brief Creates partitions given the infos about the configuration of the final memory in infos.
    *
    * @param typePartInfo
    * @param b: the builder should be placed where the alloca will be created.
    */
   void createPartitions(AllocaPartInfo& allocaPartInfo, IRBuilder<>& b)
   {
      auto* origAlloca = allocaPartInfo.inst;
      uint64_t numPartitions = allocaPartInfo.getNumPartitions();
      ArrayType* partAllocaType = allocaPartInfo.getPartitionedType();

      for(uint64_t i = 0; i < numPartitions; i++)
      {
         auto* alloc = b.CreateAlloca(partAllocaType);
         alloc->setAlignment(GET_ALIGN(origAlloca));
         allocaPartInfo.partitionMap.insert({i, alloc});
      }
   }

   /// Compute the index cell in the partitioned memory
   std::vector<Value*> computeIndexCellInMemory(IRBuilder<>& b, std::vector<Value*>& indices,
                                                const PartitionScheme& partInfos, ArrayType* arrTyPart)
   {
      assert(indices.size() >= partInfos.size());
      std::vector<Value*> idxsGepInst;

      for(uint64_t i = 0; i < partInfos.size(); i++)
      {
         auto info = partInfos[i];
         switch(info.format)
         {
            case COMPLETE:
            {
               auto* idx = ConstantInt::get(b.getInt32Ty(), 0);
               idxsGepInst.push_back(idx);
               break;
            }
            case BLOCK:
            {
               auto* idx = b.CreateURem(indices[i], ConstantInt::get(indices[i]->getType(), getSizeDim(arrTyPart, i)));
               idxsGepInst.push_back(idx);
               break;
            }
            case CYCLIC:
            {
               auto* idx = b.CreateUDiv(indices[i], ConstantInt::get(indices[i]->getType(), info.factor));
               idxsGepInst.push_back(idx);
               break;
            }
            case NONE:
            {
               idxsGepInst.push_back(indices[i]);
               break;
            }
            default:
               break;
         }
      }

      idxsGepInst.insert(idxsGepInst.end(), indices.begin() + idxsGepInst.size(), indices.end());
      return idxsGepInst;
   }

   /// Compute the index cell in the partitioned memory for an alloca
   std::vector<Value*> computeIndexCellInMemoryLeadingZero(IRBuilder<>& b, std::vector<Value*>& indices,
                                                           const PartitionScheme& partInfos, ArrayType* arrTyPart)
   {
      std::vector<Value*> idxsGepInst = computeIndexCellInMemory(b, indices, partInfos, arrTyPart);
      idxsGepInst.insert(idxsGepInst.begin(), b.getInt32(0));
      return idxsGepInst;
   }

   template <typename PartInfoT>
   std::vector<Value*> computeIndexCellInMemory(IRBuilder<>& b, std::vector<Value*>& indices, const PartInfoT& partInfo,
                                                ArrayType* arrTyPart)
   {
      using PartInfoTy = std::remove_cv_t<std::remove_reference_t<PartInfoT>>;
      if constexpr(std::is_same_v<PartInfoTy, ArgPartInfo>)
      {
         return computeIndexCellInMemory(b, indices, partInfo.scheme, arrTyPart);
      }
      else
      {
         return computeIndexCellInMemoryLeadingZero(b, indices, partInfo.scheme, arrTyPart);
      }
   }

   uint64_t getSkipNumber(const PartitionScheme& scheme, const uint64_t level)
   {
      uint64_t skip = 1;
      for(uint64_t i = level; i < scheme.size(); i++)
      {
         auto info = scheme[i];
         switch(info.format)
         {
            case COMPLETE:
            case BLOCK:
            case CYCLIC:
               skip *= info.factor;
               break;
            case NONE:
               break;
            default:
               REPORT_FATAL_ERROR_WITH_REPORT("Incorrect partition format");
         }
      }
      return skip;
   }

   Value* computeIndexMemory(IRBuilder<>& b, std::vector<Value*>& indices, const PartitionScheme& partInfos,
                             ArrayType* partTy)
   {
      auto* intType = cast<IntegerType>(indices[0]->getType());
      Value* returnValue = ConstantInt::get(intType, 0);

      for(uint64_t i = 0; i < partInfos.size(); i++)
      {
         auto info = partInfos[i];
         uint64_t skipNumber = getSkipNumber(partInfos, i + 1);
         auto* constIntSkipNumber = ConstantInt::get(intType, skipNumber);
         switch(info.format)
         {
            case COMPLETE:
            {
               auto* toAdd = b.CreateMul(indices[i], constIntSkipNumber);
               returnValue = b.CreateAdd(returnValue, toAdd);
               break;
            }
            case BLOCK:
            {
               auto* blockIdx = b.CreateUDiv(indices[i], ConstantInt::get(intType, getSizeDim(partTy, i)));
               auto* toAdd = b.CreateMul(blockIdx, constIntSkipNumber);
               returnValue = b.CreateAdd(returnValue, toAdd);
               break;
            }
            case CYCLIC:
            {
               auto* blockIdx = b.CreateURem(indices[i], ConstantInt::get(intType, info.factor));
               auto* toAdd = b.CreateMul(blockIdx, constIntSkipNumber);
               returnValue = b.CreateAdd(returnValue, toAdd);
               break;
            }
            case NONE:
               break;
            default:
               REPORT_FATAL_ERROR_WITH_REPORT("Incorrect partition format");
               break;
         }
      }

      return returnValue;
   }

   uint64_t computeIndexMemory(const std::vector<uint64_t>& indices, const std::vector<size_t>& origDims,
                               const PartitionScheme& scheme)
   {
      uint64_t returnIdx = 0;

      for(size_t i = 0; i < scheme.size(); i++)
      {
         const auto& info = scheme[i];
         uint64_t skipNumber = getSkipNumber(scheme, i + 1);
         switch(info.format)
         {
            case COMPLETE:
               returnIdx += indices[i] * skipNumber;
               break;
            case BLOCK:
               returnIdx += skipNumber * (indices[i] / (origDims[i] / info.factor));
               break;
            case CYCLIC:
               returnIdx += skipNumber * (indices[i] % info.factor);
               break;
            case NONE:
               break;
            default:
               REPORT_FATAL_ERROR_WITH_REPORT("Incorrect partition format");
               break;
         }
      }
      return returnIdx;
   }

   void deleteInstructions(std::vector<Instruction*>& instToDelete)
   {
      for(Instruction* inst : instToDelete)
      {
         if(!inst->use_empty())
         {
            llvm::errs() << "Trying to delete instruction with users: ";
            inst->print(llvm::errs());
            llvm::errs() << "\n";
            for(auto* user : inst->users())
            {
               llvm::errs() << "User: ";
               user->print(llvm::errs());
               llvm::errs() << "\n";
            }
            REPORT_WITH_PRINT(inst, "Trying to delete an instruction that still has uses");
         }
         inst->eraseFromParent();
      }
   }

   bool getArgDataIndices(GetElementPtrInst* originalGepi, const ArgPartInfo& argPartInfo,
                          std::vector<Value*>& indicesGep)
   {
      indicesGep.insert(indicesGep.end(), originalGepi->idx_begin(), originalGepi->idx_end());
      return indicesGep.size() >= argPartInfo.scheme.size();
   }

   /// Create an error basic block
   BasicBlock* createErrBB(Function* f)
   {
      BasicBlock* errBB = BasicBlock::Create(f->getContext(), "", f);
      IRBuilder<> b(errBB);
      b.CreateUnreachable();
      return errBB;
   }

   BasicBlock* getOrCreateErrBB(Function* f, FnPartInfo& fnPartInfo)
   {
      if(fnPartInfo.errBB != nullptr)
      {
         return fnPartInfo.errBB;
      }

      return fnPartInfo.errBB = createErrBB(f);
   }

   ArrayType* getPartitionTypeFromGep(GEPOperator* gep, const std::vector<size_t>& partitionedDims)
   {
      auto* srcTy = gep->getSourceElementType();
      auto* ty = getArrayBaseType(srcTy);
      for(auto dimIt = partitionedDims.rbegin(); dimIt != partitionedDims.rend(); dimIt++)
      {
         ty = ArrayType::get(ty, *dimIt);
      }
      return cast<ArrayType>(ty);
   }

   void manageLoadInstAfterGepArg(LoadInst* loadInst, GetElementPtrInst* originalGepi, const ArgPartInfo& argPartInfo,
                                  BasicBlock* errBB, std::vector<Instruction*>& instToDelete)
   {
      std::vector<Value*> indicesGep;
      if(!getArgDataIndices(originalGepi, argPartInfo, indicesGep))
      {
         REPORT_WITH_PRINT(originalGepi, "Unsupported argument GEP: index depth smaller than partitioning depth");
      }

      IRBuilder<> b(loadInst);
      auto& ctx = originalGepi->getContext();
      uint64_t numPartitions = argPartInfo.getNumPartitions();
      auto* partTy = getPartitionTypeFromGep(cast<GEPOperator>(originalGepi), argPartInfo.getPartitionedDims());
      auto* bbBeforeLoad = loadInst->getParent();
      auto* bbFromLoadOn = loadInst->getParent()->splitBasicBlock(loadInst);
      Type* loadTy = loadInst->getType();

      b.SetInsertPoint(loadInst);
      auto* phiInst = b.CreatePHI(loadTy, numPartitions);

      bbBeforeLoad->getTerminator()->eraseFromParent();
      b.SetInsertPoint(bbBeforeLoad);
      Value* idxMemory = computeIndexMemory(b, indicesGep, argPartInfo.scheme, partTy);
      std::vector<Value*> indicesBank = computeIndexCellInMemory(b, indicesGep, argPartInfo, partTy);
      SwitchInst* switchInst = b.CreateSwitch(idxMemory, errBB, numPartitions);

      for(int64_t i = 0; i < numPartitions; i++)
      {
         auto* caseBB = llvm::BasicBlock::Create(ctx, "", originalGepi->getFunction(), bbFromLoadOn);
         auto* ptr = argPartInfo.partitionMap.at(i);
         b.SetInsertPoint(caseBB);
         // it is needed to do partTy->getElementType() since the arg type is a pointer to array without the first
         // dimension but the partTy is an array with all the dimensions
         auto* gepBank = b.CreateGEP(partTy->getElementType(), ptr, indicesBank);
         auto* newLoad = b.CreateLoad(loadTy, gepBank);
         b.CreateBr(phiInst->getParent());
         phiInst->addIncoming(newLoad, caseBB);
         switchInst->addCase(ConstantInt::get(cast<IntegerType>(idxMemory->getType()), i), caseBB);
      }

      loadInst->replaceAllUsesWith(phiInst);
      instToDelete.push_back(loadInst);
   }

   void manageStoreInstAfterGepArg(StoreInst* storeInst, GetElementPtrInst* originalGepi,
                                   const ArgPartInfo& argPartInfo, BasicBlock* errBB,
                                   std::vector<Instruction*>& instToDelete)
   {
      std::vector<Value*> indicesGep;
      uint64_t numPartitions = argPartInfo.getNumPartitions();
      if(!getArgDataIndices(originalGepi, argPartInfo, indicesGep))
      {
         REPORT_WITH_PRINT(originalGepi, "Unsupported argument GEP: index depth smaller than partitioning depth");
      }

      IRBuilder<> b(storeInst);
      auto& ctx = originalGepi->getContext();
      auto* partTy = getPartitionTypeFromGep(cast<GEPOperator>(originalGepi), argPartInfo.getPartitionedDims());
      auto* bbBeforeStore = storeInst->getParent();
      auto* bbFromStoreOn = storeInst->getParent()->splitBasicBlock(storeInst);

      bbBeforeStore->getTerminator()->eraseFromParent();
      b.SetInsertPoint(bbBeforeStore);
      Value* idxMemory = computeIndexMemory(b, indicesGep, argPartInfo.scheme, partTy);
      std::vector<Value*> indicesBank = computeIndexCellInMemory(b, indicesGep, argPartInfo, partTy);
      SwitchInst* switchInst = b.CreateSwitch(idxMemory, errBB, numPartitions);

      for(int64_t i = 0; i < numPartitions; i++)
      {
         auto* caseBB = llvm::BasicBlock::Create(ctx, "", originalGepi->getFunction(), bbFromStoreOn);
         auto* ptr = argPartInfo.partitionMap.at(i);
         b.SetInsertPoint(caseBB);
         auto* gepBank = b.CreateGEP(partTy->getElementType(), ptr, indicesBank);
         auto* newStore = b.CreateStore(storeInst->getValueOperand(), gepBank);
         b.CreateBr(bbFromStoreOn);
         switchInst->addCase(ConstantInt::get(cast<IntegerType>(idxMemory->getType()), i), caseBB);
      }

      instToDelete.push_back(storeInst);
   }

   bool areGepInstIndicesConstant(GEPOperator* inst)
   {
      return std::all_of(inst->idx_begin(), inst->idx_end(), [](Use& use) { return isa<Constant>(use.get()); });
   }

   template <typename PartInfoT>
   void replaceOriginalGepiWithSpecificGepi(GEPOperator* originalGepi, PartInfoT& partInfo)
   {
      using PartInfoTy = std::remove_cv_t<std::remove_reference_t<PartInfoT>>;

      constexpr bool NeedsLeadingZeroRemoval =
          std::is_same_v<PartInfoTy, AllocaPartInfo> || std::is_same_v<PartInfoTy, GlobalPartInfo>;

      std::vector<Value*> indicesValue(originalGepi->idx_begin(), originalGepi->idx_end());
      std::vector<uint64_t> indicesUInt64;
      indicesUInt64.reserve(indicesValue.size());

      for(Value* v : indicesValue)
         indicesUInt64.push_back(cast<ConstantInt>(v)->getZExtValue());

      if constexpr(NeedsLeadingZeroRemoval)
      {
         assert(!indicesValue.empty() && "Expected at least one GEP index");

         // Optional sanity check
         assert(cast<ConstantInt>(indicesValue.front())->isZero() && "Expected first GEP index to be zero");

         indicesValue.erase(indicesValue.begin());
         indicesUInt64.erase(indicesUInt64.begin());
      }

      auto* partTy = getPartitionTypeFromGep(originalGepi, partInfo.getPartitionedDims());
      uint64_t idx = computeIndexMemory(indicesUInt64, partInfo.getOrigTypeDims(), partInfo.scheme);

      auto createGep = [&](auto& builder, Value* basePtr, ArrayRef<Value*> gepIndices) -> Value* {
         if constexpr(std::is_same_v<PartInfoTy, ArgPartInfo>)
         {
            return builder.CreateGEP(partTy->getElementType(), basePtr, gepIndices);
         }
         else
         {
            return builder.CreateGEP(partTy, basePtr, gepIndices);
         }
      };

      auto createConstExprGep = [&](Constant* basePtr, ArrayRef<Value*> gepIndices) -> Constant* {
         if constexpr(std::is_same_v<PartInfoTy, ArgPartInfo>)
         {
            return ConstantExpr::getGetElementPtr(partTy->getElementType(), basePtr, gepIndices);
         }
         else
         {
            return ConstantExpr::getGetElementPtr(partTy, basePtr, gepIndices);
         }
      };

      if(auto* gepInst = dyn_cast<GetElementPtrInst>(originalGepi))
      {
         IRBuilder<> b(gepInst);
         auto indicesBank = computeIndexCellInMemory(b, indicesValue, partInfo, partTy);
         Value* newGep = createGep(b, partInfo.partitionMap.at(idx), indicesBank);
         originalGepi->replaceAllUsesWith(newGep);
      }
      else if(isa<ConstantExpr>(originalGepi))
      {
         IRBuilder<> b(originalGepi->getContext());
         auto indicesBank = computeIndexCellInMemory(b, indicesValue, partInfo, partTy);
         auto* basePtr = cast<Constant>(partInfo.partitionMap.at(idx));
         Constant* newGep = createConstExprGep(basePtr, indicesBank);
         originalGepi->replaceAllUsesWith(newGep);
      }
   }

   bool allGEPOperatorUsersAreTriviallyReplaceable(GEPOperator* inst)
   {
      return std::all_of(inst->user_begin(), inst->user_end(),
                         [](User* user) { return isa<LoadInst>(user) || isa<StoreInst>(user) || isa<PHINode>(user); });
   }

   /**
    * Check if the call instruction is a call to llvm.lifetime.start or llvm.lifetime.end.
    * They should be the only call instructions to an alloca array object.
    * The instructions will be deleted since the array will be partitioned
    */
   void manageCallInst(CallInst* callInst, std::vector<Instruction*>& instToDelete)
   {
      if(isCallToLifetimeStartOrEnd(callInst) || isCallToBambuCsroaPartition(callInst))
      {
         instToDelete.push_back(callInst);
      }
   }

   void manageGepInstArg(GetElementPtrInst* originalGepi, ArgPartInfo& argPartInfo, BasicBlock* errBB,
                         std::vector<Instruction*>& instToDelete)
   {
      // If the indices are constants just replace it
      auto* gepOperator = cast<GEPOperator>(originalGepi);
      if(areGepInstIndicesConstant(gepOperator) && allGEPOperatorUsersAreTriviallyReplaceable(gepOperator))
      {
         replaceOriginalGepiWithSpecificGepi(gepOperator, argPartInfo);
         return;
      }

      // This is needed since the function splitBasicBlock could invalidate iterator of the for loop
      std::vector<User*> users(originalGepi->user_begin(), originalGepi->user_end());
      for(User* user : users)
      {
         printValueDebug("  USER: ", user);
         if(isa<LoadInst>(user))
         {
            manageLoadInstAfterGepArg(cast<LoadInst>(user), originalGepi, argPartInfo, errBB, instToDelete);
         }
         else if(isa<StoreInst>(user))
         {
            manageStoreInstAfterGepArg(cast<StoreInst>(user), originalGepi, argPartInfo, errBB, instToDelete);
         }
         else if(isa<CallInst>(user))
         {
            manageCallInst(cast<CallInst>(user), instToDelete);
         }
         // else if(isa<PHINode>(user))
         // {
         //    managePHINodeInstAfterGep(cast<PHINode>(user), originalGepi, argPartInfo, errBB, instToDelete);
         // }
         else
         {
            REPORT_WITH_PRINT(
                user, "Only Load/Store/CallInst instructions should be considered when replacing arg GEP users");
         }
      }
   }

   void manageBitcast(BitCastOperator* bitCastOp, std::vector<Instruction*>& instToDelete)
   {
      bool allUserAreCalls =
          std::all_of(bitCastOp->user_begin(), bitCastOp->user_end(), [](User* user) { return isa<CallInst>(user); });

      assert(allUserAreCalls && "Bitcast used by non-call instruction");

      for(User* user : bitCastOp->users())
      {
         if(auto* I = dyn_cast<CallInst>(user))
         {
            if(I->getCalledFunction()->getIntrinsicID() == Intrinsic::lifetime_start ||
               I->getCalledFunction()->getIntrinsicID() == Intrinsic::lifetime_end)
            {
               instToDelete.push_back(I);
            }
         }
      }

      // Only delete if it's actually an instruction
      if(auto* I = dyn_cast<Instruction>(bitCastOp))
         instToDelete.push_back(I);
   }

   void manageLoadInstToFirstElement(LoadInst* loadInst, Value* firstMemoryBank,
                                     std::vector<Instruction*>& instToDelete)
   {
      IRBuilder<> b(loadInst);
      auto* newLoad = b.CreateLoad(loadInst->getType(), firstMemoryBank);
      loadInst->replaceAllUsesWith(newLoad);
      instToDelete.push_back(loadInst);
   }

   void manageStoreInstToFirstElement(StoreInst* storeInst, Value* firstMemoryBank,
                                      std::vector<Instruction*>& instToDelete)
   {
      IRBuilder<> b(storeInst);
      auto* newLoad = b.CreateStore(storeInst->getValueOperand(), firstMemoryBank);
      storeInst->replaceAllUsesWith(newLoad);
      instToDelete.push_back(storeInst);
   }

   void applyArrayPartitionOnSingleArgument(ArgPartInfo& argPartInfo, BasicBlock* errBB)
   {
      LLVM_DEBUG({ dbgs() << "[CSROA] Partitioning the arg: " << argPartInfo.to_string() << "\n"; });
      std::vector<Instruction*> instToDelete;
      for(auto* user : argPartInfo.arg->users())
      {
         printValueDebug("  USER: ", user);
         if(auto* originalGepi = dyn_cast<GetElementPtrInst>(user))
         {
            manageGepInstArg(originalGepi, argPartInfo, errBB, instToDelete);
         }
         else if(auto* bitCastInst = dyn_cast<BitCastInst>(user))
         {
            manageBitcast(cast<BitCastOperator>(bitCastInst), instToDelete);
         }
         else if(auto* loadInst = dyn_cast<LoadInst>(user))
         {
            manageLoadInstToFirstElement(loadInst, argPartInfo.partitionMap.at(0), instToDelete);
         }
         else if(auto* storeInst = dyn_cast<StoreInst>(user))
         {
            manageStoreInstToFirstElement(storeInst, argPartInfo.partitionMap.at(0), instToDelete);
         }
         else if(auto* callInst = dyn_cast<CallInst>(user))
         {
            manageCallInst(callInst, instToDelete);
         }
         else
         {
            REPORT_WITH_PRINT(
                user, "Only GetElementPtr/Bitcast/CallInst instructions should be considered when replacing arguments");
         }
      }
      LLVM_DEBUG(dbgs() << "[CSROA] Finishing processing users\n");
      deleteInstructions(instToDelete);
      LLVM_DEBUG(dbgs() << "[CSROA] Finished partitioning the arg: " << argPartInfo.to_string() << "\n");
   }

   bool getAllocaDataIndices(GetElementPtrInst* originalGepi, const AllocaPartInfo& allocaPartInfo,
                             std::vector<Value*>& indicesGep)
   {
      if(originalGepi->getNumIndices() == 0)
      {
         return false;
      }

      indicesGep.insert(indicesGep.end(), originalGepi->idx_begin() + 1, originalGepi->idx_end());
      return indicesGep.size() >= allocaPartInfo.scheme.size();
   }

   bool getDataIndicesGlobal(GEPOperator* originalGepOp, const GlobalPartInfo& globalPartInfo,
                             std::vector<Value*>& indicesGep)
   {
      if(originalGepOp->getNumIndices() == 0)
      {
         return false;
      }

      indicesGep.insert(indicesGep.end(), originalGepOp->idx_begin() + 1, originalGepOp->idx_end());
      return indicesGep.size() >= globalPartInfo.scheme.size();
   }

   void manageLoadInstAlloc(LoadInst* loadInst, GetElementPtrInst* originalGepi, const AllocaPartInfo& allocaPartInfo,
                            BasicBlock* errBB, std::vector<Instruction*>& instToDelete)
   {
      std::vector<Value*> indicesGep;
      uint64_t numPartitions = allocaPartInfo.getNumPartitions();
      if(!getAllocaDataIndices(originalGepi, allocaPartInfo, indicesGep))
      {
         REPORT_WITH_PRINT(originalGepi, "Unsupported alloca GEP: index depth smaller than partitioning depth");
      }

      IRBuilder<> b(loadInst);
      auto& ctx = loadInst->getContext();
      auto* partTy = getPartitionTypeFromGep(cast<GEPOperator>(originalGepi), allocaPartInfo.getPartitionedDims());
      auto* bbBeforeLoad = loadInst->getParent();
      auto* bbFromLoadOn = loadInst->getParent()->splitBasicBlock(loadInst);
      Type* loadTy = loadInst->getType();

      b.SetInsertPoint(loadInst);
      auto* phiInst = b.CreatePHI(loadTy, numPartitions);

      bbBeforeLoad->getTerminator()->eraseFromParent();
      b.SetInsertPoint(bbBeforeLoad);
      Value* idxMemory = computeIndexMemory(b, indicesGep, allocaPartInfo.scheme, partTy);
      std::vector<Value*> indicesBank = computeIndexCellInMemory(b, indicesGep, allocaPartInfo, partTy);
      SwitchInst* switchInst = b.CreateSwitch(idxMemory, errBB, numPartitions);

      for(int64_t i = 0; i < numPartitions; i++)
      {
         auto* caseBB = BasicBlock::Create(ctx, "", loadInst->getFunction(), bbFromLoadOn);
         auto* ptr = allocaPartInfo.partitionMap.at(i);
         b.SetInsertPoint(caseBB);
         auto* gepBank = b.CreateGEP(partTy, ptr, indicesBank);
         auto* newLoad = b.CreateLoad(loadTy, gepBank);
         b.CreateBr(phiInst->getParent());
         phiInst->addIncoming(newLoad, caseBB);
         switchInst->addCase(ConstantInt::get(cast<IntegerType>(idxMemory->getType()), i), caseBB);
      }

      loadInst->replaceAllUsesWith(phiInst);
      instToDelete.push_back(loadInst);
   }

   void manageStoreInstAlloc(StoreInst* storeInst, GetElementPtrInst* originalGepi,
                             const AllocaPartInfo& allocaPartInfo, BasicBlock* errBB,
                             std::vector<Instruction*>& instToDelete)
   {
      std::vector<Value*> indicesGep;
      uint64_t numPartitions = allocaPartInfo.getNumPartitions();
      if(!getAllocaDataIndices(originalGepi, allocaPartInfo, indicesGep))
      {
         REPORT_WITH_PRINT(originalGepi, "Unsupported alloca GEP: index depth smaller than partitioning depth");
      }

      IRBuilder<> b(storeInst);
      auto& ctx = originalGepi->getContext();
      auto* partTy = getPartitionTypeFromGep(cast<GEPOperator>(originalGepi), allocaPartInfo.getPartitionedDims());
      auto* bbBeforeStore = storeInst->getParent();
      auto* bbFromStoreOn = storeInst->getParent()->splitBasicBlock(storeInst);

      bbBeforeStore->getTerminator()->eraseFromParent();
      b.SetInsertPoint(bbBeforeStore);
      Value* idxMemory = computeIndexMemory(b, indicesGep, allocaPartInfo.scheme, partTy);
      std::vector<Value*> indicesBank = computeIndexCellInMemory(b, indicesGep, allocaPartInfo, partTy);
      SwitchInst* switchInst = b.CreateSwitch(idxMemory, errBB, numPartitions);

      for(int64_t i = 0; i < numPartitions; i++)
      {
         auto* caseBB = BasicBlock::Create(ctx, "", originalGepi->getFunction(), bbFromStoreOn);
         auto* ptr = allocaPartInfo.partitionMap.at(i);
         b.SetInsertPoint(caseBB);
         auto* gepBank = b.CreateGEP(partTy, ptr, indicesBank);
         auto* newStore = b.CreateStore(storeInst->getValueOperand(), gepBank);
         b.CreateBr(bbFromStoreOn);
         switchInst->addCase(ConstantInt::get(cast<IntegerType>(idxMemory->getType()), i), caseBB);
      }

      instToDelete.push_back(storeInst);
   }

   void manageGepInstAlloc(GetElementPtrInst* originalGepi, const AllocaPartInfo& allocaPartInfo, BasicBlock* errBB,
                           std::vector<Instruction*>& instToDelete)
   {
      // If the indices are constants just replace it
      auto* gepOperator = cast<GEPOperator>(originalGepi);
      if(areGepInstIndicesConstant(gepOperator) && allGEPOperatorUsersAreTriviallyReplaceable(gepOperator))
      {
         replaceOriginalGepiWithSpecificGepi(gepOperator, allocaPartInfo);
         return;
      }

      // This is needed since the function splitBasicBlock could invalidate iterator of the for loop
      std::vector<User*> users(originalGepi->user_begin(), originalGepi->user_end());
      for(User* user : users)
      {
         printValueDebug("  USER: ", user);
         if(isa<LoadInst>(user))
         {
            manageLoadInstAlloc(cast<LoadInst>(user), originalGepi, allocaPartInfo, errBB, instToDelete);
         }
         else if(isa<StoreInst>(user))
         {
            manageStoreInstAlloc(cast<StoreInst>(user), originalGepi, allocaPartInfo, errBB, instToDelete);
         }
         else if(isa<CallInst>(user))
         {
            manageCallInst(cast<CallInst>(user), instToDelete);
         }
         else
         {
            REPORT_WITH_PRINT(user,
                              "Only Load/Store/Call instructions should be considered when replacing alloca GEP users");
         }
      }
   }

   bool isFirstElementGep(GEPOperator* gep)
   {
      return std::all_of(gep->idx_begin(), gep->idx_end(), [](Value* v) {
         auto* ci = dyn_cast<ConstantInt>(v);
         return ci != nullptr && ci->isZero();
      });
   }

   void applyArrayPartitionOnAlloc(AllocaPartInfo& info, BasicBlock* errBB)
   {
      LLVM_DEBUG({ llvm::dbgs() << "[CSROA] Partitioning the alloca: " << info.to_string() << "\n"; });
      uint64_t numPartitions = info.getNumPartitions();
      IRBuilder<> b(info.inst);
      createPartitions(info, b);
      std::vector<Instruction*> instToDelete;
      for(User* user : info.inst->users())
      {
         printValueDebug("  USER: ", user);
         if(auto* gepInst = dyn_cast<GetElementPtrInst>(user))
         {
            manageGepInstAlloc(gepInst, info, errBB, instToDelete);
         }
         else if(auto* bitCastInst = dyn_cast<BitCastInst>(user))
         {
            manageBitcast(cast<BitCastOperator>(bitCastInst), instToDelete);
         }
         else if(auto* loadInst = dyn_cast<LoadInst>(user))
         {
            manageLoadInstToFirstElement(loadInst, info.partitionMap.at(0), instToDelete);
         }
         else if(auto* storeInst = dyn_cast<StoreInst>(user))
         {
            manageStoreInstToFirstElement(storeInst, info.partitionMap.at(0), instToDelete);
         }
         else if(auto* callInst = dyn_cast<CallInst>(user))
         {
            manageCallInst(callInst, instToDelete);
         }
         else
         {
            REPORT_WITH_PRINT(
                user,
                "Only GEP/BitCast/Load/Store/Call instructions should be considered when replacing alloca GEP users");
         }
      }

      deleteInstructions(instToDelete);
   }

   void applyArrayPartitionOnAllocas(FnPartInfo& fnPartInfo, BasicBlock* errBB)
   {
      for(auto& allocaPartInfo : fnPartInfo.allocs)
      {
         applyArrayPartitionOnAlloc(allocaPartInfo, errBB);
      }
   }

   /// Recursively traverses a constant initializer and distributes its elements
   /// across multiple partitions based on the TypePartInfo mapping.
   ///
   /// @param initializer The constant to partition (may be scalar or aggregate)
   /// @param indices Current position in the nested array structure
   /// @param partInitializers Output vector where each partition's constants are collected
   /// @param typePartInfo Mapping information that determines which partition each element belongs to
   void createPartitionInitializers(Constant* initializer, std::vector<size_t>& indices,
                                    std::vector<std::vector<Constant*>>& partInitializers,
                                    const GlobalPartInfo& globPartInfo)
   {
      if(isa<ConstantInt>(initializer) || isa<ConstantFP>(initializer) || isa<ConstantStruct>(initializer) ||
         isa<UndefValue>(initializer) || isa<ConstantAggregateZero>(initializer)
#if LLVM_VERSION_MAJOR >= 12
         || isa<PoisonValue>(initializer)
#endif
      )
      {
         size_t idxMem = computeIndexMemory(indices, globPartInfo.getOrigTypeDims(), globPartInfo.scheme);

         if(idxMem >= partInitializers.size())
         {
            REPORT_WITH_PRINT(initializer, "Partition index " + Twine(idxMem) +
                                               " out of bounds (max: " + Twine(partInitializers.size()) + ")");
         }

         partInitializers[idxMem].push_back(initializer);
         return;
      }

      if(auto* constArr = dyn_cast<ConstantArray>(initializer))
      {
         for(size_t i = 0; i < constArr->getNumOperands(); i++)
         {
            indices.push_back(i);
            createPartitionInitializers(constArr->getOperand(i), indices, partInitializers, globPartInfo);
            indices.pop_back();
         }
         return;
      }

      if(auto* dataArray = dyn_cast<ConstantDataArray>(initializer))
      {
         for(size_t i = 0; i < dataArray->getNumElements(); i++)
         {
            indices.push_back(i);
            createPartitionInitializers(dataArray->getElementAsConstant(i), indices, partInitializers, globPartInfo);
            indices.pop_back();
         }
         return;
      }

      REPORT_WITH_PRINT(initializer, "Unsupported constant type in createPartitionInitializers. "
                                     "Expected ConstantInt, ConstantFP, ConstantStruct, ConstantArray, "
                                     "ConstantDataArray, UndefValue, or PoisonValue");
   }

   /// Creates partition-specific initializers for a global array.
   /// Splits a monolithic array initializer into multiple smaller initializers,
   /// one per partition, based on the partitioning scheme defined in typePartInfo.
   ///
   /// @param numPartitions Number of partitions to create
   /// @param partitionTy Type of each partition array
   /// @param typePartInfo Partitioning scheme that maps array elements to partitions
   /// @param initializer Original global array initializer
   /// @return Vector of constant initializers, one per partition
   std::vector<Constant*> createGlobalPartitionInitializers(uint64_t numPartitions, ArrayType* partitionTy,
                                                            const GlobalPartInfo& info, Constant* initializer)
   {
      if(isa<ConstantAggregateZero>(initializer))
      {
         Constant* zeroInit = ConstantAggregateZero::get(partitionTy);
         return std::vector<Constant*>(numPartitions, zeroInit);
      }

      // ConstantArray are global arrays that contain other arrays like
      // int glob[2][3] = {{1, 2, 3}, {4, 5, 6}}
      if(isa<ConstantArray>(initializer) || isa<ConstantDataArray>(initializer))
      {
         std::vector<size_t> indices;
         std::vector<std::vector<Constant*>> partInitializers(numPartitions);

         createPartitionInitializers(initializer, indices, partInitializers, info);

         std::vector<Constant*> ret(numPartitions);
         for(size_t i = 0; i < numPartitions; i++)
         {
            ret[i] = ConstantArray::get(partitionTy, partInitializers[i]);
         }

         return ret;
      }

      REPORT_WITH_PRINT(initializer, "Initializer should be only zero initialized or constant arrays");
   }

   void createGlobalVarPartitions(GlobalPartInfo& info)
   {
      const auto& var = info.var;
      uint64_t numPartitions = info.getNumPartitions();
      ArrayType* partGlobTy = info.getPartitionedType();
      info.partitionMap.clear();

      std::vector<Constant*> initializers =
          createGlobalPartitionInitializers(numPartitions, partGlobTy, info, info.var->getInitializer());
      for(uint64_t i = 0; i < numPartitions; i++)
      {
         auto* globalVar = new GlobalVariable(*(var->getParent()), partGlobTy, info.var->isConstant(),
                                              info.var->getLinkage(), initializers[i]);
         info.partitionMap.insert({i, globalVar});
         LLVM_DEBUG(dbgs() << "[CSROA] Global variable created "; globalVar->print(dbgs()); dbgs() << "\n");
      }
   }

   void manageLoadInstGlobal(LoadInst* loadInst, GEPOperator* gepOp, const GlobalPartInfo& info)
   {
      std::vector<Value*> indicesGep;

      if(!getDataIndicesGlobal(gepOp, info, indicesGep))
      {
         REPORT_WITH_PRINT(gepOp, "Unsupported global variable GEP: index depth smaller than partitioning depth");
      }

      IRBuilder<> b(loadInst);
      auto& ctx = loadInst->getContext();
      uint64_t numPartitions = info.getNumPartitions();
      auto* partTy = getPartitionTypeFromGep(gepOp, info.getPartitionedDims());
      auto* bbBeforeLoad = loadInst->getParent();
      auto* bbFromLoadOn = loadInst->getParent()->splitBasicBlock(loadInst);
      Type* loadTy = loadInst->getType();

      b.SetInsertPoint(loadInst);
      auto* phiInst = b.CreatePHI(loadTy, numPartitions);

      bbBeforeLoad->getTerminator()->eraseFromParent();
      b.SetInsertPoint(bbBeforeLoad);
      Value* idxMemory = computeIndexMemory(b, indicesGep, info.scheme, partTy);
      std::vector<Value*> indicesBank = computeIndexCellInMemory(b, indicesGep, info, partTy);
      SwitchInst* switchInst = b.CreateSwitch(idxMemory, createErrBB(loadInst->getFunction()), numPartitions);

      for(int64_t i = 0; i < numPartitions; i++)
      {
         auto* caseBB = BasicBlock::Create(ctx, "", loadInst->getFunction(), bbFromLoadOn);
         b.SetInsertPoint(caseBB);
         auto* gepBank =
             isa<GetElementPtrInst>(gepOp) ?
                 b.CreateGEP(partTy, info.partitionMap.at(i), indicesBank) :
                 ConstantExpr::getGetElementPtr(partTy, cast<Constant>(info.partitionMap.at(i)), indicesBank);
         auto* newLoad = b.CreateLoad(loadTy, gepBank);
         b.CreateBr(phiInst->getParent());
         phiInst->addIncoming(newLoad, caseBB);
         switchInst->addCase(ConstantInt::get(cast<IntegerType>(idxMemory->getType()), i), caseBB);
      }

      loadInst->replaceAllUsesWith(phiInst);
   }

   void manageStoreInstGlobal(StoreInst* storeInst, GEPOperator* gepOp, const GlobalPartInfo& info,
                              std::vector<Instruction*>& instToDelete)
   {
      std::vector<Value*> indicesGep;

      if(!getDataIndicesGlobal(gepOp, info, indicesGep))
      {
         REPORT_WITH_PRINT(gepOp, "Unsupported global variable GEP: index depth smaller than partitioning depth");
      }

      IRBuilder<> b(storeInst);
      uint64_t numPartitions = info.getNumPartitions();
      auto& ctx = storeInst->getContext();
      auto* partTy = getPartitionTypeFromGep(gepOp, info.getPartitionedDims());
      auto* bbBeforeStore = storeInst->getParent();
      auto* bbFromStoreOn = storeInst->getParent()->splitBasicBlock(storeInst);

      bbBeforeStore->getTerminator()->eraseFromParent();
      b.SetInsertPoint(bbBeforeStore);
      Value* idxMemory = computeIndexMemory(b, indicesGep, info.scheme, partTy);
      std::vector<Value*> indicesBank = computeIndexCellInMemory(b, indicesGep, info, partTy);
      SwitchInst* switchInst = b.CreateSwitch(idxMemory, createErrBB(storeInst->getFunction()), numPartitions);

      for(int64_t i = 0; i < numPartitions; i++)
      {
         auto* caseBB = BasicBlock::Create(ctx, "", storeInst->getFunction(), bbFromStoreOn);
         b.SetInsertPoint(caseBB);
         auto* gepBank =
             isa<GetElementPtrInst>(gepOp) ?
                 b.CreateGEP(partTy, info.partitionMap.at(i), indicesBank) :
                 ConstantExpr::getGetElementPtr(partTy, cast<Constant>(info.partitionMap.at(i)), indicesBank);
         b.CreateStore(storeInst->getValueOperand(), gepBank);
         b.CreateBr(bbFromStoreOn);
         switchInst->addCase(ConstantInt::get(cast<IntegerType>(idxMemory->getType()), i), caseBB);
      }

      instToDelete.push_back(storeInst);
   }

   void manageGepOpGlobal(GEPOperator* gepOp, const GlobalPartInfo& info, std::vector<Instruction*>& instToDelete)
   {
      // If the indices are constants just replace it
      if(areGepInstIndicesConstant(gepOp) && allGEPOperatorUsersAreTriviallyReplaceable(gepOp))
      {
         replaceOriginalGepiWithSpecificGepi(gepOp, info);
         return;
      }

      std::vector<User*> users(gepOp->user_begin(), gepOp->user_end());
      for(User* user : users)
      {
         printValueDebug("  USER: ", user);
         if(isa<LoadInst>(user))
         {
            manageLoadInstGlobal(cast<LoadInst>(user), gepOp, info);
         }
         else if(isa<StoreInst>(user))
         {
            manageStoreInstGlobal(cast<StoreInst>(user), gepOp, info, instToDelete);
         }
         else if(isa<CallInst>(user))
         {
            manageCallInst(cast<CallInst>(user), instToDelete);
         }
         else
         {
            REPORT_WITH_PRINT(user,
                              "Only Load/Store/Call instructions should be considered when replacing global GEP users");
         }
      }
   }

   /// Creates the partitioned global variable and replaces all GEP instructions that
   /// access the original global variable with GEP instructions that access the correct partition.
   void applyArrayPartitioningOnGlobalVar(GlobalPartInfo& info)
   {
      LLVM_DEBUG({ dbgs() << "[CSROA] Partitioning the global variable: " << info.to_string() << "\n"; });

      IRBuilder<> b(info.var->getContext());
      std::vector<Instruction*> instToDelete;
      ArrayType* partTy = info.getPartitionedType();
      createGlobalVarPartitions(info);

      std::vector<User*> users(info.var->user_begin(), info.var->user_end());
      for(auto* user : users)
      {
         printValueDebug("  USER: ", user);
         if(auto* gepOp = dyn_cast<GEPOperator>(user))
         {
            manageGepOpGlobal(gepOp, info, instToDelete);
         }
         else if(auto* bitCast = dyn_cast<BitCastOperator>(user))
         {
            manageBitcast(bitCast, instToDelete);
         }
         else if(auto* loadInst = dyn_cast<LoadInst>(user))
         {
            manageLoadInstToFirstElement(loadInst, info.partitionMap.at(0), instToDelete);
         }
         else if(auto* storeInst = dyn_cast<StoreInst>(user))
         {
            manageStoreInstToFirstElement(storeInst, info.partitionMap.at(0), instToDelete);
         }
         else if(auto* callInst = dyn_cast<CallInst>(user))
         {
            manageCallInst(callInst, instToDelete);
         }
         else
         {
            REPORT_WITH_PRINT(user, "Only GetElementPtr/Bitcast/Call/Load/Store instructions or GEP constant "
                                    "expressions should be considered for partitioned globals");
         }
      }

      deleteInstructions(instToDelete);
      LLVM_DEBUG({ dbgs() << "[CSROA] Finished to partitioning the variable\n"; });
   }

   /**
    * @brief Check if the argument is in the ArgPartInfo vector and if it has at least one type of partitions
    */
   template <typename IterT, typename ArgsContainerT>
   bool isArgPartitioned(const IterT& it, const ArgsContainerT& argsPartitionInfos)
   {
      return it != argsPartitionInfos.end() &&
             llvm::any_of(it->scheme, [](const PartInfo& p) { return p.format != PartInfoFormat::NONE; });
   }

   void applyArrayPartitionOnArguments(FnPartInfo& fnPartInfo, BasicBlock* errBB)
   {
      for(auto& argPartInfo : fnPartInfo.args)
      {
         applyArrayPartitionOnSingleArgument(argPartInfo, errBB);
      }
   }

   /// The baseType is expected to be in the form of type*, like float* or int*
   std::string getOriginalType(std::string& baseType, const ArgPartInfo* argPartInfo)
   {
      // In this way, it builds something like float (*) or int (*)
      std::string originalTy = baseType.substr(0, baseType.find('*')) + " (*)";
      const auto& originalTypeDims = argPartInfo->getOrigTypeDims();

      for(size_t i = 1; i < originalTypeDims.size(); i++)
      {
         const auto& partInfo = argPartInfo->scheme[i];
         switch(partInfo.format)
         {
            case COMPLETE:
               break;
            case BLOCK:
            case CYCLIC:
               originalTy += "[" + std::to_string(originalTypeDims[i] / partInfo.factor) + "]";
               break;
            case NONE:
               originalTy += "[" + std::to_string(originalTypeDims[i]) + "]";
               break;
            default:
               REPORT_FATAL_ERROR_WITH_REPORT("Incorrect partition format");
         }
      }

      return originalTy;
   }

   void createBundleEntryPartition(pugi::xml_node& bundlesNode, const pugi::xml_node& origBundleNode,
                                   const std::string& name, const std::string& includes)
   {
      auto newBundle = bundlesNode.append_child("bundle");

      std::string oldMode = origBundleNode.attribute("mode").as_string();
      newBundle.append_attribute("mode").set_value(
          includes.find("hls_stream.h") != std::string::npos ? "fifo" : oldMode.c_str());
      newBundle.append_attribute("name").set_value(name.c_str());
   }

   bool isCompletelyPartitioned(const ArgPartInfo& argPartInfo)
   {
      return !argPartInfo.scheme.empty() && llvm::all_of(argPartInfo.scheme, [](const PartInfo& partInfo) {
         return partInfo.format == PartInfoFormat::COMPLETE;
      });
   }

   bool isScalarPartitionInterfaceMode(const std::string& mode)
   {
      return mode == "none" || mode == "ptrdefault" || mode == "handshake" || mode == "valid" || mode == "ovalid" ||
             mode == "acknowledge";
   }

   void createParameterEntryPartition(pugi::xml_node& paramsNode, std::string& name, uint64_t elemCount,
                                      std::string& includes, uint64_t index, std::string& original_type,
                                      uint64_t sizeInBytes, std::string& type)
   {
      auto newParamNode = paramsNode.append_child("parameter");

      newParamNode.append_attribute("bundle").set_value(name.c_str());
      newParamNode.append_attribute("elem_count").set_value(elemCount);
      newParamNode.append_attribute("includes").set_value(includes.c_str());
      newParamNode.append_attribute("index").set_value(index);
      newParamNode.append_attribute("original_typename").set_value(original_type.c_str());
      newParamNode.append_attribute("port").set_value(name.c_str());
      newParamNode.append_attribute("size_in_bytes").set_value(sizeInBytes);
      newParamNode.append_attribute("typename").set_value(type.c_str());
   }

   template <typename ArgContainerT>
   void createFnPartNode(pugi::xml_node& origFnNode, pugi::xml_node& fnXml, StringRef topFnName, ArgContainerT& objs)
   {
      fnXml.append_attribute("csroa").set_value(true);
      fnXml.append_attribute("name").set_value(topFnName.str().c_str());
      fnXml.append_attribute("symbol").set_value(topFnName.str().c_str());
      for(const auto& attr : origFnNode.attributes())
      {
         const auto attrName = attr.name();
         if(strcmp(attrName, "name") == 0 || strcmp(attrName, "symbol") == 0 || strcmp(attrName, "original") == 0)
         {
            continue;
         }
         fnXml.append_attribute(attrName).set_value(attr.value());
      }

      auto origBundles = origFnNode.child("bundles");
      auto origParms = origFnNode.child("parameters");

      auto bundles = fnXml.append_child("bundles");
      auto parms = fnXml.append_child("parameters");

      uint64_t idxParmCsroa = 0;
      for(uint64_t idxOrig = 0;; idxOrig++)
      {
         auto parmNodeCsroa = origParms.find_child_by_attribute("parameter", "index", std::to_string(idxOrig).c_str());
         auto bundleNodeCsroa =
             origBundles.find_child_by_attribute("bundle", "name", parmNodeCsroa.attribute("bundle").as_string());
         if(!parmNodeCsroa || !bundleNodeCsroa)
         {
            break;
         }

         std::string bundle = parmNodeCsroa.attribute("bundle").as_string();
         uint64_t storedElemCount = parmNodeCsroa.attribute("elem_count").as_ullong();
         std::string includes = parmNodeCsroa.attribute("includes").as_string();
         std::string storedOriginalType = parmNodeCsroa.attribute("original_typename").as_string();
         uint64_t storedSizeInBytes = parmNodeCsroa.attribute("size_in_bytes").as_ullong();
         std::string type = parmNodeCsroa.attribute("typename").as_string();

         auto obj = llvm::find_if(objs, [&](const ArgPartInfo& a) { return a.argName == bundle; });
         if(obj != objs.end())
         {
            const std::string bundleMode = bundleNodeCsroa.attribute("mode").as_string();
            if(isScalarPartitionInterfaceMode(bundleMode) && !isCompletelyPartitioned(*obj))
            {
               report_fatal_error(llvm::Twine("Interface mode '") + bundleMode + "' on array-partitioned parameter '" +
                                  bundle + "' is supported only with complete partitioning of all array dimensions");
            }
            uint64_t elemCount = storedElemCount / obj->getNumPartitions();
            std::string originalType = getOriginalType(type, &(*obj));

            for(uint64_t numPartition = 0; numPartition < obj->getNumPartitions(); numPartition++)
            {
               std::string namePartition = bundle + "_" + std::to_string(numPartition);
               createBundleEntryPartition(bundles, bundleNodeCsroa, namePartition, includes);
               createParameterEntryPartition(parms, namePartition, elemCount, includes, idxParmCsroa, originalType,
                                             storedSizeInBytes / storedElemCount * elemCount, type);
               idxParmCsroa += 1;
            }
         }
         else
         {
            auto copy = parms.append_copy(parmNodeCsroa);
            copy.attribute("index").set_value(idxParmCsroa);
            copy.remove_attribute("array_dims");
            bundles.append_copy(bundleNodeCsroa);
            idxParmCsroa += 1;
         }
      }
   }

   bool isFnToBePartitioned(const FnPartInfo& fnPartInfo)
   {
      const auto& args = fnPartInfo.args;
      return !args.empty() && std::any_of(args.begin(), args.end(), [](const ArgPartInfo& argPartInfo) {
         const auto& scheme = argPartInfo.scheme;
         return std::any_of(scheme.begin(), scheme.end(),
                            [](const PartInfo& partInfo) { return partInfo.format != NONE; });
      });
   }

   Value* getUnderlyingObjectCompat(Value* v, const DataLayout& DL)
   {
#if LLVM_VERSION_MAJOR >= 12
      (void)DL;
      return getUnderlyingObject(v);
#else
      return GetUnderlyingObject(v, DL);
#endif
   }

   bool isValuePartitioned(Value* v, ArrPartCtx& arrPartCtx, const std::string& fnName)
   {
      if(auto* arg = dyn_cast<Argument>(v))
      {
         bool callerPartitioned =
             fnName.find("partitioned_", 0) == 0 && arrPartCtx.fnTable.count(fnName.substr(strlen("partitioned_")));
         bool callerHasAllocaPartitioned = arrPartCtx.fnTable.count(fnName);
         if(!callerPartitioned && !callerHasAllocaPartitioned)
         {
            return false;
         }
         auto& fnPartInfo = callerPartitioned ?
                                getFnInfoOrDie(arrPartCtx.fnTable, fnName.substr(strlen("partitioned_"))) :
                                getFnInfoOrDie(arrPartCtx.fnTable, fnName);
         return std::any_of(fnPartInfo.args.begin(), fnPartInfo.args.end(),
                            [&arg](ArgPartInfo& partArg) { return arg == partArg.arg; });
      }
      if(auto* allocaInst = dyn_cast<AllocaInst>(v))
      {
         bool callerPartitioned =
             fnName.find("partitioned_", 0) == 0 && arrPartCtx.fnTable.count(fnName.substr(strlen("partitioned_")));
         bool callerHasAllocaPartitioned = arrPartCtx.fnTable.count(fnName);
         if(!callerPartitioned && !callerHasAllocaPartitioned)
         {
            return false;
         }
         auto& fnPartInfo = callerPartitioned ?
                                getFnInfoOrDie(arrPartCtx.fnTable, fnName.substr(strlen("partitioned_"))) :
                                getFnInfoOrDie(arrPartCtx.fnTable, fnName);
         return std::any_of(fnPartInfo.allocs.begin(), fnPartInfo.allocs.end(),
                            [&allocaInst](AllocaPartInfo& allocaPart) { return allocaInst == allocaPart.inst; });
      }
      if(auto* globVar = dyn_cast<GlobalVariable>(v))
      {
         return std::any_of(arrPartCtx.globalVars.begin(), arrPartCtx.globalVars.end(),
                            [&globVar](GlobalPartInfo& globPart) { return globVar == globPart.var; });
      }
      return false;
   }

   bool isValuePartitioned(Value* v, ArrPartCtx& arrPartCtx, FnPartInfo& fnPartInfo)
   {
      if(auto* arg = dyn_cast<Argument>(v))
      {
         return std::any_of(fnPartInfo.args.begin(), fnPartInfo.args.end(),
                            [&arg](ArgPartInfo& partArg) { return arg == partArg.arg; });
      }
      if(auto* allocaInst = dyn_cast<AllocaInst>(v))
      {
         return std::any_of(fnPartInfo.allocs.begin(), fnPartInfo.allocs.end(),
                            [&allocaInst](AllocaPartInfo& allocaPart) { return allocaInst == allocaPart.inst; });
      }
      if(auto* globVar = dyn_cast<GlobalVariable>(v))
      {
         return std::any_of(arrPartCtx.globalVars.begin(), arrPartCtx.globalVars.end(),
                            [&globVar](GlobalPartInfo& globPart) { return globVar == globPart.var; });
      }
      return false;
   }

   bool atLeastOneArgPartitionedOfCallInst(ArrPartCtx& arrPartCtx, CallInst* callInst)
   {
      const DataLayout& DL = callInst->getFunction()->getParent()->getDataLayout();
      std::string fnName = callInst->getFunction()->getName().str();
      for(auto* argIt = callInst->arg_begin(); argIt != callInst->arg_end(); argIt++)
      {
         Value* obj = getUnderlyingObjectCompat(*argIt, DL);
         if(isValuePartitioned(obj, arrPartCtx, fnName))
         {
            return true;
         }
      }
      return false;
   }

   void getCallInstsToUpdate(ArrPartCtx& arrPartCtx, Function* fn, std::set<CallInst*>& callInstsToUpdate)
   {
      for(auto& bb : *fn)
      {
         for(auto& inst : bb)
         {
            if(auto* callInst = dyn_cast<CallInst>(&inst))
            {
               if(atLeastOneArgPartitionedOfCallInst(arrPartCtx, callInst))
               {
                  callInstsToUpdate.insert(callInst);
               }
            }
         }
      }
   }

   // Helper generico: dato il PartInfo già trovato, popola gli operands
   template <typename PartInfoT>
   void pushAllPartitions(std::vector<Value*>& operands, const PartInfoT& partInfo)
   {
      for(uint64_t i = 0; i < partInfo.getNumPartitions(); ++i)
         operands.push_back(partInfo.partitionMap.at(i));
   }

   template <typename PartInfoT>
   void pushFirstPartition(std::vector<Value*>& operands, const PartInfoT& partInfo)
   {
      operands.push_back(partInfo.partitionMap.at(0));
   }

   template <typename PartInfoT>
   void pushAllPartitionsWithGep(IRBuilder<>& b, std::vector<Value*>& operands, const PartInfoT& partInfo,
                                 GEPOperator* gepi)
   {
      auto* partTy = getPartitionTypeFromGep(gepi, partInfo.getPartitionedDims());
      std::vector<Value*> gepIndices(gepi->getNumIndices(), b.getInt32(0));

      for(uint64_t i = 0; i < partInfo.getNumPartitions(); ++i)
      {
         Value* partVal = partInfo.partitionMap.at(i);
         Value* elem = isa<GetElementPtrInst>(gepi) ?
                           b.CreateGEP(partTy, partVal, gepIndices) :
                           ConstantExpr::getGetElementPtr(partTy, cast<Constant>(partVal), gepIndices);
         operands.push_back(elem);
      }
   }

   template <typename PartInfoT>
   void pushSinglePartitionWithGep(IRBuilder<>& b, std::vector<Value*>& operands, const PartInfoT& partInfo,
                                   GetElementPtrInst* gepi)
   {
      assert(areGepInstIndicesConstant(cast<GEPOperator>(gepi)) && "NYI: non-constant indices");

      std::vector<Value*> indicesValue(gepi->idx_begin(), gepi->idx_end());
      std::vector<uint64_t> indicesUInt64;
      indicesUInt64.reserve(indicesValue.size());
      for(Value* v : indicesValue)
         indicesUInt64.push_back(cast<ConstantInt>(v)->getZExtValue());

      auto* partTy = getPartitionTypeFromGep(cast<GEPOperator>(gepi), partInfo.getPartitionedDims());
      uint64_t idx = computeIndexMemory(indicesUInt64, partInfo.getOrigTypeDims(), partInfo.scheme);
      auto indicesBank = computeIndexCellInMemory(b, indicesValue, partInfo, partTy);

      operands.push_back(b.CreateGEP(partTy->getElementType(), partInfo.partitionMap.at(idx), indicesBank));
   }

   template <typename TValue, typename TContainer>
   auto resolvePartInfo(TValue* val, TContainer& container)
   {
      auto it = findPartInfoInContainer(val, container);
      assert(it != container.end());
      return *it;
   }

   void replaceArgWithPartitionedArgInCalls(Argument* arg, FnPartInfo& callerFnPartInfo, std::vector<Value*>& operands)
   {
      printValueDebug("[CSROA] Processing argument operand (replacing single argument with all partitions): ", arg);
      pushAllPartitions(operands, resolvePartInfo(arg, callerFnPartInfo.args));
   }

   void replaceArgWithPartitionedArgInCallsWithGepiInBetween(IRBuilder<>& b, Argument* arg, GetElementPtrInst* gepi,
                                                             FnPartInfo& callerFnPartInfo,
                                                             std::vector<Value*>& operands)
   {
      printValueDebug("[CSROA] Processing argument operand (replacing single argument with all partitions through gep "
                      "inst): ",
                      arg);
      pushAllPartitionsWithGep(b, operands, resolvePartInfo(arg, callerFnPartInfo.args), cast<GEPOperator>(gepi));
   }

   void replaceArgWithPartitionedSingleArgInCall(Argument* arg, GetElementPtrInst* gepi, FnPartInfo& callerFnPartInfo,
                                                 std::vector<Value*>& operands)
   {
      printValueDebug("[CSROA] Processing argument operand (replacing single argument with a single partition): ", arg);
      if(areGepInstIndicesConstant(cast<GEPOperator>(gepi)))
      {
         IRBuilder<> b(gepi);
         pushSinglePartitionWithGep(b, operands, resolvePartInfo(arg, callerFnPartInfo.args), gepi);
         return;
      }

      assert(0 && "NYI");
   }

   void replaceArgWithPartitionedFirstArgInCall(Argument* arg, FnPartInfo& callerFnPartInfo,
                                                std::vector<Value*>& operands)
   {
      printValueDebug("[CSROA] Processing argument operand (replacing single argument with partition 0): ", arg);
      pushFirstPartition(operands, resolvePartInfo(arg, callerFnPartInfo.args));
   }

   void replaceAllocaWithPartitionedAllocaInCalls(AllocaInst* allocaInst, FnPartInfo& callerFnPartInfo,
                                                  std::vector<Value*>& operands)
   {
      printValueDebug("[CSROA] Processing alloca operand: ", allocaInst);
      pushAllPartitions(operands, resolvePartInfo(allocaInst, callerFnPartInfo.allocs));
   }

   void replaceAllocaWithPartitionedAllocaInCallsWithGepiInBetween(IRBuilder<>& b, AllocaInst* allocaInst,
                                                                   GetElementPtrInst* gepi,
                                                                   FnPartInfo& callerFnPartInfo,
                                                                   std::vector<Value*>& operands)
   {
      printValueDebug("[CSROA] Processing alloca operand: ", allocaInst);
      pushAllPartitionsWithGep(b, operands, resolvePartInfo(allocaInst, callerFnPartInfo.allocs),
                               cast<GEPOperator>(gepi));
   }

   void replaceAllocaWithPartitionedSingleAllocInCall(AllocaInst* allocaInst, GetElementPtrInst* gepi,
                                                      FnPartInfo& callerFnPartInfo, std::vector<Value*>& operands)
   {
      printValueDebug("[CSROA] Processing argument operand (replacing single argument with a single partition): ",
                      allocaInst);

      if(areGepInstIndicesConstant(cast<GEPOperator>(gepi)))
      {
         IRBuilder<> b(gepi);
         pushSinglePartitionWithGep(b, operands, resolvePartInfo(allocaInst, callerFnPartInfo.allocs), gepi);
         return;
      }

      assert(0 && "NYI");
   }

   void replaceAllocaWithPartitionedFirstAllocInCall(AllocaInst* allocaInst, FnPartInfo& callerFnPartInfo,
                                                     std::vector<Value*>& operands)
   {
      printValueDebug("[CSROA] Processing alloca operand (replacing single operand with partition 0): ", allocaInst);
      pushFirstPartition(operands, resolvePartInfo(allocaInst, callerFnPartInfo.allocs));
   }

   void replaceGlobVarWithPartitionedGlobVarInCalls(GlobalVariable* globalVar, ArrPartCtx& arrPartCtx,
                                                    std::vector<Value*>& operands)
   {
      printValueDebug("[CSROA] Processing global variable operand: ", globalVar->getName());
      pushAllPartitions(operands, resolvePartInfo(globalVar, arrPartCtx.globalVars));
   }

   void replaceGlobVarWithPartitionedGlobVarInCallsWithGepiInBetween(IRBuilder<>& b, GlobalVariable* globalVar,
                                                                     GEPOperator* gepi, ArrPartCtx& arrPartCtx,
                                                                     std::vector<Value*>& operands)
   {
      printValueDebug("[CSROA] Processing global variable operand: ", globalVar->getName());
      pushAllPartitionsWithGep(b, operands, resolvePartInfo(globalVar, arrPartCtx.globalVars), gepi);
   }

   void replaceGlobVarWithPartitionedFirstGlobVarInCall(GlobalVariable* globalVar, ArrPartCtx& arrPartCtx,
                                                        std::vector<Value*>& operands)
   {
      printValueDebug("[CSROA] Processing global variable operand (replacing single operand with partition 0): ",
                      globalVar->getName());
      pushFirstPartition(operands, resolvePartInfo(globalVar, arrPartCtx.globalVars));
   }

   void replaceGlobVarWithPartitionedGlobVarInCallsWithBitCastInBetween(IRBuilder<>& b, GlobalVariable* globalVar,
                                                                        BitCastOperator* bitCastOp,
                                                                        ArrPartCtx& arrPartCtx,
                                                                        std::vector<Value*>& operands)
   {
      printValueDebug("[CSROA] Processing global variable operand: ", globalVar->getName());

      auto globalPartInfoIt = findPartInfoInContainer(globalVar, arrPartCtx.globalVars);
      assert(globalPartInfoIt != arrPartCtx.globalVars.end());
      auto* castTy = bitCastOp->getDestTy();

      for(uint64_t numPartition = 0; numPartition < globalPartInfoIt->getNumPartitions(); numPartition++)
      {
         auto* partGlobVar = globalPartInfoIt->partitionMap.at(numPartition);
         auto* bitCastPart = ConstantExpr::getBitCast(cast<Constant>(partGlobVar), castTy);
         operands.push_back(bitCastPart);
      }
   }

   void fixupSingleCallInstruction(ArrPartCtx& arrPartCtx, CallInst* callInst, FnPartInfo& callerFnPartInfo,
                                   std::vector<Instruction*>& instToDelete)
   {
      LLVM_DEBUG({
         dbgs() << "[CSROA] Processing call instruction: ";
         callInst->print(dbgs());
         dbgs() << "\n";
      });

      auto* calledFn = callInst->getCalledFunction();
      if(calledFn == nullptr)
      {
         REPORT_WITH_PRINT(callInst, "Indirect calls are not supported while fixing up calls with partitioned args");
      }
      auto fnPartitionedIt = arrPartCtx.fnTable.find(calledFn->getName());
      const bool calleeHasPartitionedVersion =
          fnPartitionedIt != arrPartCtx.fnTable.end() && fnPartitionedIt->second.fnPartitioned != nullptr;
      const auto& DL = arrPartCtx.topFn->getParent()->getDataLayout();

      std::vector<Value*> operands;
      for(size_t i = 0; i < CALL_INST_ARG_SIZE(callInst); i++)
      {
         Value* operand = callInst->getArgOperand(i);

         if(!isValuePartitioned(getUnderlyingObjectCompat(operand, DL), arrPartCtx, callerFnPartInfo))
         {
            LLVM_DEBUG({
               dbgs() << "[CSROA] Processing other operand: ";
               operand->print(dbgs());
               dbgs() << "\n";
            });
            operands.push_back(operand);
            continue;
         }

         if(auto* arg = dyn_cast<Argument>(operand))
         {
            if(calleeHasPartitionedVersion)
            {
               replaceArgWithPartitionedArgInCalls(arg, callerFnPartInfo, operands);
            }
            else
            {
               replaceArgWithPartitionedFirstArgInCall(arg, callerFnPartInfo, operands);
            }
         }
         else if(isa<GetElementPtrInst>(operand) &&
                 isa<Argument>(cast<GetElementPtrInst>(operand)->getPointerOperand()))
         {
            IRBuilder<> b(callInst);
            auto* gepInst = cast<GetElementPtrInst>(operand);
            auto* arg = cast<Argument>(gepInst->getPointerOperand());
            std::vector<Value*> indices(gepInst->idx_begin(), gepInst->idx_end());
            auto* ty = GetElementPtrInst::getIndexedType(gepInst->getSourceElementType(), indices);

            if(calleeHasPartitionedVersion)
            {
               // Callee expects partitions: expand all partitions regardless of GEP type,
               // since both &arr[0] and &arr[i] mean the callee wants the full array.
               replaceArgWithPartitionedArgInCallsWithGepiInBetween(b, arg, gepInst, callerFnPartInfo, operands);
            }
            else if(!ty->isArrayTy())
            {
               replaceArgWithPartitionedSingleArgInCall(arg, gepInst, callerFnPartInfo, operands);
            }
            else
            {
               // Non-partitioned callee and array type GEP or partitioned-callee and non-array type: illegal, the
               // callee would see a non-contiguous view of the partitioned array.
               REPORT_WITH_PRINT(gepInst, "Unsupported: whole-array GEP passed to a non-partitioned callee");
            }
         }
         else if(auto* allocaInst = dyn_cast<AllocaInst>(operand))
         {
            if(calleeHasPartitionedVersion)
            {
               replaceAllocaWithPartitionedAllocaInCalls(allocaInst, callerFnPartInfo, operands);
            }
            else
            {
               replaceAllocaWithPartitionedFirstAllocInCall(allocaInst, callerFnPartInfo, operands);
            }
         }
         else if(isa<GetElementPtrInst>(operand) &&
                 isa<AllocaInst>(cast<GetElementPtrInst>(operand)->getPointerOperand()))
         {
            auto* gepInst = cast<GetElementPtrInst>(operand);
            auto* allocaInst = cast<AllocaInst>(gepInst->getPointerOperand());
            IRBuilder<> b(callInst);
            std::vector<Value*> indices(gepInst->idx_begin(), gepInst->idx_end());
            auto* ty = GetElementPtrInst::getIndexedType(gepInst->getSourceElementType(), indices);

            if(calleeHasPartitionedVersion)
            {
               // Callee expects partitions: expand all partitions regardless of GEP type,
               // since both &arr[0] and &arr[i] mean the callee wants the full array.
               replaceAllocaWithPartitionedAllocaInCallsWithGepiInBetween(b, allocaInst, gepInst, callerFnPartInfo,
                                                                          operands);
            }
            else if(!ty->isArrayTy())
            {
               replaceAllocaWithPartitionedSingleAllocInCall(allocaInst, gepInst, callerFnPartInfo, operands);
            }
            else
            {
               // Non-partitioned callee and array type GEP or partitioned-callee and non-array type: illegal, the
               // callee would see a non-contiguous view of the partitioned array.
               REPORT_WITH_PRINT(gepInst, "Unsupported: whole-array GEP passed to a non-partitioned callee");
            }
         }
         else if(auto* globalVar = dyn_cast<GlobalVariable>(operand))
         {
            if(calleeHasPartitionedVersion)
            {
               replaceGlobVarWithPartitionedGlobVarInCalls(globalVar, arrPartCtx, operands);
            }
            else
            {
               replaceGlobVarWithPartitionedFirstGlobVarInCall(globalVar, arrPartCtx, operands);
            }
         }
         else if(auto* constExpr = dyn_cast<ConstantExpr>(operand))
         {
            unsigned int opCode = constExpr->getOpcode();
            if(opCode == Instruction::GetElementPtr)
            {
               auto* gepOperator = cast<GEPOperator>(operand);
               auto* globalVar = cast<GlobalVariable>(gepOperator->getPointerOperand());
               if(!isFirstElementGep(gepOperator))
               {
                  REPORT_WITH_PRINT(
                      gepOperator,
                      "Unsupported alloca GEP call operand while fixing up calls: only first-element GEP is "
                      "supported");
               }
               IRBuilder<> b(callInst);
               if(calleeHasPartitionedVersion)
               {
                  replaceGlobVarWithPartitionedGlobVarInCallsWithGepiInBetween(b, globalVar, gepOperator, arrPartCtx,
                                                                               operands);
               }
               else
               {
                  replaceGlobVarWithPartitionedFirstGlobVarInCall(globalVar, arrPartCtx, operands);
               }
            }
            else if(opCode == Instruction::BitCast)
            {
               auto* bitCastOp = cast<BitCastOperator>(operand);
               auto* globalVar = cast<GlobalVariable>(bitCastOp->stripPointerCasts());
               IRBuilder<> b(callInst);
               if(calleeHasPartitionedVersion)
               {
                  replaceGlobVarWithPartitionedGlobVarInCallsWithBitCastInBetween(b, globalVar, bitCastOp, arrPartCtx,
                                                                                  operands);
               }
               else
               {
                  replaceGlobVarWithPartitionedFirstGlobVarInCall(globalVar, arrPartCtx, operands);
               }
            }
            else
            {
               REPORT_WITH_PRINT(constExpr, "Unsupported operation in fixup of call instructions");
            }
         }
         else
         {
            llvm_unreachable("This cannot be reached");
         }
      }

      IRBuilder<> b(callInst);
      CallInst* newCall;
      if(calleeHasPartitionedVersion)
      {
         // The callee has a partitioned version: call it with the expanded operands.
         newCall = b.CreateCall(fnPartitionedIt->second.fnPartitioned, operands);
      }
      else
      {
         // No partitioned version exists: call the original function.
         newCall = b.CreateCall(calledFn, operands);
      }
      newCall->setTailCallKind(callInst->getTailCallKind());
      if(callInst->getCalledFunction()->getName().contains("read_bambu_internal") ||
         callInst->getCalledFunction()->getName().contains("write_bambu_internal"))
      {
         newCall->setAttributes(callInst->getAttributes());
      }
      callInst->replaceAllUsesWith(newCall);
      instToDelete.push_back(callInst);
   }

   void fixupCallInstructions(ArrPartCtx& arrPartCtx, std::set<CallInst*>& callInstsToUpdate,
                              FnPartInfo& callerFnPartInfo)
   {
      LLVM_DEBUG(dbgs() << "[CSROA] Number of calls to update: " << callInstsToUpdate.size() << "\n");
      std::vector<Instruction*> instToDelete;
      for(auto* callInst : callInstsToUpdate)
      {
         fixupSingleCallInstruction(arrPartCtx, callInst, callerFnPartInfo, instToDelete);
      }

      deleteInstructions(instToDelete);
   }

   void fixupCallsWithPartitionedValues(ArrPartCtx& arrPartCtx, Function* fnToSearchCallInsts,
                                        FnPartInfo& callerFnPartInfo)
   {
      LLVM_DEBUG(dbgs() << "[CSROA] Fixing up calls with partitioned arguments\n");
      std::set<CallInst*> callInstsToUpdate;
      getCallInstsToUpdate(arrPartCtx, fnToSearchCallInsts, callInstsToUpdate);
      fixupCallInstructions(arrPartCtx, callInstsToUpdate, callerFnPartInfo);
      LLVM_DEBUG(dbgs() << "[CSROA] Finished fixing up calls with partitioned arguments\n");
   }

   void arrPartCloneFunctionWithDupl(FnPartInfo& fnPartInfo)
   {
      auto* fn = fnPartInfo.fnOriginal;
      Module& M = *fn->getParent();
      StringRef fnName = fn->getName();
      auto& argsPartitionInfos = fnPartInfo.args;
      ValueToValueMapTy vMap;
      std::vector<Type*> newFnArgTypes;
      LLVM_DEBUG(llvm::dbgs() << "[CSROA] Modifying the " << fnName << " function\n");

      // Validate that non-partitioned pointer arguments have noalias
      for(const auto& arg : fn->args())
      {
         if(!arg.getType()->isPointerTy())
         {
            continue;
         }
         auto argPartInfoIt = findPartInfoInContainer(&arg, argsPartitionInfos);
         if(!isArgPartitioned(argPartInfoIt, argsPartitionInfos) && !arg.hasAttribute(llvm::Attribute::NoAlias))
         {
            errs() << "[CSROA] Non-partitioned pointer argument '" << arg.getName() << "' (idx " << arg.getArgNo()
                   << ") of function '" << fnName << "' does not have noalias attribute\n";
            REPORT_FATAL_ERROR_WITH_REPORT(
                "Non-partitioned pointer argument must have noalias attribute for array partitioning");
         }
      }

      for(const auto& arg : fn->args())
      {
         auto argPartInfoIt = findPartInfoInContainer(&arg, argsPartitionInfos);
         newFnArgTypes.insert(newFnArgTypes.begin() + arg.getArgNo(), arg.getType());

         if(isArgPartitioned(argPartInfoIt, argsPartitionInfos))
         {
            uint64_t numPartitions = argPartInfoIt->getNumPartitions();
#if PANDA_LLVM_CLANG_MAJOR < 16
            // The "cast<ArrayType>(argPartInfoIt->getPartitionedType())->getArrayElementType()->getPointerTo()" is done
            // for this reason: An array partition type is stored like this [4 x [3 x i32]] But in LLVM the arguments
            // types are *[3 x i32]
            auto* ty = cast<ArrayType>(argPartInfoIt->getPartitionedType())->getElementType()->getPointerTo();
#elif PANDA_LLVM_CLANG_MAJOR == 16
            auto* ty = argPartInfoIt->getPartitionedType()->isOpaquePointerTy() ?
                           argPartInfoIt->getPartitionedType() :
                           cast<ArrayType>(argPartInfoIt->getPartitionedType())->getElementType()->getPointerTo();
#else
            auto* ty = argPartInfoIt->getPartitionedType();
#endif
            LLVM_DEBUG({
               dbgs() << "[CSROA] Argument with idx " << arg.getArgNo() << " is partitioned into " << numPartitions
                      << " partitions. Adding " << numPartitions << " arguments of type ";
               ty->print(dbgs());
               dbgs() << "\n";
            });
            newFnArgTypes.insert(newFnArgTypes.end(), numPartitions, ty);
         }
         else
         {
            LLVM_DEBUG({
               dbgs() << "[CSROA] Argument with idx " << arg.getArgNo() << " is not partitioned. Keeping the same "
                      << "argument of type ";
               arg.getType()->print(dbgs());
               dbgs() << "\n";
            });
            newFnArgTypes.push_back(arg.getType());
         }
      }

      auto* fnPartitionedTy = FunctionType::get(fn->getReturnType(), newFnArgTypes, fn->isVarArg());
      auto* fnPartitionedWithDupl =
          Function::Create(fnPartitionedTy, fn->getLinkage(), "partitioned_" + fnPartInfo.name, &M);
      fnPartitionedWithDupl->setCallingConv(fn->getCallingConv());

      // Map the arguments
      for(const auto& arg : fn->args())
      {
         vMap[&arg] = GET_ARG_PT(fnPartitionedWithDupl, arg.getArgNo());
      }

      // Map the basic blocks
      for(BasicBlock& BB : *fn)
      {
         BasicBlock* newBB = BasicBlock::Create(M.getContext(), BB.getName(), fnPartitionedWithDupl);
         vMap[&BB] = newBB;
      }

      // Map the instructions
      for(BasicBlock& bb : *fn)
      {
         auto* newBB = cast<BasicBlock>(vMap[&bb]);
         IRBuilder<> b(newBB);
         b.SetInsertPoint(newBB, newBB->end());

         for(auto& inst : bb)
         {
            Instruction* newInst = inst.clone();
            RemapInstruction(newInst, vMap, RF_IgnoreMissingLocals | RF_NoModuleLevelChanges);
            b.Insert(newInst);
            vMap[&inst] = newInst;
         }
      }

      // Fixup broken phi
      for(BasicBlock& bb : *fnPartitionedWithDupl)
      {
         for(Instruction& inst : bb)
         {
            RemapInstruction(&inst, vMap, RF_IgnoreMissingLocals | RF_NoModuleLevelChanges);
         }
      }

      // Map the partitions
      for(uint64_t itOrig = 0, itPart = fn->arg_size(); itPart < fnPartitionedWithDupl->arg_size(); itOrig++)
      {
         auto* arg = GET_ARG_PT(fn, itOrig);
         auto argPartInfoIt = findPartInfoInContainer(arg, argsPartitionInfos);
         if(isArgPartitioned(argPartInfoIt, argsPartitionInfos))
         {
            for(uint64_t numPartition = 0; numPartition < argPartInfoIt->getNumPartitions(); numPartition++)
            {
               argPartInfoIt->partitionMap.insert({numPartition, GET_ARG_PT(fnPartitionedWithDupl, itPart)});
               itPart++;
            }
         }
         else
         {
            LLVM_DEBUG({
               dbgs() << "[CSROA] Argument with idx " << itOrig
                      << " is not partitioned. Replace all the uses with the new argument with idx " << itPart << "\n";
            });
            GET_ARG_PT(fnPartitionedWithDupl, itOrig)->replaceAllUsesWith(GET_ARG_PT(fnPartitionedWithDupl, itPart));
            itPart++;
         }
      }

      fnPartInfo.remap(vMap);
      fnPartInfo.fnPartitionedWithDupl = fnPartitionedWithDupl;
   }

   void cleanUnusedInst(Function* fn)
   {
      std::vector<Instruction*> deadInsts;
      do
      {
         deadInsts.clear();
         for(auto& BB : *fn)
         {
            for(auto& inst : BB)
            {
               if(isInstructionTriviallyDead(&inst))
               {
                  deadInsts.push_back(&inst);
               }
            }
         }

         deleteInstructions(deadInsts);
      } while(!deadInsts.empty());
   }

   void arrPartCloneFunctionFinal(FnPartInfo& fnPartInfo)
   {
      auto* fnPartitionedWithDupl = fnPartInfo.fnPartitionedWithDupl;
      Module& M = *fnPartitionedWithDupl->getParent();
      std::vector<Type*> newFnArgTypes;

      cleanUnusedInst(fnPartitionedWithDupl);

      // Precondition: the original arguments that have been partitioned should not have any uses at this point since
      // all their uses should have been replaced with the new partitioned arguments in the "fnPartitionedWithDupl"
      // function
      for(size_t i = 0; i < fnPartInfo.fnOriginal->arg_size(); i++)
      {
         auto* arg = GET_ARG_PT(fnPartitionedWithDupl, i);
         if(!arg->use_empty())
         {
            errs() << "Original argument with idx " << i << " should not have any uses at this point. Uses:\n";
            for(auto* user : arg->users())
            {
               user->print(errs());
               errs() << "\n";
            }
            REPORT_FATAL_ERROR_WITH_REPORT("Original argument should not have any uses at this point");
         }
      }

      for(size_t i = fnPartInfo.fnOriginal->arg_size(); i < fnPartitionedWithDupl->arg_size(); i++)
      {
         auto* arg = GET_ARG_PT(fnPartitionedWithDupl, i);
         newFnArgTypes.push_back(arg->getType());
      }

      auto* fnPartitionedTy =
          FunctionType::get(fnPartitionedWithDupl->getReturnType(), newFnArgTypes, fnPartitionedWithDupl->isVarArg());
      auto* fnPartitioned =
          Function::Create(fnPartitionedTy, fnPartitionedWithDupl->getLinkage(), "partitioned_" + fnPartInfo.name, &M);
      fnPartitioned->setCallingConv(fnPartitionedWithDupl->getCallingConv());

      // Mark all pointer arguments as noalias: each partition points to a distinct memory region
      for(auto& arg : fnPartitioned->args())
      {
         if(arg.getType()->isPointerTy())
         {
            arg.addAttr(llvm::Attribute::NoAlias);
         }
      }

      ValueToValueMapTy vMap;

      // Map the arguments
      for(size_t i = fnPartInfo.fnOriginal->arg_size(); i < fnPartitionedWithDupl->arg_size(); i++)
      {
         auto* argDupl = GET_ARG_PT(fnPartitionedWithDupl, i);
         auto* argFinal = GET_ARG_PT(fnPartitioned, i - fnPartInfo.fnOriginal->arg_size());
         vMap[argDupl] = argFinal;
      }

      for(size_t i = 0; i < fnPartInfo.fnOriginal->arg_size(); i++)
      {
         auto* argOriginal = GET_ARG_PT(fnPartInfo.fnOriginal, i);
         auto argPartInfoIt = findPartInfoInContainer(argOriginal, fnPartInfo.args);
         if(!isArgPartitioned(argPartInfoIt, fnPartInfo.args))
         {
            continue;
         }

         auto firstPartitionIt = argPartInfoIt->partitionMap.find(0);
         if(firstPartitionIt == argPartInfoIt->partitionMap.end())
         {
            REPORT_FATAL_ERROR_WITH_REPORT("Missing partition 0 while remapping original partitioned arguments");
         }

         auto* firstPartitionArgWithDupl = dyn_cast<Argument>(firstPartitionIt->second);
         if(firstPartitionArgWithDupl == nullptr)
         {
            REPORT_FATAL_ERROR_WITH_REPORT("Expected partitioned argument mapped to a function argument");
         }

         auto mappedFirstPartitionIt = vMap.find(firstPartitionArgWithDupl);
         if(mappedFirstPartitionIt == vMap.end())
         {
            REPORT_FATAL_ERROR_WITH_REPORT("Missing remapping for partition 0 argument in final cloned function");
         }

         auto* argWithDupl = GET_ARG_PT(fnPartitionedWithDupl, i);
         vMap[argWithDupl] = mappedFirstPartitionIt->second;
      }

      // Map the basic blocks
      for(BasicBlock& BB : *fnPartitionedWithDupl)
      {
         BasicBlock* newBB = BasicBlock::Create(M.getContext(), BB.getName(), fnPartitioned);
         vMap[&BB] = newBB;
      }

      // Map the instructions
      for(BasicBlock& bb : *fnPartitionedWithDupl)
      {
         auto* newBB = cast<BasicBlock>(vMap[&bb]);
         IRBuilder<> b(newBB);
         b.SetInsertPoint(newBB, newBB->end());

         for(auto& inst : bb)
         {
            Instruction* newInst = inst.clone();
            RemapInstruction(newInst, vMap, RF_IgnoreMissingLocals | RF_NoModuleLevelChanges);
            b.Insert(newInst);
            vMap[&inst] = newInst;
         }
      }

      // Fixup broken phi
      for(BasicBlock& bb : *fnPartitioned)
      {
         for(Instruction& inst : bb)
         {
            RemapInstruction(&inst, vMap, RF_IgnoreMissingLocals | RF_NoModuleLevelChanges);
         }
      }

      fnPartInfo.fnPartitioned = fnPartitioned;
   }

   void applyArrayPartition(Module& M, ArrPartCtx& arrPartCtx, std::vector<std::string>& invTopSortFns)
   {
      LLVM_DEBUG({
         dbgs() << "[CSROA] Processing function order\n";
         for(size_t i = 0; i < invTopSortFns.size(); i++)
         {
            dbgs() << "[CSROA] " << i << ") " << invTopSortFns[i] << "\n";
         }
      });

      for(auto& globalPartInfo : arrPartCtx.globalVars)
      {
         applyArrayPartitioningOnGlobalVar(globalPartInfo);
      }

      for(const auto& fnName : invTopSortFns)
      {
         Function* fn = M.getFunction(fnName);
         assert(fn != nullptr);
         if(!arrPartCtx.fnTable.count(fnName))
         {
            LLVM_DEBUG(dbgs() << "[CSROA] The function " << fnName
                              << " will not be partitioned since it does not have any argument to be partitioned, but "
                                 "it could have call to function partitioned\n");
            auto emptyFnInfo = FnPartInfo(fn);
            fixupCallsWithPartitionedValues(arrPartCtx, fn, emptyFnInfo);
            continue;
         }

         auto& fnPartInfo = getFnInfoOrDie(arrPartCtx.fnTable, fnName);
         LLVM_DEBUG(dbgs() << "\n[CSROA] Processing the function " << fnName << "\n");

         if(isFnToBePartitioned(fnPartInfo))
         {
            LLVM_DEBUG(dbgs() << "[CSROA] The function " << fnName
                              << " will be partitioned since it has at least one argument to be partitioned\n");
            arrPartCloneFunctionWithDupl(fnPartInfo);
            assert(!verifyFunction(*fnPartInfo.fnPartitionedWithDupl, &errs()) &&
                   "Cloned function with duplicates is invalid");

            applyArrayPartitionOnArguments(fnPartInfo, getOrCreateErrBB(fnPartInfo.fnPartitionedWithDupl, fnPartInfo));
            assert(!verifyFunction(*fnPartInfo.fnPartitionedWithDupl, &errs()) &&
                   "Cloned function with duplicates is invalid after argument partitioning");

            applyArrayPartitionOnAllocas(fnPartInfo, getOrCreateErrBB(fnPartInfo.fnPartitionedWithDupl, fnPartInfo));
            assert(!verifyFunction(*fnPartInfo.fnPartitionedWithDupl, &errs()) &&
                   "Cloned function with duplicates is invalid after alloca partitioning");

            fixupCallsWithPartitionedValues(arrPartCtx, fnPartInfo.fnPartitionedWithDupl, fnPartInfo);
            assert(!verifyFunction(*fnPartInfo.fnPartitionedWithDupl, &errs()) &&
                   "Cloned function with duplicates is invalid after call inst fixup");

            arrPartCloneFunctionFinal(fnPartInfo);
            assert(!verifyFunction(*fnPartInfo.fnPartitioned, &errs()) && "Cloned function final is invalid");
            LLVM_DEBUG(dbgs() << "[CSROA] Created the partitioned function named "
                              << fnPartInfo.fnPartitioned->getName() << " with " << fnPartInfo.fnPartitioned->arg_size()
                              << " arguments\n");

            if(arrPartCtx.topFn == fn)
            {
               arrPartCtx.topFn = fnPartInfo.fnPartitioned;
            }
         }
         else
         {
            LLVM_DEBUG(dbgs() << "[CSROA] The function " << fnName
                              << " will not be partitioned since it does not have any argument to be partitioned\n");

            applyArrayPartitionOnAllocas(fnPartInfo, getOrCreateErrBB(fn, fnPartInfo));
            LLVM_DEBUG(dbgs() << "[CSROA] Applied array partitioning on allocas of function " << fnName << "\n");
            assert(!verifyFunction(*fnPartInfo.fnOriginal, &errs()) &&
                   "Function is invalid after partitioning only allocas");

            fixupCallsWithPartitionedValues(arrPartCtx, fnPartInfo.fnOriginal, fnPartInfo);
            assert(!verifyFunction(*fnPartInfo.fnOriginal, &errs()) && "Function is invalid after call inst fixup");
         }
      }

      for(auto& item : arrPartCtx.fnTable)
      {
         auto fnName = item.getKey();
         auto& value = item.getValue();
         if(value.fnPartitioned != nullptr)
         {
            value.fnOriginal->setName("old_" + fnName);
            value.fnPartitioned->setName(fnName);
         }
      }
   }

   void deleteAllUsersOfArrPartFunction(Module& M)
   {
      Function* builtinArrPartFun = M.getFunction(BAMBU_CSROA_PARTITION_FUN_NAME);
      std::vector<Instruction*> instToDelete;

      for(auto* U : builtinArrPartFun->users())
      {
         if(auto* i = dyn_cast<Instruction>(U))
         {
            instToDelete.push_back(i);
         }
      }

      deleteInstructions(instToDelete);
      builtinArrPartFun->eraseFromParent();
   }

   void modifyXMLModule(ArrPartCtx& arrPartCtx, std::string& outdirName)
   {
      LLVM_DEBUG(llvm::dbgs() << "[CSROA] Modify architecture.xml file\n");
      auto& doc = *arrPartCtx.doc;
      auto xmlModule = doc.child("module");
      std::vector<pugi::xml_node> functions(xmlModule.begin(), xmlModule.end());

      for(auto& f : functions)
      {
         const std::string symbol = f.attribute("symbol").as_string();
         if(!arrPartCtx.fnTable.count(symbol) || getFnInfoOrDie(arrPartCtx.fnTable, symbol).args.empty())
         {
            continue;
         }

         auto& fnInfo = getFnInfoOrDie(arrPartCtx.fnTable, symbol);
         pugi::xml_node& origFnNode = f;
         if(symbol == arrPartCtx.topFn->getName())
         {
            LLVM_DEBUG(llvm::dbgs() << "[CSROA] Update top function " << symbol << "\n");
            std::string nameFnOriginal = "original_" + std::string(symbol);
            origFnNode.attribute("name").set_value(nameFnOriginal.c_str());
            origFnNode.attribute("symbol").set_value(nameFnOriginal.c_str());
            origFnNode.append_attribute("original").set_value(true);
         }

         auto fnPartNode = xmlModule.append_child("function");
         createFnPartNode(origFnNode, fnPartNode, symbol, fnInfo.args);

         for(auto& parm : origFnNode.child("parameters").children("parameter"))
         {
            std::string parmName = parm.attribute("bundle").as_string();

            auto argPartInfoIt = llvm::find_if(
                fnInfo.args, [&](const ArgPartInfo& argPartInfo) { return argPartInfo.argName == parmName; });
            if(argPartInfoIt == fnInfo.args.end())
            {
               continue;
            }

            std::string arrayPartitionTypesValue;
            std::string arrayPartitionFactorsValue;
            for(size_t i = 0; i < argPartInfoIt->scheme.size(); i++)
            {
               if(i > 0)
               {
                  arrayPartitionTypesValue += ",";
                  arrayPartitionFactorsValue += ",";
               }
               const auto& partInfo = argPartInfoIt->scheme[i];
               arrayPartitionTypesValue += format_to_string(partInfo.format).str();
               arrayPartitionFactorsValue += std::to_string(partInfo.factor);
            }
            parm.append_attribute("array_partition_types").set_value(arrayPartitionTypesValue.c_str());
            parm.append_attribute("array_partition_factors").set_value(arrayPartitionFactorsValue.c_str());
         }
      }

      LLVM_DEBUG(dbgs() << "[CSROA] Saving the new architecture.xml\n");
      const auto arch_filename = outdirName + "/architecture.xml";
      doc.save_file(arch_filename.c_str(), "  ", pugi::format_indent | pugi::format_no_empty_element_tags);
   }

   void cleanIR(Module& M, ArrPartCtx& arrPartCtx, const std::vector<std::string>& invTopSortFns)
   {
      LLVM_DEBUG(llvm::dbgs() << "[CSROA] Cleaning up the IR\n");

      for(auto revIt = invTopSortFns.rbegin(); revIt != invTopSortFns.rend(); ++revIt)
      {
         const auto& fnName = *revIt;
         if(!arrPartCtx.fnTable.count(fnName))
         {
            continue;
         }
         auto& fnPartInfo = getFnInfoOrDie(arrPartCtx.fnTable, fnName);
         if(fnPartInfo.fnPartitioned != nullptr)
         {
            if(!fnPartInfo.fnOriginal->use_empty())
            {
               errs() << "Original function " << fnPartInfo.fnOriginal->getName()
                      << " should not have any use at this point\n";
               assert(0);
            }
            fnPartInfo.fnOriginal->eraseFromParent();
            assert(fnPartInfo.fnPartitionedWithDupl->use_empty() &&
                   "PartitionedWithDupl function should not have any uses at this point");
            fnPartInfo.fnPartitionedWithDupl->eraseFromParent();
         }
      }
   }

   void insertArrayPartitionMetadata(Module& M)
   {
      LLVMContext& Ctx = M.getContext();
      MDNode* Node = MDNode::get(Ctx, MDString::get(Ctx, "true"));
      M.getOrInsertNamedMetadata("bambu_array_partition")->addOperand(Node);
   }
} // namespace

namespace llvm
{
   cl::opt<std::string> topFunctionName_CSROA("panda-TFN-csroa", cl::desc("Specify the name of the top function"),
                                              cl::value_desc("name of the top function") /*, cl::Required*/);
   cl::opt<std::string> outdirNameCmd("panda-outputdir-csroa",
                                      cl::desc("Specify the directory where the bambu IR raw file will be written"),
                                      cl::value_desc("directory path"), cl::Required);
   static cl::opt<bool> debug_lock("panda-lock-csroa", cl::init(false), cl::desc("Obtain default from a run"));

   struct CustomSROA : public ModulePass
#if LLVM_VERSION_MAJOR >= 13
       ,
                       public PassInfoMixin<CustomSROA>
#endif
   {
      static char ID;
      static bool shouldRunExtraPasses;

      CustomSROA() : ModulePass(ID)
      {
         initializeCallGraphWrapperPassPass(*PassRegistry::getPassRegistry());
         initializeLoopInfoWrapperPassPass(*PassRegistry::getPassRegistry());
      }

#if LLVM_VERSION_MAJOR >= 13
      CustomSROA(const CustomSROA&) : CustomSROA()
      {
      }
#endif

      bool exec(Module& M)
      {
         LLVM_DEBUG(llvm::dbgs() << "CUSTOM SCALAR REPLACEMENT of AGGREGATES\n");

         if(topFunctionName_CSROA.empty())
         {
            LLVM_DEBUG(dbgs() << "[CSROA] Top function not specified, skipping the pass\n");
            return false;
         }

         if(!isArrPartFunctionPresent(M))
         {
            LLVM_DEBUG(llvm::dbgs() << "[CSROA] CSROA partition function not present. Skipping CSROA pass.\n");
            return false;
         }

         if(debug_lock)
         {
            LLVM_DEBUG(llvm::dbgs() << "[CSROA] Debug lock CSROA is enabled. Skipping CSROA pass.\n");
            deleteAllUsersOfArrPartFunction(M);
            return true;
         }

         ArrPartCtx arrPartCtx;
         pugi::xml_document doc;
         arrPartCtx.doc = &doc;
         arrPartCtx.topFn = findTopFunction(M, topFunctionName_CSROA);
         if(!arrPartCtx.topFn)
         {
            return false;
         }

         LLVM_DEBUG(llvm::dbgs() << "[CSROA] TopFunctionName_CSROA: " << arrPartCtx.topFn->getName() << "\n";);
         shouldRunExtraPasses = true;
         insertArrayPartitionMetadata(M);
         LLVM_DEBUG(printModuleOnFile(M, outdirNameCmd + "/before_csroa.ll"));

         if(!loadXMLModule(doc, outdirNameCmd))
         {
            return false;
         }

         populateArrPartCtx(M, arrPartCtx);

         std::vector<std::string> invTopSortFns;
         inverseTopologicalSort(arrPartCtx.topFn, invTopSortFns);
         LLVM_DEBUG({
            dbgs() << "\n=====================================================================\n";
            dbgs() << "[CSROA] Initial array partition requests\n";
            describeArrPartRequests(arrPartCtx);
            dbgs() << "=====================================================================\n";
         });

         diffuseArrPartConfigs(arrPartCtx, invTopSortFns);

         dbgs() << "\n=====================================================================\n";
         dbgs() << "[CSROA] Final array partition requests\n";
         describeArrPartRequests(arrPartCtx);
         dbgs() << "=====================================================================\n";

         deleteAllUsersOfArrPartFunction(M);
         applyArrayPartition(M, arrPartCtx, invTopSortFns);

         modifyXMLModule(arrPartCtx, outdirNameCmd);

         cleanIR(M, arrPartCtx, invTopSortFns);

         LLVM_DEBUG(printModuleOnFile(M, outdirNameCmd + "/after_csroa.ll"));
         assert(!llvm::verifyModule(M, &llvm::errs()));
         LLVM_DEBUG(llvm::dbgs() << "[CSROA] End Pass: CUSTOM SCALAR REPLACEMENT of AGGREGATES\n");
         return false;
      }

      bool runOnModule(Module& M) override
      {
#if LLVM_VERSION_MAJOR < 13

         CallGraphWrapperPass* CGPass = getAnalysisIfAvailable<CallGraphWrapperPass>();
         if(!CGPass)
         {
            REPORT_FATAL_ERROR_WITH_REPORT("not able to retrieve the call graph");
         }
         return exec(M);
#else
         REPORT_FATAL_ERROR_WITH_REPORT("Call to runOnModule not expected with current LLVM version");
         return false;
#endif
      }

      StringRef getPassName() const override
      {
         return "customSROA";
      }

      void getAnalysisUsage(AnalysisUsage& AU) const override
      {
         AU.addRequired<CallGraphWrapperPass>();
         // AU.addRequired<AAResultsWrapperPass>();
         AU.addRequired<LoopInfoWrapperPass>();
      }

#if LLVM_VERSION_MAJOR >= 13
      llvm::PreservedAnalyses run(llvm::Module& M, llvm::ModuleAnalysisManager& MAM)
      {
         (void)MAM;
         const auto changed = exec(M);
         return (changed ? llvm::PreservedAnalyses::none() : llvm::PreservedAnalyses::all());
      }
#endif
   };
   char CustomSROA::ID = 0;
   bool CustomSROA::shouldRunExtraPasses = false;
} // namespace llvm

namespace
{
   using namespace llvm::PatternMatch;

   static void dbgReplace(Instruction* Old, Value* New, const char* Rule)
   {
      LLVM_DEBUG(dbgs() << "[HLSPeepholePass] " << Rule << "\n"
                        << "  before:  " << *Old << "\n"
                        << "  after:   " << *New << "\n");
   }

   bool runOnBasicBlock(BasicBlock& BB)
   {
      bool Changed = false;

      for(auto It = BB.begin(); It != BB.end();)
      {
         Instruction* I = &*It;
         ++It;

         Value* X = nullptr;
         ConstantInt* C = nullptr;

         // -----------------------------------------------------------------------
         // Rule 1 – mul x, 1  -->  x   (or mul 1, x)
         // -----------------------------------------------------------------------
         if(match(I, m_Mul(m_Value(X), m_One())) || match(I, m_Mul(m_One(), m_Value(X))))
         {
            dbgReplace(I, X, "mul x, 1 --> x");
            I->replaceAllUsesWith(X);
            I->eraseFromParent();
            Changed = true;
            continue;
         }

         // -----------------------------------------------------------------------
         // Rule 2 – add 0, x  -->  x   (or add x, 0)
         // -----------------------------------------------------------------------
         if(match(I, m_Add(m_Zero(), m_Value(X))) || match(I, m_Add(m_Value(X), m_Zero())))
         {
            dbgReplace(I, X, "add 0, x --> x");
            I->replaceAllUsesWith(X);
            I->eraseFromParent();
            Changed = true;
            continue;
         }

         // -----------------------------------------------------------------------
         // Rule 3 – udiv x, pow2  -->  lshr x, log2(pow2)
         // -----------------------------------------------------------------------
         if(match(I, m_UDiv(m_Value(X), m_ConstantInt(C))))
         {
            if(C->getValue().isPowerOf2())
            {
               IRBuilder<> Builder(I);
               Value* Shift = Builder.CreateLShr(X, C->getValue().exactLogBase2());
               dbgReplace(I, Shift, "udiv x, pow2 --> lshr x, log2(pow2)");
               I->replaceAllUsesWith(Shift);
               I->eraseFromParent();
               Changed = true;
               continue;
            }
         }

         // -----------------------------------------------------------------------
         // Rule 4 – urem x, pow2  -->  and x, pow2-1
         // -----------------------------------------------------------------------
         if(match(I, m_URem(m_Value(X), m_ConstantInt(C))))
         {
            if(C->getValue().isPowerOf2())
            {
               IRBuilder<> Builder(I);
               Value* And = Builder.CreateAnd(X, C->getValue() - 1);
               dbgReplace(I, And, "urem x, pow2 --> and x, pow2-1");
               I->replaceAllUsesWith(And);
               I->eraseFromParent();
               Changed = true;
               continue;
            }
         }
      }

      return Changed;
   }
} // namespace

namespace llvm
{
   struct HLSPeepholePass : public ModulePass
#if LLVM_VERSION_MAJOR >= 13
       ,
                            public PassInfoMixin<HLSPeepholePass>
#endif
   {
      static char ID;

      HLSPeepholePass() : ModulePass(ID)
      {
         initializeCallGraphWrapperPassPass(*PassRegistry::getPassRegistry());
         initializeLoopInfoWrapperPassPass(*PassRegistry::getPassRegistry());
      }

#if LLVM_VERSION_MAJOR >= 13
      HLSPeepholePass(const HLSPeepholePass&) : HLSPeepholePass()
      {
      }
#endif

      bool exec(Module& M)
      {
         LLVM_DEBUG(llvm::dbgs() << "HLS PEEPHOLE OPTIMIZATION PASS\n");
         bool changed = false;

         for(Function& F : M)
         {
            for(BasicBlock& BB : F)
            {
               changed |= runOnBasicBlock(BB);
            }
         }

         LLVM_DEBUG(llvm::dbgs() << "END HLS PEEPHOLE OPTIMIZATION PASS\n");
         return changed;
      }

      bool runOnModule(Module& M) override
      {
#if LLVM_VERSION_MAJOR < 13
         return exec(M);
#else
         REPORT_FATAL_ERROR_WITH_REPORT("Call to runOnModule not expected with current LLVM version");
         return false;
#endif
      }

      StringRef getPassName() const override
      {
         return "HLSPeepholePass";
      }

      void getAnalysisUsage(AnalysisUsage& AU) const override
      {
      }

#if LLVM_VERSION_MAJOR >= 13
      llvm::PreservedAnalyses run(llvm::Module& M, llvm::ModuleAnalysisManager& MAM)
      {
         (void)MAM;
         const auto changed = exec(M);
         return (changed ? llvm::PreservedAnalyses::none() : llvm::PreservedAnalyses::all());
      }
#endif
   };
   char HLSPeepholePass::ID = 0;
} // namespace llvm

#if LLVM_VERSION_MAJOR >= 13
namespace llvm
{
   class ConditionalPassRunner : public PassInfoMixin<ConditionalPassRunner>
   {
    public:
      PreservedAnalyses run(Module& M, ModuleAnalysisManager& AM)
      {
         // Check the flag at runtime (when the pass executes)
         if(!CustomSROA::shouldRunExtraPasses)
         {
            return PreservedAnalyses::all();
         }

         ModulePassManager MPM;
         MPM.addPass(llvm::HLSPeepholePass());
         MPM.addPass(llvm::VerifierPass());
#if LLVM_VERSION_MAJOR >= 16
         MPM.addPass(llvm::ScalarizeFifoArrayPass());
         MPM.addPass(llvm::VerifierPass());
         // MPM.addPass(llvm::CanonicalizeHLSStreamGEPPass());
         MPM.addPass(llvm::VerifierPass());
#endif

         FunctionPassManager FPM2;
         // It should delete the bitcast of the arguments
         FPM2.addPass(llvm::DCEPass());
         // Mem2Reg pass
         FPM2.addPass(llvm::PromotePass());
         // Constant propagation also in branches, if a branch cannot be taken it is erased
         FPM2.addPass(llvm::SCCPPass());
         MPM.addPass(llvm::createModuleToFunctionPassAdaptor(std::move(FPM2)));
         // Interprocedural SCCP
         MPM.addPass(llvm::IPSCCPPass());
         std::error_code ECFirstOpt;
         std::string outPathFirstOpt = outdirNameCmd + "/first_opt.ll";
         raw_fd_ostream outStreamFirstOpt(outPathFirstOpt, ECFirstOpt,
#if LLVM_VERSION_MAJOR >= 10
                                          sys::fs::OF_None
#else
                                          sys::fs::F_None
#endif
         );
         LLVM_DEBUG({ MPM.addPass(llvm::PrintModulePass(outStreamFirstOpt)); });

         FunctionPassManager FPM_WithLoops;
         // Canonicalize the loops
         FPM_WithLoops.addPass(llvm::LoopSimplifyPass());

         // Reorganize the loops in the closed form (used in many passes)
         FPM_WithLoops.addPass(llvm::LCSSAPass());
         LoopPassManager LPM;
         // Simplify the induction variable
         LPM.addPass(llvm::IndVarSimplifyPass());
         // Substitute expensive operations for example divs with other but creates a lot of boilerplate
         // code, for example bitcasts
         // LPM.addPass(llvm::LoopStrengthReducePass());
         FPM_WithLoops.addPass(llvm::createFunctionToLoopPassAdaptor(std::move(LPM)));
         // FPM_WithLoops.addPass(llvm::ConstraintEliminationPass());
         // FPM_WithLoops.addPass(llvm::CorrelatedValuePropagationPass());
         // FPM_WithLoops.addPass(llvm::SimplifyCFGPass());
         // FPM_WithLoops.addPass(llvm::InstCombinePass());

         MPM.addPass(llvm::createModuleToFunctionPassAdaptor(std::move(FPM_WithLoops)));
         std::error_code ECSecondOpt;
         std::string outPathSecondOpt = outdirNameCmd + "/second_opt.ll";
         raw_fd_ostream outStreamSecondOpt(outPathSecondOpt, ECSecondOpt,
#if LLVM_VERSION_MAJOR >= 10
                                           sys::fs::OF_None
#else
                                           sys::fs::F_None
#endif
         );
         LLVM_DEBUG({ MPM.addPass(llvm::PrintModulePass(outStreamSecondOpt)); });

         return MPM.run(M, AM);
      }

      static StringRef name()
      {
         return "ConditionalPassRunner";
      }
   };
} // namespace llvm
#endif

#ifndef _WIN32
static llvm::RegisterPass<llvm::CustomSROA> customSROAPass("customSROA", "Custom Scalar Replacement of Aggregates",
                                                           false /* Only looks at CFG */, false /* Analysis Pass */);
#endif

#if LLVM_VERSION_MAJOR >= 13
llvm::PassPluginLibraryInfo getCustomSROAInfo()
{
   return {LLVM_PLUGIN_API_VERSION, "customSROA", "v0.1", [](llvm::PassBuilder& PB) {
              const auto load = [](llvm::ModulePassManager& MPM) {
                 FunctionPassManager FPM0;
                 // Canonicalize the loops
                 FPM0.addPass(llvm::LoopSimplifyPass());
                 // Reorganize the loops in the closed form (used in many passes)
                 FPM0.addPass(llvm::LCSSAPass());
                 LoopPassManager LPM;
                 // Simplify the induction variable
                 LPM.addPass(llvm::IndVarSimplifyPass());
                 FPM0.addPass(llvm::createFunctionToLoopPassAdaptor(std::move(LPM)));
                 MPM.addPass(llvm::createModuleToFunctionPassAdaptor(std::move(FPM0)));

                 FunctionPassManager FPM1;
                 // MPM.addPass(llvm::LoopUnrollArrPartPass(outdirNameCmd, topFunctionName_CSROA, debug_lock));
                 MPM.addPass(llvm::VerifierPass());
                 FPM1.addPass(llvm::InstCombinePass());
                 FPM1.addPass(llvm::EarlyCSEPass());
                 FPM1.addPass(llvm::SimplifyCFGPass());
                 MPM.addPass(llvm::createModuleToFunctionPassAdaptor(std::move(FPM1)));
                 MPM.addPass(llvm::CustomSROA());
                 MPM.addPass(llvm::VerifierPass());

                 MPM.addPass(llvm::ConditionalPassRunner());
                 return true;
              };

              PB.registerPipelineParsingCallback([&](llvm::StringRef Name, llvm::ModulePassManager& MPM,
                                                     llvm::ArrayRef<llvm::PassBuilder::PipelineElement>) {
                 if(Name == "customSROA")
                 {
                    return load(MPM);
                 }
                 return false;
              });
              PB.registerOptimizerLastEPCallback([&](llvm::ModulePassManager& MPM,
#if LLVM_VERSION_MAJOR < 16
                                                     llvm::PassBuilder::OptimizationLevel
#else
                                                     llvm::OptimizationLevel
#endif
                                                 ) { return load(MPM); });
           }};
}

// This part is the new way of registering your pass
extern "C" ::llvm::PassPluginLibraryInfo LLVM_ATTRIBUTE_WEAK __attribute__((visibility("default")))
llvmGetPassPluginInfo()
{
   return getCustomSROAInfo();
}
#else
#if ADD_RSP
// This function is of type PassManagerBuilder::ExtensionFn
static void loadPass(const llvm::PassManagerBuilder&, llvm::legacy::PassManagerBase& PM)
{
#if PANDA_LLVM_CLANG_MAJOR >= 11
   PM.add(llvm::createInstructionCombiningPass(1000));
#else
   PM.add(llvm::createInstructionCombiningPass(true));
#endif
   PM.add(new llvm::CustomSROA());
   // PM.add(new llvm::ScalarizeSingletonArrayPass());
}

// These constructors add our pass to a list of global extensions.
static llvm::RegisterStandardPasses customsroa_Ox(llvm::PassManagerBuilder::EP_OptimizerLast, loadPass);
#endif
#endif
