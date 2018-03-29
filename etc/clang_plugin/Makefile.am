#This is etc/clang_plugin/Makefile.am
EXTRA_DIST = $(top_srcdir)/etc/clang_plugin/plugin_dumpGimpleSSA.cpp $(top_srcdir)/etc/clang_plugin/plugin_includes.hpp $(top_srcdir)/etc/clang_plugin/plugin_dumpGimpleEmpty.cpp $(top_srcdir)/etc/clang_plugin/dumpGimple.cpp $(top_srcdir)/etc/clang_plugin/plugin_topfname.cpp gcc/cp-tree.def gcc/sync-builtins.def gcc/gtm-builtins.def gcc/sanitizer.def gcc/hsa-builtins.def gcc/c-common.def gcc/gimple.def gcc/builtins.def gcc/brig-builtins.def gcc/cilkplus.def gcc/omp-builtins.def gcc/tree.def gcc/cilk-builtins.def gcc/chkp-builtins.def gcc/COPYING3 HardekopfLin_AA.hpp HardekopfLin_AA.cpp


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
WFLAGS=$(all_includes)
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
$(I386_CLANG4_SSA_PLUGIN) : clang4_plugin_dumpGimpleSSA.o clang4_dumpGimple.o clang4_HardekopfLin_AA.o
	$(I386_CLANG4_PLUGIN_COMPILER) -fPIC -shared -fno-rtti -O3 clang4_plugin_dumpGimpleSSA.o clang4_dumpGimple.o clang4_HardekopfLin_AA.o -o clang4_plugin_dumpGimpleSSA.so -lbdd
else
$(I386_CLANG4_SSA_PLUGIN) : clang4_plugin_dumpGimpleSSA.o clang4_dumpGimple.o
	$(I386_CLANG4_PLUGIN_COMPILER) -fPIC -shared -fno-rtti -O3 clang4_plugin_dumpGimpleSSA.o clang4_dumpGimple.o -o clang4_plugin_dumpGimpleSSA.so
endif

if BUILD_PTAA
$(I386_CLANG4_SSA_PLUGINCPP) : clang4_plugin_dumpGimpleSSACpp.o clang4_dumpGimple.o clang4_HardekopfLin_AA.o
	$(I386_CLANG4_PLUGIN_COMPILER) -fPIC -shared -fno-rtti -O3 clang4_plugin_dumpGimpleSSACpp.o clang4_dumpGimple.o clang4_HardekopfLin_AA.o -o clang4_plugin_dumpGimpleSSACpp.so -lbdd
else
$(I386_CLANG4_SSA_PLUGINCPP) : clang4_plugin_dumpGimpleSSACpp.o clang4_dumpGimple.o
	$(I386_CLANG4_PLUGIN_COMPILER) -fPIC -shared -fno-rtti -O3 clang4_plugin_dumpGimpleSSACpp.o clang4_dumpGimple.o -o clang4_plugin_dumpGimpleSSACpp.so
endif

if BUILD_PTAA
$(I386_CLANG4_EMPTY_PLUGIN) : clang4_plugin_dumpGimpleEmpty.o clang4_dumpGimple.o  clang4_HardekopfLin_AA.o
	$(I386_CLANG4_PLUGIN_COMPILER) -fPIC -shared -fno-rtti -O3 clang4_plugin_dumpGimpleEmpty.o clang4_dumpGimple.o clang4_HardekopfLin_AA.o -o clang4_plugin_dumpGimpleEmpty.so -lbdd
else
$(I386_CLANG4_EMPTY_PLUGIN) : clang4_plugin_dumpGimpleEmpty.o clang4_dumpGimple.o
	$(I386_CLANG4_PLUGIN_COMPILER) -fPIC -shared -fno-rtti -O3 clang4_plugin_dumpGimpleEmpty.o clang4_dumpGimple.o -o clang4_plugin_dumpGimpleEmpty.so
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
clang4_dumpGimple.o : $(top_srcdir)/etc/clang_plugin/dumpGimple.cpp $(top_srcdir)/etc/clang_plugin/plugin_includes.hpp $(top_srcdir)/etc/clang_plugin/HardekopfLin_AA.hpp
	$(I386_CLANG4_PLUGIN_COMPILER) -std=c++11 -fPIC -fno-rtti -O3 -c $(top_srcdir)/etc/clang_plugin/dumpGimple.cpp -o clang4_dumpGimple.o -I. -I$(I386_LLVM4_HEADER_DIR) -DPANDA_PLUGIN_VERSION=\"$(PANDA_PLUGIN_VERSION)\" $(WFLAGS)
else
clang4_dumpGimple.o : $(top_srcdir)/etc/clang_plugin/dumpGimple.cpp $(top_srcdir)/etc/clang_plugin/plugin_includes.hpp
	$(I386_CLANG4_PLUGIN_COMPILER) -std=c++11 -fPIC -fno-rtti -O3 -c $(top_srcdir)/etc/clang_plugin/dumpGimple.cpp -o clang4_dumpGimple.o -I. -I$(I386_LLVM4_HEADER_DIR) -DPANDA_PLUGIN_VERSION=\"$(PANDA_PLUGIN_VERSION)\" $(WFLAGS)
endif

if BUILD_PTAA
clang4_HardekopfLin_AA.o : $(top_srcdir)/etc/clang_plugin/HardekopfLin_AA.cpp $(top_srcdir)/etc/clang_plugin/HardekopfLin_AA.hpp
	$(I386_CLANG4_PLUGIN_COMPILER) -std=c++11 -fPIC -fno-rtti -O3 -c $(top_srcdir)/etc/clang_plugin/HardekopfLin_AA.cpp -o clang4_HardekopfLin_AA.o -I. -I$(I386_LLVM4_HEADER_DIR) -DPANDA_PLUGIN_VERSION=\"$(PANDA_PLUGIN_VERSION)\" $(WFLAGS)
endif

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
$(I386_CLANG5_SSA_PLUGIN) : clang5_plugin_dumpGimpleSSA.o clang5_dumpGimple.o  clang5_HardekopfLin_AA.o
	$(I386_CLANG5_PLUGIN_COMPILER) -fPIC -shared -fno-rtti -O3 clang5_plugin_dumpGimpleSSA.o clang5_dumpGimple.o clang5_HardekopfLin_AA.o -o clang5_plugin_dumpGimpleSSA.so -lbdd
else
$(I386_CLANG5_SSA_PLUGIN) : clang5_plugin_dumpGimpleSSA.o clang5_dumpGimple.o
	$(I386_CLANG5_PLUGIN_COMPILER) -fPIC -shared -fno-rtti -O3 clang5_plugin_dumpGimpleSSA.o clang5_dumpGimple.o -o clang5_plugin_dumpGimpleSSA.so
endif
if BUILD_PTAA
$(I386_CLANG5_SSA_PLUGINCPP) : clang5_plugin_dumpGimpleSSACpp.o clang5_dumpGimple.o clang5_HardekopfLin_AA.o
	$(I386_CLANG5_PLUGIN_COMPILER) -fPIC -shared -fno-rtti -O3 clang5_plugin_dumpGimpleSSACpp.o clang5_dumpGimple.o clang5_HardekopfLin_AA.o -o clang5_plugin_dumpGimpleSSACpp.so -lbdd
else
$(I386_CLANG5_SSA_PLUGINCPP) : clang5_plugin_dumpGimpleSSACpp.o clang5_dumpGimple.o
	$(I386_CLANG5_PLUGIN_COMPILER) -fPIC -shared -fno-rtti -O3 clang5_plugin_dumpGimpleSSACpp.o clang5_dumpGimple.o -o clang5_plugin_dumpGimpleSSACpp.so
endif

if BUILD_PTAA
$(I386_CLANG5_EMPTY_PLUGIN) : clang5_plugin_dumpGimpleEmpty.o clang5_dumpGimple.o clang5_HardekopfLin_AA.o
	$(I386_CLANG5_PLUGIN_COMPILER) -fPIC -shared -fno-rtti -O3 clang5_plugin_dumpGimpleEmpty.o clang5_dumpGimple.o clang5_HardekopfLin_AA.o -o clang5_plugin_dumpGimpleEmpty.so -lbdd
else
$(I386_CLANG5_EMPTY_PLUGIN) : clang5_plugin_dumpGimpleEmpty.o clang5_dumpGimple.o
	$(I386_CLANG5_PLUGIN_COMPILER) -fPIC -shared -fno-rtti -O3 clang5_plugin_dumpGimpleEmpty.o clang5_dumpGimple.o -o clang5_plugin_dumpGimpleEmpty.so
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
clang5_dumpGimple.o : $(top_srcdir)/etc/clang_plugin/dumpGimple.cpp $(top_srcdir)/etc/clang_plugin/plugin_includes.hpp $(top_srcdir)/etc/clang_plugin/HardekopfLin_AA.hpp
	$(I386_CLANG5_PLUGIN_COMPILER) -std=c++11 -fPIC -fno-rtti -O3 -c $(top_srcdir)/etc/clang_plugin/dumpGimple.cpp -o clang5_dumpGimple.o -I. -I$(I386_LLVM5_HEADER_DIR) -DPANDA_PLUGIN_VERSION=\"$(PANDA_PLUGIN_VERSION)\" $(WFLAGS)
else
clang5_dumpGimple.o : $(top_srcdir)/etc/clang_plugin/dumpGimple.cpp $(top_srcdir)/etc/clang_plugin/plugin_includes.hpp
	$(I386_CLANG5_PLUGIN_COMPILER) -std=c++11 -fPIC -fno-rtti -O3 -c $(top_srcdir)/etc/clang_plugin/dumpGimple.cpp -o clang5_dumpGimple.o -I. -I$(I386_LLVM5_HEADER_DIR) -DPANDA_PLUGIN_VERSION=\"$(PANDA_PLUGIN_VERSION)\" $(WFLAGS)
endif

if BUILD_PTAA
clang5_HardekopfLin_AA.o : $(top_srcdir)/etc/clang_plugin/HardekopfLin_AA.cpp $(top_srcdir)/etc/clang_plugin/HardekopfLin_AA.hpp
	$(I386_CLANG5_PLUGIN_COMPILER) -std=c++11 -fPIC -fno-rtti -O3 -c $(top_srcdir)/etc/clang_plugin/HardekopfLin_AA.cpp -o clang5_HardekopfLin_AA.o -I. -I$(I386_LLVM5_HEADER_DIR) -DPANDA_PLUGIN_VERSION=\"$(PANDA_PLUGIN_VERSION)\" $(WFLAGS)
endif


###### 

clean :
	rm -rf *.o *.so *.h

install: all
	if test "`ls *so`" != ""; then mkdir -p $(DESTDIR)$(PLUGIN_DIR); cp *.so $(DESTDIR)$(PLUGIN_DIR) -v; fi

#do not touch the following line


