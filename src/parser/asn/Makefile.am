#this file is src/parser/asn/Makefile.am
noinst_LTLIBRARIES = lib_asnparser.la

lib_asnparser_la_CPPFLAGS = \
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

noinst_HEADERS = asn_parser.hpp asn_lexer.hpp asn_parser_node.hpp

lib_asnparser_la_SOURCES = asn_yparser.ypp asn_lexer.lpp asn_parser.cpp asn_parser_node.cpp
AM_YFLAGS = -d -l --report=state,lookahead,itemset -Wnone

if BISON_2_7_OR_GREATER
BUILT_SOURCES = asn_yparser.hpp asn_lexer.cpp asn_yparser.cpp
else
BUILT_SOURCES = asn_yparser.h asn_lexer.cpp asn_yparser.cpp
endif
CLEANFILES = $(BUILT_SOURCES) *.output

#DOCUMENTATION STUFF

DOX_TAGFILES=

PRJ_DOC =
EXTRA_DIST = $(PRJ_DOC)

#do not touch the following line


