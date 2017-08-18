#this is file src/HLS/architecture/Makefile.am
METASOURCES = AUTO

SUBDIRS = controller_creation datapath_creation

noinst_LTLIBRARIES = lib_architecture_creation.la

noinst_HEADERS = top_entity.hpp TopEntityMemoryMapped.hpp

lib_architecture_creation_la_SOURCES = top_entity.cpp TopEntityMemoryMapped.cpp

lib_architecture_creation_la_CPPFLAGS = \
   -I$(top_srcdir)/src \
   -I$(top_srcdir)/src/behavior \
   -I$(top_srcdir)/src/circuit \
   -I$(top_srcdir)/src/constants \
   -I$(top_srcdir)/src/design_flows\
   -I$(top_srcdir)/src/graph \
   -I$(top_srcdir)/src/HLS \
   -I$(top_srcdir)/src/HLS/architecture_creation/controller_creation \
   -I$(top_srcdir)/src/HLS/architecture_creation/datapath_creation \
   -I$(top_srcdir)/src/HLS/virtual_components \
   -I$(top_srcdir)/src/HLS/binding \
   -I$(top_srcdir)/src/HLS/binding/interconnection \
   -I$(top_srcdir)/src/HLS/binding/module \
   -I$(top_srcdir)/src/HLS/binding/register \
   -I$(top_srcdir)/src/HLS/interface \
   -I$(top_srcdir)/src/HLS/function_allocation \
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

lib_architecture_creation_la_LIBADD = \
                        $(top_builddir)/src/HLS/architecture_creation/datapath_creation/lib_datapath_creation.la \
                        $(top_builddir)/src/HLS/architecture_creation/controller_creation/lib_controller_creation.la

#do not touch the following line
include $(top_srcdir)/panda.am
