#this file is src/frontend_analysis/Makefile.am
SUBDIRS = behavior_analysis IR_analysis IR_manipulation
DIST_SUBDIRS = behavior_analysis IR_analysis IR_manipulation
METASOURCES = AUTO
noinst_LTLIBRARIES = lib_frontend_analysis.la
noinst_HEADERS = \
   application_frontend_flow_step.hpp \
   complete_bb_graph.hpp \
   complete_call_graph.hpp \
   frontend_flow_step_factory.hpp \
   frontend_flow_step.hpp \
   function_frontend_flow_step.hpp \
   symbolic_application_frontend_flow_step.hpp

lib_frontend_analysis_la_CPPFLAGS = \
   -I$(top_srcdir)/src \
   -I$(top_srcdir)/src/behavior \
   -I$(top_srcdir)/src/design_flows \
   -I$(top_srcdir)/src/frontend_analysis/behavior_analysis \
   -I$(top_srcdir)/src/frontend_analysis/IR_analysis \
   -I$(top_srcdir)/src/frontend_analysis/IR_manipulation \
   -I$(top_srcdir)/src/frontend_analysis/source_code_manipulation \
   -I$(top_srcdir)/src/graph \
   -I$(top_srcdir)/src/tree \
   -I$(top_srcdir)/src/utility \
   -I$(BOOST_DIR) $(all_includes)

lib_frontend_analysis_la_SOURCES = \
   application_frontend_flow_step.cpp \
   complete_bb_graph.cpp \
   complete_call_graph.cpp \
   frontend_flow_step_factory.cpp \
   frontend_flow_step.cpp \
   function_frontend_flow_step.cpp \
   symbolic_application_frontend_flow_step.cpp

lib_frontend_analysis_la_LIBADD = \
   $(top_builddir)/src/frontend_analysis/behavior_analysis/lib_behavior_analysis.la \
   $(top_builddir)/src/frontend_analysis/IR_analysis/lib_IR_analysis.la \
   $(top_builddir)/src/frontend_analysis/IR_manipulation/lib_IR_manipulation.la

if BUILD_BAMBU
   lib_frontend_analysis_la_SOURCES += \
      bambu_frontend_flow.cpp
   noinst_HEADERS += \
      bambu_frontend_flow.hpp
   lib_frontend_analysis_la_CPPFLAGS += \
      -I$(top_srcdir)/src/design_flows/backend/ToHDL \
      -I$(top_srcdir)/src/HLS \
      -I$(top_srcdir)/src/HLS/module_allocation \
      -I$(top_srcdir)/src/technology \
      -I$(top_srcdir)/src/technology/target_device
endif

if BUILD_BAMBU
   lib_frontend_analysis_la_CPPFLAGS += \
      -I$(top_srcdir)/src/constants
   noinst_HEADERS += \
      find_max_cfg_transformations.hpp
   lib_frontend_analysis_la_SOURCES += \
      find_max_cfg_transformations.cpp
endif

if BUILD_LIB_CODE_ESTIMATION
   lib_frontend_analysis_la_CPPFLAGS += \
      -I$(top_srcdir)/src/utility/probability_distribution
endif

if BUILD_LIB_RTL
   lib_frontend_analysis_la_CPPFLAGS += \
      -I$(top_srcdir)/src/rtl
endif

if BUILD_SOURCE_CODE_MANIPULATION
   SUBDIRS += source_code_manipulation
   DIST_SUBDIRS += source_code_manipulation
   lib_frontend_analysis_la_LIBADD +=\
       $(top_builddir)/src/frontend_analysis/source_code_manipulation/lib_source_code_manipulation.la
endif

#do not touch the following line

