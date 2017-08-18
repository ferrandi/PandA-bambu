#this file is src/parser/discrepancy/Makefile.am
noinst_LTLIBRARIES = lib_discrepancy_parser.la

lib_discrepancy_parser_la_CPPFLAGS = \
   -I$(top_srcdir)/src/ \
   -I$(top_srcdir)/src/constants \
   -I$(top_srcdir)/src/graph \
   -I$(top_srcdir)/src/HLS/vcd \
   -I$(top_srcdir)/src/parser/discrepancy \
   -I$(top_srcdir)/src/tree \
   -I$(top_srcdir)/src/utility \
   -I$(BOOST_DIR) $(all_includes)

METASOURCES = AUTO
AM_YFLAGS = -d -l --report=state,lookahead,itemset
AM_LFLAGS = -L -B -+  -olex.yy.c

noinst_HEADERS = \
   DiscrepancyOpInfo.hpp \
   discrepancyLexer.hpp \
   parse_discrepancy.hpp

lib_discrepancy_parser_la_SOURCES = \
   DiscrepancyOpInfo.cpp \
   discrepancyParser.ypp \
   discrepancyLexer.lpp \
   parse_discrepancy.cpp

if BISON_2_7_OR_GREATER
BUILT_SOURCES = discrepancyParser.hpp discrepancyLexer.cpp discrepancyParser.cpp
else
BUILT_SOURCES = discrepancyParser.h discrepancyLexer.cpp discrepancyParser.cpp
endif

CLEANFILES = $(BUILT_SOURCES) *.output

#DOCUMENTATION STUFF

DOX_TAGFILES=

PRJ_DOC = 
EXTRA_DIST = $(PRJ_DOC)

#do not touch the following line
include $(top_srcdir)/panda.am

