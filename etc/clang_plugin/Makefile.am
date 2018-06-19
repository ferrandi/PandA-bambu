#This is etc/clang_plugin/Makefile.am
EXTRA_DIST = $(top_srcdir)/etc/clang_plugin/plugin_dumpGimpleSSA.cpp $(top_srcdir)/etc/clang_plugin/plugin_includes.hpp $(top_srcdir)/etc/clang_plugin/plugin_dumpGimpleEmpty.cpp $(top_srcdir)/etc/clang_plugin/dumpGimple.cpp $(top_srcdir)/etc/clang_plugin/plugin_topfname.cpp gcc/cp-tree.def gcc/sync-builtins.def gcc/gtm-builtins.def gcc/sanitizer.def gcc/hsa-builtins.def gcc/c-common.def gcc/gimple.def gcc/builtins.def gcc/brig-builtins.def gcc/cilkplus.def gcc/omp-builtins.def gcc/tree.def gcc/cilk-builtins.def gcc/chkp-builtins.def gcc/COPYING3 HardekopfLin_AA.hpp HardekopfLin_AA.cpp llvm/eSSA.hpp llvm/eSSA.cpp llvm/RangeAnalysis.hpp llvm/RangeAnalysis.cpp llvm/LICENSE.TXT  llvm/my_OrderedInstructions.cpp  llvm/my_OrderedInstructions.hpp


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


###### clang 4.0
if BUILD_I386_CLANG4_EMPTY_PLUGIN
all: $(I386_CLANG4_EMPTY_PLUGIN)
endif

if BUILD_I386_CLANG4_SSA_PLUGIN
all: $(I386_CLANG4_SSA_PLUGIN)
endif

if BUILD_I386_CLANG4_SSA_PLUGINCPP
all: $(I386_CLANG4_SSA_PLUGINCPP)
endif

if BUILD_I386_CLANG4_TOPFNAME_PLUGIN
all: $(I386_CLANG4_TOPFNAME_PLUGIN)
endif

if BUILD_PTAA
$(I386_CLANG4_SSA_PLUGIN) : clang4_plugin_dumpGimpleSSA.o clang4_dumpGimple.o clang4_HardekopfLin_AA.o clang4_RangeAnalysis.o clang4_my_OrderedInstructions.o clang4_eSSA.o
	$(I386_CLANG4_PLUGIN_COMPILER) -fPIC -shared -fno-rtti -O3 clang4_plugin_dumpGimpleSSA.o clang4_dumpGimple.o clang4_HardekopfLin_AA.o clang4_RangeAnalysis.o clang4_my_OrderedInstructions.o clang4_eSSA.o -o clang4_plugin_dumpGimpleSSA.so -lbdd
else
$(I386_CLANG4_SSA_PLUGIN) : clang4_plugin_dumpGimpleSSA.o clang4_dumpGimple.o clang4_RangeAnalysis.o clang4_my_OrderedInstructions.o clang4_eSSA.o
	$(I386_CLANG4_PLUGIN_COMPILER) -fPIC -shared -fno-rtti -O3 clang4_plugin_dumpGimpleSSA.o clang4_dumpGimple.o clang4_RangeAnalysis.o clang4_my_OrderedInstructions.o clang4_eSSA.o -o clang4_plugin_dumpGimpleSSA.so
endif

if BUILD_PTAA
$(I386_CLANG4_SSA_PLUGINCPP) : clang4_plugin_dumpGimpleSSACpp.o clang4_dumpGimple.o clang4_HardekopfLin_AA.o clang4_RangeAnalysis.o clang4_my_OrderedInstructions.o clang4_eSSA.o
	$(I386_CLANG4_PLUGIN_COMPILER) -fPIC -shared -fno-rtti -O3 clang4_plugin_dumpGimpleSSACpp.o clang4_dumpGimple.o clang4_HardekopfLin_AA.o clang4_RangeAnalysis.o clang4_my_OrderedInstructions.o clang4_eSSA.o -o clang4_plugin_dumpGimpleSSACpp.so -lbdd
else
$(I386_CLANG4_SSA_PLUGINCPP) : clang4_plugin_dumpGimpleSSACpp.o clang4_dumpGimple.o clang4_RangeAnalysis.o clang4_my_OrderedInstructions.o clang4_eSSA.o
	$(I386_CLANG4_PLUGIN_COMPILER) -fPIC -shared -fno-rtti -O3 clang4_plugin_dumpGimpleSSACpp.o clang4_dumpGimple.o clang4_RangeAnalysis.o clang4_my_OrderedInstructions.o clang4_eSSA.o -o clang4_plugin_dumpGimpleSSACpp.so
endif

if BUILD_PTAA
$(I386_CLANG4_EMPTY_PLUGIN) : clang4_plugin_dumpGimpleEmpty.o clang4_dumpGimple.o  clang4_HardekopfLin_AA.o clang4_RangeAnalysis.o clang4_my_OrderedInstructions.o clang4_eSSA.o
	$(I386_CLANG4_PLUGIN_COMPILER) -fPIC -shared -fno-rtti -O3 clang4_plugin_dumpGimpleEmpty.o clang4_dumpGimple.o clang4_HardekopfLin_AA.o clang4_RangeAnalysis.o clang4_my_OrderedInstructions.o clang4_eSSA.o -o clang4_plugin_dumpGimpleEmpty.so -lbdd
else
$(I386_CLANG4_EMPTY_PLUGIN) : clang4_plugin_dumpGimpleEmpty.o clang4_dumpGimple.o clang4_RangeAnalysis.o clang4_my_OrderedInstructions.o clang4_eSSA.o
	$(I386_CLANG4_PLUGIN_COMPILER) -fPIC -shared -fno-rtti -O3 clang4_plugin_dumpGimpleEmpty.o clang4_dumpGimple.o clang4_RangeAnalysis.o clang4_my_OrderedInstructions.o clang4_eSSA.o -o clang4_plugin_dumpGimpleEmpty.so
endif

$(I386_CLANG4_TOPFNAME_PLUGIN) : clang4_plugin_topfname.o 
	$(I386_CLANG4_PLUGIN_COMPILER) -fPIC -shared -fno-rtti -O3 clang4_plugin_topfname.o -o clang4_plugin_topfname.so

clang4_plugin_dumpGimpleEmpty.o : $(top_srcdir)/etc/clang_plugin/plugin_dumpGimpleEmpty.cpp $(top_srcdir)/etc/clang_plugin/plugin_includes.hpp 
	$(I386_CLANG4_PLUGIN_COMPILER) -std=c++11 -fPIC -fno-rtti -O3 -c $(top_srcdir)/etc/clang_plugin/plugin_dumpGimpleEmpty.cpp -o clang4_plugin_dumpGimpleEmpty.o -I$(I386_LLVM4_HEADER_DIR) $(WFLAGS)

clang4_plugin_dumpGimpleSSA.o : $(top_srcdir)/etc/clang_plugin/plugin_dumpGimpleSSA.cpp $(top_srcdir)/etc/clang_plugin/plugin_includes.hpp 
	$(I386_CLANG4_PLUGIN_COMPILER) -std=c++11 -fPIC -fno-rtti -O3 -c $(top_srcdir)/etc/clang_plugin/plugin_dumpGimpleSSA.cpp -o clang4_plugin_dumpGimpleSSA.o -I$(I386_LLVM4_HEADER_DIR) $(WFLAGS)

clang4_plugin_dumpGimpleSSACpp.o : $(top_srcdir)/etc/clang_plugin/plugin_dumpGimpleSSA.cpp $(top_srcdir)/etc/clang_plugin/plugin_includes.hpp 
	$(I386_CLANG4_PLUGIN_COMPILER) -std=c++11 -fPIC -fno-rtti -O3 -c $(top_srcdir)/etc/clang_plugin/plugin_dumpGimpleSSA.cpp -DCPP_LANGUAGE -o clang4_plugin_dumpGimpleSSACpp.o -I$(I386_LLVM4_HEADER_DIR) $(WFLAGS)

clang4_plugin_topfname.o : $(top_srcdir)/etc/clang_plugin/plugin_topfname.cpp $(top_srcdir)/etc/clang_plugin/plugin_includes.hpp 
	$(I386_CLANG4_PLUGIN_COMPILER) -std=c++11 -fPIC -fno-rtti -O3 -c $(top_srcdir)/etc/clang_plugin/plugin_topfname.cpp -o clang4_plugin_topfname.o -I$(I386_LLVM4_HEADER_DIR) $(WFLAGS)

if BUILD_PTAA
clang4_dumpGimple.o : $(top_srcdir)/etc/clang_plugin/dumpGimple.cpp $(top_srcdir)/etc/clang_plugin/plugin_includes.hpp $(top_srcdir)/etc/clang_plugin/HardekopfLin_AA.hpp $(top_srcdir)/etc/clang_plugin/llvm/RangeAnalysis.hpp $(top_srcdir)/etc/clang_plugin/llvm/eSSA.hpp
	$(I386_CLANG4_PLUGIN_COMPILER) -std=c++11 -fPIC -fno-rtti -O3 -c $(top_srcdir)/etc/clang_plugin/dumpGimple.cpp -o clang4_dumpGimple.o -I. -I$(I386_LLVM4_HEADER_DIR) -DPANDA_PLUGIN_VERSION=\"$(PANDA_PLUGIN_VERSION)\" $(WFLAGS)
else
clang4_dumpGimple.o : $(top_srcdir)/etc/clang_plugin/dumpGimple.cpp $(top_srcdir)/etc/clang_plugin/plugin_includes.hpp $(top_srcdir)/etc/clang_plugin/llvm/RangeAnalysis.hpp $(top_srcdir)/etc/clang_plugin/llvm/eSSA.hpp
	$(I386_CLANG4_PLUGIN_COMPILER) -std=c++11 -fPIC -fno-rtti -O3 -c $(top_srcdir)/etc/clang_plugin/dumpGimple.cpp -o clang4_dumpGimple.o -I. -I$(I386_LLVM4_HEADER_DIR) -DPANDA_PLUGIN_VERSION=\"$(PANDA_PLUGIN_VERSION)\" $(WFLAGS)
endif

if BUILD_PTAA
clang4_HardekopfLin_AA.o : $(top_srcdir)/etc/clang_plugin/HardekopfLin_AA.cpp $(top_srcdir)/etc/clang_plugin/HardekopfLin_AA.hpp
	$(I386_CLANG4_PLUGIN_COMPILER) -std=c++11 -fPIC -fno-rtti -O3 -c $(top_srcdir)/etc/clang_plugin/HardekopfLin_AA.cpp -o clang4_HardekopfLin_AA.o -I. -I$(I386_LLVM4_HEADER_DIR) -DPANDA_PLUGIN_VERSION=\"$(PANDA_PLUGIN_VERSION)\" $(WFLAGS)
endif

clang4_RangeAnalysis.o : $(top_srcdir)/etc/clang_plugin/llvm/RangeAnalysis.cpp $(top_srcdir)/etc/clang_plugin/llvm/RangeAnalysis.hpp
	$(I386_CLANG4_PLUGIN_COMPILER) -std=c++11 -fPIC -fno-rtti -O3 -c $(top_srcdir)/etc/clang_plugin/llvm/RangeAnalysis.cpp -o clang4_RangeAnalysis.o -I. -I$(I386_LLVM4_HEADER_DIR) -DPANDA_PLUGIN_VERSION=\"$(PANDA_PLUGIN_VERSION)\" $(WFLAGS)

clang4_my_OrderedInstructions.o : $(top_srcdir)/etc/clang_plugin/llvm/my_OrderedInstructions.cpp $(top_srcdir)/etc/clang_plugin/llvm/my_OrderedInstructions.hpp
	$(I386_CLANG4_PLUGIN_COMPILER) -std=c++11 -fPIC -fno-rtti -O3 -c $(top_srcdir)/etc/clang_plugin/llvm/my_OrderedInstructions.cpp -o clang4_my_OrderedInstructions.o -I. -I$(I386_LLVM4_HEADER_DIR) -DPANDA_PLUGIN_VERSION=\"$(PANDA_PLUGIN_VERSION)\" $(WFLAGS)


clang4_eSSA.o : $(top_srcdir)/etc/clang_plugin/llvm/eSSA.cpp $(top_srcdir)/etc/clang_plugin/llvm/eSSA.hpp
	$(I386_CLANG4_PLUGIN_COMPILER) -std=c++11 -fPIC -fno-rtti -O3 -c $(top_srcdir)/etc/clang_plugin/llvm/eSSA.cpp -o clang4_eSSA.o -I. -I$(I386_LLVM4_HEADER_DIR) -DPANDA_PLUGIN_VERSION=\"$(PANDA_PLUGIN_VERSION)\" $(WFLAGS)

###### 

###### clang 5.0
if BUILD_I386_CLANG5_EMPTY_PLUGIN
all: $(I386_CLANG5_EMPTY_PLUGIN)
endif

if BUILD_I386_CLANG5_SSA_PLUGIN
all: $(I386_CLANG5_SSA_PLUGIN)
endif

if BUILD_I386_CLANG5_SSA_PLUGINCPP
all: $(I386_CLANG5_SSA_PLUGINCPP)
endif

if BUILD_I386_CLANG5_TOPFNAME_PLUGIN
all: $(I386_CLANG5_TOPFNAME_PLUGIN)
endif

if BUILD_PTAA
$(I386_CLANG5_SSA_PLUGIN) : clang5_plugin_dumpGimpleSSA.o clang5_dumpGimple.o  clang5_HardekopfLin_AA.o clang5_RangeAnalysis.o clang5_eSSA.o
	$(I386_CLANG5_PLUGIN_COMPILER) -fPIC -shared -fno-rtti -O3 clang5_plugin_dumpGimpleSSA.o clang5_dumpGimple.o clang5_HardekopfLin_AA.o clang5_RangeAnalysis.o clang5_eSSA.o -o clang5_plugin_dumpGimpleSSA.so -lbdd
else
$(I386_CLANG5_SSA_PLUGIN) : clang5_plugin_dumpGimpleSSA.o clang5_dumpGimple.o clang5_RangeAnalysis.o clang5_eSSA.o
	$(I386_CLANG5_PLUGIN_COMPILER) -fPIC -shared -fno-rtti -O3 clang5_plugin_dumpGimpleSSA.o clang5_dumpGimple.o clang5_RangeAnalysis.o clang5_eSSA.o -o clang5_plugin_dumpGimpleSSA.so
endif
if BUILD_PTAA
$(I386_CLANG5_SSA_PLUGINCPP) : clang5_plugin_dumpGimpleSSACpp.o clang5_dumpGimple.o clang5_HardekopfLin_AA.o clang5_RangeAnalysis.o clang5_eSSA.o
	$(I386_CLANG5_PLUGIN_COMPILER) -fPIC -shared -fno-rtti -O3 clang5_plugin_dumpGimpleSSACpp.o clang5_dumpGimple.o clang5_HardekopfLin_AA.o clang5_RangeAnalysis.o clang5_eSSA.o -o clang5_plugin_dumpGimpleSSACpp.so -lbdd
else
$(I386_CLANG5_SSA_PLUGINCPP) : clang5_plugin_dumpGimpleSSACpp.o clang5_dumpGimple.o clang5_RangeAnalysis.o clang5_eSSA.o
	$(I386_CLANG5_PLUGIN_COMPILER) -fPIC -shared -fno-rtti -O3 clang5_plugin_dumpGimpleSSACpp.o clang5_dumpGimple.o clang5_RangeAnalysis.o clang5_eSSA.o -o clang5_plugin_dumpGimpleSSACpp.so
endif

if BUILD_PTAA
$(I386_CLANG5_EMPTY_PLUGIN) : clang5_plugin_dumpGimpleEmpty.o clang5_dumpGimple.o clang5_HardekopfLin_AA.o clang5_RangeAnalysis.o clang5_eSSA.o
	$(I386_CLANG5_PLUGIN_COMPILER) -fPIC -shared -fno-rtti -O3 clang5_plugin_dumpGimpleEmpty.o clang5_dumpGimple.o clang5_HardekopfLin_AA.o clang5_RangeAnalysis.o clang5_eSSA.o -o clang5_plugin_dumpGimpleEmpty.so -lbdd
else
$(I386_CLANG5_EMPTY_PLUGIN) : clang5_plugin_dumpGimpleEmpty.o clang5_dumpGimple.o clang5_RangeAnalysis.o clang5_eSSA.o
	$(I386_CLANG5_PLUGIN_COMPILER) -fPIC -shared -fno-rtti -O3 clang5_plugin_dumpGimpleEmpty.o clang5_dumpGimple.o clang5_RangeAnalysis.o clang5_eSSA.o -o clang5_plugin_dumpGimpleEmpty.so
endif

$(I386_CLANG5_TOPFNAME_PLUGIN) : clang5_plugin_topfname.o 
	$(I386_CLANG5_PLUGIN_COMPILER) -fPIC -shared -fno-rtti -O3 clang5_plugin_topfname.o -o clang5_plugin_topfname.so

clang5_plugin_dumpGimpleEmpty.o : $(top_srcdir)/etc/clang_plugin/plugin_dumpGimpleEmpty.cpp $(top_srcdir)/etc/clang_plugin/plugin_includes.hpp 
	$(I386_CLANG5_PLUGIN_COMPILER) -std=c++11 -fPIC -fno-rtti -O3 -c $(top_srcdir)/etc/clang_plugin/plugin_dumpGimpleEmpty.cpp -o clang5_plugin_dumpGimpleEmpty.o -I$(I386_LLVM5_HEADER_DIR) $(WFLAGS)

clang5_plugin_dumpGimpleSSA.o : $(top_srcdir)/etc/clang_plugin/plugin_dumpGimpleSSA.cpp $(top_srcdir)/etc/clang_plugin/plugin_includes.hpp 
	$(I386_CLANG5_PLUGIN_COMPILER) -std=c++11 -fPIC -fno-rtti -O3 -c $(top_srcdir)/etc/clang_plugin/plugin_dumpGimpleSSA.cpp -o clang5_plugin_dumpGimpleSSA.o -I$(I386_LLVM5_HEADER_DIR) $(WFLAGS)

clang5_plugin_dumpGimpleSSACpp.o : $(top_srcdir)/etc/clang_plugin/plugin_dumpGimpleSSA.cpp $(top_srcdir)/etc/clang_plugin/plugin_includes.hpp 
	$(I386_CLANG5_PLUGIN_COMPILER) -std=c++11 -fPIC -fno-rtti -O3 -c $(top_srcdir)/etc/clang_plugin/plugin_dumpGimpleSSA.cpp -DCPP_LANGUAGE -o clang5_plugin_dumpGimpleSSACpp.o -I$(I386_LLVM5_HEADER_DIR) $(WFLAGS)

clang5_plugin_topfname.o : $(top_srcdir)/etc/clang_plugin/plugin_topfname.cpp $(top_srcdir)/etc/clang_plugin/plugin_includes.hpp 
	$(I386_CLANG5_PLUGIN_COMPILER) -std=c++11 -fPIC -fno-rtti -O3 -c $(top_srcdir)/etc/clang_plugin/plugin_topfname.cpp -o clang5_plugin_topfname.o -I$(I386_LLVM5_HEADER_DIR) $(WFLAGS)

if BUILD_PTAA
clang5_dumpGimple.o : $(top_srcdir)/etc/clang_plugin/dumpGimple.cpp $(top_srcdir)/etc/clang_plugin/plugin_includes.hpp $(top_srcdir)/etc/clang_plugin/HardekopfLin_AA.hpp $(top_srcdir)/etc/clang_plugin/llvm/RangeAnalysis.hpp  $(top_srcdir)/etc/clang_plugin/llvm/eSSA.hpp
	$(I386_CLANG5_PLUGIN_COMPILER) -std=c++11 -fPIC -fno-rtti -O3 -c $(top_srcdir)/etc/clang_plugin/dumpGimple.cpp -o clang5_dumpGimple.o -I. -I$(I386_LLVM5_HEADER_DIR) -DPANDA_PLUGIN_VERSION=\"$(PANDA_PLUGIN_VERSION)\" $(WFLAGS)
else
clang5_dumpGimple.o : $(top_srcdir)/etc/clang_plugin/dumpGimple.cpp $(top_srcdir)/etc/clang_plugin/plugin_includes.hpp $(top_srcdir)/etc/clang_plugin/llvm/RangeAnalysis.hpp  $(top_srcdir)/etc/clang_plugin/llvm/eSSA.hpp
	$(I386_CLANG5_PLUGIN_COMPILER) -std=c++11 -fPIC -fno-rtti -O3 -c $(top_srcdir)/etc/clang_plugin/dumpGimple.cpp -o clang5_dumpGimple.o -I. -I$(I386_LLVM5_HEADER_DIR) -DPANDA_PLUGIN_VERSION=\"$(PANDA_PLUGIN_VERSION)\" $(WFLAGS)
endif

if BUILD_PTAA
clang5_HardekopfLin_AA.o : $(top_srcdir)/etc/clang_plugin/HardekopfLin_AA.cpp $(top_srcdir)/etc/clang_plugin/HardekopfLin_AA.hpp
	$(I386_CLANG5_PLUGIN_COMPILER) -std=c++11 -fPIC -fno-rtti -O3 -c $(top_srcdir)/etc/clang_plugin/HardekopfLin_AA.cpp -o clang5_HardekopfLin_AA.o -I. -I$(I386_LLVM5_HEADER_DIR) -DPANDA_PLUGIN_VERSION=\"$(PANDA_PLUGIN_VERSION)\" $(WFLAGS)
endif

clang5_RangeAnalysis.o : $(top_srcdir)/etc/clang_plugin/llvm/RangeAnalysis.cpp $(top_srcdir)/etc/clang_plugin/llvm/RangeAnalysis.hpp
	$(I386_CLANG5_PLUGIN_COMPILER) -std=c++11 -fPIC -fno-rtti -O3 -c $(top_srcdir)/etc/clang_plugin/llvm/RangeAnalysis.cpp -o clang5_RangeAnalysis.o -I. -I$(I386_LLVM5_HEADER_DIR) -DPANDA_PLUGIN_VERSION=\"$(PANDA_PLUGIN_VERSION)\" $(WFLAGS)

clang5_eSSA.o : $(top_srcdir)/etc/clang_plugin/llvm/eSSA.cpp $(top_srcdir)/etc/clang_plugin/llvm/eSSA.hpp $(top_srcdir)/etc/clang_plugin/llvm/my_OrderedInstructions.hpp
	$(I386_CLANG5_PLUGIN_COMPILER) -std=c++11 -fPIC -fno-rtti -O3 -c $(top_srcdir)/etc/clang_plugin/llvm/eSSA.cpp -o clang5_eSSA.o -I. -I$(I386_LLVM5_HEADER_DIR) -DPANDA_PLUGIN_VERSION=\"$(PANDA_PLUGIN_VERSION)\" $(WFLAGS)

###### 

###### clang 6.0
if BUILD_I386_CLANG6_EMPTY_PLUGIN
all: $(I386_CLANG6_EMPTY_PLUGIN)
endif

if BUILD_I386_CLANG6_SSA_PLUGIN
all: $(I386_CLANG6_SSA_PLUGIN)
endif

if BUILD_I386_CLANG6_SSA_PLUGINCPP
all: $(I386_CLANG6_SSA_PLUGINCPP)
endif

if BUILD_I386_CLANG6_TOPFNAME_PLUGIN
all: $(I386_CLANG6_TOPFNAME_PLUGIN)
endif

if BUILD_PTAA
$(I386_CLANG6_SSA_PLUGIN) : clang6_plugin_dumpGimpleSSA.o clang6_dumpGimple.o  clang6_HardekopfLin_AA.o clang6_RangeAnalysis.o clang6_eSSA.o
	$(I386_CLANG6_PLUGIN_COMPILER) -fPIC -shared -fno-rtti -O3 clang6_plugin_dumpGimpleSSA.o clang6_dumpGimple.o clang6_HardekopfLin_AA.o clang6_RangeAnalysis.o clang6_eSSA.o -o clang6_plugin_dumpGimpleSSA.so -lbdd
else
$(I386_CLANG6_SSA_PLUGIN) : clang6_plugin_dumpGimpleSSA.o clang6_dumpGimple.o clang6_RangeAnalysis.o clang6_eSSA.o
	$(I386_CLANG6_PLUGIN_COMPILER) -fPIC -shared -fno-rtti -O3 clang6_plugin_dumpGimpleSSA.o clang6_dumpGimple.o clang6_RangeAnalysis.o clang6_eSSA.o -o clang6_plugin_dumpGimpleSSA.so
endif
if BUILD_PTAA
$(I386_CLANG6_SSA_PLUGINCPP) : clang6_plugin_dumpGimpleSSACpp.o clang6_dumpGimple.o clang6_HardekopfLin_AA.o clang6_RangeAnalysis.o clang6_eSSA.o
	$(I386_CLANG6_PLUGIN_COMPILER) -fPIC -shared -fno-rtti -O3 clang6_plugin_dumpGimpleSSACpp.o clang6_dumpGimple.o clang6_HardekopfLin_AA.o clang6_RangeAnalysis.o clang6_eSSA.o -o clang6_plugin_dumpGimpleSSACpp.so -lbdd
else
$(I386_CLANG6_SSA_PLUGINCPP) : clang6_plugin_dumpGimpleSSACpp.o clang6_dumpGimple.o clang6_RangeAnalysis.o clang6_eSSA.o
	$(I386_CLANG6_PLUGIN_COMPILER) -fPIC -shared -fno-rtti -O3 clang6_plugin_dumpGimpleSSACpp.o clang6_dumpGimple.o clang6_RangeAnalysis.o clang6_eSSA.o -o clang6_plugin_dumpGimpleSSACpp.so
endif

if BUILD_PTAA
$(I386_CLANG6_EMPTY_PLUGIN) : clang6_plugin_dumpGimpleEmpty.o clang6_dumpGimple.o clang6_HardekopfLin_AA.o clang6_RangeAnalysis.o clang6_eSSA.o
	$(I386_CLANG6_PLUGIN_COMPILER) -fPIC -shared -fno-rtti -O3 clang6_plugin_dumpGimpleEmpty.o clang6_dumpGimple.o clang6_HardekopfLin_AA.o clang6_RangeAnalysis.o clang6_eSSA.o -o clang6_plugin_dumpGimpleEmpty.so -lbdd
else
$(I386_CLANG6_EMPTY_PLUGIN) : clang6_plugin_dumpGimpleEmpty.o clang6_dumpGimple.o clang6_RangeAnalysis.o clang6_eSSA.o
	$(I386_CLANG6_PLUGIN_COMPILER) -fPIC -shared -fno-rtti -O3 clang6_plugin_dumpGimpleEmpty.o clang6_dumpGimple.o clang6_RangeAnalysis.o clang6_eSSA.o -o clang6_plugin_dumpGimpleEmpty.so
endif

$(I386_CLANG6_TOPFNAME_PLUGIN) : clang6_plugin_topfname.o 
	$(I386_CLANG6_PLUGIN_COMPILER) -fPIC -shared -fno-rtti -O3 clang6_plugin_topfname.o -o clang6_plugin_topfname.so

clang6_plugin_dumpGimpleEmpty.o : $(top_srcdir)/etc/clang_plugin/plugin_dumpGimpleEmpty.cpp $(top_srcdir)/etc/clang_plugin/plugin_includes.hpp 
	$(I386_CLANG6_PLUGIN_COMPILER) -std=c++11 -fPIC -fno-rtti -O3 -c $(top_srcdir)/etc/clang_plugin/plugin_dumpGimpleEmpty.cpp -o clang6_plugin_dumpGimpleEmpty.o -I$(I386_LLVM6_HEADER_DIR) $(WFLAGS)

clang6_plugin_dumpGimpleSSA.o : $(top_srcdir)/etc/clang_plugin/plugin_dumpGimpleSSA.cpp $(top_srcdir)/etc/clang_plugin/plugin_includes.hpp 
	$(I386_CLANG6_PLUGIN_COMPILER) -std=c++11 -fPIC -fno-rtti -O3 -c $(top_srcdir)/etc/clang_plugin/plugin_dumpGimpleSSA.cpp -o clang6_plugin_dumpGimpleSSA.o -I$(I386_LLVM6_HEADER_DIR) $(WFLAGS)

clang6_plugin_dumpGimpleSSACpp.o : $(top_srcdir)/etc/clang_plugin/plugin_dumpGimpleSSA.cpp $(top_srcdir)/etc/clang_plugin/plugin_includes.hpp 
	$(I386_CLANG6_PLUGIN_COMPILER) -std=c++11 -fPIC -fno-rtti -O3 -c $(top_srcdir)/etc/clang_plugin/plugin_dumpGimpleSSA.cpp -DCPP_LANGUAGE -o clang6_plugin_dumpGimpleSSACpp.o -I$(I386_LLVM6_HEADER_DIR) $(WFLAGS)

clang6_plugin_topfname.o : $(top_srcdir)/etc/clang_plugin/plugin_topfname.cpp $(top_srcdir)/etc/clang_plugin/plugin_includes.hpp 
	$(I386_CLANG6_PLUGIN_COMPILER) -std=c++11 -fPIC -fno-rtti -O3 -c $(top_srcdir)/etc/clang_plugin/plugin_topfname.cpp -o clang6_plugin_topfname.o -I$(I386_LLVM6_HEADER_DIR) $(WFLAGS)

if BUILD_PTAA
clang6_dumpGimple.o : $(top_srcdir)/etc/clang_plugin/dumpGimple.cpp $(top_srcdir)/etc/clang_plugin/plugin_includes.hpp $(top_srcdir)/etc/clang_plugin/HardekopfLin_AA.hpp $(top_srcdir)/etc/clang_plugin/llvm/RangeAnalysis.hpp  $(top_srcdir)/etc/clang_plugin/llvm/eSSA.hpp
	$(I386_CLANG6_PLUGIN_COMPILER) -std=c++11 -fPIC -fno-rtti -O3 -c $(top_srcdir)/etc/clang_plugin/dumpGimple.cpp -o clang6_dumpGimple.o -I. -I$(I386_LLVM6_HEADER_DIR) -DPANDA_PLUGIN_VERSION=\"$(PANDA_PLUGIN_VERSION)\" $(WFLAGS)
else
clang6_dumpGimple.o : $(top_srcdir)/etc/clang_plugin/dumpGimple.cpp $(top_srcdir)/etc/clang_plugin/plugin_includes.hpp $(top_srcdir)/etc/clang_plugin/llvm/RangeAnalysis.hpp  $(top_srcdir)/etc/clang_plugin/llvm/eSSA.hpp
	$(I386_CLANG6_PLUGIN_COMPILER) -std=c++11 -fPIC -fno-rtti -O3 -c $(top_srcdir)/etc/clang_plugin/dumpGimple.cpp -o clang6_dumpGimple.o -I. -I$(I386_LLVM6_HEADER_DIR) -DPANDA_PLUGIN_VERSION=\"$(PANDA_PLUGIN_VERSION)\" $(WFLAGS)
endif

if BUILD_PTAA
clang6_HardekopfLin_AA.o : $(top_srcdir)/etc/clang_plugin/HardekopfLin_AA.cpp $(top_srcdir)/etc/clang_plugin/HardekopfLin_AA.hpp
	$(I386_CLANG6_PLUGIN_COMPILER) -std=c++11 -fPIC -fno-rtti -O3 -c $(top_srcdir)/etc/clang_plugin/HardekopfLin_AA.cpp -o clang6_HardekopfLin_AA.o -I. -I$(I386_LLVM6_HEADER_DIR) -DPANDA_PLUGIN_VERSION=\"$(PANDA_PLUGIN_VERSION)\" $(WFLAGS)
endif

clang6_RangeAnalysis.o : $(top_srcdir)/etc/clang_plugin/llvm/RangeAnalysis.cpp $(top_srcdir)/etc/clang_plugin/llvm/RangeAnalysis.hpp
	$(I386_CLANG6_PLUGIN_COMPILER) -std=c++11 -fPIC -fno-rtti -O3 -c $(top_srcdir)/etc/clang_plugin/llvm/RangeAnalysis.cpp -o clang6_RangeAnalysis.o -I. -I$(I386_LLVM6_HEADER_DIR) -DPANDA_PLUGIN_VERSION=\"$(PANDA_PLUGIN_VERSION)\" $(WFLAGS)

clang6_eSSA.o : $(top_srcdir)/etc/clang_plugin/llvm/eSSA.cpp $(top_srcdir)/etc/clang_plugin/llvm/eSSA.hpp $(top_srcdir)/etc/clang_plugin/llvm/my_OrderedInstructions.hpp
	$(I386_CLANG6_PLUGIN_COMPILER) -std=c++11 -fPIC -fno-rtti -O3 -c $(top_srcdir)/etc/clang_plugin/llvm/eSSA.cpp -o clang6_eSSA.o -I. -I$(I386_LLVM6_HEADER_DIR) -DPANDA_PLUGIN_VERSION=\"$(PANDA_PLUGIN_VERSION)\" $(WFLAGS)


###### 

clean :
	rm -rf *.o *.so *.h

install: all
	if test "`ls *so`" != ""; then mkdir -p $(DESTDIR)$(PLUGIN_DIR); cp *.so $(DESTDIR)$(PLUGIN_DIR) -v; fi

#do not touch the following line


