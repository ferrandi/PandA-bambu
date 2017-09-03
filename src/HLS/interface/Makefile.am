#this is file src/HLS/interface/Makefile.am
AUTOMAKE_OPTIONS = subdir-objects

METASOURCES = AUTO

noinst_LTLIBRARIES = lib_interface.la

lib_interface_la_CPPFLAGS = \
    -I$(top_srcdir)/src \
    -I$(top_srcdir)/src/HLS \
    -I$(top_srcdir)/src/HLS/architecture_creation/controller_creation \
    -I$(top_srcdir)/src/HLS/architecture_creation/datapath_creation \
    -I$(top_srcdir)/src/constants \
    -I$(top_srcdir)/src/HLS/binding \
    -I$(top_srcdir)/src/HLS/binding/interconnection \
    -I$(top_srcdir)/src/HLS/binding/module \
    -I$(top_srcdir)/src/HLS/binding/register \
    -I$(top_srcdir)/src/HLS/interface \
    -I$(top_srcdir)/src/HLS/interface/WB4 \
    -I$(top_srcdir)/src/HLS/interface/minimal \
    -I$(top_srcdir)/src/HLS/memory \
    -I$(top_srcdir)/src/HLS/module_allocation \
    -I$(top_srcdir)/src/HLS/scheduling \
    -I$(top_srcdir)/src/HLS/simulation \
    -I$(top_srcdir)/src/HLS/stg \
    -I$(top_srcdir)/src/HLS/virtual_components \
    -I$(top_srcdir)/src/behavior \
    -I$(top_srcdir)/src/circuit \
    -I$(top_srcdir)/src/design_flows \
    -I$(top_srcdir)/src/design_flows/backend/ToHDL \
    -I$(top_srcdir)/src/graph \
    -I$(top_srcdir)/src/polixml \
    -I$(top_srcdir)/src/technology \
    -I$(top_srcdir)/src/technology/physical_library \
    -I$(top_srcdir)/src/technology/target_device \
    -I$(top_srcdir)/src/tree \
    -I$(top_srcdir)/src/utility \
    -I$(BOOST_DIR) $(all_includes)


noinst_HEADERS = module_interface.hpp \
                 minimal/minimal_interface.hpp \
                 WB4/WB4_interface.hpp \
                 WB4/WB4Intercon_interface.hpp

lib_interface_la_SOURCES = module_interface.cpp \
                           minimal/minimal_interface.cpp \
                           WB4/WB4_interface.cpp \
                           WB4/WB4Intercon_interface.cpp

if BUILD_LIB_EXPERIMENTAL
lib_interface_la_CPPFLAGS += \
   -I$(top_srcdir)/src/HLS/interface/AXI \
   -I$(top_srcdir)/src/HLS/interface/FSL \
   -I$(top_srcdir)/src/HLS/interface/NPI

noinst_HEADERS += AXI/axi4lite_interface.hpp \
                  FSL/FSL_interface.hpp \
                  NPI/NPI_interface.hpp

lib_interface_la_SOURCES += AXI/axi4lite_interface.cpp \
                            FSL/FSL_interface.cpp \
                            NPI/NPI_interface.cpp
endif
if BUILD_TASTE
noinst_HEADERS += \
   taste_interface_generation.hpp

lib_interface_la_SOURCES += \
   taste_interface_generation.cpp

lib_interface_la_CPPFLAGS += \
   -I$(top_srcdir)/src/constants \
   -I$(top_srcdir)/src/HLS/stg \
   -I$(top_srcdir)/src/intermediate_representations/aadl_asn \
   -I$(top_srcdir)/src/technology \
   -I$(top_srcdir)/src/technology/physical_library \
   -I$(top_srcdir)/src/technology/target_device
endif


#do not touch the following line

