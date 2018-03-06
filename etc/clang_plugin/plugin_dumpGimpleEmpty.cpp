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

#include "clang/Frontend/FrontendPluginRegistry.h"
#include "clang/AST/AST.h"
#include "clang/AST/ASTConsumer.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Sema/Sema.h"
#include "llvm/Support/raw_ostream.h"

static clang::DumpGimpleRaw *gimpleRawWriter;

namespace clang {


   class CLANG_VERSION_SYMBOL(_plugin_dumpGimpleEmpty) : public PluginASTAction
   {
         std::string outdir_name;
      protected:
         std::unique_ptr<ASTConsumer> CreateASTConsumer(CompilerInstance &CI,
                                                        llvm::StringRef InFile) override
         {
            DiagnosticsEngine &D = CI.getDiagnostics();
            if(outdir_name=="")
               D.Report(D.getCustomDiagID(DiagnosticsEngine::Error,
                                          "outputdir not specified"));
            gimpleRawWriter = new DumpGimpleRaw(CI, outdir_name, InFile, true);
            return llvm::make_unique<dummyConsumer>();
         }

         bool ParseArgs(const CompilerInstance &CI,
                        const std::vector<std::string> &args) override
         {
            DiagnosticsEngine &D = CI.getDiagnostics();
            for (size_t i = 0, e = args.size(); i != e; ++i)
            {
               if (args.at(i) == "-outputdir")
               {
                  if (i + 1 >= e) {
                     D.Report(D.getCustomDiagID(DiagnosticsEngine::Error,
                                                "missing outputdir argument"));
                     return false;
                  }
                  ++i;
                  outdir_name = args.at(i);
               }
            }
            if (!args.empty() && args.at(0) == "-help")
               PrintHelp(llvm::errs());

            if(outdir_name=="")
               D.Report(D.getCustomDiagID(DiagnosticsEngine::Error,
                                          "outputdir not specified"));
            return true;
         }
         void PrintHelp(llvm::raw_ostream& ros)
         {
            ros << "Help for " CLANG_VERSION_STRING(_plugin_dumpGimpleEmpty) " plugin\n";
            ros << "-outputdir <directory>\n";
            ros << "  Directory where the raw file will be written\n";
         }

         PluginASTAction::ActionType getActionType() override
         {
            return AddAfterMainAction;
         }
   };

}

static clang::FrontendPluginRegistry::Add<clang::CLANG_VERSION_SYMBOL(_plugin_dumpGimpleEmpty)>
X(CLANG_VERSION_STRING(_plugin_dumpGimpleEmpty), "Dump globals in a gimple ssa raw format starting from LLVM IR");



#include "llvm/Pass.h"
#include "llvm/IR/Module.h"
#include "llvm/PassRegistry.h"
#include "llvm/IR/PassManager.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/Transforms/IPO/PassManagerBuilder.h"
#include "llvm/Transforms/Utils/LoopUtils.h"
#include "llvm/InitializePasses.h"


namespace llvm {
   struct CLANG_VERSION_SYMBOL(_plugin_dumpGimpleEmptyPass): public ModulePass
   {
         static char ID;
         CLANG_VERSION_SYMBOL(_plugin_dumpGimpleEmptyPass)() : ModulePass(ID)
         {
            initializeLoopPassPass(*PassRegistry::getPassRegistry());
         }
         bool runOnModule(Module &M)
         {
            assert(gimpleRawWriter);
            auto res = gimpleRawWriter->runOnModule(M, this);
            delete gimpleRawWriter;
            gimpleRawWriter = nullptr;
            return res;
         }
         virtual StringRef getPassName() const
         {
            return CLANG_VERSION_STRING(_plugin_dumpGimpleSSAPass);
         }
         void getAnalysisUsage(AnalysisUsage &AU) const
         {
           AU.setPreservesAll();
           getLoopAnalysisUsage(AU);
         }
   };

   char CLANG_VERSION_SYMBOL(_plugin_dumpGimpleEmptyPass)::ID = 0;

}

static llvm::RegisterPass<llvm::CLANG_VERSION_SYMBOL(_plugin_dumpGimpleEmptyPass)> XPass(CLANG_VERSION_STRING(_plugin_dumpGimpleEmptyPass), "Dump gimple ssa raw format starting from LLVM IR: LLVM pass",
                                false /* Only looks at CFG */,
                                false /* Analysis Pass */);

// This function is of type PassManagerBuilder::ExtensionFn
static void loadPass(const llvm::PassManagerBuilder &, llvm::legacy::PassManagerBase &PM) {
  PM.add(new llvm::CLANG_VERSION_SYMBOL(_plugin_dumpGimpleEmptyPass)());
}
// These constructors add our pass to a list of global extensions.
static llvm::RegisterStandardPasses CLANG_VERSION_SYMBOL(_plugin_dumpGimpleEmptyLoader_Ox)(llvm::PassManagerBuilder::EP_OptimizerLast, loadPass);
