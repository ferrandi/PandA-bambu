#this file is src/parser/data_xml_/Makefile.am
noinst_LTLIBRARIES = lib_data_xml_parser.la

lib_data_xml_parser_la_CPPFLAGS = \
   -I$(top_srcdir)/src/ \
   -I$(top_srcdir)/src/constants \
   -I$(top_srcdir)/src/parser/polixml \
   -I$(top_srcdir)/src/parser/rtlgcc \
   -I$(top_srcdir)/src/polixml \
   -I$(top_srcdir)/src/rtl \
   -I$(top_srcdir)/src/tree \
   -I$(top_srcdir)/src/utility \
   -I$(BOOST_DIR)\
   $(all_includes)

METASOURCES = AUTO

noinst_HEADERS = data_xml_parser.hpp

lib_data_xml_parser_la_SOURCES = data_xml_parser.cpp

#DOCUMENTATION STUFF

DOX_TAGFILES=

PRJ_DOC =
EXTRA_DIST = $(PRJ_DOC)

#do not touch the following line
include $(top_srcdir)/panda.am
