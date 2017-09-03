#this file is src/wrapper/simulation/isim/Makefile.am
noinst_LTLIBRARIES = lib_xsimwrapper.la

lib_xsimwrapper_la_CPPFLAGS = \
    -I$(top_srcdir)/src \
    -I$(top_srcdir)/src/constants \
    -I$(top_srcdir)/src/parser/polixml \
    -I$(top_srcdir)/src/polixml \
    -I$(top_srcdir)/src/utility \
    -I$(top_srcdir)/src/wrapper \
    -I$(top_srcdir)/src/wrapper/simulation \
    -I$(BOOST_DIR) $(all_includes) 

lib_xsimwrapper_la_LDFLAGS = $(BOOST_FILESYSTEM_LIB)

METASOURCES = AUTO

noinst_HEADERS = VIVADO_xsim_wrapper.hpp

lib_xsimwrapper_la_SOURCES = VIVADO_xsim_wrapper.cpp

#do not touch the following line

