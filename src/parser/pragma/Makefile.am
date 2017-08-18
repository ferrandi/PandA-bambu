#this file is src/parser/pragma/Makefile.am
noinst_LTLIBRARIES = lib_pragmaparser.la

lib_pragmaparser_la_CPPFLAGS = \
   -I$(top_srcdir)/src \
   -I$(top_srcdir)/src/constants \
   -I$(top_srcdir)/src/graph \
   -I$(top_srcdir)/src/pragma \
   -I$(top_srcdir)/src/utility \
   -I$(BOOST_DIR) $(all_includes) -Wno-switch-enum

METASOURCES = AUTO

noinst_HEADERS = PragmaParser.hpp

lib_pragmaparser_la_SOURCES = PragmaParser.cpp

lib_pragmaparser_la_LDFLAGS = $(BOOST_FILESYSTEM_LIB) $(BOOST_REGEX_LIB)

#DOCUMENTATION STUFF
DOX_TAGFILES=

PRJ_DOC = pragma.doc
EXTRA_DIST = $(PRJ_DOC)

#do not touch the following line
include $(top_srcdir)/panda.am
