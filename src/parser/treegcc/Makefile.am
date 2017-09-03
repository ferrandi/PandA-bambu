#this file is src/parser/treegcc/Makefile.am
noinst_LTLIBRARIES = lib_treegccparser.la

lib_treegccparser_la_CPPFLAGS = \
   -I$(top_srcdir)/src \
   -I$(top_srcdir)/src/constants \
   -I$(top_srcdir)/src/graph \
   -I$(top_srcdir)/src/tree \
   -I$(top_srcdir)/src/utility \
   -I$(top_srcdir)/src/wrapper/treegcc \
   -I$(BOOST_DIR) $(all_includes)

if BUILD_LIB_CODE_ESTIMATION
   lib_treegccparser_la_CPPFLAGS += \
      -I$(top_srcdir)/src/utility/probability_distribution
endif

if BUILD_LIB_MAPPING
  lib_treegccparser_la_CPPFLAGS += \
     -I$(top_srcdir)/src/machine
endif

if BUILD_LIB_RTL
   lib_treegccparser_la_CPPFLAGS += \
      -I$(top_srcdir)/src/rtl
endif

METASOURCES = AUTO
AM_YFLAGS = -d -l --report=state,lookahead,itemset
AM_LFLAGS = -L -B -+  -olex.yy.c

noinst_HEADERS = token_interface.hpp parse_tree.hpp treeLexer.hpp

lib_treegccparser_la_SOURCES = treeParser.ypp treeLexer.lpp parse_tree.cpp token_interface.cpp

if BISON_2_7_OR_GREATER
BUILT_SOURCES = treeParser.hpp treeLexer.cpp treeParser.cpp
else
BUILT_SOURCES = treeParser.h treeLexer.cpp treeParser.cpp
endif

CLEANFILES = $(BUILT_SOURCES) *.output

#DOCUMENTATION STUFF

DOX_TAGFILES=

PRJ_DOC = treegcc.doc
EXTRA_DIST = $(PRJ_DOC)

#do not touch the following line

