#this file is src/frontend_analysis/behavior_analysis/Makefile.am
METASOURCES = AUTO
noinst_LTLIBRARIES = lib_behavior_analysis.la

lib_behavior_analysis_la_LIBADD =

lib_behavior_analysis_la_CPPFLAGS = \
   -I$(top_srcdir)/src \
   -I$(top_srcdir)/src/algorithms/dominance \
   -I$(top_srcdir)/src/algorithms/loops_detection \
   -I$(top_srcdir)/src/behavior \
   -I$(top_srcdir)/src/design_flows \
   -I$(top_srcdir)/src/design_flows/backend/ToC \
   -I$(top_srcdir)/src/design_flows/backend/ToC/progModels \
   -I$(top_srcdir)/src/design_flows/backend/ToC/source_code_writers \
   -I$(top_srcdir)/src/frontend_analysis \
   -I$(top_srcdir)/src/graph \
   -I$(top_srcdir)/src/parser/polixml \
   -I$(top_srcdir)/src/polixml\
   -I$(top_srcdir)/src/technology \
   -I$(top_srcdir)/src/technology/target_device \
   -I$(top_srcdir)/src/tree \
   -I$(top_srcdir)/src/utility \
   -I$(top_srcdir)/src/HLS \
   -I$(BOOST_DIR) $(all_includes)

noinst_HEADERS = \
   add_bb_ecfg_edges.hpp \
   add_op_exit_flow_edges.hpp \
   add_op_loop_flow_edges.hpp \
   aggregate_data_flow_analysis.hpp \
   bb_cdg_computation.hpp \
   bb_feedback_edges_computation.hpp \
   bb_order_computation.hpp \
   bb_reachability_computation.hpp \
   data_dependence_computation.hpp \
   dom_post_dom_computation.hpp \
   loops_computation.hpp \
   memory_data_flow_analysis.hpp \
   op_cdg_computation.hpp \
   op_feedback_edges_computation.hpp \
   op_order_computation.hpp \
   op_reachability_computation.hpp \
   scalar_ssa_data_dependence_computation.hpp \
   virtual_aggregate_data_flow_analysis.hpp


lib_behavior_analysis_la_SOURCES = \
   add_bb_ecfg_edges.cpp \
   add_op_exit_flow_edges.cpp \
   add_op_loop_flow_edges.cpp \
   aggregate_data_flow_analysis.cpp \
   bb_cdg_computation.cpp \
   bb_feedback_edges_computation.cpp \
   bb_order_computation.cpp \
   bb_reachability_computation.cpp \
   data_dependence_computation.cpp \
   dom_post_dom_computation.cpp \
   loops_computation.cpp \
   memory_data_flow_analysis.cpp \
   op_cdg_computation.cpp \
   op_feedback_edges_computation.cpp \
   op_order_computation.cpp \
   op_reachability_computation.cpp \
   scalar_ssa_data_dependence_computation.cpp \
   virtual_aggregate_data_flow_analysis.cpp

if BUILD_BAMBU
   noinst_HEADERS += \
      AddArtificialCallFlowEdges.hpp \
      add_op_phi_flow_edges.hpp
   lib_behavior_analysis_la_SOURCES += \
      AddArtificialCallFlowEdges.cpp \
      add_op_phi_flow_edges.cpp
   lib_behavior_analysis_la_CPPFLAGS += \
      -I$(top_srcdir)/src/design_flows/technology
endif

if BUILD_LIB_HOST_PROFILING
   lib_behavior_analysis_la_CPPFLAGS += \
      -I$(top_srcdir)/src/constants \
      -I$(top_srcdir)/src/wrapper/treegcc

   noinst_HEADERS += \
      basic_blocks_profiling.hpp \
      host_profiling.hpp

   lib_behavior_analysis_la_SOURCES += \
      basic_blocks_profiling.cpp \
      host_profiling.cpp
endif

DOX_TAGFILES =

PRJ_DOC = 
EXTRA_DIST = $(PRJ_DOC)

#do not touch the following line

