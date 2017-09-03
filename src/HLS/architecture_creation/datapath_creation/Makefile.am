#this is file src/HLS/architecture_creation/datapath_creation/Makefile.am
METASOURCES = AUTO

noinst_LTLIBRARIES =  lib_datapath_creation.la

lib_datapath_creation_la_CPPFLAGS =  \
   -I$(top_srcdir)/src \
   -I$(top_srcdir)/src/behavior \
   -I$(top_srcdir)/src/circuit \
   -I$(top_srcdir)/src/constants \
   -I$(top_srcdir)/src/design_flows \
   -I$(top_srcdir)/src/graph \
   -I$(top_srcdir)/src/HLS \
   -I$(top_srcdir)/src/HLS/virtual_components \
   -I$(top_srcdir)/src/HLS/binding \
   -I$(top_srcdir)/src/HLS/binding/interconnection \
   -I$(top_srcdir)/src/HLS/binding/module \
   -I$(top_srcdir)/src/HLS/binding/register \
   -I$(top_srcdir)/src/HLS/memory \
   -I$(top_srcdir)/src/HLS/scheduling \
   -I$(top_srcdir)/src/HLS/stg \
   -I$(top_srcdir)/src/polixml \
   -I$(top_srcdir)/src/technology \
   -I$(top_srcdir)/src/technology/physical_library \
   -I$(top_srcdir)/src/technology/target_device \
   -I$(top_srcdir)/src/tree \
   -I$(top_srcdir)/src/utility \
   -I$(BOOST_DIR) $(all_includes)

noinst_HEADERS = datapath_creator.hpp classic_datapath.hpp

lib_datapath_creation_la_SOURCES = datapath_creator.cpp classic_datapath.cpp

#DOCUMENTATION STUFF
DOX_TAGFILES=
PRJ_DOC = datapath.doc
EXTRA_DIST = $(PRJ_DOC)

#do not touch the following line

