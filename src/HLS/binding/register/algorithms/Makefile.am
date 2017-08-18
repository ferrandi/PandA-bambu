#this file is src/HLS/binding/register/algorithms/Makefile.am
METASOURCES = AUTO

noinst_LTLIBRARIES = lib_register_algorithms.la

lib_register_algorithms_la_CPPFLAGS = \
   -I$(top_srcdir)/src/ \
   -I$(top_srcdir)/src/algorithms/clique_covering \
   -I$(top_srcdir)/src/algorithms/bipartite_matching \
   -I$(top_srcdir)/src/behavior \
   -I$(top_srcdir)/src/circuit \
   -I$(top_srcdir)/src/design_flows \
   -I$(top_srcdir)/src/graph \
   -I$(top_srcdir)/src/HLS \
   -I$(top_srcdir)/src/HLS/binding \
   -I$(top_srcdir)/src/HLS/binding/interconnection \
   -I$(top_srcdir)/src/HLS/binding/module \
   -I$(top_srcdir)/src/HLS/binding/register \
   -I$(top_srcdir)/src/HLS/binding/storage_value_insertion \
   -I$(top_srcdir)/src/HLS/liveness \
   -I$(top_srcdir)/src/HLS/scheduling \
   -I$(top_srcdir)/src/HLS/virtual_components \
   -I$(top_srcdir)/src/technology \
   -I$(top_srcdir)/src/technology/physical_library \
   -I$(top_srcdir)/src/technology/target_device \
   -I$(top_srcdir)/src/tree \
   -I$(top_srcdir)/src/utility \
   -I$(BOOST_DIR) $(all_includes)

noinst_HEADERS = conflict_based_register.hpp vertex_coloring_register.hpp \
                 compatibility_based_register.hpp \
                 chordal_coloring_register.hpp network_flow.hpp unique_binding_register.hpp weighted_clique_register.hpp

lib_register_algorithms_la_SOURCES = conflict_based_register.cpp vertex_coloring_register.cpp \
                                     compatibility_based_register.cpp \
                                     chordal_coloring_register.cpp network_flow.cpp unique_binding_register.cpp weighted_clique_register.cpp


if BUILD_LIB_EXPERIMENTAL
  noinst_HEADERS += left_edge_register.hpp conf_comp_based_register.hpp k_cofamily_register.hpp
  lib_register_algorithms_la_SOURCES += left_edge_register.cpp  conf_comp_based_register.cpp k_cofamily_register.cpp
  lib_register_algorithms_la_CPPFLAGS += \
   -I$(top_builddir)/ext/trng-4.17/
endif

#do not touch the following line
include $(top_srcdir)/panda.am
