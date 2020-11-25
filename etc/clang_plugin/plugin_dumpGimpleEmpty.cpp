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
 * @file plugin_dumpGimpleEmpty.cpp
 * @brief Plugin to dump global variables in gimple raw format starting from LLVM IR
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 *
 */
#include "plugin_includes.hpp"

#include "llvm/IR/LegacyPassManager.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/PassManager.h"
#include "llvm/InitializePasses.h"
#include "llvm/Pass.h"
#include "llvm/PassRegistry.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Transforms/IPO/PassManagerBuilder.h"
#include "llvm/Transforms/Utils/LoopUtils.h"
#if __clang_major__ >= 10
#include "llvm/Support/CommandLine.h"
#endif
namespace llvm
{
   cl::opt<std::string> outdir_nameGE("pandaGE-outputdir", cl::desc("Specify the directory where the gimple raw file will be written"), cl::value_desc("directory path"));
   cl::opt<std::string> InFileGE("pandaGE-infile", cl::desc("Specify the name of the compiled source file"), cl::value_desc("filename path"));
   struct CLANG_VERSION_SYMBOL(_plugin_dumpGimpleEmpty) : public ModulePass
   {
      static char ID;
      CLANG_VERSION_SYMBOL(_plugin_dumpGimpleEmpty)() : ModulePass(ID)
      {
         initializeLoopPassPass(*PassRegistry::getPassRegistry());
      }
      bool runOnModule(Module& M) override
      {
         if(outdir_nameGE.empty())
            return false;
         if(InFileGE.empty())
            llvm::report_fatal_error("-pandaGE-infile parameter not specified");
         DumpGimpleRaw gimpleRawWriter(outdir_nameGE, InFileGE, true, nullptr, false);
         const std::string empty;
         auto res = gimpleRawWriter.runOnModule(M, this, empty);
         return res;
      }
      StringRef getPassName() const override
      {
         return CLANG_VERSION_STRING(_plugin_dumpGimpleSSAPass);
      }
      void getAnalysisUsage(AnalysisUsage& AU) const override
      {
         AU.setPreservesAll();
         getLoopAnalysisUsage(AU);
      }
   };

   char CLANG_VERSION_SYMBOL(_plugin_dumpGimpleEmpty)::ID = 0;

} // namespace llvm

#ifndef _WIN32

static llvm::RegisterPass<llvm::CLANG_VERSION_SYMBOL(_plugin_dumpGimpleEmpty)> XPass(CLANG_VERSION_STRING(_plugin_dumpGimpleEmpty), "Dump gimple ssa raw format starting from LLVM IR: LLVM pass", false /* Only looks at CFG */, false /* Analysis Pass */);
#endif
// This function is of type PassManagerBuilder::ExtensionFn
static void loadPass(const llvm::PassManagerBuilder&, llvm::legacy::PassManagerBase& PM)
{
   PM.add(new llvm::CLANG_VERSION_SYMBOL(_plugin_dumpGimpleEmpty)());
}
// These constructors add our pass to a list of global extensions.
#if ADD_RSP
static llvm::RegisterStandardPasses CLANG_VERSION_SYMBOL(_plugin_dumpGimpleEmptyLoader_Ox)(llvm::PassManagerBuilder::EP_OptimizerLast, loadPass);
#endif

#ifdef _WIN32
using namespace llvm;

INITIALIZE_PASS_BEGIN(clang7_plugin_dumpGimpleEmpty, "clang7_plugin_dumpGimpleEmpty", "Dump gimple ssa raw format starting from LLVM IR: LLVM pass", false, false)
INITIALIZE_PASS_DEPENDENCY(MemoryDependenceWrapperPass)
INITIALIZE_PASS_DEPENDENCY(MemorySSAWrapperPass)
INITIALIZE_PASS_DEPENDENCY(LazyValueInfoWrapperPass)
INITIALIZE_PASS_DEPENDENCY(AAResultsWrapperPass)
INITIALIZE_PASS_DEPENDENCY(TargetTransformInfoWrapperPass)
INITIALIZE_PASS_DEPENDENCY(TargetLibraryInfoWrapperPass)
INITIALIZE_PASS_DEPENDENCY(AssumptionCacheTracker)
INITIALIZE_PASS_DEPENDENCY(DominatorTreeWrapperPass)
INITIALIZE_PASS_DEPENDENCY(DominanceFrontierWrapperPass)
INITIALIZE_PASS_END(clang7_plugin_dumpGimpleEmpty, "clang7_plugin_dumpGimpleEmpty", "Dump gimple ssa raw format starting from LLVM IR: LLVM pass", false, false)

namespace llvm
{
   void clang7_plugin_dumpGimpleEmpty_init()
   {
   }
} // namespace llvm
#endif
