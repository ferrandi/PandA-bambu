#this file is src/parser/vcd/Makefile.am
noinst_LTLIBRARIES = lib_vcdparser.la
AM_CPPFLAGS = $(all_includes) -I$(BOOST_DIR) -I$(ABSEIL_DIR)

lib_vcdparser_la_CPPFLAGS = \
              -I$(top_srcdir)/src \
              -I$(top_srcdir)/src/circuit \
              -I$(top_srcdir)/src/utility \
              $(AM_CPPFLAGS)

noinst_HEADERS = sig_variation.hpp vcd_parser.hpp

lib_vcdparser_la_SOURCES = sig_variation.cpp vcd_parser.cpp

#do not touch the following line

