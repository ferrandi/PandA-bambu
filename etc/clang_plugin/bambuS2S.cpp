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
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"
#include "clang/ASTMatchers/ASTMatchers.h"
#include "clang/Basic/SourceManager.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Frontend/FrontendActions.h"
#include "clang/Lex/Lexer.h"
#include "clang/Lex/PPCallbacks.h"
#include "clang/Lex/Preprocessor.h"
#include "clang/Rewrite/Core/Rewriter.h"
#include "clang/Rewrite/Frontend/Rewriters.h"
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

/// non void return expression matches
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
auto loop_matcher0()
{
   using namespace clang::ast_matchers;
   return forStmt(hasBody(compoundStmt().bind("loopbody0"))).bind("loop0");
}
auto loop_matcher1()
{
   using namespace clang::ast_matchers;
   return whileStmt(hasBody(compoundStmt().bind("loopbody1"))).bind("loop1");
}
auto loop_matcher2()
{
   using namespace clang::ast_matchers;
   return cxxForRangeStmt(hasBody(compoundStmt().bind("loopbody2"))).bind("loop2");
}

/// parameter of struct type matches
auto struct_parm_decl_matcher(std::string const& fname)
{
   using namespace clang::ast_matchers;
   return parmVarDecl(hasAncestor(functionDecl(unless(isExpansionInSystemHeader()), hasName(fname))), hasType(hasCanonicalType(pointsTo(recordDecl(isStruct()).bind("record_type_decl"))))).bind("struct_parm_decl");
}

// Returns the text that makes up 'node' in the source.
// Returns an empty string if the text cannot be found.
template <typename T>
static std::string getText(const clang::SourceManager& SourceManager, const clang::LangOptions& LangOpts, const T& Node)
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
   auto End = SourceManager.getDecomposedLoc(clang::Lexer::getLocForEndOfToken(EndSpellingLocation, 0, SourceManager, LangOpts));
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

// Specialization of getText returning the text associated with the token starting at given source location
template <>
std::string getText(const clang::SourceManager& SourceManager, const clang::LangOptions& LangOpts, const clang::SourceLocation& Loc)
{
   if(!Loc.isValid())
   {
      return std::string();
   }
   bool Invalid = true;
   const char* Text = SourceManager.getCharacterData(Loc, &Invalid);
   if(Invalid)
   {
      return std::string();
   }
   auto Start = SourceManager.getDecomposedLoc(Loc);
   auto End = SourceManager.getDecomposedLoc(clang::Lexer::getLocForEndOfToken(Loc, 0, SourceManager, LangOpts));
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

static clang::Optional<clang::Token> findNextToken_local(clang::SourceLocation Loc, const clang::SourceManager& SM, const clang::LangOptions& LangOpts)
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
#if __clang_major__ > 7
      auto rsLoc = rs->getBeginLoc();
#else
      auto rsLoc = rs->getLocStart();
#endif
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
#if __clang_major__ > 7
         auto loc_end = last->getEndLoc();
#else
         auto loc_end = last->getLocEnd();
#endif
         auto LocAfterEnd = clang::Lexer::getLocForEndOfToken(loc_end, 0, *result.SourceManager, ctx.getLangOpts());
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

struct pragmaInfo
{
   const enum pragmaKind { pipeline } Kind;
   /// in case a loop pipeline the value field contains the initiation interval
   const std::string value;
   pragmaInfo(enum pragmaKind k, const std::string v) : Kind(k), value(v)
   {
   }
};

class pragmaCallBack : public clang::PPCallbacks
{
 public:
   explicit pragmaCallBack(std::map<clang::SourceLocation, std::list<pragmaInfo>>& _pragmaAnnotations, const clang::SourceManager& _SM, const clang::LangOptions& _langOpts) : SM(_SM), langOpts(_langOpts), pragmaAnnotations(_pragmaAnnotations)
   {
   }
   void PragmaDirective(clang::SourceLocation Loc, clang::PragmaIntroducerKind Introducer) override
   {
      using namespace clang;
      if(!Loc.isValid())
      {
         return;
      }
      Optional<Token> Tok = findNextToken_local(Loc, SM, langOpts);
      if(Tok.hasValue() && Tok.getValue().is(tok::raw_identifier))
      {
         Tok = findNextToken_local(Tok.getValue().getLocation(), SM, langOpts);
         if(Tok.hasValue() && Tok.getValue().is(tok::raw_identifier))
         {
            llvm::errs() << Tok.getValue().getRawIdentifier() << "\n";
            auto hls_id = Tok.getValue().getRawIdentifier();
            if(hls_id == std::string("HLS"))
            {
               Tok = findNextToken_local(Tok.getValue().getLocation(), SM, langOpts);
               if(Tok.hasValue() && Tok.getValue().is(tok::raw_identifier))
               {
                  llvm::errs() << Tok.getValue().getRawIdentifier() << "\n";
                  auto pipeline_id = Tok.getValue().getRawIdentifier();
                  if(pipeline_id == std::string("pipeline"))
                  {
                     auto II = std::string("1");
                     Tok = findNextToken_local(Tok.getValue().getLocation(), SM, langOpts);
                     if(Tok.hasValue() && Tok.getValue().is(tok::raw_identifier))
                     {
                        llvm::errs() << Tok.getValue().getRawIdentifier() << "\n";
                        auto II_id = Tok.getValue().getRawIdentifier();
                        if(II_id == std::string("II"))
                        {
                           Tok = findNextToken_local(Tok.getValue().getLocation(), SM, langOpts);
                           if(Tok.hasValue() && Tok.getValue().is(tok::equal))
                           {
                              Tok = findNextToken_local(Tok.getValue().getLocation(), SM, langOpts);
                              if(Tok.hasValue() && Tok.getValue().is(tok::numeric_constant))
                              {
                                 II = getText(SM, langOpts, Tok.getValue().getLocation());
                                 llvm::errs() << "\"" << II << "\""
                                              << "\n";
                                 Tok = findNextToken_local(Tok.getValue().getLocation(), SM, langOpts);
                                 llvm::errs() << Tok.getValue().getRawIdentifier() << "\n";
                              }
                           }
                        }
                     }
                     pragmaAnnotations[Tok.getValue().getLocation()].push_back(pragmaInfo(pragmaInfo::pipeline, II));
                  }
               }
            }
         }
      }
   }

 private:
   const clang::SourceManager& SM;
   const clang::LangOptions& langOpts;
   std::map<clang::SourceLocation, std::list<pragmaInfo>>& pragmaAnnotations;
};

struct pragmaManipulatorCallBack : public clang::tooling::SourceFileCallbacks
{
   explicit pragmaManipulatorCallBack(std::map<clang::SourceLocation, std::list<pragmaInfo>>& _pragmaAnnotations) : pragmaAnnotations(_pragmaAnnotations)
   {
   }

   bool handleBeginSource(clang::CompilerInstance& CI
#if __clang_major__ == 4
                          ,
                          llvm::StringRef Filename
#endif
                          ) override
   {
      clang::Preprocessor* PP = &CI.getPreprocessor();
      llvm::errs() << "add callback"
                   << "\n";
      PP->addPPCallbacks(std::make_unique<pragmaCallBack>(pragmaAnnotations, PP->getSourceManager(), CI.getLangOpts()));
      return true;
   }
   void handleEndSource() override
   {
      llvm::errs() << "end source"
                   << "\n";
   }

 private:
   std::map<clang::SourceLocation, std::list<pragmaInfo>>& pragmaAnnotations;
};

struct Loop_user0 : public clang::ast_matchers::MatchFinder::MatchCallback
{
 public:
   void run(const clang::ast_matchers::MatchFinder::MatchResult& result) override
   {
      llvm::errs() << "run matches 0\n";
      return;
   } // run

   //  explicit re_user(clang::tooling::Replacements *_replace) : replace(_replace) {}
   explicit Loop_user0(std::map<std::string, clang::tooling::Replacements>& _FileToReplaces) : FileToReplaces(_FileToReplaces)
   {
   }

   std::map<std::string, clang::tooling::Replacements>& FileToReplaces;
};
struct Loop_user1 : public clang::ast_matchers::MatchFinder::MatchCallback
{
 public:
   void run(const clang::ast_matchers::MatchFinder::MatchResult& result) override
   {
      llvm::errs() << "run matches 1\n";
      return;
   } // run

   //  explicit re_user(clang::tooling::Replacements *_replace) : replace(_replace) {}
   explicit Loop_user1(std::map<std::string, clang::tooling::Replacements>& _FileToReplaces) : FileToReplaces(_FileToReplaces)
   {
   }

   std::map<std::string, clang::tooling::Replacements>& FileToReplaces;
};
struct Loop_user2 : public clang::ast_matchers::MatchFinder::MatchCallback
{
 public:
   void run(const clang::ast_matchers::MatchFinder::MatchResult& result) override
   {
      llvm::errs() << "run matches 2\n";
      return;
   } // run

   //  explicit re_user(clang::tooling::Replacements *_replace) : replace(_replace) {}
   explicit Loop_user2(std::map<std::string, clang::tooling::Replacements>& _FileToReplaces) : FileToReplaces(_FileToReplaces)
   {
   }

   std::map<std::string, clang::tooling::Replacements>& FileToReplaces;
};

int pragmaManipulator(clang::tooling::CommonOptionsParser& op)
{
   clang::tooling::RefactoringTool Tool(op.getCompilations(), op.getSourcePathList());
   clang::ast_matchers::MatchFinder finder;
   Loop_user0 fl_finder(Tool.getReplacements());
   Loop_user1 wl_finder(Tool.getReplacements());
   Loop_user2 cxxl_finder(Tool.getReplacements());
   finder.addMatcher(loop_matcher0(), &fl_finder);
   finder.addMatcher(loop_matcher1(), &wl_finder);
   finder.addMatcher(loop_matcher2(), &cxxl_finder);
   std::map<clang::SourceLocation, std::list<pragmaInfo>> pragmaAnnotations;
   pragmaManipulatorCallBack PPMCB(pragmaAnnotations);
   return Tool.run(clang::tooling::newFrontendActionFactory(&finder, &PPMCB).get());
}

class OMPMASTVisitor : public clang::RecursiveASTVisitor<OMPMASTVisitor>
{
 public:
   explicit OMPMASTVisitor(clang::Rewriter& R) : TheRewriter(R)
   {
   }
   bool VisitStmt(clang::Stmt* s)
   {
      // Only care about If statements.
      if(clang::isa<clang::ForStmt>(s))
      {
         for(const clang::Stmt* Child : s->children())
         {
            if(Child)
               Child->dump(llvm::errs());
            llvm::errs() << "\n-----------------\n";
         }
         auto forStatement = llvm::cast<clang::ForStmt>(s);

         TheRewriter.InsertText(forStatement->getForLoc(), "\n", false, true);
         TheRewriter.InsertText(forStatement->getForLoc(), "\n// 'for' loop", false, false);

         auto rparent = forStatement->getRParenLoc();
         clang::Optional<clang::Token> Tok = findNextToken_local(rparent, TheRewriter.getSourceMgr(), TheRewriter.getLangOpts());
         bool isCompoundStmt = false;
         if(Tok.hasValue() && Tok.getValue().is(clang::tok::l_brace))
         {
            isCompoundStmt = true;
            TheRewriter.InsertTextAfterToken(Tok.getValue().getLocation(), "\n// 'for' loop body begin\n");
         }
         else
            TheRewriter.InsertTextAfterToken(rparent, "\n// 'for' loop body begin\n");

#if __clang_major__ > 7
         auto end = forStatement->getEndLoc();
#else
         auto end = forStatement->getLocEnd();
#endif
         if(!isCompoundStmt)
         {
            clang::Optional<clang::Token> afterSemiTok = findNextToken_local(end, TheRewriter.getSourceMgr(), TheRewriter.getLangOpts());
            if(Tok.hasValue())
               end = afterSemiTok->getLocation();
            else
               llvm_unreachable("condition not expected");
            TheRewriter.InsertTextAfterToken(end, "\n// 'for' loop body end");
         }
         else
            TheRewriter.InsertText(end, "\n// 'for' loop body end\n");
         TheRewriter.InsertTextAfterToken(end, "\n// 'for' loop end\n");
      }

      return true;
   }

 private:
   clang::Rewriter& TheRewriter;
};

class OMPMASTConsumer : public clang::ASTConsumer
{
 public:
   explicit OMPMASTConsumer(clang::Rewriter& R) : Visitor(R)
   {
   }

   // Override the method that gets called for each parsed top-level
   // declaration.
   virtual bool HandleTopLevelDecl(clang::DeclGroupRef DR)
   {
      for(clang::DeclGroupRef::iterator b = DR.begin(), e = DR.end(); b != e; ++b)
         // Traverse the declaration using our AST visitor.
         Visitor.TraverseDecl(*b);
      return true;
   }

 private:
   OMPMASTVisitor Visitor;
};

inline std::unique_ptr<clang::tooling::FrontendActionFactory> localnewFrontendActionFactory(clang::tooling::SourceFileCallbacks* Callbacks)
{
   class FrontendActionFactoryAdapter : public clang::tooling::FrontendActionFactory
   {
    public:
      explicit FrontendActionFactoryAdapter(clang::tooling::SourceFileCallbacks* _Callbacks) : Callbacks(_Callbacks)
      {
      }

#if __clang_major__ > 9
      std::unique_ptr<clang::FrontendAction> create() override
      {
         return std::make_unique<OMPMFactoryAdaptor>(Callbacks);
      }
#else
      clang::FrontendAction* create() override
      {
         return new OMPMFactoryAdaptor(Callbacks);
      }
#endif

    private:
      class OMPMFactoryAdaptor : public clang::ASTFrontendAction
      {
       public:
         explicit OMPMFactoryAdaptor(clang::tooling::SourceFileCallbacks* _Callbacks) : Callbacks(_Callbacks)
         {
         }

         // Create the AST consumer object for this action
         // CI - The current compiler instance
         // file - The current input file
         std::unique_ptr<clang::ASTConsumer> CreateASTConsumer(clang::CompilerInstance& CI, clang::StringRef file) override
         {
            TheRewriter.setSourceMgr(CI.getSourceManager(), CI.getLangOpts());
            return std::make_unique<OMPMASTConsumer>(TheRewriter);
         }

       protected:
         bool BeginSourceFileAction(clang::CompilerInstance& CI
#if __clang_major__ == 4
                                    ,
                                    llvm::StringRef Filename
#endif
                                    ) override
         {
            if(!ASTFrontendAction::BeginSourceFileAction(CI
#if __clang_major__ == 4
                                                         ,
                                                         Filename
#endif
                                                         ))
               return false;
            if(Callbacks)
               return Callbacks->handleBeginSource(CI
#if __clang_major__ == 4
                                                   ,
                                                   Filename
#endif
               );
            return true;
         }

         void EndSourceFileAction() override
         {
            if(Callbacks)
               Callbacks->handleEndSource();
            ASTFrontendAction::EndSourceFileAction();
            TheRewriter.getEditBuffer(TheRewriter.getSourceMgr().getMainFileID()).write(llvm::outs());
         }

       private:
         clang::tooling::SourceFileCallbacks* Callbacks;
         clang::Rewriter TheRewriter;
      };
      clang::tooling::SourceFileCallbacks* Callbacks;
   };

   return std::unique_ptr<clang::tooling::FrontendActionFactory>(new FrontendActionFactoryAdapter(Callbacks));
}

int OmpPragmaManipulator(clang::tooling::CommonOptionsParser& op)
{
   clang::tooling::RefactoringTool Tool(op.getCompilations(), op.getSourcePathList());
   clang::ast_matchers::MatchFinder finder;
   std::map<clang::SourceLocation, std::list<pragmaInfo>> pragmaAnnotations;
   pragmaManipulatorCallBack PPMCB(pragmaAnnotations);
   return Tool.run(localnewFrontendActionFactory(&PPMCB).get());
}

int main(int argc, const char** argv)
{
   llvm::sys::PrintStackTraceOnErrorSignal(argv[0]);

   // Initialize targets for clang module support.
   llvm::InitializeAllTargets();
   llvm::InitializeAllTargetMCs();
   llvm::InitializeAllAsmPrinters();
   llvm::InitializeAllAsmParsers();
   std::string headers = CLANG_SYSTEM_H;
   boost::char_separator<char> space_sep(" ");
   boost::tokenizer<boost::char_separator<char>> headersTokenizer(headers, space_sep);
   std::vector<std::string> headers_vec;
   for(auto h : headersTokenizer)
   {
      headers_vec.push_back("-isystem");
      headers_vec.push_back(h);
   }
   int new_argc = argc + headers_vec.size();
   auto new_argv = new const char*[new_argc + 1];
   new_argv[new_argc] = nullptr;
   for(int ii = 0; ii < argc; ++ii)
      new_argv[ii] = argv[ii];
   for(int ii = argc; ii < new_argc; ++ii)
      new_argv[ii] = headers_vec.at(ii - argc).c_str();

   // parse the command-line args passed to your code
   clang::tooling::CommonOptionsParser op(new_argc, new_argv, bambuSource2SourceCategory);
   boost::char_separator<char> sep(",");
   boost::tokenizer<boost::char_separator<char>> topFunTokenizer(top_fun_names, sep);
   auto VR = toVoidRefactor(op, topFunTokenizer);
   if(VR)
      return VR;
   //   auto PM = pragmaManipulator(op);
   //   if(PM)
   //      return PM;
   //   auto OPM = OmpPragmaManipulator(op);
   //   if(OPM)
   //      return OPM;
   return 0;
}
