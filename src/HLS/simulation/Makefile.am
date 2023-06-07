#this file is src/HLS/simulation/Makefile.am
noinst_LTLIBRARIES =  lib_simulation.la
AM_CPPFLAGS = $(all_includes) -I$(BOOST_DIR) -I$(ABSEIL_DIR)
METASOURCES = AUTO

lib_simulation_la_CPPFLAGS = \
   -I$(top_srcdir)/src \
   -I$(top_srcdir)/src/design_flows \
   -I$(top_srcdir)/src/design_flows/backend/ToC \
   -I$(top_srcdir)/src/design_flows/backend/ToC/progModels \
   -I$(top_srcdir)/src/design_flows/backend/ToC/source_code_writers \
   -I$(top_srcdir)/src/design_flows/backend/ToHDL \
   -I$(top_srcdir)/src/behavior \
   -I$(top_srcdir)/src/circuit \
   -I$(top_srcdir)/src/constants \
   -I$(top_srcdir)/src/frontend_analysis \
   -I$(top_srcdir)/src/frontend_analysis/IR_analysis \
   -I$(top_srcdir)/src/graph \
   -I$(top_srcdir)/src/HLS \
   -I$(top_srcdir)/src/HLS/binding \
   -I$(top_srcdir)/src/HLS/binding/interconnection \
   -I$(top_srcdir)/src/HLS/binding/module \
   -I$(top_srcdir)/src/HLS/binding/register \
   -I$(top_srcdir)/src/HLS/interface \
   -I$(top_srcdir)/src/HLS/memory \
   -I$(top_srcdir)/src/HLS/module_generator \
   -I$(top_srcdir)/src/HLS/scheduling \
   -I$(top_srcdir)/src/HLS/vcd \
   -I$(top_srcdir)/src/HLS/virtual_components \
   -I$(top_srcdir)/src/parser/discrepancy \
   -I$(top_srcdir)/src/parser/vcd \
   -I$(top_srcdir)/src/parser/polixml \
   -I$(top_srcdir)/src/polixml \
   -I$(top_srcdir)/src/technology \
   -I$(top_srcdir)/src/technology/physical_library \
   -I$(top_srcdir)/src/technology/target_device \
   -I$(top_srcdir)/src/tree \
   -I$(top_srcdir)/src/utility \
   -I$(top_srcdir)/src/wrapper/flopoco \
   -I$(top_srcdir)/src/wrapper/compiler \
   $(AM_CPPFLAGS)

AM_YFLAGS = -d -l --report=state,lookahead,itemset
AM_LFLAGS = -L -B -+  -olex.yy.c


noinst_HEADERS = \
   c_initialization_flex_lexer.hpp \
   c_initialization_parser.hpp \
   c_initialization_parser_functor.hpp \
   c_initialization_parser_node.hpp \
   CTestbenchExecution.hpp \
   compute_reserved_memory.hpp \
   memory_initialization_writer.hpp \
   memory_initialization_writer_base.hpp \
   memory_initialization_c_writer.hpp \
   minimal_interface_testbench.hpp \
   SimulationInformation.hpp \
   test_vector_parser.hpp \
   testbench_generation.hpp \
   testbench_generation_base_step.hpp \
   testbench_memory_allocation.hpp \
   wishbone_interface_testbench.hpp

lib_simulation_la_SOURCES = \
   c_initialization_flex_lexer.lpp \
   c_initialization_parser.cpp \
   c_initialization_parser_functor.cpp \
   c_initialization_yparser.ypp \
   CTestbenchExecution.cpp \
   compute_reserved_memory.cpp \
   memory_initialization_writer.cpp \
   memory_initialization_writer_base.cpp \
   memory_initialization_c_writer.cpp \
   minimal_interface_testbench.cpp \
   test_vector_parser.cpp \
   testbench_generation.cpp \
   testbench_generation_base_step.cpp \
   testbench_memory_allocation.cpp \
   wishbone_interface_testbench.cpp

if BISON_2_7_OR_GREATER
BUILT_SOURCES = c_initialization_yparser.hpp c_initialization_flex_lexer.cpp c_initialization_yparser.cpp
else
BUILT_SOURCES = c_initialization_yparser.h c_initialization_flex_lexer.cpp c_initialization_yparser.cpp
endif
CLEANFILES = $(BUILT_SOURCES) *.output


