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
 * @file chaining_information.hpp
 * @brief class containing information about chaining
 * @author Marco Lattuada <marco.lattuada@polimi.it>
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 *
 */
#ifndef CHAINING_INFORMATION_HPP
#define CHAINING_INFORMATION_HPP

/// graph include
#include "graph.hpp"

/// STD include
#include <cstddef>

/// utility include
#include "refcount.hpp"

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
   std::map<vertex, unsigned int> actual_bb_index_map;

   /// relation between vertices in terms of chaining in input or in output
   ChainingSetRef chaining_relation;

   /// set of vertices chained with something
   CustomOrderedSet<vertex> is_chained_with;

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
   size_t get_representative_in(vertex op1) const;

   /**
    * Return the representative vertex associated with the chained vertices set in output.
    * It is assumed that chaining define an equivalent relation between vertices.
    * @param op1 is the considered vertex
    * @return the representative vertex
    */
   size_t get_representative_out(vertex op1) const;

   /**
    * return true in case the vertex is in chaining with something
    * @param v is the operation
    */
   bool is_chained_vertex(vertex v) const;

   /**
    * check if two operations are chained in at least one state
    * @param op1 is the first vertex
    * @param op2 is the second vertex
    */
   bool may_be_chained_ops(vertex op1, vertex op2) const;

   /**
    * put into relation the vertices whith respect the chained vertices connected with the input
    * @param op1 is the considered vertex
    * @param src is the chained vertex chained in input
    */
   void add_chained_vertices_in(vertex op1, vertex src);

   /**
    * put into relation the vertices whith respect the chained vertices connected with the output
    * @param op1 is the considered vertex
    * @param tgt is the chained vertex chained in output
    */
   void add_chained_vertices_out(vertex op1, vertex tgt);
};
/// refcount definition of the class
typedef refcount<ChainingInformation> ChainingInformationRef;
#endif
