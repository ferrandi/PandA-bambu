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
 * @file plugin_ASTAnalyzer.cpp
 * @brief Plugin analyzing the Clang AST retrieving useful information for PandA
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 * @author Michele Fiorito <michele.fiorito@polimi.it>
 *
 */
// #undef NDEBUG
#include "plugin_includes.hpp"

#include <clang/AST/AST.h>
#include <clang/AST/ASTConsumer.h>
#include <clang/AST/ASTContext.h>
#include <clang/AST/DeclCXX.h>
#include <clang/AST/GlobalDecl.h>
#include <clang/AST/Mangle.h>
#include <clang/AST/RecordLayout.h>
#include <clang/AST/RecursiveASTVisitor.h>
#include <clang/AST/Type.h>
#include <clang/Frontend/CompilerInstance.h>
#include <clang/Frontend/FrontendPluginRegistry.h>
#include <clang/Lex/LexDiagnostic.h>
#include <clang/Lex/Preprocessor.h>
#include <clang/Lex/Token.h>
#include <clang/Sema/Sema.h>
#include <llvm/Support/raw_ostream.h>

#define PUGIXML_NO_EXCEPTIONS
#define PUGIXML_HEADER_ONLY

#include <pugixml.hpp>

#include <fstream>
#include <list>
#include <map>
#include <memory>
#include <regex>
#include <string>

#define REWRITE_REGEX

#define THIS_PARAM_NAME "this"

#if __clang_major__ > 9
#define make_unique std::make_unique
#else
#define make_unique llvm::make_unique
#endif

using namespace llvm;
using namespace clang;

static inline __attribute__((always_inline)) bool iequals(const char* a, const char* b)
{
   return strcasecmp(a, b) == 0;
}

static inline __attribute__((always_inline)) bool iequals(const std::string& a, const char* b)
{
   return iequals(a.c_str(), b);
}

// static inline __attribute__((always_inline)) bool iequals(const std::string& a, const std::string& b)
// {
//    return iequals(a.c_str(), b.c_str());
// }

static inline __attribute__((always_inline)) std::string to_lower(const std::string& str)
{
   std::string s;
   s.reserve(str.size());
   std::transform(str.begin(), str.end(), std::back_inserter(s), [](unsigned char c) { return std::tolower(c); });
   return s;
}

struct key_loc_t
{
   std::string id;
   SourceLocation loc;

   key_loc_t(const std::string& _id, const SourceLocation& _loc) : id(_id), loc(_loc)
   {
   }
};

struct key_loc_cmp
{
   bool operator()(const key_loc_t& a, const key_loc_t& b) const
   {
      return strcasecmp(a.id.c_str(), b.id.c_str()) < 0;
   }
};
typedef std::map<key_loc_t, std::string, key_loc_cmp> attr_map_t;

pugi::xml_node& operator<<(pugi::xml_node& _n, const attr_map_t& _m)
{
   for(auto& p : _m)
   {
      _n.append_attribute(p.first.id.c_str()).set_value(p.second.c_str());
   }
   return _n;
}

const pugi::xml_node& operator>>(const pugi::xml_node& _n, attr_map_t& _m)
{
   for(auto& attr : _n.attributes())
   {
      _m.emplace(key_loc_t(attr.name(), SourceLocation()), attr.value());
   }
   return _n;
}

static void attr_map_set_union(attr_map_t& a, const attr_map_t& b)
{
   key_loc_cmp comp{};
   auto first1 = a.begin(), last1 = a.end();
   auto first2 = b.begin(), last2 = b.end();
   auto d_first = std::inserter(a, a.end());
   for(; first1 != last1; ++d_first)
   {
      if(first2 == last2)
         return;

      if(comp(first2->first, first1->first))
         *d_first = *first2++;
      else
      {
         *d_first = *first1;
         if(!comp(first1->first, first2->first))
            ++first2;
         ++first1;
      }
   }
   std::copy(first2, last2, d_first);
}

typedef std::map<std::string, attr_map_t> key_attr_map_t;

static void key_attr_map_set_union(std::map<std::string, attr_map_t>& a, const std::map<std::string, attr_map_t>& b)
{
   auto first1 = a.begin(), last1 = a.end();
   auto first2 = b.begin(), last2 = b.end();
   auto d_first = std::inserter(a, a.end());
   for(; first1 != last1; ++d_first)
   {
      if(first2 == last2)
         return;

      if(first2->first < first1->first)
         *d_first = *first2++;
      else
      {
         // *d_first = *first1;
         if(!(first1->first < first2->first))
         {
            attr_map_set_union(first1->second, first2->second);
            ++first2;
         }
         ++first1;
      }
   }
   std::copy(first2, last2, d_first);
}

struct __interface_attr
{
   std::map<std::string, attr_map_t> bundles;
   std::map<std::string, attr_map_t> parameters;

   __interface_attr& operator=(const __interface_attr& other)
   {
      key_attr_map_set_union(bundles, other.bundles);
      key_attr_map_set_union(parameters, other.parameters);
      return *this;
   }
};
typedef __interface_attr interface_attr_t;

pugi::xml_node& operator<<(pugi::xml_node& _n, const interface_attr_t& _m)
{
   auto ifaces = _n.append_child("bundles");
   for(auto& p : _m.bundles)
   {
      auto iface = ifaces.append_child("bundle");
      iface << p.second;
   }
   auto parms = _n.append_child("parameters");
   for(auto& p : _m.parameters)
   {
      auto parm = parms.append_child("parameter");
      parm << p.second;
   }
   return _n;
}

const pugi::xml_node& operator>>(const pugi::xml_node& _n, interface_attr_t& _m)
{
   pugi::xml_node c;
   if((c = _n.child("bundles")))
   {
      for(auto& i : c)
      {
         std::string bundleName = i.attribute("name").value();
         i >> _m.bundles[bundleName];
      }
   }
   if((c = _n.child("parameters")))
   {
      for(auto& p : c)
      {
         std::string parmPort = p.attribute("port").value();
         p >> _m.parameters[parmPort];
      }
   }
   return _n;
}

struct __func_attr
{
   attr_map_t attrs;
   interface_attr_t ifaces;

   __func_attr& operator=(const __func_attr& other)
   {
      attr_map_set_union(attrs, other.attrs);
      ifaces = other.ifaces;
      return *this;
   }
};
typedef __func_attr func_attr_t;

pugi::xml_node& operator<<(pugi::xml_node& _n, const func_attr_t& _m)
{
   return _n << _m.attrs << _m.ifaces;
}

const pugi::xml_node& operator>>(const pugi::xml_node& _n, func_attr_t& _m)
{
   _n >> _m.attrs;
   _n >> _m.ifaces;
   return _n;
}

struct pragma_line_t
{
 public:
   // Preprocessor reference
   Preprocessor* PP;
   // Pragma begin location
   SourceLocation pragmaLoc;
   // Pragma class identifier
   std::string id;
   // Pragma class identifier location
   SourceLocation loc;
   // Pragma attributes
   attr_map_t attrs;

   pragma_line_t(Preprocessor* _PP, const SourceLocation& _pragmaLoc, const std::string& _id,
                 const SourceLocation& _loc)
       : PP(_PP), pragmaLoc(_pragmaLoc), id(_id), loc(_loc)
   {
   }
};

enum ParseAction
{
   ParseAction_None = 0,
   ParseAction_Analyze = 1,
   ParseAction_Optimize = 2,
};

#define DEBUG_TYPE "hls-pragmas"

static void Report(DiagnosticsEngine& D, const SourceLocation& loc, DiagnosticsEngine::Level level, StringRef msg)
{
   D.Report(loc, D.getCustomDiagID(level, "%0")).AddString(msg);
}

static void ReportDuplicate(DiagnosticsEngine& D, const SourceLocation& loc, const SourceLocation& prev, StringRef msg)
{
   Report(D, loc, DiagnosticsEngine::Error, "");
   Report(D, prev, DiagnosticsEngine::Remark, "Previously defined here");
}

static std::string MangledName(MangleContext* MC, NamedDecl* D)
{
   if(!MC->shouldMangleDeclName(D))
   {
      return D->getDeclName().getAsString();
   }
   std::string fsym;
   raw_string_ostream ss(fsym);
   if(auto CD = dyn_cast<CXXConstructorDecl>(D))
   {
#if __clang_major__ > 10
      MC->mangleName(GlobalDecl(CD, CXXCtorType::Ctor_Complete), ss);
#else
      MC->mangleCXXCtor(CD, CXXCtorType::Ctor_Complete, ss);
#endif
   }
   else if(auto CD = dyn_cast<CXXDestructorDecl>(D))
   {
#if __clang_major__ > 10
      MC->mangleName(GlobalDecl(CD, CXXDtorType::Dtor_Complete), ss);
#else
      MC->mangleCXXDtor(CD, CXXDtorType::Dtor_Complete, ss);
#endif
   }
   else if(MC->shouldMangleCXXName(D))
   {
      MC->mangleCXXName(D, ss);
   }
   else
   {
      MC->mangleName(D, ss);
   }
   ss.flush();
   return fsym;
}

static func_attr_t& GetFuncAttr(std::map<std::string, func_attr_t>& funcAttrs, MangleContext* MC, FunctionDecl* FD)
{
   const auto funcSymbol = MangledName(MC, FD);
   auto funcIt = funcAttrs.find(funcSymbol);
   if(funcIt == funcAttrs.end())
   {
      funcIt = funcAttrs.emplace(funcSymbol, func_attr_t()).first;
      funcIt->second.attrs.emplace(key_loc_t("symbol", FD->getLocation()), funcSymbol);
      funcIt->second.attrs.emplace(key_loc_t("name", FD->getLocation()), FD->getNameInfo().getName().getAsString());
   }
   return funcIt->second;
}

class HLSPragmaParser
{
 public:
   HLSPragmaParser() = default;

   virtual ~HLSPragmaParser() = default;

   /**
    * @brief Check pragma id matching and handle pragma
    * @return bool true on pragma id match, false else
    */
   virtual bool HandlePragma(Preprocessor& PP,
#if __clang_major__ >= 9
                             PragmaIntroducer
#else
                             PragmaIntroducerKind
#endif
                                 Introducer,
                             Token& PragmaTok, Token& Tok)
   {
      return false;
   }
};

class HLSPragmaHandler : public PragmaHandler
{
 public:
   using HLSPragmaParserSet = std::map<std::string, std::shared_ptr<HLSPragmaParser>>;

 private:
   const HLSPragmaParserSet& _parsers;
   std::list<pragma_line_t>& _pragmas;

   inline void Report(Preprocessor& PP, const SourceLocation& loc, DiagnosticsEngine::Level level, StringRef msg) const
   {
      ::Report(PP.getDiagnostics(), loc, level, msg);
   }

 public:
   HLSPragmaHandler(StringRef name, const HLSPragmaParserSet& parsers, std::list<pragma_line_t>& pragmas)
       : PragmaHandler(name), _parsers(parsers), _pragmas(pragmas)
   {
   }

   void HandlePragma(Preprocessor& PP,
#if __clang_major__ >= 9
                     PragmaIntroducer
#else
                     PragmaIntroducerKind
#endif
                         Introducer,
                     Token& pragmaTok) override
   {
      auto pragmaLoc = pragmaTok.getLocation();
      Token Tok{};

      PP.Lex(Tok);
      const auto pragma_class = PP.getSpelling(Tok);
      const auto parser_it = _parsers.find(pragma_class);
      if(parser_it != _parsers.end())
      {
         if(parser_it->second->HandlePragma(PP, Introducer, pragmaTok, Tok))
         {
            return;
         }
         pragma_line_t p(&PP, pragmaLoc, PP.getSpelling(Tok), Tok.getLocation());
         for(PP.Lex(Tok); Tok.isNot(tok::eod);)
         {
            if(Tok.isOneOf(tok::identifier, tok::raw_identifier, tok::kw_register))
            {
               auto& attr_val = p.attrs[key_loc_t(PP.getSpelling(Tok), Tok.getLocation())];
               attr_val = "";

               PP.Lex(Tok);
               if(Tok.is(tok::equal))
               {
                  PP.Lex(Tok);
                  if(Tok.isOneOf(tok::identifier, tok::raw_identifier, tok::kw_true, tok::kw_false) ||
                     tok::isLiteral(Tok.getKind()))
                  {
                     attr_val = PP.getSpelling(Tok);
                     PP.Lex(Tok);
                     continue;
                  }
               }
               else if(Tok.isOneOf(tok::identifier, tok::raw_identifier, tok::eod))
               {
                  continue;
               }
            }
            return Report(PP, Tok.getLocation(), DiagnosticsEngine::Error, "Unexpected token");
         }
         LLVM_DEBUG(dbgs() << "Parsed pragma " << p.id << " (" << p.attrs.size() << ") "
                           << p.loc.printToString(PP.getSourceManager()) << "\n");
         _pragmas.push_back(std::move(p));
         return;
      }
      Report(PP, Tok.getLocation(), DiagnosticsEngine::Warning, "Unknown HLS pragma");
   }
};

class HLSPragmaAnalyzer
{
   DiagnosticsEngine& _D;
   MangleContext* _MC;

 protected:
   PrintingPolicy _PP;
   std::map<std::string, func_attr_t>& _func_attributes;

   inline void Report(const SourceLocation& loc, DiagnosticsEngine::Level level, StringRef msg) const
   {
      ::Report(_D, loc, level, msg);
   }

   inline void ReportError(const SourceLocation& loc, StringRef msg) const
   {
      ::Report(_D, loc, DiagnosticsEngine::Level::Error, msg);
   }

   inline void ReportDuplicate(const SourceLocation& loc, const SourceLocation& prev, StringRef msg) const
   {
      ::ReportDuplicate(_D, loc, prev, msg);
   }

   inline std::string MangledName(NamedDecl* FD) const
   {
      return ::MangledName(_MC, FD);
   }

   inline void ForwardAttr(func_attr_t& func_attr, attr_map_t::const_reference attr, const std::string& prefix) const
   {
      func_attr.attrs.emplace(key_loc_t(to_lower(prefix + attr.first.id), attr.first.loc), attr.second);
   }

   inline func_attr_t& GetFuncAttr(FunctionDecl* FD)
   {
      return ::GetFuncAttr(_func_attributes, _MC, FD);
   }

 public:
   HLSPragmaAnalyzer(ASTContext& ctx, PrintingPolicy& pp, std::map<std::string, func_attr_t>& func_attributes)
       : _D(ctx.getDiagnostics()), _MC(ctx.createMangleContext()), _PP(pp), _func_attributes(func_attributes)
   {
   }

   virtual ~HLSPragmaAnalyzer()
   {
      delete _MC;
   };

   /**
    * @brief Analyze pragma associated with given function declaration
    *
    * @param FD Function declaration
    * @param p Parsed pragma line
    */
   virtual void operator()(FunctionDecl* FD, const pragma_line_t& p)
   {
   }

   /**
    * @brief Called for each function declaration after all associated pragmas have been analyzed
    *
    * @param FD Function declaration
    */
   virtual void finalize(FunctionDecl* FD)
   {
   }
};

class PipelineHLSPragmaHandler : public HLSPragmaAnalyzer, public HLSPragmaParser
{
 public:
   PipelineHLSPragmaHandler(ASTContext& ctx, PrintingPolicy& pp, std::map<std::string, func_attr_t>& func_attributes)
       : HLSPragmaAnalyzer(ctx, pp, func_attributes), HLSPragmaParser()
   {
   }
   ~PipelineHLSPragmaHandler() = default;

   void operator()(FunctionDecl* FD, const pragma_line_t& p) override
   {
      auto& func_attr = GetFuncAttr(FD).attrs;
      for(const auto& attr : p.attrs)
      {
         if(iequals(attr.first.id, "style"))
         {
            if(!iequals(attr.second, "frp"))
            {
               ReportError(attr.first.loc, "Not supported function pipelining style");
            }
         }
         else if(iequals(attr.first.id, "II"))
         {
            if(std::stoi(attr.second) <= 0)
            {
               ReportError(attr.first.loc, "Pipelining initiation interval must be a positive integer value");
            }
            else if(std::stoi(attr.second) > 1)
            {
               ReportError(attr.first.loc, "Pipelining initiation interval greater than one not yet supported");
            }
         }
         else if(iequals(attr.first.id, "off"))
         {
            if(attr.second.size())
            {
               ReportError(attr.first.loc, "Unexpected argument value");
            }
            func_attr[key_loc_t("pipeline_style", attr.first.loc)] = "off";
            continue;
         }
         else
         {
            ReportError(attr.first.loc, "Unexpected attribute");
            continue;
         }
         auto it_new = func_attr.emplace(key_loc_t("pipeline_" + to_lower(attr.first.id), attr.first.loc), attr.second);
         if(!it_new.second)
         {
            ReportDuplicate(attr.first.loc, it_new.first->first.loc,
                            "Duplicate definition of attribute '" + attr.first.id + "'");
         }
      }
      func_attr.emplace(key_loc_t("pipeline_ii", p.loc), "1");
      func_attr.emplace(key_loc_t("pipeline_style", p.loc), "frp");
   }

   static const char* PragmaKeyword;
};
const char* PipelineHLSPragmaHandler::PragmaKeyword = "pipeline";

class InlineHLSPragmaHandler : public HLSPragmaAnalyzer, public HLSPragmaParser
{
 public:
   InlineHLSPragmaHandler(ASTContext& ctx, PrintingPolicy& pp, std::map<std::string, func_attr_t>& func_attributes)
       : HLSPragmaAnalyzer(ctx, pp, func_attributes), HLSPragmaParser()
   {
   }
   ~InlineHLSPragmaHandler() = default;

   void operator()(FunctionDecl* FD, const pragma_line_t& p) override
   {
      auto& attrs = GetFuncAttr(FD).attrs;
      if(p.attrs.size() > 1)
      {
         ReportError(p.loc, "Unexpected attributes");
         return;
      }
      else if(p.attrs.size() == 1)
      {
         const auto& attr = *p.attrs.begin();
         if(iequals(attr.first.id, "recursive"))
         {
#if __clang_major__ >= 10
            FD->addAttr(AlwaysInlineAttr::CreateImplicit(FD->getASTContext()));
#else
            FD->addAttr(
                AlwaysInlineAttr::CreateImplicit(FD->getASTContext(), AlwaysInlineAttr::Spelling::Keyword_forceinline));
#endif
            FD->dropAttr<NoInlineAttr>();
         }
         else if(iequals(attr.first.id, "off"))
         {
            FD->addAttr(NoInlineAttr::CreateImplicit(FD->getASTContext()));
            FD->dropAttr<AlwaysInlineAttr>();
         }
         else
         {
            ReportError(attr.first.loc, "Unknown inline mode");
            return;
         }
         attrs.emplace(key_loc_t("inline", attr.first.loc), to_lower(attr.first.id));
      }
      else
      {
#if __clang_major__ >= 10
         FD->addAttr(AlwaysInlineAttr::CreateImplicit(FD->getASTContext()));
#else
         FD->addAttr(
             AlwaysInlineAttr::CreateImplicit(FD->getASTContext(), AlwaysInlineAttr::Spelling::Keyword_forceinline));
#endif
         attrs.emplace(key_loc_t("inline", p.loc), "self");
      }
   }

   void finalize(FunctionDecl* FD) override
   {
      std::string inline_attr;
      if(FD->hasAttr<NoInlineAttr>())
      {
         inline_attr = "off";
      }
      else if(FD->hasAttr<AlwaysInlineAttr>())
      {
         inline_attr = "self";
      }
      if(inline_attr.size())
      {
         GetFuncAttr(FD).attrs.emplace(key_loc_t("inline", FD->getLocation()), inline_attr);
      }
   }

   static const char* PragmaKeyword;
};
const char* InlineHLSPragmaHandler::PragmaKeyword = "inline";

#if __clang_major__ < 16
namespace
{
   struct PragmaLoopHintInfo
   {
      Token PragmaName;
      Token Option;
      ArrayRef<Token> Toks;
   };
} // end anonymous namespace
#endif

class UnrollHLSPragmaHandler : public HLSPragmaAnalyzer, public HLSPragmaParser
{
   bool HandlePragma(Preprocessor& PP,
#if __clang_major__ >= 9
                     PragmaIntroducer
#else
                     PragmaIntroducerKind
#endif
                         Introducer,
                     Token& PragmaTok, Token& UnrollTok) override
   {
      Token Tok;
      bool disable_unroll = false;
      Token UnrollFactor;
      UnrollFactor.startToken();

      for(PP.Lex(Tok); Tok.isNot(tok::eod);)
      {
         const auto id = PP.getSpelling(Tok);
         if(iequals(id, "factor"))
         {
            PP.Lex(Tok);
            if(Tok.is(tok::equal))
            {
               PP.Lex(Tok);
               if(Tok.is(tok::numeric_constant))
               {
                  UnrollFactor = Tok;
                  PP.Lex(Tok);
                  continue;
               }
            }
         }
         else if(iequals(id, "off"))
         {
            PP.Lex(Tok);
            if(Tok.is(tok::equal))
            {
               PP.Lex(Tok);
               if(Tok.is(tok::kw_true))
               {
                  disable_unroll = true;
                  PP.Lex(Tok);
                  continue;
               }
            }
         }
         ReportError(Tok.getLocation(), "Unexpected token");
         return true;
      }

      auto* Info = new(PP.getPreprocessorAllocator()) PragmaLoopHintInfo;
      Info->PragmaName = UnrollTok;
      Info->Option.startToken();
      if(disable_unroll)
      {
         Info->Option.setKind(tok::identifier);
         PP.CreateString("disable", Info->Option, UnrollTok.getEndLoc().getLocWithOffset(1));
      }
      else if(UnrollFactor.isNot(tok::unknown))
      {
         SmallVector<Token, 2> ValueList;

         Token EOFTok;
         EOFTok.startToken();
         EOFTok.setKind(tok::eof);
         EOFTok.setLocation(Tok.getLocation());

         ValueList.push_back(UnrollFactor);
         ValueList.push_back(EOFTok);
#if __clang_major__ >= 9
         for(auto& T : ValueList)
         {
            T.setFlag(Token::IsReinjected);
         }
#endif
         Info->Toks =
#if __clang_major__ > 13
             ArrayRef(ValueList).copy(PP.getPreprocessorAllocator());
#else
             makeArrayRef(ValueList).copy(PP.getPreprocessorAllocator());
#endif
      }
      auto TokenArray = make_unique<Token[]>(1);
      TokenArray[0].startToken();
      TokenArray[0].setKind(tok::annot_pragma_loop_hint);
      TokenArray[0].setLocation(Info->PragmaName.getLocation());
      TokenArray[0].setAnnotationEndLoc(Info->PragmaName.getLocation());
      TokenArray[0].setAnnotationValue(static_cast<void*>(Info));
      PP.EnterTokenStream(std::move(TokenArray), 1,
                          /*DisableMacroExpansion=*/false
#if __clang_major__ >= 9
                          ,
                          /*IsReinject=*/false
#endif
      );

      return true;
   }

 public:
   UnrollHLSPragmaHandler(ASTContext& ctx, PrintingPolicy& pp, std::map<std::string, func_attr_t>& func_attributes)
       : HLSPragmaAnalyzer(ctx, pp, func_attributes), HLSPragmaParser()
   {
   }

   ~UnrollHLSPragmaHandler() = default;

   static const char* PragmaKeyword;
};
const char* UnrollHLSPragmaHandler::PragmaKeyword = "unroll";

class DataflowHLSPragmaHandler : public HLSPragmaAnalyzer, public HLSPragmaParser
{
   void forceNoInline(FunctionDecl* FD) const
   {
      if(!FD->hasAttr<NoInlineAttr>())
      {
         FD->addAttr(NoInlineAttr::CreateImplicit(FD->getASTContext()));
         FD->dropAttr<AlwaysInlineAttr>();
      }
      FD->dropAttr<InternalLinkageAttr>();
   }

 public:
   DataflowHLSPragmaHandler(ASTContext& ctx, PrintingPolicy& pp, std::map<std::string, func_attr_t>& func_attributes)
       : HLSPragmaAnalyzer(ctx, pp, func_attributes), HLSPragmaParser()
   {
   }
   ~DataflowHLSPragmaHandler() = default;

   void operator()(FunctionDecl* FD, const pragma_line_t& p) override
   {
      for(auto& attr : p.attrs)
      {
         ReportError(attr.first.loc, "Unexpected attribute");
      }
      GetFuncAttr(FD).attrs.emplace(key_loc_t(p.id, p.loc), "top");
   }

   void finalize(FunctionDecl* FD) override
   {
      const auto functionSym = MangledName(FD);
      if(_func_attributes.find(functionSym) != _func_attributes.end())
      {
         auto& attrs = _func_attributes.find(functionSym)->second.attrs;
         if(attrs.find(key_loc_t("dataflow", SourceLocation())) != attrs.end() &&
            attrs.find(key_loc_t("dataflow", SourceLocation()))->second == "top")
         {
            bool hasModule = false;
            LLVM_DEBUG(dbgs() << "DATAFLOW: " << functionSym << "\n");
            forceNoInline(FD);
            for(auto* stmt : FD->getBody()->children())
            {
               auto callExpr = dyn_cast<CallExpr>(stmt);
               if(callExpr)
               {
                  const auto calleeDecl = dyn_cast<FunctionDecl>(callExpr->getCalleeDecl());
                  if(calleeDecl)
                  {
                     LLVM_DEBUG(dbgs() << " -> " << MangledName(calleeDecl) << "\n");
                     GetFuncAttr(calleeDecl).attrs[key_loc_t("dataflow", SourceLocation())] = "module";
                     GetFuncAttr(calleeDecl).attrs[key_loc_t("inline", SourceLocation())] = "off";
                     forceNoInline(calleeDecl);

                     hasModule = true;
                  }
               }
            }
            if(!hasModule)
            {
               ReportError(FD->getLocation(), "Dataflow function has no valid submodule");
            }
         }
      }
   }

   static const char* PragmaKeyword;
};
const char* DataflowHLSPragmaHandler::PragmaKeyword = "dataflow";

class CacheHLSPragmaHandler : public HLSPragmaAnalyzer, public HLSPragmaParser
{
 public:
   CacheHLSPragmaHandler(ASTContext& ctx, PrintingPolicy& pp, std::map<std::string, func_attr_t>& func_attributes)
       : HLSPragmaAnalyzer(ctx, pp, func_attributes), HLSPragmaParser()
   {
   }
   ~CacheHLSPragmaHandler() = default;

   void operator()(FunctionDecl* FD, const pragma_line_t& p) override
   {
      auto bundleName = p.attrs.find(key_loc_t("bundle", SourceLocation()));
      if(bundleName == p.attrs.end())
      {
         ReportError(p.loc, "Missing bundle name");
         return;
      }
      auto& iface = GetFuncAttr(FD).ifaces.bundles[bundleName->second];
      for(const auto& attr : p.attrs)
      {
         if(iequals(attr.first.id, "bundle"))
         {
            iface.emplace(key_loc_t("name", attr.first.loc), attr.second);
            continue;
         }
         else if(iequals(attr.first.id, "bus_size"))
         {
            auto bus_size = std::stoi(attr.second);
            if(bus_size < 32 || bus_size > 1024 || (bus_size & (bus_size - 1)) != 0)
            {
               ReportError(attr.first.loc, "Invalid cache bus size");
            }
         }
         else if(iequals(attr.first.id, "ways"))
         {
            if(std::stoi(attr.second) <= 0)
            {
               ReportError(attr.first.loc, "Invalid cache way count");
            }
         }
         else if(iequals(attr.first.id, "line_count"))
         {
            if(std::stoi(attr.second) <= 0)
            {
               ReportError(attr.first.loc, "Invalid cache line count");
            }
         }
         else if(iequals(attr.first.id, "line_size"))
         {
            if(std::stoi(attr.second) <= 0)
            {
               ReportError(attr.first.loc, "Invalid cache line size");
            }
         }
         else if(iequals(attr.first.id, "num_write_outstanding"))
         {
            if(std::stoi(attr.second) <= 0)
            {
               ReportError(attr.first.loc, "Invalid number of outstanding write operations");
            }
         }
         else if(iequals(attr.first.id, "rep_policy"))
         {
            if(attr.second != "lru" && attr.second != "tree")
            {
               ReportError(attr.first.loc, "Invalid cache replacement policy");
            }
         }
         else if(iequals(attr.first.id, "write_policy"))
         {
            if(attr.second != "wb" && attr.second != "wt")
            {
               ReportError(attr.first.loc, "Invalid cache write policy");
            }
         }
         else
         {
            ReportError(attr.first.loc, "Unexpected attribute");
            continue;
         }
         auto it_new = iface.emplace(key_loc_t("cache_" + attr.first.id, attr.first.loc), attr.second);
         if(!it_new.second)
         {
            ReportDuplicate(attr.first.loc, it_new.first->first.loc,
                            "Duplicate definition of attribute '" + attr.first.id + "'");
         }
      }
      if(iface.find(key_loc_t("cache_line_count", SourceLocation())) == iface.end())
      {
         ReportError(p.loc, "Missing cache line count attribute");
      }
      if(iface.find(key_loc_t("cache_line_size", SourceLocation())) == iface.end())
      {
         ReportError(p.loc, "Missing cache line size attribute");
      }
   }

   static const char* PragmaKeyword;
};
const char* CacheHLSPragmaHandler::PragmaKeyword = "cache";

class InterfaceHLSPragmaHandler : public HLSPragmaAnalyzer, public HLSPragmaParser
{
   ASTContext& _ASTContext;
   SourceManager& _SM;
   int _parseAction;

   const NamedDecl* getBaseTypeDecl(const QualType& qt) const
   {
      const clang::Type* ty = qt.getTypePtr();
      NamedDecl* ND = nullptr;

      if(ty->isPointerType() || ty->isReferenceType())
      {
         return getBaseTypeDecl(ty->getPointeeType());
      }
      if(isa<ElaboratedType>(ty))
      {
         return getBaseTypeDecl(ty->getAs<ElaboratedType>()->getNamedType());
      }
      if(isa<TypedefType>(ty) || ty->getTypeClass() == clang::Type::Typedef)
      {
         return getBaseTypeDecl(ty->getAs<TypedefType>()->getDecl()->getUnderlyingType());
      }
      if(ty->isRecordType())
      {
         ND = ty->getAs<RecordType>()->getDecl();
      }
      else if(ty->isEnumeralType())
      {
         ND = ty->getAs<EnumType>()->getDecl();
      }
      else if(ty->isArrayType())
      {
         return getBaseTypeDecl(ty->castAsArrayTypeUnsafe()->getElementType());
      }
      return ND;
   }

   QualType RemoveTypedef(QualType t) const
   {
      if(t->getTypeClass() == clang::Type::Typedef)
      {
         return RemoveTypedef(t->getAs<TypedefType>()->getDecl()->getUnderlyingType());
      }
      else if(t->getTypeClass() == clang::Type::Elaborated && !t->isClassType())
      {
         return RemoveTypedef(t->getAs<ElaboratedType>()->getNamedType());
      }
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

   std::string getIncludePaths(const QualType& type)
   {
      std::string includes;
      if(const auto BTD = getBaseTypeDecl(type))
      {
         const auto include_file = _SM.getPresumedLoc(BTD->getSourceRange().getBegin(), false).getFilename();
         includes = include_file;
      }
      const auto tmpl_decl = dyn_cast_or_null<ClassTemplateSpecializationDecl>(type->getAsTagDecl());
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
                  const auto include_file = _SM.getPresumedLoc(BTD->getSourceRange().getBegin(), false).getFilename();
                  includes += std::string(";") + include_file;
               }
            }
         }
      }
      return includes;
   }

   int64_t getSizeInBytes(QualType T)
   {
      if(T->isDependentType() || T->isIncompleteType() || T->isTemplateTypeParmType())
      {
         return 1;
      }
      if(isa<InjectedClassNameType>(T))
      {
         return getSizeInBytes(cast<InjectedClassNameType>(T)->getInjectedSpecializationType());
      }
      else if(isa<TemplateSpecializationType>(T))
      {
         const auto TST = cast<TemplateSpecializationType>(T);
         return TST->isTypeAlias() ? getSizeInBytes(TST->getAliasedType()) : 1;
      }
      return _ASTContext.getTypeInfoDataSizeInChars(T)
          .
#if __clang_major__ <= 11
          first
#else
          Width
#endif
          .getQuantity();
   }

   bool hasThisParameter(FunctionDecl* FD)
   {
      return FD->isCXXInstanceMember() && !isa<CXXConstructorDecl>(FD) && !isa<CXXDestructorDecl>(FD);
   }

   bool isUnsupportedInterface(FunctionDecl* FD)
   {
      return FD->isVariadic() ||
             llvm::any_of(FD->parameters(), [](const ParmVarDecl* D) { return D->isParameterPack(); });
   }

   std::string removeSpaces(std::string str)
   {
      // The default CentOS 7 system compiler is gcc 4.8.5, which ships an
      // incomplete version of <regex> in libstdc++. As such, the following
      // line of code will not compile with the system compiler or any other
      // compiler that relies on the default system libstdc++. When using
      // toolchains built with /opt/rh/devtoolset-9/ (gcc 9.3.1) or that can
      // provide their own c++ libs, this is no longer an issue.
#ifndef REWRITE_REGEX
      return std::regex_replace(str, std::regex(" ([<>&*])|([<>&*]) | $|^ "), "$1$2");
#else
      const char anchors[] = "[<>&*]";
      const char remove = ' ';

      size_t num_deleted = 0;
      for(int i = str.size() - 1; i > 0; i--)
      {
         char curr = str[i];
         char prev = str[i - 1];

         char curr_in_anchors = 0;
         char prev_in_anchors = 0;
         for(size_t j = 0; j < sizeof(anchors) - 1; j++)
         {
            curr_in_anchors |= (curr == anchors[j]);
            prev_in_anchors |= (prev == anchors[j]);
         }

         // "delete" the curr/prev char by shifting it to the end
         if(curr == remove && (i == str.size() - 1 || prev_in_anchors))
         {
            for(size_t k = i; k < str.size() - 1; k++)
            {
               str[k] = str[k + 1];
            }
            str[str.size() - 1] = curr;
            num_deleted++;
         }
         else if(prev == remove && (i == 1 || curr_in_anchors))
         {
            for(size_t k = i - 1; k < str.size() - 1; k++)
            {
               str[k] = str[k + 1];
            }
            str[str.size() - 1] = prev;
            num_deleted++;
         }
      }

      str.erase(str.end() - num_deleted, str.end());
      return str;
#endif
   }

   void AnalyzeParameterInterface(FunctionDecl* FD, const pragma_line_t& p)
   {
      if(isUnsupportedInterface(FD))
      {
         ReportError(p.loc, "HLS pragma not supported on variadic function declarations");
      }
      auto portName = p.attrs.find(key_loc_t("port", SourceLocation()));
      if(portName == p.attrs.end())
      {
         ReportError(p.loc, "Missing interface port attribute");
         return;
      }
      ParmVarDecl* parmDecl = nullptr;
      size_t parmIndex = hasThisParameter(FD) ? 1 : 0; // Account for parameter this in CXX instance function signature
      for(auto par : FD->parameters())
      {
         if((parmDecl = dyn_cast<ParmVarDecl>(par)))
         {
            const auto parName =
                parmDecl->getNameAsString().empty() ? ("P" + std::to_string(parmIndex)) : parmDecl->getNameAsString();
            if(parName == portName->second)
            {
               break;
            }
         }
         ++parmIndex;
      }
      if(!parmDecl)
      {
         ReportError(portName->first.loc, "Unkown port name");
         return;
      }
      LLVM_DEBUG(dbgs() << "PORT: " << portName->second << "\n");
      const auto ifaceModeReq = p.attrs.find(key_loc_t("mode", SourceLocation()));
      if(ifaceModeReq == p.attrs.end())
      {
         ReportError(p.loc, "Missing interface mode attribute");
         return;
      }
      const auto ifaceBundle = p.attrs.find(key_loc_t("bundle", SourceLocation())) != p.attrs.end() ?
                                   p.attrs.find(key_loc_t("bundle", SourceLocation()))->second :
                                   portName->second;
      auto& func_attrs = GetFuncAttr(FD);

      auto& parm_attrs = func_attrs.ifaces.parameters[portName->second];
      parm_attrs.insert(*portName);
      parm_attrs[key_loc_t("index", SourceLocation())] = std::to_string(parmIndex);
      parm_attrs[key_loc_t("bundle", SourceLocation())] = ifaceBundle;

      auto& bundle_attrs = func_attrs.ifaces.bundles[ifaceBundle];
      bundle_attrs[key_loc_t("name", SourceLocation())] = ifaceBundle;
      for(auto& attr : p.attrs)
      {
         if(iequals(attr.first.id, "register") || iequals(attr.first.id, "register_mode") ||
            iequals(attr.first.id, "num_write_outstanding") || iequals(attr.first.id, "depth"))
         {
            auto it_res = bundle_attrs.emplace(key_loc_t(to_lower(attr.first.id), attr.first.loc), attr.second);
            if(!it_res.second && it_res.first->second != attr.second)
            {
               ReportDuplicate(attr.first.loc, it_res.first->first.loc,
                               "Mismatching duplicate interface attribute definition");
            }
         }
         else if(iequals(attr.first.id, "offset") || iequals(attr.first.id, "elem_count"))
         {
            parm_attrs.emplace(key_loc_t(to_lower(attr.first.id), attr.first.loc), attr.second);
         }
      }

      const auto argType = parmDecl->getType();
      auto& ifaceMode = bundle_attrs.insert(*ifaceModeReq).first->second;
      auto& parmTypename = parm_attrs[key_loc_t("typename", SourceLocation())];
      auto& parmSizeInBytes = parm_attrs[key_loc_t("size_in_bytes", SourceLocation())];
      auto& parmOriginalTypename = parm_attrs[key_loc_t("original_typename", SourceLocation())];
      auto& parmIncludePaths = parm_attrs[key_loc_t("includes", SourceLocation())];

      const auto manageArray = [&](const ConstantArrayType* CA, bool setInterfaceType) {
         auto OrigTotArraySize = CA->getSize();
         std::string Dimensions;
         if(!setInterfaceType)
         {
#if __clang_major__ >= 13
            Dimensions = "[" + toString(OrigTotArraySize, 10, false) + "]";
#else
            Dimensions = "[" + OrigTotArraySize.toString(10, false) + "]";
#endif
         }
         while(CA->getElementType()->isConstantArrayType())
         {
            CA = cast<ConstantArrayType>(CA->getElementType());
            const auto n_el = CA->getSize();
#if __clang_major__ >= 13
            Dimensions = Dimensions + "[" + toString(n_el, 10, false) + "]";
#else
            Dimensions = Dimensions + "[" + n_el.toString(10, false) + "]";
#endif
            OrigTotArraySize *= n_el;
         }
         if(setInterfaceType)
         {
            ifaceMode = "array";
            auto& array_size = parm_attrs[key_loc_t("elem_count", SourceLocation())];
#if __clang_major__ >= 13
            array_size = toString(OrigTotArraySize, 10, false);
#else
            array_size = OrigTotArraySize.toString(10, false);
#endif
            assert(array_size != "0");
         }
         const auto paramTypeRemTD = RemoveTypedef(CA->getElementType());
         parmTypename = GetTypeNameCanonical(paramTypeRemTD, _PP) + " *";
         parmOriginalTypename =
             GetTypeNameCanonical(CA->getElementType(), _PP) + (Dimensions == "" ? " *" : " (*)" + Dimensions);
         parmIncludePaths = getIncludePaths(paramTypeRemTD);
      };

      if(isa<DecayedType>(argType))
      {
         const auto DT = cast<DecayedType>(argType)->getOriginalType().IgnoreParens();
         parmSizeInBytes = std::to_string(getSizeInBytes(DT));
         if(isa<ConstantArrayType>(DT))
         {
            manageArray(cast<ConstantArrayType>(DT), ifaceMode == "default");
         }
         else
         {
            const auto paramTypeRemTD = RemoveTypedef(argType);
            parmTypename = GetTypeNameCanonical(paramTypeRemTD, _PP);
            parmOriginalTypename = GetTypeNameCanonical(argType, _PP);
            parmIncludePaths = getIncludePaths(argType);
         }
         if(ifaceMode != "default")
         {
            if(ifaceMode != "handshake" && ifaceMode != "fifo" && ifaceMode != "array" && ifaceMode != "bus" &&
               ifaceMode != "m_axi" && ifaceMode != "axis")
            {
               ReportError(ifaceModeReq->first.loc, "Invalid HLS interface mode");
            }
         }
      }
      else if(argType->isPointerType() || argType->isReferenceType())
      {
         const auto ptdType = argType->getPointeeType().IgnoreParens();
         parmSizeInBytes = std::to_string(getSizeInBytes(ptdType));
         if(isa<ConstantArrayType>(ptdType))
         {
            manageArray(cast<ConstantArrayType>(ptdType), false);
         }
         else
         {
            const auto suffix = argType->isPointerType() ? "*" : "&";
            const auto paramTypeRemTD = RemoveTypedef(ptdType);
            parmTypename = GetTypeNameCanonical(paramTypeRemTD, _PP) + suffix;
            parmOriginalTypename = GetTypeNameCanonical(argType, _PP);
            parmIncludePaths = getIncludePaths(ptdType);
         }
         const auto is_channel_if = parmTypename.find("ac_channel<") == 0 || parmTypename.find("stream<") == 0 ||
                                    parmTypename.find("hls::stream<") == 0;
         if(ifaceMode != "default")
         {
            if((ifaceMode != "ptrdefault" && ifaceMode != "none" && ifaceMode != "handshake" && ifaceMode != "valid" &&
                ifaceMode != "ovalid" && ifaceMode != "acknowledge" && ifaceMode != "fifo" && ifaceMode != "bus" &&
                ifaceMode != "m_axi" && ifaceMode != "axis") ||
               (is_channel_if && ifaceMode != "fifo" && ifaceMode != "axis"))
            {
               ReportError(ifaceModeReq->first.loc, "Invalid HLS interface mode");
            }
         }
         else
         {
            ifaceMode = is_channel_if ? "fifo" : "ptrdefault";
         }
      }
      else
      {
         const auto paramTypeRemTD = RemoveTypedef(argType);
         parmSizeInBytes = std::to_string(getSizeInBytes(paramTypeRemTD));
         parmTypename = GetTypeNameCanonical(paramTypeRemTD, _PP);
         parmOriginalTypename = GetTypeNameCanonical(argType, _PP);
         parmIncludePaths = getIncludePaths(argType);
         if(ifaceMode != "default")
         {
            if(ifaceMode != "default" && ifaceMode != "none" && ifaceMode != "handshake" && ifaceMode != "valid" &&
               ifaceMode != "ovalid" && ifaceMode != "acknowledge")
            {
               ReportError(ifaceModeReq->first.loc, "Invalid HLS interface mode");
            }
            if((argType->isBuiltinType() || argType->isEnumeralType()) && ifaceMode == "none")
            {
               ifaceMode = "default";
            }
         }
         else if(!argType->isBuiltinType() && !argType->isEnumeralType())
         {
            ifaceMode = "none";
         }
      }

      if(ifaceMode == "array" && parm_attrs.find(key_loc_t("elem_count", SourceLocation())) == parm_attrs.end())
      {
         ReportError(ifaceModeReq->first.loc, "HLS interface array element count missing");
      }

      parmTypename = removeSpaces(parmTypename);
      parmOriginalTypename = removeSpaces(parmOriginalTypename);
      if(parmOriginalTypename == "size_t")
      {
         parmIncludePaths = "stddef.h";
      }
      parmIncludePaths = removeSpaces(parmIncludePaths);

      LLVM_DEBUG(dbgs() << " MODE: " << ifaceMode << "\n");
      LLVM_DEBUG(dbgs() << " TYPE: " << parmTypename << "\n");
      LLVM_DEBUG(dbgs() << " ORIG: " << parmOriginalTypename << "\n");
      LLVM_DEBUG(dbgs() << " INCL: " << parmIncludePaths << "\n");
   }

 public:
   InterfaceHLSPragmaHandler(ASTContext& ctx, PrintingPolicy& pp, int ParseAction,
                             std::map<std::string, func_attr_t>& func_attributes)
       : HLSPragmaAnalyzer(ctx, pp, func_attributes),
         HLSPragmaParser(),
         _ASTContext(ctx),
         _SM(ctx.getSourceManager()),
         _parseAction(ParseAction)
   {
   }

   ~InterfaceHLSPragmaHandler() = default;

   void operator()(FunctionDecl* FD, const pragma_line_t& p) override
   {
      if(_parseAction & ParseAction_Analyze)
      {
         AnalyzeParameterInterface(FD, p);
         GetFuncAttr(FD).attrs.emplace(key_loc_t("inline", p.loc), "off");
      }

      if(_parseAction & ParseAction_Optimize)
      {
         // Try to avoid inlining on interfaced functions
         if(!FD->hasAttr<NoInlineAttr>())
         {
            FD->addAttr(NoInlineAttr::CreateImplicit(FD->getASTContext()));
            FD->dropAttr<AlwaysInlineAttr>();
         }
      }
   }

   void finalize(FunctionDecl* FD) override
   {
      if(_parseAction & ParseAction_Analyze)
      {
         if(isUnsupportedInterface(FD))
         {
            LLVM_DEBUG(dbgs() << " Skip variadic arguments or unexpandend parameter pack tempalte declaration\n");
            return;
         }
         auto& func_ifaces = GetFuncAttr(FD).ifaces;
         size_t idx = 0;
         // Prepend this pointer parameter to CXX class member declarations
         if(func_ifaces.parameters.find(THIS_PARAM_NAME) == func_ifaces.parameters.end() && hasThisParameter(FD))
         {
            LLVM_DEBUG(dbgs() << "PORT: " THIS_PARAM_NAME "\n");
            assert(isa<CXXMethodDecl>(FD) && "Expected CXX method declaration");
            assert(func_ifaces.bundles.find(THIS_PARAM_NAME) == func_ifaces.bundles.end() &&
                   "Expected no bundle for 'this'");

            auto& this_param = func_ifaces.parameters[THIS_PARAM_NAME];
            auto& this_iface = func_ifaces.bundles[THIS_PARAM_NAME];
            this_param[key_loc_t("port", SourceLocation())] = THIS_PARAM_NAME;
            this_param[key_loc_t("bundle", SourceLocation())] = THIS_PARAM_NAME;
            this_param[key_loc_t("index", SourceLocation())] = "0";
            const auto thisType =
#if __clang_major__ >= 8
                dyn_cast<CXXMethodDecl>(FD)->getThisType();
#else
                dyn_cast<CXXMethodDecl>(FD)->getThisType(_ASTContext);
#endif
            const auto ptdType = thisType->getPointeeType().IgnoreParens();
            const auto paramTypeRemTD = RemoveTypedef(ptdType);
            this_param[key_loc_t("size_in_bytes", SourceLocation())] = std::to_string(getSizeInBytes(ptdType));
            this_param[key_loc_t("typename", SourceLocation())] = GetTypeNameCanonical(paramTypeRemTD, _PP) + "*";
            this_param[key_loc_t("original_typename", SourceLocation())] = GetTypeNameCanonical(thisType, _PP);
            this_param[key_loc_t("includes", SourceLocation())] = getIncludePaths(ptdType);

            this_iface[key_loc_t("name", SourceLocation())] = THIS_PARAM_NAME;
            this_iface[key_loc_t("mode", SourceLocation())] = "default";
            LLVM_DEBUG(dbgs() << " MODE: default\n");
            LLVM_DEBUG(dbgs() << " TYPE: " << this_param[key_loc_t("typename", SourceLocation())] << "\n");
            LLVM_DEBUG(dbgs() << " ORIG: " << this_param[key_loc_t("original_typename", SourceLocation())] << "\n");
            LLVM_DEBUG(dbgs() << " INCL: " << this_param[key_loc_t("includes", SourceLocation())] << "\n");
            ++idx;
         }
         for(auto par : FD->parameters())
         {
            ParmVarDecl* pdecl;
            if((pdecl = dyn_cast<ParmVarDecl>(par)))
            {
               const auto parName =
                   pdecl->getNameAsString().empty() ? ("P" + std::to_string(idx)) : pdecl->getNameAsString();

               if(func_ifaces.parameters.find(parName) == func_ifaces.parameters.end())
               {
                  pragma_line_t default_iface(nullptr, SourceLocation(), "interface", SourceLocation());
                  default_iface.attrs[key_loc_t("port", SourceLocation())] = parName;
                  default_iface.attrs[key_loc_t("mode", SourceLocation())] = "default";
                  AnalyzeParameterInterface(FD, default_iface);
               }
            }
            ++idx;
         }
      }
   }
   static const char* PragmaKeyword;
};
const char* InterfaceHLSPragmaHandler::PragmaKeyword = "interface";

class HLSASTConsumer : public ASTConsumer
{
   DiagnosticsEngine& _D;
   SourceManager& _SM;
   MangleContext* _MC;
   PrintingPolicy _PP;
   const std::string _archXML;
   const int _parseAction;

   std::list<pragma_line_t> _pragmas;

   std::map<std::string, func_attr_t> _func_attributes;

   HLSPragmaHandler::HLSPragmaParserSet _parsers;

   std::map<std::string, std::shared_ptr<HLSPragmaAnalyzer>> _analyzers;

   inline void Report(const SourceLocation& loc, DiagnosticsEngine::Level level, StringRef msg) const
   {
      ::Report(_D, loc, level, msg);
   }
   inline void ReportError(const SourceLocation& loc, StringRef msg) const
   {
      ::Report(_D, loc, DiagnosticsEngine::Error, msg);
   }

   inline std::string MangledName(NamedDecl* FD) const
   {
      return ::MangledName(_MC, FD);
   }

   inline func_attr_t& GetFuncAttr(FunctionDecl* FD)
   {
      return ::GetFuncAttr(_func_attributes, _MC, FD);
   }

   bool isBefore(const SourceLocation& loc_a, const SourceLocation& loc_b) const
   {
      if(_SM.getFilename(loc_a) == _SM.getFilename(loc_b))
      {
         return loc_a < loc_b;
      }
      return false;
   }

   void AnalyzePragma(FunctionDecl* FD, const pragma_line_t& p) const
   {
      auto analyzer_it = _analyzers.find(p.id);
      if(analyzer_it != _analyzers.end())
      {
         analyzer_it->second->operator()(FD, p);
      }
      else
      {
         Report(p.loc, DiagnosticsEngine::Level::Warning, "Unknown HLS pragma class");
      }
   }

   void AnalyzeFunctionDecl(FunctionDecl* FD)
   {
      const auto fd_end = FD->getSourceRange().getEnd();
      if(_SM.isInSystemHeader(fd_end) || FD->isImplicit() || FD->isInStdNamespace())
      {
         return;
      }
      LLVM_DEBUG({
         dbgs() << "Parse " << std::string(FD->Decl::getDeclKindName()) << ": " << FD->getDeclName().getAsString()
                << " - " << MangledName(FD) << "\n";
      });
      if(FD->isTemplateInstantiation())
      {
         const auto patternDecl = FD->getTemplateInstantiationPattern();
         assert(patternDecl && "Expected template pattern declaration");
         LLVM_DEBUG(dbgs() << "Import attributes from template pattern " << MangledName(patternDecl) << "\n");
         GetFuncAttr(FD) = GetFuncAttr(patternDecl);
      }
      while(_pragmas.size() && isBefore(_pragmas.front().loc, fd_end))
      {
         const pragma_line_t pragma_line = _pragmas.front();
         _pragmas.pop_front();
         LLVM_DEBUG({
            dbgs() << "  " << pragma_line.loc.printToString(_SM) << " " << pragma_line.id;
            for(auto& attr : pragma_line.attrs)
            {
               dbgs() << " " << attr.first.id << (attr.second.size() ? ("=" + attr.second) : "");
            }
            dbgs() << "\n";
         });
         AnalyzePragma(FD, pragma_line);
      }
      if(FD->hasBody())
      {
         LLVM_DEBUG(dbgs() << "Finalize " << MangledName(FD) << "\n");
         for(auto& pa : _analyzers)
         {
            pa.second->finalize(FD);
         }
      }
   }

   void UpdateXML(const std::string& filename)
   {
      pugi::xml_document doc;
      if(std::ifstream(filename.c_str()).good())
      {
         pugi::xml_document e;
         pugi::xml_node n;
         auto result = e.load_file(filename.c_str());
         if(!result)
         {
            ReportError(SourceLocation(), "Error parsing architecutre XML file: " + std::string(result.description()));
            return;
         }
         if((n = e.child("module")))
         {
            for(auto& f : n)
            {
               func_attr_t func;
               std::string funcSymbol = f.attribute("symbol").value();
               if(funcSymbol.empty())
               {
                  ReportError(SourceLocation(), "Invalid architecture XML file: '" + filename + "'");
                  continue;
               }
               f >> func;
               _func_attributes[funcSymbol] = func;
            }
         }
      }

      auto funcs = doc.append_child("module");
      for(auto& id_func : _func_attributes)
      {
         auto func = funcs.append_child("function");
         func << id_func.second;
      }

      doc.save_file(filename.c_str(), "  ", pugi::format_indent | pugi::format_no_empty_element_tags);
   }

 public:
   HLSASTConsumer(ASTContext& ctx, PrintingPolicy& pp, const std::string outdir, int _pa)
       : _D(ctx.getDiagnostics()),
         _SM(ctx.getSourceManager()),
         _MC(ctx.createMangleContext()),
         _PP(pp),
         _archXML(outdir + "/architecture.xml"),
         _parseAction(_pa)
   {
#define ADD_HANDLER(Name, ...)                                                                 \
   do                                                                                          \
   {                                                                                           \
      const auto _handler = std::make_shared<Name>(__VA_ARGS__);                               \
      _parsers[Name::PragmaKeyword] = std::static_pointer_cast<HLSPragmaParser>(_handler);     \
      _analyzers[Name::PragmaKeyword] = std::static_pointer_cast<HLSPragmaAnalyzer>(_handler); \
   } while(false)

      ADD_HANDLER(InterfaceHLSPragmaHandler, ctx, _PP, _pa, _func_attributes);
      ADD_HANDLER(DataflowHLSPragmaHandler, ctx, _PP, _func_attributes);

      if(_pa & ParseAction_Analyze)
      {
         ADD_HANDLER(PipelineHLSPragmaHandler, ctx, _PP, _func_attributes);
         ADD_HANDLER(CacheHLSPragmaHandler, ctx, _PP, _func_attributes);
      }

      if(_pa & ParseAction_Optimize)
      {
         ADD_HANDLER(InlineHLSPragmaHandler, ctx, _PP, _func_attributes);
         ADD_HANDLER(UnrollHLSPragmaHandler, ctx, _PP, _func_attributes);
      }

#undef ADD_HANDLER

      LLVM_DEBUG(dbgs() << "Supported pragmas:\n");
#ifndef NDEBUG
      for(auto& it : _analyzers)
      {
         LLVM_DEBUG(dbgs() << " - " << it.first << "\n");
      }
#endif
   }

   ~HLSASTConsumer()
   {
      delete _MC;
   }

   void addPragmaHandler(Preprocessor& PP)
   {
      PP.AddPragmaHandler(new HLSPragmaHandler("HLS", _parsers, _pragmas));
      PP.AddPragmaHandler(new HLSPragmaHandler("hls", _parsers, _pragmas));
      PP.AddPragmaHandler(new HLSPragmaHandler("bambu", _parsers, _pragmas));
   }

   bool HandleTopLevelDecl(DeclGroupRef DG) override
   {
      std::deque<Decl*> declQueue(DG.begin(), DG.end());
      while(declQueue.size())
      {
         auto decl = declQueue.front();
         declQueue.pop_front();
         if(auto FD = dyn_cast<FunctionDecl>(decl))
         {
            AnalyzeFunctionDecl(FD);
         }
         else if(isa<LinkageSpecDecl>(decl) || isa<NamespaceDecl>(decl))
         {
            auto declContext = cast<DeclContext>(decl);
            if(declContext->isStdNamespace())
               continue;
            declQueue.insert(declQueue.end(), declContext->decls_begin(), declContext->decls_end());
         }
      }
      return true;
   }

   void HandleInlineFunctionDefinition(FunctionDecl* FD) override
   {
      AnalyzeFunctionDecl(FD);
   }

   // void HandleTagDeclDefinition(TagDecl* FD) override
   // {
   //    for(auto d : FD->decls())
   //    {
   //       if(auto fd = dyn_cast<FunctionDecl>(d))
   //       {
   //          AnalyzeFunctionDecl(fd);
   //       }
   //    }
   // }

   // void HandleCXXImplicitFunctionInstantiation(FunctionDecl* FD) override
   // {
   //    AnalyzeFunctionDecl(FD);
   // }

   void HandleTranslationUnit(ASTContext&) override
   {
      while(_pragmas.size())
      {
         const pragma_line_t p = _pragmas.front();
         _pragmas.pop_front();
         ReportError(p.loc, "Unassociated HLS pragma");
      }

      if(_parseAction & ParseAction_Analyze)
      {
         UpdateXML(_archXML);
      }
   }
};

class EmptyASTConsumer : public ASTConsumer
{
 public:
   EmptyASTConsumer() = default;
};

namespace clang
{
   class CLANG_VERSION_SYMBOL(_plugin_ASTAnalyzer) : public PluginASTAction
   {
      int _action;
      std::string _outdir;
      bool _cppflag;

    protected:
      std::unique_ptr<ASTConsumer> CreateASTConsumer(CompilerInstance& CI, StringRef InFile) override
      {
         if(_action)
         {
            auto pp = PrintingPolicy(LangOptions());
            if(_cppflag)
            {
               pp.adjustForCPlusPlus();
            }
            auto pragmaConsumer = make_unique<HLSASTConsumer>(CI.getASTContext(), pp, _outdir, _action);
            pragmaConsumer->addPragmaHandler(CI.getPreprocessor());
            return pragmaConsumer;
         }
         return make_unique<EmptyASTConsumer>();
      }

      bool ParseArgs(const CompilerInstance& CI, const std::vector<std::string>& args) override
      {
         DiagnosticsEngine& D = CI.getDiagnostics();
         for(size_t i = 0, e = args.size(); i != e;)
         {
            auto& arg = args.at(i++);
            if(arg == "-action")
            {
               if(i >= e)
               {
                  D.Report(D.getCustomDiagID(DiagnosticsEngine::Error, "missing outputdir argument"));
                  return false;
               }
               const auto& action = args.at(i++);
               if(action == "analyze")
               {
                  _action |= ParseAction_Analyze;
               }
               else if(action == "optimize")
               {
                  _action |= ParseAction_Optimize;
               }
            }
            else if(arg == "-outputdir")
            {
               if(i >= e)
               {
                  D.Report(D.getCustomDiagID(DiagnosticsEngine::Error, "missing outputdir argument"));
                  return false;
               }
               _outdir = args.at(i++);
            }
            else if(arg == "-cppflag")
            {
               _cppflag = true;
            }
            else if(arg == "-help")
            {
               PrintHelp(errs());
               return true;
            }
         }

         if(_outdir.empty() && (_action & ParseAction_Analyze))
         {
            D.Report(D.getCustomDiagID(DiagnosticsEngine::Error, "Output directory not specified"));
         }
         return true;
      }

      void PrintHelp(raw_ostream& ros)
      {
         ros << "Help for " CLANG_VERSION_STRING(_plugin_ASTAnalyzer) " plugin:\n";
         ros << " -outputdir <directory>\n";
         ros << "   Directory where the architecture.xml file will be written\n";
         ros << " -cppflag\n";
         ros << "   Input source file is C++\n";
      }

      PluginASTAction::ActionType getActionType() override
      {
         return AddBeforeMainAction;
      }

    public:
      CLANG_VERSION_SYMBOL(_plugin_ASTAnalyzer)
      () : _action(ParseAction_None), _outdir(""), _cppflag(false)
      {
      }
      CLANG_VERSION_SYMBOL(_plugin_ASTAnalyzer)(const CLANG_VERSION_SYMBOL(_plugin_ASTAnalyzer) & step) = delete;
      CLANG_VERSION_SYMBOL(_plugin_ASTAnalyzer) & operator=(const CLANG_VERSION_SYMBOL(_plugin_ASTAnalyzer) &) = delete;
   };

#ifdef _WIN32

   void initializeplugin_ASTAnalyzer()
   {
      static FrontendPluginRegistry::Add<CLANG_VERSION_SYMBOL(_plugin_ASTAnalyzer)> X(
          CLANG_VERSION_STRING(_plugin_ASTAnalyzer), "Analyze Clang AST to retrieve information useful for PandA");
   }
#endif
} // namespace clang

#ifndef _WIN32
static FrontendPluginRegistry::Add<CLANG_VERSION_SYMBOL(_plugin_ASTAnalyzer)>
    X(CLANG_VERSION_STRING(_plugin_ASTAnalyzer), "Analyze Clang AST to retrieve information useful for PandA");
#endif
