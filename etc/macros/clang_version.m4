dnl
dnl check clang version 
dnl
AC_DEFUN([AC_CHECK_CLANG40_I386_VERSION],[
    AC_ARG_WITH(clang40,
    [  --with-clang40=executable-path path where the CLANG 4.0 is installed ],
    [
       ac_clang40="$withval"
    ])
dnl switch to c
AC_LANG_PUSH([C])

if test "x$ac_clang40" = x; then
   CLANG_TO_BE_CHECKED="/usr/bin/clang /usr/bin/clang-4.0"
else
   CLANG_TO_BE_CHECKED=$ac_clang40;
fi

echo "looking for clang 4.0..."
for compiler in $CLANG_TO_BE_CHECKED; do
   if test -f $compiler; then
      echo "checking $compiler..."
      dnl check for clang
      I386_CLANG40_VERSION=`$compiler --version | grep "4\.0\."`
      if test x"$I386_CLANG40_VERSION" = "x"; then
         I386_CLANG40_VERSION=""
      else
         I386_CLANG40_VERSION="4.0.0"
      fi

      AS_VERSION_COMPARE($1, [4.0.0], MIN_CLANG40=[4.0.0], MIN_CLANG40=$1, MIN_CLANG40=$1)
      AS_VERSION_COMPARE([5.0.0], $2, MAX_CLANG40=[5.0.0], MAX_CLANG40=$2, MAX_CLANG40=$2)
      AS_VERSION_COMPARE($I386_CLANG40_VERSION, $MIN_CLANG40, echo "checking $compiler >= $MIN_CLANG40... no"; min=no, echo "checking $compiler >= $MIN_CLANG40... yes"; min=yes, echo "checking $compiler >= $MIN_CLANG40... yes"; min=yes)
      if test "$min" = "no" ; then
         continue;
      fi
      AS_VERSION_COMPARE($I386_CLANG40_VERSION, $MAX_CLANG40, echo "checking $compiler < $MAX_CLANG40... yes"; max=yes, echo "checking $compiler < $MAX_CLANG40... no"; max=no, echo "checking $compiler < $MAX_CLANG40... no"; max=no)
      if test "$max" = "no" ; then
         continue;
      fi
      I386_CLANG40_EXE=$compiler;
      clang_file=`basename $I386_CLANG40_EXE`
      clang_dir=`dirname $I386_CLANG40_EXE`

      llvm_config=`echo $clang_file | sed s/clang/llvm-config/`
      I386_LLVM_CONFIG40_EXE=$clang_dir/$llvm_config
      I386_LLVM40_HEADER_DIR=`$I386_LLVM_CONFIG40_EXE --includedir`
      if test "x$I386_LLVM40_HEADER_DIR" = "x"; then
         echo "checking CLANG/LLVM plugin support... no. Package llvm-4.0 missing?"
         break;
      fi
      echo "checking plugin directory...$I386_LLVM40_HEADER_DIR"
      cpp=`echo $clang_file | sed s/clang/clang-cpp/`
      I386_CLANG_CPP40_EXE=$clang_dir/$cpp
      if test -f $I386_CLANG_CPP40_EXE; then
         echo "checking cpp...$I386_CLANG_CPP40_EXE"
      else
         echo "checking cpp...no"
         I386_CLANG40_EXE=""
         continue
      fi
      clangpp=`echo $clang_file | sed s/clang/clang\+\+/`
      I386_CLANGPP40_EXE=$clang_dir/$clangpp
      if test -f $I386_CLANGPP40_EXE; then
         echo "checking clang++...$I386_CLANGPP40_EXE"
      else
         echo "checking clang++...no"
         continue
      fi
      ac_save_CC="$CC"
      ac_save_CFLAGS="$CFLAGS"
      ac_save_LDFLAGS="$LDFLAGS"
      ac_save_LIBS="$LIBS"
      CC=$I386_CLANG40_EXE
      CFLAGS="-m32"
      LDFLAGS=
      LIBS=
      AC_LANG_PUSH([C])
      AC_LINK_IFELSE([AC_LANG_SOURCE([int main(void){ return 0;}])],I386_CLANG40_MULTIARCH=yes,I386_CLANG40_MULTIARCH=no)
      AC_LANG_POP([C])
      CC=$ac_save_CC
      CFLAGS=$ac_save_CFLAGS
      LDFLAGS=$ac_save_LDFLAGS
      LIBS=$ac_save_LIBS
      if test "x$I386_CLANG40_MULTIARCH" != xyes; then
         echo "checking support to -m32... no"
         continue
      else
         echo "checking support to -m32... yes"
      fi
      ac_save_CC="$CC"
      ac_save_CFLAGS="$CFLAGS"
      ac_save_LDFLAGS="$LDFLAGS"
      ac_save_LIBS="$LIBS"
      CC=$I386_CLANG40_EXE
      CFLAGS="-mx32"
      LDFLAGS=
      LIBS=
      AC_LANG_PUSH([C])
      AC_LINK_IFELSE([AC_LANG_SOURCE([int main(void){ return 0;}])],I386_CLANG40_MX32=yes,I386_CLANG40_MX32=no)
      AC_LANG_POP([C])
      CC=$ac_save_CC
      CFLAGS=$ac_save_CFLAGS
      LDFLAGS=$ac_save_LDFLAGS
      LIBS=$ac_save_LIBS
      if test "x$I386_CLANG40_MX32" == xyes; then
         AC_DEFINE(HAVE_I386_CLANG40_MX32,1,[Define if clang 4.0 supports -mx32 ])
         echo "checking support to -mx32... yes"
      else
         echo "checking support to -mx32... no"
      fi
      cat > plugin_test.cpp <<PLUGIN_TEST
//===- PrintFunctionNames.cpp ---------------------------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// Example clang plugin which simply prints the names of all the top-level decls
// in the input file.
//
//===----------------------------------------------------------------------===//

#include "clang/Frontend/FrontendPluginRegistry.h"
#include "clang/AST/AST.h"
#include "clang/AST/ASTConsumer.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Sema/Sema.h"
#include "llvm/Support/raw_ostream.h"
using namespace clang;

namespace {

class PrintFunctionsConsumer : public ASTConsumer {
  CompilerInstance &Instance;
  std::set<std::string> ParsedTemplates;

public:
  PrintFunctionsConsumer(CompilerInstance &Instance,
                         std::set<std::string> ParsedTemplates)
      : Instance(Instance), ParsedTemplates(ParsedTemplates) {}

  bool HandleTopLevelDecl(DeclGroupRef DG) override {
    for (DeclGroupRef::iterator i = DG.begin(), e = DG.end(); i != e; ++i) {
      const Decl *D = *i;
      if (const NamedDecl *ND = dyn_cast<NamedDecl>(D))
        llvm::errs() << "top-level-decl: \"" << ND->getNameAsString() << "\"\n";
    }

    return true;
  }

  void HandleTranslationUnit(ASTContext& context) override {
    if (!Instance.getLangOpts().DelayedTemplateParsing)
      return;

    // This demonstrates how to force instantiation of some templates in
    // -fdelayed-template-parsing mode. (Note: Doing this unconditionally for
    // all templates is similar to not using -fdelayed-template-parsig in the
    // first place.)
    // The advantage of doing this in HandleTranslationUnit() is that all
    // codegen (when using -add-plugin) is completely finished and this can't
    // affect the compiler output.
    struct Visitor : public RecursiveASTVisitor<Visitor> {
      const std::set<std::string> &ParsedTemplates;
      Visitor(const std::set<std::string> &ParsedTemplates)
          : ParsedTemplates(ParsedTemplates) {}
      bool VisitFunctionDecl(FunctionDecl *FD) {
        if (FD->isLateTemplateParsed() &&
            ParsedTemplates.count(FD->getNameAsString()))
          LateParsedDecls.insert(FD);
        return true;
      }

      std::set<FunctionDecl*> LateParsedDecls;
    } v(ParsedTemplates);
    v.TraverseDecl(context.getTranslationUnitDecl());
    clang::Sema &sema = Instance.getSema();
    for (const FunctionDecl *FD : v.LateParsedDecls) {
      clang::LateParsedTemplate &LPT =
          *sema.LateParsedTemplateMap.find(FD)->second;
      sema.LateTemplateParser(sema.OpaqueParser, LPT);
      llvm::errs() << "late-parsed-decl: \"" << FD->getNameAsString() << "\"\n";
    }   
  }
};

class PrintFunctionNamesAction : public PluginASTAction {
  std::set<std::string> ParsedTemplates;
protected:
  std::unique_ptr<ASTConsumer> CreateASTConsumer(CompilerInstance &CI,
                                                 llvm::StringRef) override {
    return llvm::make_unique<PrintFunctionsConsumer>(CI, ParsedTemplates);
  }

  bool ParseArgs(const CompilerInstance &CI,
                 const std::vector<std::string> &args) override {
    for (unsigned i = 0, e = args.size(); i != e; ++i) {
      llvm::errs() << "PrintFunctionNames arg = " << args.at(i) << "\n";

      // Example error handling.
      DiagnosticsEngine &D = CI.getDiagnostics();
      if (args.at(i) == "-an-error") {
        unsigned DiagID = D.getCustomDiagID(DiagnosticsEngine::Error,
                                            "invalid argument '%0'");
        D.Report(DiagID) << args.at(i);
        return false;
      } else if (args.at(i) == "-parse-template") {
        if (i + 1 >= e) {
          D.Report(D.getCustomDiagID(DiagnosticsEngine::Error,
                                     "missing -parse-template argument"));
          return false;
        }
        ++i;
        ParsedTemplates.insert(args.at(i));
      }
    }
    if (!args.empty() && args.at(0) == "help")
      PrintHelp(llvm::errs());

    return true;
  }
  void PrintHelp(llvm::raw_ostream& ros) {
    ros << "Help for PrintFunctionNames plugin goes here\n";
  }

  PluginASTAction::ActionType getActionType() override {
  return AddAfterMainAction;
  }
};

}

static FrontendPluginRegistry::Add<PrintFunctionNamesAction>
X("print-fns", "print function names");
PLUGIN_TEST
      for plugin_compiler in $I386_CLANGPP40_EXE; do
         if test -f plugin_test.so; then
            rm plugin_test.so
         fi
         $plugin_compiler -I$TOPSRCDIR/etc/clang_plugin/ -fPIC -shared plugin_test.cpp -o plugin_test.so -std=c++11 -I$I386_LLVM40_HEADER_DIR 2> /dev/null
         if test ! -f plugin_test.so; then
            echo "checking $plugin_compiler -I$TOPSRCDIR/etc/clang_plugin/ -fPIC -shared plugin_test.cpp -o plugin_test.so -std=c++11 -I$I386_LLVM40_HEADER_DIR... no... Package libclang-4.0-dev missing?"
            continue
         fi
         echo "checking $plugin_compiler -I$TOPSRCDIR/etc/clang_plugin/ -fPIC -shared plugin_test.cpp -o plugin_test.so -std=c++11 -I$I386_LLVM40_HEADER_DIR... yes"
         ac_save_CC="$CC"
         ac_save_CFLAGS="$CFLAGS"
         CC=$I386_CLANG40_EXE
         CFLAGS="-fplugin=$BUILDDIR/plugin_test.so -Xclang -add-plugin -Xclang print-fns"
         AC_LANG_PUSH([C])
         AC_COMPILE_IFELSE([AC_LANG_SOURCE([[
               ]],[[
                  return 0;
               ]])],
         I386_CLANG40_PLUGIN_COMPILER=$plugin_compiler,I386_CLANG40_PLUGIN_COMPILER=)
         AC_LANG_POP([C])
         CC=$ac_save_CC
         CFLAGS=$ac_save_CFLAGS
         #If plugin compilation fails, skip this executable
         if test "x$I386_CLANG40_PLUGIN_COMPILER" = x; then
            echo "plugin compilation does not work... $I386_CLANG40_EXE -fplugin=$BUILDDIR/plugin_test.so -Xclang -add-plugin -Xclang print-fns ?"
            continue
         fi
         echo "OK, we have found the compiler"
         build_I386_CLANG40=yes;
         build_I386_CLANG40_EMPTY_PLUGIN=no;
         build_I386_CLANG40_SSA_PLUGIN=yes;
         build_I386_CLANG40_SSA_PLUGINCPP=no;
         build_I386_CLANG40_TOPFNAME_PLUGIN=no;
      done
      if test "x$I386_CLANG40_PLUGIN_COMPILER" != x; then
         break;
      fi
   else
      echo "checking $compiler... not found"
   fi
done

if test x$I386_CLANG40_PLUGIN_COMPILER != x; then
  dnl set configure and makefile variables
  I386_CLANG40_EMPTY_PLUGIN=clang40_plugin_dumpGimpleEmpty
  I386_CLANG40_SSA_PLUGIN=clang40_plugin_dumpGimpleSSA
  I386_CLANG40_SSA_PLUGINCPP=clang40_plugin_dumpGimpleSSACpp
  I386_CLANG40_TOPFNAME_PLUGIN=clang40_plugin_topfname
  AC_SUBST(I386_CLANG40_EMPTY_PLUGIN)
  AC_SUBST(I386_CLANG40_SSA_PLUGIN)
  AC_SUBST(I386_CLANG40_SSA_PLUGINCPP)
  AC_SUBST(I386_CLANG40_TOPFNAME_PLUGIN)
  AC_SUBST(I386_LLVM40_HEADER_DIR)
  AC_SUBST(I386_CLANG40_EXE)
  AC_SUBST(I386_CLANG40_VERSION)
  AC_SUBST(I386_CLANG40_PLUGIN_COMPILER)
  AC_DEFINE(HAVE_I386_CLANG40_COMPILER, 1, "Define if CLANG 4.0 I386 compiler is compliant")
  AC_DEFINE_UNQUOTED(I386_CLANG40_EXE, "${I386_CLANG40_EXE}", "Define the plugin clang")
  AC_DEFINE_UNQUOTED(I386_CLANG_CPP40_EXE, "${I386_CLANG_CPP40_EXE}", "Define the plugin cpp")
  AC_DEFINE_UNQUOTED(I386_CLANGPP40_EXE, "${I386_CLANGPP40_EXE}", "Define the plugin clang++")
  AC_DEFINE_UNQUOTED(I386_CLANG40_EMPTY_PLUGIN, "${I386_CLANG40_EMPTY_PLUGIN}", "Define the filename of the CLANG PandA Empty plugin")
  AC_DEFINE_UNQUOTED(I386_CLANG40_SSA_PLUGIN, "${I386_CLANG40_SSA_PLUGIN}", "Define the filename of the CLANG PandA SSA plugin")
  AC_DEFINE_UNQUOTED(I386_CLANG40_SSA_PLUGINCPP, "${I386_CLANG40_SSA_PLUGINCPP}", "Define the filename of the CLANG PandA C++ SSA plugin")
  AC_DEFINE_UNQUOTED(I386_CLANG40_TOPFNAME_PLUGIN, "${I386_CLANG40_TOPFNAME_PLUGIN}", "Define the filename of the CLANG PandA topfname plugin")
  AC_DEFINE_UNQUOTED(I386_CLANG40_VERSION, "${I386_CLANG40_VERSION}", "Define the clang version")
  AC_DEFINE_UNQUOTED(I386_CLANG40_PLUGIN_COMPILER, "${I386_CLANG40_PLUGIN_COMPILER}", "Define the plugin compiler")
fi

dnl switch back to old language
AC_LANG_POP([C])

])





