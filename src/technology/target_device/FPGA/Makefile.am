#this file is src/technology/target_device/FPGA/Makefile.am
METASOURCES = AUTO
noinst_LTLIBRARIES = lib_FPGA_device.la

lib_FPGA_device_la_CPPFLAGS = \
   -I$(top_srcdir)/src \
   -I$(top_srcdir)/src/circuit \
   -I$(top_srcdir)/src/constants \
   -I$(top_srcdir)/src/graph \
   -I$(top_srcdir)/src/parser/polixml \
   -I$(top_srcdir)/src/polixml \
   -I$(top_srcdir)/src/technology \
   -I$(top_srcdir)/src/technology/physical_library \
   -I$(top_srcdir)/src/technology/target_technology \
   -I$(top_srcdir)/src/technology/target_device \
   -I$(top_srcdir)/src/utility \
   -I$(BOOST_DIR)


noinst_HEADERS = FPGA_device.hpp
lib_FPGA_device_la_SOURCES = FPGA_device.cpp

BUILT_SOURCES=xc4vlx100-10ff1513.data xc5vlx110t-1ff1136.data xc5vlx330t-2ff1738.data xc5vlx50-3ff1153.data xc6vlx240t-1ff1156.data xc7vx330t-1ffg1157.data xc7z020-1clg484.data \
EP2C70F896C6.data EP2C70F896C6-R.data \
EP4SGX530KH40C2.data \
5CSEMA5F31C6.data 5SGXEA7N2F45C1.data\
xc7z020-1clg484-VVD.data xc7vx485t-2ffg1761-VVD.data xc7vx690t-3ffg1930-VVD.data\
xc7a100t-1csg324-VVD.data\
xc7z020-1clg484-YOSYS-VVD.data \
LFE335EA8FN484C.data

FPGA_device.cpp : xc4vlx100-10ff1513.data xc5vlx110t-1ff1136.data xc5vlx330t-2ff1738.data xc5vlx50-3ff1153.data xc6vlx240t-1ff1156.data xc7vx330t-1ffg1157.data xc7z020-1clg484.data \
EP2C70F896C6.data EP2C70F896C6-R.data \
EP4SGX530KH40C2.data \
5CSEMA5F31C6.data 5SGXEA7N2F45C1.data\
xc7z020-1clg484-VVD.data xc7vx485t-2ffg1761-VVD.data xc7vx690t-3ffg1930-VVD.data\
xc7a100t-1csg324-VVD.data\
xc7z020-1clg484-YOSYS-VVD.data \
LFE335EA8FN484C.data

#The tab in this rule can't be replaced by blanks

xc4vlx100-10ff1513.data: $(top_srcdir)/etc/devices/Xilinx_devices/xc4vlx100-10ff1513.xml
	$(top_srcdir)/etc/scripts/add_escape.sh $(top_srcdir)/etc/devices/Xilinx_devices/xc4vlx100-10ff1513.xml xc4vlx100-10ff1513.data

xc5vlx50-3ff1153.data: $(top_srcdir)/etc/devices/Xilinx_devices/xc5vlx50-3ff1153.xml
	$(top_srcdir)/etc/scripts/add_escape.sh $(top_srcdir)/etc/devices/Xilinx_devices/xc5vlx50-3ff1153.xml xc5vlx50-3ff1153.data

xc5vlx330t-2ff1738.data: $(top_srcdir)/etc/devices/Xilinx_devices/xc5vlx330t-2ff1738.xml
	$(top_srcdir)/etc/scripts/add_escape.sh $(top_srcdir)/etc/devices/Xilinx_devices/xc5vlx330t-2ff1738.xml xc5vlx330t-2ff1738.data

xc5vlx110t-1ff1136.data: $(top_srcdir)/etc/devices/Xilinx_devices/xc5vlx110t-1ff1136.xml
	$(top_srcdir)/etc/scripts/add_escape.sh $(top_srcdir)/etc/devices/Xilinx_devices/xc5vlx110t-1ff1136.xml xc5vlx110t-1ff1136.data

xc6vlx240t-1ff1156.data: $(top_srcdir)/etc/devices/Xilinx_devices/xc6vlx240t-1ff1156.xml
	$(top_srcdir)/etc/scripts/add_escape.sh $(top_srcdir)/etc/devices/Xilinx_devices/xc6vlx240t-1ff1156.xml xc6vlx240t-1ff1156.data

xc7vx330t-1ffg1157.data: $(top_srcdir)/etc/devices/Xilinx_devices/xc7vx330t-1ffg1157.xml
	$(top_srcdir)/etc/scripts/add_escape.sh $(top_srcdir)/etc/devices/Xilinx_devices/xc7vx330t-1ffg1157.xml xc7vx330t-1ffg1157.data

xc7z020-1clg484.data: $(top_srcdir)/etc/devices/Xilinx_devices/xc7z020-1clg484.xml
	$(top_srcdir)/etc/scripts/add_escape.sh $(top_srcdir)/etc/devices/Xilinx_devices/xc7z020-1clg484.xml xc7z020-1clg484.data

xc7z020-1clg484-VVD.data: $(top_srcdir)/etc/devices/Xilinx_devices/xc7z020-1clg484-VVD.xml
	$(top_srcdir)/etc/scripts/add_escape.sh $(top_srcdir)/etc/devices/Xilinx_devices/xc7z020-1clg484-VVD.xml xc7z020-1clg484-VVD.data

xc7z020-1clg484-YOSYS-VVD.data: $(top_srcdir)/etc/devices/Xilinx_devices/xc7z020-1clg484-YOSYS-VVD.xml
	$(top_srcdir)/etc/scripts/add_escape.sh $(top_srcdir)/etc/devices/Xilinx_devices/xc7z020-1clg484-YOSYS-VVD.xml xc7z020-1clg484-YOSYS-VVD.data

xc7vx485t-2ffg1761-VVD.data: $(top_srcdir)/etc/devices/Xilinx_devices/xc7vx485t-2ffg1761-VVD.xml
	$(top_srcdir)/etc/scripts/add_escape.sh $(top_srcdir)/etc/devices/Xilinx_devices/xc7vx485t-2ffg1761-VVD.xml xc7vx485t-2ffg1761-VVD.data

xc7vx690t-3ffg1930-VVD.data: $(top_srcdir)/etc/devices/Xilinx_devices/xc7vx690t-3ffg1930-VVD.xml
	$(top_srcdir)/etc/scripts/add_escape.sh $(top_srcdir)/etc/devices/Xilinx_devices/xc7vx690t-3ffg1930-VVD.xml xc7vx690t-3ffg1930-VVD.data

xc7a100t-1csg324-VVD.data: $(top_srcdir)/etc/devices/Xilinx_devices/xc7a100t-1csg324-VVD.xml
	$(top_srcdir)/etc/scripts/add_escape.sh $(top_srcdir)/etc/devices/Xilinx_devices/xc7a100t-1csg324-VVD.xml xc7a100t-1csg324-VVD.data

#Spartan-3-xc3s1500l-4fg676.data
#Spartan-3-xc3s1500l-4fg676.data: $(top_srcdir)/etc/devices/Xilinx_devices/Spartan-3-xc3s1500l-4fg676.xml
#	$(top_srcdir)/etc/scripts/add_escape.sh $(top_srcdir)/etc/devices/Xilinx_devices/Spartan-3-xc3s1500l-4fg676.xml Spartan-3-xc3s1500l-4fg676.data

EP2C70F896C6.data: $(top_srcdir)/etc/devices/Altera_devices/EP2C70F896C6.xml
	$(top_srcdir)/etc/scripts/add_escape.sh $(top_srcdir)/etc/devices/Altera_devices/EP2C70F896C6.xml EP2C70F896C6.data

EP2C70F896C6-R.data: $(top_srcdir)/etc/devices/Altera_devices/EP2C70F896C6-R.xml
	$(top_srcdir)/etc/scripts/add_escape.sh $(top_srcdir)/etc/devices/Altera_devices/EP2C70F896C6-R.xml EP2C70F896C6-R.data

5CSEMA5F31C6.data: $(top_srcdir)/etc/devices/Altera_devices/5CSEMA5F31C6.xml
	$(top_srcdir)/etc/scripts/add_escape.sh $(top_srcdir)/etc/devices/Altera_devices/5CSEMA5F31C6.xml 5CSEMA5F31C6.data

5SGXEA7N2F45C1.data: $(top_srcdir)/etc/devices/Altera_devices/5SGXEA7N2F45C1.xml
	$(top_srcdir)/etc/scripts/add_escape.sh $(top_srcdir)/etc/devices/Altera_devices/5SGXEA7N2F45C1.xml 5SGXEA7N2F45C1.data

EP4SGX530KH40C2.data: $(top_srcdir)/etc/devices/Altera_devices/EP4SGX530KH40C2.xml
	$(top_srcdir)/etc/scripts/add_escape.sh $(top_srcdir)/etc/devices/Altera_devices/EP4SGX530KH40C2.xml EP4SGX530KH40C2.data

LFE335EA8FN484C.data: $(top_srcdir)/etc/devices/Lattice_devices/LFE335EA8FN484C.xml
	$(top_srcdir)/etc/scripts/add_escape.sh $(top_srcdir)/etc/devices/Lattice_devices/LFE335EA8FN484C.xml LFE335EA8FN484C.data


#do not touch the following line
include $(top_srcdir)/panda.am

