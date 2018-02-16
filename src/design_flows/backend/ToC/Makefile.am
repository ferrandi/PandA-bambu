#this file is src/design_flows/backend/ToC/Makefile.am
SUBDIRS = progModels source_code_writers
DIST_SUBDIRS = progModels source_code_writers

METASOURCES = AUTO
noinst_LTLIBRARIES =  lib_ToC.la

lib_ToC_la_CPPFLAGS = \
   -I$(top_srcdir)/src \
   -I$(top_srcdir)/src/behavior \
   -I$(top_srcdir)/src/design_flows \
   -I$(top_srcdir)/src/design_flows/backend/ToC/progModels \
   -I$(top_srcdir)/src/design_flows/backend/ToC/source_code_writers \
   -I$(top_srcdir)/src/graph \
   -I$(top_srcdir)/src/utility \
   -I$(BOOST_DIR) $(all_includes)

lib_ToC_la_SOURCES = \
   c_backend_information.cpp \
   c_backend_step_factory.cpp
noinst_HEADERS = \
   c_backend_information.hpp \
   c_backend_step_factory.hpp

if BUILD_BAMBU
   lib_ToC_la_SOURCES += \
      hls_c_backend_information.cpp

   noinst_HEADERS += \
      hls_c_backend_information.hpp
endif

if BUILD_LIB_ACTOR_GRAPHS
   lib_ToC_la_CPPFLAGS += \
      -I$(top_srcdir)/src/intermediate_representations/actor_graphs
endif

if BUILD_LIB_GRAPH_PARTITIONING
   lib_ToC_la_CPPFLAGS += \
      -I$(top_srcdir)/src/design_flows/codesign/partitioning/graph_partitioning
endif

lib_ToC_la_LIBADD = \
   $(top_builddir)/src/design_flows/backend/ToC/progModels/lib_progModel.la \
   $(top_builddir)/src/design_flows/backend/ToC/source_code_writers/lib_source_code_writers.la

#do not touch the following line

