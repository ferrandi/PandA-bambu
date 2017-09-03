#this file is src/wrapper/Makefile.am
METASOURCES = AUTO
SUBDIRS =
DIST_SUBDIRS =
noinst_LTLIBRARIES = lib_wrapper.la
noinst_HEADERS = ToolManager.hpp

lib_wrapper_la_CPPFLAGS = \
    -I$(top_srcdir)/src \
    -I$(top_srcdir)/src/constants \
    -I$(top_srcdir)/src/utility \
    -I$(BOOST_DIR)

lib_wrapper_la_SOURCES = ToolManager.cpp
lib_wrapper_la_LIBADD =

if BUILD_LIB_FROM_C
  SUBDIRS += treegcc
  DIST_SUBDIRS += treegcc
  lib_wrapper_la_LIBADD += $(top_builddir)/src/wrapper/treegcc/lib_treegccwrapper.la
endif

if BUILD_LIB_TARGET_PROFILING
  SUBDIRS += target_architectures
  DIST_SUBDIRS += target_architectures
  lib_wrapper_la_LIBADD += $(top_builddir)/src/wrapper/target_architectures/lib_target_architectureswrapper.la
endif

if BUILD_SIMULATION
  SUBDIRS += simulation
  DIST_SUBDIRS += simulation
  lib_wrapper_la_LIBADD += $(top_builddir)/src/wrapper/simulation/lib_simulation.la
endif

if BUILD_NODELIB
  SUBDIRS += NeuralNetwork
  DIST_SUBDIRS += NeuralNetwork
  lib_wrapper_la_LIBADD += $(top_builddir)/src/wrapper/NeuralNetwork/lib_NNWrapper.la
endif

if BUILD_SYNTHESIS
  SUBDIRS += synthesis
  DIST_SUBDIRS += synthesis
  lib_wrapper_la_LIBADD += $(top_builddir)/src/wrapper/synthesis/lib_synthesis.la
endif

if BUILD_FLOPOCO
  SUBDIRS += flopoco
  DIST_SUBDIRS += flopoco
  lib_wrapper_la_LIBADD += $(top_builddir)/src/wrapper/flopoco/lib_flopocowrapper.la
endif

if BUILD_R
  SUBDIRS += R
  DIST_SUBDIRS += R
  lib_wrapper_la_LIBADD += $(top_builddir)/src/wrapper/R/lib_Rwrapper.la
endif

#do not touch the following line

