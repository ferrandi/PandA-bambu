#this file is src/technology/target_technology/Makefile.am
SUBDIRS = FPGA
DIST_SUBDIRS = FPGA

METASOURCES = AUTO
noinst_LTLIBRARIES = lib_target_technology.la

lib_target_technology_la_CPPFLAGS = \
   -I$(top_srcdir)/src \
   -I$(top_srcdir)/src/behavior \
   -I$(top_srcdir)/src/circuit \
   -I$(top_srcdir)/src/constants \
   -I$(top_srcdir)/src/graph \
   -I$(top_srcdir)/src/parser/polixml \
   -I$(top_srcdir)/src/polixml \
   -I$(top_srcdir)/src/technology \
   -I$(top_srcdir)/src/technology/target_technology/FPGA \
   -I$(top_srcdir)/src/utility \
   -I$(BOOST_DIR) $(all_includes)

noinst_HEADERS = target_technology.hpp
lib_target_technology_la_SOURCES = target_technology.cpp

lib_target_technology_la_LIBADD = \
                $(top_builddir)/src/technology/target_technology/FPGA/lib_target_technology_FPGA.la

if BUILD_LIB_CMOS
  SUBDIRS += CMOS
  DIST_SUBDIRS += CMOS
  lib_target_technology_la_CPPFLAGS += \
                 -I$(top_srcdir)/src/technology/target_technology/CMOS

if BUILD_LIB_EXPERIMENTAL
  lib_target_technology_la_CPPFLAGS += \
                 -I$(top_srcdir)/src/parser/blif \
                 -I$(top_srcdir)/src/circuit/aig
endif

  lib_target_technology_la_LIBADD += \
                $(top_builddir)/src/technology/target_technology/CMOS/lib_target_technology_CMOS.la
endif

#do not touch the following line


