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
 * @file tree_node_finder.hpp
 * @brief tree node finder. This class exploiting the visitor design pattern find a tree node in a tree_manager.
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *
 */
#ifndef TREE_NODE_FINDER_HPP
#define TREE_NODE_FINDER_HPP

#include "custom_map.hpp"      // for map
#include "token_interface.hpp" // for TreeVocabula...
#include <string>              // for string

/// Tree include
#include "tree_node.hpp"
#include "tree_node_mask.hpp"

/// Utility include
#include "refcount.hpp"
#include <boost/preprocessor/facilities/empty.hpp>
#include <boost/preprocessor/seq/for_each.hpp>

/**
 * @name forward declarations
 */
//@{
REF_FORWARD_DECL(tree_node_finder);
//@}

struct tree_node_finder : public tree_node_mask
{
   /// default constructor
   explicit tree_node_finder(const std::map<TreeVocabularyTokenTypes_TokenEnum, std::string>& _tree_node_schema) : find_res(true), tree_node_schema(_tree_node_schema)
   {
   }
   /// tree_node visitors
   BOOST_PP_SEQ_FOR_EACH(OPERATOR_MACRO_DECL, BOOST_PP_EMPTY, OBJ_SPECIALIZED_SEQ)
   BOOST_PP_SEQ_FOR_EACH(OPERATOR_MACRO, BOOST_PP_EMPTY, OBJ_NOT_SPECIALIZED_SEQ)

   /// Return true in case the tree node is compatible with the tree_node_schema. Usually called by tree_manager::create_tree_node.
   bool check(const tree_nodeRef& t)
   {
      find_res = true;
      t->visit(this);
      return find_res;
   }

 private:
   /// result of the search
   bool find_res;
   /// tree_node_schema expresses the value of the fields of the tree node we are looking for.
   const std::map<TreeVocabularyTokenTypes_TokenEnum, std::string>& tree_node_schema;
};

#endif
