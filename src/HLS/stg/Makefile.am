#this is file src/HLS/stg/Makefile.am
METASOURCES = AUTO

noinst_LTLIBRARIES =  lib_stg.la

lib_stg_la_CPPFLAGS = \
   -I$(top_srcdir)/src \
   -I$(top_srcdir)/src/algorithms/graph_helpers \
   -I$(top_srcdir)/src/algorithms/loops_detection \
   -I$(top_srcdir)/src/behavior \
   -I$(top_srcdir)/src/circuit \
   -I$(top_srcdir)/src/design_flows \
   -I$(top_srcdir)/src/graph \
   -I$(top_srcdir)/src/HLS \
   -I$(top_srcdir)/src/HLS/function_allocation \
   -I$(top_srcdir)/src/HLS/module_allocation \
   -I$(top_srcdir)/src/HLS/memory \
   -I$(top_srcdir)/src/HLS/virtual_components \
   -I$(top_srcdir)/src/HLS/binding \
   -I$(top_srcdir)/src/HLS/binding/interconnection \
   -I$(top_srcdir)/src/HLS/binding/module \
   -I$(top_srcdir)/src/HLS/binding/register \
   -I$(top_srcdir)/src/HLS/liveness \
   -I$(top_srcdir)/src/HLS/scheduling \
   -I$(top_srcdir)/src/HLS/vcd \
   -I$(top_srcdir)/src/intermediate_representations \
   -I$(top_srcdir)/src/intermediate_representations/hls \
   -I$(top_srcdir)/src/parser/discrepancy \
   -I$(top_srcdir)/src/polixml \
   -I$(top_srcdir)/src/technology \
   -I$(top_srcdir)/src/technology/physical_library \
   -I$(top_srcdir)/src/technology/physical_library/models \
   -I$(top_srcdir)/src/technology/target_device \
   -I$(top_srcdir)/src/tree \
   -I$(top_srcdir)/src/utility \
   -I$(BOOST_DIR) $(all_includes)

noinst_HEADERS = state_transition_graph.hpp state_transition_graph_manager.hpp StateTransitionGraph_constructor.hpp STG_creator.hpp BB_based_stg.hpp

lib_stg_la_SOURCES = state_transition_graph.cpp state_transition_graph_manager.cpp StateTransitionGraph_constructor.cpp STG_creator.cpp BB_based_stg.cpp

#do not touch the following line

