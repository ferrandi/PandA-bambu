#this file is src/HLS/evaluation/Makefile.am
SUBDIRS = exact
DIST_SUBDIRS = exact

if BUILD_LIB_EXPERIMENTAL
  SUBDIRS += linear
  DIST_SUBDIRS += linear
endif


noinst_LTLIBRARIES =  lib_evaluation.la

lib_evaluation_la_CPPFLAGS = \
   -I$(top_srcdir)/src \
   -I$(top_srcdir)/src/HLS \
   -I$(top_srcdir)/src/HLS/binding \
   -I$(top_srcdir)/src/HLS/binding/interconnection \
   -I$(top_srcdir)/src/HLS/binding/module \
   -I$(top_srcdir)/src/HLS/binding/register \
   -I$(top_srcdir)/src/HLS/evaluation/exact \
   -I$(top_srcdir)/src/HLS/evaluation/linear \
   -I$(top_srcdir)/src/HLS/module_allocation \
   -I$(top_srcdir)/src/HLS/scheduling \
   -I$(top_srcdir)/src/HLS/simulation \
   -I$(top_srcdir)/src/algorithms/loops_detection  \
   -I$(top_srcdir)/src/design_flows/\
   -I$(top_srcdir)/src/design_flows/backend  \
   -I$(top_srcdir)/src/behavior  \
   -I$(top_srcdir)/src/circuit  \
   -I$(top_srcdir)/src/constants  \
   -I$(top_srcdir)/src/graph \
   -I$(top_srcdir)/src/polixml \
   -I$(top_srcdir)/src/technology \
   -I$(top_srcdir)/src/technology/physical_library \
   -I$(top_srcdir)/src/technology/physical_library/models \
   -I$(top_srcdir)/src/technology/target_device \
   -I$(top_srcdir)/src/tree  \
   -I$(top_srcdir)/src/wrapper/synthesis/ \
   -I$(top_srcdir)/src/wrapper/synthesis/xilinx/ise \
   -I$(top_srcdir)/src/wrapper/simulation \
   -I$(top_srcdir)/src/wrapper/simulation/modelsim \
   -I$(top_srcdir)/src/utility \
   -I$(BOOST_DIR) $(all_includes)

METASOURCES = AUTO

noinst_HEADERS = evaluation.hpp evaluation_base_step.hpp dry_run_evaluation.hpp

lib_evaluation_la_SOURCES = evaluation.cpp evaluation_base_step.cpp dry_run_evaluation.cpp

lib_evaluation_la_LIBADD = $(top_builddir)/src/HLS/evaluation/exact/lib_exact_evaluation.la

if BUILD_LIB_EXPERIMENTAL
  lib_evaluation_la_CPPFLAGS += \
    -I$(top_srcdir)/src/HLS/evaluation/linear
  lib_evaluation_la_LIBADD += \
     $(top_builddir)/src/HLS/evaluation/linear/lib_linear_estimation.la
endif

#DOCUMENTATION STUFF
DOX_TAGFILES= src src/HLS

PRJ_DOC = evaluation.doc
EXTRA_DIST = $(PRJ_DOC)

#do not touch the following line

