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
 *              Copyright (c) 2018 Politecnico di Milano
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

namespace llvm
{
   cl::opt<std::string> outdir_name("panda-outputdir", cl::desc("Specify the directory where the gimple raw file will be written"), cl::value_desc("directory path"));
   cl::opt<std::string> InFile("panda-infile", cl::desc("Specify the name of the compiled source file"), cl::value_desc("filename path"));
   struct CLANG_VERSION_SYMBOL(_plugin_dumpGimpleEmpty) : public ModulePass
   {
      static char ID;
      CLANG_VERSION_SYMBOL(_plugin_dumpGimpleEmpty)() : ModulePass(ID)
      {
         initializeLoopPassPass(*PassRegistry::getPassRegistry());
      }
      bool runOnModule(Module& M) override
      {
         if(outdir_name.empty())
            llvm::report_fatal_error("-panda-outputdir parameter not specified");
         if(InFile.empty())
            llvm::report_fatal_error("-panda-infile parameter not specified");
         DumpGimpleRaw gimpleRawWriter(outdir_name, InFile, true, nullptr);
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

static llvm::RegisterPass<llvm::CLANG_VERSION_SYMBOL(_plugin_dumpGimpleEmpty)> XPass(CLANG_VERSION_STRING(_plugin_dumpGimpleEmpty), "Dump gimple ssa raw format starting from LLVM IR: LLVM pass", false /* Only looks at CFG */,
                                                                                         false /* Analysis Pass */);

// This function is of type PassManagerBuilder::ExtensionFn
static void loadPass(const llvm::PassManagerBuilder&, llvm::legacy::PassManagerBase& PM)
{
   PM.add(new llvm::CLANG_VERSION_SYMBOL(_plugin_dumpGimpleEmpty)());
}
// These constructors add our pass to a list of global extensions.
static llvm::RegisterStandardPasses CLANG_VERSION_SYMBOL(_plugin_dumpGimpleEmptyLoader_Ox)(llvm::PassManagerBuilder::EP_OptimizerLast, loadPass);
