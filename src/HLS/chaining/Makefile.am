#this file is src/HLS/chaining/Makefile.am
METASOURCES = AUTO
noinst_LTLIBRARIES = lib_hls_chaining.la

lib_hls_chaining_la_CPPFLAGS = \
   -I$(top_srcdir)/src \
   -I$(top_srcdir)/src/behavior \
   -I$(top_srcdir)/src/HLS \
   -I$(top_srcdir)/src/design_flows \
   -I$(top_srcdir)/src/graph \
   -I$(top_srcdir)/src/polixml \
   -I$(top_srcdir)/src/parser/polixml \
   -I$(top_srcdir)/src/HLS/scheduling \
   -I$(top_srcdir)/src/HLS/module_allocation \
   -I$(top_srcdir)/src/HLS/binding/module \
   -I$(top_srcdir)/src/HLS/virtual_components \
   -I$(top_srcdir)/src/intermediate_representations \
   -I$(top_srcdir)/src/intermediate_representations/hls \
   -I$(top_srcdir)/src/utility \
   -I$(BOOST_DIR) $(all_includes)

noinst_HEADERS =  chaining.hpp sched_based_chaining_computation.hpp chaining_information.hpp
lib_hls_chaining_la_SOURCES = chaining.cpp sched_based_chaining_computation.cpp chaining_information.cpp

if BUILD_LIB_EXPERIMENTAL
noinst_HEADERS +=  epdg_sched_based_chaining_computation.hpp
lib_hls_chaining_la_SOURCES +=  epdg_sched_based_chaining_computation.cpp
endif


#do not touch the following line
include $(top_srcdir)/panda.am
