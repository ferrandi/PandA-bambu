#this file is src/wrapper/synthesis/altera/Makefile.am
METASOURCES = AUTO

noinst_LTLIBRARIES = lib_Alterawrapper.la

lib_Alterawrapper_la_CPPFLAGS = \
    -I$(top_srcdir)/src \
    -I$(top_srcdir)/src/circuit \
    -I$(top_srcdir)/src/constants \
    -I$(top_srcdir)/src/design_flows/backend/ToHDL \
    -I$(top_srcdir)/src/graph \
    -I$(top_srcdir)/src/HLS/scheduling \
    -I$(top_srcdir)/src/parser/polixml \
    -I$(top_srcdir)/src/polixml \
    -I$(top_srcdir)/src/technology \
    -I$(top_srcdir)/src/technology/target_device \
    -I$(top_srcdir)/src/technology/physical_library \
    -I$(top_srcdir)/src/technology/physical_library/models \
    -I$(top_srcdir)/src/technology/physical_library/models/time \
    -I$(top_srcdir)/src/technology/physical_library/models/area \
    -I$(top_srcdir)/src/tree \
    -I$(top_srcdir)/src/utility \
    -I$(top_srcdir)/src/wrapper \
    -I$(top_srcdir)/src/wrapper/simulation \
    -I$(top_srcdir)/src/wrapper/simulation/modelsim \
    -I$(top_srcdir)/src/wrapper/synthesis \
    -I$(BOOST_DIR) $(all_includes)

lib_Alterawrapper_la_LDFLAGS = $(BOOST_FILESYSTEM_LIB)

AlteraBackendFlow.cpp : CycloneII.data CycloneII-R.data CycloneV.data StratixIV.data StratixV.data
#The tab in this rule can't be replaced by blanks
CycloneII.data: $(top_srcdir)/etc/devices/Altera_devices/CycloneII.xml
	$(top_srcdir)/etc/scripts/add_escape.sh $(top_srcdir)/etc/devices/Altera_devices/CycloneII.xml CycloneII.data

CycloneII-R.data: $(top_srcdir)/etc/devices/Altera_devices/CycloneII-R.xml
	$(top_srcdir)/etc/scripts/add_escape.sh $(top_srcdir)/etc/devices/Altera_devices/CycloneII-R.xml CycloneII-R.data

CycloneV.data: $(top_srcdir)/etc/devices/Altera_devices/CycloneV.xml
	$(top_srcdir)/etc/scripts/add_escape.sh $(top_srcdir)/etc/devices/Altera_devices/CycloneV.xml CycloneV.data

StratixIV.data: $(top_srcdir)/etc/devices/Altera_devices/StratixIV.xml
	$(top_srcdir)/etc/scripts/add_escape.sh $(top_srcdir)/etc/devices/Altera_devices/StratixIV.xml StratixIV.data

StratixV.data: $(top_srcdir)/etc/devices/Altera_devices/StratixV.xml
	$(top_srcdir)/etc/scripts/add_escape.sh $(top_srcdir)/etc/devices/Altera_devices/StratixV.xml StratixV.data

noinst_HEADERS = \
   AlteraBackendFlow.hpp \
   AlteraWrapper.hpp \
   quartus_13_report_wrapper.hpp \
   quartus_13_wrapper.hpp \
   quartus_report_wrapper.hpp \
   quartus_wrapper.hpp

lib_Alterawrapper_la_SOURCES = \
   AlteraBackendFlow.cpp \
   AlteraWrapper.cpp \
   quartus_13_report_wrapper.cpp \
   quartus_13_wrapper.cpp \
   quartus_report_wrapper.cpp \
   quartus_wrapper.cpp

#do not touch the following line
include $(top_srcdir)/panda.am
