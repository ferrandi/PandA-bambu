#this is file src/HLS/liveness/Makefile.am
noinst_LTLIBRARIES =  lib_liveness.la

lib_liveness_la_CPPFLAGS =  \
                          -I$(top_srcdir)/src \
                          -I$(top_srcdir)/src/algorithms/loops_detection \
                          -I$(top_srcdir)/src/behavior \
                          -I$(top_srcdir)/src/design_flows \
                          -I$(top_srcdir)/src/graph \
                          -I$(top_srcdir)/src/HLS \
                          -I$(top_srcdir)/src/HLS/chaining \
                          -I$(top_srcdir)/src/HLS/memory \
                          -I$(top_srcdir)/src/HLS/scheduling \
                          -I$(top_srcdir)/src/HLS/stg \
                          -I$(top_srcdir)/src/polixml \
                          -I$(top_srcdir)/src/tree \
                          -I$(top_srcdir)/src/technology \
                          -I$(top_srcdir)/src/utility \
                          -I$(BOOST_DIR) $(all_includes)

METASOURCES = AUTO

noinst_HEADERS = liveness.hpp liveness_computer.hpp FSM_NI_SSA_liveness.hpp

lib_liveness_la_SOURCES = liveness.cpp liveness_computer.cpp FSM_NI_SSA_liveness.cpp

#do not touch the following line

