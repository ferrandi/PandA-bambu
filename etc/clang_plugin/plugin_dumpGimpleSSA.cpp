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
*              Copyright (c) 2004-2017 Politecnico di Milano
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

#include "clang/Frontend/FrontendPluginRegistry.h"
#include "clang/AST/AST.h"
#include "clang/AST/ASTConsumer.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Sema/Sema.h"

namespace llvm {
   struct CLANG_VERSION_SYMBOL(_plugin_dumpGimpleSSAPass);
}

static clang::DumpGimpleRaw *gimpleRawWriter;

namespace clang {

   class CLANG_VERSION_SYMBOL(_plugin_dumpGimpleSSA) : public PluginASTAction
   {
         std::string outdir_name;
         friend struct llvm::CLANG_VERSION_SYMBOL(_plugin_dumpGimpleSSAPass);
      protected:
         std::unique_ptr<ASTConsumer> CreateASTConsumer(CompilerInstance &CI,
                                                        llvm::StringRef InFile) override
         {
            DiagnosticsEngine &D = CI.getDiagnostics();
            if(outdir_name=="")
               D.Report(D.getCustomDiagID(DiagnosticsEngine::Error,
                                       "outputdir argument not specified"));
            gimpleRawWriter = new DumpGimpleRaw(CI, outdir_name, InFile, false);
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
            ros << "Help for " CLANG_VERSION_STRING(_plugin_dumpGimpleSSA) " plugin\n";
            ros << "-outputdir <directory>\n";
            ros << "  Directory where the raw file will be written\n";
         }

         PluginASTAction::ActionType getActionType() override
         {
            return AddAfterMainAction;
         }

      public:

         CLANG_VERSION_SYMBOL(_plugin_dumpGimpleSSA)() {}
         CLANG_VERSION_SYMBOL(_plugin_dumpGimpleSSA)(const CLANG_VERSION_SYMBOL(_plugin_dumpGimpleSSA)& step) = delete;
         CLANG_VERSION_SYMBOL(_plugin_dumpGimpleSSA) & operator=(const CLANG_VERSION_SYMBOL(_plugin_dumpGimpleSSA)&) = delete;

   };

}

/// Currently there is no difference between c++ or c serialization
#if CPP_LANGUAGE
static clang::FrontendPluginRegistry::Add<clang::CLANG_VERSION_SYMBOL(_plugin_dumpGimpleSSA)>
Y(CLANG_VERSION_STRING(_plugin_dumpGimpleSSACpp), "Dump gimple ssa raw format starting from LLVM IR when c++ is processed");
#else
static clang::FrontendPluginRegistry::Add<clang::CLANG_VERSION_SYMBOL(_plugin_dumpGimpleSSA)>
X(CLANG_VERSION_STRING(_plugin_dumpGimpleSSA), "Dump gimple ssa raw format starting from LLVM IR");
#endif


#include "llvm/Pass.h"
#include "llvm/PassRegistry.h"
#include "llvm/IR/PassManager.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/Transforms/IPO/PassManagerBuilder.h"
#include "llvm/Transforms/Utils/LoopUtils.h"
#include "llvm/Analysis/LazyValueInfo.h"
#include "llvm/Analysis/AliasAnalysis.h"
//#include "llvm/Analysis/TypeBasedAliasAnalysis.h"
//#include "llvm/Analysis/CFLSteensAliasAnalysis.h"
#include "llvm/Analysis/MemoryDependenceAnalysis.h"
#include "llvm/Analysis/TargetTransformInfo.h"
#if __clang_major__ >= 5
#include "llvm/Analysis/MemorySSA.h"
#else
#include "llvm/Transforms/Utils/MemorySSA.h"
#endif
#include "llvm/Transforms/Scalar.h"
#include "llvm/InitializePasses.h"
#include "llvm-c/Transforms/Scalar.h"
#include "llvm/CodeGen/Passes.h"

namespace llvm {

   class llvm;

   struct CLANG_VERSION_SYMBOL(_plugin_dumpGimpleSSAPass): public ModulePass
   {
         static char ID;
         CLANG_VERSION_SYMBOL(_plugin_dumpGimpleSSAPass)() : ModulePass(ID)
         {
            initializeLoopPassPass(*PassRegistry::getPassRegistry());
            initializeLazyValueInfoWrapperPassPass(*PassRegistry::getPassRegistry());
            initializeMemoryDependenceWrapperPassPass(*PassRegistry::getPassRegistry());
            initializeMemorySSAWrapperPassPass(*PassRegistry::getPassRegistry());
            //initializeMemorySSAPrinterLegacyPassPass(*PassRegistry::getPassRegistry());
            initializeAAResultsWrapperPassPass(*PassRegistry::getPassRegistry());
//            initializeCFLSteensAAWrapperPassPass(*PassRegistry::getPassRegistry());
//            initializeTypeBasedAAWrapperPassPass(*PassRegistry::getPassRegistry());
            initializeTargetTransformInfoWrapperPassPass(*PassRegistry::getPassRegistry());
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
           AU.addRequired<MemoryDependenceWrapperPass>();
           AU.addRequired<MemorySSAWrapperPass>();
//           AU.addRequired<MemorySSAPrinterLegacyPass>();
           AU.addRequired<LazyValueInfoWrapperPass>();
           AU.addRequired<AAResultsWrapperPass>();
//           AU.addRequired<CFLSteensAAWrapperPass>();
//           AU.addRequired<TypeBasedAAWrapperPass>();

           AU.addPreserved<MemorySSAWrapperPass>();
           //AU.addPreserved<MemorySSAPrinterLegacyPass>();
           AU.addRequired<TargetTransformInfoWrapperPass>();
         }
   };
   char CLANG_VERSION_SYMBOL(_plugin_dumpGimpleSSAPass)::ID = 0;

}

static llvm::RegisterPass<llvm::CLANG_VERSION_SYMBOL(_plugin_dumpGimpleSSAPass)> XPass(CLANG_VERSION_STRING(_plugin_dumpGimpleSSAPass), "Dump gimple ssa raw format starting from LLVM IR: LLVM pass",
                                false /* Only looks at CFG */,
                                false /* Analysis Pass */);

// This function is of type PassManagerBuilder::ExtensionFn
static void loadPass(const llvm::PassManagerBuilder &PMB, llvm::legacy::PassManagerBase &PM)
{
   //PM.add(llvm::createCodeGenPreparePass());
   PM.add(llvm::createCFGSimplificationPass());
   if(PMB.OptLevel >= 2)
   {
      PM.add(llvm::createDeadStoreEliminationPass());
      PM.add(llvm::createAggressiveDCEPass());
      PM.add(llvm::createLoopLoadEliminationPass());
      PM.add(llvm::createPromoteMemoryToRegisterPass());
   }
   PM.add(new llvm::CLANG_VERSION_SYMBOL(_plugin_dumpGimpleSSAPass)());
}
// These constructors add our pass to a list of global extensions.
static llvm::RegisterStandardPasses clangtoolLoader_Ox(llvm::PassManagerBuilder::EP_OptimizerLast, loadPass);
