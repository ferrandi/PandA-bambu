/* Copyright (C) 2026 Politecnico di Milano
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */
#ifndef PANDA_CHANNEL_SEAM_AA_HPP
#define PANDA_CHANNEL_SEAM_AA_HPP

#include "panda_clang_compat.hpp"
#include <llvm/Analysis/AliasAnalysis.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/Operator.h>
#if PANDA_LLVM_CLANG_MAJOR < 8
#include <llvm/IR/CallSite.h>
#endif

namespace bambu_channel_seam
{
   /// Call-vs-call AA may use same-iteration facts (including PHI correlation).
   /// Even MayBeCrossIteration on LLVM 16/19 does not reject all such cases.
   /// Only use it for pointers whose SSA value cannot change across iterations.
   /// Entry-block instructions execute once per invocation; constant-offset GEPs
   /// and pointer casts preserve this property. PHIs, selects and loads in other
   /// blocks are deliberately rejected, even when a stronger analysis could prove
   /// them invariant. This also covers irreducible cycles, without relying on LI.
   inline bool hasIterationInvariantAddress(const llvm::Value* V)
   {
      while(const auto* I = llvm::dyn_cast<llvm::Instruction>(V))
      {
         if(I->getParent() == &I->getFunction()->getEntryBlock())
         {
            return true;
         }
         if(I->getOpcode() == llvm::Instruction::BitCast || I->getOpcode() == llvm::Instruction::AddrSpaceCast)
         {
            V = I->getOperand(0);
         }
         else if(const auto* GEP = llvm::dyn_cast<llvm::GEPOperator>(I))
         {
            if(!GEP->hasAllConstantIndices())
            {
               return false;
            }
            V = GEP->getPointerOperand();
         }
         else
         {
            return false;
         }
      }
      return llvm::isa<llvm::Argument>(V) || llvm::isa<llvm::Constant>(V);
   }

   inline bool hasIterationInvariantPointerArgs(const llvm::CallInst* C)
   {
#if PANDA_LLVM_CLANG_MAJOR >= 14
      const auto args = C->args();
#else
      const auto args = C->arg_operands();
#endif
      for(const auto& Arg : args)
      {
         if(Arg->getType()->isPointerTy() && !hasIterationInvariantAddress(Arg.get()))
         {
            return false;
         }
      }
      return true;
   }

   /// Both calls must already be recognized hardware seams. The serializer walks
   /// MemoryPhi backedges, so independence must hold across iterations as well as
   /// within one iteration. Include every pointer argument (channel, valid, sret).
   inline bool callsModRefIndependentAcrossIterations(const llvm::CallInst* A, const llvm::CallInst* B,
                                                     llvm::AAResults& AA)
   {
      if(!hasIterationInvariantPointerArgs(A) || !hasIterationInvariantPointerArgs(B))
      {
         return false;
      }
#if PANDA_LLVM_CLANG_MAJOR >= 14
      llvm::SimpleAAQueryInfo AAQI(AA);
      const auto ab = AA.getModRefInfo(A, B, AAQI);
      const auto ba = AA.getModRefInfo(B, A, AAQI);
#elif PANDA_LLVM_CLANG_MAJOR >= 8
      const auto ab = AA.getModRefInfo(A, B);
      const auto ba = AA.getModRefInfo(B, A);
#else
      const auto ab = AA.getModRefInfo(llvm::ImmutableCallSite(A), llvm::ImmutableCallSite(B));
      const auto ba = AA.getModRefInfo(llvm::ImmutableCallSite(B), llvm::ImmutableCallSite(A));
#endif
#if PANDA_LLVM_CLANG_MAJOR < 6
      return ab == llvm::MRI_NoModRef && ba == llvm::MRI_NoModRef;
#else
      return ab == llvm::ModRefInfo::NoModRef && ba == llvm::ModRefInfo::NoModRef;
#endif
   }
}

#endif
