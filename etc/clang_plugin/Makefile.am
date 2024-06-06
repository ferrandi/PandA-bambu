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
	$(top_srcdir)/etc/clang_plugin/plugin_dumpGimpleSSACpp.cpp \
	$(top_srcdir)/etc/clang_plugin/plugin_dumpGimpleEmpty.cpp \
	$(top_srcdir)/etc/clang_plugin/plugin_expandMemOps.cpp \
	$(top_srcdir)/etc/clang_plugin/plugin_GepiCanon.cpp \
	$(top_srcdir)/etc/clang_plugin/plugin_includes.hpp \
	$(top_srcdir)/etc/clang_plugin/plugin_topfname.cpp \
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
PANDA_PLUGIN_VERSION=1.00
COMPILER_PLUGINS_BUILD_DIR=../$(notdir $(COMPILER_PLUGINS_DIR))

CLANG_LIBS=-lclangAST -lclangASTMatchers -lclangAnalysis -lclangBasic -lclangDriver -lclangEdit -lclangFrontend -lclangFrontendTool -lclangLex -lclangParse -lclangSema -lclangEdit -lclangRewrite -lclangRewriteFrontend -lclangStaticAnalyzerFrontend -lclangStaticAnalyzerCheckers -lclangStaticAnalyzerCore -lclangIndex -lclangSerialization -lclangToolingCore -lclangTooling -lclangFormat
CLANG_PLUGIN_CXXFLAGS_EXTRA=$(all_includes) -isystem $(top_srcdir)/ext/pugixml/src -DPANDA_PLUGIN_VERSION=\"$(PANDA_PLUGIN_VERSION)\"

install-exec-local: all-local
	@mkdir -p $(DESTDIR)$(COMPILER_PLUGINS_DIR)
	@cp -r $(COMPILER_PLUGINS_BUILD_DIR)/* $(DESTDIR)$(COMPILER_PLUGINS_DIR)

### SET_PLUGIN_RULES(macro_prefix)
define SET_PLUGIN_RULES
LOCAL_PLUGIN_BUILD_DIR=$(COMPILER_PLUGINS_BUILD_DIR)/$$($(1)_PLUGIN_DIR)

.PHONY: all-local_$(1) clean-local_$(1)

all-local: all-local_$(1)

all-local_$(1): \
	$(COMPILER_PLUGINS_BUILD_DIR)/$$($(1)_PLUGIN_DIR)/$(COMPILER_EMPTY_PLUGIN).so \
	$(COMPILER_PLUGINS_BUILD_DIR)/$$($(1)_PLUGIN_DIR)/opt_$(COMPILER_EMPTY_PLUGIN).so \
	$(COMPILER_PLUGINS_BUILD_DIR)/$$($(1)_PLUGIN_DIR)/$(COMPILER_SSA_PLUGIN).so \
	$(COMPILER_PLUGINS_BUILD_DIR)/$$($(1)_PLUGIN_DIR)/opt_$(COMPILER_SSA_PLUGIN).so \
	$(COMPILER_PLUGINS_BUILD_DIR)/$$($(1)_PLUGIN_DIR)/$(COMPILER_SSA_PLUGINCPP).so \
	$(COMPILER_PLUGINS_BUILD_DIR)/$$($(1)_PLUGIN_DIR)/opt_$(COMPILER_SSA_PLUGINCPP).so \
	$(COMPILER_PLUGINS_BUILD_DIR)/$$($(1)_PLUGIN_DIR)/$(COMPILER_TOPFNAME_PLUGIN).so \
	$(COMPILER_PLUGINS_BUILD_DIR)/$$($(1)_PLUGIN_DIR)/opt_$(COMPILER_TOPFNAME_PLUGIN).so \
	$(COMPILER_PLUGINS_BUILD_DIR)/$$($(1)_PLUGIN_DIR)/$(COMPILER_EXPANDMEMOPS_PLUGIN).so \
	$(COMPILER_PLUGINS_BUILD_DIR)/$$($(1)_PLUGIN_DIR)/opt_$(COMPILER_EXPANDMEMOPS_PLUGIN).so \
	$(COMPILER_PLUGINS_BUILD_DIR)/$$($(1)_PLUGIN_DIR)/$(COMPILER_GEPICANON_PLUGIN).so \
	$(COMPILER_PLUGINS_BUILD_DIR)/$$($(1)_PLUGIN_DIR)/opt_$(COMPILER_GEPICANON_PLUGIN).so \
	$(COMPILER_PLUGINS_BUILD_DIR)/$$($(1)_PLUGIN_DIR)/$(COMPILER_CSROA_PLUGIN).so \
	$(COMPILER_PLUGINS_BUILD_DIR)/$$($(1)_PLUGIN_DIR)/opt_$(COMPILER_CSROA_PLUGIN).so \
	$(COMPILER_PLUGINS_BUILD_DIR)/$$($(1)_PLUGIN_DIR)/$(COMPILER_ASTANALYZER_PLUGIN).so \
	$(COMPILER_PLUGINS_BUILD_DIR)/$$($(1)_PLUGIN_DIR)/opt_$(COMPILER_ASTANALYZER_PLUGIN).so

clean-local: clean-local_$(1)

clean-local_$(1):
	@rm -rf $$($(1)_PLUGIN_DIR)

$(COMPILER_PLUGINS_BUILD_DIR)/$$($(1)_PLUGIN_DIR)/$(COMPILER_EMPTY_PLUGIN).so: \
	$$($(1)_PLUGIN_DIR)/plugin_dumpGimpleEmpty.o $$($(1)_PLUGIN_DIR)/dumpGimple.o
$(COMPILER_PLUGINS_BUILD_DIR)/$$($(1)_PLUGIN_DIR)/$(COMPILER_SSA_PLUGIN).so: \
	$$($(1)_PLUGIN_DIR)/plugin_dumpGimpleSSA.o $$($(1)_PLUGIN_DIR)/dumpGimple.o
$(COMPILER_PLUGINS_BUILD_DIR)/$$($(1)_PLUGIN_DIR)/$(COMPILER_SSA_PLUGINCPP).so: \
	$$($(1)_PLUGIN_DIR)/plugin_dumpGimpleSSACpp.o $$($(1)_PLUGIN_DIR)/dumpGimple.o
$(COMPILER_PLUGINS_BUILD_DIR)/$$($(1)_PLUGIN_DIR)/$(COMPILER_TOPFNAME_PLUGIN).so: \
	$$($(1)_PLUGIN_DIR)/plugin_topfname.o
$(COMPILER_PLUGINS_BUILD_DIR)/$$($(1)_PLUGIN_DIR)/$(COMPILER_EXPANDMEMOPS_PLUGIN).so: \
	$$($(1)_PLUGIN_DIR)/plugin_expandMemOps.o
$(COMPILER_PLUGINS_BUILD_DIR)/$$($(1)_PLUGIN_DIR)/$(COMPILER_GEPICANON_PLUGIN).so: \
	$$($(1)_PLUGIN_DIR)/plugin_GepiCanon.o $$($(1)_PLUGIN_DIR)/GepiCanonicalizationPass.o
$(COMPILER_PLUGINS_BUILD_DIR)/$$($(1)_PLUGIN_DIR)/$(COMPILER_CSROA_PLUGIN).so: \
	$$($(1)_PLUGIN_DIR)/plugin_CSROA.o $$($(1)_PLUGIN_DIR)/CustomScalarReplacementOfAggregatesPass.o $$($(1)_PLUGIN_DIR)/GepiCanonicalizationPass.o
$(COMPILER_PLUGINS_BUILD_DIR)/$$($(1)_PLUGIN_DIR)/$(COMPILER_ASTANALYZER_PLUGIN).so: \
	$$($(1)_PLUGIN_DIR)/plugin_ASTAnalyzer.o
if BUILD_PTAA
$(COMPILER_PLUGINS_BUILD_DIR)/$$($(1)_PLUGIN_DIR)/$(COMPILER_EMPTY_PLUGIN).so $(COMPILER_PLUGINS_BUILD_DIR)/$$($(1)_PLUGIN_DIR)/$(COMPILER_SSA_PLUGIN).so $(COMPILER_PLUGINS_BUILD_DIR)/$$($(1)_PLUGIN_DIR)/$(COMPILER_SSA_PLUGINCPP).so: $$($(1)_PLUGIN_DIR)/HardekopfLin_AA.o
endif

$(COMPILER_PLUGINS_BUILD_DIR)/$$($(1)_PLUGIN_DIR)/opt_$(COMPILER_EMPTY_PLUGIN).so: \
	$$($(1)_PLUGIN_DIR)/opt_plugin_dumpGimpleEmpty.o $$($(1)_PLUGIN_DIR)/dumpGimple.o
$(COMPILER_PLUGINS_BUILD_DIR)/$$($(1)_PLUGIN_DIR)/opt_$(COMPILER_SSA_PLUGIN).so: \
	$$($(1)_PLUGIN_DIR)/opt_plugin_dumpGimpleSSA.o $$($(1)_PLUGIN_DIR)/dumpGimple.o
$(COMPILER_PLUGINS_BUILD_DIR)/$$($(1)_PLUGIN_DIR)/opt_$(COMPILER_SSA_PLUGINCPP).so: \
	$$($(1)_PLUGIN_DIR)/opt_plugin_dumpGimpleSSACpp.o $$($(1)_PLUGIN_DIR)/dumpGimple.o
$(COMPILER_PLUGINS_BUILD_DIR)/$$($(1)_PLUGIN_DIR)/opt_$(COMPILER_TOPFNAME_PLUGIN).so: \
	$$($(1)_PLUGIN_DIR)/opt_plugin_topfname.o
$(COMPILER_PLUGINS_BUILD_DIR)/$$($(1)_PLUGIN_DIR)/opt_$(COMPILER_EXPANDMEMOPS_PLUGIN).so: \
	$$($(1)_PLUGIN_DIR)/opt_plugin_expandMemOps.o
$(COMPILER_PLUGINS_BUILD_DIR)/$$($(1)_PLUGIN_DIR)/opt_$(COMPILER_GEPICANON_PLUGIN).so: \
	$$($(1)_PLUGIN_DIR)/opt_plugin_GepiCanon.o $$($(1)_PLUGIN_DIR)/GepiCanonicalizationPass.o
$(COMPILER_PLUGINS_BUILD_DIR)/$$($(1)_PLUGIN_DIR)/opt_$(COMPILER_CSROA_PLUGIN).so: \
	$$($(1)_PLUGIN_DIR)/opt_plugin_CSROA.o $$($(1)_PLUGIN_DIR)/CustomScalarReplacementOfAggregatesPass.o $$($(1)_PLUGIN_DIR)/GepiCanonicalizationPass.o
$(COMPILER_PLUGINS_BUILD_DIR)/$$($(1)_PLUGIN_DIR)/opt_$(COMPILER_ASTANALYZER_PLUGIN).so: \
	$$($(1)_PLUGIN_DIR)/opt_plugin_ASTAnalyzer.o
if BUILD_PTAA
$(COMPILER_PLUGINS_BUILD_DIR)/$$($(1)_PLUGIN_DIR)/opt_$(COMPILER_EMPTY_PLUGIN).so $(COMPILER_PLUGINS_BUILD_DIR)/$$($(1)_PLUGIN_DIR)/opt_$(COMPILER_SSA_PLUGIN).so $(COMPILER_PLUGINS_BUILD_DIR)/$$($(1)_PLUGIN_DIR)/opt_$(COMPILER_SSA_PLUGINCPP).so: $$($(1)_PLUGIN_DIR)/HardekopfLin_AA.o
endif

$(COMPILER_PLUGINS_BUILD_DIR)/$$($(1)_PLUGIN_DIR)/%.so:
	@mkdir -p $(COMPILER_PLUGINS_BUILD_DIR)/$$($(1)_PLUGIN_DIR)
	@echo "  CXXLD $$@"
	@case $$(host_os) in \
	  mingw*) \
	  	plugin_ldflags='-Wl,--start-group $$(CLANG_LIBS) -lclangCrossTU -lclangToolingInclusions -Wl,--end-group'; \
	    plugin_ldflags+=' -lversion `$$($(subst CLANG,LLVM_CONFIG,$1)_EXE) --ldflags --libs --system-libs` -lbdd';; \
	  darwin*) \
	    plugin_ldflags='-fPIC -shared -undefined dynamic_lookup -fno-rtti -lbdd';; \
	  *) \
	    plugin_ldflags='-fPIC -shared -fno-rtti -Wl,-znodelete -lbdd';; \
	esac; \
	$$($(1)_PLUGIN_COMPILER) $$$$plugin_ldflags -o $$@ $$^

$$($(1)_PLUGIN_DIR)/%.o: $$(top_srcdir)/etc/clang_plugin/%.cpp $(top_srcdir)/etc/clang_plugin/plugin_includes.hpp
	@mkdir -p $$($(1)_PLUGIN_DIR)
	@echo "  CXX $$@"
	@$$($(1)_PLUGIN_COMPILER) -fPIC $$($(1)_PLUGIN_CXXFLAGS) $$(WFLAGS) -DADD_RSP=1 $$(CLANG_PLUGIN_CXXFLAGS_EXTRA) -c -o $$@ $$<

$$($(1)_PLUGIN_DIR)/opt_%.o: $$(top_srcdir)/etc/clang_plugin/%.cpp $$(top_srcdir)/etc/clang_plugin/plugin_includes.hpp
	@mkdir -p $$($(1)_PLUGIN_DIR)
	@echo "  CXX $$@"
	@$$($(1)_PLUGIN_COMPILER) -fPIC $$($(1)_PLUGIN_CXXFLAGS) $$(WFLAGS) -DADD_RSP=0 $$(CLANG_PLUGIN_CXXFLAGS_EXTRA) -c -o $$@ $$<
endef

if BUILD_I386_CLANG4
$(eval $(call SET_PLUGIN_RULES,I386_CLANG4))
endif
if BUILD_I386_CLANG5
$(eval $(call SET_PLUGIN_RULES,I386_CLANG5))
endif
if BUILD_I386_CLANG6
$(eval $(call SET_PLUGIN_RULES,I386_CLANG6))
endif
if BUILD_I386_CLANG7
$(eval $(call SET_PLUGIN_RULES,I386_CLANG7))
endif
if BUILD_I386_CLANG8
$(eval $(call SET_PLUGIN_RULES,I386_CLANG8))
endif
if BUILD_I386_CLANG9
$(eval $(call SET_PLUGIN_RULES,I386_CLANG9))
endif
if BUILD_I386_CLANG10
$(eval $(call SET_PLUGIN_RULES,I386_CLANG10))
endif
if BUILD_I386_CLANG11
$(eval $(call SET_PLUGIN_RULES,I386_CLANG11))
endif
if BUILD_I386_CLANG12
$(eval $(call SET_PLUGIN_RULES,I386_CLANG12))
endif
if BUILD_I386_CLANG13
$(eval $(call SET_PLUGIN_RULES,I386_CLANG13))
endif
if BUILD_I386_CLANG16
$(eval $(call SET_PLUGIN_RULES,I386_CLANG16))
endif
if BUILD_I386_CLANGVVD
$(eval $(call SET_PLUGIN_RULES,I386_CLANGVVD))
endif
