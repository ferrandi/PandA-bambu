#this file is src/pragma/Makefile.am
noinst_LTLIBRARIES = libpragma.la

libpragma_la_CPPFLAGS = \
   -I$(top_srcdir)/src \
   -I$(top_srcdir)/src/algorithms/loops_detection \
   -I$(top_srcdir)/src/behavior \
   -I$(top_srcdir)/src/constants \
   -I$(top_srcdir)/src/graph \
   -I$(top_srcdir)/src/machine \
   -I$(top_srcdir)/src/parser/pragma \
   -I$(top_srcdir)/src/parser/treegcc \
   -I$(top_srcdir)/src/tree \
   -I$(top_srcdir)/src/utility \
   -I$(BOOST_DIR) $(all_includes)

if BUILD_LIB_CODESIGN
if BUILD_LIB_MAPPING
   libpragma_la_CPPFLAGS += \
   -I$(top_srcdir)/src/design_flows/codesign/mapping
endif

if BUILD_LIB_PARTITIONING
if BUILD_LIB_GRAPH_PARTITIONING
   libpragma_la_CPPFLAGS += \
   -I$(top_srcdir)/src/design_flows/codesign/partitioning/graph_partitioning \
   -I$(top_srcdir)/src/design_flows/codesign/partitioning/graph_partitioning/utility
endif
endif
endif

if BUILD_LIB_ACTOR_GRAPHS
   libpragma_la_CPPFLAGS += \
   -I$(top_srcdir)/src/intermediate_representations/actor_graphs/

if BUILD_LIB_TASK_GRAPHS
   libpragma_la_CPPFLAGS += \
   -I$(top_srcdir)/src/intermediate_representations/actor_graphs/task_graphs
endif
endif

if BUILD_LIB_CODE_ESTIMATION
   libpragma_la_CPPFLAGS += \
      -I$(top_srcdir)/src/utility/probability_distribution
endif

if BUILD_LIB_MAPPING
   libpragma_la_CPPFLAGS += \
      -I$(top_srcdir)/src/intermediate_representations/mapping_annotations
endif

noinst_HEADERS = pragma_manager.hpp

#PRAGMA STUFF
libpragma_la_SOURCES = pragma_manager.cpp

#DOCUMENTATION STUFF
DOX_TAGFILES =

PRJ_DOC = pragma.doc
EXTRA_DIST = $(PRJ_DOC)

#do not touch the following line

