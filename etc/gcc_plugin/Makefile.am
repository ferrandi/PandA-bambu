#This is etc/gcc_plugin/Makefile.am
EXTRA_DIST =   $(top_srcdir)/etc/gcc_plugin/plugin_includes.h $(top_srcdir)/etc/gcc_plugin/plugin_dumpGimpleEmpty.c $(top_srcdir)/etc/gcc_plugin/plugin_dumpGimpleSSA.c  $(top_srcdir)/etc/gcc_plugin/plugin_dumpGimpleSSAVRP.c $(top_srcdir)/etc/gcc_plugin/dumpGimple.c $(top_srcdir)/etc/gcc_plugin/dumpVrp.c $(top_srcdir)/etc/gcc_plugin/plugin_dumpRTL.c $(top_srcdir)/etc/gcc_plugin/plugin_topfname.c $(top_srcdir)/etc/gcc_plugin/cfgexpand_4_5_2.c VRP_data.h dumpVRP_common.h patched-tree-vrp.454.c patched-tree-vrp.463.c  patched-tree-vrp.472.c  patched-tree-vrp.480.c $(top_srcdir)/etc/gcc_plugin/ggc_clike_extern.h $(top_srcdir)/etc/gcc_plugin/ggc_clike_static.h $(top_srcdir)/etc/gcc_plugin/ggc_cpplike_static.h $(top_srcdir)/etc/gcc_plugin/ggc_cpplike_extern.h $(top_srcdir)/etc/gcc_plugin/gcc-4.9/dbgcnt.def $(top_srcdir)/etc/gcc_plugin/gcc-4.9/dbgcnt.h $(top_srcdir)/etc/gcc_plugin/gcc-4.9/expr.h $(top_srcdir)/etc/gcc_plugin/gcc-4.9/insn-config.h $(top_srcdir)/etc/gcc_plugin/gcc-4.9/ipa-inline.h $(top_srcdir)/etc/gcc_plugin/gcc-4.9/tree-sra.c $(top_srcdir)/etc/gcc_plugin/gcc-5/insn-config.h $(top_srcdir)/etc/gcc_plugin/gcc-5/tree-sra.c $(top_srcdir)/etc/gcc_plugin/gcc-6/tree-sra.c $(top_srcdir)/etc/gcc_plugin/gcc-7/tree-sra.c $(top_srcdir)/etc/gcc_plugin/gcc-8/tree-sra.c

LD_LIBRARY_PATH=
PANDA_PLUGIN_VERSION=0.12
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

###### gcc 4.5
if BUILD_I386_GCC45_EMPTY_PLUGIN
all: $(I386_GCC45_EMPTY_PLUGIN)
endif

if BUILD_I386_GCC45_SSA_PLUGIN
all: $(I386_GCC45_SSA_PLUGIN)
endif

if BUILD_I386_GCC45_SSA_PLUGINCPP
all: $(I386_GCC45_SSA_PLUGINCPP)
endif

if BUILD_I386_GCC45_SSAVRP_PLUGIN
all: $(I386_GCC45_SSAVRP_PLUGIN)
endif

if BUILD_I386_GCC45_TOPFNAME_PLUGIN
all: $(I386_GCC45_TOPFNAME_PLUGIN)
endif

$(I386_GCC45_SSA_PLUGIN) : gcc45_plugin_dumpGimpleSSA.o gcc45_dumpGimple.o gcc45_version.h
	$(I386_GCC45_PLUGIN_COMPILER) -shared gcc45_plugin_dumpGimpleSSA.o gcc45_dumpGimple.o -o gcc45_plugin_dumpGimpleSSA.so -fPIC

$(I386_GCC45_SSA_PLUGINCPP) : gcc45_plugin_dumpGimpleSSA.o gcc45_dumpGimpleCpp.o gcc45_version.h
	$(I386_GCC45_PLUGIN_COMPILER) -shared gcc45_plugin_dumpGimpleSSA.o gcc45_dumpGimpleCpp.o -o gcc45_plugin_dumpGimpleSSACpp.so -fPIC

$(I386_GCC45_SSAVRP_PLUGIN) : dumpVRP_common.h patched-tree-vrp.454.c  patched-tree-vrp.463.c  patched-tree-vrp.472.c  patched-tree-vrp.480.c gcc45_plugin_dumpGimpleSSAVRP.o gcc45_dumpGimple.o gcc45_version.h
	$(I386_GCC45_PLUGIN_COMPILER) -shared gcc45_plugin_dumpGimpleSSAVRP.o gcc45_dumpGimple.o -o gcc45_plugin_dumpGimpleSSAVRP.so -fPIC

$(I386_GCC45_EMPTY_PLUGIN) : gcc45_plugin_dumpGimpleEmpty.o gcc45_dumpGimple.o gcc45_version.h
	$(I386_GCC45_PLUGIN_COMPILER) -shared gcc45_plugin_dumpGimpleEmpty.o gcc45_dumpGimple.o -o gcc45_plugin_dumpGimpleEmpty.so -fPIC

$(I386_GCC45_TOPFNAME_PLUGIN) : gcc45_plugin_topfname.o gcc45_version.h
	$(I386_GCC45_PLUGIN_COMPILER) -shared gcc45_plugin_topfname.o -o gcc45_plugin_topfname.so -fPIC

gcc45_plugin_dumpGimpleEmpty.o : $(top_srcdir)/etc/gcc_plugin/plugin_dumpGimpleEmpty.c $(top_srcdir)/etc/gcc_plugin/plugin_includes.h gcc45_version.h
	$(I386_GCC45_PLUGIN_COMPILER) -c $(top_srcdir)/etc/gcc_plugin/plugin_dumpGimpleEmpty.c -o gcc45_plugin_dumpGimpleEmpty.o -I$(I386_GCC45_PLUGIN_DIR)/include -fPIC -DPANDA_PLUGIN_VERSION=\"$(PANDA_PLUGIN_VERSION)\"

gcc45_plugin_dumpGimpleSSA.o : $(top_srcdir)/etc/gcc_plugin/plugin_dumpGimpleSSA.c $(top_srcdir)/etc/gcc_plugin/plugin_includes.h gcc45_version.h
	$(I386_GCC45_PLUGIN_COMPILER) -c $(top_srcdir)/etc/gcc_plugin/plugin_dumpGimpleSSA.c -o gcc45_plugin_dumpGimpleSSA.o -I$(I386_GCC45_PLUGIN_DIR)/include -fPIC -DPANDA_PLUGIN_VERSION=\"$(PANDA_PLUGIN_VERSION)\"

gcc45_plugin_dumpGimpleSSAVRP.o : $(top_srcdir)/etc/gcc_plugin/plugin_dumpGimpleSSAVRP.c $(top_srcdir)/etc/gcc_plugin/plugin_includes.h gcc45_version.h
	$(I386_GCC45_PLUGIN_COMPILER) -c $(top_srcdir)/etc/gcc_plugin/plugin_dumpGimpleSSAVRP.c -o gcc45_plugin_dumpGimpleSSAVRP.o -I$(I386_GCC45_PLUGIN_DIR)/include -fPIC -DPANDA_PLUGIN_VERSION=\"$(PANDA_PLUGIN_VERSION)\"

gcc45_plugin_topfname.o : $(top_srcdir)/etc/gcc_plugin/plugin_topfname.c $(top_srcdir)/etc/gcc_plugin/plugin_includes.h gcc45_version.h
	$(I386_GCC45_PLUGIN_COMPILER) -c $(top_srcdir)/etc/gcc_plugin/plugin_topfname.c -o gcc45_plugin_topfname.o -I$(I386_GCC45_PLUGIN_DIR)/include -fPIC -DPANDA_PLUGIN_VERSION=\"$(PANDA_PLUGIN_VERSION)\"

gcc45_dumpGimple.o : $(top_srcdir)/etc/gcc_plugin/dumpGimple.c gcc45_version.h
	$(I386_GCC45_PLUGIN_COMPILER) -c $(top_srcdir)/etc/gcc_plugin/dumpGimple.c -o gcc45_dumpGimple.o  -I$(I386_GCC45_PLUGIN_DIR)/include -fPIC -DPANDA_PLUGIN_VERSION=\"$(PANDA_PLUGIN_VERSION)\"

gcc45_dumpGimpleCpp.o : $(top_srcdir)/etc/gcc_plugin/dumpGimple.c gcc45_version.h
	$(I386_GCC45_PLUGIN_COMPILER) -c $(top_srcdir)/etc/gcc_plugin/dumpGimple.c -DCPP_LANGUAGE -o gcc45_dumpGimpleCpp.o  -I$(I386_GCC45_PLUGIN_DIR)/include -fPIC -DPANDA_PLUGIN_VERSION=\"$(PANDA_PLUGIN_VERSION)\"

gcc45_version.h : FORCE
	$(I386_GCC45_PLUGIN_COMPILER) --version > new_gcc45_version.h
	if ! test -f gcc45_version.h; then mv new_gcc45_version.h gcc45_version.h; else if ! diff -q gcc45_version.h  new_gcc45_version.h > /dev/null; then mv new_gcc45_version.h gcc45_version.h; fi fi

###### 

###### gcc 4.6
if BUILD_I386_GCC46_EMPTY_PLUGIN
all: $(I386_GCC46_EMPTY_PLUGIN)
endif

if BUILD_I386_GCC46_SSA_PLUGIN
all: $(I386_GCC46_SSA_PLUGIN)
endif

if BUILD_I386_GCC46_SSA_PLUGINCPP
all: $(I386_GCC46_SSA_PLUGINCPP)
endif

if BUILD_I386_GCC46_SSAVRP_PLUGIN
all: $(I386_GCC46_SSAVRP_PLUGIN)
endif

if BUILD_I386_GCC46_TOPFNAME_PLUGIN
all: $(I386_GCC46_TOPFNAME_PLUGIN)
endif

$(I386_GCC46_SSA_PLUGIN) : gcc46_plugin_dumpGimpleSSA.o gcc46_dumpGimple.o gcc46_version.h
	$(I386_GCC46_PLUGIN_COMPILER) -shared gcc46_plugin_dumpGimpleSSA.o gcc46_dumpGimple.o -o gcc46_plugin_dumpGimpleSSA.so -fPIC

$(I386_GCC46_SSA_PLUGINCPP) : gcc46_plugin_dumpGimpleSSA.o gcc46_dumpGimpleCpp.o gcc46_version.h
	$(I386_GCC46_PLUGIN_COMPILER) -shared gcc46_plugin_dumpGimpleSSA.o gcc46_dumpGimpleCpp.o -o gcc46_plugin_dumpGimpleSSACpp.so -fPIC

$(I386_GCC46_SSAVRP_PLUGIN) : dumpVRP_common.h patched-tree-vrp.454.c  patched-tree-vrp.463.c  patched-tree-vrp.472.c  patched-tree-vrp.480.c gcc46_plugin_dumpGimpleSSAVRP.o gcc46_dumpGimple.o gcc46_version.h
	$(I386_GCC46_PLUGIN_COMPILER) -shared gcc46_plugin_dumpGimpleSSAVRP.o gcc46_dumpGimple.o -o gcc46_plugin_dumpGimpleSSAVRP.so -fPIC

$(I386_GCC46_EMPTY_PLUGIN) : gcc46_plugin_dumpGimpleEmpty.o gcc46_dumpGimple.o gcc46_version.h
	$(I386_GCC46_PLUGIN_COMPILER) -shared gcc46_plugin_dumpGimpleEmpty.o gcc46_dumpGimple.o -o gcc46_plugin_dumpGimpleEmpty.so -fPIC

$(I386_GCC46_TOPFNAME_PLUGIN) : gcc46_plugin_topfname.o gcc46_version.h
	$(I386_GCC46_PLUGIN_COMPILER) -shared gcc46_plugin_topfname.o -o gcc46_plugin_topfname.so -fPIC

gcc46_plugin_dumpGimpleEmpty.o : $(top_srcdir)/etc/gcc_plugin/plugin_dumpGimpleEmpty.c $(top_srcdir)/etc/gcc_plugin/plugin_includes.h gcc46_version.h
	$(I386_GCC46_PLUGIN_COMPILER) -c $(top_srcdir)/etc/gcc_plugin/plugin_dumpGimpleEmpty.c -o gcc46_plugin_dumpGimpleEmpty.o -I. -I$(I386_GCC46_PLUGIN_DIR)/include -fPIC -DPANDA_PLUGIN_VERSION=\"$(PANDA_PLUGIN_VERSION)\" $(WFLAGS)

gcc46_plugin_dumpGimpleSSA.o : $(top_srcdir)/etc/gcc_plugin/plugin_dumpGimpleSSA.c $(top_srcdir)/etc/gcc_plugin/plugin_includes.h gcc46_version.h
	$(I386_GCC46_PLUGIN_COMPILER) -c $(top_srcdir)/etc/gcc_plugin/plugin_dumpGimpleSSA.c -o gcc46_plugin_dumpGimpleSSA.o -I. -I$(I386_GCC46_PLUGIN_DIR)/include -fPIC -DPANDA_PLUGIN_VERSION=\"$(PANDA_PLUGIN_VERSION)\" $(WFLAGS)

gcc46_plugin_dumpGimpleSSAVRP.o : $(top_srcdir)/etc/gcc_plugin/plugin_dumpGimpleSSAVRP.c $(top_srcdir)/etc/gcc_plugin/plugin_includes.h gcc46_version.h
	$(I386_GCC46_PLUGIN_COMPILER) -c $(top_srcdir)/etc/gcc_plugin/plugin_dumpGimpleSSAVRP.c -o gcc46_plugin_dumpGimpleSSAVRP.o -I. -I$(I386_GCC46_PLUGIN_DIR)/include -fPIC -DPANDA_PLUGIN_VERSION=\"$(PANDA_PLUGIN_VERSION)\"

gcc46_plugin_topfname.o : $(top_srcdir)/etc/gcc_plugin/plugin_topfname.c $(top_srcdir)/etc/gcc_plugin/plugin_includes.h gcc46_version.h
	$(I386_GCC46_PLUGIN_COMPILER) -c $(top_srcdir)/etc/gcc_plugin/plugin_topfname.c -o gcc46_plugin_topfname.o -I$(I386_GCC46_PLUGIN_DIR)/include -fPIC -DPANDA_PLUGIN_VERSION=\"$(PANDA_PLUGIN_VERSION)\"

gtype_roots_gcc46.h : $(top_srcdir)/etc/gcc_plugin/dumpGimple.c $(top_srcdir)/etc/gcc_plugin/plugin_includes.h $(top_srcdir)/etc/gcc_plugin/ggc_clike_static.h $(top_srcdir)/etc/gcc_plugin/ggc_clike_extern.h
	$(I386_GCC46_GENGTYPE) -r  $(I386_GCC46_GTYPESTATE) -P $@ $(top_srcdir)/etc/gcc_plugin/dumpGimple.c $(I386_GCC46_PLUGIN_DIR)/include/splay-tree.h $(top_srcdir)/etc/gcc_plugin/plugin_includes.h $(top_srcdir)/etc/gcc_plugin/plugin_dumpGimpleSSA.c $(top_srcdir)/etc/gcc_plugin/ggc_clike_static.h $(top_srcdir)/etc/gcc_plugin/ggc_clike_extern.h

gcc46_dumpGimple.o : $(top_srcdir)/etc/gcc_plugin/dumpGimple.c gcc46_version.h gtype_roots_gcc46.h
	$(I386_GCC46_PLUGIN_COMPILER) -c $(top_srcdir)/etc/gcc_plugin/dumpGimple.c -o gcc46_dumpGimple.o  -I. -I$(I386_GCC46_PLUGIN_DIR)/include -fPIC -DPANDA_PLUGIN_VERSION=\"$(PANDA_PLUGIN_VERSION)\" $(WFLAGS)

gcc46_dumpGimpleCpp.o : $(top_srcdir)/etc/gcc_plugin/dumpGimple.c gcc46_version.h gtype_roots_gcc46.h
	$(I386_GCC46_PLUGIN_COMPILER) -c $(top_srcdir)/etc/gcc_plugin/dumpGimple.c -DCPP_LANGUAGE -o gcc46_dumpGimpleCpp.o  -I. -I$(I386_GCC46_PLUGIN_DIR)/include -fPIC -DPANDA_PLUGIN_VERSION=\"$(PANDA_PLUGIN_VERSION)\" $(WFLAGS)

gcc46_version.h : FORCE
	$(I386_GCC46_PLUGIN_COMPILER) --version > new_gcc46_version.h
	if ! test -f gcc46_version.h; then mv new_gcc46_version.h gcc46_version.h; else if ! diff -q gcc46_version.h  new_gcc46_version.h > /dev/null; then mv new_gcc46_version.h gcc46_version.h; fi fi

###### 
###### gcc 4.7
if BUILD_I386_GCC47_EMPTY_PLUGIN
all: $(I386_GCC47_EMPTY_PLUGIN)
endif

if BUILD_I386_GCC47_SSA_PLUGIN
all: $(I386_GCC47_SSA_PLUGIN)
endif

if BUILD_I386_GCC47_SSA_PLUGINCPP
all: $(I386_GCC47_SSA_PLUGINCPP)
endif

if BUILD_I386_GCC47_SSAVRP_PLUGIN
all: $(I386_GCC47_SSAVRP_PLUGIN)
endif

if BUILD_I386_GCC47_TOPFNAME_PLUGIN
all: $(I386_GCC47_TOPFNAME_PLUGIN)
endif

$(I386_GCC47_SSA_PLUGIN) : gcc47_plugin_dumpGimpleSSA.o gcc47_dumpGimple.o gcc47_version.h
	$(I386_GCC47_PLUGIN_COMPILER) -shared gcc47_plugin_dumpGimpleSSA.o gcc47_dumpGimple.o -o gcc47_plugin_dumpGimpleSSA.so -fPIC

$(I386_GCC47_SSA_PLUGINCPP) : gcc47_plugin_dumpGimpleSSA.o gcc47_dumpGimpleCpp.o gcc47_version.h
	$(I386_GCC47_PLUGIN_COMPILER) -shared gcc47_plugin_dumpGimpleSSA.o gcc47_dumpGimpleCpp.o -o gcc47_plugin_dumpGimpleSSACpp.so -fPIC

$(I386_GCC47_SSAVRP_PLUGIN) : dumpVRP_common.h patched-tree-vrp.454.c  patched-tree-vrp.463.c  patched-tree-vrp.472.c  patched-tree-vrp.480.c gcc47_plugin_dumpGimpleSSAVRP.o gcc47_dumpGimple.o gcc47_version.h
	$(I386_GCC47_PLUGIN_COMPILER) -shared gcc47_plugin_dumpGimpleSSAVRP.o gcc47_dumpGimple.o -o gcc47_plugin_dumpGimpleSSAVRP.so -fPIC

$(I386_GCC47_EMPTY_PLUGIN) : gcc47_plugin_dumpGimpleEmpty.o gcc47_dumpGimple.o gcc47_version.h
	$(I386_GCC47_PLUGIN_COMPILER) -shared gcc47_plugin_dumpGimpleEmpty.o gcc47_dumpGimple.o -o gcc47_plugin_dumpGimpleEmpty.so -fPIC

$(I386_GCC47_TOPFNAME_PLUGIN) : gcc47_plugin_topfname.o gcc47_version.h
	$(I386_GCC47_PLUGIN_COMPILER) -shared gcc47_plugin_topfname.o -o gcc47_plugin_topfname.so -fPIC

gcc47_plugin_dumpGimpleEmpty.o : $(top_srcdir)/etc/gcc_plugin/plugin_dumpGimpleEmpty.c $(top_srcdir)/etc/gcc_plugin/plugin_includes.h gcc47_version.h
	$(I386_GCC47_PLUGIN_COMPILER) -c $(top_srcdir)/etc/gcc_plugin/plugin_dumpGimpleEmpty.c -o gcc47_plugin_dumpGimpleEmpty.o -I$(I386_GCC47_PLUGIN_DIR)/include -fPIC -DPANDA_PLUGIN_VERSION=\"$(PANDA_PLUGIN_VERSION)\" $(WFLAGS)

gcc47_plugin_dumpGimpleSSA.o : $(top_srcdir)/etc/gcc_plugin/plugin_dumpGimpleSSA.c $(top_srcdir)/etc/gcc_plugin/plugin_includes.h gcc47_version.h
	$(I386_GCC47_PLUGIN_COMPILER) -c $(top_srcdir)/etc/gcc_plugin/plugin_dumpGimpleSSA.c -o gcc47_plugin_dumpGimpleSSA.o -I$(I386_GCC47_PLUGIN_DIR)/include -fPIC -DPANDA_PLUGIN_VERSION=\"$(PANDA_PLUGIN_VERSION)\" $(WFLAGS)

gcc47_plugin_dumpGimpleSSAVRP.o : $(top_srcdir)/etc/gcc_plugin/plugin_dumpGimpleSSAVRP.c $(top_srcdir)/etc/gcc_plugin/plugin_includes.h gcc47_version.h
	$(I386_GCC47_PLUGIN_COMPILER) -c $(top_srcdir)/etc/gcc_plugin/plugin_dumpGimpleSSAVRP.c -o gcc47_plugin_dumpGimpleSSAVRP.o -I$(I386_GCC47_PLUGIN_DIR)/include -fPIC -DPANDA_PLUGIN_VERSION=\"$(PANDA_PLUGIN_VERSION)\"

gcc47_plugin_topfname.o : $(top_srcdir)/etc/gcc_plugin/plugin_topfname.c $(top_srcdir)/etc/gcc_plugin/plugin_includes.h gcc47_version.h
	$(I386_GCC47_PLUGIN_COMPILER) -c $(top_srcdir)/etc/gcc_plugin/plugin_topfname.c -o gcc47_plugin_topfname.o -I$(I386_GCC47_PLUGIN_DIR)/include -fPIC -DPANDA_PLUGIN_VERSION=\"$(PANDA_PLUGIN_VERSION)\"

gtype_roots_gcc47.h : $(top_srcdir)/etc/gcc_plugin/dumpGimple.c $(top_srcdir)/etc/gcc_plugin/plugin_includes.h $(top_srcdir)/etc/gcc_plugin/ggc_clike_static.h $(top_srcdir)/etc/gcc_plugin/ggc_clike_extern.h
	$(I386_GCC47_GENGTYPE) -r  $(I386_GCC47_GTYPESTATE) -P $@ $(top_srcdir)/etc/gcc_plugin/dumpGimple.c $(I386_GCC47_PLUGIN_DIR)/include/splay-tree.h $(top_srcdir)/etc/gcc_plugin/plugin_includes.h $(top_srcdir)/etc/gcc_plugin/plugin_dumpGimpleSSA.c $(top_srcdir)/etc/gcc_plugin/ggc_clike_static.h $(top_srcdir)/etc/gcc_plugin/ggc_clike_extern.h

gcc47_dumpGimple.o : $(top_srcdir)/etc/gcc_plugin/dumpGimple.c gcc47_version.h gtype_roots_gcc47.h
	$(I386_GCC47_PLUGIN_COMPILER) -c $(top_srcdir)/etc/gcc_plugin/dumpGimple.c -o gcc47_dumpGimple.o -I. -I$(I386_GCC47_PLUGIN_DIR)/include -fPIC -DPANDA_PLUGIN_VERSION=\"$(PANDA_PLUGIN_VERSION)\" $(WFLAGS)

gcc47_dumpGimpleCpp.o : $(top_srcdir)/etc/gcc_plugin/dumpGimple.c gcc47_version.h gtype_roots_gcc47.h
	$(I386_GCC47_PLUGIN_COMPILER) -c $(top_srcdir)/etc/gcc_plugin/dumpGimple.c -DCPP_LANGUAGE -o gcc47_dumpGimpleCpp.o -I. -I$(I386_GCC47_PLUGIN_DIR)/include -fPIC -DPANDA_PLUGIN_VERSION=\"$(PANDA_PLUGIN_VERSION)\" $(WFLAGS)

gcc47_version.h : FORCE
	$(I386_GCC47_PLUGIN_COMPILER) --version > new_gcc47_version.h
	if ! test -f gcc47_version.h; then mv new_gcc47_version.h gcc47_version.h; else if ! diff -q gcc47_version.h  new_gcc47_version.h > /dev/null; then mv new_gcc47_version.h gcc47_version.h; fi fi

###### 
###### gcc 4.8
if BUILD_I386_GCC48_EMPTY_PLUGIN
all: $(I386_GCC48_EMPTY_PLUGIN)
endif

if BUILD_I386_GCC48_SSA_PLUGIN
all: $(I386_GCC48_SSA_PLUGIN)
endif

if BUILD_I386_GCC48_SSA_PLUGINCPP
all: $(I386_GCC48_SSA_PLUGINCPP)
endif

if BUILD_I386_GCC48_SSAVRP_PLUGIN
all: $(I386_GCC48_SSAVRP_PLUGIN)
endif

if BUILD_I386_GCC48_TOPFNAME_PLUGIN
all: $(I386_GCC48_TOPFNAME_PLUGIN)
endif

$(I386_GCC48_SSA_PLUGIN) : gcc48_plugin_dumpGimpleSSA.o gcc48_dumpGimple.o gcc48_version.h
	$(I386_GCC48_PLUGIN_COMPILER) -shared gcc48_plugin_dumpGimpleSSA.o gcc48_dumpGimple.o -o gcc48_plugin_dumpGimpleSSA.so -fPIC

$(I386_GCC48_SSA_PLUGINCPP) : gcc48_plugin_dumpGimpleSSA.o gcc48_dumpGimpleCpp.o gcc48_version.h
	$(I386_GCC48_PLUGIN_COMPILER) -shared gcc48_plugin_dumpGimpleSSA.o gcc48_dumpGimpleCpp.o -o gcc48_plugin_dumpGimpleSSACpp.so -fPIC

$(I386_GCC48_SSAVRP_PLUGIN) : dumpVRP_common.h patched-tree-vrp.454.c  patched-tree-vrp.463.c  patched-tree-vrp.472.c  patched-tree-vrp.480.c gcc48_plugin_dumpGimpleSSAVRP.o gcc48_dumpGimple.o gcc48_version.h
	$(I386_GCC48_PLUGIN_COMPILER) -shared gcc48_plugin_dumpGimpleSSAVRP.o gcc48_dumpGimple.o -o gcc48_plugin_dumpGimpleSSAVRP.so -fPIC

$(I386_GCC48_EMPTY_PLUGIN) : gcc48_plugin_dumpGimpleEmpty.o gcc48_dumpGimple.o gcc48_version.h
	$(I386_GCC48_PLUGIN_COMPILER) -shared gcc48_plugin_dumpGimpleEmpty.o gcc48_dumpGimple.o -o gcc48_plugin_dumpGimpleEmpty.so -fPIC

$(I386_GCC48_TOPFNAME_PLUGIN) : gcc48_plugin_topfname.o gcc48_version.h
	$(I386_GCC48_PLUGIN_COMPILER) -shared gcc48_plugin_topfname.o -o gcc48_plugin_topfname.so -fPIC

gcc48_plugin_dumpGimpleEmpty.o : $(top_srcdir)/etc/gcc_plugin/plugin_dumpGimpleEmpty.c $(top_srcdir)/etc/gcc_plugin/plugin_includes.h gcc48_version.h
	$(I386_GCC48_PLUGIN_COMPILER) -c $(top_srcdir)/etc/gcc_plugin/plugin_dumpGimpleEmpty.c -o gcc48_plugin_dumpGimpleEmpty.o -I$(I386_GCC48_PLUGIN_DIR)/include -fPIC -DPANDA_PLUGIN_VERSION=\"$(PANDA_PLUGIN_VERSION)\" $(WFLAGS)

gcc48_plugin_dumpGimpleSSA.o : $(top_srcdir)/etc/gcc_plugin/plugin_dumpGimpleSSA.c $(top_srcdir)/etc/gcc_plugin/plugin_includes.h gcc48_version.h
	$(I386_GCC48_PLUGIN_COMPILER) -c $(top_srcdir)/etc/gcc_plugin/plugin_dumpGimpleSSA.c -o gcc48_plugin_dumpGimpleSSA.o -I$(I386_GCC48_PLUGIN_DIR)/include -fPIC -DPANDA_PLUGIN_VERSION=\"$(PANDA_PLUGIN_VERSION)\" $(WFLAGS)

gcc48_plugin_dumpGimpleSSAVRP.o : $(top_srcdir)/etc/gcc_plugin/plugin_dumpGimpleSSAVRP.c $(top_srcdir)/etc/gcc_plugin/plugin_includes.h gcc48_version.h
	$(I386_GCC48_PLUGIN_COMPILER) -c $(top_srcdir)/etc/gcc_plugin/plugin_dumpGimpleSSAVRP.c -o gcc48_plugin_dumpGimpleSSAVRP.o -I$(I386_GCC48_PLUGIN_DIR)/include -fPIC -DPANDA_PLUGIN_VERSION=\"$(PANDA_PLUGIN_VERSION)\"

gcc48_plugin_topfname.o : $(top_srcdir)/etc/gcc_plugin/plugin_topfname.c $(top_srcdir)/etc/gcc_plugin/plugin_includes.h gcc48_version.h
	$(I386_GCC48_PLUGIN_COMPILER) -c $(top_srcdir)/etc/gcc_plugin/plugin_topfname.c -o gcc48_plugin_topfname.o -I$(I386_GCC48_PLUGIN_DIR)/include -fPIC -DPANDA_PLUGIN_VERSION=\"$(PANDA_PLUGIN_VERSION)\"

gcc48_dumpGimple.o : $(top_srcdir)/etc/gcc_plugin/dumpGimple.c gcc48_version.h gtype_roots_gcc48.h
	$(I386_GCC48_PLUGIN_COMPILER) -c $(top_srcdir)/etc/gcc_plugin/dumpGimple.c -o gcc48_dumpGimple.o -I. -I$(I386_GCC48_PLUGIN_DIR)/include -fPIC -DPANDA_PLUGIN_VERSION=\"$(PANDA_PLUGIN_VERSION)\" $(WFLAGS)

gcc48_dumpGimpleCpp.o : $(top_srcdir)/etc/gcc_plugin/dumpGimple.c gcc48_version.h gtype_roots_gcc48.h
	$(I386_GCC48_PLUGIN_COMPILER) -c $(top_srcdir)/etc/gcc_plugin/dumpGimple.c -DCPP_LANGUAGE -o gcc48_dumpGimpleCpp.o -I. -I$(I386_GCC48_PLUGIN_DIR)/include -fPIC -DPANDA_PLUGIN_VERSION=\"$(PANDA_PLUGIN_VERSION)\" $(WFLAGS)

gtype_roots_gcc48.h : $(top_srcdir)/etc/gcc_plugin/dumpGimple.c $(top_srcdir)/etc/gcc_plugin/plugin_includes.h $(top_srcdir)/etc/gcc_plugin/ggc_clike_static.h $(top_srcdir)/etc/gcc_plugin/ggc_clike_extern.h
	$(I386_GCC48_GENGTYPE) -r  $(I386_GCC48_GTYPESTATE) -P $@ $(top_srcdir)/etc/gcc_plugin/dumpGimple.c $(I386_GCC48_PLUGIN_DIR)/include/splay-tree.h $(top_srcdir)/etc/gcc_plugin/plugin_includes.h $(top_srcdir)/etc/gcc_plugin/plugin_dumpGimpleSSA.c $(top_srcdir)/etc/gcc_plugin/ggc_clike_static.h $(top_srcdir)/etc/gcc_plugin/ggc_clike_extern.h

gcc48_version.h : FORCE
	$(I386_GCC48_PLUGIN_COMPILER) --version > new_gcc48_version.h
	if ! test -f gcc48_version.h; then mv new_gcc48_version.h gcc48_version.h; else if ! diff -q gcc48_version.h  new_gcc48_version.h > /dev/null; then mv new_gcc48_version.h gcc48_version.h; fi fi

###### 

###### gcc 4.9
if BUILD_I386_GCC49_EMPTY_PLUGIN
all: $(I386_GCC49_EMPTY_PLUGIN)
endif

if BUILD_I386_GCC49_SSA_PLUGIN
all: $(I386_GCC49_SSA_PLUGIN)
endif

if BUILD_I386_GCC49_SSA_PLUGINCPP
all: $(I386_GCC49_SSA_PLUGINCPP)
endif

if BUILD_I386_GCC49_TOPFNAME_PLUGIN
all: $(I386_GCC49_TOPFNAME_PLUGIN)
endif

$(I386_GCC49_SSA_PLUGIN) : gcc49_plugin_dumpGimpleSSA.o gcc49_dumpGimple.o gcc49_tree-sra.o gcc49_version.h
	case $(host_os) in \
	  mingw*) \
	    flexlink -chain mingw -o gcc49_plugin_dumpGimpleSSA.so gcc49_plugin_dumpGimpleSSA.o gcc49_dumpGimple.o gcc49_tree-sra.o -lstdc++;;\
	  *) \
	    $(I386_GCC49_PLUGIN_COMPILER) -fPIC -shared -fno-rtti gcc49_plugin_dumpGimpleSSA.o gcc49_dumpGimple.o gcc49_tree-sra.o -o gcc49_plugin_dumpGimpleSSA.so;; \
	esac;

$(I386_GCC49_SSA_PLUGINCPP) : gcc49_plugin_dumpGimpleSSA.o gcc49_dumpGimpleCpp.o gcc49_tree-sra.o gcc49_version.h
	case $(host_os) in \
	  mingw*) \
	    flexlink -chain mingw -o gcc49_plugin_dumpGimpleSSACpp.so gcc49_plugin_dumpGimpleSSA.o gcc49_dumpGimpleCpp.o gcc49_tree-sra.o -lstdc++;;\
	  *) \
	    $(I386_GCC49_PLUGIN_COMPILER) -fPIC -shared -fno-rtti gcc49_plugin_dumpGimpleSSA.o gcc49_dumpGimpleCpp.o gcc49_tree-sra.o -o gcc49_plugin_dumpGimpleSSACpp.so;; \
	esac;

$(I386_GCC49_EMPTY_PLUGIN) : gcc49_plugin_dumpGimpleEmpty.o gcc49_dumpGimple.o gcc49_tree-sra.o gcc49_version.h
	case $(host_os) in \
	  mingw*) \
	    flexlink -chain mingw -o gcc49_plugin_dumpGimpleEmpty.so gcc49_plugin_dumpGimpleEmpty.o gcc49_dumpGimple.o gcc49_tree-sra.o -lstdc++;;\
	  *) \
	    $(I386_GCC49_PLUGIN_COMPILER) -fPIC -shared -fno-rtti gcc49_plugin_dumpGimpleEmpty.o gcc49_dumpGimple.o gcc49_tree-sra.o -o gcc49_plugin_dumpGimpleEmpty.so;;\
	esac;

$(I386_GCC49_TOPFNAME_PLUGIN) : gcc49_plugin_topfname.o gcc49_version.h
	case $(host_os) in \
	  mingw*) \
	    flexlink -chain mingw -o gcc49_plugin_topfname.so gcc49_plugin_topfname.o -lstdc++;;\
	  *) \
	    $(I386_GCC49_PLUGIN_COMPILER) -fPIC -shared -fno-rtti gcc49_plugin_topfname.o -o gcc49_plugin_topfname.so;;\
	esac;

gcc49_plugin_dumpGimpleEmpty.o : $(top_srcdir)/etc/gcc_plugin/plugin_dumpGimpleEmpty.c $(top_srcdir)/etc/gcc_plugin/plugin_includes.h gcc49_version.h
	case $(host_os) in \
	  mingw*) \
	    $(I386_GCC49_PLUGIN_COMPILER) -x c++ -c -fno-exceptions -fno-rtti -shared -O2 $(top_srcdir)/etc/gcc_plugin/plugin_dumpGimpleEmpty.c -o gcc49_plugin_dumpGimpleEmpty.o -I$(I386_GCC49_PLUGIN_DIR)/include $(WFLAGS);;\
	  *) \
	    $(I386_GCC49_PLUGIN_COMPILER) -x c++ -fPIC -fno-rtti -shared -c $(top_srcdir)/etc/gcc_plugin/plugin_dumpGimpleEmpty.c -o gcc49_plugin_dumpGimpleEmpty.o -I$(I386_GCC49_PLUGIN_DIR)/include $(WFLAGS);;\
	esac;

gcc49_plugin_dumpGimpleSSA.o : $(top_srcdir)/etc/gcc_plugin/plugin_dumpGimpleSSA.c $(top_srcdir)/etc/gcc_plugin/plugin_includes.h gcc49_version.h
	case $(host_os) in \
	  mingw*) \
	    $(I386_GCC49_PLUGIN_COMPILER) -x c++ -fno-exceptions -fno-rtti -shared -O2 -c $(top_srcdir)/etc/gcc_plugin/plugin_dumpGimpleSSA.c -o gcc49_plugin_dumpGimpleSSA.o -I$(I386_GCC49_PLUGIN_DIR)/include $(WFLAGS);;\
	  *) \
	    $(I386_GCC49_PLUGIN_COMPILER) -x c++ -fPIC -fno-rtti -shared -c $(top_srcdir)/etc/gcc_plugin/plugin_dumpGimpleSSA.c -o gcc49_plugin_dumpGimpleSSA.o -I$(I386_GCC49_PLUGIN_DIR)/include $(WFLAGS);;\
	esac;

gcc49_plugin_topfname.o : $(top_srcdir)/etc/gcc_plugin/plugin_topfname.c $(top_srcdir)/etc/gcc_plugin/plugin_includes.h gcc49_version.h
	case $(host_os) in \
	  mingw*) \
	    $(I386_GCC49_PLUGIN_COMPILER) -x c++ -fno-exceptions -fno-rtti -shared -O2 -c $(top_srcdir)/etc/gcc_plugin/plugin_topfname.c -o gcc49_plugin_topfname.o -I$(I386_GCC49_PLUGIN_DIR)/include $(WFLAGS);;\
	  *) \
	    $(I386_GCC49_PLUGIN_COMPILER) -x c++ -fPIC -fno-rtti -shared -c $(top_srcdir)/etc/gcc_plugin/plugin_topfname.c -o gcc49_plugin_topfname.o -I$(I386_GCC49_PLUGIN_DIR)/include $(WFLAGS);;\
	esac;

gcc49_dumpGimple.o : $(top_srcdir)/etc/gcc_plugin/dumpGimple.c gcc49_version.h gtype_roots_gcc49.h 
	case $(host_os) in \
	  mingw*) \
	    $(I386_GCC49_PLUGIN_COMPILER) -x c++ -fno-exceptions -fno-rtti -shared -O2 -c $(top_srcdir)/etc/gcc_plugin/dumpGimple.c -o gcc49_dumpGimple.o -I. -I$(I386_GCC49_PLUGIN_DIR)/include -DPANDA_PLUGIN_VERSION=\"$(PANDA_PLUGIN_VERSION)\" $(WFLAGS);;\
	  *) \
	    $(I386_GCC49_PLUGIN_COMPILER) -x c++ -fPIC -fno-rtti -shared -c $(top_srcdir)/etc/gcc_plugin/dumpGimple.c -o gcc49_dumpGimple.o -I. -I$(I386_GCC49_PLUGIN_DIR)/include -DPANDA_PLUGIN_VERSION=\"$(PANDA_PLUGIN_VERSION)\" $(WFLAGS);;\
	esac;

gcc49_dumpGimpleCpp.o : $(top_srcdir)/etc/gcc_plugin/dumpGimple.c gcc49_version.h gtype_roots_gcc49.h 
	case $(host_os) in \
	  mingw*) \
	    $(I386_GCC49_PLUGIN_COMPILER) -x c++ -fno-exceptions -fno-rtti -shared -O2 -c $(top_srcdir)/etc/gcc_plugin/dumpGimple.c -DCPP_LANGUAGE -o gcc49_dumpGimpleCpp.o -I. -I$(I386_GCC49_PLUGIN_DIR)/include -DPANDA_PLUGIN_VERSION=\"$(PANDA_PLUGIN_VERSION)\" $(WFLAGS);;\
	  *) \
	    $(I386_GCC49_PLUGIN_COMPILER) -x c++ -fPIC -fno-rtti -shared -c $(top_srcdir)/etc/gcc_plugin/dumpGimple.c -DCPP_LANGUAGE -o gcc49_dumpGimpleCpp.o -I. -I$(I386_GCC49_PLUGIN_DIR)/include -DPANDA_PLUGIN_VERSION=\"$(PANDA_PLUGIN_VERSION)\" $(WFLAGS);;\
	esac;

gcc49_tree-sra.o : $(top_srcdir)/etc/gcc_plugin/gcc-4.9/tree-sra.c
	case $(host_os) in \
	  mingw*) \
	    $(I386_GCC49_PLUGIN_COMPILER) -x c++ -fno-exceptions -fno-rtti -shared -O2 -c $(top_srcdir)/etc/gcc_plugin/gcc-4.9/tree-sra.c -o gcc49_tree-sra.o -I. -I$(I386_GCC49_PLUGIN_DIR)/include -DPANDA_PLUGIN_VERSION=\"$(PANDA_PLUGIN_VERSION)\" -DIN_GCC;;\
	  *) \
	    $(I386_GCC49_PLUGIN_COMPILER) -x c++ -fPIC -fno-rtti -shared -c $(top_srcdir)/etc/gcc_plugin/gcc-4.9/tree-sra.c -o gcc49_tree-sra.o -I. -I$(I386_GCC49_PLUGIN_DIR)/include -DPANDA_PLUGIN_VERSION=\"$(PANDA_PLUGIN_VERSION)\" -DIN_GCC ;;\
	esac;



gtype_roots_gcc49.h : $(top_srcdir)/etc/gcc_plugin/dumpGimple.c $(top_srcdir)/etc/gcc_plugin/plugin_includes.h $(top_srcdir)/etc/gcc_plugin/ggc_clike_static.h $(top_srcdir)/etc/gcc_plugin/ggc_clike_extern.h
	$(I386_GCC49_GENGTYPE) -r  $(I386_GCC49_GTYPESTATE) -P $@ $(top_srcdir)/etc/gcc_plugin/dumpGimple.c $(I386_GCC49_PLUGIN_DIR)/include/splay-tree.h $(top_srcdir)/etc/gcc_plugin/plugin_includes.h $(top_srcdir)/etc/gcc_plugin/plugin_dumpGimpleSSA.c $(top_srcdir)/etc/gcc_plugin/ggc_clike_static.h $(top_srcdir)/etc/gcc_plugin/ggc_clike_extern.h

gcc49_version.h : FORCE
	$(I386_GCC49_PLUGIN_COMPILER) --version > new_gcc49_version.h
	if ! test -f gcc49_version.h; then mv new_gcc49_version.h gcc49_version.h; else if ! diff -q gcc49_version.h  new_gcc49_version.h > /dev/null; then mv new_gcc49_version.h gcc49_version.h; fi fi

###### 

###### gcc 5
if BUILD_I386_GCC5_EMPTY_PLUGIN
all: $(I386_GCC5_EMPTY_PLUGIN)
endif

if BUILD_I386_GCC5_SSA_PLUGIN
all: $(I386_GCC5_SSA_PLUGIN)
endif

if BUILD_I386_GCC5_SSA_PLUGINCPP
all: $(I386_GCC5_SSA_PLUGINCPP)
endif

if BUILD_I386_GCC5_TOPFNAME_PLUGIN
all: $(I386_GCC5_TOPFNAME_PLUGIN)
endif

$(I386_GCC5_SSA_PLUGIN) : gcc5_plugin_dumpGimpleSSA.o gcc5_dumpGimple.o  gcc5_tree-sra.o gcc5_version.h
	$(I386_GCC5_PLUGIN_COMPILER) -fPIC -shared -fno-rtti gcc5_plugin_dumpGimpleSSA.o gcc5_dumpGimple.o gcc5_tree-sra.o -o gcc5_plugin_dumpGimpleSSA.so

$(I386_GCC5_SSA_PLUGINCPP) : gcc5_plugin_dumpGimpleSSA.o gcc5_dumpGimpleCpp.o  gcc5_tree-sra.o gcc5_version.h
	$(I386_GCC5_PLUGIN_COMPILER) -fPIC -shared -fno-rtti gcc5_plugin_dumpGimpleSSA.o gcc5_dumpGimpleCpp.o gcc5_tree-sra.o -o gcc5_plugin_dumpGimpleSSACpp.so

$(I386_GCC5_EMPTY_PLUGIN) : gcc5_plugin_dumpGimpleEmpty.o gcc5_dumpGimple.o gcc5_tree-sra.o gcc5_version.h
	$(I386_GCC5_PLUGIN_COMPILER) -fPIC -shared -fno-rtti gcc5_plugin_dumpGimpleEmpty.o gcc5_dumpGimple.o  gcc5_tree-sra.o -o gcc5_plugin_dumpGimpleEmpty.so

$(I386_GCC5_TOPFNAME_PLUGIN) : gcc5_plugin_topfname.o gcc5_version.h
	$(I386_GCC5_PLUGIN_COMPILER) -fPIC -shared -fno-rtti gcc5_plugin_topfname.o -o gcc5_plugin_topfname.so

gcc5_plugin_dumpGimpleEmpty.o : $(top_srcdir)/etc/gcc_plugin/plugin_dumpGimpleEmpty.c $(top_srcdir)/etc/gcc_plugin/plugin_includes.h gcc5_version.h
	$(I386_GCC5_PLUGIN_COMPILER) -x c++ -fPIC -fno-rtti -c $(top_srcdir)/etc/gcc_plugin/plugin_dumpGimpleEmpty.c -o gcc5_plugin_dumpGimpleEmpty.o -I$(I386_GCC5_PLUGIN_DIR)/include

gcc5_plugin_dumpGimpleSSA.o : $(top_srcdir)/etc/gcc_plugin/plugin_dumpGimpleSSA.c $(top_srcdir)/etc/gcc_plugin/plugin_includes.h gcc5_version.h
	$(I386_GCC5_PLUGIN_COMPILER) -x c++ -fPIC -fno-rtti -c $(top_srcdir)/etc/gcc_plugin/plugin_dumpGimpleSSA.c -o gcc5_plugin_dumpGimpleSSA.o -I$(I386_GCC5_PLUGIN_DIR)/include 

gcc5_plugin_topfname.o : $(top_srcdir)/etc/gcc_plugin/plugin_topfname.c $(top_srcdir)/etc/gcc_plugin/plugin_includes.h gcc5_version.h
	$(I386_GCC5_PLUGIN_COMPILER) -x c++ -fPIC -fno-rtti -c $(top_srcdir)/etc/gcc_plugin/plugin_topfname.c -o gcc5_plugin_topfname.o -I$(I386_GCC5_PLUGIN_DIR)/include

gcc5_dumpGimple.o : $(top_srcdir)/etc/gcc_plugin/dumpGimple.c gcc5_version.h gtype_roots_gcc5.h 
	$(I386_GCC5_PLUGIN_COMPILER) -x c++ -fPIC -fno-rtti -c $(top_srcdir)/etc/gcc_plugin/dumpGimple.c -o gcc5_dumpGimple.o -I. -I$(I386_GCC5_PLUGIN_DIR)/include -DPANDA_PLUGIN_VERSION=\"$(PANDA_PLUGIN_VERSION)\" 

gcc5_dumpGimpleCpp.o : $(top_srcdir)/etc/gcc_plugin/dumpGimple.c gcc5_version.h gtype_roots_gcc5.h 
	$(I386_GCC5_PLUGIN_COMPILER) -x c++ -fPIC -fno-rtti -c $(top_srcdir)/etc/gcc_plugin/dumpGimple.c -DCPP_LANGUAGE -o gcc5_dumpGimpleCpp.o -I. -I$(I386_GCC5_PLUGIN_DIR)/include -DPANDA_PLUGIN_VERSION=\"$(PANDA_PLUGIN_VERSION)\" 

gcc5_tree-sra.o : $(top_srcdir)/etc/gcc_plugin/gcc-5/tree-sra.c
	    $(I386_GCC5_PLUGIN_COMPILER) -x c++ -fPIC -fno-rtti -c $(top_srcdir)/etc/gcc_plugin/gcc-5/tree-sra.c -o gcc5_tree-sra.o -I. -I$(I386_GCC5_PLUGIN_DIR)/include -DPANDA_PLUGIN_VERSION=\"$(PANDA_PLUGIN_VERSION)\" -DIN_GCC 

gtype_roots_gcc5.h : $(top_srcdir)/etc/gcc_plugin/dumpGimple.c $(top_srcdir)/etc/gcc_plugin/plugin_includes.h $(top_srcdir)/etc/gcc_plugin/ggc_cpplike_static.h $(top_srcdir)/etc/gcc_plugin/ggc_cpplike_extern.h
	$(I386_GCC5_GENGTYPE) -r  $(I386_GCC5_GTYPESTATE) -P $@ $(top_srcdir)/etc/gcc_plugin/dumpGimple.c $(I386_GCC5_PLUGIN_DIR)/include/splay-tree.h $(top_srcdir)/etc/gcc_plugin/plugin_includes.h $(top_srcdir)/etc/gcc_plugin/plugin_dumpGimpleSSA.c $(top_srcdir)/etc/gcc_plugin/ggc_cpplike_static.h $(top_srcdir)/etc/gcc_plugin/ggc_cpplike_extern.h

gcc5_version.h : FORCE
	$(I386_GCC5_PLUGIN_COMPILER) --version > new_gcc5_version.h
	if ! test -f gcc5_version.h; then mv new_gcc5_version.h gcc5_version.h; else if ! diff -q gcc5_version.h  new_gcc5_version.h > /dev/null; then mv new_gcc5_version.h gcc5_version.h; fi fi

###### 

###### gcc 6
if BUILD_I386_GCC6_EMPTY_PLUGIN
all: $(I386_GCC6_EMPTY_PLUGIN)
endif

if BUILD_I386_GCC6_SSA_PLUGIN
all: $(I386_GCC6_SSA_PLUGIN)
endif

if BUILD_I386_GCC6_SSA_PLUGINCPP
all: $(I386_GCC6_SSA_PLUGINCPP)
endif

if BUILD_I386_GCC6_TOPFNAME_PLUGIN
all: $(I386_GCC6_TOPFNAME_PLUGIN)
endif

$(I386_GCC6_SSA_PLUGIN) : gcc6_plugin_dumpGimpleSSA.o gcc6_dumpGimple.o gcc6_tree-sra.o gcc6_version.h
	$(I386_GCC6_PLUGIN_COMPILER) -fPIC -shared -fno-rtti gcc6_plugin_dumpGimpleSSA.o gcc6_dumpGimple.o gcc6_tree-sra.o -o gcc6_plugin_dumpGimpleSSA.so

$(I386_GCC6_SSA_PLUGINCPP) : gcc6_plugin_dumpGimpleSSA.o gcc6_dumpGimpleCpp.o gcc6_tree-sra.o gcc6_version.h
	$(I386_GCC6_PLUGIN_COMPILER) -fPIC -shared -fno-rtti gcc6_plugin_dumpGimpleSSA.o gcc6_dumpGimpleCpp.o gcc6_tree-sra.o -o gcc6_plugin_dumpGimpleSSACpp.so

$(I386_GCC6_EMPTY_PLUGIN) : gcc6_plugin_dumpGimpleEmpty.o gcc6_dumpGimple.o gcc6_tree-sra.o gcc6_version.h
	$(I386_GCC6_PLUGIN_COMPILER) -fPIC -shared -fno-rtti gcc6_plugin_dumpGimpleEmpty.o gcc6_dumpGimple.o gcc6_tree-sra.o -o gcc6_plugin_dumpGimpleEmpty.so

$(I386_GCC6_TOPFNAME_PLUGIN) : gcc6_plugin_topfname.o gcc6_version.h
	$(I386_GCC6_PLUGIN_COMPILER) -fPIC -shared -fno-rtti gcc6_plugin_topfname.o -o gcc6_plugin_topfname.so

gcc6_plugin_dumpGimpleEmpty.o : $(top_srcdir)/etc/gcc_plugin/plugin_dumpGimpleEmpty.c $(top_srcdir)/etc/gcc_plugin/plugin_includes.h gcc6_version.h
	$(I386_GCC6_PLUGIN_COMPILER) -x c++ -fPIC -fno-rtti -c $(top_srcdir)/etc/gcc_plugin/plugin_dumpGimpleEmpty.c -o gcc6_plugin_dumpGimpleEmpty.o -I$(I386_GCC6_PLUGIN_DIR)/include

gcc6_plugin_dumpGimpleSSA.o : $(top_srcdir)/etc/gcc_plugin/plugin_dumpGimpleSSA.c $(top_srcdir)/etc/gcc_plugin/plugin_includes.h gcc6_version.h
	$(I386_GCC6_PLUGIN_COMPILER) -x c++ -fPIC -fno-rtti -c $(top_srcdir)/etc/gcc_plugin/plugin_dumpGimpleSSA.c -o gcc6_plugin_dumpGimpleSSA.o -I$(I386_GCC6_PLUGIN_DIR)/include 

gcc6_plugin_topfname.o : $(top_srcdir)/etc/gcc_plugin/plugin_topfname.c $(top_srcdir)/etc/gcc_plugin/plugin_includes.h gcc6_version.h
	$(I386_GCC6_PLUGIN_COMPILER) -x c++ -fPIC -fno-rtti -c $(top_srcdir)/etc/gcc_plugin/plugin_topfname.c -o gcc6_plugin_topfname.o -I$(I386_GCC6_PLUGIN_DIR)/include

gcc6_dumpGimple.o : $(top_srcdir)/etc/gcc_plugin/dumpGimple.c gcc6_version.h gtype_roots_gcc6.h 
	$(I386_GCC6_PLUGIN_COMPILER) -x c++ -fPIC -fno-rtti -c $(top_srcdir)/etc/gcc_plugin/dumpGimple.c -o gcc6_dumpGimple.o -I. -I$(I386_GCC6_PLUGIN_DIR)/include -DPANDA_PLUGIN_VERSION=\"$(PANDA_PLUGIN_VERSION)\" 

gcc6_dumpGimpleCpp.o : $(top_srcdir)/etc/gcc_plugin/dumpGimple.c gcc6_version.h gtype_roots_gcc6.h 
	$(I386_GCC6_PLUGIN_COMPILER) -x c++ -fPIC -fno-rtti -c $(top_srcdir)/etc/gcc_plugin/dumpGimple.c -DCPP_LANGUAGE -o gcc6_dumpGimpleCpp.o -I. -I$(I386_GCC6_PLUGIN_DIR)/include -DPANDA_PLUGIN_VERSION=\"$(PANDA_PLUGIN_VERSION)\" 

gcc6_tree-sra.o : $(top_srcdir)/etc/gcc_plugin/gcc-6/tree-sra.c
	    $(I386_GCC6_PLUGIN_COMPILER) -x c++ -fPIC -fno-rtti -c $(top_srcdir)/etc/gcc_plugin/gcc-6/tree-sra.c -o gcc6_tree-sra.o -I. -I$(I386_GCC6_PLUGIN_DIR)/include -DPANDA_PLUGIN_VERSION=\"$(PANDA_PLUGIN_VERSION)\" -DIN_GCC 

gtype_roots_gcc6.h : $(top_srcdir)/etc/gcc_plugin/dumpGimple.c $(top_srcdir)/etc/gcc_plugin/plugin_includes.h $(top_srcdir)/etc/gcc_plugin/ggc_cpplike_static.h $(top_srcdir)/etc/gcc_plugin/ggc_cpplike_extern.h
	$(I386_GCC6_GENGTYPE) -r  $(I386_GCC6_GTYPESTATE) -P $@ $(top_srcdir)/etc/gcc_plugin/dumpGimple.c $(I386_GCC6_PLUGIN_DIR)/include/splay-tree.h $(top_srcdir)/etc/gcc_plugin/plugin_includes.h $(top_srcdir)/etc/gcc_plugin/plugin_dumpGimpleSSA.c $(top_srcdir)/etc/gcc_plugin/ggc_cpplike_static.h $(top_srcdir)/etc/gcc_plugin/ggc_cpplike_extern.h

gcc6_version.h : FORCE
	$(I386_GCC6_PLUGIN_COMPILER) --version > new_gcc6_version.h
	if ! test -f gcc6_version.h; then mv new_gcc6_version.h gcc6_version.h; else if ! diff -q gcc6_version.h  new_gcc6_version.h > /dev/null; then mv new_gcc6_version.h gcc6_version.h; fi fi

###### 

###### gcc 7
if BUILD_I386_GCC7_EMPTY_PLUGIN
all: $(I386_GCC7_EMPTY_PLUGIN)
endif

if BUILD_I386_GCC7_SSA_PLUGIN
all: $(I386_GCC7_SSA_PLUGIN)
endif

if BUILD_I386_GCC7_SSA_PLUGINCPP
all: $(I386_GCC7_SSA_PLUGINCPP)
endif

if BUILD_I386_GCC7_TOPFNAME_PLUGIN
all: $(I386_GCC7_TOPFNAME_PLUGIN)
endif

$(I386_GCC7_SSA_PLUGIN) : gcc7_plugin_dumpGimpleSSA.o gcc7_dumpGimple.o gcc7_tree-sra.o gcc7_version.h
	$(I386_GCC7_PLUGIN_COMPILER) -fPIC -shared -fno-rtti gcc7_plugin_dumpGimpleSSA.o gcc7_dumpGimple.o gcc7_tree-sra.o -o gcc7_plugin_dumpGimpleSSA.so

$(I386_GCC7_SSA_PLUGINCPP) : gcc7_plugin_dumpGimpleSSA.o gcc7_dumpGimpleCpp.o gcc7_tree-sra.o gcc7_version.h
	$(I386_GCC7_PLUGIN_COMPILER) -fPIC -shared -fno-rtti gcc7_plugin_dumpGimpleSSA.o gcc7_dumpGimpleCpp.o gcc7_tree-sra.o -o gcc7_plugin_dumpGimpleSSACpp.so

$(I386_GCC7_EMPTY_PLUGIN) : gcc7_plugin_dumpGimpleEmpty.o gcc7_dumpGimple.o gcc7_tree-sra.o gcc7_version.h
	$(I386_GCC7_PLUGIN_COMPILER) -fPIC -shared -fno-rtti gcc7_plugin_dumpGimpleEmpty.o gcc7_dumpGimple.o gcc7_tree-sra.o -o gcc7_plugin_dumpGimpleEmpty.so

$(I386_GCC7_TOPFNAME_PLUGIN) : gcc7_plugin_topfname.o gcc7_version.h
	$(I386_GCC7_PLUGIN_COMPILER) -fPIC -shared -fno-rtti gcc7_plugin_topfname.o -o gcc7_plugin_topfname.so

gcc7_plugin_dumpGimpleEmpty.o : $(top_srcdir)/etc/gcc_plugin/plugin_dumpGimpleEmpty.c $(top_srcdir)/etc/gcc_plugin/plugin_includes.h gcc7_version.h
	$(I386_GCC7_PLUGIN_COMPILER) -x c++ -fPIC -fno-rtti -c $(top_srcdir)/etc/gcc_plugin/plugin_dumpGimpleEmpty.c -o gcc7_plugin_dumpGimpleEmpty.o -I$(I386_GCC7_PLUGIN_DIR)/include

gcc7_plugin_dumpGimpleSSA.o : $(top_srcdir)/etc/gcc_plugin/plugin_dumpGimpleSSA.c $(top_srcdir)/etc/gcc_plugin/plugin_includes.h gcc7_version.h
	$(I386_GCC7_PLUGIN_COMPILER) -x c++ -fPIC -fno-rtti -c $(top_srcdir)/etc/gcc_plugin/plugin_dumpGimpleSSA.c -o gcc7_plugin_dumpGimpleSSA.o -I$(I386_GCC7_PLUGIN_DIR)/include 

gcc7_plugin_topfname.o : $(top_srcdir)/etc/gcc_plugin/plugin_topfname.c $(top_srcdir)/etc/gcc_plugin/plugin_includes.h gcc7_version.h
	$(I386_GCC7_PLUGIN_COMPILER) -x c++ -fPIC -fno-rtti -c $(top_srcdir)/etc/gcc_plugin/plugin_topfname.c -o gcc7_plugin_topfname.o -I$(I386_GCC7_PLUGIN_DIR)/include

gcc7_dumpGimple.o : $(top_srcdir)/etc/gcc_plugin/dumpGimple.c gcc7_version.h gtype_roots_gcc7.h 
	$(I386_GCC7_PLUGIN_COMPILER) -x c++ -fPIC -fno-rtti -c $(top_srcdir)/etc/gcc_plugin/dumpGimple.c -o gcc7_dumpGimple.o -I. -I$(I386_GCC7_PLUGIN_DIR)/include -DPANDA_PLUGIN_VERSION=\"$(PANDA_PLUGIN_VERSION)\" 

gcc7_dumpGimpleCpp.o : $(top_srcdir)/etc/gcc_plugin/dumpGimple.c gcc7_version.h gtype_roots_gcc7.h 
	$(I386_GCC7_PLUGIN_COMPILER) -x c++ -fPIC -fno-rtti -c $(top_srcdir)/etc/gcc_plugin/dumpGimple.c -DCPP_LANGUAGE -o gcc7_dumpGimpleCpp.o -I. -I$(I386_GCC7_PLUGIN_DIR)/include -DPANDA_PLUGIN_VERSION=\"$(PANDA_PLUGIN_VERSION)\" 

gcc7_tree-sra.o : $(top_srcdir)/etc/gcc_plugin/gcc-7/tree-sra.c
	    $(I386_GCC7_PLUGIN_COMPILER) -x c++ -fPIC -fno-rtti -c $(top_srcdir)/etc/gcc_plugin/gcc-7/tree-sra.c -o gcc7_tree-sra.o -I. -I$(I386_GCC7_PLUGIN_DIR)/include -DPANDA_PLUGIN_VERSION=\"$(PANDA_PLUGIN_VERSION)\" -DIN_GCC 

gtype_roots_gcc7.h : $(top_srcdir)/etc/gcc_plugin/dumpGimple.c $(top_srcdir)/etc/gcc_plugin/plugin_includes.h $(top_srcdir)/etc/gcc_plugin/ggc_cpplike_static.h $(top_srcdir)/etc/gcc_plugin/ggc_cpplike_extern.h
	$(I386_GCC7_GENGTYPE) -r  $(I386_GCC7_GTYPESTATE) -P $@ $(top_srcdir)/etc/gcc_plugin/dumpGimple.c $(I386_GCC7_PLUGIN_DIR)/include/splay-tree.h $(top_srcdir)/etc/gcc_plugin/plugin_includes.h $(top_srcdir)/etc/gcc_plugin/plugin_dumpGimpleSSA.c $(top_srcdir)/etc/gcc_plugin/ggc_cpplike_static.h $(top_srcdir)/etc/gcc_plugin/ggc_cpplike_extern.h

gcc7_version.h : FORCE
	$(I386_GCC7_PLUGIN_COMPILER) --version > new_gcc7_version.h
	if ! test -f gcc7_version.h; then mv new_gcc7_version.h gcc7_version.h; else if ! diff -q gcc7_version.h  new_gcc7_version.h > /dev/null; then mv new_gcc7_version.h gcc7_version.h; fi fi

###### 

###### gcc 8
if BUILD_I386_GCC8_EMPTY_PLUGIN
all: $(I386_GCC8_EMPTY_PLUGIN)
endif

if BUILD_I386_GCC8_SSA_PLUGIN
all: $(I386_GCC8_SSA_PLUGIN)
endif

if BUILD_I386_GCC8_SSA_PLUGINCPP
all: $(I386_GCC8_SSA_PLUGINCPP)
endif

if BUILD_I386_GCC8_TOPFNAME_PLUGIN
all: $(I386_GCC8_TOPFNAME_PLUGIN)
endif

$(I386_GCC8_SSA_PLUGIN) : gcc8_plugin_dumpGimpleSSA.o gcc8_dumpGimple.o gcc8_tree-sra.o gcc8_version.h
	case $(host_os) in \
	  mingw*) \
	    $(I386_GCC8_PLUGIN_COMPILER) -shared -Wl,--export-all-symbols -fno-rtti gcc8_plugin_dumpGimpleSSA.o gcc8_dumpGimple.o gcc8_tree-sra.o -o gcc8_plugin_dumpGimpleSSA.so $(I386_GCC8_PLUGIN_DIR)/cc1.exe.a;; \
	  darwin*) \
	    $(I386_GCC8_PLUGIN_COMPILER) -fPIC -shared -undefined dynamic_lookup -fno-rtti gcc8_plugin_dumpGimpleSSA.o gcc8_dumpGimple.o gcc8_tree-sra.o -o gcc8_plugin_dumpGimpleSSA.so;; \
	  *) \
	    $(I386_GCC8_PLUGIN_COMPILER) -fPIC -shared -fno-rtti gcc8_plugin_dumpGimpleSSA.o gcc8_dumpGimple.o gcc8_tree-sra.o -o gcc8_plugin_dumpGimpleSSA.so;; \
	esac;

$(I386_GCC8_SSA_PLUGINCPP) : gcc8_plugin_dumpGimpleSSA.o gcc8_dumpGimpleCpp.o gcc8_tree-sra.o gcc8_version.h
	case $(host_os) in \
	  mingw*) \
	    $(I386_GCC8_PLUGIN_COMPILER) -shared -Wl,--export-all-symbols -fno-rtti gcc8_plugin_dumpGimpleSSA.o gcc8_dumpGimpleCpp.o gcc8_tree-sra.o -o gcc8_plugin_dumpGimpleSSACpp.so $(I386_GCC8_PLUGIN_DIR)/cc1plus.exe.a;; \
	  darwin*) \
	    $(I386_GCC8_PLUGIN_COMPILER) -fPIC -shared -undefined dynamic_lookup -fno-rtti gcc8_plugin_dumpGimpleSSA.o gcc8_dumpGimpleCpp.o gcc8_tree-sra.o -o gcc8_plugin_dumpGimpleSSACpp.so;; \
	  *) \
	    $(I386_GCC8_PLUGIN_COMPILER) -fPIC -shared -fno-rtti gcc8_plugin_dumpGimpleSSA.o gcc8_dumpGimpleCpp.o gcc8_tree-sra.o -o gcc8_plugin_dumpGimpleSSACpp.so;; \
	esac;

$(I386_GCC8_EMPTY_PLUGIN) : gcc8_plugin_dumpGimpleEmpty.o gcc8_dumpGimple.o gcc8_tree-sra.o gcc8_version.h
	case $(host_os) in \
	  mingw*) \
	    $(I386_GCC8_PLUGIN_COMPILER) -shared -Wl,--export-all-symbols -fno-rtti gcc8_plugin_dumpGimpleEmpty.o gcc8_dumpGimple.o gcc8_tree-sra.o -o gcc8_plugin_dumpGimpleEmpty.so $(I386_GCC8_PLUGIN_DIR)/cc1.exe.a;; \
	  darwin*) \
	    $(I386_GCC8_PLUGIN_COMPILER) -fPIC -shared -undefined dynamic_lookup -fno-rtti gcc8_plugin_dumpGimpleEmpty.o gcc8_dumpGimple.o gcc8_tree-sra.o -o gcc8_plugin_dumpGimpleEmpty.so;; \
	  *) \
	    $(I386_GCC8_PLUGIN_COMPILER) -fPIC -shared -fno-rtti gcc8_plugin_dumpGimpleEmpty.o gcc8_dumpGimple.o gcc8_tree-sra.o -o gcc8_plugin_dumpGimpleEmpty.so;; \
	esac;

$(I386_GCC8_TOPFNAME_PLUGIN) : gcc8_plugin_topfname.o gcc8_version.h
	case $(host_os) in \
	  mingw*) \
	    $(I386_GCC8_PLUGIN_COMPILER) -shared -Wl,--export-all-symbols -fno-rtti gcc8_plugin_topfname.o -o gcc8_plugin_topfname.so $(I386_GCC8_PLUGIN_DIR)/cc1.exe.a;; \
	  darwin*) \
	    $(I386_GCC8_PLUGIN_COMPILER) -fPIC -shared -undefined dynamic_lookup -fno-rtti gcc8_plugin_topfname.o -o gcc8_plugin_topfname.so;; \
	  *) \
	    $(I386_GCC8_PLUGIN_COMPILER) -fPIC -shared -fno-rtti gcc8_plugin_topfname.o -o gcc8_plugin_topfname.so;; \
	esac;

gcc8_plugin_dumpGimpleEmpty.o : $(top_srcdir)/etc/gcc_plugin/plugin_dumpGimpleEmpty.c $(top_srcdir)/etc/gcc_plugin/plugin_includes.h gcc8_version.h
	$(I386_GCC8_PLUGIN_COMPILER) -x c++ -fPIC -fno-rtti -c $(top_srcdir)/etc/gcc_plugin/plugin_dumpGimpleEmpty.c -o gcc8_plugin_dumpGimpleEmpty.o -I$(I386_GCC8_PLUGIN_DIR)/include

gcc8_plugin_dumpGimpleSSA.o : $(top_srcdir)/etc/gcc_plugin/plugin_dumpGimpleSSA.c $(top_srcdir)/etc/gcc_plugin/plugin_includes.h gcc8_version.h
	$(I386_GCC8_PLUGIN_COMPILER) -x c++ -fPIC -fno-rtti -c $(top_srcdir)/etc/gcc_plugin/plugin_dumpGimpleSSA.c -o gcc8_plugin_dumpGimpleSSA.o -I$(I386_GCC8_PLUGIN_DIR)/include 

gcc8_plugin_topfname.o : $(top_srcdir)/etc/gcc_plugin/plugin_topfname.c $(top_srcdir)/etc/gcc_plugin/plugin_includes.h gcc8_version.h
	$(I386_GCC8_PLUGIN_COMPILER) -x c++ -fPIC -fno-rtti -c $(top_srcdir)/etc/gcc_plugin/plugin_topfname.c -o gcc8_plugin_topfname.o -I$(I386_GCC8_PLUGIN_DIR)/include

gcc8_dumpGimple.o : $(top_srcdir)/etc/gcc_plugin/dumpGimple.c gcc8_version.h gtype_roots_gcc8.h 
	$(I386_GCC8_PLUGIN_COMPILER) -x c++ -fPIC -fno-rtti -c $(top_srcdir)/etc/gcc_plugin/dumpGimple.c -o gcc8_dumpGimple.o -I. -I$(I386_GCC8_PLUGIN_DIR)/include -DPANDA_PLUGIN_VERSION=\"$(PANDA_PLUGIN_VERSION)\" 

gcc8_dumpGimpleCpp.o : $(top_srcdir)/etc/gcc_plugin/dumpGimple.c gcc8_version.h gtype_roots_gcc8.h 
	$(I386_GCC8_PLUGIN_COMPILER) -x c++ -fPIC -fno-rtti -c $(top_srcdir)/etc/gcc_plugin/dumpGimple.c -DCPP_LANGUAGE -o gcc8_dumpGimpleCpp.o -I. -I$(I386_GCC8_PLUGIN_DIR)/include -DPANDA_PLUGIN_VERSION=\"$(PANDA_PLUGIN_VERSION)\" 

gcc8_tree-sra.o : $(top_srcdir)/etc/gcc_plugin/gcc-8/tree-sra.c
	    $(I386_GCC8_PLUGIN_COMPILER) -x c++ -fPIC -fno-rtti -c $(top_srcdir)/etc/gcc_plugin/gcc-8/tree-sra.c -o gcc8_tree-sra.o -I. -I$(I386_GCC8_PLUGIN_DIR)/include -DPANDA_PLUGIN_VERSION=\"$(PANDA_PLUGIN_VERSION)\" -DIN_GCC 

gtype_roots_gcc8.h : $(top_srcdir)/etc/gcc_plugin/dumpGimple.c $(top_srcdir)/etc/gcc_plugin/plugin_includes.h $(top_srcdir)/etc/gcc_plugin/ggc_cpplike_static.h $(top_srcdir)/etc/gcc_plugin/ggc_cpplike_extern.h
	$(I386_GCC8_GENGTYPE) -r  $(I386_GCC8_GTYPESTATE) -P $@ $(top_srcdir)/etc/gcc_plugin/dumpGimple.c $(I386_GCC8_PLUGIN_DIR)/include/splay-tree.h $(top_srcdir)/etc/gcc_plugin/plugin_includes.h $(top_srcdir)/etc/gcc_plugin/plugin_dumpGimpleSSA.c $(top_srcdir)/etc/gcc_plugin/ggc_cpplike_static.h $(top_srcdir)/etc/gcc_plugin/ggc_cpplike_extern.h

gcc8_version.h : FORCE
	$(I386_GCC8_PLUGIN_COMPILER) --version > new_gcc8_version.h
	if ! test -f gcc8_version.h; then mv new_gcc8_version.h gcc8_version.h; else if ! diff -q gcc8_version.h  new_gcc8_version.h > /dev/null; then mv new_gcc8_version.h gcc8_version.h; fi fi

###### 


if BUILD_ARM_GCC
all: $(ARM_GCC_EXE)
endif

if BUILD_ARM_EMPTY_PLUGIN
all: $(ARM_EMPTY_PLUGIN)
endif

if BUILD_ARM_SSA_PLUGIN
all: $(ARM_SSA_PLUGIN)
endif

if BUILD_ARM_SSA_PLUGINCPP
all: $(ARM_SSA_PLUGINCPP)
endif

if BUILD_ARM_RTL_PLUGIN
all: $(ARM_RTL_PLUGIN)
endif

$(ARM_SSA_PLUGIN) : ARM_plugin_dumpGimpleSSA.o ARM_dumpGimple.o
	$(ARM_HOST_PLUGIN_COMPILER) -shared ARM_plugin_dumpGimpleSSA.o ARM_dumpGimple.o -o $(ARM_SSA_PLUGIN) -fPIC

$(ARM_SSA_PLUGINCPP) : ARM_plugin_dumpGimpleSSA.o ARM_dumpGimpleCpp.o
	$(ARM_HOST_PLUGIN_COMPILER) -shared ARM_plugin_dumpGimpleSSA.o ARM_dumpGimpleCpp.o -o $(ARM_SSA_PLUGINCPP) -fPIC

$(ARM_EMPTY_PLUGIN) : ARM_plugin_dumpGimpleEmpty.o ARM_dumpGimple.o
	$(ARM_HOST_PLUGIN_COMPILER) -shared ARM_plugin_dumpGimpleEmpty.o ARM_dumpGimple.o -o $(ARM_EMPTY_PLUGIN) -fPIC

$(ARM_RTL_PLUGIN) : ARM_plugin_dumpRTL.o ARM_dumpGimple.o
	$(ARM_HOST_PLUGIN_COMPILER) -shared ARM_plugin_dumpRTL.o ARM_dumpGimple.o -o $(ARM_RTL_PLUGIN) -fPIC

ARM_plugin_dumpGimpleEmpty.o : $(top_srcdir)/etc/gcc_plugin/plugin_dumpGimpleEmpty.c
	$(ARM_HOST_PLUGIN_COMPILER) -c $(top_srcdir)/etc/gcc_plugin/plugin_dumpGimpleEmpty.c -o ARM_plugin_dumpGimpleEmpty.o -I$(ARM_GCC_PLUGIN_DIR)/include -fPIC -I$(ARM_GCC_SRC_DIR)/gcc -I$(ARM_GCC_SRC_DIR)/gcc/config/arm -DARM -DPANDA_PLUGIN_VERSION=\"$(PANDA_PLUGIN_VERSION)\"

ARM_plugin_dumpGimpleSSA.o : $(top_srcdir)/etc/gcc_plugin/plugin_dumpGimpleSSA.c
	$(ARM_HOST_PLUGIN_COMPILER) -c $(top_srcdir)/etc/gcc_plugin/plugin_dumpGimpleSSA.c -o ARM_plugin_dumpGimpleSSA.o -I$(ARM_GCC_PLUGIN_DIR)/include -fPIC -I$(ARM_GCC_SRC_DIR)/gcc -I$(ARM_GCC_SRC_DIR)/gcc/config/arm -DARM -DPANDA_PLUGIN_VERSION=\"$(PANDA_PLUGIN_VERSION)\"

ARM_plugin_dumpRTL.o : $(top_srcdir)/etc/gcc_plugin/plugin_dumpRTL.c  $(top_srcdir)/etc/gcc_plugin/plugin_includes.h $(top_srcdir)/etc/gcc_plugin/cfgexpand_4_5_2.c
	$(ARM_HOST_PLUGIN_COMPILER) -c $(top_srcdir)/etc/gcc_plugin/plugin_dumpRTL.c -o ARM_plugin_dumpRTL.o -I$(ARM_GCC_PLUGIN_DIR)/include -fPIC -I$(ARM_GCC_SRC_DIR)/gcc -I$(ARM_GCC_SRC_DIR)/gcc/config/arm -I$(top_srcdir)/etc/gcc_plugin -I$(ARM_GCC_BUILT_DIR)/gcc  -DARM -DARM_GCC_$(ARM_GCC_VERSION) -DARM -fPIC -DPANDA_PLUGIN_VERSION=\"$(PANDA_PLUGIN_VERSION)\"

ARM_dumpGimple.o : $(top_srcdir)/etc/gcc_plugin/dumpGimple.c
	$(ARM_HOST_PLUGIN_COMPILER) -c $(top_srcdir)/etc/gcc_plugin/dumpGimple.c -o ARM_dumpGimple.o  -I$(ARM_GCC_PLUGIN_DIR)/include -fPIC -I$(ARM_GCC_SRC_DIR)/gcc -I$(ARM_GCC_SRC_DIR)/gcc/config/arm -DPANDA_PLUGIN_VERSION=\"$(PANDA_PLUGIN_VERSION)\" -DARM

ARM_dumpGimpleCpp.o : $(top_srcdir)/etc/gcc_plugin/dumpGimple.c
	$(ARM_HOST_PLUGIN_COMPILER) -c $(top_srcdir)/etc/gcc_plugin/dumpGimple.c -DCPP_LANGUAGE -o ARM_dumpGimpleCpp.o  -I$(ARM_GCC_PLUGIN_DIR)/include -fPIC -I$(ARM_GCC_SRC_DIR)/gcc -I$(ARM_GCC_SRC_DIR)/gcc/config/arm -DPANDA_PLUGIN_VERSION=\"$(PANDA_PLUGIN_VERSION)\" -DARM

/opt/x-tools/arm/bin/armeb-unknown-linux-gnueabi-gcc :
	$(top_srcdir)/etc/gcc_plugin/build_arm.sh

if BUILD_SPARC_GCC
all: $(SPARC_GCC_EXE)
endif

if BUILD_SPARC_EMPTY_PLUGIN
all: $(SPARC_EMPTY_PLUGIN)
endif

if BUILD_SPARC_SSA_PLUGIN
all: $(SPARC_SSA_PLUGIN)
endif

if BUILD_SPARC_SSA_PLUGINCPP
all: $(SPARC_SSA_PLUGINCPP)
endif

if BUILD_SPARC_RTL_PLUGIN
all: $(SPARC_RTL_PLUGIN)
endif

$(SPARC_SSA_PLUGIN) : SPARC_plugin_dumpGimpleSSA.o SPARC_dumpGimple.o $(SPARC_GCC_EXE)
	$(SPARC_HOST_PLUGIN_COMPILER) -shared SPARC_plugin_dumpGimpleSSA.o SPARC_dumpGimple.o -o $(SPARC_SSA_PLUGIN) -fPIC

$(SPARC_SSA_PLUGINCPP) : SPARC_plugin_dumpGimpleSSA.o SPARC_dumpGimpleCpp.o $(SPARC_GCC_EXE)
	$(SPARC_HOST_PLUGIN_COMPILER) -shared SPARC_plugin_dumpGimpleSSA.o SPARC_dumpGimpleCpp.o -o $(SPARC_SSA_PLUGINCPP) -fPIC

$(SPARC_EMPTY_PLUGIN) : SPARC_plugin_dumpGimpleEmpty.o SPARC_dumpGimple.o $(SPARC_GCC_EXE)
	$(SPARC_HOST_PLUGIN_COMPILER) -shared SPARC_plugin_dumpGimpleEmpty.o SPARC_dumpGimple.o -o $(SPARC_EMPTY_PLUGIN) -fPIC

$(SPARC_RTL_PLUGIN) : SPARC_plugin_dumpRTL.o SPARC_dumpGimple.o $(SPARC_GCC_EXE)
	$(SPARC_HOST_PLUGIN_COMPILER) -shared SPARC_plugin_dumpRTL.o SPARC_dumpGimple.o -o $(SPARC_RTL_PLUGIN) -fPIC

SPARC_plugin_dumpGimpleEmpty.o : $(top_srcdir)/etc/gcc_plugin/plugin_dumpGimpleEmpty.c $(SPARC_GCC_EXE)
	$(SPARC_HOST_PLUGIN_COMPILER) -c $(top_srcdir)/etc/gcc_plugin/plugin_dumpGimpleEmpty.c -o SPARC_plugin_dumpGimpleEmpty.o -I$(SPARC_GCC_PLUGIN_DIR)/include -fPIC -I$(SPARC_GCC_SRC_DIR)/gcc -I$(SPARC_GCC_SRC_DIR)/gcc/config/sparc -DSPARC -DPANDA_PLUGIN_VERSION=\"$(PANDA_PLUGIN_VERSION)\"

SPARC_plugin_dumpGimpleSSA.o : $(top_srcdir)/etc/gcc_plugin/plugin_dumpGimpleSSA.c $(SPARC_GCC_EXE)
	$(SPARC_HOST_PLUGIN_COMPILER) -c $(top_srcdir)/etc/gcc_plugin/plugin_dumpGimpleSSA.c -o SPARC_plugin_dumpGimpleSSA.o -I$(SPARC_GCC_PLUGIN_DIR)/include -fPIC -I$(SPARC_GCC_SRC_DIR)/gcc -I$(SPARC_GCC_SRC_DIR)/gcc/config/sparc -DSPARC -DPANDA_PLUGIN_VERSION=\"$(PANDA_PLUGIN_VERSION)\"

SPARC_plugin_dumpRTL.o : $(top_srcdir)/etc/gcc_plugin/plugin_dumpRTL.c $(top_srcdir)/etc/gcc_plugin/cfgexpand_4_5_2.c  $(top_srcdir)/etc/gcc_plugin/plugin_includes.h $(SPARC_GCC_EXE)
	$(SPARC_HOST_PLUGIN_COMPILER) -c $(top_srcdir)/etc/gcc_plugin/plugin_dumpRTL.c -o SPARC_plugin_dumpRTL.o -I$(SPARC_GCC_PLUGIN_DIR)/include -fPIC -I$(SPARC_GCC_SRC_DIR)/gcc -I$(SPARC_GCC_SRC_DIR)/gcc/config/sparc -I$(top_srcdir)/etc/gcc_plugin -I$(SPARC_GCC_BUILT_DIR)/gcc  -DSPARC -DSPARC_GCC_$(SPARC_GCC_VERSION) -fPIC -DPANDA_PLUGIN_VERSION=\"$(PANDA_PLUGIN_VERSION)\"

SPARC_dumpGimple.o : $(top_srcdir)/etc/gcc_plugin/dumpGimple.c $(SPARC_GCC_EXE)
	$(SPARC_HOST_PLUGIN_COMPILER) -c $(top_srcdir)/etc/gcc_plugin/dumpGimple.c -o SPARC_dumpGimple.o  -I$(SPARC_GCC_PLUGIN_DIR)/include -fPIC -I$(SPARC_GCC_SRC_DIR)/gcc -I$(SPARC_GCC_SRC_DIR)/gcc/config/sparc -DPANDA_PLUGIN_VERSION=\"$(PANDA_PLUGIN_VERSION)\" -DSPARC

SPARC_dumpGimpleCpp.o : $(top_srcdir)/etc/gcc_plugin/dumpGimple.c $(SPARC_GCC_EXE)
	$(SPARC_HOST_PLUGIN_COMPILER) -c $(top_srcdir)/etc/gcc_plugin/dumpGimple.c -DCPP_LANGUAGE -o SPARC_dumpGimpleCpp.o  -I$(SPARC_GCC_PLUGIN_DIR)/include -fPIC -I$(SPARC_GCC_SRC_DIR)/gcc -I$(SPARC_GCC_SRC_DIR)/gcc/config/sparc -DPANDA_PLUGIN_VERSION=\"$(PANDA_PLUGIN_VERSION)\" -DSPARC

/opt/x-tools/sparc/bin/sparc-unknown-linux-gnu-gcc :
	$(top_srcdir)/etc/gcc_plugin/build_sparc.sh

clean :
	rm -rf *.o *.so *.h

install: all
	if test "`ls *so`" != ""; then mkdir -p $(DESTDIR)$(GCC_PLUGIN_DIR); cp -v *.so $(DESTDIR)$(GCC_PLUGIN_DIR); fi

#do not touch the following line


