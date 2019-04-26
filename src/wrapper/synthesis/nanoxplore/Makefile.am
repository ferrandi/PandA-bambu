#this file is src/wrapper/synthesis/NanoXplore/Makefile.am
METASOURCES = AUTO

noinst_LTLIBRARIES = lib_NanoXplorewrapper.la

lib_NanoXplorewrapper_la_CPPFLAGS = \
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

lib_NanoXplorewrapper_la_LDFLAGS = $(BOOST_FILESYSTEM_LIB)

NanoXploreBackendFlow.cpp : NG-medium.data
#The tab in this rule can't be replaced by blanks
NG-medium.data: $(top_srcdir)/etc/devices/NanoXplore_devices/NG-medium.xml
	$(top_srcdir)/etc/scripts/add_escape.sh $(top_srcdir)/etc/devices/NanoXplore_devices/NG-medium.xml NG-medium.data


noinst_HEADERS = \
   NanoXploreBackendFlow.hpp \
   NanoXploreWrapper.hpp \
   nxpython_flow_wrapper.hpp

lib_NanoXplorewrapper_la_SOURCES = \
   NanoXploreBackendFlow.cpp \
   NanoXploreWrapper.cpp \
   nxpython_flow_wrapper.cpp

#do not touch the following line

