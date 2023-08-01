#this file is src/parser/pragma/Makefile.am
noinst_LTLIBRARIES = lib_pragmaparser.la
AM_CXXFLAGS = -Wno-switch-enum
AM_CPPFLAGS = $(all_includes) -I$(BOOST_DIR) -I$(ABSEIL_DIR)

lib_pragmaparser_la_CPPFLAGS = \
   -I$(top_srcdir)/src \
   -I$(top_srcdir)/src/constants \
   -I$(top_srcdir)/src/graph \
   -I$(top_srcdir)/src/pragma \
   -I$(top_srcdir)/src/utility \
   $(AM_CPPFLAGS)

METASOURCES = AUTO

noinst_HEADERS = PragmaParser.hpp

lib_pragmaparser_la_SOURCES = PragmaParser.cpp

#DOCUMENTATION STUFF
DOX_TAGFILES=

PRJ_DOC = pragma.doc
EXTRA_DIST = $(PRJ_DOC)

#do not touch the following line

