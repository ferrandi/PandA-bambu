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
 * @file loops.hpp
 * @brief interface of loops finding algorithm
 *
 * Loops are detection described in
 * in Vugranam C. Sreedhar, Guang R. Gao, Yong-Fong Lee: Identifying Loops Using DJ Graphs. ACM Trans. Program. Lang. Syst. 18(6): 649-658 (1996)
 *
 * @author Marco Garatti <m.garatti@gmail.com>
 * @author Marco Lattuada <marco.lattuada@polimi.it>
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *
 */

#ifndef LOOPS_HPP
#define LOOPS_HPP

/// Autoheader include
#include "config_HAVE_HOST_PROFILING_BUILT.hpp"

#include "custom_map.hpp" // for unordered_map
#include "custom_set.hpp" // for unordered_set
#include "graph.hpp"
#include "refcount.hpp"

#include <cstddef> // for size_t
#include <list>    // for list
#include <string>  // for string
#include <utility> // for pair
#include <vector>  // for vector

/**
 * @name forward declarations
 */
//@{
REF_FORWARD_DECL(BBGraph);
REF_FORWARD_DECL(FunctionBehavior);
CONSTREF_FORWARD_DECL(Loop);
REF_FORWARD_DECL(Loop);
CONSTREF_FORWARD_DECL(Parameter);
CONSTREF_FORWARD_DECL(ProfilingInformation);
//@}

// This class represents a loop forest for a given method.
// The loop forest is built by Tarjan algorithm and each loop
// is represented by a loop instance (see loop.h).
//
// The class allows a client to iterate on the loops in a method
// and provides a map from basic block to loop to know if a block
// belongs to a loop.
//
// If necessary it is possible to extend the class to navigate the
// hierarchy in an ordered way.
//
class Loops
{
 private:
   /// The function behavior
   const FunctionBehaviorRef FB;

   /// class containing all the parameters
   const ParameterConstRef Param;

   /// Debug level
   int debug_level;

   /// Maps between basic block and loop to which it belongs
   CustomUnorderedMap<vertex, LoopRef> block_to_loop;

   /// List of found loops
   std::list<LoopRef> modifiable_loops_list;
   std::list<LoopConstRef> const_loops_list;

   typedef std::pair<vertex, vertex> vertex_pair;

   bool is_edge_in_list(CustomUnorderedSet<vertex_pair>& l, vertex source, vertex target);

   Loops() = delete;

   /**
    * Computes the loops of the control flow graph
    */
   void DetectLoops();

   /**
    * Reducible loop construction
    * @param djg is the DJ graph
    * @param visited is the set of vertex visited
    * @param loop is the current loop
    * @param node is current vertex
    * @param header is the entry of the reducible loop
    */
   void DetectReducibleLoop(const BBGraphRef djg, CustomOrderedSet<vertex>& visited, LoopRef loop, vertex node, vertex header);

   void DetectIrreducibleLoop(const BBGraphRef djg, unsigned int min_level, unsigned int max_level, std::vector<std::list<vertex>>& level_vertices_rel);

   void tarjan_scc(const BBGraphRef djg, vertex v, CustomUnorderedMap<vertex, unsigned int>& dfs_order, CustomUnorderedMap<vertex, unsigned int>& lowlink, std::list<vertex>& s, CustomOrderedSet<vertex>& u, unsigned int& max_dfs);

   bool stack_contains(std::list<vertex> stack, vertex v);

   /**
    * Creates Loop zero data structure
    */
   void BuildZeroLoop();

   /**
    * Sets depth for each loop in the forest starting from loop
    * @param loop is the root of the loop forest
    */
   void computeDepth(const LoopConstRef loop);

 public:
   /**
    * The constructor builds the loop forest for the given method
    * @param FB is the function behavior of the control flow graph to be analyzed
    * @param parameter is the set of input parameters
    */
   Loops(const FunctionBehaviorRef FB, const ParameterConstRef parameter);

   /**
    * Returns the number of loops
    */
   size_t NumLoops() const;

   /**
    * Returns the list of loops (const)
    * @param return the const list of loops
    */
   const std::list<LoopConstRef>& GetList() const;

   /**
    * Return the list of loops
    * @param return the const list of loops
    */
   const std::list<LoopRef>& GetModifiableList() const;

   /**
    * Returns a loop given the id
    * @param id is the id of the loop
    */
   const LoopConstRef CGetLoop(unsigned int id) const;

   /**
    * Returns a loop given the id
    * @param id is the id of the loop
    */
   const LoopRef GetLoop(unsigned int id);

   /**
    * Write dot files representing the loop forest
    * @param file_name is the file name to be produced
    * @param profiling_information is the profiling information used to print data about loop iterations
    */
   void WriteDot(const std::string& file_name
#if HAVE_HOST_PROFILING_BUILT
                 ,
                 const ProfilingInformationConstRef profiling_information = ProfilingInformationConstRef()
#endif
   ) const;
};

#endif // LOOPS_HPP
