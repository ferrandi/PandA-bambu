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
 * @file plugin_topfname.cpp
 * @brief Dummy Plugin. LLVM does not need this plugin but we add just for the sake of completeness.
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 *
 */
#include "plugin_includes.hpp"

#include "clang/AST/AST.h"
#include "clang/AST/ASTConsumer.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Frontend/FrontendPluginRegistry.h"
#include "clang/Sema/Sema.h"
#include "llvm/Support/raw_ostream.h"

#include <cxxabi.h>

#define PRINT_DBG_MSG 0

namespace llvm
{
   struct CLANG_VERSION_SYMBOL(_plugin_DoNotExposeGlobalsPass);
}

static std::string TopFunctionName;

namespace clang
{
   class CLANG_VERSION_SYMBOL(_plugin_topfname) : public PluginASTAction
   {
      std::string topfname;
      friend struct llvm::CLANG_VERSION_SYMBOL(_plugin_DoNotExposeGlobalsPass);

    protected:
      std::unique_ptr<ASTConsumer> CreateASTConsumer(CompilerInstance&, llvm::StringRef) override
      {
         TopFunctionName = topfname;
         return llvm::make_unique<dummyConsumer>();
      }

      bool ParseArgs(const CompilerInstance& CI, const std::vector<std::string>& args) override
      {
         DiagnosticsEngine& D = CI.getDiagnostics();
         for(size_t i = 0, e = args.size(); i != e; ++i)
         {
            if(args.at(i) == "-topfname")
            {
               if(i + 1 >= e)
               {
                  D.Report(D.getCustomDiagID(DiagnosticsEngine::Error, "missing topfname argument"));
                  return false;
               }
               ++i;
               topfname = args.at(i);
            }
         }
         if(!args.empty() && args.at(0) == "-help")
            PrintHelp(llvm::errs());

         if(topfname == "")
            D.Report(D.getCustomDiagID(DiagnosticsEngine::Error, "topfname not specified"));
         return true;
      }
      void PrintHelp(llvm::raw_ostream& ros)
      {
         ros << "Help for " CLANG_VERSION_STRING(_plugin_topfname) "  plugin\n";
         ros << "-topfname <topfunctionname>\n";
         ros << "  name of the top function\n";
      }

      PluginASTAction::ActionType getActionType() override
      {
         return AddAfterMainAction;
      }
   };

} // namespace clang

static clang::FrontendPluginRegistry::Add<clang::CLANG_VERSION_SYMBOL(_plugin_topfname)> X(CLANG_VERSION_STRING(_plugin_topfname), "Dumy plugin");

#include "llvm/IR/LegacyPassManager.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/PassManager.h"
#include "llvm/InitializePasses.h"
#include "llvm/Pass.h"
#include "llvm/PassRegistry.h"
#include "llvm/Transforms/IPO/PassManagerBuilder.h"
#include "llvm/Transforms/Utils/LoopUtils.h"

namespace llvm
{
   struct CLANG_VERSION_SYMBOL(_plugin_DoNotExposeGlobalsPass) : public ModulePass
   {
      static char ID;
      static const std::set<std::string> builtinsNames;
      CLANG_VERSION_SYMBOL(_plugin_DoNotExposeGlobalsPass)() : ModulePass(ID)
      {
         initializeLoopPassPass(*PassRegistry::getPassRegistry());
      }
      std::string getDemangled(const std::string& declname)
      {
         int status;
         char* demangled_outbuffer = abi::__cxa_demangle(declname.c_str(), NULL, NULL, &status);
         if(status == 0)
         {
            std::string res = declname;
            if(std::string(demangled_outbuffer).find_last_of('(') != std::string::npos)
            {
               res = demangled_outbuffer;
               auto parPos = res.find('(');
               assert(parPos != std::string::npos);
               res = res.substr(0, parPos);
            }
            free(demangled_outbuffer);
            return res;
         }
         else
            assert(demangled_outbuffer == nullptr);
         return declname;
      }
      bool is_builtin_fn(const std::string& declname) const
      {
         if(builtinsNames.find(std::string("__builtin_") + declname) != builtinsNames.end() || builtinsNames.find(declname) != builtinsNames.end())
            return true;
         else
            return false;
      }

      bool runOnModule(Module& M)
      {
         bool changed = false;
#if PRINT_DBG_MSG
         llvm::errs() << "Top function name: " << TopFunctionName << "\n";
#endif
         for(auto& globalVar : M.getGlobalList())
         {
            std::string varName = std::string(globalVar.getName());
#if PRINT_DBG_MSG
            llvm::errs() << "Found global name: " << varName << "\n";
#endif
            if(varName == "llvm.global_ctors" || varName == "llvm.global_dtors" || varName == "llvm.used" || varName == "llvm.compiler.used")
            {
#if PRINT_DBG_MSG
               llvm::errs() << "Global intrinsic skipped: " << globalVar.getName() << "\n";
#endif
            }
            else if(!globalVar.hasInternalLinkage())
            {
#if PRINT_DBG_MSG
               llvm::errs() << "it becomes internal\n";
#endif
               changed = true;
               globalVar.setLinkage(llvm::GlobalValue::InternalLinkage);
            }
         }
         for(auto& fun : M.getFunctionList())
         {
            if(fun.isIntrinsic() || fun.isDeclaration())
            {
#if PRINT_DBG_MSG
               llvm::errs() << "Function intrinsic skipped: " << fun.getName() << "\n";
#endif
            }
            else
            {
               auto funName = fun.getName();
               auto demangled = getDemangled(funName);
#if PRINT_DBG_MSG
               llvm::errs() << "Found function: " << funName << "|" << demangled << "\n";
#endif
               if(!fun.hasInternalLinkage() && funName != TopFunctionName && demangled != TopFunctionName && !is_builtin_fn(funName) && !is_builtin_fn(demangled))
               {
#if PRINT_DBG_MSG
                  llvm::errs() << "it becomes internal\n";
#endif
                  changed = true;
                  fun.setLinkage(llvm::GlobalValue::InternalLinkage);
               }
            }
         }
         return changed;
      }
      virtual StringRef getPassName() const
      {
         return CLANG_VERSION_STRING(_plugin_DoNotExposeGlobalsPass);
      }
      void getAnalysisUsage(AnalysisUsage& AU) const
      {
         getLoopAnalysisUsage(AU);
      }
   };

   char CLANG_VERSION_SYMBOL(_plugin_DoNotExposeGlobalsPass)::ID = 0;

#define DEF_BUILTIN(X, N, C, T, LT, B, F, NA, AT, IM, COND) N,
   const std::set<std::string> CLANG_VERSION_SYMBOL(_plugin_DoNotExposeGlobalsPass)::builtinsNames = {
#include "gcc/builtins.def"
   };
#undef DEF_BUILTIN

} // namespace llvm

static llvm::RegisterPass<llvm::CLANG_VERSION_SYMBOL(_plugin_DoNotExposeGlobalsPass)> XPass(CLANG_VERSION_STRING(_plugin_DoNotExposeGlobalsPass), "Make all private/static but the top function", false /* Only looks at CFG */, false /* Analysis Pass */);

// This function is of type PassManagerBuilder::ExtensionFn
static void loadPass(const llvm::PassManagerBuilder&, llvm::legacy::PassManagerBase& PM)
{
   PM.add(new llvm::CLANG_VERSION_SYMBOL(_plugin_DoNotExposeGlobalsPass)());
}
// These constructors add our pass to a list of global extensions.
static llvm::RegisterStandardPasses CLANG_VERSION_SYMBOL(_plugin_DoNotExposeGlobalsLoader_Ox)(llvm::PassManagerBuilder::EP_ModuleOptimizerEarly, loadPass);
