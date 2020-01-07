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
 * @file state_transition_graph_manager.hpp
 * @brief This file contains the structures needed to manage a graph that will represent the state transition graph
 *
 * @author Christian Pilato <pilato@elet.polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *
 */
#ifndef STATE_TRANSITION_GRAPH_MANAGER_HPP
#define STATE_TRANSITION_GRAPH_MANAGER_HPP

/// Graph include
#include "graph.hpp"

/// Refcount include
#include "refcount.hpp"

CONSTREF_FORWARD_DECL(FunctionBehavior);
CONSTREF_FORWARD_DECL(hls);
CONSTREF_FORWARD_DECL(HLS_manager);
CONSTREF_FORWARD_DECL(OpGraph);
CONSTREF_FORWARD_DECL(StateTransitionGraph);
REF_FORWARD_DECL(hls);
REF_FORWARD_DECL(StateTransitionGraph);
REF_FORWARD_DECL(StateTransitionGraph_constructor);
REF_FORWARD_DECL(StateTransitionGraphsCollection);
REF_FORWARD_DECL(structural_object);
REF_FORWARD_DECL(generic_obj);
REF_FORWARD_DECL(structural_manager);

/**
 * Class used to manage a graph into finite state machine representation; it contains methods to build the graph,
 * to add nodes, edges... and also to write the dotty representation of the graph. Two different graphs have been stored:
 * the finite state machine and the related acyclic version (without any feedback edges)
 */
class StateTransitionGraphManager
{
 private:
   /// The bulk graph
   const StateTransitionGraphsCollectionRef state_transition_graphs_collection;

   /// The acyclic version of the STG
   const StateTransitionGraphRef ACYCLIC_STG_graph;

   /// The complete version of the STG
   const StateTransitionGraphRef STG_graph;

   /// The acyclic version of stg with additional edges necessary for Efficient Path Profiling
   const StateTransitionGraphRef EPP_STG_graph;

   /// reference to operation graph
   const OpGraphConstRef op_function_graph;

   /// class containing all the parameters
   const ParameterConstRef Param;

   /// verbosity level
   int output_level;

   /// debugging level
   int debug_level;

   // Tells to the get_states method which states you are looking for
   enum StateTypes
   {
      EXECUTING,
      STARTING,
      ENDING
   };

   // helper method to retrieve states
   CustomOrderedSet<vertex> get_states(const vertex& op, StateTypes statetypes) const;

   /// HLS data-structure
   hlsRef HLS;

   /// map between state and multi-unbounded controllers
   std::map<vertex, generic_objRef> multi_unbounded_table;

 public:
   /// reference to the class for building the graph
   const StateTransitionGraph_constructorRef STG_builder;

   /**
    * Constructor of the class. It creates a new empty graph and it sets reference to hls class
    * @param HLS is the HLS data structure
    */
   StateTransitionGraphManager(const HLS_managerConstRef HLSMgr, hlsRef HLS, const ParameterConstRef parameters);

   /**
    * Destructor
    */
   ~StateTransitionGraphManager();

   /* States retrievers */
   CustomOrderedSet<vertex> get_execution_states(const vertex& op) const;
   CustomOrderedSet<vertex> get_ending_states(const vertex& op) const;
   CustomOrderedSet<vertex> get_starting_states(const vertex& op) const;

   /// compute the minimum and maximum number of cycles when possible
   void compute_min_max();

   /**
    * Returns pointer to state transition graph created.
    * @return reference to a graph that contains information about operations to be executed and control edges
    */
   StateTransitionGraphRef GetStg();

   /**
    * Returns pointer to acyclic STG with additional edges for Efficient Path Profiling
    */
   StateTransitionGraphRef GetEPPStg();

   /**
    * Returns pointer to acyclic STG with additional edges for Efficient Path Profiling
    */
   const StateTransitionGraphConstRef CGetEPPStg() const;

   /**
    * Returns pointer to state transition graph created.
    * @return reference to a graph that contains information about operations to be executed and control edges
    */
   const StateTransitionGraphConstRef CGetStg() const;

   /**
    * Returns pointer to state transition graph created.
    * @return reference to a graph that contains information about operations to be executed and control edges
    */
   StateTransitionGraphRef GetAstg();

   /**
    * Returns pointer to state transition graph created.
    * @return reference to a graph that contains information about operations to be executed and control edges
    */
   const StateTransitionGraphConstRef CGetAstg() const;

   void print_statistics() const;

   /**
    * Gets vertex that represents state that contains entry node
    * @return the vertex of state associated to entry node
    */
   vertex get_entry_state() const;

   /**
    * Get the name of a state
    * @param state is the state
    * @return the name of the state
    */
   std::string get_state_name(vertex state) const;

   /**
    * Gets vertex that represents state that contains exit node
    * @return the vertex of state associated to exit node
    */
   vertex get_exit_state() const;

   /**
    * @return the number of states of the FSM
    */
   unsigned int get_number_of_states() const;

   void add_multi_unbounded_obj(vertex s, const CustomOrderedSet<vertex>& ops);

   void specialise_mu(structural_objectRef& mu_mod, generic_objRef mu) const;

   const std::map<vertex, generic_objRef>& get_mu_ctrls() const
   {
      return multi_unbounded_table;
   }

   /**
    * Add components to the datapath required by the FSM
    */
   void add_to_SM(structural_objectRef clock_port, structural_objectRef reset_port);
};
/// refcount definition to allocate the class
typedef refcount<StateTransitionGraphManager> StateTransitionGraphManagerRef;
typedef refcount<const StateTransitionGraphManager> StateTransitionGraphManagerConstRef;

/// state name prefix
#define STATE_NAME_PREFIX "S_"

#endif
