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
 *              Copyright (C) 2018-2024 Politecnico di Milano
 *
 *   This file is part of the PandA framework.
 *
 *   The PandA framework is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */
/**
 * @file plugin_dumpGimpleEmpty.cpp
 * @brief Plugin to dump global variables in gimple raw format starting from LLVM IR
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
#include <llvm/Transforms/IPO/PassManagerBuilder.h>
#include <llvm/Transforms/Utils/LoopUtils.h>

#if __clang_major__ != 4
#include <llvm/Analysis/MemorySSA.h>
#else
#include <llvm/Transforms/Utils/MemorySSA.h>
#endif
#if __clang_major__ > 5
#include <llvm/Analysis/OptimizationRemarkEmitter.h>
#endif
#if __clang_major__ >= 10
#include <llvm/Support/CommandLine.h>
#endif
#if __clang_major__ >= 13
#include <llvm/Passes/PassBuilder.h>
#include <llvm/Passes/PassPlugin.h>
#endif

namespace llvm
{
   cl::opt<std::string> outdir_nameGE("pandaGE-outputdir",
                                      cl::desc("Specify the directory where the gimple raw file will be written"),
                                      cl::value_desc("directory path"));
   cl::opt<std::string> InFileGE("pandaGE-infile", cl::desc("Specify the name of the compiled source file"),
                                 cl::value_desc("filename path"), cl::OneOrMore);
   struct CLANG_VERSION_SYMBOL(_plugin_dumpGimpleEmpty)
       : public ModulePass
#if __clang_major__ >= 13
         ,
         public PassInfoMixin<CLANG_VERSION_SYMBOL(_plugin_dumpGimpleEmpty)>
#endif
   {
      static char ID;

      CLANG_VERSION_SYMBOL(_plugin_dumpGimpleEmpty)
      () : ModulePass(ID)
      {
         initializeLoopInfoWrapperPassPass(*PassRegistry::getPassRegistry());
         initializeLazyValueInfoWrapperPassPass(*PassRegistry::getPassRegistry());
         initializeMemorySSAWrapperPassPass(*PassRegistry::getPassRegistry());
         initializeTargetTransformInfoWrapperPassPass(*PassRegistry::getPassRegistry());
         initializeTargetLibraryInfoWrapperPassPass(*PassRegistry::getPassRegistry());
         initializeAssumptionCacheTrackerPass(*PassRegistry::getPassRegistry());
         initializeDominatorTreeWrapperPassPass(*PassRegistry::getPassRegistry());
#if __clang_major__ > 5
         initializeOptimizationRemarkEmitterWrapperPassPass(*PassRegistry::getPassRegistry());
#endif
      }

#if __clang_major__ >= 13
      CLANG_VERSION_SYMBOL(_plugin_dumpGimpleEmpty)
      (const CLANG_VERSION_SYMBOL(_plugin_dumpGimpleEmpty) &) : CLANG_VERSION_SYMBOL(_plugin_dumpGimpleEmpty)()
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
#if __clang_major__ > 5
                ,
                llvm::function_ref<llvm::OptimizationRemarkEmitter&(llvm::Function&)> GetORE
#endif
      )
      {
         if(outdir_nameGE.empty())
            return false;

         DumpGimpleRaw gimpleRawWriter(outdir_nameGE, InFileGE, true, nullptr, false);
         std::vector<std::string> empty;
         auto res = gimpleRawWriter.exec(M, empty, GetTLI, GetTTI, GetDomTree, GetLI, GetMSSA, GetLVI, GetAC,
#if __clang_major__ > 5
                                         GetORE,
#endif
                                         "");
         return res;
      }

      bool runOnModule(Module& M) override
      {
#if __clang_major__ < 13
#if __clang_major__ >= 10
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
#if __clang_major__ > 5
         auto GetORE = [&](llvm::Function& F) -> llvm::OptimizationRemarkEmitter& {
#if __clang_major__ >= 11
            return getAnalysis<llvm::OptimizationRemarkEmitterWrapperPass>(F).getORE();
#else
            return getAnalysis<llvm::OptimizationRemarkEmitterWrapperPass>(F).getORE();
#endif
         };
#endif

         return exec(M, GetTLI, GetTTI, GetDomTree, GetLI, GetMSSA, GetLVI, GetAC
#if __clang_major__ > 5
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
         return CLANG_VERSION_STRING(_plugin_dumpGimpleEmpty);
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
#if __clang_major__ > 5
         AU.addRequired<OptimizationRemarkEmitterWrapperPass>();
#endif
      }

#if __clang_major__ >= 13
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

   char CLANG_VERSION_SYMBOL(_plugin_dumpGimpleEmpty)::ID = 0;

} // namespace llvm

#if !defined(_WIN32)
static llvm::RegisterPass<llvm::CLANG_VERSION_SYMBOL(_plugin_dumpGimpleEmpty)>
    XPass(CLANG_VERSION_STRING(_plugin_dumpGimpleEmpty), "Dump gimple ssa raw format starting from LLVM IR: LLVM pass",
          false /* Only looks at CFG */, false /* Analysis Pass */);
#endif

#if __clang_major__ >= 13
llvm::PassPluginLibraryInfo CLANG_PLUGIN_INFO(_plugin_dumpGimpleEmpty)()
{
   return {LLVM_PLUGIN_API_VERSION, CLANG_VERSION_STRING(_plugin_dumpGimpleEmpty), "v0.12", [](llvm::PassBuilder& PB) {
              const auto load = [](llvm::ModulePassManager& MPM) {
                 MPM.addPass(llvm::CLANG_VERSION_SYMBOL(_plugin_dumpGimpleEmpty)());
                 return true;
              };
              PB.registerPipelineParsingCallback([&](llvm::StringRef Name, llvm::ModulePassManager& MPM,
                                                     llvm::ArrayRef<llvm::PassBuilder::PipelineElement>) {
                 if(Name == CLANG_VERSION_STRING(_plugin_dumpGimpleEmpty))
                 {
                    return load(MPM);
                 }
                 return false;
              });
              PB.registerOptimizerLastEPCallback([&](llvm::ModulePassManager& MPM,
#if __clang_major__ < 16
                                                     llvm::PassBuilder::OptimizationLevel
#else
                                                     llvm::OptimizationLevel
#endif
                                                 ) { return load(MPM); });
           }};
}

// This part is the new way of registering your pass
extern "C" ::llvm::PassPluginLibraryInfo LLVM_ATTRIBUTE_WEAK llvmGetPassPluginInfo()
{
   return CLANG_PLUGIN_INFO(_plugin_dumpGimpleEmpty)();
}
#else
#if ADD_RSP
// This function is of type PassManagerBuilder::ExtensionFn
static void loadPass(const llvm::PassManagerBuilder&, llvm::legacy::PassManagerBase& PM)
{
   PM.add(new llvm::CLANG_VERSION_SYMBOL(_plugin_dumpGimpleEmpty)());
}

// These constructors add our pass to a list of global extensions.
static llvm::RegisterStandardPasses
    CLANG_VERSION_SYMBOL(_plugin_dumpGimpleEmptyLoader_Ox)(llvm::PassManagerBuilder::EP_OptimizerLast, loadPass);
#endif
#endif

// using namespace llvm;
//
// namespace llvm
// {
//    void CLANG_PLUGIN_INIT(_plugin_dumpGimpleEmpty)(PassRegistry&);
// } // namespace llvm
//
// INITIALIZE_PASS_BEGIN(CLANG_VERSION_SYMBOL(_plugin_dumpGimpleEmpty), CLANG_VERSION_STRING(_plugin_dumpGimpleEmpty),
//                       "Dump gimple ssa raw format starting from LLVM IR: LLVM pass", false, false)
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
// INITIALIZE_PASS_END(CLANG_VERSION_SYMBOL(_plugin_dumpGimpleEmpty), CLANG_VERSION_STRING(_plugin_dumpGimpleEmpty),
//                     "Dump gimple ssa raw format starting from LLVM IR: LLVM pass", false, false)
