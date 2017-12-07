#this is file src/HLS/binding/module/Makefile.am

noinst_LTLIBRARIES =  lib_modulebinding.la

lib_modulebinding_la_CPPFLAGS = \
   -I$(top_srcdir)/src \
   -I$(top_srcdir)/src/algorithms/clique_covering \
   -I$(top_srcdir)/src/algorithms/bipartite_matching \
   -I$(top_srcdir)/src/algorithms/clustering \
   -I$(top_srcdir)/src/behavior  \
   -I$(top_srcdir)/src/circuit \
   -I$(top_srcdir)/src/design_flows \
   -I$(top_srcdir)/src/design_flows/backend/ToHDL \
   -I$(top_srcdir)/src/graph \
   -I$(top_srcdir)/src/HLS \
   -I$(top_srcdir)/src/HLS/function_allocation \
   -I$(top_srcdir)/src/HLS/module_allocation \
   -I$(top_srcdir)/src/HLS/virtual_components \
   -I$(top_srcdir)/src/HLS/binding \
   -I$(top_srcdir)/src/HLS/binding/interconnection \
   -I$(top_srcdir)/src/HLS/binding/register \
   -I$(top_srcdir)/src/HLS/binding/register/algorithms \
   -I$(top_srcdir)/src/HLS/binding/storage_value_insertion \
   -I$(top_srcdir)/src/HLS/chaining \
   -I$(top_srcdir)/src/HLS/liveness \
   -I$(top_srcdir)/src/HLS/memory \
   -I$(top_srcdir)/src/HLS/scheduling \
   -I$(top_srcdir)/src/HLS/stg \
   -I$(top_srcdir)/src/intermediate_representations \
   -I$(top_srcdir)/src/intermediate_representations/hls \
   -I$(top_srcdir)/src/polixml \
   -I$(top_srcdir)/src/technology \
   -I$(top_srcdir)/src/technology/physical_library \
   -I$(top_srcdir)/src/technology/physical_library/models \
   -I$(top_srcdir)/src/technology/target_device \
   -I$(top_srcdir)/src/tree \
   -I$(top_srcdir)/src/utility \
   -I$(top_srcdir)/src/wrapper/flopoco \
   -I$(BOOST_DIR) $(all_includes)

METASOURCES = AUTO

noinst_HEADERS = fu_binding.hpp fu_binding_creator.hpp easy_module_binding.hpp unique_binding.hpp cdfc_module_binding.hpp module_binding_check.hpp

lib_modulebinding_la_SOURCES = fu_binding.cpp fu_binding_creator.cpp easy_module_binding.cpp unique_binding.cpp cdfc_module_binding.cpp


#DOCUMENTATION STUFF

DOX_TAGFILES= src

PRJ_DOC = fu_binding_creator.doc
EXTRA_DIST = $(PRJ_DOC)

#do not touch the following line

