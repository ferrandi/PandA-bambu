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
 *   Copyright (C) 2026 Politecnico di Milano
 *
 * Part of the PandA Project, under the Apache License v2.0 with LLVM Exceptions.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 *
 */
/**
 * @file ac_channel_cross_iteration_aa.hpp
 * @brief Tells whether two ac_channel hardware-primitive calls may be proven
 * memory independent, which is what allows to drop the ordering between them.
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 *
 * dumpBambuIr serializes the MemorySSA reaching-def chain of every memory access,
 * turning each def it reaches into a vuse. When a def is another ac_channel
 * hardware-primitive call - read, write, peek - and the alias analysis proves that
 * the two calls may not mod/ref any common memory, that ordering is spurious: the
 * two channels are independent hardware objects, so the edge is not serialized and
 * the obtained IR does not force the two accesses to be interleaved.
 *
 * That decision must hold for every iteration of the enclosing loop, not only for
 * the one the alias analysis happened to look at, because the serializer also
 * walks MemoryPhi backedges. LLVM may answer NoModRef using same-iteration facts:
 * a PHI or a select may carry different pointers in different iterations, and a
 * stronger analysis may prove them independent now while they may alias later.
 * This header therefore rejects every pointer whose SSA value could change from
 * one iteration to the next, and queries the alias analysis only when all the
 * pointer arguments of both calls - the channel, the valid bit, the sret result -
 * are iteration invariant, which means:
 *
 * - a function argument or a constant;
 * - an instruction of the entry block, which executes once per invocation;
 * - a constant-offset GEP or a pointer cast of one of the above.
 *
 * PHIs, selects and loads in any other block are rejected even when a stronger
 * analysis could prove them invariant: this is a deliberate, LLVM-version
 * independent bound, so that the decision never depends on which facts a given
 * toolchain happens to use. LLVM's own MayBeCrossIteration does not reject all of
 * these cases even on LLVM 16/19, and the bound imposed here is syntactic, so it
 * also covers irreducible cycles without relying on loop analysis.
 *
 * The API is header-only and version dependent: the call-vs-call mod/ref query is
 * ImmutableCallSite based below LLVM 8, plain from 8 to 13, and SimpleAAQueryInfo
 * based from 14 on; the NoModRef sentinel differs below LLVM 6. panda_clang_compat
 * supplies the PANDA_LLVM_CLANG_MAJOR macro this file branches on.
 */
#ifndef PANDA_AC_CHANNEL_CROSS_ITERATION_AA_HPP
#define PANDA_AC_CHANNEL_CROSS_ITERATION_AA_HPP

#include "panda_clang_compat.hpp"
#include <llvm/Analysis/AliasAnalysis.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/Operator.h>
#if PANDA_LLVM_CLANG_MAJOR < 8
#include <llvm/IR/CallSite.h>
#endif

namespace bambu_ac_channel_primitives
{
   /// True when V denotes a pointer whose SSA value cannot change from one iteration
   /// to the next: an argument or a constant, an instruction of the entry block -
   /// which executes once per invocation - or a constant-offset GEP and a pointer
   /// cast of one of those. PHIs, selects and loads in any other block are rejected,
   /// even when a stronger analysis could prove them invariant, so that the answer
   /// does not depend on the facts a given LLVM version happens to use.
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

   /// True when every pointer argument of the call - the channel, a valid-bit output,
   /// an sret result - is iteration invariant. Only such calls are queried at all:
   /// when a pointer is recomputed in the loop, the alias analysis may answer from
   /// the values it happens to see now, and that answer does not carry over to the
   /// other iterations.
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

   /// True when the two calls may not mod/ref any common memory, in this iteration
   /// and in every other one. Both must already be recognized ac_channel hardware
   /// primitives: the conclusion is about independent hardware objects, and it is
   /// not a statement about arbitrary calls.
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
} // namespace bambu_ac_channel_primitives

#endif
