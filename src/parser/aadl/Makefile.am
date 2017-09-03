#this file is src/parser/aadl/Makefile.am
noinst_LTLIBRARIES = lib_aadlparser.la

lib_aadlparser_la_CPPFLAGS = \
   -I$(top_srcdir)/src/ \
   -I$(top_srcdir)/src/behavior \
   -I$(top_srcdir)/src/constants \
   -I$(top_srcdir)/src/design_flows \
   -I$(top_srcdir)/src/graph \
   -I$(top_srcdir)/src/HLS \
   -I$(top_srcdir)/src/intermediate_representations/aadl_asn \
   -I$(top_srcdir)/src/parser \
   -I$(top_srcdir)/src/utility \
   -I$(BOOST_DIR)\
   $(all_includes)

METASOURCES = AUTO
AM_LFLAGS = -L -B -+  -olex.yy.c

noinst_HEADERS = aadl_parser.hpp aadl_lexer.hpp aadl_parser_node.hpp

lib_aadlparser_la_SOURCES = aadl_yparser.ypp aadl_lexer.lpp aadl_parser.cpp aadl_parser_node.cpp

AM_YFLAGS = -d -l --report=state,lookahead,itemset -Wnone

if BISON_2_7_OR_GREATER
BUILT_SOURCES = aadl_yparser.hpp aadl_lexer.cpp aadl_yparser.cpp
else
BUILT_SOURCES = aadl_yparser.h aadl_lexer.cpp aadl_yparser.cpp
endif
CLEANFILES = $(BUILT_SOURCES) *.output

#DOCUMENTATION STUFF

DOX_TAGFILES=

PRJ_DOC =
EXTRA_DIST = $(PRJ_DOC)

#do not touch the following line

