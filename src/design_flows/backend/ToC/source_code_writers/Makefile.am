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

if BUILD_A3
   noinst_HEADERS += a3_instrument_writer.hpp a3_instruction_writer.hpp
   lib_source_code_writers_la_SOURCES += a3_instrument_writer.cpp a3_instruction_writer.cpp
endif

if BUILD_BAMBU
   noinst_HEADERS += \
      hls_c_writer.hpp \
      hls_instruction_writer.hpp\
      discrepancy_analysis_c_writer.hpp

   lib_source_code_writers_la_SOURCES += \
      hls_c_writer.cpp \
      hls_instruction_writer.cpp\
      discrepancy_analysis_c_writer.cpp

   lib_source_code_writers_la_CPPFLAGS += \
      -I$(top_srcdir)/src/circuit \
      -I$(top_srcdir)/src/HLS \
      -I$(top_srcdir)/src/HLS/memory \
      -I$(top_srcdir)/src/HLS/simulation \
      -I$(top_srcdir)/src/intermediate_representations \
      -I$(top_srcdir)/src/intermediate_representations/hls \
      -I$(top_srcdir)/src/technology/physical_library
endif

if BUILD_LIB_GRAPH_PARTITIONING
   lib_source_code_writers_la_CPPFLAGS += \
      -I$(top_srcdir)/src/design_flows/codesign/partitioning/graph_partitioning
   noinst_HEADERS += \
      actor_graph_writer.hpp \
      parallel_c_writer.hpp
   lib_source_code_writers_la_SOURCES += \
      actor_graph_writer.cpp \
      parallel_c_writer.cpp
if BUILD_LIB_SDF_GRAPHS
      noinst_HEADERS += \
         openmp_sdf_graph_writer.hpp \
         sdf_graph_writer.hpp
      lib_source_code_writers_la_SOURCES += \
         openmp_sdf_graph_writer.cpp \
         sdf_graph_writer.cpp
endif
if BUILD_LIB_TASK_GRAPHS
      noinst_HEADERS += \
         openMP_task_graph_writer.hpp \
         pthread_c_writer.hpp \
         pthread_instruction_writer.hpp \
         task_graph_writer.hpp
      lib_source_code_writers_la_CPPFLAGS += \
         -I$(top_srcdir)/src/algorithms/graph_helpers \
         -I$(top_srcdir)/src/intermediate_representations/actor_graphs \
         -I$(top_srcdir)/src/intermediate_representations/actor_graphs/task_graphs
      lib_source_code_writers_la_SOURCES += \
         openMP_task_graph_writer.cpp \
         pthread_c_writer.cpp \
         pthread_instruction_writer.cpp \
         task_graph_writer.cpp
endif
endif

if BUILD_DIOPSIS
   noinst_HEADERS += diopsis_instrument_writer.hpp
   lib_source_code_writers_la_SOURCES += diopsis_instrument_writer.cpp
endif

if BUILD_LEON3
   noinst_HEADERS += leon3_instrument_writer.hpp
   lib_source_code_writers_la_SOURCES += leon3_instrument_writer.cpp
endif

if BUILD_LIB_HOST_PROFILING
   noinst_HEADERS += basic_blocks_profiling_c_writer.hpp data_memory_profiling_instruction_writer.hpp data_memory_profiling_c_writer.hpp edge_c_writer.hpp efficient_path_profiling_c_writer.hpp hierarchical_path_profiling_c_writer.hpp tree_path_profiling_c_writer.hpp loops_profiling_c_writer.hpp
   lib_source_code_writers_la_SOURCES += basic_blocks_profiling_c_writer.cpp data_memory_profiling_c_writer.cpp data_memory_profiling_instruction_writer.cpp edge_c_writer.cpp efficient_path_profiling_c_writer.cpp hierarchical_path_profiling_c_writer.cpp tree_path_profiling_c_writer.cpp loops_profiling_c_writer.cpp
endif

if BUILD_LIB_MAPPING
   lib_source_code_writers_la_CPPFLAGS += \
      -I$(top_srcdir)/src/design_flows/codesign/mapping \
      -I$(top_srcdir)/src/intermediate_representations/mapping_annotations \
      -I$(top_srcdir)/src/machine
   noinst_HEADERS += pragma_c_writer.hpp
   lib_source_code_writers_la_SOURCES += pragma_c_writer.cpp
endif

if BUILD_LIB_PRAGMA
   lib_source_code_writers_la_CPPFLAGS += \
      -I$(top_srcdir)/src/pragma
   lib_source_code_writers_la_CPPFLAGS += \
      -I$(top_srcdir)/src/utility/probability_distribution
if BUILD_LIB_ACTOR_GRAPHS
   lib_source_code_writers_la_CPPFLAGS += \
      -I$(top_srcdir)/src/intermediate_representations/actor_graphs/sdf_graphs
endif

endif

if BUILD_LIB_TARGET_PROFILING
   noinst_HEADERS += instrument_writer.hpp instrument_instruction_writer.hpp escape_instruction_writer.hpp loops_instrument_c_writer.hpp instrument_c_writer.hpp
   lib_source_code_writers_la_SOURCES += instrument_writer.cpp instrument_instruction_writer.cpp escape_instruction_writer.cpp loops_instrument_c_writer.cpp instrument_c_writer.cpp
if BUILD_LIB_GRAPH_PARTITIONING
   noinst_HEADERS += \
      instrument_actor_graph_writer.hpp \
      instrument_parallel_c_writer.hpp \
      instrument_task_graph_writer.hpp \
      openMP_instrument_task_graph_writer.hpp
   lib_source_code_writers_la_SOURCES += \
      instrument_actor_graph_writer.cpp \
      instrument_parallel_c_writer.cpp \
      instrument_task_graph_writer.cpp \
      openMP_instrument_task_graph_writer.cpp
endif
if BUILD_LIB_SDF_GRAPHS
       lib_source_code_writers_la_SOURCES += \
          instrument_sdf_graph_writer.cpp \
          openmp_instrument_sdf_graph_writer.cpp
       noinst_HEADERS += \
          instrument_sdf_graph_writer.hpp \
          openmp_instrument_sdf_graph_writer.hpp
endif
endif

if BUILD_MPPB
   noinst_HEADERS += mppb_instrument_task_graph_writer.hpp mppb_instrument_writer.hpp mppb_instruction_writer.hpp mppb_task_graph_writer.hpp
   lib_source_code_writers_la_SOURCES += mppb_instrument_task_graph_writer.cpp mppb_instrument_writer.cpp mppb_instruction_writer.cpp mppb_task_graph_writer.cpp
   lib_source_code_writers_la_CPPFLAGS += \
      -I$(top_srcdir)/src/constants
if BUILD_LIB_SDF_GRAPHS
      noinst_HEADERS += \
         mppb_instrument_sdf_graph_writer.hpp \
         mppb_sdf_graph_writer.hpp
      lib_source_code_writers_la_SOURCES += \
         mppb_instrument_sdf_graph_writer.cpp \
         mppb_sdf_graph_writer.cpp
endif
endif

if BUILD_SIMIT
   noinst_HEADERS += simit_instrument_writer.hpp
   lib_source_code_writers_la_SOURCES += simit_instrument_writer.cpp
endif

if BUILD_TSIM
   noinst_HEADERS += tsim_instrument_writer.hpp
   lib_source_code_writers_la_SOURCES += tsim_instrument_writer.cpp
endif

if BUILD_ZEBU
   noinst_HEADERS += memory_profiling_c_writer.hpp memory_profiling_instruction_writer.hpp
   lib_source_code_writers_la_SOURCES += memory_profiling_c_writer.cpp memory_profiling_instruction_writer.cpp
endif

if BUILD_LIB_TARGET_PROFILING
   lib_source_code_writers_la_CPPFLAGS += \
      -I$(top_srcdir)/src/wrapper/target_architectures
endif

#DOCUMENTATION STUFF

EXTRA_DIST = $(PRJ_DOC)

#do not touch the following line

