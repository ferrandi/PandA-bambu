#this file is src/design_flows/backend/ToC/progModels/Makefile.am
noinst_LTLIBRARIES =  lib_progModel.la

lib_progModel_la_CPPFLAGS = \
   -I$(top_srcdir)/src/ \
   -I$(top_srcdir)/src/algorithms/dominance \
   -I$(top_srcdir)/src/algorithms/loops_detection \
   -I$(top_srcdir)/src/behavior \
   -I$(top_srcdir)/src/design_flows \
   -I$(top_srcdir)/src/design_flows/backend/ToC \
   -I$(top_srcdir)/src/design_flows/backend/ToC/source_code_writers \
   -I$(top_srcdir)/src/frontend_analysis \
   -I$(top_srcdir)/src/graph \
   -I$(top_srcdir)/src/parser/polixml \
   -I$(top_srcdir)/src/polixml \
   -I$(top_srcdir)/src/tree \
   -I$(top_srcdir)/src/utility \
   -I$(BOOST_DIR) $(all_includes)

METASOURCES = AUTO

noinst_HEADERS = \
  actor_graph_backend.hpp
lib_progModel_la_SOURCES = \
  actor_graph_backend.cpp

if BUILD_LIB_TO_C
  noinst_HEADERS += c_backend.hpp
  lib_progModel_la_SOURCES += c_backend.cpp
endif

if BUILD_LIB_HLS
  lib_progModel_la_CPPFLAGS += \
   -I$(top_srcdir)/src/HLS
endif

#do not touch the following line

