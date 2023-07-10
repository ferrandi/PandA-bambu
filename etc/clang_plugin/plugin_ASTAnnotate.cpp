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
 *              Copyright (C) 2018-2023 Politecnico di Milano
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
 * @file plugin_ASTAnnotate.cpp
 * @brief Plugin annotating the Clang AST with some attribute relevant for the HLS process
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 *
 */
// #undef NDEBUG
#include "debug_print.hpp"

#include "plugin_includes.hpp"

#include <clang/AST/AST.h>
#include <clang/AST/ASTConsumer.h>
#include <clang/AST/DeclCXX.h>
#include <clang/AST/Mangle.h>
#include <clang/AST/RecursiveASTVisitor.h>
#include <clang/AST/Type.h>
#include <clang/Frontend/CompilerInstance.h>
#include <clang/Frontend/FrontendPluginRegistry.h>
#include <clang/Lex/LexDiagnostic.h>
#include <clang/Lex/Preprocessor.h>
#include <clang/Lex/Token.h>
#include <clang/Sema/Sema.h>
#include <llvm/Support/raw_ostream.h>
#include <regex>
#include <string>

enum directiveType
{
   inline_directive,
   unroll_directive,
};

enum attributeOptions
{
   inline_opt_off,
   inline_opt_recursice,
   unroll_opt_factor,
};

namespace clang
{
   static std::map<std::string, std::map<clang::SourceLocation,
                                         std::map<directiveType, std::pair<Token, std::map<attributeOptions, Token>>>>>
       file_loc_directive;
   class FunctionDirectiveConsumer : public clang::ASTConsumer
   {
      void Analyze(FunctionDecl* FD)
      {
         auto& SM = FD->getASTContext().getSourceManager();
         const auto prev = FD->getSourceRange().getBegin();
         const auto locEnd = FD->getSourceRange().getEnd();
         const auto filename = SM.getPresumedLoc(locEnd, false).getFilename();
         if(file_loc_directive.find(filename) == file_loc_directive.end())
         {
            return;
         }
         for(const auto& location_directives : file_loc_directive.find(filename)->second)
         {
            const auto& loc = location_directives.first;
            const auto& dAttr = location_directives.second;
            if((prev < loc) && (loc < locEnd))
            {
               if(dAttr.find(inline_directive) != dAttr.end())
               {
                  auto inline_options = dAttr.find(inline_directive)->second;
                  if(inline_options.second.find(inline_opt_off) != inline_options.second.end())
                  {
                     FD->addAttr(NoInlineAttr::CreateImplicit(FD->getASTContext()));
                  }
                  else
                  {
                     FD->addAttr(AlwaysInlineAttr::CreateImplicit(FD->getASTContext()));
                  }
               }
            }
         }
      }

    public:
      FunctionDirectiveConsumer() = default;

      bool HandleTopLevelDecl(DeclGroupRef DG) override
      {
         for(auto& D : DG)
         {
            if(auto* FD = dyn_cast<FunctionDecl>(D))
            {
               Analyze(FD);
            }
            else if(auto* LSD = dyn_cast<LinkageSpecDecl>(D))
            {
               for(auto& d : LSD->decls())
               {
                  if(auto* fd = dyn_cast<FunctionDecl>(d))
                  {
                     Analyze(fd);
                  }
               }
            }
         }
         return true;
      }

      void HandleTranslationUnit(ASTContext&) override
      {
      }
   };

   class parsePragma
   {
      std::pair<Token, std::map<attributeOptions, Token>> mapRelation;
      std::set<attributeOptions> hasValueRelation;
      std::map<attributeOptions, Token> TokenMap;

      Preprocessor& PP;
      const StringRef& PragmaClass;
      std::map<std::string, attributeOptions> expected_tokens;
      void print_error(const std::string& msg, Token& Tok)
      {
         auto& D = PP.getDiagnostics();
         D.Report(Tok.getLocation(), D.getCustomDiagID(DiagnosticsEngine::Error, "#pragma %0"))
             .AddString(PragmaClass.str() + " " + msg + " Token: " + PP.getSpelling(Tok));
      };

    public:
      parsePragma(const StringRef& _PragmaClass, Token PragmaName, Preprocessor& _PP,
                  const std::map<std::string, attributeOptions>& _expected_tokens)
          : PP(_PP), PragmaClass(_PragmaClass), expected_tokens(_expected_tokens)
      {
         mapRelation.first = PragmaName;
      }

      void analyzePragma()
      {
         Token Tok{};
         PP.Lex(Tok);
         while(Tok.isNot(tok::eod))
         {
            std::string identifier;
            Token value;
            value.startToken();
            if(Tok.is(tok::identifier))
            {
               identifier = PP.getSpelling(Tok);
            }
            else
            {
               print_error("malformed pragma,", Tok);
            }
            if(expected_tokens.find(identifier) == expected_tokens.end())
            {
               print_error("not supported token,", Tok);
            }
            auto id = expected_tokens.find(identifier)->second;
            TokenMap[id] = Tok;
            PP.Lex(Tok);
            if(Tok.is(tok::equal))
            {
               PP.Lex(Tok);
               if(Tok.is(tok::identifier))
               {
                  value = Tok;
               }
               else if(Tok.is(tok::numeric_constant))
               {
                  value = Tok;
               }
               else
               {
                  print_error("malformed pragma,", Tok);
               }
               PP.Lex(Tok);
            }

            if(mapRelation.second.count(id))
            {
               print_error("duplicated identifier,", Tok);
            }
            mapRelation.second[id] = value;
            if(value.isNot(tok::unknown))
            {
               hasValueRelation.insert(id);
            }
         }
      }
      bool hasIdentifier(attributeOptions id) const
      {
         return mapRelation.second.count(id) != 0;
      }
      bool hasValue(attributeOptions id) const
      {
         assert(hasIdentifier(id));
         return hasValueRelation.count(id) != 0;
      }
      Token getValue(attributeOptions id) const
      {
         assert(hasValue(id));
         return mapRelation.second.find(id)->second;
      }
      Token getToken(attributeOptions id) const
      {
         assert(hasIdentifier(id));
         return TokenMap.find(id)->second;
      }
      std::pair<Token, std::map<attributeOptions, Token>> getMap()
      {
         return mapRelation;
      }
   };

   class HLS_PragmaHandler : public PragmaHandler
   {
    public:
      explicit HLS_PragmaHandler(StringRef Name) : PragmaHandler(Name)
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
         std::map<std::string, directiveType> s2directive = {{"inline", inline_directive},
                                                             {"unroll", unroll_directive}};
         Token Tok{};
         const auto print_error = [&](const std::string& msg) {
            auto& D = PP.getDiagnostics();
            D.Report(Tok.getLocation(), D.getCustomDiagID(DiagnosticsEngine::Error, "#pragma %0"))
                .AddString(getName().str() + " " + msg);
         };
         const auto print_warning = [&](const std::string& msg) {
            auto& D = PP.getDiagnostics();
            D.Report(Tok.getLocation(), D.getCustomDiagID(DiagnosticsEngine::Warning, "#pragma %0"))
                .AddString(getName().str() + " " + msg);
         };

         const auto loc = PragmaTok.getLocation();
         auto& SM = PP.getSourceManager();
         auto filename = SM.getPresumedLoc(loc, false).getFilename();

         PP.Lex(Tok);
         if(Tok.is(tok::kw_inline))
         {
            parsePragma pp(getName(), Tok, PP, {{"recursive", inline_opt_recursice}, {"off", inline_opt_off}});
            pp.analyzePragma();
            bool has_recursive = pp.hasIdentifier(inline_opt_off);
            bool has_off = pp.hasIdentifier(inline_opt_recursice);
            if(has_recursive)
            {
               print_warning("recursive clause not yet implemented");
            }
            file_loc_directive[filename][loc][inline_directive] = pp.getMap();
         }
         else if(Tok.isNot(tok::identifier))
         {
            print_error("malformed pragma");
         }
         else
         {
            const std::string name = PP.getSpelling(Tok);
            const auto PragmaName = Tok;
            if(s2directive.count(name))
            {
               if(s2directive.at(name) == unroll_directive)
               {
                  parsePragma pp(getName(), Tok, PP, {{"factor", unroll_opt_factor}});
                  pp.analyzePragma();
                  bool has_factor = pp.hasIdentifier(unroll_opt_factor);
                  bool has_factor_value = pp.hasValue(unroll_opt_factor);

                  auto* Info = new(PP.getPreprocessorAllocator()) PragmaLoopHintInfo;
                  Info->PragmaName = PragmaName;
                  Info->Option.startToken();
                  if(has_factor_value)
                  {
                     SmallVector<Token, 1> ValueList;
                     ValueList.push_back(pp.getValue(unroll_opt_factor));
                     Token EOFTok;
                     EOFTok.startToken();
                     EOFTok.setKind(tok::eof);
                     EOFTok.setLocation(Tok.getLocation());
                     ValueList.push_back(EOFTok);
                     for(auto& T : ValueList)
                     {
                        T.setFlag(clang::Token::IsReinjected);
                     }
                     Info->Toks = llvm::makeArrayRef(ValueList).copy(PP.getPreprocessorAllocator());
                  }
                  auto TokenArray = std::make_unique<Token[]>(1);
                  TokenArray[0].startToken();
                  TokenArray[0].setKind(tok::annot_pragma_loop_hint);
                  TokenArray[0].setLocation(loc);
                  TokenArray[0].setAnnotationEndLoc(loc);
                  TokenArray[0].setAnnotationValue(static_cast<void*>(Info));
                  PP.EnterTokenStream(std::move(TokenArray), 1,
                                      /*DisableMacroExpansion=*/false, /*IsReinject=*/false);
               }
            }
         }
      }
   };

   class MyPPCallbacks : public PPCallbacks
   {
    public:
      // Implement callbacks here
   };
   class CLANG_VERSION_SYMBOL(_plugin_ASTAnnotate) : public PluginASTAction
   {
    protected:
      std::unique_ptr<ASTConsumer> CreateASTConsumer(CompilerInstance& CI, llvm::StringRef) override
      {
         clang::Preprocessor& PP = CI.getPreprocessor();
         PP.AddPragmaHandler(new HLS_PragmaHandler("HLS"));
         PP.AddPragmaHandler(new HLS_PragmaHandler("hls"));
         PP.AddPragmaHandler(new HLS_PragmaHandler("bambu"));
#if __clang_major__ > 9
         return std::make_unique<FunctionDirectiveConsumer>();
#else
         return llvm::make_unique<FunctionDirectiveConsumer>();
#endif
      }

      bool ParseArgs(const CompilerInstance& CI, const std::vector<std::string>& args) override
      {
         if(!args.empty() && args.at(0) == "-help")
         {
            PrintHelp(llvm::errs());
         }
         return true;
      }

      void PrintHelp(llvm::raw_ostream& ros)
      {
         ros << "Help for " CLANG_VERSION_STRING(_plugin_ASTAnnotate) " plugin\n";
      }

      PluginASTAction::ActionType getActionType() override
      {
         return AddBeforeMainAction;
      }

    public:
      CLANG_VERSION_SYMBOL(_plugin_ASTAnnotate)()
      {
      }
      CLANG_VERSION_SYMBOL(_plugin_ASTAnnotate)(const CLANG_VERSION_SYMBOL(_plugin_ASTAnnotate) & step) = delete;
      CLANG_VERSION_SYMBOL(_plugin_ASTAnnotate) & operator=(const CLANG_VERSION_SYMBOL(_plugin_ASTAnnotate) &) = delete;
   };

#ifdef _WIN32

   void initializeplugin_ASTAnnotate()
   {
      static clang::FrontendPluginRegistry::Add<clang::CLANG_VERSION_SYMBOL(_plugin_ASTAnnotate)> X(
          CLANG_VERSION_STRING(_plugin_ASTAnnotate), "Annotate Clang AST starting from HLS pragmas");
   }
#endif
} // namespace clang

#ifndef _WIN32
static clang::FrontendPluginRegistry::Add<clang::CLANG_VERSION_SYMBOL(_plugin_ASTAnnotate)>
    X(CLANG_VERSION_STRING(_plugin_ASTAnnotate), "Annotate Clang AST starting from HLS pragmas");
#endif
