EXTRA_DIST = \
	$(top_srcdir)/etc/gcc_plugin/VRP_data.h \
	$(top_srcdir)/etc/gcc_plugin/dumpGimple.c \
	$(top_srcdir)/etc/gcc_plugin/dumpGimpleCpp.c \
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
	$(top_srcdir)/etc/gcc_plugin/plugin_topfname.cpp

LD_LIBRARY_PATH=
PANDA_PLUGIN_VERSION=1.00
COMPILER_PLUGINS_BUILD_DIR=../$(notdir $(COMPILER_PLUGINS_DIR))
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

install-exec-local: all-local
	@mkdir -p $(DESTDIR)$(COMPILER_PLUGINS_DIR)
	@cp -r $(COMPILER_PLUGINS_BUILD_DIR)/* $(DESTDIR)$(COMPILER_PLUGINS_DIR)

### SET_PLUGIN_RULES(macro_prefix,api_type)
define SET_PLUGIN_RULES
.PHONY: all-local_$(1) clean-local_$(1)

all-local: all-local_$(1)

all-local_$(1): \
	$(COMPILER_PLUGINS_BUILD_DIR)/$$($(1)_PLUGIN_DIR)/$(COMPILER_EMPTY_PLUGIN).so \
	$(COMPILER_PLUGINS_BUILD_DIR)/$$($(1)_PLUGIN_DIR)/$(COMPILER_SSA_PLUGIN).so \
	$(COMPILER_PLUGINS_BUILD_DIR)/$$($(1)_PLUGIN_DIR)/$(COMPILER_SSA_PLUGINCPP).so \
	$(COMPILER_PLUGINS_BUILD_DIR)/$$($(1)_PLUGIN_DIR)/$(COMPILER_TOPFNAME_PLUGIN).so

clean-local: clean-local_$(1)

clean-local_$(1):
	@rm -rf $$($(1)_PLUGIN_DIR)

$(COMPILER_PLUGINS_BUILD_DIR)/$$($(1)_PLUGIN_DIR)/$(COMPILER_EMPTY_PLUGIN).so: \
	$$($(1)_PLUGIN_DIR)/plugin_dumpGimpleEmpty.o $$($(1)_PLUGIN_DIR)/dumpGimple.o $$($(1)_PLUGIN_DIR)/tree-sra.o
$(COMPILER_PLUGINS_BUILD_DIR)/$$($(1)_PLUGIN_DIR)/$(COMPILER_SSA_PLUGIN).so: \
	$$($(1)_PLUGIN_DIR)/plugin_dumpGimpleSSA.o $$($(1)_PLUGIN_DIR)/dumpGimple.o $$($(1)_PLUGIN_DIR)/tree-sra.o
$(COMPILER_PLUGINS_BUILD_DIR)/$$($(1)_PLUGIN_DIR)/$(COMPILER_SSA_PLUGINCPP).so: \
	$$($(1)_PLUGIN_DIR)/plugin_dumpGimpleSSA.o $$($(1)_PLUGIN_DIR)/dumpGimpleCpp.o $$($(1)_PLUGIN_DIR)/tree-sra.o
$(COMPILER_PLUGINS_BUILD_DIR)/$$($(1)_PLUGIN_DIR)/$(COMPILER_TOPFNAME_PLUGIN).so: \
	$$($(1)_PLUGIN_DIR)/plugin_topfname.o

$(COMPILER_PLUGINS_BUILD_DIR)/$$($(1)_PLUGIN_DIR)/%.so:
	@mkdir -p $(COMPILER_PLUGINS_BUILD_DIR)/$$($(1)_PLUGIN_DIR)
	@echo "  CXXLD $$@"
	@case $$(host_os) in \
	  mingw*) \
	    flexlink -chain mingw -o $$@ $$^ -lstdc++;;\
	  darwin*) \
	    $$($(1)_PLUGIN_COMPILER) -fPIC -shared -undefined dynamic_lookup -fno-rtti $$@ $$^;; \
	  *) \
	    $$($(1)_PLUGIN_COMPILER) -fPIC -shared -fno-rtti -o $$@ $$^;; \
	esac;

$$($(1)_PLUGIN_DIR)/%.o: $$(top_srcdir)/etc/gcc_plugin/$$($(1)_PLUGIN_DIR)/%.c
	@mkdir -p $$($(1)_PLUGIN_DIR)
	@echo "  CXX $$@"
	@case $$(host_os) in \
	  mingw*) \
	    plugin_cxxflags="-fno-exceptions -fno-rtti -shared -O2";; \
	  *) \
	    plugin_cxxflags="-fPIC -fno-rtti -shared";; \
	esac; \
	$$($(1)_PLUGIN_COMPILER) -x c++ $$$$plugin_cxxflags -c -o $$@ $$^ -I$$($(1)_PLUGIN_DIR) -I$$($(1)_PLUGIN_INC)/include -DPANDA_PLUGIN_VERSION=\"$$(PANDA_PLUGIN_VERSION)\" -DIN_GCC

$$($(1)_PLUGIN_DIR)/%.o: $$(top_srcdir)/etc/gcc_plugin/%.* $$($(1)_PLUGIN_DIR)/gtype_roots.h 
	@mkdir -p $$($(1)_PLUGIN_DIR)
	@echo "  CXX $$@"
	@case $$(host_os) in \
	  mingw*) \
	  	plugin_cxxflags="-fno-exceptions -fno-rtti -shared -O2";; \
	  *) \
	    plugin_cxxflags="-fPIC -fno-rtti -shared";; \
	esac; \
	$$($(1)_PLUGIN_COMPILER) -x c++ $$$$plugin_cxxflags -c -o $$@ $$< -I$$($(1)_PLUGIN_DIR) -I$$($(1)_PLUGIN_INC)/include -DPANDA_PLUGIN_VERSION=\"$$(PANDA_PLUGIN_VERSION)\" $$(WFLAGS)

$$($(1)_PLUGIN_DIR)/gtype_roots.h: $$(top_srcdir)/etc/gcc_plugin/dumpGimple.c $$(top_srcdir)/etc/gcc_plugin/plugin_dumpGimpleSSA.c $$(top_srcdir)/etc/gcc_plugin/plugin_includes.h $$(top_srcdir)/etc/gcc_plugin/ggc_$(2)like_static.h $$(top_srcdir)/etc/gcc_plugin/ggc_$(2)like_extern.h
	@mkdir -p $$($(1)_PLUGIN_DIR)
	@echo "  GEN $$@"
	@cd $$($(1)_PLUGIN_DIR) && $$($(1)_GENGTYPE) -r $$($(1)_GTYPESTATE) -P $$(@F) $$(addprefix ../, $$^) $$($(1)_PLUGIN_INC)/include/splay-tree.h
endef

if BUILD_I386_GCC49
$(eval $(call SET_PLUGIN_RULES,I386_GCC49,c))
endif
if BUILD_I386_GCC5
$(eval $(call SET_PLUGIN_RULES,I386_GCC5,cpp))
endif
if BUILD_I386_GCC6
$(eval $(call SET_PLUGIN_RULES,I386_GCC6,cpp))
endif
if BUILD_I386_GCC7
$(eval $(call SET_PLUGIN_RULES,I386_GCC7,cpp))
endif
if BUILD_I386_GCC8
$(eval $(call SET_PLUGIN_RULES,I386_GCC8,cpp))
endif
