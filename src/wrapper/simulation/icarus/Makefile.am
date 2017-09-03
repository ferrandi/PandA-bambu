#this file is src/wrapper/simulation/icarus/Makefile.am
noinst_LTLIBRARIES = lib_icaruswrapper.la

lib_icaruswrapper_la_CPPFLAGS = \
   -I$(top_srcdir)/src \
   -I$(top_srcdir)/src/constants \
   -I$(top_srcdir)/src/parser/polixml \
   -I$(top_srcdir)/src/polixml \
   -I$(top_srcdir)/src/tree \
   -I$(top_srcdir)/src/utility \
   -I$(top_srcdir)/src/wrapper/simulation \
   -I$(BOOST_DIR) $(all_includes)

lib_icaruswrapper_la_LDFLAGS = $(BOOST_FILESYSTEM_LIB)

METASOURCES = AUTO

noinst_HEADERS = IcarusWrapper.hpp

lib_icaruswrapper_la_SOURCES = IcarusWrapper.cpp 

#do not touch the following line

