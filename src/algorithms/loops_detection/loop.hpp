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
 * @file loop.hpp
 * @brief interface of a loop
 *
 * A loop is a strongly connected component in a CFG. We use Tarjan algorithm
 * to build a loop forest (see loops.h).
 * Loop represents a single loop. A loop is composed by a set of nodes and arcs
 * which belong to the CFG. Some of the nodes in a loop are special and have
 * been given a name. Some nodes outside the loop are also somehow related to
 * loops and have a special name.
 * Here we introduce the naming conventions we will be using.
 *
 *               -----------------------
 *               |      PreHeader      |
 *               -----------------------
 *                          |
 *          |-------------->|
 *          |               V
 *          |    -----------------------
 *          |    |       Header        |
 *          |    -----------------------
 *          |               |
 *          |               |
 *          |               V
 *          |    -----------------------
 *          |    |    Any CFG allowed  |
 *          |    -----------------------
 *          |               |
 *          |               |
 *          |               V
 *          |    -----------------------
 *          |    |        Exit         |
 *          |    -----------------------
 *          |               |
 *          ----------------|
 *                          V
 *               -----------------------
 *               |     Landing pad     |
 *               -----------------------
 *
 * This example shows an example of a loop, but it is not the most general case.
 * The example has the following properties:
 *
 * 1) it is bottom tested (the exit condition is evaulated at the end of the loop,
 *    which means the loop is do-while)
 * 2) it is reducible (single entry point)
 * 3) has a single pre header (for reducible loops it is always possible to change the
 *    CFG to meet this condition)
 * 4) has a single exit
 * 5) has a single landing pad
 * 6) has a single back edge (for a definition of back edge see for example Tarjan)
 *
 * In general a loop can be much more complicated, having multiple entries (in C this can
 * be achieved using gotos), mutiple exits, landing pads and back edges.
 *
 * In our Loop representation a loop has a header (and only one!) if and only if the loop is
 * reducible. If the loop is not reducible then there's no header info. Exits are kept in a
 * list and landing pads as well. An interesting case is when we have N landing pads and N-1 landing
 * pads have a single successor being the Nth landing pad. This case is generate by break statements
 * in C/C++. For this reason a Loop has a special property to signal this case.
 *
 * @author Marco Garatti <m.garatti@gmail.com>
 * @author Marco Lattuada <lattuada@elet.polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *
 */

#ifndef LOOP_HPP
#define LOOP_HPP

#include "custom_map.hpp"
#include "custom_set.hpp"
#include "graph.hpp"    // for vertex
#include "refcount.hpp" // for REF_FORWARD_DECL
#include <cstddef>      // for size_t
#include <list>         // for list, list<>::co...
#include <utility>      // for pair

/**
 * @name Constants identifying the type of the loops
 */
//@{
/// loop with single exit
#define SINGLE_EXIT_LOOP 1

/// unknown loop
#define UNKNOWN_LOOP 2

/// while or for loop
#define WHILE_LOOP 4

/// for loop
#define FOR_LOOP 8

/// parallelizable for loop
#define DOALL_LOOP 16

/// do while loop
#define DO_WHILE_LOOP 32

/// countable loop
#define COUNTABLE_LOOP 64

/// pipelinable loop
#define PIPELINABLE_LOOP 128

//@}

REF_FORWARD_DECL(BBGraph);
CONSTREF_FORWARD_DECL(Loop);
REF_FORWARD_DECL(Loop);
REF_FORWARD_DECL(Loops);
CONSTREF_FORWARD_DECL(OpGraph);
class OpVertexSet;
REF_FORWARD_DECL(tree_node);

class Loop
{
 private:
   /// Friend definition of loop
   friend class Loops;

   /// compute landing pad exits
   void ComputeLandingPadExits();

   /// The basic block control flow graph
   const BBGraphRef g;

   /// tells if there aren't loop nested in this
   bool is_innermost_loop;

   /// Parent loop
   refcount<Loop> parent_loop;

   /// Child loops
   CustomOrderedSet<LoopConstRef> children;

   /// Blocks which belong to this loop
   CustomUnorderedSet<vertex> blocks;

   /// exit blocks for this loop
   std::list<vertex> exits;

   /// landing_pads for this loop
   CustomUnorderedSet<vertex> landing_pads;

   ///???
   vertex primary_landing_pad_block;

   /// the header of the loop
   vertex header_block;

   /// in case the loop is irreducible the loop has multiple entries while for reducible loops the entry is just one
   CustomOrderedSet<vertex> alternative_entries;

   /// the id of the loop
   unsigned int loop_id;

   /// Map storing the association between an exit basic block and the corresponding landing pads
   std::map<vertex, CustomUnorderedSet<vertex>> exit_landing_association;

   /// used to label irreducible loops
   static unsigned int curr_unused_irreducible_id;

   /// set of vertex pairs describing a spanning tree back edge for the loop
   CustomOrderedSet<std::pair<vertex, vertex>> sp_back_edges;

 public:
   /// Nesting depth of this loop
   unsigned int depth;

   /// loop type
   int loop_type;

   /// loop memory footprint
   long long footprint_size;

   /// loop instruction footprint
   long long instruction_size;

   /// body start basic block (at the moment defined only for SIMPLE_LOOP & WHILE_LOOP)
   vertex body_start;

   /// the main induction variable of countable loop;
   unsigned int main_iv;

   /// The index of the tree node containing the value of the initialization of the induction variable
   unsigned int initialization_tree_node_id;

   /// The index of the gimple tree node containing the initialization of the induction variable; right operand can be different from initialization_tree_node_id because of assignments chain
   unsigned int init_gimple_id;

   /// The node id containing the increment statement
   unsigned int inc_id;

   /// Increment of induction variable
   long long int increment;

   /// Increment of induction variable
   tree_nodeRef increment_tn;

   /// Initial value of induction variable
   long long int lower_bound;

   /// Final value of induction variable
   long long int upper_bound;

   /// Final value of induction variable
   tree_nodeRef upper_bound_tn;

   /// flag for induction variable close interval
   bool close_interval;

   /**
    * Constructor for empty loop (used for irreducible)
    * @param g is the basic block control flow grah
    */
   explicit Loop(const BBGraphRef g);

   /**
    * Constructor for reducible loop
    * @param g is the basic block control flow graph
    * @param header is the header basic block
    */
   Loop(const BBGraphRef g, vertex header);

   /**
    * returns the loop id
    * @return the loop id
    */
   unsigned int GetId() const;

   /**
    * tells if the loop is innermost
    * @return true if the loop is innermost
    */
   bool is_innermost() const;

   /**
    * tells if the loop is reducible
    * @return true if the loop is reducible
    */
   bool IsReducible() const;

   /**
    * returns loop header
    * @return returns loop header
    */
   vertex GetHeader() const;

   /// return the alternative entries of a loop
   CustomOrderedSet<vertex> get_entries() const
   {
      return alternative_entries;
   }

   /// add an entry of the loop
   void add_entry(vertex v)
   {
      alternative_entries.insert(v);
   }

   /**
    * returns the parent loop
    * @return the parent loop if this loop is nested, NULL otherwise
    */
   const LoopRef Parent() const;

   /**
    * adds a block to this loop
    * @param block is the basic block to be added
    */
   void add_block(vertex block);

   /**
    * returns the number of basic blocks belonging to this loop
    * @return the number of basic blocks
    */
   size_t num_blocks() const;

   /**
    * returns the blocks
    * @return the blocks
    */
   const CustomUnorderedSet<vertex>& get_blocks() const;

   // Info on the exit blocks
   size_t num_exits() const;
   std::list<vertex>::const_iterator exit_block_iter_begin() const;
   std::list<vertex>::const_iterator exit_block_iter_end() const;

   // Info on the landing pads
   size_t num_landing_pads() const;

   /**
    * Return the landing pads of the loops
    * @param the basic block landing pads of the loop
    */
   const CustomUnorderedSet<vertex> GetLandingPadBlocks() const;

   // Return the primary landing pad if it exists, NULL otherwise
   // The primary landing pad is the one that is the unique successor
   // of all the other LPs
   vertex primary_landing_pad() const;

   /**
    * Sets parent for this loop
    * @param parent is the parent loop
    */
   void SetParent(LoopRef parent);

   /**
    * Adds a child loop
    * @param child is the child loop
    */
   void AddChild(LoopRef child);

   /**
    * Returns the children of this loop in the loop forest
    */
   const CustomOrderedSet<LoopConstRef>& GetChildren() const;

   /**
    * Returns the basic blocks which belong to this loop and to loop nested in this loop
    * @param ret is the returned set of basic block of this loop and of its children
    */
   void get_recursively_bb(CustomUnorderedSet<vertex>& ret) const;

   /**
    * Returns the operation which belongs to this loop or to a nested loop
    * @param op_graph is the operation graph
    * @return the contained operations
    */
   OpVertexSet GetRecursivelyOps(const OpGraphConstRef op_graph) const;

   /**
    * Returns the map exit_landing_association
    * @return the map exit_landing_association
    */
   const std::map<vertex, CustomUnorderedSet<vertex>>& get_exit_landing_association() const;

   /// add a spanning tree back edge
   void add_sp_back_edge(vertex vs, vertex vd)
   {
      std::pair<vertex, vertex> vpair(vs, vd);
      sp_back_edges.insert(vpair);
   }

   /// check if a pair of vertex is a spanning tree back edge for the loop
   bool is_sp_back_edge(vertex vs, vertex vd) const
   {
      std::pair<vertex, vertex> vpair(vs, vd);
      return sp_back_edges.find(vpair) != sp_back_edges.end();
   }

   /// return the list of spanning tree back edges
   CustomOrderedSet<std::pair<vertex, vertex>> get_sp_back_edges() const
   {
      return sp_back_edges;
   }

   /// Definition of friend class add_loop_nop
   friend class add_loop_nop;
};
/// refcount definition of the class
typedef refcount<Loop> LoopRef;

#endif // LOOP_HPP
