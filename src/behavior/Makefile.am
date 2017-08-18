#this file is src/behavior/Makefile.am
METASOURCES = AUTO
noinst_LTLIBRARIES = lib_behavior.la

lib_behavior_la_CPPFLAGS = \
   -I$(top_srcdir)/src \
   -I$(top_srcdir)/src/algorithms/dominance \
   -I$(top_srcdir)/src/algorithms/loops_detection \
   -I$(top_srcdir)/src/constants \
   -I$(top_srcdir)/src/graph \
   -I$(top_srcdir)/src/parser/polixml \
   -I$(top_srcdir)/src/polixml \
   -I$(top_srcdir)/src/tree \
   -I$(top_srcdir)/src/utility \
   -I$(top_srcdir)/src/wrapper/treegcc \
   -I$(BOOST_DIR) $(all_includes)

noinst_HEADERS = \
   application_manager.hpp \
   call_graph.hpp call_graph_manager.hpp function_behavior.hpp basic_block.hpp \
   behavioral_writer_helper.hpp basic_blocks_graph_constructor.hpp level_constructor.hpp operations_graph_constructor.hpp \
   op_graph.hpp cdfg_edge_info.hpp

lib_behavior_la_SOURCES = \
   application_manager.cpp \
   call_graph.cpp call_graph_manager.cpp function_behavior.cpp basic_block.cpp \
   behavioral_writer_helper.cpp basic_blocks_graph_constructor.cpp level_constructor.cpp operations_graph_constructor.cpp \
   op_graph.cpp cdfg_edge_info.cpp

if BUILD_BAMBU
   lib_behavior_la_CPPFLAGS += \
      -I$(top_srcdir)/src/HLS \
      -I$(top_srcdir)/src/HLS/module_allocation \
      -I$(top_srcdir)/src/HLS/scheduling \
      -I$(top_srcdir)/src/HLS/vcd \
      -I$(top_srcdir)/src/intermediate_representations \
      -I$(top_srcdir)/src/intermediate_representations/hls \
      -I$(top_srcdir)/src/parser/discrepancy
endif

if BUILD_LIB_CODE_ESTIMATION
   lib_behavior_la_CPPFLAGS += \
      -I$(top_srcdir)/src/design_flows/codesign/estimation
endif

if BUILD_LIB_EXPERIMENTAL
   lib_behavior_la_CPPFLAGS += -I$(top_srcdir)/src/algorithms/graph_helpers -I$(top_srcdir)/src/machine -I$(top_srcdir)/src/rtl -I$(top_srcdir)/src/wrapper/target_architectures
   noinst_HEADERS += ActivationFunction.hpp epd_graph.hpp extended_pdg_constructor.hpp parallel_regions_graph.hpp parallel_regions_graph_constructor.hpp
   lib_behavior_la_SOURCES += ActivationFunction.cpp epd_graph.cpp extended_pdg_constructor.cpp parallel_regions_graph.cpp parallel_regions_graph_constructor.cpp
endif

if BUILD_LIB_HOST_PROFILING
   lib_behavior_la_SOURCES += profiling_information.cpp
   noinst_HEADERS += profiling_information.hpp
endif

if BUILD_LIB_MAPPING
   lib_behavior_la_CPPFLAGS += \
      -I$(top_srcdir)/src/machine
endif

if BUILD_LIB_PRAGMA
   lib_behavior_la_CPPFLAGS += \
      -I$(top_srcdir)/src/pragma
endif

if USE_SOURCE_CODE_STATISTICS_XML
   lib_behavior_la_CPPFLAGS += \
      -I$(top_srcdir)/src/constants
endif

#DOCUMENTATION STUFF
DOX_TAGFILES= src/utility src/technology src/tree src/graph

PRJ_DOC = behavior.doc
EXTRA_DIST = $(PRJ_DOC)

#do not touch the following line
include $(top_srcdir)/panda.am
