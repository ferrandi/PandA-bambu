#this file is src/wrapper/synthesis/lattice/Makefile.am
METASOURCES = AUTO

noinst_LTLIBRARIES = lib_Latticewrapper.la

lib_Latticewrapper_la_CPPFLAGS = \
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

lib_Latticewrapper_la_LDFLAGS = $(BOOST_FILESYSTEM_LIB)

LatticeBackendFlow.cpp : LatticeECP3.data
#The tab in this rule can't be replaced by blanks
LatticeECP3.data: $(top_srcdir)/etc/devices/Lattice_devices/LatticeECP3.xml
	$(top_srcdir)/etc/scripts/add_escape.sh $(top_srcdir)/etc/devices/Lattice_devices/LatticeECP3.xml LatticeECP3.data

noinst_HEADERS = LatticeBackendFlow.hpp LatticeWrapper.hpp lattice_flow_wrapper.hpp 

lib_Latticewrapper_la_SOURCES = LatticeBackendFlow.cpp LatticeWrapper.cpp lattice_flow_wrapper.cpp 

#do not touch the following line

