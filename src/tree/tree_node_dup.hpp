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
 *              Copyright (C) 2004-2023 Politecnico di Milano
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
 * @file tree_node_dup.hpp
 * @brief tree node duplication class.
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 * @author Michele Fiorito <michele.fiorito@polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *
 */
#ifndef TREE_NODE_DUP_HPP
#define TREE_NODE_DUP_HPP

#include "custom_map.hpp"
#include "refcount.hpp"
#include "tree_node.hpp"
#include "tree_node_mask.hpp"
#include <boost/preprocessor/facilities/empty.hpp>
#include <boost/preprocessor/seq/for_each.hpp>

/**
 * @name forward declarations
 */
//@{
REF_FORWARD_DECL(tree_node);
REF_FORWARD_DECL(bloc);
REF_FORWARD_DECL(application_manager);
//@}

enum tree_node_dup_mode
{
   DEFAULT = 0,  // Nodes are duplicated
   RENAME = 1,   // SSA variables are renamed during duplication
   FUNCTION = 4, // All nodes including declarations are duplicated (first function_decl only)
   FULL = 8      // All nodes including declarations are duplicated
};

struct tree_node_dup : public tree_node_mask
{
   /**
    * @brief Construct a new tree node dup object
    *
    * @param _remap is the struture to map old nodes to new one
    * @param _AppM is the application manager instance
    * @param _remap_bbi is the base index to renumber duplicated bloc (default = 0, no remapping)
    * @param _remap_loop_id is the base index to renumber loop ids in blocs (default = 0, no remapping)
    * @param _use_counting set use counting on nodes after duplication (bloc, gimple_phi) (default = false)
    */
   tree_node_dup(CustomUnorderedMapStable<unsigned int, unsigned int>& _remap, const application_managerRef _AppM,
                 unsigned int _remap_bbi = 0, unsigned int _remap_loop_id = 0, bool _use_counting = false);

   /// tree_node visitors
   BOOST_PP_SEQ_FOR_EACH(OPERATOR_MACRO_DECL, BOOST_PP_EMPTY, OBJ_SPECIALIZED_SEQ)
   BOOST_PP_SEQ_FOR_EACH(OPERATOR_MACRO, BOOST_PP_EMPTY, OBJ_NOT_SPECIALIZED_SEQ)

   /**
    * Factory method.
    * It duplicates a tree_node when needed according to the source node tn
    * @param tn is the source tree node
    * @param mode required duplication mode
    * @return the node_id of the created object or of tn.
    */
   unsigned int create_tree_node(const tree_nodeRef& tn, int mode = tree_node_dup_mode::DEFAULT);

 private:
   /// remap old indexes in new indexes
   CustomUnorderedMapStable<unsigned int, unsigned int>& remap;
   /// enable use counting on duplicated instances (when possible)
   const bool use_counting;
   /// basic block indexes remap base
   unsigned int remap_bbi;
   /// remap old basic block indexes in new indexes
   CustomUnorderedMapStable<unsigned int, unsigned int> remap_bb;
   /// basic block loop ids remap base
   unsigned int remap_loop_id;
   /// remap old basic block loop ids in new ids
   CustomUnorderedMapStable<unsigned int, unsigned int> remap_lid;
   /// application manager
   const application_managerRef AppM;
   /// tree manager
   const tree_managerRef TM;
   /// current tree node filled according to the source tree_node
   tree_node* curr_tree_node_ptr;
   /// current basic block pointer
   bloc* curr_bloc;
   /// current tree_node source
   tree_nodeRef source_tn;
   /// current basic block source
   blocRef source_bloc;
   /// enables full nodes duplication
   int mode;

   unsigned int get_bbi(unsigned int old_bb);

   unsigned int get_loop_id(unsigned int old_loop_id);
};

#endif
