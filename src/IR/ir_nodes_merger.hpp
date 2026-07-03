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
 * @file ir_nodes_merger.hpp
 * @brief IR node merger classes.
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 *
 */
#ifndef IR_NODES_MERGER_HPP
#define IR_NODES_MERGER_HPP

#include "custom_map.hpp"
#include "custom_set.hpp"
#include "ir_node.hpp"
#include "ir_node_mask.hpp"
#include "refcount.hpp"
#include <boost/preprocessor/facilities/empty.hpp>
#include <boost/preprocessor/seq/for_each.hpp>

REF_FORWARD_DECL(ir_node_reached);
REF_FORWARD_DECL(ir_node);
REF_FORWARD_DECL(bloc);

struct ir_node_reached : public ir_node_mask
{
   /// default constructor
   ir_node_reached(CustomUnorderedMapUnstable<unsigned int, unsigned int>& _remap,
                   OrderedSetStd<unsigned int>& _not_yet_remapped, const ir_managerRef _TM)
       : remap(_remap), TM(_TM), not_yet_remapped(_not_yet_remapped)
   {
   }
   /// ir_node visitors
   BOOST_PP_SEQ_FOR_EACH(OPERATOR_MACRO_DECL, BOOST_PP_EMPTY, OBJ_SPECIALIZED_SEQ)
   BOOST_PP_SEQ_FOR_EACH(OPERATOR_MACRO, BOOST_PP_EMPTY, OBJ_NOT_SPECIALIZED_SEQ)

 private:
   /// remap old indexes in new indexes
   CustomUnorderedMapUnstable<unsigned int, unsigned int>& remap;
   /// IR manager
   const ir_managerRef TM;
   /// ir_node not yet added to the IR Manager
   OrderedSetStd<unsigned int>& not_yet_remapped;
};

struct ir_node_index_factory : public ir_node_mask
{
   /// default constructor
   ir_node_index_factory(CustomUnorderedMapUnstable<unsigned int, unsigned int>& _remap, const ir_managerRef _TM)
       : remap(_remap), TM(_TM), curr_ir_node_ptr(nullptr), curr_bloc(nullptr)
   {
   }
   /// ir_node visitors
   BOOST_PP_SEQ_FOR_EACH(OPERATOR_MACRO_DECL, BOOST_PP_EMPTY, OBJ_SPECIALIZED_SEQ)
   BOOST_PP_SEQ_FOR_EACH(OPERATOR_MACRO, BOOST_PP_EMPTY, OBJ_NOT_SPECIALIZED_SEQ)

   /**
    * Factory method.
    * It creates an ir_node with the same type as tn, remapping all fields according to remap.
    * relation passed to the constructor
    * @param node_id is the node id of the created object.
    * @param tn is the starting IR node
    */
   void create_ir_node(const unsigned int node_id, const ir_nodeRef& tn);

 private:
   /// remap old indexes in new indexes
   CustomUnorderedMapUnstable<unsigned int, unsigned int>& remap;

   /// IR manager
   const ir_managerRef TM;

   /// current IR node filled according to the ir_node_schema
   ir_node* curr_ir_node_ptr;
   /// current basic block pointer
   bloc* curr_bloc;
   /// current ir_node source
   ir_nodeRef source_tn;
   /// current basic block source
   blocRef source_bloc;
};

#endif
