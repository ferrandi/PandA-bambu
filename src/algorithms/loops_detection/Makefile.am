#this file is src/algorithms/loop_detection/Makefile.am
METASOURCES = AUTO

noinst_LTLIBRARIES =  lib_loops_detection.la

noinst_HEADERS = loops.hpp loop.hpp

lib_loops_detection_la_CPPFLAGS = \
   -I$(top_srcdir)/src/ \
   -I$(top_srcdir)/src/algorithms/dominance \
   -I$(top_srcdir)/src/behavior \
   -I$(top_srcdir)/src/graph \
   -I$(top_srcdir)/src/tree \
   -I$(top_srcdir)/src/utility \
   -I$(BOOST_DIR) $(all_includes)

lib_loops_detection_la_SOURCES = loops.cpp loop.cpp

#DOCUMENTATION STUFF

DOX_TAGFILES= 

PRJ_DOC =  
EXTRA_DIST = $(PRJ_DOC)

#do not touch the following line

