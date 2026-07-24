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
 *   Copyright (C) 2018-2026 Politecnico di Milano
 *
 * Part of the PandA Project, under the Apache License v2.0 with LLVM Exceptions.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 *
 */
/**
 * @file plugin_openmp.cpp
 * @brief In case openmp function is available all global objects but the top function can be private.
 * This is going to simplify quite much the obtained IR.
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 * @author Michele Fiorito <michele.fiorito@polimi.it>
 *
 */
#ifndef NDEBUG
#define NDEBUG
#endif
#include "plugin_includes.hpp"

#include <llvm/ADT/DepthFirstIterator.h>
#include <llvm/ADT/STLExtras.h>
#include <llvm/ADT/ScopeExit.h>
#include <llvm/ADT/SmallPtrSet.h>
#include <llvm/ADT/SmallVector.h>
#include <llvm/ADT/Statistic.h>
#include <llvm/ADT/StringExtras.h>
#include <llvm/ADT/Twine.h>
#include <llvm/Analysis/AssumptionCache.h>
#include <llvm/Analysis/BasicAliasAnalysis.h>
#include <llvm/Analysis/CallGraph.h>
#include <llvm/Analysis/CallGraphSCCPass.h>
#include <llvm/Analysis/LazyCallGraph.h>
#include <llvm/Analysis/Loads.h>
#include <llvm/Analysis/MemoryLocation.h>
#include <llvm/Analysis/TargetLibraryInfo.h>
#include <llvm/Analysis/TargetTransformInfo.h>
#include <llvm/IR/Argument.h>
#include <llvm/IR/Attributes.h>
#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/CFG.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/DataLayout.h>
#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/InstrTypes.h>
#include <llvm/IR/Instruction.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/IntrinsicInst.h>
#include <llvm/IR/Intrinsics.h>
#include <llvm/IR/LegacyPassManager.h>
#include <llvm/IR/Metadata.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/NoFolder.h>
#include <llvm/IR/PassManager.h>
#include <llvm/IR/Type.h>
#include <llvm/IR/Use.h>
#include <llvm/IR/User.h>
#include <llvm/IR/Value.h>
#include <llvm/InitializePasses.h>
#include <llvm/Pass.h>
#include <llvm/PassRegistry.h>
#include <llvm/Support/Casting.h>
#include <llvm/Support/CommandLine.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/Support/FormatVariadic.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Transforms/IPO.h>
#include <llvm/Transforms/Utils/BasicBlockUtils.h>
#include <llvm/Transforms/Utils/LoopUtils.h>
#if PANDA_LLVM_CLANG_MAJOR < 13
#include <llvm/Transforms/IPO/PassManagerBuilder.h>
#elif PANDA_LLVM_CLANG_MAJOR >= 13
#include <llvm/Passes/PassBuilder.h>
#include <llvm/Passes/PassPlugin.h>
#endif

#include <algorithm>
#include <cstdint>
#include <cxxabi.h>
#include <functional>
#include <iostream>
#include <iterator>
#include <map>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "kmp_bambu_names.h"

#define DEBUG_TYPE "openmp"
#define VAL(str) #str
#define TOSTRING(str) VAL(str)

namespace llvm
{
   struct openmpBambu;
   static bool remove_openmp_lifetime(llvm::Function& function)
   {
      std::vector<llvm::Instruction*> intrinsic_to_remove;

      for(llvm::BasicBlock& bb : function)
      {
         for(llvm::Instruction& i : bb)
         {
            if(llvm::CallInst* call_inst = llvm::dyn_cast<llvm::CallInst>(&i))
            {
               llvm::Function* called_function = call_inst->getCalledFunction();

               if(called_function && called_function->isIntrinsic())
               {
                  if(called_function->getIntrinsicID() == llvm::Intrinsic::lifetime_start)
                  {
                     intrinsic_to_remove.push_back(call_inst);
                  }
                  if(called_function->getIntrinsicID() == llvm::Intrinsic::lifetime_end)
                  {
                     intrinsic_to_remove.push_back(call_inst);
                  }
               }
            }
         }
      }

      for(llvm::Instruction* instr : intrinsic_to_remove)
      {
         instr->eraseFromParent();
      }

      return !intrinsic_to_remove.empty();
   }

   static bool remove_openmp_stack_intrinsics(llvm::Function& function)
   {
      std::vector<llvm::Instruction*> intrinsic_to_remove;

      for(llvm::BasicBlock& bb : function)
      {
         for(llvm::Instruction& i : bb)
         {
            if(llvm::CallInst* call_inst = llvm::dyn_cast<llvm::CallInst>(&i))
            {
               llvm::Function* called_function = call_inst->getCalledFunction();

               if(called_function && called_function->isIntrinsic())
               {
                  if(called_function->getIntrinsicID() == llvm::Intrinsic::stackrestore)
                  {
                     intrinsic_to_remove.push_back(call_inst);
                  }
                  if(called_function->getIntrinsicID() == llvm::Intrinsic::stacksave)
                  {
                     intrinsic_to_remove.push_back(call_inst);
                  }
               }
            }
         }
      }

      for(llvm::Instruction* instr : intrinsic_to_remove)
      {
         instr->eraseFromParent();
      }

      return !intrinsic_to_remove.empty();
   }

   static bool propagate_nproc0(llvm::Function& function, ConstantInt* nt)
   {
      std::vector<llvm::Instruction*> kmp_to_remove;

      for(llvm::BasicBlock& bb : function)
      {
         for(llvm::Instruction& i : bb)
         {
            if(llvm::CallInst* call_inst = llvm::dyn_cast<llvm::CallInst>(&i))
            {
               llvm::Function* called_function = call_inst->getCalledFunction();

               if(called_function && !called_function->isIntrinsic())
               {
                  auto calledFunName = called_function->getName();
                  if(calledFunName == TOSTRING(KMP_T_NPROC))
                  {
                     auto cst = llvm::ConstantInt::get(nt->getType(), nt->getValue());
                     i.replaceAllUsesWith(cst);
                     kmp_to_remove.push_back(call_inst);
                  }
               }
            }
         }
      }

      for(llvm::Instruction* instr : kmp_to_remove)
      {
         instr->eraseFromParent();
      }

      return !kmp_to_remove.empty();
   }

   template <class Inst>
   static void traverse_users_to(llvm::Value* val, std::set<Inst*>& found, bool same_bb = false)
   {
      for(const auto& user : val->users())
      {
         if(same_bb)
         {
            const auto inst = dyn_cast<llvm::Instruction>(val);
            const auto use_i = dyn_cast<llvm::Instruction>(user);
            if(!inst || !use_i || inst->getParent() != use_i->getParent())
            {
               continue;
            }
         }

         if(dyn_cast<CastInst>(user))
         {
            LLVM_DEBUG(llvm::dbgs() << "     cast: " << user << "\n");
            traverse_users_to<Inst>(user, found);
         }
         else if(const auto search = dyn_cast<Inst>(user))
         {
            LLVM_DEBUG(llvm::dbgs() << "     match: " << user << "\n");
            found.insert(search);
         }
      }
   }

   static void promote_ptr_to_ptr(llvm::Instruction* inst, llvm::Instruction* new_ptr)
   {
      assert(isa<PointerType>(inst->getType()) && isa<PointerType>(new_ptr->getType()));
      std::set<llvm::Instruction*> users;
#if PANDA_LLVM_CLANG_MAJOR < 16
      const auto elem_type = cast<PointerType>(new_ptr->getType())->getElementType();
#else
      if(new_ptr->getType()->isOpaquePointerTy())
      {
         LLVM_DEBUG(llvm::dbgs() << "OpenMP pluging does not support opaque pointers.\n");
         return;
      }
      const auto elem_type = new_ptr->getType()->getNonOpaquePointerElementType();
#endif
      LLVM_DEBUG(llvm::dbgs() << "     elem type: " << elem_type << "\n");
      const auto ptr_val = [&]() -> llvm::Instruction* {
         if(!dyn_cast<GetElementPtrInst>(new_ptr))
         {
            const std::array<llvm::Value*, 2> idxs = {
                ConstantInt::get(Type::getIntNTy(new_ptr->getParent()->getParent()->getContext(), 32), 0),
                ConstantInt::get(Type::getIntNTy(new_ptr->getParent()->getParent()->getContext(), 32), 0),
            };
            const auto gep = GetElementPtrInst::Create(elem_type, new_ptr, idxs);
            gep->insertAfter(new_ptr);
            return gep;
         }
         return new_ptr;
      }();
      traverse_users_to<llvm::Instruction>(inst, users);
      for(const auto user : users)
      {
         LLVM_DEBUG(llvm::dbgs() << "     user: " << user << "\n");
         if(auto gep = dyn_cast<GetElementPtrInst>(user))
         {
            const auto new_gep = [&]() {
               const auto idx = [&]() {
                  if(gep->getSourceElementType()->isArrayTy())
                  {
                     assert(gep->getNumIndices() == 2);
                     auto idx_it = gep->idx_begin();
                     std::advance(idx_it, 1U);
                     assert(isa<ConstantInt>(*idx_it));
                     return cast<ConstantInt>(*idx_it)->getValue();
                  }
                  assert(isa<ConstantInt>(*gep->idx_begin()));
                  const auto val = cast<ConstantInt>(*gep->idx_begin())->getValue();
                  return val.udiv(APInt(val.getBitWidth(), 4));
               }();
               const std::array<llvm::Value*, 2> idxs = {
                   ConstantInt::get(Type::getIntNTy(new_ptr->getParent()->getParent()->getContext(), 32), 0),
                   ConstantInt::get(Type::getIntNTy(new_ptr->getParent()->getParent()->getContext(), 32), idx),
               };
               return GetElementPtrInst::Create(elem_type, new_ptr, idxs, "", gep);
            }();
            LLVM_DEBUG(llvm::dbgs() << "     gep: " << new_gep << "\n");
            promote_ptr_to_ptr(gep, new_gep);
            gep->replaceUsesOfWith(inst, UndefValue::get(inst->getType()));
            gep->replaceAllUsesWith(UndefValue::get(gep->getType()));
         }
         else if(auto li = dyn_cast<LoadInst>(user))
         {
            LLVM_DEBUG(llvm::dbgs() << "     load\n");
            li->replaceUsesOfWith(inst, UndefValue::get(inst->getType()));
            li->replaceAllUsesWith(ptr_val);
         }
         else if(auto si = dyn_cast<StoreInst>(user))
         {
            LLVM_DEBUG(llvm::dbgs() << "     store\n");
            if(si->getValueOperand()->getType()->isPointerTy())
            {
               // TODO: assert stored pointer address equals new_ptr address
               user->replaceUsesOfWith(inst, UndefValue::get(inst->getType()));
            }
            else
            {
               si->replaceUsesOfWith(si->getPointerOperand(), new_ptr);
            }
         }
      }
      if(ptr_val->user_empty())
      {
         ptr_val->eraseFromParent();
      }
   }

   static bool propagate_nproc(llvm::Function& function, ConstantInt* default_nt)
   {
      std::vector<llvm::Instruction*> kmp_to_remove;
      Type* local_data_struct_t = nullptr;

      for(llvm::BasicBlock& bb : function)
      {
         auto cur_thread_number = default_nt;
         for(llvm::Instruction& i : bb)
         {
            if(llvm::CallInst* call_inst = llvm::dyn_cast<llvm::CallInst>(&i))
            {
               llvm::Function* cf = call_inst->getCalledFunction();
               auto analyzeFunction = [&](llvm::Function* called_function) {
                  auto calledFunName = called_function->getName();
                  LLVM_DEBUG(llvm::dbgs() << "  Found: " << calledFunName << "\n");
                  if(calledFunName == TOSTRING(KMP_TH_SET_NPROC))
                  {
                     auto arg1 = call_inst->getArgOperand(1);
                     if(isa<llvm::ConstantInt>(arg1))
                     {
                        cur_thread_number = cast<llvm::ConstantInt>(arg1);
                        LLVM_DEBUG(llvm::dbgs() << "   number of threads: " << cur_thread_number << "\n");
                     }
                  }
                  else if(calledFunName == "__kmpc_fork_call")
                  {
                     auto lambdaFun = call_inst->getArgOperand(2);
                     if(isa<llvm::ConstantExpr>(lambdaFun))
                     {
                        auto ce = cast<llvm::ConstantExpr>(lambdaFun);
                        if(ce->getOpcode() == llvm::Instruction::BitCast)
                        {
                           auto op = ce->getOperand(0);
                           if(isa<llvm::Function>(op))
                           {
                              propagate_nproc0(*dyn_cast<llvm::Function>(op), cur_thread_number);
                           }
                        }
                        else if(isa<llvm::Function>(lambdaFun))
                        {
                           propagate_nproc0(*dyn_cast<llvm::Function>(lambdaFun), cur_thread_number);
                        }
                     }
                     cur_thread_number = default_nt;
                  }
                  else if(calledFunName == TOSTRING(KMP_SET_REDUCE_DATA))
                  {
#if PANDA_LLVM_CLANG_MAJOR < 14
                     assert(call_inst->getNumArgOperands() >= 2 && "Expected to have at least 2 arguments!");
#else
                     assert(call_inst->arg_size() >= 2 && "Expected to have at least 2 arguments!");
#endif
                     auto set_reduce_data_arg = call_inst->getArgOperand(1U);
                     LLVM_DEBUG(llvm::dbgs() << "   set reduce data arg: " << set_reduce_data_arg << "\n");
                     if(!isa<Instruction>(set_reduce_data_arg))
                     {
                        LLVM_DEBUG(llvm::dbgs() << "Unable to optimize nested function calls\n");
                        return;
                     }

                     auto local_data_stack = cast<Instruction>(set_reduce_data_arg);
                     while(auto cast_inst = dyn_cast<CastInst>(local_data_stack))
                     {
                        if(!isa<Instruction>(cast_inst->getOperand(0)))
                        {
                           report_fatal_error("Cast operand must be a valid instruction!");
                        }
                        local_data_stack = cast<Instruction>(cast_inst->getOperand(0));
                        LLVM_DEBUG(llvm::dbgs() << "    cast from: " << local_data_stack << "\n");
                     }
                     LLVM_DEBUG(llvm::dbgs() << "   local reduce stack: " << local_data_stack << "\n");
#if PANDA_LLVM_CLANG_MAJOR < 19
                     assert((local_data_stack->getType()->isPointerTy() &&
                             local_data_stack->getType()->getPointerElementType()->isArrayTy()) &&
                            "Argument should be a pointer to array type!");
#endif
                     std::vector<Instruction*> local_data_ptrs;
                     for(const auto& use : local_data_stack->users())
                     {
                        if(use == set_reduce_data_arg)
                        {
                           continue;
                        }
                        LLVM_DEBUG(llvm::dbgs() << "   user: " << use << "\n");
                        uint64_t local_data_idx = 0U;
                        Value* local_data_ptr = use;
                        if(auto gep = dyn_cast<GetElementPtrInst>(local_data_ptr))
                        {
                           auto element_idx = [&]() {
                              auto idx_it = gep->idx_begin();
                              std::advance(idx_it, gep->getNumIndices() - 1U);
                              return dyn_cast<ConstantInt>(idx_it);
                           }();
                           assert(element_idx);
                           local_data_idx = element_idx->getValue().getZExtValue();
                        }
                        LLVM_DEBUG(llvm::dbgs() << "     local data index: " << local_data_idx << "\n");
                        std::set<StoreInst*> stores;
                        traverse_users_to<StoreInst>(use, stores, true);
                        if(local_data_ptrs.size() <= local_data_idx)
                        {
                           local_data_ptrs.resize(local_data_idx + 1U);
                        }
                        if(stores.empty())
                        {
                           continue;
                        }
                        assert(stores.size() == 1U && "Expected a single store to update local reduce data!");
                        local_data_ptrs[local_data_idx] = dyn_cast<Instruction>((*stores.begin())->getOperand(0));
                        assert(local_data_ptrs[local_data_idx] &&
                               local_data_ptrs[local_data_idx]->getType()->isPointerTy());
                        // remove store operation which will become useless after promotion
                        kmp_to_remove.push_back((*stores.begin()));
                        LLVM_DEBUG(llvm::dbgs()
                                   << "     local data var  : " << local_data_ptrs[local_data_idx] << "\n");
                     }
                     const auto found_all = [&]() {
                        bool missing = false;
                        for(const auto& val : local_data_ptrs)
                        {
                           missing |= !val;
                        }
                        return !missing && !local_data_ptrs.empty();
                     }();
                     if(found_all)
                     {
                        // Create new struct with local reduce data
                        std::vector<Type*> local_data_types;
                        for(const auto& val : local_data_ptrs)
                        {
#if PANDA_LLVM_CLANG_MAJOR < 16
                           local_data_types.push_back(cast<PointerType>(val->getType())->getElementType());
#else
                           if(val->getType()->isOpaquePointerTy())
                           {
                              LLVM_DEBUG(llvm::dbgs() << "OpenMP pluging does not support opaque pointers.\n");
                              return;
                           }
                           local_data_types.push_back(
                               cast<PointerType>(val->getType())->getNonOpaquePointerElementType());
#endif
                        }
                        assert(!local_data_struct_t);
                        local_data_struct_t = StructType::create(function.getContext(), local_data_types);
                        LLVM_DEBUG(llvm::dbgs() << "   local data type: " << local_data_struct_t << "\n");
                        // Allocate local reduce data structure
#if PANDA_LLVM_CLANG_MAJOR != 4
                        const auto DL = function.getParent()->getDataLayout();
#endif
                        const auto local_data_ptr = new AllocaInst(local_data_struct_t,
#if PANDA_LLVM_CLANG_MAJOR != 4
                                                                   DL.getAllocaAddrSpace(),
#endif
                                                                   nullptr, "", local_data_stack);
                        LLVM_DEBUG(llvm::dbgs() << "   local data struct: " << local_data_ptr << "\n");
                        // Fix set th local reduce data call
                        const auto local_data_cast =
                            new BitCastInst(local_data_ptr, set_reduce_data_arg->getType(), "", call_inst);
                        call_inst->setArgOperand(1U, local_data_cast);
                        LLVM_DEBUG(llvm::dbgs() << "   local data set: " << call_inst << "\n");
                        promote_ptr_to_ptr(local_data_stack, local_data_ptr);
#ifndef NDEBUG
                        for(const auto u : local_data_ptr->users())
                        {
                           LLVM_DEBUG(llvm::dbgs() << "   after: " << u << "\n");
                        }
#endif
                        kmp_to_remove.push_back(local_data_stack);
                        for(auto i = 0U; i < local_data_ptrs.size(); ++i)
                        {
                           const auto data_ptr = local_data_ptrs.at(i);
                           const auto field_ptr = [&]() {
                              const std::array<Value*, 2> idxs = {
                                  ConstantInt::get(Type::getInt32Ty(function.getContext()), 0),
                                  ConstantInt::get(Type::getInt32Ty(function.getContext()), i),
                              };
                              const auto gep = GetElementPtrInst::Create(local_data_struct_t, local_data_ptr, idxs);
                              gep->insertAfter(local_data_ptr);
                              return gep;
                           }();
#ifndef NDEBUG
                           errs() << "   reduce data " << i << ": ";
                           data_ptr->print(errs(), true);
                           errs() << " -> ";
                           field_ptr->print(errs(), true);
                           errs() << "\n";
#endif
                           data_ptr->replaceAllUsesWith(field_ptr);
#ifndef NDEBUG
                           for(const auto u : field_ptr->users())
                           {
                              LLVM_DEBUG(llvm::dbgs() << "   after: " << u << "\n");
                           }
#endif
                           data_ptr->eraseFromParent();
                        }
                     }
                  }
                  else if(calledFunName == TOSTRING(KMP_GET_REDUCE_DATA))
                  {
                     if(local_data_struct_t)
                     {
                        const auto data_struct_ptr_t = PointerType::getUnqual(local_data_struct_t);
                        const auto placeholder = UndefValue::get(call_inst->getType());
                        const auto ptr_cast = new BitCastInst(placeholder, data_struct_ptr_t);
                        ptr_cast->insertAfter(call_inst);
                        promote_ptr_to_ptr(call_inst, ptr_cast);
                        ptr_cast->replaceUsesOfWith(placeholder, call_inst);
                     }
                  }
               };

               if(cf)
               {
                  analyzeFunction(cf);
               }
               else
               {
#if PANDA_LLVM_CLANG_MAJOR >= 11
                  auto calledFun = call_inst->getCalledOperand();
#else
                  auto calledFun = call_inst->getCalledValue();
#endif
                  if(isa<llvm::ConstantExpr>(calledFun))
                  {
                     auto ce = cast<llvm::ConstantExpr>(calledFun);
                     if(ce->getOpcode() == llvm::Instruction::BitCast)
                     {
                        auto op = ce->getOperand(0);
                        if(isa<llvm::Function>(op))
                        {
                           analyzeFunction(cast<llvm::Function>(op));
                        }
                     }
                     else if(isa<llvm::Function>(calledFun))
                     {
                        analyzeFunction(cast<llvm::Function>(calledFun));
                     }
                  }
               }
            }
         }
      }

      for(auto instr : kmp_to_remove)
      {
         instr->replaceAllUsesWith(UndefValue::get(instr->getType()));
         instr->eraseFromParent();
      }

      return !kmp_to_remove.empty();
   }

   static bool fix_omp_outline_parameters(llvm::Function* function,
                                          std::map<Function*, std::set<uint64_t>>& argToPromote,
                                          std::set<uint64_t>& deadArgToPromote, llvm::Function*& updatedFunction)
   {
      updatedFunction = function;
      bool hasDirectCall = false;
      for(Use& U : function->uses())
      {
#if PANDA_LLVM_CLANG_MAJOR >= 11
         CallBase* CB = dyn_cast<CallBase>(U.getUser());
         // Must be an indirect call.
         if(CB == nullptr || !CB->isCallee(&U))
            continue;
#else
         CallSite CS(U.getUser());
         // Must be am indirect call.
         if(CS.getInstruction() == nullptr || !CS.isCallee(&U))
            continue;
#endif
         hasDirectCall = true;
      }
      assert(!hasDirectCall);

      /// A vector used to hold the indices of a single GEP instruction
      using IndicesVector = std::vector<uint64_t>;
      using ScalarizeTable = std::set<std::pair<Type*, IndicesVector>>;
      FunctionType* FTy = function->getFunctionType();
      std::vector<Type*> Params;
#if PANDA_LLVM_CLANG_MAJOR == 4
      SmallVector<AttributeSet, 8> AttributesVec;
#else
      SmallVector<AttributeSet, 8> ArgAttrVec;
#endif
      auto PAL = function->getAttributes();

#if PANDA_LLVM_CLANG_MAJOR == 4
      // Add any return attributes.
      if(PAL.hasAttributes(AttributeSet::ReturnIndex))
      {
         AttributesVec.push_back(AttributeSet::get(function->getContext(), PAL.getRetAttributes()));
      }
#endif
      std::map<Argument*, ScalarizeTable> ScalarizedElements;
      const bool hasArgToBePromoted = argToPromote.count(function);
      // create the new parameter list
      unsigned ArgNo = 2;
      auto I = function->arg_begin();
      std::advance(I, 2U);
      for(auto E = function->arg_end(); I != E; ++I, ++ArgNo)
      {
         if(hasArgToBePromoted && argToPromote.at(function).count(ArgNo))
         {
            if(I->user_empty())
            {
               deadArgToPromote.insert(ArgNo);
#if PANDA_LLVM_CLANG_MAJOR > 4 && PANDA_LLVM_CLANG_MAJOR < 12
               // There may be remaining metadata uses of the argument for things like
               // llvm.dbg.value. Replace them with undef.
               I->replaceAllUsesWith(UndefValue::get(I->getType()));
#endif
            }
            else
            {
               if(I->user_empty())
               {
                  assert(I->getType()->isPointerTy());
#if PANDA_LLVM_CLANG_MAJOR < 16
                  Params.push_back(llvm::cast<llvm::PointerType>(I->getType())->getElementType());
#else
                  if(I->getType()->isOpaquePointerTy())
                  {
                     LLVM_DEBUG(llvm::dbgs() << "OpenMP pluging does not support opaque pointers.\n");
                     return false;
                  }
                  Params.push_back(llvm::cast<llvm::PointerType>(I->getType())->getNonOpaquePointerElementType());
#endif
#if PANDA_LLVM_CLANG_MAJOR > 4
                  ArgAttrVec.push_back(AttributeSet());
#endif
               }
               else
               {
#if PANDA_LLVM_CLANG_MAJOR < 16
                  Type* AgTy = cast<PointerType>(I->getType())->getElementType();
#else
                  if(I->getType()->isOpaquePointerTy())
                  {
                     LLVM_DEBUG(llvm::dbgs() << "OpenMP pluging does not support opaque pointers.\n");
                     return false;
                  }
                  Type* AgTy = cast<PointerType>(I->getType())->getNonOpaquePointerElementType();
#endif
                  if(AgTy->isStructTy())
                  {
                     StructType* STy = cast<StructType>(AgTy);
                     for(auto elType : STy->elements())
                     {
                        if(elType->isArrayTy())
                        {
                           auto aT = dyn_cast<ArrayType>(elType);
#if PANDA_LLVM_CLANG_MAJOR < 16
                           auto arrElType = aT->getElementType();
#else
                           if(aT->isOpaquePointerTy())
                           {
                              LLVM_DEBUG(llvm::dbgs() << "OpenMP pluging does not support opaque pointers.\n");
                              return false;
                           }
                           auto arrElType = aT->getNonOpaquePointerElementType();
#endif
                           unsigned ind = 0;
                           unsigned nEl = elType->getArrayNumElements();
                           for(ind = 0; ind < nEl; ++ind)
                           {
                              Params.push_back(arrElType);
#if PANDA_LLVM_CLANG_MAJOR != 4
                              ArgAttrVec.push_back(AttributeSet());
#endif
                           }
                        }
                        else
                        {
                           Params.push_back(elType);
#if PANDA_LLVM_CLANG_MAJOR != 4
                           ArgAttrVec.push_back(AttributeSet());
#endif
                        }
                     }
                  }
                  else
                  {
                     ScalarizeTable& ArgIndices = ScalarizedElements[&*I];
                     for(User* U : I->users())
                     {
                        Instruction* UI = cast<Instruction>(U);
                        Type* SrcTy;
                        if(LoadInst* L = dyn_cast<LoadInst>(UI))
                        {
                           SrcTy = L->getType();
                        }
                        else
                        {
                           SrcTy = cast<GetElementPtrInst>(UI)->getSourceElementType();
                        }
                        IndicesVector Indices;
                        Indices.reserve(UI->getNumOperands() - 1);
                        // Since loads will only have a single operand, and GEPs only a single
                        // non-index operand, this will record direct loads without any indices,
                        // and gep+loads with the GEP indices.
                        for(User::op_iterator II = UI->op_begin() + 1, IE = UI->op_end(); II != IE; ++II)
                        {
                           Indices.push_back(cast<ConstantInt>(*II)->getSExtValue());
                        }
                        // GEPs with a single 0 index can be merged with direct loads
                        if(Indices.size() == 1 && Indices.front() == 0)
                        {
                           Indices.clear();
                        }
                        ArgIndices.insert(std::make_pair(SrcTy, Indices));
                     }
                     assert(ArgIndices.size() == 1);

                     for(const auto& ArgIndex : ArgIndices)
                     {
                        // not allowed to dereference ->begin() if size() is 0
#if PANDA_LLVM_CLANG_MAJOR < 16
                        Params.push_back(GetElementPtrInst::getIndexedType(
                            cast<PointerType>(I->getType()->getScalarType())->getElementType(), ArgIndex.second));
#else
                        if(I->getType()->getScalarType()->isOpaquePointerTy())
                        {
                           LLVM_DEBUG(llvm::dbgs() << "OpenMP pluging does not support opaque pointers.\n");
                           return false;
                        }
                        Params.push_back(GetElementPtrInst::getIndexedType(
                            cast<PointerType>(I->getType()->getScalarType())->getNonOpaquePointerElementType(),
                            ArgIndex.second));
#endif
#if PANDA_LLVM_CLANG_MAJOR > 4
                        ArgAttrVec.push_back(AttributeSet());
#endif
                        assert(Params.back());
                     }
                  }
               }
            }
         }
         else
         {
            Params.push_back(I->getType());
#if PANDA_LLVM_CLANG_MAJOR == 4
            auto ArgIndex = ArgNo + 1;
            AttributeSet attrs = PAL.getParamAttributes(ArgIndex);
            if(attrs.hasAttributes(ArgIndex))
            {
               AttrBuilder B(attrs, ArgIndex);
               AttributesVec.push_back(AttributeSet::get(function->getContext(), Params.size(), B));
            }
#elif PANDA_LLVM_CLANG_MAJOR < 16
            ArgAttrVec.push_back(PAL.getParamAttributes(ArgNo));
#else
            ArgAttrVec.push_back(PAL.getParamAttrs(ArgNo));
#endif
         }
      }

#if PANDA_LLVM_CLANG_MAJOR == 4
      // Add any function attributes.
      if(PAL.hasAttributes(AttributeSet::FunctionIndex))
      {
         AttributesVec.push_back(AttributeSet::get(FTy->getContext(), PAL.getFnAttributes()));
      }
#endif
      Type* RetTy = FTy->getReturnType();

      // Construct the new function type using the new arguments.
      FunctionType* NFTy = FunctionType::get(RetTy, Params, FTy->isVarArg());

      // Create the new function body and insert it into the module.
      Function* NF = Function::Create(NFTy, function->getLinkage(), function->getName());
      NF->copyAttributesFrom(function);

      // Patch the pointer to LLVM function in debug info descriptor.
#if PANDA_LLVM_CLANG_MAJOR <= 11
      NF->setSubprogram(function->getSubprogram());
#else
      NF->copyMetadata(function, 0);
#endif
      function->setSubprogram(nullptr);

      // Recompute the parameter attributes list based on the new arguments for
      // the function.
#if PANDA_LLVM_CLANG_MAJOR == 4
      NF->setAttributes(AttributeSet::get(function->getContext(), AttributesVec));
      AttributesVec.clear();
#elif PANDA_LLVM_CLANG_MAJOR < 16
      NF->setAttributes(
          AttributeList::get(function->getContext(), PAL.getFnAttributes(), PAL.getRetAttributes(), ArgAttrVec));
      ArgAttrVec.clear();
#else
      NF->setAttributes(AttributeList::get(function->getContext(), PAL.getFnAttrs(), PAL.getRetAttrs(), ArgAttrVec));
      ArgAttrVec.clear();
#endif

      function->getParent()->getFunctionList().insert(function->getIterator(), NF);
      NF->takeName(function);

      function->replaceAllUsesWith(NF);

#if PANDA_LLVM_CLANG_MAJOR != 4
      const DataLayout& DL = function->getParent()->getDataLayout();
#endif

      // Splice the body of the old function into the new function
#if PANDA_LLVM_CLANG_MAJOR < 16
      NF->getBasicBlockList().splice(NF->begin(), function->getBasicBlockList());
#else
      NF->splice(NF->begin(), NF);
#endif

      // Replace the use of the promoted parameters in the function
      ArgNo = 0;
      for(Function::arg_iterator I = function->arg_begin(), E = function->arg_end(), I2 = NF->arg_begin(); I != E;
          ++I, ++ArgNo)
      {
         if(ArgNo <= 1 || (hasArgToBePromoted && argToPromote.at(function).count(ArgNo)))
         {
            if(I->use_empty())
            {
               continue;
            }
#if PANDA_LLVM_CLANG_MAJOR < 16
            Type* AgTy = cast<PointerType>(I->getType())->getElementType();
#else
            if(I->getType()->isOpaquePointerTy())
            {
               LLVM_DEBUG(llvm::dbgs() << "OpenMP pluging does not support opaque pointers.\n");
               return false;
            }
            Type* AgTy = cast<PointerType>(I->getType())->getNonOpaquePointerElementType();
#endif
            if(AgTy->isStructTy())
            {
               assert(ArgNo > 1);
               // In the callee, we create an alloca, and store each of the new incoming
               // arguments into the alloca.
               Instruction* InsertPt = &NF->begin()->front();

               // Just add all the struct element types.
               Value* TheAlloca = new AllocaInst(AgTy,
#if PANDA_LLVM_CLANG_MAJOR != 4
                                                 DL.getAllocaAddrSpace(),
#endif
                                                 nullptr,
#if PANDA_LLVM_CLANG_MAJOR == 4

#elif PANDA_LLVM_CLANG_MAJOR < 10
                                                 I->getParamAlignment(),
#elif PANDA_LLVM_CLANG_MAJOR == 10
                                                 MaybeAlign(I->getParamAlignment()),
#elif PANDA_LLVM_CLANG_MAJOR < 16
                                                 I->getParamAlign().getValueOr(DL.getPrefTypeAlign(AgTy)),
#else
                                                 I->getParamAlign().valueOrOne() == llvm::Align(1) ?
                                                     DL.getPrefTypeAlign(AgTy) :
                                                     I->getParamAlign().valueOrOne(),
#endif
                                                 "", InsertPt);
               StructType* STy = cast<StructType>(AgTy);
               Value* Idxs[2] = {ConstantInt::get(Type::getInt32Ty(function->getContext()), 0), nullptr};
               std::vector<llvm::Value*> idxs(3, nullptr);
               idxs[0] = Idxs[0];

               for(unsigned i = 0, e = STy->getNumElements(); i != e; ++i)
               {
                  Idxs[1] = ConstantInt::get(Type::getInt32Ty(function->getContext()), i);
                  if(STy->getElementType(i)->isArrayTy())
                  {
                     idxs[1] = Idxs[1];
                     for(unsigned j = 0; j < STy->getElementType(i)->getArrayNumElements(); ++j)
                     {
                        idxs[2] = ConstantInt::get(Type::getInt32Ty(function->getContext()), j);
                        Value* Idx = GetElementPtrInst::Create(AgTy, TheAlloca, idxs,
                                                               TheAlloca->getName() + "." + Twine(i), InsertPt);
                        new StoreInst(&*I2++, Idx, InsertPt);
                     }
                  }
                  else
                  {
                     Value* Idx = GetElementPtrInst::Create(AgTy, TheAlloca, Idxs,
                                                            TheAlloca->getName() + "." + Twine(i), InsertPt);
                     new StoreInst(&*I2++, Idx, InsertPt);
                  }
               }

               // Anything that used the arg should now use the alloca.
               I->replaceAllUsesWith(TheAlloca);
               TheAlloca->takeName(&*I);

               // If the alloca is used in a call, we must clear the tail flag since
               // the callee now uses an alloca from the caller.
               for(User* U : TheAlloca->users())
               {
                  CallInst* Call = dyn_cast<CallInst>(U);
                  if(!Call)
                  {
                     continue;
                  }
                  Call->setTailCall(false);
               }
            }
            else
            {
               assert((ArgNo <= 1 || ScalarizedElements.count(&*I)) && "Argument not found");
               ScalarizeTable* ArgIndices = ArgNo <= 1 ? nullptr : &ScalarizedElements.at(&*I);
               llvm::Value* TheArg = &*I2;
               if(ArgNo <= 1)
               {
                  const std::vector<std::string> fcall = {TOSTRING(KMP_CS_GET_GTID), TOSTRING(KMP_CS_GET_TID)};
                  const auto& fname = fcall[ArgNo];
                  const auto M = NF->getParent();
                  IRBuilder<> Builder(M->getContext());
                  Builder.SetInsertPoint(&NF->front().front());
                  auto func = M->getOrInsertFunction(
                      fname, llvm::FunctionType::get(llvm::Type::getInt32Ty(M->getContext()), false));
                  const auto ci = Builder.CreateCall(func);
                  ci->setTailCall();
                  LLVM_DEBUG(llvm::dbgs() << "   call: " << ci << "\n");
                  TheArg = ci;
               }
               while(!I->use_empty())
               {
                  if(LoadInst* LI = dyn_cast<LoadInst>(I->user_back()))
                  {
                     assert((!ArgIndices || ArgIndices->begin()->second.empty()) &&
                            "Load element should sort to front!");
#ifndef NDEBUG
                     for(const auto& user : LI->users())
                     {
                        LLVM_DEBUG(llvm::dbgs() << "   before: " << user << "\n");
                     }
#endif
                     LI->replaceAllUsesWith(TheArg);
#ifndef NDEBUG
                     for(const auto& user : TheArg->users())
                     {
                        LLVM_DEBUG(llvm::dbgs() << "   after: " << user << "\n");
                     }
#endif
                     LI->eraseFromParent();
                  }
                  else
                  {
                     assert(ArgIndices &&
                            "First and second argument of .omp.outlined not expected to be complex pointers");
                     GetElementPtrInst* GEP = cast<GetElementPtrInst>(I->user_back());
                     IndicesVector Operands;
                     Operands.reserve(GEP->getNumIndices());
                     for(User::op_iterator II = GEP->idx_begin(), IE = GEP->idx_end(); II != IE; ++II)
                        Operands.push_back(cast<ConstantInt>(*II)->getSExtValue());

                     // GEPs with a single 0 index can be merged with direct loads
                     if(Operands.size() == 1 && Operands.front() == 0)
                        Operands.clear();

                     Function::arg_iterator ArgIt = I2;
                     for(ScalarizeTable::iterator It = ArgIndices->begin(); It->second != Operands; ++It, ++ArgIt)
                     {
                        assert(It != ArgIndices->end() && "GEP not handled??");
                     }
                     TheArg = &*ArgIt;

                     // All of the uses must be load instructions.  Replace them all with
                     // the argument specified by ArgNo.
                     while(!GEP->use_empty())
                     {
                        LoadInst* L = cast<LoadInst>(GEP->user_back());
#if PRINT_DBG_MSG
                        for(const auto& user : L->users())
                        {
                           LLVM_DEBUG(llvm::dbgs() << "   before: " << user << "\n");
                        }
#endif
                        L->replaceAllUsesWith(TheArg);
#ifndef NDEBUG
                        for(const auto& user : TheArg->users())
                        {
                           LLVM_DEBUG(llvm::dbgs() << "   after: " << user << "\n");
                        }
#endif
                        L->eraseFromParent();
                     }
                     GEP->eraseFromParent();
                  }
               }
               // Increment I2 past all of the arguments added for this promoted pointer.
               if(ArgIndices)
               {
                  std::advance(I2, ArgIndices->size());
               }
            }
         }
         else
         {
            I->replaceAllUsesWith(&*I2);
            ++I2;
         }
      }
      function->setLinkage(Function::ExternalLinkage);
      if(hasArgToBePromoted)
      {
         // update argToPromote
         argToPromote[NF] = argToPromote.at(function);
         argToPromote.erase(function);
      }
      updatedFunction = NF;
      function->eraseFromParent();
      return true;
   }

   using IndicesVector = std::vector<uint64_t>;

   /// Returns true if Prefix is a prefix of longer. That means, Longer has a size
   /// that is greater than or equal to the size of prefix, and each of the
   /// elements in Prefix is the same as the corresponding elements in Longer.
   ///
   /// This means it also returns true when Prefix and Longer are equal!
   static bool IsPrefix(const IndicesVector& Prefix, const IndicesVector& Longer)
   {
      if(Prefix.size() > Longer.size())
         return false;
      return std::equal(Prefix.begin(), Prefix.end(), Longer.begin());
   }

   /// Checks if Indices, or a prefix of Indices, is in Set.
   static bool PrefixIn(const IndicesVector& Indices, std::set<IndicesVector>& Set)
   {
      std::set<IndicesVector>::iterator Low;
      Low = Set.upper_bound(Indices);
      if(Low != Set.begin())
         Low--;
      // Low is now the last element smaller than or equal to Indices. This means
      // it points to a prefix of Indices (possibly Indices itself), if such
      // prefix exists.
      //
      // This load is safe if any prefix of its operands is safe to load.
      return Low != Set.end() && IsPrefix(*Low, Indices);
   }

   /// Mark the given indices (ToMark) as safe in the given set of indices
   /// (Safe). Marking safe usually means adding ToMark to Safe. However, if there
   /// is already a prefix of Indices in Safe, Indices are implicitly marked safe
   /// already. Furthermore, any indices that Indices is itself a prefix of, are
   /// removed from Safe (since they are implicitly safe because of Indices now).
   static void MarkIndicesSafe(const IndicesVector& ToMark, std::set<IndicesVector>& Safe)
   {
      std::set<IndicesVector>::iterator Low;
      Low = Safe.upper_bound(ToMark);
      // Guard against the case where Safe is empty
      if(Low != Safe.begin())
         Low--;
      // Low is now the last element smaller than or equal to Indices. This
      // means it points to a prefix of Indices (possibly Indices itself), if
      // such prefix exists.
      if(Low != Safe.end())
      {
         if(IsPrefix(*Low, ToMark))
            // If there is already a prefix of these indices (or exactly these
            // indices) marked a safe, don't bother adding these indices
            return;

         // Increment Low, so we can use it as a "insert before" hint
         ++Low;
      }
      // Insert
      Low = Safe.insert(Low, ToMark);
      ++Low;
      // If there we're a prefix of longer index list(s), remove those
      std::set<IndicesVector>::iterator End = Safe.end();
      while(Low != End && IsPrefix(ToMark, *Low))
      {
         std::set<IndicesVector>::iterator Remove = Low;
         ++Low;
         Safe.erase(Remove);
      }
   }

   static bool canPaddingBeAccessed(Argument* arg)
   {
      assert(arg->hasByValAttr());

      // Track all the pointers to the argument to make sure they are not captured.
      SmallPtrSet<Value*, 16> PtrValues;
      PtrValues.insert(arg);

      // Track all of the stores.
      SmallVector<StoreInst*, 16> Stores;

      // Scan through the uses recursively to make sure the pointer is always used
      // sanely.
      SmallVector<Value*, 16> WorkList(arg->users());
      while(!WorkList.empty())
      {
         Value* V = WorkList.pop_back_val();
         if(isa<GetElementPtrInst>(V) || isa<PHINode>(V))
         {
            if(PtrValues.insert(V).second)
            {
               WorkList.insert(WorkList.end(), V->user_begin(), V->user_end());
            }
         }
         else if(StoreInst* Store = dyn_cast<StoreInst>(V))
         {
            Stores.push_back(Store);
         }
         else if(!isa<LoadInst>(V))
         {
            return true;
         }
      }

      // Check to make sure the pointers aren't captured
      for(StoreInst* Store : Stores)
      {
         if(PtrValues.count(Store->getValueOperand()))
         {
            return true;
         }
      }

      return false;
   }

   /// \brief Checks if a type could have padding bytes.
   static bool isDenselyPacked(Type* type, const DataLayout& DL)
   {
      // There is no size information, so be conservative.
      if(!type->isSized())
      {
         return false;
      }

      // If the alloc size is not equal to the storage size, then there are padding
      // bytes. For x86_fp80 on x86-64, size: 80 alloc size: 128.
      if(DL.getTypeSizeInBits(type) != DL.getTypeAllocSizeInBits(type))
      {
         return false;
      }

      // FIXME: This isn't the right way to check for padding in vectors with
      // non-byte-size elements.
      // For array types, check for padding within members.
      if(auto seqTy = dyn_cast<VectorType>(type))
      {
#if PANDA_LLVM_CLANG_MAJOR < 16
         return isDenselyPacked(seqTy->getElementType(), DL);
#else
         if(seqTy->isOpaquePointerTy())
         {
            LLVM_DEBUG(llvm::dbgs() << "OpenMP pluging does not support opaque pointers.\n");
            return false;
         }
         return isDenselyPacked(seqTy->getNonOpaquePointerElementType(), DL);
#endif
      }
      if(auto seqTy = dyn_cast<ArrayType>(type))
      {
#if PANDA_LLVM_CLANG_MAJOR < 16
         return isDenselyPacked(seqTy->getElementType(), DL);
#else
         if(seqTy->isOpaquePointerTy())
         {
            LLVM_DEBUG(llvm::dbgs() << "OpenMP pluging does not support opaque pointers.\n");
            return false;
         }
         return isDenselyPacked(seqTy->getNonOpaquePointerElementType(), DL);
#endif
      }

      if(!isa<StructType>(type))
      {
         return true;
      }

      // Check for padding within and between elements of a struct.
      StructType* StructTy = cast<StructType>(type);
      const StructLayout* Layout = DL.getStructLayout(StructTy);
      uint64_t StartPos = 0;
      for(unsigned i = 0, E = StructTy->getNumElements(); i < E; ++i)
      {
         Type* ElTy = StructTy->getElementType(i);
         if(!isDenselyPacked(ElTy, DL))
         {
            return false;
         }
         if(StartPos != Layout->getElementOffsetInBits(i))
         {
            return false;
         }
         StartPos += DL.getTypeAllocSizeInBits(ElTy);
      }

      return true;
   }

   static bool promoteArgumentsCheck(llvm::Function* F, uint64_t argIndex)
   {
      // Make sure that it is local to this module.
      if(!F || !F->hasLocalLinkage())
      {
         return false;
      }

      // Don't promote arguments for variadic functions. Adding, removing, or
      // changing non-pack parameters can change the classification of pack
      // parameters. Frontends encode that classification at the call site in the
      // IR, while in the callee the classification is determined dynamically based
      // on the number of registers consumed so far.
      if(F->isVarArg())
      {
         return false;
      }

#if PANDA_LLVM_CLANG_MAJOR >= 10
      auto Arg = F->getArg(argIndex);
#else
      Argument* Arg;
      uint64_t ind = 0;
      for(auto& a : F->args())
      {
         if(ind == argIndex)
         {
            Arg = &a;
            break;
         }
         ++ind;
      }
#endif
      if(!Arg->getType()->isPointerTy())
      {
         return false;
      }
      if(Arg->use_empty())
      {
         return true;
      }

      /// A vector used to hold the indices of a single GEP instruction
      using GEPIndicesSet = std::set<IndicesVector>;

      // This set will contain all sets of indices that are loaded in the entry
      // block, and thus are safe to unconditionally load in the caller.
      GEPIndicesSet SafeToUnconditionallyLoad;

      // This set contains all the sets of indices that we are planning to promote.
      // This makes it possible to limit the number of arguments added.
      GEPIndicesSet ToPromote;

      SafeToUnconditionallyLoad.insert(IndicesVector(1, 0));

      // First, iterate the entry block and mark loads of (geps of) arguments as
      // safe.
      BasicBlock& EntryBlock = Arg->getParent()->front();
      // Declare this here so we can reuse it
      IndicesVector Indices;
      for(Instruction& I : EntryBlock)
      {
         if(LoadInst* LI = dyn_cast<LoadInst>(&I))
         {
            Value* V = LI->getPointerOperand();
            if(GetElementPtrInst* GEP = dyn_cast<GetElementPtrInst>(V))
            {
               V = GEP->getPointerOperand();
               if(V == Arg)
               {
                  // This load actually loads (part of) Arg? Check the indices then.
                  Indices.reserve(GEP->getNumIndices());
                  for(User::op_iterator II = GEP->idx_begin(), IE = GEP->idx_end(); II != IE; ++II)
                  {
                     if(ConstantInt* CI = dyn_cast<ConstantInt>(*II))
                     {
                        Indices.push_back(CI->getSExtValue());
                     }
                     else
                     {
                        // We found a non-constant GEP index for this argument? Bail out
                        // right away, can't promote this argument at all.
                        return false;
                     }
                  }

                  // Indices checked out, mark them as safe
                  MarkIndicesSafe(Indices, SafeToUnconditionallyLoad);
                  Indices.clear();
               }
            }
            else if(V == Arg)
            {
               // Direct loads are equivalent to a GEP with a single 0 index.
               MarkIndicesSafe(IndicesVector(1, 0), SafeToUnconditionallyLoad);
            }
         }
      }

      // Now, iterate all uses of the argument to see if there are any uses that are
      // not (GEP+)loads, or any (GEP+)loads that are not safe to promote.
      SmallVector<LoadInst*, 16> Loads;
      IndicesVector Operands;
      for(Use& U : Arg->uses())
      {
         User* UR = U.getUser();
         Operands.clear();
         if(LoadInst* LI = dyn_cast<LoadInst>(UR))
         {
            // Don't hack volatile/atomic loads
            if(!LI->isSimple())
            {
               return false;
            }
            Loads.push_back(LI);
            // Direct loads are equivalent to a GEP with a zero index and then a load.
            Operands.push_back(0);
         }
         else if(auto GEP = dyn_cast<GetElementPtrInst>(UR))
         {
            if(GEP->use_empty())
            {
               // Dead GEP's cause trouble later.  Just remove them if we run into
               // them.
               GEP->eraseFromParent();
               // TODO: This runs the above loop over and over again for dead GEPs
               // Couldn't we just do increment the UI iterator earlier and erase the
               // use?
               //               return isSafeToPromoteArgument(Arg, isByValOrInAlloca, AAR,
               //                                              MaxElements);
               return false;
            }

            // Ensure that all of the indices are constants.
            for(User::op_iterator i = GEP->idx_begin(), e = GEP->idx_end(); i != e; ++i)
            {
               if(ConstantInt* C = dyn_cast<ConstantInt>(*i))
               {
                  Operands.push_back(C->getSExtValue());
               }
               else
               {
                  return false; // Not a constant operand GEP!
               }
            }

            // Ensure that the only users of the GEP are load instructions.
            for(User* GEPU : GEP->users())
            {
               if(LoadInst* LI = dyn_cast<LoadInst>(GEPU))
               {
                  // Don't hack volatile/atomic loads
                  if(!LI->isSimple())
                  {
                     return false;
                  }
                  Loads.push_back(LI);
               }
               else
               {
                  // Other uses than load?
                  return false;
               }
            }
         }
         else
         {
            return false; // Not a load or a GEP.
         }
         // Now, see if it is safe to promote this load / loads of this GEP. Loading
         // is safe if Operands, or a prefix of Operands, is marked as safe.
         if(!PrefixIn(Operands, SafeToUnconditionallyLoad))
         {
            return false;
         }
         // See if we are already promoting a load with these indices. If not, check
         // to make sure that we aren't promoting too many elements.  If so, nothing
         // to do.
         if(ToPromote.find(Operands) == ToPromote.end())
         {
            ToPromote.insert(std::move(Operands));
         }
      }

#if PANDA_LLVM_CLANG_MAJOR < 16
      Type* AgTy = cast<PointerType>(Arg->getType())->getElementType();
#else
      if(Arg->getType()->isOpaquePointerTy())
      {
         LLVM_DEBUG(llvm::dbgs() << "OpenMP pluging does not support opaque pointers.\n");
         return false;
      }
      Type* AgTy = cast<PointerType>(Arg->getType())->getNonOpaquePointerElementType();
#endif

      const auto& DL = F->getParent()->getDataLayout();
      const auto isSafeToPromote = Arg->hasByValAttr() && (isDenselyPacked(AgTy, DL) || !canPaddingBeAccessed(Arg));
      if(isSafeToPromote)
      {
         if(StructType* STy = dyn_cast<StructType>(AgTy))
         {
            // If all the elements are single-value types, we can promote it.
            bool AllSimple = true;
            for(const auto* EltTy : STy->elements())
            {
               if(!EltTy->isSingleValueType() && !(EltTy->isArrayTy() && EltTy->getArrayNumElements() != 0 &&
                                                   EltTy->getArrayElementType()->isSingleValueType()))
               {
                  AllSimple = false;
                  break;
               }
            }
            if(!AllSimple)
            {
               return false;
            }
         }
      }

      // Okay, now we know that the argument is only used by load instructions and
      // it is safe to unconditionally perform all of them.
      return true;
   }

   // Checks if the padding bytes of an argument could be accessed.

   static bool manage_fork_call(Module& M, bool computeArgToPromote,
                                std::map<Function*, std::set<uint64_t>>& argToPromote,
                                std::set<uint64_t> deadArgToPromote, std::set<Function*>& ompOutlinedList)
   {
      bool res = false;
      llvm::Function* bambuFun = M.getFunction(TOSTRING(KMP_FORK_CALL));
      if(!bambuFun && !computeArgToPromote)
      {
         auto origForkFunction = M.getFunction("__kmpc_fork_call");
         if(!origForkFunction)
         {
            llvm::report_fatal_error("__kmpc_fork_call not declared!");
         }
         llvm::SmallVector<llvm::Type*, sizeof(origForkFunction->arg_size() - 1)> ArgTys;
         unsigned argNo = 0;
         for(auto& V : origForkFunction->args())
         {
            if(argNo != 0)
            {
               if(argNo == 1 || argNo > 2)
               {
                  if(deadArgToPromote.find(argNo - 1) == deadArgToPromote.end())
                  {
                     ArgTys.push_back(V.getType());
                  }
               }
               else
               {
#if PANDA_LLVM_CLANG_MAJOR < 19
                  std::vector<Type*> Params;
                  auto ptrToFun = dyn_cast<PointerType>(V.getType());
                  auto ft = dyn_cast<FunctionType>(ptrToFun->getPointerElementType());
                  unsigned prmNum = 0;
                  for(auto prm : ft->params())
                  {
                     if(prmNum == 0 || prmNum == 1)
                     {
                        Params.push_back(prm->getPointerElementType());
                     }
                     else
                     {
                        Params.push_back(prm);
                     }
                     ++prmNum;
                  }
                  auto rt = ft->getReturnType();
                  FunctionType* nFTy = FunctionType::get(rt, Params, ft->isVarArg());
                  PointerType* nPTy = PointerType::get(nFTy, ptrToFun->getAddressSpace());
                  ArgTys.push_back(nPTy);
#endif
               }
            }
            ++argNo;
         }
         // Just get a void return type
         auto RetTy = llvm::Type::getVoidTy(M.getContext());
         auto FT = llvm::FunctionType::get(RetTy, ArgTys, true);

         bambuFun = llvm::Function::Create(FT, origForkFunction->getLinkage(), TOSTRING(KMP_FORK_CALL), &M);
      }
      std::vector<llvm::CallInst*> callInst_toReplace;
      for(auto& function : M.getFunctionList())
      {
         for(auto& bb : function)
         {
            for(auto& i : bb)
            {
               if(auto* call_inst = llvm::dyn_cast<llvm::CallInst>(&i))
               {
                  llvm::Function* cf = call_inst->getCalledFunction();
                  auto fork_callfFix = [&](llvm::Function* called_function) {
                     auto calledFunName = called_function->getName();
                     if(calledFunName == "__kmpc_fork_call")
                     {
                        LLVM_DEBUG(llvm::dbgs() << "  manage_fork_call Found: " << calledFunName << "\n");
                        callInst_toReplace.push_back(call_inst);
                        res = true;
                     }
                  };

                  if(cf)
                  {
                     fork_callfFix(cf);
                  }
                  else
                  {
#if PANDA_LLVM_CLANG_MAJOR >= 11
                     auto calledFun = call_inst->getCalledOperand();
#else
                     auto calledFun = call_inst->getCalledValue();
#endif
                     if(isa<llvm::ConstantExpr>(calledFun))
                     {
                        auto ce = cast<llvm::ConstantExpr>(calledFun);
                        if(ce->getOpcode() == llvm::Instruction::BitCast)
                        {
                           auto op = ce->getOperand(0);
                           if(isa<llvm::Function>(op))
                           {
                              fork_callfFix(cast<llvm::Function>(op));
                           }
                        }
                        else if(isa<llvm::Function>(calledFun))
                        {
                           fork_callfFix(cast<llvm::Function>(calledFun));
                        }
                     }
                  }
               }
            }
         }
      }
      for(auto& call_inst : callInst_toReplace)
      {
         llvm::SmallVector<Value*, 8> Args;
         unsigned int argNum = 0;
         llvm::IRBuilder<> Builder(call_inst);
         Function* omp_outlined = nullptr;
         for(auto It = call_inst->arg_begin(), ItEnd = call_inst->arg_end(); It != ItEnd; ++It, ++argNum)
         {
            if(argNum == 1 && !computeArgToPromote)
            {
               Args.push_back(*It);
            }
            else if(argNum == 2)
            {
               auto bc = dyn_cast<BitCastOperator>(*It);
               assert(bc);
               omp_outlined = dyn_cast<Function>(bc->getOperand(0));
               assert(omp_outlined);
               if(!computeArgToPromote)
               {
                  auto BFType = bambuFun->getFunctionType();
                  assert(BFType);
                  auto BCI = Builder.CreateBitCast(bc->getOperand(0), BFType->getParamType(1));
                  Args.push_back(BCI);
               }
               else
               {
                  ompOutlinedList.insert(omp_outlined);
               }
            }
            else if(argNum > 2)
            {
               if(computeArgToPromote)
               {
                  if((*It)->getValueID() == llvm::Value::InstructionVal + llvm::Instruction::Alloca &&
                     promoteArgumentsCheck(omp_outlined, argNum - 1))
                  {
                     argToPromote[omp_outlined].insert(argNum - 1);
                  }
               }
               else
               {
                  if(argToPromote.find(omp_outlined) != argToPromote.end() &&
                     argToPromote.find(omp_outlined)->second.find(argNum - 1) !=
                         argToPromote.find(omp_outlined)->second.end())
                  {
                     if(deadArgToPromote.find(argNum - 1) == deadArgToPromote.end())
                     {
#if PANDA_LLVM_CLANG_MAJOR < 16
                        Type* AgTy = cast<PointerType>((*It)->getType())->getElementType();
#else
                        if((*It)->getType()->isOpaquePointerTy())
                        {
                           LLVM_DEBUG(llvm::dbgs() << "OpenMP pluging does not support opaque pointers.\n");
                           return false;
                        }
                        Type* AgTy = cast<PointerType>((*It)->getType())->getNonOpaquePointerElementType();
#endif
                        if(AgTy->isStructTy())
                        {
                           StructType* STy = cast<StructType>(AgTy);
                           Value* Idxs[2] = {ConstantInt::get(Type::getInt32Ty(omp_outlined->getContext()), 0),
                                             nullptr};
                           std::vector<llvm::Value*> idxs(3, nullptr);
                           idxs[0] = Idxs[0];

                           for(unsigned i = 0, e = STy->getNumElements(); i != e; ++i)
                           {
                              Idxs[1] = ConstantInt::get(Type::getInt32Ty(omp_outlined->getContext()), i);
                              if(STy->getElementType(i)->isArrayTy())
                              {
                                 idxs[1] = Idxs[1];
                                 for(unsigned j = 0; j < STy->getElementType(i)->getArrayNumElements(); ++j)
                                 {
                                    idxs[2] = ConstantInt::get(Type::getInt32Ty(omp_outlined->getContext()), j);
                                    auto* Idx =
                                        Builder.CreateInBoundsGEP(STy, *It, idxs, (*It)->getName() + "." + Twine(i));
                                    // TODO: Tell AA about the new values?
                                    Args.push_back(Builder.CreateLoad(STy->getElementType(i)->getArrayElementType(),
                                                                      Idx, Idx->getName() + ".val"));
                                 }
                              }
                              else
                              {
                                 auto* Idx = Builder.CreateGEP(STy, *It, Idxs, (*It)->getName() + "." + Twine(i));
                                 // TODO: Tell AA about the new values?
                                 Args.push_back(
                                     Builder.CreateLoad(STy->getElementType(i), Idx, Idx->getName() + ".val"));
                              }
                           }
                        }
                        else
                        {
                           LoadInst* newLoad = Builder.CreateLoad((*It)->getType(), *It, (*It)->getName() + ".val");
                           Args.push_back(newLoad);
                        }
                     }
                  }
                  else
                  {
                     Args.push_back(*It);
                  }
               }
            }
         }
         if(!computeArgToPromote)
         {
            llvm::CallInst* NewCI = CallInst::Create(bambuFun, Args);
            NewCI->setCallingConv(bambuFun->getCallingConv());
            assert(call_inst->use_empty());
            llvm::ReplaceInstWithInst(call_inst, NewCI);
         }
      }
      return res;
   }
} // namespace llvm

namespace llvm
{
   static cl::opt<unsigned> dflt_nthreads("dflt_nthreads", cl::init(4),
                                          cl::desc("Specify the default number of threads"));

   struct openmpBambu : public ModulePass
#if PANDA_LLVM_CLANG_MAJOR >= 13
       ,
                        public PassInfoMixin<openmpBambu>
#endif
   {
      static char ID;

      openmpBambu() : ModulePass(ID)
      {
         initializeLoopPassPass(*PassRegistry::getPassRegistry());
      }

#if PANDA_LLVM_CLANG_MAJOR >= 13
      openmpBambu(const openmpBambu&) : openmpBambu()
      {
      }
#endif

      std::string getDemangled(const std::string& declname)
      {
         int status;
         char* demangled_outbuffer = abi::__cxa_demangle(declname.c_str(), nullptr, nullptr, &status);
         if(status == 0)
         {
            std::string res = declname;
            if(std::string(demangled_outbuffer).find_last_of('(') != std::string::npos)
            {
               res = demangled_outbuffer;
               auto parPos = res.find('(');
               assert(parPos != std::string::npos);
               res = res.substr(0, parPos);
            }
            free(demangled_outbuffer);
            return res;
         }

         assert(demangled_outbuffer == nullptr);

         return declname;
      }

      bool exec(Module& M)
      {
         bool changed = false;
         auto default_nt = llvm::ConstantInt::get(M.getContext(), llvm::APInt(32, dflt_nthreads));
         std::set<Function*> ompOutlinedList;
         std::map<Function*, std::set<uint64_t>> argToPromote;
         std::set<uint64_t> deadArgToPromote;
         manage_fork_call(M, true, argToPromote, deadArgToPromote, ompOutlinedList);
         for(auto& fun : M.getFunctionList())
         {
            if(fun.isIntrinsic() || fun.isDeclaration())
            {
               LLVM_DEBUG(llvm::dbgs() << "Function intrinsic/declaration skipped: " << fun.getName() << "\n");
            }
            else
            {
               LLVM_DEBUG(llvm::dbgs() << "Found function: " << fun.getName() << "|"
                                       << getDemangled(fun.getName().data()) << "\n");
               auto res_propagate_nproc = propagate_nproc(fun, default_nt);
               changed = res_propagate_nproc || changed;
               auto res_remove_openmp_stack_intrinsics = remove_openmp_stack_intrinsics(fun);
               changed = res_remove_openmp_stack_intrinsics || changed;
            }
         }
         for(auto F : ompOutlinedList)
         {
            auto res_remove_openmp_lifetime = remove_openmp_lifetime(*F);
            changed = res_remove_openmp_lifetime || changed;
         }
         std::set<Function*> updatedOutlinedList;
         for(auto F : ompOutlinedList)
         {
            Function* updatedF = F;
            auto res_fix_omp_outline_parameters =
                fix_omp_outline_parameters(F, argToPromote, deadArgToPromote, updatedF);
            changed = res_fix_omp_outline_parameters || changed;
            updatedOutlinedList.insert(updatedF);
         }
         ompOutlinedList = std::move(updatedOutlinedList);

         auto res_manage_fork_call = manage_fork_call(M, false, argToPromote, deadArgToPromote, ompOutlinedList);
         changed = res_manage_fork_call || changed;
         return changed;
      }

      bool runOnModule(Module& M) override
      {
         return exec(M);
      }

      StringRef getPassName() const override
      {
         return "openmpBambu";
      }

      void getAnalysisUsage(AnalysisUsage& AU) const override
      {
         getLoopAnalysisUsage(AU);
      }

#if PANDA_LLVM_CLANG_MAJOR >= 13
      PreservedAnalyses run(Module& M, ModuleAnalysisManager&)
      {
         const auto changed = exec(M);
         return (changed ? PreservedAnalyses::none() : PreservedAnalyses::all());
      }
#endif
   };

   char openmpBambu::ID = 0;

} // namespace llvm

#ifndef _WIN32
static llvm::RegisterPass<llvm::openmpBambu> XPass("openmpBambu",
                                                   "Make all transformations required by openmp bambu runtime",
                                                   false /* Only looks at CFG */, false /* Analysis Pass */);
#endif

#if PANDA_LLVM_CLANG_MAJOR >= 13
llvm::PassPluginLibraryInfo getopenmpBambuPluginInfo()
{
   return {LLVM_PLUGIN_API_VERSION, "openmpBambu", "v0.12", [](llvm::PassBuilder& PB) {
              const auto load = [](llvm::ModulePassManager& MPM) {
                 MPM.addPass(llvm::openmpBambu());
                 return true;
              };
              PB.registerPipelineParsingCallback([&](llvm::StringRef Name, llvm::ModulePassManager& MPM,
                                                     llvm::ArrayRef<llvm::PassBuilder::PipelineElement>) {
                 if(Name == "openmpBambu")
                 {
                    return load(MPM);
                 }
                 return false;
              });
              PB.registerPipelineEarlySimplificationEPCallback([&](llvm::ModulePassManager& MPM,
#if PANDA_LLVM_CLANG_MAJOR < 16
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
   return getopenmpBambuPluginInfo();
}
#else
#if ADD_RSP
// This function is of type PassManagerBuilder::ExtensionFn
static void loadPass(const llvm::PassManagerBuilder&, llvm::legacy::PassManagerBase& PM)
{
   PM.add(new llvm::openmpBambu());
}

// These constructors add our pass to a list of global extensions.
static llvm::RegisterStandardPasses openmpBambu_Ox(llvm::PassManagerBuilder::EP_ModuleOptimizerEarly, loadPass);
#endif
#endif
