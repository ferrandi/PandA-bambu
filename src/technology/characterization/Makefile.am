#this file is src/technology/characterization/Makefile.am
METASOURCES = AUTO
SUBDIRS =
DIST_SUBDIRS = 
noinst_LTLIBRARIES = lib_characterization.la

lib_characterization_la_CPPFLAGS = \
   -I$(top_srcdir)/src \
   -I$(top_srcdir)/src/constants \
   -I$(top_srcdir)/src/behavior \
   -I$(top_srcdir)/src/design_flows \
   -I$(top_srcdir)/src/design_flows/backend/ToHDL \
   -I$(top_srcdir)/src/design_flows/technology \
   -I$(top_srcdir)/src/graph \
   -I$(top_srcdir)/src/HLS \
   -I$(top_srcdir)/src/HLS/module_allocation \
   -I$(top_srcdir)/src/HLS/scheduling \
   -I$(top_srcdir)/src/intermediate_representations \
   -I$(top_srcdir)/src/intermediate_representations/hls \
   -I$(top_srcdir)/src/technology \
   -I$(top_srcdir)/src/technology/physical_library \
   -I$(top_srcdir)/src/technology/physical_library/models \
   -I$(top_srcdir)/src/technology/physical_library/models/area \
   -I$(top_srcdir)/src/technology/physical_library/models/time \
   -I$(top_srcdir)/src/technology/target_device \
   -I$(top_srcdir)/src/technology/target_technology \
   -I$(top_srcdir)/src/circuit \
   -I$(top_srcdir)/src/polixml \
   -I$(top_srcdir)/src/parser/polixml \
   -I$(top_srcdir)/src/graph \
   -I$(top_srcdir)/src/wrapper \
   -I$(top_srcdir)/src/wrapper/synthesis\
   -I$(top_srcdir)/src/wrapper/synthesis/xilinx \
   -I$(top_srcdir)/src/utility \
   -I$(BOOST_DIR) $(all_includes)

if BUILD_FLOPOCO
lib_characterization_la_CPPFLAGS += -I$(top_srcdir)/src/wrapper/flopoco
endif

noinst_HEADERS =
lib_characterization_la_SOURCES =
lib_characterization_la_LIBADD =


if BUILD_LIB_RTL_CHARACTERIZATION
noinst_HEADERS += RTL_characterization.hpp
lib_characterization_la_SOURCES += RTL_characterization.cpp
if BUILD_LIB_EXPERIMENTAL
noinst_HEADERS += core_generation.hpp
lib_characterization_la_SOURCES += core_generation.cpp
endif
endif

if BUILD_LIB_TECHNOLOGY_CHARACTERIZATION
if BUILD_LIB_EXPERIMENTAL
   SUBDIRS += area time wirelength
   DIST_SUBDIRS += area time wirelength
   noinst_HEADERS += AreaCharacterization.hpp TimeCharacterization.hpp WireCharacterization.hpp ModelValidation.hpp
   lib_characterization_la_SOURCES += AreaCharacterization.cpp TimeCharacterization.cpp WireCharacterization.cpp ModelValidation.cpp
   lib_characterization_la_CPPFLAGS += \
       -I$(top_srcdir)/src/technology/characterization/area \
       -I$(top_srcdir)/src/technology/characterization/time \
       -I$(top_srcdir)/src/technology/characterization/wirelength \
       -I$(top_srcdir)/src/technology/utility
   lib_characterization_la_LIBADD += \
        $(top_builddir)/src/technology/characterization/area/lib_area_characterization.la \
        $(top_builddir)/src/technology/characterization/time/lib_time_characterization.la \
        $(top_builddir)/src/technology/characterization/wirelength/lib_wire_characterization.la
endif
endif

#do not touch the following line
include $(top_srcdir)/panda.am
