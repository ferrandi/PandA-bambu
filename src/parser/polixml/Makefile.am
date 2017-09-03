#this file is src/parser/polixml/Makefile.am
noinst_LTLIBRARIES = lib_xml_dom_parser.la

lib_xml_dom_parser_la_CPPFLAGS = \
   -I$(top_srcdir)/src/ \
   -I$(top_srcdir)/src/constants \
   -I$(top_srcdir)/src/polixml \
   -I$(top_srcdir)/src/utility \
   -I$(BOOST_DIR) $(all_includes)

METASOURCES = AUTO
AM_YFLAGS = -d -l --report=state,lookahead,itemset
AM_LFLAGS = -L -B -+  -olex.yy.c

noinst_HEADERS = xml_dom_parser.hpp xmlLexer.hpp

lib_xml_dom_parser_la_SOURCES = xmlParser.ypp xmlLexer.lpp xml_dom_parser.cpp

if BISON_2_7_OR_GREATER
BUILT_SOURCES = xmlParser.hpp xmlLexer.cpp xmlParser.cpp
else
BUILT_SOURCES = xmlParser.h xmlLexer.cpp xmlParser.cpp
endif

CLEANFILES = $(BUILT_SOURCES) *.output

#DOCUMENTATION STUFF

DOX_TAGFILES=

PRJ_DOC = 
EXTRA_DIST = $(PRJ_DOC)

#do not touch the following line


