#this file is src/HLS/evaluation/exact/Makefile.am
METASOURCES = AUTO
noinst_LTLIBRARIES = lib_exact_evaluation.la

lib_exact_evaluation_la_CPPFLAGS = \
   -I$(top_srcdir)/src  \
   -I$(top_srcdir)/src/algorithms/loops_detection  \
   -I$(top_srcdir)/src/design_flows/  \
   -I$(top_srcdir)/src/design_flows/backend  \
   -I$(top_srcdir)/src/design_flows/backend/ToHDL  \
   -I$(top_srcdir)/src/behavior  \
   -I$(top_srcdir)/src/circuit  \
   -I$(top_srcdir)/src/graph \
   -I$(top_srcdir)/src/HLS \
   -I$(top_srcdir)/src/HLS/architecture_creation \
   -I$(top_srcdir)/src/HLS/architecture_creation/controller_creation \
   -I$(top_srcdir)/src/HLS/architecture_creation/datapath_creation \
   -I$(top_srcdir)/src/HLS/evaluation \
   -I$(top_srcdir)/src/HLS/scheduling \
   -I$(top_srcdir)/src/HLS/simulation \
   -I$(top_srcdir)/src/technology \
   -I$(top_srcdir)/src/technology/physical_library \
   -I$(top_srcdir)/src/technology/physical_library/models \
   -I$(top_srcdir)/src/technology/physical_library/models/area \
   -I$(top_srcdir)/src/technology/target_device \
   -I$(top_srcdir)/src/tree  \
   -I$(top_srcdir)/src/utility \
   -I$(top_srcdir)/src/wrapper/simulation \
   -I$(top_srcdir)/src/wrapper/simulation/modelsim \
   -I$(top_srcdir)/src/wrapper/synthesis/ \
   -I$(top_srcdir)/src/wrapper/treegcc \
   -I$(BOOST_DIR) $(all_includes)

noinst_HEADERS =
lib_exact_evaluation_la_SOURCES =

if BUILD_LIB_RTL
   lib_exact_evaluation_la_CPPFLAGS += \
      -I$(top_srcdir)/src/rtl
endif

if BUILD_SIMULATION
   noinst_HEADERS += simulation_evaluation.hpp
   lib_exact_evaluation_la_SOURCES += simulation_evaluation.cpp
endif

if BUILD_LIB_RTL_CHARACTERIZATION
   noinst_HEADERS += synthesis_evaluation.hpp
   lib_exact_evaluation_la_SOURCES += synthesis_evaluation.cpp
endif

#do not touch the following line

