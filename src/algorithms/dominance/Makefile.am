#this file is src/algorithms/dominance/Makefile.am
METASOURCES = AUTO

noinst_LTLIBRARIES =  lib_dominance.la

noinst_HEADERS = Dominance.hpp weak_dominance.hpp

lib_dominance_la_CPPFLAGS = \
   -I$(top_srcdir)/src/ \
   -I$(top_srcdir)/src/graph \
   -I$(top_srcdir)/src/utility \
   -I$(BOOST_DIR) $(all_includes)

lib_dominance_la_SOURCES = weak_dominance.cpp

#DOCUMENTATION STUFF

DOX_TAGFILES= 

PRJ_DOC = Dominance.doc 
EXTRA_DIST = $(PRJ_DOC)

#do not touch the following line
include $(top_srcdir)/panda.am
