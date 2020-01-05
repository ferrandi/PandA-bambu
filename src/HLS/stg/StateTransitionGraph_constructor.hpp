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
 * @file StateTransitionGraph_constructor.hpp
 * @brief File contanining the structures necessary to manage a graph that will represent a state transition graph
 *
 * This file contains the necessary data structures used to represent a graph used to represent a state transition graph
 *
 * @author Christian Pilato <pilato@elet.polimi.it>
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *
 */

#ifndef STATETRANSITIONGRAPHCONSTR_HPP
#define STATETRANSITIONGRAPHCONSTR_HPP

#include "graph.hpp"

#include "refcount.hpp"

CONSTREF_FORWARD_DECL(HLS_manager);
CONSTREF_FORWARD_DECL(OpGraph);
REF_FORWARD_DECL(StateTransitionGraph);
REF_FORWARD_DECL(StateTransitionGraphsCollection);
enum transition_type : int;

class StateTransitionGraph_constructor
{
 private:
   /// Index of the next state to be created
   unsigned int state_index;

   /// The bulk state transition graph
   const StateTransitionGraphsCollectionRef state_transition_graphs_collection;

   /// The complete state transition graph
   const StateTransitionGraphRef state_transition_graph;

   /// The HLSMgr
   const Wrefcount<const HLS_manager> HLSMgr;

   unsigned int funId;

 public:
   /**
    * Constructor of the class. It creates a new empty graph and it sets reference to hls class
    * @param state_transition_graphs_collection is the graph to be manipulated
    * @param HLS is the HLS data structure
    */
   StateTransitionGraph_constructor(const StateTransitionGraphsCollectionRef state_transition_graphs_collection, const HLS_managerConstRef HLSMgr, unsigned int funId);

   /**
    * Adds a new state managing the operations given as parameters
    * @param exec_op is a list of operation vertices that will be executed by the created state
    * @param start_op is a list of operation vertices that will start to be executed by the created state
    * @param end_op is a list of operation vertices that will end in the created state
    * @return the created vertex
    */
   vertex create_state(const std::list<vertex>& exec_op, const std::list<vertex>& start_op, const std::list<vertex>& end_op, const CustomOrderedSet<unsigned int>& BB_ids);

   /**
    *  create the STG entry vertex
    */
   void create_entry_state();
   /**
    *  create the STG exit vertex
    */
   void create_exit_state();

   /**
    * Creates a connection between two vertices into the graph. You can also specify the edge kind. The method checks
    * if the two vertices are stored into the graph, if one of them (or both) isn't, an exception is thrown.
    * @param src is the source vertex (as returned by create_state method)
    * @param tgt is the target vertex (as returned by create_state method)
    * @param type is the edge type that it's going to be created (normal, direct edge is default)
    * @param exec_op is a list of operation vertices that will be executed by the transition
    * @param end_op is a list of operation vertices that will end in the transition
    * @return the descriptor of edge here created
    */
   EdgeDescriptor connect_state(const vertex& src, const vertex& tgt, int type);

   /**
    * Changes the control condition associated to an edge. If no condition is given, previous one is erased and
    * edge becomes without a true or false control dependence.
    * @param e is the FSM edge
    * @param t is the condition type
    * @param ops is the vertex involved by this condition
    */
   void set_condition(const EdgeDescriptor& e, transition_type t, vertex ops);

   void set_unbounded_condition(const EdgeDescriptor& e, transition_type t, const CustomOrderedSet<vertex>& ops, vertex ref_state);

   /**
    * @brief function setting the condition on edge derived from switch statements
    * @param e is the FSM edge
    * @param op is the controlling operations
    * @param labels are the the switch guards/labels associated with the edge
    * @param has_default is true when with the edge is associated the default guard/label
    */
   void set_switch_condition(const EdgeDescriptor& e, vertex op, const CustomOrderedSet<unsigned>& labels, bool has_default);

   /**
    * @brief copy condition from one edge to another
    * @param dest is the destination edge of the fsm
    * @param source is the source edge of the fsm
    */
   void copy_condition(const EdgeDescriptor& dest, const EdgeDescriptor& source);

   /** Removes the specified edge from the graph.
    * If the graph does not contain the specified edge, this method throws an exception. */
   void delete_edge(const vertex& src, const vertex& tgt);

   /** Removes the specified state from the graph.
    * If the graph does not contain a vertex representing that state, this method throws an exception. */
   void delete_state(const vertex& src);
};

/// refcount definition to StateTransitionGraph_constructor class
typedef refcount<StateTransitionGraph_constructor> StateTransitionGraph_constructorRef;

#endif
