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
 *              Copyright (C) 2018-2026 Politecnico di Milano
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
 */
/**
 * @file plugin_dumpBambuIrEmpty.cpp
 * @brief Plugin to dump global variables starting from LLVM IR
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 *
 */
#include "plugin_includes.hpp"

#include <llvm/Analysis/AssumptionCache.h>
#include <llvm/Analysis/LazyValueInfo.h>
#include <llvm/Analysis/LoopInfo.h>
#include <llvm/Analysis/MemoryDependenceAnalysis.h>
#include <llvm/Analysis/TargetLibraryInfo.h>
#include <llvm/Analysis/TargetTransformInfo.h>
#include <llvm/IR/LegacyPassManager.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/PassManager.h>
#include <llvm/InitializePasses.h>
#include <llvm/Pass.h>
#include <llvm/PassRegistry.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Transforms/Utils/LoopUtils.h>

#if PANDA_LLVM_CLANG_MAJOR != 4
#include <llvm/Analysis/MemorySSA.h>
#else
#include <llvm/Transforms/Utils/MemorySSA.h>
#endif
#if PANDA_LLVM_CLANG_MAJOR > 5
#include <llvm/Analysis/OptimizationRemarkEmitter.h>
#endif
#if PANDA_LLVM_CLANG_MAJOR >= 10
#include <llvm/Support/CommandLine.h>
#endif
#if PANDA_LLVM_CLANG_MAJOR >= 13
#include <llvm/Passes/PassBuilder.h>
#include <llvm/Passes/PassPlugin.h>
#else
#include <llvm/Transforms/IPO/PassManagerBuilder.h>
#endif

namespace llvm
{
   cl::opt<std::string> outdir_nameGE("pandaGE-outputdir",
                                      cl::desc("Specify the directory where the bambuir file will be written"),
                                      cl::value_desc("directory path"));
   cl::opt<std::string> InFileGE("pandaGE-infile", cl::desc("Specify the name of the compiled source file"),
                                 cl::value_desc("filename path"), cl::OneOrMore);
   struct dumpBambuIrEmpty : public ModulePass
#if PANDA_LLVM_CLANG_MAJOR >= 13
       ,
                             public PassInfoMixin<dumpBambuIrEmpty>
#endif
   {
      static char ID;

      dumpBambuIrEmpty() : ModulePass(ID)
      {
         initializeLoopInfoWrapperPassPass(*PassRegistry::getPassRegistry());
         initializeLazyValueInfoWrapperPassPass(*PassRegistry::getPassRegistry());
         initializeMemorySSAWrapperPassPass(*PassRegistry::getPassRegistry());
         initializeTargetTransformInfoWrapperPassPass(*PassRegistry::getPassRegistry());
         initializeTargetLibraryInfoWrapperPassPass(*PassRegistry::getPassRegistry());
         initializeAssumptionCacheTrackerPass(*PassRegistry::getPassRegistry());
         initializeDominatorTreeWrapperPassPass(*PassRegistry::getPassRegistry());
#if PANDA_LLVM_CLANG_MAJOR > 5
         initializeOptimizationRemarkEmitterWrapperPassPass(*PassRegistry::getPassRegistry());
#endif
      }

#if PANDA_LLVM_CLANG_MAJOR >= 13
      dumpBambuIrEmpty(const dumpBambuIrEmpty&) : dumpBambuIrEmpty()
      {
      }
#endif

      bool exec(Module& M, llvm::function_ref<llvm::TargetLibraryInfo&(llvm::Function&)> GetTLI,
                llvm::function_ref<llvm::TargetTransformInfo&(llvm::Function&)> GetTTI,
                llvm::function_ref<llvm::DominatorTree&(llvm::Function&)> GetDomTree,
                llvm::function_ref<llvm::LoopInfo&(llvm::Function&)> GetLI,
                llvm::function_ref<MemorySSAAnalysisResult&(llvm::Function&)> GetMSSA,
                llvm::function_ref<llvm::LazyValueInfo&(llvm::Function&)> GetLVI,
                llvm::function_ref<llvm::AssumptionCache&(llvm::Function&)> GetAC
#if PANDA_LLVM_CLANG_MAJOR > 5
                ,
                llvm::function_ref<llvm::OptimizationRemarkEmitter&(llvm::Function&)> GetORE
#endif
      )
      {
         if(outdir_nameGE.empty())
            return false;

         DumpBambuIR RawWriter(outdir_nameGE, InFileGE, true, nullptr, false);
         std::vector<std::string> empty;
         auto res = RawWriter.exec(M, empty, GetTLI, GetTTI, GetDomTree, GetLI, GetMSSA, GetLVI, GetAC,
#if PANDA_LLVM_CLANG_MAJOR > 5
                                   GetORE,
#endif
                                   "");
         return res;
      }

      bool runOnModule(Module& M) override
      {
#if PANDA_LLVM_CLANG_MAJOR < 13
#if PANDA_LLVM_CLANG_MAJOR >= 10
         auto GetTLI = [&](llvm::Function& F) -> llvm::TargetLibraryInfo& {
            return getAnalysis<llvm::TargetLibraryInfoWrapperPass>().getTLI(F);
         };
#else
         auto GetTLI = [&](llvm::Function&) -> llvm::TargetLibraryInfo& {
            return getAnalysis<llvm::TargetLibraryInfoWrapperPass>().getTLI();
         };
#endif
         auto GetTTI = [&](llvm::Function& F) -> llvm::TargetTransformInfo& {
            return getAnalysis<llvm::TargetTransformInfoWrapperPass>().getTTI(F);
         };
         auto GetDomTree = [&](llvm::Function& F) -> llvm::DominatorTree& {
            return getAnalysis<llvm::DominatorTreeWrapperPass>(F).getDomTree();
         };
         auto GetLI = [&](llvm::Function& F) -> llvm::LoopInfo& {
            return getAnalysis<llvm::LoopInfoWrapperPass>(F).getLoopInfo();
         };
         auto GetMSSA = [&](llvm::Function& F) -> MemorySSAAnalysisResult& {
            return getAnalysis<llvm::MemorySSAWrapperPass>(F);
         };
         auto GetLVI = [&](llvm::Function& F) -> llvm::LazyValueInfo& {
            return getAnalysis<llvm::LazyValueInfoWrapperPass>(F).getLVI();
         };
         auto GetAC = [&](llvm::Function& F) -> llvm::AssumptionCache& {
            return getAnalysis<llvm::AssumptionCacheTracker>().getAssumptionCache(F);
         };
#if PANDA_LLVM_CLANG_MAJOR > 5
         auto GetORE = [&](llvm::Function& F) -> llvm::OptimizationRemarkEmitter& {
#if PANDA_LLVM_CLANG_MAJOR >= 11
            return getAnalysis<llvm::OptimizationRemarkEmitterWrapperPass>(F).getORE();
#else
            return getAnalysis<llvm::OptimizationRemarkEmitterWrapperPass>(F).getORE();
#endif
         };
#endif

         return exec(M, GetTLI, GetTTI, GetDomTree, GetLI, GetMSSA, GetLVI, GetAC
#if PANDA_LLVM_CLANG_MAJOR > 5
                     ,
                     GetORE
#endif
         );
#else
         report_fatal_error("Call to runOnModule not expected");
         return false;
#endif
      }

      StringRef getPassName() const override
      {
         return "dumpBambuIrEmpty";
      }

      void getAnalysisUsage(AnalysisUsage& AU) const override
      {
         AU.setPreservesAll();
         AU.addRequired<LoopInfoWrapperPass>();
         AU.addPreserved<MemorySSAWrapperPass>();
         AU.addRequired<MemorySSAWrapperPass>();
         AU.addRequired<LazyValueInfoWrapperPass>();
         AU.addRequired<TargetTransformInfoWrapperPass>();
         AU.addRequired<TargetLibraryInfoWrapperPass>();
         AU.addRequired<AssumptionCacheTracker>();
         AU.addRequired<DominatorTreeWrapperPass>();
#if PANDA_LLVM_CLANG_MAJOR > 5
         AU.addRequired<OptimizationRemarkEmitterWrapperPass>();
#endif
      }

#if PANDA_LLVM_CLANG_MAJOR >= 13
      llvm::PreservedAnalyses run(llvm::Module& M, llvm::ModuleAnalysisManager& MAM)
      {
         MAM.invalidate(M, llvm::PreservedAnalyses::none());
         auto& FAM = MAM.getResult<FunctionAnalysisManagerModuleProxy>(M).getManager();
         auto GetTLI = [&](llvm::Function& F) -> llvm::TargetLibraryInfo& {
            return FAM.getResult<llvm::TargetLibraryAnalysis>(F);
         };
         auto GetTTI = [&](llvm::Function& F) -> llvm::TargetTransformInfo& {
            return FAM.getResult<llvm::TargetIRAnalysis>(F);
         };
         auto GetDomTree = [&](llvm::Function& F) -> llvm::DominatorTree& {
            return FAM.getResult<llvm::DominatorTreeAnalysis>(F);
         };
         auto GetLI = [&](llvm::Function& F) -> llvm::LoopInfo& { return FAM.getResult<llvm::LoopAnalysis>(F); };
         auto GetMSSA = [&](llvm::Function& F) -> MemorySSAAnalysisResult& {
            return FAM.getResult<llvm::MemorySSAAnalysis>(F);
         };
         auto GetLVI = [&](llvm::Function& F) -> llvm::LazyValueInfo& {
            return FAM.getResult<llvm::LazyValueAnalysis>(F);
         };
         auto GetAC = [&](llvm::Function& F) -> llvm::AssumptionCache& {
            return FAM.getResult<llvm::AssumptionAnalysis>(F);
         };
         auto GetORE = [&](llvm::Function& F) -> llvm::OptimizationRemarkEmitter& {
            return FAM.getResult<llvm::OptimizationRemarkEmitterAnalysis>(F);
         };

         const auto changed = exec(M, GetTLI, GetTTI, GetDomTree, GetLI, GetMSSA, GetLVI, GetAC, GetORE);
         return (changed ? llvm::PreservedAnalyses::none() : llvm::PreservedAnalyses::all());
      }
#endif
   };

   char dumpBambuIrEmpty::ID = 0;

} // namespace llvm

#if !defined(_WIN32)
static llvm::RegisterPass<llvm::dumpBambuIrEmpty> XPass("dumpBambuIrEmpty",
                                                        "Dump the bambuir starting from LLVM IR: LLVM pass",
                                                        false /* Only looks at CFG */, false /* Analysis Pass */);
#endif

#if PANDA_LLVM_CLANG_MAJOR >= 13
llvm::PassPluginLibraryInfo getdumpBambuIrEmptyPluginInfo()
{
   return {LLVM_PLUGIN_API_VERSION, "dumpBambuIrEmpty", "v0.12", [](llvm::PassBuilder& PB) {
              const auto load = [](llvm::ModulePassManager& MPM) {
                 MPM.addPass(llvm::dumpBambuIrEmpty());
                 return true;
              };
              PB.registerPipelineParsingCallback([&](llvm::StringRef Name, llvm::ModulePassManager& MPM,
                                                     llvm::ArrayRef<llvm::PassBuilder::PipelineElement>) {
                 if(Name == "dumpBambuIrEmpty")
                 {
                    return load(MPM);
                 }
                 return false;
              });
              PB.registerOptimizerLastEPCallback([&](llvm::ModulePassManager& MPM,
#if PANDA_LLVM_CLANG_MAJOR < 16
                                                     llvm::PassBuilder::OptimizationLevel
#else
                                                     llvm::OptimizationLevel
#endif
                                                 ) { return load(MPM); });
           }};
}

// This part is the new way of registering your pass
extern "C" ::llvm::PassPluginLibraryInfo LLVM_ATTRIBUTE_WEAK __attribute__((visibility("default")))
llvmGetPassPluginInfo()
{
   return getdumpBambuIrEmptyPluginInfo();
}
#else
#if ADD_RSP
// This function is of type PassManagerBuilder::ExtensionFn
static void loadPass(const llvm::PassManagerBuilder&, llvm::legacy::PassManagerBase& PM)
{
   PM.add(new llvm::dumpBambuIrEmpty());
}

// These constructors add our pass to a list of global extensions.
static llvm::RegisterStandardPasses dumpBambuIrEmptyLoader_Ox(llvm::PassManagerBuilder::EP_OptimizerLast, loadPass);
#endif
#endif

// using namespace llvm;
//
// namespace llvm
// {
//    void initializedumpBambuIrEmptyPass(PassRegistry&);
// } // namespace llvm
//
// INITIALIZE_PASS_BEGIN(dumpBambuIrEmpty, "dumpBambuIrEmpty",
//                       "Dump the bambuir starting from LLVM IR: LLVM pass", false, false)
// INITIALIZE_PASS_DEPENDENCY(MemoryDependenceWrapperPass)
// INITIALIZE_PASS_DEPENDENCY(MemorySSAWrapperPass)
// INITIALIZE_PASS_DEPENDENCY(LazyValueInfoWrapperPass)
// INITIALIZE_PASS_DEPENDENCY(AAResultsWrapperPass)
// INITIALIZE_PASS_DEPENDENCY(TargetTransformInfoWrapperPass)
// INITIALIZE_PASS_DEPENDENCY(TargetLibraryInfoWrapperPass)
// INITIALIZE_PASS_DEPENDENCY(AssumptionCacheTracker)
// INITIALIZE_PASS_DEPENDENCY(DominatorTreeWrapperPass)
// INITIALIZE_PASS_DEPENDENCY(DominanceFrontierWrapperPass)
// INITIALIZE_PASS_DEPENDENCY(OptimizationRemarkEmitterWrapperPass)
// INITIALIZE_PASS_END(dumpBambuIrEmpty, "dumpBambuIrEmpty",
//                     "Dump the bambuir raw format starting from LLVM IR: LLVM pass", false, false)
