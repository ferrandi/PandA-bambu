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
 * @file compiler_constants.hpp
 * @brief constants used by compiler wrapper
 *
 * @author Marco Lattuada <lattuada@elet.polimi.it>
 *
 */

#ifndef COMPILER_CONSTANTS_HPP
#define COMPILER_CONSTANTS_HPP

/// The output file for bambu-cc
#define STR_CST_cc_obj_file "run-%%%%-%%%%-%%%%-%%%%.o"

/// concatenated C temporary file
#define STR_CST_concat_c_file ".concat-%%%%-%%%%-%%%%-%%%%.c"

/// The file where output messages of cc are saved
#define STR_CST_cc_output "__cc_output"

/// The suffix of bambu IR files
#define STR_CST_bambu_ir_suffix ".bambuir"

/// OpenMP source library file
#define STR_CST_libopenmp_filename "libopenmp/kmp_single_file.cpp"

#endif
