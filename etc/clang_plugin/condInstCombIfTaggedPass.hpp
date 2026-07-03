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