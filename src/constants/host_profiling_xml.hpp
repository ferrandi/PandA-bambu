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
 * @file host_profiling_xml.hpp
 * @brief xml nodes of host profiling data
 *
 * @author Marco Lattuada <marco.lattuada@polimi.it>
 *
 */

/// The node containing information about absolute iterations of a loop
#define STR_XML_host_profiling_abs_iterations "abs_iteration"

/// The node containing information about absolute iterations of loop
#define STR_XML_host_profiling_abs_iteration "abs_iteration"

/// The node containing information about average iterations of a loop
#define STR_XML_host_profiling_avg_iteration "avg_iteration"

/// The node containing information about average iterations of loops
#define STR_XML_host_profiling_avg_iterations "avg_iterations"

/// The node containing information about executions of a basic block
#define STR_XML_host_profiling_bb_execution "bb_execution"

/// The node containing information about basic blocks executions
#define STR_XML_host_profiling_bb_executions "bb_executions"

/// The attribute containing the sequence of cers of the path
#define STR_XML_host_profiling_cers "cers"

/// The node containing information about executions of a basic block
#define STR_XML_host_profiling_edge_execution "edge_execution"

/// The node containing information about basic blocks executions
#define STR_XML_host_profiling_edge_executions "edge_executions"

/// The attribute containing the number of executions of a basic block
#define STR_XML_host_profiling_executions "executions"

/// The attribute containing the frequency of a path
#define STR_XML_host_profiling_frequency "frequency"

/// The node containing information about profiling of a function
#define STR_XML_host_profiling_function "function"

/// The attribute containing the name of the function
#define STR_XML_host_profiling_name "function_name"

/// The attribute containing the id of a bb, function or loop
#define STR_XML_host_profiling_id "id"

/// The attribute containing the number of iterations
#define STR_XML_host_profiling_iterations "iterations"

/// The node containing information about max iterations of a loop
#define STR_XML_host_profiling_max_iteration "max_iteration"

/// The node containing information about max iterations of loops
#define STR_XML_host_profiling_max_iterations "max_iterations"

/// The node containing the EPP overhead time
#define STR_XML_host_profiling_overhead_EPP_time "EPP_time"

/// The node containing the HPP overhead time
#define STR_XML_host_profiling_overhead_HPP_time "HPP_time"

/// The root of the profiling overhead time file
#define STR_XML_host_profiling_overhead_time "profiling_overhead_time"

/// The attribute containing the value of a node
#define STR_XML_host_profiling_overhead_value "value"

/// The node containing information about a single path
#define STR_XML_host_profiling_path "path"

/// The node containing information path profiling
#define STR_XML_host_profiling_paths "profiling_paths"

/// The node containing information about path profiling in a loop
#define STR_XML_host_profiling_paths_loop "loop"

/// The root
#define STR_XML_host_profiling_root "host_profiling_data"

/// The attribute containing  id of the source vertex of the edge
#define STR_XML_host_profiling_source_id "source"

/// The attribute containing  id of the target vertex of the edge
#define STR_XML_host_profiling_target_id "target"
