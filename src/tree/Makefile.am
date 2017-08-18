#this file is src/tree/Makefile.am
noinst_LTLIBRARIES = lib_tree.la

lib_tree_la_CPPFLAGS =\
      -I$(top_srcdir)/src/parser/treegcc \
      -I$(top_srcdir)/src/utility \
      -I$(BOOST_DIR) $(all_includes)

if BUILD_LIB_RTL
  lib_tree_la_CPPFLAGS += -I$(top_srcdir)/src/rtl
endif

METASOURCES = AUTO

noinst_HEADERS = tree_node.hpp tree_reindex.hpp tree_basic_block.hpp

#TREE STUFF
lib_tree_la_SOURCES = tree_node.cpp tree_reindex.cpp tree_basic_block.cpp

if BUILD_LIB_CODE_ESTIMATION
   lib_tree_la_SOURCES += weight_information.cpp
   noinst_HEADERS += weight_information.hpp
   lib_tree_la_CPPFLAGS += \
     -I$(top_srcdir)/src/utility/probability_distribution
endif
if BUILD_BAMBU
   lib_tree_la_CPPFLAGS += \
      -I$(top_srcdir)/src/graph \
      -I$(top_srcdir)/src/HLS/scheduling
endif

if BUILD_LIB_RTL
   lib_tree_la_CPPFLAGS += \
     -I$(top_srcdir)/src/rtl
endif

if BUILD_LIB_TREE_MANIPULATION
   noinst_LTLIBRARIES += lib_tree_manipulation.la
   noinst_HEADERS += \
      tree_helper.hpp behavioral_helper.hpp var_pp_functor.hpp tree_manager.hpp tree_manipulation.hpp tree_common.hpp prettyPrintVertex.hpp ext_tree_node.hpp tree_node_mask.hpp \
      raw_writer.hpp  tree_node_finder.hpp tree_node_factory.hpp tree_nodes_merger.hpp gimple_writer.hpp type_casting.hpp tree_node_dup.hpp function_decl_refs.hpp
   lib_tree_manipulation_la_CPPFLAGS =\
      -I$(top_srcdir)/src \
      -I$(top_srcdir)/src/behavior \
      -I$(top_srcdir)/src/constants \
      -I$(top_srcdir)/src/graph \
      -I$(top_srcdir)/src/parser/treegcc \
      -I$(top_srcdir)/src/utility \
      -I$(top_srcdir)/src/wrapper/treegcc \
      -I$(BOOST_DIR) \
      $(all_includes)
   lib_tree_manipulation_la_SOURCES = \
     tree_helper.cpp behavioral_helper.cpp var_pp_functor.cpp tree_manager.cpp tree_manipulation.cpp prettyPrintVertex.cpp ext_tree_node.cpp tree_node_mask.cpp raw_writer.cpp tree_node_finder.cpp \
     tree_node_factory.cpp tree_nodes_merger.cpp gimple_writer.cpp type_casting.cpp tree_node_dup.cpp function_decl_refs.cpp
if BUILD_LIB_CODE_ESTIMATION
       lib_tree_manipulation_la_CPPFLAGS += \
          -I$(top_srcdir)/src/utility/probability_distribution
endif
if BUILD_LIB_MAPPING
       lib_tree_manipulation_la_CPPFLAGS += \
       -I$(top_srcdir)/src/machine
endif
if BUILD_LIB_RTL
       lib_tree_manipulation_la_CPPFLAGS += \
          -I$(top_srcdir)/src/rtl
endif
endif



#do not touch the following line
include $(top_srcdir)/panda.am
