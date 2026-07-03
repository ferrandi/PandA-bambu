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
 * @file ir_node_factory.hpp
 * @brief IR node factory. This class, exploiting the visitor design pattern, adds an IR node to the ir_manager.
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 *
 */
#ifndef IR_NODE_FACTORY_HPP
#define IR_NODE_FACTORY_HPP

#include "custom_map.hpp"
#include "ir_node.hpp"
#include "ir_node_mask.hpp"
#include "refcount.hpp"
#include "token_interface.hpp"

#include <boost/preprocessor/facilities/empty.hpp>
#include <boost/preprocessor/seq/for_each.hpp>

#include <string>

enum kind : int;
REF_FORWARD_DECL(ir_node_factory);
REF_FORWARD_DECL(ir_manager);

struct ir_node_factory : public ir_node_mask
{
   /// default constructor
   ir_node_factory(const std::map<IRVocabularyTokenTypes_TokenEnum, std::string>& _ir_node_schema, ir_manager& _TM)
       : ir_node_schema(_ir_node_schema), TM(_TM), curr_ir_node_ptr(nullptr)
   {
   }
   /// ir_node visitors
   BOOST_PP_SEQ_FOR_EACH(OPERATOR_MACRO_DECL, BOOST_PP_EMPTY, OBJ_SPECIALIZED_SEQ)
   BOOST_PP_SEQ_FOR_EACH(OPERATOR_MACRO, BOOST_PP_EMPTY, OBJ_NOT_SPECIALIZED_SEQ)

   /**
    * Factory method.
    * It creates an ir_node of type ir_node_type by using an ir_node_schema mapping
    * IR node fields to their values.
    * @param node_id is the node id of the created object.
    * @param kind is the type of the node added to the ir_manager
    * For example the following code:
    * std::map<int, std::string> identifier_schema;
    * int identifier_node_id = TM->new_ir_node_id();
    * identifier_schema[TOK(TOK_STRG)]= "my_identifier";
    * ir_node_factory TNF(identifier_schema, TM);
    * TNF.create_ir_node(identifier_node_id, TOK(TOK_IDENTIFIER_NODE));
    * * will add an identifier node to the ir_manager TM.
    */
   ir_nodeRef create_ir_node(const unsigned int node_id, enum kind);

 private:
   /// ir_node_schema expresses the value of the fields of the IR node we would like to create.
   const std::map<IRVocabularyTokenTypes_TokenEnum, std::string>& ir_node_schema;
   /// IR manager
   ir_manager& TM;
   /// current IR node filled according to the ir_node_schema
   ir_node* curr_ir_node_ptr;
};

#endif
