#this file is src/HLS/hls_flow/synthesis/Makefile.am
noinst_LTLIBRARIES =  lib_hls_synthesis.la

lib_hls_synthesis_la_CPPFLAGS = \
   -I$(top_srcdir)/src \
   -I$(top_srcdir)/src/behavior \
   -I$(top_srcdir)/src/circuit \
   -I$(top_srcdir)/src/design_flows/ \
   -I$(top_srcdir)/src/design_flows/backend/ToHDL \
   -I$(top_srcdir)/src/frontend_analysis \
   -I$(top_srcdir)/src/graph \
   -I$(top_srcdir)/src/HLS \
   -I$(top_srcdir)/src/HLS/architecture_creation \
   -I$(top_srcdir)/src/HLS/architecture_creation/datapath_creation \
   -I$(top_srcdir)/src/HLS/architecture_creation/controller_creation \
   -I$(top_srcdir)/src/HLS/virtual_components \
   -I$(top_srcdir)/src/HLS/binding \
   -I$(top_srcdir)/src/HLS/binding/register \
   -I$(top_srcdir)/src/HLS/binding/register/algorithms \
   -I$(top_srcdir)/src/HLS/binding/storage_value_insertion \
   -I$(top_srcdir)/src/HLS/binding/module \
   -I$(top_srcdir)/src/HLS/binding/interconnection \
   -I$(top_srcdir)/src/HLS/chaining \
   -I$(top_srcdir)/src/HLS/function_allocation \
   -I$(top_srcdir)/src/HLS/hls_flow \
   -I$(top_srcdir)/src/HLS/interface \
   -I$(top_srcdir)/src/HLS/liveness \
   -I$(top_srcdir)/src/HLS/module_allocation \
   -I$(top_srcdir)/src/HLS/scheduling \
   -I$(top_srcdir)/src/HLS/stg \
   -I$(top_srcdir)/src/intermediate_representations \
   -I$(top_srcdir)/src/intermediate_representations/hls \
   -I$(top_srcdir)/src/polixml \
   -I$(top_srcdir)/src/tree \
   -I$(top_srcdir)/src/technology \
   -I$(top_srcdir)/src/technology/physical_library \
   -I$(top_srcdir)/src/technology/target_device \
   -I$(top_srcdir)/src/wrapper/synthesis \
   -I$(top_srcdir)/src/utility \
   -I$(BOOST_DIR) $(all_includes)

METASOURCES = AUTO

noinst_HEADERS = standard_hls.hpp virtual_hls.hpp hls_synthesis_flow.hpp
lib_hls_synthesis_la_SOURCES = standard_hls.cpp virtual_hls.cpp hls_synthesis_flow.cpp

if BUILD_LIB_FROM_PRAGMA
   noinst_HEADERS += omp_body_loop_synthesis_flow.hpp omp_for_wrapper_synthesis_flow.hpp omp_for_wrapper_cs_synthesis_flow.hpp
   lib_hls_synthesis_la_SOURCES += omp_body_loop_synthesis_flow.cpp omp_for_wrapper_synthesis_flow.cpp omp_for_wrapper_cs_synthesis_flow.cpp
endif

#do not touch the following line

