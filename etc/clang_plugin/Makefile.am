#This is etc/clang_plugin/Makefile.am
EXTRA_DIST = $(top_srcdir)/etc/clang_plugin/plugin_dumpGimpleSSA.cpp $(top_srcdir)/etc/clang_plugin/plugin_includes.hpp $(top_srcdir)/etc/clang_plugin/plugin_dumpGimpleEmpty.cpp $(top_srcdir)/etc/clang_plugin/dumpGimple.cpp $(top_srcdir)/etc/clang_plugin/plugin_topfname.cpp gcc/cp-tree.def gcc/sync-builtins.def gcc/gtm-builtins.def gcc/sanitizer.def gcc/hsa-builtins.def gcc/c-common.def gcc/gimple.def gcc/builtins.def gcc/brig-builtins.def gcc/cilkplus.def gcc/omp-builtins.def gcc/tree.def gcc/cilk-builtins.def gcc/chkp-builtins.def gcc/COPYING3


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
WFLAGS=
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

$(I386_CLANG4_SSA_PLUGIN) : clang4_plugin_dumpGimpleSSA.o clang4_dumpGimple.o  
	$(I386_CLANG4_PLUGIN_COMPILER) -fPIC -shared -fno-rtti clang4_plugin_dumpGimpleSSA.o clang4_dumpGimple.o  -o clang4_plugin_dumpGimpleSSA.so

$(I386_CLANG4_SSA_PLUGINCPP) : clang4_plugin_dumpGimpleSSACpp.o clang4_dumpGimpleCpp.o  
	$(I386_CLANG4_PLUGIN_COMPILER) -fPIC -shared -fno-rtti clang4_plugin_dumpGimpleSSACpp.o clang4_dumpGimpleCpp.o  -o clang4_plugin_dumpGimpleSSACpp.so

$(I386_CLANG4_EMPTY_PLUGIN) : clang4_plugin_dumpGimpleEmpty.o clang4_dumpGimple.o  
	$(I386_CLANG4_PLUGIN_COMPILER) -fPIC -shared -fno-rtti clang4_plugin_dumpGimpleEmpty.o clang4_dumpGimple.o  -o clang4_plugin_dumpGimpleEmpty.so

$(I386_CLANG4_TOPFNAME_PLUGIN) : clang4_plugin_topfname.o 
	$(I386_CLANG4_PLUGIN_COMPILER) -fPIC -shared -fno-rtti clang4_plugin_topfname.o -o clang4_plugin_topfname.so

clang4_plugin_dumpGimpleEmpty.o : $(top_srcdir)/etc/clang_plugin/plugin_dumpGimpleEmpty.cpp $(top_srcdir)/etc/clang_plugin/plugin_includes.hpp 
	$(I386_CLANG4_PLUGIN_COMPILER) -std=c++11 -fPIC -fno-rtti -c $(top_srcdir)/etc/clang_plugin/plugin_dumpGimpleEmpty.cpp -o clang4_plugin_dumpGimpleEmpty.o -I$(I386_LLVM4_HEADER_DIR) $(WFLAGS)

clang4_plugin_dumpGimpleSSA.o : $(top_srcdir)/etc/clang_plugin/plugin_dumpGimpleSSA.cpp $(top_srcdir)/etc/clang_plugin/plugin_includes.hpp 
	$(I386_CLANG4_PLUGIN_COMPILER) -std=c++11 -fPIC -fno-rtti -c $(top_srcdir)/etc/clang_plugin/plugin_dumpGimpleSSA.cpp -o clang4_plugin_dumpGimpleSSA.o -I$(I386_LLVM4_HEADER_DIR) $(WFLAGS)

clang4_plugin_dumpGimpleSSACpp.o : $(top_srcdir)/etc/clang_plugin/plugin_dumpGimpleSSA.cpp $(top_srcdir)/etc/clang_plugin/plugin_includes.hpp 
	$(I386_CLANG4_PLUGIN_COMPILER) -std=c++11 -fPIC -fno-rtti -c $(top_srcdir)/etc/clang_plugin/plugin_dumpGimpleSSA.cpp -DCPP_LANGUAGE -o clang4_plugin_dumpGimpleSSACpp.o -I$(I386_LLVM4_HEADER_DIR) $(WFLAGS)

clang4_plugin_topfname.o : $(top_srcdir)/etc/clang_plugin/plugin_topfname.cpp $(top_srcdir)/etc/clang_plugin/plugin_includes.hpp 
	$(I386_CLANG4_PLUGIN_COMPILER) -std=c++11 -fPIC -fno-rtti -c $(top_srcdir)/etc/clang_plugin/plugin_topfname.cpp -o clang4_plugin_topfname.o -I$(I386_LLVM4_HEADER_DIR) $(WFLAGS)

clang4_dumpGimple.o : $(top_srcdir)/etc/clang_plugin/dumpGimple.cpp $(top_srcdir)/etc/clang_plugin/plugin_includes.hpp
	$(I386_CLANG4_PLUGIN_COMPILER) -std=c++11 -fPIC -fno-rtti -c $(top_srcdir)/etc/clang_plugin/dumpGimple.cpp -o clang4_dumpGimple.o -I. -I$(I386_LLVM4_HEADER_DIR) -DPANDA_PLUGIN_VERSION=\"$(PANDA_PLUGIN_VERSION)\" $(WFLAGS)

clang4_dumpGimpleCpp.o : $(top_srcdir)/etc/clang_plugin/dumpGimple.cpp $(top_srcdir)/etc/clang_plugin/plugin_includes.hpp 
	$(I386_CLANG4_PLUGIN_COMPILER) -std=c++11 -fPIC -fno-rtti -c $(top_srcdir)/etc/clang_plugin/dumpGimple.cpp -DCPP_LANGUAGE -o clang4_dumpGimpleCpp.o -I. -I$(I386_LLVM4_HEADER_DIR) -DPANDA_PLUGIN_VERSION=\"$(PANDA_PLUGIN_VERSION)\" $(WFLAGS)

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

$(I386_CLANG5_SSA_PLUGIN) : clang5_plugin_dumpGimpleSSA.o clang5_dumpGimple.o  
	$(I386_CLANG5_PLUGIN_COMPILER) -fPIC -shared -fno-rtti clang5_plugin_dumpGimpleSSA.o clang5_dumpGimple.o  -o clang5_plugin_dumpGimpleSSA.so

$(I386_CLANG5_SSA_PLUGINCPP) : clang5_plugin_dumpGimpleSSACpp.o clang5_dumpGimpleCpp.o  
	$(I386_CLANG5_PLUGIN_COMPILER) -fPIC -shared -fno-rtti clang5_plugin_dumpGimpleSSACpp.o clang5_dumpGimpleCpp.o  -o clang5_plugin_dumpGimpleSSACpp.so

$(I386_CLANG5_EMPTY_PLUGIN) : clang5_plugin_dumpGimpleEmpty.o clang5_dumpGimple.o  
	$(I386_CLANG5_PLUGIN_COMPILER) -fPIC -shared -fno-rtti clang5_plugin_dumpGimpleEmpty.o clang5_dumpGimple.o  -o clang5_plugin_dumpGimpleEmpty.so

$(I386_CLANG5_TOPFNAME_PLUGIN) : clang5_plugin_topfname.o 
	$(I386_CLANG5_PLUGIN_COMPILER) -fPIC -shared -fno-rtti clang5_plugin_topfname.o -o clang5_plugin_topfname.so

clang5_plugin_dumpGimpleEmpty.o : $(top_srcdir)/etc/clang_plugin/plugin_dumpGimpleEmpty.cpp $(top_srcdir)/etc/clang_plugin/plugin_includes.hpp 
	$(I386_CLANG5_PLUGIN_COMPILER) -std=c++11 -fPIC -fno-rtti -c $(top_srcdir)/etc/clang_plugin/plugin_dumpGimpleEmpty.cpp -o clang5_plugin_dumpGimpleEmpty.o -I$(I386_LLVM5_HEADER_DIR) $(WFLAGS)

clang5_plugin_dumpGimpleSSA.o : $(top_srcdir)/etc/clang_plugin/plugin_dumpGimpleSSA.cpp $(top_srcdir)/etc/clang_plugin/plugin_includes.hpp 
	$(I386_CLANG5_PLUGIN_COMPILER) -std=c++11 -fPIC -fno-rtti -c $(top_srcdir)/etc/clang_plugin/plugin_dumpGimpleSSA.cpp -o clang5_plugin_dumpGimpleSSA.o -I$(I386_LLVM5_HEADER_DIR) $(WFLAGS)

clang5_plugin_dumpGimpleSSACpp.o : $(top_srcdir)/etc/clang_plugin/plugin_dumpGimpleSSA.cpp $(top_srcdir)/etc/clang_plugin/plugin_includes.hpp 
	$(I386_CLANG5_PLUGIN_COMPILER) -std=c++11 -fPIC -fno-rtti -c $(top_srcdir)/etc/clang_plugin/plugin_dumpGimpleSSA.cpp -DCPP_LANGUAGE -o clang5_plugin_dumpGimpleSSACpp.o -I$(I386_LLVM5_HEADER_DIR) $(WFLAGS)

clang5_plugin_topfname.o : $(top_srcdir)/etc/clang_plugin/plugin_topfname.cpp $(top_srcdir)/etc/clang_plugin/plugin_includes.hpp 
	$(I386_CLANG5_PLUGIN_COMPILER) -std=c++11 -fPIC -fno-rtti -c $(top_srcdir)/etc/clang_plugin/plugin_topfname.cpp -o clang5_plugin_topfname.o -I$(I386_LLVM5_HEADER_DIR) $(WFLAGS)

clang5_dumpGimple.o : $(top_srcdir)/etc/clang_plugin/dumpGimple.cpp $(top_srcdir)/etc/clang_plugin/plugin_includes.hpp
	$(I386_CLANG5_PLUGIN_COMPILER) -std=c++11 -fPIC -fno-rtti -c $(top_srcdir)/etc/clang_plugin/dumpGimple.cpp -o clang5_dumpGimple.o -I. -I$(I386_LLVM5_HEADER_DIR) -DPANDA_PLUGIN_VERSION=\"$(PANDA_PLUGIN_VERSION)\" $(WFLAGS)

clang5_dumpGimpleCpp.o : $(top_srcdir)/etc/clang_plugin/dumpGimple.cpp $(top_srcdir)/etc/clang_plugin/plugin_includes.hpp 
	$(I386_CLANG5_PLUGIN_COMPILER) -std=c++11 -fPIC -fno-rtti -c $(top_srcdir)/etc/clang_plugin/dumpGimple.cpp -DCPP_LANGUAGE -o clang5_dumpGimpleCpp.o -I. -I$(I386_LLVM5_HEADER_DIR) -DPANDA_PLUGIN_VERSION=\"$(PANDA_PLUGIN_VERSION)\" $(WFLAGS)


###### 

clean :
	rm -rf *.o *.so *.h

install: all
	if test "`ls *so`" != ""; then cp *.so $(PLUGIN_DIR) -v; fi

#do not touch the following line


