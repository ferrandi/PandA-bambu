#this file is src/technology/physical_library/models/area/Makefile.am
METASOURCES = AUTO
noinst_LTLIBRARIES = lib_area_models.la

lib_area_models_la_CPPFLAGS = \
    -I$(top_srcdir)/src \
    -I$(top_srcdir)/src/polixml \
    -I$(top_srcdir)/src/technology/target_device \
    -I$(top_srcdir)/src/technology/physical_library \
    -I$(top_srcdir)/src/technology/physical_library/models \
    -I$(top_srcdir)/src/utility \
    -I$(BOOST_DIR) $(all_includes)

noinst_HEADERS = clb_model.hpp
lib_area_models_la_SOURCES = clb_model.cpp

if BUILD_LIB_CMOS
   noinst_HEADERS += cell_model.hpp
   lib_area_models_la_SOURCES += cell_model.cpp
   lib_area_models_la_CPPFLAGS += \
      -I$(top_srcdir)/src/technology/target_technology/CMOS
endif

#do not touch the following line
include $(top_srcdir)/panda.am
