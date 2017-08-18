#this file is src/graph/Makefile.am
METASOURCES = AUTO
noinst_HEADERS = edge_info.hpp graph.hpp graph_info.hpp node_info.hpp typed_node_info.hpp Vertex.hpp
noinst_LTLIBRARIES = lib_graph.la

lib_graph_la_CPPFLAGS = \
   -I$(top_srcdir)/src \
   -I$(top_srcdir)/src/behavior \
   -I$(top_srcdir)/src/utility \
   -I$(top_srcdir)/src/tree \
   -I$(BOOST_DIR) $(all_includes)

lib_graph_la_SOURCES = \
   graph.cpp \
   node_info.cpp \
   typed_node_info.cpp

#do not touch the following line
include $(top_srcdir)/panda.am
