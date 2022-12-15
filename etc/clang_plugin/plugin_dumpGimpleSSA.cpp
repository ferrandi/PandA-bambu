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
 *              Copyright (C) 2018-2022 Politecnico di Milano
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
 * @file plugin_dumpGimpleSSA.cpp
 * @brief Plugin to dump functions and global variables in gimple raw format starting from LLVM IR
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 *
 */
#undef NDEBUG
#include "debug_print.hpp"

#include "plugin_includes.hpp"

#include <llvm-c/Transforms/Scalar.h>
#include <llvm/Analysis/AliasAnalysis.h>
#include <llvm/Analysis/AssumptionCache.h>
#include <llvm/Analysis/CFLSteensAliasAnalysis.h>
#include <llvm/Analysis/DominanceFrontier.h>
#include <llvm/Analysis/LazyValueInfo.h>
#include <llvm/Analysis/LoopInfo.h>
#include <llvm/Analysis/MemoryDependenceAnalysis.h>
#include <llvm/Analysis/TargetLibraryInfo.h>
#include <llvm/Analysis/TargetTransformInfo.h>
#include <llvm/Analysis/TypeBasedAliasAnalysis.h>
#include <llvm/CodeGen/Passes.h>
#include <llvm/IR/Dominators.h>
#include <llvm/IR/LegacyPassManager.h>
#include <llvm/IR/PassManager.h>
#include <llvm/InitializePasses.h>
#include <llvm/Pass.h>
#include <llvm/PassRegistry.h>
#include <llvm/Support/CommandLine.h>
#include <llvm/Support/MemoryBuffer.h>
#include <llvm/Transforms/IPO.h>
#include <llvm/Transforms/IPO/PassManagerBuilder.h>
#include <llvm/Transforms/Scalar.h>
#include <llvm/Transforms/Scalar/GVN.h>
#include <llvm/Transforms/Utils/LoopUtils.h>
#include <llvm/Transforms/Utils/UnifyFunctionExitNodes.h>

#if __clang_major__ != 4
#include <llvm/Analysis/MemorySSA.h>
#else
#include <llvm/Transforms/Utils/MemorySSA.h>
#endif
#if __clang_major__ > 5
#include <llvm/Analysis/OptimizationRemarkEmitter.h>
#endif
#if __clang_major__ >= 7
#include <llvm/Transforms/InstCombine/InstCombine.h>
#endif
#if __clang_major__ >= 7 && !defined(VVD)
#include <llvm/Transforms/Utils.h>
#endif
#if __clang_major__ >= 13
#include <llvm/Analysis/CGSCCPassManager.h>
#include <llvm/Passes/PassBuilder.h>
#include <llvm/Passes/PassPlugin.h>
#include <llvm/Transforms/IPO/ArgumentPromotion.h>
#include <llvm/Transforms/IPO/GlobalOpt.h>
#include <llvm/Transforms/InstCombine/InstCombine.h>
#include <llvm/Transforms/Scalar/LowerAtomic.h>
#include <llvm/Transforms/Utils/BreakCriticalEdges.h>
#include <llvm/Transforms/Utils/Mem2Reg.h>
#include <llvm/Transforms/Utils/UnifyFunctionExitNodes.h>
#endif

#include <boost/tokenizer.hpp>
#include <sstream>
#include <string>

namespace llvm
{
   cl::opt<std::string> TopFunctionName("panda-topfname", cl::desc("Specify the name of the top function"),
                                        cl::value_desc("name of the top function"));
   cl::opt<std::string> outdir_name("panda-outputdir",
                                    cl::desc("Specify the directory where the gimple raw file will be written"),
                                    cl::value_desc("directory path"));
   cl::opt<std::string> InFile("panda-infile", cl::desc("Specify the name of the compiled source file"),
                               cl::value_desc("filename path"));
   cl::opt<std::string> CostTable("panda-cost-table", cl::desc("Specify the cost per operation"),
                                  cl::value_desc("cost table"));

   struct CLANG_VERSION_SYMBOL(_plugin_dumpGimpleSSA)
       : public ModulePass
#if __clang_major__ >= 13
         ,
         public PassInfoMixin<CLANG_VERSION_SYMBOL(_plugin_dumpGimpleSSA)>
#endif
   {
      static char ID;
      bool earlyAnalysis;

      CLANG_VERSION_SYMBOL(_plugin_dumpGimpleSSA)
      (bool _earlyAnalysis = false) : ModulePass(ID), earlyAnalysis(_earlyAnalysis)
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
      CLANG_VERSION_SYMBOL(_plugin_dumpGimpleSSA)
      (const CLANG_VERSION_SYMBOL(_plugin_dumpGimpleSSA) & other)
          : CLANG_VERSION_SYMBOL(_plugin_dumpGimpleSSA)(other.earlyAnalysis)
      {
      }
#endif

      std::string create_file_basename_string(const std::string& on, const std::string& original_filename) const
      {
         std::size_t found = original_filename.find_last_of("/\\");
         std::string dump_base_name;
         if(found == std::string::npos)
         {
            dump_base_name = original_filename;
         }
         else
         {
            dump_base_name = original_filename.substr(found + 1);
         }
         return on + "/" + dump_base_name;
      }

      bool exec(Module& M, llvm::function_ref<llvm::TargetLibraryInfo&(llvm::Function&)> GetTLI,
                llvm::function_ref<llvm::TargetTransformInfo&(llvm::Function&)> GetTTI,
                llvm::function_ref<llvm::DominatorTree&(llvm::Function&)> GetDomTree,
                llvm::function_ref<llvm::LoopInfo&(llvm::Function&)> GetLI,
                llvm::function_ref<MemorySSAAnalysisResult&(llvm::Function&)> GetMSSA,
                llvm::function_ref<llvm::LazyValueInfo&(llvm::Function&)> GetLVI,
                llvm::function_ref<llvm::AssumptionCache&(llvm::Function&)> GetAC,
#if __clang_major__ > 5
                llvm::function_ref<llvm::OptimizationRemarkEmitter&(llvm::Function&)> GetORE,
#endif
                const std::string& costTable)
      {
         if(outdir_name.empty())
         {
            llvm::report_fatal_error("-panda-outdir parameter not specified");
         }
         if(InFile.empty())
         {
            llvm::report_fatal_error("-panda-infile parameter not specified");
         }

         /// load parameter names
         boost::char_separator<char> sep(",");
         boost::tokenizer<boost::char_separator<char>> FileTokenizer(InFile, sep);
         std::map<std::string, std::vector<std::string>> Fun2Params;
         for(const auto& file_string : FileTokenizer)
         {
            auto parms_file_name = create_file_basename_string(outdir_name, file_string) + ".params.txt";
            ErrorOr<std::unique_ptr<MemoryBuffer>> BufOrErr = MemoryBuffer::getFile(parms_file_name);
            if(BufOrErr)
            {
               std::unique_ptr<MemoryBuffer> Buffer = std::move(BufOrErr.get());
               std::string buf = Buffer->getBuffer().data();
               std::stringstream ss(buf);
               std::string item;
               while(std::getline(ss, item, '\n'))
               {
                  bool is_first = true;
                  std::stringstream ss_inner(item);
                  std::string item_inner;
                  std::string funName;
                  while(std::getline(ss_inner, item_inner, ' '))
                  {
                     if(is_first)
                     {
                        funName = item_inner;
                        is_first = false;
                     }
                     else
                     {
                        Fun2Params[funName].push_back(item_inner);
                     }
                  }
               }
            }
         }
         DumpGimpleRaw gimpleRawWriter(outdir_name, *(FileTokenizer.begin()), false, &Fun2Params, earlyAnalysis);

         if(!TopFunctionName.empty())
         {
            PRINT_DBG("Top function name: " << TopFunctionName << "\n");
         }

         auto res = gimpleRawWriter.exec(M, TopFunctionName, GetTLI, GetTTI, GetDomTree, GetLI, GetMSSA, GetLVI, GetAC,
#if __clang_major__ > 5
                                         GetORE,
#endif
                                         costTable);
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

         return exec(M, GetTLI, GetTTI, GetDomTree, GetLI, GetMSSA, GetLVI, GetAC,
#if __clang_major__ > 5
                     GetORE,
#endif
                     CostTable);
#else
         report_fatal_error("Call to runOnModule not expected with current LLVM version");
         return false;
#endif
      }

      StringRef getPassName() const override
      {
         return CLANG_VERSION_STRING(_plugin_dumpGimpleSSA);
      }

      void getAnalysisUsage(AnalysisUsage& AU) const override
      {
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

         const auto changed = exec(M, GetTLI, GetTTI, GetDomTree, GetLI, GetMSSA, GetLVI, GetAC, GetORE, CostTable);
         return (changed ? llvm::PreservedAnalyses::none() : llvm::PreservedAnalyses::all());
      }
#endif
   };

   char CLANG_VERSION_SYMBOL(_plugin_dumpGimpleSSA)::ID = 0;

} // namespace llvm

// Currently there is no difference between c++ or c serialization
#if !defined(_WIN32)
#if CPP_LANGUAGE
// static llvm::RegisterPass<llvm::CLANG_VERSION_SYMBOL(_plugin_dumpGimpleSSA)<true>>
// XPassEarly(CLANG_VERSION_STRING(_plugin_dumpGimpleSSACppEarly), "Custom Value Range Based optimization step: LLVM
// pass", false /* Only looks at CFG */, false /* Analysis Pass */);
static llvm::RegisterPass<llvm::CLANG_VERSION_SYMBOL(_plugin_dumpGimpleSSA)>
    XPass(CLANG_VERSION_STRING(_plugin_dumpGimpleSSACpp), "Dump gimple ssa raw format starting from LLVM IR: LLVM pass",
          false /* Only looks at CFG */, false /* Analysis Pass */);
#else
// static llvm::RegisterPass<llvm::CLANG_VERSION_SYMBOL(_plugin_dumpGimpleSSA)<true>>
// XPassEarly(CLANG_VERSION_STRING(_plugin_dumpGimpleSSAEarly), "Custom Value Range Based optimization step: LLVM pass",
// false /* Only looks at CFG */, false /* Analysis Pass */);
static llvm::RegisterPass<llvm::CLANG_VERSION_SYMBOL(_plugin_dumpGimpleSSA)>
    XPass(CLANG_VERSION_STRING(_plugin_dumpGimpleSSA), "Dump gimple ssa raw format starting from LLVM IR: LLVM pass",
          false /* Only looks at CFG */, false /* Analysis Pass */);
#endif
#endif

#if __clang_major__ >= 13
llvm::PassPluginLibraryInfo CLANG_PLUGIN_INFO(_plugin_dumpGimpleSSA)()
{
   return {LLVM_PLUGIN_API_VERSION, CLANG_VERSION_STRING(_plugin_dumpGimpleSSA), "v0.12", [](llvm::PassBuilder& PB) {
              const auto load = [](llvm::ModulePassManager& MPM) {
                 llvm::FunctionPassManager FPM1;
                 FPM1.addPass(llvm::LowerAtomicPass());
                 FPM1.addPass(llvm::PromotePass());
                 MPM.addPass(llvm::createModuleToFunctionPassAdaptor(std::move(FPM1)));
                 MPM.addPass(llvm::GlobalOptPass());
                 MPM.addPass(llvm::createModuleToPostOrderCGSCCPassAdaptor(llvm::ArgumentPromotionPass(256)));
                 llvm::FunctionPassManager FPM2;
                 FPM2.addPass(llvm::InstCombinePass());
                 FPM2.addPass(llvm::BreakCriticalEdgesPass());
                 FPM2.addPass(llvm::UnifyFunctionExitNodesPass());
                 MPM.addPass(llvm::createModuleToFunctionPassAdaptor(std::move(FPM2)));
                 MPM.addPass(llvm::CLANG_VERSION_SYMBOL(_plugin_dumpGimpleSSA)());
                 return true;
              };
              PB.registerPipelineParsingCallback([&](llvm::StringRef Name, llvm::ModulePassManager& MPM,
                                                     llvm::ArrayRef<llvm::PassBuilder::PipelineElement>) {
                 if(Name == CLANG_VERSION_STRING(_plugin_dumpGimpleSSA))
                 {
                    return load(MPM);
                 }
                 return false;
              });
              PB.registerOptimizerLastEPCallback(
                  [&](llvm::ModulePassManager& MPM, llvm::PassBuilder::OptimizationLevel) { return load(MPM); });
           }};
}

// This part is the new way of registering your pass
extern "C" ::llvm::PassPluginLibraryInfo LLVM_ATTRIBUTE_WEAK llvmGetPassPluginInfo()
{
   return CLANG_PLUGIN_INFO(_plugin_dumpGimpleSSA)();
}
#else
#if ADD_RSP
// This function is of type PassManagerBuilder::ExtensionFn
static void loadPass(const llvm::PassManagerBuilder&, llvm::legacy::PassManagerBase& PM)
{
   //   PM.add(llvm::createGVNPass());
   //   PM.add(llvm::createGVNHoistPass());
   //   PM.add(llvm::createMergedLoadStoreMotionPass());
   PM.add(llvm::createLowerAtomicPass());
   PM.add(llvm::createPromoteMemoryToRegisterPass());
   PM.add(llvm::createGlobalOptimizerPass());
   PM.add(llvm::createArgumentPromotionPass(256));
   PM.add(llvm::createInstructionCombiningPass(true));
   PM.add(llvm::createBreakCriticalEdgesPass());
   PM.add(llvm::createUnifyFunctionExitNodesPass());

   PM.add(new llvm::CLANG_VERSION_SYMBOL(_plugin_dumpGimpleSSA)());
}

static llvm::RegisterStandardPasses llvmtoolLoader_Ox(llvm::PassManagerBuilder::EP_OptimizerLast, loadPass);

//    static void loadPassEarly(const llvm::PassManagerBuilder&, llvm::legacy::PassManagerBase& PM)
//    {
//       PM.add(llvm::createPromoteMemoryToRegisterPass());
//       PM.add(llvm::createGlobalOptimizerPass());
//       PM.add(llvm::createBreakCriticalEdgesPass());
//       PM.add(new llvm::CLANG_VERSION_SYMBOL(_plugin_dumpGimpleSSA) < true > ());
//    }
// These constructors add our pass to a list of global extensions.

// static llvm::RegisterStandardPasses llvmtoolLoader_O0(llvm::PassManagerBuilder::EP_ModuleOptimizerEarly,
// loadPassEarly);
#endif
#endif

// namespace llvm
// {
//    void CLANG_PLUGIN_INIT(_plugin_dumpGimpleSSA)(PassRegistry&);
// } // namespace llvm
//
// using namespace llvm;
//
// INITIALIZE_PASS_BEGIN(CLANG_VERSION_SYMBOL(_plugin_dumpGimpleSSA), CLANG_VERSION_STRING(_plugin_dumpGimpleSSA),
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
// INITIALIZE_PASS_END(CLANG_VERSION_SYMBOL(_plugin_dumpGimpleSSA), CLANG_VERSION_STRING(_plugin_dumpGimpleSSA),
//                     "Dump gimple ssa raw format starting from LLVM IR: LLVM pass", false, false)
