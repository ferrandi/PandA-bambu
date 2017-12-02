#This is etc/clang_plugin/Makefile.am
EXTRA_DIST = $(top_srcdir)/etc/clang_plugin/plugin_dumpGimpleSSA.cpp $(top_srcdir)/etc/clang_plugin/plugin_includes.hpp $(top_srcdir)/etc/clang_plugin/plugin_dumpGimpleEmpty.cpp $(top_srcdir)/etc/clang_plugin/dumpGimple.cpp $(top_srcdir)/etc/clang_plugin/plugin_topfname.cpp

LD_LIBRARY_PATH=
PANDA_PLUGIN_VERSION=0.11
if ENABLE_WERROR
WFLAGS=-Wall \
   -Werror \
   -Wextra \
   -Wconversion \
   -Wno-deprecated \
   -Winit-self \
   -Wsign-conversion \
   -Wredundant-decls \
   -Wvla -Wshadow -Wlogical-op \
   -Wmissing-include-dirs \
   -Winit-self \
   -Wpointer-arith \
   -Wcast-qual \
   -Wcast-align \
   -Wswitch-enum \
   -Wswitch-default \
   -Wno-unused-function \
   -fdiagnostics-show-option
else
WFLAGS=
endif

FORCE:

check:all


###### clang 4.0
if BUILD_I386_CLANG40_EMPTY_PLUGIN
all: $(I386_CLANG40_EMPTY_PLUGIN)
endif

if BUILD_I386_CLANG40_SSA_PLUGIN
all: $(I386_CLANG40_SSA_PLUGIN)
endif

if BUILD_I386_CLANG40_SSA_PLUGINCPP
all: $(I386_CLANG40_SSA_PLUGINCPP)
endif

if BUILD_I386_CLANG40_TOPFNAME_PLUGIN
all: $(I386_CLANG40_TOPFNAME_PLUGIN)
endif

$(I386_CLANG40_SSA_PLUGIN) : clang40_plugin_dumpGimpleSSA.o clang40_dumpGimple.o  
	$(I386_CLANG40_PLUGIN_COMPILER) -fPIC -shared -fno-rtti clang40_plugin_dumpGimpleSSA.o clang40_dumpGimple.o  -o clang40_plugin_dumpGimpleSSA.so

$(I386_CLANG40_SSA_PLUGINCPP) : clang40_plugin_dumpGimpleSSA.o clang40_dumpGimpleCpp.o  
	$(I386_CLANG40_PLUGIN_COMPILER) -fPIC -shared -fno-rtti clang40_plugin_dumpGimpleSSA.o clang40_dumpGimpleCpp.o  -o clang40_plugin_dumpGimpleSSACpp.so

$(I386_CLANG40_EMPTY_PLUGIN) : clang40_plugin_dumpGimpleEmpty.o clang40_dumpGimple.o  
	$(I386_CLANG40_PLUGIN_COMPILER) -fPIC -shared -fno-rtti clang40_plugin_dumpGimpleEmpty.o clang40_dumpGimple.o  -o clang40_plugin_dumpGimpleEmpty.so

$(I386_CLANG40_TOPFNAME_PLUGIN) : clang40_plugin_topfname.o 
	$(I386_CLANG40_PLUGIN_COMPILER) -fPIC -shared -fno-rtti clang40_plugin_topfname.o -o clang40_plugin_topfname.so

clang40_plugin_dumpGimpleEmpty.o : $(top_srcdir)/etc/clang_plugin/plugin_dumpGimpleEmpty.cpp $(top_srcdir)/etc/clang_plugin/plugin_includes.hpp 
	$(I386_CLANG40_PLUGIN_COMPILER) -std=c++11 -fPIC -fno-rtti -c $(top_srcdir)/etc/clang_plugin/plugin_dumpGimpleEmpty.cpp -o clang40_plugin_dumpGimpleEmpty.o -I$(I386_LLVM40_HEADER_DIR) $(WFLAGS)

clang40_plugin_dumpGimpleSSA.o : $(top_srcdir)/etc/clang_plugin/plugin_dumpGimpleSSA.cpp $(top_srcdir)/etc/clang_plugin/plugin_includes.hpp 
	$(I386_CLANG40_PLUGIN_COMPILER) -std=c++11 -fPIC -fno-rtti -c $(top_srcdir)/etc/clang_plugin/plugin_dumpGimpleSSA.cpp -o clang40_plugin_dumpGimpleSSA.o -I$(I386_LLVM40_HEADER_DIR) $(WFLAGS)

clang40_plugin_topfname.o : $(top_srcdir)/etc/clang_plugin/plugin_topfname.cpp $(top_srcdir)/etc/clang_plugin/plugin_includes.hpp 
	$(I386_CLANG40_PLUGIN_COMPILER) -std=c++11 -fPIC -fno-rtti -c $(top_srcdir)/etc/clang_plugin/plugin_topfname.cpp -o clang40_plugin_topfname.o -I$(I386_LLVM40_HEADER_DIR) $(WFLAGS)

clang40_dumpGimple.o : $(top_srcdir)/etc/clang_plugin/dumpGimple.cpp $(top_srcdir)/etc/clang_plugin/plugin_includes.hpp
	$(I386_CLANG40_PLUGIN_COMPILER) -std=c++11 -fPIC -fno-rtti -c $(top_srcdir)/etc/clang_plugin/dumpGimple.cpp -o clang40_dumpGimple.o -I. -I$(I386_LLVM40_HEADER_DIR) -DPANDA_PLUGIN_VERSION=\"$(PANDA_PLUGIN_VERSION)\" $(WFLAGS)

clang40_dumpGimpleCpp.o : $(top_srcdir)/etc/clang_plugin/dumpGimple.cpp $(top_srcdir)/etc/clang_plugin/plugin_includes.hpp 
	$(I386_CLANG40_PLUGIN_COMPILER) -std=c++11 -fPIC -fno-rtti -c $(top_srcdir)/etc/clang_plugin/dumpGimple.cpp -DCPP_LANGUAGE -o clang40_dumpGimpleCpp.o -I. -I$(I386_LLVM40_HEADER_DIR) -DPANDA_PLUGIN_VERSION=\"$(PANDA_PLUGIN_VERSION)\" $(WFLAGS)





 : FORCE
	$(I386_CLANG40_PLUGIN_COMPILER) --version > new_
	if ! test -f ; then mv new_ ; else if ! diff -q   new_ > /dev/null; then mv new_ ; fi fi

###### 

clean :
	rm -rf *.o *.so *.h

install: all
	if test "`ls *so`" != ""; then cp *.so $(PLUGIN_DIR) -v; fi

#do not touch the following line


