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
 *              Copyright (C) 2018-2020 Politecnico di Milano
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

#include "plugin_includes.hpp"

#include "llvm/Analysis/AliasAnalysis.h"
#include "llvm/Analysis/AssumptionCache.h"
#include "llvm/Analysis/CFLSteensAliasAnalysis.h"
#include "llvm/Analysis/DominanceFrontier.h"
#include "llvm/Analysis/LazyValueInfo.h"
#include "llvm/Analysis/LoopInfo.h"
#include "llvm/Analysis/MemoryDependenceAnalysis.h"
#include "llvm/Analysis/TargetTransformInfo.h"
#include "llvm/Analysis/TypeBasedAliasAnalysis.h"
#include "llvm/IR/Dominators.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/IR/PassManager.h"
#include "llvm/Pass.h"
#include "llvm/PassRegistry.h"
#include "llvm/Transforms/IPO/PassManagerBuilder.h"
#include "llvm/Transforms/Utils/LoopUtils.h"
#if __clang_major__ != 4
#include "llvm/Analysis/MemorySSA.h"
#else
#include "llvm/Transforms/Utils/MemorySSA.h"
#endif
#include "llvm-c/Transforms/Scalar.h"
#include "llvm/CodeGen/Passes.h"
#include "llvm/InitializePasses.h"
#include "llvm/Support/MemoryBuffer.h"
#include "llvm/Transforms/IPO.h"
#include "llvm/Transforms/Scalar.h"
#include "llvm/Transforms/Utils/UnifyFunctionExitNodes.h"
#if __clang_major__ >= 7
#include "llvm/Transforms/Utils.h"
#endif
#include <sstream>

#include <boost/tokenizer.hpp>

#define PRINT_DBG_MSG 0

namespace llvm
{
   cl::opt<std::string> TopFunctionName("panda-topfname", cl::desc("Specify the name of the top function"), cl::value_desc("name of the top function"));
   cl::opt<std::string> outdir_name("panda-outputdir", cl::desc("Specify the directory where the gimple raw file will be written"), cl::value_desc("directory path"));
   cl::opt<std::string> InFile("panda-infile", cl::desc("Specify the name of the compiled source file"), cl::value_desc("filename path"));

   template <bool earlyAnalysis>
   struct CLANG_VERSION_SYMBOL(_plugin_dumpGimpleSSA) : public ModulePass
   {
      static char ID;

      CLANG_VERSION_SYMBOL(_plugin_dumpGimpleSSA)() : ModulePass(ID)
      {
         initializeLoopPassPass(*PassRegistry::getPassRegistry());
         initializeLazyValueInfoWrapperPassPass(*PassRegistry::getPassRegistry());
         initializeMemoryDependenceWrapperPassPass(*PassRegistry::getPassRegistry());
         initializeMemorySSAWrapperPassPass(*PassRegistry::getPassRegistry());
         // initializeMemorySSAPrinterLegacyPassPass(*PassRegistry::getPassRegistry());
         initializeAAResultsWrapperPassPass(*PassRegistry::getPassRegistry());
         initializeCFLSteensAAWrapperPassPass(*PassRegistry::getPassRegistry());
         initializeTypeBasedAAWrapperPassPass(*PassRegistry::getPassRegistry());
         initializeTargetTransformInfoWrapperPassPass(*PassRegistry::getPassRegistry());
         initializeTargetLibraryInfoWrapperPassPass(*PassRegistry::getPassRegistry());
         initializeAssumptionCacheTrackerPass(*PassRegistry::getPassRegistry());
         initializeDominatorTreeWrapperPassPass(*PassRegistry::getPassRegistry());
         initializeDominanceFrontierWrapperPassPass(*PassRegistry::getPassRegistry());
         initializeUnifyFunctionExitNodesPass(*PassRegistry::getPassRegistry());
      }

      std::string create_file_basename_string(const std::string& on, const std::string& original_filename)
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

      bool runOnModule(Module& M) override
      {
         if(outdir_name.empty())
            return false;
         if(InFile.empty())
            llvm::report_fatal_error("-panda-infile parameter not specified");
         /// load parameter names
         boost::char_separator<char> sep(",");
         boost::tokenizer<boost::char_separator<char>> FileTokenizer(InFile, sep);
         std::map<std::string, std::vector<std::string>> Fun2Params;
         for(auto file_string : FileTokenizer)
         {
            auto parms_file_name = create_file_basename_string(outdir_name, file_string) + ".params.txt";
            ErrorOr<std::unique_ptr<MemoryBuffer>> BufOrErr = MemoryBuffer::getFile(parms_file_name);
            if(BufOrErr)
            {
               std::unique_ptr<MemoryBuffer> Buffer = std::move(BufOrErr.get());
               std::string buf = Buffer->getBuffer();
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

#if PRINT_DBG_MSG
         if(!TopFunctionName.empty())
            llvm::errs() << "Top function name: " << TopFunctionName << "\n";
#endif
         auto res = gimpleRawWriter.runOnModule(M, this, TopFunctionName);
         return res;
      }
      StringRef getPassName() const override
      {
         return CLANG_VERSION_STRING(_plugin_dumpGimpleSSA);
      }
      void getAnalysisUsage(AnalysisUsage& AU) const override
      {
         AU.addRequired<UnifyFunctionExitNodes>();
         AU.addRequired<CFLSteensAAWrapperPass>();
         AU.addRequired<TypeBasedAAWrapperPass>();
         getLoopAnalysisUsage(AU);

         AU.addRequired<MemoryDependenceWrapperPass>();
         AU.addRequired<MemorySSAWrapperPass>();
         //           AU.addRequired<MemorySSAPrinterLegacyPass>();
         AU.addRequired<LazyValueInfoWrapperPass>();
         AU.addRequired<AAResultsWrapperPass>();

         AU.addPreserved<MemorySSAWrapperPass>();
         // AU.addPreserved<MemorySSAPrinterLegacyPass>();
         AU.addRequired<TargetTransformInfoWrapperPass>();
         AU.addRequired<TargetLibraryInfoWrapperPass>();
         AU.addRequired<AssumptionCacheTracker>();
         AU.addRequired<DominatorTreeWrapperPass>();
         AU.addRequired<DominanceFrontierWrapperPass>();
      }
   };
   template <>
   char CLANG_VERSION_SYMBOL(_plugin_dumpGimpleSSA)<false>::ID = 0;
   template <>
   char CLANG_VERSION_SYMBOL(_plugin_dumpGimpleSSA)<true>::ID = 0;

} // namespace llvm

// Currently there is no difference between c++ or c serialization
#ifndef _WIN32
#if CPP_LANGUAGE
// static llvm::RegisterPass<llvm::CLANG_VERSION_SYMBOL(_plugin_dumpGimpleSSA)<true>> XPassEarly(CLANG_VERSION_STRING(_plugin_dumpGimpleSSACppEarly), "Custom Value Range Based optimization step: LLVM pass", false /* Only looks at CFG */, false /* Analysis
// Pass */);
static llvm::RegisterPass<llvm::CLANG_VERSION_SYMBOL(_plugin_dumpGimpleSSA) < false>>
    XPass(CLANG_VERSION_STRING(_plugin_dumpGimpleSSACpp), "Dump gimple ssa raw format starting from LLVM IR: LLVM pass", false /* Only looks at CFG */, false /* Analysis Pass */);
#else
// static llvm::RegisterPass<llvm::CLANG_VERSION_SYMBOL(_plugin_dumpGimpleSSA)<true>> XPassEarly(CLANG_VERSION_STRING(_plugin_dumpGimpleSSAEarly), "Custom Value Range Based optimization step: LLVM pass", false /* Only looks at CFG */, false /* Analysis
// Pass */);
static llvm::RegisterPass<llvm::CLANG_VERSION_SYMBOL(_plugin_dumpGimpleSSA) < false>>
    XPass(CLANG_VERSION_STRING(_plugin_dumpGimpleSSA), "Dump gimple ssa raw format starting from LLVM IR: LLVM pass", false /* Only looks at CFG */, false /* Analysis Pass */);
#endif
#endif
#if ADD_RSP
// This function is of type PassManagerBuilder::ExtensionFn
static void loadPass(const llvm::PassManagerBuilder&, llvm::legacy::PassManagerBase& PM)
{
   PM.add(llvm::createLowerAtomicPass());
   PM.add(llvm::createPromoteMemoryToRegisterPass());
   PM.add(llvm::createGlobalOptimizerPass());
   PM.add(llvm::createBreakCriticalEdgesPass());
   PM.add(new llvm::CLANG_VERSION_SYMBOL(_plugin_dumpGimpleSSA) < false > ());
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

// static llvm::RegisterStandardPasses llvmtoolLoader_O0(llvm::PassManagerBuilder::EP_ModuleOptimizerEarly, loadPassEarly);
#endif

#ifdef _WIN32
using namespace llvm;

INITIALIZE_PASS_BEGIN(clang7_plugin_dumpGimpleSSA<false>, "clang7_plugin_dumpGimpleSSA", "Dump gimple ssa raw format starting from LLVM IR: LLVM pass", false, false)
INITIALIZE_PASS_DEPENDENCY(MemoryDependenceWrapperPass)
INITIALIZE_PASS_DEPENDENCY(MemorySSAWrapperPass)
INITIALIZE_PASS_DEPENDENCY(LazyValueInfoWrapperPass)
INITIALIZE_PASS_DEPENDENCY(AAResultsWrapperPass)
INITIALIZE_PASS_DEPENDENCY(TargetTransformInfoWrapperPass)
INITIALIZE_PASS_DEPENDENCY(TargetLibraryInfoWrapperPass)
INITIALIZE_PASS_DEPENDENCY(AssumptionCacheTracker)
INITIALIZE_PASS_DEPENDENCY(DominatorTreeWrapperPass)
INITIALIZE_PASS_DEPENDENCY(DominanceFrontierWrapperPass)
INITIALIZE_PASS_END(clang7_plugin_dumpGimpleSSA<false>, "clang7_plugin_dumpGimpleSSA", "Dump gimple ssa raw format starting from LLVM IR: LLVM pass", false, false)

namespace llvm
{
   void clang7_plugin_dumpGimpleSSA_init()
   {
   }
} // namespace llvm

#endif
