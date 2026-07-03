/*
 *
 *                   _/_/_/    _/_/   _/    _/ _/_/_/    _/_/
 *                  _/   _/ _/    _/ _/_/  _/ _/   _/ _/    _/
 *                 _/_/_/  _/_/_/_/ _/  _/_/ _/   _/ _/_/_/_/
 *                _/      _/    _/ _/    _/ _/   _/ _/    _/
 *               _/      _/    _/ _/    _/ _/_/_/  _/    _/
 *
 *             ***********************************************
 *                              PandA Project
 *                     URL: http://panda.dei.polimi.it
 *                       Politecnico di Milano - DEIB
 *                        System Architectures Group
 *             ***********************************************
 *                Copyright (C) 2025-2026 Politecnico di Milano
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 *
 *   This file is part of the PandA framework.
 *
 *   Licensed under the Apache License, Version 2.0, with BAMBU exceptions (the "License");
 *   you may not use this file except in compliance with the License.
 *   You may obtain a copy of the License at
 *
 *       http://www.apache.org/licenses/LICENSE-2.0
 *
 *   Unless required by applicable law or agreed to in writing, software
 *   distributed under the License is distributed on an "AS IS" BASIS,
 *   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *   See the License for the specific language governing permissions and
 *   limitations under the License.
 *
 * @author Tommaso Fellegara <tommaso.fellegara@polimi.it>
 *
 */
#ifndef NDEBUG
#define NDEBUG
#endif
#include "loopUnrollArrPartPass.hpp"
#include "arrPart.hpp"
#include <llvm/Analysis/AliasAnalysis.h>
#include <llvm/Analysis/AssumptionCache.h>
#include <llvm/Analysis/LoopInfo.h>
#include <llvm/Analysis/ScalarEvolution.h>
#include <llvm/Analysis/TargetTransformInfo.h>
#include <llvm/Analysis/ValueTracking.h>
#include <llvm/IR/Dominators.h>
#include <llvm/IR/Module.h>
#include <llvm/Transforms/Utils/UnrollLoop.h>
#include <pugixml.hpp>

#if LLVM_VERSION_MAJOR >= 8
#include <llvm/Analysis/OptimizationRemarkEmitter.h>
#endif

#include "llvm/Analysis/ScalarEvolutionExpressions.h"

#include "debug_print.hpp"

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
   PHINode* getLoopInductionVariableCompat(Loop* loop, ScalarEvolution& SE)
   {
#if LLVM_VERSION_MAJOR >= 9
      return loop->getInductionVariable(SE);
#else
      (void)SE;
      return loop->getCanonicalInductionVariable();
#endif
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

   void getUnderlyingObjectsCompat(llvm::Value* v, llvm::SmallVector<llvm::Value*, 4>& objects,
                                   const llvm::DataLayout& DL)
   {
#if LLVM_VERSION_MAJOR >= 12
      llvm::SmallVector<const llvm::Value*, 4> tmp;
      llvm::getUnderlyingObjects(v, tmp);

      for(auto* obj : tmp)
      {
         objects.push_back(const_cast<llvm::Value*>(obj));
      }
#elif LLVM_VERSION_MAJOR >= 9
      llvm::SmallVector<const llvm::Value*, 4> tmp;
      llvm::GetUnderlyingObjects(v, tmp, DL, nullptr, 6);

      for(auto* obj : tmp)
      {
         objects.push_back(const_cast<llvm::Value*>(obj));
      }
#else
      llvm::GetUnderlyingObjects(v, objects, DL, nullptr, 6);
#endif
   }

   bool unrollLoopCompatAndGetChanged(Loop* loop, uint64_t iters, LoopInfo& LI, ScalarEvolution& SE, DominatorTree& DT,
                                      AssumptionCache& AC, TargetTransformInfo& TTI)
   {
#if LLVM_VERSION_MAJOR <= 5
      const bool Force = true;
      const bool Runtime = false;
      const bool AllowExpensiveTripCount = true;
      const bool PreserveLCSSA = false;
      const bool PreserveOnlyFirst = false;
      const unsigned UnrollRemainder = 0;
      const unsigned UnrollAndJam = 0;

      return UnrollLoop(loop, iters, 0, Force, Runtime, AllowExpensiveTripCount, PreserveLCSSA, PreserveOnlyFirst,
                        UnrollRemainder, UnrollAndJam, &LI, &SE, &DT, &AC, nullptr, false);
#elif LLVM_VERSION_MAJOR <= 8
      const bool Force = true;
      const bool Runtime = false;
      const bool AllowExpensiveTripCount = true;
      const bool PreserveCondBr = false;
      const bool PreserveOnlyFirst = false;
      const unsigned TripMultiple = 0;
      const unsigned PeelCount = 0;
      const bool UnrollRemainder = false;
      const bool PreserveLCSSA = false;

      return UnrollLoop(loop, iters, 0, Force, Runtime, AllowExpensiveTripCount, PreserveCondBr, PreserveOnlyFirst,
                        TripMultiple, PeelCount, UnrollRemainder, &LI, &SE, &DT, &AC, nullptr,
                        PreserveLCSSA) != llvm::LoopUnrollResult::Unmodified;
#elif LLVM_VERSION_MAJOR <= 10
      OptimizationRemarkEmitter ORE(loop->getHeader()->getParent());
      UnrollLoopOptions opts = {0};
      opts.Count = iters;
      // Ignore profitibility heuristics
      opts.Force = true;
      // Allow expensive analysis (multiple SCEV or other analysis) to compute trip count
      opts.AllowExpensiveTripCount = true;
      // Unroll leftover iterations
      opts.UnrollRemainder = false;
      opts.ForgetAllSCEV = true;
      return UnrollLoop(loop, opts, &LI, &SE, &DT, &AC, &ORE, false, nullptr) != LoopUnrollResult::Unmodified;
#elif LLVM_VERSION_MAJOR <= 12
      OptimizationRemarkEmitter ORE(loop->getHeader()->getParent());
      UnrollLoopOptions opts = {0};
      opts.Count = iters;
      opts.Force = true;
      opts.AllowExpensiveTripCount = true;
      opts.UnrollRemainder = false;
      opts.ForgetAllSCEV = true;
      return UnrollLoop(loop, opts, &LI, &SE, &DT, &AC, &TTI, &ORE, false, nullptr) != LoopUnrollResult::Unmodified;
#else
      OptimizationRemarkEmitter ORE(loop->getHeader()->getParent());
      UnrollLoopOptions opts = {0};
      opts.Count = iters;
      opts.Force = true;
      opts.Runtime = false;
      opts.AllowExpensiveTripCount = true;
      opts.UnrollRemainder = false;
      opts.ForgetAllSCEV = true;
      return UnrollLoop(loop, opts, &LI, &SE, &DT, &AC, &TTI, &ORE, false, nullptr) != LoopUnrollResult::Unmodified;
#endif
   }

   /**
    * @brief Check if an allocation has COMPLETE partitioning
    * @param alloc The allocation to check
    * @param arrPartCtx The partition context containing allocation info
    * @param dim The dimension to check
    * @return true if the allocation has exactly one partition with COMPLETE format
    */
   bool allocHasCompletePartitionOnDim(const AllocaInst* alloc, ArrPartCtx& arrPartCtx, unsigned dim)
   {
      assert(alloc && "Alloca instruction should not be null");

      // Check if the function has partition info
      if(!arrPartCtx.fnTable.count(alloc->getFunction()->getName()))
      {
         return false;
      }

      // Get partition allocations for this function
      auto& partAllocs = getFnInfoOrDie(arrPartCtx.fnTable, alloc->getFunction()->getName()).allocs;

      for(const AllocaPartInfo& info : partAllocs)
      {
         if(info.inst == alloc && info.scheme.size() > dim && info.scheme[dim].format == COMPLETE)
         {
            LLVM_DEBUG(dbgs() << "[UNROLL] Allocation with COMPLETE partitioning in index " << dim
                              << " found: " << info.to_string() << "\n");
            return true;
         }
      }

      return false;
   }

   bool argHasCompletePartitionOnDim(const Argument* arg, ArrPartCtx& arrPartCtx, unsigned dim)
   {
      assert(arg && "Argument should not be null");

      // Check if the function has partition info
      if(!arrPartCtx.fnTable.count(arg->getParent()->getName()))
      {
         return false;
      }

      // Get partition allocations for this function
      auto& partArg = getFnInfoOrDie(arrPartCtx.fnTable, arg->getParent()->getName()).args;

      for(const ArgPartInfo& info : partArg)
      {
         if(info.arg == arg && info.scheme.size() > dim && info.scheme[dim].format == COMPLETE)
         {
            LLVM_DEBUG(dbgs() << "[UNROLL] Argument with COMPLETE partitioning in index " << dim
                              << " found: " << info.to_string() << "\n");
            return true;
         }
      }

      return false;
   }

   bool globHasCompletePartitionOnDim(const GlobalVariable* glob, ArrPartCtx& arrPartCtx, unsigned dim)
   {
      assert(glob && "GlobalVariable should not be null");

      for(const GlobalPartInfo& info : arrPartCtx.globalVars)
      {
         if(info.var == glob && info.scheme.size() > dim && info.scheme[dim].format == COMPLETE)
         {
            LLVM_DEBUG(dbgs() << "[UNROLL] GlobalVariable with COMPLETE partitioning in index " << dim
                              << " found: " << info.to_string() << "\n");
            return true;
         }
      }

      return false;
   }

   bool allocHasAtLeastOneCompletePartition(const AllocaInst* alloc, ArrPartCtx& arrPartCtx)
   {
      assert(alloc && "Alloca instruction should not be null");

      // Check if the function has partition info
      if(!arrPartCtx.fnTable.count(alloc->getFunction()->getName()))
      {
         return false;
      }

      // Get partition allocations for this function
      auto& partAllocs = getFnInfoOrDie(arrPartCtx.fnTable, alloc->getFunction()->getName()).allocs;
      for(const AllocaPartInfo& info : partAllocs)
      {
         if(info.inst == alloc)
         {
            for(const PartInfo& partInfo : info.scheme)
            {
               if(partInfo.format == COMPLETE)
               {
                  LLVM_DEBUG(dbgs() << "[UNROLL] Allocation with at least one COMPLETE partitioning found: "
                                    << info.to_string() << "\n");
                  return true;
               }
            }
         }
      }

      return false;
   }

   bool argHasAtLeastOneCompletePartition(const Argument* arg, ArrPartCtx& arrPartCtx)
   {
      assert(arg && "Argument should not be null");

      // Check if the function has partition info
      if(!arrPartCtx.fnTable.count(arg->getParent()->getName()))
      {
         return false;
      }

      // Get partition allocations for this function
      auto& partArg = getFnInfoOrDie(arrPartCtx.fnTable, arg->getParent()->getName()).args;
      for(const ArgPartInfo& info : partArg)
      {
         if(info.arg == arg)
         {
            for(const PartInfo& partInfo : info.scheme)
            {
               if(partInfo.format == COMPLETE)
               {
                  LLVM_DEBUG(dbgs() << "[UNROLL] Argument with at least one COMPLETE partitioning found: "
                                    << info.to_string() << "\n");
                  return true;
               }
            }
         }
      }

      return false;
   }

   bool globHasAtLeastOneCompletePartition(const GlobalVariable* glob, ArrPartCtx& arrPartCtx)
   {
      assert(glob && "GlobalVariable should not be null");

      for(const GlobalPartInfo& info : arrPartCtx.globalVars)
      {
         if(info.var == glob)
         {
            for(const PartInfo& partInfo : info.scheme)
            {
               if(partInfo.format == COMPLETE)
               {
                  LLVM_DEBUG(dbgs() << "[UNROLL] GlobalVariable with at least one COMPLETE partitioning found: "
                                    << info.to_string() << "\n");
                  return true;
               }
            }
         }
      }

      return false;
   }

   Value* getLoadStorePointerOperand(Instruction& inst)
   {
      if(auto* loadInst = dyn_cast<LoadInst>(&inst))
      {
         return loadInst->getPointerOperand();
      }

      if(auto* storeInst = dyn_cast<StoreInst>(&inst))
      {
         return storeInst->getPointerOperand();
      }

      return nullptr;
   }

   // bool ivAccessPartitionedDimension(Value* memPtr, Loop* loop, const DataLayout& DL, ArrPartCtx& arrPartCtx,
   //                                   ScalarEvolution& SE)
   // {
   //    // Only the second case is supported
   //    auto* inductionVar = getLoopInductionVariableCompat(loop, SE);

   //    if(!inductionVar)
   //    {
   //       LLVM_DEBUG(dbgs() << "[UNROLL] Loop has no induction variable, skipping complete partitioned access
   //       check\n"); return false;
   //    }

   //    if(auto* gep = dyn_cast<GetElementPtrInst>(memPtr))
   //    {
   //       auto* idxsBegin = std::next(gep->op_begin());
   //       auto* it = std::find_if(idxsBegin, gep->op_end(), [&](const Use& use) { return use == inductionVar; });

   //       if(it == gep->op_end())
   //       {
   //          LLVM_DEBUG(dbgs() << "[UNROLL] GEP instruction does not use the loop induction variable as index,
   //          skipping "
   //                               "complete partitioned access check\n");
   //          return false;
   //       }

   //       unsigned dimIndex = std::distance(idxsBegin, it);

   //       Value* baseValue = getUnderlyingObjectCompat(gep, DL);
   //       auto* alloc = dyn_cast<AllocaInst>(baseValue);
   //       auto* arg = dyn_cast<Argument>(baseValue);
   //       auto* global = dyn_cast<GlobalVariable>(baseValue);
   //       if(!alloc && !arg && !global)
   //       {
   //          LLVM_DEBUG(dbgs() << "[UNROLL] Skipping use: cannot trace back to an allocation (";
   //                     baseValue->print(dbgs()); dbgs() << ")\n");
   //          return false;
   //       }

   //       // %4 = alloca [66 x i32], align 4
   //       // %10 = getelementptr [66 x i32], [66 x i32]* %4, i32 0, i32 %9
   //       //                                              Adjust dimension index since the second operand of
   //       //                                              the GEP is used to access the array type of alloca
   //       if(alloc)
   //       {
   //          assert(dimIndex > 0 && "Dimension index should be greater than 0 for alloca access since the first GEP "
   //                                 "operand is the dereference of the alloca");
   //          return allocHasCompletePartitionOnDim(alloc, arrPartCtx, dimIndex - 1);
   //       }

   //       if(arg)
   //       {
   //          return argHasCompletePartitionOnDim(arg, arrPartCtx, dimIndex);
   //       }

   //       if(global)
   //       {
   //          assert(dimIndex > 0 && "Dimension index should be greater than 0 for global variable access since the "
   //                                 "first GEP operand is the dereference of the global variable");
   //          return globHasCompletePartitionOnDim(global, arrPartCtx, dimIndex - 1);
   //       }
   //    }

   //    return false;
   // }

   // bool ivMarkedCompletePartition(Value* memPtr, const DataLayout& DL, ScalarEvolution& SE, ArrPartCtx& arrPartCtx)
   // {
   //    Value* baseValue = getUnderlyingObjectCompat(memPtr, DL);

   //    if(auto* phi = dyn_cast<PHINode>(baseValue))
   //    {
   //       if(!isa<SCEVAddRecExpr>(SE.getSCEV(phi)))
   //       {
   //          LLVM_DEBUG(dbgs() << "[UNROLL] PHI node is not an induction variable, skipping IV marked with COMPLETE "
   //                               "partition check\n");
   //          return false;
   //       }

   //       SmallVector<Value*, 4> objects;
   //       getUnderlyingObjectsCompat(phi, objects, DL);
   //       for(const Value* obj : objects)
   //       {
   //          const auto* alloc = dyn_cast<AllocaInst>(obj);
   //          const auto* arg = dyn_cast<Argument>(obj);
   //          const auto* global = dyn_cast<GlobalVariable>(obj);
   //          if(!alloc && !arg && !global)
   //          {
   //             LLVM_DEBUG(dbgs() << "[UNROLL] Skipping PHI operand: cannot trace back to an allocation (";
   //                        obj->print(dbgs()); dbgs() << ")\n");
   //             continue;
   //          }

   //          if(alloc && allocHasCompletePartitionOnDim(alloc, arrPartCtx, 0))
   //          {
   //             return true;
   //          }

   //          if(arg && argHasCompletePartitionOnDim(arg, arrPartCtx, 0))
   //          {
   //             return true;
   //          }

   //          if(global && globHasCompletePartitionOnDim(global, arrPartCtx, 0))
   //          {
   //             return true;
   //          }
   //       }
   //    }

   //    LLVM_DEBUG(dbgs() << "[UNROLL] Skipping loop: no induction variable marked with COMPLETE partition\n");
   //    return false;
   // }

   // bool processInstructionsInLoop(Loop* loop, ScalarEvolution& SE, ArrPartCtx& arrPartCtx,
   //                                std::set<Loop*>& loopsToUnroll)
   // {
   //    const auto& DL = loop->getHeader()->getModule()->getDataLayout();
   //    std::set<const BasicBlock*> nestedBlocks;

   //    for(Loop* subLoop : loop->getSubLoops())
   //    {
   //       for(BasicBlock* subLoopBlock : subLoop->blocks())
   //       {
   //          nestedBlocks.insert(subLoopBlock);
   //       }
   //    }

   //    for(BasicBlock* BB : loop->blocks())
   //    {
   //       if(nestedBlocks.count(BB))
   //       {
   //          continue;
   //       }

   //       for(Instruction& I : *BB)
   //       {
   //          Value* memPtr = getLoadStorePointerOperand(I);
   //          if(!memPtr)
   //          {
   //             continue;
   //          }

   //          // There are two main cases where we can have an access to a complete partitioned alloca/arg:
   //          // the first case when the pointer operand of the load/store is a GEP instruction that
   //          // accesses the partitions with an index that is a loop induction variable or a value derived from it.
   //          // the first case when the pointer operand of the load/store is an induction variable that is used to
   //          access
   //          // the partitions
   //          if(ivAccessPartitionedDimension(memPtr, loop, DL, arrPartCtx, SE))
   //          {
   //             LLVM_DEBUG(dbgs() << "[UNROLL] Loop marked for unrolling due to IV accesses complete partition: ";
   //                        loop->print(dbgs()); dbgs() << "\n\n");
   //             loopsToUnroll.insert(loop);
   //             return true;
   //          }

   //          if(ivMarkedCompletePartition(memPtr, DL, SE, arrPartCtx))
   //          {
   //             LLVM_DEBUG(dbgs() << "[UNROLL] Loop marked for unrolling due to IV marked with COMPLETE partition: ";
   //                        loop->print(dbgs()); dbgs() << "\n\n");
   //             loopsToUnroll.insert(loop);
   //             return true;
   //          }
   //       }
   //    }
   //    return false;
   // }

   // First method to mark loops for unrolling
   // void processLoopRecursively(Loop* loop, ScalarEvolution& SE, ArrPartCtx& arrPartCtx, std::set<Loop*>&
   // loopsToUnroll)
   // {
   //    if(!loop)
   //    {
   //       return;
   //    }

   //    // Check if the loop has a constant trip count
   //    if(!isa<SCEVConstant>(SE.getBackedgeTakenCount(loop)))
   //    {
   //       LLVM_DEBUG(dbgs() << "[UNROLL] Skipping loop: non-constant trip count\n");
   //       return;
   //    }

   //    LLVM_DEBUG(dbgs() << "[UNROLL] Processing loop: "; loop->print(dbgs()););
   //    bool loopInserted = processInstructionsInLoop(loop, SE, arrPartCtx, loopsToUnroll);
   //    if(!loopInserted)
   //    {
   //       LLVM_DEBUG(dbgs() << "[UNROLL] Loop NOT marked for unrolling: "; loop->print(dbgs()); dbgs() << "\n\n");
   //    }

   //    // Recursively process nested loops
   //    for(Loop* subLoop : loop->getSubLoops())
   //    {
   //       processLoopRecursively(subLoop, SE, arrPartCtx, loopsToUnroll);
   //    }
   // }

   // Second Method to mark loops for unrolling
   // void processLoop(Loop* loop, LoopInfo& LI, ScalarEvolution& SE, ArrPartCtx& arrPartCtx, std::set<Loop*>&
   // loopsToUnroll)
   // {
   //    if(!loop)
   //    {
   //       return;
   //    }

   //    LLVM_DEBUG(dbgs() << "[UNROLL] Processing loop: "; loop->print(dbgs()););
   //    for (auto* BB : loop->blocks())
   //    {
   //       for(auto& inst : *BB)
   //       {
   //          Value* memPtr = getLoadStorePointerOperand(inst);
   //          if(!memPtr)
   //          {
   //             continue;
   //          }

   //          const auto* memPtrScev = SE.getSCEV(memPtr);
   //          if (const auto* ptrSCEV = dyn_cast<SCEVAddRecExpr>(memPtrScev))
   //          {
   //             LLVM_DEBUG(dbgs() << "[UNROLL] Found memory access: "; memPtr->print(dbgs()); dbgs() << " with
   //             AddRecExpr
   // SCEV "); LLVM_DEBUG(memPtrScev->print(dbgs()); dbgs() << "\n");
   //             loopsToUnroll.insert(LI.getLoopFor(ptrSCEV->getLoop()->getHeader()));
   //          }
   //          else if (auto* gep = dyn_cast<GetElementPtrInst>(memPtr))
   //          {
   //             for (auto& idx : gep->indices())
   //             {
   //                if (const auto* idxSCEV = dyn_cast<SCEVAddRecExpr>(SE.getSCEV(idx.get())))
   //                {
   //                   loopsToUnroll.insert(LI.getLoopFor(idxSCEV->getLoop()->getHeader()));
   //                }
   //             }
   //          }
   //       }
   //    }

   // }

   void insertAllSubLoops(Loop* loop, std::set<Loop*>& loopsToUnroll)
   {
      if(!loop)
      {
         return;
      }

      loopsToUnroll.insert(loop);
      for(Loop* subLoop : loop->getSubLoops())
      {
         insertAllSubLoops(subLoop, loopsToUnroll);
      }
   }

   Value* stripBitcasts(Value* memPtr)
   {
      while(auto* bitcast = dyn_cast<BitCastInst>(memPtr))
      {
         memPtr = bitcast->getOperand(0);
      }
      return memPtr;
   }

   void processLoopRecursively(Loop* loop, LoopInfo& LI, ScalarEvolution& SE, ArrPartCtx& arrPartCtx,
                               std::set<Loop*>& loopsToUnroll)
   {
      if(!loop)
      {
         return;
      }

      // Visit nested loops first so loop discovery runs from innermost to outermost.
      for(Loop* subLoop : loop->getSubLoops())
      {
         processLoopRecursively(subLoop, LI, SE, arrPartCtx, loopsToUnroll);
      }

      LLVM_DEBUG(dbgs() << "[UNROLL] Processing loop: "; loop->print(dbgs()););
      std::set<const BasicBlock*> nestedBlocks;
      for(Loop* subLoop : loop->getSubLoops())
      {
         for(BasicBlock* subLoopBlock : subLoop->blocks())
         {
            nestedBlocks.insert(subLoopBlock);
         }
      }

      const auto& DL = loop->getHeader()->getModule()->getDataLayout();
      for(auto* BB : loop->blocks())
      {
         if(nestedBlocks.count(BB))
         {
            continue;
         }

         for(auto& inst : *BB)
         {
            Value* memPtr = getLoadStorePointerOperand(inst);
            if(!memPtr)
            {
               continue;
            }

            memPtr = stripBitcasts(memPtr);
            SmallVector<Value*, 4> idxs;
            if(auto* gep = dyn_cast<GetElementPtrInst>(memPtr))
            {
               memPtr = gep->getPointerOperand();
               idxs.append(gep->idx_begin(), gep->idx_end());
            }
            memPtr = stripBitcasts(memPtr);

            const auto* alloc = dyn_cast<AllocaInst>(memPtr);
            const auto* arg = dyn_cast<Argument>(memPtr);
            const auto* glob = dyn_cast<GlobalVariable>(memPtr);
            if(!alloc && !arg && !glob)
            {
               LLVM_DEBUG(dbgs() << "[UNROLL] Skipping memory operand: cannot trace back to an allocation (";
                          memPtr->print(dbgs()); dbgs() << ")\n");
               continue;
            }

            if(alloc)
            {
               idxs.erase(idxs.begin());
               for(size_t i = 0; i < idxs.size(); ++i)
               {
                  if(const auto* idxSCEV = dyn_cast<SCEVAddRecExpr>(SE.getSCEV(idxs[i])))
                  {
                     if(idxSCEV->getLoop() == loop && allocHasCompletePartitionOnDim(alloc, arrPartCtx, i))
                     {
                        insertAllSubLoops(loop, loopsToUnroll);
                        goto end;
                     }
                  }
               }
            }

            if(arg)
            {
               for(size_t i = 0; i < idxs.size(); ++i)
               {
                  if(const auto* idxSCEV = dyn_cast<SCEVAddRecExpr>(SE.getSCEV(idxs[i])))
                  {
                     if(idxSCEV->getLoop() == loop && argHasCompletePartitionOnDim(arg, arrPartCtx, i))
                     {
                        insertAllSubLoops(loop, loopsToUnroll);
                        goto end;
                     }
                  }
               }
            }

            if(glob)
            {
               idxs.erase(idxs.begin());
               for(size_t i = 0; i < idxs.size(); ++i)
               {
                  if(const auto* idxSCEV = dyn_cast<SCEVAddRecExpr>(SE.getSCEV(idxs[i])))
                  {
                     if(idxSCEV->getLoop() == loop && globHasCompletePartitionOnDim(glob, arrPartCtx, i))
                     {
                        insertAllSubLoops(loop, loopsToUnroll);
                        goto end;
                     }
                  }
               }
            }

            // SmallVector<Value*, 4> objects;
            // getUnderlyingObjectsCompat(memPtr, objects, DL);
            // for(const Value* obj : objects)
            // {
            //    const auto* alloc = dyn_cast<AllocaInst>(obj);
            //    const auto* arg = dyn_cast<Argument>(obj);
            //    const auto* global = dyn_cast<GlobalVariable>(obj);
            //    if(!alloc && !arg && !global)
            //    {
            //       LLVM_DEBUG(dbgs() << "[UNROLL] Skipping memory operand: cannot trace back to an allocation (";
            //                  obj->print(dbgs()); dbgs() << ")\n");
            //       continue;
            //    }

            //    if(alloc && allocHasAtLeastOneCompletePartition(alloc, arrPartCtx))
            //    {
            //       insertAllSubLoops(loop, loopsToUnroll);
            //       goto end;
            //    }

            //    if(arg && argHasAtLeastOneCompletePartition(arg, arrPartCtx))
            //    {
            //       insertAllSubLoops(loop, loopsToUnroll);
            //       goto end;
            //    }

            //    if(global && globHasAtLeastOneCompletePartition(global, arrPartCtx))
            //    {
            //       insertAllSubLoops(loop, loopsToUnroll);
            //       goto end;
            //    }
            // }
         }
      }
   end:;
   }

   /**
    * @brief Populate the set of loops to unroll based on memory access patterns
    *
    * This function identifies loops that should be unrolled when:
    * 1. The loop has a constant trip count
    * 2. The loop body (excluding nested subloops) contains a load/store
    * 3. The load/store accesses an alloca/argument with COMPLETE partitioning
    *
    * @param LI Loop information analysis
    * @param SE Scalar evolution analysis
    * @param AA Alias analysis results (currently unused but kept for future use)
    * @param arrPartCtx Array partition context containing allocation partition information
    * @param loopsToUnroll Output set to populate with loops that should be unrolled
    */
   void populateLoopsToUnrollInFunction(LoopInfo& LI, ScalarEvolution& SE, ArrPartCtx& arrPartCtx,
                                        std::set<Loop*>& loopsToUnroll)
   {
#if LLVM_VERSION_MAJOR >= 11
      for(auto* loop : LI.getTopLevelLoops())
#else
      for(auto* loop : LI)
#endif
      {
         processLoopRecursively(loop, LI, SE, arrPartCtx, loopsToUnroll);
      }
   }

   bool unrollLoopsInFunction(const std::set<Loop*>& loopsToUnroll, LoopInfo& LI, ScalarEvolution& SE,
                              DominatorTree& DT, AssumptionCache& AC, TargetTransformInfo& TTI)
   {
      bool changed = false;
      // Process inner loops before their parents to avoid invalidating loop handles that are still pending.
      std::vector<Loop*> orderedLoops(loopsToUnroll.begin(), loopsToUnroll.end());
      std::sort(orderedLoops.begin(), orderedLoops.end(), [](const Loop* lhs, const Loop* rhs) {
         if(lhs->getLoopDepth() != rhs->getLoopDepth())
         {
            return lhs->getLoopDepth() > rhs->getLoopDepth();
         }
         return lhs < rhs;
      });

      for(Loop* loop : orderedLoops)
      {
         const auto* backedgeTakenCount = SE.getBackedgeTakenCount(loop);
         const auto* tripCount = dyn_cast<SCEVConstant>(backedgeTakenCount);
         if(!tripCount)
         {
            LLVM_DEBUG(dbgs() << "[UNROLL] Skipping loop " << loop
                              << ": cannot fully unroll due to non-constant backedge taken count: ";
                       backedgeTakenCount->print(dbgs()); dbgs() << "\n";);
            continue;
         }

         uint64_t iters = tripCount->getValue()->getZExtValue() + 1;
         LLVM_DEBUG(dbgs() << "[UNROLL] Completely unrolling loop " << loop << " with trip count: " << iters << "\n");
         changed |= unrollLoopCompatAndGetChanged(loop, iters, LI, SE, DT, AC, TTI);
      }
      return changed;
   }

   uint64_t getTotalNumberOfSubLoops(Loop* loop)
   {
      uint64_t count = loop->getSubLoops().size();
      for(Loop* subLoop : loop->getSubLoops())
      {
         count += getTotalNumberOfSubLoops(subLoop);
      }
      return count;
   }

   uint64_t getTotalNumberOfLoops(LoopInfo& LI)
   {
      uint64_t count = 0;
#if LLVM_VERSION_MAJOR >= 11
      for(auto* loop : LI.getTopLevelLoops())
#else
      for(auto* loop : LI)
#endif
      {
         count += 1;
         count += getTotalNumberOfSubLoops(loop);
      }
      return count;
   }
} // namespace

namespace llvm
{
   bool LoopUnrollArrPartPass::exec(Module& M, llvm::function_ref<llvm::LoopInfo&(llvm::Function&)> GetLI,
                                    llvm::function_ref<llvm::ScalarEvolution&(llvm::Function&)> GetSE,
                                    llvm::function_ref<llvm::TargetTransformInfo&(llvm::Function&)> GetTTI,
                                    llvm::function_ref<llvm::DominatorTree&(llvm::Function&)> GetDomTree,
                                    llvm::function_ref<llvm::AssumptionCache&(llvm::Function&)> GetAC)
   {
      LLVM_DEBUG(printModuleOnFile(M, outdirNameCmd + "/before_automatic_unrolling.ll"));
      LLVM_DEBUG(llvm::dbgs() << "UNROLLING LOOP INIT\n");

      if(debug_lock)
      {
         LLVM_DEBUG(llvm::dbgs() << "[UNROLL] Debug lock CSROA is enabled. Skipping loop unrolling.\n");
         return false;
      }

      if(!isArrPartFunctionPresent(M))
      {
         return false;
      }

      Function* topFn = findTopFunction(M, topFnName);
      ArrPartCtx arrPartCtx;
      pugi::xml_document doc;
      arrPartCtx.topFn = topFn;
      arrPartCtx.doc = &doc;
      if(!arrPartCtx.topFn)
      {
         return false;
      }

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

      LLVM_DEBUG({
         dbgs() << "\n=====================================================================\n";
         dbgs() << "[CSROA] Final array partition requests\n";
         describeArrPartRequests(arrPartCtx);
         dbgs() << "=====================================================================\n";
      });

      bool changed = false;
      for(const auto& fnName : invTopSortFns)
      {
         Function* fn = M.getFunction(fnName);
         LLVM_DEBUG(dbgs() << "[UNROLL] Analyzing function for loop unrolling: " << fn->getName() << "\n");
         auto& initialLI = GetLI(*fn);

         uint64_t totalLoopsUnrolled = 0;
         uint64_t totalLoops = getTotalNumberOfLoops(initialLI);
         bool repeat = true;
         while(repeat)
         {
            std::set<Loop*> loopsToUnroll;
            auto& LI = GetLI(*fn);
            auto& SE = GetSE(*fn);
            auto& DT = GetDomTree(*fn);
            auto& AC = GetAC(*fn);
            auto& TTI = GetTTI(*fn);
            populateLoopsToUnrollInFunction(LI, SE, arrPartCtx, loopsToUnroll);
            if(!loopsToUnroll.empty())
            {
               totalLoopsUnrolled += loopsToUnroll.size();
               const bool unrolledAnyLoop = unrollLoopsInFunction(loopsToUnroll, LI, SE, DT, AC, TTI);
               changed |= unrolledAnyLoop;
               repeat = unrolledAnyLoop;
            }
            else
            {
               repeat = false;
            }
         }
         LLVM_DEBUG(dbgs() << "[UNROLL] Marked " << totalLoopsUnrolled << " out of " << totalLoops
                           << " total loops for unrolling\n");
      }

      LLVM_DEBUG(printModuleOnFile(M, outdirNameCmd + "/after_automatic_unrolling.ll"));
      LLVM_DEBUG(dbgs() << "[UNROLL] Finished loop unrolling pass\n");
      return changed;
   }

   StringRef LoopUnrollArrPartPass::getPassName() const
   {
      return "UNROLL_INIT_LOOPS";
   }

   void LoopUnrollArrPartPass::getAnalysisUsage(AnalysisUsage& AU) const
   {
      AU.addRequired<LoopInfoWrapperPass>();
      AU.addRequired<ScalarEvolutionWrapperPass>();
      AU.addRequired<DominatorTreeWrapperPass>();
      AU.addRequired<AssumptionCacheTracker>();
      AU.addRequired<TargetTransformInfoWrapperPass>();
   }

   bool LoopUnrollArrPartPass::runOnModule(Module& M)
   {
#if LLVM_VERSION_MAJOR < 13
      auto GetLI = [&](llvm::Function& F) -> llvm::LoopInfo& {
         return getAnalysis<llvm::LoopInfoWrapperPass>(F).getLoopInfo();
      };

      auto GetSE = [&](llvm::Function& F) -> llvm::ScalarEvolution& {
         return getAnalysis<llvm::ScalarEvolutionWrapperPass>(F).getSE();
      };

      auto GetTTI = [&](llvm::Function& F) -> llvm::TargetTransformInfo& {
         return getAnalysis<llvm::TargetTransformInfoWrapperPass>().getTTI(F);
      };

      auto GetDomTree = [&](llvm::Function& F) -> llvm::DominatorTree& {
         return getAnalysis<llvm::DominatorTreeWrapperPass>(F).getDomTree();
      };

      auto GetAC = [&](llvm::Function& F) -> llvm::AssumptionCache& {
         return getAnalysis<llvm::AssumptionCacheTracker>().getAssumptionCache(F);
      };

      return exec(M, GetLI, GetSE, GetTTI, GetDomTree, GetAC);
#else
      REPORT_FATAL_ERROR_WITH_REPORT("Call to runOnModule not expected with current LLVM version");
      return false;
#endif
   }

#if LLVM_VERSION_MAJOR >= 13
   llvm::PreservedAnalyses LoopUnrollArrPartPass::run(llvm::Module& M, llvm::ModuleAnalysisManager& MAM)
   {
      MAM.invalidate(M, llvm::PreservedAnalyses::none());
      auto& FAM = MAM.getResult<FunctionAnalysisManagerModuleProxy>(M).getManager();
      auto GetLI = [&](llvm::Function& F) -> llvm::LoopInfo& { return FAM.getResult<llvm::LoopAnalysis>(F); };
      auto GetSE = [&](llvm::Function& F) -> llvm::ScalarEvolution& {
         return FAM.getResult<llvm::ScalarEvolutionAnalysis>(F);
      };
      auto GetTTI = [&](llvm::Function& F) -> llvm::TargetTransformInfo& {
         return FAM.getResult<llvm::TargetIRAnalysis>(F);
      };
      auto GetDomTree = [&](llvm::Function& F) -> llvm::DominatorTree& {
         return FAM.getResult<llvm::DominatorTreeAnalysis>(F);
      };
      auto GetAC = [&](llvm::Function& F) -> llvm::AssumptionCache& {
         return FAM.getResult<llvm::AssumptionAnalysis>(F);
      };

      const auto changed = exec(M, GetLI, GetSE, GetTTI, GetDomTree, GetAC);
      return (changed ? llvm::PreservedAnalyses::none() : llvm::PreservedAnalyses::all());
   }
#endif

   char LoopUnrollArrPartPass::ID = 0;
} // namespace llvm
