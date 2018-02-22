#this is file src/HLS/binding/interconnection/Makefile.am
noinst_LTLIBRARIES =  lib_interconnectionbinding.la

lib_interconnectionbinding_la_CPPFLAGS = \
   -I$(top_srcdir)/src \
   -I$(top_srcdir)/src/algorithms/clique_covering \
   -I$(top_srcdir)/src/behavior  \
   -I$(top_srcdir)/src/circuit \
   -I$(top_srcdir)/src/design_flows \
   -I$(top_srcdir)/src/graph \
   -I$(top_srcdir)/src/HLS \
   -I$(top_srcdir)/src/HLS/binding \
   -I$(top_srcdir)/src/HLS/binding/module \
   -I$(top_srcdir)/src/HLS/binding/register \
   -I$(top_srcdir)/src/HLS/binding/register/algorithms \
   -I$(top_srcdir)/src/HLS/binding/storage_value_insertion \
   -I$(top_srcdir)/src/parser/treegcc\
   -I$(top_srcdir)/src/HLS/function_allocation \
   -I$(top_srcdir)/src/HLS/liveness \
   -I$(top_srcdir)/src/HLS/memory \
   -I$(top_srcdir)/src/HLS/module_allocation \
   -I$(top_srcdir)/src/HLS/scheduling \
   -I$(top_srcdir)/src/HLS/stg \
   -I$(top_srcdir)/src/HLS/virtual_components \
   -I$(top_srcdir)/src/intermediate_representations \
   -I$(top_srcdir)/src/intermediate_representations/hls \
   -I$(top_srcdir)/src/parser/treegcc \
   -I$(top_srcdir)/src/polixml \
   -I$(top_srcdir)/src/technology \
   -I$(top_srcdir)/src/technology/physical_library \
   -I$(top_srcdir)/src/technology/physical_library/models \
   -I$(top_srcdir)/src/technology/target_device \
   -I$(top_srcdir)/src/tree \
   -I$(top_srcdir)/src/wrapper/flopoco \
   -I$(top_srcdir)/src/utility -I$(BOOST_DIR) $(all_includes)

METASOURCES = AUTO

noinst_HEADERS = conn_binding_creator.hpp connection_obj.hpp conn_binding.hpp port_swapping.hpp mux_connection_binding.hpp  direct_conn.hpp mux_conn.hpp

lib_interconnectionbinding_la_SOURCES = conn_binding_creator.cpp conn_binding.cpp port_swapping.cpp mux_connection_binding.cpp

PRJ_DOC = mux_connection_binding.doc

#do not touch the following line

