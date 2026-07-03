/*
 *
 *                   _/_/_/    _/_/   _/    _/ _/_/_/    _/_/
 *                  _/   _/ _/    _/ _/_/  _/ _/   _/ _/    _/
 *                 _/_/_/  _/_/_/_/ _/  _/_/ _/   _/ _/_/_/_/
 *                _/      _/    _/ _/    _/ _/   _/ _/    _/
 *               _/      _/    _/ _/    _/ _/_/_/  _/    _/
 *
 *             ***********************************************
 *                              PandA Project
 *                     URL: http://panda.dei.polimi.it
 *                       Politecnico di Milano - DEIB
 *                        System Architectures Group
 *             ***********************************************
 *              Copyright (C) 2004-2026 Politecnico di Milano
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 *
 *   This file is part of the PandA framework.
 *
 *   Licensed under the Apache License, Version 2.0, with BAMBU exceptions (the "License");
 *   you may not use this file except in compliance with the License.
 *   You may obtain a copy of the License at
 *
 *       http://www.apache.org/licenses/LICENSE-2.0
 *
 *   Unless required by applicable law or agreed to in writing, software
 *   distributed under the License is distributed on an "AS IS" BASIS,
 *   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *   See the License for the specific language governing permissions and
 *   limitations under the License.
 *
 */
/**
 * @file BambuParameter.cpp
 * @brief This file contains the implementation of some methods for parameter parsing in Bambu tool
 *
 * @author Christian Pilato <pilato@elet.polimi.it>
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 *
 */
#include "BambuParameter.hpp"

#include "CompilerWrapper.hpp"
#include "HDL_output_mode.hpp"
#include "allocation_constants.hpp"
#include "cdfc_module_binding.hpp"
#include "chaining.hpp"
#include "clique_covering.hpp"
#include "compiler_constants.hpp"
#include "constant_strings.hpp"
#include "cpu_time.hpp"
#include "datapath_creator.hpp"
#include "dbgPrintHelper.hpp"
#include "evaluation_mode.hpp"
#include "fileIO.hpp"
#include "generic_device.hpp"
#include "ir_helper.hpp"
#include "language_writer.hpp"
#include "memory_allocation.hpp"
#include "parametric_list_based.hpp"
#include "parse_technology.hpp"
#include "string_manipulation.hpp"
#include "technology_manager.hpp"
#include "technology_node.hpp"
#include "utility.hpp"

#include "config_HAVE_HOST_PROFILING_BUILT.hpp"
#include "config_HAVE_I386_CLANG16_COMPILER.hpp"
#include "config_HAVE_I386_CLANG19_COMPILER.hpp"
#include "config_HAVE_LIBRARY_CHARACTERIZATION_BUILT.hpp"
#include "config_HAVE_VCD_BUILT.hpp"
#include "config_PANDA_DATA_INSTALLDIR.hpp"
#include "config_PANDA_INCLUDE_INSTALLDIR.hpp"
#include "config_PANDA_LIB_INSTALLDIR.hpp"

#if HAVE_HOST_PROFILING_BUILT
#include "host_profiling.hpp"
#endif

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <getopt.h>
#include <iosfwd>
#include <list>
#include <regex>
#include <string>
#include <thread>
#include <vector>

/// Design Space Exploration
#define OPT_ACCEPT_NONZERO_RETURN 256
#define INPUT_OPT_C_NO_PARSE (1 + OPT_ACCEPT_NONZERO_RETURN)
#define OPT_ACO_FLOW (1 + INPUT_OPT_C_NO_PARSE)
#define OPT_ACO_GENERATIONS (1 + OPT_ACO_FLOW)
#define OPT_ALIGNED_ACCESS_PARAMETER (1 + OPT_ACO_GENERATIONS)
#define OPT_AREA_WEIGHT (1 + OPT_ALIGNED_ACCESS_PARAMETER)
#define OPT_BACKEND_SCRIPT_EXTENSIONS_PARAMETER (1 + OPT_AREA_WEIGHT)
#define OPT_BACKEND_SDC_EXTENSIONS_PARAMETER (1 + OPT_BACKEND_SCRIPT_EXTENSIONS_PARAMETER)
#define OPT_INPUT_CONTEXT_SWITCH (1 + OPT_BACKEND_SDC_EXTENSIONS_PARAMETER)
#define OPT_DISABLE_BITVALUE_IPA (1 + OPT_INPUT_CONTEXT_SWITCH)
#define OPT_BRAM_HIGH_LATENCY (1 + OPT_DISABLE_BITVALUE_IPA)
#define OPT_CHANNELS_NUMBER (1 + OPT_BRAM_HIGH_LATENCY)
#define OPT_CHANNELS_TYPE (1 + OPT_CHANNELS_NUMBER)
#define OPT_CLOCK_PERIOD_RESOURCE_FRACTION (1 + OPT_CHANNELS_TYPE)
#define OPT_DEVICE_NAME (1 + OPT_CLOCK_PERIOD_RESOURCE_FRACTION)
#define OPT_DISABLE_BOUNDED_FUNCTION (1 + OPT_DEVICE_NAME)
#define OPT_ENABLE_FUNCTION_PROXY (1 + OPT_DISABLE_BOUNDED_FUNCTION)
#define OPT_DISABLE_FUNCTION_PROXY (1 + OPT_ENABLE_FUNCTION_PROXY)
#define OPT_CONNECT_IOB (1 + OPT_DISABLE_FUNCTION_PROXY)
#define OPT_DISTRAM_THRESHOLD (1 + OPT_CONNECT_IOB)
#define OPT_DO_NOT_CHAIN_MEMORIES (1 + OPT_DISTRAM_THRESHOLD)
#define OPT_EXPOSE_GLOBALS (1 + OPT_DO_NOT_CHAIN_MEMORIES)
#define OPT_ROM_DUPLICATION (1 + OPT_EXPOSE_GLOBALS)
#define OPT_DO_NOT_USE_ASYNCHRONOUS_MEMORIES (1 + OPT_ROM_DUPLICATION)
#define OPT_DSE (1 + OPT_DO_NOT_USE_ASYNCHRONOUS_MEMORIES)
#define OPT_DSP_ALLOCATION_COEFFICIENT (1 + OPT_DSE)
#define OPT_DSP_MARGIN_COMBINATIONAL (1 + OPT_DSP_ALLOCATION_COEFFICIENT)
#define OPT_DSP_MARGIN_PIPELINED (1 + OPT_DSP_MARGIN_COMBINATIONAL)
#define OPT_DSP_FRACTURING (1 + OPT_DSP_MARGIN_PIPELINED)
#define OPT_DUMP_CONSTRAINTS (1 + OPT_DSP_FRACTURING)
#define OPT_DISCREPANCY (1 + OPT_DUMP_CONSTRAINTS)
#define OPT_DISCREPANCY_FORCE (1 + OPT_DISCREPANCY)
#define OPT_DISCREPANCY_NO_LOAD_POINTERS (1 + OPT_DISCREPANCY_FORCE)
#define OPT_DISCREPANCY_ONLY (1 + OPT_DISCREPANCY_NO_LOAD_POINTERS)
#define OPT_DISCREPANCY_PERMISSIVE_PTRS (1 + OPT_DISCREPANCY_ONLY)
#define INPUT_OPT_DRY_RUN_EVALUATION (1 + OPT_DISCREPANCY_PERMISSIVE_PTRS)
#define OPT_ENABLE_IOB (1 + INPUT_OPT_DRY_RUN_EVALUATION)
#define OPT_EVALUATION (1 + OPT_ENABLE_IOB)
#define OPT_EVALUATION_MODE (1 + OPT_EVALUATION)
#define OPT_EXPERIMENTAL_SETUP (1 + OPT_EVALUATION_MODE)
#define OPT_GENERATE_VCD (1 + OPT_EXPERIMENTAL_SETUP)
#define OPT_GENERATION (1 + OPT_GENERATE_VCD)
#define OPT_HLS_DIV (1 + OPT_GENERATION)
#define OPT_HLS_FPDIV (1 + OPT_HLS_DIV)
#define OPT_HOST_PROFILING (1 + OPT_HLS_FPDIV)
#define INPUT_OPT_FILE_INPUT_DATA (1 + OPT_HOST_PROFILING)
#define OPT_INSERT_MEMORY_PROFILE (1 + INPUT_OPT_FILE_INPUT_DATA)
#define OPT_INSERT_VERIFICATION_OPERATION (1 + OPT_INSERT_MEMORY_PROFILE)
#define OPT_LIST_BASED (1 + OPT_INSERT_VERIFICATION_OPERATION)
#define OPT_LOGICAL_OPTIMIZATION (1 + OPT_LIST_BASED)
#define OPT_MAX_EVALUATIONS (1 + OPT_LOGICAL_OPTIMIZATION)
#define OPT_MAX_INHERITANCE (1 + OPT_MAX_EVALUATIONS)
#define OPT_MAX_SIM_CYCLES (1 + OPT_MAX_INHERITANCE)
#define OPT_MAX_ULP (1 + OPT_MAX_SIM_CYCLES)
#define OPT_MEMORY_MAPPED_TOP (1 + OPT_MAX_ULP)
#define OPT_MEM_DELAY_READ (1 + OPT_MEMORY_MAPPED_TOP)
#define OPT_MEM_DELAY_WRITE (1 + OPT_MEM_DELAY_READ)
#define OPT_TB_QUEUE_SIZE (1 + OPT_MEM_DELAY_WRITE)
#define OPT_MIN_INHERITANCE (1 + OPT_TB_QUEUE_SIZE)
#define OPT_MOSA_FLOW (1 + OPT_MIN_INHERITANCE)
#define OPT_NO_MIXED_DESIGN (1 + OPT_MOSA_FLOW)
#define OPT_NUM_ACCELERATORS (1 + OPT_NO_MIXED_DESIGN)
#define OPT_PARALLEL_CONTROLLER (1 + OPT_NUM_ACCELERATORS)
#define OPT_PERIOD_CLOCK (1 + OPT_PARALLEL_CONTROLLER)
#define OPT_CLOCK_NAME (1 + OPT_PERIOD_CLOCK)
#define OPT_RESET_NAME (1 + OPT_CLOCK_NAME)
#define OPT_START_NAME (1 + OPT_RESET_NAME)
#define OPT_DONE_NAME (1 + OPT_START_NAME)
#define OPT_IDLE_NAME (1 + OPT_DONE_NAME)
#define OPT_POWER_OPTIMIZATION (1 + OPT_IDLE_NAME)
#define OPT_PRETTY_PRINT (1 + OPT_POWER_OPTIMIZATION)
#define OPT_REGISTER_ALLOCATION (1 + OPT_PRETTY_PRINT)
#define OPT_REGISTERED_INPUTS (1 + OPT_REGISTER_ALLOCATION)
#define OPT_FSM_ENCODING (1 + OPT_REGISTERED_INPUTS)
#define OPT_RESET_TYPE (1 + OPT_FSM_ENCODING)
#define OPT_RESET_LEVEL (1 + OPT_RESET_TYPE)
#define OPT_DISABLE_REG_INIT_VALUE (1 + OPT_RESET_LEVEL)
#define OPT_SCHEDULING_MUX_MARGINS (1 + OPT_DISABLE_REG_INIT_VALUE)
#define OPT_USE_ALUS (1 + OPT_SCHEDULING_MUX_MARGINS)
#define OPT_SERIALIZE_MEMORY_ACCESSES (1 + OPT_USE_ALUS)
#define OPT_SIMULATE (1 + OPT_SERIALIZE_MEMORY_ACCESSES)
#define OPT_SKIP_PIPE_PARAMETER (1 + OPT_SIMULATE)
#define OPT_FP_SUB (1 + OPT_SKIP_PIPE_PARAMETER)
#define OPT_FP_RND (1 + OPT_FP_SUB)
#define OPT_FP_EXC (1 + OPT_FP_RND)
#define INPUT_OPT_TEST_MULTIPLE_NON_DETERMINISTIC_FLOWS (1 + OPT_FP_EXC)
#define INPUT_OPT_TEST_SINGLE_NON_DETERMINISTIC_FLOW (1 + INPUT_OPT_TEST_MULTIPLE_NON_DETERMINISTIC_FLOWS)
#define OPT_TESTBENCH (1 + INPUT_OPT_TEST_SINGLE_NON_DETERMINISTIC_FLOW)
#define OPT_TESTBENCH_ARGV (1 + OPT_TESTBENCH)
#define OPT_TESTBENCH_PARAM_SIZE (1 + OPT_TESTBENCH_ARGV)
#define OPT_TESTBENCH_MAP_MODE (1 + OPT_TESTBENCH_PARAM_SIZE)
#define OPT_TB_EXTRA_CC_OPTIONS (1 + OPT_TESTBENCH_MAP_MODE)
#define OPT_TIME_WEIGHT (1 + OPT_TB_EXTRA_CC_OPTIONS)
#define OPT_TIMING_MODEL (1 + OPT_TIME_WEIGHT)
#define OPT_TIMING_VIOLATION (1 + OPT_TIMING_MODEL)
#define OPT_TOP_FNAME (1 + OPT_TIMING_VIOLATION)
#define OPT_TOP_RTLDESIGN_NAME (1 + OPT_TOP_FNAME)
#define OPT_UNALIGNED_ACCESS_PARAMETER (1 + OPT_TOP_RTLDESIGN_NAME)
#define OPT_VHDL_LIBRARY_PARAMETER (1 + OPT_UNALIGNED_ACCESS_PARAMETER)
#define OPT_XML_CONFIG (1 + OPT_VHDL_LIBRARY_PARAMETER)
#define OPT_RANGE_ANALYSIS_MODE (1 + OPT_XML_CONFIG)
#define OPT_FP_FORMAT (1 + OPT_RANGE_ANALYSIS_MODE)
#define OPT_FP_FORMAT_PROPAGATE (1 + OPT_FP_FORMAT)
#define OPT_FP_FORMAT_INTERFACE (1 + OPT_FP_FORMAT_PROPAGATE)
#define OPT_PARALLEL_BACKEND (1 + OPT_FP_FORMAT_INTERFACE)
#define OPT_ARCHITECTURE_XML (1 + OPT_PARALLEL_BACKEND)
#define OPT_LATTICE_ROOT (1 + OPT_ARCHITECTURE_XML)
#define OPT_XILINX_ROOT (1 + OPT_LATTICE_ROOT)
#define OPT_MENTOR_ROOT (1 + OPT_XILINX_ROOT)
#define OPT_MENTOR_OPTIMIZER (1 + OPT_MENTOR_ROOT)
#define OPT_SYNOPSYS_VCS_ROOT (1 + OPT_MENTOR_OPTIMIZER)
#define OPT_ALTERA_ROOT (1 + OPT_SYNOPSYS_VCS_ROOT)
#define OPT_NANOXPLORE_ROOT (1 + OPT_ALTERA_ROOT)
#define OPT_SHARED_INPUT_REGISTERS (1 + OPT_NANOXPLORE_ROOT)
#define OPT_INLINE_FUNCTIONS (1 + OPT_SHARED_INPUT_REGISTERS)
#define OPT_AXI_BURST_TYPE (1 + OPT_INLINE_FUNCTIONS)
#define OPT_NOC_PROFILING (1 + OPT_AXI_BURST_TYPE)
#define OPT_GENERATE_COMPONENTS (1 + OPT_NOC_PROFILING)
#define OPT_BUS_PIPELINED (1 + OPT_GENERATE_COMPONENTS)
#define OPT_BUS_ARBITER_TYPE (1 + OPT_BUS_PIPELINED)
#define OPT_BUS_ARCHITECTURE (1 + OPT_BUS_ARBITER_TYPE)

/// constant correspond to the "parametric list based option"
#define PAR_LIST_BASED_OPT "parametric-list-based"

void BambuParameter::PrintHelp(std::ostream& os) const
{
   os << "Usage:\n"
      << "       bambu [Options] <source_file> [<constraints_file>] [<technology_file>]\n\n"
      << "Options:\n\n";
   PrintGeneralOptionsUsage(os);
   PrintOutputOptionsUsage(os);
   os << "    --pretty-print=<file>.c\n"
      << "        C-based pretty print of the internal IRx\n\n"
      << "    --writer,-w<language>\n"
      << "        Output RTL language:\n"
      << "            V - Verilog (default)\n"
      << "            H - VHDL\n\n"
      << "    --no-mixed-design\n"
      << "        Avoid mixed output RTL language designs.\n\n"
      << "    --generate-tb=<file>\n"
      << "        Generate testbench using the given files.\n"
      << "        <file> must be a valid testbench XML file or a C/C++ file specifying\n"
      << "        a main function calling the top-level interface. (May be repeated)\n\n"
      << "    --generate-tb=<elf>\n"
      << "        Generate testbench environment using <elf> as system simulation.\n"
      << "        <elf> must be an executable that dynamically loads the synthesized\n"
      << "        top-function symbol.\n\n"
      << "    --tb-extra-cc-options=<string>\n"
      << "        Specify custom extra options to the compiler for testbench compilation only.\n\n"
      << "    --tb-arg=<arg>\n"
      << "        Passes <arg> to the testbench main function as an argument.\n"
      << "        The option may be repeated to pass multiple arguments in order.\n\n"
      << "    --tb-param-size=<param_name>:<byte_size>\n"
      << "        A comma-separated list of pairs representing a pointer parameter name and\n"
      << "        the size for the related memory space.\n\n"
      << "    --tb-memory-mapping=<arg>\n"
      << "        Testbench memory mapping mode:\n"
      << "            DEVICE - Emulate host/device memory mapping (default)\n"
      << "            SHARED - Emulate shared memory space between host and device\n"
      << "                     (BEAWARE: no memory integrity checks in shared mode)\n\n"
      << "    --top-fname=<fun_name>\n"
      << "        Define the top function to be synthesized. (default=main)\n\n"
      << "    --top-rtldesign-name=<top_name>\n"
      << "        Define the top module name for the RTL backend.\n\n"
      << "    --inline-fname=<fun_name>[,<fun_name>]*\n"
      << "        Define functions to be always inlined.\n"
      << "        Automatic inlining is always performed using internal metrics.\n"
      << "        Maximum cost to allow function inlining is defined through\n"
      << "        --bambu-parameter=inline-max-cost=<value>. (default=60)\n\n"
      << "    --file-input-data=<file_list>\n"
      << "        A comma-separated list of input files used by the C specification.\n\n"
      << "    --C-no-parse=<file>\n"
      << "        Specify a comma-separated list of C files used only during the\n"
      << "        co-simulation phase.\n\n"
      << std::endl;

   PrintCCOptionsUsage(os);

   // Target options
   os << "  Target:\n\n"
      << "    --target-file=file, -b<file>\n"
      << "        Specify an XML description of the target device.\n\n"
      << "    --generate-interface=<type>\n"
      << "        Wrap the top level module with an external interface.\n"
      << "        Possible values for <type> and related interfaces:\n"
      << "            MINIMAL  -  minimal interface (default)\n"
      << "            INFER    -  top function is built with an hardware interface inferred from\n"
      << "                        the pragmas or from the top function signature\n"
      << "            WB4      -  WishBone 4 interface\n\n"
      << "    --architecture-xml=<filename>\n"
      << "        User-defined module architecture file.\n\n"
      << "    --memory-mapped-top\n"
      << "        Generate a memory mapped interface for the top level function.\n"
      << "        The start signal and each one of function parameter are mapped to a memory address\n\n"
      << std::endl;

   // HLS options
   os << "  Scheduling:\n\n"
      << "    --parametric-list-based[=<type>]\n"
      << "        Perform priority list-based scheduling. This is the default scheduling algorithm\n"
      << "        in bambu. The optional <type> argument can be used to set options for\n"
      << "        list-based scheduling as follows:\n"
      << "            0 - Dynamic mobility (default)\n"
      << "            1 - Static mobility\n"
      << "            2 - Priority-fixed mobility\n\n"
      << "    --speculative-sdc-scheduling,-s\n"
      << "        Perform scheduling by using speculative SDC.\n"
      << "        The speculative SDC is more conservative, in case \n"
      << "        --bambu-parameter=enable-conservative-sdc=1 is passed.\n\n"
      << "    --pipelining,-p=<func_name>[=<init_interval>][,<func_name>[=<init_interval>]]*\n"
      << "        Perform pipelining of comma separated list of specified functions with optional \n"
      << "        initiation interval (default II=1).\n"
      << "        To pipeline softfloat operators it is possible to specify the __float_<op_name> prefix \n"
      << "        or simply __float to pipeline all softfloat library.\n\n"
      << "    --no-chaining\n"
      << "        Disable chaining optimization.\n\n"
      << std::endl;

   // Binding options
   os << "  Binding:\n\n"
      << "    --register-allocation=<type>\n"
      << "        Set the algorithm used for register allocation. Possible values for the\n"
      << "        <type> argument are the following:\n"
      << "            WEIGHTED_TS         - use weighted clique covering algorithm by\n"
      << "                                  exploiting the Tseng&Siewiorek heuristics\n"
      << "                                  (default)\n"
      << "            BIPARTITE_MATCHING  - use bipartite matching algorithm\n"
      << "            COLORING            - use simple coloring algorithm\n"
      << "            WEIGHTED_COLORING   - use weighted coloring algorithm\n"
      << "            CHORDAL_COLORING    - use chordal coloring algorithm\n"
      << "            TTT_CLIQUE_COVERING - use a weighted clique covering algorithm\n"
      << "            UNIQUE_BINDING      - unique binding algorithm\n"
      << "\n"
      << "    --module-binding=<type>\n"
      << "        Set the algorithm used for module binding. Possible values for the\n"
      << "        <type> argument are one the following:\n"
      << "            TS_WEIGHTED_CLIQUE_COVERING - solve the weighted clique covering problem by\n"
      << "                                          exploiting the Tseng&Siewiorek heuristics\n"
      << "                                          (default)\n"
      << "            BIPARTITE_MATCHING          - solve the weighted clique covering problem\n"
      << "                                          exploiting the bipartite matching approach\n"
      << "            WEIGHTED_COLORING           - solve the weighted clique covering problem\n"
      << "                                          performing a coloring on the conflict graph\n"
      << "            COLORING                    - solve the unweighted clique covering problem\n"
      << "                                          performing a coloring on the conflict graph\n"
      << "            TTT_CLIQUE_COVERING_FAST                    - use Tomita, A. Tanaka, H. Takahashi maxima\n"
      << "                                          weighted cliques heuristic to solve the clique\n"
      << "                                          covering problem\n"
      << "            TTT_CLIQUE_COVERING_FAST2   - use Tomita, A. Tanaka, H. Takahashi maximal\n"
      << "                                          weighted cliques heuristic to incrementally\n"
      << "                                          solve the clique covering problem\n"
      << "            TTT_CLIQUE_COVERING         - use Tomita, A. Tanaka, H. Takahashi maximal\n"
      << "                                          weighted cliques algorithm to solve the clique\n"
      << "                                          covering problem\n"
      << "            TTT_CLIQUE_COVERING2        - use Tomita, A. Tanaka, H. Takahashi maximal\n"
      << "                                          weighted cliques algorithm to incrementally\n"
      << "                                          solve the clique covering problem\n"
      << "            TS_CLIQUE_COVERING          - solve the unweighted clique covering problem\n"
      << "                                          by exploiting the Tseng&Siewiorek heuristic\n"
      << "            UNIQUE                      - use a 1-to-1 binding algorithm\n\n"
      << std::endl;
   os << "    --shared-input-registers\n"
      << "        The module bindings and the register binding try to share more resources by \n"
      << "        sharing the input registers\n\n"
      << std::endl;

   // Memory allocation options
   os << "  Memory allocation:\n\n"
      << "    --xml-memory-allocation=<xml_file_name>\n"
      << "        Specify the file where the XML configuration has been defined.\n\n"
      << "    --memory-allocation-policy=<type>\n"
      << "        Set the policy for memory allocation. Possible values for the <type>\n"
      << "        argument are the following:\n"
      << "            ALL_BRAM           - all objects that need to be stored in memory\n"
      << "                                 are allocated on BRAMs (default),\n"
      << "                                 even the objects allocated outside the accelerator\n "
      << "            LSS                - all local variables, static variables and\n"
      << "                                 strings are allocated on BRAMs\n"
      << "            GSS                - all global variables, static variables and\n"
      << "                                 strings are allocated on BRAMs\n"
      << "            GLSS               - all global and local variables, static variables and\n"
      << "                                 strings are allocated on BRAMs\n"
      << "            NO_BRAM            - all objects that need to be stored in memory\n"
      << "                                 are allocated on an external memory\n"
      << "            EXT_PIPELINED_BRAM - all objects that need to be stored in memory\n"
      << "                                 are allocated on an external pipelined memory\n\n"
      << "   --base-address=address\n"
      << "        Define the starting address for objects allocated externally to the top\n"
      << "        module.\n\n"
      << "   --initial-internal-address=address\n"
      << "        Define the starting address for the objects allocated internally to the\n"
      << "        top module.\n\n"
      << "   --sparse-memory[=on/off]\n"
      << "        Control how the memory allocation happens.\n"
      << "            on - allocate the data in addresses which reduce the decoding logic (default)\n"
      << "           off - allocate the data in a contiguous addresses.\n\n"
      << "   --do-not-use-asynchronous-memories\n"
      << "        Do not add asynchronous memories to the possible set of memories used\n"
      << "        by bambu during the memory allocation step.\n\n"
      << "   --distram-threshold=value\n"
      << "        Define the threshold in bitsize used to infer DISTRIBUTED/ASYNCHRONOUS RAMs (default 256).\n\n"
      << "   --serialize-memory-accesses\n"
      << "        Serialize the memory accesses using the CC virtual use-def chains\n"
      << "        without taking into account any alias analysis information.\n\n"
      << "   --unaligned-access\n"
      << "        Use only memories supporting unaligned accesses.\n\n"
      << "   --aligned-access\n"
      << "        Assume that all accesses are aligned and so only memories supporting aligned\n\n"
      << "        accesses are used.\n\n"
      << "   --do-not-chain-memories\n"
      << "        When enabled LOADs and STOREs will not be chained with other\n"
      << "        operations.\n\n"
      << "   --rom-duplication\n"
      << "        Assume that read-only memories can be duplicated in case timing requires.\n\n"
      << "   --bram-high-latency=[3,4]\n"
      << "        Assume a 'high latency bram'-'faster clock frequency' block RAM memory\n"
      << "        based architectures:\n"
      << "        3 => LOAD(II=1,L=3) STORE(1).\n"
      << "        4 => LOAD(II=1,L=4) STORE(II=1,L=2).\n\n"
      << "   --expose-globals\n"
      << "        All global variables can be accessed from outside the accelerator.\n\n"
      << std::endl;

   // Bus options
   // Memory allocation options
   os << "  External memory bus:\n\n"
      << "   --channels-type=<type>\n"
      << "        Set the type of memory connections.\n"
      << "        Possible values for <type> are:\n"
      << "            MEM_ACC_11 - the accesses to the memory have a single direct\n"
      << "                         connection or a single indirect connection\n"
      << "            MEM_ACC_N1 - the accesses to the memory have n parallel direct\n"
      << "                         connections or a single indirect connection\n"
      << "            MEM_ACC_NN - the accesses to the memory have n parallel direct\n"
      << "                         connections or n parallel indirect connections (default)\n\n"
      << "   --channels-number=<n>\n"
      << "        Define the number of indipendent channels of the memory bus.\n\n"
      << "   --memory-ctrl-type=type\n"
      << "        Define which type of memory controller is used. Possible values for the\n"
      << "        <type> argument are the following:\n"
      << "            D00 - no extra delay (default)\n"
      << "            D10 - 1 clock cycle extra-delay for LOAD, 0 for STORE\n"
      << "            D11 - 1 clock cycle extra-delay for LOAD, 1 for STORE\n"
      << "            D21 - 2 clock cycle extra-delay for LOAD, 1 for STORE\n\n"
      << "   --mem-delay-read=value\n"
      << "        Define the external memory latency when LOAD are performed (default 2).\n\n"
      << "   --mem-delay-write=value\n"
      << "        Define the external memory latency when STORE are performed (default 1).\n\n"
      << "   --bus-pipelined\n"
      << "        Allows concurrent requests on the bus.\n\n"
      << "   --tb-queue-size=value\n"
      << "        Define the maximum number of concurrent requests accepted by the testbench. (default 4).\n\n"
      << "   --data-bus-bitsize=<bitsize>\n"
      << "        Set the bitsize of the external data bus.\n\n"
      << "   --addr-bus-bitsize=<bitsize>\n"
      << "        Set the bitsize of the external address bus.\n\n"
      << "   --bus-architecture=<type>\n"
      << "        Specify the bus partitioning architecture.\n"
      << "        Possible values for <type> are:\n"
      << "            GROUPED_CHANNEL - Each bus partition is restricted to a specific channel.\n"
      << "                              This can improve the accelerator's frequency.\n"
      << "            ALL_TO_ALL      - Each bus partition can access every bus channel. (default)\n\n"
      << "   --bus-arbiter-type=<type>\n"
      << "        Specify the arbiter policy used to handle bus partitions.\n"
      << "        Possible values for <type> are:\n"
      << "            RP - Rightmost priority: the arbiter prioritizes requests from the rightmost source."
         "\n"
      << "            LP - Leftmost priority: the arbiter prioritizes requests from the leftmost source.\n"
      << "            RR - Round robin: the arbiter uses a round-robin policy. (default).\n\n"
      << std::endl;

   // Options for Evaluation of HLS results
   os << "  Evaluation of HLS results:\n\n"
      << "    --simulate\n"
      << "        Simulate the generated design.\n\n"
      << "    --simulator=<type>\n"
      << "        Specify the simulator used in generated simulation scripts:\n"
      << "            CSIM      - Post-scheduling C simulation\n"
      << "            MODELSIM  - Mentor Modelsim\n"
      << "            VCS       - Synopsys VCS simulator\n"
      << "            VERILATOR - Verilator simulator\n"
      << "            XSIM      - Xilinx XSim\n\n"
      << "    --max-sim-cycles=<cycles>\n"
      << "        Specify the maximum number of cycles a HDL simulation may run.\n"
      << "        (default 200000000).\n\n"
      << "    --accept-nonzero-return\n"
      << "        Do not assume that application main must return 0.\n\n"
      << "    --generate-vcd\n"
      << "        Enable .vcd output file generation for waveform visualization (requires\n"
      << "        testbench generation).\n\n"
      << "    --evaluation[=type]\n"
      << "        Perform evaluation of the results.\n"
      << "        The value of 'type' selects the objectives to be evaluated\n"
      << "        If nothing is specified all the following are evaluated\n"
      << "        The 'type' argument can be a string containing any of the following\n"
      << "        strings, separated with commas, without spaces:\n"
      << "            AREA            - Normalized area usage (logic + DSP LUT-equivalent on FPGA backends)\n"
      << "            AREAxTIME       - Normalized area x latency product\n"
      << "            TIME            - Latency for the average computation [ns]\n"
      << "            TOTAL_TIME      - Latency for the whole computation [ns]\n"
      << "            CYCLES          - n. of cycles for the average computation\n"
      << "            TOTAL_CYCLES    - n. of cycles for the whole computation\n"
      << "            BRAMS           - number of BRAMs\n"
      << "            DRAMS           - number of DRAMs\n"
      << "            CLOCK_SLACK     - Slack between actual and required clock period\n"
      << "            DSPS            - number of DSPs\n"
      << "            FREQUENCY       - Maximum target frequency\n"
      << "            PERIOD          - Actual clock period\n"
      << "            REGISTERS       - number of registers\n"
      << "\n"
      << std::endl;

   // RTL synthesis options
   os << "  RTL synthesis:\n\n"
      << "    --clock-name=id\n"
      << "        Specify the clock signal name of the top interface (default = clock).\n\n"
      << "    --reset-name=id\n"
      << "        Specify the reset signal name of the top interface (default = reset).\n\n"
      << "    --start-name=id\n"
      << "        Specify the start signal name of the top interface (default = start_port).\n\n"
      << "    --done-name=id\n"
      << "        Specify the done signal name of the top interface (default = done_port).\n\n"
      << "    --idle-name=id\n"
      << "        Specify the idle signal name of the top interface (default = idle_port).\n\n"
      << "    --clock-period=value\n"
      << "        Specify the period of the clock signal (default = 10ns).\n\n"
      << "    --backend-script-extensions=file\n"
      << "        Specify a file that will be included in the backend specific synthesis\n"
      << "        scripts.\n\n"
      << "    --backend-sdc-extensions=file\n"
      << "        Specify a file that will be included in the Synopsys Design Constraints\n"
      << "        file (SDC).\n\n"
      << "    --VHDL-library=libraryname\n"
      << "        Specify the library in which the VHDL generated files are compiled.\n\n"
      << "    --device-name=value\n"
      << "        Specify the name of the device. Three different cases are foreseen:\n"
      << "            - Xilinx:  a comma separated string specifying device, speed grade\n"
      << "                       and package (e.g.,: \"xc7z020,-1,clg484\")\n"
      << "            - Altera:  a string defining the device string (e.g. EP2C70F896C6)\n"
      << "            - Lattice: a string defining the device string (e.g.\n"
      << "                       LFE5U85F8BG756C)\n"
      << "            - NanoXplore: a string defining the device string (e.g. nx2h540tsc))\n\n"
      << "    --power-optimization\n"
      << "        Enable Xilinx power based optimization (default no).\n\n"
      << "    --connect-iob\n"
      << "        Connect primary input and output ports to IOBs.\n\n"
      << "    --skip-pipe-parameter=<value>\n"
      << "        Used during the allocation of pipelined units. <value> specifies how\n"
      << "        many pipelined units, compliant with the clock period, will be skipped.\n"
      << "        (default=0).\n\n"
      << "    --reset-type=value\n"
      << "        Specify the type of reset:\n"
      << "             no    - use registers without reset (default)\n"
      << "             async - use registers with asynchronous reset\n"
      << "             sync  - use registers with synchronous reset\n\n"
      << "    --reset-level=value\n"
      << "        Specify if the reset is active high or low:\n"
      << "             low   - use registers with active low reset (default)\n"
      << "             high  - use registers with active high reset\n\n"
      << "    --disable-reg-init-value\n"
      << "        Used to remove the INIT value from registers (useful for ASIC designs)\n\n"
      << "    --registered-inputs=value\n"
      << "        Specify if inputs are registered or not:\n"
      << "             auto  - inputs are registered only for proxy functions (default)\n"
      << "             top   - inputs and return are registered only for top and proxy functions\n"
      << "             yes   - all inputs are registered\n"
      << "             no    - none of the inputs is registered\n\n"
      << "    --fsm-encoding=value\n"
      << "             auto    - it depends on the target technology. VVD prefers one encoding\n"
      << "                       while the other are fine with the standard binary encoding. (default)\n"
      << "             one-hot - one hot encoding\n"
      << "             binary  - binary encoding\n\n"
      << "    --cprf=value\n"
      << "        Clock Period Resource Fraction (default = 1.0).\n\n"
      << "    --DSP-allocation-coefficient=value\n"
      << "        During the allocation step the timing of the DSP-based modules is\n"
      << "        multiplied by value (default = 1.0).\n\n"
      << "    --DSP-margin-combinational=value\n"
      << "        Timing of combinational DSP-based modules is multiplied by value.\n"
      << "        (default = 1.0).\n\n"
      << "    --DSP-margin-pipelined=value\n"
      << "        Timing of pipelined DSP-based modules is multiplied by value.\n"
      << "        (default = 1.0).\n\n"
      << "    --DSP-fracturing=[16,32]\n"
      << "        Restructure multiplication by fracturing the computation.\n"
      << "        16 => All multiplications will be decomposed into multiplications with input size not larger than "
         "16.\n"
      << "        32 => All multiplications will be decomposed into multiplications with input size not larger than "
         "32.\n\n"
      << "    --mux-margins=n\n"
      << "        Scheduling reserves a margin corresponding to the delay of n 32 bit\n"
      << "        multiplexers.\n\n"
      << "    --timing-model=value\n"
      << "        Specify the timing model used by HLS:\n"
      << "             EC     - estimate timing overhead of glue logics and connections\n"
      << "                      between resources (default)\n"
      << "             SIMPLE - just consider the resource delay\n\n"
      << "    --experimental-setup=<setup>\n"
      << "        Specify the experimental setup. This is a shorthand to set multiple\n"
      << "        options with a single command.\n"
      << "        Available values for <setup> are the following:\n"
      << "             BAMBU-AREA           - this setup implies:\n"
      << "                                    -Os  -D'printf(fmt, ...)='\n"
      << "                                    --memory-allocation-policy=ALL_BRAM\n"
      << "                                    --DSP-allocation-coefficient=1.75\n"
      << "                                    --distram-threshold=256\n"
      << "                                    --enable-function-proxy\n"
      << "             BAMBU-AREA-MP        - this setup implies:\n"
      << "                                    -Os  -D'printf(fmt, ...)='\n"
      << "                                    --channels-type=MEM_ACC_NN\n"
      << "                                    --channels-number=2\n"
      << "                                    --memory-allocation-policy=ALL_BRAM\n"
      << "                                    --DSP-allocation-coefficient=1.75\n"
      << "                                    --distram-threshold=256\n"
      << "                                    --enable-function-proxy\n"
      << "             BAMBU-BALANCED       - this setup implies:\n"
      << "                                    -O2  -D'printf(fmt, ...)='\n"
      << "                                    --channels-type=MEM_ACC_11\n"
      << "                                    --memory-allocation-policy=ALL_BRAM\n"
      << "                                    -fgcse-after-reload  -fipa-cp-clone\n"
      << "                                    -ftree-partial-pre  -funswitch-loops\n"
      << "                                    -finline-functions  -fdisable-tree-bswap\n"
      << "                                    --param max-inline-insns-auto=25\n"
      << "                                    -fno-tree-loop-ivcanon\n"
      << "                                    --distram-threshold=256\n"
      << "                                    -C='*'\n"
      << "                                    --disable-function-proxy\n"
      << "             BAMBU-BALANCED-MP    - (default) this setup implies:\n"
      << "                                    -O2  -D'printf(fmt, ...)='\n"
      << "                                    --channels-type=MEM_ACC_NN\n"
      << "                                    --channels-number=2\n"
      << "                                    --memory-allocation-policy=ALL_BRAM\n"
      << "                                    -fgcse-after-reload  -fipa-cp-clone\n"
      << "                                    -ftree-partial-pre  -funswitch-loops\n"
      << "                                    -finline-functions  -fdisable-tree-bswap\n"
      << "                                    --param max-inline-insns-auto=25\n"
      << "                                    -fno-tree-loop-ivcanon\n"
      << "                                    --disable-function-proxy\n"
      << "                                    -C='*'\n"
      << "                                    --distram-threshold=256\n"
      << "             BAMBU-PERFORMANCE    - this setup implies:\n"
      << "                                    -O3  -D'printf(fmt, ...)='\n"
      << "                                    --memory-allocation-policy=ALL_BRAM\n"
      << "                                    --distram-threshold=512\n"
      << "                                    --disable-function-proxy\n"
      << "             BAMBU-PERFORMANCE-MP - this setup implies:\n"
      << "                                    -O3  -D'printf(fmt, ...)='\n"
      << "                                    --channels-type=MEM_ACC_NN\n"
      << "                                    --channels-number=2\n"
      << "                                    --memory-allocation-policy=ALL_BRAM\n"
      << "                                    --distram-threshold=512\n"
      << "                                    --disable-function-proxy\n"
      << "             BAMBU                - this setup implies:\n"
      << "                                    -O0 --channels-type=MEM_ACC_11\n"
      << "                                    --memory-allocation-policy=LSS\n"
      << "                                    --distram-threshold=256\n"
      << "             BAMBU092             - this setup implies:\n"
      << "                                    -O3  -D'printf(fmt, ...)='\n"
      << "                                    --timing-model=SIMPLE\n"
      << "                                    --DSP-margin-combinational=1.3\n"
      << "                                    --cprf=0.9  -skip-pipe-parameter=1\n"
      << "                                    --channels-type=MEM_ACC_11\n"
      << "                                    --memory-allocation-policy=LSS\n"
      << "                                    --distram-threshold=256\n"
      << "             VVD                  - this setup implies:\n"
      << "                                    -O3  -D'printf(fmt, ...)='\n"
      << "                                    --channels-type=MEM_ACC_NN\n"
      << "                                    --memory-allocation-policy=ALL_BRAM\n"
      << "                                    --distram-threshold=256\n"
      << "                                    --DSP-allocation-coefficient=1.75\n"
      << "                                    --cprf=0.875\n\n"
      << "  Floating-point support:\n\n"
      << "    --max-ulp\n"
      << "        Define the maximal ULP (Unit in the last place, i.e., is the spacing\n"
      << "        between floating-point numbers) accepted.\n\n"
      << "    --fp-subnormal\n"
      << "        Enable the soft-based implementation of floating-point operations with\n"
      << "        subnormals support.\n\n"
      << "    --fp-exception-mode=<ieee|saturation|overflow>\n"
      << "        Set the soft-based exception handling mode:\n"
      << "              ieee    - IEEE754 standard exceptions (default)\n"
      << "           saturation - Inf is replaced with max value, Nan becomes undefined behaviour\n"
      << "            overflow  - Inf and Nan results in undefined behaviour\n\n"
      << "    --fp-rounding-mode=<nearest_even|truncate>\n"
      << "        Set the soft-based rounding handling mode:\n"
      << "           nearest_even - IEEE754 standard rounding mode (default)\n"
      << "              truncate  - No rounding is applied\n\n"
      << "    --fp-format=<func_name>*e<exp_bits>m<frac_bits>b<exp_bias><rnd_mode><exc_mode><?spec><?sign>\n"
      << "        Define arbitrary precision floating-point format by function (use comma separated\n"
      << "        list for multiple definitions). (i.e.: e8m23b-127nihs represent IEEE754 single precision FP)\n"
      << "           func_name - Set arbitrary floating-point format for a specific function (using\n"
      << "                       @ symbol here will resolve to the top function)\n"
      << "                       (Arbitrary floating-point format will apply to specified function\n"
      << "                       only, use --propagate-fp-format to extend it to called functions)\n"
      << "            exp_bits - Number of bits used by the exponent\n"
      << "           frac_bits - Number of bits used by the fractional value\n"
      << "            exp_bias - Bias applied to the unsigned value represented by the exponent bits\n"
      << "            rnd_mode - Rounding mode (exclusive option):\n"
      << "                          n - nearest_even: IEEE754 standard rounding mode\n"
      << "                          t - truncate    : no rounding is applied\n"
      << "            exc_mode - Exception mode (exclusive option):\n"
      << "                          i - ieee      : IEEE754 standard exceptions\n"
      << "                          a - saturation: Inf is replaced with max value, Nan becomes undefined behaviour\n"
      << "                          o - overflow  : Inf and Nan results in undefined behaviour\n"
      << "              spec   - Floating-point specialization string (multiple choice):\n"
      << "                          h - hidden one: IEEE754 standard representation with hidden one\n"
      << "                          s - subnormals: IEEE754 subnormal numbers\n"
      << "              sign   - Static sign representation (exclusive option):\n"
      << "                            - IEEE754 dynamic sign is used if omitted\n"
      << "                          1 - all values are considered as negative numbers\n"
      << "                          0 - all values are considered as positive numbers\n\n"
      << "    --fp-format=inline-math\n"
      << "        The \"inline-math\" flag may be added to fp-format option to force floating-point\n"
      << "        arithmetic operators always inline policy\n\n"
      << "    --fp-format=inline-conversion\n"
      << "        The \"inline-conversion\" flag may be added to fp-format option to force floating-point\n"
      << "        conversion operators always inline policy\n\n"
      << "    --fp-format-interface\n"
      << "        User-defined floating-point format is applied to top interface signature if required\n"
      << "        (default modifies top function body only)\n\n"
      << "    --fp-format-propagate\n"
      << "        Propagate user-defined floating-point format to called function when possible\n\n"
      << "    --hls-div=<method>\n"
      << "        Perform the high-level synthesis of integer division and modulo\n"
      << "        operations starting from a C library based implementation or a HDL component:\n"
      << "            none  - use a HDL based pipelined restoring division\n"
      << "            nr1   - use a C-based non-restoring division with unrolling factor equal to 1\n"
      << "            nr2   - use a C-based non-restoring division with unrolling factor equal to 2\n"
      << "            NR    - use a C-based Newton-Raphson division (default)\n"
      << "            as    - use a C-based align divisor shift dividend method\n\n"
      << "    --hls-fpdiv=<method>\n"
      << "        Select the algorithm used for floating-point division implementation:\n"
      << "            SRT4  - Sweeney, Robertson, Tocher radix-4 (default)\n"
      << "            SRT4U - SRT4 fully unrolled implementation\n"
      << "            G     - Goldschmidt division algorithm\n\n"
      << "    -l<m,m_s,m_newlib,m_musl>\n"
      << "         Select which version of the math library to use for the synthesis:\n"
      << "           m        - Hardware-optimized libm implementation (default)\n"
      << "           m_s      - Hardware-optimized libm implementation with subnormals\n"
      << "           m_newlib - Newlib-derived libm implementation\n"
      << "           m_musl   - Musl-derived libm implementation\n\n"
      << std::endl;
   os << "  Other options:\n\n";
#if HAVE_HOST_PROFILING_BUILT
   os << "    --host-profiling\n"
      << "        Perform host-profiling.\n\n";
#endif
   os << "    --disable-bitvalue-ipa\n"
      << "        Disable inter-procedural bitvalue analysis.\n\n";
   os << "    --enable-function-proxy\n"
      << "        Enable function proxy. May reduce the resource usage.\n\n";
   os << "    --disable-function-proxy\n"
      << "        Disable function proxy. May increase FSMD parallelism.\n\n";
   os << "    --constraints,-C=<func_name>[=<num_resources>][,<func_name>[=<num_resources>]]*\n"
      << "        Perform resource sharing of functions inside the datapath,\n"
      << "        limiting the number of function instances to 'num_resources'.\n"
      << "        Functions are specified as a comma-separated list with an optional\n"
      << "        number of resources. (num_resources is by default equal to 1 when not specified).\n"
      << "        In case <num_resources> is equal to 'u', the function is unconstrained.\n"
      << "        If the first character of func_name is '*', then 'num_resources'\n"
      << "        applies to all functions having as a prefix 'func_name' with '*'\n"
      << "        character removed.\n"
      << "        In case we have -C='*', all functions have 1 instance constraint. \n\n";
   os << "    "
         "--resource-constraints,-c=<resource_name>:<library_name>[=<num_resources>][,<resource_name>:<library_name>[=<"
         "num_resources>]]*\n"
      << "        Perform resource sharing inside the datapath, limiting the number of\n"
      << "        resource instances to 'num_resources'.\n"
      << "        Resources are specified as a comma-separated list of pairs (<resource_name>:<library_name>)\n"
      << "        with an optional number of resources. (num_resources is by default equal to 1\n"
      << "        when not specified).\n\n";
   os << "    --AXI-burst-type=value\n."
      << "        Specify the type of AXI burst when performing single beat operations:\n"
      << "              FIXED        - fixed type burst (default)\n"
      << "              INCREMENTAL  - incremental type burst\n\n";
   os << std::endl;

   // Checks and debugging options
   os << "  Debug options:\n\n"
      << "    --discrepancy\n"
      << "           Performs automated discrepancy analysis between the execution\n"
      << "           of the original source code and the generated HDL (currently\n"
      << "           supports only Verilog). If a mismatch is detected reports\n"
      << "           useful information to the user.\n"
      << "           Uninitialized variables in C are legal, but if they are used\n"
      << "           before initialization in HDL it is possible to obtain X values\n"
      << "           in simulation. This is not necessarily wrong, so these errors\n"
      << "           are not reported by default to avoid reporting false positives.\n"
      << "           If you can guarantee that in your C code there are no\n"
      << "           uninitialized variables and you want the X values in HDL to be\n"
      << "           reported use the option --discrepancy-force-uninitialized.\n"
      << "           Note that the discrepancy of pointers relies on ASAN to properly\n"
      << "           allocate objects in memory. Unfortunately, there is a well-known\n"
      << "           bug on ASAN (https://github.com/google/sanitizers/issues/914)\n"
      << "           when -fsanitize=address is passed to CLANG.\n"
      << "           On some compiler versions this issues has been fixed but since the\n"
      << "           fix has not been upstreamed the bambu option --discrepancy may not\n"
      << "           work. To circumvent the issue, the user may perform the discrepancy\n"
      << "           by adding these two options: --discrepancy --discrepancy-permissive-ptrs.\n\n"
      << "    --discrepancy-force-uninitialized\n"
      << "           Reports errors due to uninitialized values in HDL.\n"
      << "           See the option --discrepancy for details\n\n"
      << "    --discrepancy-no-load-pointers\n"
      << "           Assume that the data loaded from memories in HDL are never used\n"
      << "           to represent addresses, unless they are explicitly assigned to\n"
      << "           pointer variables.\n"
      << "           The discrepancy analysis is able to compare pointers in software\n"
      << "           execution and addresses in hardware. By default all the values\n"
      << "           loaded from memory are treated as if they could contain addresses,\n"
      << "           even if they are integer variables. This is due to the fact that\n"
      << "           C code doing this tricks is valid and actually used in embedded\n"
      << "           systems, but it can lead to imprecise bug reports, because only\n"
      << "           pointers pointing to actual data are checked by the discrepancy\n"
      << "           analysis.\n"
      << "           If you can guarantee that your code always manipulates addresses\n"
      << "           using pointers and never using plain int, then you can use this\n"
      << "           option to get more precise bug reports.\n\n"
      << "    --discrepancy-only=comma,separated,list,of,function,names\n"
      << "           Restricts the discrepancy analysis only to the functions whose\n"
      << "           name is in the list passed as argument.\n\n"
      << "    --discrepancy-permissive-ptrs\n"
      << "           Do not trigger hard errors on pointer variables.\n\n"
      << "    --discrepancy-hw\n"
      << "           Hardware Discrepancy Analysis.\n\n"
      << "    --assert-debug\n"
      << "        Enable assertion debugging performed by Modelsim.\n"
      << "        (Mentor Modelsim should be selected to use this option)\n\n"
      << std::endl;
   // options defining where backend tools could be found
   os << "  Backend configuration:\n\n"
      << "    --generate-components=<mode>\n"
      << "        Select RTL components generation mode (default: separate):\n"
      << "            separate - Generate a HDL file with derived work from input and\n"
      << "                       a HDL file with PandA/Bambu IP library components\n"
      << "            library  - Generate a HDL file with derived work from input and\n"
      << "                       a separate HDL file for each PandA/Bambu IP library component\n"
      << "            mix      - Generate a single HDL file containing all components\n\n"
      << "   --parallel-backend[=N]\n"
      << "        Generate parallel backend synthesis script with the given number of threads if possible (default: "
         "1)\n"
      << "    --mentor-optimizer=<0|1>\n"
      << "        Enable or disable mentor optimizer. (default=enabled)\n\n"
      << "    --altera-root=<path>\n"
      << "        Define Altera tools path. Given path is searched for Quartus.\n"
      << "        (default=/opt/altera:/opt/intelFPGA)\n\n"
      << "    --lattice-root=<path>\n"
      << "        Define Lattice tools path. Given path is searched for Diamond.\n"
      << "        (default=/opt/diamond:/usr/local/diamond)\n\n"
      << "    --mentor-root=<path>\n"
      << "        Define Mentor tools path.\n"
      << "        (default=/opt/mentor)\n\n"
      << "    --synopsys-vcs-root=<path>\n"
      << "        Define Synopsys VCS tools path.\n"
      << "        (default=/opt/synopsys/vcs)\n\n"
      << "    --nanoxplore-root=<path>\n"
      << "        Define NanoXplore tools path. Given directory is searched for NXMap.\n"
      << "        (default=/opt/NanoXplore)\n\n"
      << "    --xilinx-root=<path>\n"
      << "        Define Xilinx tools path. Given directory is searched for both ISE and Vivado\n"
      << "        (default=/opt/Xilinx)\n\n"
      << std::endl;
}

void BambuParameter::PrintProgramName(std::ostream& os) const
{
   os << std::endl;
   os << "********************************************************************************" << std::endl;
   os << "                    ____                  _" << std::endl;
   os << "                   | __ )  __ _ _ __ ___ | |_   _   _" << std::endl;
   os << R"(                   |  _ \ / _` | '_ ` _ \| '_ \| | | |)" << std::endl;
   os << "                   | |_) | (_| | | | | | | |_) | |_| |" << std::endl;
   os << "                   |____/ \\__,_|_| |_| |_|_.__/ \\__,_|" << std::endl;
   os << std::endl;
   os << "********************************************************************************" << std::endl;
   os << "                         High-Level Synthesis Tool" << std::endl;
   os << std::endl;
}

BambuParameter::BambuParameter(const std::string& _program_name, int _argc, char** const _argv)
    : Parameter(_program_name, _argc, _argv)
{
   SetDefaults();
}

int BambuParameter::Exec()
{
   exit_code = PARAMETER_NOTPARSED;

   /// variable used into option parsing
   int opt, option_index;

   // Bambu short option. An option character in this string can be followed by a colon (`:') to indicate that it
   // takes a required argument. If an option character is followed by two colons (`::'), its argument is optional;
   // this is a GNU extension.
   const char* const short_options = COMMON_SHORT_OPTIONS_STRING "o:H:sSc:C::b:w:p::" CC_SHORT_OPTIONS_STRING;

   const struct option long_options[] = {
      COMMON_LONG_OPTIONS,
      /// General options
      {"top-fname", required_argument, nullptr, OPT_TOP_FNAME},
      {"top-rtldesign-name", required_argument, nullptr, OPT_TOP_RTLDESIGN_NAME},
      {"file-input-data", required_argument, nullptr, INPUT_OPT_FILE_INPUT_DATA},
      /// Scheduling options
      {"speculative-sdc-scheduling", no_argument, nullptr, 's'},
      {"pipelining", optional_argument, nullptr, 'p'},
      {"serialize-memory-accesses", no_argument, nullptr, OPT_SERIALIZE_MEMORY_ACCESSES},
      {PAR_LIST_BASED_OPT, optional_argument, nullptr, OPT_LIST_BASED}, // no short option
      {"no-chaining", no_argument, nullptr, 0},
      /// module binding
      {"module-binding", required_argument, nullptr, 0},
      /// register allocation
      {"register-allocation", required_argument, nullptr, OPT_REGISTER_ALLOCATION},
      {"storage-value-insertion", required_argument, nullptr, 0},
      /// Memory allocation
      {"memory-allocation-policy", required_argument, nullptr, 0},
      {"xml-memory-allocation", required_argument, nullptr, 0},
      {"base-address", required_argument, nullptr, 0},
      {"initial-internal-address", required_argument, nullptr, 0},
      {"channels-type", required_argument, nullptr, 0},
      {"channels-number", required_argument, nullptr, 0},
      {"memory-ctrl-type", required_argument, nullptr, 0},
      {"sparse-memory", optional_argument, nullptr, 0},
      {"expose-globals", no_argument, nullptr, OPT_EXPOSE_GLOBALS},
      // parameter based resource constraints
      {"constraints", required_argument, nullptr, 'C'},
      {"resource-constraints", required_argument, nullptr, 'c'},
      /// evaluation options
      {"evaluation", optional_argument, nullptr, OPT_EVALUATION},
      {"timing-violation", no_argument, nullptr, OPT_TIMING_VIOLATION},
      {"assert-debug", no_argument, nullptr, 0},
      {"device-name", required_argument, nullptr, OPT_DEVICE_NAME},
      {"clock-period", required_argument, nullptr, OPT_PERIOD_CLOCK},
      {"clock-name", required_argument, nullptr, OPT_CLOCK_NAME},
      {"reset-name", required_argument, nullptr, OPT_RESET_NAME},
      {"start-name", required_argument, nullptr, OPT_START_NAME},
      {"done-name", required_argument, nullptr, OPT_DONE_NAME},
      {"idle-name", required_argument, nullptr, OPT_IDLE_NAME},
      {"power-optimization", no_argument, nullptr, OPT_POWER_OPTIMIZATION},
      {"connect-iob", no_argument, nullptr, OPT_CONNECT_IOB},
      {"reset-type", required_argument, nullptr, OPT_RESET_TYPE},
      {"reset-level", required_argument, nullptr, OPT_RESET_LEVEL},
      {"disable-reg-init-value", no_argument, nullptr, OPT_DISABLE_REG_INIT_VALUE},
      {"fp-subnormal", no_argument, nullptr, OPT_FP_SUB},
      {"fp-rounding-mode", required_argument, nullptr, OPT_FP_RND},
      {"fp-exception-mode", required_argument, nullptr, OPT_FP_EXC},
      {"hls-div", optional_argument, nullptr, OPT_HLS_DIV},
      {"hls-fpdiv", optional_argument, nullptr, OPT_HLS_FPDIV},
      {"max-ulp", required_argument, nullptr, OPT_MAX_ULP},
      {"skip-pipe-parameter", required_argument, nullptr, OPT_SKIP_PIPE_PARAMETER},
      {"unaligned-access", no_argument, nullptr, OPT_UNALIGNED_ACCESS_PARAMETER},
      {"aligned-access", no_argument, nullptr, OPT_ALIGNED_ACCESS_PARAMETER},
      {"backend-script-extensions", required_argument, nullptr, OPT_BACKEND_SCRIPT_EXTENSIONS_PARAMETER},
      {"backend-sdc-extensions", required_argument, nullptr, OPT_BACKEND_SDC_EXTENSIONS_PARAMETER},
      {"parallel-backend", required_argument, nullptr, OPT_PARALLEL_BACKEND},
      {"VHDL-library", required_argument, nullptr, OPT_VHDL_LIBRARY_PARAMETER},
      {"do-not-use-asynchronous-memories", no_argument, nullptr, OPT_DO_NOT_USE_ASYNCHRONOUS_MEMORIES},
      {"do-not-chain-memories", no_argument, nullptr, OPT_DO_NOT_CHAIN_MEMORIES},
      {"rom-duplication", no_argument, nullptr, OPT_ROM_DUPLICATION},
      {"bram-high-latency", optional_argument, nullptr, OPT_BRAM_HIGH_LATENCY},
      {"cprf", required_argument, nullptr, OPT_CLOCK_PERIOD_RESOURCE_FRACTION},
      {"experimental-setup", required_argument, nullptr, OPT_EXPERIMENTAL_SETUP},
      {"distram-threshold", required_argument, nullptr, OPT_DISTRAM_THRESHOLD},
      {"DSP-allocation-coefficient", required_argument, nullptr, OPT_DSP_ALLOCATION_COEFFICIENT},
      {"DSP-margin-combinational", required_argument, nullptr, OPT_DSP_MARGIN_COMBINATIONAL},
      {"DSP-margin-pipelined", required_argument, nullptr, OPT_DSP_MARGIN_PIPELINED},
      {"DSP-fracturing", optional_argument, nullptr, OPT_DSP_FRACTURING},
      {"mux-margins", required_argument, nullptr, OPT_SCHEDULING_MUX_MARGINS},
      {"use-ALUs", no_argument, nullptr, OPT_USE_ALUS},
      {"timing-model", required_argument, nullptr, OPT_TIMING_MODEL},
      {"registered-inputs", required_argument, nullptr, OPT_REGISTERED_INPUTS},
      {"fsm-encoding", required_argument, nullptr, OPT_FSM_ENCODING},
      /// target options
      {"target-file", required_argument, nullptr, 'b'},
      {"export-core", required_argument, nullptr, 0},
      /// Output options
      {"writer", required_argument, nullptr, 'w'},
      {"no-mixed-design", no_argument, nullptr, OPT_NO_MIXED_DESIGN},
      {"pretty-print", required_argument, nullptr, OPT_PRETTY_PRINT},
      {"generate-interface", required_argument, nullptr, 0},
      {"architecture-xml", required_argument, nullptr, OPT_ARCHITECTURE_XML},
      {"data-bus-bitsize", required_argument, nullptr, 0},
      {"addr-bus-bitsize", required_argument, nullptr, 0},
      {"generate-tb", optional_argument, nullptr, OPT_TESTBENCH},
      {"tb-arg", required_argument, nullptr, OPT_TESTBENCH_ARGV},
      {"tb-param-size", required_argument, nullptr, OPT_TESTBENCH_PARAM_SIZE},
      {"tb-memory-mapping", required_argument, nullptr, OPT_TESTBENCH_MAP_MODE},
      {"tb-extra-cc-options", required_argument, nullptr, OPT_TB_EXTRA_CC_OPTIONS},
      {"max-sim-cycles", required_argument, nullptr, OPT_MAX_SIM_CYCLES},
      {"generate-vcd", no_argument, nullptr, OPT_GENERATE_VCD},
      {"simulate", no_argument, nullptr, OPT_SIMULATE},
      {"simulator", required_argument, nullptr, 0},
      {"enable-function-proxy", no_argument, nullptr, OPT_ENABLE_FUNCTION_PROXY},
      {"disable-function-proxy", no_argument, nullptr, OPT_DISABLE_FUNCTION_PROXY},
      {"disable-bounded-function", no_argument, nullptr, OPT_DISABLE_BOUNDED_FUNCTION},
      {"memory-mapped-top", no_argument, nullptr, OPT_MEMORY_MAPPED_TOP},
      {"mem-delay-read", required_argument, nullptr, OPT_MEM_DELAY_READ},
      {"mem-delay-write", required_argument, nullptr, OPT_MEM_DELAY_WRITE},
      {"tb-queue-size", required_argument, nullptr, OPT_TB_QUEUE_SIZE},
      {"host-profiling", no_argument, nullptr, OPT_HOST_PROFILING},
      {"disable-bitvalue-ipa", no_argument, nullptr, OPT_DISABLE_BITVALUE_IPA},
      {"discrepancy", no_argument, nullptr, OPT_DISCREPANCY},
      {"discrepancy-force-uninitialized", no_argument, nullptr, OPT_DISCREPANCY_FORCE},
      {"discrepancy-no-load-pointers", no_argument, nullptr, OPT_DISCREPANCY_NO_LOAD_POINTERS},
      {"discrepancy-only", required_argument, nullptr, OPT_DISCREPANCY_ONLY},
      {"discrepancy-permissive-ptrs", no_argument, nullptr, OPT_DISCREPANCY_PERMISSIVE_PTRS},
      {"range-analysis-mode", optional_argument, nullptr, OPT_RANGE_ANALYSIS_MODE},
      {"fp-format", optional_argument, nullptr, OPT_FP_FORMAT},
      {"fp-format-propagate", optional_argument, nullptr, OPT_FP_FORMAT_PROPAGATE},
      {"fp-format-interface", optional_argument, nullptr, OPT_FP_FORMAT_INTERFACE},
      {"context_switch", optional_argument, nullptr, OPT_INPUT_CONTEXT_SWITCH},
      {"bus-pipelined", optional_argument, nullptr, OPT_BUS_PIPELINED},
      {"bus-arbiter-type", optional_argument, nullptr, OPT_BUS_ARBITER_TYPE},
      {"bus-architecture", optional_argument, nullptr, OPT_BUS_ARCHITECTURE},
      {"AXI-burst-type", optional_argument, nullptr, OPT_AXI_BURST_TYPE},
      {"C-no-parse", required_argument, nullptr, INPUT_OPT_C_NO_PARSE},
      {"accept-nonzero-return", no_argument, nullptr, OPT_ACCEPT_NONZERO_RETURN},
#if !HAVE_UNORDERED
#ifndef NDEBUG
      {"test-multiple-non-deterministic-flows", required_argument, nullptr,
       INPUT_OPT_TEST_MULTIPLE_NON_DETERMINISTIC_FLOWS},
      {"test-single-non-deterministic-flow", required_argument, nullptr, INPUT_OPT_TEST_SINGLE_NON_DETERMINISTIC_FLOW},
#endif
#endif
      {"dry-run-evaluation", no_argument, nullptr, INPUT_OPT_DRY_RUN_EVALUATION},
      {"altera-root", optional_argument, nullptr, OPT_ALTERA_ROOT},
      {"lattice-root", optional_argument, nullptr, OPT_LATTICE_ROOT},
      {"mentor-root", optional_argument, nullptr, OPT_MENTOR_ROOT},
      {"mentor-optimizer", optional_argument, nullptr, OPT_MENTOR_OPTIMIZER},
      {"synopsys-vcs-root", optional_argument, nullptr, OPT_SYNOPSYS_VCS_ROOT},
      {"nanoxplore-root", optional_argument, nullptr, OPT_NANOXPLORE_ROOT},
      {"xilinx-root", optional_argument, nullptr, OPT_XILINX_ROOT},
      {"shared-input-registers", no_argument, nullptr, OPT_SHARED_INPUT_REGISTERS},
      {"inline-fname", required_argument, nullptr, OPT_INLINE_FUNCTIONS},
      {"noc-profiling", optional_argument, nullptr, OPT_NOC_PROFILING},
      {"generate-components", optional_argument, nullptr, OPT_GENERATE_COMPONENTS},
      CC_LONG_OPTIONS,
      {nullptr, 0, nullptr, 0}
   };

   if(argc == 1) // Bambu called without arguments, it simple prints help message
   {
      PrintUsage(std::cout);
      return EXIT_SUCCESS;
   }

   while((opt = getopt_long(argc, argv, short_options, long_options, &option_index)) != -1)
   {
      switch(opt)
      {
         /// general options
         case OPT_TOP_FNAME:
         {
            const auto top_fname = string_to_container<std::vector<std::string>>(std::string(optarg), ",");
            setOption(OPT_top_functions_names, container_to_string(top_fname, STR_CST_string_separator));
            break;
         }
         case OPT_TOP_RTLDESIGN_NAME:
         {
            setOption(OPT_top_design_name, optarg);
            break;
         }
         case 'o':
            setOption(OPT_output_file, std::string(optarg));
            break;
         case 'S':
         {
            setOption(OPT_serialize_output, true);
            break;
         }
         case INPUT_OPT_FILE_INPUT_DATA:
         {
            const auto in_files = string_to_container<std::vector<std::filesystem::path>>(optarg, ",");
            for(const auto& in_file : in_files)
            {
               const auto local_file = in_file.filename();
               if(!std::filesystem::exists(local_file))
               {
                  std::filesystem::create_symlink(in_file.lexically_normal(), local_file);
               }
            }
            break;
         }
         case OPT_LIST_BASED: // enable list based scheduling
         {
            setOption(OPT_scheduling_algorithm, HLSFlowStep_Type::LIST_BASED_SCHEDULING);
            if(optarg)
            {
               setOption(OPT_scheduling_priority, optarg);
            }
            break;
         }
         /// binding options
         // register
         case OPT_REGISTER_ALLOCATION: // enable register allocation
         {
            if(std::string(optarg) == "COLORING")
            {
               setOption(OPT_register_allocation_algorithm, HLSFlowStep_Type::COLORING_REGISTER_BINDING);
            }
            else if(std::string(optarg) == "CHORDAL_COLORING")
            {
               setOption(OPT_register_allocation_algorithm, HLSFlowStep_Type::CHORDAL_COLORING_REGISTER_BINDING);
            }
            else if(std::string(optarg) == "WEIGHTED_COLORING")
            {
               setOption(OPT_register_allocation_algorithm, HLSFlowStep_Type::WEIGHTED_CLIQUE_REGISTER_BINDING);
               setOption(OPT_weighted_clique_register_algorithm, CliqueCovering_Algorithm::WEIGHTED_COLORING);
            }
            else if(std::string(optarg) == "BIPARTITE_MATCHING")
            {
               setOption(OPT_register_allocation_algorithm, HLSFlowStep_Type::WEIGHTED_CLIQUE_REGISTER_BINDING);
               setOption(OPT_weighted_clique_register_algorithm, CliqueCovering_Algorithm::BIPARTITE_MATCHING);
            }
            else if(std::string(optarg) == "TTT_CLIQUE_COVERING")
            {
               setOption(OPT_register_allocation_algorithm, HLSFlowStep_Type::WEIGHTED_CLIQUE_REGISTER_BINDING);
               setOption(OPT_weighted_clique_register_algorithm, CliqueCovering_Algorithm::TTT_CLIQUE_COVERING);
            }
            else if(std::string(optarg) == "WEIGHTED_TS")
            {
               setOption(OPT_register_allocation_algorithm, HLSFlowStep_Type::WEIGHTED_CLIQUE_REGISTER_BINDING);
               setOption(OPT_weighted_clique_register_algorithm, CliqueCovering_Algorithm::TS_WEIGHTED_CLIQUE_COVERING);
            }
            else if(std::string(optarg) == "UNIQUE_BINDING")
            {
               setOption(OPT_register_allocation_algorithm, HLSFlowStep_Type::UNIQUE_REGISTER_BINDING);
            }
            else
            {
               THROW_ERROR_USAGE("BadParameters: invalid --register-allocation value '" + std::string(optarg) +
                                 "'. Supported values are: WEIGHTED_TS, BIPARTITE_MATCHING, COLORING, "
                                 "WEIGHTED_COLORING, CHORDAL_COLORING, TTT_CLIQUE_COVERING, UNIQUE_BINDING.");
            }
            break;
         }
         case OPT_TIMING_VIOLATION:
         {
            setOption(OPT_timing_violation_abort, true);
            break;
         }
         // parameter based function constraints
         case 'C':
         {
            if(optarg)
            {
               appendOption(OPT_function_constraints, optarg, ",");
            }
            else
            {
               THROW_ERROR_USAGE("BadParameters: missing or invalid argument for -C/--constraints. "
                                 "Expected format: <func_name>[=<num_resources>][,<func_name>[=<num_resources>]]* "
                                 "(example: -C='foo=2,bar=u').");
            }
            break;
         }
         case 'c':
         {
            if(optarg)
            {
               appendOption(OPT_resource_constraints, optarg, ",");
            }
            else
            {
               THROW_ERROR_USAGE("BadParameters: missing or invalid argument for -c/--resource-constraints. "
                                 "Expected format: <resource_name>:<library_name>[=<num_resources>]"
                                 "[,<resource_name>:<library_name>[=<num_resources>]]* "
                                 "(example: -c='ui_add_node_FU:STD_FU=2,ui_mul_node_FU:STD_FU=u').");
            }
            break;
         }
         /// target options
         case 'b':
         {
            setOption(OPT_target_device_file, optarg);
            break;
         }
         /// evaluation
         case OPT_EVALUATION:
         {
            // set OPT_evaluation, because the evaluation has to be performed
            setOption(OPT_evaluation, true);
            /*
             * check if OPT_evaluation_mode has already been decided (for
             * example with OPT_EVALUATION_MODE). In case it's already set, we
             * don't overwrite it since OPT_EVALUATION is meant to set the
             * objectives, not the mode, hence the mode set from other options
             * has precedence
             */
            if(getOption<EvaluationMode::evaluation_mode>(OPT_evaluation_mode) == EvaluationMode::NONE)
            {
               setOption(OPT_evaluation_mode, EvaluationMode::BACKEND);
            }

            auto objectives = getOption<CustomSet<std::string>>(OPT_evaluation_objectives);
            if(optarg)
            {
               const auto values = string_to_container<std::vector<std::string>>(optarg, ",");
               objectives.insert(values.begin(), values.end());
            }
            else
            {
#if HAVE_LIBRARY_CHARACTERIZATION_BUILT
               objectives.emplace("AREAxTIME");
               objectives.emplace("AREA");
               objectives.emplace("REGISTERS");
               objectives.emplace("DSPS");
               objectives.emplace("BRAMS");
               objectives.emplace("DRAMS");
               objectives.emplace("PERIOD");
               objectives.emplace("CLOCK_SLACK");
               objectives.emplace("FREQUENCY");
               objectives.emplace("TIME");
               objectives.emplace("TOTAL_TIME");
#endif
               objectives.emplace("CYCLES");
               objectives.emplace("TOTAL_CYCLES");
            }
            setOption(OPT_evaluation_objectives, container_to_string(objectives, STR_CST_string_separator));

            const auto has_simulation_objectives =
                objectives.find("TIME") != objectives.end() || objectives.find("TOTAL_TIME") != objectives.end() ||
                objectives.find("CYCLES") != objectives.end() || objectives.find("TOTAL_CYCLES") != objectives.end();
            const auto has_synthesis_objectives =
                objectives.find("AREA") != objectives.end() || objectives.find("AREAxTIME") != objectives.end() ||
                objectives.find("BRAMS") != objectives.end() || objectives.find("DRAMS") != objectives.end() ||
                objectives.find("CLOCK_SLACK") != objectives.end() || objectives.find("DSPS") != objectives.end() ||
                objectives.find("FREQUENCY") != objectives.end() || objectives.find("PERIOD") != objectives.end() ||
                objectives.find("REGISTERS") != objectives.end();

            std::string backend_pipeline;
            if(has_simulation_objectives)
            {
               backend_pipeline = "<simulation>";
            }
            if(has_synthesis_objectives)
            {
               backend_pipeline =
                   backend_pipeline.empty() ? "synthesis" : (backend_pipeline + STR_CST_string_separator "synthesis");
            }
            else
            {
               backend_pipeline =
                   backend_pipeline.empty() ? "#synthesis" : (backend_pipeline + STR_CST_string_separator "#synthesis");
            }
            setOption(OPT_backend_pipeline, backend_pipeline);
            break;
         }
         case OPT_SIMULATE:
         {
            setOption(OPT_evaluation, true);
            setOption(OPT_evaluation_mode, EvaluationMode::BACKEND);

            auto objectives = getOption<CustomSet<std::string>>(OPT_evaluation_objectives);
            if(objectives.empty())
            {
               setOption(OPT_evaluation_objectives, "CYCLES");
            }
            else
            {
               objectives.emplace("CYCLES");
               setOption(OPT_evaluation_objectives, container_to_string(objectives, STR_CST_string_separator));
            }
            auto backend_pipeline = getOption<std::string>(OPT_backend_pipeline);
            if(backend_pipeline.find("simulation") == std::string::npos)
            {
               backend_pipeline = backend_pipeline.empty() ?
                                      "<simulation>" :
                                      ("<simulation>" STR_CST_string_separator + backend_pipeline);
            }
            else if(backend_pipeline.find("#<simulation>") != std::string::npos)
            {
               boost::replace_all(backend_pipeline, "#<simulation>", "<simulation>");
            }
            setOption(OPT_backend_pipeline, backend_pipeline);
            break;
         }
         case OPT_DEVICE_NAME:
         {
            auto values = string_to_container<std::vector<std::string>>(optarg, ",");
            setOption("device_name", "");
            setOption("device_speed", "");
            setOption("device_package", "");
            if(values.size() == 1)
            {
               setOption(OPT_device_string, values[0]);
            }
            else if(values.size() == 3)
            {
               setOption("device_name", values[0]);
               setOption("device_speed", values[1]);
               setOption("device_package", values[2]);
            }
            else
            {
               THROW_ERROR_USAGE("BadParameters: malformed --device-name value '" + std::string(optarg) +
                                 "'. Expected either a single token (<device_string>) or exactly three comma-separated "
                                 "fields (<name>,<speed>,<package>). "
                                 "Examples: --device-name=xc7z020-1clg484 or --device-name=xc7z020,1,clg484.");
            }
            break;
         }
         case OPT_PERIOD_CLOCK:
         {
            setOption(OPT_clock_period, optarg);
            break;
         }
         case OPT_CLOCK_NAME:
         {
            setOption(OPT_clock_name, optarg);
            break;
         }
         case OPT_RESET_NAME:
         {
            setOption(OPT_reset_name, optarg);
            break;
         }
         case OPT_START_NAME:
         {
            setOption(OPT_start_name, optarg);
            break;
         }
         case OPT_DONE_NAME:
         {
            setOption(OPT_done_name, optarg);
            break;
         }
         case OPT_IDLE_NAME:
         {
            setOption(OPT_idle_name, optarg);
            break;
         }
         case OPT_POWER_OPTIMIZATION:
         {
            setOption("power_optimization", true);
            break;
         }
         case OPT_CONNECT_IOB:
         {
            setOption(OPT_connect_iob, true);
            THROW_WARNING("Input and output ports will be connected to I/O buffers in the generated design.");
            break;
         }
         case OPT_RESET_TYPE:
         {
            if(std::string(optarg) == "no")
            {
               setOption(OPT_reset_type, std::string(optarg));
            }
            else if(std::string(optarg) == "async")
            {
               setOption(OPT_reset_type, std::string(optarg));
            }
            else if(std::string(optarg) == "sync")
            {
               setOption(OPT_reset_type, std::string(optarg));
            }
            else
            {
               THROW_ERROR_USAGE("BadParameters: invalid --reset-type value '" + std::string(optarg ? optarg : "") +
                                 "'. Supported values are: no, async, sync.");
            }
            break;
         }
         case OPT_RESET_LEVEL:
         {
            if(std::string(optarg) == "high")
            {
               setOption(OPT_reset_level, true);
            }
            else if(std::string(optarg) == "low")
            {
               setOption(OPT_reset_level, false);
            }
            else
            {
               THROW_ERROR_USAGE("BadParameters: invalid --reset-level value '" + std::string(optarg ? optarg : "") +
                                 "'. Supported values are: high, low.");
            }
            break;
         }
         case OPT_DISABLE_REG_INIT_VALUE:
         {
            setOption(OPT_reg_init_value, false);
            break;
         }
         case 's':
         {
            setOption(OPT_scheduling_algorithm, HLSFlowStep_Type::SDC_SCHEDULING);
            break;
         }
         case 'p':
         {
            if(optarg)
            {
               appendOption(OPT_pipelining, optarg, ",");
            }
            else
            {
               appendOption(OPT_pipelining, "@ll", ",");
            }
            break;
         }
         case OPT_SERIALIZE_MEMORY_ACCESSES:
         {
            setOption(OPT_cc_serialize_memory_accesses, true);
            break;
         }
         case OPT_FP_SUB:
         {
            setOption(OPT_fp_subnormal, true);
            break;
         }
         case OPT_FP_RND:
         {
            setOption(OPT_fp_rounding_mode, std::string(optarg));
            break;
         }
         case OPT_FP_EXC:
         {
            setOption(OPT_fp_exception_mode, std::string(optarg));
            break;
         }
         case OPT_HLS_DIV:
         {
            static const std::vector<std::string> supported_algs = {"NR", "nr1", "nr2", "as", "none"};
            const std::string alg(optarg ? optarg : supported_algs.front());
            if(std::find(supported_algs.begin(), supported_algs.end(), alg) == supported_algs.end())
            {
               THROW_ERROR_USAGE("BadParameters: invalid --hls-div algorithm '" + alg +
                                 "'. Supported values are: NR, nr1, nr2, as, none.");
            }
            setOption(OPT_hls_div, alg);
            break;
         }
         case OPT_HLS_FPDIV:
         {
            static const std::vector<std::string> supported_algs = {"SRT4", "SRT4U", "G"};
            const std::string alg(optarg ? optarg : supported_algs.front());
            if(std::find(supported_algs.begin(), supported_algs.end(), alg) == supported_algs.end())
            {
               THROW_ERROR_USAGE("BadParameters: invalid --hls-fpdiv algorithm '" + alg +
                                 "'. Supported values are: SRT4, SRT4U, G.");
            }
            setOption(OPT_hls_fpdiv, alg);
            break;
         }
         case OPT_CLOCK_PERIOD_RESOURCE_FRACTION:
         {
            setOption(OPT_clock_period_resource_fraction, optarg);
            break;
         }
         case OPT_SCHEDULING_MUX_MARGINS:
         {
            setOption(OPT_scheduling_mux_margins, optarg);
            break;
         }
         case OPT_USE_ALUS:
         {
            setOption(OPT_use_ALUs, true);
            break;
         }
         case OPT_TIMING_MODEL:
         {
            if(std::string(optarg) == "SIMPLE")
            {
               setOption(OPT_estimate_logic_and_connections, false);
            }
            else if(std::string(optarg) == "EC")
            {
               setOption(OPT_estimate_logic_and_connections, true);
            }
            else
            {
               THROW_ERROR_USAGE("BadParameters: invalid --timing-model value '" + std::string(optarg ? optarg : "") +
                                 "'. Supported values are: SIMPLE, EC.");
            }
            break;
         }
         case OPT_REGISTERED_INPUTS:
         {
            setOption(OPT_registered_inputs, optarg);
            break;
         }
         case OPT_FSM_ENCODING:
         {
            setOption(OPT_fsm_encoding, optarg);
            break;
         }
         case OPT_MAX_ULP:
         {
            if(std::regex_search(std::string(optarg), std::regex("^\\d+(\\.\\d+)?$")))
            {
               setOption(OPT_max_ulp, optarg);
            }
            else
            {
               THROW_ERROR_USAGE("BadParameters: max ulp value must be a number.");
            }
            break;
         }
         case OPT_SKIP_PIPE_PARAMETER:
         {
            setOption(OPT_skip_pipe_parameter, optarg);
            break;
         }
         case OPT_UNALIGNED_ACCESS_PARAMETER:
         {
            setOption(OPT_unaligned_access, true);
            break;
         }
         case OPT_ALIGNED_ACCESS_PARAMETER:
         {
            setOption(OPT_aligned_access, true);
            break;
         }
         case OPT_BACKEND_SCRIPT_EXTENSIONS_PARAMETER:
         {
            setOption(OPT_backend_script_extensions, optarg);
            break;
         }
         case OPT_BACKEND_SDC_EXTENSIONS_PARAMETER:
         {
            if(!std::filesystem::exists(optarg))
            {
               THROW_ERROR_USAGE("BadParameters: backend sdc extension file does not exists");
            }
            setOption(OPT_backend_sdc_extensions, std::filesystem::canonical(optarg).string());
            break;
         }
         case OPT_PARALLEL_BACKEND:
         {
            unsigned long nthreads = std::strtoul(optarg, nullptr, 10);
            if(!nthreads || nthreads == std::numeric_limits<unsigned long>::max())
            {
               THROW_ERROR_USAGE("BadParameters: parallel backend synthesis thread count invalid");
            }
            setOption(OPT_parallel_backend, nthreads);
            break;
         }
         case OPT_VHDL_LIBRARY_PARAMETER:
         {
            setOption(OPT_VHDL_library, optarg);
            break;
         }
         case OPT_DO_NOT_USE_ASYNCHRONOUS_MEMORIES:
         {
            setOption(OPT_use_asynchronous_memories, false);
            break;
         }
         case OPT_DISTRAM_THRESHOLD:
         {
            setOption(OPT_distram_threshold, optarg);
            break;
         }
         case OPT_DO_NOT_CHAIN_MEMORIES:
         {
            setOption(OPT_do_not_chain_memories, true);
            break;
         }
         case OPT_ROM_DUPLICATION:
         {
            setOption(OPT_rom_duplication, true);
            break;
         }
         case OPT_BRAM_HIGH_LATENCY:
         {
            setOption(OPT_bram_high_latency, "_3");
            if(optarg && std::string(optarg) == "4")
            {
               setOption(OPT_bram_high_latency, "_4");
            }
            break;
         }
         case OPT_EXPOSE_GLOBALS:
         {
            setOption(OPT_expose_globals, true);
            break;
         }
         case OPT_EXPERIMENTAL_SETUP:
         {
            setOption(OPT_experimental_setup, optarg);
            break;
         }
         case OPT_DSP_ALLOCATION_COEFFICIENT:
         {
            setOption(OPT_DSP_allocation_coefficient, optarg);
            break;
         }
         case OPT_DSP_MARGIN_COMBINATIONAL:
         {
            setOption(OPT_DSP_margin_combinational, optarg);
            break;
         }
         case OPT_DSP_MARGIN_PIPELINED:
         {
            setOption(OPT_DSP_margin_pipelined, optarg);
            break;
         }
         case OPT_DSP_FRACTURING:
         {
            setOption(OPT_DSP_fracturing, 16);
            if(optarg && std::string(optarg) == "32")
            {
               setOption(OPT_DSP_fracturing, "32");
            }
            break;
         }
         /// output options
         case 'w':
         {
            if(std::string(optarg) == "V")
            {
               setOption(OPT_writer_language, HDLWriter_Language::VERILOG);
            }
            else if(std::string(optarg) == "H")
            {
               setOption(OPT_writer_language, HDLWriter_Language::VHDL);
            }
            else
            {
               THROW_ERROR_USAGE("BadParameters: invalid --writer/-w value '" + std::string(optarg ? optarg : "") +
                                 "'. Supported values are: V (Verilog), H (VHDL).");
            }
            break;
         }
         case OPT_PRETTY_PRINT:
         {
            std::filesystem::path pp_src(optarg);
            if(!pp_src.has_extension())
            {
               pp_src.append(".c");
            }
            if(pp_src.extension() != ".c")
            {
               THROW_ERROR_USAGE("BadParameters: pretty print output file must have .c extension");
            }
            setOption(OPT_pretty_print, pp_src.string());
            break;
         }
         case OPT_TESTBENCH:
         {
            setOption(OPT_generate_testbench, true);
            if(optarg)
            {
               std::string arg(optarg);
               std::error_code ec;
               if(std::filesystem::exists(arg, ec))
               {
                  appendOption(OPT_testbench_input_file, arg);
                  break;
               }
               appendOption(OPT_testbench_input_string, arg);
            }
            break;
         }
         case OPT_TESTBENCH_ARGV:
         {
            appendOption(OPT_testbench_argv, std::string(optarg), " ");
            break;
         }
         case OPT_TESTBENCH_PARAM_SIZE:
         {
            std::string param_size(optarg);
            if(!std::regex_match(param_size, std::regex("^([\\w\\d]+:\\d+)(,[\\w\\d]+:\\d+)*$")))
            {
               THROW_ERROR_USAGE("BadParameters: invalid --tb-param-size value '" + param_size +
                                 "'. Expected format is <param_name>:<byte_size>[,<param_name>:<byte_size>...], "
                                 "for example 'input:64,output:32'.");
            }
            boost::replace_all(param_size, ",", STR_CST_string_separator);
            appendOption(OPT_testbench_param_size, param_size);
            break;
         }
         case OPT_TESTBENCH_MAP_MODE:
         {
            std::string map_mode(optarg);
            if(map_mode != "DEVICE" && map_mode != "SHARED")
            {
               THROW_ERROR_USAGE("BadParameters: invalid --tb-memory-mapping value '" + map_mode +
                                 "'. Supported values are: DEVICE, SHARED.");
            }
            setOption(OPT_testbench_map_mode, map_mode);
            break;
         }
         case OPT_TB_EXTRA_CC_OPTIONS:
         {
            appendOption(OPT_tb_extra_cc_options, std::string(optarg), " ");
            break;
         }
         case OPT_MAX_SIM_CYCLES:
         {
            setOption(OPT_max_sim_cycles, optarg);
            break;
         }
         case OPT_GENERATE_VCD:
         {
            setOption(OPT_generate_vcd, true);
            break;
         }
         case OPT_ENABLE_FUNCTION_PROXY:
         {
            setOption(OPT_disable_function_proxy, false);
            break;
         }
         case OPT_DISABLE_FUNCTION_PROXY:
         {
            setOption(OPT_disable_function_proxy, true);
            break;
         }
         case OPT_DISABLE_BOUNDED_FUNCTION:
         {
            setOption(OPT_disable_bounded_function, true);
            break;
         }
         case OPT_MEMORY_MAPPED_TOP:
         {
            setOption(OPT_memory_mapped_top, true);
            break;
         }
         case OPT_MEM_DELAY_READ:
         {
            setOption(OPT_mem_delay_read, optarg);
            break;
         }
         case OPT_MEM_DELAY_WRITE:
         {
            setOption(OPT_mem_delay_write, optarg);
            break;
         }
         case OPT_TB_QUEUE_SIZE:
         {
            setOption(OPT_tb_queue_size, optarg);
            break;
         }
#if HAVE_HOST_PROFILING_BUILT
         case OPT_HOST_PROFILING:
         {
            setOption(OPT_profiling_method, HostProfiling_Method::PM_BBP);
            break;
         }
#endif
         case OPT_NO_MIXED_DESIGN:
         {
            setOption<bool>(OPT_mixed_design, false);
            break;
         }
         case OPT_DISABLE_BITVALUE_IPA:
         {
            setOption(OPT_bitvalue_ipa, false);
            break;
         }
         case OPT_DISCREPANCY:
         {
            setOption(OPT_discrepancy, true);
            break;
         }
         case OPT_DISCREPANCY_NO_LOAD_POINTERS:
         {
            setOption(OPT_discrepancy, true);
            setOption(OPT_discrepancy_no_load_pointers, true);
            break;
         }
         case OPT_DISCREPANCY_FORCE:
         {
            setOption(OPT_discrepancy, true);
            setOption(OPT_discrepancy_force, true);
            break;
         }
         case OPT_DISCREPANCY_ONLY:
         {
            setOption(OPT_discrepancy, true);
            auto splitted = string_to_container<std::vector<std::string>>(optarg, " ,");
            for(auto& f : splitted)
            {
               boost::trim(f);
               appendOption(OPT_discrepancy_only, f);
            }
            break;
         }
         case OPT_DISCREPANCY_PERMISSIVE_PTRS:
         {
            setOption(OPT_discrepancy, true);
            setOption(OPT_discrepancy_permissive_ptrs, true);
            break;
         }
         case OPT_RANGE_ANALYSIS_MODE:
         {
            setOption(OPT_range_analysis_mode, optarg);
            break;
         }
         case OPT_FP_FORMAT:
         {
            setOption(OPT_fp_format, optarg);
            break;
         }
         case OPT_FP_FORMAT_PROPAGATE:
         {
            setOption(OPT_fp_format_propagate, true);
            break;
         }
         case OPT_FP_FORMAT_INTERFACE:
         {
            setOption(OPT_fp_format_interface, true);
            break;
         }
         case OPT_NUM_ACCELERATORS:
         {
            auto num_acc = std::stoull(std::string(optarg));
            if((num_acc != 0) && ((num_acc & (num_acc - 1)) == 0))
            {
               setOption(OPT_num_accelerators, std::string(optarg));
            }
            else
            {
               THROW_ERROR_USAGE("Bad parameters: The number of physical accelerator has to be a power of two");
            };
            break;
         }
         case OPT_INPUT_CONTEXT_SWITCH:
         {
            if(optarg)
            {
               const auto num = std::stoull(std::string(optarg));
               if(!num)
               {
                  THROW_ERROR_USAGE("Bad parameters: number of contexts must be a positive integer.");
               }
               setOption(OPT_context_switch, std::string(optarg));
               break;
            }
            setOption(OPT_context_switch, "4");
            break;
         }
         case OPT_BUS_PIPELINED:
         {
            setOption(OPT_bus_pipelined, true);
            break;
         }
         case OPT_BUS_ARBITER_TYPE:
         {
            if(std::string(optarg) == "RP")
            {
               setOption(OPT_bus_arbiter_type, "RP");
            }
            else if(std::string(optarg) == "RR")
            {
               setOption(OPT_bus_arbiter_type, "RR");
            }
            else if(std::string(optarg) == "LP")
            {
               setOption(OPT_bus_arbiter_type, "LP");
            }
            else
            {
               THROW_ERROR_USAGE("BadParameters: invalid --bus-arbiter-type value '" +
                                 std::string(optarg ? optarg : "") + "'. Supported values are: RP, RR, LP.");
            }
            break;
         }
         case OPT_BUS_ARCHITECTURE:
         {
            if(std::string(optarg) == "ALL_TO_ALL")
            {
               setOption(OPT_bus_architecture, "ALL_TO_ALL");
            }
            else if(std::string(optarg) == "GROUPED_CHANNEL")
            {
               setOption(OPT_bus_architecture, "GROUPED_CHANNEL");
            }
            else
            {
               THROW_ERROR_USAGE("BadParameters: invalid --bus-architecture value '" +
                                 std::string(optarg ? optarg : "") +
                                 "'. Supported values are: ALL_TO_ALL, GROUPED_CHANNEL.");
            }
            break;
         }
         case OPT_NOC_PROFILING:
         {
            setOption(OPT_noc_profiling, true);
            break;
         }
         case OPT_AXI_BURST_TYPE:
         {
            std::string burst_type(optarg);
            if(burst_type == "FIXED")
            {
               setOption(OPT_axi_burst_type, 0);
            }
            else if(burst_type == "INCREMENTAL")
            {
               setOption(OPT_axi_burst_type, 1);
            }
            else
            {
               THROW_ERROR_USAGE(
                   "Bad parameters: AXI burst type not recognized. Supported values are FIXED and INCREMENTAL.");
            };
            break;
         }
         case OPT_ACCEPT_NONZERO_RETURN:
         {
            setOption(OPT_no_return_zero, true);
            break;
         }
         case INPUT_OPT_C_NO_PARSE:
         {
            appendOption(OPT_no_parse_files,
                         boost::replace_all_copy(std::string(optarg), ",", STR_CST_string_separator));
            break;
         }
#if !HAVE_UNORDERED
#ifndef NDEBUG
         case INPUT_OPT_TEST_MULTIPLE_NON_DETERMINISTIC_FLOWS:
         {
            setOption(OPT_test_multiple_non_deterministic_flows, std::string(optarg));
            break;
         }
         case INPUT_OPT_TEST_SINGLE_NON_DETERMINISTIC_FLOW:
         {
            setOption(OPT_test_single_non_deterministic_flow, std::string(optarg));
            break;
         }
#endif
#endif
         case INPUT_OPT_DRY_RUN_EVALUATION:
         {
            setOption(OPT_evaluation_mode, EvaluationMode::DRY_RUN);
            break;
         }
         case OPT_ARCHITECTURE_XML:
         {
            std::string XMLfilename(optarg);
            if(!std::filesystem::exists(XMLfilename))
            {
               THROW_ERROR_USAGE("Bad parameters: The file " + XMLfilename +
                                 " passed to --architecture-xml option does not exist");
            }
            setOption(OPT_architecture_xml, XMLfilename);
            break;
         }
         case OPT_ALTERA_ROOT:
         {
            setOption(OPT_altera_root, std::string(optarg));
            break;
         }
         case OPT_LATTICE_ROOT:
         {
            setOption(OPT_lattice_root, std::string(optarg));
            break;
         }
         case OPT_MENTOR_ROOT:
         {
            setOption(OPT_mentor_root, std::string(optarg));
            break;
         }
         case OPT_MENTOR_OPTIMIZER:
         {
            setOption(OPT_mentor_optimizer, static_cast<bool>(std::stoi(optarg)));
            break;
         }
         case OPT_NANOXPLORE_ROOT:
         {
            setOption(OPT_nanoxplore_root, std::string(optarg));
            break;
         }
         case OPT_SYNOPSYS_VCS_ROOT:
         {
            setOption(OPT_synopsys_vcs_root, std::string(optarg));
            break;
         }
         case OPT_XILINX_ROOT:
         {
            setOption(OPT_xilinx_root, std::string(optarg));
            break;
         }
         case OPT_SHARED_INPUT_REGISTERS:
         {
            setOption(OPT_shared_input_registers, true);
            break;
         }
         case OPT_INLINE_FUNCTIONS:
         {
            setOption(OPT_inline_functions, std::string(optarg));
            break;
         }
         case OPT_GENERATE_COMPONENTS:
         {
            std::string mode_str(optarg ? optarg : "");
            HDL_output_mode out_mode = HDL_OUT_WORK_SEPARATE;
            if(mode_str == "mix")
            {
               out_mode = HDL_OUT_MIX;
            }
            else if(mode_str == "library")
            {
               out_mode = HDL_OUT_WORK_LIBRARY;
            }
            else if(mode_str == "separate")
            {
               out_mode = HDL_OUT_WORK_SEPARATE;
            }
            else if(!mode_str.empty())
            {
               THROW_ERROR_USAGE("BadParameters: invalid --generate-components mode '" + mode_str +
                                 "'. Supported values are: separate, library, mix.");
            }
            setOption(OPT_generate_components, out_mode);
            break;
         }
         case 0:
         {
            if(strcmp(long_options[option_index].name, "module-binding") == 0)
            {
               if(auto algo = parseCliqueAlgo(optarg))
               {
                  setOption(OPT_fu_binding_algorithm, HLSFlowStep_Type::CDFC_MODULE_BINDING);
                  setOption(OPT_cdfc_module_binding_algorithm, *algo);
               }
               else if(std::string(optarg) == "UNIQUE")
               {
                  setOption(OPT_fu_binding_algorithm, HLSFlowStep_Type::UNIQUE_MODULE_BINDING);
               }
               else
               {
                  THROW_ERROR_USAGE("BadParameters: invalid --module-binding value '" +
                                    std::string(optarg ? optarg : "") +
                                    "'. Supported values are: COLORING, WEIGHTED_COLORING, "
                                    "TTT_CLIQUE_COVERING, TTT_CLIQUE_COVERING2, "
                                    "TTT_CLIQUE_COVERING_FAST, TTT_CLIQUE_COVERING_FAST2, "
                                    "TS_CLIQUE_COVERING, TS_WEIGHTED_CLIQUE_COVERING, "
                                    "BIPARTITE_MATCHING, UNIQUE.");
               }
               break;
            }
            if(strcmp(long_options[option_index].name, "xml-memory-allocation") == 0)
            {
               setOption(OPT_xml_memory_allocation, std::string(optarg));
               break;
            }
            if(strcmp(long_options[option_index].name, "memory-allocation-policy") == 0)
            {
               if(std::string(optarg) == "LSS")
               {
                  setOption(OPT_memory_allocation_policy, MemoryAllocation_Policy::LSS);
               }
               else if(std::string(optarg) == "GSS")
               {
                  setOption(OPT_memory_allocation_policy, MemoryAllocation_Policy::GSS);
               }
               else if(std::string(optarg) == "GLSS")
               {
                  setOption(OPT_memory_allocation_policy, MemoryAllocation_Policy::GLSS);
               }
               else if(std::string(optarg) == "ALL_BRAM")
               {
                  setOption(OPT_memory_allocation_policy, MemoryAllocation_Policy::ALL_BRAM);
               }
               else if(std::string(optarg) == "NO_BRAM")
               {
                  setOption(OPT_memory_allocation_policy, MemoryAllocation_Policy::NO_BRAM);
               }
               else if(std::string(optarg) == "EXT_PIPELINED_BRAM")
               {
                  setOption(OPT_memory_allocation_policy, MemoryAllocation_Policy::EXT_PIPELINED_BRAM);
               }
               else
               {
                  THROW_ERROR_USAGE("BadParameters: invalid --memory-allocation-policy value '" +
                                    std::string(optarg ? optarg : "") +
                                    "'. Supported values are: ALL_BRAM, LSS, GSS, GLSS, "
                                    "NO_BRAM, EXT_PIPELINED_BRAM.");
               }
               break;
            }
            if(strcmp(long_options[option_index].name, "base-address") == 0)
            {
               setOption(OPT_base_address, optarg);
               break;
            }
            if(strcmp(long_options[option_index].name, "initial-internal-address") == 0)
            {
               setOption(OPT_initial_internal_address, optarg);
               break;
            }
            if(strcmp(long_options[option_index].name, "channels-type") == 0)
            {
               if(std::string(optarg) == CHANNELS_TYPE_MEM_ACC_11)
               {
                  setOption(OPT_channels_type, MemoryAllocation_ChannelsType::MEM_ACC_11);
                  if(!isOption(OPT_channels_number))
                  {
                     setOption(OPT_channels_number, 1);
                  }
               }
               else if(std::string(optarg) == CHANNELS_TYPE_MEM_ACC_N1)
               {
                  setOption(OPT_channels_type, MemoryAllocation_ChannelsType::MEM_ACC_N1);
               }
               else if(std::string(optarg) == CHANNELS_TYPE_MEM_ACC_NN)
               {
                  setOption(OPT_channels_type, MemoryAllocation_ChannelsType::MEM_ACC_NN);
               }
               else
               {
                  THROW_ERROR_USAGE("BadParameters: invalid --channels-type value '" +
                                    std::string(optarg ? optarg : "") +
                                    "'. Supported values are: MEM_ACC_11, MEM_ACC_N1, MEM_ACC_NN.");
               }
               break;
            }
            if(strcmp(long_options[option_index].name, "channels-number") == 0)
            {
               setOption(OPT_channels_number, optarg);
               if(std::string(optarg) == "1" && !isOption(OPT_channels_type))
               {
                  setOption(OPT_channels_type, MemoryAllocation_ChannelsType::MEM_ACC_11);
               }
               break;
            }
            if(strcmp(long_options[option_index].name, "memory-ctrl-type") == 0)
            {
               if(std::string(optarg) == "D00")
               {
                  setOption(OPT_memory_controller_type, optarg);
               }
               else if(std::string(optarg) == "D10")
               {
                  setOption(OPT_memory_controller_type, optarg);
               }
               else if(std::string(optarg) == "D11")
               {
                  setOption(OPT_memory_controller_type, optarg);
               }
               else if(std::string(optarg) == "D21")
               {
                  setOption(OPT_memory_controller_type, optarg);
               }
               else
               {
                  THROW_ERROR_USAGE("BadParameters: invalid --memory-ctrl-type value '" +
                                    std::string(optarg ? optarg : "") + "'. Supported values are: D00, D10, D11, D21.");
               }
               break;
            }
            if(strcmp(long_options[option_index].name, "sparse-memory") == 0)
            {
               if(!optarg || std::string(optarg) == "on")
               {
                  setOption(OPT_sparse_memory, true);
               }
               else if(std::string(optarg) == "off")
               {
                  setOption(OPT_sparse_memory, false);
               }
               else
               {
                  THROW_ERROR_USAGE("BadParameters: invalid --sparse-memory value '" + std::string(optarg) +
                                    "'. Supported values are: on, off. "
                                    "You can also use --sparse-memory without a value (equivalent to on).");
               }
               break;
            }
            if(strcmp(long_options[option_index].name, "assert-debug") == 0)
            {
               setOption(OPT_assert_debug, true);
               break;
            }
            /// scheduling options
            if(strcmp(long_options[option_index].name, "no-chaining") == 0)
            {
               setOption(OPT_chaining, false);
               break;
            }
            if(strcmp(long_options[option_index].name, "generate-interface") == 0)
            {
               setOption(OPT_interface, true);
               if(std::string(optarg) == "MINIMAL")
               {
                  setOption(OPT_interface_type, HLSFlowStep_Type::MINIMAL_INTERFACE_GENERATION);
               }
               else if(std::string(optarg) == "INFER")
               {
                  setOption(OPT_interface_type, HLSFlowStep_Type::INFERRED_INTERFACE_GENERATION);
               }
               else if(std::string(optarg) == "WB4")
               {
                  setOption(OPT_interface_type, HLSFlowStep_Type::WB4_INTERFACE_GENERATION);
                  setOption(OPT_memory_mapped_top, true);
                  setOption(OPT_memory_allocation_policy, MemoryAllocation_Policy::NO_BRAM);
                  setOption(OPT_channels_number, 1);
                  setOption(OPT_channels_type, MemoryAllocation_ChannelsType::MEM_ACC_11);
               }
               else
               {
                  THROW_ERROR_USAGE("BadParameters: invalid --generate-interface value '" +
                                    std::string(optarg ? optarg : "") +
                                    "'. Supported values are: MINIMAL, INFER, WB4.");
               }
               break;
            }
            if(strcmp(long_options[option_index].name, "data-bus-bitsize") == 0)
            {
               setOption(OPT_data_bus_bitsize, std::stoi(optarg));
               break;
            }
            if(strcmp(long_options[option_index].name, "addr-bus-bitsize") == 0)
            {
               setOption(OPT_addr_bus_bitsize, std::stoi(optarg));
               break;
            }
            if(strcmp(long_options[option_index].name, "simulator") == 0)
            {
               setOption(OPT_simulator, std::string(optarg));
               break;
            }
#if(__GNUC__ >= 7)
            [[gnu::fallthrough]];
#endif
         }
         /// other options
         default:
         {
            bool exit_success = false;
            bool res = ManageCCOptions(opt, optarg);
            if(res)
            {
               res = ManageDefaultOptions(opt, optarg, exit_success);
            }
            if(exit_success)
            {
               return EXIT_SUCCESS;
            }
            if(res)
            {
               return PARAMETER_NOTPARSED;
            }
         }
      }
   }

   const auto cat_args = shell_escape_argv(argc, argv);
   setOption(OPT_cat_args, cat_args);

   INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, getOption<int>(OPT_output_level),
                  " ==  Bambu executed with: " + cat_args + "\n");

   while(optind < argc)
   {
      const std::string filename(argv[optind]);
      const auto file_type = GetFileFormat(filename, true);
      if(file_type == Parameters_FileFormat::FF_XML_CON)
      {
         setOption(OPT_constraints_file, filename);
      }
      else if(file_type == Parameters_FileFormat::FF_XML_TEC)
      {
         appendOption(OPT_technology_file, filename);
      }
      else if(file_type == Parameters_FileFormat::FF_C || file_type == Parameters_FileFormat::FF_OBJECTIVEC ||
              file_type == Parameters_FileFormat::FF_CPP || file_type == Parameters_FileFormat::FF_FORTRAN ||
              file_type == Parameters_FileFormat::FF_LLVM || file_type == Parameters_FileFormat::FF_LLVM_CPP)
      {
         appendOption(OPT_input_file, filename);
         setOption(OPT_input_format, file_type);
      }
      else if(file_type == Parameters_FileFormat::FF_RAW ||
              (isOption(OPT_input_format) &&
               getOption<Parameters_FileFormat>(OPT_input_format) == Parameters_FileFormat::FF_RAW))
      {
         appendOption(OPT_input_file, filename);
         setOption(OPT_input_format, Parameters_FileFormat::FF_RAW);
         if(!isOption(OPT_pretty_print))
         {
            setOption(OPT_pretty_print, "_a.c");
         }
      }
      else
      {
         THROW_ERROR_USAGE("BadParameters: unsupported input file '" + std::string(argv[optind]) +
                           "'. Supported auto-detected inputs are C/C++/Objective-C/Fortran sources, "
                           "LLVM IR/bitcode files, and Bambu XML constraint/technology files. "
                           "Use --input-format explicitly for raw input or non-standard extensions.");
      }
      optind++;
   }

   CheckParameters();

   return PARAMETER_PARSED;
}

void BambuParameter::add_experimental_setup_compiler_options(bool kill_printf)
{
   if(kill_printf)
   {
      appendOption(OPT_cc_defines, "printf(fmt, ...)=");
   }
   if(getOption<std::string>(OPT_top_functions_names) == "main")
   {
      THROW_ASSERT(isOption(OPT_input_file), "Input file not specified");
      if(getOption<std::string>(OPT_input_file).find(STR_CST_string_separator) == std::string::npos &&
         !isOption(OPT_top_design_name))
      {
         appendOption(OPT_cc_optimizations, "whole-program");
      }
      if(isOption(OPT_top_design_name))
      {
         appendOption(OPT_cc_optimizations, "no-ipa-cp");
      }
   }
}

void BambuParameter::CheckParameters()
{
   Parameter::CheckParameters();

   setOption(OPT_simulation_output, getOption<std::string>(OPT_output_directory) + "/bambu_time_simulation.txt");
   setOption(OPT_profiling_output, getOption<std::string>(OPT_output_directory) + "/bambu_profiling_simulation.txt");
   // TODO: this is a temporary hack. Before starting anything, the directory HLS_output/simulation/ needs to be
   // removed.
   auto sim_dir = getOption<std::filesystem::path>(OPT_output_hls_directory) / "simulation";
   if(std::filesystem::exists(sim_dir))
   {
      std::filesystem::remove_all(sim_dir);
   }

   if(isOption(OPT_aligned_access) && getOption<bool>(OPT_aligned_access) && isOption(OPT_unaligned_access) &&
      getOption<bool>(OPT_unaligned_access))
   {
      THROW_ERROR_USAGE("Both --unaligned-access and --aligned-access were specified. Use only one of them.");
   }

   if(!isOption(OPT_top_functions_names))
   {
      if(getOption<HLSFlowStep_Type>(OPT_interface_type) == HLSFlowStep_Type::INFERRED_INTERFACE_GENERATION)
      {
         THROW_ERROR_USAGE("Top function name must be specified when interface inference is enabled. "
                           "Set --top-fname=<function_name>.");
      }
      setOption(OPT_top_functions_names, "main");
      THROW_WARNING("Top function name was not specified: main will be set as top");
   }
   if(getOption<std::string>(OPT_top_functions_names) == "main")
   {
      THROW_WARNING("Using 'main' as top function name is strongly discouraged.");
      THROW_WARNING("   Please note that C simulation output may be truncated down to 8-bits.");
   }
   if((isOption(OPT_input_format) &&
       getOption<Parameters_FileFormat>(OPT_input_format) == Parameters_FileFormat::FF_RAW) ||
      getOption<std::string>(OPT_top_functions_names) == "main" ||
      getOption<std::string>(OPT_testbench_map_mode) == "SHARED")
   {
      appendOption(OPT_cc_defines, "BAMBU_SKIP_VERIFICATION");
   }

   const auto sorted_dirs = [](const std::string& parent_dir) {
      std::vector<std::filesystem::path> sorted_paths;
      std::copy_if(std::filesystem::directory_iterator(parent_dir), std::filesystem::directory_iterator(),
                   std::back_inserter(sorted_paths), [](const auto& it) { return std::filesystem::is_directory(it); });
      std::sort(sorted_paths.begin(), sorted_paths.end(), NaturalVersionOrder);
      return sorted_paths;
   };

   const auto altera_dirs = getOption<std::vector<std::string>>(OPT_altera_root);
   removeOption(OPT_altera_root);
   const auto search_quartus = [&](const std::string& dir) {
      if(std::filesystem::exists(dir + "/quartus/bin/quartus_sh"))
      {
         if(system(STR("bash -c \"if [ $(" + dir +
                       "/quartus/bin/quartus_sh --version | grep Version | sed -E 's/Version ([0-9]+).*/\\1/') -lt 14 "
                       "]; then exit 1; else exit 0; fi\" > /dev/null 2>&1")
                       .c_str()))
         {
            setOption(OPT_quartus_13_settings, "export PATH=$PATH:" + dir + "/quartus/bin");
            if(system(STR("bash -c \"" + dir + "/quartus/bin/quartus_sh --help | grep '--64bit'\" > /dev/null 2>&1")
                          .c_str()) == 0)
            {
               setOption(OPT_quartus_13_64bit, true);
            }
            else
            {
               setOption(OPT_quartus_13_64bit, false);
            }
         }
         else
         {
            setOption(OPT_quartus_settings, "export PATH=$PATH:" + dir + "/quartus/bin");
         }
      }
   };
   for(const auto& altera_dir : altera_dirs)
   {
      if(std::filesystem::is_directory(altera_dir))
      {
         for(const auto& ver_dir : sorted_dirs(altera_dir))
         {
            search_quartus(ver_dir.string());
         }
         search_quartus(altera_dir);
      }
   }

   /// Search for lattice tool
   const auto lattice_dirs = getOption<std::vector<std::string>>(OPT_lattice_root);
   removeOption(OPT_lattice_root);
   auto has_lattice = 0; // 0 = not found, 1 = 32-bit version, 2 = 64-bit version
   const auto search_lattice = [&](const std::string& dir) {
      if(std::filesystem::exists(dir + "/bin/lin/diamondc"))
      {
         has_lattice = 1;
         setOption(OPT_lattice_root, dir);
      }
      else if(std::filesystem::exists(dir + "/bin/lin64/diamondc"))
      {
         has_lattice = 2;
         setOption(OPT_lattice_root, dir);
      }
      if(std::filesystem::exists(dir + "/cae_library/synthesis/verilog/pmi_def.v"))
      {
         setOption(OPT_lattice_pmi_def, dir + "/cae_library/synthesis/verilog/pmi_def.v");
      }
      if(std::filesystem::exists(dir + "/cae_library/simulation/verilog/pmi/pmi_dsp_mult.v") &&
         std::filesystem::exists(dir + "/cae_library/simulation/verilog/pmi/pmi_ram_dp_true_be.v"))
      {
         setOption(OPT_lattice_inc_dirs, dir + "/cae_library");
      }
   };
   for(const auto& lattice_dir : lattice_dirs)
   {
      if(std::filesystem::is_directory(lattice_dir))
      {
         for(const auto& ver_dir : sorted_dirs(lattice_dir))
         {
            search_lattice(ver_dir.string());
         }
         search_lattice(lattice_dir);
      }
   }
   if(has_lattice == 1)
   {
      const auto lattice_dir = getOption<std::string>(OPT_lattice_root);
      setOption(OPT_lattice_settings, "export TEMP=/tmp;export LSC_INI_PATH=\"\";"
                                      "export LSC_DIAMOND=true;"
                                      "export TCL_LIBRARY=" +
                                          lattice_dir +
                                          "/tcltk/lib/tcl8.5;"
                                          "export FOUNDRY=" +
                                          lattice_dir +
                                          "/ispfpga;"
                                          "export PATH=$FOUNDRY/bin/lin:" +
                                          lattice_dir + "/bin/lin:$PATH");
   }
   else if(has_lattice == 2)
   {
      const auto lattice_dir = getOption<std::string>(OPT_lattice_root);
      setOption(OPT_lattice_settings, "export TEMP=/tmp;export LSC_INI_PATH=\"\";"
                                      "export LSC_DIAMOND=true;"
                                      "export TCL_LIBRARY=" +
                                          lattice_dir +
                                          "/tcltk/lib/tcl8.5;"
                                          "export FOUNDRY=" +
                                          lattice_dir +
                                          "/ispfpga;"
                                          "export PATH=$FOUNDRY/bin/lin64:" +
                                          lattice_dir + "/bin/lin64:$PATH");
   }

   /// Search for Mentor tools
   const auto mentor_dirs = getOption<std::vector<std::string>>(OPT_mentor_root);
   removeOption(OPT_mentor_root);
   const auto search_mentor = [&](const std::string& dir) {
      if(std::filesystem::exists(dir + "/bin/vsim"))
      {
         setOption(OPT_mentor_modelsim_bin, dir + "/bin");
      }
   };
   for(const auto& mentor_dir : mentor_dirs)
   {
      if(std::filesystem::is_directory(mentor_dir))
      {
         for(const auto& ver_dir : sorted_dirs(mentor_dir))
         {
            search_mentor(ver_dir.string());
         }
         search_mentor(mentor_dir);
      }
   }
   /// Search for Synopsys VCS tools
   const auto vcs_dirs = getOption<std::vector<std::string>>(OPT_synopsys_vcs_root);
   removeOption(OPT_synopsys_vcs_root);
   const auto search_vcs = [&](const std::string& dir) {
      if(std::filesystem::exists(dir + "/bin/vcs"))
      {
         setOption(OPT_synopsys_vcs_home, dir);
      }
   };
   for(const auto& vcs_dir : vcs_dirs)
   {
      if(std::filesystem::is_directory(vcs_dir))
      {
         for(const auto& ver_dir : sorted_dirs(vcs_dir))
         {
            search_vcs(ver_dir.string());
         }
         search_vcs(vcs_dir);
      }
   }

   /// Search for NanoXplore tools
   const auto nanox_dirs = getOption<std::vector<std::string>>(OPT_nanoxplore_root);
   removeOption(OPT_nanoxplore_root);
   const auto search_xmap = [&](const std::string& dir) {
      if(std::filesystem::exists(dir + "/bin/nxpython"))
      {
         setOption(OPT_nanoxplore_root, dir);
      }
   };
   for(const auto& nanox_dir : nanox_dirs)
   {
      if(std::filesystem::is_directory(nanox_dir))
      {
         search_xmap(nanox_dir);
         for(const auto& ver_dir : sorted_dirs(nanox_dir))
         {
            search_xmap(ver_dir.string());
         }
      }
   }

   /// Search for Xilinx tools
   const auto target_64 = true;
   const auto xilinx_dirs = getOption<std::vector<std::string>>(OPT_xilinx_root);
   removeOption(OPT_xilinx_root);
   const auto search_xilinx = [&](const std::string& dir) {
      if(std::filesystem::exists(dir + "/ISE"))
      {
         if(target_64 && std::filesystem::exists(dir + "/settings64.sh"))
         {
            setOption(OPT_xilinx_settings, dir + "/settings64.sh");
            setOption(OPT_xilinx_root, dir);
         }
         else if(std::filesystem::exists(dir + "/settings32.sh"))
         {
            setOption(OPT_xilinx_settings, dir + "/settings32.sh");
            setOption(OPT_xilinx_root, dir);
         }
         if(std::filesystem::exists(dir + "/ISE/verilog/src/glbl.v"))
         {
            setOption(OPT_xilinx_glbl, dir + "/ISE/verilog/src/glbl.v");
         }
      }
   };
   const auto search_xilinx_vivado = [&](const std::string& dir) {
      if(std::filesystem::exists(dir + "/ids_lite"))
      {
         if(target_64 && std::filesystem::exists(dir + "/settings64.sh"))
         {
            setOption(OPT_xilinx_vivado_settings, dir + "/settings64.sh");
            setOption(OPT_xilinx_root, dir);
         }
         else if(std::filesystem::exists(dir + "/settings32.sh"))
         {
            setOption(OPT_xilinx_vivado_settings, dir + "/settings32.sh");
            setOption(OPT_xilinx_root, dir);
         }
         if(std::filesystem::exists(dir + "/data/verilog/src/glbl.v"))
         {
            setOption(OPT_xilinx_glbl, dir + "/data/verilog/src/glbl.v");
         }
      }
   };
   for(const auto& xilinx_dir : xilinx_dirs)
   {
      if(std::filesystem::is_directory(xilinx_dir))
      {
         for(const auto& ver_dir : sorted_dirs(xilinx_dir))
         {
            for(const auto& ise_dir : std::filesystem::directory_iterator(ver_dir))
            {
               const auto ise_path = ise_dir.path().string();
               if(std::filesystem::is_directory(ise_dir) && ise_path.find("ISE") > ise_path.find_last_of('/'))
               {
                  search_xilinx(ise_path);
               }
            }
         }
         search_xilinx(xilinx_dir);
      }
   }
   for(const auto& xilinx_dir : xilinx_dirs)
   {
      if(std::filesystem::is_directory(xilinx_dir))
      {
         for(const auto& vivado_dir : std::filesystem::directory_iterator(xilinx_dir))
         {
            const auto vivado_path = vivado_dir.path().string();
            if(std::filesystem::is_directory(vivado_dir) && vivado_path.find("Vivado") > vivado_path.find_last_of('/'))
            {
               for(const auto& ver_dir : sorted_dirs(vivado_path))
               {
                  search_xilinx_vivado(ver_dir.string());
               }
            }
         }
         search_xilinx_vivado(xilinx_dir);
      }
   }

   /// Search for verilator
   setOption(OPT_verilator, system("command -v verilator > /dev/null 2>&1") == 0);

   if(!isOption(OPT_simulator))
   {
      if(isOption(OPT_mentor_modelsim_bin))
      {
         setOption(OPT_simulator, "MODELSIM"); /// Mixed language simulator
      }
      else if(isOption(OPT_synopsys_vcs_home))
      {
         setOption(OPT_simulator, "VCS"); /// Mixed language simulator
      }
      else if(isOption(OPT_xilinx_vivado_settings))
      {
         setOption(OPT_simulator, "XSIM"); /// Mixed language simulator
      }
      else if(getOption<bool>(OPT_verilator))
      {
         setOption(OPT_simulator, "VERILATOR");
      }
      else
      {
         setOption(OPT_simulator, "CSIM");
      }
   }

   /// target device options
   if(!isOption(OPT_device_string))
   {
      std::string device_string = getOption<std::string>("device_name") + getOption<std::string>("device_speed") +
                                  getOption<std::string>("device_package");
      setOption(OPT_device_string, device_string);
   }

   const auto device_name = getOption<std::string>("device_name");
   if(device_name.find("nangate45") != std::string::npos || device_name.find("asap7") != std::string::npos)
   {
      if(!isOption(OPT_memory_allocation_policy) ||
         getOption<MemoryAllocation_Policy>(OPT_memory_allocation_policy) != MemoryAllocation_Policy::NO_BRAM)
      {
         THROW_WARNING(
             "ASIC synthesis does not support internal memory, switching memory allocation policy to NO_BRAM.");
         setOption(OPT_memory_allocation_policy, MemoryAllocation_Policy::NO_BRAM);
      }
   }

   if(isOption(OPT_channels_type) &&
      (getOption<MemoryAllocation_ChannelsType>(OPT_channels_type) == MemoryAllocation_ChannelsType::MEM_ACC_N1 ||
       getOption<MemoryAllocation_ChannelsType>(OPT_channels_type) == MemoryAllocation_ChannelsType::MEM_ACC_NN) &&
      !isOption(OPT_channels_number))
   {
      setOption(OPT_channels_number, 2);
   }

   /// circuit debugging options
   if(isOption(OPT_generate_vcd) && getOption<bool>(OPT_generate_vcd))
   {
      setOption(OPT_assert_debug, true);
   }
   if(getOption<int>(OPT_debug_level) >= DEBUG_LEVEL_VERY_PEDANTIC)
   {
      setOption(OPT_assert_debug, true);
   }

   /// chaining options
   setOption(OPT_chaining_algorithm, HLSFlowStep_Type::SCHED_CHAINING);

   /// evaluation options
   if(auto backend_pipeline = getOption<std::string>(OPT_backend_pipeline);
      backend_pipeline.find("<simulation>") != std::string::npos)
   {
      auto simulator = getOption<std::string>(OPT_simulator);
      std::transform(simulator.begin(), simulator.end(), simulator.begin(),
                     [](const unsigned char c) { return static_cast<char>(std::tolower(c)); });
      boost::replace_all(backend_pipeline, "<simulation>", "simulation/" + simulator);
      setOption(OPT_backend_pipeline, backend_pipeline);
   }
   if(getOption<bool>(OPT_evaluation))
   {
      THROW_ASSERT(isOption(OPT_evaluation_objectives), "missing evaluation objectives");
      auto objectives = getOption<CustomSet<std::string>>(OPT_evaluation_objectives);
      THROW_ASSERT(!objectives.empty(), "");

      if(getOption<EvaluationMode::evaluation_mode>(OPT_evaluation_mode) == EvaluationMode::BACKEND)
      {
         if(objectives.find("AREAxTIME") != objectives.end())
         {
            objectives.emplace("AREA");
            objectives.emplace("TIME");
            setOption(OPT_evaluation_objectives, container_to_string(objectives, STR_CST_string_separator));
         }

         if(objectives.find("TIME") != objectives.end() || objectives.find("TOTAL_TIME") != objectives.end() ||
            objectives.find("CYCLES") != objectives.end() || objectives.find("TOTAL_CYCLES") != objectives.end())
         {
            if(getOption<std::string>(OPT_simulator) == "MODELSIM" && !isOption(OPT_mentor_modelsim_bin))
            {
               THROW_ERROR_USAGE("Mentor Modelsim was not detected by Bambu. Please check --mentor-root and your "
                                 "Modelsim installation.");
            }
            else if(getOption<std::string>(OPT_simulator) == "VCS" && !isOption(OPT_synopsys_vcs_home))
            {
               THROW_ERROR_USAGE("Synopsys VCS was not detected by Bambu. Please check --synopsys-vcs-root and your "
                                 "VCS installation.");
            }
            else if(getOption<std::string>(OPT_simulator) == "XSIM" && !isOption(OPT_xilinx_vivado_settings))
            {
               THROW_ERROR_USAGE(
                   "Xilinx XSim was not detected by Bambu. Please check --xilinx-root and your Vivado installation.");
            }
            else if(getOption<std::string>(OPT_simulator) == "VERILATOR" && !getOption<bool>(OPT_verilator))
            {
               THROW_ERROR_USAGE(
                   "Verilator was not detected by Bambu. Please install Verilator or select a different simulator.");
            }
            if(!getOption<bool>(OPT_generate_testbench))
            {
               setOption(OPT_generate_testbench, true);
               setOption(OPT_testbench_input_file, "test.xml");
            }
            if(getOption<std::list<std::string>>(OPT_top_functions_names).size() > 1)
            {
               THROW_ERROR_USAGE("Simulation cannot be enabled with multiple top functions. "
                                 "Select a single top function for simulation.");
            }
            if(isOption(OPT_device_string) && starts_with(getOption<std::string>(OPT_device_string), "LFE"))
            {
               if(getOption<std::string>(OPT_simulator) == "VERILATOR")
               {
                  THROW_WARNING("Simulation of Lattice device may not work with VERILATOR. Recent versions ignore some "
                                "issue in Verilog Lattice libraries.");
               }
               if(!isOption(OPT_lattice_settings))
               {
                  THROW_ERROR_USAGE("Simulation of Lattice devices requires Lattice support. "
                                    "Please configure --lattice-root.");
               }
            }
         }
         const auto is_valid_evaluation_mode = [](const std::string& s) -> bool {
            return s == "AREA" || s == "AREAxTIME" || s == "TIME" || s == "TOTAL_TIME" || s == "CYCLES" ||
                   s == "TOTAL_CYCLES" || s == "BRAMS" || s == "DRAMS" || s == "CLOCK_SLACK" || s == "DSPS" ||
                   s == "FREQUENCY" || s == "PERIOD" || s == "REGISTERS";
         };
         if(!all_of(objectives.begin(), objectives.end(), is_valid_evaluation_mode))
         {
            THROW_ERROR_USAGE("BadParameters: invalid --evaluation-objectives for --evaluation-mode=BACKEND. "
                              "Supported objectives in BACKEND mode are: AREA, AREAxTIME, TIME, TOTAL_TIME, "
                              "CYCLES, TOTAL_CYCLES, BRAMS, DRAMS, CLOCK_SLACK, DSPS, FREQUENCY, PERIOD, REGISTERS.");
         }
      }
      else
      {
         THROW_ERROR_USAGE("BadParameters: unsupported evaluation mode '" +
                           EvaluationMode::to_string(getOption<EvaluationMode::evaluation_mode>(OPT_evaluation_mode)) +
                           "' when --evaluation is enabled. Supported mode is: BACKEND. "
                           "Use --dry-run-evaluation for dry-run evaluation.");
      }
   }

   if(getOption<HLSFlowStep_Type>(OPT_interface_type) == HLSFlowStep_Type::INFERRED_INTERFACE_GENERATION)
   {
      setOption(OPT_expose_globals, false);
   }

   if(!getOption<bool>(OPT_expose_globals))
   {
      if(getOption<MemoryAllocation_Policy>(OPT_memory_allocation_policy) == MemoryAllocation_Policy::NONE &&
         getOption<HLSFlowStep_Type>(OPT_interface_type) != HLSFlowStep_Type::WB4_INTERFACE_GENERATION)
      {
         setOption(OPT_memory_allocation_policy, MemoryAllocation_Policy::ALL_BRAM);
      }
   }
   ir_helper::debug_level = get_class_debug_level("ir_helper");

   const auto default_compiler = getOption<CompilerWrapper_CompilerTarget>(OPT_default_compiler);
   const auto flag_cpp = isOption(OPT_input_format) &&
                         (getOption<Parameters_FileFormat>(OPT_input_format) == Parameters_FileFormat::FF_CPP ||
                          getOption<Parameters_FileFormat>(OPT_input_format) == Parameters_FileFormat::FF_LLVM_CPP);
   appendOption(OPT_cc_includes, "-isystem " + relocate_install_path(PANDA_INCLUDE_INSTALLDIR, true).string(), " ");
   if(!isOption(OPT_cc_standard))
   {
      if(flag_cpp)
      {
         setOption(OPT_cc_standard, "c++14");
      }
      else

      {
         setOption(OPT_cc_standard, "c11");
      }
   }
   /// Disable proxy when there are multiple top functions
   // TODO: check this, function proxies should work properly even with multiple top functions
   if(getOption<std::list<std::string>>(OPT_top_functions_names).size() > 1)
   {
      if(isOption(OPT_disable_function_proxy))
      {
         if(!getOption<bool>(OPT_disable_function_proxy))
         {
            THROW_ERROR_USAGE("Function proxies are not supported with multiple top functions. "
                              "Use --disable-function-proxy.");
         }
      }
      else
      {
         setOption(OPT_disable_function_proxy, true);
      }
   }

   /// add experimental setup options
   if(getOption<std::string>(OPT_experimental_setup) == "VVD")
   {
      if(!isOption(OPT_compiler_opt_level))
      {
         setOption(OPT_compiler_opt_level, CompilerWrapper_OptimizationSet::O3);
      }
      if(!isOption(OPT_channels_type))
      {
         setOption(OPT_channels_type, MemoryAllocation_ChannelsType::MEM_ACC_NN);
      }
      if(!isOption(OPT_channels_number))
      {
         setOption(OPT_channels_number, 2);
      }
      if(getOption<MemoryAllocation_Policy>(OPT_memory_allocation_policy) == MemoryAllocation_Policy::NONE)
      {
         setOption(OPT_memory_allocation_policy, MemoryAllocation_Policy::ALL_BRAM);
      }
      setOption(OPT_DSP_allocation_coefficient, 1.75);
      setOption(OPT_clock_period_resource_fraction, "0.875");
      setOption(OPT_expose_globals, false);
      if(!isOption(OPT_distram_threshold))
      {
         setOption(OPT_distram_threshold, 256);
      }
      add_experimental_setup_compiler_options(!flag_cpp);
      if(!isOption(OPT_disable_function_proxy))
      {
         setOption(OPT_disable_function_proxy, true);
      }
   }
   else if(getOption<std::string>(OPT_experimental_setup) == "BAMBU092")
   {
      if(!isOption(OPT_compiler_opt_level))
      {
         setOption(OPT_compiler_opt_level, CompilerWrapper_OptimizationSet::O0);
      }
      setOption(OPT_estimate_logic_and_connections, false);
      setOption(OPT_clock_period_resource_fraction, "0.9");
      setOption(OPT_DSP_margin_combinational, 1.3);
      setOption(OPT_skip_pipe_parameter, 1);
      if(!isOption(OPT_distram_threshold))
      {
         setOption(OPT_distram_threshold, 256);
      }
      add_experimental_setup_compiler_options(!flag_cpp);
      if(!isOption(OPT_disable_function_proxy))
      {
         setOption(OPT_disable_function_proxy, true);
      }
   }
   else if(getOption<std::string>(OPT_experimental_setup) == "BAMBU-BALANCED" ||
           getOption<std::string>(OPT_experimental_setup) == "BAMBU-BALANCED-MP")
   {
      if(!isOption(OPT_compiler_opt_level))
      {
         setOption(OPT_compiler_opt_level, CompilerWrapper_OptimizationSet::O2);

         appendOption(OPT_cc_optimizations, "inline-functions");
      }
      if(getOption<std::string>(OPT_experimental_setup) == "BAMBU-BALANCED-MP")
      {
         if(!isOption(OPT_channels_type))
         {
            setOption(OPT_channels_type, MemoryAllocation_ChannelsType::MEM_ACC_NN);
         }
         if(!isOption(OPT_channels_number))
         {
            setOption(OPT_channels_number, 2);
         }
      }
      if(getOption<MemoryAllocation_Policy>(OPT_memory_allocation_policy) == MemoryAllocation_Policy::NONE)
      {
         setOption(OPT_memory_allocation_policy, MemoryAllocation_Policy::ALL_BRAM);
      }
      if(!isOption(OPT_distram_threshold))
      {
         setOption(OPT_distram_threshold, 256);
      }
      add_experimental_setup_compiler_options(!flag_cpp);
      if(!isOption(OPT_disable_function_proxy))
      {
         setOption(OPT_disable_function_proxy, true);
         if(!isOption(OPT_function_constraints))
         {
            setOption(OPT_function_constraints, "*");
         }
      }
   }
   else if(getOption<std::string>(OPT_experimental_setup) == "BAMBU-PERFORMANCE-MP")
   {
      if(!isOption(OPT_compiler_opt_level))
      {
         setOption(OPT_compiler_opt_level, CompilerWrapper_OptimizationSet::O3);
      }
      if(!isOption(OPT_channels_type))
      {
         setOption(OPT_channels_type, MemoryAllocation_ChannelsType::MEM_ACC_NN);
      }
      if(!isOption(OPT_channels_number))
      {
         setOption(OPT_channels_number, 2);
      }
      if(getOption<MemoryAllocation_Policy>(OPT_memory_allocation_policy) == MemoryAllocation_Policy::NONE)
      {
         setOption(OPT_memory_allocation_policy, MemoryAllocation_Policy::ALL_BRAM);
      }
      if(!isOption(OPT_distram_threshold))
      {
         setOption(OPT_distram_threshold, 512);
      }
      add_experimental_setup_compiler_options(!flag_cpp);
      if(!isOption(OPT_disable_function_proxy))
      {
         setOption(OPT_disable_function_proxy, true);
      }
   }
   else if(getOption<std::string>(OPT_experimental_setup) == "BAMBU-PERFORMANCE")
   {
      if(!isOption(OPT_compiler_opt_level))
      {
         setOption(OPT_compiler_opt_level, CompilerWrapper_OptimizationSet::O3);
      }
      if(getOption<MemoryAllocation_Policy>(OPT_memory_allocation_policy) == MemoryAllocation_Policy::NONE)
      {
         setOption(OPT_memory_allocation_policy, MemoryAllocation_Policy::ALL_BRAM);
      }
      if(!isOption(OPT_distram_threshold))
      {
         setOption(OPT_distram_threshold, 512);
      }
      add_experimental_setup_compiler_options(!flag_cpp);
      if(!isOption(OPT_disable_function_proxy))
      {
         setOption(OPT_disable_function_proxy, true);
      }
   }
   else if(getOption<std::string>(OPT_experimental_setup) == "BAMBU-AREA-MP")
   {
      if(!isOption(OPT_compiler_opt_level))
      {
         setOption(OPT_compiler_opt_level, CompilerWrapper_OptimizationSet::Os);
      }
      appendOption(OPT_cc_optimizations, "no-unroll-loops");
      if(!isOption(OPT_channels_type))
      {
         setOption(OPT_channels_type, MemoryAllocation_ChannelsType::MEM_ACC_NN);
      }
      if(!isOption(OPT_channels_number))
      {
         setOption(OPT_channels_number, 2);
      }
      if(getOption<MemoryAllocation_Policy>(OPT_memory_allocation_policy) == MemoryAllocation_Policy::NONE)
      {
         setOption(OPT_memory_allocation_policy, MemoryAllocation_Policy::ALL_BRAM);
      }
      setOption(OPT_DSP_allocation_coefficient, 1.75);
      if(!isOption(OPT_distram_threshold))
      {
         setOption(OPT_distram_threshold, 256);
      }
      add_experimental_setup_compiler_options(!flag_cpp);
      if(!isOption(OPT_disable_function_proxy))
      {
         if(!isOption(OPT_function_constraints))
         {
            setOption(OPT_disable_function_proxy, false);
         }
         else
         {
            setOption(OPT_disable_function_proxy, true);
         }
      }
   }
   else if(getOption<std::string>(OPT_experimental_setup) == "BAMBU-AREA")
   {
      if(!isOption(OPT_compiler_opt_level))
      {
         setOption(OPT_compiler_opt_level, CompilerWrapper_OptimizationSet::Os);
      }
      appendOption(OPT_cc_optimizations, "no-unroll-loops");
      if(getOption<MemoryAllocation_Policy>(OPT_memory_allocation_policy) == MemoryAllocation_Policy::NONE)
      {
         setOption(OPT_memory_allocation_policy, MemoryAllocation_Policy::ALL_BRAM);
      }
      setOption(OPT_DSP_allocation_coefficient, 1.75);
      if(!isOption(OPT_distram_threshold))
      {
         setOption(OPT_distram_threshold, 256);
      }
      add_experimental_setup_compiler_options(!flag_cpp);
      if(!isOption(OPT_disable_function_proxy))
      {
         if(!isOption(OPT_function_constraints))
         {
            setOption(OPT_disable_function_proxy, false);
         }
         else
         {
            setOption(OPT_disable_function_proxy, true);
         }
      }
   }
   else if(getOption<std::string>(OPT_experimental_setup) == "BAMBU")
   {
      if(!isOption(OPT_compiler_opt_level))
      {
         setOption(OPT_compiler_opt_level, CompilerWrapper_OptimizationSet::O0);
      }
      if(!isOption(OPT_distram_threshold))
      {
         setOption(OPT_distram_threshold, 256);
      }
      add_experimental_setup_compiler_options(false);
      if(!isOption(OPT_disable_function_proxy))
      {
         if(!isOption(OPT_function_constraints))
         {
            setOption(OPT_disable_function_proxy, false);
         }
         else
         {
            setOption(OPT_disable_function_proxy, true);
         }
      }
   }
   else
   {
      THROW_ERROR_USAGE("Experimental setup not recognized: " + getOption<std::string>(OPT_experimental_setup) +
                        ". Use a supported setup name or remove --experimental-setup.");
   }

   add_bambu_library("bambu");

   if(isOption(OPT_fp_subnormal) && getOption<bool>(OPT_fp_subnormal))
   {
      add_bambu_library("softfloat_s");
   }
   else
   {
      add_bambu_library("softfloat");
   }

   if(isOption(OPT_hls_div) && getOption<std::string>(OPT_hls_div) != "none")
   {
      add_bambu_library("softint" + getOption<std::string>(OPT_hls_div));
   }

   if(isOption(OPT_cc_libraries))
   {
      const auto libraries = getOption<CustomSet<std::string>>(OPT_cc_libraries);
      for(const auto& library : libraries)
      {
         add_bambu_library(library);
      }
   }

   /// default for memory allocation policy
   if(getOption<MemoryAllocation_Policy>(OPT_memory_allocation_policy) == MemoryAllocation_Policy::NONE)
   {
      setOption(OPT_memory_allocation_policy, MemoryAllocation_Policy::LSS);
   }

   /// base address and initial internal address checks
   if(isOption(OPT_initial_internal_address) && (getOption<unsigned long long int>(OPT_base_address) == 0 ||
                                                 getOption<unsigned int>(OPT_initial_internal_address) == 0))
   {
      appendOption(OPT_cc_optimizations, "no-delete-null-pointer-checks");
   }

   /// Checks
   if(!isOption(OPT_channels_type))
   {
      setOption(OPT_channels_type, MemoryAllocation_ChannelsType::MEM_ACC_11);
      setOption(OPT_channels_number, 1);
   }
   if(isOption(OPT_channels_number) && getOption<unsigned int>(OPT_channels_number) == 1U)
   {
      setOption(OPT_channels_type, MemoryAllocation_ChannelsType::MEM_ACC_11);
   }
   if(isOption(OPT_channels_number) &&
      getOption<MemoryAllocation_ChannelsType>(OPT_channels_type) == MemoryAllocation_ChannelsType::MEM_ACC_11)
   {
      if(getOption<unsigned int>(OPT_channels_number) != 1)
      {
         THROW_ERROR_USAGE("The number of channels cannot be specified for MEM_ACC_11. "
                           "Set --channels-number=1 or use MEM_ACC_NN.");
      }
   }

   if(isOption(OPT_mem_delay_read))
   {
      const auto mem_delay_read = getOption<unsigned int>(OPT_mem_delay_read);
      if(getOption<MemoryAllocation_Policy>(OPT_memory_allocation_policy) == MemoryAllocation_Policy::ALL_BRAM ||
         getOption<MemoryAllocation_Policy>(OPT_memory_allocation_policy) ==
             MemoryAllocation_Policy::EXT_PIPELINED_BRAM)
      {
         if(getOption<std::string>(OPT_bram_high_latency) == "" && mem_delay_read != 2U)
         {
            THROW_ERROR_USAGE("When using ALL_BRAM or EXT_PIPELINED_BRAM, --mem-delay-read must be 2.");
         }
         else if(getOption<std::string>(OPT_bram_high_latency) == "_3" && mem_delay_read != 3U)
         {
            THROW_ERROR_USAGE("When using ALL_BRAM or EXT_PIPELINED_BRAM with --bram-high-latency=_3, "
                              "--mem-delay-read must be 3.");
         }
         else if(getOption<std::string>(OPT_bram_high_latency) == "_4" && mem_delay_read != 4U)
         {
            THROW_ERROR_USAGE("When using ALL_BRAM or EXT_PIPELINED_BRAM with --bram-high-latency=_4, "
                              "--mem-delay-read must be 4.");
         }
         else
         {
            THROW_ERROR_USAGE("Invalid BRAM latency configuration. Use --bram-high-latency in {\"\", \"_3\", \"_4\"}.");
         }
      }
      else
      {
         if(mem_delay_read < 2U)
         {
            THROW_ERROR_USAGE("--mem-delay-read must be at least 2.");
         }
      }
   }
   else
   {
      if(getOption<MemoryAllocation_Policy>(OPT_memory_allocation_policy) == MemoryAllocation_Policy::ALL_BRAM ||
         getOption<MemoryAllocation_Policy>(OPT_memory_allocation_policy) ==
             MemoryAllocation_Policy::EXT_PIPELINED_BRAM)
      {
         if(getOption<std::string>(OPT_bram_high_latency) == "")
         {
            setOption(OPT_mem_delay_read, 2U);
         }
         else if(getOption<std::string>(OPT_bram_high_latency) == "_3")
         {
            setOption(OPT_mem_delay_read, 3U);
         }
         else if(getOption<std::string>(OPT_bram_high_latency) == "_4")
         {
            setOption(OPT_mem_delay_read, 4U);
         }
         else
         {
            THROW_ERROR_USAGE("Invalid BRAM latency configuration. Use --bram-high-latency in {\"\", \"_3\", \"_4\"}.");
         }
      }
      else
      {
         setOption(OPT_mem_delay_read, 2U);
      }
   }

   if(isOption(OPT_mem_delay_write))
   {
      const auto mem_delay_write = getOption<unsigned int>(OPT_mem_delay_write);
      if(getOption<MemoryAllocation_Policy>(OPT_memory_allocation_policy) == MemoryAllocation_Policy::ALL_BRAM ||
         getOption<MemoryAllocation_Policy>(OPT_memory_allocation_policy) ==
             MemoryAllocation_Policy::EXT_PIPELINED_BRAM)
      {
         if(getOption<std::string>(OPT_bram_high_latency) == "" && mem_delay_write != 1U)
         {
            THROW_ERROR_USAGE("When using ALL_BRAM or EXT_PIPELINED_BRAM, --mem-delay-write must be 1.");
         }
         else if(getOption<std::string>(OPT_bram_high_latency) == "_3" && mem_delay_write != 1U)
         {
            THROW_ERROR_USAGE("When using ALL_BRAM or EXT_PIPELINED_BRAM with --bram-high-latency=_3, "
                              "--mem-delay-write must be 1.");
         }
         else if(getOption<std::string>(OPT_bram_high_latency) == "_4" && mem_delay_write != 2U)
         {
            THROW_ERROR_USAGE("When using ALL_BRAM or EXT_PIPELINED_BRAM with --bram-high-latency=_4, "
                              "--mem-delay-write must be 2.");
         }
         else
         {
            THROW_ERROR_USAGE("Invalid BRAM latency configuration. Use --bram-high-latency in {\"\", \"_3\", \"_4\"}.");
         }
      }
      else
      {
         if(mem_delay_write < 2U)
         {
            THROW_ERROR_USAGE("--mem-delay-write must be at least 2.");
         }
      }
   }
   else
   {
      if(getOption<MemoryAllocation_Policy>(OPT_memory_allocation_policy) == MemoryAllocation_Policy::ALL_BRAM ||
         getOption<MemoryAllocation_Policy>(OPT_memory_allocation_policy) ==
             MemoryAllocation_Policy::EXT_PIPELINED_BRAM)
      {
         if(getOption<std::string>(OPT_bram_high_latency) == "")
         {
            setOption(OPT_mem_delay_write, 1U);
         }
         else if(getOption<std::string>(OPT_bram_high_latency) == "_3")
         {
            setOption(OPT_mem_delay_write, 1U);
         }
         else if(getOption<std::string>(OPT_bram_high_latency) == "_4")
         {
            setOption(OPT_mem_delay_write, 2U);
         }
         else
         {
            THROW_ERROR_USAGE("Invalid BRAM latency configuration. Use --bram-high-latency in {\"\", \"_3\", \"_4\"}.");
         }
      }
      else
      {
         setOption(OPT_mem_delay_write, 2U);
      }
   }

   if(isOption(OPT_bus_pipelined) && getOption<bool>(OPT_bus_pipelined))
   {
      if(getOption<MemoryAllocation_Policy>(OPT_memory_allocation_policy) != MemoryAllocation_Policy::NO_BRAM)
      {
         THROW_ERROR_USAGE("Bus pipelining requires --memory-allocation-policy=NO_BRAM.");
      }

      const unsigned int tb_mem_delay_read = getOption<unsigned int>(OPT_mem_delay_read);
      const unsigned int tb_mem_delay_write = getOption<unsigned int>(OPT_mem_delay_write);

      if(tb_mem_delay_read != tb_mem_delay_write)
      {
         THROW_ERROR_USAGE("When bus pipelining is enabled, --mem-delay-read and --mem-delay-write must be equal.");
      }
   }

   if(isOption(OPT_openmp) && getOption<bool>(OPT_openmp))
   {
      if(getOption<MemoryAllocation_Policy>(OPT_memory_allocation_policy) == MemoryAllocation_Policy::ALL_BRAM ||
         getOption<MemoryAllocation_Policy>(OPT_memory_allocation_policy) ==
             MemoryAllocation_Policy::EXT_PIPELINED_BRAM)
      {
         THROW_ERROR_USAGE("OpenMP requires variable latency on the bus and does not support "
                           "--memory-allocation-policy=ALL_BRAM or EXT_PIPELINED_BRAM.");
      }
   }

   if(getOption<HLSFlowStep_Type>(OPT_interface_type) == HLSFlowStep_Type::WB4_INTERFACE_GENERATION)
   {
      if(getOption<MemoryAllocation_ChannelsType>(OPT_channels_type) == MemoryAllocation_ChannelsType::MEM_ACC_NN)
      {
         THROW_ERROR_USAGE("Wishbone 4 interface does not support multi-channel architectures (MEM_ACC_NN).");
      }

      if(getOption<MemoryAllocation_Policy>(OPT_memory_allocation_policy) == MemoryAllocation_Policy::ALL_BRAM)
      {
         THROW_ERROR_USAGE("Wishbone 4 interface does not support --memory-allocation-policy=ALL_BRAM.");
      }
      else if(getOption<MemoryAllocation_Policy>(OPT_memory_allocation_policy) ==
              MemoryAllocation_Policy::EXT_PIPELINED_BRAM)
      {
         THROW_ERROR_USAGE("Wishbone 4 interface does not support --memory-allocation-policy=EXT_PIPELINED_BRAM.");
      }
   }

   if(getOption<bool>(OPT_memory_mapped_top) &&
      (isOption(OPT_clock_name) || isOption(OPT_reset_name) || isOption(OPT_start_name) || isOption(OPT_done_name) ||
       isOption(OPT_idle_name)))
   {
      THROW_ERROR_USAGE("Memory-mapped top interface does not allow renaming control signals. "
                        "Remove clock/reset/start/done/idle renaming options.");
   }

   if(!isOption(OPT_input_file))
   {
      THROW_ERROR_USAGE("No input file specified. Provide at least one source/input file.");
   }
   if(isOption(OPT_cc_optimizations) &&
      getOption<std::string>(OPT_input_file).find(STR_CST_string_separator) != std::string::npos &&
      getOption<std::string>(OPT_cc_optimizations).find("whole-program") != std::string::npos &&
      getOption<std::string>(OPT_cc_optimizations).find("no-whole-program") == std::string::npos)
   {
      THROW_ERROR_USAGE("-fwhole-program cannot be used with multiple input files. "
                        "Use a single input file or remove whole-program optimization.");
   }
   if(isOption(OPT_openmp) && getOption<bool>(OPT_openmp) &&
      getOption<Parameters_FileFormat>(OPT_input_format) != Parameters_FileFormat::FF_RAW)
   {
      appendOption(OPT_input_file,
                   (relocate_install_path(PANDA_DATA_INSTALLDIR, true) / STR_CST_libopenmp_filename).string());
   }
   if((isOption(OPT_generate_vcd) && getOption<bool>(OPT_generate_vcd)) ||
      (isOption(OPT_discrepancy) && getOption<bool>(OPT_discrepancy)))
   {
      if(!isOption(OPT_generate_testbench) || !getOption<bool>(OPT_generate_testbench))
      {
         THROW_ERROR_USAGE("Testbench generation is required. Use --generate-tb=<...> or enable simulation.");
      }
   }

   setOption<unsigned int>(OPT_host_compiler, static_cast<unsigned int>(default_compiler));
   if(isOption(OPT_lattice_settings))
   {
      if(isOption(OPT_evaluation_objectives) &&
         getOption<std::string>(OPT_evaluation_objectives).find("AREA") != std::string::npos &&
         isOption(OPT_device_string) && starts_with(getOption<std::string>(OPT_device_string), "LFE") &&
         !getOption<bool>(OPT_connect_iob))
      {
         THROW_WARNING("--connect-iob must be used when target is a Lattice board");
      }
   }

   if(starts_with(getOption<std::string>(OPT_device_string), "nx"))
   {
      THROW_WARNING("Asynchronous memories are disabled by default when targeting NanoXplore devices");
      setOption(OPT_use_asynchronous_memories, false);
   }
}

void BambuParameter::SetDefaults()
{
   /// Debugging level
   setOption(OPT_output_level, OUTPUT_LEVEL_MINIMUM);
   setOption(OPT_debug_level, DEBUG_LEVEL_NONE);

   /// pragmas related
   setOption(OPT_ignore_parallelism, false);

   /// ---------- HLS process options ----------- //
   setOption(OPT_synthesis_flow, HLSFlowStep_Type::CLASSICAL_HLS_SYNTHESIS_FLOW);
   setOption(OPT_hls_flow, HLSFlowStep_Type::STANDARD_HLS_FLOW);

   /// ---------- HLS specification reference ----------- //
   setOption(OPT_generate_testbench, false);
   setOption(OPT_max_sim_cycles, 200000000);
   setOption(OPT_chaining, true);

   /// High-level synthesis constraints dump -- //
   setOption("dumpConstraints", false);
   setOption("dumpConstraints_file", "Constraints.xml");

   /// -- Scheduling -- //
   /// Scheduling algorithm (default is list based one)
   setOption(OPT_scheduling_algorithm, HLSFlowStep_Type::LIST_BASED_SCHEDULING);
   setOption(OPT_scheduling_priority, ParametricListBased_Metric::DYNAMIC_MOBILITY);

   /// -- Module binding -- //
   /// module binding algorithm
   setOption(OPT_fu_binding_algorithm, HLSFlowStep_Type::CDFC_MODULE_BINDING);
   setOption(OPT_cdfc_module_binding_algorithm, CliqueCovering_Algorithm::TS_WEIGHTED_CLIQUE_COVERING);

   /// -- Dataflow analysis -- //
   setOption(OPT_liveVariableAlgorithm, HLSFlowStep_Type::FSM_NI_SSA_LIVENESS);

   /// -- Register allocation -- //
   /// register allocation algorithm
   setOption(OPT_register_allocation_algorithm, HLSFlowStep_Type::WEIGHTED_CLIQUE_REGISTER_BINDING);
   setOption(OPT_weighted_clique_register_algorithm, CliqueCovering_Algorithm::TS_WEIGHTED_CLIQUE_COVERING);
   /// storage value insertion algorithm
   setOption(OPT_storage_value_insertion_algorithm, HLSFlowStep_Type::VALUES_SCHEME_STORAGE_VALUE_INSERTION);
   setOption(OPT_reset_type, "no");
   setOption(OPT_reset_level, false);
   setOption(OPT_reg_init_value, true);

   setOption(OPT_shared_input_registers, false);

   /// Function allocation
   setOption(OPT_function_allocation_algorithm, HLSFlowStep_Type::DOMINATOR_FUNCTION_ALLOCATION);

   /// Enable function proxy by default
   setOption(OPT_disable_bounded_function, false);

   /// Disable memory mapped interface for top function by default
   setOption(OPT_memory_mapped_top, false);

   setOption(OPT_tb_queue_size, 4);

   /// -- Memory allocation -- //
   setOption(OPT_memory_allocation_algorithm, HLSFlowStep_Type::DOMINATOR_MEMORY_ALLOCATION);
   setOption(OPT_memory_allocation_policy, MemoryAllocation_Policy::NONE);
   setOption(OPT_base_address, 1073741824); // 1Gbytes maximum address space reserved for the accelerator
   setOption(OPT_memory_controller_type, "D00");
   setOption(OPT_sparse_memory, true);
   setOption(OPT_expose_globals, false);

   /// -- Bus options -- //
   setOption(OPT_bus_pipelined, false);
   setOption(OPT_bus_arbiter_type, "RR");
   setOption(OPT_bus_architecture, "ALL_TO_ALL");

   /// -- Datapath -- //
   /// Datapath interconnection architecture
   setOption(OPT_datapath_interconnection_algorithm, HLSFlowStep_Type::MUX_INTERCONNECTION_BINDING);
   /// Datapath architecture
   setOption(OPT_datapath_architecture, HLSFlowStep_Type::CLASSIC_DATAPATH_CREATOR);

   /// backend HDL
   setOption(OPT_writer_language, HDLWriter_Language::VERILOG);

   /// -- Module Interfaces -- //
   setOption(OPT_interface, true);
   setOption(OPT_interface_type, HLSFlowStep_Type::MINIMAL_INTERFACE_GENERATION);

   /// -- Module Characterization -- //
   setOption(OPT_evaluation, false);
   setOption(OPT_evaluation_mode, EvaluationMode::NONE);
   setOption(OPT_evaluation_objectives, "");
   setOption(OPT_backend_pipeline, "#synthesis");

   setOption(OPT_generate_components, HDL_OUT_WORK_SEPARATE);
   setOption(OPT_altera_root, "/opt/altera" STR_CST_string_separator "/opt/intelFPGA");
   setOption(OPT_lattice_root, "/opt/diamond" STR_CST_string_separator "/usr/local/diamond");
   setOption(OPT_mentor_root, "/opt/mentor");
   setOption(OPT_mentor_optimizer, true);
   setOption(OPT_synopsys_vcs_root, "/opt/synopsys/vcs");
   setOption(OPT_nanoxplore_root, "/opt/NanoXplore");
   setOption(OPT_xilinx_root, "/opt/Xilinx");

   /// -- Module Synthesis -- //
   setOption("device_name", "xc7z020");
   setOption("device_speed", "-1");
   setOption("device_package", "clg484");
   setOption(OPT_timing_violation_abort, false);
   setOption(OPT_export_core, false);
   setOption(OPT_connect_iob, false);

   /// -- Compiler options -- //
   setOption(OPT_default_compiler, CompilerWrapper::getDefaultCompiler());
   setOption(OPT_compatible_compilers, CompilerWrapper::getCompatibleCompilers());

   setOption(OPT_without_transformation, true);

   appendOption(OPT_cc_defines, "__BAMBU__");
   appendOption(OPT_cc_defines, "__SYNTHESIS__");

   setOption(OPT_hls_div, "NR");
   setOption(OPT_hls_fpdiv, "SRT4");
   setOption(OPT_max_ulp, 1.0);
   setOption(OPT_skip_pipe_parameter, 0);
   setOption(OPT_unaligned_access, false);
   setOption(OPT_aligned_access, false);
   setOption(OPT_cc_serialize_memory_accesses, false);
   setOption(OPT_use_asynchronous_memories, true);
   setOption(OPT_do_not_chain_memories, false);
   setOption(OPT_bram_high_latency, "");
   setOption(OPT_experimental_setup, "BAMBU-BALANCED-MP");
   setOption(OPT_DSP_margin_combinational, 1.0);
   setOption(OPT_DSP_margin_pipelined, 1.0);
   setOption(OPT_DSP_allocation_coefficient, NUM_CST_allocation_default_allocation_coefficient);
   setOption(OPT_estimate_logic_and_connections, true);
   setOption(OPT_registered_inputs, "auto");
   setOption(OPT_fsm_encoding, "auto");
   setOption(OPT_scheduling_mux_margins, 0.0);
   setOption(OPT_no_return_zero, false);
   setOption(OPT_bitvalue_ipa, true);
   setOption(OPT_range_analysis_mode, "");
   setOption(OPT_fp_format, "");
   setOption(OPT_fp_format_propagate, false);
   setOption(OPT_parallel_backend, "1");

#if HAVE_HOST_PROFILING_BUILT
   setOption(OPT_exec_argv, STR_CST_string_separator);
   setOption(OPT_profiling_method, HostProfiling_Method::PM_NONE);
   setOption(OPT_host_compiler, CompilerWrapper::getDefaultCompiler());
#endif
   setOption(OPT_clock_period, 10.0);
   setOption(OPT_clock_period_resource_fraction, "1.0");
   setOption(OPT_mixed_design, true);
   setOption(OPT_num_accelerators, 4);

   /// ---------- Simulation options ----------- //
   setOption(OPT_testbench_map_mode, "DEVICE");

   SetPandaParameter("CSE_size", "2");
   SetPandaParameter("MAX_LUT_INT_SIZE", "8");
}

void BambuParameter::add_bambu_library(const std::string& lib)
{
   auto m_arch = getOption<std::string>(OPT_cc_m_env);
   std::filesystem::path filename = relocate_install_path(PANDA_LIB_INSTALLDIR);
   THROW_ASSERT(m_arch.size() > 2, "unexpected condition");
   filename /= m_arch.substr(2);
   filename /= "lib" + lib;
   filename += ".a";

   appendOption(OPT_archive_files, filename.string());
}
