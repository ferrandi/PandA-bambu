#This is etc/clang_plugin/Makefile.am
EXTRA_DIST = $(top_srcdir)/etc/clang_plugin/plugin_dumpGimpleSSA.cpp $(top_srcdir)/etc/clang_plugin/plugin_includes.hpp $(top_srcdir)/etc/clang_plugin/plugin_dumpGimpleEmpty.cpp $(top_srcdir)/etc/clang_plugin/dumpGimple.cpp $(top_srcdir)/etc/clang_plugin/plugin_topfname.cpp $(top_srcdir)/etc/clang_plugin/plugin_expandMemOps.cpp $(top_srcdir)/etc/clang_plugin/plugin_ASTAnalyzer.cpp $(top_srcdir)/etc/clang_plugin/gcc/cp-tree.def $(top_srcdir)/etc/clang_plugin/gcc/sync-builtins.def $(top_srcdir)/etc/clang_plugin/gcc/gtm-builtins.def $(top_srcdir)/etc/clang_plugin/gcc/sanitizer.def $(top_srcdir)/etc/clang_plugin/gcc/hsa-builtins.def $(top_srcdir)/etc/clang_plugin/gcc/c-common.def $(top_srcdir)/etc/clang_plugin/gcc/gimple.def $(top_srcdir)/etc/clang_plugin/gcc/builtins.def $(top_srcdir)/etc/clang_plugin/gcc/brig-builtins.def $(top_srcdir)/etc/clang_plugin/gcc/cilkplus.def $(top_srcdir)/etc/clang_plugin/gcc/omp-builtins.def $(top_srcdir)/etc/clang_plugin/gcc/tree.def $(top_srcdir)/etc/clang_plugin/gcc/cilk-builtins.def $(top_srcdir)/etc/clang_plugin/gcc/chkp-builtins.def $(top_srcdir)/etc/clang_plugin/gcc/COPYING3 $(top_srcdir)/etc/clang_plugin/HardekopfLin_AA.hpp $(top_srcdir)/etc/clang_plugin/HardekopfLin_AA.cpp $(top_srcdir)/etc/clang_plugin/llvm/eSSA.hpp $(top_srcdir)/etc/clang_plugin/llvm/eSSA.cpp $(top_srcdir)/etc/clang_plugin/llvm/RangeAnalysis.hpp $(top_srcdir)/etc/clang_plugin/llvm/RangeAnalysis.cpp $(top_srcdir)/etc/clang_plugin/llvm/LICENSE.TXT $(top_srcdir)/etc/clang_plugin/llvm/my_OrderedInstructions.cpp $(top_srcdir)/etc/clang_plugin/llvm/my_OrderedInstructions.hpp $(top_srcdir)/etc/clang_plugin/bambuS2S.cpp $(top_srcdir)/etc/clang_plugin/CSROA.hpp $(top_srcdir)/etc/clang_plugin/CSROA.cpp $(top_srcdir)/etc/clang_plugin/plugin_CSROA.cpp

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
CLANG4_LIBS=-Wl,--start-group $(CLANG_LIBS) -Wl,--end-group
CLANG4_LIBSD=$(CLANG_LIBS)

CLANG5_LIBS=-Wl,--start-group $(CLANG_LIBS) -Wl,--end-group
CLANG5_LIBSD=$(CLANG_LIBS)

CLANG6_LIBS=-Wl,--start-group $(CLANG_LIBS) -lclangCrossTU -Wl,--end-group
CLANG6_LIBSD=$(CLANG_LIBS) -lclangCrossTU

CLANG7_LIBS=-Wl,--start-group $(CLANG_LIBS) -lclangCrossTU -lclangToolingInclusions -Wl,--end-group
CLANG7_LIBSD=$(CLANG_LIBS) -lclangCrossTU -lclangToolingInclusions

###### clang 4.0
if BUILD_I386_CLANG4_EMPTY_PLUGIN
all: $(I386_CLANG4_EMPTY_PLUGIN) bambuS2S-4
endif

if BUILD_I386_CLANG4_SSA_PLUGIN
all: $(I386_CLANG4_SSA_PLUGIN)
endif

if BUILD_I386_CLANG4_SSA_PLUGINCPP
all: $(I386_CLANG4_SSA_PLUGINCPP)
endif

if BUILD_I386_CLANG4_EXPANDMEMOPS_PLUGIN
all: $(I386_CLANG4_EXPANDMEMOPS_PLUGIN)
endif

if BUILD_I386_CLANG4_CSROA_PLUGIN
all: $(I386_CLANG4_CSROA_PLUGIN)
endif

if BUILD_I386_CLANG4_TOPFNAME_PLUGIN
all: $(I386_CLANG4_TOPFNAME_PLUGIN)
endif

if BUILD_I386_CLANG4_ASTANALYZER_PLUGIN
all: $(I386_CLANG4_ASTANALYZER_PLUGIN)
endif

if BUILD_PTAA
$(I386_CLANG4_SSA_PLUGIN) : opt_clang4_plugin_dumpGimpleSSA.o clang4_plugin_dumpGimpleSSA.o clang4_dumpGimple.o clang4_HardekopfLin_AA.o clang4_RangeAnalysis.o clang4_eSSA.o clang4_my_OrderedInstructions.o
	case $(host_os) in \
	  mingw*) \
	    compile_options='$(CLANG4_LIBS) -lversion `$(I386_LLVM_CONFIG4_EXE) --ldflags --libs --system-libs` -lbdd';\
	    compile_options='';; \
	  darwin*) \
	    compile_options='-fPIC -shared -undefined dynamic_lookup -fno-rtti -lbdd';; \
	  *) \
	    compile_options='-fPIC -shared -fno-rtti -lbdd';;\
	esac;\
	obj_list='clang4_dumpGimple.o clang4_HardekopfLin_AA.o clang4_RangeAnalysis.o clang4_eSSA.o clang4_my_OrderedInstructions.o';\
	if test -n "$$compile_options"; then \
	  $(I386_CLANG4_PLUGIN_COMPILER) opt_clang4_plugin_dumpGimpleSSA.o $$obj_list $$compile_options -o clang4_plugin_dumpGimpleSSA_opt.so;\
	  $(I386_CLANG4_PLUGIN_COMPILER) clang4_plugin_dumpGimpleSSA.o $$obj_list $$compile_options -o clang4_plugin_dumpGimpleSSA.so ; \
	fi;
else
$(I386_CLANG4_SSA_PLUGIN) : opt_clang4_plugin_dumpGimpleSSA.o clang4_plugin_dumpGimpleSSA.o clang4_dumpGimple.o clang4_RangeAnalysis.o clang4_eSSA.o clang4_my_OrderedInstructions.o
	case $(host_os) in \
	  mingw*) \
	    compile_options='$(CLANG4_LIBS) -lversion `$(I386_LLVM_CONFIG4_EXE) --ldflags --libs --system-libs`'; \
	    compile_options='';; \
	  darwin*) \
	    compile_options='-fPIC -shared -undefined dynamic_lookup -fno-rtti';; \
	  *) \
	    compile_options='-fPIC -shared -fno-rtti';;\
	esac;\
	obj_list='clang4_dumpGimple.o clang4_RangeAnalysis.o clang4_eSSA.o clang4_my_OrderedInstructions.o';\
	if test -n "$$compile_options"; then \
	  $(I386_CLANG4_PLUGIN_COMPILER) opt_clang4_plugin_dumpGimpleSSA.o $$obj_list $$compile_options -o clang4_plugin_dumpGimpleSSA_opt.so;\
	  $(I386_CLANG4_PLUGIN_COMPILER) clang4_plugin_dumpGimpleSSA.o $$obj_list $$compile_options -o clang4_plugin_dumpGimpleSSA.so ; \
	fi;
endif
if BUILD_PTAA
$(I386_CLANG4_SSA_PLUGINCPP) : opt_clang4_plugin_dumpGimpleSSACpp.o clang4_plugin_dumpGimpleSSACpp.o clang4_dumpGimple.o clang4_HardekopfLin_AA.o clang4_RangeAnalysis.o clang4_eSSA.o clang4_my_OrderedInstructions.o
	case $(host_os) in \
	  mingw*) \
	    compile_options='$(CLANG4_LIBS) -lversion `$(I386_LLVM_CONFIG4_EXE) --ldflags --libs --system-libs` -lbdd'; \
	    compile_options='';; \
	  darwin*) \
	    compile_options='-fPIC -shared -undefined dynamic_lookup -fno-rtti -lbdd';; \
	  *) \
	    compile_options='-fPIC -shared -fno-rtti -lbdd';;\
	esac;\
	obj_list='clang4_dumpGimple.o clang4_HardekopfLin_AA.o clang4_RangeAnalysis.o clang4_eSSA.o clang4_my_OrderedInstructions.o';\
	if test -n "$$compile_options"; then \
	  $(I386_CLANG4_PLUGIN_COMPILER) opt_clang4_plugin_dumpGimpleSSACpp.o $$obj_list $$compile_options -o clang4_plugin_dumpGimpleSSACpp_opt.so;\
	  $(I386_CLANG4_PLUGIN_COMPILER) clang4_plugin_dumpGimpleSSACpp.o $$obj_list $$compile_options -o clang4_plugin_dumpGimpleSSACpp.so;\
	fi;
else
$(I386_CLANG4_SSA_PLUGINCPP) : opt_clang4_plugin_dumpGimpleSSACpp.o clang4_plugin_dumpGimpleSSACpp.o clang4_dumpGimple.o clang4_RangeAnalysis.o clang4_eSSA.o clang4_my_OrderedInstructions.o
	case $(host_os) in \
	  mingw*) \
	    compile_options='$(CLANG4_LIBS) -lversion `$(I386_LLVM_CONFIG4_EXE) --ldflags --libs --system-libs`'; \
	    compile_options='';; \
	  darwin*) \
	    compile_options='-fPIC -shared -undefined dynamic_lookup -fno-rtti';; \
	  *) \
	    compile_options='-fPIC -shared -fno-rtti';;\
	esac;\
	obj_list='clang4_dumpGimple.o clang4_RangeAnalysis.o clang4_eSSA.o clang4_my_OrderedInstructions.o';\
	if test -n "$$compile_options"; then \
	  $(I386_CLANG4_PLUGIN_COMPILER) opt_clang4_plugin_dumpGimpleSSACpp.o $$obj_list $$compile_options -o clang4_plugin_dumpGimpleSSACpp_opt.so;\
	  $(I386_CLANG4_PLUGIN_COMPILER) clang4_plugin_dumpGimpleSSACpp.o $$obj_list $$compile_options -o clang4_plugin_dumpGimpleSSACpp.so;\
	fi;
endif

if BUILD_PTAA
$(I386_CLANG4_EMPTY_PLUGIN) : opt_clang4_plugin_dumpGimpleEmpty.o clang4_plugin_dumpGimpleEmpty.o clang4_dumpGimple.o clang4_HardekopfLin_AA.o clang4_RangeAnalysis.o clang4_eSSA.o clang4_my_OrderedInstructions.o
	case $(host_os) in \
	  mingw*) \
	    compile_options='$(CLANG4_LIBS) -lversion `$(I386_LLVM_CONFIG4_EXE) --ldflags --libs --system-libs` -lbdd'; \
	    compile_options='';; \
	  darwin*) \
	    compile_options='-fPIC -shared -undefined dynamic_lookup -fno-rtti -lbdd';; \
	  *) \
	    compile_options='-fPIC -shared -fno-rtti -lbdd';;\
	esac;\
	obj_list='clang4_dumpGimple.o clang4_HardekopfLin_AA.o clang4_RangeAnalysis.o clang4_eSSA.o clang4_my_OrderedInstructions.o';\
	if test -n "$$compile_options"; then \
	  $(I386_CLANG4_PLUGIN_COMPILER) opt_clang4_plugin_dumpGimpleEmpty.o $$obj_list $$compile_options -o clang4_plugin_dumpGimpleEmpty_opt.so; \
	  $(I386_CLANG4_PLUGIN_COMPILER) clang4_plugin_dumpGimpleEmpty.o $$obj_list $$compile_options -o clang4_plugin_dumpGimpleEmpty.so; \
	fi;
else
$(I386_CLANG4_EMPTY_PLUGIN) : opt_clang4_plugin_dumpGimpleEmpty.o clang4_plugin_dumpGimpleEmpty.o clang4_dumpGimple.o clang4_RangeAnalysis.o clang4_eSSA.o clang4_my_OrderedInstructions.o
	case $(host_os) in \
	  mingw*) \
	    compile_options='$(CLANG4_LIBS) -lversion `$(I386_LLVM_CONFIG4_EXE) --ldflags --libs --system-libs`'; \
	    compile_options='';; \
	  darwin*) \
	    compile_options='-fPIC -shared -undefined dynamic_lookup -fno-rtti';; \
	  *) \
	    compile_options='-fPIC -shared -fno-rtti';;\
	esac;\
	obj_list='clang4_dumpGimple.o clang4_RangeAnalysis.o clang4_eSSA.o clang4_my_OrderedInstructions.o';\
	if test -n "$$compile_options"; then \
	  $(I386_CLANG4_PLUGIN_COMPILER) opt_clang4_plugin_dumpGimpleEmpty.o $$obj_list $$compile_options -o clang4_plugin_dumpGimpleEmpty_opt.so; \
	  $(I386_CLANG4_PLUGIN_COMPILER) clang4_plugin_dumpGimpleEmpty.o $$obj_list $$compile_options -o clang4_plugin_dumpGimpleEmpty.so; \
	fi;
endif

$(I386_CLANG4_EXPANDMEMOPS_PLUGIN) : opt_clang4_plugin_expandMemOps.o clang4_plugin_expandMemOps.o 
	case $(host_os) in \
	  mingw*) \
	    compile_options='$(CLANG4_LIBS) -lversion `$(I386_LLVM_CONFIG4_EXE) --ldflags --libs --system-libs`'; \
	    compile_options='';; \
	  darwin*) \
	    compile_options='-fPIC -shared -undefined dynamic_lookup -fno-rtti';; \
	  *) \
	    compile_options='-fPIC -shared -fno-rtti';;\
	esac;\
	if test -n "$$compile_options"; then \
	  $(I386_CLANG4_PLUGIN_COMPILER) opt_clang4_plugin_expandMemOps.o $$compile_options -o clang4_plugin_expandMemOps_opt.so; \
	  $(I386_CLANG4_PLUGIN_COMPILER) clang4_plugin_expandMemOps.o $$compile_options -o clang4_plugin_expandMemOps.so; \
	fi;

$(I386_CLANG4_CSROA_PLUGIN) : opt_clang4_plugin_CSROA.o clang4_plugin_CSROA.o clang4_CSROA.o
	case $(host_os) in \
	  mingw*) \
	    compile_options='$(CLANG4_LIBS) -lversion `$(I386_LLVM_CONFIG4_EXE) --ldflags --libs --system-libs`'; \
	    compile_options='';; \
	  darwin*) \
	    compile_options='-fPIC -shared -undefined dynamic_lookup -fno-rtti';; \
	  *) \
	    compile_options='-fPIC -shared -fno-rtti';;\
	esac;\
	if test -n "$$compile_options"; then \
	  $(I386_CLANG4_PLUGIN_COMPILER) opt_clang4_plugin_CSROA.o clang4_CSROA.o $$compile_options -o clang4_plugin_CSROA_opt.so; \
	  $(I386_CLANG4_PLUGIN_COMPILER) clang4_plugin_CSROA.o clang4_CSROA.o $$compile_options -o clang4_plugin_CSROA.so; \
	fi;


$(I386_CLANG4_TOPFNAME_PLUGIN) : opt_clang4_plugin_topfname.o clang4_plugin_topfname.o 
	case $(host_os) in \
	  mingw*) \
	    compile_options='$(CLANG4_LIBS) -lversion `$(I386_LLVM_CONFIG4_EXE) --ldflags --libs --system-libs`'; \
	    compile_options='';; \
	  darwin*) \
	    compile_options='-fPIC -shared -undefined dynamic_lookup -fno-rtti';; \
	  *) \
	    compile_options='-fPIC -shared -fno-rtti';;\
	esac;\
	if test -n "$$compile_options"; then \
	  $(I386_CLANG4_PLUGIN_COMPILER) opt_clang4_plugin_topfname.o $$compile_options -o clang4_plugin_topfname_opt.so; \
	  $(I386_CLANG4_PLUGIN_COMPILER) clang4_plugin_topfname.o $$compile_options -o clang4_plugin_topfname.so; \
	fi;

$(I386_CLANG4_ASTANALYZER_PLUGIN) : clang4_plugin_ASTAnalyzer.o 
	case $(host_os) in \
	  mingw*) \
	    compile_options='$(CLANG4_LIBS) -lversion `$(I386_LLVM_CONFIG4_EXE) --ldflags --libs --system-libs`'; \
	    compile_options='';; \
	  darwin*) \
	    compile_options='-fPIC -shared -undefined dynamic_lookup -fno-rtti';; \
	  *) \
	    compile_options='-fPIC -shared -fno-rtti';;\
	esac;\
	if test -n "$$compile_options"; then \
	  $(I386_CLANG4_PLUGIN_COMPILER) clang4_plugin_ASTAnalyzer.o $$compile_options -o clang4_plugin_ASTAnalyzer.so; \
	fi;

clang4_plugin_dumpGimpleEmpty.o : $(top_srcdir)/etc/clang_plugin/plugin_dumpGimpleEmpty.cpp $(top_srcdir)/etc/clang_plugin/plugin_includes.hpp 
	case $(host_os) in \
	  mingw*) \
	    compile_options=' -std=c++11 -fno-rtti -O2 -I$(I386_LLVM4_HEADER_DIR) $(WFLAGS) -I.'; \
	    compile_options='';; \
	  *) \
	    compile_options=' -std=c++11 -fPIC -fno-rtti -O2 -I$(I386_LLVM4_HEADER_DIR) $(WFLAGS) -I.';; \
	esac;\
	if test -n "$$compile_options"; then \
	    $(I386_CLANG4_PLUGIN_COMPILER) -DADD_RSP=0 $$compile_options -c $(top_srcdir)/etc/clang_plugin/plugin_dumpGimpleEmpty.cpp -o opt_clang4_plugin_dumpGimpleEmpty.o; \
	    $(I386_CLANG4_PLUGIN_COMPILER) -DADD_RSP=1 $$compile_options -c $(top_srcdir)/etc/clang_plugin/plugin_dumpGimpleEmpty.cpp -o clang4_plugin_dumpGimpleEmpty.o; \
	fi;

clang4_plugin_dumpGimpleSSA.o : $(top_srcdir)/etc/clang_plugin/plugin_dumpGimpleSSA.cpp $(top_srcdir)/etc/clang_plugin/plugin_includes.hpp 
	case $(host_os) in \
	  mingw*) \
	    compile_options=' -std=c++11 -fno-rtti -O2 -I$(I386_LLVM4_HEADER_DIR) $(WFLAGS) -I.'; \
	    compile_options='';; \
	  *) \
	    compile_options=' -std=c++11 -fPIC -fno-rtti -O2 -I$(I386_LLVM4_HEADER_DIR) $(WFLAGS) -I.';; \
	esac;\
	if test -n "$$compile_options"; then \
	  $(I386_CLANG4_PLUGIN_COMPILER) -DADD_RSP=0 $$compile_options -c $(top_srcdir)/etc/clang_plugin/plugin_dumpGimpleSSA.cpp -o opt_clang4_plugin_dumpGimpleSSA.o;\
	  $(I386_CLANG4_PLUGIN_COMPILER) -DADD_RSP=1 $$compile_options -c $(top_srcdir)/etc/clang_plugin/plugin_dumpGimpleSSA.cpp -o clang4_plugin_dumpGimpleSSA.o;\
	fi;

clang4_plugin_dumpGimpleSSACpp.o : $(top_srcdir)/etc/clang_plugin/plugin_dumpGimpleSSA.cpp $(top_srcdir)/etc/clang_plugin/plugin_includes.hpp 
	case $(host_os) in \
	  mingw*) \
	    compile_options=' -std=c++11 -fno-rtti -O2 -I$(I386_LLVM4_HEADER_DIR) $(WFLAGS) -I. -DCPP_LANGUAGE'; \
	    compile_options='';; \
	  *) \
	    compile_options=' -std=c++11 -fPIC -fno-rtti -O2 -I$(I386_LLVM4_HEADER_DIR) $(WFLAGS) -I. -DCPP_LANGUAGE';; \
	esac;\
	if test -n "$$compile_options"; then \
	  $(I386_CLANG4_PLUGIN_COMPILER) -DADD_RSP=0 $$compile_options -c $(top_srcdir)/etc/clang_plugin/plugin_dumpGimpleSSA.cpp -o opt_clang4_plugin_dumpGimpleSSACpp.o;\
	  $(I386_CLANG4_PLUGIN_COMPILER) -DADD_RSP=1 $$compile_options -c $(top_srcdir)/etc/clang_plugin/plugin_dumpGimpleSSA.cpp -o clang4_plugin_dumpGimpleSSACpp.o;\
	fi;

clang4_plugin_expandMemOps.o : $(top_srcdir)/etc/clang_plugin/plugin_expandMemOps.cpp $(top_srcdir)/etc/clang_plugin/plugin_includes.hpp 
	case $(host_os) in \
	  mingw*) \
	    compile_options=' -std=c++11 -fno-rtti -O2 -I$(I386_LLVM4_HEADER_DIR) $(WFLAGS) -I.'; \
	    compile_options='';; \
	  *) \
	    compile_options=' -std=c++11 -fPIC -fno-rtti -O2 -I$(I386_LLVM4_HEADER_DIR) $(WFLAGS) -I.';; \
	esac;\
	if test -n "$$compile_options"; then \
	  $(I386_CLANG4_PLUGIN_COMPILER) -DADD_RSP=0 $$compile_options -c $(top_srcdir)/etc/clang_plugin/plugin_expandMemOps.cpp -o opt_clang4_plugin_expandMemOps.o; \
	  $(I386_CLANG4_PLUGIN_COMPILER) -DADD_RSP=1 $$compile_options -c $(top_srcdir)/etc/clang_plugin/plugin_expandMemOps.cpp -o clang4_plugin_expandMemOps.o; \
	fi;

clang4_plugin_CSROA.o : $(top_srcdir)/etc/clang_plugin/plugin_CSROA.cpp $(top_srcdir)/etc/clang_plugin/plugin_includes.hpp 
	case $(host_os) in \
	  mingw*) \
	    compile_options=' -std=c++11 -fno-rtti -O2 -I$(I386_LLVM4_HEADER_DIR) $(WFLAGS) -I.'; \
	    compile_options='';; \
	  *) \
	    compile_options=' -std=c++11 -fPIC -fno-rtti -O2 -I$(I386_LLVM4_HEADER_DIR) $(WFLAGS) -I.';; \
	esac;\
	if test -n "$$compile_options"; then \
	  $(I386_CLANG4_PLUGIN_COMPILER) -DADD_RSP=0 $$compile_options -c $(top_srcdir)/etc/clang_plugin/plugin_CSROA.cpp -o opt_clang4_plugin_CSROA.o; \
	  $(I386_CLANG4_PLUGIN_COMPILER) -DADD_RSP=1 $$compile_options -c $(top_srcdir)/etc/clang_plugin/plugin_CSROA.cpp -o clang4_plugin_CSROA.o; \
	fi;

clang4_plugin_topfname.o : $(top_srcdir)/etc/clang_plugin/plugin_topfname.cpp $(top_srcdir)/etc/clang_plugin/plugin_includes.hpp 
	case $(host_os) in \
	  mingw*) \
	    compile_options=' -std=c++11 -fno-rtti -O2 -I$(I386_LLVM4_HEADER_DIR) $(WFLAGS) -I.'; \
	    compile_options='';; \
	  *) \
	    compile_options=' -std=c++11 -fPIC -fno-rtti -O2 -I$(I386_LLVM4_HEADER_DIR) $(WFLAGS) -I.';; \
	esac;\
	if test -n "$$compile_options"; then \
	  $(I386_CLANG4_PLUGIN_COMPILER) -DADD_RSP=0 $$compile_options -c $(top_srcdir)/etc/clang_plugin/plugin_topfname.cpp -o opt_clang4_plugin_topfname.o; \
	  $(I386_CLANG4_PLUGIN_COMPILER) -DADD_RSP=1 $$compile_options -c $(top_srcdir)/etc/clang_plugin/plugin_topfname.cpp -o clang4_plugin_topfname.o; \
	fi;

clang4_plugin_ASTAnalyzer.o : $(top_srcdir)/etc/clang_plugin/plugin_ASTAnalyzer.cpp $(top_srcdir)/etc/clang_plugin/plugin_includes.hpp 
	case $(host_os) in \
	  mingw*) \
	    compile_options=' -std=c++11 -fno-rtti -O2 -I$(I386_LLVM4_HEADER_DIR) $(WFLAGS) -I.'; \
	    compile_options='';; \
	  *) \
	    compile_options=' -std=c++11 -fPIC -fno-rtti -O2 -I$(I386_LLVM4_HEADER_DIR) $(WFLAGS) -I.';; \
	esac;\
	if test -n "$$compile_options"; then \
	  $(I386_CLANG4_PLUGIN_COMPILER) $$compile_options -c $(top_srcdir)/etc/clang_plugin/plugin_ASTAnalyzer.cpp -o clang4_plugin_ASTAnalyzer.o;\
	fi;

clang4_my_OrderedInstructions.o : $(top_srcdir)/etc/clang_plugin/llvm/my_OrderedInstructions.cpp $(top_srcdir)/etc/clang_plugin/llvm/my_OrderedInstructions.hpp
	case $(host_os) in \
	  mingw*) \
	    compile_options=' -std=c++11 -fno-rtti -O2 -I$(I386_LLVM4_HEADER_DIR) -DPANDA_PLUGIN_VERSION="$(PANDA_PLUGIN_VERSION)" $(WFLAGS) -I.'; \
	    compile_options='';; \
	  *) \
	    compile_options=' -std=c++11 -fPIC -fno-rtti -O2 -I$(I386_LLVM4_HEADER_DIR) -DPANDA_PLUGIN_VERSION="$(PANDA_PLUGIN_VERSION)" $(WFLAGS) -I.';; \
	esac;\
	if test -n "$$compile_options"; then \
	  $(I386_CLANG4_PLUGIN_COMPILER) $$compile_options -c $(top_srcdir)/etc/clang_plugin/llvm/my_OrderedInstructions.cpp -o clang4_my_OrderedInstructions.o;\
	fi;

clang4_dumpGimple.o : $(top_srcdir)/etc/clang_plugin/dumpGimple.cpp $(top_srcdir)/etc/clang_plugin/plugin_includes.hpp $(top_srcdir)/etc/clang_plugin/HardekopfLin_AA.hpp $(top_srcdir)/etc/clang_plugin/llvm/RangeAnalysis.hpp  $(top_srcdir)/etc/clang_plugin/llvm/eSSA.hpp
	case $(host_os) in \
	  mingw*) \
	    compile_options=' -std=c++11 -fno-rtti -O2 -I$(I386_LLVM4_HEADER_DIR) -DPANDA_PLUGIN_VERSION="$(PANDA_PLUGIN_VERSION)" $(WFLAGS) -I.'; \
	    compile_options='';; \
	  *) \
	    compile_options=' -std=c++11 -fPIC -fno-rtti -O2 -I$(I386_LLVM4_HEADER_DIR) -DPANDA_PLUGIN_VERSION="$(PANDA_PLUGIN_VERSION)" $(WFLAGS) -I.';; \
	esac;\
	if test -n "$$compile_options"; then \
	  $(I386_CLANG4_PLUGIN_COMPILER) $$compile_options -c $(top_srcdir)/etc/clang_plugin/dumpGimple.cpp -o clang4_dumpGimple.o;\
	fi;

if BUILD_PTAA
clang4_HardekopfLin_AA.o : $(top_srcdir)/etc/clang_plugin/HardekopfLin_AA.cpp $(top_srcdir)/etc/clang_plugin/HardekopfLin_AA.hpp
	case $(host_os) in \
	  mingw*) \
	    compile_options=' -std=c++11 -fno-rtti -O2 -I$(I386_LLVM4_HEADER_DIR) $(WFLAGS) -I.'; \
	    compile_options='';; \
	  *) \
	    compile_options=' -std=c++11 -fPIC -fno-rtti -O2 -I$(I386_LLVM4_HEADER_DIR) $(WFLAGS) -I.';; \
	esac;\
	if test -n "$$compile_options"; then \
	  $(I386_CLANG4_PLUGIN_COMPILER) $$compile_options -c $(top_srcdir)/etc/clang_plugin/HardekopfLin_AA.cpp -o clang4_HardekopfLin_AA.o ; \
	fi;
endif

clang4_RangeAnalysis.o : $(top_srcdir)/etc/clang_plugin/llvm/RangeAnalysis.cpp $(top_srcdir)/etc/clang_plugin/llvm/RangeAnalysis.hpp
	case $(host_os) in \
	  mingw*) \
	    compile_options=' -std=c++11 -fno-rtti -O2 -I$(I386_LLVM4_HEADER_DIR) $(WFLAGS) -I.'; \
	    compile_options='';; \
	  *) \
	    compile_options=' -std=c++11 -fPIC -fno-rtti -O2 -I$(I386_LLVM4_HEADER_DIR) $(WFLAGS) -I.';; \
	esac;\
	if test -n "$$compile_options"; then \
	    $(I386_CLANG4_PLUGIN_COMPILER) $$compile_options -c $(top_srcdir)/etc/clang_plugin/llvm/RangeAnalysis.cpp -o clang4_RangeAnalysis.o; \
	fi;

clang4_eSSA.o : $(top_srcdir)/etc/clang_plugin/llvm/eSSA.cpp $(top_srcdir)/etc/clang_plugin/llvm/eSSA.hpp $(top_srcdir)/etc/clang_plugin/llvm/my_OrderedInstructions.hpp
	case $(host_os) in \
	  mingw*) \
	    compile_options=' -std=c++11 -fno-rtti -O2 -I$(I386_LLVM4_HEADER_DIR) -DPANDA_PLUGIN_VERSION="$(PANDA_PLUGIN_VERSION)" $(WFLAGS) -I.'; \
	    compile_options='';; \
	  *) \
	    compile_options=' -std=c++11 -fPIC -fno-rtti -O2 -I$(I386_LLVM4_HEADER_DIR) -DPANDA_PLUGIN_VERSION="$(PANDA_PLUGIN_VERSION)" $(WFLAGS) -I.';; \
	esac;\
	if test -n "$$compile_options"; then \
	    $(I386_CLANG4_PLUGIN_COMPILER) $$compile_options -c $(top_srcdir)/etc/clang_plugin/llvm/eSSA.cpp -o clang4_eSSA.o; \
	fi;

clang4_CSROA.o : $(top_srcdir)/etc/clang_plugin/CSROA.cpp $(top_srcdir)/etc/clang_plugin/CSROA.hpp
	case $(host_os) in \
	  mingw*) \
	    compile_options=' -std=c++11 -fno-rtti -O2 -I$(I386_LLVM4_HEADER_DIR) -DPANDA_PLUGIN_VERSION="$(PANDA_PLUGIN_VERSION)" $(WFLAGS) -I.'; \
	    compile_options='';; \
	  *) \
	    compile_options=' -std=c++11 -fPIC -fno-rtti -O2 -I$(I386_LLVM4_HEADER_DIR) -DPANDA_PLUGIN_VERSION="$(PANDA_PLUGIN_VERSION)" $(WFLAGS) -I.';; \
	esac;\
	if test -n "$$compile_options"; then \
	    $(I386_CLANG4_PLUGIN_COMPILER) $$compile_options -c $(top_srcdir)/etc/clang_plugin/CSROA.cpp -o clang4_CSROA.o; \
	fi;

bambuS2S-4: $(top_srcdir)/etc/clang_plugin/bambuS2S.cpp
	case $(host_os) in \
	  darwin*) \
	    $(I386_CLANG4_PLUGIN_COMPILER) -fno-rtti -O2 -std=c++14 `$(I386_LLVM_CONFIG4_EXE) --cppflags --ldflags --libs --system-libs` $^ $(CLANG4_LIBSD)  -o $@;;\
	  *) \
	    $(I386_CLANG4_PLUGIN_COMPILER) -fno-rtti -O2 -std=c++14 `$(I386_LLVM_CONFIG4_EXE) --cppflags --ldflags --libs --system-libs` $^ $(CLANG4_LIBS)  -o $@;;\
	esac;


###### 

###### clang 5.0
if BUILD_I386_CLANG5_EMPTY_PLUGIN
all: $(I386_CLANG5_EMPTY_PLUGIN) bambuS2S-5
endif

if BUILD_I386_CLANG5_SSA_PLUGIN
all: $(I386_CLANG5_SSA_PLUGIN)
endif

if BUILD_I386_CLANG5_SSA_PLUGINCPP
all: $(I386_CLANG5_SSA_PLUGINCPP)
endif

if BUILD_I386_CLANG5_EXPANDMEMOPS_PLUGIN
all: $(I386_CLANG5_EXPANDMEMOPS_PLUGIN)
endif

if BUILD_I386_CLANG5_CSROA_PLUGIN
all: $(I386_CLANG5_CSROA_PLUGIN)
endif


if BUILD_I386_CLANG5_TOPFNAME_PLUGIN
all: $(I386_CLANG5_TOPFNAME_PLUGIN)
endif

if BUILD_I386_CLANG5_ASTANALYZER_PLUGIN
all: $(I386_CLANG5_ASTANALYZER_PLUGIN)
endif

if BUILD_PTAA
$(I386_CLANG5_SSA_PLUGIN) : clang5_plugin_dumpGimpleSSA.o clang5_dumpGimple.o clang5_HardekopfLin_AA.o clang5_RangeAnalysis.o clang5_eSSA.o
	case $(host_os) in \
	  mingw*) \
	    compile_options='$(CLANG5_LIBS) -lversion `$(I386_LLVM_CONFIG5_EXE) --ldflags --libs --system-libs` -lbdd';\
	    compile_options='';; \
	  darwin*) \
	    compile_options='-fPIC -shared -undefined dynamic_lookup -fno-rtti -lbdd';; \
	  *) \
	    compile_options='-fPIC -shared -fno-rtti -lbdd';;\
	esac;\
	obj_list='clang5_dumpGimple.o clang5_HardekopfLin_AA.o clang5_RangeAnalysis.o clang5_eSSA.o';\
	if test -n "$$compile_options"; then \
	  $(I386_CLANG5_PLUGIN_COMPILER) opt_clang5_plugin_dumpGimpleSSA.o $$obj_list $$compile_options -o clang5_plugin_dumpGimpleSSA_opt.so;\
	  $(I386_CLANG5_PLUGIN_COMPILER) clang5_plugin_dumpGimpleSSA.o $$obj_list $$compile_options -o clang5_plugin_dumpGimpleSSA.so ; \
	fi;
else
$(I386_CLANG5_SSA_PLUGIN) : clang5_plugin_dumpGimpleSSA.o clang5_dumpGimple.o clang5_RangeAnalysis.o clang5_eSSA.o
	case $(host_os) in \
	  mingw*) \
	    compile_options='$(CLANG5_LIBS) -lversion `$(I386_LLVM_CONFIG5_EXE) --ldflags --libs --system-libs`'; \
	    compile_options='';; \
	  darwin*) \
	    compile_options='-fPIC -shared -undefined dynamic_lookup -fno-rtti';; \
	  *) \
	    compile_options='-fPIC -shared -fno-rtti';;\
	esac;\
	obj_list='clang5_dumpGimple.o clang5_RangeAnalysis.o clang5_eSSA.o';\
	if test -n "$$compile_options"; then \
	  $(I386_CLANG5_PLUGIN_COMPILER) opt_clang5_plugin_dumpGimpleSSA.o $$obj_list $$compile_options -o clang5_plugin_dumpGimpleSSA_opt.so;\
	  $(I386_CLANG5_PLUGIN_COMPILER) clang5_plugin_dumpGimpleSSA.o $$obj_list $$compile_options -o clang5_plugin_dumpGimpleSSA.so ; \
	fi;
endif
if BUILD_PTAA
$(I386_CLANG5_SSA_PLUGINCPP) : clang5_plugin_dumpGimpleSSACpp.o clang5_dumpGimple.o clang5_HardekopfLin_AA.o clang5_RangeAnalysis.o clang5_eSSA.o
	case $(host_os) in \
	  mingw*) \
	    compile_options='$(CLANG5_LIBS) -lversion `$(I386_LLVM_CONFIG5_EXE) --ldflags --libs --system-libs` -lbdd'; \
	    compile_options='';; \
	  darwin*) \
	    compile_options='-fPIC -shared -undefined dynamic_lookup -fno-rtti -lbdd';; \
	  *) \
	    compile_options='-fPIC -shared -fno-rtti -lbdd';;\
	esac;\
	obj_list='clang5_dumpGimple.o clang5_HardekopfLin_AA.o clang5_RangeAnalysis.o clang5_eSSA.o';\
	if test -n "$$compile_options"; then \
	  $(I386_CLANG5_PLUGIN_COMPILER) opt_clang5_plugin_dumpGimpleSSACpp.o $$obj_list $$compile_options -o clang5_plugin_dumpGimpleSSACpp_opt.so;\
	  $(I386_CLANG5_PLUGIN_COMPILER) clang5_plugin_dumpGimpleSSACpp.o $$obj_list $$compile_options -o clang5_plugin_dumpGimpleSSACpp.so;\
	fi;
else
$(I386_CLANG5_SSA_PLUGINCPP) : clang5_plugin_dumpGimpleSSACpp.o clang5_dumpGimple.o clang5_RangeAnalysis.o clang5_eSSA.o
	case $(host_os) in \
	  mingw*) \
	    compile_options='$(CLANG5_LIBS) -lversion `$(I386_LLVM_CONFIG5_EXE) --ldflags --libs --system-libs`'; \
	    compile_options='';; \
	  darwin*) \
	    compile_options='-fPIC -shared -undefined dynamic_lookup -fno-rtti';; \
	  *) \
	    compile_options='-fPIC -shared -fno-rtti';;\
	esac;\
	obj_list='clang5_dumpGimple.o clang5_RangeAnalysis.o clang5_eSSA.o';\
	if test -n "$$compile_options"; then \
	  $(I386_CLANG5_PLUGIN_COMPILER) opt_clang5_plugin_dumpGimpleSSACpp.o $$obj_list $$compile_options -o clang5_plugin_dumpGimpleSSACpp_opt.so;\
	  $(I386_CLANG5_PLUGIN_COMPILER) clang5_plugin_dumpGimpleSSACpp.o $$obj_list $$compile_options -o clang5_plugin_dumpGimpleSSACpp.so;\
	fi;
endif

if BUILD_PTAA
$(I386_CLANG5_EMPTY_PLUGIN) : clang5_plugin_dumpGimpleEmpty.o clang5_dumpGimple.o clang5_HardekopfLin_AA.o clang5_RangeAnalysis.o clang5_eSSA.o
	case $(host_os) in \
	  mingw*) \
	    compile_options='$(CLANG5_LIBS) -lversion `$(I386_LLVM_CONFIG5_EXE) --ldflags --libs --system-libs` -lbdd'; \
	    compile_options='';; \
	  darwin*) \
	    compile_options='-fPIC -shared -undefined dynamic_lookup -fno-rtti -lbdd';; \
	  *) \
	    compile_options='-fPIC -shared -fno-rtti -lbdd';;\
	esac;\
	obj_list='clang5_dumpGimple.o clang5_HardekopfLin_AA.o clang5_RangeAnalysis.o clang5_eSSA.o';\
	if test -n "$$compile_options"; then \
	  $(I386_CLANG5_PLUGIN_COMPILER) opt_clang5_plugin_dumpGimpleEmpty.o $$obj_list $$compile_options -o clang5_plugin_dumpGimpleEmpty_opt.so; \
	  $(I386_CLANG5_PLUGIN_COMPILER) clang5_plugin_dumpGimpleEmpty.o $$obj_list $$compile_options -o clang5_plugin_dumpGimpleEmpty.so; \
	fi;
else
$(I386_CLANG5_EMPTY_PLUGIN) : clang5_plugin_dumpGimpleEmpty.o clang5_dumpGimple.o clang5_RangeAnalysis.o clang5_eSSA.o
	case $(host_os) in \
	  mingw*) \
	    compile_options='$(CLANG5_LIBS) -lversion `$(I386_LLVM_CONFIG5_EXE) --ldflags --libs --system-libs`'; \
	    compile_options='';; \
	  darwin*) \
	    compile_options='-fPIC -shared -undefined dynamic_lookup -fno-rtti';; \
	  *) \
	    compile_options='-fPIC -shared -fno-rtti';;\
	esac;\
	obj_list='clang5_dumpGimple.o clang5_RangeAnalysis.o clang5_eSSA.o';\
	if test -n "$$compile_options"; then \
	  $(I386_CLANG5_PLUGIN_COMPILER) opt_clang5_plugin_dumpGimpleEmpty.o $$obj_list $$compile_options -o clang5_plugin_dumpGimpleEmpty_opt.so; \
	  $(I386_CLANG5_PLUGIN_COMPILER) clang5_plugin_dumpGimpleEmpty.o $$obj_list $$compile_options -o clang5_plugin_dumpGimpleEmpty.so; \
	fi;
endif

$(I386_CLANG5_EXPANDMEMOPS_PLUGIN) : clang5_plugin_expandMemOps.o 
	case $(host_os) in \
	  mingw*) \
	    compile_options='$(CLANG5_LIBS) -lversion `$(I386_LLVM_CONFIG5_EXE) --ldflags --libs --system-libs`'; \
	    compile_options='';; \
	  darwin*) \
	    compile_options='-fPIC -shared -undefined dynamic_lookup -fno-rtti';; \
	  *) \
	    compile_options='-fPIC -shared -fno-rtti';;\
	esac;\
	if test -n "$$compile_options"; then \
	  $(I386_CLANG5_PLUGIN_COMPILER) opt_clang5_plugin_expandMemOps.o $$compile_options -o clang5_plugin_expandMemOps_opt.so; \
	  $(I386_CLANG5_PLUGIN_COMPILER) clang5_plugin_expandMemOps.o $$compile_options -o clang5_plugin_expandMemOps.so; \
	fi;

$(I386_CLANG5_CSROA_PLUGIN) : clang5_plugin_CSROA.o clang5_CSROA.o
	case $(host_os) in \
	  mingw*) \
	    compile_options='$(CLANG5_LIBS) -lversion `$(I386_LLVM_CONFIG5_EXE) --ldflags --libs --system-libs`'; \
	    compile_options='';; \
	  darwin*) \
	    compile_options='-fPIC -shared -undefined dynamic_lookup -fno-rtti';; \
	  *) \
	    compile_options='-fPIC -shared -fno-rtti';;\
	esac;\
	if test -n "$$compile_options"; then \
	  $(I386_CLANG5_PLUGIN_COMPILER) opt_clang5_plugin_CSROA.o clang5_CSROA.o $$compile_options -o clang5_plugin_CSROA_opt.so; \
	  $(I386_CLANG5_PLUGIN_COMPILER) clang5_plugin_CSROA.o clang5_CSROA.o $$compile_options -o clang5_plugin_CSROA.so; \
	fi;

$(I386_CLANG5_TOPFNAME_PLUGIN) : clang5_plugin_topfname.o 
	case $(host_os) in \
	  mingw*) \
	    compile_options='$(CLANG5_LIBS) -lversion `$(I386_LLVM_CONFIG5_EXE) --ldflags --libs --system-libs`'; \
	    compile_options='';; \
	  darwin*) \
	    compile_options='-fPIC -shared -undefined dynamic_lookup -fno-rtti';; \
	  *) \
	    compile_options='-fPIC -shared -fno-rtti';;\
	esac;\
	if test -n "$$compile_options"; then \
	  $(I386_CLANG5_PLUGIN_COMPILER) opt_clang5_plugin_topfname.o $$compile_options -o clang5_plugin_topfname_opt.so; \
	  $(I386_CLANG5_PLUGIN_COMPILER) clang5_plugin_topfname.o $$compile_options -o clang5_plugin_topfname.so; \
	fi;

$(I386_CLANG5_ASTANALYZER_PLUGIN) : clang5_plugin_ASTAnalyzer.o 
	case $(host_os) in \
	  mingw*) \
	    compile_options='$(CLANG5_LIBS) -lversion `$(I386_LLVM_CONFIG5_EXE) --ldflags --libs --system-libs`'; \
	    compile_options='';; \
	  darwin*) \
	    compile_options='-fPIC -shared -undefined dynamic_lookup -fno-rtti';; \
	  *) \
	    compile_options='-fPIC -shared -fno-rtti';;\
	esac;\
	if test -n "$$compile_options"; then \
	  $(I386_CLANG5_PLUGIN_COMPILER) clang5_plugin_ASTAnalyzer.o $$compile_options -o clang5_plugin_ASTAnalyzer.so; \
	fi;

clang5_plugin_dumpGimpleEmpty.o : $(top_srcdir)/etc/clang_plugin/plugin_dumpGimpleEmpty.cpp $(top_srcdir)/etc/clang_plugin/plugin_includes.hpp 
	case $(host_os) in \
	  mingw*) \
	    compile_options=' -std=c++11 -fno-rtti -O2 -I$(I386_LLVM5_HEADER_DIR) $(WFLAGS) -I.'; \
	    compile_options='';; \
	  *) \
	    compile_options=' -std=c++11 -fPIC -fno-rtti -O2 -I$(I386_LLVM5_HEADER_DIR) $(WFLAGS) -I.';; \
	esac;\
	if test -n "$$compile_options"; then \
	    $(I386_CLANG5_PLUGIN_COMPILER) -DADD_RSP=0 $$compile_options -c $(top_srcdir)/etc/clang_plugin/plugin_dumpGimpleEmpty.cpp -o opt_clang5_plugin_dumpGimpleEmpty.o; \
	    $(I386_CLANG5_PLUGIN_COMPILER) -DADD_RSP=1 $$compile_options -c $(top_srcdir)/etc/clang_plugin/plugin_dumpGimpleEmpty.cpp -o clang5_plugin_dumpGimpleEmpty.o; \
	fi;

clang5_plugin_dumpGimpleSSA.o : $(top_srcdir)/etc/clang_plugin/plugin_dumpGimpleSSA.cpp $(top_srcdir)/etc/clang_plugin/plugin_includes.hpp 
	case $(host_os) in \
	  mingw*) \
	    compile_options=' -std=c++11 -fno-rtti -O2 -I$(I386_LLVM5_HEADER_DIR) $(WFLAGS) -I.'; \
	    compile_options='';; \
	  *) \
	    compile_options=' -std=c++11 -fPIC -fno-rtti -O2 -I$(I386_LLVM5_HEADER_DIR) $(WFLAGS) -I.';; \
	esac;\
	if test -n "$$compile_options"; then \
	  $(I386_CLANG5_PLUGIN_COMPILER) -DADD_RSP=0 $$compile_options -c $(top_srcdir)/etc/clang_plugin/plugin_dumpGimpleSSA.cpp -o opt_clang5_plugin_dumpGimpleSSA.o;\
	  $(I386_CLANG5_PLUGIN_COMPILER) -DADD_RSP=1 $$compile_options -c $(top_srcdir)/etc/clang_plugin/plugin_dumpGimpleSSA.cpp -o clang5_plugin_dumpGimpleSSA.o;\
	fi;

clang5_plugin_dumpGimpleSSACpp.o : $(top_srcdir)/etc/clang_plugin/plugin_dumpGimpleSSA.cpp $(top_srcdir)/etc/clang_plugin/plugin_includes.hpp 
	case $(host_os) in \
	  mingw*) \
	    compile_options=' -std=c++11 -fno-rtti -O2 -I$(I386_LLVM5_HEADER_DIR) $(WFLAGS) -I. -DCPP_LANGUAGE'; \
	    compile_options='';; \
	  *) \
	    compile_options=' -std=c++11 -fPIC -fno-rtti -O2 -I$(I386_LLVM5_HEADER_DIR) $(WFLAGS) -I. -DCPP_LANGUAGE';; \
	esac;\
	if test -n "$$compile_options"; then \
	  $(I386_CLANG5_PLUGIN_COMPILER) -DADD_RSP=0 $$compile_options -c $(top_srcdir)/etc/clang_plugin/plugin_dumpGimpleSSA.cpp -o opt_clang5_plugin_dumpGimpleSSACpp.o;\
	  $(I386_CLANG5_PLUGIN_COMPILER) -DADD_RSP=1 $$compile_options -c $(top_srcdir)/etc/clang_plugin/plugin_dumpGimpleSSA.cpp -o clang5_plugin_dumpGimpleSSACpp.o;\
	fi;

clang5_plugin_expandMemOps.o : $(top_srcdir)/etc/clang_plugin/plugin_expandMemOps.cpp $(top_srcdir)/etc/clang_plugin/plugin_includes.hpp 
	case $(host_os) in \
	  mingw*) \
	    compile_options=' -std=c++11 -fno-rtti -O2 -I$(I386_LLVM5_HEADER_DIR) $(WFLAGS) -I.'; \
	    compile_options='';; \
	  *) \
	    compile_options=' -std=c++11 -fPIC -fno-rtti -O2 -I$(I386_LLVM5_HEADER_DIR) $(WFLAGS) -I.';; \
	esac;\
	if test -n "$$compile_options"; then \
	  $(I386_CLANG5_PLUGIN_COMPILER) -DADD_RSP=0 $$compile_options -c $(top_srcdir)/etc/clang_plugin/plugin_expandMemOps.cpp -o opt_clang5_plugin_expandMemOps.o; \
	  $(I386_CLANG5_PLUGIN_COMPILER) -DADD_RSP=1 $$compile_options -c $(top_srcdir)/etc/clang_plugin/plugin_expandMemOps.cpp -o clang5_plugin_expandMemOps.o; \
	fi;

clang5_plugin_CSROA.o : $(top_srcdir)/etc/clang_plugin/plugin_CSROA.cpp $(top_srcdir)/etc/clang_plugin/plugin_includes.hpp 
	case $(host_os) in \
	  mingw*) \
	    compile_options=' -std=c++11 -fno-rtti -O2 -I$(I386_LLVM5_HEADER_DIR) $(WFLAGS) -I.'; \
	    compile_options='';; \
	  *) \
	    compile_options=' -std=c++11 -fPIC -fno-rtti -O2 -I$(I386_LLVM5_HEADER_DIR) $(WFLAGS) -I.';; \
	esac;\
	if test -n "$$compile_options"; then \
	  $(I386_CLANG5_PLUGIN_COMPILER) -DADD_RSP=0 $$compile_options -c $(top_srcdir)/etc/clang_plugin/plugin_CSROA.cpp -o opt_clang5_plugin_CSROA.o; \
	  $(I386_CLANG5_PLUGIN_COMPILER) -DADD_RSP=1 $$compile_options -c $(top_srcdir)/etc/clang_plugin/plugin_CSROA.cpp -o clang5_plugin_CSROA.o; \
	fi;

clang5_plugin_topfname.o : $(top_srcdir)/etc/clang_plugin/plugin_topfname.cpp $(top_srcdir)/etc/clang_plugin/plugin_includes.hpp 
	case $(host_os) in \
	  mingw*) \
	    compile_options=' -std=c++11 -fno-rtti -O2 -I$(I386_LLVM5_HEADER_DIR) $(WFLAGS) -I.'; \
	    compile_options='';; \
	  *) \
	    compile_options=' -std=c++11 -fPIC -fno-rtti -O2 -I$(I386_LLVM5_HEADER_DIR) $(WFLAGS) -I.';; \
	esac;\
	if test -n "$$compile_options"; then \
	  $(I386_CLANG5_PLUGIN_COMPILER) -DADD_RSP=0 $$compile_options -c $(top_srcdir)/etc/clang_plugin/plugin_topfname.cpp -o opt_clang5_plugin_topfname.o; \
	  $(I386_CLANG5_PLUGIN_COMPILER) -DADD_RSP=1 $$compile_options -c $(top_srcdir)/etc/clang_plugin/plugin_topfname.cpp -o clang5_plugin_topfname.o; \
	fi;

clang5_plugin_ASTAnalyzer.o : $(top_srcdir)/etc/clang_plugin/plugin_ASTAnalyzer.cpp $(top_srcdir)/etc/clang_plugin/plugin_includes.hpp 
	case $(host_os) in \
	  mingw*) \
	    compile_options=' -std=c++11 -fno-rtti -O2 -I$(I386_LLVM5_HEADER_DIR) $(WFLAGS) -I.'; \
	    compile_options='';; \
	  *) \
	    compile_options=' -std=c++11 -fPIC -fno-rtti -O2 -I$(I386_LLVM5_HEADER_DIR) $(WFLAGS) -I.';; \
	esac;\
	if test -n "$$compile_options"; then \
	  $(I386_CLANG5_PLUGIN_COMPILER) $$compile_options -c $(top_srcdir)/etc/clang_plugin/plugin_ASTAnalyzer.cpp -o clang5_plugin_ASTAnalyzer.o;\
	fi;

clang5_dumpGimple.o : $(top_srcdir)/etc/clang_plugin/dumpGimple.cpp $(top_srcdir)/etc/clang_plugin/plugin_includes.hpp $(top_srcdir)/etc/clang_plugin/HardekopfLin_AA.hpp $(top_srcdir)/etc/clang_plugin/llvm/RangeAnalysis.hpp  $(top_srcdir)/etc/clang_plugin/llvm/eSSA.hpp
	case $(host_os) in \
	  mingw*) \
	    compile_options=' -std=c++11 -fno-rtti -O2 -I$(I386_LLVM5_HEADER_DIR) -DPANDA_PLUGIN_VERSION="$(PANDA_PLUGIN_VERSION)" $(WFLAGS) -I.'; \
	    compile_options='';; \
	  *) \
	    compile_options=' -std=c++11 -fPIC -fno-rtti -O2 -I$(I386_LLVM5_HEADER_DIR) -DPANDA_PLUGIN_VERSION="$(PANDA_PLUGIN_VERSION)" $(WFLAGS) -I.';; \
	esac;\
	if test -n "$$compile_options"; then \
	  $(I386_CLANG5_PLUGIN_COMPILER) $$compile_options -c $(top_srcdir)/etc/clang_plugin/dumpGimple.cpp -o clang5_dumpGimple.o;\
	fi;

if BUILD_PTAA
clang5_HardekopfLin_AA.o : $(top_srcdir)/etc/clang_plugin/HardekopfLin_AA.cpp $(top_srcdir)/etc/clang_plugin/HardekopfLin_AA.hpp
	case $(host_os) in \
	  mingw*) \
	    compile_options=' -std=c++11 -fno-rtti -O2 -I$(I386_LLVM5_HEADER_DIR) $(WFLAGS) -I.'; \
	    compile_options='';; \
	  *) \
	    compile_options=' -std=c++11 -fPIC -fno-rtti -O2 -I$(I386_LLVM5_HEADER_DIR) $(WFLAGS) -I.';; \
	esac;\
	if test -n "$$compile_options"; then \
	  $(I386_CLANG5_PLUGIN_COMPILER) $$compile_options -c $(top_srcdir)/etc/clang_plugin/HardekopfLin_AA.cpp -o clang5_HardekopfLin_AA.o ; \
	fi;
endif

clang5_RangeAnalysis.o : $(top_srcdir)/etc/clang_plugin/llvm/RangeAnalysis.cpp $(top_srcdir)/etc/clang_plugin/llvm/RangeAnalysis.hpp
	case $(host_os) in \
	  mingw*) \
	    compile_options=' -std=c++11 -fno-rtti -O2 -I$(I386_LLVM5_HEADER_DIR) $(WFLAGS) -I.'; \
	    compile_options='';; \
	  *) \
	    compile_options=' -std=c++11 -fPIC -fno-rtti -O2 -I$(I386_LLVM5_HEADER_DIR) $(WFLAGS) -I.';; \
	esac;\
	if test -n "$$compile_options"; then \
	    $(I386_CLANG5_PLUGIN_COMPILER) $$compile_options -c $(top_srcdir)/etc/clang_plugin/llvm/RangeAnalysis.cpp -o clang5_RangeAnalysis.o; \
	fi;

clang5_eSSA.o : $(top_srcdir)/etc/clang_plugin/llvm/eSSA.cpp $(top_srcdir)/etc/clang_plugin/llvm/eSSA.hpp $(top_srcdir)/etc/clang_plugin/llvm/my_OrderedInstructions.hpp
	case $(host_os) in \
	  mingw*) \
	    compile_options=' -std=c++11 -fno-rtti -O2 -I$(I386_LLVM5_HEADER_DIR) -DPANDA_PLUGIN_VERSION="$(PANDA_PLUGIN_VERSION)" $(WFLAGS) -I.'; \
	    compile_options='';; \
	  *) \
	    compile_options=' -std=c++11 -fPIC -fno-rtti -O2 -I$(I386_LLVM5_HEADER_DIR) -DPANDA_PLUGIN_VERSION="$(PANDA_PLUGIN_VERSION)" $(WFLAGS) -I.';; \
	esac;\
	if test -n "$$compile_options"; then \
	    $(I386_CLANG5_PLUGIN_COMPILER) $$compile_options -c $(top_srcdir)/etc/clang_plugin/llvm/eSSA.cpp -o clang5_eSSA.o; \
	fi;

clang5_CSROA.o : $(top_srcdir)/etc/clang_plugin/CSROA.cpp $(top_srcdir)/etc/clang_plugin/CSROA.hpp
	case $(host_os) in \
	  mingw*) \
	    compile_options=' -std=c++11 -fno-rtti -O2 -I$(I386_LLVM5_HEADER_DIR) -DPANDA_PLUGIN_VERSION="$(PANDA_PLUGIN_VERSION)" $(WFLAGS) -I.'; \
	    compile_options='';; \
	  *) \
	    compile_options=' -std=c++11 -fPIC -fno-rtti -O2 -I$(I386_LLVM5_HEADER_DIR) -DPANDA_PLUGIN_VERSION="$(PANDA_PLUGIN_VERSION)" $(WFLAGS) -I.';; \
	esac;\
	if test -n "$$compile_options"; then \
	    $(I386_CLANG5_PLUGIN_COMPILER) $$compile_options -c $(top_srcdir)/etc/clang_plugin/CSROA.cpp -o clang5_CSROA.o; \
	fi;


bambuS2S-5: $(top_srcdir)/etc/clang_plugin/bambuS2S.cpp
	case $(host_os) in \
	  darwin*) \
	    $(I386_CLANG5_PLUGIN_COMPILER) -fno-rtti -O2 -std=c++14 `$(I386_LLVM_CONFIG5_EXE) --cppflags --ldflags --libs --system-libs` $^ $(CLANG5_LIBSD)  -o $@;;\
	  *) \
	    $(I386_CLANG5_PLUGIN_COMPILER) -fno-rtti -O2 -std=c++14 `$(I386_LLVM_CONFIG5_EXE) --cppflags --ldflags --libs --system-libs` $^ $(CLANG5_LIBS)  -o $@;;\
	esac;

###### 


###### clang 6.0
if BUILD_I386_CLANG6_EMPTY_PLUGIN
all: $(I386_CLANG6_EMPTY_PLUGIN) bambuS2S-6
endif

if BUILD_I386_CLANG6_SSA_PLUGIN
all: $(I386_CLANG6_SSA_PLUGIN)
endif

if BUILD_I386_CLANG6_SSA_PLUGINCPP
all: $(I386_CLANG6_SSA_PLUGINCPP)
endif

if BUILD_I386_CLANG6_EXPANDMEMOPS_PLUGIN
all: $(I386_CLANG6_EXPANDMEMOPS_PLUGIN)
endif

if BUILD_I386_CLANG6_CSROA_PLUGIN
all: $(I386_CLANG6_CSROA_PLUGIN)
endif

if BUILD_I386_CLANG6_TOPFNAME_PLUGIN
all: $(I386_CLANG6_TOPFNAME_PLUGIN)
endif

if BUILD_I386_CLANG6_ASTANALYZER_PLUGIN
all: $(I386_CLANG6_ASTANALYZER_PLUGIN)
endif

if BUILD_PTAA
$(I386_CLANG6_SSA_PLUGIN) : clang6_plugin_dumpGimpleSSA.o clang6_dumpGimple.o clang6_HardekopfLin_AA.o clang6_RangeAnalysis.o clang6_eSSA.o
	case $(host_os) in \
	  mingw*) \
	    compile_options='$(CLANG6_LIBS) -lversion `$(I386_LLVM_CONFIG6_EXE) --ldflags --libs --system-libs` -lbdd';\
	    compile_options='';; \
	  darwin*) \
	    compile_options='-fPIC -shared -undefined dynamic_lookup -fno-rtti -lbdd';; \
	  *) \
	    compile_options='-fPIC -shared -fno-rtti -lbdd';;\
	esac;\
	obj_list='clang6_dumpGimple.o clang6_HardekopfLin_AA.o clang6_RangeAnalysis.o clang6_eSSA.o';\
	if test -n "$$compile_options"; then \
	  $(I386_CLANG6_PLUGIN_COMPILER) opt_clang6_plugin_dumpGimpleSSA.o $$obj_list $$compile_options -o clang6_plugin_dumpGimpleSSA_opt.so;\
	  $(I386_CLANG6_PLUGIN_COMPILER) clang6_plugin_dumpGimpleSSA.o $$obj_list $$compile_options -o clang6_plugin_dumpGimpleSSA.so ; \
	fi;
else
$(I386_CLANG6_SSA_PLUGIN) : clang6_plugin_dumpGimpleSSA.o clang6_dumpGimple.o clang6_RangeAnalysis.o clang6_eSSA.o
	case $(host_os) in \
	  mingw*) \
	    compile_options='$(CLANG6_LIBS) -lversion `$(I386_LLVM_CONFIG6_EXE) --ldflags --libs --system-libs`'; \
	    compile_options='';; \
	  darwin*) \
	    compile_options='-fPIC -shared -undefined dynamic_lookup -fno-rtti';; \
	  *) \
	    compile_options='-fPIC -shared -fno-rtti';;\
	esac;\
	obj_list='clang6_dumpGimple.o clang6_RangeAnalysis.o clang6_eSSA.o';\
	if test -n "$$compile_options"; then \
	  $(I386_CLANG6_PLUGIN_COMPILER) opt_clang6_plugin_dumpGimpleSSA.o $$obj_list $$compile_options -o clang6_plugin_dumpGimpleSSA_opt.so;\
	  $(I386_CLANG6_PLUGIN_COMPILER) clang6_plugin_dumpGimpleSSA.o $$obj_list $$compile_options -o clang6_plugin_dumpGimpleSSA.so ; \
	fi;
endif
if BUILD_PTAA
$(I386_CLANG6_SSA_PLUGINCPP) : clang6_plugin_dumpGimpleSSACpp.o clang6_dumpGimple.o clang6_HardekopfLin_AA.o clang6_RangeAnalysis.o clang6_eSSA.o
	case $(host_os) in \
	  mingw*) \
	    compile_options='$(CLANG6_LIBS) -lversion `$(I386_LLVM_CONFIG6_EXE) --ldflags --libs --system-libs` -lbdd'; \
	    compile_options='';; \
	  darwin*) \
	    compile_options='-fPIC -shared -undefined dynamic_lookup -fno-rtti -lbdd';; \
	  *) \
	    compile_options='-fPIC -shared -fno-rtti -lbdd';;\
	esac;\
	obj_list='clang6_dumpGimple.o clang6_HardekopfLin_AA.o clang6_RangeAnalysis.o clang6_eSSA.o';\
	if test -n "$$compile_options"; then \
	  $(I386_CLANG6_PLUGIN_COMPILER) opt_clang6_plugin_dumpGimpleSSACpp.o $$obj_list $$compile_options -o clang6_plugin_dumpGimpleSSACpp_opt.so;\
	  $(I386_CLANG6_PLUGIN_COMPILER) clang6_plugin_dumpGimpleSSACpp.o $$obj_list $$compile_options -o clang6_plugin_dumpGimpleSSACpp.so;\
	fi;
else
$(I386_CLANG6_SSA_PLUGINCPP) : clang6_plugin_dumpGimpleSSACpp.o clang6_dumpGimple.o clang6_RangeAnalysis.o clang6_eSSA.o
	case $(host_os) in \
	  mingw*) \
	    compile_options='$(CLANG6_LIBS) -lversion `$(I386_LLVM_CONFIG6_EXE) --ldflags --libs --system-libs`'; \
	    compile_options='';; \
	  darwin*) \
	    compile_options='-fPIC -shared -undefined dynamic_lookup -fno-rtti';; \
	  *) \
	    compile_options='-fPIC -shared -fno-rtti';;\
	esac;\
	obj_list='clang6_dumpGimple.o clang6_RangeAnalysis.o clang6_eSSA.o';\
	if test -n "$$compile_options"; then \
	  $(I386_CLANG6_PLUGIN_COMPILER) opt_clang6_plugin_dumpGimpleSSACpp.o $$obj_list $$compile_options -o clang6_plugin_dumpGimpleSSACpp_opt.so;\
	  $(I386_CLANG6_PLUGIN_COMPILER) clang6_plugin_dumpGimpleSSACpp.o $$obj_list $$compile_options -o clang6_plugin_dumpGimpleSSACpp.so;\
	fi;
endif

if BUILD_PTAA
$(I386_CLANG6_EMPTY_PLUGIN) : clang6_plugin_dumpGimpleEmpty.o clang6_dumpGimple.o clang6_HardekopfLin_AA.o clang6_RangeAnalysis.o clang6_eSSA.o
	case $(host_os) in \
	  mingw*) \
	    compile_options='$(CLANG6_LIBS) -lversion `$(I386_LLVM_CONFIG6_EXE) --ldflags --libs --system-libs` -lbdd'; \
	    compile_options='';; \
	  darwin*) \
	    compile_options='-fPIC -shared -undefined dynamic_lookup -fno-rtti -lbdd';; \
	  *) \
	    compile_options='-fPIC -shared -fno-rtti -lbdd';;\
	esac;\
	obj_list='clang6_dumpGimple.o clang6_HardekopfLin_AA.o clang6_RangeAnalysis.o clang6_eSSA.o';\
	if test -n "$$compile_options"; then \
	  $(I386_CLANG6_PLUGIN_COMPILER) opt_clang6_plugin_dumpGimpleEmpty.o $$obj_list $$compile_options -o clang6_plugin_dumpGimpleEmpty_opt.so; \
	  $(I386_CLANG6_PLUGIN_COMPILER) clang6_plugin_dumpGimpleEmpty.o $$obj_list $$compile_options -o clang6_plugin_dumpGimpleEmpty.so; \
	fi;
else
$(I386_CLANG6_EMPTY_PLUGIN) : clang6_plugin_dumpGimpleEmpty.o clang6_dumpGimple.o clang6_RangeAnalysis.o clang6_eSSA.o
	case $(host_os) in \
	  mingw*) \
	    compile_options='$(CLANG6_LIBS) -lversion `$(I386_LLVM_CONFIG6_EXE) --ldflags --libs --system-libs`'; \
	    compile_options='';; \
	  darwin*) \
	    compile_options='-fPIC -shared -undefined dynamic_lookup -fno-rtti';; \
	  *) \
	    compile_options='-fPIC -shared -fno-rtti';;\
	esac;\
	obj_list='clang6_dumpGimple.o clang6_RangeAnalysis.o clang6_eSSA.o';\
	if test -n "$$compile_options"; then \
	  $(I386_CLANG6_PLUGIN_COMPILER) opt_clang6_plugin_dumpGimpleEmpty.o $$obj_list $$compile_options -o clang6_plugin_dumpGimpleEmpty_opt.so; \
	  $(I386_CLANG6_PLUGIN_COMPILER) clang6_plugin_dumpGimpleEmpty.o $$obj_list $$compile_options -o clang6_plugin_dumpGimpleEmpty.so; \
	fi;
endif

$(I386_CLANG6_EXPANDMEMOPS_PLUGIN) : clang6_plugin_expandMemOps.o 
	case $(host_os) in \
	  mingw*) \
	    compile_options='$(CLANG6_LIBS) -lversion `$(I386_LLVM_CONFIG6_EXE) --ldflags --libs --system-libs`'; \
	    compile_options='';; \
	  darwin*) \
	    compile_options='-fPIC -shared -undefined dynamic_lookup -fno-rtti';; \
	  *) \
	    compile_options='-fPIC -shared -fno-rtti';;\
	esac;\
	if test -n "$$compile_options"; then \
	  $(I386_CLANG6_PLUGIN_COMPILER) opt_clang6_plugin_expandMemOps.o $$compile_options -o clang6_plugin_expandMemOps_opt.so; \
	  $(I386_CLANG6_PLUGIN_COMPILER) clang6_plugin_expandMemOps.o $$compile_options -o clang6_plugin_expandMemOps.so; \
	fi;

$(I386_CLANG6_CSROA_PLUGIN) : clang6_plugin_CSROA.o clang6_CSROA.o
	case $(host_os) in \
	  mingw*) \
	    compile_options='$(CLANG6_LIBS) -lversion `$(I386_LLVM_CONFIG6_EXE) --ldflags --libs --system-libs`'; \
	    compile_options='';; \
	  darwin*) \
	    compile_options='-fPIC -shared -undefined dynamic_lookup -fno-rtti';; \
	  *) \
	    compile_options='-fPIC -shared -fno-rtti';;\
	esac;\
	if test -n "$$compile_options"; then \
	  $(I386_CLANG6_PLUGIN_COMPILER) opt_clang6_plugin_CSROA.o clang6_CSROA.o $$compile_options -o clang6_plugin_CSROA_opt.so; \
	  $(I386_CLANG6_PLUGIN_COMPILER) clang6_plugin_CSROA.o clang6_CSROA.o $$compile_options -o clang6_plugin_CSROA.so; \
	fi;

$(I386_CLANG6_TOPFNAME_PLUGIN) : clang6_plugin_topfname.o 
	case $(host_os) in \
	  mingw*) \
	    compile_options='$(CLANG6_LIBS) -lversion `$(I386_LLVM_CONFIG6_EXE) --ldflags --libs --system-libs`'; \
	    compile_options='';; \
	  darwin*) \
	    compile_options='-fPIC -shared -undefined dynamic_lookup -fno-rtti';; \
	  *) \
	    compile_options='-fPIC -shared -fno-rtti';;\
	esac;\
	if test -n "$$compile_options"; then \
	  $(I386_CLANG6_PLUGIN_COMPILER) opt_clang6_plugin_topfname.o $$compile_options -o clang6_plugin_topfname_opt.so; \
	  $(I386_CLANG6_PLUGIN_COMPILER) clang6_plugin_topfname.o $$compile_options -o clang6_plugin_topfname.so; \
	fi;

$(I386_CLANG6_ASTANALYZER_PLUGIN) : clang6_plugin_ASTAnalyzer.o 
	case $(host_os) in \
	  mingw*) \
	    compile_options='$(CLANG6_LIBS) -lversion `$(I386_LLVM_CONFIG6_EXE) --ldflags --libs --system-libs`'; \
	    compile_options='';; \
	  darwin*) \
	    compile_options='-fPIC -shared -undefined dynamic_lookup -fno-rtti';; \
	  *) \
	    compile_options='-fPIC -shared -fno-rtti';;\
	esac;\
	if test -n "$$compile_options"; then \
	  $(I386_CLANG6_PLUGIN_COMPILER) clang6_plugin_ASTAnalyzer.o $$compile_options -o clang6_plugin_ASTAnalyzer.so; \
	fi;

clang6_plugin_dumpGimpleEmpty.o : $(top_srcdir)/etc/clang_plugin/plugin_dumpGimpleEmpty.cpp $(top_srcdir)/etc/clang_plugin/plugin_includes.hpp 
	case $(host_os) in \
	  mingw*) \
	    compile_options=' -std=c++11 -fno-rtti -O2 -I$(I386_LLVM6_HEADER_DIR) $(WFLAGS) -I.'; \
	    compile_options='';; \
	  *) \
	    compile_options=' -std=c++11 -fPIC -fno-rtti -O2 -I$(I386_LLVM6_HEADER_DIR) $(WFLAGS) -I.';; \
	esac;\
	if test -n "$$compile_options"; then \
	    $(I386_CLANG6_PLUGIN_COMPILER) -DADD_RSP=0 $$compile_options -c $(top_srcdir)/etc/clang_plugin/plugin_dumpGimpleEmpty.cpp -o opt_clang6_plugin_dumpGimpleEmpty.o; \
	    $(I386_CLANG6_PLUGIN_COMPILER) -DADD_RSP=1 $$compile_options -c $(top_srcdir)/etc/clang_plugin/plugin_dumpGimpleEmpty.cpp -o clang6_plugin_dumpGimpleEmpty.o; \
	fi;

clang6_plugin_dumpGimpleSSA.o : $(top_srcdir)/etc/clang_plugin/plugin_dumpGimpleSSA.cpp $(top_srcdir)/etc/clang_plugin/plugin_includes.hpp 
	case $(host_os) in \
	  mingw*) \
	    compile_options=' -std=c++11 -fno-rtti -O2 -I$(I386_LLVM6_HEADER_DIR) $(WFLAGS) -I.'; \
	    compile_options='';; \
	  *) \
	    compile_options=' -std=c++11 -fPIC -fno-rtti -O2 -I$(I386_LLVM6_HEADER_DIR) $(WFLAGS) -I.';; \
	esac;\
	if test -n "$$compile_options"; then \
	  $(I386_CLANG6_PLUGIN_COMPILER) -DADD_RSP=0 $$compile_options -c $(top_srcdir)/etc/clang_plugin/plugin_dumpGimpleSSA.cpp -o opt_clang6_plugin_dumpGimpleSSA.o;\
	  $(I386_CLANG6_PLUGIN_COMPILER) -DADD_RSP=1 $$compile_options -c $(top_srcdir)/etc/clang_plugin/plugin_dumpGimpleSSA.cpp -o clang6_plugin_dumpGimpleSSA.o;\
	fi;

clang6_plugin_dumpGimpleSSACpp.o : $(top_srcdir)/etc/clang_plugin/plugin_dumpGimpleSSA.cpp $(top_srcdir)/etc/clang_plugin/plugin_includes.hpp 
	case $(host_os) in \
	  mingw*) \
	    compile_options=' -std=c++11 -fno-rtti -O2 -I$(I386_LLVM6_HEADER_DIR) $(WFLAGS) -I. -DCPP_LANGUAGE'; \
	    compile_options='';; \
	  *) \
	    compile_options=' -std=c++11 -fPIC -fno-rtti -O2 -I$(I386_LLVM6_HEADER_DIR) $(WFLAGS) -I. -DCPP_LANGUAGE';; \
	esac;\
	if test -n "$$compile_options"; then \
	  $(I386_CLANG6_PLUGIN_COMPILER) -DADD_RSP=0 $$compile_options -c $(top_srcdir)/etc/clang_plugin/plugin_dumpGimpleSSA.cpp -o opt_clang6_plugin_dumpGimpleSSACpp.o;\
	  $(I386_CLANG6_PLUGIN_COMPILER) -DADD_RSP=1 $$compile_options -c $(top_srcdir)/etc/clang_plugin/plugin_dumpGimpleSSA.cpp -o clang6_plugin_dumpGimpleSSACpp.o;\
	fi;

clang6_plugin_expandMemOps.o : $(top_srcdir)/etc/clang_plugin/plugin_expandMemOps.cpp $(top_srcdir)/etc/clang_plugin/plugin_includes.hpp 
	case $(host_os) in \
	  mingw*) \
	    compile_options=' -std=c++11 -fno-rtti -O2 -I$(I386_LLVM6_HEADER_DIR) $(WFLAGS) -I.'; \
	    compile_options='';; \
	  *) \
	    compile_options=' -std=c++11 -fPIC -fno-rtti -O2 -I$(I386_LLVM6_HEADER_DIR) $(WFLAGS) -I.';; \
	esac;\
	if test -n "$$compile_options"; then \
	  $(I386_CLANG6_PLUGIN_COMPILER) -DADD_RSP=0 $$compile_options -c $(top_srcdir)/etc/clang_plugin/plugin_expandMemOps.cpp -o opt_clang6_plugin_expandMemOps.o; \
	  $(I386_CLANG6_PLUGIN_COMPILER) -DADD_RSP=1 $$compile_options -c $(top_srcdir)/etc/clang_plugin/plugin_expandMemOps.cpp -o clang6_plugin_expandMemOps.o; \
	fi;

clang6_plugin_CSROA.o : $(top_srcdir)/etc/clang_plugin/plugin_CSROA.cpp $(top_srcdir)/etc/clang_plugin/plugin_includes.hpp 
	case $(host_os) in \
	  mingw*) \
	    compile_options=' -std=c++11 -fno-rtti -O2 -I$(I386_LLVM6_HEADER_DIR) $(WFLAGS) -I.'; \
	    compile_options='';; \
	  *) \
	    compile_options=' -std=c++11 -fPIC -fno-rtti -O2 -I$(I386_LLVM6_HEADER_DIR) $(WFLAGS) -I.';; \
	esac;\
	if test -n "$$compile_options"; then \
	  $(I386_CLANG6_PLUGIN_COMPILER) -DADD_RSP=0 $$compile_options -c $(top_srcdir)/etc/clang_plugin/plugin_CSROA.cpp -o opt_clang6_plugin_CSROA.o; \
	  $(I386_CLANG6_PLUGIN_COMPILER) -DADD_RSP=1 $$compile_options -c $(top_srcdir)/etc/clang_plugin/plugin_CSROA.cpp -o clang6_plugin_CSROA.o; \
	fi;

clang6_plugin_topfname.o : $(top_srcdir)/etc/clang_plugin/plugin_topfname.cpp $(top_srcdir)/etc/clang_plugin/plugin_includes.hpp 
	case $(host_os) in \
	  mingw*) \
	    compile_options=' -std=c++11 -fno-rtti -O2 -I$(I386_LLVM6_HEADER_DIR) $(WFLAGS) -I.'; \
	    compile_options='';; \
	  *) \
	    compile_options=' -std=c++11 -fPIC -fno-rtti -O2 -I$(I386_LLVM6_HEADER_DIR) $(WFLAGS) -I.';; \
	esac;\
	if test -n "$$compile_options"; then \
	  $(I386_CLANG6_PLUGIN_COMPILER) -DADD_RSP=0 $$compile_options -c $(top_srcdir)/etc/clang_plugin/plugin_topfname.cpp -o opt_clang6_plugin_topfname.o; \
	  $(I386_CLANG6_PLUGIN_COMPILER) -DADD_RSP=1 $$compile_options -c $(top_srcdir)/etc/clang_plugin/plugin_topfname.cpp -o clang6_plugin_topfname.o; \
	fi;

clang6_plugin_ASTAnalyzer.o : $(top_srcdir)/etc/clang_plugin/plugin_ASTAnalyzer.cpp $(top_srcdir)/etc/clang_plugin/plugin_includes.hpp 
	case $(host_os) in \
	  mingw*) \
	    compile_options=' -std=c++11 -fno-rtti -O2 -I$(I386_LLVM6_HEADER_DIR) $(WFLAGS) -I.'; \
	    compile_options='';; \
	  *) \
	    compile_options=' -std=c++11 -fPIC -fno-rtti -O2 -I$(I386_LLVM6_HEADER_DIR) $(WFLAGS) -I.';; \
	esac;\
	if test -n "$$compile_options"; then \
	  $(I386_CLANG6_PLUGIN_COMPILER) $$compile_options -c $(top_srcdir)/etc/clang_plugin/plugin_ASTAnalyzer.cpp -o clang6_plugin_ASTAnalyzer.o;\
	fi;

clang6_dumpGimple.o : $(top_srcdir)/etc/clang_plugin/dumpGimple.cpp $(top_srcdir)/etc/clang_plugin/plugin_includes.hpp $(top_srcdir)/etc/clang_plugin/HardekopfLin_AA.hpp $(top_srcdir)/etc/clang_plugin/llvm/RangeAnalysis.hpp  $(top_srcdir)/etc/clang_plugin/llvm/eSSA.hpp
	case $(host_os) in \
	  mingw*) \
	    compile_options=' -std=c++11 -fno-rtti -O2 -I$(I386_LLVM6_HEADER_DIR) -DPANDA_PLUGIN_VERSION="$(PANDA_PLUGIN_VERSION)" $(WFLAGS) -I.'; \
	    compile_options='';; \
	  *) \
	    compile_options=' -std=c++11 -fPIC -fno-rtti -O2 -I$(I386_LLVM6_HEADER_DIR) -DPANDA_PLUGIN_VERSION="$(PANDA_PLUGIN_VERSION)" $(WFLAGS) -I.';; \
	esac;\
	if test -n "$$compile_options"; then \
	  $(I386_CLANG6_PLUGIN_COMPILER) $$compile_options -c $(top_srcdir)/etc/clang_plugin/dumpGimple.cpp -o clang6_dumpGimple.o;\
	fi;

if BUILD_PTAA
clang6_HardekopfLin_AA.o : $(top_srcdir)/etc/clang_plugin/HardekopfLin_AA.cpp $(top_srcdir)/etc/clang_plugin/HardekopfLin_AA.hpp
	case $(host_os) in \
	  mingw*) \
	    compile_options=' -std=c++11 -fno-rtti -O2 -I$(I386_LLVM6_HEADER_DIR) $(WFLAGS) -I.'; \
	    compile_options='';; \
	  *) \
	    compile_options=' -std=c++11 -fPIC -fno-rtti -O2 -I$(I386_LLVM6_HEADER_DIR) $(WFLAGS) -I.';; \
	esac;\
	if test -n "$$compile_options"; then \
	  $(I386_CLANG6_PLUGIN_COMPILER) $$compile_options -c $(top_srcdir)/etc/clang_plugin/HardekopfLin_AA.cpp -o clang6_HardekopfLin_AA.o ; \
	fi;
endif

clang6_RangeAnalysis.o : $(top_srcdir)/etc/clang_plugin/llvm/RangeAnalysis.cpp $(top_srcdir)/etc/clang_plugin/llvm/RangeAnalysis.hpp
	case $(host_os) in \
	  mingw*) \
	    compile_options=' -std=c++11 -fno-rtti -O2 -I$(I386_LLVM6_HEADER_DIR) $(WFLAGS) -I.'; \
	    compile_options='';; \
	  *) \
	    compile_options=' -std=c++11 -fPIC -fno-rtti -O2 -I$(I386_LLVM6_HEADER_DIR) $(WFLAGS) -I.';; \
	esac;\
	if test -n "$$compile_options"; then \
	    $(I386_CLANG6_PLUGIN_COMPILER) $$compile_options -c $(top_srcdir)/etc/clang_plugin/llvm/RangeAnalysis.cpp -o clang6_RangeAnalysis.o; \
	fi;

clang6_eSSA.o : $(top_srcdir)/etc/clang_plugin/llvm/eSSA.cpp $(top_srcdir)/etc/clang_plugin/llvm/eSSA.hpp $(top_srcdir)/etc/clang_plugin/llvm/my_OrderedInstructions.hpp
	case $(host_os) in \
	  mingw*) \
	    compile_options=' -std=c++11 -fno-rtti -O2 -I$(I386_LLVM6_HEADER_DIR) -DPANDA_PLUGIN_VERSION="$(PANDA_PLUGIN_VERSION)" $(WFLAGS) -I.'; \
	    compile_options='';; \
	  *) \
	    compile_options=' -std=c++11 -fPIC -fno-rtti -O2 -I$(I386_LLVM6_HEADER_DIR) -DPANDA_PLUGIN_VERSION="$(PANDA_PLUGIN_VERSION)" $(WFLAGS) -I.';; \
	esac;\
	if test -n "$$compile_options"; then \
	    $(I386_CLANG6_PLUGIN_COMPILER) $$compile_options -c $(top_srcdir)/etc/clang_plugin/llvm/eSSA.cpp -o clang6_eSSA.o; \
	fi;

clang6_CSROA.o : $(top_srcdir)/etc/clang_plugin/CSROA.cpp $(top_srcdir)/etc/clang_plugin/CSROA.hpp
	case $(host_os) in \
	  mingw*) \
	    compile_options=' -std=c++11 -fno-rtti -O2 -I$(I386_LLVM6_HEADER_DIR) -DPANDA_PLUGIN_VERSION="$(PANDA_PLUGIN_VERSION)" $(WFLAGS) -I.'; \
	    compile_options='';; \
	  *) \
	    compile_options=' -std=c++11 -fPIC -fno-rtti -O2 -I$(I386_LLVM6_HEADER_DIR) -DPANDA_PLUGIN_VERSION="$(PANDA_PLUGIN_VERSION)" $(WFLAGS) -I.';; \
	esac;\
	if test -n "$$compile_options"; then \
	    $(I386_CLANG6_PLUGIN_COMPILER) $$compile_options -c $(top_srcdir)/etc/clang_plugin/CSROA.cpp -o clang6_CSROA.o; \
	fi;

bambuS2S-6: $(top_srcdir)/etc/clang_plugin/bambuS2S.cpp
	case $(host_os) in \
	  darwin*) \
	    $(I386_CLANG6_PLUGIN_COMPILER) -fno-rtti -O2 -std=c++14 `$(I386_LLVM_CONFIG6_EXE) --cppflags --ldflags --libs --system-libs` $^ $(CLANG6_LIBSD)  -o $@;;\
	  *) \
	    $(I386_CLANG6_PLUGIN_COMPILER) -fno-rtti -O2 -std=c++14 `$(I386_LLVM_CONFIG6_EXE) --cppflags --ldflags --libs --system-libs` $^ $(CLANG6_LIBS)  -o $@;;\
	esac;

###### 

###### clang 7.0
if BUILD_I386_CLANG7_EMPTY_PLUGIN
all: $(I386_CLANG7_EMPTY_PLUGIN) bambuS2S-7
endif

if BUILD_I386_CLANG7_SSA_PLUGIN
all: $(I386_CLANG7_SSA_PLUGIN)
endif

if BUILD_I386_CLANG7_SSA_PLUGINCPP
all: $(I386_CLANG7_SSA_PLUGINCPP)
endif

if BUILD_I386_CLANG7_EXPANDMEMOPS_PLUGIN
all: $(I386_CLANG7_EXPANDMEMOPS_PLUGIN)
endif

if BUILD_I386_CLANG7_CSROA_PLUGIN
all: $(I386_CLANG7_CSROA_PLUGIN)
endif

if BUILD_I386_CLANG7_TOPFNAME_PLUGIN
all: $(I386_CLANG7_TOPFNAME_PLUGIN)
endif

if BUILD_I386_CLANG7_ASTANALYZER_PLUGIN
all: $(I386_CLANG7_ASTANALYZER_PLUGIN)
endif

if BUILD_PTAA
$(I386_CLANG7_SSA_PLUGIN) : opt_clang7_plugin_dumpGimpleSSA.o clang7_plugin_dumpGimpleSSA.o clang7_dumpGimple.o clang7_HardekopfLin_AA.o clang7_RangeAnalysis.o clang7_eSSA.o
	case $(host_os) in \
	  mingw*) \
	    compile_options='$(CLANG7_LIBS) -lversion `$(I386_LLVM_CONFIG7_EXE) --ldflags --libs --system-libs` -lbdd';\
	    compile_options='';; \
	  darwin*) \
	    compile_options='-fPIC -shared -undefined dynamic_lookup -fno-rtti -lbdd';; \
	  *) \
	    compile_options='-fPIC -shared -fno-rtti -lbdd';;\
	esac;\
	obj_list='clang7_dumpGimple.o clang7_HardekopfLin_AA.o clang7_RangeAnalysis.o clang7_eSSA.o';\
	if test -n "$$compile_options"; then \
	  $(I386_CLANG7_PLUGIN_COMPILER) opt_clang7_plugin_dumpGimpleSSA.o $$obj_list $$compile_options -o clang7_plugin_dumpGimpleSSA_opt.so;\
	  $(I386_CLANG7_PLUGIN_COMPILER) clang7_plugin_dumpGimpleSSA.o $$obj_list $$compile_options -o clang7_plugin_dumpGimpleSSA.so ; \
	fi;
else
$(I386_CLANG7_SSA_PLUGIN) : opt_clang7_plugin_dumpGimpleSSA.o clang7_plugin_dumpGimpleSSA.o clang7_dumpGimple.o clang7_RangeAnalysis.o clang7_eSSA.o
	case $(host_os) in \
	  mingw*) \
	    compile_options='$(CLANG7_LIBS) -lversion `$(I386_LLVM_CONFIG7_EXE) --ldflags --libs --system-libs`'; \
	    compile_options='';; \
	  darwin*) \
	    compile_options='-fPIC -shared -undefined dynamic_lookup -fno-rtti';; \
	  *) \
	    compile_options='-fPIC -shared -fno-rtti';;\
	esac;\
	obj_list='clang7_dumpGimple.o clang7_RangeAnalysis.o clang7_eSSA.o';\
	if test -n "$$compile_options"; then \
	  $(I386_CLANG7_PLUGIN_COMPILER) opt_clang7_plugin_dumpGimpleSSA.o $$obj_list $$compile_options -o clang7_plugin_dumpGimpleSSA_opt.so;\
	  $(I386_CLANG7_PLUGIN_COMPILER) clang7_plugin_dumpGimpleSSA.o $$obj_list $$compile_options -o clang7_plugin_dumpGimpleSSA.so ; \
	fi;
endif
if BUILD_PTAA
$(I386_CLANG7_SSA_PLUGINCPP) : opt_clang7_plugin_dumpGimpleSSACpp.o clang7_plugin_dumpGimpleSSACpp.o clang7_dumpGimple.o clang7_HardekopfLin_AA.o clang7_RangeAnalysis.o clang7_eSSA.o
	case $(host_os) in \
	  mingw*) \
	    compile_options='$(CLANG7_LIBS) -lversion `$(I386_LLVM_CONFIG7_EXE) --ldflags --libs --system-libs` -lbdd'; \
	    compile_options='';; \
	  darwin*) \
	    compile_options='-fPIC -shared -undefined dynamic_lookup -fno-rtti -lbdd';; \
	  *) \
	    compile_options='-fPIC -shared -fno-rtti -lbdd';;\
	esac;\
	obj_list='clang7_dumpGimple.o clang7_HardekopfLin_AA.o clang7_RangeAnalysis.o clang7_eSSA.o';\
	if test -n "$$compile_options"; then \
	  $(I386_CLANG7_PLUGIN_COMPILER) opt_clang7_plugin_dumpGimpleSSACpp.o $$obj_list $$compile_options -o clang7_plugin_dumpGimpleSSACpp_opt.so;\
	  $(I386_CLANG7_PLUGIN_COMPILER) clang7_plugin_dumpGimpleSSACpp.o $$obj_list $$compile_options -o clang7_plugin_dumpGimpleSSACpp.so;\
	fi;
else
$(I386_CLANG7_SSA_PLUGINCPP) : opt_clang7_plugin_dumpGimpleSSACpp.o clang7_plugin_dumpGimpleSSACpp.o clang7_dumpGimple.o clang7_RangeAnalysis.o clang7_eSSA.o
	case $(host_os) in \
	  mingw*) \
	    compile_options='$(CLANG7_LIBS) -lversion `$(I386_LLVM_CONFIG7_EXE) --ldflags --libs --system-libs`'; \
	    compile_options='';; \
	  darwin*) \
	    compile_options='-fPIC -shared -undefined dynamic_lookup -fno-rtti';; \
	  *) \
	    compile_options='-fPIC -shared -fno-rtti';;\
	esac;\
	obj_list='clang7_dumpGimple.o clang7_RangeAnalysis.o clang7_eSSA.o';\
	if test -n "$$compile_options"; then \
	  $(I386_CLANG7_PLUGIN_COMPILER) opt_clang7_plugin_dumpGimpleSSACpp.o $$obj_list $$compile_options -o clang7_plugin_dumpGimpleSSACpp_opt.so;\
	  $(I386_CLANG7_PLUGIN_COMPILER) clang7_plugin_dumpGimpleSSACpp.o $$obj_list $$compile_options -o clang7_plugin_dumpGimpleSSACpp.so;\
	fi;
endif

if BUILD_PTAA
$(I386_CLANG7_EMPTY_PLUGIN) : opt_clang7_plugin_dumpGimpleEmpty.o clang7_plugin_dumpGimpleEmpty.o clang7_dumpGimple.o clang7_HardekopfLin_AA.o clang7_RangeAnalysis.o clang7_eSSA.o
	case $(host_os) in \
	  mingw*) \
	    compile_options='$(CLANG7_LIBS) -lversion `$(I386_LLVM_CONFIG7_EXE) --ldflags --libs --system-libs` -lbdd'; \
	    compile_options='';; \
	  darwin*) \
	    compile_options='-fPIC -shared -undefined dynamic_lookup -fno-rtti -lbdd';; \
	  *) \
	    compile_options='-fPIC -shared -fno-rtti -lbdd';;\
	esac;\
	obj_list='clang7_dumpGimple.o clang7_HardekopfLin_AA.o clang7_RangeAnalysis.o clang7_eSSA.o';\
	if test -n "$$compile_options"; then \
	  $(I386_CLANG7_PLUGIN_COMPILER) opt_clang7_plugin_dumpGimpleEmpty.o $$obj_list $$compile_options -o clang7_plugin_dumpGimpleEmpty_opt.so; \
	  $(I386_CLANG7_PLUGIN_COMPILER) clang7_plugin_dumpGimpleEmpty.o $$obj_list $$compile_options -o clang7_plugin_dumpGimpleEmpty.so; \
	fi;
else
$(I386_CLANG7_EMPTY_PLUGIN) : opt_clang7_plugin_dumpGimpleEmpty.o clang7_plugin_dumpGimpleEmpty.o clang7_dumpGimple.o clang7_RangeAnalysis.o clang7_eSSA.o
	case $(host_os) in \
	  mingw*) \
	    compile_options='$(CLANG7_LIBS) -lversion `$(I386_LLVM_CONFIG7_EXE) --ldflags --libs --system-libs`'; \
	    compile_options='';; \
	  darwin*) \
	    compile_options='-fPIC -shared -undefined dynamic_lookup -fno-rtti';; \
	  *) \
	    compile_options='-fPIC -shared -fno-rtti';;\
	esac;\
	obj_list='clang7_dumpGimple.o clang7_RangeAnalysis.o clang7_eSSA.o';\
	if test -n "$$compile_options"; then \
	  $(I386_CLANG7_PLUGIN_COMPILER) opt_clang7_plugin_dumpGimpleEmpty.o $$obj_list $$compile_options -o clang7_plugin_dumpGimpleEmpty_opt.so; \
	  $(I386_CLANG7_PLUGIN_COMPILER) clang7_plugin_dumpGimpleEmpty.o $$obj_list $$compile_options -o clang7_plugin_dumpGimpleEmpty.so; \
	fi;
endif

$(I386_CLANG7_EXPANDMEMOPS_PLUGIN) : opt_clang7_plugin_expandMemOps.o clang7_plugin_expandMemOps.o 
	case $(host_os) in \
	  mingw*) \
	    compile_options='$(CLANG7_LIBS) -lversion `$(I386_LLVM_CONFIG7_EXE) --ldflags --libs --system-libs`'; \
	    compile_options='';; \
	  darwin*) \
	    compile_options='-fPIC -shared -undefined dynamic_lookup -fno-rtti';; \
	  *) \
	    compile_options='-fPIC -shared -fno-rtti';;\
	esac;\
	if test -n "$$compile_options"; then \
	  $(I386_CLANG7_PLUGIN_COMPILER) opt_clang7_plugin_expandMemOps.o $$compile_options -o clang7_plugin_expandMemOps_opt.so; \
	  $(I386_CLANG7_PLUGIN_COMPILER) clang7_plugin_expandMemOps.o $$compile_options -o clang7_plugin_expandMemOps.so; \
	fi;

$(I386_CLANG7_CSROA_PLUGIN) : opt_clang7_plugin_CSROA.o clang7_plugin_CSROA.o clang7_CSROA.o
	case $(host_os) in \
	  mingw*) \
	    compile_options='$(CLANG7_LIBS) -lversion `$(I386_LLVM_CONFIG7_EXE) --ldflags --libs --system-libs`'; \
	    compile_options='';; \
	  darwin*) \
	    compile_options='-fPIC -shared -undefined dynamic_lookup -fno-rtti';; \
	  *) \
	    compile_options='-fPIC -shared -fno-rtti';;\
	esac;\
	if test -n "$$compile_options"; then \
	  $(I386_CLANG7_PLUGIN_COMPILER) opt_clang7_plugin_CSROA.o clang7_CSROA.o $$compile_options -o clang7_plugin_CSROA_opt.so; \
	  $(I386_CLANG7_PLUGIN_COMPILER) clang7_plugin_CSROA.o clang7_CSROA.o $$compile_options -o clang7_plugin_CSROA.so; \
	fi;

$(I386_CLANG7_TOPFNAME_PLUGIN) : clang7_plugin_topfname.o 
	case $(host_os) in \
	  mingw*) \
	    compile_options='$(CLANG7_LIBS) -lversion `$(I386_LLVM_CONFIG7_EXE) --ldflags --libs --system-libs`'; \
	    compile_options='';; \
	  darwin*) \
	    compile_options='-fPIC -shared -undefined dynamic_lookup -fno-rtti';; \
	  *) \
	    compile_options='-fPIC -shared -fno-rtti';;\
	esac;\
	if test -n "$$compile_options"; then \
	  $(I386_CLANG7_PLUGIN_COMPILER) opt_clang7_plugin_topfname.o $$compile_options -o clang7_plugin_topfname_opt.so; \
	  $(I386_CLANG7_PLUGIN_COMPILER) clang7_plugin_topfname.o $$compile_options -o clang7_plugin_topfname.so; \
	fi;

$(I386_CLANG7_ASTANALYZER_PLUGIN) : clang7_plugin_ASTAnalyzer.o 
	case $(host_os) in \
	  mingw*) \
	    compile_options='$(CLANG7_LIBS) -lversion `$(I386_LLVM_CONFIG7_EXE) --ldflags --libs --system-libs`'; \
	    compile_options='';; \
	  darwin*) \
	    compile_options='-fPIC -shared -undefined dynamic_lookup -fno-rtti';; \
	  *) \
	    compile_options='-fPIC -shared -fno-rtti';;\
	esac;\
	if test -n "$$compile_options"; then \
	  $(I386_CLANG7_PLUGIN_COMPILER) clang7_plugin_ASTAnalyzer.o $$compile_options -o clang7_plugin_ASTAnalyzer.so; \
	fi;

clang7_plugin_dumpGimpleEmpty.o : $(top_srcdir)/etc/clang_plugin/plugin_dumpGimpleEmpty.cpp $(top_srcdir)/etc/clang_plugin/plugin_includes.hpp 
	case $(host_os) in \
	  mingw*) \
	    compile_options=' -std=c++11 -fno-rtti -O2 -I$(I386_LLVM7_HEADER_DIR) $(WFLAGS) -I.'; \
	    compile_options='';; \
	  *) \
	    compile_options=' -std=c++11 -fPIC -fno-rtti -O2 -I$(I386_LLVM7_HEADER_DIR) $(WFLAGS) -I.';; \
	esac;\
	if test -n "$$compile_options"; then \
	    $(I386_CLANG7_PLUGIN_COMPILER) -DADD_RSP=0 $$compile_options -c $(top_srcdir)/etc/clang_plugin/plugin_dumpGimpleEmpty.cpp -o opt_clang7_plugin_dumpGimpleEmpty.o; \
	    $(I386_CLANG7_PLUGIN_COMPILER) -DADD_RSP=1 $$compile_options -c $(top_srcdir)/etc/clang_plugin/plugin_dumpGimpleEmpty.cpp -o clang7_plugin_dumpGimpleEmpty.o; \
	fi;

clang7_plugin_dumpGimpleSSA.o : $(top_srcdir)/etc/clang_plugin/plugin_dumpGimpleSSA.cpp $(top_srcdir)/etc/clang_plugin/plugin_includes.hpp 
	case $(host_os) in \
	  mingw*) \
	    compile_options=' -std=c++11 -fno-rtti -O2 -I$(I386_LLVM7_HEADER_DIR) $(WFLAGS) -I.'; \
	    compile_options='';; \
	  *) \
	    compile_options=' -std=c++11 -fPIC -fno-rtti -O2 -I$(I386_LLVM7_HEADER_DIR) $(WFLAGS) -I.';; \
	esac;\
	if test -n "$$compile_options"; then \
	  $(I386_CLANG7_PLUGIN_COMPILER) -DADD_RSP=0 $$compile_options -c $(top_srcdir)/etc/clang_plugin/plugin_dumpGimpleSSA.cpp -o opt_clang7_plugin_dumpGimpleSSA.o;\
	  $(I386_CLANG7_PLUGIN_COMPILER) -DADD_RSP=1 $$compile_options -c $(top_srcdir)/etc/clang_plugin/plugin_dumpGimpleSSA.cpp -o clang7_plugin_dumpGimpleSSA.o;\
	fi;

clang7_plugin_dumpGimpleSSACpp.o : $(top_srcdir)/etc/clang_plugin/plugin_dumpGimpleSSA.cpp $(top_srcdir)/etc/clang_plugin/plugin_includes.hpp 
	case $(host_os) in \
	  mingw*) \
	    compile_options=' -std=c++11 -fno-rtti -O2 -I$(I386_LLVM7_HEADER_DIR) $(WFLAGS) -I. -DCPP_LANGUAGE'; \
	    compile_options='';; \
	  *) \
	    compile_options=' -std=c++11 -fPIC -fno-rtti -O2 -I$(I386_LLVM7_HEADER_DIR) $(WFLAGS) -I. -DCPP_LANGUAGE';; \
	esac;\
	if test -n "$$compile_options"; then \
	  $(I386_CLANG7_PLUGIN_COMPILER) -DADD_RSP=0 $$compile_options -c $(top_srcdir)/etc/clang_plugin/plugin_dumpGimpleSSA.cpp -o opt_clang7_plugin_dumpGimpleSSACpp.o;\
	  $(I386_CLANG7_PLUGIN_COMPILER) -DADD_RSP=1 $$compile_options -c $(top_srcdir)/etc/clang_plugin/plugin_dumpGimpleSSA.cpp -o clang7_plugin_dumpGimpleSSACpp.o;\
	fi;

clang7_plugin_expandMemOps.o : $(top_srcdir)/etc/clang_plugin/plugin_expandMemOps.cpp $(top_srcdir)/etc/clang_plugin/plugin_includes.hpp 
	case $(host_os) in \
	  mingw*) \
	    compile_options=' -std=c++11 -fno-rtti -O2 -I$(I386_LLVM7_HEADER_DIR) $(WFLAGS) -I.'; \
	    compile_options='';; \
	  *) \
	    compile_options=' -std=c++11 -fPIC -fno-rtti -O2 -I$(I386_LLVM7_HEADER_DIR) $(WFLAGS) -I.';; \
	esac;\
	if test -n "$$compile_options"; then \
	  $(I386_CLANG7_PLUGIN_COMPILER) -DADD_RSP=0 $$compile_options -c $(top_srcdir)/etc/clang_plugin/plugin_expandMemOps.cpp -o opt_clang7_plugin_expandMemOps.o; \
	  $(I386_CLANG7_PLUGIN_COMPILER) -DADD_RSP=1 $$compile_options -c $(top_srcdir)/etc/clang_plugin/plugin_expandMemOps.cpp -o clang7_plugin_expandMemOps.o; \
	fi;

clang7_plugin_CSROA.o : $(top_srcdir)/etc/clang_plugin/plugin_CSROA.cpp $(top_srcdir)/etc/clang_plugin/plugin_includes.hpp 
	case $(host_os) in \
	  mingw*) \
	    compile_options=' -std=c++11 -fno-rtti -O2 -I$(I386_LLVM7_HEADER_DIR) $(WFLAGS) -I.'; \
	    compile_options='';; \
	  *) \
	    compile_options=' -std=c++11 -fPIC -fno-rtti -O2 -I$(I386_LLVM7_HEADER_DIR) $(WFLAGS) -I.';; \
	esac;\
	if test -n "$$compile_options"; then \
	  $(I386_CLANG7_PLUGIN_COMPILER) -DADD_RSP=0 $$compile_options -c $(top_srcdir)/etc/clang_plugin/plugin_CSROA.cpp -o opt_clang7_plugin_CSROA.o; \
	  $(I386_CLANG7_PLUGIN_COMPILER) -DADD_RSP=1 $$compile_options -c $(top_srcdir)/etc/clang_plugin/plugin_CSROA.cpp -o clang7_plugin_CSROA.o; \
	fi;

clang7_plugin_topfname.o : $(top_srcdir)/etc/clang_plugin/plugin_topfname.cpp $(top_srcdir)/etc/clang_plugin/plugin_includes.hpp 
	case $(host_os) in \
	  mingw*) \
	    compile_options=' -std=c++11 -fno-rtti -O2 -I$(I386_LLVM7_HEADER_DIR) $(WFLAGS) -I.'; \
	    compile_options='';; \
	  *) \
	    compile_options=' -std=c++11 -fPIC -fno-rtti -O2 -I$(I386_LLVM7_HEADER_DIR) $(WFLAGS) -I.';; \
	esac;\
	if test -n "$$compile_options"; then \
	  $(I386_CLANG7_PLUGIN_COMPILER) -DADD_RSP=0 $$compile_options -c $(top_srcdir)/etc/clang_plugin/plugin_topfname.cpp -o opt_clang7_plugin_topfname.o; \
	  $(I386_CLANG7_PLUGIN_COMPILER) -DADD_RSP=1 $$compile_options -c $(top_srcdir)/etc/clang_plugin/plugin_topfname.cpp -o clang7_plugin_topfname.o; \
	fi;

clang7_plugin_ASTAnalyzer.o : $(top_srcdir)/etc/clang_plugin/plugin_ASTAnalyzer.cpp $(top_srcdir)/etc/clang_plugin/plugin_includes.hpp 
	case $(host_os) in \
	  mingw*) \
	    compile_options=' -std=c++11 -fno-rtti -O2 -I$(I386_LLVM7_HEADER_DIR) $(WFLAGS) -I.'; \
	    compile_options='';; \
	  *) \
	    compile_options=' -std=c++11 -fPIC -fno-rtti -O2 -I$(I386_LLVM7_HEADER_DIR) $(WFLAGS) -I.';; \
	esac;\
	if test -n "$$compile_options"; then \
	  $(I386_CLANG7_PLUGIN_COMPILER) $$compile_options -c $(top_srcdir)/etc/clang_plugin/plugin_ASTAnalyzer.cpp -o clang7_plugin_ASTAnalyzer.o;\
	fi;

clang7_dumpGimple.o : $(top_srcdir)/etc/clang_plugin/dumpGimple.cpp $(top_srcdir)/etc/clang_plugin/plugin_includes.hpp $(top_srcdir)/etc/clang_plugin/HardekopfLin_AA.hpp $(top_srcdir)/etc/clang_plugin/llvm/RangeAnalysis.hpp  $(top_srcdir)/etc/clang_plugin/llvm/eSSA.hpp
	case $(host_os) in \
	  mingw*) \
	    compile_options=' -std=c++11 -fno-rtti -O2 -I$(I386_LLVM7_HEADER_DIR) -DPANDA_PLUGIN_VERSION="$(PANDA_PLUGIN_VERSION)" $(WFLAGS) -I.'; \
	    compile_options='';; \
	  *) \
	    compile_options=' -std=c++11 -fPIC -fno-rtti -O2 -I$(I386_LLVM7_HEADER_DIR) -DPANDA_PLUGIN_VERSION="$(PANDA_PLUGIN_VERSION)" $(WFLAGS) -I.';; \
	esac;\
	if test -n "$$compile_options"; then \
	  $(I386_CLANG7_PLUGIN_COMPILER) $$compile_options -c $(top_srcdir)/etc/clang_plugin/dumpGimple.cpp -o clang7_dumpGimple.o;\
	fi;

if BUILD_PTAA
clang7_HardekopfLin_AA.o : $(top_srcdir)/etc/clang_plugin/HardekopfLin_AA.cpp $(top_srcdir)/etc/clang_plugin/HardekopfLin_AA.hpp
	case $(host_os) in \
	  mingw*) \
	    compile_options=' -std=c++11 -fno-rtti -O2 -I$(I386_LLVM7_HEADER_DIR) $(WFLAGS) -I.'; \
	    compile_options='';; \
	  *) \
	    compile_options=' -std=c++11 -fPIC -fno-rtti -O2 -I$(I386_LLVM7_HEADER_DIR) $(WFLAGS) -I.';; \
	esac;\
	if test -n "$$compile_options"; then \
	  $(I386_CLANG7_PLUGIN_COMPILER) $$compile_options -c $(top_srcdir)/etc/clang_plugin/HardekopfLin_AA.cpp -o clang7_HardekopfLin_AA.o ; \
	fi;
endif

clang7_RangeAnalysis.o : $(top_srcdir)/etc/clang_plugin/llvm/RangeAnalysis.cpp $(top_srcdir)/etc/clang_plugin/llvm/RangeAnalysis.hpp
	case $(host_os) in \
	  mingw*) \
	    compile_options=' -std=c++11 -fno-rtti -O2 -I$(I386_LLVM7_HEADER_DIR) $(WFLAGS) -I.'; \
	    compile_options='';; \
	  *) \
	    compile_options=' -std=c++11 -fPIC -fno-rtti -O2 -I$(I386_LLVM7_HEADER_DIR) $(WFLAGS) -I.';; \
	esac;\
	if test -n "$$compile_options"; then \
	    $(I386_CLANG7_PLUGIN_COMPILER) $$compile_options -c $(top_srcdir)/etc/clang_plugin/llvm/RangeAnalysis.cpp -o clang7_RangeAnalysis.o; \
	fi;

clang7_eSSA.o : $(top_srcdir)/etc/clang_plugin/llvm/eSSA.cpp $(top_srcdir)/etc/clang_plugin/llvm/eSSA.hpp $(top_srcdir)/etc/clang_plugin/llvm/my_OrderedInstructions.hpp
	case $(host_os) in \
	  mingw*) \
	    compile_options=' -std=c++11 -fno-rtti -O2 -I$(I386_LLVM7_HEADER_DIR) -DPANDA_PLUGIN_VERSION="$(PANDA_PLUGIN_VERSION)" $(WFLAGS) -I.'; \
	    compile_options='';; \
	  *) \
	    compile_options=' -std=c++11 -fPIC -fno-rtti -O2 -I$(I386_LLVM7_HEADER_DIR) -DPANDA_PLUGIN_VERSION="$(PANDA_PLUGIN_VERSION)" $(WFLAGS) -I.';; \
	esac;\
	if test -n "$$compile_options"; then \
	    $(I386_CLANG7_PLUGIN_COMPILER) $$compile_options -c $(top_srcdir)/etc/clang_plugin/llvm/eSSA.cpp -o clang7_eSSA.o; \
	fi;

clang7_CSROA.o : $(top_srcdir)/etc/clang_plugin/CSROA.cpp $(top_srcdir)/etc/clang_plugin/CSROA.hpp
	case $(host_os) in \
	  mingw*) \
	    compile_options=' -std=c++11 -fno-rtti -O2 -I$(I386_LLVM7_HEADER_DIR) -DPANDA_PLUGIN_VERSION="$(PANDA_PLUGIN_VERSION)" $(WFLAGS) -I.'; \
	    compile_options='';; \
	  *) \
	    compile_options=' -std=c++11 -fPIC -fno-rtti -O2 -I$(I386_LLVM7_HEADER_DIR) -DPANDA_PLUGIN_VERSION="$(PANDA_PLUGIN_VERSION)" $(WFLAGS) -I.';; \
	esac;\
	if test -n "$$compile_options"; then \
	    $(I386_CLANG7_PLUGIN_COMPILER) $$compile_options -c $(top_srcdir)/etc/clang_plugin/CSROA.cpp -o clang7_CSROA.o; \
	fi;

bambuS2S-7: $(top_srcdir)/etc/clang_plugin/bambuS2S.cpp
	case $(host_os) in \
	  darwin*) \
	    $(I386_CLANG7_PLUGIN_COMPILER) -fno-rtti -O2 -std=c++14 `$(I386_LLVM_CONFIG7_EXE) --cppflags --ldflags --libs --system-libs` $^ $(CLANG7_LIBSD)  -o $@;;\
	  *) \
	    $(I386_CLANG7_PLUGIN_COMPILER) -fno-rtti -O2 -std=c++14 `$(I386_LLVM_CONFIG7_EXE) --cppflags --ldflags --libs --system-libs` $^ $(CLANG7_LIBS)  -o $@;;\
	esac;

###### 


clean :
	rm -rf *.o *.so *.h bambuS2S-*

install: all
	if test "`ls *so`" != ""; then mkdir -p $(DESTDIR)$(PLUGIN_DIR); cp -v *.so $(DESTDIR)$(PLUGIN_DIR); fi
	if test "`ls bambuS2S-*`" != ""; then mkdir -p $(DESTDIR)$(bindir); cp -v  bambuS2S-* $(DESTDIR)$(bindir); fi

#do not touch the following line


