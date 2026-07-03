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
 * @file chaining_information.hpp
 * @brief class containing information about chaining
 * @author Marco Lattuada <marco.lattuada@polimi.it>
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 *
 */
#ifndef CHAINING_INFORMATION_HPP
#define CHAINING_INFORMATION_HPP

#include "op_graph.hpp"
#include "refcount.hpp"

#include <cstddef>

REF_FORWARD_DECL(ChainingSet);
class HLS_manager;
CONSTREF_FORWARD_DECL(HLS_manager);

class ChainingInformation
{
 protected:
   friend class chaining;
   friend class epdg_sched_based_chaining_computation;
   friend class sched_based_chaining_computation;

   /// relation between operation and basic block
   std::map<OpGraph::vertex_descriptor, unsigned int> actual_bb_index_map;

   /// relation between vertices in terms of chaining in input or in output
   ChainingSetRef chaining_relation;

   /// set of vertices chained with something
   CustomOrderedSet<OpGraph::vertex_descriptor> is_chained_with;

   /// The HLS manager
   const Wrefcount<const HLS_manager> HLS_mgr;

   /// The index of the function
   const unsigned int function_id;

 public:
   /**
    * Constructor
    * @param HLS_mgr is the HLS manager
    * @param function_id is the index of the function to which this data structure refers
    */
   ChainingInformation(const HLS_managerConstRef HLS_mgr, const unsigned int function_id);

   /**
    * Initialize the object (i.e., like a constructor, but executed just before exec of a step)
    */
   void Initialize();

   /**
    * Return the representative vertex associated with the chained vertices set in input.
    * It is assumed that chaining define an equivalent relation between vertices.
    * @param op1 is the considered vertex
    * @return the representative vertex
    */
   size_t get_representative_in(OpGraph::vertex_descriptor op1) const;

   /**
    * Return the representative vertex associated with the chained vertices set in output.
    * It is assumed that chaining define an equivalent relation between vertices.
    * @param op1 is the considered vertex
    * @return the representative vertex
    */
   size_t get_representative_out(OpGraph::vertex_descriptor op1) const;

   /**
    * return true in case the vertex is in chaining with something
    * @param v is the operation
    */
   bool is_chained_vertex(OpGraph::vertex_descriptor v) const;

   /**
    * check if two operations are chained in at least one state
    * @param tgt is the target vertex
    * @param src is the source vertex
    */
   bool may_be_chained_ops(OpGraph::vertex_descriptor tgt, OpGraph::vertex_descriptor src) const;

   /**
    * put into relation the vertices whith respect the chained vertices connected with the input
    * @param op1 is the considered vertex
    * @param src is the chained vertex chained in input
    */
   void add_chained_vertices_in(OpGraph::vertex_descriptor op1, OpGraph::vertex_descriptor src);

   /**
    * put into relation the vertices whith respect the chained vertices connected with the output
    * @param op1 is the considered vertex
    * @param tgt is the chained vertex chained in output
    */
   void add_chained_vertices_out(OpGraph::vertex_descriptor op1, OpGraph::vertex_descriptor tgt);
};
#endif
