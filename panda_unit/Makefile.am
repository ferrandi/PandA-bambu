
../src/Parameter.cpp : branch_name.hpp revision_hash.hpp

if INSIDE_GIT
branch_name.hpp : $(GIT_WORKTREE_PATH)/HEAD $(GIT_WORKTREE_PATH)/index
	@echo "\"`git --git-dir $(GIT_WORKTREE_PATH) rev-parse --abbrev-ref HEAD`\"" >$@

revision_hash.hpp : $(GIT_WORKTREE_PATH)/HEAD $(GIT_WORKTREE_PATH)/index
	@echo "\"`git --git-dir $(GIT_WORKTREE_PATH) rev-parse HEAD`\"" > $@

else

branch_name.hpp :
	echo "" > $@

revision_hash.hpp :
	cat $(top_srcdir)/dist_revision_hash > $@

endif

check_PROGRAMS = program_tests
program_tests_CPPFLAGS = $(all_includes) -I$(BOOST_DIR) -I$(ABSEIL_DIR) \
     ${BOOST_CPPFLAGS} -DBOOST_TEST_DYN_LINK \
     -DPKGLIBDIR="\"$(pkglibdir)\"" -DPKGDATADIR="\"$(pkgdatadir)\""\
     -I$(top_srcdir)/src \
     -I$(top_srcdir)/src/algorithms/bipartite_matching \
     -I$(top_srcdir)/src/algorithms/clique_covering \
     -I$(top_srcdir)/src/design_flows/backend/ToHDL \
     -I$(top_srcdir)/src/design_flows/backend/ToC \
     -I$(top_srcdir)/src/design_flows/backend/ToC/progModels \
     -I$(top_srcdir)/src/design_flows/backend/ToC/source_code_writers \
     -I$(top_srcdir)/src/behavior \
     -I$(top_srcdir)/src/constants \
     -I$(top_srcdir)/src/circuit\
     -I$(top_srcdir)/src/design_flows\
     -I$(top_srcdir)/src/design_flows/technology \
     -I$(top_srcdir)/src/frontend_analysis \
     -I$(top_srcdir)/src/frontend_analysis/IR_analysis \
     -I$(top_srcdir)/src/graph \
     -I$(top_srcdir)/src/tree \
     -I$(top_srcdir)/src/HLS \
     -I$(top_srcdir)/src/HLS/architecture_creation \
     -I$(top_srcdir)/src/HLS/architecture_creation/controller_creation \
     -I$(top_srcdir)/src/HLS/architecture_creation/datapath_creation \
     -I$(top_srcdir)/src/HLS/binding \
     -I$(top_srcdir)/src/HLS/binding/interconnection \
     -I$(top_srcdir)/src/HLS/binding/module \
     -I$(top_srcdir)/src/HLS/binding/register \
     -I$(top_srcdir)/src/HLS/binding/storage_value_insertion \
     -I$(top_srcdir)/src/HLS/chaining \
     -I$(top_srcdir)/src/HLS/evaluation \
     -I$(top_srcdir)/src/HLS/function_allocation \
     -I$(top_srcdir)/src/HLS/hls_flow \
     -I$(top_srcdir)/src/HLS/interface \
     -I$(top_srcdir)/src/HLS/liveness \
     -I$(top_srcdir)/src/HLS/memory \
     -I$(top_srcdir)/src/HLS/scheduling \
     -I$(top_srcdir)/src/HLS/stg \
     -I$(top_srcdir)/src/parser \
     -I$(top_srcdir)/src/technology \
     -I$(top_srcdir)/src/technology/physical_library \
     -I$(top_srcdir)/src/technology/physical_library/models \
     -I$(top_srcdir)/src/technology/target_technology \
     -I$(top_srcdir)/src/technology/target_device \
     -I$(top_srcdir)/src/wrapper/treegcc \
      $(AM_CPPFLAGS)

if BUILD_LIB_ILP
  program_tests_CPPFLAGS += \
        -I$(top_srcdir)/src/ilp

if BUILD_LIB_EXPERIMENTAL
  program_tests_CPPFLAGS += \
        -I$(top_srcdir)/src/parser/estimation \
        -I$(top_srcdir)/src/parser/discrepancy \
        -I$(top_srcdir)/src/design_flows/backend/ToDataFile \
        -I$(top_srcdir)/src/HLS/export \
        -I$(top_srcdir)/src/HLS/virtual_components \
        -I$(top_srcdir)/src/wrapper/synthesis/xilinx/edk
endif

if BUILD_LIB_HOST_PROFILING
  program_tests_CPPFLAGS += \
        -I$(top_srcdir)/src/frontend_analysis/behavior_analysis
endif
endif

if BUILD_BEAGLE
  program_tests_CPPFLAGS += \
        -I$(top_srcdir)/src/HLS/hls_flow/design_space_exploration \
        -I$(top_srcdir)/src/HLS/hls_flow/design_space_exploration/common \
        -I$(top_srcdir)/src/HLS/hls_flow/design_space_exploration/GA \
        -I$(top_builddir)/ext/beagle/beagle/include/ \
        -I$(BEAGLE_DIR)/beagle/include/ \
        -I$(BEAGLE_DIR)/beagle/GA/include/ \
        -I$(BEAGLE_DIR)/PACC
endif
if BUILD_LIB_CODE_ESTIMATION
   program_tests_CPPFLAGS += \
      -I$(top_srcdir)/src/design_flows/codesign/estimation \
      -I$(top_srcdir)/src/utility/probability_distribution
endif

if BUILD_LIB_PRAGMA
  program_tests_CPPFLAGS += \
     -I$(top_srcdir)/src/design_flows/codesign \
     -I$(top_srcdir)/src/pragma
endif

if BUILD_TRNG
   program_tests_CPPFLAGS += \
      -I$(top_builddir)/ext/trng-4.17
endif

if BUILD_LIB_UTILITY
   program_tests_CPPFLAGS += -I$(top_srcdir)/src/utility
endif

if BUILD_LIB_PARSER
if BUILD_LIB_FROM_C
  program_tests_CPPFLAGS += \
     -I$(top_srcdir)/src/parser/treegcc
endif
endif

if BUILD_LIB_POLIXML
   program_tests_CPPFLAGS += -I$(top_srcdir)/src/polixml \
                 -I$(top_srcdir)/src/parser/polixml
endif

program_tests_LDADD = -lboost_unit_test_framework ../src/lib_bambu.la @PTHREAD_HACK@ ../src/bambu-BambuParameter.o ../src/bambu-Parameter.o ../src/bambu-global_variables.o
program_tests_LDFLAGS = $(BOOST_LDFLAGS)
program_tests_SOURCES = \
     main_tests.cpp \
     frontend_analysis/IR_analysis/Range.cpp \
     frontend_analysis/IR_analysis/APInt.cpp

TESTS = program_tests