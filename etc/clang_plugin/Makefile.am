#This is etc/clang_plugin/Makefile.am
EXTRA_DIST = $(top_srcdir)/etc/clang_plugin/plugin_dumpGimpleSSA.cpp $(top_srcdir)/etc/clang_plugin/plugin_includes.hpp $(top_srcdir)/etc/clang_plugin/plugin_dumpGimpleEmpty.cpp \
             $(top_srcdir)/etc/clang_plugin/dumpGimple.cpp $(top_srcdir)/etc/clang_plugin/plugin_topfname.cpp $(top_srcdir)/etc/clang_plugin/plugin_expandMemOps.cpp \
             $(top_srcdir)/etc/clang_plugin/plugin_GepiCanon.cpp $(top_srcdir)/etc/clang_plugin/plugin_ASTAnalyzer.cpp $(top_srcdir)/etc/clang_plugin/gcc/cp-tree.def \
             $(top_srcdir)/etc/clang_plugin/gcc/sync-builtins.def $(top_srcdir)/etc/clang_plugin/gcc/gtm-builtins.def $(top_srcdir)/etc/clang_plugin/gcc/sanitizer.def \
             $(top_srcdir)/etc/clang_plugin/gcc/hsa-builtins.def $(top_srcdir)/etc/clang_plugin/gcc/c-common.def $(top_srcdir)/etc/clang_plugin/gcc/gimple.def \
             $(top_srcdir)/etc/clang_plugin/gcc/builtins.def $(top_srcdir)/etc/clang_plugin/gcc/brig-builtins.def $(top_srcdir)/etc/clang_plugin/gcc/cilkplus.def \
             $(top_srcdir)/etc/clang_plugin/gcc/omp-builtins.def $(top_srcdir)/etc/clang_plugin/gcc/tree.def $(top_srcdir)/etc/clang_plugin/gcc/cilk-builtins.def \
             $(top_srcdir)/etc/clang_plugin/gcc/chkp-builtins.def $(top_srcdir)/etc/clang_plugin/gcc/COPYING3 $(top_srcdir)/etc/clang_plugin/HardekopfLin_AA.hpp \
             $(top_srcdir)/etc/clang_plugin/HardekopfLin_AA.cpp $(top_srcdir)/etc/clang_plugin/llvm/eSSA.hpp $(top_srcdir)/etc/clang_plugin/llvm/eSSA.cpp \
             $(top_srcdir)/etc/clang_plugin/GepiCanonicalizationPass.hpp $(top_srcdir)/etc/clang_plugin/GepiCanonicalizationPass.cpp $(top_srcdir)/etc/clang_plugin/llvm/RangeAnalysis.hpp \
             $(top_srcdir)/etc/clang_plugin/llvm/RangeAnalysis.cpp $(top_srcdir)/etc/clang_plugin/llvm/LICENSE.TXT $(top_srcdir)/etc/clang_plugin/llvm/my_OrderedInstructions.cpp \
             $(top_srcdir)/etc/clang_plugin/llvm/my_OrderedInstructions.hpp $(top_srcdir)/etc/clang_plugin/bambuS2S.cpp $(top_srcdir)/etc/clang_plugin/CustomScalarReplacementOfAggregatesPass.hpp \
             $(top_srcdir)/etc/clang_plugin/CustomScalarReplacementOfAggregatesPass.cpp $(top_srcdir)/etc/clang_plugin/plugin_CSROA.cpp \
             $(top_srcdir)/etc/clang_plugin/ExpandMemOpsPass.hpp $(top_srcdir)/etc/clang_plugin/PtrIteratorSimplifyPass.hpp $(top_srcdir)/etc/clang_plugin/fpga_callbacks.hpp

LD_LIBRARY_PATH=
PANDA_PLUGIN_VERSION=0.11
#if ENABLE_WERROR
#WFLAGS=-Wall \
#   -Werror \
#   -Wextra \
#   -Wconversion \
#   -Wno-deprecated \
#   -Winit-self \
#   -Wsign-conversion \
#   -Wredundant-decls \
#   -Wvla -Wshadow \
#   -Wmissing-include-dirs \
#   -Winit-self \
#   -Wpointer-arith \
#   -Wcast-qual \
#   -Wcast-align \
#   -Wswitch-enum \
#   -Wswitch-default \
#   -Wno-unused-function \
#   -fdiagnostics-show-option
#else
WFLAGS=$(all_includes) -I$(top_srcdir)/etc/clang_plugin/
#endif

check:all

CLANG_LIBS=-lclangAST -lclangASTMatchers -lclangAnalysis -lclangBasic -lclangDriver -lclangEdit -lclangFrontend -lclangFrontendTool -lclangLex -lclangParse -lclangSema -lclangEdit -lclangRewrite -lclangRewriteFrontend -lclangStaticAnalyzerFrontend -lclangStaticAnalyzerCheckers -lclangStaticAnalyzerCore -lclangIndex -lclangSerialization -lclangToolingCore -lclangTooling -lclangFormat

###### clang 4.0
include $(top_srcdir)/etc/clang_plugin/clang4.am

###### clang 5.0
include $(top_srcdir)/etc/clang_plugin/clang5.am

###### clang 6.0
include $(top_srcdir)/etc/clang_plugin/clang6.am

###### clang 7.0
include $(top_srcdir)/etc/clang_plugin/clang7.am

###### clang 8.0
include $(top_srcdir)/etc/clang_plugin/clang8.am

###### clang 9.0
include $(top_srcdir)/etc/clang_plugin/clang9.am


clean :
	rm -rf *.o *.so *.h bambuS2S-*

install: all
	if test "`ls *so`" != ""; then mkdir -p $(DESTDIR)$(CLANG_PLUGIN_DIR); cp -v *.so $(DESTDIR)$(CLANG_PLUGIN_DIR); fi
	if test "`ls bambuS2S-*`" != ""; then mkdir -p $(DESTDIR)$(bindir); cp -v  bambuS2S-* $(DESTDIR)$(bindir); fi

#do not touch the following line


