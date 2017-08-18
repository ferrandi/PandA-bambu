#this file is src/wrapper/simulation/verilator/Makefile.am
noinst_LTLIBRARIES = lib_verilatorwrapper.la

lib_verilatorwrapper_la_CPPFLAGS = \
   -I$(top_srcdir)/src \
   -I$(top_srcdir)/src/constants \
   -I$(top_srcdir)/src/parser/polixml \
   -I$(top_srcdir)/src/polixml \
   -I$(top_srcdir)/src/tree \
   -I$(top_srcdir)/src/utility \
   -I$(top_srcdir)/src/wrapper/simulation \
   -I$(BOOST_DIR) $(all_includes)

lib_verilatorwrapper_la_LDFLAGS = $(BOOST_FILESYSTEM_LIB)

METASOURCES = AUTO

noinst_HEADERS = VerilatorWrapper.hpp

lib_verilatorwrapper_la_SOURCES = VerilatorWrapper.cpp 

#do not touch the following line
include $(top_srcdir)/panda.am
