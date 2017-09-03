#this file is src/intermediate_representations/hls/Makefile.am
SUBDIRS =
DIST_SUBDIRS =

METASOURCES = AUTO
noinst_LTLIBRARIES = lib_hls.la

noinst_HEADERS = \
   hls_function_ir.hpp \
   hls_ir.hpp

lib_hls_la_CPPFLAGS = \
    -I$(top_srcdir)/src \
    -I$(top_srcdir)/src/behavior \
    -I$(top_srcdir)/src/graph \
    -I$(top_srcdir)/src/HLS \
    -I$(top_srcdir)/src/intermediate_representations \
    -I$(top_srcdir)/src/utility \
    -I$(BOOST_DIR) $(all_includes)

lib_hls_la_SOURCES = \
   hls_function_ir.cpp \
   hls_ir.cpp

PRJ_DOC =
EXTRA_DIST = $(PRJ_DOC)

#do not touch the following line


