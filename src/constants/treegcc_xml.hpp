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
 * @file treegcc_xml.hpp
 * @brief xml nodes used in gcc configuration
 *
 * @author Marco Lattuada <lattuada@elet.polimi.it>
 * $Date$
 * Last modified by $Author$
 *
 */

#ifndef TREEGCC_XML_HPP
#define TREEGCC_XML_HPP

/// Node containing information about defines
#define STR_XML_gcc_defines "defines"

/// Node containing information about includes
#define STR_XML_gcc_includes "includes"

/// Node containing information about libraries
#define STR_XML_gcc_libraries "libraries"

/// Node containing information about library_directories
#define STR_XML_gcc_library_directories "library_directories"

/// Attribute containing information about name of optimization/parameter
#define STR_XML_gcc_name "name"

/// Node containing information about optimizations
#define STR_XML_gcc_optimizations "optimizations"

/// Node containing information about an optimization flag
#define STR_XML_gcc_optimization_flag "optimization_flag"

/// Node containing information about optimization flags
#define STR_XML_gcc_optimization_flags "optimization_flags"

/// Node containing information about an optimization value
#define STR_XML_gcc_optimization_value "optmization_value"

/// Node containing information about optimization values
#define STR_XML_gcc_optimization_values "optmization_values"

/// Node containing information about a parameter value
#define STR_XML_gcc_parameter_value "parameter_value"

/// Node containing information about parameter values
#define STR_XML_gcc_parameter_values "parameter_values"

/// The root node of a gcc configuration
#define STR_XML_gcc_root "gcc_configuration"

/// Node containing information about standard
#define STR_XML_gcc_standard "standard"

/// Node containing information about undefines
#define STR_XML_gcc_undefines "undefines"

/// Attribute containing information about value stored in a node
#define STR_XML_gcc_value "value"

/// Node containing information about warnings
#define STR_XML_gcc_warnings "warnings"

#endif
