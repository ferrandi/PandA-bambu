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
 * @brief In case topfname function is available all global objects but the top function can be private.
 * This is going to simplify quite much the obtained IR.
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
#include "llvm/Support/CommandLine.h"
#include "llvm/Transforms/IPO/PassManagerBuilder.h"
#include "llvm/Transforms/Utils/LoopUtils.h"

#include <cxxabi.h>

#define PRINT_DBG_MSG 0

namespace llvm
{
   struct CLANG_VERSION_SYMBOL(_plugin_topfname);
}

namespace llvm
{
   cl::opt<std::string> TopFunctionName_DNEGP("panda-TFN", cl::desc("Specify the name of the top function"), cl::value_desc("name of the top function"));
   struct CLANG_VERSION_SYMBOL(_plugin_topfname) : public ModulePass
   {
      static char ID;
      static const std::set<std::string> builtinsNames;
      CLANG_VERSION_SYMBOL(_plugin_topfname)() : ModulePass(ID)
      {
         initializeLoopPassPass(*PassRegistry::getPassRegistry());
      }
      std::string getDemangled(const std::string& declname)
      {
         int status;
         char* demangled_outbuffer = abi::__cxa_demangle(declname.c_str(), nullptr, nullptr, &status);
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

         assert(demangled_outbuffer == nullptr);

         return declname;
      }
      bool is_builtin_fn(const std::string& declname) const
      {
         return builtinsNames.find(std::string("__builtin_") + declname) != builtinsNames.end() || builtinsNames.find(declname) != builtinsNames.end();
      }

      bool runOnModule(Module& M) override
      {
         bool changed = false;
         bool hasTopFun = false;
         if(TopFunctionName_DNEGP.empty())
            llvm::report_fatal_error("-panda-TFN parameter not specified");
         for(auto& fun : M.getFunctionList())
         {
            if(!fun.isIntrinsic() && !fun.isDeclaration())
            {
               auto funName = fun.getName();
               auto demangled = getDemangled(funName);
               if(!fun.hasInternalLinkage() && (funName == TopFunctionName_DNEGP || demangled == TopFunctionName_DNEGP))
               {
                  hasTopFun = true;
               }
            }
         }
         if(!hasTopFun)
            return changed;
         /// check if the translation unit has the top function name
#if PRINT_DBG_MSG
         llvm::errs() << "Top function name: " << TopFunctionName_DNEGP << "\n";
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
               if(!fun.hasInternalLinkage() && funName != TopFunctionName_DNEGP && demangled != TopFunctionName_DNEGP && !is_builtin_fn(funName) && !is_builtin_fn(demangled))
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
      StringRef getPassName() const override
      {
         return CLANG_VERSION_STRING(_plugin_topfname);
      }
      void getAnalysisUsage(AnalysisUsage& AU) const override
      {
         getLoopAnalysisUsage(AU);
      }
   };

   char CLANG_VERSION_SYMBOL(_plugin_topfname)::ID = 0;

#define DEF_BUILTIN(X, N, C, T, LT, B, F, NA, AT, IM, COND) N,
   const std::set<std::string> CLANG_VERSION_SYMBOL(_plugin_topfname)::builtinsNames = {
#include "gcc/builtins.def"
   };
#undef DEF_BUILTIN

} // namespace llvm

static llvm::RegisterPass<llvm::CLANG_VERSION_SYMBOL(_plugin_topfname)> XPass(CLANG_VERSION_STRING(_plugin_topfname), "Make all private/static but the top function", false /* Only looks at CFG */, false /* Analysis Pass */);

// This function is of type PassManagerBuilder::ExtensionFn
static void loadPass(const llvm::PassManagerBuilder&, llvm::legacy::PassManagerBase& PM)
{
   PM.add(new llvm::CLANG_VERSION_SYMBOL(_plugin_topfname)());
}
// These constructors add our pass to a list of global extensions.
static llvm::RegisterStandardPasses CLANG_VERSION_SYMBOL(_plugin_topfname_Ox)(llvm::PassManagerBuilder::EP_ModuleOptimizerEarly, loadPass);
