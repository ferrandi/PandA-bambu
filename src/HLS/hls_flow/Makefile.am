#this file is src/HLS/hls_flow/Makefile.am
SUBDIRS = synthesis
DIST_SUBDIRS = synthesis

noinst_LTLIBRARIES =  lib_hls_flow.la

lib_hls_flow_la_CPPFLAGS = \
   -I$(top_srcdir)/src \
   -I$(top_srcdir)/src/algorithms/clique_covering \
   -I$(top_srcdir)/src/algorithms/bipartite_matching\
   -I$(top_srcdir)/src/behavior \
   -I$(top_srcdir)/src/circuit \
   -I$(top_srcdir)/src/constants \
   -I$(top_srcdir)/src/design_flows \
   -I$(top_srcdir)/src/design_flows/backend/ToHDL \
   -I$(top_srcdir)/src/design_flows/backend/ToC\
   -I$(top_srcdir)/src/design_flows/backend/ToC/progModels \
   -I$(top_srcdir)/src/design_flows/backend/ToC/source_code_writers \
   -I$(top_srcdir)/src/frontend_analysis \
   -I$(top_srcdir)/src/graph \
   -I$(top_srcdir)/src/HLS \
   -I$(top_srcdir)/src/HLS/module_allocation \
   -I$(top_srcdir)/src/HLS/architecture_creation \
   -I$(top_srcdir)/src/HLS/architecture_creation/controller_creation \
   -I$(top_srcdir)/src/HLS/architecture_creation/datapath_creation \
   -I$(top_srcdir)/src/HLS/backend\
   -I$(top_srcdir)/src/HLS/binding \
   -I$(top_srcdir)/src/HLS/binding/interconnection \
   -I$(top_srcdir)/src/HLS/binding/module \
   -I$(top_srcdir)/src/HLS/binding/register \
   -I$(top_srcdir)/src/HLS/binding/register/algorithms \
   -I$(top_srcdir)/src/HLS/binding/storage_value_insertion \
   -I$(top_srcdir)/src/HLS/chaining \
   -I$(top_srcdir)/src/HLS/function_allocation \
   -I$(top_srcdir)/src/HLS/hls_flow/synthesis \
   -I$(top_srcdir)/src/HLS/evaluation \
   -I$(top_srcdir)/src/HLS/interface \
   -I$(top_srcdir)/src/HLS/liveness \
   -I$(top_srcdir)/src/HLS/memory \
   -I$(top_srcdir)/src/HLS/scheduling \
   -I$(top_srcdir)/src/HLS/simulation \
   -I$(top_srcdir)/src/HLS/stg \
   -I$(top_srcdir)/src/HLS/vcd \
   -I$(top_srcdir)/src/intermediate_representations \
   -I$(top_srcdir)/src/intermediate_representations/hls \
   -I$(top_srcdir)/src/parser/polixml \
   -I$(top_srcdir)/src/parser/vcd \
   -I$(top_srcdir)/src/polixml \
   -I$(top_srcdir)/src/technology \
   -I$(top_srcdir)/src/technology/physical_library \
   -I$(top_srcdir)/src/technology/target_device \
   -I$(top_srcdir)/src/technology/target_technology \
   -I$(top_srcdir)/src/technology/physical_library/models \
   -I$(top_srcdir)/src/tree \
   -I$(top_srcdir)/src/utility \
   -I$(top_srcdir)/src/wrapper/synthesis \
   -I$(top_srcdir)/src/wrapper/synthesis/xilinx/ise \
   -I$(BOOST_DIR) $(all_includes)

METASOURCES = AUTO

noinst_HEADERS = \
   initialize_hls.hpp \
   classical_synthesis_flow.hpp \
   write_hls_summary.hpp

lib_hls_flow_la_SOURCES = \
   initialize_hls.cpp \
   classical_synthesis_flow.cpp \
   write_hls_summary.cpp

lib_hls_flow_la_LIBADD = \
   $(top_builddir)/src/HLS/hls_flow/synthesis/lib_hls_synthesis.la

if BUILD_LIB_EXPERIMENTAL
  lib_hls_flow_la_CPPFLAGS += \
    -I$(top_srcdir)/src/HLS/export \
    -I$(top_srcdir)/src/HLS/evaluation
  noinst_HEADERS += dump_hls.hpp xml_config_hls.hpp
  lib_hls_flow_la_SOURCES += dump_hls.cpp xml_config_hls.cpp
endif

if BUILD_LIB_VCD
   lib_hls_flow_la_CPPFLAGS += \
      -I$(top_srcdir)/src/HLS/vcd \
      -I$(top_srcdir)/src/HLS/virtual_components \
      -I$(top_srcdir)/src/parser/vcd
endif

if BUILD_DSE
  SUBDIRS += design_space_exploration
  DIST_SUBDIRS += design_space_exploration
  lib_hls_flow_la_CPPFLAGS += \
                           -I$(top_srcdir)/src/HLS/hls_flow/design_space_exploration \
                           -I$(BEAGLE_DIR)/beagle/include -I$(top_builddir)/ext/beagle/beagle/include -I$(BEAGLE_DIR)/PACC
  lib_hls_flow_la_LIBADD += $(top_builddir)/src/HLS/hls_flow/design_space_exploration/lib_hls_dse.la
endif

if BUILD_TRNG
   lib_hls_flow_la_CPPFLAGS += \
      -I$(top_builddir)/ext/trng-4.17
endif

#do not touch the following line

