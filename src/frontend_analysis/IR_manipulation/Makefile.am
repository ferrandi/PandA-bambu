#this file is src/frontend_analysis/IR_manipulation/Makefile.am
METASOURCES = AUTO
noinst_LTLIBRARIES = lib_IR_manipulation.la

lib_IR_manipulation_la_CPPFLAGS = \
   -I$(top_srcdir)/src \
   -I$(top_srcdir)/src/algorithms/dominance \
   -I$(top_srcdir)/src/algorithms/loops_detection \
   -I$(top_srcdir)/src/behavior \
   -I$(top_srcdir)/src/constants \
   -I$(top_srcdir)/src/design_flows \
   -I$(top_srcdir)/src/frontend_analysis \
   -I$(top_srcdir)/src/graph \
   -I$(top_srcdir)/src/parser/treegcc \
   -I$(top_srcdir)/src/rtl \
   -I$(top_srcdir)/src/tree \
   -I$(top_srcdir)/src/utility \
   -I$(top_srcdir)/src/utility/probability_distribution \
   -I$(top_srcdir)/src/wrapper/treegcc \
   -I$(BOOST_DIR) $(all_includes)

if BUILD_LIB_FROM_PRAGMA
  lib_IR_manipulation_la_CPPFLAGS += \
   -I$(top_srcdir)/src/pragma
if BUILD_LIB_CODESIGN
if BUILD_LIB_PARTITIONING
if BUILD_LIB_GRAPH_PARTITIONING
  lib_IR_manipulation_la_CPPFLAGS += \
   -I$(top_srcdir)/src/design_flows/codesign/partitioning/graph_partitioning
endif
endif
endif
endif

noinst_HEADERS = \
   build_virtual_phi.hpp \
   clean_virtual_phi.hpp \
   create_tree_manager.hpp

lib_IR_manipulation_la_SOURCES = \
   build_virtual_phi.cpp \
   clean_virtual_phi.cpp \
   create_tree_manager.cpp

if BUILD_ZEBU
   noinst_HEADERS += \
      loops_rebuilding.hpp
   lib_IR_manipulation_la_SOURCES += \
      loops_rebuilding.cpp
endif

if BUILD_BAMBU
   noinst_HEADERS += \
      cond_expr_restructuring.hpp \
      extract_gimple_cond_op.hpp \
      predicate_statements.hpp \
      serialize_mutual_exclusions.hpp \
      un_comparison_lowering.hpp
   lib_IR_manipulation_la_SOURCES += \
      cond_expr_restructuring.cpp \
      extract_gimple_cond_op.cpp \
      predicate_statements.cpp \
      serialize_mutual_exclusions.cpp \
      un_comparison_lowering.cpp
   lib_IR_manipulation_la_CPPFLAGS += \
      -I$(top_srcdir)/src/intermediate_representations \
      -I$(top_srcdir)/src/intermediate_representations/hls
if BUILD_LIB_FROM_AADL_ASN
   lib_IR_manipulation_la_CPPFLAGS += \
      -I$(top_srcdir)/src/parser
endif
if BUILD_LIB_ILP
  lib_IR_manipulation_la_CPPFLAGS += \
   -I$(top_srcdir)/src/HLS/ \
   -I$(top_srcdir)/src/HLS/binding/module \
   -I$(top_srcdir)/src/HLS/module_allocation \
   -I$(top_srcdir)/src/HLS/scheduling

if BUILD_LIB_FROM_PRAGMA
if BUILD_LIB_EXPERIMENTAL
   noinst_HEADERS += \
      unroll_loops.hpp
   lib_IR_manipulation_la_SOURCES += \
      unroll_loops.cpp
endif
endif


if BUILD_LIB_ILP
   noinst_HEADERS += \
      sdc_code_motion.hpp
   lib_IR_manipulation_la_SOURCES += \
      sdc_code_motion.cpp
  lib_IR_manipulation_la_CPPFLAGS += \
   -I$(top_srcdir)/src/frontend_analysis/IR_analysis \
   -I$(top_srcdir)/src/HLS \
   -I$(top_srcdir)/src/HLS/module_allocation \
   -I$(top_srcdir)/src/HLS/scheduling
endif
endif
endif
#do not touch the following line
include $(top_srcdir)/panda.am
