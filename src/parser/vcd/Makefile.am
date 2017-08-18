#this file is src/parser/vcd/Makefile.am
noinst_LTLIBRARIES = lib_vcdparser.la

lib_vcdparser_la_CPPFLAGS = \
              -I$(top_srcdir)/src \
              -I$(top_srcdir)/src/circuit \
              -I$(top_srcdir)/src/utility \
              -I$(BOOST_DIR) $(all_includes)

noinst_HEADERS = sig_variation.hpp vcd_parser.hpp

lib_vcdparser_la_SOURCES = sig_variation.cpp vcd_parser.cpp

#do not touch the following line
include $(top_srcdir)/panda.am
