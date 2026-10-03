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
#ifndef BAMBU_SCALARIZE_SINGLETON_BANKS_PASS_HPP
#define BAMBU_SCALARIZE_SINGLETON_BANKS_PASS_HPP

#include "llvm/ADT/StringRef.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/PassManager.h"
#include "llvm/Pass.h"

namespace llvm
{
   /// Replaces  alloca [1 x ... [1 x {{{ T }}}]]  - a chain of singleton arrays and
   /// single-member structs wrapping a scalar T - with a plain  alloca T,  and drops the
   /// all-zero GEPs that reached the leaf. Only when every access is a plain load/store of
   /// T: an alloca whose address goes anywhere else is left alone.
   ///
   /// This is what makes the completely partitioned banks CSROA emits promotable. mem2reg
   /// requires the accessed type to equal the allocated type, so it refuses  alloca {i37},
   /// alloca {{{i37}}}  and even  alloca [1 x i37].  Upstream SROA cannot help either: it
   /// bails on any integer whose bit width differs from its store size in bits
   /// (isIntegerWideningViable in llvm/lib/Transforms/Scalar/SROA.cpp) and degrades the
   /// alloca to  [5 x i8],  which mem2reg can no longer touch. Measured on LLVM 9/13/16/19
   /// with 600 banks in one basic block: sroa, mem2reg, mem2reg+sroa and default<O2> all
   /// leave every alloca in place; after this pass mem2reg promotes all of them.
   ///
   /// A PromotePass must therefore follow this one, and it must run before -O2's own SROA,
   /// which actively degrades the scalar allocas this pass creates.
   struct ScalarizeSingletonBanksPass : public ModulePass, public PassInfoMixin<ScalarizeSingletonBanksPass>
   {
    public:
      static char ID;

      ScalarizeSingletonBanksPass() : ModulePass(ID)
      {
      }

      /// Pass::Pass deletes the copy constructor, and the new PM's addPass needs to move the
      /// pass into its PassModel. The pass is stateless, so delegating to the default
      /// constructor is all it takes.
      ScalarizeSingletonBanksPass(const ScalarizeSingletonBanksPass&) : ScalarizeSingletonBanksPass()
      {
      }

      /// Shared implementation called by both PM entry-points.
      bool exec(Module& M);

      /// New Pass Manager entry-point.
      PreservedAnalyses run(Module& M, ModuleAnalysisManager& AM);

      /// Legacy Pass Manager entry-point.
      bool runOnModule(Module& M) override;

      StringRef getPassName() const override;

      void getAnalysisUsage(AnalysisUsage& AU) const override;
   };

} // namespace llvm

#endif // BAMBU_SCALARIZE_SINGLETON_BANKS_PASS_HPP
