#This is etc/clang_plugin/Makefile.am
EXTRA_DIST = \
	$(top_srcdir)/etc/clang_plugin/gcc/cp-tree.def \
	$(top_srcdir)/etc/clang_plugin/gcc/sync-builtins.def \
	$(top_srcdir)/etc/clang_plugin/gcc/gtm-builtins.def \
	$(top_srcdir)/etc/clang_plugin/gcc/sanitizer.def \
	$(top_srcdir)/etc/clang_plugin/gcc/hsa-builtins.def \
	$(top_srcdir)/etc/clang_plugin/gcc/c-common.def \
	$(top_srcdir)/etc/clang_plugin/gcc/gimple.def \
	$(top_srcdir)/etc/clang_plugin/gcc/builtins.def \
	$(top_srcdir)/etc/clang_plugin/gcc/brig-builtins.def \
	$(top_srcdir)/etc/clang_plugin/gcc/cilkplus.def \
	$(top_srcdir)/etc/clang_plugin/gcc/omp-builtins.def \
	$(top_srcdir)/etc/clang_plugin/gcc/tree.def \
	$(top_srcdir)/etc/clang_plugin/gcc/cilk-builtins.def \
	$(top_srcdir)/etc/clang_plugin/gcc/chkp-builtins.def \
	$(top_srcdir)/etc/clang_plugin/gcc/COPYING3 \
	$(top_srcdir)/etc/clang_plugin/plugin_ASTAnalyzer.cpp \
	$(top_srcdir)/etc/clang_plugin/plugin_dumpGimpleSSA.cpp \
	$(top_srcdir)/etc/clang_plugin/plugin_dumpGimpleEmpty.cpp \
	$(top_srcdir)/etc/clang_plugin/plugin_expandMemOps.cpp \
	$(top_srcdir)/etc/clang_plugin/plugin_GepiCanon.cpp \
	$(top_srcdir)/etc/clang_plugin/plugin_includes.hpp \
	$(top_srcdir)/etc/clang_plugin/plugin_topfname.cpp \
	$(top_srcdir)/etc/clang_plugin/clang_version_symbol.hpp \
	$(top_srcdir)/etc/clang_plugin/CustomScalarReplacementOfAggregatesPass.cpp \
	$(top_srcdir)/etc/clang_plugin/CustomScalarReplacementOfAggregatesPass.hpp \
	$(top_srcdir)/etc/clang_plugin/debug_print.hpp \
	$(top_srcdir)/etc/clang_plugin/dumpGimple.cpp \
	$(top_srcdir)/etc/clang_plugin/ExpandMemOpsPass.hpp \
	$(top_srcdir)/etc/clang_plugin/fpga_callbacks.hpp \
	$(top_srcdir)/etc/clang_plugin/GepiCanonicalizationPass.hpp \
	$(top_srcdir)/etc/clang_plugin/GepiCanonicalizationPass.cpp \
	$(top_srcdir)/etc/clang_plugin/HardekopfLin_AA.hpp \
	$(top_srcdir)/etc/clang_plugin/HardekopfLin_AA.cpp \
	$(top_srcdir)/etc/clang_plugin/plugin_CSROA.cpp \
	$(top_srcdir)/etc/clang_plugin/PtrIteratorSimplifyPass.hpp \
	$(top_srcdir)/etc/clang_plugin/TreeHeightReduction.hpp

LD_LIBRARY_PATH=
PANDA_PLUGIN_VERSION=0.12

WFLAGS=$(all_includes) -I$(top_srcdir)/etc/clang_plugin/

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

###### clang 10.0
include $(top_srcdir)/etc/clang_plugin/clang10.am

###### clang 11.0
include $(top_srcdir)/etc/clang_plugin/clang11.am

###### clang vvd
include $(top_srcdir)/etc/clang_plugin/clangvvd.am

###### clang 12.0
include $(top_srcdir)/etc/clang_plugin/clang12.am

###### clang 13.0
include $(top_srcdir)/etc/clang_plugin/clang13.am

###### clang 16.0
include $(top_srcdir)/etc/clang_plugin/clang16.am

clean :
	rm -rf *.o *.so *.h

install: all
	if test "`ls *so`" != ""; then mkdir -p $(DESTDIR)$(CLANG_PLUGIN_DIR); cp -v *.so $(DESTDIR)$(CLANG_PLUGIN_DIR); fi

#do not touch the following line


