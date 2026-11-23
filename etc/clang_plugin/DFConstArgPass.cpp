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
 * @file DFConstArgPass.cpp
 * @brief Propagation of constant arguments into the dataflow modules.
 *
 * @author Francesco Bazzano <francesco.bazzano@mail.polimi.it>
 *
 */
#ifndef NDEBUG
#define NDEBUG
#endif

#include "DFConstArgPass.hpp"

#if LLVM_VERSION_MAJOR >= 13
#include "ArrPart.hpp"

#include "llvm/ADT/STLExtras.h"
#include "llvm/ADT/SmallPtrSet.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/Analysis/ValueTracking.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/GlobalVariable.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/InstrTypes.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Operator.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Transforms/Utils/Cloning.h"

#include "debug_print.hpp"

#include <pugixml.hpp>
#include <string>
#include <vector>

using namespace llvm;

namespace
{
   // True if the memory reached through V is only read. Only what hls4ml generates for weights and biases is
   // recognized: loads, address computations, the partition marker and the pointer passed down to a defined
   // function. Anything else gives false, which only means that the argument is not propagated.
   bool onlyRead(const Value* V)
   {
      SmallVector<const Value*, 16> worklist{V};
      SmallPtrSet<const Value*, 16> visited{V};
      while(!worklist.empty())
      {
         for(const auto& U : worklist.pop_back_val()->uses())
         {
            const auto* user = U.getUser();
            const Value* next = nullptr;
            if(isa<LoadInst>(user))
            {
               continue;
            }
            // instructions and constant expressions alike
            if(isa<GEPOperator>(user) || isa<BitCastOperator>(user))
            {
               next = user;
            }
            else if(const auto* CB = dyn_cast<CallBase>(user); CB && CB->isArgOperand(&U))
            {
               const auto* callee = CB->getCalledFunction();
               const auto arg_i = CB->getArgOperandNo(&U);
               // The ARRAY_PARTITION marker only tells CSROA how to split the array, it does not access it.
               if(callee && callee->getName() == BAMBU_CSROA_PARTITION_FUN_NAME)
               {
                  continue;
               }
               if(callee && !callee->isDeclaration() && arg_i < callee->arg_size())
               {
                  next = callee->getArg(arg_i);
               }
            }
            if(!next)
            {
               return false;
            }
            if(visited.insert(next).second)
            {
               worklist.push_back(next);
            }
         }
      }
      return true;
   }

   bool isEligible(Constant* C)
   {
      if(isa<ConstantInt>(C) || isa<ConstantFP>(C))
      {
         return true;
      }
      if(!C->getType()->isPointerTy() || isa<ConstantPointerNull>(C) || isa<UndefValue>(C))
      {
         return false;
      }
      const auto* GV = dyn_cast<GlobalVariable>(getUnderlyingObject(C));
      if(!GV || !GV->hasDefinitiveInitializer())
      {
         return false;
      }
      if(GV->isConstant())
      {
         return true;
      }
      return GV->hasLocalLinkage() && onlyRead(GV);
   }
} // namespace

namespace llvm
{
   bool DFConstArgPass::isTop(const Function& F) const
   {
      SmallVector<StringRef, 4> tops;
      StringRef(topFnNames).split(tops, ',', -1, false);
      const auto name = F.getName();
      const auto demangled = getDemangled(name.str());
      return llvm::any_of(tops, [&](StringRef top) { return top == name || top == demangled; });
   }

   PreservedAnalyses DFConstArgPass::run(Module& M, ModuleAnalysisManager&)
   {
      pugi::xml_document doc;
      if(!loadXMLModule(doc, outdirName))
      {
         return PreservedAnalyses::all();
      }

      // Same roots as topfname: every dataflow top or module, but the top functions themselves.
      std::vector<Function*> dataflow_functions;
      for(auto& f : doc.child("module").children("function"))
      {
         const auto is_dataflow = !f.attribute("dataflow_top").empty() || !f.attribute("dataflow_module").empty();
         auto* F = M.getFunction(f.attribute("symbol").value());
         if(is_dataflow && F && !F->isDeclaration() && !isTop(*F) && !llvm::is_contained(dataflow_functions, F))
         {
            dataflow_functions.push_back(F);
         }
      }

      // Repeated until nothing changes, so that a constant propagated into a dataflow top reaches the
      // modules it calls in turn.
      bool changed = false;
      bool changed_any = false;
      do
      {
         changed_any = false;
         // By reference: F is replaced by the new function without the propagated arguments.
         for(auto*& F : dataflow_functions)
         {
            if(F->hasAddressTaken())
            {
               continue;
            }
            std::vector<CallBase*> calls;
            auto same_type = true;
            for(auto& U : F->uses())
            {
               if(auto* CB = dyn_cast<CallBase>(U.getUser()); CB && CB->isCallee(&U))
               {
                  calls.push_back(CB);
                  same_type &= CB->getFunctionType() == F->getFunctionType();
               }
            }
            if(calls.empty() || !same_type)
            {
               continue;
            }

            SmallVector<Type*, 8> params;
            SmallVector<unsigned, 8> removed_args;
            for(auto& arg : F->args())
            {
               if(arg.use_empty() || arg.hasPassPointeeByValueCopyAttr() || arg.hasStructRetAttr())
               {
                  params.push_back(arg.getType());
                  continue;
               }
               const auto arg_i = arg.getArgNo();
               auto* C = dyn_cast<Constant>(calls.front()->getArgOperand(arg_i));
               const auto same_constant =
                   C && llvm::all_of(calls, [&](const CallBase* CB) { return CB->getArgOperand(arg_i) == C; });
               if(!same_constant || !isEligible(C))
               {
                  params.push_back(arg.getType());
                  continue;
               }
               LLVM_DEBUG(dbgs() << "[CSROA] Argument " << arg_i << " of dataflow module " << F->getName()
                                 << " propagated: " << *C << "\n");
               arg.replaceAllUsesWith(C);
               removed_args.push_back(arg_i);
               changed_any = true;
            }

            if(removed_args.empty())
            {
               continue;
            }

            // New function without the propagated arguments. Until F is erased the name is taken, so LLVM gives
            // NewF a temporary unique name; NewF takes the name of F below.
            FunctionType* FTy = FunctionType::get(F->getReturnType(), params, F->isVarArg());
            Function* NewF = Function::Create(FTy, F->getLinkage(), F->getName(), M);

            // Every argument of F must be in VMap: the kept ones go to the new arguments, the removed ones (which
            // no longer have uses) to poison.
            ValueToValueMapTy VMap;
            auto new_arg = NewF->arg_begin();
            for(auto& arg : F->args())
            {
               if(llvm::is_contained(removed_args, arg.getArgNo()))
               {
                  VMap[&arg] = PoisonValue::get(arg.getType());
               }
               else
               {
                  new_arg->setName(arg.getName());
                  VMap[&arg] = &*new_arg++;
               }
            }
            SmallVector<ReturnInst*, 8> returns;
            CloneFunctionInto(NewF, F, VMap, CloneFunctionChangeType::LocalChangesOnly, returns);

            // Every call to F becomes a call to NewF without the removed arguments.
            for(auto* CB : calls)
            {
               const auto PAL = CB->getAttributes();
               SmallVector<Value*, 8> args;
               SmallVector<AttributeSet, 8> arg_attrs;
               for(unsigned i = 0; i < CB->arg_size(); ++i)
               {
                  if(llvm::is_contained(removed_args, i))
                  {
                     continue;
                  }
                  args.push_back(CB->getArgOperand(i));
#if LLVM_VERSION_MAJOR >= 14
                  arg_attrs.push_back(PAL.getParamAttrs(i));
#else
                  arg_attrs.push_back(PAL.getParamAttributes(i));
#endif
               }
               SmallVector<OperandBundleDef, 1> bundles;
               CB->getOperandBundlesAsDefs(bundles);

               IRBuilder<> B(CB);
               CallBase* NewCB;
               if(auto* II = dyn_cast<InvokeInst>(CB))
               {
                  NewCB = B.CreateInvoke(NewF, II->getNormalDest(), II->getUnwindDest(), args, bundles);
               }
               else
               {
                  auto* CI = B.CreateCall(NewF, args, bundles);
                  CI->setTailCallKind(cast<CallInst>(CB)->getTailCallKind());
                  NewCB = CI;
               }
               NewCB->setCallingConv(CB->getCallingConv());
#if LLVM_VERSION_MAJOR >= 14
               NewCB->setAttributes(AttributeList::get(M.getContext(), PAL.getFnAttrs(), PAL.getRetAttrs(), arg_attrs));
#else
               NewCB->setAttributes(
                   AttributeList::get(M.getContext(), PAL.getFnAttributes(), PAL.getRetAttributes(), arg_attrs));
#endif
               NewCB->setDebugLoc(CB->getDebugLoc());
               NewCB->takeName(CB);
               CB->replaceAllUsesWith(NewCB);
               CB->eraseFromParent();
            }

            // NewF replaces F under the same name.
            NewF->takeName(F);
            F->eraseFromParent();
            F = NewF;
         }
         changed |= changed_any;
      } while(changed_any);

      return changed ? PreservedAnalyses::none() : PreservedAnalyses::all();
   }
} // namespace llvm
#endif
