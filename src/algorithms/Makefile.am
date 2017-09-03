#this file is src/algorithms/Makefile.am
METASOURCES = AUTO
SUBDIRS =
DIST_SUBDIRS = 
noinst_LTLIBRARIES = lib_algorithms.la 
lib_algorithms_la_LIBADD = 
lib_algorithms_la_SOURCES =

if BUILD_LIB_CLIQUECOVER
   SUBDIRS += clique_covering bipartite_matching
   DIST_SUBDIRS += clique_covering bipartite_matching
   lib_algorithms_la_LIBADD += $(top_builddir)/src/algorithms/clique_covering/lib_clique_covering.la
endif

if BUILD_LIB_CLUSTERING
   SUBDIRS += clustering
   DIST_SUBDIRS += clustering
endif

if BUILD_LIB_DOMINANCE
   SUBDIRS += dominance
   DIST_SUBDIRS += dominance
   lib_algorithms_la_LIBADD += $(top_builddir)/src/algorithms/dominance/lib_dominance.la
endif

if BUILD_LIB_SETHELPER
   SUBDIRS += set_helper
   DIST_SUBDIRS += set_helper
   lib_algorithms_la_LIBADD += $(top_builddir)/src/algorithms/set_helper/lib_SetHelp.la
endif

if BUILD_LIB_GRAPHHELPER
   SUBDIRS += graph_helpers
   DIST_SUBDIRS += graph_helpers
   lib_algorithms_la_LIBADD += $(top_builddir)/src/algorithms/graph_helpers/lib_graph_helpers.la
endif

if BUILD_LIB_LOOPS
   SUBDIRS += loops_detection
   DIST_SUBDIRS += loops_detection
   lib_algorithms_la_LIBADD += $(top_builddir)/src/algorithms/loops_detection/lib_loops_detection.la
endif

if BUILD_LIB_BOOLEAN
   SUBDIRS += boolean_functions
   DIST_SUBDIRS += boolean_functions
   lib_algorithms_la_LIBADD += $(top_builddir)/src/algorithms/boolean_functions/lib_boolean_functions.la
endif

if BUILD_LIB_EVALUATION
   SUBDIRS += evaluation
   DIST_SUBDIRS += evaluation
   lib_algorithms_la_LIBADD += $(top_builddir)/src/algorithms/evaluation/lib_evaluation.la
endif

if BUILD_LIB_REGRESSORS
   SUBDIRS += regressors
   DIST_SUBDIRS += regressors
   lib_algorithms_la_LIBADD += $(top_builddir)/src/algorithms/regressors/lib_regressors.la
endif

if BUILD_VFLIB
   SUBDIRS += pattern_matching
   lib_algorithms_la_LIBADD += $(top_builddir)/src/algorithms/pattern_matching/lib_pattern_matching.la \
                               $(top_builddir)/ext/vflib/src/libvf.la
endif

#do not touch the following line

