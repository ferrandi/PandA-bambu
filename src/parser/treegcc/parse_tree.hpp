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
 * @file parse_tree.hpp
 * @brief Specification of the tree (GCC raw) parsing interface function.
 *
 * Declaration of the function that parse a tree from a file.
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 * $Revision$
 * $Date$
 * @warning This file is still in a work in progress state
 * @warning Last modified by $Author$
 *
 */
#ifndef PARSE_TREE_HPP
#define PARSE_TREE_HPP

/// Utility include
#include "refcount.hpp"

/**
 * @name forward declarations
 */
//@{
CONSTREF_FORWARD_DECL(Parameter);
REF_FORWARD_DECL(tree_manager);
//@}
/**
 * Function that parse the dump of the patched GCC.
 *
 * @param Param is the set of input parameters
 * @param f the input file name
 * @param debug_level is the the debug_level
 * @return the tree manager associated to the raw file.

*/
tree_managerRef ParseTreeFile(const ParameterConstRef& Param, const std::string& f);

#endif
