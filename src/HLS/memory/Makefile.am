#this is file src/HLS/memory/Makefile.am
METASOURCES = AUTO

noinst_LTLIBRARIES = lib_HLS_memory.la

lib_HLS_memory_la_CPPFLAGS = \
   -I$(top_srcdir)/src \
   -I$(top_srcdir)/src/algorithms/clique_covering \
   -I$(top_srcdir)/src/algorithms/dominance \
   -I$(top_srcdir)/src/behavior \
   -I$(top_srcdir)/src/circuit \
   -I$(top_srcdir)/src/constants \
   -I$(top_srcdir)/src/design_flows \
   -I$(top_srcdir)/src/design_flows/backend/ToHDL \
   -I$(top_srcdir)/src/graph \
   -I$(top_srcdir)/src/HLS \
   -I$(top_srcdir)/src/HLS/binding/module \
   -I$(top_srcdir)/src/HLS/binding/register \
   -I$(top_srcdir)/src/HLS/interface \
   -I$(top_srcdir)/src/HLS/function_allocation \
   -I$(top_srcdir)/src/HLS/module_allocation \
   -I$(top_srcdir)/src/HLS/scheduling \
   -I$(top_srcdir)/src/HLS/stg \
   -I$(top_srcdir)/src/HLS/virtual_components \
   -I$(top_srcdir)/src/intermediate_representations \
   -I$(top_srcdir)/src/intermediate_representations/hls \
   -I$(top_srcdir)/src/parser/polixml \
   -I$(top_srcdir)/src/polixml \
   -I$(top_srcdir)/src/tree  \
   -I$(top_srcdir)/src/technology  \
   -I$(top_srcdir)/src/technology/physical_library \
   -I$(top_srcdir)/src/technology/target_device  \
   -I$(top_srcdir)/src/utility  \
   -I$(BOOST_DIR) $(all_includes)

noinst_HEADERS = \
   memory_allocation.hpp \
   memory_symbol.hpp memory.hpp \
   mem_dominator_allocation.hpp \
   mem_xml_allocation.hpp

if BUILD_LIB_EXPERIMENTAL
noinst_HEADERS += \
   memory_conflict_graph.hpp
endif

lib_HLS_memory_la_SOURCES = \
   memory_allocation.cpp \
   memory_symbol.cpp memory.cpp \
   mem_dominator_allocation.cpp \
   mem_xml_allocation.cpp

if BUILD_LIB_EXPERIMENTAL
lib_HLS_memory_la_SOURCES += \
   memory_conflict_graph.cpp
endif


#do not touch the following line

