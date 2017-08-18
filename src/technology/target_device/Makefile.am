#this file is src/technology/target_device/Makefile.am
METASOURCES = AUTO
SUBDIRS = FPGA
DIST_SUBDIRS = FPGA

noinst_LTLIBRARIES = lib_target_device.la

lib_target_device_la_CPPFLAGS = \
   -I$(top_srcdir)/src \
   -I$(top_srcdir)/src/constants \
   -I$(top_srcdir)/src/polixml \
   -I$(top_srcdir)/src/parser/polixml \
   -I$(top_srcdir)/src/technology \
   -I$(top_srcdir)/src/technology/physical_library \
   -I$(top_srcdir)/src/technology/target_device/FPGA \
   -I$(top_srcdir)/src/technology/target_technology \
   -I$(top_srcdir)/src/utility \
   -I$(BOOST_DIR) $(all_includes)


noinst_HEADERS = target_device.hpp
lib_target_device_la_SOURCES = target_device.cpp

lib_target_device_la_LIBADD = \
   $(top_builddir)/src/technology/target_device/FPGA/lib_FPGA_device.la

if BUILD_LIB_CMOS
   SUBDIRS += IC
   DIST_SUBDIRS += IC
   lib_target_device_la_CPPFLAGS += \
      -I$(top_srcdir)/src/technology/target_device/IC
   lib_target_device_la_LIBADD += \
      $(top_builddir)/src/technology/target_device/IC/lib_IC_device.la
endif


#do not touch the following line
include $(top_srcdir)/panda.am

