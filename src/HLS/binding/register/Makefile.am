#this file is src/HLS/binding/register/Makefile.am
SUBDIRS = algorithms

noinst_LTLIBRARIES =  lib_registerbinding.la

lib_registerbinding_la_CPPFLAGS = \
    -I$(top_srcdir)/src/ \
    -I$(top_srcdir)/src/algorithms/clique_covering \
    -I$(top_srcdir)/src/circuit \
    -I$(top_srcdir)/src/behavior \
    -I$(top_srcdir)/src/design_flows\
    -I$(top_srcdir)/src/graph \
    -I$(top_srcdir)/src/HLS \
    -I$(top_srcdir)/src/HLS/virtual_components \
    -I$(top_srcdir)/src/HLS/binding \
    -I$(top_srcdir)/src/HLS/binding/interconnection \
    -I$(top_srcdir)/src/HLS/binding/module \
    -I$(top_srcdir)/src/HLS/binding/register/algorithms \
    -I$(top_srcdir)/src/HLS/binding/storage_value_insertion \
    -I$(top_srcdir)/src/HLS/liveness \
    -I$(top_srcdir)/src/polixml \
    -I$(top_srcdir)/src/technology/target_device \
    -I$(top_srcdir)/src/technology/target_device/FPGA \
    -I$(top_srcdir)/src/technology \
    -I$(top_srcdir)/src/technology/physical_library \
    -I$(top_srcdir)/src/tree \
    -I$(top_srcdir)/src/utility \
    -I$(BOOST_DIR) $(all_includes)

METASOURCES = AUTO

noinst_HEADERS = reg_binding.hpp reg_binding_creator.hpp Variable.hpp

lib_registerbinding_la_SOURCES = reg_binding.cpp reg_binding_creator.cpp

lib_registerbinding_la_LIBADD = \
      $(top_builddir)/src/HLS/binding/register/algorithms/lib_register_algorithms.la

#DOCUMENTATION STUFF


DOX_TAGFILES= src src/HLS/binding/register/algorithms src/HLS/controller

PRJ_DOC = reg_binding_creator.doc
EXTRA_DIST = $(PRJ_DOC)
if BUILD_LIB_EXPERIMENTAL
   PRJ_DOC += \
      register_grouping.doc
endif


#do not touch the following line
include $(top_srcdir)/panda.am
