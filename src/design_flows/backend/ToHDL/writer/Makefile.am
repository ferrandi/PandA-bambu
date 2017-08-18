noinst_LTLIBRARIES =  lib_HDL_writers.la

lib_HDL_writers_la_CPPFLAGS = \
   -I$(top_srcdir)/src \
   -I$(top_srcdir)/src/behavior \
   -I$(top_srcdir)/src/circuit \
   -I$(top_srcdir)/src/design_flows \
   -I$(top_srcdir)/src/design_flows/backend/ToHDL \
   -I$(top_srcdir)/src/graph \
   -I$(top_srcdir)/src/HLS/scheduling\
   -I$(top_srcdir)/src/technology \
   -I$(top_srcdir)/src/technology/physical_library \
   -I$(top_srcdir)/src/technology/physical_library/models \
   -I$(top_srcdir)/src/technology/target_device \
   -I$(top_srcdir)/src/tree \
   -I$(top_srcdir)/src/utility \
   -I$(top_srcdir)/src/HLS/stg \
   -I$(BOOST_DIR) \
   $(all_includes)
METASOURCES = AUTO

noinst_HEADERS = verilog_writer.hpp sv_writer.hpp VHDL_writer.hpp
lib_HDL_writers_la_SOURCES = verilog_writer.cpp sv_writer.cpp VHDL_writer.cpp

if BUILD_LIB_EXPERIMENTAL
   noinst_HEADERS += blif_writer.hpp SystemC_writer.hpp edif_writer.hpp
   lib_HDL_writers_la_SOURCES += blif_writer.cpp SystemC_writer.cpp edif_writer.cpp
endif

#DOCUMENTATION STUFF
DOX_TAGFILES= src/design_flows/backend/ src/circuit

#do not touch the following line
include $(top_srcdir)/panda.am
