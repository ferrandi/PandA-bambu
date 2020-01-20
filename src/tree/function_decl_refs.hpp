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
 * @file function_decl_refs.hpp
 * @brief tree node visitor collecting the function_decl referred by starting from a given node
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *
 */
#ifndef function_decl_refs_HPP
#define function_decl_refs_HPP

#include "custom_set.hpp" // for unordered_set

/// Tree include
#include "tree_node.hpp"

/// Utility include
#include "refcount.hpp"
#include <boost/preprocessor/facilities/empty.hpp>
#include <boost/preprocessor/seq/for_each.hpp>

/**
 * @name forward declarations
 */
//@{
REF_FORWARD_DECL(function_decl_refs);
//@}

struct function_decl_refs : public tree_node_visitor
{
   /// default constructor
   explicit function_decl_refs(CustomUnorderedSet<unsigned int>& _function_decl_set) : function_decl_set(_function_decl_set)
   {
   }

   /// tree_node visitors
   BOOST_PP_SEQ_FOR_EACH(OPERATOR_MACRO_DECL, BOOST_PP_EMPTY, OBJ_SPECIALIZED_SEQ)
   BOOST_PP_SEQ_FOR_EACH(OPERATOR_MACRO, BOOST_PP_EMPTY, OBJ_NOT_SPECIALIZED_SEQ)

 private:
   /// set of types used in type casting
   CustomUnorderedSet<unsigned int>& function_decl_set;

   /// already visited
   CustomUnorderedSet<unsigned int> visited;
};

#endif
