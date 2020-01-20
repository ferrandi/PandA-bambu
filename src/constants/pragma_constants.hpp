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
 * @file pragma_constants.hpp
 * @brief constant strings used in pragma identification
 *
 * @author Marco Lattuada <lattuada@elet.polimi.it>
 * $Date$
 * Last modified by $Author$
 *
 */

#ifndef PRAGMA_CONSTANTS_HPP
#define PRAGMA_CONSTANTS_HPP

/// The prefix common to all functions replacing pragma
#define STR_CST_pragma_prefix "__pragma"

/// The implicit clause for critical session name
#define STR_CST_pragma_clause_name "name"

/// The name of default critical session
#define STR_CST_pragma_default_name "__default__"

/// The function replacing an ending pragma
#define STR_CST_pragma_function_end STR_CST_pragma_prefix "_end__"

/// The function replacin a generic pragma
#define STR_CST_pragma_function_generic STR_CST_pragma_prefix "_generic__"

/// The function replacing a single line pragma with single argument
#define STR_CST_pragma_function_single_line_one_argument STR_CST_pragma_prefix "_single_line_one_argument__"

/// The function replacing a single line pragma with two argument
#define STR_CST_pragma_function_single_line_two_arguments STR_CST_pragma_prefix "_single_line_two_arguments__"

/// The function replacing an opening pragma
#define STR_CST_pragma_function_start STR_CST_pragma_prefix "_start__"

/// The call_point_hw pragma keyword
#define STR_CST_pragma_keyword_call_hw "call_hw"

/// The call_point_hw pragma keyword
#define STR_CST_pragma_keyword_call_point_hw "call_point_hw"

/// The keyword 'map' which identifies mapping pragmas
#define STR_CST_pragma_keyword_map "map"

/// The keyword 'omp' which identifies openmp pragmas
#define STR_CST_pragma_keyword_omp "omp"

/// The keyword 'omp' which identifies openmp recursive
#define STR_CST_pragma_keyword_recursive "recursive"

#endif
