EXTRA_DIST = \
	$(top_srcdir)/etc/gcc_plugin/VRP_data.h \
	$(top_srcdir)/etc/gcc_plugin/dumpGimple.c \
	$(top_srcdir)/etc/gcc_plugin/gcc-4.9/dbgcnt.def \
	$(top_srcdir)/etc/gcc_plugin/gcc-4.9/dbgcnt.h \
	$(top_srcdir)/etc/gcc_plugin/gcc-4.9/expr.h \
	$(top_srcdir)/etc/gcc_plugin/gcc-4.9/insn-config.h \
	$(top_srcdir)/etc/gcc_plugin/gcc-4.9/ipa-inline.h \
	$(top_srcdir)/etc/gcc_plugin/gcc-4.9/tree-sra.c \
	$(top_srcdir)/etc/gcc_plugin/gcc-5/insn-config.h \
	$(top_srcdir)/etc/gcc_plugin/gcc-5/tree-sra.c \
	$(top_srcdir)/etc/gcc_plugin/gcc-6/tree-sra.c \
	$(top_srcdir)/etc/gcc_plugin/gcc-7/tree-sra.c \
	$(top_srcdir)/etc/gcc_plugin/gcc-8/tree-sra.c \
	$(top_srcdir)/etc/gcc_plugin/ggc_clike_extern.h \
	$(top_srcdir)/etc/gcc_plugin/ggc_clike_static.h \
	$(top_srcdir)/etc/gcc_plugin/ggc_cpplike_extern.h \
	$(top_srcdir)/etc/gcc_plugin/ggc_cpplike_static.h \
	$(top_srcdir)/etc/gcc_plugin/plugin_dumpGimpleEmpty.c \
	$(top_srcdir)/etc/gcc_plugin/plugin_dumpGimpleSSA.c \
	$(top_srcdir)/etc/gcc_plugin/plugin_includes.h \
	$(top_srcdir)/etc/gcc_plugin/plugin_topfname.c

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

clean :
	rm -rf *.o *.so *.h

install: all
	if test "`ls *so`" != ""; then mkdir -p $(DESTDIR)$(GCC_PLUGIN_DIR); cp -v *.so $(DESTDIR)$(GCC_PLUGIN_DIR); fi

#do not touch the following line


