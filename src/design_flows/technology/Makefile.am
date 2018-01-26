#this file is src/design_flows/technology/Makefile.am
SUBDIRS=
DIST_SUBDIRS=
METASOURCES = AUTO
noinst_LTLIBRARIES = lib_technology.la

noinst_HEADERS = \
   fix_characterization.hpp \
   load_builtin_technology.hpp \
   load_default_technology.hpp \
   load_device_technology.hpp \
   load_file_technology.hpp \
   load_technology.hpp \
   technology_flow_step.hpp \
   technology_flow_step_factory.hpp \
   write_technology.hpp

lib_technology_la_SOURCES = \
   fix_characterization.cpp \
   load_default_technology.cpp \
   load_device_technology.cpp \
   load_file_technology.cpp \
   load_technology.cpp \
   technology_flow_step.cpp \
   technology_flow_step_factory.cpp \
   write_technology.cpp

if BUILD_LIB_CIRCUIT
   noinst_HEADERS += \
      functional_unit_step.hpp \
      load_builtin_technology.hpp
   lib_technology_la_SOURCES += \
      functional_unit_step.cpp \
      load_builtin_technology.cpp
endif

lib_technology_la_CPPFLAGS = \
   -I$(top_srcdir)/src \
   -I$(top_srcdir)/src/behavior \
   -I$(top_srcdir)/src/circuit \
   -I$(top_srcdir)/src/constants \
   -I$(top_srcdir)/src/design_flows \
   -I$(top_srcdir)/src/graph \
   -I$(top_srcdir)/src/HLS \
   -I$(top_srcdir)/src/HLS/module_allocation \
   -I$(top_srcdir)/src/HLS/scheduling \
   -I$(top_srcdir)/src/intermediate_representations \
   -I$(top_srcdir)/src/intermediate_representations/hls \
   -I$(top_srcdir)/src/parser/polixml \
   -I$(top_srcdir)/src/polixml \
   -I$(top_srcdir)/src/technology \
   -I$(top_srcdir)/src/technology/physical_library \
   -I$(top_srcdir)/src/technology/physical_library/models \
   -I$(top_srcdir)/src/technology/physical_library/models/area \
   -I$(top_srcdir)/src/technology/target_device \
   -I$(top_srcdir)/src/utility \
   -I$(BOOST_DIR) $(all_includes)
load_default_technology.cpp : C_COMPLEX_IPs.data C_FP_IPs.data C_HLS_IPs.data C_IO_IPs.data C_MEM_IPs.data C_PROFILING_IPs.data CS_COMPONENT.data C_STD_IPs.data C_VEC_IPs.data NC_HLS_IPs.data NC_MEM_IPs.data NC_PC_IPs.data NC_SF_IPs.data NC_STD_IPs.data NC_VEC_IPs.data NC_wishbone_IPs.data

#The tab in this rule can't be replaced by blanks
C_COMPLEX_IPs.data: $(top_srcdir)/etc/lib/technology/C_COMPLEX_IPs.xml
	$(top_srcdir)/etc/scripts/add_escape.sh $(top_srcdir)/etc/lib/technology/C_COMPLEX_IPs.xml C_COMPLEX_IPs.data

CS_COMPONENT.data: $(top_srcdir)/etc/lib/technology/CS_COMPONENT.xml
	$(top_srcdir)/etc/scripts/add_escape.sh $(top_srcdir)/etc/lib/technology/CS_COMPONENT.xml CS_COMPONENT.data

C_FP_IPs.data: $(top_srcdir)/etc/lib/technology/C_FP_IPs.xml
	$(top_srcdir)/etc/scripts/add_escape.sh $(top_srcdir)/etc/lib/technology/C_FP_IPs.xml C_FP_IPs.data

C_HLS_IPs.data: $(top_srcdir)/etc/lib/technology/C_HLS_IPs.xml
	$(top_srcdir)/etc/scripts/add_escape.sh $(top_srcdir)/etc/lib/technology/C_HLS_IPs.xml C_HLS_IPs.data

C_IO_IPs.data: $(top_srcdir)/etc/lib/technology/C_IO_IPs.xml
	$(top_srcdir)/etc/scripts/add_escape.sh $(top_srcdir)/etc/lib/technology/C_IO_IPs.xml C_IO_IPs.data

C_MEM_IPs.data: $(top_srcdir)/etc/lib/technology/C_MEM_IPs.xml
	$(top_srcdir)/etc/scripts/add_escape.sh $(top_srcdir)/etc/lib/technology/C_MEM_IPs.xml C_MEM_IPs.data

C_PROFILING_IPs.data: $(top_srcdir)/etc/lib/technology/C_PROFILING_IPs.xml
	$(top_srcdir)/etc/scripts/add_escape.sh $(top_srcdir)/etc/lib/technology/C_PROFILING_IPs.xml C_PROFILING_IPs.data

C_STD_IPs.data: $(top_srcdir)/etc/lib/technology/C_STD_IPs.xml
	$(top_srcdir)/etc/scripts/add_escape.sh $(top_srcdir)/etc/lib/technology/C_STD_IPs.xml C_STD_IPs.data

C_VEC_IPs.data: $(top_srcdir)/etc/lib/technology/C_VEC_IPs.xml
	$(top_srcdir)/etc/scripts/add_escape.sh $(top_srcdir)/etc/lib/technology/C_VEC_IPs.xml C_VEC_IPs.data

NC_HLS_IPs.data: $(top_srcdir)/etc/lib/technology/NC_HLS_IPs.xml
	$(top_srcdir)/etc/scripts/add_escape.sh $(top_srcdir)/etc/lib/technology/NC_HLS_IPs.xml NC_HLS_IPs.data

NC_MEM_IPs.data: $(top_srcdir)/etc/lib/technology/NC_MEM_IPs.xml
	$(top_srcdir)/etc/scripts/add_escape.sh $(top_srcdir)/etc/lib/technology/NC_MEM_IPs.xml NC_MEM_IPs.data

NC_PC_IPs.data: $(top_srcdir)/etc/lib/technology/NC_PC_IPs.xml
	$(top_srcdir)/etc/scripts/add_escape.sh $(top_srcdir)/etc/lib/technology/NC_PC_IPs.xml NC_PC_IPs.data

NC_SF_IPs.data: $(top_srcdir)/etc/lib/technology/NC_SF_IPs.xml
	$(top_srcdir)/etc/scripts/add_escape.sh $(top_srcdir)/etc/lib/technology/NC_SF_IPs.xml NC_SF_IPs.data

NC_STD_IPs.data: $(top_srcdir)/etc/lib/technology/NC_STD_IPs.xml
	$(top_srcdir)/etc/scripts/add_escape.sh $(top_srcdir)/etc/lib/technology/NC_STD_IPs.xml NC_STD_IPs.data

NC_VEC_IPs.data: $(top_srcdir)/etc/lib/technology/NC_VEC_IPs.xml
	$(top_srcdir)/etc/scripts/add_escape.sh $(top_srcdir)/etc/lib/technology/NC_VEC_IPs.xml NC_VEC_IPs.data

NC_wishbone_IPs.data: $(top_srcdir)/etc/lib/technology/NC_wishbone_IPs.xml
	$(top_srcdir)/etc/scripts/add_escape.sh $(top_srcdir)/etc/lib/technology/NC_wishbone_IPs.xml NC_wishbone_IPs.data


