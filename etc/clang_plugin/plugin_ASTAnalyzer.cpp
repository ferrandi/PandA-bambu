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
 * @file plugin_ASTAnalyzer.cpp
 * @brief Plugin analyzing the Clang AST retrieving useful information for PandA
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 *
 */

#include "plugin_includes.hpp"

#include "clang/AST/AST.h"
#include "clang/AST/ASTConsumer.h"
#include "clang/AST/DeclCXX.h"
#include "clang/AST/Mangle.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/AST/Type.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Frontend/FrontendPluginRegistry.h"
#include "clang/Lex/LexDiagnostic.h"
#include "clang/Lex/Preprocessor.h"
#include "clang/Sema/Sema.h"
#include "llvm/Support/raw_ostream.h"

static std::map<std::string, std::map<clang::SourceLocation, std::pair<std::string, std::string>>> HLS_interface_PragmaMap;
static std::map<std::string, std::map<clang::SourceLocation, std::pair<std::string, std::string>>> HLS_interface_PragmaMapArraySize;

namespace clang
{
   class FunctionArgConsumer : public clang::ASTConsumer
   {
      CompilerInstance& CI;
      std::string topfname;
      std::string outdir_name;
      std::string InFile;

      std::map<std::string, std::vector<std::string>> Fun2Params;
      std::map<std::string, std::vector<std::string>> Fun2ParamType;
      std::map<std::string, std::map<std::string, std::string>> Fun2ParamSize;
      std::map<std::string, std::vector<std::string>> Fun2ParamInclude;
      std::map<std::string, std::string> Fun2Demangled;
      std::map<std::string, clang::SourceLocation> prevLoc;

      std::map<std::string, std::vector<std::string>> HLS_interfaceMap;
      std::map<std::string, std::map<std::string, std::string>> HLS_interfaceArraySizeMap;

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

      void convert_unescaped(std::string& ioString) const
      {
         std::string::size_type lPos = 0;
         while((lPos = ioString.find_first_of("&<>'\"", lPos)) != std::string::npos)
         {
            switch(ioString[lPos])
            {
               case '&':
                  ioString.replace(lPos++, 1, "&amp;");
                  break;
               case '<':
                  ioString.replace(lPos++, 1, "&lt;");
                  break;
               case '>':
                  ioString.replace(lPos++, 1, "&gt;");
                  break;
               case '\'':
                  ioString.replace(lPos++, 1, "&apos;");
                  break;
               case '"':
                  ioString.replace(lPos++, 1, "&quot;");
                  break;
               default:
               {
                  // Do nothing
               }
            }
         }
      }

      void writeXML_interfaceFile(const std::string& filename, const std::string& TopFunctionName) const
      {
         std::error_code EC;
#if __clang_major__ >= 7
         llvm::raw_fd_ostream stream(filename, EC, llvm::sys::fs::FA_Read | llvm::sys::fs::FA_Write);
#else
         llvm::raw_fd_ostream stream(filename, EC, llvm::sys::fs::F_RW);
#endif
         stream << "<?xml version=\"1.0\"?>\n";
         stream << "<module>\n";
         for(auto funArgPair : Fun2Params)
         {
            if(!TopFunctionName.empty() && Fun2Demangled.find(funArgPair.first)->second != TopFunctionName && funArgPair.first != TopFunctionName)
            {
               continue;
            }
            bool hasInterfaceType = HLS_interfaceMap.find(funArgPair.first) != HLS_interfaceMap.end();
            if(hasInterfaceType)
            {
               stream << "  <function id=\"" << funArgPair.first << "\">\n";
               const auto& interfaceTypeVec = HLS_interfaceMap.find(funArgPair.first)->second;
               const auto& interfaceTypenameVec = Fun2ParamType.find(funArgPair.first)->second;
               const auto& interfaceTypenameIncludeVec = Fun2ParamInclude.find(funArgPair.first)->second;
               unsigned int ArgIndex = 0;
               for(const auto& par : funArgPair.second)
               {
                  std::string typenameArg = interfaceTypenameVec.at(ArgIndex);
                  convert_unescaped(typenameArg);
                  stream << "    <arg id=\"" << par << "\" interface_type=\"" << interfaceTypeVec.at(ArgIndex) << "\" interface_typename=\"" << typenameArg << "\"";
                  if(Fun2ParamSize.find(funArgPair.first) != Fun2ParamSize.end() && Fun2ParamSize.find(funArgPair.first)->second.find(par) != Fun2ParamSize.find(funArgPair.first)->second.end())
                     stream << " size=\"" << Fun2ParamSize.find(funArgPair.first)->second.find(par)->second << "\"";
                  stream << " interface_typename_include=\"" << interfaceTypenameIncludeVec.at(ArgIndex) << "\"/>\n";
                  ++ArgIndex;
               }
               stream << "  </function>\n";
            }
         }
         stream << "</module>\n";
      }

      void writeFun2Params(const std::string& filename) const
      {
         std::error_code EC;
#if __clang_major__ >= 7
         llvm::raw_fd_ostream stream(filename, EC, llvm::sys::fs::FA_Read | llvm::sys::fs::FA_Write);
#else
         llvm::raw_fd_ostream stream(filename, EC, llvm::sys::fs::F_RW);
#endif
         for(auto fun2parms_el : Fun2Params)
         {
            stream << fun2parms_el.first;
            for(auto par : fun2parms_el.second)
               stream << " " << par;
            stream << "\n";
         }
      }
      const NamedDecl* getBaseTypeDecl(const QualType& qt) const
      {
         const Type* ty = qt.getTypePtr();
         NamedDecl* ND = nullptr;

         if(ty->isPointerType() || ty->isReferenceType())
         {
            return getBaseTypeDecl(ty->getPointeeType());
         }
         if(ty->isRecordType())
         {
            ND = ty->getAs<RecordType>()->getDecl();
         }
         else if(ty->isEnumeralType())
         {
            ND = ty->getAs<EnumType>()->getDecl();
         }
         else if(ty->getTypeClass() == Type::Typedef)
         {
            ND = ty->getAs<TypedefType>()->getDecl();
         }
         else if(ty->isArrayType())
         {
            return getBaseTypeDecl(ty->castAsArrayTypeUnsafe()->getElementType());
         }
         return ND;
      }

      QualType RemoveTypedef(QualType t) const
      {
         if(t->getTypeClass() == Type::Typedef)
            return RemoveTypedef(t->getAs<TypedefType>()->getDecl()->getUnderlyingType());
         else if(t->getTypeClass() == Type::TemplateSpecialization)
         {
            return t;
         }
         else
            return t;
      }

      std::string GetTypeNameCanonical(QualType t) const
      {
         auto typeName = t->getCanonicalTypeInternal().getAsString();
         auto key = std::string("class ");
         if(typeName.find(key) == 0)
            typeName = typeName.substr(key.size());
         return typeName;
      }

      void AnalyzeFunctionDecl(const FunctionDecl* FD)
      {
         auto& SM = FD->getASTContext().getSourceManager();
         std::map<std::string, std::string> interface_PragmaMap;
         std::map<std::string, std::string> interface_PragmaMapArraySize;
         auto locEnd = FD->getSourceRange().getEnd();
         auto filename = SM.getPresumedLoc(locEnd, false).getFilename();
         if(HLS_interface_PragmaMap.find(filename) != HLS_interface_PragmaMap.end())
         {
            SourceLocation prev;
            if(prevLoc.find(filename) != prevLoc.end())
            {
               prev = prevLoc.find(filename)->second;
            }
            for(auto& loc2pair : HLS_interface_PragmaMap.find(filename)->second)
            {
               if((prev.isInvalid() || prev < loc2pair.first) && (loc2pair.first < locEnd))
               {
                  interface_PragmaMap[loc2pair.second.first] = loc2pair.second.second;
               }
            }
            if(HLS_interface_PragmaMapArraySize.find(filename) != HLS_interface_PragmaMapArraySize.end())
            {
               for(auto& loc2pair : HLS_interface_PragmaMapArraySize.find(filename)->second)
               {
                  if((prev.isInvalid() || prev < loc2pair.first) && (loc2pair.first < locEnd))
                  {
                     interface_PragmaMapArraySize[loc2pair.second.first] = loc2pair.second.second;
                  }
               }
            }
         }

         if(!FD->isVariadic() && FD->hasBody())
         {
            const auto getMangledName = [&](const FunctionDecl* decl) {
               auto mangleContext = decl->getASTContext().createMangleContext();

               if(!mangleContext->shouldMangleDeclName(decl))
               {
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
            Fun2Demangled[funName] = FD->getNameInfo().getName().getAsString();
            // llvm::errs()<<"funName:"<<funName<<"\n";
            for(const auto par : FD->parameters())
            {
               if(const ParmVarDecl* ND = dyn_cast<ParmVarDecl>(par))
               {
                  std::string interfaceType = "default";
                  std::string arraySize;
                  std::string UserDefinedInterfaceType;
                  std::string ParamTypeName;
                  auto parName = ND->getNameAsString();
                  bool UDIT_p = false;
                  if(interface_PragmaMap.find(parName) != interface_PragmaMap.end())
                  {
                     UserDefinedInterfaceType = interface_PragmaMap.find(parName)->second;
                     UDIT_p = true;
                  }
                  auto argType = ND->getType();
                  // argType->dump (llvm::errs() );
                  if(isa<DecayedType>(argType))
                  {
                     auto DT = cast<DecayedType>(argType);
                     if(DT->getOriginalType()->isConstantArrayType())
                     {
                        auto CA = cast<ConstantArrayType>(DT->getOriginalType());
                        auto OrigTotArraySize = CA->getSize();
                        while(CA->getElementType()->isConstantArrayType())
                        {
                           CA = cast<ConstantArrayType>(CA->getElementType());
                           OrigTotArraySize *= CA->getSize();
                        }
                        if(CA->getElementType()->getTypeClass() == Type::Typedef)
                           ParamTypeName = GetTypeNameCanonical(RemoveTypedef(CA->getElementType())) + " *";
                        else
                           ParamTypeName = GetTypeNameCanonical(CA->getElementType()) + " *";
                        interfaceType = "array";
                        arraySize = OrigTotArraySize.toString(10, false);
                        assert(arraySize != "0");
                     }
                     if(UDIT_p)
                     {
                        if(UserDefinedInterfaceType != "handshake" && UserDefinedInterfaceType != "fifo" && UserDefinedInterfaceType.find("array") == std::string::npos && UserDefinedInterfaceType != "bus")
                        {
                           DiagnosticsEngine& D = CI.getDiagnostics();
                           D.Report(D.getCustomDiagID(DiagnosticsEngine::Error, "#pragma HLS_interface non-consistent with parameter of constant array type, where user defined interface is: %0")).AddString(UserDefinedInterfaceType);
                        }
                        else
                        {
                           interfaceType = UserDefinedInterfaceType;
                           if(interfaceType == "array")
                           {
                              if(interface_PragmaMapArraySize.find(parName) == interface_PragmaMapArraySize.end())
                              {
                                 DiagnosticsEngine& D = CI.getDiagnostics();
                                 D.Report(D.getCustomDiagID(DiagnosticsEngine::Error, "#pragma HLS_interface inconsistent internal data structure: %0")).AddString(UserDefinedInterfaceType);
                              }
                              else
                                 arraySize = interface_PragmaMapArraySize.find(parName)->second;
                           }
                        }
                     }
                  }
                  else if(argType->isPointerType() || argType->isReferenceType())
                  {
                     auto PT = dyn_cast<PointerType>(argType);
                     if(PT && PT->getPointeeType()->getTypeClass() == Type::Typedef)
                        ParamTypeName = GetTypeNameCanonical(RemoveTypedef(PT->getPointeeType())) + " *";
                     auto RT = dyn_cast<ReferenceType>(argType);
                     if(RT && RT->getPointeeType()->getTypeClass() == Type::Typedef)
                        ParamTypeName = GetTypeNameCanonical(RemoveTypedef(RT->getPointeeType())) + " &";
                     interfaceType = "ptrdefault";
                     if(UDIT_p)
                     {
                        if(UserDefinedInterfaceType != "none" && UserDefinedInterfaceType != "none_registered" && UserDefinedInterfaceType != "handshake" && UserDefinedInterfaceType != "valid" && UserDefinedInterfaceType != "ovalid" &&
                           UserDefinedInterfaceType != "acknowledge" && UserDefinedInterfaceType != "fifo" && UserDefinedInterfaceType != "bus")
                        {
                           DiagnosticsEngine& D = CI.getDiagnostics();
                           D.Report(D.getCustomDiagID(DiagnosticsEngine::Error, "#pragma HLS_interface non-consistent with parameter of pointer type, where user defined interface is: %0")).AddString(UserDefinedInterfaceType);
                        }
                        else
                        {
                           interfaceType = UserDefinedInterfaceType;
                        }
                     }
                  }
                  else
                  {
                     if(argType->getTypeClass() == Type::Typedef)
                        ParamTypeName = GetTypeNameCanonical(RemoveTypedef(argType));
                     if(!argType->isBuiltinType() && !argType->isEnumeralType())
                     {
                        interfaceType = "none";
                     }
                     if(UDIT_p)
                     {
                        if(UserDefinedInterfaceType != "none" && UserDefinedInterfaceType != "none_registered" && UserDefinedInterfaceType != "handshake" && UserDefinedInterfaceType != "valid" && UserDefinedInterfaceType != "ovalid" &&
                           UserDefinedInterfaceType != "acknowledge")
                        {
                           DiagnosticsEngine& D = CI.getDiagnostics();
                           D.Report(D.getCustomDiagID(DiagnosticsEngine::Error, "#pragma HLS_interface non-consistent with parameter of builtin type, where user defined interface is: %0")).AddString(UserDefinedInterfaceType);
                        }
                        else
                        {
                           interfaceType = UserDefinedInterfaceType;
                        }
                        if((argType->isBuiltinType() || argType->isEnumeralType()) && interfaceType == "none")
                        {
                           interfaceType = "default";
                        }
                     }
                  }

                  HLS_interfaceMap[funName].push_back(interfaceType);
                  Fun2Params[funName].push_back(parName);
                  if(ParamTypeName.empty())
                     ParamTypeName = GetTypeNameCanonical(ND->getType());
                  Fun2ParamType[funName].push_back(ParamTypeName);
                  if(interfaceType == "array")
                     Fun2ParamSize[funName][parName] = arraySize;
                  if(auto BTD = getBaseTypeDecl(ND->getType()))
                  {
                     Fun2ParamInclude[funName].push_back(SM.getPresumedLoc(BTD->getSourceRange().getBegin(), false).getFilename());
                  }
                  else
                  {
                     Fun2ParamInclude[funName].push_back("");
                  }
               }
            }
         }
      }

    public:
      FunctionArgConsumer(CompilerInstance& Instance, const std::string& _topfname, const std::string& _outdir_name, std::string _InFile) : CI(Instance), topfname(_topfname), outdir_name(_outdir_name), InFile(_InFile)
      {
      }

      bool HandleTopLevelDecl(DeclGroupRef DG) override
      {
         for(auto D : DG)
         {
            if(const auto* FD = dyn_cast<FunctionDecl>(D))
            {
               AnalyzeFunctionDecl(FD);
               auto endLoc = FD->getSourceRange().getEnd();
               auto& SM = FD->getASTContext().getSourceManager();
               auto filename = SM.getPresumedLoc(endLoc, false).getFilename();
               prevLoc[filename] = endLoc;
            }
            else if(const auto* LSD = dyn_cast<LinkageSpecDecl>(D))
            {
               for(auto d : LSD->decls())
               {
                  if(const FunctionDecl* fd = dyn_cast<FunctionDecl>(d))
                  {
                     AnalyzeFunctionDecl(fd);
                     auto endLoc = fd->getSourceRange().getEnd();
                     auto& SM = fd->getASTContext().getSourceManager();
                     auto filename = SM.getPresumedLoc(endLoc, false).getFilename();
                     prevLoc[filename] = endLoc;
                  }
               }
            }
         }
         auto baseFilename = create_file_basename_string(outdir_name, InFile);
         std::string interface_fun2parms_filename = baseFilename + ".params.txt";
         writeFun2Params(interface_fun2parms_filename);
         return true;
      }

      void HandleTranslationUnit(ASTContext&) override
      {
         auto baseFilename = create_file_basename_string(outdir_name, InFile);
         std::string interface_XML_filename = baseFilename + ".interface.xml";
         writeXML_interfaceFile(interface_XML_filename, topfname);
      }
   };

   class HLS_interface_PragmaHandler : public PragmaHandler
   {
    public:
      HLS_interface_PragmaHandler() : PragmaHandler("HLS_interface")
      {
      }

      void HandlePragma(Preprocessor& PP, PragmaIntroducerKind /*Introducer*/, Token& PragmaTok) override
      {
         Token Tok{};
         unsigned int index = 0;
         std::string par;
         std::string interface;
         std::string ArraySize;
         auto loc = PragmaTok.getLocation();
         while(Tok.isNot(tok::eod))
         {
            PP.Lex(Tok);
            if(Tok.isNot(tok::eod))
            {
               if(index == 0)
               {
                  par = PP.getSpelling(Tok);
               }
               else if(index >= 1)
               {
                  auto tokString = PP.getSpelling(Tok);
                  if(index == 1)
                  {
                     if(tokString != "none" && tokString != "none_registered" && tokString != "array" && tokString != "bus" && tokString != "fifo" && tokString != "handshake" && tokString != "valid" && tokString != "ovalid" && tokString != "acknowledge")
                     {
                        DiagnosticsEngine& D = PP.getDiagnostics();
                        unsigned ID = D.getCustomDiagID(DiagnosticsEngine::Error, "#pragma HLS_interface unexpected interface type. Currently accepted keywords are: none,none_registered,array,bus,fifo,handshake,valid,ovalid,acknowledge");
                        D.Report(PragmaTok.getLocation(), ID);
                     }
                     interface += tokString;
                  }
                  else if(index == 2)
                  {
                     if(Tok.isNot(tok::numeric_constant) || interface != "array")
                     {
                        DiagnosticsEngine& D = PP.getDiagnostics();
                        unsigned ID = D.getCustomDiagID(DiagnosticsEngine::Error, "#pragma HLS_interface malformed");
                        D.Report(PragmaTok.getLocation(), ID);
                     }
                     ArraySize = tokString;
                  }
               }
               ++index;
            }
         }
         if(index >= 2)
         {
            auto& SM = PP.getSourceManager();
            std::map<std::string, std::string> interface_PragmaMap;
            auto filename = SM.getPresumedLoc(loc, false).getFilename();
            if(ArraySize != "" && ArraySize != "0" && interface != "array")
            {
               DiagnosticsEngine& D = PP.getDiagnostics();
               unsigned ID = D.getCustomDiagID(DiagnosticsEngine::Error, "#pragma HLS_interface malformed");
               D.Report(PragmaTok.getLocation(), ID);
            }
            HLS_interface_PragmaMap[filename][loc] = std::make_pair(par, interface);
            if(interface == "array")
            {
               HLS_interface_PragmaMapArraySize[filename][loc] = std::make_pair(par, ArraySize);
            }
         }
         else
         {
            DiagnosticsEngine& D = PP.getDiagnostics();
            unsigned ID = D.getCustomDiagID(DiagnosticsEngine::Error, "#pragma HLS_interface malformed");
            D.Report(PragmaTok.getLocation(), ID);
         }
      }
   };

   class CLANG_VERSION_SYMBOL(_plugin_ASTAnalyzer) : public PluginASTAction
   {
      std::string topfname;
      std::string outdir_name;

    protected:
      std::unique_ptr<ASTConsumer> CreateASTConsumer(CompilerInstance& CI, llvm::StringRef InFile) override
      {
         DiagnosticsEngine& D = CI.getDiagnostics();
         if(outdir_name.empty())
         {
            D.Report(D.getCustomDiagID(DiagnosticsEngine::Error, "outputdir argument not specified"));
         }
         clang::Preprocessor& PP = CI.getPreprocessor();
         PP.AddPragmaHandler(new HLS_interface_PragmaHandler());
         return llvm::make_unique<FunctionArgConsumer>(CI, topfname, outdir_name, InFile);
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
            else if(args.at(i) == "-outputdir")
            {
               if(i + 1 >= e)
               {
                  D.Report(D.getCustomDiagID(DiagnosticsEngine::Error, "missing outputdir argument"));
                  return false;
               }
               ++i;
               outdir_name = args.at(i);
            }
         }
         if(!args.empty() && args.at(0) == "-help")
         {
            PrintHelp(llvm::errs());
         }

         if(outdir_name.empty())
         {
            D.Report(D.getCustomDiagID(DiagnosticsEngine::Error, "outputdir not specified"));
         }
         return true;
      }
      void PrintHelp(llvm::raw_ostream& ros)
      {
         ros << "Help for " CLANG_VERSION_STRING(_plugin_ASTAnalyzer) " plugin\n";
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
      CLANG_VERSION_SYMBOL(_plugin_ASTAnalyzer)()
      {
      }
      CLANG_VERSION_SYMBOL(_plugin_ASTAnalyzer)(const CLANG_VERSION_SYMBOL(_plugin_ASTAnalyzer) & step) = delete;
      CLANG_VERSION_SYMBOL(_plugin_ASTAnalyzer) & operator=(const CLANG_VERSION_SYMBOL(_plugin_ASTAnalyzer) &) = delete;
   };

#ifdef _WIN32

   void initializeplugin_ASTAnalyzer()
   {
      static clang::FrontendPluginRegistry::Add<clang::CLANG_VERSION_SYMBOL(_plugin_ASTAnalyzer)> X(CLANG_VERSION_STRING(_plugin_ASTAnalyzer), "Analyze Clang AST to retrieve information useful for PandA");
   }
#endif
} // namespace clang

#ifndef _WIN32
static clang::FrontendPluginRegistry::Add<clang::CLANG_VERSION_SYMBOL(_plugin_ASTAnalyzer)> X(CLANG_VERSION_STRING(_plugin_ASTAnalyzer), "Analyze Clang AST to retrieve information useful for PandA");
#endif
