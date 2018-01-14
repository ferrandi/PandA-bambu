#this is file src/HLS/function_allocation/Makefile.am
METASOURCES = AUTO

noinst_LTLIBRARIES = lib_function_allocation.la

lib_function_allocation_la_CPPFLAGS = \
   -I$(top_srcdir)/src \
   -I$(top_srcdir)/src/algorithms/dominance \
   -I$(top_srcdir)/src/behavior \
   -I$(top_srcdir)/src/design_flows \
   -I$(top_srcdir)/src/design_flows/technology \
   -I$(top_srcdir)/src/frontend_analysis \
   -I$(top_srcdir)/src/graph \
   -I$(top_srcdir)/src/HLS \
   -I$(top_srcdir)/src/HLS/interface \
   -I$(top_srcdir)/src/tree \
   -I$(top_srcdir)/src/utility  \
   -I$(top_srcdir)/src/polixml \
   -I$(top_srcdir)/src/technology  \
   -I$(top_srcdir)/src/technology/physical_library \
   -I$(top_srcdir)/src/technology/target_device/ \
   -I$(BOOST_DIR) $(all_includes)

noinst_HEADERS = \
   function_allocation.hpp fun_dominator_allocation.hpp functions.hpp

lib_function_allocation_la_SOURCES = \
   function_allocation.cpp fun_dominator_allocation.cpp functions.cpp

if BUILD_LIB_FROM_PRAGMA
   lib_function_allocation_la_CPPFLAGS += \
      -I$(top_srcdir)/src/design_flows/codesign

   noinst_HEADERS += \
      omp_function_allocation.hpp \
      omp_functions.hpp

   lib_function_allocation_la_SOURCES += \
      omp_function_allocation.cpp \
      omp_functions.cpp
endif
#DOCUMENTATION STUFF
DOX_TAGFILES = 
PRJ_DOC = 

EXTRA_DIST = $(PRJ_DOC)

#do not touch the following line


