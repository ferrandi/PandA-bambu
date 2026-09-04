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
#ifndef BAMBU_COND_INST_COMB_IF_TAGGED_PASS_HPP
#define BAMBU_COND_INST_COMB_IF_TAGGED_PASS_HPP

#include <llvm/IR/Module.h>
#include <llvm/IR/PassManager.h>
#include <llvm/Pass.h>

namespace llvm
{
   struct CondInstCombIfTaggedPass : public ModulePass
#if LLVM_VERSION_MAJOR >= 13
       ,
                                     public PassInfoMixin<CondInstCombIfTaggedPass>
#endif
   {
      static char ID;
      CondInstCombIfTaggedPass() : ModulePass(ID)
      {
      }

#if LLVM_VERSION_MAJOR >= 13
      CondInstCombIfTaggedPass(const CondInstCombIfTaggedPass&) : CondInstCombIfTaggedPass()
      {
      }
#endif

      PreservedAnalyses run(Module& M, ModuleAnalysisManager& MAM);
      bool runOnModule(Module& M) override;
      StringRef getPassName() const override;
   };
} // namespace llvm

#endif // BAMBU_COND_INST_COMB_IF_TAGGED_PASS_HPP