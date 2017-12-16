#this file is src/technology/physical_library/models/Makefile.am
SUBDIRS = area time
DIST_SUBDIRS = area time

METASOURCES = AUTO
noinst_LTLIBRARIES = lib_models.la

lib_models_la_CPPFLAGS = \
   -I$(top_srcdir)/src/ \
   -I$(top_srcdir)/src/constants \
   -I$(top_srcdir)/src/graph\
   -I$(top_srcdir)/src/HLS/scheduling\
   -I$(top_srcdir)/src/polixml \
   -I$(top_srcdir)/src/technology/ \
   -I$(top_srcdir)/src/technology/physical_library/ \
   -I$(top_srcdir)/src/technology/physical_library/models/area \
   -I$(top_srcdir)/src/technology/physical_library/models/time \
   -I$(top_srcdir)/src/technology/target_device \
   -I$(top_srcdir)/src/utility \
   -I$(BOOST_DIR) $(all_includes)

if BUILD_LIB_CUDD
  lib_models_la_CPPFLAGS += \
    $(CUDD_CPPFLAGS)
endif

noinst_HEADERS = area_model.hpp time_model.hpp
lib_models_la_SOURCES = area_model.cpp time_model.cpp

lib_models_la_LIBADD = \
             $(top_builddir)/src/technology/physical_library/models/area/lib_area_models.la \
             $(top_builddir)/src/technology/physical_library/models/time/lib_time_models.la


#do not touch the following line

