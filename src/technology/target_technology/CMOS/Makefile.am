#this file is src/logic_synthesis/target/CMOS/Makefile.am
METASOURCES = AUTO
noinst_LTLIBRARIES = lib_target_technology_CMOS.la

lib_target_technology_CMOS_la_CPPFLAGS = \
   -I$(top_srcdir)/src \
   -I$(top_srcdir)/src/constants \
   -I$(top_srcdir)/src/parser/polixml \
   -I$(top_srcdir)/src/polixml \
   -I$(top_srcdir)/src/technology \
   -I$(top_srcdir)/src/technology/target_technology \
   -I$(top_srcdir)/src/utility \
   -I$(BOOST_DIR) $(all_includes)

noinst_HEADERS = CMOS_technology.hpp
lib_target_technology_CMOS_la_SOURCES = CMOS_technology.cpp

#do not touch the following line


