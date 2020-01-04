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
 *              Copyright (C) 2019-2020 Politecnico di Milano
 *
 *   This file is part of the PandA framework.
 *
 *   The PandA framework is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 3 of the License, or
 *  (at your option) any later version.
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
 * @file bambuS2S.cpp
 * @brief clang tool used to port the original C/C++ code towards a better HLS compliance.
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 *
 */
#include "clang/ASTMatchers/ASTMatchFinder.h"
#include "clang/ASTMatchers/ASTMatchers.h"
#include "clang/Basic/SourceManager.h"
#include "clang/Frontend/FrontendActions.h"
#include "clang/Lex/Lexer.h"
#include "clang/Tooling/ArgumentsAdjusters.h"
#include "clang/Tooling/CommonOptionsParser.h"
#include "clang/Tooling/Refactoring.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/Signals.h"
#include "llvm/Support/TargetSelect.h"

#include <boost/tokenizer.hpp>
#include <set>

static llvm::cl::OptionCategory bambuSource2SourceCategory("bambu source to source options");

static llvm::cl::opt<std::string> top_fun_names("topf", llvm::cl::desc("top function names, separated by commas if needed. -topf=\"fun1,fun2,fun3\""), llvm::cl::value_desc("top-function-names"), llvm::cl::cat(bambuSource2SourceCategory));

/**
 * Matchers
 */

/// non void return expression matcher
auto return_expr_matcher(std::string const& fname)
{
   using namespace clang::ast_matchers;
   return returnStmt(hasAncestor(functionDecl(unless(isExpansionInSystemHeader()), hasName(fname), returns(unless(hasCanonicalType(asString("void"))))).bind("function"))).bind("return_expr");
}
/// function declaration returning non-void values matchers
auto function_decl_matcher(std::string const& fname)
{
   using namespace clang::ast_matchers;
   return functionDecl(unless(isExpansionInSystemHeader()), hasName(fname), returns(unless(hasCanonicalType(asString("void"))))).bind("function");
}
/// parameter of struct type matcher
auto struct_parm_decl_matcher(std::string const& fname)
{
   using namespace clang::ast_matchers;
   return parmVarDecl(hasAncestor(functionDecl(unless(isExpansionInSystemHeader()), hasName(fname))), hasType(hasCanonicalType(pointsTo(recordDecl(isStruct()).bind("record_type_decl"))))).bind("struct_parm_decl");
}

// Returns the text that makes up 'node' in the source.
// Returns an empty string if the text cannot be found.
template <typename T>
static std::string getText(const clang::SourceManager& SourceManager, const T& Node)
{
   clang::SourceLocation StartSpellingLocation = SourceManager.getSpellingLoc(Node.getLocStart());
   clang::SourceLocation EndSpellingLocation = SourceManager.getSpellingLoc(Node.getLocEnd());
   if(!StartSpellingLocation.isValid() || !EndSpellingLocation.isValid())
   {
      return std::string();
   }
   bool Invalid = true;
   const char* Text = SourceManager.getCharacterData(StartSpellingLocation, &Invalid);
   if(Invalid)
   {
      return std::string();
   }
   auto Start = SourceManager.getDecomposedLoc(StartSpellingLocation);
   auto End = SourceManager.getDecomposedLoc(clang::Lexer::getLocForEndOfToken(EndSpellingLocation, 0, SourceManager, clang::LangOptions()));
   if(Start.first != End.first)
   {
      // Start and end are in different files.
      return std::string();
   }
   if(End.second < Start.second)
   {
      // Shuffling text with macros may cause this.
      return std::string();
   }
   return std::string(Text, End.second - Start.second);
}

#define RES_DECL_PREFIX "__res_"

struct re_user : public clang::ast_matchers::MatchFinder::MatchCallback
{
 public:
   void run(const clang::ast_matchers::MatchFinder::MatchResult& result) override
   {
      using namespace clang;
      ASTContext& ctx(*(result.Context));
      ReturnStmt const* rs = result.Nodes.getNodeAs<ReturnStmt>("return_expr");
      FunctionDecl const* func = result.Nodes.getNodeAs<FunctionDecl>("function");
      assert(rs && func);
      llvm::errs() << "-- Replacement in function: " << func->getName() << " np=" << func->getNumParams() << "\n";
      auto rsLoc = rs->getLocStart();
      auto Loc = clang::Lexer::getLocForEndOfToken(rsLoc, 0, *result.SourceManager, ctx.getLangOpts());
      auto returnRange = clang::SourceRange(rsLoc, Loc);
      const CharSourceRange CRT_sourceRange(returnRange, true);
      std::string result_varNameAssignment = func->getName();
      result_varNameAssignment = std::string("*") + RES_DECL_PREFIX + result_varNameAssignment + " =";
      clang::tooling::Replacement Rep(*result.SourceManager, CRT_sourceRange, result_varNameAssignment, ctx.getLangOpts());
      auto errRes = FileToReplaces[result.SourceManager->getFilename(rsLoc)].add(Rep);
      if(errRes)
      {
         llvm_unreachable("something of wrong happen in replacement");
      }

      return;
   } // run

   //  explicit re_user(clang::tooling::Replacements *_replace) : replace(_replace) {}
   explicit re_user(std::map<std::string, clang::tooling::Replacements>& _FileToReplaces) : FileToReplaces(_FileToReplaces)
   {
   }

   std::map<std::string, clang::tooling::Replacements>& FileToReplaces;
};

struct fd_user : public clang::ast_matchers::MatchFinder::MatchCallback
{
   clang::Optional<clang::Token> findNextToken_local(clang::SourceLocation Loc, const clang::SourceManager& SM, const clang::LangOptions& LangOpts)
   {
      if(Loc.isMacroID())
      {
         if(!clang::Lexer::isAtEndOfMacroExpansion(Loc, SM, LangOpts, &Loc))
            return clang::None;
      }
      Loc = clang::Lexer::getLocForEndOfToken(Loc, 0, SM, LangOpts);

      // Break down the source location.
      std::pair<clang::FileID, unsigned> LocInfo = SM.getDecomposedLoc(Loc);

      // Try to load the file buffer.
      bool InvalidTemp = false;
      llvm::StringRef File = SM.getBufferData(LocInfo.first, &InvalidTemp);
      if(InvalidTemp)
         return clang::None;

      const char* TokenBegin = File.data() + LocInfo.second;

      // Lex from the start of the given location.
      clang::Lexer lexer(SM.getLocForStartOfFile(LocInfo.first), LangOpts, File.begin(), TokenBegin, File.end());
      // Find the token.
      clang::Token Tok;
      lexer.LexFromRawLexer(Tok);
      return Tok;
   }

 public:
   void run(const clang::ast_matchers::MatchFinder::MatchResult& result) override
   {
      using namespace clang;
      ASTContext& ctx(*(result.Context));
      FunctionDecl const* func = result.Nodes.getNodeAs<FunctionDecl>("function");
      assert(func);
      llvm::errs() << "-- Replacement in function: " << func->getName() << " np=" << func->getNumParams() << "\n";
      auto RT_sourceRange = func->getReturnTypeSourceRange();
      const CharSourceRange CRT_sourceRange(RT_sourceRange, true);
      clang::tooling::Replacement RepRT(*result.SourceManager, CRT_sourceRange, "void", ctx.getLangOpts());
      auto errRes0 = FileToReplaces[result.SourceManager->getFilename(RT_sourceRange.getBegin())].add(RepRT);
      if(errRes0)
      {
         llvm_unreachable("something of wrong happen in replacement");
      }

      std::string result_varNameDecl = func->getName();
      result_varNameDecl = func->getReturnType().getAsString() + "*" + RES_DECL_PREFIX + result_varNameDecl;

      if(func->getNumParams() == 0)
      {
         auto locFD = func->getLocation();
         Optional<Token> Tok = findNextToken_local(locFD, *result.SourceManager, ctx.getLangOpts());

         if(!(Tok.hasValue() && Tok.getValue().is(tok::l_paren)))
            llvm_unreachable("something of wrong happen in AST analysis");
         Tok = findNextToken_local(Tok.getValue().getLocation(), *result.SourceManager, ctx.getLangOpts());
         if(Tok.hasValue() && (Tok.getValue().is(tok::kw_void) || (Tok.getValue().is(tok::raw_identifier))))
         {
            auto LocAfterEnd = Tok.getValue().getLocation();
            clang::tooling::Replacement RepVoid(*result.SourceManager, LocAfterEnd, Tok.getValue().getLength(), result_varNameDecl);
            auto errRes = FileToReplaces[result.SourceManager->getFilename(LocAfterEnd)].add(RepVoid);
            if(errRes)
            {
               llvm_unreachable("something of wrong happen in replacement");
            }
         }
         else if(Tok.hasValue() && Tok.getValue().is(tok::r_paren))
         {
            auto LocAfterEnd = Tok.getValue().getLocation();
            clang::tooling::Replacement RepVoid(*result.SourceManager, LocAfterEnd, 0, result_varNameDecl);
            auto errRes = FileToReplaces[result.SourceManager->getFilename(LocAfterEnd)].add(RepVoid);
            if(errRes)
            {
               llvm_unreachable("something of wrong happen in replacement");
            }
         }
         else
         {
            llvm::errs() << Tok.getValue().getName() << "\n";
            llvm_unreachable("unexpected condition0");
         }
      }
      else
      {
         auto last = *(func->param_begin());
         for(auto par : func->parameters())
         {
            if(par->hasDefaultArg())
               break;
            else
               last = par;
         }
         auto LocAfterEnd = clang::Lexer::getLocForEndOfToken(last->getLocEnd(), 0, *result.SourceManager, ctx.getLangOpts());
         clang::tooling::Replacement RepLast(*result.SourceManager, LocAfterEnd, 0, ", " + result_varNameDecl);
         auto errRes = FileToReplaces[result.SourceManager->getFilename(LocAfterEnd)].add(RepLast);
         if(errRes)
         {
            llvm_unreachable("something of wrong happen in replacement");
         }
      }
      return;
   } // run

   //  explicit re_user(clang::tooling::Replacements *_replace) : replace(_replace) {}
   explicit fd_user(std::map<std::string, clang::tooling::Replacements>& _FileToReplaces) : FileToReplaces(_FileToReplaces)
   {
   }

   std::map<std::string, clang::tooling::Replacements>& FileToReplaces;
};

/**
 * @brief toVoidRefactor refactor the code such that void is the return type.
 * As example the following function
 * int test1(int *a )
 * {
 *   return 10;
 * }
 * becomes
 * void test1(int *a, int*__res_test1 )
 * {
 *   *__res_test1 = 10;
 * }
 * @return 0 upon success. Non-zero upon failure.
 */
int toVoidRefactor(clang::tooling::CommonOptionsParser& op, const boost::tokenizer<boost::char_separator<char>>& topFunTokenizer)
{
   clang::tooling::RefactoringTool Tool(op.getCompilations(), op.getSourcePathList());
   Tool.clearArgumentsAdjusters();
   Tool.appendArgumentsAdjuster(clang::tooling::getClangStripOutputAdjuster());

   clang::ast_matchers::MatchFinder finder;
   re_user rs_finder(Tool.getReplacements());
   fd_user fd_finder(Tool.getReplacements());
   for(auto fun : topFunTokenizer)
   {
      finder.addMatcher(return_expr_matcher(fun), &rs_finder);
      finder.addMatcher(function_decl_matcher(fun), &fd_finder);
   }
   return Tool.runAndSave(clang::tooling::newFrontendActionFactory(&finder).get());
}

int main(int argc, const char** argv)
{
   llvm::sys::PrintStackTraceOnErrorSignal(argv[0]);

   // Initialize targets for clang module support.
   llvm::InitializeAllTargets();
   llvm::InitializeAllTargetMCs();
   llvm::InitializeAllAsmPrinters();
   llvm::InitializeAllAsmParsers();

   // parse the command-line args passed to your code
   clang::tooling::CommonOptionsParser op(argc, argv, bambuSource2SourceCategory);
   boost::char_separator<char> sep(",");
   boost::tokenizer<boost::char_separator<char>> topFunTokenizer(top_fun_names, sep);
   auto VR = toVoidRefactor(op, topFunTokenizer);
   if(VR)
      return VR;
   return 0;
}
