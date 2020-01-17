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
 * @file tree_node_factory.hpp
 * @brief tree node factory. This class, exploiting the visitor design pattern, add a tree node to the tree_manager.
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *
 */
#ifndef TREE_NODE_FACTORY_HPP
#define TREE_NODE_FACTORY_HPP

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
enum kind : int;
REF_FORWARD_DECL(tree_node_factory);
REF_FORWARD_DECL(tree_manager);
//@}

struct tree_node_factory : public tree_node_mask
{
   /// default constructor
   tree_node_factory(const std::map<TreeVocabularyTokenTypes_TokenEnum, std::string>& _tree_node_schema, tree_manager& _TM) : tree_node_schema(_tree_node_schema), TM(_TM), curr_tree_node_ptr(nullptr)
   {
   }
   /// tree_node visitors
   BOOST_PP_SEQ_FOR_EACH(OPERATOR_MACRO_DECL, BOOST_PP_EMPTY, OBJ_SPECIALIZED_SEQ)
   BOOST_PP_SEQ_FOR_EACH(OPERATOR_MACRO, BOOST_PP_EMPTY, OBJ_NOT_SPECIALIZED_SEQ)

   /**
    * Factory method.
    * It creates a tree_node of type equal tree_node_type by using a relation (tree_node_schema) between tree node fields and their values.
    * @param node_id is the node id of the created object.
    * @param tree_node_type is the type of the node added to the tree_manager
    * For example the following code:
    * std::map<int, std::string> identifier_schema;
    * int identifier_node_id = TM->get_tree_node_n();
    * identifier_schema[TOK(TOK_STRG)]= "my_identifier";
    * tree_node_factory TNF(identifier_schema, TM);
    * TNF.create_tree_node(identifier_node_id, TOK(TOK_IDENTIFIER_NODE));
    * * will add an identifier node to the tree_manager TM.
    */
   void create_tree_node(const unsigned int node_id, enum kind);

 private:
   /// tree_node_schema expresses the value of the fields of the tree node we would like to create.
   const std::map<TreeVocabularyTokenTypes_TokenEnum, std::string>& tree_node_schema;
   /// tree manager
   tree_manager& TM;
   /// current tree node filled according to the tree_node_schema
   tree_node* curr_tree_node_ptr;
};

#endif
