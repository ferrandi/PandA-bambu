#this file is src/design_flows/Makefile.am
SUBDIRS=
DIST_SUBDIRS=
METASOURCES = AUTO
noinst_LTLIBRARIES = lib_design_flows.la
lib_design_flows_la_LIBADD =
lib_design_flows_la_SOURCES = design_flow.cpp design_flow_aux_step.cpp design_flow_factory.cpp design_flow_graph.cpp design_flow_manager.cpp design_flow_step.cpp design_flow_step_factory.cpp non_deterministic_flows.cpp
noinst_HEADERS = design_flow.hpp design_flow_aux_step.hpp design_flow_factory.hpp design_flow_graph.hpp design_flow_manager.hpp design_flow_step.hpp design_flow_step_factory.hpp non_deterministic_flows.hpp
lib_design_flows_la_CPPFLAGS = \
   -I$(top_srcdir)/src \
   -I$(top_srcdir)/src/constants \
   -I$(top_srcdir)/src/graph \
   -I$(top_srcdir)/src/utility \
   -I$(BOOST_DIR) $(all_includes)

if BUILD_LIB_BACKEND
   SUBDIRS += backend
   DIST_SUBDIRS += backend
   lib_design_flows_la_LIBADD += $(top_builddir)/src/design_flows/backend/lib_backend.la
endif

if BUILD_LIB_CODESIGN
   SUBDIRS += codesign
   DIST_SUBDIRS += codesign
   lib_design_flows_la_LIBADD += $(top_builddir)/src/design_flows/codesign/lib_codesign.la
endif

if BUILD_LIB_TECHNOLOGY
   SUBDIRS += technology
   DIST_SUBDIRS += technology
   lib_design_flows_la_LIBADD += $(top_builddir)/src/design_flows/technology/lib_technology.la
endif

#do not touch the following line
include $(top_srcdir)/panda.am

