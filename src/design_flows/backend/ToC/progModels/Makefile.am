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

if BUILD_LIB_CODESIGN
  lib_progModel_la_CPPFLAGS += \
   -I$(top_srcdir)/src/algorithms/graph_helpers \
   -I$(top_srcdir)/src/design_flows/codesign/mapping \
   -I$(top_srcdir)/src/design_flows/codesign/partitioning/graph_partitioning \
   -I$(top_srcdir)/src/design_flows/codesign/partitioning/graph_partitioning/models/Fork_Join \
   -I$(top_srcdir)/src/intermediate_representations/actor_graphs/ \
   -I$(top_srcdir)/src/intermediate_representations/actor_graphs/sdf_graphs \
   -I$(top_srcdir)/src/intermediate_representations/actor_graphs/task_graphs \
   -I$(top_srcdir)/src/machine \
   -I$(top_srcdir)/src/pragma \
   -I$(top_srcdir)/src/utility/probability_distribution

  noinst_HEADERS += \
     fork_join_flat_backend.hpp \
     sdf_graph_backend.hpp \
     scheduled_fork_join_flat_backend.hpp
  lib_progModel_la_SOURCES += \
     fork_join_flat_backend.cpp \
     sdf_graph_backend.cpp \
     scheduled_fork_join_flat_backend.cpp
endif

if BUILD_LIB_TO_C
  noinst_HEADERS += c_backend.hpp
  lib_progModel_la_SOURCES += c_backend.cpp
endif

if BUILD_LIB_HLS
  lib_progModel_la_CPPFLAGS += \
   -I$(top_srcdir)/src/HLS
endif

#do not touch the following line
include $(top_srcdir)/panda.am
