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
#ifndef BAMBU_LOOP_UNROLL_ARR_PART_PASS_HPP
#define BAMBU_LOOP_UNROLL_ARR_PART_PASS_HPP

#include "llvm/IR/PassManager.h"
#include <llvm/ADT/StringRef.h>
#include <llvm/Analysis/ScalarEvolution.h>
#include <llvm/Analysis/TargetLibraryInfo.h>
#include <llvm/Analysis/TargetTransformInfo.h>
#include <llvm/IR/Function.h>
#include <llvm/InitializePasses.h>
#include <llvm/Pass.h>
#include <llvm/PassRegistry.h>
#include <string>
#include <utility>

namespace llvm
{
   struct LoopUnrollArrPartPass : public ModulePass
#if LLVM_VERSION_MAJOR >= 13
       ,
                                  public PassInfoMixin<LoopUnrollArrPartPass>
#endif
   {
    public:
      static char ID;
      std::string outdirNameCmd;
      std::string topFnName;
      bool debug_lock;

      LoopUnrollArrPartPass(std::string outdirNameCmd, std::string topFnName, bool debug_lock)
          : ModulePass(ID),
            outdirNameCmd(std::move(outdirNameCmd)),
            topFnName(std::move(topFnName)),
            debug_lock(debug_lock)
      {
         initializeLoopInfoWrapperPassPass(*PassRegistry::getPassRegistry());
         initializeScalarEvolutionWrapperPassPass(*PassRegistry::getPassRegistry());
         initializeDominatorTreeWrapperPassPass(*PassRegistry::getPassRegistry());
         initializeAssumptionCacheTrackerPass(*PassRegistry::getPassRegistry());
         initializeTargetTransformInfoWrapperPassPass(*PassRegistry::getPassRegistry());
      }

      LoopUnrollArrPartPass(const LoopUnrollArrPartPass& other)
          : LoopUnrollArrPartPass(other.outdirNameCmd, other.topFnName, other.debug_lock)
      {
      }

      bool exec(Module& M, llvm::function_ref<llvm::LoopInfo&(llvm::Function&)> GetLI,
                llvm::function_ref<llvm::ScalarEvolution&(llvm::Function&)> GetSE,
                llvm::function_ref<llvm::TargetTransformInfo&(llvm::Function&)> GetTTI,
                llvm::function_ref<llvm::DominatorTree&(llvm::Function&)> GetDomTree,
                llvm::function_ref<llvm::AssumptionCache&(llvm::Function&)> GetAC);

      PreservedAnalyses run(Module& M, ModuleAnalysisManager& AM);
      bool runOnModule(Module& M) override;
      StringRef getPassName() const override;
      void getAnalysisUsage(AnalysisUsage& AU) const override;
   };

} // end namespace llvm

#endif // BAMBU_LOOP_UNROLL_ARR_PART_PASS_HPP