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
 *              Copyright (C) 2004-2024 Politecnico di Milano
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
 * @file treegcc_constants.hpp
 * @brief constants used by gcc wrapper
 *
 * @author Marco Lattuada <lattuada@elet.polimi.it>
 * $Date$
 * Last modified by $Author$
 *
 */

#ifndef TREEGCC_CONSTANTS_HPP
#define TREEGCC_CONSTANTS_HPP

/// The suffix of gcc initial dump
#define STR_CST_gcc_empty_suffix ".001t.tu"

/// The temporary directory used by compiler
#define STR_CST_gcc_include "__include"

/// The output file for tree-panda-gcc
#define STR_CST_gcc_obj_file "run-%%%%-%%%%-%%%%-%%%%.o"

/// bitcode LLVM temporary file
#define STR_CST_llvm_obj_file "run-%%%%-%%%%-%%%%-%%%%.bc"

/// concatenated C temporary file
#define STR_CST_concat_c_file ".concat-%%%%-%%%%-%%%%-%%%%.c"

/// The file where output messages of gcc are saved
#define STR_CST_gcc_output "__gcc_output"

/// The suffix of gimple files
#define STR_CST_bambu_ir_suffix ".bambuir"

/// The string used to replace sizeof keyword in the original source code (first step)
#define STR_CST_panda_sizeof "__panda_sizeof"

/// The string used to replace sizeof keyword in the original source code (second step)
#define STR_CST_string_sizeof "__string_sizeof"

#endif
