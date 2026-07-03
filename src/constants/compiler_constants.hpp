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
