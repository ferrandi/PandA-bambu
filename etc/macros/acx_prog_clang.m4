dnl
dnl check clang with plugin support enabled and plugins
dnl ARGS:
dnl   clang_exe
dnl   gmin_ver
dnl   gmax_ver
dnl   extra_cxxflags
dnl
AC_DEFUN([ACX_PROG_CLANG],[
AC_REQUIRE([AC_PROG_AWK])
AC_REQUIRE([AC_PROG_GREP])
AC_REQUIRE([AC_PROG_SED])
m4_define([clang_id], m4_esyscmd([echo -n $1 | sed -e 's/-//;s/\.[0-9]//']))
m4_define([clang_macro_prefix], m4_toupper(I386_[]clang_id))
m4_define([llvm_macro_prefix], m4_bpatsubsts(clang_macro_prefix,[CLANG],[LLVM]))
m4_define([req_version], m4_esyscmd([echo -n $1 | tr -d "clang-" | awk -F. '{printf $(1)"."($(2)?$(2):0)"."($(3)?$(3):0)}']))
m4_define([max_version], m4_esyscmd([echo -n $1 | tr -d "clang-" | awk -F. '{printf $(1)"."($(2)?$(2):0)"."($(3)?$(3):0)}' | awk -F. '{printf $(1)+1"."0"."0;}']))
m4_define([req_version_pretty], m4_esyscmd([echo -n $1 | tr -d "clang-"]))
AC_ARG_WITH(clang_id,
   [AS_HELP_STRING([--with-]clang_id, [absolute path for the Clang ]req_version_pretty[ executable])],
   [exe_to_check="$withval"],
   [exe_to_check="$1 clang"])
AC_LANG_PUSH([C])

echo "looking for clang []req_version_pretty[]..."
for compiler in $exe_to_check; do
   if command -v "$compiler" > /dev/null; then
      echo "checking $compiler..."
      if test `$compiler --version |grep clang|grep -v InstalledDir|awk -F' ' '{print $[1]}'` = "clang"; then
         CLANG_VERSION=`$compiler --version |grep clang|grep -v InstalledDir|awk -F' ' '{print $[3]}'| awk -F'-' '{print $[1]}'`
      else
         CLANG_VERSION=`$compiler --version |grep clang|grep -v InstalledDir|awk -F' ' '{print $[4]}'| awk -F'-' '{print $[1]}'`
      fi
      AS_VERSION_COMPARE($2, [req_version], MIN_CLANG=[req_version], MIN_CLANG=$2, MIN_CLANG=$2)
      AS_VERSION_COMPARE([max_version], $3, MAX_CLANG=[max_version], MAX_CLANG=$3, MAX_CLANG=$3)
      AS_VERSION_COMPARE($CLANG_VERSION, $MIN_CLANG, echo "checking $compiler >= $MIN_CLANG... no"; min=no, echo "checking $compiler >= $MIN_CLANG... yes"; min=yes, echo "checking $compiler >= $MIN_CLANG... yes"; min=yes)
      if test "$min" = "no" ; then
         continue;
      fi
      AS_VERSION_COMPARE($CLANG_VERSION, $MAX_CLANG, echo "checking $compiler < $MAX_CLANG... yes"; max=yes, echo "checking $compiler < $MAX_CLANG... no"; max=no, echo "checking $compiler < $MAX_CLANG... no"; max=no)
      if test "$max" = "no" ; then
         continue;
      fi
      CLANG_EXE=$compiler;

      LLVM_CONFIG_EXE=`echo $CLANG_EXE | sed s/clang/llvm-config/`
      LLVM_CXXFLAGS=`$LLVM_CONFIG_EXE --cxxflags`
      LLVM_CXXFLAGS+=" -O2 -DNDEBUG $4"
      if test "x$LLVM_CXXFLAGS" = "x"; then
         echo "checking llvm cxxflags... no. Package llvm-[]req_version_pretty[] missing?"
         break;
      fi
      echo "checking llvm cxxflags...$LLVM_CXXFLAGS"
      CLANG_CPP_EXE=`echo $CLANG_EXE | sed s/clang/clang-cpp/`
      if command -v "$CLANG_CPP_EXE" > /dev/null; then
         echo "checking cpp...$CLANG_CPP_EXE"
      else
         echo "checking cpp...no"
         CLANG_EXE=""
         continue
      fi
      CLANGPP_EXE=`echo $CLANG_EXE | sed s/clang/clang\+\+/`
      if command -v "$CLANGPP_EXE" > /dev/null; then
         echo "checking clang++...$CLANGPP_EXE"
      else
         echo "checking clang++...no"
         continue
      fi
      LLVM_LINK_EXE=`echo $CLANG_EXE | sed s/clang/llvm-link/`
      if command -v "$LLVM_LINK_EXE" > /dev/null; then
         echo "checking llvm-link...$LLVM_LINK_EXE"
      else
         echo "checking llvm-link...no"
         continue
      fi
      LLVM_OPT_EXE=`echo $CLANG_EXE | sed s/clang/opt/`
      if command -v "$LLVM_OPT_EXE" > /dev/null; then
         echo "checking llvm-opt...$LLVM_OPT_EXE"
      else
         echo "checking llvm-opt...no"
         continue
      fi
      AC_CACHE_CHECK([$compiler supports -m32],
        ax_cv_[]clang_id[]_m32,
        [
          ac_save_CC="$CC"
          ac_save_CFLAGS="$CFLAGS"
          ac_save_LDFLAGS="$LDFLAGS"
          ac_save_LIBS="$LIBS"
          CC=$CLANG_EXE
          CFLAGS="-m32"
          LDFLAGS=
          LIBS=
          AC_LANG_PUSH([C])
          AC_LINK_IFELSE([AC_LANG_SOURCE([int main(void){ return 0;}])],ax_cv_[]clang_id[]_m32=yes,ax_cv_[]clang_id[]_m32=no)
          AC_LANG_POP([C])
          CC=$ac_save_CC
          CFLAGS=$ac_save_CFLAGS
          LDFLAGS=$ac_save_LDFLAGS
          LIBS=$ac_save_LIBS
        ])
      if test "x$ax_cv_[]clang_id[]_m32" == xyes; then
         AC_DEFINE(HAVE_[]clang_macro_prefix[]_M32,1,[Define if clang []req_version_pretty supports -m32 ])
      fi
      AC_CACHE_CHECK([$compiler supports -mx32],
        ax_cv_[]clang_id[]_mx32,
        [
          ac_save_CC="$CC"
          ac_save_CFLAGS="$CFLAGS"
          ac_save_LDFLAGS="$LDFLAGS"
          ac_save_LIBS="$LIBS"
          CC=$CLANG_EXE
          CFLAGS="-mx32"
          LDFLAGS=
          LIBS=
          AC_LANG_PUSH([C])
          AC_LINK_IFELSE([AC_LANG_SOURCE([int main(void){ return 0;}])],ax_cv_[]clang_id[]_mx32=yes,ax_cv_[]clang_id[]_mx32=no)
          AC_LANG_POP([C])
          CC=$ac_save_CC
          CFLAGS=$ac_save_CFLAGS
          LDFLAGS=$ac_save_LDFLAGS
          LIBS=$ac_save_LIBS
        ])
      if test "x$ax_cv_[]clang_id[]_mx32" == xyes; then
         AC_DEFINE(HAVE_[]clang_macro_prefix[]_MX32,1,[Define if clang []req_version_pretty supports -mx32 ])
      fi
      AC_CACHE_CHECK([$compiler supports -m64],
        ax_cv_[]clang_id[]_m64,
        [
          ac_save_CC="$CC"
          ac_save_CFLAGS="$CFLAGS"
          ac_save_LDFLAGS="$LDFLAGS"
          ac_save_LIBS="$LIBS"
          CC=$CLANG_EXE
          CFLAGS="-m64"
          LDFLAGS=
          LIBS=
          AC_LANG_PUSH([C])
          AC_LINK_IFELSE([AC_LANG_SOURCE([int main(void){ return 0;}])],ax_cv_[]clang_id[]_m64=yes,ax_cv_[]clang_id[]_m64=no)
          AC_LANG_POP([C])
          CC=$ac_save_CC
          CFLAGS=$ac_save_CFLAGS
          LDFLAGS=$ac_save_LDFLAGS
          LIBS=$ac_save_LIBS
        ])
      if test "x$ax_cv_[]clang_id[]_m64" == xyes; then
         AC_DEFINE(HAVE_[]clang_macro_prefix[]_M64,1,[Define if clang []req_version_pretty supports -m64 ])
      fi
      AC_CACHE_CHECK([$compiler supports plugins], 
         ax_cv_[]clang_id[]_plugin_compiler,
         [
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
#ifdef _WIN32
int check;
#else
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
  PrintFunctionsConsumer(CompilerInstance &Instance, std::set<std::string> ParsedTemplates) : Instance(Instance), ParsedTemplates(ParsedTemplates) {}
  bool HandleTopLevelDecl(DeclGroupRef DG) override {
    for (DeclGroupRef::iterator i = DG.begin(), e = DG.end(); i != e; ++i) {
      const Decl *D = *i;
      if (const NamedDecl *ND = dyn_cast<NamedDecl>(D)) llvm::errs() << "top-level-decl: \"" << ND->getNameAsString() << "\"\n";
    }
    return true;
  }
  void HandleTranslationUnit(ASTContext& context) override {
    if (!Instance.getLangOpts().DelayedTemplateParsing) return;
    struct Visitor : public RecursiveASTVisitor<Visitor> {
      const std::set<std::string> &ParsedTemplates;
      Visitor(const std::set<std::string> &ParsedTemplates) : ParsedTemplates(ParsedTemplates) {}
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
      clang::LateParsedTemplate &LPT = *sema.LateParsedTemplateMap.find(FD)->second;
      sema.LateTemplateParser(sema.OpaqueParser, LPT);
      llvm::errs() << "late-parsed-decl: \"" << FD->getNameAsString() << "\"\n";
    }   
  }
};
class PrintFunctionNamesAction : public PluginASTAction {
  std::set<std::string> ParsedTemplates;
protected:
  std::unique_ptr<ASTConsumer> CreateASTConsumer(CompilerInstance &CI, llvm::StringRef) override {
#if __clang_major__ >= 10
         return std::make_unique<PrintFunctionsConsumer>(CI, ParsedTemplates);
#else
         return llvm::make_unique<PrintFunctionsConsumer>(CI, ParsedTemplates);
#endif
  }
  bool ParseArgs(const CompilerInstance &CI, const std::vector<std::string> &args) override {
    for (unsigned i = 0, e = args.size(); i != e; ++i) {
      llvm::errs() << "PrintFunctionNames arg = " << args.at(i) << "\n";
      DiagnosticsEngine &D = CI.getDiagnostics();
      if (args.at(i) == "-an-error") {
        unsigned DiagID = D.getCustomDiagID(DiagnosticsEngine::Error, "invalid argument '%0'");
        D.Report(DiagID) << args.at(i);
        return false;
      } else if (args.at(i) == "-parse-template") {
        if (i + 1 >= e) {
          D.Report(D.getCustomDiagID(DiagnosticsEngine::Error, "missing -parse-template argument"));
          return false;
        }
        ++i;
        ParsedTemplates.insert(args.at(i));
      }
    }
    if (!args.empty() && args.at(0) == "help") PrintHelp(llvm::errs());
    return true;
  }
  void PrintHelp(llvm::raw_ostream& ros) { ros << "Help for PrintFunctionNames plugin goes here\n"; }
  PluginASTAction::ActionType getActionType() override { return AddAfterMainAction; }
};
}
static FrontendPluginRegistry::Add<PrintFunctionNamesAction>X1("print-fns", "print function names");
#endif
PLUGIN_TEST
         ax_cv_[]clang_id[]_plugin_compiler=no
         for plugin_compiler in $CLANGPP_EXE; do
            plugin_option=
            case $host_os in
               mingw*) 
                  plugin_option="-shared -Wl,--export-all-symbols -Wl,--start-group -lclangAST -lclangASTMatchers -lclangAnalysis -lclangBasic -lclangDriver -lclangEdit -lclangFrontend -lclangFrontendTool -lclangLex -lclangParse -lclangSema -lclangEdit -lclangRewrite -lclangRewriteFrontend -lclangStaticAnalyzerFrontend -lclangStaticAnalyzerCheckers -lclangStaticAnalyzerCore -lclangCrossTU -lclangIndex -lclangSerialization -lclangToolingCore -lclangTooling -lclangFormat -Wl,--end-group -lversion `$LLVM_CONFIG_EXE --ldflags --libs --system-libs`"
               ;;
               darwin*)
                  plugin_option='-fPIC -shared -undefined dynamic_lookup '
               ;;
               *)
                  plugin_option='-fPIC -shared '
               ;;
            esac
            if test -f plugin_test.so; then
                  rm plugin_test.so
            fi
            case $host_os in
               mingw*) 
                  ax_cv_[]clang_id[]_plugin_compiler=$plugin_compiler
                  ;;
               *)
                  $plugin_compiler $LLVM_CXXFLAGS -c plugin_test.cpp -o plugin_test.o -fPIC 2> /dev/null
                  $plugin_compiler plugin_test.o $plugin_option -o plugin_test.so 2> /dev/null
                  if test ! -f plugin_test.so; then
                     continue
                  fi
                  ac_save_CC="$CC"
                  ac_save_CFLAGS="$CFLAGS"
                  CC=$CLANG_EXE
                  CFLAGS="-fplugin=$BUILDDIR/plugin_test.so -Xclang -add-plugin -Xclang print-fns"
                  AC_LANG_PUSH([C])
                  AC_COMPILE_IFELSE([AC_LANG_SOURCE([[
                  ]],[[
                        return 0;
                  ]])],
                  ax_cv_[]clang_id[]_plugin_compiler=$plugin_compiler,ax_cv_[]clang_id[]_plugin_compiler=no)
                  AC_LANG_POP([C])
                  CC=$ac_save_CC
                  CFLAGS=$ac_save_CFLAGS
                  #If plugin compilation fails, skip this executable
                  if test "x$ax_cv_[]clang_id[]_plugin_compiler" = xno; then
                     continue
                  fi
               ;;
            esac
         done
      ])
      if test "x$ax_cv_[]clang_id[]_plugin_compiler" != xno; then
         echo "OK, we have found the compiler"
         AC_SUBST(clang_macro_prefix[]_EXE, ${CLANG_EXE})
         AC_SUBST(clang_macro_prefix[]_VERSION, ${CLANG_VERSION})
         AC_SUBST(clang_macro_prefix[]_PLUGIN_DIR, $1)
         AC_SUBST(clang_macro_prefix[]_PLUGIN_COMPILER, ${ax_cv_[]clang_id[]_plugin_compiler})
         AC_SUBST(clang_macro_prefix[]_PLUGIN_CXXFLAGS, ${LLVM_CXXFLAGS})
         AC_DEFINE(HAVE_[]clang_macro_prefix[]_COMPILER, 1, "Define if CLANG []req_version_pretty I386 compiler is compliant")
         AC_DEFINE_UNQUOTED(clang_macro_prefix[]_EXE, "${CLANG_EXE}", "Define the plugin clang")
         AC_DEFINE_UNQUOTED(m4_bpatsubsts(clang_macro_prefix[_EXE],[CLANG],[CLANGPP]), "${CLANGPP_EXE}", "Define the plugin clang++")
         AC_DEFINE_UNQUOTED(m4_bpatsubsts(clang_macro_prefix[_EXE],[CLANG],[CLANG_CPP]), "${CLANG_CPP_EXE}", "Define the plugin cpp")
         AC_DEFINE_UNQUOTED(clang_macro_prefix[]_VERSION, "${CLANG_VERSION}", "Define the clang version")
         AC_DEFINE_UNQUOTED(clang_macro_prefix[]_PLUGIN_DIR, "$1", "Define the compiler plugins directory")
         AC_DEFINE_UNQUOTED(clang_macro_prefix[]_PLUGIN_COMPILER, "${ax_cv_[]clang_id[]_plugin_compiler}", "Define the plugin compiler")

         AC_DEFINE_UNQUOTED(llvm_macro_prefix[]_LINK_EXE, "${LLVM_LINK_EXE}", "Define the plugin clang++")
         AC_DEFINE_UNQUOTED(llvm_macro_prefix[]_OPT_EXE, "${LLVM_OPT_EXE}", "Define the plugin clang++")
         break;
      fi
   else
      echo "checking $compiler... not found"
   fi
done
AC_LANG_POP([C])
])
