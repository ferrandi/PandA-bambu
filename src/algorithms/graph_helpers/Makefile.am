#this file is src/algorithms/graph_helpers/Makefile.am
SUBDIRS =
DIST_SUBDIRS =

METASOURCES = AUTO

noinst_LTLIBRARIES =  lib_graph_helpers.la

noinst_HEADERS = cyclic_topological_sort.hpp
lib_graph_helpers_la_CPPFLAGS =
lib_graph_helpers_la_SOURCES =
lib_graph_helpers_la_LIBADD =

if BUILD_LIB_BSG_HELPERS
   SUBDIRS += bsg_helpers
   DIST_SUBDIRS += bsg_helpers
   lib_graph_helpers_la_LIBADD += $(top_builddir)/src/algorithms/graph_helpers/bsg_helpers/lib_bsg_helpers.la
endif

#DOCUMENTATION STUFF
DOX_TAGFILES=

PRJ_DOC =
EXTRA_DIST = $(PRJ_DOC)

#do not touch the following line

