#this file is src/wrapper/treegcc/Makefile.am
noinst_LTLIBRARIES = lib_treegccwrapper.la

lib_treegccwrapper_la_CPPFLAGS = \
    -I$(top_srcdir)/src \
    -I$(top_srcdir)/src/behavior \
    -I$(top_srcdir)/src/constants \
    -I$(top_srcdir)/src/design_flows \
    -I$(top_srcdir)/src/graph \
    -I$(top_srcdir)/src/HLS \
    -I$(top_srcdir)/src/parser/polixml \
    -I$(top_srcdir)/src/parser/treegcc \
    -I$(top_srcdir)/src/polixml \
    -I$(top_srcdir)/src/tree \
    -I$(top_srcdir)/src/utility \
    -I$(BOOST_DIR) $(all_includes)

if BUILD_LIB_RTL
  lib_treegccwrapper_la_CPPFLAGS += -I$(top_srcdir)/src/rtl\
    -I$(top_srcdir)/src/parser/rtlgcc
endif

lib_treegccwrapper_la_LDFLAGS = $(BOOST_FILESYSTEM_LIB) $(BOOST_REGEX_LIB)

METASOURCES = AUTO

noinst_HEADERS = gcc_wrapper.hpp

lib_treegccwrapper_la_SOURCES = gcc_wrapper.cpp

if BUILD_LIB_OBJECT_CODE_WRAPPER
   lib_treegccwrapper_la_SOURCES += bfd_wrapper.cpp obj_code_wrapper.cpp
   noinst_HEADERS += bfd_wrapper.hpp obj_code_wrapper.hpp
   lib_treegccwrapper_la_CPPFLAGS += \
      -I$(top_srcdir)/src/design_flows/backend/ToC/source_code_writers
endif

#DOCUMENTATION STUFF

DOX_TAGFILES=

PRJ_DOC = treegcc.doc
EXTRA_DIST = $(PRJ_DOC)

#do not touch the following line
include $(top_srcdir)/panda.am
