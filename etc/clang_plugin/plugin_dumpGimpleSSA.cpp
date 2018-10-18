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
* @file plugin_dumpGimpleSSA.cpp
* @brief Plugin to dump functions and global variables in gimple raw format starting from LLVM IR
*
* @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
*
*/

#include "plugin_includes.hpp"

#include "clang/Frontend/FrontendPluginRegistry.h"
#include "clang/AST/AST.h"
#include "clang/AST/Mangle.h"
#include "clang/AST/ASTConsumer.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/AST/Type.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Lex/Preprocessor.h"
#include "clang/Lex/LexDiagnostic.h"
#include "clang/Sema/Sema.h"
#include <sstream>
#include "llvm/Support/raw_ostream.h"

#define PRINT_DBG_MSG 0
//#define UNIFYFUNCTIONEXITNODES

namespace llvm {
   struct CLANG_VERSION_SYMBOL(_plugin_dumpGimpleSSAPass);
}

static clang::DumpGimpleRaw *gimpleRawWriter;
static std::string TopFunctionName;
static std::map<std::string,std::vector<std::string>> Fun2Params;
static std::map<std::string,std::string> HLS_interface_PragmaMap;

static std::map<std::string,std::vector<std::string>> HLS_interfaceMap;
static std::string interface_XML_filename;


namespace clang {

   static std::string create_file_name_string(const std::string &outdir_name, const std::string & original_filename, const std::string & fileSuffix)
   {
      std::size_t found = original_filename.find_last_of("/\\");
      std::string dump_base_name;
      if(found == std::string::npos)
         dump_base_name = original_filename;
      else
         dump_base_name = original_filename.substr(found+1);
      return outdir_name + "/" + dump_base_name + fileSuffix;
   }

   static void writeXML_interfaceFile(const std::string filename)
   {
      std::error_code EC;
      llvm::raw_fd_ostream stream(filename, EC, llvm::sys::fs::F_RW);

      stream << "<?xml version=\"1.0\"?>\n";
      stream << "<module>\n";
      for(auto funArgPair: Fun2Params)
      {
         if(TopFunctionName != "" && funArgPair.first != TopFunctionName) continue;
         bool hasInterfaceType = HLS_interfaceMap.find(funArgPair.first) != HLS_interfaceMap.end();
         if(hasInterfaceType)
         {
            stream << "  <function id=\""<<funArgPair.first<<"\">\n";
            const auto & interfaceTypeVec = HLS_interfaceMap.find(funArgPair.first)->second;
            unsigned int ArgIndex=0;
            for(auto par: funArgPair.second)
            {
               stream << "    <arg id=\""<<par<<"\" interface_type=\""<< interfaceTypeVec.at(ArgIndex)<<"\"/>\n";
               ++ArgIndex;
            }
            stream << "  </function>\n";
         }
      }
      stream << "</module>\n";

   }
   class FunctionArgConsumer : public clang::ASTConsumer
   {
         CompilerInstance &CI;
      public:
         FunctionArgConsumer(CompilerInstance &Instance) : CI(Instance) {}
         bool HandleTopLevelDecl(DeclGroupRef DG) override
         {
            for (DeclGroupRef::iterator i = DG.begin(), e = DG.end(); i != e; ++i) {
               const Decl *D = *i;
               if(const FunctionDecl * FD = dyn_cast<FunctionDecl>(D))
               {
                  if(!FD->isVariadic() && FD->hasBody())
                  {
                     const auto getMangledName = [&](const FunctionDecl* decl) {

                        auto mangleContext = decl->getASTContext().createMangleContext();

                        if (!mangleContext->shouldMangleDeclName(decl)) {
                           return decl->getNameInfo().getName().getAsString();
                        }
                        std::string mangledName;
                        llvm::raw_string_ostream ostream(mangledName);
                        mangleContext->mangleName(decl, ostream);
                        ostream.flush();
                        delete mangleContext;
                        return mangledName;
                     };
                     auto funName = getMangledName(FD);
                     for(const auto par : FD->parameters())
                     {
                        if (const ParmVarDecl *ND = dyn_cast<ParmVarDecl>(par))
                        {
                           std::string interfaceType="default";
                           std::string UserDefinedInterfaceType;
                           auto parName = ND->getNameAsString();
                           bool UDIT_p = false;
                           if(HLS_interface_PragmaMap.find(parName) != HLS_interface_PragmaMap.end())
                           {
                              UserDefinedInterfaceType = HLS_interface_PragmaMap.find(parName)->second;
                              UDIT_p = true;
                           }
                           auto argType = ND->getType().getTypePtr();
                           //argType->dump (llvm::errs() );
                           if(isa<DecayedType>(argType))
                           {
                              auto DT = cast<DecayedType>(argType);
                              if(DT->getOriginalType()->isConstantArrayType())
                              {
                                 auto CA = cast<ConstantArrayType>(DT->getOriginalType());
                                 auto OrigTotArraySize=CA->getSize();
                                 while(CA->getElementType()->isConstantArrayType())
                                 {
                                    CA = cast<ConstantArrayType>(CA->getElementType());
                                    OrigTotArraySize *= CA->getSize();
                                 }
                                 if(CA->getElementType()->isBuiltinType())
                                 {
                                    interfaceType="array-"+OrigTotArraySize.toString(10,false);
                                 }
                              }
                              if(UDIT_p)
                              {
                                 if(UserDefinedInterfaceType != "handshake" &&
                                       UserDefinedInterfaceType != "fifo" &&
                                       UserDefinedInterfaceType.find("array")==std::string::npos &&
                                       UserDefinedInterfaceType != "bus")
                                 {
                                    DiagnosticsEngine &D = CI.getDiagnostics();
                                    D.Report(D.getCustomDiagID(DiagnosticsEngine::Error,
                                                            "#pragma HLS_interface non-consistent with parameter of constant array type, where user defined interface is: %0")).AddString(UserDefinedInterfaceType);
                                 }
                                 else
                                    interfaceType=UserDefinedInterfaceType;
                              }
                           }
                           else if(argType->isPointerType() || argType->isReferenceType())
                           {
                              auto PT = cast<PointerType>(argType);
                              if(PT->getPointeeType()->isBuiltinType())
                                 interfaceType="ptrdefault";
                              if(UDIT_p)
                              {
                                 if(UserDefinedInterfaceType != "none" &&
                                       UserDefinedInterfaceType != "handshake" &&
                                       UserDefinedInterfaceType != "valid" &&
                                       UserDefinedInterfaceType != "acknowledge" &&
                                       UserDefinedInterfaceType != "fifo" &&
                                       UserDefinedInterfaceType != "bus")
                                 {
                                    DiagnosticsEngine &D = CI.getDiagnostics();
                                    D.Report(D.getCustomDiagID(DiagnosticsEngine::Error,
                                                            "#pragma HLS_interface non-consistent with parameter of pointer type, where user defined interface is: %0")).AddString(UserDefinedInterfaceType);
                                 }
                                 else
                                    interfaceType=UserDefinedInterfaceType;
                              }
                           }
                           else
                           {
                              if(UDIT_p)
                              {
                                 if(UserDefinedInterfaceType != "none" &&
                                       UserDefinedInterfaceType != "handshake" &&
                                       UserDefinedInterfaceType != "valid" &&
                                       UserDefinedInterfaceType != "acknowledge")
                                 {
                                    DiagnosticsEngine &D = CI.getDiagnostics();
                                    D.Report(D.getCustomDiagID(DiagnosticsEngine::Error,
                                                            "#pragma HLS_interface non-consistent with parameter of builtin type, where user defined interface is: %0")).AddString(UserDefinedInterfaceType);
                                 }
                                 else
                                    interfaceType=UserDefinedInterfaceType;
                              }
                           }

                           HLS_interfaceMap[funName].push_back(interfaceType);
                           Fun2Params[funName].push_back(parName);
                        }
                     }
                  }
                  if(!HLS_interface_PragmaMap.empty())
                  {
                     HLS_interface_PragmaMap.clear();
                  }
               }
            }
            return true;
         }

   };

   class HLS_interface_PragmaHandler : public PragmaHandler
   {
      public:
         HLS_interface_PragmaHandler() : PragmaHandler("HLS_interface") { }

         void HandlePragma(Preprocessor &PP, PragmaIntroducerKind Introducer,
                           Token &PragmaTok) override
         {

            Token Tok;
            unsigned int index=0;
            std::string par;
            std::string interface;
            while(Tok.isNot(tok::eod))
            {
               PP.Lex(Tok);
               if(Tok.isNot(tok::eod))
               {
                  if(index==0)
                     par=PP.getSpelling(Tok);
                  else if(index>=1)
                  {
                     auto tokString = PP.getSpelling(Tok);
                     if(index==1)
                     {
                        if(tokString!="none" &&
                              tokString!="array" &&
                              tokString!="bus" &&
                              tokString!="fifo" &&
                              tokString!="handshake" &&
                              tokString!="valid")
                        {
                           DiagnosticsEngine &D = PP.getDiagnostics();
                           unsigned ID = D.getCustomDiagID(
                                            DiagnosticsEngine::Error,
                                            "#pragma HLS_interface unexpected interface type. Currently accepted keywords are: none,array,bus,fifo,handshake,valid,acknowledge");
                           D.Report(PragmaTok.getLocation(), ID);
                        }
                     }
                     else if(index==2)
                     {
                        if(tokString != "-" && interface != "array")
                        {
                           DiagnosticsEngine &D = PP.getDiagnostics();
                           unsigned ID = D.getCustomDiagID(
                                            DiagnosticsEngine::Error,
                                            "#pragma HLS_interface malformed");
                           D.Report(PragmaTok.getLocation(), ID);
                        }
                     }
                     else if(index==3)
                     {
                        if(Tok.isNot(tok::numeric_constant) || interface != "array-")
                        {
                           DiagnosticsEngine &D = PP.getDiagnostics();
                           unsigned ID = D.getCustomDiagID(
                                            DiagnosticsEngine::Error,
                                            "#pragma HLS_interface malformed");
                           D.Report(PragmaTok.getLocation(), ID);
                        }
                     }
                     interface+=tokString;
                  }
                  ++index;
               }
            }
            if(index>=2)
            {
               HLS_interface_PragmaMap[par]=interface;
            }
            else
            {
               DiagnosticsEngine &D = PP.getDiagnostics();
               unsigned ID = D.getCustomDiagID(
                                DiagnosticsEngine::Error,
                       "#pragma HLS_interface malformed");
               D.Report(PragmaTok.getLocation(), ID);
            }
         }
   };


   class CLANG_VERSION_SYMBOL(_plugin_dumpGimpleSSA) : public PluginASTAction
   {
         std::string topfname;
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
            clang::Preprocessor &PP = CI.getPreprocessor();
            PP.AddPragmaHandler(new HLS_interface_PragmaHandler());
            gimpleRawWriter = new DumpGimpleRaw(CI, outdir_name, InFile, false, &Fun2Params);
            TopFunctionName = topfname;
            interface_XML_filename=create_file_name_string(outdir_name,InFile,".interface.xml");
            return llvm::make_unique<FunctionArgConsumer>(CI);
         }

         bool ParseArgs(const CompilerInstance &CI,
                        const std::vector<std::string> &args) override
         {
            DiagnosticsEngine &D = CI.getDiagnostics();
            for (size_t i = 0, e = args.size(); i != e; ++i)
            {
               if (args.at(i) == "-topfname")
               {
                  if (i + 1 >= e) {
                     D.Report(D.getCustomDiagID(DiagnosticsEngine::Error,
                                                "missing topfname argument"));
                     return false;
                  }
                  ++i;
                  topfname = args.at(i);
               }
               else if (args.at(i) == "-outputdir")
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
            ros << "-topfname <function name>\n";
            ros << "  Function from which the Point-To analysis has to start\n";
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
#include "llvm/IR/Dominators.h"
#include "llvm/Analysis/DominanceFrontier.h"
#include "llvm/IR/PassManager.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/Transforms/IPO/PassManagerBuilder.h"
#include "llvm/Transforms/Utils/LoopUtils.h"
#include "llvm/Analysis/LazyValueInfo.h"
#include "llvm/Analysis/AliasAnalysis.h"
#include "llvm/Analysis/AssumptionCache.h"
//#include "llvm/Analysis/TypeBasedAliasAnalysis.h"
//#include "llvm/Analysis/CFLSteensAliasAnalysis.h"
#include "llvm/Analysis/MemoryDependenceAnalysis.h"
#include "llvm/Analysis/TargetTransformInfo.h"
#if __clang_major__ != 4
#include "llvm/Analysis/MemorySSA.h"
#else
#include "llvm/Transforms/Utils/MemorySSA.h"
#endif
#include "llvm/Transforms/IPO.h"
#include "llvm/Transforms/Scalar.h"
#ifdef UNIFYFUNCTIONEXITNODES
#include "llvm/Transforms/Utils/UnifyFunctionExitNodes.h"
#endif
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
            initializeTargetLibraryInfoWrapperPassPass(*PassRegistry::getPassRegistry());
            initializeAssumptionCacheTrackerPass(*PassRegistry::getPassRegistry());
            initializeDominatorTreeWrapperPassPass(*PassRegistry::getPassRegistry());
            initializeDominanceFrontierWrapperPassPass(*PassRegistry::getPassRegistry());
         }
         bool runOnModule(Module &M)
         {
#if PRINT_DBG_MSG
            llvm::errs() << "Top function name: " << TopFunctionName << "\n";
#endif
            assert(gimpleRawWriter);
            assert(interface_XML_filename!="");
            clang::writeXML_interfaceFile(interface_XML_filename);
            auto res = gimpleRawWriter->runOnModule(M, this, TopFunctionName);
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
           AU.addRequired<TargetLibraryInfoWrapperPass>();
           AU.addRequired<AssumptionCacheTracker>();
           AU.addRequired<DominatorTreeWrapperPass>();
           AU.addRequired<DominanceFrontierWrapperPass>();
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
   PM.add(llvm::createPromoteMemoryToRegisterPass());
   PM.add(llvm::createGlobalOptimizerPass());
   PM.add(llvm::createBreakCriticalEdgesPass());
#ifdef UNIFYFUNCTIONEXITNODES
   PM.add(llvm::createUnifyFunctionExitNodesPass());
#endif
   if(PMB.OptLevel >= 2)
   {
      PM.add(llvm::createDeadStoreEliminationPass());
      PM.add(llvm::createAggressiveDCEPass());
      PM.add(llvm::createLoopLoadEliminationPass());
   }
   PM.add(new llvm::CLANG_VERSION_SYMBOL(_plugin_dumpGimpleSSAPass)());
}
// These constructors add our pass to a list of global extensions.
static llvm::RegisterStandardPasses clangtoolLoader_Ox(llvm::PassManagerBuilder::EP_OptimizerLast, loadPass);
