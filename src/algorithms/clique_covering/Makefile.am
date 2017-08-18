#this file is src/algorithms/clique_covering/Makefile.am
METASOURCES = AUTO

noinst_HEADERS = check_clique.hpp clique_covering.hpp clique_covering_graph.hpp degree_coloring.hpp dsatur2_coloring.hpp dsatur_coloring.hpp filter_clique.hpp maxclique_dsatur_coloring.hpp

lib_clique_covering_la_SOURCES = clique_covering.cpp

noinst_LTLIBRARIES =  lib_clique_covering.la

lib_clique_covering_la_CPPFLAGS = \
   -I$(top_srcdir)/src/ \
   -I$(top_srcdir)/src/algorithms/bipartite_matching \
   -I$(top_srcdir)/src/utility \
   -I$(BOOST_DIR) $(all_includes)

if BUILD_TRNG
lib_clique_covering_la_CPPFLAGS += \
   -I$(top_builddir)/ext/trng-4.17
endif

if BUILD_LIB_EXPERIMENTAL
  noinst_HEADERS+= \
    DawsonRun.hpp
endif

#DOCUMENTATION STUFF

DOX_TAGFILES= 

PRJ_DOC = clique_covering.doc 
EXTRA_DIST = $(PRJ_DOC) test_degree_coloring.cpp test_dsatur2_coloring.cpp test_dsatur_coloring.cpp test_maxclique_dsatur_coloring.cpp


#do not touch the following line
include $(top_srcdir)/panda.am

