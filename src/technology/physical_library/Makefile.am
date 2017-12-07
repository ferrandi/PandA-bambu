#this file is src/technology/physical_library/Makefile.am
SUBDIRS =

METASOURCES = AUTO
noinst_LTLIBRARIES = lib_physical_library.la

lib_physical_library_la_CPPFLAGS = \
   -I$(top_srcdir)/src \
   -I$(top_srcdir)/src/behavior \
   -I$(top_srcdir)/src/circuit \
   -I$(top_srcdir)/src/constants \
   -I$(top_srcdir)/src/graph \
   -I$(top_srcdir)/src/HLS/scheduling\
   -I$(top_srcdir)/src/parser/polixml \
   -I$(top_srcdir)/src/polixml \
   -I$(top_srcdir)/src/technology \
   -I$(top_srcdir)/src/technology/physical_library \
   -I$(top_srcdir)/src/technology/physical_library/models \
   -I$(top_srcdir)/src/technology/physical_library/models/area \
   -I$(top_srcdir)/src/technology/physical_library/models/time \
   -I$(top_srcdir)/src/technology/target_device \
   -I$(top_srcdir)/src/technology/target_technology \
   -I$(top_srcdir)/src/technology/target_technology/FPGA \
   -I$(top_srcdir)/src/tree \
   -I$(top_srcdir)/src/utility \
   -I$(BOOST_DIR) $(all_includes)

noinst_HEADERS = technology_node.hpp library_manager.hpp technology_wishbone.hpp
lib_physical_library_la_SOURCES = technology_node.cpp library_manager.cpp
lib_physical_library_la_LIBADD =

if BUILD_LIB_PHYSICAL_LIBRARY_MODELS
  SUBDIRS += models
  lib_physical_library_la_LIBADD += $(top_builddir)/src/technology/physical_library/models/lib_models.la
endif

if BUILD_LIB_CMOS
  lib_physical_library_la_CPPFLAGS += \
              -I$(top_srcdir)/src/technology/target_technology/CMOS
endif

if BUILD_LIBRARY_COMPILER
  lib_physical_library_la_CPPFLAGS += \
              -I$(top_srcdir)/src/wrapper/synthesis \
              -I$(top_srcdir)/src/wrapper/synthesis/synopsys \
              -I$(top_srcdir)/src/wrapper/synthesis/synopsys/library_compiler
endif


#do not touch the following line

