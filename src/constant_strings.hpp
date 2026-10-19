/*
 *
 *        _/_/_/    _/_/   _/    _/ _/_/_/    _/_/
 *       _/   _/ _/    _/ _/_/  _/ _/   _/ _/    _/
 *      _/_/_/  _/_/_/_/ _/  _/_/ _/   _/ _/_/_/_/
 *     _/      _/    _/ _/    _/ _/   _/ _/    _/
 *    _/      _/    _/ _/    _/ _/_/_/  _/    _/
 *
 *  ***********************************************
 *                   PandA Project
 *   URL: https://github.com/ferrandi/PandA-bambu
 *            Politecnico di Milano - DEIB
 *             System Architectures Group
 *  ***********************************************
 *   Copyright (C) 2004-2026 Politecnico di Milano
 *
 * Part of the PandA Project, under the Apache License v2.0 with LLVM Exceptions.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 *
 */
/**
 * @file constant_strings.hpp
 * @brief constant strings
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 * @author Marco Lattuada <lattuada@elet.polimi.it>
 *
 */
#ifndef CONSTANT_OPTIONS_HPP
#define CONSTANT_OPTIONS_HPP

/// Parameter of the current benchmark for table results (this value is not used for profiling)
#define STR_OPT_benchmark_fake_parameters "benchmark_fake_parameters"

/// The number of surviving benchmarks
#define NUM_CST_surviving_benchmarks 300

/// The temporary directory pattern
#define STR_CST_temporary_directory "panda-temp"

/**
 * Parameters
 */
/// The string representing all classes
#define STR_CST_debug_all "ALL"

/// interface_parameter_keyword
#define STR_CST_interface_parameter_keyword "_bambu_artificial_ParmMgr"

#endif
