#this file is src/technology/Makefile.am
SUBDIRS =
DIST_SUBDIRS =

noinst_LTLIBRARIES = lib_technology.la

lib_technology_la_CPPFLAGS = \
   -I$(top_srcdir)/src \
   -I$(top_srcdir)/src/circuit \
   -I$(top_srcdir)/src/constants \
   -I$(top_srcdir)/src/graph \
   -I$(top_srcdir)/src/HLS/module_allocation \
   -I$(top_srcdir)/src/HLS/scheduling \
   -I$(top_srcdir)/src/parser/polixml \
   -I$(top_srcdir)/src/polixml \
   -I$(top_srcdir)/src/technology/physical_library \
   -I$(top_srcdir)/src/technology/physical_library/models \
   -I$(top_srcdir)/src/technology/physical_library/models/area \
   -I$(top_srcdir)/src/technology/target_device \
   -I$(top_srcdir)/src/utility \
   -I$(BOOST_DIR) $(all_includes)

METASOURCES = AUTO

noinst_HEADERS = technology_manager.hpp target_manager.hpp parse_technology.hpp

lib_technology_la_SOURCES = technology_manager.cpp target_manager.cpp parse_technology.cpp
lib_technology_la_LIBADD =


if BUILD_LIBERTY_PARSER
   lib_technology_la_CPPFLAGS += \
      -I$(top_srcdir)/ext/liberty_parser
endif

if BUILD_LIB_TARGET
   SUBDIRS += target_technology target_device
   DIST_SUBDIRS += target_technology target_device
   lib_technology_la_LIBADD += \
                   $(top_builddir)/src/technology/target_technology/lib_target_technology.la \
                   $(top_builddir)/src/technology/target_device/lib_target_device.la
endif

if BUILD_LIB_PHYSICAL_LIBRARY
   SUBDIRS += physical_library
   DIST_SUBDIRS += physical_library
   lib_technology_la_LIBADD += $(top_builddir)/src/technology/physical_library/lib_physical_library.la
endif

if BUILD_LIB_VIRTUAL_LIBRARY
   SUBDIRS += virtual_library
   DIST_SUBDIRS += virtual_library
   lib_technology_la_LIBADD += $(top_builddir)/src/technology/virtual_library/lib_virtual_library.la
endif

if BUILD_LIB_LIBRARY_CHARACTERIZATION
   SUBDIRS += characterization
   DIST_SUBDIRS += characterization
   lib_technology_la_LIBADD += $(top_builddir)/src/technology/characterization/lib_characterization.la
endif

#do not touch the following line

