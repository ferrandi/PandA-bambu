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
 * @file liveness.hpp
 * @brief Class specification to contain liveness information
 *
 * This class contains the information about variables incoming and outcomin each state.
 *
 * @author Christian Pilato <pilato@elet.polimi.it>
 * @author Francesca Malcotti <francy_malco@virgilio.it>
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 *
 */

#ifndef LIVENESS_HPP
#define LIVENESS_HPP

/// Autoheader include
#include "config_HAVE_EXPERIMENTAL.hpp"

/// graph include
#include "graph.hpp"

/// STD include
#include <list>
#include <string>

#include "custom_map.hpp"
#include "custom_set.hpp"

/// utility include
#include "refcount.hpp"

/**
 * @name Forward declarations.
 */
//@{
REF_FORWARD_DECL(hls);
REF_FORWARD_DECL(HLS_manager);
CONSTREF_FORWARD_DECL(Parameter);
CONSTREF_FORWARD_DECL(BBGraph);
REF_FORWARD_DECL(tree_manager);
//@}

class liveness
{
 private:
   /// The tree manager
   const tree_managerRef TreeM;

   /// class containing all the parameters
   const ParameterConstRef Param;

   /// This is the map from each vertex to the set of variables live at the input of vertex.
   std::map<vertex, CustomOrderedSet<unsigned int>> live_in;

   /// This is the map from each vertex to the set of variables live at the output of vertex.
   std::map<vertex, CustomOrderedSet<unsigned int>> live_out;

   /// null vertex string
   const std::string null_vertex_string;

   /// used to return a reference to an empty set
   const CustomOrderedSet<unsigned int> empty_set;

   /// vertex over which the live in/out is computed
   std::list<vertex> support_set;

   /// store which operation defines the variable
   std::map<unsigned int, vertex> var_op_definition;

   /// store where an operation is terminating its execution
   std::map<vertex, CustomOrderedSet<vertex>> ending_operations;

   /// store where an operation run and need its input
   std::map<vertex, CustomOrderedSet<vertex>> running_operations;

   /// store where a variable comes from given a support state and an operation
   std::map<vertex, std::map<vertex, std::map<unsigned int, CustomOrderedSet<vertex>>>> state_in_definitions;

   /// store along which transitions the variable has to be stored
   std::map<vertex, std::map<vertex, std::map<unsigned int, CustomOrderedSet<vertex>>>> state_out_definitions;

   /// store the name of each state
   std::map<vertex, std::string> names;

#if HAVE_EXPERIMENTAL
   std::map<vertex, CustomOrderedSet<vertex>> in_conflict_ops;
   std::map<vertex, CustomOrderedSet<vertex>> compatible_ops;
#endif
   hlsRef HLS;
   const HLS_managerRef HLSMgr;

   CustomOrderedSet<vertex> dummy_states;

 public:
   /**
    * Constructor
    * @param HLSMgr is the HLS manager
    * @param Param is the refcount to the class containing all the parameters
    */
   liveness(const HLS_managerRef HLSMgr, const ParameterConstRef Param);

   /**
    * Destructor
    */
   ~liveness();

   /**
    * Store a variable alive at the input of the given vertex
    * @param v is the vertex
    * @param the identifier of the variable
    */
   void set_live_in(const vertex& v, unsigned int var);

   /**
    * Store a set of variables alive at the input of the given vertex
    * @param v is the vertex
    * @param live_set the set of ids of live variables
    */
   void set_live_in(const vertex& v, const CustomOrderedSet<unsigned int>& live_set);

   /**
    * Store the variables alive at the input of the given vertex
    * @param v is the vertex
    * @param first is the first iterator of a set to be merged in the live in
    * @param last is the last iterator of a set to be merged in the live in
    */
   void set_live_in(const vertex& v, const CustomOrderedSet<unsigned int>::const_iterator first, const CustomOrderedSet<unsigned int>::const_iterator last);

   /**
    * erase a variable from the live in
    * @param v is the vertex
    * @param var is the variable
    */
   void erase_el_live_in(const vertex& v, unsigned int var);

   /**
    * Store the variables alive at the output of the given vertex
    * @param v is the vertex
    * @param vars is a set containing the identifiers of the variables
    */
   void set_live_out(const vertex& v, const CustomOrderedSet<unsigned int>& vars);

   /**
    * Store a variable alive at the output of the given vertex
    * @param v is the vertex
    * @param the identifier of the variable
    */
   void set_live_out(const vertex& v, unsigned int var);

   /**
    * Store the variables alive at the output of the given vertex
    * @param v is the vertex
    * @param first is the first iterator of a set to be merged in the live out
    * @param last is the last iterator of a set to be merged in the live out
    */
   void set_live_out(const vertex& v, const CustomOrderedSet<unsigned int>::const_iterator first, const CustomOrderedSet<unsigned int>::const_iterator last);

   /**
    * erase a variable from the live out
    * @param v is the vertex
    * @param var is the variable
    */
   void erase_el_live_out(const vertex& v, unsigned int var);

   /**
    * Get the set of variables live at the input of a vertex
    * @param v is the vertex
    * @return a set containing the identifiers of the variables
    */
   const CustomOrderedSet<unsigned int>& get_live_in(const vertex& v) const;

   /**
    * Get the set of variables live at the output of a vertex
    * @param v is the vertex
    * @return a set containing the identifiers of the variables
    */
   const CustomOrderedSet<unsigned int>& get_live_out(const vertex& v) const;

   /// map a chained vertex with one of the starting operation
   std::map<vertex, vertex> start_op;

   /**
    * return the support set of the live in/out
    */
   const std::list<vertex>& get_support() const
   {
      return support_set;
   }

   /**
    * return which operation defines the variable
    * @param var is the variable
    */
   vertex get_op_where_defined(unsigned int var) const;

   /**
    * return true in case there exist an operation defining it
    */
   bool has_op_where_defined(unsigned int var) const;

   /**
    * add a definition vertex for a variable
    * @param var is the variable defined
    * @param v is the operation defining the variable
    */
   void add_op_definition(unsigned int var, vertex v)
   {
      var_op_definition[var] = v;
   }

   /**
    * return true in case the variable var for a given opoeration v has been defined
    * @param var is the variable
    */
   bool is_defined(unsigned int var) const;

   /**
    * given a variable and a state it returns the set of states from which the variable may come
    * @param state is the state where the variable is used
    * @param op is the operation that uses the variable
    * @param var is the variable
    */
   const CustomOrderedSet<vertex>& get_state_in(vertex state, vertex op, unsigned int var) const;

   /**
    * return true in case the variable for a given op and a given state has a state in
    * @param state is the state where the variable is used
    * @param op is the operation that uses the variable
    * @param var is the variable
    */
   bool has_state_in(vertex state, vertex op, unsigned int var) const;

   /**
    * put into relation for a given variable used in a state the state from which the variable comes
    * @param var is the variable
    * @param state is the state where var is used
    * @param state_in is the state from which the variable comes
    */
   void add_state_in_for_var(unsigned int var, vertex op, vertex state, vertex state_in);

   /**
    * given a variable and a state it returns the set of destination states where the variable may be used
    * @param state is the state where the variable is defined
    * @param op is the operation that defines the variable
    * @param var is the variable
    */
   const CustomOrderedSet<vertex>& get_state_out(vertex state, vertex op, unsigned int var) const;

   /**
    * return true in case the variable for a given op and a given state has a state out
    * @param state is the state where the variable is defined
    * @param op is the operation that defines the variable
    * @param var is the variable
    */
   bool has_state_out(vertex state, vertex op, unsigned int var) const;

   /**
    * put into relation for a given variable defined in a state the state to which the variable goes
    * @param var is the variable
    * @param state is the state where var is defined
    * @param state_in is the state to which the variable goes
    */
   void add_state_out_for_var(unsigned int var, vertex op, vertex state, vertex state_in);

   /**
    * return in which support vertex the operation is ending
    * @param op is the operation
    */
   const CustomOrderedSet<vertex>& get_state_where_end(vertex op) const;

   /**
    * add an ending state for a given operation
    * @param op is the operation
    * @param v is the support vertex where the variable is defined
    */
   void add_state_for_ending_op(vertex op, vertex v)
   {
      ending_operations[op].insert(v);
   }

   /**
    * return in which support vertex the operation is running
    * @param op is the operation
    */
   const CustomOrderedSet<vertex>& get_state_where_run(vertex op) const;

   /**
    * add a running state for a given operation
    * @param op is the operation
    * @param v is the support vertex where the variable is in execution and need its input
    */
   void add_state_for_running_op(vertex op, vertex v)
   {
      running_operations[op].insert(v);
   }

   /**
    * define the name of a state
    * @param v is state
    * @param name is a string associated with the vertex v
    */
   void add_name(vertex v, const std::string& name)
   {
      names[v] = name;
   }

   /**
    * return the name of the given state
    * @param v is the state
    */
   const std::string& get_name(vertex v) const;

   /**
    * add a support state
    * @param v is the support state
    */
   void add_support_state(vertex v)
   {
      support_set.push_front(v);
   }

   /**
    * states if two operations are in conflict (i.e. may be executed concurrently)
    * @param op1 is an operation node
    * @param op2 is an operation node
    */
   bool are_in_conflict(vertex op1, vertex op2) const;

   // activate conflicts with reachability computation
   void compute_conflicts_with_reachability(hlsRef _HLS)
   {
      HLS = _HLS;
   }

   /**
    * Return the operation from which the computation start.
    * More than one operation can be a starting operation in a chained vertex, we choose one randomly.
    * @param state is the chaining state
    * @return is the associated operation that start the computation in a chaining vertex
    */
   vertex get_start_op(vertex state) const;

   /**
    * Set the starting operation for a specified chained state
    * @param state is the chained state
    * @param op is the operation
    */
   void set_start_op(vertex state, vertex op);

   /**
    * add a state to the set of dummy vertices
    * @param state is the dummy state
    */
   void add_dummy_state(vertex state)
   {
      dummy_states.insert(state);
   }

   /**
    * check if a state is a dummy state
    * @param state is the state checked
    * @return true in case the state is a dummy state, false otherwise
    */
   bool is_a_dummy_state(vertex state)
   {
      return dummy_states.find(state) != dummy_states.end();
   }

   bool non_in_parallel(vertex v1, vertex v2, const BBGraphConstRef cdg) const;
};

// refcount definition for class
typedef refcount<liveness> livenessRef;

#endif
