#this file is src/technology/target_device/IC/Makefile.am
METASOURCES = AUTO
noinst_LTLIBRARIES = lib_IC_device.la

lib_IC_device_la_CPPFLAGS = \
   -I$(top_srcdir)/src \
   -I$(top_srcdir)/src/circuit \
   -I$(top_srcdir)/src/constants \
   -I$(top_srcdir)/src/graph \
   -I$(top_srcdir)/src/polixml \
   -I$(top_srcdir)/src/parser/polixml \
   -I$(top_srcdir)/src/technology \
   -I$(top_srcdir)/src/technology/physical_library \
   -I$(top_srcdir)/src/technology/target_device \
   -I$(top_srcdir)/src/technology/target_technology \
   -I$(top_srcdir)/src/technology/target_technology/CMOS \
   -I$(top_srcdir)/src/utility \
   -I$(top_srcdir)/src/wrapper/synthesis \
   -I$(BOOST_DIR) $(all_includes)


noinst_HEADERS = IC_device.hpp
lib_IC_device_la_SOURCES = IC_device.cpp

IC_device.cpp : Nangate.data
#The tab in this rule can't be replaced by blanks
Nangate.data: $(top_srcdir)/etc/devices/ASIC_devices/Nangate_device.xml
	$(top_srcdir)/etc/scripts/add_escape.sh $(top_srcdir)/etc/devices/ASIC_devices/Nangate_device.xml Nangate.data

#do not touch the following line


