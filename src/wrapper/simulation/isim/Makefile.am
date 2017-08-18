#this file is src/wrapper/simulation/isim/Makefile.am
noinst_LTLIBRARIES = lib_isimwrapper.la

lib_isimwrapper_la_CPPFLAGS = \
    -I$(top_srcdir)/src \
    -I$(top_srcdir)/src/constants \
    -I$(top_srcdir)/src/parser/polixml \
    -I$(top_srcdir)/src/polixml \
    -I$(top_srcdir)/src/utility \
    -I$(top_srcdir)/src/wrapper \
    -I$(top_srcdir)/src/wrapper/simulation \
    -I$(BOOST_DIR) $(all_includes) 

lib_isimwrapper_la_LDFLAGS = $(BOOST_FILESYSTEM_LIB)

METASOURCES = AUTO

noinst_HEADERS = ISE_isim_wrapper.hpp

lib_isimwrapper_la_SOURCES = ISE_isim_wrapper.cpp

#do not touch the following line
include $(top_srcdir)/panda.am
