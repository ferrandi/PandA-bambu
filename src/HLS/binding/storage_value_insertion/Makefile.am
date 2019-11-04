#this file is src/HLS/binding/storage_value_insertion/Makefile.am
noinst_LTLIBRARIES =  lib_storage_value_insertion.la

lib_storage_value_insertion_la_CPPFLAGS = \
   -I$(top_srcdir)/src/ \
   -I$(top_srcdir)/src/behavior \
   -I$(top_srcdir)/src/design_flows \
   -I$(top_srcdir)/src/graph \
   -I$(top_srcdir)/src/HLS \
   -I$(top_srcdir)/src/HLS/binding/module \
   -I$(top_srcdir)/src/HLS/liveness \
   -I$(top_srcdir)/src/HLS/stg \
   -I$(top_srcdir)/src/polixml \
   -I$(top_srcdir)/src/tree \
   -I$(top_srcdir)/src/utility \
   -I$(BOOST_DIR) $(all_includes)

METASOURCES = AUTO

noinst_HEADERS = storage_value_information.hpp storage_value_information_pipeline.hpp storage_value_insertion.hpp values_scheme.hpp 

lib_storage_value_insertion_la_SOURCES = storage_value_information.cpp storage_value_information_pipeline.cpp storage_value_insertion.cpp values_scheme.cpp 

#DOCUMENTATION STUFF

DOX_TAGFILES= src src/HLS/controller

PRJ_DOC =
EXTRA_DIST = $(PRJ_DOC)

#do not touch the following line

