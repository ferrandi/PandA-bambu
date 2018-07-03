#this file is src/wrapper/synthesis/Makefile.am
METASOURCES = AUTO
SUBDIRS = xilinx altera synopsys lattice
DIST_SUBDIRS = xilinx altera synopsys lattice

noinst_LTLIBRARIES = lib_synthesis.la

lib_synthesis_la_CPPFLAGS = \
                 -I$(top_srcdir)/src \
                 -I$(top_srcdir)/src/design_flows/ \
                 -I$(top_srcdir)/src/design_flows/backend/ToHDL \
                 -I$(top_srcdir)/src/circuit \
                 -I$(top_srcdir)/src/constants \
                 -I$(top_srcdir)/src/graph \
                 -I$(top_srcdir)/src/HLS/scheduling \
                 -I$(top_srcdir)/src/polixml \
                 -I$(top_srcdir)/src/parser/polixml \
                 -I$(top_srcdir)/src/technology/physical_library/models \
                 -I$(top_srcdir)/src/technology/target_device \
                 -I$(top_srcdir)/src/technology/target_device/IC \
                 -I$(top_srcdir)/src/technology/target_device/FPGA \
                 -I$(top_srcdir)/src/wrapper \
                 -I$(top_srcdir)/src/wrapper/simulation \
                 -I$(top_srcdir)/src/wrapper/synthesis \
                 -I$(top_srcdir)/src/wrapper/synthesis/synopsys \
                 -I$(top_srcdir)/src/wrapper/synthesis/synopsys/design_compiler \
                 -I$(top_srcdir)/src/wrapper/synthesis/xilinx \
                 -I$(top_srcdir)/src/wrapper/synthesis/xilinx/ise \
                 -I$(top_srcdir)/src/wrapper/synthesis/altera \
                 -I$(top_srcdir)/src/wrapper/synthesis/lattice \
                 -I$(top_srcdir)/src/technology \
                 -I$(top_srcdir)/src/technology/physical_library \
                 -I$(top_srcdir)/src/utility \
                 -I$(BOOST_DIR) $(all_includes)

noinst_HEADERS = xml_script_command.hpp \
                 DesignParameters.hpp \
                 BackendFlow.hpp \
                 SynthesisTool.hpp \
                 ASICBackendFlow.hpp

lib_synthesis_la_SOURCES = xml_script_command.cpp \
                           DesignParameters.cpp \
                           BackendFlow.cpp \
                           SynthesisTool.cpp \
                           ASICBackendFlow.cpp

ASICBackendFlow.cpp : Nangate.data
#The tab in this rule can't be replaced by blanks
Nangate.data: $(top_srcdir)/etc/devices/ASIC_devices/Nangate.xml
	$(top_srcdir)/etc/scripts/add_escape.sh $(top_srcdir)/etc/devices/ASIC_devices/Nangate.xml Nangate.data


lib_synthesis_la_LIBADD = \
                        $(top_builddir)/src/wrapper/synthesis/xilinx/lib_Xilinxwrapper.la \
                        $(top_builddir)/src/wrapper/synthesis/altera/lib_Alterawrapper.la \
                        $(top_builddir)/src/wrapper/synthesis/lattice/lib_Latticewrapper.la \
                        $(top_builddir)/src/wrapper/synthesis/synopsys/lib_Synopsyswrapper.la


if BUILD_ABC
  SUBDIRS += abc
  DIST_SUBDIRS += abc

  lib_synthesis_la_LIBADD += \
                             $(top_builddir)/src/wrapper/synthesis/abc/lib_abc.la
endif

#do not touch the following line

