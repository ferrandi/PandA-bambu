#this file is src/technology/physical_library/models/time/Makefile.am
METASOURCES = AUTO
noinst_LTLIBRARIES = lib_time_models.la

lib_time_models_la_CPPFLAGS = \
   -I$(top_srcdir)/src/graph \
   -I$(top_srcdir)/src/HLS/scheduling \
   -I$(top_srcdir)/src/polixml \
   -I$(top_srcdir)/src/technology/physical_library \
   -I$(top_srcdir)/src/technology/physical_library/models \
   -I$(top_srcdir)/src/technology/target_device \
   -I$(top_srcdir)/src/utility \
   -I$(BOOST_DIR) $(all_includes)

noinst_HEADERS = LUT_model.hpp
lib_time_models_la_SOURCES = LUT_model.cpp

if BUILD_LIB_CMOS
   noinst_HEADERS += liberty_model.hpp timing_group.hpp
   lib_time_models_la_SOURCES += liberty_model.cpp timing_group.cpp
endif
#do not touch the following line

