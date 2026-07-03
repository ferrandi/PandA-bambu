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
 * @file ir_node_finder.hpp
 * @brief IR node finder. This class exploiting the visitor design pattern finds an IR node in an ir_manager.
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 *
 */
#ifndef IR_NODE_FINDER_HPP
#define IR_NODE_FINDER_HPP

#include "custom_map.hpp"
#include "ir_node.hpp"
#include "ir_node_mask.hpp"
#include "refcount.hpp"
#include "token_interface.hpp"

#include <boost/preprocessor/facilities/empty.hpp>
#include <boost/preprocessor/seq/for_each.hpp>

#include <string>

REF_FORWARD_DECL(ir_node_finder);

struct ir_node_finder : public ir_node_mask
{
   /// default constructor
   explicit ir_node_finder(const std::map<IRVocabularyTokenTypes_TokenEnum, std::string>& _ir_node_schema)
       : find_res(true), ir_node_schema(_ir_node_schema)
   {
   }
   /// ir_node visitors
   BOOST_PP_SEQ_FOR_EACH(OPERATOR_MACRO_DECL, BOOST_PP_EMPTY, OBJ_SPECIALIZED_SEQ)
   BOOST_PP_SEQ_FOR_EACH(OPERATOR_MACRO, BOOST_PP_EMPTY, OBJ_NOT_SPECIALIZED_SEQ)

   /// Return true in case the IR node is compatible with the ir_node_schema. Usually called by
   /// ir_manager::create_ir_node.
   bool check(const ir_nodeRef& t)
   {
      find_res = true;
      t->visit(this);
      return find_res;
   }

 private:
   /// result of the search
   bool find_res;
   /// ir_node_schema expresses the value of the fields of the IR node we are looking for.
   const std::map<IRVocabularyTokenTypes_TokenEnum, std::string>& ir_node_schema;
};

#endif
