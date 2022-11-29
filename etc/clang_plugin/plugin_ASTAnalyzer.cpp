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
 *              Copyright (C) 2018-2022 Politecnico di Milano
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
 * @author Michele Fiorito <michele.fiorito@polimi.it>
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

#define PRINT_DBG_MSG 0

static std::map<std::string, std::map<clang::SourceLocation, std::map<std::string, std::string>>> file_loc_attr;
static std::map<std::string, std::map<clang::SourceLocation, std::map<std::string, std::map<std::string, std::string>>>>
    file_loc_arg_attr;

namespace clang
{
   class FunctionArgConsumer : public clang::ASTConsumer
   {
      CompilerInstance& CI;
      std::string topfname;
      std::string outdir_name;
      std::string InFile;
      clang::PrintingPolicy pp;

      std::map<std::string, std::map<std::string, std::string>> fun_attr;
      std::map<std::string, std::map<std::string, std::map<std::string, std::string>>> fun_arg_attr;

      std::map<std::string, std::vector<std::string>> Fun2Params;
      std::map<std::string, std::string> Fun2Demangled;
      std::map<std::string, clang::SourceLocation> prevLoc;

      std::string create_file_basename_string(const std::string& on, const std::string& original_filename)
      {
         const auto found = original_filename.find_last_of("/\\");
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

      std::string convert_unescaped(std::string ioString) const
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
         return ioString;
      }

      void writeXML_interfaceFile(const std::string& filename, const std::string& TopFunctionName) const
      {
         std::error_code EC;
#if __clang_major__ >= 7 && !defined(VVD)
         llvm::raw_fd_ostream stream(filename, EC, llvm::sys::fs::FA_Read | llvm::sys::fs::FA_Write);
#else
         llvm::raw_fd_ostream stream(filename, EC, llvm::sys::fs::F_RW);
#endif
         stream << "<?xml version=\"1.0\"?>\n";
         stream << "<module>\n";
         for(const auto& function_attribute : fun_arg_attr)
         {
            const auto& fname = function_attribute.first;
            const auto& arg_attr = function_attribute.second;
            if(!TopFunctionName.empty() && Fun2Demangled.at(fname) != TopFunctionName && fname != TopFunctionName)
            {
               continue;
            }
            stream << "  <function id=\"" << fname << "\">\n";
            assert(Fun2Params.count(fname));
            for(const auto& aname : Fun2Params.at(fname))
            {
               stream << "    <arg id=\"" << aname << "\"";
               assert(Fun2Params.count(aname));
               for(const auto& attr_val : arg_attr.at(aname))
               {
                  stream << " " << attr_val.first << "=\"" << convert_unescaped(attr_val.second) << "\"";
               }
               stream << "/>\n";
            }
            stream << "  </function>\n";
         }
         stream << "</module>\n";
      }

      void writeXML_pipelineFile(const std::string& filename, const std::string& TopFunctionName) const
      {
         std::error_code EC;
#if __clang_major__ >= 7 && !defined(VVD)
         llvm::raw_fd_ostream stream(filename, EC, llvm::sys::fs::FA_Read | llvm::sys::fs::FA_Write);
#else
         llvm::raw_fd_ostream stream(filename, EC, llvm::sys::fs::F_RW);
#endif
         stream << "<?xml version=\"1.0\"?>\n";
         stream << "<module>\n";
         for(const auto& function_attributes : fun_attr)
         {
            const auto& fname = function_attributes.first;
            const auto& fattr = function_attributes.second;
            stream << "  <function id=\"" << fname << "\"";
            for(const auto& attr_val : fattr)
            {
               stream << " " << attr_val.first << "=\"" << attr_val.second << "\"";
            }
            stream << "/>\n";
         }
         stream << "</module>\n";
      }

      void writeFun2Params(const std::string& filename) const
      {
         std::error_code EC;
#if __clang_major__ >= 7 && !defined(VVD)
         llvm::raw_fd_ostream stream(filename, EC, llvm::sys::fs::FA_Read | llvm::sys::fs::FA_Write);
#else
         llvm::raw_fd_ostream stream(filename, EC, llvm::sys::fs::F_RW);
#endif
         for(const auto& fun2parms_el : Fun2Params)
         {
            stream << fun2parms_el.first;
            for(const auto& par : fun2parms_el.second)
            {
               stream << " " << par;
            }
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

      std::string GetTypeNameCanonical(const QualType& t, const PrintingPolicy& pp) const
      {
         auto typeName = t->getCanonicalTypeInternal().getAsString(pp);
         const auto key = std::string("class ");
         const auto constkey = std::string("const class ");
         if(typeName.find(key) == 0)
         {
            typeName = typeName.substr(key.size());
         }
         else if(typeName.find(constkey) == 0)
         {
            typeName = typeName.substr(constkey.size());
         }
         return typeName;
      }

      std::string getMangledName(const FunctionDecl* decl)
      {
         const auto mangleContext = decl->getASTContext().createMangleContext();

         if(!mangleContext->shouldMangleDeclName(decl))
         {
            delete mangleContext;
            return decl->getNameInfo().getName().getAsString();
         }
         std::string mangledName;
         if(llvm::isa<CXXConstructorDecl>(decl) || llvm::isa<CXXDestructorDecl>(decl))
         {
            delete mangleContext;
            return decl->getNameInfo().getName().getAsString();
            ;
         }
         llvm::raw_string_ostream ostream(mangledName);
         if(mangleContext->shouldMangleCXXName(decl))
         {
            mangleContext->mangleCXXName(decl, ostream);
            ostream.flush();
            delete mangleContext;
            return mangledName;
         }
         mangleContext->mangleName(decl, ostream);
         ostream.flush();
         delete mangleContext;
         return mangledName;
      }

      void AnalyzeFunctionDecl(const FunctionDecl* FD)
      {
         const auto print_error = [&](const std::string& msg)
         {
            auto& D = CI.getDiagnostics();
            D.Report(D.getCustomDiagID(DiagnosticsEngine::Error, "%0")).AddString(msg);
         };

         auto& SM = FD->getASTContext().getSourceManager();
         std::map<std::string, std::string> interface_PragmaMap;
         std::map<std::string, std::string> interface_PragmaMapArraySize;
         std::map<std::string, std::string> interface_PragmaMapAttribute2;
         std::map<std::string, std::string> interface_PragmaMapAttribute3;
         const auto locEnd = FD->getSourceRange().getEnd();
         const auto filename = SM.getPresumedLoc(locEnd, false).getFilename();
         const auto fname = getMangledName(FD);
         const auto loc_arg = file_loc_arg_attr.find(filename);
         if(loc_arg != file_loc_arg_attr.end())
         {
            const auto prev = [&]() -> SourceLocation
            {
               const auto prev_it = prevLoc.find(filename);
               if(prev_it != prevLoc.end())
               {
                  return prev_it->second;
               }
               return SourceLocation();
            }();

            for(const auto& location_argument : loc_arg->second)
            {
               const auto& loc = location_argument.first;
               const auto& aattr = location_argument.second;
               if((prev.isInvalid() || prev < loc) && (loc < locEnd))
               {
                  fun_arg_attr[fname].insert(aattr.begin(), aattr.end());
               }
            }
         }
         const auto loc_attr = file_loc_attr.find(filename);
         if(loc_attr != file_loc_attr.end())
         {
            const auto prev = [&]() -> SourceLocation
            {
               const auto prev_it = prevLoc.find(filename);
               if(prev_it != prevLoc.end())
               {
                  return prev_it->second;
               }
               return SourceLocation();
            }();

            for(const auto& location_attribute : loc_attr->second)
            {
               const auto& loc = location_attribute.first;
               const auto& fattr = location_attribute.second;
               if((prev.isInvalid() || prev < loc) && (loc < locEnd))
               {
                  fun_attr[fname].insert(fattr.begin(), fattr.end());
               }
            }
         }

         if(!FD->isVariadic() && FD->hasBody())
         {
            Fun2Demangled[fname] = FD->getNameInfo().getName().getAsString();
#if PRINT_DBG_MSG
            llvm::errs() << "function: " << fname << "\n";
#endif

            auto par_index = 0u;
            for(const auto par : FD->parameters())
            {
               if(const auto PVD = dyn_cast<ParmVarDecl>(par))
               {
                  const auto pname = [&]()
                  {
                     const auto name = PVD->getNameAsString();
                     if(name.empty())
                     {
                        return "P" + std::to_string(par_index);
                     }
                     return name;
                  }();
#if PRINT_DBG_MSG
                  llvm::errs() << "  arg: " << pname << "\n";
#endif

                  auto& attr_val = fun_arg_attr[fname][pname];
                  const auto argType = PVD->getType();
                  std::string interface = "default";
                  std::string ParamTypeName;
                  std::string ParamTypeNameOrig;
                  std::string ParamTypeInclude;
                  const auto getIncludes = [&](const clang::QualType& type)
                  {
                     std::string includes;
                     if(const auto BTD = getBaseTypeDecl(type))
                     {
                        includes = SM.getPresumedLoc(BTD->getSourceRange().getBegin(), false).getFilename();
                     }
                     const auto tmpl_decl =
                         llvm::dyn_cast_or_null<ClassTemplateSpecializationDecl>(type->getAsTagDecl());
                     if(tmpl_decl)
                     {
                        const auto& args = tmpl_decl->getTemplateArgs();
                        for(auto i = 0U; i < args.size(); ++i)
                        {
                           const auto& argT = args[i];
                           if(argT.getKind() == TemplateArgument::ArgKind::Type)
                           {
                              if(const auto BTD = getBaseTypeDecl(argT.getAsType()))
                              {
                                 includes += std::string(";") +
                                             SM.getPresumedLoc(BTD->getSourceRange().getBegin(), false).getFilename();
                              }
                           }
                        }
                     }
                     return includes;
                  };
                  const auto manageArray = [&](const ConstantArrayType* CA, bool setInterfaceType)
                  {
                     auto OrigTotArraySize = CA->getSize();
                     std::string Dimensions;
                     if(!setInterfaceType)
                     {
                        Dimensions = "[" + OrigTotArraySize.toString(10, false) + "]";
                     }
                     while(CA->getElementType()->isConstantArrayType())
                     {
                        CA = cast<ConstantArrayType>(CA->getElementType());
                        const auto n_el = CA->getSize();
                        Dimensions = Dimensions + "[" + n_el.toString(10, false) + "]";
                        OrigTotArraySize *= n_el;
                     }
                     const auto paramTypeRemTD = RemoveTypedef(CA->getElementType());
                     ParamTypeName = GetTypeNameCanonical(paramTypeRemTD, pp) + " *";
                     ParamTypeNameOrig =
                         paramTypeRemTD.getAsString(pp) + (Dimensions == "" ? " *" : " (*)" + Dimensions);
                     ParamTypeInclude = getIncludes(paramTypeRemTD);
                     if(setInterfaceType)
                     {
                        interface = "array";
                        const auto array_size = OrigTotArraySize.toString(10, false);
                        assert(array_size != "0");
                        attr_val["size"] = array_size;
                     }
                  };
                  if(isa<DecayedType>(argType))
                  {
                     const auto DT = cast<DecayedType>(argType);
                     if(DT->getOriginalType().IgnoreParens()->isConstantArrayType())
                     {
                        manageArray(llvm::cast<ConstantArrayType>(DT->getOriginalType().IgnoreParens()), true);
                     }
                     else
                     {
                        const auto paramTypeRemTD = RemoveTypedef(argType);
                        ParamTypeName = GetTypeNameCanonical(paramTypeRemTD, pp);
                        ParamTypeNameOrig = paramTypeRemTD.getAsString(pp);
                        ParamTypeInclude = getIncludes(paramTypeRemTD);
                     }
                     if(attr_val.find("interface_type") != attr_val.end())
                     {
                        interface = attr_val.at("interface_type");
                        if(interface != "handshake" && interface != "fifo" &&
                           interface.find("array") == std::string::npos && interface != "bus" && interface != "m_axi" &&
                           interface != "axis")
                        {
                           print_error("#pragma HLS_interface non-consistent with parameter of constant "
                                       "array type, where user defined interface is: " +
                                       interface);
                        }
                        else if(interface == "array" && attr_val.find("size") == attr_val.end())
                        {
                           print_error("#pragma HLS_interface inconsistent internal data structure: " + interface);
                        }
                     }
                  }
                  else if(argType->isPointerType() || argType->isReferenceType())
                  {
                     if(const auto PT = llvm::dyn_cast<PointerType>(argType))
                     {
                        if(const auto CA = llvm::dyn_cast<ConstantArrayType>(PT->getPointeeType().IgnoreParens()))
                        {
                           manageArray(CA, false);
                        }
                        else
                        {
                           const auto paramTypeRemTD = RemoveTypedef(PT->getPointeeType());
                           ParamTypeName = GetTypeNameCanonical(paramTypeRemTD, pp) + " *";
                           ParamTypeNameOrig = paramTypeRemTD.getAsString(pp) + " *";
                           ParamTypeInclude = getIncludes(paramTypeRemTD);
                        }
                     }
                     else if(const auto RT = dyn_cast<ReferenceType>(argType))
                     {
                        const auto paramTypeRemTD = RemoveTypedef(RT->getPointeeType());
                        ParamTypeName = GetTypeNameCanonical(paramTypeRemTD, pp) + " &";
                        ParamTypeNameOrig = paramTypeRemTD.getAsString(pp) + " &";
                        ParamTypeInclude = getIncludes(paramTypeRemTD);
                     }
                     else
                     {
                        const auto paramTypeRemTD = RemoveTypedef(argType);
                        ParamTypeName = GetTypeNameCanonical(paramTypeRemTD, pp);
                        ParamTypeNameOrig = paramTypeRemTD.getAsString(pp);
                        ParamTypeInclude = getIncludes(paramTypeRemTD);
                     }
                     interface = "ptrdefault";
                     if(attr_val.find("interface_type") != attr_val.end())
                     {
                        interface = attr_val.at("interface_type");
                        if(interface != "ptrdefault" && interface != "none" && interface != "none_registered" &&
                           interface != "handshake" && interface != "valid" && interface != "ovalid" &&
                           interface != "acknowledge" && interface != "fifo" && interface != "bus" &&
                           interface != "m_axi" && interface != "axis")
                        {
                           print_error("#pragma HLS_interface non-consistent with parameter of pointer "
                                       "type, where user defined interface is: " +
                                       interface);
                        }
                     }
                  }
                  else
                  {
                     const auto paramTypeRemTD = RemoveTypedef(argType);
                     ParamTypeName = GetTypeNameCanonical(paramTypeRemTD, pp);
                     ParamTypeNameOrig = paramTypeRemTD.getAsString(pp);
                     ParamTypeInclude = getIncludes(paramTypeRemTD);
                     if(!argType->isBuiltinType() && !argType->isEnumeralType())
                     {
                        interface = "none";
                     }
                     if(attr_val.find("interface_type") != attr_val.end())
                     {
                        interface = attr_val.at("interface_type");
                        if(interface != "default" && interface != "none" && interface != "none_registered" &&
                           interface != "handshake" && interface != "valid" && interface != "ovalid" &&
                           interface != "acknowledge")
                        {
                           print_error("#pragma HLS_interface non-consistent with parameter of builtin "
                                       "type, where user defined interface is: " +
                                       interface);
                        }
                        if((argType->isBuiltinType() || argType->isEnumeralType()) && interface == "none")
                        {
                           interface = "default";
                        }
                     }
                  }
                  Fun2Params[fname].push_back(pname);
                  attr_val["interface_type"] = interface;
                  attr_val["interface_typename"] = ParamTypeName;
                  attr_val["interface_typename_orig"] = ParamTypeNameOrig;
                  attr_val["interface_typename_include"] = ParamTypeInclude;
#if PRINT_DBG_MSG
                  llvm::errs() << "    interface_type            : " << interface << "\n";
                  llvm::errs() << "    interface_typename        : " << ParamTypeName << "\n";
                  llvm::errs() << "    interface_typename_orig   : " << ParamTypeNameOrig << "\n";
                  llvm::errs() << "    interface_typename_include: " << ParamTypeInclude << "\n";
#endif
               }
               ++par_index;
            }
         }
      }

    public:
      FunctionArgConsumer(CompilerInstance& Instance, const std::string& _topfname, const std::string& _outdir_name,
                          std::string _InFile, const clang::PrintingPolicy& _pp)
          : CI(Instance), topfname(_topfname), outdir_name(_outdir_name), InFile(_InFile), pp(_pp)
      {
      }

      bool HandleTopLevelDecl(DeclGroupRef DG) override
      {
         for(const auto& D : DG)
         {
            if(const auto* FD = dyn_cast<FunctionDecl>(D))
            {
               AnalyzeFunctionDecl(FD);
               const auto endLoc = FD->getSourceRange().getEnd();
               const auto& SM = FD->getASTContext().getSourceManager();
               const auto filename = SM.getPresumedLoc(endLoc, false).getFilename();
               prevLoc[filename] = endLoc;
            }
            else if(const auto* LSD = dyn_cast<LinkageSpecDecl>(D))
            {
               for(const auto& d : LSD->decls())
               {
                  if(const auto* fd = dyn_cast<FunctionDecl>(d))
                  {
                     AnalyzeFunctionDecl(fd);
                     const auto endLoc = fd->getSourceRange().getEnd();
                     const auto& SM = fd->getASTContext().getSourceManager();
                     const auto filename = SM.getPresumedLoc(endLoc, false).getFilename();
                     prevLoc[filename] = endLoc;
                  }
               }
            }
         }
         return true;
      }

      void HandleTranslationUnit(ASTContext&) override
      {
         const auto baseFilename = create_file_basename_string(outdir_name, InFile);
         const auto interface_fun2parms_filename = baseFilename + ".params.txt";
         const auto interface_XML_filename = baseFilename + ".interface.xml";
         const auto pipeline_XML_filename = baseFilename + ".pipeline.xml";
         writeFun2Params(interface_fun2parms_filename);
         writeXML_interfaceFile(interface_XML_filename, topfname);
         writeXML_pipelineFile(pipeline_XML_filename, topfname);
      }
   };

   class HLS_interface_PragmaHandler : public PragmaHandler
   {
    public:
      HLS_interface_PragmaHandler() : PragmaHandler("HLS_interface")
      {
      }

      void HandlePragma(Preprocessor& PP,
#if __clang_major__ >= 9
                        PragmaIntroducer
#else
                        PragmaIntroducerKind
#endif
                        /*Introducer*/,
                        Token& PragmaTok) override
      {
         Token Tok{};
         const auto print_error = [&](const std::string& msg)
         {
            auto& D = PP.getDiagnostics();
            D.Report(Tok.getLocation(), D.getCustomDiagID(DiagnosticsEngine::Error, "#pragma HLS_interface %0"))
                .AddString(msg);
         };
         std::string pname;
         std::string interface;
         const auto loc = PragmaTok.getLocation();
         const auto filename = PP.getSourceManager().getPresumedLoc(loc, false).getFilename();
         const auto end_parse = [&]()
         {
            PP.Lex(Tok);
            if(Tok.isNot(tok::eod))
            {
               print_error("malformed");
            }
         };
         const auto bundle_parse = [&]()
         {
            PP.Lex(Tok);
            if(Tok.is(tok::equal))
            {
               PP.Lex(Tok);
            }
            if(Tok.is(tok::eod))
            {
               print_error("malformed bundle");
            }
            const auto bundle = PP.getSpelling(Tok);
            file_loc_arg_attr[filename][loc][pname]["attribute3"] = bundle;
            // llvm::errs() << " bundle=" << bundle;
         };
         const auto array_parse = [&]()
         {
            PP.Lex(Tok);
            if(Tok.isNot(tok::numeric_constant))
            {
               print_error("array malformed");
            }
            const auto asize = PP.getSpelling(Tok);
            file_loc_arg_attr[filename][loc][pname]["size"] = asize;
            // llvm::errs() << " " << asize;

            PP.Lex(Tok);
            if(Tok.is(tok::identifier) && PP.getSpelling(Tok) == "bundle")
            {
               bundle_parse();
               end_parse();
            }
            else if(Tok.isNot(tok::eod))
            {
               print_error("array malformed");
            }
         };
         const auto axi_parse = [&]()
         {
            PP.Lex(Tok);
            if(Tok.isNot(tok::identifier))
            {
               print_error("axi malformed");
            }
            const auto tmp = PP.getSpelling(Tok);
            if(tmp == "direct" || tmp == "axi_slave")
            {
               file_loc_arg_attr[filename][loc][pname]["attribute2"] = tmp;
            }

            PP.Lex(Tok);
            if(Tok.is(tok::identifier) && PP.getSpelling(Tok) == "bundle")
            {
               bundle_parse();
               end_parse();
            }
            else if(Tok.isNot(tok::eod))
            {
               print_error("axi malformed");
            }
         };
         const std::map<std::string, std::function<void()>> interface_parser = {
             {"none", end_parse},      {"none_registered", end_parse},
             {"bus", end_parse},       {"fifo", end_parse},
             {"handshake", end_parse}, {"valid", end_parse},
             {"ovalid", end_parse},    {"acknowledge", end_parse},
             {"array", array_parse},   {"m_axi", axi_parse},
             {"axis", end_parse}};

         // llvm::errs() << "HLS_interface";
         PP.Lex(Tok);
         if(Tok.isNot(tok::identifier))
         {
            print_error("malformed");
         }
         pname = PP.getSpelling(Tok);
         // llvm::errs() << " " << pname;
         PP.Lex(Tok);
         if(Tok.isNot(tok::identifier))
         {
            print_error("missing interface type");
         }
         interface = PP.getSpelling(Tok);
         // llvm::errs() << " " << interface;
         const auto parser = interface_parser.find(interface);
         if(parser != interface_parser.end())
         {
            file_loc_arg_attr[filename][loc][pname]["interface_type"] = interface;
            parser->second();
         }
         else
         {
            print_error("interface type not supported");
         }
         // llvm::errs() << "\n";
      }
   };

   class HLS_simple_pipeline_PragmaHandler : public PragmaHandler
   {
    public:
      HLS_simple_pipeline_PragmaHandler() : PragmaHandler("HLS_simple_pipeline")
      {
      }

      void HandlePragma(Preprocessor& PP,
#if __clang_major__ >= 9
                        PragmaIntroducer
#else
                        PragmaIntroducerKind
#endif
                        /*Introducer*/,
                        Token& PragmaTok) override
      {
         Token Tok{};
         auto loc = PragmaTok.getLocation();
         auto& SM = PP.getSourceManager();
         auto filename = SM.getPresumedLoc(loc, false).getFilename();

         while(Tok.isNot(tok::eod))
         {
            PP.Lex(Tok);
            if(Tok.isNot(tok::eod))
            {
               DiagnosticsEngine& D = PP.getDiagnostics();
               unsigned ID = D.getCustomDiagID(DiagnosticsEngine::Error, "#pragma HLS_simple_pipeline malformed");
               D.Report(PragmaTok.getLocation(), ID);
            }
         }
         file_loc_attr[filename][loc]["is_pipelined"] = "yes";
         file_loc_attr[filename][loc]["is_simple"] = "yes";
         file_loc_attr[filename][loc]["initiation_time"] = "1";
      }
   };

   class HLS_stallable_pipeline_PragmaHandler : public PragmaHandler
   {
    public:
      HLS_stallable_pipeline_PragmaHandler() : PragmaHandler("HLS_stallable_pipeline")
      {
      }

      void HandlePragma(Preprocessor& PP,
#if __clang_major__ >= 9
                        PragmaIntroducer
#else
                        PragmaIntroducerKind
#endif
                        /*Introducer*/,
                        Token& PragmaTok) override
      {
         Token Tok{};
         auto loc = PragmaTok.getLocation();
         auto& SM = PP.getSourceManager();
         auto filename = SM.getPresumedLoc(loc, false).getFilename();
         std::string time;
         int index = 0;
         while(Tok.isNot(tok::eod))
         {
            PP.Lex(Tok);
            if(Tok.isNot(tok::eod))
            {
               auto tokString = PP.getSpelling(Tok);
               if(index == 0)
               {
                  time = tokString;
                  if(Tok.isNot(tok::numeric_constant))
                  {
                     DiagnosticsEngine& D = PP.getDiagnostics();
                     unsigned ID =
                         D.getCustomDiagID(DiagnosticsEngine::Error, "#pragma HLS_stallable_pipeline malformed");
                     D.Report(PragmaTok.getLocation(), ID);
                  }
               }
               else
               {
                  DiagnosticsEngine& D = PP.getDiagnostics();
                  unsigned ID = D.getCustomDiagID(DiagnosticsEngine::Error, "#pragma HLS_stallable_pipeline malformed");
                  D.Report(PragmaTok.getLocation(), ID);
               }
               ++index;
            }
         }
         file_loc_attr[filename][loc]["is_pipelined"] = "yes";
         file_loc_attr[filename][loc]["is_simple"] = "no";
         file_loc_attr[filename][loc]["initiation_time"] = time;
      }
   };

   class CLANG_VERSION_SYMBOL(_plugin_ASTAnalyzer) : public PluginASTAction
   {
      std::string topfname;
      std::string outdir_name;
      bool cppflag;

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
         PP.AddPragmaHandler(new HLS_simple_pipeline_PragmaHandler());
         PP.AddPragmaHandler(new HLS_stallable_pipeline_PragmaHandler());
         auto pp = clang::PrintingPolicy(clang::LangOptions());
         if(cppflag)
         {
            pp.adjustForCPlusPlus();
         }
#if __clang_major__ > 9
         return std::make_unique<FunctionArgConsumer>(CI, topfname, outdir_name, InFile.data(), pp);
#else
         return llvm::make_unique<FunctionArgConsumer>(CI, topfname, outdir_name, InFile, pp);
#endif
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
            else if(args.at(i) == "-cppflag")
            {
               if(i + 1 >= e)
               {
                  D.Report(D.getCustomDiagID(DiagnosticsEngine::Error, "missing cppflag argument"));
                  return false;
               }
               ++i;
               cppflag = std::atoi(args.at(i).data()) == 1;
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
         ros << "-cppflag <type>\n";
         ros << "  1 if input source file is C++, 0 else\n";
         ros << "-topfname <function name>\n";
         ros << "  Function from which the Point-To analysis has to start\n";
      }

      PluginASTAction::ActionType getActionType() override
      {
         return AddAfterMainAction;
      }

    public:
      CLANG_VERSION_SYMBOL(_plugin_ASTAnalyzer)() : cppflag(false)
      {
      }
      CLANG_VERSION_SYMBOL(_plugin_ASTAnalyzer)(const CLANG_VERSION_SYMBOL(_plugin_ASTAnalyzer) & step) = delete;
      CLANG_VERSION_SYMBOL(_plugin_ASTAnalyzer) & operator=(const CLANG_VERSION_SYMBOL(_plugin_ASTAnalyzer) &) = delete;
   };

#ifdef _WIN32

   void initializeplugin_ASTAnalyzer()
   {
      static clang::FrontendPluginRegistry::Add<clang::CLANG_VERSION_SYMBOL(_plugin_ASTAnalyzer)> X(
          CLANG_VERSION_STRING(_plugin_ASTAnalyzer), "Analyze Clang AST to retrieve information useful for PandA");
   }
#endif
} // namespace clang

#ifndef _WIN32
static clang::FrontendPluginRegistry::Add<clang::CLANG_VERSION_SYMBOL(_plugin_ASTAnalyzer)>
    X(CLANG_VERSION_STRING(_plugin_ASTAnalyzer), "Analyze Clang AST to retrieve information useful for PandA");
#endif
