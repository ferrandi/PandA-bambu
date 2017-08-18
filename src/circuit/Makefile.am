#this file is src/circuit/Makefile.am
METASOURCES = AUTO

SUBDIRS =
DIST_SUBDIRS =


noinst_LTLIBRARIES = lib_circuit.la lib_structuralIO.la

lib_circuit_la_CPPFLAGS = \
   -I$(top_srcdir)/src \
   -I$(top_srcdir)/src/behavior \
   -I$(top_srcdir)/src/circuit \
   -I$(top_srcdir)/src/constants \
   -I$(top_srcdir)/src/design_flows \
   -I$(top_srcdir)/src/design_flows/backend/ToHDL \
   -I$(top_srcdir)/src/design_flows/backend/ToHDL/writer \
   -I$(top_srcdir)/src/frontend_analysis \
   -I$(top_srcdir)/src/graph \
   -I$(top_srcdir)/src/HLS \
   -I$(top_srcdir)/src/HLS/module_allocation \
   -I$(top_srcdir)/src/HLS/scheduling \
   -I$(top_srcdir)/src/polixml \
   -I$(top_srcdir)/src/technology \
   -I$(top_srcdir)/src/technology/physical_library \
   -I$(top_srcdir)/src/technology/physical_library/models \
   -I$(top_srcdir)/src/technology/target_device \
   -I$(top_srcdir)/src/technology/target_technology \
   -I$(top_srcdir)/src/technology/target_technology/FPGA \
   -I$(top_srcdir)/src/tree \
   -I$(top_srcdir)/src/utility \
   -I$(top_srcdir)/src/wrapper/simulation/icarus \
   -I$(top_srcdir)/src/wrapper/synthesis/synopsys/design_compiler \
   -I$(BOOST_DIR) $(all_includes)

if BUILD_LIB_PRAGMA
  lib_circuit_la_CPPFLAGS += -I$(top_srcdir)/src/pragma
endif

AM_YFLAGS = -d -l --report=state,lookahead,itemset
AM_LFLAGS = -L -+ -olex.yy.c

noinst_HEADERS = structuralIO.hpp structural_manager.hpp cg_node.hpp structural_objects.hpp NP_functionality.hpp
lib_circuit_la_SOURCES = structural_objects.cpp structural_manager.cpp cg_node.cpp NP_functionality.cpp

lib_circuit_la_LIBADD =

if BUILD_LIB_FROM_SYSTEMC
  noinst_HEADERS += SC_structural_manager.hpp tree_to_structural_manager.hpp
  lib_circuit_la_SOURCES += SC_structural_manager.cpp tree_to_structural_manager.cpp
  lib_circuit_la_CPPFLAGS += \
    -I$(top_srcdir)/src/design_flows
endif

if BUILD_LIB_EDIF
  SUBDIRS += edif
  DIST_SUBDIRS += edif
  lib_circuit_la_CPPFLAGS += \
                            -I$(top_srcdir)/src/circuit/edif
  lib_circuit_la_LIBADD += $(top_builddir)/src/circuit/edif/lib_edif.la
endif

if BUILD_LIB_AIG
  SUBDIRS += aig
  DIST_SUBDIRS += aig
  lib_circuit_la_CPPFLAGS += \
                            -I$(top_srcdir)/src/circuit/aig
  lib_circuit_la_LIBADD += $(top_builddir)/src/circuit/aig/lib_aig.la
endif

lib_structuralIO_la_CPPFLAGS = \
   -I$(top_srcdir)/src \
   -I$(top_srcdir)/src/constants \
   -I$(top_srcdir)/src/graph \
   -I$(top_srcdir)/src/parser/polixml \
   -I$(top_srcdir)/src/polixml \
   -I$(top_srcdir)/src/technology \
   -I$(top_srcdir)/src/technology/physical_library \
   -I$(top_srcdir)/src/technology/target_device \
   -I$(top_srcdir)/src/utility \
   -I$(BOOST_DIR) $(all_includes)

lib_structuralIO_la_SOURCES = structuralIO.cpp

#DOCUMENTATION STUFF

DOX_TAGFILES = src/utility src/tree src/technology src/design_flows/backend/ToHDL

PRJ_DOC = circuit.doc
EXTRA_DIST = $(PRJ_DOC)

#do not touch the following line
include $(top_srcdir)/panda.am
