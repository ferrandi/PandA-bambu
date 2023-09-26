#this file is src/parser/compiler/Makefile.am
noinst_LTLIBRARIES = lib_compilerparser.la
AM_CPPFLAGS = $(all_includes) -I$(BOOST_DIR) -I$(ABSEIL_DIR)
METASOURCES = AUTO

lib_compilerparser_la_CPPFLAGS = \
   -I$(top_srcdir)/src \
   -I$(top_srcdir)/src/constants \
   -I$(top_srcdir)/src/graph \
   -I$(top_srcdir)/src/parser/polixml \
   -I$(top_srcdir)/src/polixml \
   -I$(top_srcdir)/src/tree \
   -I$(top_srcdir)/src/utility \
   -I$(top_srcdir)/src/wrapper/compiler \
   $(AM_CPPFLAGS)

AM_YFLAGS = -d -l --report=state,lookahead,itemset
AM_LFLAGS = -L -B -+  -olex.yy.c

noinst_HEADERS = token_interface.hpp parse_tree.hpp treeLexer.hpp

lib_compilerparser_la_SOURCES = treeParser.ypp treeLexer.lpp parse_tree.cpp token_interface.cpp

if BISON_2_7_OR_GREATER
BUILT_SOURCES = treeParser.hpp treeLexer.cpp treeParser.cpp
else
BUILT_SOURCES = treeParser.h treeLexer.cpp treeParser.cpp
endif

CLEANFILES = $(BUILT_SOURCES) *.output

#DOCUMENTATION STUFF

DOX_TAGFILES=

PRJ_DOC = compiler.doc
EXTRA_DIST = $(PRJ_DOC)

#do not touch the following line

