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
#include "panda_clang_compat.hpp"

/**
 * @file DcePass.cpp
 *
 * @author Kaixi Matteo Chen <kaiximatteo.chen@mail.polimi.it>
 *
 */

#if PANDA_LLVM_CLANG_MAJOR >= 13

#include "OpenMPTaskPassConfig.hpp"
#include "Utils.hpp"

#include <llvm/IR/LegacyPassManager.h>
#include <llvm/Passes/PassBuilder.h>
#include <llvm/Passes/PassPlugin.h>
#include <llvm/Transforms/Scalar/DCE.h>

using namespace llvm;

namespace
{
   struct DcePass : public PassInfoMixin<DcePass>
   {
      PreservedAnalyses run(Function& F, FunctionAnalysisManager& FAM)
      {
         DCEPass dce;
         dce.run(F, FAM);

         return PreservedAnalyses::none();
      }
   };
} // namespace

// Registration for the new pass
llvm::PassPluginLibraryInfo getDcePassPluginInfo()
{
   return {LLVM_PLUGIN_API_VERSION, "DcePass", "v0.1.0", [](PassBuilder& PB) {
              PB.registerPipelineParsingCallback(
                  [](StringRef Name, FunctionPassManager& FPM, ArrayRef<PassBuilder::PipelineElement>) {
                     if(Name == "deadCodeElimination")
                     {
                        FPM.addPass(DcePass());
                        return true;
                     }
                     return false;
                  });
           }};
}

extern "C" PassPluginLibraryInfo LLVM_ATTRIBUTE_WEAK __attribute__((visibility("default"))) llvmGetPassPluginInfo()
{
   return getDcePassPluginInfo();
}

#else

#include "llvm/Transforms/IPO/PassManagerBuilder.h"
#include <llvm/IR/LegacyPassManager.h>

namespace
{
   struct DcePass : public llvm::ModulePass
   {
      char ID = 0;
      DcePass() : llvm::ModulePass(ID)
      {
      }

      bool runOnModule(llvm::Module& M) override
      {
         return false;
      }
   };

} // namespace

#if ADD_RSP
// This function is of type PassManagerBuilder::ExtensionFn
static void loadPass(const llvm::PassManagerBuilder&, llvm::legacy::PassManagerBase& PM)
{
   PM.add(new DcePass());
}

// These constructors add our pass to a list of global extensions.
static llvm::RegisterStandardPasses DcePass_Ox(llvm::PassManagerBuilder::EP_ModuleOptimizerEarly, loadPass);
#endif

#endif
