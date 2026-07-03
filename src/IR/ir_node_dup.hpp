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
 * @file ir_node_dup.hpp
 * @brief IR node duplication class.
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 * @author Michele Fiorito <michele.fiorito@polimi.it>
 *
 */
#ifndef IR_NODE_DUP_HPP
#define IR_NODE_DUP_HPP

#include "custom_map.hpp"
#include "ir_node.hpp"
#include "ir_node_mask.hpp"
#include "refcount.hpp"

#include <boost/preprocessor/facilities/empty.hpp>
#include <boost/preprocessor/seq/for_each.hpp>

REF_FORWARD_DECL(ir_node);
REF_FORWARD_DECL(bloc);
REF_FORWARD_DECL(application_manager);

enum ir_node_dup_mode
{
   DEFAULT = 0,  // Nodes are duplicated
   RENAME = 1,   // SSA variables are renamed during duplication
   FUNCTION = 4, // All nodes including declarations are duplicated (first function_val_node only)
   FULL = 8      // All nodes including declarations are duplicated
};

struct ir_node_dup : public ir_node_mask
{
   /**
    * @brief Construct a new IR node dup object
    *
    * @param _remap is the struture to map old nodes to new one
    * @param _AppM is the application manager instance
    * @param _remap_bbi is the base index to renumber duplicated bloc (default = 0, no remapping)
    * @param _remap_loop_id is the base index to renumber loop ids in blocs (default = 0, no remapping)
    * @param _use_counting set use counting on nodes after duplication (bloc, phi_stmt) (default = false)
    */
   ir_node_dup(CustomUnorderedMapStable<unsigned int, unsigned int>& _remap, const application_managerRef _AppM,
               unsigned int _remap_bbi = 0, unsigned int _remap_loop_id = 0, bool _use_counting = false);

   /// ir_node visitors
   BOOST_PP_SEQ_FOR_EACH(OPERATOR_MACRO_DECL, BOOST_PP_EMPTY, OBJ_SPECIALIZED_SEQ)
   BOOST_PP_SEQ_FOR_EACH(OPERATOR_MACRO, BOOST_PP_EMPTY, OBJ_NOT_SPECIALIZED_SEQ)

   /**
    * Factory method.
    * It duplicates an ir_node when needed according to the source node tn
    * @param tn is the source IR node
    * @param mode required duplication mode
    * @return the node_id of the created object or of tn.
    */
   unsigned int create_ir_node(const ir_nodeRef& tn, int mode = ir_node_dup_mode::DEFAULT);

 private:
   const application_managerRef AppM;

   const ir_managerRef TM;

   /// enable use counting on duplicated instances (when possible)
   const bool use_counting;

   const int debug_level;

   /// remap old indexes in new indexes
   CustomUnorderedMapStable<unsigned int, unsigned int>& remap;

   /// basic block indexes remap base
   unsigned int remap_bbi;

   /// remap old basic block indexes in new indexes
   CustomUnorderedMapStable<unsigned int, unsigned int> remap_bb;

   /// basic block loop ids remap base
   unsigned int remap_loop_id;

   /// remap old basic block loop ids in new ids
   CustomUnorderedMapStable<unsigned int, unsigned int> remap_lid;

   /* Active duplication mode */
   int mode;

   /* Active duplicated node */
   ir_node* curr_ir_node_ptr;

   /* Active duplicated basic block */
   bloc* curr_bloc;

   /* Active source node */
   ir_nodeRef source_tn;

   /* Active source basic block */
   blocRef source_bloc;

   unsigned int get_bbi(unsigned int old_bb);

   unsigned int get_loop_id(unsigned int old_loop_id);
};

#endif
