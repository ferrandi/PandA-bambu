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
 * @file state_transition_graph.hpp
 * @brief This file contains the structures needed to manage a graph that will represent the state transition graph
 *
 * This file contains the needed data structures used to represent a graph used to represent a state
 * transition graph associated to the specification. The State Transition Graph (STG) is defined as follows:
 *  - the nodes represent the states to be executed. Each node contains the set of operations to be executed when
 *    the control reaches the related state.
 *  - the edges represent the transition, subjected to the related conditions. In particular, a condition is the
 *    result of the evaluation of a control operation that modifies the control flow.
 *
 * @author Christian Pilato <pilato@elet.polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *
 */
#ifndef STATE_TRANSTION_GRAPH_HPP
#define STATE_TRANSTION_GRAPH_HPP

/// Superclasses includ
#include <edge_info.hpp>
#include <graph_info.hpp>
#include <node_info.hpp>

/// Graph include
#include <graph.hpp>

/// STL includes
#include "custom_set.hpp"

/// Utility includes
#include "refcount.hpp"
#include "string_manipulation.hpp" // for STR

/**
 * @name Forward declarations
 */
//@{
CONSTREF_FORWARD_DECL(BehavioralHelper);
class HLS_manager;
CONSTREF_FORWARD_DECL(OpGraph);
REF_FORWARD_DECL(Parameter);

/**
 * Structure holding information about a node into graph. A node represent an execution state of the machine. It
 * contains operations to be started when execution will reach this state.
 */
struct StateInfo : public NodeInfo
{
   /// Weak Reference to function behavior
   Wrefcount<const HLS_manager> HLSMgr;

   /// function identifier of the function from which the state has been derived
   unsigned int funId;

   /// string used to give a label to the state
   std::string name;

   /// set of operation vertices which require inputs in this state
   std::list<vertex> executing_operations;

   /// set of operation vertices which starts in this state
   std::list<vertex> starting_operations;

   /// set of operation vertices completing their execution in this state
   std::list<vertex> ending_operations;

   /// set of operation vertices already in execution at the beginning of this state
   std::list<vertex> onfly_operations;

   /// set of BB ids associated with the state
   CustomOrderedSet<unsigned int> BB_ids;

   /// flag to check if the state is dummy or not
   bool is_dummy;

   /// flag to check if the state is duplicated or not
   bool is_duplicated;

   /// ID of the bb edge associated with the phi operation
   unsigned int sourceBb;

   /// flag to check if the state is duplicated and the original one
   bool isOriginalState;

   /// pointer to the cloned state
   vertex clonedState;

   /// this state is duplicated but manage of path incoming in the phis
   bool all_paths;

   /// set of ssa vars defined in the state by the moved operations
   CustomOrderedSet<unsigned int> moved_op_def_set;

   /// set of ssa vars used in the state by the moved operations
   CustomOrderedSet<unsigned int> moved_op_use_set;

   /// set of moved operations in execution
   std::list<vertex> moved_exec_op;

   /// set of moved operations ending in this state
   std::list<vertex> moved_ending_op;

   /**
    * Implementation of print method for this kind of node. It simply prints the list of operations contained into this
    * state
    * @param os is the stream reference where you want to print
    */
   void print(std::ostream& os, const int detail_level) const override;

   /**
    * Constructor
    */
   StateInfo() : funId(0), is_dummy(false), is_duplicated(false), sourceBb(0), isOriginalState(false), clonedState(NULL_VERTEX), all_paths(false)
   {
   }
};
/// refcount definition
typedef refcount<StateInfo> StateInfoRef;
typedef refcount<const StateInfo> StateInfoConstRef;

enum transition_type : int
{
   DONTCARE_COND = 0,
   TRUE_COND,
   FALSE_COND,
   CASE_COND,
   ALL_FINISHED,
   NOT_ALL_FINISHED
};

/**
 * Structure holding the information about an edge into the graph. It specialize generic edge_info
 * The property associated with edge is the control condition
 */
class TransitionInfo : public EdgeInfo
{
 private:
   /// pointer to graph storing information about operations
   OpGraphConstRef op_function_graph;

   transition_type t{DONTCARE_COND};
   CustomOrderedSet<vertex> ops;
   bool has_default{false};
   CustomOrderedSet<unsigned> labels;
   vertex ref_state{NULL_VERTEX};
   /// the edge increment computed by Efficient Path Profiling
   size_t epp_increment{0};
   bool epp_incrementValid{false};

 public:
   TransitionInfo(OpGraphConstRef g) : op_function_graph(g)
   {
   }

   friend class StateTransitionGraph_constructor;

   /**
    * Print the information associated with the edge of the graph.
    * @param os is the output stream.
    */
   void print(std::ostream& os, const int detail_level) const;

   bool get_has_default() const
   {
      return has_default;
   }
   transition_type get_type() const
   {
      return t;
   }
   const CustomOrderedSet<vertex>& get_operations() const
   {
      return ops;
   }
   vertex get_operation() const;
   const CustomOrderedSet<unsigned>& get_labels() const
   {
      return labels;
   }
   vertex get_ref_state() const;

   /// Types associated with the edges of the graph.
   enum StateTransitionType
   {
      /// Normal edge
      ST_EDGE_NORMAL = 1 << 0,
      /// Feedback edge
      ST_EDGE_FEEDBACK = 1 << 1,
      /// Artificial edge for computation of Efficient Path Profiling edge increments
      ST_EDGE_EPP = 1 << 2,
   };

   void set_epp_increment(size_t n)
   {
      epp_increment = n;
      epp_incrementValid = true;
   }
   size_t get_epp_increment() const
   {
      return epp_increment;
   }
};
/// refcount about edge info
typedef refcount<TransitionInfo> TransitionInfoRef;
typedef refcount<const TransitionInfo> TransitionInfoConstRef;

/**
 * Structure holding information about the whole graph.
 */
struct StateTransitionGraphInfo : public GraphInfo
{
   /// reference to operation graph
   const OpGraphConstRef op_function_graph;

   /// this vertex represents the entry state of the state transition graph
   vertex entry_node;

   /// this vertex represents the exit state of the state transition graph
   vertex exit_node;

   /// true when the FSM has cycles
   bool is_a_dag;

   /// in case of a dag it is possible to compute the minimum number of cycles
   unsigned int min_cycles;
   /// maximum number of cycles
   unsigned int max_cycles;

   std::map<unsigned int, vertex> state_id_to_vertex;

   std::map<vertex, unsigned int> vertex_to_state_id;

   /**
    * Constructor
    */
   explicit StateTransitionGraphInfo(const OpGraphConstRef op_function_graph);

   friend class StateTransitionGraph_constructor;
};
/// definition of the refcount
typedef refcount<StateTransitionGraphInfo> StateTransitionGraphInfoRef;
typedef refcount<const StateTransitionGraphInfo> StateTransitionGraphInfoConstRef;

/**
 * This structure defines the bulk for the state transition graph
 */
class StateTransitionGraphsCollection : public graphs_collection
{
 public:
   /**
    * Constructor
    * @param state_transition_graph_info is the info to be associated with the graph
    * @param parameters is the set of input parameters
    */
   StateTransitionGraphsCollection(const StateTransitionGraphInfoRef state_transition_graph_info, const ParameterConstRef parameters);

   /**
    * Destructor
    */
   ~StateTransitionGraphsCollection() override;

   /**
    * Add an edge with information associated
    * @param source is the source of the edge
    * @param target is the target of the edge
    * @param selector is the selector to be added
    * @param info is the information to be associated
    * @return the created edge
    */
   inline EdgeDescriptor AddEdge(const vertex source, const vertex target, const int selector, const TransitionInfoRef info)
   {
      THROW_ASSERT(not ExistsEdge(source, target), "Trying to create an already existing edge");
      return InternalAddEdge(source, target, selector, RefcountCast<EdgeInfo>(info));
   }
};
/// refcount definition of the class
typedef refcount<StateTransitionGraphsCollection> StateTransitionGraphsCollectionRef;
typedef refcount<const StateTransitionGraphsCollection> StateTransitionGraphsCollectionConstRef;

/**
 * Class used to describe a state transition graph
 */
struct StateTransitionGraph : public graph
{
 public:
   /**
    * Standard constructor.
    * @param state_transition_graphs_collection is the bulk graph.
    * @param selector is the selector used to filter the bulk graph.
    */
   StateTransitionGraph(const StateTransitionGraphsCollectionRef state_transition_graphs_collection, int selector);

   /**
    * Sub-graph constructor.
    * @param state_transition_graphs_collection is the bulk graph.
    * @param selector is the selector used to filter the bulk graph.
    * @param sub is the set of vertices on which the graph is filtered.
    */
   StateTransitionGraph(const StateTransitionGraphsCollectionRef state_transition_graphs_collection, int selector, CustomUnorderedSet<vertex>& sub);

   /**
    * Destructor
    */
   ~StateTransitionGraph() override;

   /**
    * Return the info associated with a state
    * @param state is the state to be considered
    */
   inline StateInfoRef GetStateInfo(const vertex state)
   {
      return RefcountCast<StateInfo>(graph::GetNodeInfo(state));
   }

   /**
    * Return the info associated with a state
    * @param state is the state to be considered
    */
   inline const StateInfoConstRef CGetStateInfo(const vertex state) const
   {
      return RefcountCast<const StateInfo>(graph::CGetNodeInfo(state));
   }

   /**
    * Return the info associated with a state, given its ID
    * @state_id is the ID of the state to be considered
    */
   inline const StateInfoConstRef CGetStateInfo(unsigned int state_id) const
   {
      return CGetStateInfo(GetVertex(state_id));
   }

   /**
    * Return the state vertex corresponding to the given id
    */
   inline vertex GetVertex(unsigned int state_id) const
   {
      THROW_ASSERT(CGetStateTransitionGraphInfo()->state_id_to_vertex.find(state_id) != CGetStateTransitionGraphInfo()->state_id_to_vertex.end(), "State ID " + STR(state_id) + " was not stored");
      return CGetStateTransitionGraphInfo()->state_id_to_vertex.at(state_id);
   }

   /**
    * Return the info associated with a transition
    * @param transition is the transition to be considered
    */
   inline TransitionInfoRef GetTransitionInfo(const EdgeDescriptor transition)
   {
      return RefcountCast<TransitionInfo>(graph::GetEdgeInfo(transition));
   }

   /**
    * Return the info associated with a transition
    * @param transition is the transition to be considered
    */
   inline const TransitionInfoConstRef CGetTransitionInfo(const EdgeDescriptor transition) const
   {
      return RefcountCast<const TransitionInfo>(graph::CGetEdgeInfo(transition));
   }

   /**
    * Return the info associated with the graph
    * @return the info associated with the graph
    */
   inline StateTransitionGraphInfoRef GetStateTransitionGraphInfo()
   {
      return RefcountCast<StateTransitionGraphInfo>(graph::GetGraphInfo());
   }

   /**
    * Return the info associated with the graph
    * @return the info associated with the graph
    */
   inline const StateTransitionGraphInfoConstRef CGetStateTransitionGraphInfo() const
   {
      return RefcountCast<const StateTransitionGraphInfo>(graph::CGetGraphInfo());
   }

   /**
    * Writes this graph in dot format
    * @param file_name is the file where the graph has to be printed
    * @param detail_level is the detail level of the printed graph
    */
   void WriteDot(const std::string& file_name, const int detail_level = 0) const;
};
/// refcount definition of the class
typedef refcount<StateTransitionGraph> StateTransitionGraphRef;
typedef refcount<const StateTransitionGraph> StateTransitionGraphConstRef;

/**
 * Functor template used to write the content of the nodes to a dotty file.
 */
class StateWriter : public VertexWriter
{
 private:
   /// this is the helper associated to the actual module
   const BehavioralHelperConstRef BH;

   /// reference to operation graph
   const OpGraphConstRef op_function_graph;

   /// vertex that contains entry node
   vertex entry_node;

   /// vertex that contains exit node
   vertex exit_node;

 public:
   /**
    * Constructor. It initialize reference to the graph provided as parameter
    */
   StateWriter(const graph* _stg, const OpGraphConstRef _op_function_graph, int _detail_level);

   /**
    * Functor actually called by the boost library to perform the writing
    */
   void operator()(std::ostream& out, const vertex& v) const override;
};

/**
 * Functor used to write the content of the edges to a dotty file and it is used to write specific edge properties
 * such as condition.
 */
class TransitionWriter : public EdgeWriter
{
 private:
   /// this is the helper associated to the actual module
   const BehavioralHelperConstRef BH;

   /// reference to operation graph
   const OpGraphConstRef op_function_graph;

 public:
   /**
    * Constructor. It initialize reference to the graph provided as parameter
    * @param stg is the graph to be printed
    * @param op_function_graph is the associated operation graph
    */
   TransitionWriter(const graph* stg, const OpGraphConstRef op_function_graph, int _detail_level);

   /**
    * Functor actually called by the boost library to perform the writing
    */
   void operator()(std::ostream& out, const EdgeDescriptor& e) const override;
};
#endif
