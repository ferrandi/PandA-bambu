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