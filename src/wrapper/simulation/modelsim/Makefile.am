#this file is src/wrapper/simulation/modelsim/Makefile.am
noinst_LTLIBRARIES = lib_modelsimwrapper.la

lib_modelsimwrapper_la_CPPFLAGS = \
    -I$(top_srcdir)/src \
    -I$(top_srcdir)/src/constants \
    -I$(top_srcdir)/src/wrapper/simulation \
    -I$(top_srcdir)/src/utility \
    -I$(BOOST_DIR) $(all_includes)

lib_modelsimwrapper_la_LDFLAGS = $(BOOST_FILESYSTEM_LIB)

METASOURCES = AUTO

noinst_HEADERS = modelsimWrapper.hpp

lib_modelsimwrapper_la_SOURCES = modelsimWrapper.cpp

#do not touch the following line
include $(top_srcdir)/panda.am
