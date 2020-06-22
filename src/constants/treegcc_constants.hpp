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

/// The first version of gcc not suppoted
#define STR_CST_gcc_first_not_supported "9.0.0"

/// The temporary directory used by treegcc
#define STR_CST_gcc_include "__include"

/// The maximum version of the plugin supported
#define STR_CST_gcc_max_plugin_version "0.11"

/// The minimum version of the plugin supported
#define STR_CST_gcc_min_plugin_version "0.10"

/// The output file for tree-panda-gcc
#define STR_CST_gcc_obj_file "run-%%%%-%%%%-%%%%-%%%%.o"

/// bitcode LLVM temporary file
#define STR_CST_llvm_obj_file "run-%%%%-%%%%-%%%%-%%%%.bc"

/// concatenated C temporary file
#define STR_CST_concat_c_file ".concat-%%%%-%%%%-%%%%-%%%%.c"

/// The file where output messages of gcc are saved
#define STR_CST_gcc_output "__gcc_output"

/// The suffix of rtl files
#define STR_CST_gcc_rtl_suffix ".rtlExpand"

/// The list of tested versions of gcc
#define STR_CST_gcc_supported_versions                                                                                                                                                                                                                      \
   "4.5.2 4.5.3 4.5.4 4.6.1 4.6.3 4.6.4 4.7.0 4.7.2 4.7.3 4.7.4 4.8.0 4.8.1 4.8.2 4.8.3 4.8.4 4.8.5 4.9.0 4.9.1 4.9.2 4.9.3 4.9.4 5.1.0 5.1.1 5.2.0 5.2.1 5.3.0 5.3.1 5.4.0 5.4.1 5.5.0 6.0.0 6.2.0 6.2.1 6.3.0 6.4.0 6.5.0 7.0.1 7.1.0 7.3.0 7.4.0 7.5.0 " \
   "8.0.1 8.2.0 8.2.1 8.3.0 8.3.1 8.4.0 4.2.1"

/// The suffix of gimple files
#define STR_CST_gcc_tree_suffix ".gimplePSSA"

/// The string used to replace sizeof keyword in the original source code (first step)
#define STR_CST_panda_sizeof "__panda_sizeof"

/// The string used to replace sizeof keyword in the original source code (second step)
#define STR_CST_string_sizeof "__string_sizeof"

#endif
