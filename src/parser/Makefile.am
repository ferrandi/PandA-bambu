#this file is src/parser/Makefile.am
METASOURCES = AUTO
AM_CPPFLAGS = $(all_includes) -I$(BOOST_DIR) -I$(ABSEIL_DIR)

SUBDIRS =
DIST_SUBDIRS =

noinst_HEADERS =

noinst_LTLIBRARIES = lib_parser.la

lib_parser_la_SOURCES =
if BUILD_LIB_FROM_AADL_ASN
noinst_HEADERS += \
   parser_flow_step.hpp \
   parser_flow_step_factory.hpp
lib_parser_la_SOURCES += \
   parser_flow_step.cpp \
   parser_flow_step_factory.cpp
endif

lib_parser_la_LIBADD =

lib_parser_la_CPPFLAGS = \
   -I$(top_srcdir)/src/design_flows \
   -I$(top_srcdir)/src/graph \
   -I$(top_srcdir)/src/utility \
   $(AM_CPPFLAGS)

if BUILD_LIB_FROM_AADL_ASN
  SUBDIRS += aadl asn
  DIST_SUBDIRS += aadl asn
  lib_parser_la_LIBADD += \
     $(top_builddir)/src/parser/aadl/lib_aadlparser.la \
     $(top_builddir)/src/parser/asn/lib_asnparser.la
  lib_parser_la_CPPFLAGS += \
     -I$(top_srcdir)/src/parser/aadl \
     -I$(top_srcdir)/src/parser/asn
endif

if BUILD_LIB_FROM_DATA_XML
  SUBDIRS += data_xml
  DIST_SUBDIRS += data_xml
  lib_parser_la_LIBADD += $(top_builddir)/src/parser/data_xml/lib_data_xml_parser.la
endif

if BUILD_LIB_FROM_DISCREPANCY
  SUBDIRS += discrepancy
  DIST_SUBDIRS += discrepancy
  lib_parser_la_LIBADD += $(top_builddir)/src/parser/discrepancy/lib_discrepancy_parser.la
endif

if BUILD_LIB_POLIXML
  SUBDIRS += polixml
  DIST_SUBDIRS += polixml
endif

if BUILD_LIB_FROM_PRAGMA
  SUBDIRS += pragma
  DIST_SUBDIRS += pragma
  lib_parser_la_LIBADD += $(top_builddir)/src/parser/pragma/lib_pragmaparser.la
endif

if BUILD_LIB_TREE_PARSER
  SUBDIRS += compiler
  DIST_SUBDIRS += compiler
  lib_parser_la_LIBADD += $(top_builddir)/src/parser/compiler/lib_compilerparser.la
endif

if BUILD_LIB_VCD_PARSER
  SUBDIRS += vcd
  DIST_SUBDIRS += vcd
  lib_parser_la_LIBADD += $(top_builddir)/src/parser/vcd/lib_vcdparser.la
endif

#do not touch the following line

