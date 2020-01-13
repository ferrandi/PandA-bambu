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
 * @file host_profiling_constants.hpp
 * @brief constants used in host profiling library
 *
 * @author Marco Lattuada <marco.lattuada@polimi.it>
 *
 */

#ifndef HOST_PROFILING_CONSTANTS_HPP
#define HOST_PROFILING_CONSTANTS_HPP

/// The default number of iteration per loop
#define NUM_CST_host_profiling_max_iterations_number 100

/// The default average number of iterations
#define NUM_CST_host_profiling_avg_iterations_number 100

/// The string identifing the beginning of a loop execution
#define STR_CST_host_profiling_begin "begin"

/// The file where profiling data are written by instrumented executable
#define STR_CST_host_profiling_data "profile.dat"

/// The instrumented file for data memory profiling
#define STR_CST_host_profiling_data_memory_profiling "data_memory_profiling.c"

/// The string identifing the end of a loop execution
#define STR_CST_host_profiling_end "end"

/// The file where output of grep will be stored
#define STR_CST_host_profiling_grep "grep_output_file"

/// The file used to store profiling information about maximum number of loop iteration
#define STR_CST_host_profiling_loop_data "__loop_data.dat"

/// The instrumented file for loop profiling
#define STR_CST_host_profiling_loops_profiling "LP_profiling.c"

/// The function to get entry in memory profiling data map
#define STR_CST_host_profiling_map_get "__map_get"

/// The function to insert entry in memory profiling data map
#define STR_CST_host_profiling_map_insert "__map_insert"

/// The instrumented file for memory profiling
#define STR_CST_host_profiling_memory_profiling "memory_profiling.c"

/// The instrumented file for tree path profiling
#define STR_CST_host_profiling_tp_profiling "tp_profiling.c"

/// The file where profiling overhead data are saved
#define STR_CST_host_profiling_overhead "profiling_overhead.xml"

/// The file where output of instrumented executable is saved
#define STR_CST_host_profiling_output "profiling_exe_output"

/// The file containing pointed data evaluation results
#define STR_CST_host_profiling_pointed_data_evaluation_results "pointed_data_evaluation.xml"

/// The string identifing a read memory access
#define STR_CST_host_profiling_read "read"

/// The compiled file for running on host architecture
#define STR_CST_host_profiling_run "host.run"

/// The tag identifing in the output
#define STR_CST_host_profiling_tag "#HP#"

/// The string identifing a write memory access
#define STR_CST_host_profiling_write "write"

/// The xml file containing profiling data
#define STR_CST_host_profiling_xml_data "profiling_data.xml"

#endif
