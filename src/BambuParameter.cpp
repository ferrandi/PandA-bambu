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
 *              Copyright (C) 2004-2020 Politecnico di Milano
 *
 *   This file is part of the PandA framework.
 *
 *   The PandA framework is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>.
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

/// Autoheader
#include "config_HAVE_ALTERA.hpp"
#include "config_HAVE_BEAGLE.hpp"
#include "config_HAVE_COIN_OR.hpp"
#include "config_HAVE_CUDD.hpp"
#include "config_HAVE_EXPERIMENTAL.hpp"
#include "config_HAVE_FLOPOCO.hpp"
#include "config_HAVE_GLPK.hpp"
#include "config_HAVE_HOST_PROFILING_BUILT.hpp"
#include "config_HAVE_I386_CLANG4_COMPILER.hpp"
#include "config_HAVE_I386_CLANG4_M32.hpp"
#include "config_HAVE_I386_CLANG4_M64.hpp"
#include "config_HAVE_I386_CLANG4_MX32.hpp"
#include "config_HAVE_I386_CLANG5_COMPILER.hpp"
#include "config_HAVE_I386_CLANG5_M32.hpp"
#include "config_HAVE_I386_CLANG5_M64.hpp"
#include "config_HAVE_I386_CLANG5_MX32.hpp"
#include "config_HAVE_I386_CLANG6_COMPILER.hpp"
#include "config_HAVE_I386_CLANG6_M32.hpp"
#include "config_HAVE_I386_CLANG6_M64.hpp"
#include "config_HAVE_I386_CLANG6_MX32.hpp"
#include "config_HAVE_I386_CLANG7_COMPILER.hpp"
#include "config_HAVE_I386_CLANG7_M32.hpp"
#include "config_HAVE_I386_CLANG7_M64.hpp"
#include "config_HAVE_I386_CLANG7_MX32.hpp"
#include "config_HAVE_I386_GCC45_COMPILER.hpp"
#include "config_HAVE_I386_GCC46_COMPILER.hpp"
#include "config_HAVE_I386_GCC47_COMPILER.hpp"
#include "config_HAVE_I386_GCC47_M32.hpp"
#include "config_HAVE_I386_GCC47_M64.hpp"
#include "config_HAVE_I386_GCC47_MX32.hpp"
#include "config_HAVE_I386_GCC48_COMPILER.hpp"
#include "config_HAVE_I386_GCC48_M32.hpp"
#include "config_HAVE_I386_GCC48_M64.hpp"
#include "config_HAVE_I386_GCC48_MX32.hpp"
#include "config_HAVE_I386_GCC49_COMPILER.hpp"
#include "config_HAVE_I386_GCC49_M32.hpp"
#include "config_HAVE_I386_GCC49_M64.hpp"
#include "config_HAVE_I386_GCC49_MX32.hpp"
#include "config_HAVE_I386_GCC5_COMPILER.hpp"
#include "config_HAVE_I386_GCC5_M32.hpp"
#include "config_HAVE_I386_GCC5_M64.hpp"
#include "config_HAVE_I386_GCC5_MX32.hpp"
#include "config_HAVE_I386_GCC6_COMPILER.hpp"
#include "config_HAVE_I386_GCC6_M32.hpp"
#include "config_HAVE_I386_GCC6_M64.hpp"
#include "config_HAVE_I386_GCC6_MX32.hpp"
#include "config_HAVE_I386_GCC7_COMPILER.hpp"
#include "config_HAVE_I386_GCC7_M32.hpp"
#include "config_HAVE_I386_GCC7_M64.hpp"
#include "config_HAVE_I386_GCC7_MX32.hpp"
#include "config_HAVE_I386_GCC8_COMPILER.hpp"
#include "config_HAVE_I386_GCC8_M32.hpp"
#include "config_HAVE_I386_GCC8_M64.hpp"
#include "config_HAVE_I386_GCC8_MX32.hpp"
#include "config_HAVE_ICARUS.hpp"
#include "config_HAVE_ILP_BUILT.hpp"
#include "config_HAVE_LATTICE.hpp"
#include "config_HAVE_LIBRARY_CHARACTERIZATION_BUILT.hpp"
#include "config_HAVE_LP_SOLVE.hpp"
#include "config_HAVE_MAPPING_BUILT.hpp"
#include "config_HAVE_MENTOR_VISUALIZER_EXE.hpp"
#include "config_HAVE_MODELSIM.hpp"
#include "config_HAVE_VCD_BUILT.hpp"
#include "config_HAVE_VERILATOR.hpp"
#include "config_HAVE_XILINX.hpp"
#include "config_HAVE_XILINX_VIVADO.hpp"
#include "config_PANDA_DATA_INSTALLDIR.hpp"
#include "config_PANDA_LIB_INSTALLDIR.hpp"
#include "config_SKIP_WARNING_SECTIONS.hpp"

/// algorithms/clique_covering
#include "clique_covering.hpp"

/// constants include
#include "allocation_constants.hpp"
#include "constants.hpp"

#if HAVE_HOST_PROFILING_BUILT
/// frontend_flow/behavior_analysis
#include "host_profiling.hpp"
#endif

/// HLS/binding/module
#include "cdfc_module_binding.hpp"

/// HLS/evaluation include
#include "evaluation.hpp"

/// HLS/memory include
#include "memory_allocation.hpp"

/// HLS/scheduling include
#include "parametric_list_based.hpp"
#if HAVE_ILP_BUILT
#include "sdc_scheduling.hpp"
#endif

#if HAVE_ILP_BUILT
/// ilp includes
#include "meilp_solver.hpp"
#endif

/// tree include
#include "tree_helper.hpp"

/// Header include
#include "BambuParameter.hpp"

/// creation of the target architecture
#include "chaining.hpp"
#include "datapath_creator.hpp"

#if HAVE_BEAGLE
#if SKIP_WARNING_SECTIONS
#pragma GCC diagnostic ignored "-Wshadow"
#pragma GCC diagnostic ignored "-Wsign-conversion"
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wconversion"
#pragma GCC diagnostic ignored "-Woverloaded-virtual"
#pragma GCC diagnostic ignored "-Wtype-limits"
#pragma GCC diagnostic ignored "-Wold-style-cast"
#pragma GCC diagnostic ignored "-Wextra"
#pragma GCC diagnostic ignored "-Wignored-qualifiers"
#pragma GCC diagnostic ignored "-Wsign-compare"
#pragma GCC diagnostic ignored "-Wsign-promo"
#endif
#include "FitnessFunction.hpp"
#include "dse_hls.hpp"
#endif

/// Constant option include
#include "constant_strings.hpp"

/// STD include
#include <cstring>
#include <iosfwd>
#include <string>

/// STL includes
#include <list>
#include <vector>

/// Technology include
#include "language_writer.hpp"
#include "parse_technology.hpp"
#include "target_device.hpp"
#include "technology_manager.hpp"

/// technology/physical_library
#include "technology_node.hpp"

/// Utility include
#include "cpu_time.hpp"
#include "dbgPrintHelper.hpp"
#include "fileIO.hpp"
#include "string_manipulation.hpp"
#include "treegcc_constants.hpp"
#include "utility.hpp"
#include <boost/filesystem.hpp>
#include <boost/lexical_cast.hpp>
#include <getopt.h>

/// Wrapper include
#include "gcc_wrapper.hpp"

/// Design Space Exploration
#define OPT_ACCEPT_NONZERO_RETURN 256
#define INPUT_OPT_C_NO_PARSE (1 + OPT_ACCEPT_NONZERO_RETURN)
#define INPUT_OPT_C_PYTHON_NO_PARSE (1 + INPUT_OPT_C_NO_PARSE)
#define OPT_ACO_FLOW (1 + INPUT_OPT_C_PYTHON_NO_PARSE)
#define OPT_ACO_GENERATIONS (1 + OPT_ACO_FLOW)
#define OPT_ADDITIONAL_TOP (1 + OPT_ACO_GENERATIONS)
#define OPT_ALIGNED_ACCESS_PARAMETER (1 + OPT_ADDITIONAL_TOP)
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
#define OPT_DISABLE_FUNCTION_PROXY (1 + OPT_DISABLE_BOUNDED_FUNCTION)
#define OPT_DISABLE_IOB (1 + OPT_DISABLE_FUNCTION_PROXY)
#define OPT_DISTRAM_THRESHOLD (1 + OPT_DISABLE_IOB)
#define OPT_DO_NOT_CHAIN_MEMORIES (1 + OPT_DISTRAM_THRESHOLD)
#define OPT_DO_NOT_EXPOSE_GLOBALS (1 + OPT_DO_NOT_CHAIN_MEMORIES)
#define OPT_ROM_DUPLICATION (1 + OPT_DO_NOT_EXPOSE_GLOBALS)
#define OPT_DO_NOT_USE_ASYNCHRONOUS_MEMORIES (1 + OPT_ROM_DUPLICATION)
#define OPT_DSE (1 + OPT_DO_NOT_USE_ASYNCHRONOUS_MEMORIES)
#define OPT_DSP_ALLOCATION_COEFFICIENT (1 + OPT_DSE)
#define OPT_DSP_MARGIN_COMBINATIONAL (1 + OPT_DSP_ALLOCATION_COEFFICIENT)
#define OPT_DSP_MARGIN_PIPELINED (1 + OPT_DSP_MARGIN_COMBINATIONAL)
#define OPT_DUMP_CONSTRAINTS (1 + OPT_DSP_MARGIN_PIPELINED)
#define OPT_DYNAMIC_GENERATORS_DIR (1 + OPT_DUMP_CONSTRAINTS)
#define OPT_DISCREPANCY (1 + OPT_DYNAMIC_GENERATORS_DIR)
#define OPT_DISCREPANCY_FORCE (1 + OPT_DISCREPANCY)
#define OPT_DISCREPANCY_HW (1 + OPT_DISCREPANCY_FORCE)
#define OPT_DISCREPANCY_NO_LOAD_POINTERS (1 + OPT_DISCREPANCY_HW)
#define OPT_DISCREPANCY_ONLY (1 + OPT_DISCREPANCY_NO_LOAD_POINTERS)
#define OPT_DISCREPANCY_PERMISSIVE_PTRS (1 + OPT_DISCREPANCY_ONLY)
#define INPUT_OPT_DRY_RUN_EVALUATION (1 + OPT_DISCREPANCY_PERMISSIVE_PTRS)
#define OPT_ENABLE_IOB (1 + INPUT_OPT_DRY_RUN_EVALUATION)
#define OPT_EVALUATION (1 + OPT_ENABLE_IOB)
#define OPT_EVALUATION_MODE (1 + OPT_EVALUATION)
#define OPT_EXPERIMENTAL_SETUP (1 + OPT_EVALUATION_MODE)
#define INPUT_OPT_FIND_MAX_CFG_TRANSFORMATIONS (1 + OPT_EXPERIMENTAL_SETUP)
#define OPT_FIXED_SCHED (1 + INPUT_OPT_FIND_MAX_CFG_TRANSFORMATIONS)
#define OPT_FLOPOCO (1 + OPT_FIXED_SCHED)
#define OPT_GENERATE_VCD (1 + OPT_FLOPOCO)
#define OPT_GENERATION (1 + OPT_GENERATE_VCD)
#define OPT_HLS_DIV (1 + OPT_GENERATION)
#define OPT_HLS_FPDIV (1 + OPT_HLS_DIV)
#define OPT_HOST_PROFILING (1 + OPT_HLS_FPDIV)
#define OPT_ILP (1 + OPT_HOST_PROFILING)
#define OPT_ILP_NEWFORM (1 + OPT_ILP)
#define OPT_ILP_SOLVER (1 + OPT_ILP_NEWFORM)
#define INPUT_OPT_FILE_INPUT_DATA (1 + OPT_ILP_SOLVER)
#define OPT_INSERT_MEMORY_PROFILE (1 + INPUT_OPT_FILE_INPUT_DATA)
#define OPT_INSERT_VERIFICATION_OPERATION (1 + OPT_INSERT_MEMORY_PROFILE)
#define OPT_LIBM_STD_ROUNDING (1 + OPT_INSERT_VERIFICATION_OPERATION)
#define OPT_LIST_BASED (1 + OPT_LIBM_STD_ROUNDING)
#define OPT_LOGICAL_OPTIMIZATION (1 + OPT_LIST_BASED)
#define OPT_MAX_EVALUATIONS (1 + OPT_LOGICAL_OPTIMIZATION)
#define OPT_MAX_INHERITANCE (1 + OPT_MAX_EVALUATIONS)
#define OPT_MAX_SIM_CYCLES (1 + OPT_MAX_INHERITANCE)
#define OPT_MAX_ULP (1 + OPT_MAX_SIM_CYCLES)
#define OPT_MEMORY_MAPPED_TOP (1 + OPT_MAX_ULP)
#define OPT_MEM_DELAY_READ (1 + OPT_MEMORY_MAPPED_TOP)
#define OPT_MEM_DELAY_WRITE (1 + OPT_MEM_DELAY_READ)
#define OPT_MEMORY_BANKS_NUMBER (1 + OPT_MEM_DELAY_WRITE)
#define OPT_MIN_INHERITANCE (1 + OPT_MEMORY_BANKS_NUMBER)
#define OPT_MOSA_FLOW (1 + OPT_MIN_INHERITANCE)
#define OPT_NO_MIXED_DESIGN (1 + OPT_MOSA_FLOW)
#define OPT_NUM_ACCELERATORS (1 + OPT_NO_MIXED_DESIGN)
#define OPT_PARALLEL_CONTROLLER (1 + OPT_NUM_ACCELERATORS)
#define OPT_PERIOD_CLOCK (1 + OPT_PARALLEL_CONTROLLER)
#define OPT_CLOCK_NAME (1 + OPT_PERIOD_CLOCK)
#define OPT_RESET_NAME (1 + OPT_CLOCK_NAME)
#define OPT_START_NAME (1 + OPT_RESET_NAME)
#define OPT_DONE_NAME (1 + OPT_START_NAME)
#define OPT_POWER_OPTIMIZATION (1 + OPT_DONE_NAME)
#define OPT_PRAGMA_PARSE (1 + OPT_POWER_OPTIMIZATION)
#define OPT_PRETTY_PRINT (1 + OPT_PRAGMA_PARSE)
#define OPT_POST_RESCHEDULING (1 + OPT_PRETTY_PRINT)
#define OPT_REGISTER_ALLOCATION (1 + OPT_POST_RESCHEDULING)
#define OPT_REGISTERED_INPUTS (1 + OPT_REGISTER_ALLOCATION)
#define OPT_FSM_ENCODING (1 + OPT_REGISTERED_INPUTS)
#define OPT_RESET (1 + OPT_FSM_ENCODING)
#define OPT_LEVEL_RESET (1 + OPT_RESET)
#define OPT_DISABLE_REG_INIT_VALUE (1 + OPT_LEVEL_RESET)
#define OPT_SCHEDULING_MUX_MARGINS (1 + OPT_DISABLE_REG_INIT_VALUE)
#define OPT_USE_ALUS (1 + OPT_SCHEDULING_MUX_MARGINS)
#define OPT_SERIALIZE_MEMORY_ACCESSES (1 + OPT_USE_ALUS)
#define OPT_SILP (1 + OPT_SERIALIZE_MEMORY_ACCESSES)
#define OPT_SIMULATE (1 + OPT_SILP)
#define OPT_SKIP_PIPE_PARAMETER (1 + OPT_SIMULATE)
#define OPT_SOFT_FLOAT (1 + OPT_SKIP_PIPE_PARAMETER)
#define OPT_SOFTFLOAT_SUBNORMAL (1 + OPT_SOFT_FLOAT)
#define OPT_SOFT_FP (1 + OPT_SOFTFLOAT_SUBNORMAL)
#define OPT_STG (1 + OPT_SOFT_FP)
#define OPT_SPECULATIVE (1 + OPT_STG)
#define INPUT_OPT_TEST_MULTIPLE_NON_DETERMINISTIC_FLOWS (1 + OPT_SPECULATIVE)
#define INPUT_OPT_TEST_SINGLE_NON_DETERMINISTIC_FLOW (1 + INPUT_OPT_TEST_MULTIPLE_NON_DETERMINISTIC_FLOWS)
#define OPT_TESTBENCH (1 + INPUT_OPT_TEST_SINGLE_NON_DETERMINISTIC_FLOW)
#define OPT_TESTBENCH_EXTRA_GCC_FLAGS (1 + OPT_TESTBENCH)
#define OPT_TIME_WEIGHT (1 + OPT_TESTBENCH_EXTRA_GCC_FLAGS)
#define OPT_TIMING_MODEL (1 + OPT_TIME_WEIGHT)
#define OPT_TIMING_VIOLATION (1 + OPT_TIMING_MODEL)
#define OPT_TOP_FNAME (1 + OPT_TIMING_VIOLATION)
#define OPT_TOP_RTLDESIGN_NAME (1 + OPT_TOP_FNAME)
#define OPT_UNALIGNED_ACCESS_PARAMETER (1 + OPT_TOP_RTLDESIGN_NAME)
#define OPT_VHDL_LIBRARY_PARAMETER (1 + OPT_UNALIGNED_ACCESS_PARAMETER)
#define OPT_VISUALIZER (1 + OPT_VHDL_LIBRARY_PARAMETER)
#define OPT_XML_CONFIG (1 + OPT_VISUALIZER)

/// constant correspond to the "parametric list based option"
#define PAR_LIST_BASED_OPT "parametric-list-based"
/// constant correspond to the fixed scheduling option
#define FIXED_SCHEDULING_OPT "fixed-scheduling"

static bool is_evaluation_objective_string(const std::vector<std::string>& obj_vec, const std::string& s)
{
   return std::find(obj_vec.begin(), obj_vec.end(), s) != obj_vec.end();
}

static void add_evaluation_objective_string(std::string& obj_string, const std::string& obj_to_add)
{
   if(obj_string.empty())
   {
      obj_string = obj_to_add;
      return;
   }
   std::vector<std::string> obj_vec = convert_string_to_vector<std::string>(obj_string, ",");
   const std::vector<std::string> obj_vec_to_add = convert_string_to_vector<std::string>(obj_to_add, ",");

   for(const auto& s : obj_vec_to_add)
   {
      if(not is_evaluation_objective_string(obj_vec, s))
      {
         obj_string += "," + s;
         obj_vec.push_back(s);
      }
   }

   return;
}

void BambuParameter::PrintHelp(std::ostream& os) const
{
   os << "Usage:\n"
      << "       bambu [Options] <source_file> [<constraints_file>] [<technology_file>]\n\n"
      << "Options:\n\n";
   PrintGeneralOptionsUsage(os);
   PrintOutputOptionsUsage(os);
   os << "    --pretty-print=<file>\n"
      << "        C-based pretty print of the internal IR.\n\n"
      << "    --writer,-w<language>\n"
      << "        Output RTL language:\n"
      << "            V - Verilog (default)\n"
#if HAVE_EXPERIMENTAL
      << "            S - SystemC\n"
#endif
      << "            H - VHDL\n\n"
      << "    --no-mixed-design\n"
      << "        Avoid mixed design.\n\n"
      << "    --generate-tb=<file>\n"
      << "        Generate testbench for the input values defined in the specified XML\n"
      << "        file.\n\n"
      << "    --top-fname=<fun_name>\n"
      << "        Define the top function to be synthesized.\n\n"
      << "    --top-rtldesign-name=<top_name>\n"
      << "        Define the top module name for the RTL backend.\n\n"
      << "    --file-input-data=<file_list>\n"
      << "        A comma-separated list of input files used by the C specification.\n\n"
      << "    --C-no-parse=<file>\n"
      << "        Specify a comma-separated list of C files used only during the\n"
      << "        co-simulation phase.\n\n"
      << std::endl;

   PrintGccOptionsUsage(os);

   // Target options
   os << "  Target:\n\n"
      << "    --target-file=file, -b<file>\n"
      << "        Specify an XML description of the target device.\n\n"
      << "    --generate-interface=<type>\n"
      << "        Wrap the top level module with an external interface.\n"
      << "        Possible values for <type> and related interfaces:\n"
      << "            MINIMAL  -  (minimal interface - default)\n"
      << "            INFER    -  (top function is built with an hardware interface inferred from the pragmas or from the top function signature)\n"
      << "            WB4      -  (WishBone 4 interface)\n"
#if HAVE_EXPERIMENTAL
      << "            AXI4LITE -  (AXI4-Lite interface)\n"
      << "            FSL      -  (interface to the FSL bus)\n"
      << "            NPI      -  (interface to the NPI bus)\n"
#endif
      << "\n"
#if HAVE_EXPERIMENTAL
      << "    --edk-config <file>\n"
      << "        Specify the configuration file for Xilinx EDK.\n\n"
#endif
      << std::endl;
#if HAVE_EXPERIMENTAL

   // Frontend analysis options
   os << "  Frontend analysis:\n\n"
      << "    --pdg-reduction[=<algorithm>]\n"
      << "        Perform Program Dependence Graph reduction. The reduced PDG is used to\n"
      << "        build the parallel controller, if enabled.\n"
      << "        Possible values for <algorithm> are:\n"
      << "            GP - Girkar-Polychronopoulos algorithm (default)\n"
      << "            P  - Polimi algorithm (not supported yet)\n\n"
      << std::endl;
#endif

   // HLS options
   os << "  Scheduling:\n\n"
      << "    --parametric-list-based[=<type>]\n"
      << "        Perform priority list-based scheduling. This is the default scheduling algorithm\n"
      << "        in bambu. The optional <type> argument can be used to set options for\n"
      << "        list-based scheduling as follows:\n"
      << "            0 - Dynamic mobility (default)\n"
      << "            1 - Static mobility\n"
      << "            2 - Priority-fixed mobility\n\n"
      << "    --post-rescheduling\n"
      << "        Perform post rescheduling to better distribute resources.\n\n"
#if HAVE_ILP_BUILT
#if HAVE_EXPERIMENTAL
      << "    --ilp-solver=<solver>\n"
      << "        Sets the ilp solver. Possible values for <solver> are:\n"
      << "            G - Solve the ilp problem using glpk solver (default)\n"
#if HAVE_COIN_OR
      << "            C - Solve the ilp problem using the COIN-OR solver\n"
#endif
#if HAVE_LP_SOLVE
      << "            L - Solve the ilp problem using lp_solve solver\n"
#endif
      << "\n"
      << "    --ilp\n"
      << "        Perform scheduling by using the integer linear programming formulation.\n"
      << "        Default: off.\n\n"
      << "    --ilp-newform\n"
      << "        Perform scheduling by using the integer linear programming with the new\n"
      << "        formulation. Default: off.\n\n"
      << "    --silp\n"
      << "        Perform scheduling by using the scheduling and allocation with ILP\n"
      << "        formulation. Default: off.\n\n"
#endif
      << "    --speculative-sdc-scheduling,-s\n"
      << "        Perform scheduling by using speculative sdc.\n\n"
#endif
      << "    --pipelining,-p\n"
      << "        Perform functional pipelining starting from the top function.\n\n"
      << "    --fixed-scheduling=<file>\n"
      << "        Provide scheduling as an XML file.\n\n"
      << "    --no-chaining\n"
      << "        Disable chaining optimization.\n\n"
      << std::endl;

#if HAVE_EXPERIMENTAL
   // Controller style options
   os << "  Controller style:\n\n"
      << "    --stg[=<type>]\n"
      << "        Create the FSM-based controller. This is default for bambu. The\n"
      << "        optional argument <type> can be used to set options for FSM-based\n"
      << "        controller as follows:\n"
      << "            0 - Basic Block-based (exploiting scheduling information) (default)\n\n"
      << "    --parallel-controller\n"
      << "        Create the parallel controller\n\n"
      << std::endl;
#endif

   // Binding options
   os << "  Binding:\n\n"
#if HAVE_EXPERIMENTAL
      << "    --storage-value-insertion\n"
      << "        Specify the storage value insertion algorithm:\n"
      << "            0 - VARIABLE/VALUES (default)\n"
      << "            1 - VALUES_INSTANCES\n"
      << "            2 - LIMITED_VALUES_INSTANCES\n"
      << "            3 - PAULIN_SCHEME\n\n"
#endif
#if HAVE_EXPERIMENTAL
      << "    --explore-fu-reg=[string]\n"
      << "        Perform simultaneous FU/reg binding.\n\n"
#endif
      << "    --register-allocation=<type>\n"
      << "        Set the algorithm used for register allocation. Possible values for the\n"
      << "        <type> argument are the following:\n"
      << "            WEIGHTED_TS        - solve the weighted clique covering problem by\n"
      << "                                 exploiting the Tseng&Siewiorek heuristics\n"
      << "                                 (default)\n"
      << "            WEIGHTED_COLORING   - use weighted coloring algorithm\n"
      << "            COLORING            - use simple coloring algorithm\n"
      << "            CHORDAL_COLORING    - use chordal coloring algorithm\n"
      << "            BIPARTITE_MATCHING  - use bipartite matching algorithm\n"
      << "            TTT_CLIQUE_COVERING - use a weighted clique covering algorithm\n"
      << "            UNIQUE_BINDING      - unique binding algorithm\n"
#if HAVE_EXPERIMENTAL
      << "            K_COFAMILY          - use k_cofamily algorithm\n"
      << "            LEFT_EDGE           - use left edge algorithm\n"
      << "            CYCLIC_ALLOCATION   - use cyclic allocation algorithm\n"
#endif
      << "\n"
      << "    --module-binding=<type>\n"
      << "        Set the algorithm used for module binding. Possible values for the\n"
      << "        <type> argument are one the following:\n"
      << "            WEIGHTED_TS        - solve the weighted clique covering problem by\n"
      << "                                 exploiting the Tseng&Siewiorek heuristics\n"
      << "                                 (default)\n"
      << "            WEIGHTED_COLORING  - solve the weighted clique covering problem\n"
      << "                                 performing a coloring on the conflict graph\n"
      << "            COLORING           - solve the unweighted clique covering problem\n"
      << "                                 performing a coloring on the conflict graph\n"
      << "            TTT_FAST           - use Tomita, A. Tanaka, H. Takahashi maxima\n"
      << "                                 weighted cliques heuristic to solve the clique\n"
      << "                                 covering problem\n"
      << "            TTT_FAST2          - use Tomita, A. Tanaka, H. Takahashi maximal\n"
      << "                                 weighted cliques heuristic to incrementally\n"
      << "                                 solve the clique covering problem\n"
      << "            TTT_FULL           - use Tomita, A. Tanaka, H. Takahashi maximal\n"
      << "                                 weighted cliques algorithm to solve the clique\n"
      << "                                 covering problem\n"
      << "            TTT_FULL2          - use Tomita, A. Tanaka, H. Takahashi maximal\n"
      << "                                 weighted cliques algorithm to incrementally\n"
      << "                                 solve the clique covering problem\n"
      << "            TS                 - solve the unweighted clique covering problem\n"
      << "                                 by exploiting the Tseng&Siewiorek heuristic\n"
      << "            BIPARTITE_MATCHING - solve the weighted clique covering problem\n"
      << "                                 exploiting the bipartite matching approach\n"
#if HAVE_EXPERIMENTAL
      << "            RANDOMIZED         - solve the weighted clique covering problem\n"
      << "                                 exploiting a randomized approach\n"
#endif
      << "            UNIQUE             - use a 1-to-1 binding algorithm\n\n"
      << std::endl;

   // Memory allocation options
   os << "  Memory allocation:\n\n"
      << "    --memory-allocation=<type>\n"
      << "        Set the algorithm used for memory allocation. Possible values for the\n"
      << "        type argument are the following:\n"
      << "            DOMINATOR          - all local variables, static variables and\n"
      << "                                 strings are allocated on BRAMs (default)\n"
      << "            XML_SPECIFICATION  - import the memory allocation from an XML\n"
      << "                                 specification\n\n"
      << "    --xml-memory-allocation=<xml_file_name>\n"
      << "        Specify the file where the XML configuration has been defined.\n\n"
      << "    --memory-allocation-policy=<type>\n"
      << "        Set the policy for memory allocation. Possible values for the <type>\n"
      << "        argument are the following:\n"
      << "            ALL_BRAM           - all objects that need to be stored in memory\n"
      << "                                 are allocated on BRAMs (default)\n"
      << "            LSS                - all local variables, static variables and\n"
      << "                                 strings are allocated on BRAMs\n"
      << "            GSS                - all global variables, static variables and\n"
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
      << "   --channels-type=<type>\n"
      << "        Set the type of memory connections.\n"
      << "        Possible values for <type> are:\n"
      << "            MEM_ACC_11 - the accesses to the memory have a single direct\n"
      << "                         connection or a single indirect connection (default)\n"
      << "            MEM_ACC_N1 - the accesses to the memory have n parallel direct\n"
      << "                         connections or a single indirect connection\n"
      << "            MEM_ACC_NN - the accesses to the memory have n parallel direct\n"
      << "                         connections or n parallel indirect connections\n\n"
      << "   --channels-number=<n>\n"
      << "        Define the number of parallel direct or indirect accesses.\n\n"
      << "   --memory-ctrl-type=type\n"
      << "        Define which type of memory controller is used. Possible values for the\n"
      << "        <type> argument are the following:\n"
      << "            D00 - no extra delay (default)\n"
      << "            D10 - 1 clock cycle extra-delay for LOAD, 0 for STORE\n"
      << "            D11 - 1 clock cycle extra-delay for LOAD, 1 for STORE\n"
      << "            D21 - 2 clock cycle extra-delay for LOAD, 1 for STORE\n\n"
      << "    --memory-banks-number=<n>\n"
      << "        Define the number of memory banks.\n\n"
      << "    --sparse-memory[=on/off]\n"
      << "        Control how the memory allocation happens.\n"
      << "            on - allocate the data in addresses which reduce the decoding logic (default)\n"
      << "           off - allocate the data in a contiguous addresses.\n\n"
      << "    --do-not-use-asynchronous-memories\n"
      << "        Do not add asynchronous memories to the possible set of memories used\n"
      << "        by bambu during the memory allocation step.\n\n"
      << "    --distram-threshold=value\n"
      << "        Define the threshold in bitsize used to infer DISTRIBUTED/ASYNCHRONOUS RAMs (default 256).\n\n"
      << "    --serialize-memory-accesses\n"
      << "        Serialize the memory accesses using the GCC virtual use-def chains\n"
      << "        without taking into account any alias analysis information.\n\n"
      << "    --unaligned-access\n"
      << "        Use only memories supporting unaligned accesses.\n\n"
      << "    --aligned-access\n"
      << "        Assume that all accesses are aligned and so only memories supporting aligned\n\n"
      << "        accesses are used.\n\n"
      << "    --do-not-chain-memories\n"
      << "        When enabled LOADs and STOREs will not be chained with other\n"
      << "        operations.\n\n"
      << "    --rom-duplication\n"
      << "        Assume that read-only memories can be duplicated in case timing requires.\n\n"
      << "    --bram-high-latency=[3,4]\n"
      << "        Assume a 'high latency bram'-'faster clock frequency' block RAM memory\n"
      << "        based architectures:\n"
      << "        3 => LOAD(II=1,L=3) STORE(1).\n"
      << "        4 => LOAD(II=1,L=4) STORE(II=1,L=2).\n\n"
      << "    --mem-delay-read=value\n"
      << "        Define the external memory latency when LOAD are performed (default 2).\n\n"
      << "    --mem-delay-write=value\n"
      << "        Define the external memory latency when LOAD are performed (default 1).\n\n"
      << "    --do-not-expose-globals\n"
      << "        All global variables are considered local to the compilation units.\n\n"
      << "    --data-bus-bitsize=<bitsize>\n"
      << "        Set the bitsize of the external data bus.\n\n"
      << "    --addr-bus-bitsize=<bitsize>\n"
      << "        Set the bitsize of the external address bus.\n\n"
      << std::endl;

   // Interconnection options
#if HAVE_EXPERIMENTAL
   os << "  Interconnection:\n\n"
      << "    --interconnection, -C[<type>]\n"
      << "        Perform interconnection binding. Possible values for <type> are:\n"
      << "            M - mux-based architecture (default)\n\n"
      << std::endl;

   // HLS Design Space Exploration Options
#if HAVE_BEAGLE
   os << "  High-Level Synthesis Design Space Exploration:\n\n"
      << "    --dse=<type>\n"
      << "        Perform design space exploration (default off). Possible values for\n"
      << "        <type> are the following:\n"
      << "            MOGA  - Multi-objective Genetic Algorithm with NSGA-II\n"
      << "            MOSA  - Multi-objective Simulated Annealing\n"
      << "            MOTS  - Multi-objective Tabu Search\n"
      << "            ACO   - Multi-objective Ant Colony Optimization\n"
      << "            GRASP - Grasp\n\n"
      << "    --max-evaluations=<num>\n"
      << "        Perform no more than the specified number of evaluations.\n\n"
      << "    --aco-flow[=<number of ants>]\n"
      << "        Use Ant Colony Optimization (default off).\n\n"
      << "    --generations[=<num>]\n"
      << "        Number ant colony's generations\n\n"
      << "    --mixed_synthesis, -H[<type>]\n"
      << "        Determine the exploration space. Possible values for <type> are:\n"
      << "            0 - based on operation binding information (default)\n"
      << "            1 - based on scheduling priority information\n"
      << "            2 - based on binding and scheduling information\n"
      << std::endl;
#endif
#endif

   // Options for Evaluation of HLS results
   os << "  Evaluation of HLS results:\n\n"
#if HAVE_ICARUS || HAVE_XILINX || HAVE_VERILATOR || HAVE_MODELSIM
      << "    --simulate\n"
      << "        Simulate the RTL implementation.\n\n"
#if HAVE_MENTOR_VISUALIZER_EXE
      << "    --mentor-visualizer\n"
      << "        Simulate the RTL implementation and then open Mentor Visualizer.\n\n"
#endif
      << "    --simulator=<type>\n"
      << "        Specify the simulator used in generated simulation scripts:\n"
#if HAVE_MODELSIM
      << "            MODELSIM - Mentor Modelsim\n"
#endif
#if HAVE_XILINX
      << "            XSIM - Xilinx XSim\n"
      << "            ISIM - Xilinx iSim\n"
#endif
#if HAVE_ICARUS
      << "            ICARUS - Verilog Icarus simulator\n"
#endif
#if HAVE_VERILATOR
      << "            VERILATOR - Verilator simulator\n"
#endif
      << "\n"
      << "    --max-sim-cycles=<cycles>\n"
      << "        Specify the maximum number of cycles a HDL simulation may run.\n"
      << "        (default 20000000).\n\n"
      << "    --accept-nonzero-return\n"
      << "        Do not assume that application main must return 0.\n\n"
      << "    --generate-vcd\n"
      << "        Enable .vcd output file generation for waveform visualization (requires\n"
      << "        testbench generation).\n\n"
#endif
      << "    --evaluation[=type]\n"
      << "        Perform evaluation of the results.\n"
      << "        The value of 'type' selects the objectives to be evaluated\n"
      << "        If nothing is specified all the following are evaluated\n"
      << "        The 'type' argument can be a string containing any of the following\n"
      << "        strings, separated with commas, without spaces:\n"
      << "            AREA            - Area usage\n"
      << "            AREAxTIME       - Area x Latency product\n"
      << "            TIME            - Latency for the average computation\n"
      << "            TOTAL_TIME      - Latency for the whole computation\n"
      << "            CYCLES          - n. of cycles for the average computation\n"
      << "            TOTAL_CYCLES    - n. of cycles for the whole computation\n"
      << "            BRAMS           - number of BRAMs\n"
      << "            CLOCK_SLACK     - Slack between actual and required clock period\n"
      << "            DSPS            - number of DSPs\n"
#if HAVE_EXPERIMENTAL
      << "            EDGES_REDUCTION - Performance evaluation for dependence reduction\n"
#endif
      << "            FREQUENCY       - Maximum target frequency\n"
#if HAVE_EXPERIMENTAL
      << "            NUM_AF_EDGES    - Number of incoming edges in the FAFG\n"
      << "                              techniques\n"
#endif
      << "            PERIOD          - Actual clock period\n"
      << "            REGISTERS       - number of registers\n"
      << "\n"
#if HAVE_EXPERIMENTAL
      << "    --evaluation-mode[=type]\n"
      << "        Perform evaluation of the results:\n"
      << "            EXACT:  based on actual synthesis and simulation (default)\n"
      << "            LINEAR: based on linear regression. Unlike EXACT it supports\n"
      << "                    only the evaluation of the following objectives:\n"
      << "                    - AREA\n"
      << "                    - CLOCK_SLACK\n"
      << "                    - TIME\n"
      << "\n"
      << "    --timing-simulation\n"
      << "        Perform a simulation considering the timing delays.\n\n"
      << "    --timing-violation\n"
      << "        Aborts if synthesized circuit does not meet the timing.\n\n"
#endif // HAVE_EXPERIMENTAL
      << std::endl;

   // Export options
#if HAVE_EXPERIMENTAL
   os << "  Export:\n\n"
      << "    --export-core <type>\n"
      << "        Exports the generated accelerator:\n"
      << "            PCORE         - Xilinx XPS pcore\n\n"
      << std::endl;
#endif

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
      << "                       and package (e.g.,: \"xc7z020,-1,clg484,VVD\")\n"
      << "            - Altera:  a string defining the device string (e.g. EP2C70F896C6)\n"
      << "            - Lattice: a string defining the device string (e.g.\n"
      << "                       LFE335EA8FN484C)\n\n"
      << "    --power-optimization\n"
      << "        Enable Xilinx power based optimization (default no).\n\n"
      << "    --no-iob\n"
      << "        Disconnect primary ports from the IOB (the default is to connect\n"
      << "        primary input and outpur ports to IOBs).\n\n"
      << "    --soft-float\n"
      << "        Enable the soft-based implementation of floating-point operations.\n"
      << "        Bambu uses as default a faithfully rounded version of softfloat with rounding mode equal to round to nearest even.\n\n"
      << "        This is the default for bambu.\n\n"
#if HAVE_FLOPOCO
      << "    --flopoco\n"
      << "        Enable the flopoco-based implementation of floating-point operations.\n\n"
#endif
      << "    --softfloat-subnormal\n"
      << "        Enable the soft-based implementation of floating-point operations with subnormals support.\n\n"
      << "    --libm-std-rounding\n"
      << "        Enable the use of classical libm. This library combines a customized version of glibc, newlib and musl libm implementations into a single libm library synthetizable with bambu.\n"
      << "        Without this option, Bambu uses as default a faithfully rounded version of libm.\n\n"
      << "    --soft-fp\n"
      << "        Enable the use of soft_fp GCC library instead of bambu customized version of John R. Hauser softfloat library.\n\n"
      << "    --max-ulp\n"
      << "        Define the maximal ULP (Unit in the last place, i.e., is the spacing\n"
      << "        between floating-point numbers) accepted.\n\n"
      << "    --hls-div=<method>\n"
      << "        Perform the high-level synthesis of integer division and modulo\n"
      << "        operations starting from a C library based implementation or a HDL component:\n"
      << "             none  - use a HDL based pipelined restoring division\n"
      << "             nr1   - use a C-based non-restoring division with unrolling factor equal to 1 (default)\n"
      << "             nr2   - use a C-based non-restoring division with unrolling factor equal to 2\n"
      << "             NR    - use a C-based Newton-Raphson division\n"
      << "             as    - use a C-based align divisor shift dividend method\n\n"
      << "    --hls-fpdiv=<method>\n"
      << "        Perform the high-level synthesis of floating point division \n"
      << "        operations starting from a C library based implementation:\n"
      << "             SRT4 - use a C-based Sweeney, Robertson, Tocher floating point division with radix 4 (default)\n"
      << "             G    - use a C-based Goldschmidt floating point division.\n"
      << "             SF   - use a C-based floating point division as describe in soft-fp library (it requires --soft-fp).\n"
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
      << "             auto    - it depends on the target technology. VVD prefers one encoding while the other are fine with the standard binary encoding. (default)\n"
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
      << "             BAMBU-AREA-MP        - this setup implies:\n"
      << "                                    -Os  -D'printf(fmt, ...)='\n"
      << "                                    --channels-type=MEM_ACC_NN\n"
      << "                                    --memory-allocation-policy=ALL_BRAM\n"
      << "                                    --DSP-allocation-coefficient=1.75\n"
      << "                                    --distram-threshold=256\n"
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
      << "             BAMBU-BALANCED-MP    - (default) this setup implies:\n"
      << "                                    -O2  -D'printf(fmt, ...)='\n"
      << "                                    --channels-type=MEM_ACC_NN\n"
      << "                                    --memory-allocation-policy=ALL_BRAM\n"
      << "                                    -fgcse-after-reload  -fipa-cp-clone\n"
      << "                                    -ftree-partial-pre  -funswitch-loops\n"
      << "                                    -finline-functions  -fdisable-tree-bswap\n"
      << "                                    --param max-inline-insns-auto=25\n"
      << "                                    -fno-tree-loop-ivcanon\n"
      << "                                    --distram-threshold=256\n"
      << "             BAMBU-TASTE          - this setup concatenate the input files and\n"
      << "                                    passes these options to the compiler:\n"
      << "                                    -O2  -D'printf(fmt, ...)='\n"
      << "                                    --channels-type=MEM_ACC_NN\n"
      << "                                    --memory-allocation-policy=ALL_BRAM\n"
      << "                                    -fgcse-after-reload  -fipa-cp-clone\n"
      << "                                    -ftree-partial-pre  -funswitch-loops\n"
      << "                                    -finline-functions  -fdisable-tree-bswap\n"
      << "                                    --param max-inline-insns-auto=25\n"
      << "                                    -fno-tree-loop-ivcanon\n"
      << "                                    --distram-threshold=256\n"
      << "             BAMBU-PERFORMANCE    - this setup implies:\n"
      << "                                    -O3  -D'printf(fmt, ...)='\n"
      << "                                    --memory-allocation-policy=ALL_BRAM\n"
      << "                                    --distram-threshold=512\n"
      << "             BAMBU-PERFORMANCE-MP - this setup implies:\n"
      << "                                    -O3  -D'printf(fmt, ...)='\n"
      << "                                    --channels-type=MEM_ACC_NN\n"
      << "                                    --memory-allocation-policy=ALL_BRAM\n"
      << "                                    --distram-threshold=512\n"
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
      << "                                    --do-not-expose-globals --cprf=0.875\n\n"
      << std::endl;
   os << "  Other options:\n\n";
   os << "    --pragma-parse\n"
      << "        Perform source code parsing to extract information about pragmas.\n"
      << "        (default=no).\n\n";
#if HAVE_FROM_PRAGMA_BUILT && HAVE_BAMBU_BUILT
   os << "    --num-accelerators\n"
      << "        Set the number of physical accelerator instantiated in parallel sections. It must be a power of two (default=4).\n\n";
#endif
#if HAVE_EXPERIMENTAL
   os << "    --xml-config <file>\n"
      << "        Define the path to the XML configuration file for the synthesis.\n\n";
#endif
#if HAVE_EXPERIMENTAL
   os << "    --logical-optimization=<level>\n"
      << "        Enable logic optimization on EPDG:\n"
      << "             1  - enable optimizations based on CONDITIONAL instructions\n"
      << "                  information\n"
      << "             2  - enable optimizations based on ACTIVATION PATH information\n"
      << "             3  - enable CONDITIONAL and ACTIVATION PATH optimizations\n"
      << "             4  - enable DATA_TRANSITIVITY optimizations\n"
      << "             5  - enable CONDITIONAL and DATA_TRANSITIVITY optimizations\n"
      << "             6  - enable ACTIVATION PATH and DATA_TRANSITIVITY optimizations\n"
      << "             7  - enable CONDITIONAL, ACTIVATION PATH and DATA_TRANSITIVITY\n"
      << "                  optimizations\n"
      << "             8  - enable CONTROL_DATA_TRANSITIVITY optimizations\n"
      << "             9  - enable CONTROL_DATA_TRANSITIVITY and CONDITIONAL instructions\n"
      << "                  optimizations\n"
      << "             16 - enable CONTROL_DATA_CONTROL_FLOW_TRANSITIVITY and CONDITIONAL\n"
      << "                  instructions optimizations\n\n";
#endif
#if HAVE_ILP_BUILT
   os << "    --time, -t <time>\n"
      << "        Set maximum execution time (in seconds) for ILP solvers. (infinite).\n\n";
#endif
#if HAVE_HOST_PROFILING_BUILT
   os << "    --host-profiling\n"
      << "        Perform host-profiling.\n\n";
#endif
   os << "    --disable-bitvalue-ipa\n"
      << "        Disable inter-procedural bitvalue analysis.\n\n";
   os << std::endl;

   // Checks and debugging options
   os << "  Debug options:\n\n"
      << "    --discrepancy\n"
      << "           Performs automated discrepancy analysis between the execution\n"
      << "           of the original source code and the generated HDL (currently\n"
      << "           supports only Verilog). If a mismatch is detected reports\n"
      << "           useful information the user.\n"
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
      << "           when -fsanitize=address is passed to GCC or CLANG.\n"
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

#if HAVE_MODELSIM
      << "    --assert-debug\n"
      << "        Enable assertion debugging performed by Modelsim.\n\n"
#endif
      << std::endl;
}

void BambuParameter::PrintProgramName(std::ostream& os) const
{
   os << std::endl;
   os << "********************************************************************************" << std::endl;
   os << "                    ____                  _" << std::endl;
   os << "                   | __ )  __ _ _ __ ___ | |_   _   _" << std::endl;
   os << "                   |  _ \\ / _` | \'_ ` _ \\| \'_ \\| | | |" << std::endl;
   os << "                   | |_) | (_| | | | | | | |_) | |_| |" << std::endl;
   os << "                   |____/ \\__,_|_| |_| |_|_.__/ \\__,_|" << std::endl;
   os << std::endl;
   os << "********************************************************************************" << std::endl;
   os << "                         High-Level Synthesis Tool" << std::endl;
   os << std::endl;
}

BambuParameter::BambuParameter(const std::string& _program_name, int _argc, char** const _argv) : Parameter(_program_name, _argc, _argv)
{
   SetDefaults();
}

int BambuParameter::Exec()
{
   exit_code = PARAMETER_NOTPARSED;

   /// flag to check if scheduling algorithm has been already chosen
   bool scheduling_set_p = false;
   /// variable used into option parsing
   int option_index;

   // Bambu short option. An option character in this string can be followed by a colon (`:') to indicate that it
   // takes a required argument. If an option character is followed by two colons (`::'), its argument is optional;
   // this is a GNU extension.
   const char* const short_options = COMMON_SHORT_OPTIONS_STRING "o:t:u:H:sSC::b:w:p" GCC_SHORT_OPTIONS_STRING;

   const struct option long_options[] = {
      COMMON_LONG_OPTIONS,
      /// General options
      {"top-fname", required_argument, nullptr, OPT_TOP_FNAME},
      {"top-rtldesign-name", required_argument, nullptr, OPT_TOP_RTLDESIGN_NAME},
      {"xml-config", required_argument, nullptr, OPT_XML_CONFIG},
      {"time", required_argument, nullptr, 't'},
      {"file-input-data", required_argument, nullptr, INPUT_OPT_FILE_INPUT_DATA},
      /// Frontend options
      {"circuit-dbg", required_argument, nullptr, 0},
#if HAVE_EXPERIMENTAL
      {"pdg-reduction", optional_argument, nullptr, 0},
      ///--- Flow options ---
      {"dump-constraints", optional_argument, nullptr, OPT_DUMP_CONSTRAINTS},
   /// --- Design Space Exploration options ---
#if HAVE_BEAGLE
      {"dse", required_argument, nullptr, OPT_DSE},
      {"max-evaluations", required_argument, nullptr, OPT_MAX_EVALUATIONS},
      // ACO
      {"aco-flow", optional_argument, nullptr, OPT_ACO_FLOW},
      {"generations", optional_argument, nullptr, OPT_ACO_GENERATIONS},
      // Genetic algorithm
      {"mixed_synthesis", optional_argument, nullptr, 'H'},
      {"seed", required_argument, nullptr, 'A'},
      {"run", required_argument, nullptr, 'B'},
      {"population", required_argument, nullptr, 'E'},
      {"generation", required_argument, nullptr, OPT_GENERATION},
      {"fitness_function", required_argument, nullptr, 'P'},
      {"inheritance_rate", required_argument, nullptr, 'F'},
      {"inheritance_mode", required_argument, nullptr, 'G'},
      {"max_inheritance", required_argument, nullptr, OPT_MAX_INHERITANCE},
      {"min_inheritance", required_argument, nullptr, OPT_MIN_INHERITANCE},
      {"distance_rate", required_argument, nullptr, 'M'},
      {"weighting_function", required_argument, nullptr, 'N'},
      {"normalize", optional_argument, nullptr, 'Q'},
      {"time-weight", required_argument, nullptr, OPT_TIME_WEIGHT}, // no short option
      {"area-weight", required_argument, nullptr, OPT_AREA_WEIGHT}, // no short option
#endif
      /// --- Algorithms options ---
      {"explore-mux", optional_argument, nullptr, 0},
      {"explore-fu-reg", required_argument, nullptr, 0},
#endif
      /// Scheduling options
      {FIXED_SCHEDULING_OPT, required_argument, nullptr, OPT_FIXED_SCHED},
#if HAVE_ILP_BUILT
      {"speculative-sdc-scheduling", no_argument, nullptr, 's'},
#endif
      {"pipelining", no_argument, nullptr, 'p'},
      {"serialize-memory-accesses", no_argument, nullptr, OPT_SERIALIZE_MEMORY_ACCESSES},
      {PAR_LIST_BASED_OPT, optional_argument, nullptr, OPT_LIST_BASED}, // no short option
      {"post-rescheduling", no_argument, nullptr, OPT_POST_RESCHEDULING},
#if HAVE_ILP_BUILT
      {"ilp-solver", required_argument, nullptr, OPT_ILP_SOLVER},
      {"ilp", no_argument, nullptr, OPT_ILP},
      {"ilp-newform", no_argument, nullptr, OPT_ILP_NEWFORM},
      {"silp", no_argument, nullptr, OPT_SILP},
#endif
      {"speculative", no_argument, nullptr, OPT_SPECULATIVE},
      {"no-chaining", no_argument, nullptr, 0},
      /// Finite state machine options
      {"stg", required_argument, nullptr, OPT_STG},
      /// module binding
      {"module-binding", required_argument, nullptr, 0},
      /// register allocation
      {"register-allocation", required_argument, nullptr, OPT_REGISTER_ALLOCATION},
      {"storage-value-insertion", required_argument, nullptr, 0},
      /// Memory allocation
      {"memory-allocation", required_argument, nullptr, 0},
      {"memory-allocation-policy", required_argument, nullptr, 0},
      {"xml-memory-allocation", required_argument, nullptr, 0},
      {"base-address", required_argument, nullptr, 0},
      {"initial-internal-address", required_argument, nullptr, 0},
      {"channels-type", required_argument, nullptr, 0},
      {"channels-number", required_argument, nullptr, 0},
      {"memory-ctrl-type", required_argument, nullptr, 0},
      {"sparse-memory", optional_argument, nullptr, 0},
      {"do-not-expose-globals", no_argument, nullptr, OPT_DO_NOT_EXPOSE_GLOBALS},
      // interconnections
      {"interconnection", required_argument, nullptr, 'C'},
#if HAVE_EXPERIMENTAL
      {"parallel-controller", no_argument, nullptr, OPT_PARALLEL_CONTROLLER},
#if HAVE_CUDD
      {"logical-optimization", required_argument, nullptr, OPT_LOGICAL_OPTIMIZATION},
#endif
#endif
      /// evaluation options
      {"evaluation", optional_argument, nullptr, OPT_EVALUATION},
#if HAVE_EXPERIMENTAL
      {"evaluation-mode", required_argument, nullptr, OPT_EVALUATION_MODE},
      {"timing-simulation", no_argument, nullptr, 0},
      {"timing-violation", no_argument, nullptr, OPT_TIMING_VIOLATION},
#endif
      {"assert-debug", no_argument, nullptr, 0},
      {"device-name", required_argument, nullptr, OPT_DEVICE_NAME},
      {"clock-period", required_argument, nullptr, OPT_PERIOD_CLOCK},
      {"clock-name", required_argument, nullptr, OPT_CLOCK_NAME},
      {"reset-name", required_argument, nullptr, OPT_RESET_NAME},
      {"start-name", required_argument, nullptr, OPT_START_NAME},
      {"done-name", required_argument, nullptr, OPT_DONE_NAME},
      {"power-optimization", no_argument, nullptr, OPT_POWER_OPTIMIZATION},
      {"no-iob", no_argument, nullptr, OPT_DISABLE_IOB},
      {"reset-type", required_argument, nullptr, OPT_RESET},
      {"reset-level", required_argument, nullptr, OPT_LEVEL_RESET},
      {"disable-reg-init-value", no_argument, nullptr, OPT_DISABLE_REG_INIT_VALUE},
      {"soft-float", no_argument, nullptr, OPT_SOFT_FLOAT},
#if HAVE_FLOPOCO
      {"flopoco", no_argument, nullptr, OPT_FLOPOCO},
#endif
      {"softfloat-subnormal", no_argument, nullptr, OPT_SOFTFLOAT_SUBNORMAL},
      {"libm-std-rounding", no_argument, nullptr, OPT_LIBM_STD_ROUNDING},
      {"soft-fp", no_argument, nullptr, OPT_SOFT_FP},
      {"hls-div", optional_argument, nullptr, OPT_HLS_DIV},
      {"hls-fpdiv", optional_argument, nullptr, OPT_HLS_FPDIV},
      {"max-ulp", required_argument, nullptr, OPT_MAX_ULP},
      {"skip-pipe-parameter", required_argument, nullptr, OPT_SKIP_PIPE_PARAMETER},
      {"unaligned-access", no_argument, nullptr, OPT_UNALIGNED_ACCESS_PARAMETER},
      {"aligned-access", no_argument, nullptr, OPT_ALIGNED_ACCESS_PARAMETER},
      {"backend-script-extensions", required_argument, nullptr, OPT_BACKEND_SCRIPT_EXTENSIONS_PARAMETER},
      {"backend-sdc-extensions", required_argument, nullptr, OPT_BACKEND_SDC_EXTENSIONS_PARAMETER},
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
      {"mux-margins", required_argument, nullptr, OPT_SCHEDULING_MUX_MARGINS},
      {"use-ALUs", no_argument, nullptr, OPT_USE_ALUS},
      {"timing-model", required_argument, nullptr, OPT_TIMING_MODEL},
      {"registered-inputs", required_argument, nullptr, OPT_REGISTERED_INPUTS},
      {"fsm-encoding", required_argument, nullptr, OPT_FSM_ENCODING},
      /// target options
      {"target-file", required_argument, nullptr, 'b'},
#if HAVE_EXPERIMENTAL
      {"edk-config", required_argument, nullptr, 0},
#endif
      {"export-core", required_argument, nullptr, 0},
      /// Output options
      {"writer", required_argument, nullptr, 'w'},
#if HAVE_EXPERIMENTAL
      {"no-mixed-design", no_argument, nullptr, OPT_NO_MIXED_DESIGN},
#endif
      {"dynamic-generators-dir", required_argument, nullptr, OPT_DYNAMIC_GENERATORS_DIR},
      {"pretty-print", required_argument, nullptr, OPT_PRETTY_PRINT},
      {"pragma-parse", no_argument, nullptr, OPT_PRAGMA_PARSE},
      {"generate-interface", required_argument, nullptr, 0},
      {"additional-top", required_argument, nullptr, OPT_ADDITIONAL_TOP},
      {"data-bus-bitsize", required_argument, nullptr, 0},
      {"addr-bus-bitsize", required_argument, nullptr, 0},
#if HAVE_EXPERIMENTAL
      {"resp-model", required_argument, nullptr, 0},
#endif
      {"generate-tb", required_argument, nullptr, OPT_TESTBENCH},
      {"testbench-extra-gcc-flags", required_argument, nullptr, OPT_TESTBENCH_EXTRA_GCC_FLAGS},
      {"max-sim-cycles", required_argument, nullptr, OPT_MAX_SIM_CYCLES},
      {"generate-vcd", no_argument, nullptr, OPT_GENERATE_VCD},
      {"simulate", no_argument, nullptr, OPT_SIMULATE},
#if HAVE_MENTOR_VISUALIZER_EXE
      {"mentor-visualizer", no_argument, nullptr, OPT_VISUALIZER},
#endif
      {"simulator", required_argument, nullptr, 0},
      {"disable-function-proxy", no_argument, nullptr, OPT_DISABLE_FUNCTION_PROXY},
      {"disable-bounded-function", no_argument, nullptr, OPT_DISABLE_BOUNDED_FUNCTION},
      {"memory-mapped-top", no_argument, nullptr, OPT_MEMORY_MAPPED_TOP},
      {"mem-delay-read", required_argument, nullptr, OPT_MEM_DELAY_READ},
      {"mem-delay-write", required_argument, nullptr, OPT_MEM_DELAY_WRITE},
      {"host-profiling", no_argument, nullptr, OPT_HOST_PROFILING},
      {"disable-bitvalue-ipa", no_argument, nullptr, OPT_DISABLE_BITVALUE_IPA},
      {"discrepancy", no_argument, nullptr, OPT_DISCREPANCY},
      {"discrepancy-force-uninitialized", no_argument, nullptr, OPT_DISCREPANCY_FORCE},
      {"discrepancy-hw", no_argument, nullptr, OPT_DISCREPANCY_HW},
      {"discrepancy-no-load-pointers", no_argument, nullptr, OPT_DISCREPANCY_NO_LOAD_POINTERS},
      {"discrepancy-only", required_argument, nullptr, OPT_DISCREPANCY_ONLY},
      {"discrepancy-permissive-ptrs", no_argument, nullptr, OPT_DISCREPANCY_PERMISSIVE_PTRS},
#if HAVE_FROM_PRAGMA_BUILT && HAVE_BAMBU_BUILT
      {"num-accelerators", required_argument, nullptr, OPT_NUM_ACCELERATORS},
      {"context_switch", optional_argument, nullptr, OPT_INPUT_CONTEXT_SWITCH},
#endif
      {"memory-banks-number", required_argument, nullptr, OPT_MEMORY_BANKS_NUMBER},
      {"C-no-parse", required_argument, nullptr, INPUT_OPT_C_NO_PARSE},
      {"C-python-no-parse", required_argument, nullptr, INPUT_OPT_C_PYTHON_NO_PARSE},
      {"accept-nonzero-return", no_argument, nullptr, OPT_ACCEPT_NONZERO_RETURN},
      {"find-max-cfg-transformations", no_argument, nullptr, INPUT_OPT_FIND_MAX_CFG_TRANSFORMATIONS},
#if !HAVE_UNORDERED
#ifndef NDEBUG
      {"test-multiple-non-deterministic-flows", required_argument, nullptr, INPUT_OPT_TEST_MULTIPLE_NON_DETERMINISTIC_FLOWS},
      {"test-single-non-deterministic-flow", required_argument, nullptr, INPUT_OPT_TEST_SINGLE_NON_DETERMINISTIC_FLOW},
#endif
#endif
      {"dry-run-evaluation", no_argument, nullptr, INPUT_OPT_DRY_RUN_EVALUATION},
      GCC_LONG_OPTIONS,
      {nullptr, 0, nullptr, 0}
   };

   if(argc == 1) // Bambu called without arguments, it simple prints help message
   {
      PrintUsage(std::cout);
      return EXIT_SUCCESS;
   }

   while(true)
   {
      int next_option = getopt_long(argc, argv, short_options, long_options, &option_index);

      // no more options are available
      if(next_option == -1)
         break;

      switch(next_option)
      {
         /// general options
         case OPT_TOP_FNAME:
         {
            // set name of the function to be take into account as top function
            std::string top_function_names;
            std::vector<std::string> splitted;
            std::string to_be_splitted = std::string(optarg);
            boost::split(splitted, to_be_splitted, boost::is_any_of(","));
            for(const auto& counter : splitted)
            {
               if(top_function_names != "")
                  top_function_names += STR_CST_string_separator;
               top_function_names += counter;
            }
            setOption(OPT_top_functions_names, top_function_names);
            if(splitted.size() == 1)
               setOption(OPT_top_file, optarg);
            break;
         }
         case OPT_TOP_RTLDESIGN_NAME:
         {
            setOption(OPT_top_design_name, optarg);
            break;
         }
         case 'o':
            setOption(OPT_output_file, optarg);
            break;
         case 'S':
         {
            setOption(OPT_serialize_output, true);
            break;
         }
         case 't':
         {
            setOption(OPT_ilp_max_time, optarg);
            break;
         }
         case INPUT_OPT_FILE_INPUT_DATA:
         {
            setOption(OPT_file_input_data, optarg);
            break;
         }
#if HAVE_EXPERIMENTAL
         case OPT_XML_CONFIG:
         {
            setOption(OPT_synthesis_flow, HLSFlowStep_Type::XML_HLS_SYNTHESIS_FLOW);
            setOption(OPT_xml_input_configuration, optarg);
            break;
         }
         case OPT_DUMP_CONSTRAINTS:
         {
            setOption(OPT_hls_flow, HLSFlowStep_Type::DUMP_DESIGN_FLOW);
            setOption("dumpConstraints", true);
            if(optarg)
               setOption("dumpConstraints_file", optarg);
            break;
         }
         /// frontend options
         case 'u': // compute the unrolling degree of a loop
         {
            setOption("compute_unrolling_degree", true);
            if(optarg)
               setOption("IImax", optarg);
            else
               throw "BadParameters: compute unrolling degree needs a additional parameter";
            break;
         }
         case OPT_INSERT_VERIFICATION_OPERATION:
         {
            setOption("insert_verification_operation", true);
            break;
         }
         /// design space exploration options
#if HAVE_BEAGLE
         case OPT_DSE:
         {
            setOption(OPT_hls_flow, hls_flow::DSE);
            if(std::string(optarg) == ("MOSA"))
               setOption("dse_algorithm", dse_hls::MOSA);
            else if(std::string(optarg) == ("MOTS"))
               setOption("dse_algorithm", dse_hls::MOTS);
            else if(std::string(optarg) == ("ACO"))
               setOption("dse_algorithm", dse_hls::ACO);
            else if(std::string(optarg) == ("GRASP"))
               setOption("dse_algorithm", dse_hls::GRASP);
            else if(std::string(optarg) == ("MOGA"))
               setOption("dse_algorithm", dse_hls::MOGA);
            else
               THROW_ERROR("Not supported exploration algorithm: " + std::string(optarg));
            break;
         }
         case OPT_MAX_EVALUATIONS:
         {
            setOption("max_evaluations", optarg);
            break;
         }
         // ACO
         case OPT_ACO_FLOW:
         {
            setOption(OPT_hls_flow, hls_flow::DSE);
            setOption("dse_algorithm", dse_hls::ACO);
            if(optarg)
               setOption("aco_ant_num", optarg);
            break;
         }
         case OPT_ACO_GENERATIONS:
         {
            if(optarg)
               setOption("aco_generations", optarg);
            break;
         }
         case OPT_MOSA_FLOW:
         {
            setOption(OPT_hls_flow, hls_flow::DSE);
            setOption("dse_algorithm", dse_hls::MOSA);
            break;
         }
         // multi-objective genetic algorithm
         case 'H': // enable mixed high level synthesis
         {
            setOption(OPT_hls_flow, hls_flow::DSE);
            setOption("dse_algorithm", dse_hls::MOGA);
            if(optarg)
               setOption("exploration_technique", optarg);
            else
               setOption("exploration_technique", dse_hls::BINDING);
            break;
         }
         case 'Q':
         {
            if(optarg)
               setOption("to_normalize", optarg);
            else
               setOption("to_normalize", 1);
            break;
         }
         case 'P': // mixed high level synthesis fitness function
         {
            setOption("fitness_function", optarg);
            break;
         }
         case 'A':
         {
            setOption("seed", optarg);
            break;
         }
         case 'B':
         {
            setOption("run", optarg);
            break;
         }
         case 'E':
         {
            setOption("population", optarg);
            break;
         }
         case OPT_GENERATION:
         {
            setOption("GA_generation", optarg);
            break;
         }
         case 'F':
         {
            setOption("fitness_inheritance_rate", optarg);
            break;
         }
         case 'G':
         {
            setOption("inheritance_mode", optarg);
            break;
         }
         case OPT_MAX_INHERITANCE:
         {
            setOption("max_for_inheritance", optarg);
            break;
         }
         case OPT_MIN_INHERITANCE:
         {
            setOption("min_for_inheritance", optarg);
            break;
         }
         case 'M':
         {
            setOption("distance_rate", optarg);
            break;
         }
         case 'N':
         {
            setOption("weighting_function", optarg);
            break;
         }
         case OPT_TIME_WEIGHT:
         {
            setOption("time_weight", optarg);
            break;
         }
         case OPT_AREA_WEIGHT:
         {
            setOption("area_weight", optarg);
            break;
         }
#endif
#endif
         /// scheduling options
         case OPT_FIXED_SCHED:
         {
            if(scheduling_set_p)
               THROW_ERROR("BadParameters: only one scheduler can be specified");
            scheduling_set_p = true;
            setOption(OPT_scheduling_algorithm, HLSFlowStep_Type::FIXED_SCHEDULING);
            if(optarg)
               setOption("fixed_scheduling_file", optarg);
            break;
         }
         case OPT_LIST_BASED: // enable list based scheduling
         {
            if(scheduling_set_p)
               THROW_ERROR("BadParameters: only one scheduler can be specified");
            scheduling_set_p = true;
            setOption(OPT_scheduling_algorithm, HLSFlowStep_Type::LIST_BASED_SCHEDULING);
            if(optarg)
               setOption(OPT_scheduling_priority, optarg);
            break;
         }
         case OPT_POST_RESCHEDULING:
         {
            setOption(OPT_post_rescheduling, true);
            break;
         }
#if HAVE_EXPERIMENTAL
#if HAVE_ILP_BUILT
         case OPT_ILP_SOLVER:
         {
#if HAVE_GLPK
            if(optarg[0] == 'G')
            {
               setOption(OPT_ilp_solver, meilp_solver::GLPK);
            }
            else
#endif
#if HAVE_COIN_OR
                if(optarg[0] == 'C')
            {
               setOption(OPT_ilp_solver, meilp_solver::COIN_OR);
            }
            else
#endif
#if HAVE_LP_SOLVE
                if(optarg[0] == 'L')
            {
               setOption(OPT_ilp_solver, meilp_solver::LP_SOLVE);
            }
            else
#endif
            {
               THROW_ERROR("BadParameters: not recognized ilp solver");
            }
            break;
         }
         case OPT_ILP:
         {
            if(scheduling_set_p)
               THROW_ERROR("BadParameters: only one scheduler can be specified");
            scheduling_set_p = true;
            setOption(OPT_scheduling_algorithm, HLSFlowStep_Type::ILP_SCHEDULING);
            setOption(OPT_chaining, true); /// ILP formulation tries to chain the operations
            break;
         }
         case OPT_ILP_NEWFORM:
         {
            if(scheduling_set_p)
               THROW_ERROR("BadParameters: only one scheduler can be specified");
            scheduling_set_p = true;
            setOption(OPT_scheduling_algorithm, HLSFlowStep_Type::ILP_NEW_FORM_SCHEDULING);
            break;
         }
         case OPT_SILP:
         {
            if(scheduling_set_p)
               THROW_ERROR("BadParameters: only one scheduler can be specified");
            scheduling_set_p = true;
            setOption(OPT_scheduling_algorithm, HLSFlowStep_Type::SILP_SCHEDULING);
            break;
         }
#endif
         case OPT_SPECULATIVE: // enable scheduling with speculative computation
         {
            setOption(OPT_speculative, true);
            break;
         }
#endif
         case OPT_STG:
         {
            setOption(OPT_stg, true);
            if(optarg)
               setOption(OPT_stg_algorithm, optarg);
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
#if HAVE_EXPERIMENTAL
            else if(std::string(optarg) == "LEFT_EDGE")
            {
               setOption(OPT_register_allocation_algorithm, HLSFlowStep_Type::LEFT_EDGE_REGISTER_BINDING);
            }
            else if(std::string(optarg) == "K_COFAMILY")
            {
               setOption(OPT_register_allocation_algorithm, HLSFlowStep_Type::K_COFAMILY_REGISTER_BINDING);
            }
#endif
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
               THROW_ERROR("BadParameters: register allocation not correctly specified");
            }
            break;
         }
#if HAVE_EXPERIMENTAL
         case OPT_TIMING_VIOLATION:
         {
            setOption(OPT_timing_violation_abort, true);
            break;
         }
#endif
         // interconnection
         case 'C':
         {
            if(std::string(optarg) == "M")
               setOption(OPT_datapath_interconnection_algorithm, HLSFlowStep_Type::MUX_INTERCONNECTION_BINDING);
            else
               throw "BadParameters: interconnection binding not correctly specified";
            break;
         }
#if HAVE_EXPERIMENTAL
         case OPT_PARALLEL_CONTROLLER:
         {
            setOption(OPT_controller_architecture, HLSFlowStep_Type::PARALLEL_CONTROLLER_CREATOR);
            break;
         }
#endif
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
             * example with OPT_EVALUATION_MODE). in cas it's already set, we
             * don't overwrite it since OPT_EVALUATION is meant to set the
             * objectives, not the mode, hence the mode set from other options
             * has precedence
             */
            if(not isOption(OPT_evaluation_mode) or getOption<Evaluation_Mode>(OPT_evaluation_mode) == Evaluation_Mode::NONE)
            {
               setOption(OPT_evaluation_mode, Evaluation_Mode::EXACT);
            }
            std::string objective_string = getOption<std::string>(OPT_evaluation_objectives);
            std::vector<std::string> objective_vector = convert_string_to_vector<std::string>(objective_string, ",");
            objective_string = "";
            for(const auto& objective : objective_vector)
            {
               if(objective == "CYCLES")
               {
                  objective_string += ",CYCLES";
               }
               else if(objective == "TOTAL_CYCLES")
               {
                  objective_string += ",TOTAL_CYCLES";
               }
            }
            if(optarg == nullptr)
            {
               if(isOption(OPT_evaluation_mode) and getOption<Evaluation_Mode>(OPT_evaluation_mode) == Evaluation_Mode::EXACT)
               {
                  std::string to_add =
#if HAVE_LIBRARY_CHARACTERIZATION_BUILT
                      "AREAxTIME,"
                      "AREA,"
                      "REGISTERS,"
                      "DSPS,"
                      "BRAMS,"
                      "PERIOD,"
                      "CLOCK_SLACK,"
                      "FREQUENCY,"
                      "TIME,"
                      "TOTAL_TIME,"
                      "CYCLES,"
                      "TOTAL_CYCLES"
#else
                      "CYCLES"
#endif
                      ;
                  add_evaluation_objective_string(objective_string, to_add);
               }
#if HAVE_EXPERIMENTAL
               else if(isOption(OPT_evaluation_mode) and getOption<Evaluation_Mode>(OPT_evaluation_mode) == Evaluation_Mode::ESTIMATION)
               {
                  add_evaluation_objective_string(objective_string, "AREA,"
                                                                    "TIME,"
                                                                    "CLOCK_SLACK");
               }
#endif
               else
               {
                  THROW_ERROR("BadParameters: invalid evaluation mode");
               }
            }
            else
            {
               add_evaluation_objective_string(objective_string, std::string(optarg));
            }
            setOption(OPT_evaluation_objectives, objective_string);
            break;
         }
#if HAVE_EXPERIMENTAL
         case OPT_EVALUATION_MODE:
         {
            // set OPT_evaluation, because the evaluation has to be performed
            setOption(OPT_evaluation, true);
            /*
             * here don't check if OPT_evaluation mode was already set by
             * someone else. if someone else has already defined
             * OPT_evaluation_mode the --evaluation-mode parameter should
             * overwrite it anyway.
             */
            if(optarg == nullptr)
            {
               throw "BadParameters: evaluation mode must be specified, use EXACT or LINEAR";
            }
            else if(std::string(optarg) == "EXACT")
            {
               setOption(OPT_evaluation_mode, Evaluation_Mode::EXACT);
            }
            else if(std::string(optarg) == "LINEAR")
            {
               setOption(OPT_evaluation_mode, Evaluation_Mode::ESTIMATION);
            }
            else
            {
               throw "BadParameters: evaluation mode not correctly specified, use EXACT or LINEAR";
            }
            break;
         }
#endif
         case OPT_SIMULATE:
         {
            /*
             * OPT_SIMULATE is a shorthand form OPT_EVALUATION with optarg =
             * CYCLES,TOTAL_CYCLES
             */
            // set OPT_evaluation, because the evaluation has to be performed
            setOption(OPT_evaluation, true);
            /*
             * check if OPT_evaluation_mode has already been decided (for
             * example with OPT_EVALUATION_MODE). in cas it's already set, we
             * don't overwrite it since OPT_SIMULATION is meant to set the
             * objectives, not the mode, hence the mode set from other options
             * has precedence
             */
            if(not isOption(OPT_evaluation_mode) or getOption<Evaluation_Mode>(OPT_evaluation_mode) == Evaluation_Mode::NONE)
            {
               setOption(OPT_evaluation_mode, Evaluation_Mode::EXACT);
            }
            else if(isOption(OPT_evaluation_mode) and getOption<Evaluation_Mode>(OPT_evaluation_mode) != Evaluation_Mode::EXACT)
            {
               THROW_ERROR("Simulation is only supported with EXACT evaluation mode");
            }
            std::string objective_string = getOption<std::string>(OPT_evaluation_objectives);
            std::vector<std::string> objective_vector = convert_string_to_vector<std::string>(objective_string, ",");
            /*
             * look among the objectives of the evaluation. if "CYCLES" is
             * already there, the simulation will be executed.
             * if it's not, it's enough to add it to enable simulation
             */
            if(objective_string.empty())
            {
               objective_string = "CYCLES";
            }
            else if(std::find(objective_vector.begin(), objective_vector.end(), "CYCLES") == objective_vector.end())
            {
               objective_string = objective_string + ",CYCLES";
            }
            setOption(OPT_evaluation_objectives, objective_string);
            break;
         }
#if HAVE_MENTOR_VISUALIZER_EXE
         case OPT_VISUALIZER:
         {
            setOption(OPT_visualizer, true);
            break;
         }
#endif
         case OPT_DEVICE_NAME:
         {
            std::string tmp_string = optarg;
            std::vector<std::string> values = convert_string_to_vector<std::string>(tmp_string, std::string(","));
            setOption("device_name", "");
            setOption("device_speed", "");
            setOption("device_package", "");
            setOption("device_synthesis_tool", "");
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
            else if(values.size() == 4)
            {
               setOption("device_name", values[0]);
               setOption("device_speed", values[1]);
               setOption("device_package", values[2]);
               setOption("device_synthesis_tool", values[3]);
            }
            else
            {
               THROW_ERROR("Malformed device: " + tmp_string);
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
         case OPT_POWER_OPTIMIZATION:
         {
            setOption("power_optimization", true);
            break;
         }
         case OPT_DISABLE_IOB:
         {
            setOption(OPT_connect_iob, false);
            break;
         }
         case OPT_RESET:
         {
            if(std::string(optarg) == "no")
               setOption(OPT_sync_reset, std::string(optarg));
            else if(std::string(optarg) == "async")
               setOption(OPT_sync_reset, std::string(optarg));
            else if(std::string(optarg) == "sync")
               setOption(OPT_sync_reset, std::string(optarg));
            else
               throw "BadParameters: reset type not correctly specified";
            break;
         }
         case OPT_LEVEL_RESET:
         {
            if(std::string(optarg) == "high")
               setOption(OPT_level_reset, true);
            else if(std::string(optarg) == "low")
               setOption(OPT_level_reset, false);
            else
               throw "BadParameters: reset edge type not correctly specified";
            break;
         }
         case OPT_DISABLE_REG_INIT_VALUE:
         {
            setOption(OPT_reg_init_value, false);
            break;
         }
#if HAVE_ILP_BUILT
         case 's':
         {
            if(scheduling_set_p and getOption<HLSFlowStep_Type>(OPT_scheduling_algorithm) != HLSFlowStep_Type::SDC_SCHEDULING)
               THROW_ERROR("BadParameters: only one scheduler can be specified");
            scheduling_set_p = true;
            setOption(OPT_scheduling_algorithm, HLSFlowStep_Type::SDC_SCHEDULING);
            std::string defines;
            if(isOption(OPT_gcc_defines))
               defines = getOption<std::string>(OPT_gcc_defines) + STR_CST_string_separator;
            defines += std::string("PANDA_SDC");
            setOption(OPT_gcc_defines, defines);
            break;
         }
#endif
         case 'p':
         {
            setOption(OPT_pipelining, true);
            break;
         }
         case OPT_SERIALIZE_MEMORY_ACCESSES:
         {
            setOption(OPT_gcc_serialize_memory_accesses, true);
            break;
         }
         case OPT_SOFT_FLOAT:
         {
            setOption(OPT_soft_float, true);
            break;
         }
#if HAVE_FLOPOCO
         case OPT_FLOPOCO:
         {
            setOption(OPT_soft_float, false);
            break;
         }
#endif
         case OPT_SOFTFLOAT_SUBNORMAL:
         {
            setOption(OPT_softfloat_subnormal, true);
            break;
         }
         case OPT_LIBM_STD_ROUNDING:
         {
            setOption(OPT_libm_std_rounding, true);
            break;
         }
         case OPT_SOFT_FP:
         {
            setOption(OPT_soft_fp, true);
            setOption(OPT_hls_fpdiv, "SF");
            break;
         }
         case OPT_HLS_DIV:
         {
            setOption(OPT_hls_div, "NR");
            if(optarg && std::string(optarg) == "nr1")
               setOption(OPT_hls_div, optarg);
            else if(optarg && std::string(optarg) == "nr2")
               setOption(OPT_hls_div, optarg);
            else if(optarg && std::string(optarg) == "as")
               setOption(OPT_hls_div, optarg);
            else if(optarg && std::string(optarg) == "none")
               setOption(OPT_hls_div, optarg);
            break;
         }
         case OPT_HLS_FPDIV:
         {
            setOption(OPT_hls_fpdiv, "SRT4");
            if(optarg && (std::string(optarg) == "G" || std::string(optarg) == "SF"))
               setOption(OPT_hls_fpdiv, optarg);
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
               setOption(OPT_estimate_logic_and_connections, false);
            else if(std::string(optarg) == "EC")
               setOption(OPT_estimate_logic_and_connections, true);
            else
               throw "BadParameters: unknown timing model";
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
            setOption(OPT_max_ulp, optarg);
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
            setOption(OPT_backend_sdc_extensions, optarg);
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
               setOption(OPT_bram_high_latency, "_4");
            break;
         }
         case OPT_DO_NOT_EXPOSE_GLOBALS:
         {
            setOption(OPT_do_not_expose_globals, true);
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
         /// output options
         case 'w':
         {
            if(std::string(optarg) == "V")
               setOption(OPT_writer_language, static_cast<int>(HDLWriter_Language::VERILOG));
#if HAVE_EXPERIMENTAL
            else if(std::string(optarg) == "S")
               setOption(OPT_writer_language, static_cast<int>(HDLWriter_Language::SYSTEMC));
#endif
            else if(std::string(optarg) == "H")
               setOption(OPT_writer_language, static_cast<int>(HDLWriter_Language::VHDL));
            else
               throw "BadParameters: backend language not correctly specified";
            break;
         }
         case OPT_DYNAMIC_GENERATORS_DIR:
         {
            setOption("dynamic_generators_dir", optarg);
            break;
         }
         case OPT_PRETTY_PRINT:
         {
            setOption(OPT_pretty_print, optarg);
            break;
         }
         case OPT_PRAGMA_PARSE:
         {
            setOption(OPT_parse_pragma, true);
            break;
         }
         case OPT_TESTBENCH:
         {
            setOption(OPT_generate_testbench, true);
            const std::string arg = TrimSpaces(std::string(optarg));
            if(arg.size() >= 4 and arg.substr(arg.size() - 4) == ".xml")
            {
               setOption(OPT_testbench_input_xml, optarg);
            }
            else
            {
               setOption(OPT_testbench_input_string, optarg);
            }
            break;
         }
         case OPT_TESTBENCH_EXTRA_GCC_FLAGS:
         {
            setOption(OPT_testbench_extra_gcc_flags, optarg);
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
#if HAVE_CUDD
         case OPT_LOGICAL_OPTIMIZATION:
         {
            if(optarg)
               setOption("logical_optimization", optarg);
            break;
         }
#endif
         case OPT_ADDITIONAL_TOP:
         {
            setOption(OPT_additional_top, optarg);
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
#if HAVE_HOST_PROFILING_BUILT
         case OPT_HOST_PROFILING:
         {
            setOption(OPT_profiling_method, static_cast<int>(HostProfiling_Method::PM_BBP));
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
         case OPT_DISCREPANCY_HW:
         {
            setOption(OPT_discrepancy_hw, true);
            break;
         }
         case OPT_DISCREPANCY_ONLY:
         {
            setOption(OPT_discrepancy, true);
            std::vector<std::string> splitted = SplitString(optarg, " ,");
            std::string discrepancy_functions;
            for(auto& f : splitted)
            {
               boost::trim(f);
               discrepancy_functions += f + STR_CST_string_separator;
            }
            setOption(OPT_discrepancy_only, discrepancy_functions);
            break;
         }
         case OPT_DISCREPANCY_PERMISSIVE_PTRS:
         {
            setOption(OPT_discrepancy, true);
            setOption(OPT_discrepancy_permissive_ptrs, true);
            break;
         }
#if HAVE_FROM_PRAGMA_BUILT && HAVE_BAMBU_BUILT
         case OPT_NUM_ACCELERATORS:
         {
            auto num_acc = boost::lexical_cast<unsigned>(std::string(optarg));
            if((num_acc != 0) && ((num_acc & (num_acc - 1)) == 0))
               setOption(OPT_num_accelerators, std::string(optarg));
            else
               THROW_ERROR("Currently the number of physical accelerator has to be a power of two");
            ;
            break;
         }
         case OPT_INPUT_CONTEXT_SWITCH:
         {
            if(optarg)
            {
               setOption(OPT_context_switch, std::string(optarg));
               break;
            }
            else
            {
               setOption(OPT_context_switch, "4");
               break;
            }
         }
#endif
         case OPT_MEMORY_BANKS_NUMBER:
         {
            setOption(OPT_memory_banks_number, std::string(optarg));
            break;
         }
         case OPT_ACCEPT_NONZERO_RETURN:
         {
            setOption(OPT_no_return_zero, true);
            break;
         }
         case INPUT_OPT_C_NO_PARSE:
         {
            std::vector<std::string> Splitted = SplitString(optarg, " ,");
            std::string no_parse_files;
            for(auto& i : Splitted)
            {
               boost::trim(i);
               no_parse_files += i + " ";
            }
            setOption(OPT_no_parse_files, no_parse_files);
            break;
         }
         case INPUT_OPT_C_PYTHON_NO_PARSE:
         {
            std::vector<std::string> Splitted = SplitString(optarg, " ,");
            std::string no_parse_c_python_files;
            for(auto& i : Splitted)
            {
               boost::trim(i);
               no_parse_c_python_files += i + " ";
            }
            setOption(OPT_no_parse_c_python, no_parse_c_python_files);
            break;
         }
         case INPUT_OPT_FIND_MAX_CFG_TRANSFORMATIONS:
         {
            setOption(OPT_find_max_cfg_transformations, true);
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
            setOption(OPT_dry_run_evaluation, true);
            break;
         }
         case 0:
         {
            if(strcmp(long_options[option_index].name, "module-binding") == 0)
            {
               if(std::string(optarg) == "TTT_FAST")
               {
                  setOption(OPT_fu_binding_algorithm, HLSFlowStep_Type::CDFC_MODULE_BINDING);
                  setOption(OPT_cdfc_module_binding_algorithm, CliqueCovering_Algorithm::TTT_CLIQUE_COVERING_FAST);
               }
               else if(std::string(optarg) == "TTT_FAST2")
               {
                  setOption(OPT_fu_binding_algorithm, HLSFlowStep_Type::CDFC_MODULE_BINDING);
                  setOption(OPT_cdfc_module_binding_algorithm, CliqueCovering_Algorithm::TTT_CLIQUE_COVERING_FAST2);
               }
               else if(std::string(optarg) == "TTT_FULL")
               {
                  setOption(OPT_fu_binding_algorithm, HLSFlowStep_Type::CDFC_MODULE_BINDING);
                  setOption(OPT_cdfc_module_binding_algorithm, CliqueCovering_Algorithm::TTT_CLIQUE_COVERING);
               }
               else if(std::string(optarg) == "TTT_FULL2")
               {
                  setOption(OPT_fu_binding_algorithm, HLSFlowStep_Type::CDFC_MODULE_BINDING);
                  setOption(OPT_cdfc_module_binding_algorithm, CliqueCovering_Algorithm::TTT_CLIQUE_COVERING2);
               }
               else if(std::string(optarg) == "TS")
               {
                  setOption(OPT_fu_binding_algorithm, HLSFlowStep_Type::CDFC_MODULE_BINDING);
                  setOption(OPT_cdfc_module_binding_algorithm, CliqueCovering_Algorithm::TS_CLIQUE_COVERING);
               }
               else if(std::string(optarg) == "WEIGHTED_TS")
               {
                  setOption(OPT_fu_binding_algorithm, HLSFlowStep_Type::CDFC_MODULE_BINDING);
                  setOption(OPT_cdfc_module_binding_algorithm, CliqueCovering_Algorithm::TS_WEIGHTED_CLIQUE_COVERING);
               }
               else if(std::string(optarg) == "COLORING")
               {
                  setOption(OPT_fu_binding_algorithm, HLSFlowStep_Type::CDFC_MODULE_BINDING);
                  setOption(OPT_cdfc_module_binding_algorithm, CliqueCovering_Algorithm::COLORING);
               }
               else if(std::string(optarg) == "WEIGHTED_COLORING")
               {
                  setOption(OPT_fu_binding_algorithm, HLSFlowStep_Type::CDFC_MODULE_BINDING);
                  setOption(OPT_cdfc_module_binding_algorithm, CliqueCovering_Algorithm::WEIGHTED_COLORING);
               }
               else if(std::string(optarg) == "BIPARTITE_MATCHING")
               {
                  setOption(OPT_fu_binding_algorithm, HLSFlowStep_Type::CDFC_MODULE_BINDING);
                  setOption(OPT_cdfc_module_binding_algorithm, CliqueCovering_Algorithm::BIPARTITE_MATCHING);
               }
#if HAVE_EXPERIMENTAL
               else if(std::string(optarg) == "RANDOMIZED")
               {
                  setOption(OPT_fu_binding_algorithm, HLSFlowStep_Type::CDFC_MODULE_BINDING);
                  setOption(OPT_cdfc_module_binding_algorithm, CliqueCovering_Algorithm::RANDOMIZED);
               }
#endif
               else if(std::string(optarg) == "UNIQUE")
               {
                  setOption(OPT_fu_binding_algorithm, HLSFlowStep_Type::UNIQUE_MODULE_BINDING);
               }
               else
                  throw "BadParameters: module binding option not correctly specified";
               break;
            }
            if(strcmp(long_options[option_index].name, "memory-allocation") == 0)
            {
               if(std::string(optarg) == "DOMINATOR")
                  setOption(OPT_memory_allocation_algorithm, HLSFlowStep_Type::DOMINATOR_MEMORY_ALLOCATION);
               else if(std::string(optarg) == "XML_SPECIFICATION")
                  setOption(OPT_memory_allocation_algorithm, HLSFlowStep_Type::XML_MEMORY_ALLOCATOR);
               else
                  throw "BadParameters: memory allocation option not correctly specified";
               break;
            }
            if(strcmp(long_options[option_index].name, "xml-memory-allocation") == 0)
            {
               setOption(OPT_memory_allocation_algorithm, HLSFlowStep_Type::XML_MEMORY_ALLOCATOR);
               setOption(OPT_xml_memory_allocation, optarg);
               break;
            }
            if(strcmp(long_options[option_index].name, "memory-allocation-policy") == 0)
            {
               if(std::string(optarg) == "LSS")
                  setOption(OPT_memory_allocation_policy, MemoryAllocation_Policy::LSS);
               else if(std::string(optarg) == "GSS")
                  setOption(OPT_memory_allocation_policy, MemoryAllocation_Policy::GSS);
               else if(std::string(optarg) == "ALL_BRAM")
                  setOption(OPT_memory_allocation_policy, MemoryAllocation_Policy::ALL_BRAM);
               else if(std::string(optarg) == "NO_BRAM")
                  setOption(OPT_memory_allocation_policy, MemoryAllocation_Policy::NO_BRAM);
               else if(std::string(optarg) == "EXT_PIPELINED_BRAM")
                  setOption(OPT_memory_allocation_policy, MemoryAllocation_Policy::EXT_PIPELINED_BRAM);
               else if(std::string(optarg) == "INTERN_UNALIGNED")
                  setOption(OPT_memory_allocation_policy, MemoryAllocation_Policy::INTERN_UNALIGNED);
               else
                  throw "BadParameters: memory allocation policy option not correctly specified";
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
                  if(not isOption(OPT_channels_number))
                     setOption(OPT_channels_number, 1);
               }
               else if(std::string(optarg) == CHANNELS_TYPE_MEM_ACC_N1)
                  setOption(OPT_channels_type, MemoryAllocation_ChannelsType::MEM_ACC_N1);
               else if(std::string(optarg) == CHANNELS_TYPE_MEM_ACC_NN)
                  setOption(OPT_channels_type, MemoryAllocation_ChannelsType::MEM_ACC_NN);
               else
                  throw "BadParameters: memory accesses type not correctly specified";
               break;
            }
            if(strcmp(long_options[option_index].name, "channels-number") == 0)
            {
               setOption(OPT_channels_number, optarg);
               break;
            }
            if(strcmp(long_options[option_index].name, "memory-ctrl-type") == 0)
            {
               if(std::string(optarg) == "D00")
                  setOption(OPT_memory_controller_type, optarg);
               else if(std::string(optarg) == "D10")
                  setOption(OPT_memory_controller_type, optarg);
               else if(std::string(optarg) == "D11")
                  setOption(OPT_memory_controller_type, optarg);
               else if(std::string(optarg) == "D21")
                  setOption(OPT_memory_controller_type, optarg);
               else
                  throw "BadParameters: memory controller type not correctly specified";
               break;
            }
            if(strcmp(long_options[option_index].name, "sparse-memory") == 0)
            {
               if(!optarg || std::string(optarg) == "on")
                  setOption(OPT_sparse_memory, true);
               else if(std::string(optarg) == "off")
                  setOption(OPT_sparse_memory, false);
               else
                  throw "BadParameters: sparse-memory option not expected";
               break;
            }
#if HAVE_EXPERIMENTAL
            if(strcmp(long_options[option_index].name, "timing-simulation") == 0)
            {
               setOption(OPT_timing_simulation, true);
               break;
            }
            if(strcmp(long_options[option_index].name, "resp-model") == 0)
            {
               setOption(OPT_resp_model, optarg);
               break;
            }
            // front-end analysis option
            if(strcmp(long_options[option_index].name, "pdg-reduction") == 0)
            {
               // Set the default
               if(optarg)
                  setOption("pdg-reduction", optarg);
               break;
            }
            if(strcmp(long_options[option_index].name, "explore-mux") == 0)
            {
               setOption("explore-mux", true);
               if(optarg)
                  setOption("worst_case_delay", optarg);
               break;
            }
            if(strcmp(long_options[option_index].name, "explore-fu-reg") == 0)
            {
               setOption("explore-fu-reg", true);
               setOption("explore-fu-reg-param", optarg);
               break;
            }
#endif
            if(strcmp(long_options[option_index].name, "assert-debug") == 0)
            {
               setOption(OPT_assert_debug, true);
               break;
            }
            if(strcmp(long_options[option_index].name, "circuit-dbg") == 0)
            {
               setOption(OPT_circuit_debug_level, optarg);
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
               if(std::string(optarg) == "INFER")
               {
                  setOption(OPT_interface_type, HLSFlowStep_Type::INFERRED_INTERFACE_GENERATION);
               }
               else if(std::string(optarg) == "WB4")
               {
                  setOption(OPT_interface_type, HLSFlowStep_Type::WB4_INTERFACE_GENERATION);
               }
#if HAVE_EXPERIMENTAL
               else if(std::string(optarg) == "AXI4LITE")
               {
                  setOption(OPT_interface_type, HLSFlowStep_Type::AXI4LITE_INTERFACE_GENERATION);
               }
               else if(std::string(optarg) == "FSL")
               {
                  setOption(OPT_interface_type, HLSFlowStep_Type::FSL_INTERFACE_GENERATION);
               }
               else if(std::string(optarg) == "NPI")
               {
                  setOption(OPT_interface_type, HLSFlowStep_Type::NPI_INTERFACE_GENERATION);
               }
#endif
               else
               {
                  THROW_ERROR("Not supported interface: " + std::string(optarg));
               }
               break;
            }
#if HAVE_EXPERIMENTAL
            if(strcmp(long_options[option_index].name, "export-core") == 0)
            {
               setOption(OPT_export_core, true);
               if(std::string(optarg) == "PCORE")
               {
                  setOption(OPT_export_core_mode, HLSFlowStep_Type::EXPORT_PCORE);
               }
               else
               {
                  THROW_ERROR("Not supported export mode: " + std::string(optarg));
               }
               break;
            }
#endif
            if(strcmp(long_options[option_index].name, "data-bus-bitsize") == 0)
            {
               setOption(OPT_data_bus_bitsize, boost::lexical_cast<int>(optarg));
               break;
            }
            if(strcmp(long_options[option_index].name, "addr-bus-bitsize") == 0)
            {
               setOption(OPT_addr_bus_bitsize, boost::lexical_cast<int>(optarg));
               break;
            }
#if HAVE_EXPERIMENTAL
            if(strcmp(long_options[option_index].name, "edk-config") == 0)
            {
               setOption("edk_wrapper", true);
               setOption("edk_config_file", std::string(optarg));
               break;
            }
#endif
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
            bool res = ManageGccOptions(next_option, optarg);
            if(res)
               res = ManageDefaultOptions(next_option, optarg, exit_success);
            if(exit_success)
               return EXIT_SUCCESS;
            if(res)
            {
               return PARAMETER_NOTPARSED;
            }
         }
      }
   }

#if HAVE_EXPERIMENTAL
   if(isOption(OPT_gcc_write_xml))
   {
      const std::string filenameXML = getOption<std::string>(OPT_gcc_write_xml);
      write_xml_configuration_file(filenameXML);
      PRINT_MSG("Configuration saved into file \"" + filenameXML + "\"");
      return EXIT_SUCCESS;
   }
#endif

   std::string cat_args;

   for(int i = 0; i < argc; i++)
   {
      cat_args += std::string(argv[i]) + " ";
   }
   setOption(OPT_cat_args, cat_args);

   INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, getOption<int>(OPT_output_level), " ==  Bambu executed with: " + cat_args + "\n");

   while(optind < argc)
   {
      const auto file_type = GetFileFormat(argv[optind], true);
      if(file_type == Parameters_FileFormat::FF_XML_CON)
      {
         setOption(OPT_constraints_file, argv[optind]);
      }
      else if(file_type == Parameters_FileFormat::FF_XML_TEC)
      {
         const auto tech_files = isOption(OPT_technology_file) ? getOption<std::string>(OPT_technology_file) + STR_CST_string_separator : "";
         setOption(OPT_technology_file, tech_files + argv[optind]);
      }
#if HAVE_FROM_AADL_ASN_BUILT
      else if(file_type == Parameters_FileFormat::FF_AADL)
      {
         const auto input_file = isOption(OPT_input_file) ? getOption<std::string>(OPT_input_file) + STR_CST_string_separator : "";
         setOption(OPT_input_file, input_file + argv[optind]);
         setOption(OPT_input_format, static_cast<int>(Parameters_FileFormat::FF_AADL));
      }
#endif
      else if(file_type == Parameters_FileFormat::FF_C || file_type == Parameters_FileFormat::FF_OBJECTIVEC || file_type == Parameters_FileFormat::FF_CPP || file_type == Parameters_FileFormat::FF_FORTRAN
#if HAVE_I386_CLANG4_COMPILER || HAVE_I386_CLANG5_COMPILER || HAVE_I386_CLANG6_COMPILER || HAVE_I386_CLANG7_COMPILER
              || file_type == Parameters_FileFormat::FF_LLVM
#endif
      )
      {
         const auto input_file = isOption(OPT_input_file) ? getOption<std::string>(OPT_input_file) + STR_CST_string_separator : "";
         setOption(OPT_input_file, input_file + argv[optind]);
         setOption(OPT_input_format, static_cast<int>(file_type));
      }
      else if(file_type == Parameters_FileFormat::FF_RAW or (isOption(OPT_input_format) and getOption<Parameters_FileFormat>(OPT_input_format) == Parameters_FileFormat::FF_RAW))
      {
         const auto input_file = isOption(OPT_input_file) ? getOption<std::string>(OPT_input_file) + STR_CST_string_separator : "";
         setOption(OPT_input_file, input_file + argv[optind]);
         setOption(OPT_input_format, static_cast<int>(Parameters_FileFormat::FF_RAW));
         if(!isOption(OPT_pretty_print))
            setOption(OPT_pretty_print, "_a.c");
      }
      else
      {
         THROW_ERROR("Unexpected file extension: " + std::string(argv[optind]));
      }
      optind++;
   }

   CheckParameters();

   return PARAMETER_PARSED;
}

void BambuParameter::add_experimental_setup_gcc_options(bool kill_printf)
{
   if(kill_printf)
   {
      std::string defines;
      if(isOption(OPT_gcc_defines))
         defines = getOption<std::string>(OPT_gcc_defines) + STR_CST_string_separator;
      defines += "\'printf(fmt, ...)=\'";
      setOption(OPT_gcc_defines, defines);
   }
   if(isOption(OPT_top_functions_names) && getOption<std::string>(OPT_top_functions_names) == "main")
   {
      std::string optimizations;
      if(isOption(OPT_gcc_optimizations))
         optimizations = getOption<std::string>(OPT_gcc_optimizations);
      THROW_ASSERT(isOption(OPT_input_file), "Input file not specified");
      if(getOption<std::string>(OPT_input_file).find(STR_CST_string_separator) == std::string::npos && !isOption(OPT_top_design_name))
      {
         if(optimizations != "")
            optimizations = optimizations + STR_CST_string_separator;
         optimizations = optimizations + "whole-program";
         setOption(OPT_gcc_optimizations, optimizations);
      }
      if(isOption(OPT_top_design_name))
      {
         if(optimizations != "")
            optimizations = optimizations + STR_CST_string_separator;
         optimizations = optimizations + "no-ipa-cp" + STR_CST_string_separator + "no-ipa-cp-clone";
         setOption(OPT_gcc_optimizations, optimizations);
      }
   }
}

void BambuParameter::CheckParameters()
{
   Parameter::CheckParameters();
#if HAVE_TASTE
   if(isOption(OPT_input_format) and getOption<Parameters_FileFormat>(OPT_input_format) == Parameters_FileFormat::FF_AADL)
   {
      setOption(OPT_generate_taste_architecture, true);
      setOption(OPT_clock_period, 20);
      setOption(OPT_writer_language, static_cast<int>(HDLWriter_Language::VHDL));
      setOption(OPT_interface_type, HLSFlowStep_Type::TASTE_INTERFACE_GENERATION);
      setOption(OPT_channels_type, MemoryAllocation_ChannelsType::MEM_ACC_NN);
      setOption(OPT_channels_number, 2);
      setOption("device_name", "xc4vlx100");
      setOption("device_speed", "-10");
      setOption("device_package", "ff1513");
      setOption("device_synthesis_tool", "");
      if(getOption<std::string>(OPT_bram_high_latency) != "")
      {
         THROW_ERROR("High latency bram cannot be used in taste architecture");
      }
   }
#endif

   /// target device options
   if(not isOption(OPT_device_string))
   {
      std::string device_string = getOption<std::string>("device_name") + getOption<std::string>("device_speed") + getOption<std::string>("device_package");
      if(isOption("device_synthesis_tool") && getOption<std::string>("device_synthesis_tool") != "")
         device_string += "-" + getOption<std::string>("device_synthesis_tool");
      setOption(OPT_device_string, device_string);
   }

   if(isOption(OPT_channels_type) and (getOption<MemoryAllocation_ChannelsType>(OPT_channels_type) == MemoryAllocation_ChannelsType::MEM_ACC_N1 or getOption<MemoryAllocation_ChannelsType>(OPT_channels_type) == MemoryAllocation_ChannelsType::MEM_ACC_NN) and
      not isOption(OPT_channels_number))
   {
      setOption(OPT_channels_number, 2);
   }

   /// DSE options
#if HAVE_EXPERIMENTAL
#if HAVE_BEAGLE
   if(getOption<int>("exploration_technique") == dse_hls::PRIORITY and getOption<int>("to_normalize"))
      setOption("to_normalize", false);
   if(isOption("dse_algorithm") and getOption<unsigned int>("dse_algorithm") == dse_hls::ACO)
   {
      if(!isOption("aco_ant_num"))
         setOption("aco_ant_num", 5);
      if(!isOption("aco_generations"))
         setOption("aco_generations", 10);
   }
#endif
#endif

   /// circuit debugging options
   if(isOption(OPT_generate_vcd) and getOption<bool>(OPT_generate_vcd))
   {
      setOption(OPT_assert_debug, true);
   }
   if(getOption<int>(OPT_debug_level) >= DEBUG_LEVEL_VERY_PEDANTIC)
   {
      setOption(OPT_assert_debug, true);
   }

   /// controller options
   if(getOption<HLSFlowStep_Type>(OPT_controller_architecture) == HLSFlowStep_Type::FSM_CONTROLLER_CREATOR || getOption<HLSFlowStep_Type>(OPT_controller_architecture) == HLSFlowStep_Type::FSM_CS_CONTROLLER_CREATOR)
      setOption(OPT_stg, true);

   /// chaining options
   setOption(OPT_chaining_algorithm, HLSFlowStep_Type::SCHED_CHAINING);
#if HAVE_EXPERIMENTAL
   if(getOption<HLSFlowStep_Type>(OPT_controller_architecture) == HLSFlowStep_Type::PARALLEL_CONTROLLER_CREATOR)
   {
      setOption(OPT_liveness_algorithm, HLSFlowStep_Type::CHAINING_BASED_LIVENESS);
      setOption(OPT_chaining_algorithm, HLSFlowStep_Type::EPDG_SCHED_CHAINING);
   }
#endif

   /// evaluation options
   if(getOption<bool>(OPT_evaluation))
   {
      THROW_ASSERT(isOption(OPT_evaluation_objectives), "missing evaluation objectives");
      std::string objective_string = getOption<std::string>(OPT_evaluation_objectives);
      THROW_ASSERT(not objective_string.empty(), "");
      std::vector<std::string> objective_vector = convert_string_to_vector<std::string>(objective_string, ",");

      if(getOption<Evaluation_Mode>(OPT_evaluation_mode) == Evaluation_Mode::EXACT)
      {
         if(is_evaluation_objective_string(objective_vector, "AREAxTIME"))
         {
            add_evaluation_objective_string(objective_string, "AREA,TIME");
            setOption(OPT_evaluation_objectives, objective_string);
            objective_vector = convert_string_to_vector<std::string>(objective_string, ",");
         }

         if(is_evaluation_objective_string(objective_vector, "TIME") or is_evaluation_objective_string(objective_vector, "TOTAL_TIME") or is_evaluation_objective_string(objective_vector, "CYCLES") or
            is_evaluation_objective_string(objective_vector, "TOTAL_CYCLES"))
         {
            if(not getOption<bool>(OPT_generate_testbench))
            {
               setOption(OPT_generate_testbench, true);
               setOption(OPT_testbench_input_xml, "test.xml");
            }
         }
         const auto is_valid_evaluation_mode = [](const std::string& s) -> bool {
            return s == "AREA" or s == "AREAxTIME" or s == "TIME" or s == "TOTAL_TIME" or s == "CYCLES" or s == "TOTAL_CYCLES" or s == "BRAMS" or s == "CLOCK_SLACK" or s == "DSPS" or
#if HAVE_EXPERIMENTAL
                   s == "EDGES_REDUCTION" or
#endif
                   s == "FREQUENCY" or
#if HAVE_EXPERIMENTAL
                   s == "NUM_AD_EDGES" or
#endif
                   s == "PERIOD" or s == "REGISTERS";
         };
         if(not all_of(objective_vector.begin(), objective_vector.end(), is_valid_evaluation_mode))
         {
            THROW_ERROR("BadParameters: evaluation mode EXACT don't support the evaluation objectives");
         }
      }
#if HAVE_EXPERIMENTAL
      else if(getOption<Evaluation_Mode>(OPT_evaluation_mode) == Evaluation_Mode::ESTIMATION)
      {
         const auto is_valid_evaluation_mode = [](const std::string& s) -> bool { return s == "AREA" or s == "TIME" or s == "CLOCK_SLACK"; };
         if(not all_of(objective_vector.begin(), objective_vector.end(), is_valid_evaluation_mode))
         {
            THROW_ERROR("BadParameters: evaluation mode LINEAR don't support the evaluation objectives");
         }
      }
#endif
      else
      {
         THROW_ERROR("BadParameters: invalid evaluation mode");
      }
   }
#if HAVE_EXPERIMENTAL
   /// Export and interface generation
   if(getOption<bool>(OPT_export_core) && getOption<HLSFlowStep_Type>(OPT_export_core_mode) == HLSFlowStep_Type::EXPORT_PCORE)
   {
      setOption(OPT_interface, true);
      setOption(OPT_interface_type, HLSFlowStep_Type::AXI4LITE_INTERFACE_GENERATION);
   }
#endif

   if(isOption(OPT_interface_type) && getOption<HLSFlowStep_Type>(OPT_interface_type) == HLSFlowStep_Type::INFERRED_INTERFACE_GENERATION)
   {
      setOption(OPT_do_not_expose_globals, true);
   }

   if(getOption<bool>(OPT_do_not_expose_globals))
   {
      if(getOption<MemoryAllocation_Policy>(OPT_memory_allocation_policy) == MemoryAllocation_Policy::NONE && (!isOption(OPT_interface_type) || getOption<HLSFlowStep_Type>(OPT_interface_type) != HLSFlowStep_Type::WB4_INTERFACE_GENERATION))
         setOption(OPT_memory_allocation_policy, MemoryAllocation_Policy::ALL_BRAM);
   }
   tree_helper::debug_level = get_class_debug_level("tree_helper");

   bool flag_cpp;
   if(isOption(OPT_input_format) && getOption<Parameters_FileFormat>(OPT_input_format) == Parameters_FileFormat::FF_CPP)
      flag_cpp = true;
   else
      flag_cpp = false;

   if(flag_cpp)
   {
      /// add -I <ac_types_dir> and -I <ac_math_dir>
      std::string includes = "-I " + std::string(PANDA_DATA_INSTALLDIR "/panda/ac_types/include") + " -I " + std::string(PANDA_DATA_INSTALLDIR "/panda/ac_math/include");
      if(isOption(OPT_gcc_includes))
         includes = getOption<std::string>(OPT_gcc_includes) + " " + includes;
      setOption(OPT_gcc_includes, includes);
      if(!isOption(OPT_gcc_standard))
      {
         setOption(OPT_gcc_standard, "c++14");
      }
   }
   /// add experimental setup options
   if(getOption<std::string>(OPT_experimental_setup) == "VVD")
   {
      if(not isOption(OPT_gcc_opt_level))
         setOption(OPT_gcc_opt_level, GccWrapper_OptimizationSet::O3);
      if(not isOption(OPT_channels_type))
         setOption(OPT_channels_type, MemoryAllocation_ChannelsType::MEM_ACC_NN);
      if(not isOption(OPT_channels_number))
         setOption(OPT_channels_number, 2);
      if(getOption<MemoryAllocation_Policy>(OPT_memory_allocation_policy) == MemoryAllocation_Policy::NONE)
         setOption(OPT_memory_allocation_policy, MemoryAllocation_Policy::ALL_BRAM);
      setOption(OPT_DSP_allocation_coefficient, 1.75);
      setOption(OPT_clock_period_resource_fraction, "0.875");
      setOption(OPT_do_not_expose_globals, true);
      if(not isOption(OPT_distram_threshold))
         setOption(OPT_distram_threshold, 256);
      add_experimental_setup_gcc_options(!flag_cpp);
   }
   else if(getOption<std::string>(OPT_experimental_setup) == "BAMBU092")
   {
      if(not isOption(OPT_gcc_opt_level))
         setOption(OPT_gcc_opt_level, GccWrapper_OptimizationSet::O0);
      setOption(OPT_estimate_logic_and_connections, false);
      setOption(OPT_clock_period_resource_fraction, "0.9");
      setOption(OPT_DSP_margin_combinational, 1.3);
      setOption(OPT_skip_pipe_parameter, 1);
      if(not isOption(OPT_distram_threshold))
         setOption(OPT_distram_threshold, 256);
      add_experimental_setup_gcc_options(!flag_cpp);
   }
   else if(getOption<std::string>(OPT_experimental_setup) == "BAMBU-BALANCED" or getOption<std::string>(OPT_experimental_setup) == "BAMBU-BALANCED-MP" or getOption<std::string>(OPT_experimental_setup) == "BAMBU-TASTE")
   {
      std::string tuning_optimizations;
      if(not isOption(OPT_gcc_opt_level))
      {
         setOption(OPT_gcc_opt_level, GccWrapper_OptimizationSet::O2);
         /// GCC SECTION
         if(false
#if HAVE_I386_GCC45_COMPILER
            or getOption<GccWrapper_CompilerTarget>(OPT_default_compiler) == GccWrapper_CompilerTarget::CT_I386_GCC45
#endif
#if HAVE_I386_GCC46_COMPILER
            or getOption<GccWrapper_CompilerTarget>(OPT_default_compiler) == GccWrapper_CompilerTarget::CT_I386_GCC46
#endif
#if HAVE_I386_GCC47_COMPILER
            or getOption<GccWrapper_CompilerTarget>(OPT_default_compiler) == GccWrapper_CompilerTarget::CT_I386_GCC47
#endif
#if HAVE_I386_GCC48_COMPILER
            or getOption<GccWrapper_CompilerTarget>(OPT_default_compiler) == GccWrapper_CompilerTarget::CT_I386_GCC48
#endif
#if HAVE_I386_GCC49_COMPILER
            or getOption<GccWrapper_CompilerTarget>(OPT_default_compiler) == GccWrapper_CompilerTarget::CT_I386_GCC49
#endif
#if HAVE_I386_GCC5_COMPILER
            or getOption<GccWrapper_CompilerTarget>(OPT_default_compiler) == GccWrapper_CompilerTarget::CT_I386_GCC5
#endif
#if HAVE_I386_GCC6_COMPILER
            or getOption<GccWrapper_CompilerTarget>(OPT_default_compiler) == GccWrapper_CompilerTarget::CT_I386_GCC6
#endif
#if HAVE_I386_GCC7_COMPILER
            or getOption<GccWrapper_CompilerTarget>(OPT_default_compiler) == GccWrapper_CompilerTarget::CT_I386_GCC7
#endif
#if HAVE_I386_GCC8_COMPILER
            or getOption<GccWrapper_CompilerTarget>(OPT_default_compiler) == GccWrapper_CompilerTarget::CT_I386_GCC8
#endif
         )
         {
            tuning_optimizations += "inline-functions" + STR_CST_string_separator + "gcse-after-reload" + STR_CST_string_separator + "ipa-cp-clone" + STR_CST_string_separator + "unswitch-loops" + STR_CST_string_separator + "no-tree-loop-ivcanon";
            if(false
#if HAVE_I386_GCC48_COMPILER
               or getOption<GccWrapper_CompilerTarget>(OPT_default_compiler) == GccWrapper_CompilerTarget::CT_I386_GCC48
#endif
#if HAVE_I386_GCC49_COMPILER
               or getOption<GccWrapper_CompilerTarget>(OPT_default_compiler) == GccWrapper_CompilerTarget::CT_I386_GCC49
#endif
#if HAVE_I386_GCC5_COMPILER
               or getOption<GccWrapper_CompilerTarget>(OPT_default_compiler) == GccWrapper_CompilerTarget::CT_I386_GCC5
#endif
#if HAVE_I386_GCC6_COMPILER
               or getOption<GccWrapper_CompilerTarget>(OPT_default_compiler) == GccWrapper_CompilerTarget::CT_I386_GCC6
#endif
#if HAVE_I386_GCC7_COMPILER
               or getOption<GccWrapper_CompilerTarget>(OPT_default_compiler) == GccWrapper_CompilerTarget::CT_I386_GCC7
#endif
#if HAVE_I386_GCC8_COMPILER
               or getOption<GccWrapper_CompilerTarget>(OPT_default_compiler) == GccWrapper_CompilerTarget::CT_I386_GCC8
#endif
            )
            {
               tuning_optimizations += STR_CST_string_separator + "tree-partial-pre" + STR_CST_string_separator + "disable-tree-bswap";
            }
            if(false
#if HAVE_I386_GCC7_COMPILER
               or getOption<GccWrapper_CompilerTarget>(OPT_default_compiler) == GccWrapper_CompilerTarget::CT_I386_GCC7
#endif
#if HAVE_I386_GCC8_COMPILER
               or getOption<GccWrapper_CompilerTarget>(OPT_default_compiler) == GccWrapper_CompilerTarget::CT_I386_GCC8
#endif
            )
            {
               tuning_optimizations += STR_CST_string_separator + "no-store-merging";
            }
         }
         /// CLANG SECTION
         else if(false
#if HAVE_I386_CLANG4_COMPILER
                 or getOption<GccWrapper_CompilerTarget>(OPT_default_compiler) == GccWrapper_CompilerTarget::CT_I386_CLANG4
#endif
#if HAVE_I386_CLANG5_COMPILER
                 or getOption<GccWrapper_CompilerTarget>(OPT_default_compiler) == GccWrapper_CompilerTarget::CT_I386_CLANG5
#endif
#if HAVE_I386_CLANG6_COMPILER
                 or getOption<GccWrapper_CompilerTarget>(OPT_default_compiler) == GccWrapper_CompilerTarget::CT_I386_CLANG6
#endif
#if HAVE_I386_CLANG7_COMPILER
                 or getOption<GccWrapper_CompilerTarget>(OPT_default_compiler) == GccWrapper_CompilerTarget::CT_I386_CLANG7
#endif
         )
         {
            tuning_optimizations += "inline-functions";
         }
      }
      std::string optimizations;
      if(isOption(OPT_gcc_optimizations))
      {
         optimizations += getOption<std::string>(OPT_gcc_optimizations);
      }
      if(optimizations != "" and tuning_optimizations != "")
      {
         optimizations += STR_CST_string_separator;
      }
      optimizations += tuning_optimizations;
      if(optimizations != "")
         setOption(OPT_gcc_optimizations, optimizations);
#if 0
      std::string parameters;
      if(isOption(OPT_gcc_parameters))
         parameters = getOption<std::string>(OPT_gcc_parameters) + STR_CST_string_separator;
      setOption(OPT_gcc_parameters, parameters + "max-inline-insns-auto=25");
#endif
      if(getOption<std::string>(OPT_experimental_setup) == "BAMBU-BALANCED-MP")
      {
         if(not isOption(OPT_channels_type))
            setOption(OPT_channels_type, MemoryAllocation_ChannelsType::MEM_ACC_NN);
         if(not isOption(OPT_channels_number))
            setOption(OPT_channels_number, 2);
      }
      if(getOption<MemoryAllocation_Policy>(OPT_memory_allocation_policy) == MemoryAllocation_Policy::NONE)
         setOption(OPT_memory_allocation_policy, MemoryAllocation_Policy::ALL_BRAM);
      if(not isOption(OPT_distram_threshold))
         setOption(OPT_distram_threshold, 256);
      add_experimental_setup_gcc_options(!flag_cpp);
      if(getOption<std::string>(OPT_experimental_setup) == "BAMBU-TASTE")
      {
         const auto source_files = getOption<const CustomSet<std::string>>(OPT_input_file);
         if(source_files.size() > 1 && isOption(OPT_input_format) && getOption<Parameters_FileFormat>(OPT_input_format) == Parameters_FileFormat::FF_C)
         {
            auto concat_filename = boost::filesystem::path(getOption<std::string>(OPT_output_temporary_directory) + "/" + boost::filesystem::unique_path(std::string(STR_CST_concat_c_file)).string()).string();
            std::ofstream filestream(concat_filename.c_str());
            for(const auto& source_file : source_files)
            {
               filestream << "#include \"../" << source_file << "\"\n";
            }
            filestream.close();
            setOption(OPT_input_file, concat_filename);
         }
      }
   }
   else if(getOption<std::string>(OPT_experimental_setup) == "BAMBU-PERFORMANCE-MP")
   {
      if(not isOption(OPT_gcc_opt_level))
         setOption(OPT_gcc_opt_level, GccWrapper_OptimizationSet::O3);
      if(not isOption(OPT_channels_type))
         setOption(OPT_channels_type, MemoryAllocation_ChannelsType::MEM_ACC_NN);
      if(not isOption(OPT_channels_number))
         setOption(OPT_channels_number, 2);
      if(getOption<MemoryAllocation_Policy>(OPT_memory_allocation_policy) == MemoryAllocation_Policy::NONE)
         setOption(OPT_memory_allocation_policy, MemoryAllocation_Policy::ALL_BRAM);
      if(not isOption(OPT_distram_threshold))
         setOption(OPT_distram_threshold, 512);
      add_experimental_setup_gcc_options(!flag_cpp);
   }
   else if(getOption<std::string>(OPT_experimental_setup) == "BAMBU-PERFORMANCE")
   {
      if(not isOption(OPT_gcc_opt_level))
         setOption(OPT_gcc_opt_level, GccWrapper_OptimizationSet::O3);
      if(getOption<MemoryAllocation_Policy>(OPT_memory_allocation_policy) == MemoryAllocation_Policy::NONE)
         setOption(OPT_memory_allocation_policy, MemoryAllocation_Policy::ALL_BRAM);
      if(not isOption(OPT_distram_threshold))
         setOption(OPT_distram_threshold, 512);
      add_experimental_setup_gcc_options(!flag_cpp);
   }
   else if(getOption<std::string>(OPT_experimental_setup) == "BAMBU-AREA-MP")
   {
      if(not isOption(OPT_gcc_opt_level))
         setOption(OPT_gcc_opt_level, GccWrapper_OptimizationSet::Os);
      if(not isOption(OPT_channels_type))
         setOption(OPT_channels_type, MemoryAllocation_ChannelsType::MEM_ACC_NN);
      if(not isOption(OPT_channels_number))
         setOption(OPT_channels_number, 2);
      if(getOption<MemoryAllocation_Policy>(OPT_memory_allocation_policy) == MemoryAllocation_Policy::NONE)
         setOption(OPT_memory_allocation_policy, MemoryAllocation_Policy::ALL_BRAM);
      setOption(OPT_DSP_allocation_coefficient, 1.75);
      if(not isOption(OPT_distram_threshold))
         setOption(OPT_distram_threshold, 256);
      add_experimental_setup_gcc_options(!flag_cpp);
   }
   else if(getOption<std::string>(OPT_experimental_setup) == "BAMBU-AREA")
   {
      if(not isOption(OPT_gcc_opt_level))
         setOption(OPT_gcc_opt_level, GccWrapper_OptimizationSet::Os);
      if(getOption<MemoryAllocation_Policy>(OPT_memory_allocation_policy) == MemoryAllocation_Policy::NONE)
         setOption(OPT_memory_allocation_policy, MemoryAllocation_Policy::ALL_BRAM);
      setOption(OPT_DSP_allocation_coefficient, 1.75);
      if(not isOption(OPT_distram_threshold))
         setOption(OPT_distram_threshold, 256);
      add_experimental_setup_gcc_options(!flag_cpp);
   }
   else if(getOption<std::string>(OPT_experimental_setup) == "BAMBU")
   {
      if(not isOption(OPT_gcc_opt_level))
         setOption(OPT_gcc_opt_level, GccWrapper_OptimizationSet::O0);
      if(not isOption(OPT_distram_threshold))
         setOption(OPT_distram_threshold, 256);
      add_experimental_setup_gcc_options(false);
   }
   else
   {
      THROW_ERROR("Experimental setup not recognized: " + getOption<std::string>(OPT_experimental_setup));
   }

   add_bambu_library("bambu");

   if(isOption(OPT_soft_float) && getOption<bool>(OPT_soft_float))
   {
      if(isOption(OPT_soft_fp) && getOption<bool>(OPT_soft_fp))
         add_bambu_library("soft-fp");
      else if(getOption<std::string>(OPT_hls_fpdiv) != "SRT4" && getOption<std::string>(OPT_hls_fpdiv) != "G")
         THROW_ERROR("--hls-fpdiv=SF requires --soft-fp option");
      else if(isOption(OPT_softfloat_subnormal) && getOption<bool>(OPT_softfloat_subnormal))
         add_bambu_library("softfloat_subnormals");
      else
         add_bambu_library("softfloat");
   }

   if(isOption(OPT_hls_div) && getOption<std::string>(OPT_hls_div) != "none")
      add_bambu_library("hls-div" + getOption<std::string>(OPT_hls_div));
   add_bambu_library("hls-cdiv");
#if HAVE_FROM_PRAGMA_BUILT && HAVE_BAMBU_BUILT
   if(getOption<bool>(OPT_parse_pragma))
   {
      setOption(OPT_disable_function_proxy, true);
      if(isOption(OPT_context_switch))
      {
         if(getOption<unsigned int>(OPT_channels_number) >= getOption<unsigned int>(OPT_memory_banks_number))
            THROW_ERROR("This configuration doesn't support a number of channel equal or greater than the number of memory_bank");
         if(getOption<std::string>(OPT_registered_inputs) != "auto")
            THROW_ERROR("Registered inputs option cannot be set for context switch architecture");
         unsigned int v = getOption<unsigned int>(OPT_channels_number); // we want to see if v is a power of 2
         bool f;                                                        // the result goes here
         f = v && !(v & (v - 1));
         if(!f)
            THROW_ERROR("Number of channel must be a power of 2");
         v = getOption<unsigned int>(OPT_memory_banks_number); // we want to see if v is a power of 2
         f = v && !(v & (v - 1));
         if(!f)
            THROW_ERROR("Number of bank must be a power of 2");
         setOption(OPT_function_allocation_algorithm, HLSFlowStep_Type::OMP_FUNCTION_ALLOCATION_CS);
         setOption(OPT_memory_allocation_algorithm, HLSFlowStep_Type::DOMINATOR_MEMORY_ALLOCATION_CS);
         setOption(OPT_channels_type, MemoryAllocation_ChannelsType::MEM_ACC_CS);
         setOption(OPT_datapath_architecture, HLSFlowStep_Type::DATAPATH_CS_CREATOR);
         setOption(OPT_controller_architecture, HLSFlowStep_Type::FSM_CS_CONTROLLER_CREATOR);
         setOption(OPT_interface_type, HLSFlowStep_Type::INTERFACE_CS_GENERATION);
      }
      else
      {
         setOption(OPT_function_allocation_algorithm, HLSFlowStep_Type::OMP_FUNCTION_ALLOCATION);
      }
      add_bambu_library("pthread");
   }
#endif

   if(isOption(OPT_gcc_libraries))
   {
      const auto libraries = getOption<const CustomSet<std::string>>(OPT_gcc_libraries);
      for(const auto& library : libraries)
      {
         add_bambu_library(library);
      }
   }

   /// default for memory allocation policy
   if(getOption<MemoryAllocation_Policy>(OPT_memory_allocation_policy) == MemoryAllocation_Policy::NONE)
      setOption(OPT_memory_allocation_policy, MemoryAllocation_Policy::LSS);

   /// base address and initial internal address checks
   if(isOption(OPT_initial_internal_address) && (getOption<unsigned int>(OPT_base_address) == 0 || getOption<unsigned int>(OPT_initial_internal_address) == 0))
   {
      std::string optimizations;
      if(isOption(OPT_gcc_optimizations))
         optimizations = getOption<std::string>(OPT_gcc_optimizations) + STR_CST_string_separator;
      setOption(OPT_gcc_optimizations, optimizations + "no-delete-null-pointer-checks");
   }

   /// Checks
   if(getOption<HLSFlowStep_Type>(OPT_memory_allocation_algorithm) == HLSFlowStep_Type::XML_MEMORY_ALLOCATOR and not!isOption(OPT_xml_memory_allocation))
   {
      if(!isOption(OPT_xml_input_configuration))
         THROW_ERROR("XML specification of the memory allocation has not been specified!");
   }
   if(isOption(OPT_memory_banks_number) && getOption<int>(OPT_memory_banks_number) > 1)
   {
      setOption(OPT_channels_type, MemoryAllocation_ChannelsType::MEM_ACC_CS);
   }
   if(not isOption(OPT_channels_type))
   {
      setOption(OPT_channels_type, MemoryAllocation_ChannelsType::MEM_ACC_11);
      setOption(OPT_channels_number, 1);
   }
   if(isOption(OPT_channels_number) and getOption<MemoryAllocation_ChannelsType>(OPT_channels_type) == MemoryAllocation_ChannelsType::MEM_ACC_11)
   {
      if(getOption<unsigned int>(OPT_channels_number) != 1)
         THROW_ERROR("the number of channels cannot be specified for MEM_ACC_11");
   }
   if(isOption(OPT_channels_number) and (getOption<MemoryAllocation_ChannelsType>(OPT_channels_type) == MemoryAllocation_ChannelsType::MEM_ACC_N1 or getOption<MemoryAllocation_ChannelsType>(OPT_channels_type) == MemoryAllocation_ChannelsType::MEM_ACC_NN))
   {
      if(getOption<unsigned int>(OPT_channels_number) > 2 and getOption<MemoryAllocation_Policy>(OPT_memory_allocation_policy) != MemoryAllocation_Policy::NO_BRAM and
         getOption<MemoryAllocation_Policy>(OPT_memory_allocation_policy) != MemoryAllocation_Policy::EXT_PIPELINED_BRAM)
         THROW_ERROR("no more than two channels is supported for MEM_ACC_N1 and MEM_ACC_NN: try to add this option --memory-allocation-policy=NO_BRAM or --memory-allocation-policy=EXT_PIPELINED_BRAM");
   }
   if(getOption<MemoryAllocation_ChannelsType>(OPT_channels_type) == MemoryAllocation_ChannelsType::MEM_ACC_NN and isOption(OPT_interface_type) and getOption<HLSFlowStep_Type>(OPT_interface_type) == HLSFlowStep_Type::WB4_INTERFACE_GENERATION)
      THROW_ERROR("Wishbone 4 interface does not yet support multi-channel architectures (MEM_ACC_NN)");

   if(getOption<MemoryAllocation_Policy>(OPT_memory_allocation_policy) == MemoryAllocation_Policy::ALL_BRAM and isOption(OPT_interface_type) and getOption<HLSFlowStep_Type>(OPT_interface_type) == HLSFlowStep_Type::WB4_INTERFACE_GENERATION)
      THROW_ERROR("Wishbone 4 interface does not yet support --memory-allocation-policy=ALL_BRAM");

   if(getOption<MemoryAllocation_Policy>(OPT_memory_allocation_policy) == MemoryAllocation_Policy::EXT_PIPELINED_BRAM and isOption(OPT_interface_type) and getOption<HLSFlowStep_Type>(OPT_interface_type) == HLSFlowStep_Type::WB4_INTERFACE_GENERATION)
      THROW_ERROR("Wishbone 4 interface does not yet support --memory-allocation-policy=EXT_PIPELINED_BRAM");

   if(isOption(OPT_interface_type) and getOption<HLSFlowStep_Type>(OPT_interface_type) == HLSFlowStep_Type::WB4_INTERFACE_GENERATION and (isOption(OPT_clock_name) or isOption(OPT_reset_name) or isOption(OPT_start_name) or isOption(OPT_done_name)))
      THROW_ERROR("Wishbone 4 interface does not allow the renaming of the control signals");

   if(getOption<MemoryAllocation_Policy>(OPT_memory_allocation_policy) == MemoryAllocation_Policy::INTERN_UNALIGNED && !getOption<bool>(OPT_do_not_expose_globals))
      THROW_ERROR("--memory-allocation-policy=INTERN_UNALIGNED implies --do-not-expose-globals");
   if(not getOption<bool>(OPT_gcc_include_sysdir))
   {
      if(not isOption(OPT_input_file))
      {
         THROW_ERROR("No input file specified");
      }
      if(isOption(OPT_gcc_optimizations) and getOption<std::string>(OPT_input_file).find(STR_CST_string_separator) != std::string::npos and getOption<std::string>(OPT_gcc_optimizations).find("whole-program") != std::string::npos and
         getOption<std::string>(OPT_gcc_optimizations).find("no-whole-program") == std::string::npos)
      {
         THROW_ERROR("-fwhole-program cannot be used with multiple input files");
      }
   }
#if HAVE_EXPERIMENTAL
   if(isOption(OPT_interface_type) && getOption<HLSFlowStep_Type>(OPT_interface_type) == HLSFlowStep_Type::AXI4LITE_INTERFACE_GENERATION)
   {
      if(isOption(OPT_evaluation_objectives) && getOption<std::string>(OPT_evaluation_objectives).find("AREA") != std::string::npos)
         THROW_ERROR("AXI-based synthesis has to be performed outside of bambu");
      else
         THROW_WARNING("Note that synthesis and simulation scripts generated by bambu for AXI-based designs are actually only templates not real synthesis scripts\n");
   }
#endif
   if(isOption(OPT_discrepancy) and getOption<bool>(OPT_discrepancy) and isOption(OPT_discrepancy_hw) and getOption<bool>(OPT_discrepancy_hw))
   {
      THROW_ERROR("--discrepancy and --discrepancy-hw are mutually exclusive");
   }
   if(isOption(OPT_discrepancy_hw) and getOption<bool>(OPT_discrepancy_hw) and getOption<HLSFlowStep_Type>(OPT_controller_architecture) != HLSFlowStep_Type::FSM_CONTROLLER_CREATOR)
   {
      THROW_ERROR("--discrepancy-hw is only compatible with classic FSM controllers");
   }
   if(isOption(OPT_discrepancy_hw) and getOption<bool>(OPT_discrepancy_hw) and static_cast<HDLWriter_Language>(getOption<unsigned int>(OPT_writer_language)) != HDLWriter_Language::VERILOG)
   {
      THROW_ERROR("--discrepancy-hw is only compatible with Verilog");
   }
   if(isOption(OPT_discrepancy_hw) and getOption<bool>(OPT_discrepancy_hw) and getOption<HLSFlowStep_Type>(OPT_function_allocation_algorithm) != HLSFlowStep_Type::DOMINATOR_FUNCTION_ALLOCATION)
   {
      THROW_ERROR("--discrepancy-hw Hardware Discrepancy Analysis only works with dominator function allocation");
   }
   if(isOption(OPT_discrepancy_hw) and getOption<bool>(OPT_discrepancy_hw) and isOption(OPT_disable_function_proxy) and getOption<bool>(OPT_disable_function_proxy))
   {
      THROW_ERROR("--discrepancy-hw Hardware Discrepancy Analysis only works with function proxies");
   }
   if(isOption(OPT_discrepancy) and getOption<bool>(OPT_discrepancy))
   {
      if(false
#if HAVE_I386_GCC45_COMPILER
         or getOption<GccWrapper_CompilerTarget>(OPT_default_compiler) == GccWrapper_CompilerTarget::CT_I386_GCC45
#endif
#if HAVE_I386_GCC46_COMPILER
         or getOption<GccWrapper_CompilerTarget>(OPT_default_compiler) == GccWrapper_CompilerTarget::CT_I386_GCC46
#endif
#if HAVE_I386_GCC47_COMPILER
         or getOption<GccWrapper_CompilerTarget>(OPT_default_compiler) == GccWrapper_CompilerTarget::CT_I386_GCC47
#endif
#if HAVE_I386_GCC48_COMPILER
         or getOption<GccWrapper_CompilerTarget>(OPT_default_compiler) == GccWrapper_CompilerTarget::CT_I386_GCC48
#endif
#if HAVE_I386_GCC49_COMPILER
         or getOption<GccWrapper_CompilerTarget>(OPT_default_compiler) == GccWrapper_CompilerTarget::CT_I386_GCC49
#endif
      )
      {
         THROW_WARNING("discrepancy analysis can report false positives with old compilers, use --compiler=I386_GCC5 or higher to avoid them");
      }
   }
   if((isOption(OPT_generate_vcd) and getOption<bool>(OPT_generate_vcd)) or (isOption(OPT_discrepancy) and getOption<bool>(OPT_discrepancy)))
   {
      if(not isOption(OPT_generate_testbench) or not getOption<bool>(OPT_generate_testbench))
         THROW_ERROR("Testbench generation required. (--generate-tb or --simulate undeclared).");
   }
   if((getOption<Evaluation_Mode>(OPT_evaluation_mode) == Evaluation_Mode::EXACT and getOption<std::string>(OPT_evaluation_objectives).find("CYCLES") != std::string::npos) or (isOption(OPT_discrepancy) and getOption<bool>(OPT_discrepancy)))
   {
      if(isOption(OPT_top_functions_names) and getOption<const std::list<std::string>>(OPT_top_functions_names).size() > 1)
      {
         THROW_ERROR("Simulation cannot be enabled with multiple top functions");
      }
   }
   /// Disable proxy when there are multiple top functions
   if(isOption(OPT_top_functions_names) and getOption<const std::list<std::string>>(OPT_top_functions_names).size() > 1)
   {
      setOption(OPT_disable_function_proxy, true);
   }
   /// In case copy input files
   if(isOption(OPT_file_input_data))
   {
      std::string input_data = getOption<std::string>(OPT_file_input_data);
      std::vector<std::string> splitted = SplitString(input_data, ",");
      size_t i_end = splitted.size();
      for(size_t i = 0; i < i_end; i++)
      {
         std::string command = "cp " + splitted[i] + " .";
         int ret = PandaSystem(ParameterConstRef(this, null_deleter()), command);
         if(IsError(ret))
         {
            THROW_ERROR("cp returns an error");
         }
      }
   }

   if(isOption(OPT_no_parse_c_python) and not isOption(OPT_testbench_extra_gcc_flags))
   {
      THROW_ERROR("Include directories and library directories for Python bindings are missing.\n"
                  "use --testbench-extra-gcc-flags=\"string\" to provide them");
   }
   setOption<unsigned int>(OPT_host_compiler, static_cast<unsigned int>(getOption<GccWrapper_CompilerTarget>(OPT_default_compiler)));
   if(isOption(OPT_evaluation_objectives) and getOption<std::string>(OPT_evaluation_objectives).find("CYCLES") != std::string::npos and isOption(OPT_device_string) and getOption<std::string>(OPT_device_string) == "LFE335EA8FN484C")
   {
      if(getOption<std::string>(OPT_simulator) == "VERILATOR")
      {
         THROW_ERROR("Simulation of Lattice device does not work with VERILATOR");
      }
   }
#if !HAVE_LATTICE
   if(isOption(OPT_evaluation_objectives) and getOption<std::string>(OPT_evaluation_objectives).find("CYCLES") != std::string::npos and isOption(OPT_device_string) and getOption<std::string>(OPT_device_string) == "LFE335EA8FN484C")
   {
      THROW_ERROR("Simulation of Lattice devices requires to enable Lattice support");
   }
#endif
#if HAVE_LATTICE
   if(isOption(OPT_evaluation_objectives) and getOption<std::string>(OPT_evaluation_objectives).find("ARE") != std::string::npos and isOption(OPT_device_string) and getOption<std::string>(OPT_device_string) == "LFE335EA8FN484C" and
      !getOption<bool>(OPT_connect_iob))
   {
      THROW_WARNING("--no-iob cannot be used when target is a Lattice board");
   }
#endif
   if(isOption(OPT_evaluation_objectives) and getOption<std::string>(OPT_evaluation_objectives).find("CYCLES") != std::string::npos and (not isOption(OPT_simulator) or getOption<std::string>(OPT_simulator) == ""))
   {
      THROW_ERROR("At least a simulator must be enabled");
   }
   if(isOption(OPT_dry_run_evaluation) and getOption<bool>(OPT_dry_run_evaluation))
   {
      setOption(OPT_evaluation_mode, Evaluation_Mode::DRY_RUN);
   }
   /// When simd is enabled bit value analysis and optimization are disabled
   if(getOption<int>(OPT_gcc_openmp_simd))
      setOption(OPT_bitvalue_ipa, false);
}

void BambuParameter::SetDefaults()
{
   // ---------- general options ----------- //
   /// Revision
#ifdef _WIN32
   setOption(OPT_dot_directory, GetCurrentPath() + "/HLS_output/dot/");
   setOption(OPT_output_directory, GetCurrentPath() + "/HLS_output/");
#else
   setOption(OPT_dot_directory, "HLS_output/dot/");
   setOption(OPT_output_directory, "HLS_output/");
#endif
   setOption(OPT_simulation_output, "results.txt");
   setOption(OPT_profiling_output, "profiling_results.txt");
   /// Debugging level
   setOption(OPT_output_level, OUTPUT_LEVEL_MINIMUM);
   setOption(OPT_debug_level, DEBUG_LEVEL_NONE);
   setOption(OPT_circuit_debug_level, DEBUG_LEVEL_NONE);
   setOption(OPT_print_dot, false);
   /// maximum execution time: 0 means no time limit
   setOption(OPT_ilp_max_time, 0);

#if HAVE_MAPPING_BUILT
   setOption(OPT_driving_component_type, "ARM");
#endif

   /// pragmas related
   setOption(OPT_parse_pragma, false);
   setOption(OPT_ignore_parallelism, false);

   /// ---------- frontend analysis ----------//
#if HAVE_FROM_RTL_BUILT
   setOption(OPT_use_rtl, false);
#endif

   // setOption("pdg-reduction", false);

#if HAVE_EXPERIMENTAL
   setOption("compute_unrolling_degree", false);
   setOption("insert_verification_operation", false);
#endif
   setOption(OPT_frontend_statistics, false);

   /// ---------- HLS process options ----------- //
   setOption(OPT_synthesis_flow, HLSFlowStep_Type::CLASSICAL_HLS_SYNTHESIS_FLOW);
   setOption(OPT_hls_flow, HLSFlowStep_Type::STANDARD_HLS_FLOW);

   /// ---------- HLS specification reference ----------- //
   setOption(OPT_generate_testbench, false);
   setOption(OPT_max_sim_cycles, 200000000);
   setOption(OPT_chaining, true);

   /// High-level synthesis contraints dump -- //
   setOption("dumpConstraints", false);
   setOption("dumpConstraints_file", "Constraints.XML");

   /// -- Scheduling -- //
   /// Scheduling algorithm (default is list based one)
   setOption(OPT_scheduling_algorithm, HLSFlowStep_Type::LIST_BASED_SCHEDULING);
   setOption(OPT_scheduling_priority, ParametricListBased_Metric::DYNAMIC_MOBILITY);
   /// ilp solver
#if HAVE_ILP_BUILT
#if HAVE_GLPK
   setOption(OPT_ilp_solver, meilp_solver::GLPK);
#elif HAVE_COIN_OR
   setOption(OPT_ilp_solver, meilp_solver::COIN_OR);
#elif HAVE_LP_SOLVE
   setOption(OPT_ilp_solver, meilp_solver::LP_SOLVE);
#endif
#endif
   /// speculative execution flag
   setOption(OPT_speculative, false);

   /// -- Module binding -- //
   /// module binding algorithm
   setOption(OPT_fu_binding_algorithm, HLSFlowStep_Type::CDFC_MODULE_BINDING);
   setOption(OPT_cdfc_module_binding_algorithm, CliqueCovering_Algorithm::TS_WEIGHTED_CLIQUE_COVERING);

   /// -- Finite state machine -- //
   /// flag to check if finite state machine has to be created
   setOption(OPT_stg, false);
   /// state transition graph creation algorithm
   setOption(OPT_stg_algorithm, HLSFlowStep_Type::BB_STG_CREATOR);

   /// -- Dataflow analysis -- //
   setOption(OPT_liveness_algorithm, HLSFlowStep_Type::FSM_NI_SSA_LIVENESS);

   /// -- Register allocation -- //
   /// register allocation algorithm
   setOption(OPT_register_allocation_algorithm, HLSFlowStep_Type::WEIGHTED_CLIQUE_REGISTER_BINDING);
   setOption(OPT_weighted_clique_register_algorithm, CliqueCovering_Algorithm::TS_WEIGHTED_CLIQUE_COVERING);
   /// storage value insertion algorithm
   setOption(OPT_storage_value_insertion_algorithm, HLSFlowStep_Type::VALUES_SCHEME_STORAGE_VALUE_INSERTION);
   setOption(OPT_sync_reset, "no");
   setOption(OPT_level_reset, false);
   setOption(OPT_reg_init_value, true);

   /// Function allocation
   setOption(OPT_function_allocation_algorithm, HLSFlowStep_Type::DOMINATOR_FUNCTION_ALLOCATION);

   /// Enable function proxy by default
   setOption(OPT_disable_function_proxy, false);
   setOption(OPT_disable_bounded_function, false);

   /// Disable memory mapped interface for top function by default
   setOption(OPT_memory_mapped_top, false);

   setOption(OPT_mem_delay_read, 2);
   setOption(OPT_mem_delay_write, 1);

   /// -- Memory allocation -- //
   setOption(OPT_memory_allocation_algorithm, HLSFlowStep_Type::DOMINATOR_MEMORY_ALLOCATION);
   setOption(OPT_memory_allocation_policy, MemoryAllocation_Policy::NONE);
   setOption(OPT_base_address, 1073741824); // 1Gbytes maximum address space reserved for the accelerator
   setOption(OPT_memory_controller_type, "D00");
   setOption(OPT_sparse_memory, true);
   setOption(OPT_do_not_expose_globals, false);

   /// -- Datapath -- //
   /// Datapath interconnection architecture
   setOption(OPT_datapath_interconnection_algorithm, HLSFlowStep_Type::MUX_INTERCONNECTION_BINDING);
   /// Datapath architecture
   setOption(OPT_datapath_architecture, HLSFlowStep_Type::CLASSIC_DATAPATH_CREATOR);

   /// -- Controller -- //
   /// target architecture for the controller
   setOption(OPT_controller_architecture, HLSFlowStep_Type::FSM_CONTROLLER_CREATOR);
#if HAVE_CUDD
   setOption("logical_optimization", 0);
#endif

   /// -- top entity -- //
   /// Output file name for top entity
   setOption(OPT_top_file, "top");

   /// backend HDL
   setOption(OPT_writer_language, static_cast<int>(HDLWriter_Language::VERILOG));
   std::string mingw_prefix;
   if(getenv("MINGW_INST_DIR"))
      mingw_prefix = getenv("MINGW_INST_DIR");
#ifdef _WIN32
   else
      mingw_prefix = "c:/msys64/";
#endif

   setOption("dynamic_generators_dir", mingw_prefix + PANDA_DATA_INSTALLDIR "/panda");

   /// -- Module Interfaces -- //
   setOption(OPT_interface, true);
   setOption(OPT_interface_type, HLSFlowStep_Type::MINIMAL_INTERFACE_GENERATION);
   setOption(OPT_additional_top, "");

   /// -- Module Characterization -- //
   setOption(OPT_evaluation, false);
   setOption(OPT_evaluation_mode, Evaluation_Mode::NONE);
   setOption(OPT_evaluation_objectives, "");
#if HAVE_EXPERIMENTAL
   setOption("evaluation_statistic", false);
   setOption("evaluation_reduced", false);
   setOption("evaluation_output", "evaluation.txt");
#endif

   /// -- Module Synthesis -- //
   setOption(OPT_rtl, true); /// the resulting specification will be a RTL description
#if HAVE_MODELSIM
   setOption(OPT_simulator, "MODELSIM"); /// Mixed language simulator
#elif HAVE_XILINX_VIVADO
   setOption(OPT_simulator, "XSIM"); /// Mixed language simulator
#elif HAVE_XILINX
   setOption(OPT_simulator, "ISIM"); /// Mixed language simulator
#elif HAVE_ICARUS
   setOption(OPT_simulator, "ICARUS");
#elif HAVE_VERILATOR
   setOption(OPT_simulator, "VERILATOR");
#endif
   setOption("device_name", "xc7z020");
   setOption("device_speed", "-1");
   setOption("device_package", "clg484");
   setOption("device_synthesis_tool", "VVD");
   setOption(OPT_timing_simulation, false);
   setOption(OPT_timing_violation_abort, false);
   setOption(OPT_target_device_type, static_cast<int>(TargetDevice_Type::FPGA));
   setOption(OPT_export_core, false);
#if HAVE_EXPERIMENTAL
   setOption("edk_wrapper", false);
#endif
   setOption(OPT_connect_iob, true);

#if(HAVE_EXPERIMENTAL && HAVE_BEAGLE)
   // -- Parameters for the design space exploration -- //
   setOption("exploration_technique", dse_hls::BINDING);
   setOption("to_normalize", 0);
   setOption("seed", 0);
   setOption("run", 1);
   setOption("deme_size", 1);
   setOption("population", 50);
   setOption("GA_generation", 10);
   setOption("max_evaluations", 0);
   setOption("fitness_function", objective_evaluator::LINEAR);
   setOption("fitness_inheritance_rate", 0.0);
   /// if the parameter is even, the cache is analysed. Otherwise only the latest population
   setOption("inheritance_mode", 0);
   setOption("max_for_inheritance", 50);
   setOption("min_for_inheritance", 1);
   setOption("distance_rate", 0.20);
   setOption("weighting_function", FitnessFunction::QUADRATIC);
   setOption("time_weight", 0.5);
   setOption("area_weight", 0.5);
   setOption("remove_duplicated", true);
   setOption("lower", 0.05);
   setOption("upper", 0.05);
   setOption("prob_dupl", 0.50);
#endif

   /// -- GCC options -- //

#if HAVE_I386_CLANG7_COMPILER && defined(_WIN32)
   setOption(OPT_default_compiler, static_cast<int>(GccWrapper_CompilerTarget::CT_I386_CLANG7));
#elif HAVE_I386_GCC49_COMPILER
   setOption(OPT_default_compiler, static_cast<int>(GccWrapper_CompilerTarget::CT_I386_GCC49));
#elif HAVE_I386_GCC8_COMPILER
   setOption(OPT_default_compiler, static_cast<int>(GccWrapper_CompilerTarget::CT_I386_GCC8));
#elif HAVE_I386_GCC7_COMPILER
   setOption(OPT_default_compiler, static_cast<int>(GccWrapper_CompilerTarget::CT_I386_GCC7));
#elif HAVE_I386_GCC6_COMPILER
   setOption(OPT_default_compiler, static_cast<int>(GccWrapper_CompilerTarget::CT_I386_GCC6));
#elif HAVE_I386_GCC5_COMPILER
   setOption(OPT_default_compiler, static_cast<int>(GccWrapper_CompilerTarget::CT_I386_GCC5));
#elif HAVE_I386_GCC47_COMPILER
   setOption(OPT_default_compiler, static_cast<int>(GccWrapper_CompilerTarget::CT_I386_GCC47));
#elif HAVE_I386_GCC46_COMPILER
   setOption(OPT_default_compiler, static_cast<int>(GccWrapper_CompilerTarget::CT_I386_GCC46));
#elif HAVE_I386_GCC45_COMPILER
   setOption(OPT_default_compiler, static_cast<int>(GccWrapper_CompilerTarget::CT_I386_GCC45));
#elif HAVE_I386_GCC48_COMPILER
   setOption(OPT_default_compiler, static_cast<int>(GccWrapper_CompilerTarget::CT_I386_GCC48));
#elif HAVE_I386_CLANG4_COMPILER
   setOption(OPT_default_compiler, static_cast<int>(GccWrapper_CompilerTarget::CT_I386_CLANG4));
#elif HAVE_I386_CLANG5_COMPILER
   setOption(OPT_default_compiler, static_cast<int>(GccWrapper_CompilerTarget::CT_I386_CLANG5));
#elif HAVE_I386_CLANG6_COMPILER
   setOption(OPT_default_compiler, static_cast<int>(GccWrapper_CompilerTarget::CT_I386_CLANG6));
#elif HAVE_I386_CLANG7_COMPILER
   setOption(OPT_default_compiler, static_cast<int>(GccWrapper_CompilerTarget::CT_I386_CLANG7));
#else
   THROW_ERROR("No GCC compiler available");
#endif
   setOption(OPT_compatible_compilers, 0
#if HAVE_I386_GCC45_COMPILER
                                           | static_cast<int>(GccWrapper_CompilerTarget::CT_I386_GCC45)
#endif
#if HAVE_I386_GCC46_COMPILER
                                           | static_cast<int>(GccWrapper_CompilerTarget::CT_I386_GCC46)
#endif
#if HAVE_I386_GCC47_COMPILER
                                           | static_cast<int>(GccWrapper_CompilerTarget::CT_I386_GCC47)
#endif
#if HAVE_I386_GCC48_COMPILER
                                           | static_cast<int>(GccWrapper_CompilerTarget::CT_I386_GCC48)
#endif
#if HAVE_I386_GCC49_COMPILER
                                           | static_cast<int>(GccWrapper_CompilerTarget::CT_I386_GCC49)
#endif
#if HAVE_I386_GCC5_COMPILER
                                           | static_cast<int>(GccWrapper_CompilerTarget::CT_I386_GCC5)
#endif
#if HAVE_I386_GCC6_COMPILER
                                           | static_cast<int>(GccWrapper_CompilerTarget::CT_I386_GCC6)
#endif
#if HAVE_I386_GCC7_COMPILER
                                           | static_cast<int>(GccWrapper_CompilerTarget::CT_I386_GCC7)
#endif
#if HAVE_I386_GCC8_COMPILER
                                           | static_cast<int>(GccWrapper_CompilerTarget::CT_I386_GCC8)
#endif
#if HAVE_I386_CLANG4_COMPILER
                                           | static_cast<int>(GccWrapper_CompilerTarget::CT_I386_CLANG4)
#endif
#if HAVE_I386_CLANG5_COMPILER
                                           | static_cast<int>(GccWrapper_CompilerTarget::CT_I386_CLANG5)
#endif
#if HAVE_I386_CLANG6_COMPILER
                                           | static_cast<int>(GccWrapper_CompilerTarget::CT_I386_CLANG6)
#endif
#if HAVE_I386_CLANG7_COMPILER
                                           | static_cast<int>(GccWrapper_CompilerTarget::CT_I386_CLANG7)
#endif
#if HAVE_ARM_COMPILER
                                           | static_cast<int>(GccWrapper_CompilerTarget::CT_ARM_GCC)
#endif
#if HAVE_SPARC_COMPILER
                                           | static_cast<int>(GccWrapper_CompilerTarget::CT_SPARC_GCC)
#endif
   );

#if(HAVE_I386_GCC49_COMPILER && HAVE_I386_GCC49_M32)
   setOption(OPT_gcc_m32_mx32, "-m32 -mno-sse2 ");
#elif(HAVE_I386_GCC49_COMPILER && HAVE_I386_GCC49_MX32)
   setOption(OPT_gcc_m32_mx32, "-mx32 ");
#elif(HAVE_I386_GCC49_COMPILER && HAVE_I386_GCC49_M64)
   setOption(OPT_gcc_m32_mx32, "-m64 ");
#elif(HAVE_I386_GCC8_COMPILER && HAVE_I386_GCC8_M32)
   setOption(OPT_gcc_m32_mx32, "-m32 -mno-sse2 ");
#elif(HAVE_I386_GCC8_COMPILER && HAVE_I386_GCC8_MX32)
   setOption(OPT_gcc_m32_mx32, "-mx32 ");
#elif(HAVE_I386_GCC8_COMPILER && HAVE_I386_GCC8_M64)
   setOption(OPT_gcc_m32_mx32, "-m64 ");
#elif(HAVE_I386_GCC7_COMPILER && HAVE_I386_GCC7_M32)
   setOption(OPT_gcc_m32_mx32, "-m32 -mno-sse2 ");
#elif(HAVE_I386_GCC7_COMPILER && HAVE_I386_GCC7_MX32)
   setOption(OPT_gcc_m32_mx32, "-mx32 ");
#elif(HAVE_I386_GCC7_COMPILER && HAVE_I386_GCC7_M64)
   setOption(OPT_gcc_m32_mx32, "-m64 ");
#elif(HAVE_I386_GCC6_COMPILER && HAVE_I386_GCC6_M32)
   setOption(OPT_gcc_m32_mx32, "-m32 -mno-sse2 ");
#elif(HAVE_I386_GCC6_COMPILER && HAVE_I386_GCC6_MX32)
   setOption(OPT_gcc_m32_mx32, "-mx32 ");
#elif(HAVE_I386_GCC6_COMPILER && HAVE_I386_GCC6_M64)
   setOption(OPT_gcc_m32_mx32, "-m64 ");
#elif(HAVE_I386_GCC5_COMPILER && HAVE_I386_GCC5_M32)
   setOption(OPT_gcc_m32_mx32, "-m32 -mno-sse2 ");
#elif(HAVE_I386_GCC5_COMPILER && HAVE_I386_GCC5_MX32)
   setOption(OPT_gcc_m32_mx32, "-mx32 ");
#elif(HAVE_I386_GCC5_COMPILER && HAVE_I386_GCC5_M64)
   setOption(OPT_gcc_m32_mx32, "-m64 ");
#elif(HAVE_I386_GCC48_COMPILER && HAVE_I386_GCC48_M32)
   setOption(OPT_gcc_m32_mx32, "-m32 -mno-sse2 ");
#elif(HAVE_I386_GCC48_COMPILER && HAVE_I386_GCC48_MX32)
   setOption(OPT_gcc_m32_mx32, "-mx32 ");
#elif(HAVE_I386_GCC48_COMPILER && HAVE_I386_GCC48_M64)
   setOption(OPT_gcc_m32_mx32, "-m64 ");
#elif(HAVE_I386_GCC47_COMPILER && HAVE_I386_GCC47_M32)
   setOption(OPT_gcc_m32_mx32, "-m32 -mno-sse2 ");
#elif(HAVE_I386_GCC47_COMPILER && HAVE_I386_GCC47_MX32)
   setOption(OPT_gcc_m32_mx32, "-mx32 ");
#elif(HAVE_I386_GCC47_COMPILER && HAVE_I386_GCC47_M64)
   setOption(OPT_gcc_m32_mx32, "-m64 ");
#elif(HAVE_I386_GCC46_COMPILER)
   setOption(OPT_gcc_m32_mx32, "-m32 -mno-sse2 ");
#elif(HAVE_I386_GCC45_COMPILER)
   setOption(OPT_gcc_m32_mx32, "-m32 -mno-sse2 ");
#elif(HAVE_I386_CLANG4_COMPILER && HAVE_I386_CLANG4_M32)
   setOption(OPT_gcc_m32_mx32, "-m32 -mno-sse2 ");
#elif(HAVE_I386_CLANG4_COMPILER && HAVE_I386_CLANG4_MX32)
   setOption(OPT_gcc_m32_mx32, "-mx32 ");
#elif(HAVE_I386_CLANG4_COMPILER && HAVE_I386_CLANG4_M64)
   setOption(OPT_gcc_m32_mx32, "-m64 ");
#elif(HAVE_I386_CLANG5_COMPILER && HAVE_I386_CLANG5_M32)
   setOption(OPT_gcc_m32_mx32, "-m32 -mno-sse2 ");
#elif(HAVE_I386_CLANG5_COMPILER && HAVE_I386_CLANG5_MX32)
   setOption(OPT_gcc_m32_mx32, "-mx32 ");
#elif(HAVE_I386_CLANG5_COMPILER && HAVE_I386_CLANG5_M64)
   setOption(OPT_gcc_m32_mx32, "-m64 ");
#elif(HAVE_I386_CLANG6_COMPILER && HAVE_I386_CLANG6_M32)
   setOption(OPT_gcc_m32_mx32, "-m32 -mno-sse2 ");
#elif(HAVE_I386_CLANG6_COMPILER && HAVE_I386_CLANG6_MX32)
   setOption(OPT_gcc_m32_mx32, "-mx32 ");
#elif(HAVE_I386_CLANG6_COMPILER && HAVE_I386_CLANG6_M64)
   setOption(OPT_gcc_m32_mx32, "-m64 ");
#elif(HAVE_I386_CLANG7_COMPILER && HAVE_I386_CLANG7_M32)
   setOption(OPT_gcc_m32_mx32, "-m32 -mno-sse2 ");
#elif(HAVE_I386_CLANG7_COMPILER && HAVE_I386_CLANG7_MX32)
   setOption(OPT_gcc_m32_mx32, "-mx32 ");
#elif(HAVE_I386_CLANG7_COMPILER && HAVE_I386_CLANG7_M64)
   setOption(OPT_gcc_m32_mx32, "-m64 ");
#else
   THROW_ERROR("None of -m32, -mx32, -m64 GCC option is supported");
#endif

   setOption(OPT_without_transformation, true);
   setOption(OPT_compute_size_of, true);
   setOption(OPT_precision, 3);
   setOption(OPT_gcc_c, true);
   setOption(OPT_gcc_config, false);
   setOption(OPT_gcc_costs, false);
   setOption(OPT_gcc_openmp_simd, 0);
   setOption(OPT_gcc_optimization_set, GccWrapper_OptimizationSet::OBAMBU);
   setOption(OPT_gcc_include_sysdir, false);
   setOption(OPT_model_costs, false);

   std::string defines;
   if(isOption(OPT_gcc_defines))
      defines = getOption<std::string>(OPT_gcc_defines) + STR_CST_string_separator;
   defines += "__BAMBU__";
   setOption(OPT_gcc_defines, defines);

   setOption(OPT_soft_float, true);
   setOption(OPT_hls_div, "nr1");
   setOption(OPT_hls_fpdiv, "SRT4");
   setOption(OPT_max_ulp, 1.0);
   setOption(OPT_skip_pipe_parameter, 0);
   setOption(OPT_unaligned_access, false);
   setOption(OPT_aligned_access, false);
   setOption(OPT_gcc_serialize_memory_accesses, false);
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

#if HAVE_HOST_PROFILING_BUILT
   setOption(OPT_exec_argv, STR_CST_string_separator);
   setOption(OPT_profiling_method, static_cast<int>(HostProfiling_Method::PM_NONE));
#if HAVE_I386_GCC45_COMPILER
   setOption(OPT_host_compiler, static_cast<int>(GccWrapper_CompilerTarget::CT_I386_GCC45));
#elif HAVE_I386_GCC46_COMPILER
   setOption(OPT_host_compiler, static_cast<int>(GccWrapper_CompilerTarget::CT_I386_GCC46));
#elif HAVE_I386_GCC47_COMPILER
   setOption(OPT_host_compiler, static_cast<int>(GccWrapper_CompilerTarget::CT_I386_GCC47));
#elif HAVE_I386_GCC48_COMPILER
   setOption(OPT_host_compiler, static_cast<int>(GccWrapper_CompilerTarget::CT_I386_GCC48));
#elif HAVE_I386_GCC49_COMPILER
   setOption(OPT_host_compiler, static_cast<int>(GccWrapper_CompilerTarget::CT_I386_GCC49));
#elif HAVE_I386_GCC5_COMPILER
   setOption(OPT_host_compiler, static_cast<int>(GccWrapper_CompilerTarget::CT_I386_GCC5));
#elif HAVE_I386_GCC6_COMPILER
   setOption(OPT_host_compiler, static_cast<int>(GccWrapper_CompilerTarget::CT_I386_GCC6));
#elif HAVE_I386_GCC7_COMPILER
   setOption(OPT_host_compiler, static_cast<int>(GccWrapper_CompilerTarget::CT_I386_GCC7));
#elif HAVE_I386_GCC8_COMPILER
   setOption(OPT_host_compiler, static_cast<int>(GccWrapper_CompilerTarget::CT_I386_GCC8));
#elif HAVE_I386_CLANG4_COMPILER
   setOption(OPT_host_compiler, static_cast<int>(GccWrapper_CompilerTarget::CT_I386_CLANG4));
#elif HAVE_I386_CLANG5_COMPILER
   setOption(OPT_host_compiler, static_cast<int>(GccWrapper_CompilerTarget::CT_I386_CLANG5));
#elif HAVE_I386_CLANG6_COMPILER
   setOption(OPT_host_compiler, static_cast<int>(GccWrapper_CompilerTarget::CT_I386_CLANG6));
#elif HAVE_I386_CLANG7_COMPILER
   setOption(OPT_host_compiler, static_cast<int>(GccWrapper_CompilerTarget::CT_I386_CLANG7));
#else
   THROW_ERROR("No GCC compiler available");
#endif
#endif
   setOption(OPT_clock_period, 10.0);
#if HAVE_EXPERIMENTAL
   setOption(OPT_mixed_design, true);
#endif
#if HAVE_TASTE
   setOption(OPT_generate_taste_architecture, false);
#endif
#if HAVE_FROM_PRAGMA_BUILT && HAVE_BAMBU_BUILT
   setOption(OPT_num_accelerators, 4);
#endif
   setOption(OPT_memory_banks_number, 1);
   setOption(OPT_find_max_cfg_transformations, false);

   panda_parameters["CSE_size"] = "2";
   panda_parameters["PortSwapping"] = "1";
   //   panda_parameters["enable-CSROA"] = "1";
   panda_parameters["MAX_LUT_INT_SIZE"] = "8";
}

void BambuParameter::add_bambu_library(std::string lib)
{
#if HAVE_I386_GCC45_COMPILER || HAVE_I386_GCC46_COMPILER || HAVE_I386_GCC47_COMPILER || HAVE_I386_GCC48_COMPILER || HAVE_I386_GCC49_COMPILER || HAVE_I386_GCC5_COMPILER || HAVE_I386_GCC6_COMPILER || HAVE_I386_GCC7_COMPILER || HAVE_I386_GCC8_COMPILER || \
    HAVE_I386_CLANG4_COMPILER || HAVE_I386_CLANG5_COMPILER || HAVE_I386_CLANG6_COMPILER || HAVE_I386_CLANG7_COMPILER
   auto preferred_compiler = getOption<unsigned int>(OPT_default_compiler);
   std::string archive_files;
   bool is_subnormals = isOption(OPT_softfloat_subnormal) && getOption<bool>(OPT_softfloat_subnormal);
   std::string VSuffix = "";
   if(is_subnormals && lib == "m")
   {
      if(isOption(OPT_libm_std_rounding) && getOption<int>(OPT_libm_std_rounding))
         VSuffix = "_subnormals_std";
      else
         VSuffix = "_subnormals";
   }
   else if(lib == "m")
   {
      if(isOption(OPT_libm_std_rounding) && getOption<int>(OPT_libm_std_rounding))
         VSuffix = "_std";
   }
   if(isOption(OPT_archive_files))
      archive_files = getOption<std::string>(OPT_archive_files) + STR_CST_string_separator;
   std::string mingw_prefix;
   if(getenv("MINGW_INST_DIR"))
      mingw_prefix = getenv("MINGW_INST_DIR");
#ifdef _WIN32
   else
      mingw_prefix = "c:/msys64/";
#endif
#endif

#if HAVE_I386_GCC45_COMPILER
   if(static_cast<int>(preferred_compiler) & static_cast<int>(GccWrapper_CompilerTarget::CT_I386_GCC45))
   {
      setOption(OPT_archive_files, archive_files + mingw_prefix + PANDA_LIB_INSTALLDIR "/panda/lib" + lib + "_gcc45" + VSuffix + ".a");
   }
#endif
#if HAVE_I386_GCC46_COMPILER
   if(static_cast<int>(preferred_compiler) & static_cast<int>(GccWrapper_CompilerTarget::CT_I386_GCC46))
   {
      setOption(OPT_archive_files, archive_files + mingw_prefix + PANDA_LIB_INSTALLDIR "/panda/lib" + lib + "_gcc46" + VSuffix + ".a");
   }
#endif
#if HAVE_I386_GCC47_COMPILER
   if(static_cast<int>(preferred_compiler) & static_cast<int>(GccWrapper_CompilerTarget::CT_I386_GCC47))
   {
      setOption(OPT_archive_files, archive_files + mingw_prefix + PANDA_LIB_INSTALLDIR "/panda/lib" + lib + "_gcc47" + VSuffix + ".a");
   }
#endif
#if HAVE_I386_GCC48_COMPILER
   if(static_cast<int>(preferred_compiler) & static_cast<int>(GccWrapper_CompilerTarget::CT_I386_GCC48))
   {
      setOption(OPT_archive_files, archive_files + mingw_prefix + PANDA_LIB_INSTALLDIR "/panda/lib" + lib + "_gcc48" + VSuffix + ".a");
   }
#endif
#if HAVE_I386_GCC49_COMPILER
   if(static_cast<int>(preferred_compiler) & static_cast<int>(GccWrapper_CompilerTarget::CT_I386_GCC49))
   {
      setOption(OPT_archive_files, archive_files + mingw_prefix + PANDA_LIB_INSTALLDIR "/panda/lib" + lib + "_gcc49" + VSuffix + ".a");
   }
#endif
#if HAVE_I386_GCC5_COMPILER
   if(static_cast<int>(preferred_compiler) & static_cast<int>(GccWrapper_CompilerTarget::CT_I386_GCC5))
   {
      setOption(OPT_archive_files, archive_files + mingw_prefix + PANDA_LIB_INSTALLDIR "/panda/lib" + lib + "_gcc5" + VSuffix + ".a");
   }
#endif
#if HAVE_I386_GCC6_COMPILER
   if(static_cast<int>(preferred_compiler) & static_cast<int>(GccWrapper_CompilerTarget::CT_I386_GCC6))
   {
      setOption(OPT_archive_files, archive_files + PANDA_LIB_INSTALLDIR "/panda/lib" + lib + "_gcc6" + VSuffix + ".a");
   }
#endif
#if HAVE_I386_GCC7_COMPILER
   if(static_cast<int>(preferred_compiler) & static_cast<int>(GccWrapper_CompilerTarget::CT_I386_GCC7))
   {
      setOption(OPT_archive_files, archive_files + mingw_prefix + PANDA_LIB_INSTALLDIR "/panda/lib" + lib + "_gcc7" + VSuffix + ".a");
   }
#endif
#if HAVE_I386_GCC8_COMPILER
   if(static_cast<int>(preferred_compiler) & static_cast<int>(GccWrapper_CompilerTarget::CT_I386_GCC8))
   {
      setOption(OPT_archive_files, archive_files + mingw_prefix + PANDA_LIB_INSTALLDIR "/panda/lib" + lib + "_gcc8" + VSuffix + ".a");
   }
#endif
#if HAVE_I386_CLANG4_COMPILER
   if(static_cast<int>(preferred_compiler) & static_cast<int>(GccWrapper_CompilerTarget::CT_I386_CLANG4))
   {
      setOption(OPT_archive_files, archive_files + mingw_prefix + PANDA_LIB_INSTALLDIR "/panda/lib" + lib + "_clang4" + VSuffix + ".a");
   }
#endif
#if HAVE_I386_CLANG5_COMPILER
   if(static_cast<int>(preferred_compiler) & static_cast<int>(GccWrapper_CompilerTarget::CT_I386_CLANG5))
   {
      setOption(OPT_archive_files, archive_files + mingw_prefix + PANDA_LIB_INSTALLDIR "/panda/lib" + lib + "_clang5" + VSuffix + ".a");
   }
#endif
#if HAVE_I386_CLANG6_COMPILER
   if(static_cast<int>(preferred_compiler) & static_cast<int>(GccWrapper_CompilerTarget::CT_I386_CLANG6))
   {
      setOption(OPT_archive_files, archive_files + mingw_prefix + PANDA_LIB_INSTALLDIR "/panda/lib" + lib + "_clang6" + VSuffix + ".a");
   }
#endif
#if HAVE_I386_CLANG7_COMPILER
   if(static_cast<int>(preferred_compiler) & static_cast<int>(GccWrapper_CompilerTarget::CT_I386_CLANG7))
   {
      setOption(OPT_archive_files, archive_files + mingw_prefix + PANDA_LIB_INSTALLDIR "/panda/lib" + lib + "_clang7" + VSuffix + ".a");
   }
#endif
}
