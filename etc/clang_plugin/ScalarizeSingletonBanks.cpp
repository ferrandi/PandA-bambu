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
 * @file scalarizeSingletonBanks.cpp
 * @brief Turn single-cell aggregate banks into scalar allocas so mem2reg can promote them.
 *
 * CSROA emits one bank per cell when an array is partitioned completely, and each bank keeps
 * the element type wrapped in the singleton array and in the single-member structs that came
 * from the C++ type:
 *
 *     %81   = alloca [1 x %class.ac_fixed.3], align 16   ; %class.ac_fixed.3 = {{{ i37 }}}
 *     %1178 = getelementptr [1 x %class.ac_fixed.3], ..., i32 0, i32 0, i32 0, i32 0, i32 0
 *     store i37 %1177, i37* %1178, align 16
 *     ...
 *     %8157 = getelementptr [1 x %class.ac_fixed.3], ..., i32 0, i32 0, i32 0, i32 0, i32 0
 *     %8158 = load i37, i37* %8157, align 16
 *
 * Nothing upstream promotes that. mem2reg needs the accessed type to equal the allocated type
 * and refuses the aggregate; SROA bails on any integer whose bit width differs from its store
 * size in bits (isIntegerWideningViable, llvm/lib/Transforms/Scalar/SROA.cpp) and degrades the
 * alloca to [5 x i8], which mem2reg can no longer touch. Every surviving alloca then becomes
 * a memory object in bambu - 4412 of them on issue1b_plain_backend_timeout.
 *
 * Peeling the wrappers is enough: `alloca i37` with direct accesses is promotable
 */
#ifndef NDEBUG
#define NDEBUG
#endif

#include "ScalarizeSingletonBanks.hpp"
#include "panda_clang_compat.hpp"

#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/InstIterator.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/IntrinsicInst.h"
#include "llvm/IR/Intrinsics.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Type.h"
#include "llvm/Support/raw_ostream.h"

#include "debug_print.hpp"

#include <vector>

#define PREFIX "[SCALARIZE BANKS] "

// setAlignment takes an unsigned up to LLVM 9, a MaybeAlign on LLVM 10 and an Align since 11.
#if PANDA_LLVM_CLANG_MAJOR <= 9
#define BANKS_GET_ALIGN(alloca) alloca->getAlignment()
#elif PANDA_LLVM_CLANG_MAJOR == 10
#define BANKS_GET_ALIGN(alloca) llvm::MaybeAlign(alloca->getAlignment())
#else
#define BANKS_GET_ALIGN(alloca) alloca->getAlign()
#endif

using namespace llvm;

namespace
{
   /// Peel singleton array wrappers and single-member structs off `ty`. Returns the scalar
   /// leaf, or null when `ty` is not a chain of singleton wrappers around a scalar. A struct
   /// with more than one member, an array with more than one element and an opaque struct all
   /// stop the walk: those must keep their aggregate identity.
   Type* peelToScalarLeaf(Type* ty)
   {
      for(;;)
      {
         if(auto* arrTy = dyn_cast<ArrayType>(ty))
         {
            if(arrTy->getNumElements() != 1)
            {
               return nullptr;
            }
            ty = arrTy->getElementType();
         }
         else if(auto* structTy = dyn_cast<StructType>(ty))
         {
            if(structTy->isOpaque() || structTy->getNumElements() != 1)
            {
               return nullptr;
            }
            ty = structTy->getElementType(0);
         }
         else
         {
            break;
         }
      }
      return (ty->isIntegerTy() || ty->isFloatingPointTy()) ? ty : nullptr;
   }

   /// The lifetime intrinsics are the only calls allowed to see a bank's address: they carry
   /// no data and are dropped along with the aggregate alloca. isLifetimeStartOrEnd() only
   /// exists from LLVM 9 on, hence the explicit ids.
   bool isLifetimeMarker(const User* user)
   {
      const auto* intrinsic = dyn_cast<IntrinsicInst>(user);
      return intrinsic != nullptr && (intrinsic->getIntrinsicID() == Intrinsic::lifetime_start ||
                                      intrinsic->getIntrinsicID() == Intrinsic::lifetime_end);
   }

   /// True when every access through `gep` is a plain, non-volatile, non-atomic load or store
   /// of `leafTy`. A store whose *value* operand is the pointer would let the address escape,
   /// so it is rejected even though its type would match.
   bool onlyScalarAccesses(const GetElementPtrInst* gep, const Type* leafTy)
   {
      for(const User* user : gep->users())
      {
         if(const auto* LD = dyn_cast<LoadInst>(user))
         {
            if(LD->getType() != leafTy || LD->isVolatile() || LD->isAtomic())
            {
               return false;
            }
         }
         else if(const auto* ST = dyn_cast<StoreInst>(user))
         {
            if(ST->getPointerOperand() != gep || ST->getValueOperand()->getType() != leafTy ||
               ST->isVolatile() || ST->isAtomic())
            {
               return false;
            }
         }
         else
         {
            return false;
         }
      }
      return true;
   }

   /// Everything that has to be erased once `AI` has been replaced, gathered before any
   /// mutation so no use list is walked while it changes.
   struct BankUses
   {
      std::vector<GetElementPtrInst*> geps;
      std::vector<Instruction*> markers; ///< lifetime intrinsics, erased first
      std::vector<Instruction*> casts;   ///< the i8* bitcasts they were fed through
   };

   /// Collect the uses of `AI` if it is a single-cell bank whose every access is a plain
   /// load/store of its scalar leaf, and return the leaf type. Returns null otherwise, and
   /// then `uses` must be ignored.
   Type* classifyBank(AllocaInst* AI, BankUses& uses)
   {
      if(AI->isArrayAllocation())
      {
         return nullptr; // a dynamic element count is not a single cell
      }

      Type* leafTy = peelToScalarLeaf(AI->getAllocatedType());
      if(leafTy == nullptr || leafTy == AI->getAllocatedType())
      {
         return nullptr; // nothing to peel: mem2reg already handles this shape
      }

      SmallPtrSet<User*, 8> seen;
      for(User* user : AI->users())
      {
         if(!seen.insert(user).second)
         {
            continue;
         }
         if(auto* gep = dyn_cast<GetElementPtrInst>(user))
         {
            if(!gep->hasAllZeroIndices() || gep->getResultElementType() != leafTy ||
               !onlyScalarAccesses(gep, leafTy))
            {
               return nullptr;
            }
            uses.geps.push_back(gep);
         }
         else if(isLifetimeMarker(user))
         {
            uses.markers.push_back(cast<Instruction>(user));
         }
         else if(auto* cast_inst = dyn_cast<BitCastInst>(user))
         {
            // Typed-pointer IR feeds the lifetime intrinsics through an i8* bitcast. Any
            // other bitcast reinterprets the cell and must stop the transformation.
            for(User* castUser : cast_inst->users())
            {
               if(!isLifetimeMarker(castUser))
               {
                  return nullptr;
               }
               uses.markers.push_back(cast<Instruction>(castUser));
            }
            uses.casts.push_back(cast_inst);
         }
         else
         {
            return nullptr;
         }
      }

      return uses.geps.empty() ? nullptr : leafTy;
   }

   /// Replace `AI` with an alloca of `leafTy` and drop the no-op GEPs that reached it.
   void scalarizeBank(AllocaInst* AI, Type* leafTy, BankUses& uses)
   {
      IRBuilder<> b(AI);
      auto* scalar = b.CreateAlloca(leafTy, nullptr, AI->getName() + ".scalar");
      scalar->setAlignment(BANKS_GET_ALIGN(AI));
      scalar->setDebugLoc(AI->getDebugLoc());

      for(GetElementPtrInst* gep : uses.geps)
      {
         gep->replaceAllUsesWith(scalar);
         gep->eraseFromParent();
      }
      // The intrinsics have to go before the bitcasts that feed them.
      for(Instruction* marker : uses.markers)
      {
         marker->eraseFromParent();
      }
      for(Instruction* cast_inst : uses.casts)
      {
         cast_inst->eraseFromParent();
      }
      AI->eraseFromParent();
   }
} // namespace

char ScalarizeSingletonBanksPass::ID = 0;

bool ScalarizeSingletonBanksPass::exec(Module& M)
{
   bool changed = false;
   for(Function& F : M)
   {
      // Collect first: scalarizeBank erases instructions.
      std::vector<AllocaInst*> allocas;
      for(Instruction& I : instructions(F))
      {
         if(auto* AI = dyn_cast<AllocaInst>(&I))
         {
            allocas.push_back(AI);
         }
      }

      // A completely partitioned array yields thousands of banks, so this reports one line
      // per function instead of one per alloca.
      size_t scalarized = 0;
      for(AllocaInst* AI : allocas)
      {
         BankUses uses;
         if(Type* leafTy = classifyBank(AI, uses))
         {
            scalarizeBank(AI, leafTy, uses);
            ++scalarized;
            changed = true;
         }
      }
      LLVM_DEBUG({
         if(!allocas.empty())
         {
            dbgs() << PREFIX << "@" << F.getName() << ": scalarized " << scalarized << " of " << allocas.size()
                   << " allocas\n";
         }
      });
   }
   return changed;
}

PreservedAnalyses ScalarizeSingletonBanksPass::run(Module& M, ModuleAnalysisManager& /*AM*/)
{
   return exec(M) ? PreservedAnalyses::none() : PreservedAnalyses::all();
}

bool ScalarizeSingletonBanksPass::runOnModule(Module& M)
{
   return exec(M);
}

StringRef ScalarizeSingletonBanksPass::getPassName() const
{
   return "ScalarizeSingletonBanksPass";
}

void ScalarizeSingletonBanksPass::getAnalysisUsage(AnalysisUsage& /*AU*/) const
{
}
