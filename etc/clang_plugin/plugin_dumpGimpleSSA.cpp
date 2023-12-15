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
 *              Copyright (C) 2018-2023 Politecnico di Milano
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
// #undef NDEBUG
#include "plugin_includes.hpp"

#include <llvm-c/Transforms/Scalar.h>
#include <llvm/Analysis/AliasAnalysis.h>
#include <llvm/Analysis/AssumptionCache.h>
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
#if __clang_major__ >= 13
#include <llvm/Passes/PassBuilder.h>
#include <llvm/Transforms/IPO/DeadArgumentElimination.h>
#include <llvm/Transforms/IPO/ForceFunctionAttrs.h>
#include <llvm/Transforms/IPO/FunctionAttrs.h>
#include <llvm/Transforms/IPO/GlobalDCE.h>
#include <llvm/Transforms/IPO/GlobalSplit.h>
#include <llvm/Transforms/IPO/InferFunctionAttrs.h>
#include <llvm/Transforms/IPO/Inliner.h>
#include <llvm/Transforms/IPO/MergeFunctions.h>
#include <llvm/Transforms/Scalar/DeadStoreElimination.h>
#include <llvm/Transforms/Scalar/IndVarSimplify.h>
#include <llvm/Transforms/Scalar/JumpThreading.h>
#include <llvm/Transforms/Scalar/LICM.h>
#include <llvm/Transforms/Scalar/LoopDeletion.h>
#include <llvm/Transforms/Scalar/LoopFlatten.h>
#include <llvm/Transforms/Scalar/LoopFuse.h>
#include <llvm/Transforms/Scalar/LoopRotation.h>
#include <llvm/Transforms/Scalar/MemCpyOptimizer.h>
#include <llvm/Transforms/Scalar/MergedLoadStoreMotion.h>
#include <llvm/Transforms/Scalar/NewGVN.h>
#include <llvm/Transforms/Scalar/SimplifyCFG.h>
#include <llvm/Transforms/Scalar/TailRecursionElimination.h>
#if __clang_major__ >= 16
#include <llvm/Transforms/Scalar/SROA.h>
#endif
#endif
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
#if __clang_major__ >= 16
#include <llvm/Transforms/Scalar/LowerAtomicPass.h>
#else
#include <llvm/Transforms/Scalar/LowerAtomic.h>
#endif
#include <llvm/Transforms/Utils/Mem2Reg.h>
#include <llvm/Transforms/Utils/UnifyFunctionExitNodes.h>
#endif

#define PUGIXML_NO_EXCEPTIONS
#define PUGIXML_HEADER_ONLY

#include <pugixml.hpp>

#include <fstream>
#include <sstream>
#include <string>

#ifdef CPP_LANGUAGE
#define CLANG_VERSION_SYMBOL_DUMP_SSA CLANG_VERSION_SYMBOL(_plugin_dumpGimpleSSACpp)
#define CLANG_VERSION_STRING_DUMP_SSA CLANG_VERSION_STRING(_plugin_dumpGimpleSSACpp)
#else
#define CLANG_VERSION_SYMBOL_DUMP_SSA CLANG_VERSION_SYMBOL(_plugin_dumpGimpleSSA)
#define CLANG_VERSION_STRING_DUMP_SSA CLANG_VERSION_STRING(_plugin_dumpGimpleSSA)
#endif

#define DEBUG_TYPE "dump-gimple-ssa"

namespace llvm
{
   cl::opt<std::string> TopFunctionName("panda-topfname", cl::desc("Specify the name of the top function"),
                                        cl::value_desc("name of the top function"));
   cl::opt<std::string> outdir_name("panda-outputdir",
                                    cl::desc("Specify the directory where the gimple raw file will be written"),
                                    cl::value_desc("directory path"), cl::Required);
   cl::opt<std::string> InFile("panda-infile", cl::desc("Specify the name of the compiled source file"),
                               cl::value_desc("filename path"), cl::OneOrMore);
   cl::opt<std::string> CostTable("panda-cost-table", cl::desc("Specify the cost per operation"),
                                  cl::value_desc("cost table"));

   struct CLANG_VERSION_SYMBOL_DUMP_SSA : public ModulePass
#if __clang_major__ >= 13
       ,
                                          public PassInfoMixin<CLANG_VERSION_SYMBOL_DUMP_SSA>
#endif
   {
      static char ID;
      bool earlyAnalysis;

      CLANG_VERSION_SYMBOL_DUMP_SSA(bool _earlyAnalysis = false) : ModulePass(ID), earlyAnalysis(_earlyAnalysis)
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
      CLANG_VERSION_SYMBOL_DUMP_SSA(const CLANG_VERSION_SYMBOL_DUMP_SSA& other)
          : CLANG_VERSION_SYMBOL_DUMP_SSA(other.earlyAnalysis)
      {
      }
#endif

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
         /// load parameter names
         std::map<std::string, std::vector<std::string>> Fun2Params;
         const auto first_filename =
             InFile.find(",") != std::string::npos ? InFile.substr(0, InFile.find(",")) : InFile;

         pugi::xml_document doc;
         const auto arch_filename = outdir_name + "/architecture.xml";
         if(doc.load_file(arch_filename.c_str()))
         {
            for(auto& f : doc.child("module"))
            {
               const auto func_symbol = std::string(f.attribute("symbol").value());
               auto& func_parms = Fun2Params[func_symbol];
               for(auto& p : f.child("parameters"))
               {
                  const auto parm_index = p.attribute("index").value();
                  const auto parm_name = std::string(p.attribute("port").value());
                  size_t idx = std::strtoul(parm_index, nullptr, 10);
                  if(func_parms.size() <= idx)
                  {
                     func_parms.resize(idx + 1);
                  }
                  func_parms[idx] = parm_name;
               }
            }
         }

         DumpGimpleRaw gimpleRawWriter(outdir_name, first_filename, false, &Fun2Params, earlyAnalysis);

         if(!TopFunctionName.empty())
         {
            LLVM_DEBUG(llvm::dbgs() << "Top function name: " << TopFunctionName << "\n");
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
         return CLANG_VERSION_STRING_DUMP_SSA;
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

   char CLANG_VERSION_SYMBOL_DUMP_SSA::ID = 0;

} // namespace llvm

// Currently there is no difference between c++ or c serialization
#if !defined(_WIN32)
// static llvm::RegisterPass<llvm::CLANG_VERSION_SYMBOL_DUMP_SSA<true>>
// XPassEarly(CLANG_VERSION_STRING(_plugin_dumpGimpleSSAEarly), "Custom Value Range Based optimization step: LLVM pass",
// false /* Only looks at CFG */, false /* Analysis Pass */);
static llvm::RegisterPass<llvm::CLANG_VERSION_SYMBOL_DUMP_SSA>
    XPass(CLANG_VERSION_STRING_DUMP_SSA, "Dump gimple ssa raw format starting from LLVM IR: LLVM pass",
          false /* Only looks at CFG */, false /* Analysis Pass */);
#endif

#if __clang_major__ >= 13
llvm::PassPluginLibraryInfo CLANG_PLUGIN_INFO(_plugin_dumpGimpleSSA)()
{
   return {
       LLVM_PLUGIN_API_VERSION, CLANG_VERSION_STRING_DUMP_SSA, "v0.12", [](llvm::PassBuilder& PB) {
          const auto load = [](llvm::ModulePassManager& MPM, bool doOpt) {
             llvm::FunctionPassManager FPM0;
             FPM0.addPass(llvm::LowerAtomicPass());
             MPM.addPass(llvm::createModuleToFunctionPassAdaptor(std::move(FPM0)));
             if(doOpt)
             {
                MPM.addPass(llvm::GlobalDCEPass());
                MPM.addPass(llvm::ForceFunctionAttrsPass());
                MPM.addPass(llvm::InferFunctionAttrsPass());
                MPM.addPass(llvm::createModuleToPostOrderCGSCCPassAdaptor(llvm::PostOrderFunctionAttrsPass()));
                MPM.addPass(llvm::ReversePostOrderFunctionAttrsPass());
                MPM.addPass(llvm::GlobalSplitPass());
                MPM.addPass(llvm::GlobalOptPass());
             }
             MPM.addPass(createModuleToFunctionPassAdaptor(llvm::PromotePass()));
             if(doOpt)
             {
                MPM.addPass(llvm::DeadArgumentEliminationPass());
                llvm::FunctionPassManager PeepholeFPM;
                PeepholeFPM.addPass(llvm::InstCombinePass());
                MPM.addPass(llvm::createModuleToFunctionPassAdaptor(std::move(PeepholeFPM)));
             }
             MPM.addPass(llvm::GlobalOptPass());
             MPM.addPass(llvm::GlobalDCEPass());
             MPM.addPass(llvm::createModuleToPostOrderCGSCCPassAdaptor(llvm::ArgumentPromotionPass(256)));
             llvm::FunctionPassManager FPM1a;
             FPM1a.addPass(llvm::InstCombinePass());
             MPM.addPass(llvm::createModuleToFunctionPassAdaptor(std::move(FPM1a)));
             if(doOpt)
             {
                llvm::FunctionPassManager FPM1;
                FPM1.addPass(llvm::JumpThreadingPass());
#if __clang_major__ >= 16
                FPM1.addPass(llvm::SROAPass(llvm::SROAOptions::ModifyCFG));
#endif
                FPM1.addPass(llvm::TailCallElimPass());
                MPM.addPass(llvm::createModuleToFunctionPassAdaptor(std::move(FPM1)));
                MPM.addPass(llvm::createModuleToPostOrderCGSCCPassAdaptor(llvm::PostOrderFunctionAttrsPass()));
                MPM.addPass(llvm::createModuleToFunctionPassAdaptor(llvm::InvalidateAnalysisPass<llvm::AAManager>()));

                llvm::FunctionPassManager FPM2;
                FPM2.addPass(llvm::createFunctionToLoopPassAdaptor(llvm::LICMPass(
#if __clang_major__ >= 16
                                                                       llvm::LICMOptions()
#endif
                                                                           ),
                                                                   /*USeMemorySSA=*/true,
                                                                   /*UseBlockFrequencyInfo=*/true));
                FPM2.addPass(llvm::NewGVNPass());
                FPM2.addPass(llvm::MemCpyOptPass());
                FPM2.addPass(llvm::DSEPass());
                FPM2.addPass(llvm::MergedLoadStoreMotionPass());

                llvm::LoopPassManager LPM2;
                LPM2.addPass(llvm::LoopRotatePass());
#if __clang_major__ >= 16
                LPM2.addPass(llvm::LoopFlattenPass());
#endif
                LPM2.addPass(llvm::IndVarSimplifyPass());
                LPM2.addPass(llvm::LoopDeletionPass());
                LPM2.addPass(llvm::LoopRotatePass());
                FPM2.addPass(llvm::createFunctionToLoopPassAdaptor(std::move(LPM2), /*UseMemorySSA=*/false,
                                                                   /*UseBlockFrequencyInfo=*/true));
                FPM2.addPass(llvm::LoopFusePass());
                FPM2.addPass(llvm::JumpThreadingPass());
                MPM.addPass(llvm::createModuleToFunctionPassAdaptor(std::move(FPM2)));
                MPM.addPass(createModuleToFunctionPassAdaptor(llvm::SimplifyCFGPass(llvm::SimplifyCFGOptions()
#if __clang_major__ >= 16
                                                                                        .convertSwitchRangeToICmp(true)
#endif
                                                                                        .sinkCommonInsts(true)
                                                                                        .hoistCommonInsts(true))));
                MPM.addPass(llvm::GlobalDCEPass());
                MPM.addPass(llvm::MergeFunctionsPass());
             }
             llvm::FunctionPassManager FPM3;
             FPM3.addPass(llvm::UnifyFunctionExitNodesPass());
             MPM.addPass(llvm::createModuleToFunctionPassAdaptor(std::move(FPM3)));
             MPM.addPass(llvm::CLANG_VERSION_SYMBOL_DUMP_SSA());
             return true;
          };
          PB.registerPipelineParsingCallback([&](llvm::StringRef Name, llvm::ModulePassManager& MPM,
                                                 llvm::ArrayRef<llvm::PassBuilder::PipelineElement>) {
             if(Name == CLANG_VERSION_STRING_DUMP_SSA)
             {
                return load(MPM, false);
             }
             return false;
          });
          PB.registerOptimizerLastEPCallback([&](llvm::ModulePassManager& MPM,
#if __clang_major__ <= 13
                                                 llvm::PassBuilder::OptimizationLevel Opt
#else
                  llvm::OptimizationLevel Opt
#endif
                                             ) {
             return load(
                 MPM,
#if __clang_major__ <= 13
                 Opt != llvm::PassBuilder::OptimizationLevel::O0 && Opt != llvm::PassBuilder::OptimizationLevel::O1
#else
                     Opt != llvm::OptimizationLevel::O0 && Opt != llvm::OptimizationLevel::O1
#endif
             );
          });
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
   PM.add(llvm::createUnifyFunctionExitNodesPass());

   PM.add(new llvm::CLANG_VERSION_SYMBOL_DUMP_SSA());
}

static llvm::RegisterStandardPasses llvmtoolLoader_Ox(llvm::PassManagerBuilder::EP_OptimizerLast, loadPass);

//    static void loadPassEarly(const llvm::PassManagerBuilder&, llvm::legacy::PassManagerBase& PM)
//    {
//       PM.add(llvm::createPromoteMemoryToRegisterPass());
//       PM.add(llvm::createGlobalOptimizerPass());
//       PM.add(new llvm::CLANG_VERSION_SYMBOL_DUMP_SSA < true > ());
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
// INITIALIZE_PASS_BEGIN(CLANG_VERSION_SYMBOL_DUMP_SSA, CLANG_VERSION_STRING_DUMP_SSA,
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
// INITIALIZE_PASS_END(CLANG_VERSION_SYMBOL_DUMP_SSA, CLANG_VERSION_STRING_DUMP_SSA,
//                     "Dump gimple ssa raw format starting from LLVM IR: LLVM pass", false, false)
