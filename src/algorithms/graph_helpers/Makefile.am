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

if BUILD_LIB_TRANSITIVE_REDUCTION
   noinst_HEADERS += TransitiveReduction.hpp
   lib_graph_helpers_la_SOURCES += TransitiveReduction.cpp
   lib_graph_helpers_la_CPPFLAGS += \
     -I$(top_srcdir)/src \
     -I$(top_srcdir)/src/graph \
     -I$(top_srcdir)/src/utility -I$(BOOST_DIR) $(all_includes)
endif

if BUILD_ZEBU
  noinst_HEADERS += DistanceCalc.hpp TopOrderVisitor.hpp
  lib_graph_helpers_la_CPPFLAGS += -I$(top_srcdir)/src/utility -I$(top_srcdir)/src/graph -I$(BOOST_DIR) $(all_includes)
endif

#DOCUMENTATION STUFF
DOX_TAGFILES=

PRJ_DOC =
EXTRA_DIST = $(PRJ_DOC)

#do not touch the following line
include $(top_srcdir)/panda.am
