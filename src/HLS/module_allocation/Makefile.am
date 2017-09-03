#this is file src/HLS/module_allocation/Makefile.am
METASOURCES = AUTO

noinst_LTLIBRARIES = lib_allocation.la

lib_allocation_la_CPPFLAGS = \
   -I$(top_srcdir)/src/ \
   -I$(top_srcdir)/src/behavior \
   -I$(top_srcdir)/src/circuit \
   -I$(top_srcdir)/src/constants \
   -I$(top_srcdir)/src/design_flows \
   -I$(top_srcdir)/src/design_flows/backend/ToHDL  \
   -I$(top_srcdir)/src/frontend_analysis \
   -I$(top_srcdir)/src/graph \
   -I$(top_srcdir)/src/HLS \
   -I$(top_srcdir)/src/HLS/binding/module \
   -I$(top_srcdir)/src/HLS/binding/register \
   -I$(top_srcdir)/src/HLS/memory \
   -I$(top_srcdir)/src/HLS/function_allocation \
   -I$(top_srcdir)/src/HLS/module_generator \
   -I$(top_srcdir)/src/HLS/scheduling \
   -I$(top_srcdir)/src/HLS/stg \
   -I$(top_srcdir)/src/intermediate_representations \
   -I$(top_srcdir)/src/intermediate_representations/hls \
   -I$(top_srcdir)/src/technology \
   -I$(top_srcdir)/src/technology/physical_library \
   -I$(top_srcdir)/src/technology/physical_library/models \
   -I$(top_srcdir)/src/technology/physical_library/models/area \
   -I$(top_srcdir)/src/technology/target_device/ \
   -I$(top_srcdir)/src/tree \
   -I$(top_srcdir)/src/utility  \
   -I$(top_srcdir)/src/wrapper/flopoco \
   -I$(BOOST_DIR) $(all_includes)

noinst_HEADERS = \
   add_library.hpp \
   allocation.hpp \
   allocation_information.hpp \
   hls_bit_value.hpp

lib_allocation_la_SOURCES = \
   add_library.cpp \
   allocation.cpp \
   allocation_information.cpp \
   hls_bit_value.cpp

if BUILD_LIB_FROM_PRAGMA
if BUILD_LIB_EXPERIMENTAL
   noinst_HEADERS += \
      omp_allocation.hpp
   lib_allocation_la_SOURCES += \
      omp_allocation.cpp
endif
endif

#DOCUMENTATION STUFF
DOX_TAGFILES = src src/graph src/technology src/parser/HLS_constraintsIO src/HLS 
PRJ_DOC = main_allocation.doc allocation.doc binding_constraints.doc

EXTRA_DIST = $(PRJ_DOC)

#do not touch the following line


