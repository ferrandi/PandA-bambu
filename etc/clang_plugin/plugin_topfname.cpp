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
 * @file plugin_topfname.cpp
 * @brief In case topfname function is available all global objects but the top function can be private.
 * This is going to simplify quite much the obtained IR.
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 *
 */
// #undef NDEBUG
#include "plugin_includes.hpp"

#include "llvm/Analysis/CallGraph.h"
#include <llvm/ADT/StringExtras.h>
#include <llvm/ADT/StringSet.h>
#include <llvm/IR/LegacyPassManager.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/PassManager.h>
#include <llvm/InitializePasses.h>
#include <llvm/Pass.h>
#include <llvm/PassRegistry.h>
#include <llvm/Support/CommandLine.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Transforms/IPO.h>
#include <llvm/Transforms/IPO/Internalize.h>
#include <llvm/Transforms/IPO/PassManagerBuilder.h>

#if __clang_major__ >= 13
#include <llvm/Passes/PassBuilder.h>
#include <llvm/Passes/PassPlugin.h>
#endif

#define PUGIXML_NO_EXCEPTIONS
#define PUGIXML_HEADER_ONLY

#include <pugixml.hpp>

#include <cxxabi.h>
#include <list>
#include <set>
#include <sstream>
#include <string>

#define DEBUG_TYPE "top-fname"

// Helper to load an API list to preserve and expose it as a functor for internalization.
class PreserveSymbolList
{
 public:
   PreserveSymbolList()
   {
   }

   explicit PreserveSymbolList(const std::list<std::string>& symbolList)
   {
      ExternalNames.insert(symbolList.begin(), symbolList.end());
   }

   void addSymbols(const std::list<std::string>& symbolList)
   {
      ExternalNames.insert(symbolList.begin(), symbolList.end());
   }

   bool operator()(const llvm::GlobalValue& GV)
   {
      return ExternalNames.count(GV.getName());
   }

 private:
   // Contains the set of symbols loaded to preserve
   static llvm::StringSet<> ExternalNames;
};
llvm::StringSet<> PreserveSymbolList::ExternalNames;
static PreserveSymbolList preservedSyms;

#define DEF_BUILTIN(X, N, C, T, LT, B, F, NA, AT, IM, COND) N,
static const std::set<std::string> builtinsNames = {
#include "gcc/builtins.def"
};
#undef DEF_BUILTIN

static bool is_builtin_fn(const std::string& declname)
{
   return builtinsNames.count(std::string("__builtin_") + declname) + builtinsNames.count(declname);
}

static std::string getDemangled(const std::string& declname)
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

namespace llvm
{
   static cl::opt<std::string> TopFunctionName_TFP("panda-TFN", cl::desc("Specify the list of top function symbols"),
                                                   cl::value_desc("comma-separated list of top function symbols"));
   static cl::opt<bool> Internalize_TFP("panda-Internalize", cl::init(false),
                                        cl::desc("Specify if the global variables has to be internalized"));
   static cl::opt<std::string> ExternSymbolsList("panda-ESL",
                                                 cl::desc("Specify the list of symbols not to be internalized"),
                                                 cl::value_desc("comma-separated list of external symbols"));
   static cl::opt<std::string>
       outdir_name("internalize-outputdir",
                   cl::desc("Specify the directory where the external symbols file will be written"),
                   cl::value_desc("directory path"));
   static cl::opt<bool> add_noalias("add-noalias", cl::init(false), cl::desc("Force noalias to pointer parameters"),
                                    cl::value_desc("specify if pointer parameters are noalias"));

   struct CLANG_VERSION_SYMBOL(_plugin_topfname)
       : public ModulePass
#if __clang_major__ >= 13
         ,
         public PassInfoMixin<CLANG_VERSION_SYMBOL(_plugin_topfname)>
#endif
   {
      static char ID;

      CLANG_VERSION_SYMBOL(_plugin_topfname)
      () : ModulePass(ID)
      {
         initializeCallGraphWrapperPassPass(*PassRegistry::getPassRegistry());
      }

#if __clang_major__ >= 13
      CLANG_VERSION_SYMBOL(_plugin_topfname)
      (const CLANG_VERSION_SYMBOL(_plugin_topfname) &) : CLANG_VERSION_SYMBOL(_plugin_topfname)()
      {
      }
#endif

      bool exec(Module& M, const CallGraph& CG)
      {
         bool changed = false;
         bool hasTopFun = false;
         std::list<std::string> symbolList;
         std::vector<std::string> TopFunctionNames;
         std::vector<std::string> RootFunctionNames;

         const auto handleFunction = [&](Function* F, const std::string& fsymbol, const std::string& fname) {
            if(!F->isIntrinsic() && !F->isDeclaration())
            {
               LLVM_DEBUG(llvm::dbgs() << "Checking function: " << fsymbol << " | " << fname << " (" << F->getLinkage()
                                       << ")\n");
               if(is_builtin_fn(fsymbol) || is_builtin_fn(fname))
               {
                  LLVM_DEBUG(llvm::dbgs() << "  builtin\n");
                  symbolList.push_back(fsymbol);
               }
               if(llvm::find(RootFunctionNames, fsymbol) != RootFunctionNames.end() ||
                  llvm::find(RootFunctionNames, fname) != RootFunctionNames.end())
               {
                  LLVM_DEBUG(llvm::dbgs() << "  top function\n");
                  F->addFnAttr(Attribute::NoInline);
                  F->setLinkage(GlobalValue::LinkageTypes::ExternalLinkage);
#if __clang_major__ >= 7
                  F->setDSOLocal(false);
#endif
                  symbolList.push_back(fsymbol);
                  hasTopFun = true;
                  /// in case add noalias
                  if(add_noalias)
                  {
                     for(auto& par : F->args())
                     {
                        if(!par.hasNoAliasAttr() && par.getType()->isPointerTy())
                        {
                           par.addAttr(llvm::Attribute::NoAlias);
                        }
                     }
                  }
               }
            }
         };

         // Initialize top functions list
         for(std::size_t last = 0, it = 0; it < TopFunctionName_TFP.size(); last = it + 1)
         {
            it = TopFunctionName_TFP.find(",", last);
            const auto func_symbol = TopFunctionName_TFP.substr(last, it);
            LLVM_DEBUG(dbgs() << " - " << func_symbol << "\n");
            TopFunctionNames.push_back(func_symbol);
            RootFunctionNames.push_back(func_symbol);
         }
         pugi::xml_document doc;
         const auto arch_filename = outdir_name + "/architecture.xml";
         if(doc.load_file(arch_filename.c_str()))
         {
            for(auto& f : doc.child("module"))
            {
               const auto func_symbol = std::string(f.attribute("symbol").value());
               const auto func_name = std::string(f.attribute("name").value());
               const auto is_dataflow = !f.attribute("dataflow").empty();
               if(is_dataflow && (llvm::find(RootFunctionNames, func_name) == RootFunctionNames.end() ||
                                  llvm::find(RootFunctionNames, func_symbol) == RootFunctionNames.end()))
               {
                  LLVM_DEBUG(dbgs() << " - " << func_symbol << "|" << func_name << "\n");
                  RootFunctionNames.push_back(func_symbol);
               }
            }
         }
         if(TopFunctionNames.empty())
         {
            LLVM_DEBUG(llvm::dbgs() << "No top function specified\n");
            return false;
         }
         // Initialize external symbols list
         if(!ExternSymbolsList.empty())
         {
            std::stringstream ss(ExternSymbolsList);

            while(ss.good())
            {
               std::string substr;
               std::getline(ss, substr, ',');
               symbolList.push_back(substr);
            }
         }

         SmallPtrSet<Function*, 32> Reachable;
         for(auto&& CGN : CG)
         {
            if(!CGN.second)
            {
               continue;
            }
            auto fun = CGN.second->getFunction();
            if(!fun)
            {
               continue;
            }
            const auto fsymbol = fun->getName().str();
            const auto fname = getDemangled(fsymbol);
            if(!(is_builtin_fn(fsymbol) || is_builtin_fn(fname) ||
                 llvm::find(TopFunctionNames, fsymbol) != TopFunctionNames.end() ||
                 llvm::find(TopFunctionNames, fname) != TopFunctionNames.end()))
            {
               continue;
            }

            Reachable.insert(fun);
            handleFunction(fun, fsymbol, fname);

            SmallVector<const Function*, 8> Tmp({CGN.first});
            do
            {
               auto F = std::move(Tmp.back());
               Tmp.pop_back();

               for(auto&& N : *CG[F])
               {
                  if(!N.second)
                     continue;
                  auto funCalled = N.second->getFunction();
                  if(!funCalled)
                     continue;
                  if(Reachable.find(funCalled) != Reachable.end())
                     continue;

                  Tmp.push_back(funCalled);

                  const auto _fsymbol = funCalled->getName().str();
                  const auto _fname = getDemangled(_fsymbol);
                  handleFunction(funCalled, _fsymbol, _fname);
               }
            } while(!Tmp.empty());
         }

         if(!hasTopFun)
         {
            return changed;
         }
         LLVM_DEBUG(llvm::dbgs() << "Top function symbols: "
                                 << llvm::join(TopFunctionNames.begin(), TopFunctionNames.end(), ", ") << "\n"
                                 << "Root function symbols: "
                                 << llvm::join(RootFunctionNames.begin(), RootFunctionNames.end(), ", ") << "\n");
         symbolList.push_back("signgam");

         if(!Internalize_TFP)
         {
            for(auto& globalVar : M.getGlobalList())
            {
               const auto varName = globalVar.getName().str();
               symbolList.push_back(varName);
            }
         }
         preservedSyms.addSymbols(symbolList);
         if(!outdir_name.empty())
         {
            std::error_code EC;
            std::string filename = outdir_name + "external-symbols.txt";
#if __clang_major__ >= 7 && !defined(VVD)
            llvm::raw_fd_ostream stream(filename, EC, llvm::sys::fs::FA_Read | llvm::sys::fs::FA_Write);
#else
            llvm::raw_fd_ostream stream(filename, EC, llvm::sys::fs::F_RW);
#endif
            for(auto symb : symbolList)
            {
               stream << symb << "\n";
            }
         }

         return changed;
      }

      bool runOnModule(Module& M) override
      {
#if __clang_major__ < 13

         CallGraphWrapperPass* CGPass = getAnalysisIfAvailable<CallGraphWrapperPass>();
         if(!CGPass)
         {
            report_fatal_error("not able to retrieve the call graph");
         }
         return exec(M, CGPass->getCallGraph());
#else
         report_fatal_error("Call to runOnModule not expected with current LLVM version");
         return false;
#endif
      }

      StringRef getPassName() const override
      {
         return CLANG_VERSION_STRING(_plugin_topfname);
      }

      void getAnalysisUsage(AnalysisUsage& AU) const override
      {
         AU.addRequired<CallGraphWrapperPass>();
      }

#if __clang_major__ >= 13
      llvm::PreservedAnalyses run(llvm::Module& M, llvm::ModuleAnalysisManager& MAM)
      {
         const auto changed = exec(M, MAM.getResult<CallGraphAnalysis>(M));
         return (changed ? llvm::PreservedAnalyses::none() : llvm::PreservedAnalyses::all());
      }
#endif
   };

   char CLANG_VERSION_SYMBOL(_plugin_topfname)::ID = 0;

} // namespace llvm

#ifndef _WIN32
static llvm::RegisterPass<llvm::CLANG_VERSION_SYMBOL(_plugin_topfname)>
    XPass(CLANG_VERSION_STRING(_plugin_topfname), "Make all private/static but the top function",
          false /* Only looks at CFG */, false /* Analysis Pass */);
#endif

#if __clang_major__ >= 13
llvm::PassPluginLibraryInfo CLANG_PLUGIN_INFO(_plugin_topfname)()
{
   return {LLVM_PLUGIN_API_VERSION, CLANG_VERSION_STRING(_plugin_topfname), "v0.12", [](llvm::PassBuilder& PB) {
              const auto load = [](llvm::ModulePassManager& MPM) {
                 MPM.addPass(llvm::CLANG_VERSION_SYMBOL(_plugin_topfname)());
                 MPM.addPass(llvm::InternalizePass(preservedSyms));
                 return true;
              };
              PB.registerPipelineParsingCallback([&](llvm::StringRef Name, llvm::ModulePassManager& MPM,
                                                     llvm::ArrayRef<llvm::PassBuilder::PipelineElement>) {
                 if(Name == CLANG_VERSION_STRING(_plugin_topfname))
                 {
                    return load(MPM);
                 }
                 return false;
              });
              PB.registerPipelineEarlySimplificationEPCallback([&](llvm::ModulePassManager& MPM,
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
   return CLANG_PLUGIN_INFO(_plugin_topfname)();
}
#else
#if ADD_RSP
// This function is of type PassManagerBuilder::ExtensionFn
static void loadPass(const llvm::PassManagerBuilder&, llvm::legacy::PassManagerBase& PM)
{
   PM.add(new llvm::CLANG_VERSION_SYMBOL(_plugin_topfname)());
   PM.add(llvm::createInternalizePass(preservedSyms));
}

// These constructors add our pass to a list of global extensions.
static llvm::RegisterStandardPasses
    CLANG_VERSION_SYMBOL(_plugin_topfname_Ox)(llvm::PassManagerBuilder::EP_ModuleOptimizerEarly, loadPass);
#endif
#endif

// using namespace llvm;
//
// namespace llvm
// {
//    void CLANG_PLUGIN_INIT(_plugin_topfname)(PassRegistry&);
// } // namespace llvm
//
// INITIALIZE_PASS_BEGIN(CLANG_VERSION_SYMBOL(_plugin_topfname), CLANG_VERSION_STRING(_plugin_topfname),
//                       "Make all private/static but the top function", false, false)
// INITIALIZE_PASS_END(CLANG_VERSION_SYMBOL(_plugin_topfname), CLANG_VERSION_STRING(_plugin_topfname),
//                     "Make all private/static but the top function", false, false)
