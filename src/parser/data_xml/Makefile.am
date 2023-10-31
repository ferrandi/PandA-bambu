#this file is src/parser/data_xml_/Makefile.am
noinst_LTLIBRARIES = lib_data_xml_parser.la
AM_CPPFLAGS = $(all_includes) -I$(BOOST_DIR) -I$(ABSEIL_DIR)
METASOURCES = AUTO

lib_data_xml_parser_la_CPPFLAGS = \
   -I$(top_srcdir)/src/ \
   -I$(top_srcdir)/src/constants \
   -I$(top_srcdir)/src/parser/polixml \
   -I$(top_srcdir)/src/polixml \
   -I$(top_srcdir)/src/tree \
   -I$(top_srcdir)/src/utility \
   $(AM_CPPFLAGS)

noinst_HEADERS = data_xml_parser.hpp

lib_data_xml_parser_la_SOURCES = data_xml_parser.cpp

#DOCUMENTATION STUFF

DOX_TAGFILES=

PRJ_DOC =
EXTRA_DIST = $(PRJ_DOC)

#do not touch the following line

