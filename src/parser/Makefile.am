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

if BUILD_LIB_FROM_AIGER
  SUBDIRS += aiger
  DIST_SUBDIRS += aiger
  lib_parser_la_LIBADD += $(top_builddir)/src/parser/aiger/lib_aigerparser.la
endif

if BUILD_LIB_FROM_BENCH
  SUBDIRS += bench
  DIST_SUBDIRS += bench
  lib_parser_la_LIBADD += $(top_builddir)/src/parser/bench/lib_benchparser.la
endif

if BUILD_LIB_FROM_BLIF
  SUBDIRS += blif
  DIST_SUBDIRS += blif
  lib_parser_la_LIBADD += $(top_builddir)/src/parser/blif/lib_blifparser.la
endif

if BUILD_LIB_FROM_BOOLEAN
  SUBDIRS += boolean
  DIST_SUBDIRS += boolean
endif

if BUILD_LIB_FROM_CSV
  SUBDIRS += csv
  DIST_SUBDIRS += csv
  lib_parser_la_LIBADD += $(top_builddir)/src/parser/csv/lib_csv_parser.la
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

if BUILD_LIB_EDIF
  SUBDIRS += edif
  DIST_SUBDIRS += edif
  lib_parser_la_LIBADD += $(top_builddir)/src/parser/edif/lib_edifparser.la
endif

if BUILD_LIB_FROM_ESTIMATION
  SUBDIRS += estimation
  DIST_SUBDIRS += estimation
  lib_parser_la_LIBADD += $(top_builddir)/src/parser/estimation/lib_estimationparser.la
endif

if BUILD_LIB_FROM_ARCH
   SUBDIRS += machine
   DIST_SUBDIRS += machine
   lib_parser_la_LIBADD += $(top_builddir)/src/parser/machine/lib_machineparser.la
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

if BUILD_LIB_FROM_DEEP_PROFILING
  SUBDIRS += profiling_analysis
  DIST_SUBDIRS += profiling_analysis
  lib_parser_la_LIBADD += $(top_builddir)/src/parser/profiling_analysis/lib_profiling_analysisparser.la
endif

if BUILD_LIB_FROM_PSPLIB
  SUBDIRS += psplib
  DIST_SUBDIRS += psplib
  lib_parser_la_LIBADD += $(top_builddir)/src/parser/psplib/lib_psplibparser.la
endif

if BUILD_LIB_FROM_RTL
  SUBDIRS += rtlgcc
  DIST_SUBDIRS += rtlgcc
  lib_parser_la_LIBADD += $(top_builddir)/src/parser/rtlgcc/lib_rtlgccparser.la
endif

if BUILD_LIB_FROM_SDF3
  SUBDIRS += sdf3
  DIST_SUBDIRS += sdf3
  lib_parser_la_LIBADD += $(top_builddir)/src/parser/sdf3/lib_sdf3_parser.la
endif

if BUILD_LIB_FROM_TGFF
  SUBDIRS += TGFF
  DIST_SUBDIRS += TGFF
  lib_parser_la_LIBADD += $(top_builddir)/src/parser/TGFF/lib_TGFFparser.la
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

