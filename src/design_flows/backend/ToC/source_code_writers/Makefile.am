#this file is src/design_flows/backend/ToC/source_code_writers/Makefile.am
noinst_LTLIBRARIES =  lib_source_code_writers.la

lib_source_code_writers_la_CPPFLAGS = \
   -I$(top_srcdir)/src \
   -I$(top_srcdir)/src/constants \
   -I$(top_srcdir)/src/algorithms/dominance \
   -I$(top_srcdir)/src/algorithms/loops_detection \
   -I$(top_srcdir)/src/behavior \
   -I$(top_srcdir)/src/design_flows \
   -I$(top_srcdir)/src/design_flows/backend/ToC \
   -I$(top_srcdir)/src/design_flows/backend/ToHDL \
   -I$(top_srcdir)/src/design_flows/backend/ToC/progModels \
   -I$(top_srcdir)/src/frontend_analysis \
   -I$(top_srcdir)/src/frontend_analysis/IR_analysis \
   -I$(top_srcdir)/src/graph \
   -I$(top_srcdir)/src/parser/treegcc \
   -I$(top_srcdir)/src/tree \
   -I$(top_srcdir)/src/utility \
   -I$(top_srcdir)/src/wrapper/treegcc \
   -I$(top_srcdir)/src/HLS/binding/module/ \
   -I$(top_srcdir)/src/HLS/module_allocation/ \
   -I$(top_srcdir)/src/HLS/scheduling \
   -I$(top_srcdir)/src/HLS/stg \
   -I$(top_srcdir)/src/HLS/vcd \
   -I$(top_srcdir)/src/HLS \
   -I$(top_srcdir)/src/parser/discrepancy \
   -I$(top_srcdir)/src/technology/physical_library/ \
   -I$(top_srcdir)/src/technology/physical_library/models/ \
   -I$(top_srcdir)/src/technology/target_device/ \
   -I$(BOOST_DIR) $(all_includes)

METASOURCES = AUTO

noinst_HEADERS = c_writer.hpp instruction_writer.hpp

lib_source_code_writers_la_SOURCES = c_writer.cpp instruction_writer.cpp

if BUILD_BAMBU
   noinst_HEADERS += \
      hls_c_writer.hpp \
      hls_instruction_writer.hpp\
      discrepancy_analysis_c_writer.hpp\
      discrepancy_instruction_writer.hpp

   lib_source_code_writers_la_SOURCES += \
      hls_c_writer.cpp \
      hls_instruction_writer.cpp\
      discrepancy_analysis_c_writer.cpp\
      discrepancy_instruction_writer.cpp

   lib_source_code_writers_la_CPPFLAGS += \
      -I$(top_srcdir)/src/circuit \
      -I$(top_srcdir)/src/HLS \
      -I$(top_srcdir)/src/HLS/memory \
      -I$(top_srcdir)/src/HLS/simulation \
      -I$(top_srcdir)/src/intermediate_representations \
      -I$(top_srcdir)/src/intermediate_representations/hls \
      -I$(top_srcdir)/src/technology/physical_library
endif


if BUILD_LIB_HOST_PROFILING
   noinst_HEADERS += \
      basic_blocks_profiling_c_writer.hpp \
      edge_c_writer.hpp
   lib_source_code_writers_la_SOURCES += \
      basic_blocks_profiling_c_writer.cpp \
      edge_c_writer.cpp
endif


if BUILD_LIB_PRAGMA
   lib_source_code_writers_la_CPPFLAGS += \
      -I$(top_srcdir)/src/pragma
   lib_source_code_writers_la_CPPFLAGS += \
      -I$(top_srcdir)/src/utility/probability_distribution

endif



#DOCUMENTATION STUFF

EXTRA_DIST = $(PRJ_DOC)

#do not touch the following line

