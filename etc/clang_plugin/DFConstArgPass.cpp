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
#include "llvm/IR/InstrTypes.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Operator.h"
#include "llvm/Support/raw_ostream.h"

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
         for(auto* F : dataflow_functions)
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

            for(auto& arg : F->args())
            {
               // use_empty: already propagated, the call sites now pass null.
               if(arg.use_empty() || arg.hasPassPointeeByValueCopyAttr() || arg.hasStructRetAttr())
               {
                  continue;
               }
               const auto arg_i = arg.getArgNo();
               auto* C = dyn_cast<Constant>(calls.front()->getArgOperand(arg_i));
               const auto same_constant =
                   C && llvm::all_of(calls, [&](const CallBase* CB) { return CB->getArgOperand(arg_i) == C; });
               if(!same_constant || !isEligible(C))
               {
                  continue;
               }
               LLVM_DEBUG(dbgs() << "[CSROA] Argument " << arg_i << " of dataflow module " << F->getName()
                                 << " propagated: " << *C << "\n");
               arg.replaceAllUsesWith(C);
               // The call sites pass null so that the top no longer uses the global. Once the argument has become
               // null, keeping nonnull and dereferenceable (clang puts them on a const T& parameter) is UB, so we
               // remove them from the definition and from every call.
               for(const auto kind : {Attribute::NonNull, Attribute::Dereferenceable})
               {
                  F->removeParamAttr(arg_i, kind);
                  for(auto* CB : calls)
                  {
                     CB->removeParamAttr(arg_i, kind);
                  }
               }
               for(auto* CB : calls)
               {
                  CB->setArgOperand(arg_i, Constant::getNullValue(arg.getType()));
               }
               changed_any = true;
            }
         }
         changed |= changed_any;
      } while(changed_any);

      return changed ? PreservedAnalyses::none() : PreservedAnalyses::all();
   }
} // namespace llvm
#endif
