METASOURCES = AUTO
SUBDIRS = writer
noinst_LTLIBRARIES =  lib_ToHDL.la

lib_ToHDL_la_CPPFLAGS = -I$(top_srcdir)/src \
                        -I$(top_srcdir)/src/behavior \
                        -I$(top_srcdir)/src/circuit \
                        -I$(top_srcdir)/src/constants \
                        -I$(top_srcdir)/src/design_flows/ \
                        -I$(top_srcdir)/src/design_flows/backend/ToHDL/writer \
                        -I$(top_srcdir)/src/graph \
                        -I$(top_srcdir)/src/HLS \
                        -I$(top_srcdir)/src/technology \
                        -I$(top_srcdir)/src/technology/physical_library \
                        -I$(top_srcdir)/src/technology/target_device \
                        -I$(top_srcdir)/src/technology/target_device/FPGA \
                        -I$(top_srcdir)/src/utility \
                        -I$(BOOST_DIR) $(all_includes)
if BUILD_FLOPOCO
lib_ToHDL_la_CPPFLAGS += -I$(top_srcdir)/src/wrapper/flopoco
endif

   
lib_ToHDL_la_LIBADD = \
   $(top_builddir)/src/design_flows/backend/ToHDL/writer/lib_HDL_writers.la

noinst_HEADERS = HDL_manager.hpp language_writer.hpp

lib_ToHDL_la_SOURCES = HDL_manager.cpp language_writer.cpp

#DOCUMENTATION STUFF

DOX_TAGFILES= src/design_flows/backend/ src/circuit

PRJ_DOC = ToHDL.doc
EXTRA_DIST = $(PRJ_DOC)

#do not touch the following line
include $(top_srcdir)/panda.am
