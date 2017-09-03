#this file is src/wrapper/synthesis/xilinx/Makefile.am
METASOURCES = AUTO
SUBDIRS = ise
DIST_SUBDIRS = ise

noinst_LTLIBRARIES = lib_Xilinxwrapper.la

noinst_HEADERS = XilinxBackendFlow.hpp XilinxWrapper.hpp vivado_flow_wrapper.hpp
lib_Xilinxwrapper_la_SOURCES = XilinxBackendFlow.cpp XilinxWrapper.cpp vivado_flow_wrapper.cpp

lib_Xilinxwrapper_la_CPPFLAGS = \
   -I$(top_srcdir)/src  \
   -I$(top_srcdir)/src/circuit  \
   -I$(top_srcdir)/src/constants  \
   -I$(top_srcdir)/src/graph\
   -I$(top_srcdir)/src/HLS/scheduling \
   -I$(top_srcdir)/src/parser/polixml  \
   -I$(top_srcdir)/src/polixml  \
   -I$(top_srcdir)/src/utility  \
   -I$(top_srcdir)/src/technology \
   -I$(top_srcdir)/src/technology/target_device \
   -I$(top_srcdir)/src/technology/physical_library \
   -I$(top_srcdir)/src/technology/physical_library/models \
   -I$(top_srcdir)/src/technology/physical_library/models/area \
   -I$(top_srcdir)/src/technology/physical_library/models/time \
   -I$(top_srcdir)/src/wrapper \
   -I$(top_srcdir)/src/wrapper/synthesis \
   -I$(top_srcdir)/src/wrapper/synthesis/xilinx/ise \
   -I$(BOOST_DIR) $(all_includes)

if BUILD_TASTE
   noinst_HEADERS += \
      xilinx_taste_backend_flow.hpp
   lib_Xilinxwrapper_la_SOURCES += \
      xilinx_taste_backend_flow.cpp
endif

lib_Xilinxwrapper_la_LIBADD = $(top_builddir)/src/wrapper/synthesis/xilinx/ise/lib_xilinx_ise.la 

XilinxWrapper.cpp : Virtex-4.data Virtex-5.data Virtex-6.data Virtex-7.data

if BUILD_TASTE
XilinxBackendFlow.cpp : Virtex-4.data Virtex-4-Taste.data Virtex-5.data Virtex-6.data Virtex-7.data Virtex-7-VVD.data Artix-7-VVD.data Zynq.data Zynq-VVD.data Zynq-YOSYS-VVD.data
else
XilinxBackendFlow.cpp : Virtex-4.data Virtex-5.data Virtex-6.data Virtex-7.data Virtex-7-VVD.data Artix-7-VVD.data Zynq.data Zynq-VVD.data Zynq-YOSYS-VVD.data
endif

#The tab in this rule can't be replaced by blanks
Virtex-4.data: $(top_srcdir)/etc/devices/Xilinx_devices/Virtex-4.xml
	$(top_srcdir)/etc/scripts/add_escape.sh $(top_srcdir)/etc/devices/Xilinx_devices/Virtex-4.xml Virtex-4.data

if BUILD_TASTE
Virtex-4-Taste.data: $(top_srcdir)/etc/devices/Xilinx_devices/Virtex-4-Taste.xml
	$(top_srcdir)/etc/scripts/add_escape.sh $(top_srcdir)/etc/devices/Xilinx_devices/Virtex-4-Taste.xml Virtex-4-Taste.data
endif

Virtex-5.data: $(top_srcdir)/etc/devices/Xilinx_devices/Virtex-5.xml
	$(top_srcdir)/etc/scripts/add_escape.sh $(top_srcdir)/etc/devices/Xilinx_devices/Virtex-5.xml Virtex-5.data

Virtex-6.data: $(top_srcdir)/etc/devices/Xilinx_devices/Virtex-6.xml
	$(top_srcdir)/etc/scripts/add_escape.sh $(top_srcdir)/etc/devices/Xilinx_devices/Virtex-6.xml Virtex-6.data

Virtex-7.data: $(top_srcdir)/etc/devices/Xilinx_devices/Virtex-7.xml
	$(top_srcdir)/etc/scripts/add_escape.sh $(top_srcdir)/etc/devices/Xilinx_devices/Virtex-7.xml Virtex-7.data

Virtex-7-VVD.data: $(top_srcdir)/etc/devices/Xilinx_devices/Virtex-7-VVD.xml
	$(top_srcdir)/etc/scripts/add_escape.sh $(top_srcdir)/etc/devices/Xilinx_devices/Virtex-7-VVD.xml Virtex-7-VVD.data

Artix-7-VVD.data: $(top_srcdir)/etc/devices/Xilinx_devices/Artix-7-VVD.xml
	$(top_srcdir)/etc/scripts/add_escape.sh $(top_srcdir)/etc/devices/Xilinx_devices/Artix-7-VVD.xml Artix-7-VVD.data

Zynq.data: $(top_srcdir)/etc/devices/Xilinx_devices/Zynq.xml
	$(top_srcdir)/etc/scripts/add_escape.sh $(top_srcdir)/etc/devices/Xilinx_devices/Zynq.xml Zynq.data

Zynq-VVD.data: $(top_srcdir)/etc/devices/Xilinx_devices/Zynq-VVD.xml
	$(top_srcdir)/etc/scripts/add_escape.sh $(top_srcdir)/etc/devices/Xilinx_devices/Zynq-VVD.xml Zynq-VVD.data

Zynq-YOSYS-VVD.data: $(top_srcdir)/etc/devices/Xilinx_devices/Zynq-YOSYS-VVD.xml
	$(top_srcdir)/etc/scripts/add_escape.sh $(top_srcdir)/etc/devices/Xilinx_devices/Zynq-YOSYS-VVD.xml Zynq-YOSYS-VVD.data

if BUILD_LIB_EXPERIMENTAL
  SUBDIRS += edk
  DIST_SUBDIRS += edk
  lib_Xilinxwrapper_la_LIBADD += $(top_builddir)/src/wrapper/synthesis/xilinx/edk/lib_xilinx_edk.la
endif

#do not touch the following line

