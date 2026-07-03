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
 * @file conn_binding.hpp
 * @brief Data structure used to store the interconnection binding of datapath elements
 *
 *
 * @author Christian Pilato <pilato@elet.polimi.it>
 *
 */
#ifndef CONN_BINDING_HPP
#define CONN_BINDING_HPP

#include "HLS/fsm/FSMInfo.hpp"
#include "custom_map.hpp"
#include "generic_obj.hpp"
#include "graph.hpp"
#include "refcount.hpp"
#include <iosfwd>
#include <string>

#include "config_HAVE_UNORDERED.hpp"

class GenericObjUnsignedIntSorter;
CONSTREF_FORWARD_DECL(BehavioralHelper);
CONSTREF_FORWARD_DECL(OpGraph);
CONSTREF_FORWARD_DECL(Parameter);
REF_FORWARD_DECL(conn_binding);
REF_FORWARD_DECL(connection_obj);
REF_FORWARD_DECL(generic_obj);
REF_FORWARD_DECL(HLS_manager);
REF_FORWARD_DECL(hls);
REF_FORWARD_DECL(structural_manager);
REF_FORWARD_DECL(structural_object);
REF_FORWARD_DECL(technology_manager);

/// definition of the data transfer (ir_node, precision, from, to, data_transferred, current_op). Note that from/to
/// can represent either chained vertices or FSM states
using data_transfer =
    std::tuple<unsigned int, unsigned int, FSMInfo::state_descriptor, FSMInfo::state_descriptor, gc_vertex_descriptor>;

/**
 * @class conn_binding
 * Class managing the interconnection binding.
 */
class conn_binding
{
 public:
   /// direction port identifier
   using direction_type = enum { IN = 0, OUT };

   /// type of the data-structure
   using type_t = enum { STG = 0 };

   /// connection between two objects (<src, tgt, tgt_port, tgt_port_index>)
   using connection = std::tuple<generic_objRef, generic_objRef, unsigned int, unsigned int>;

   /// definition of the connection implementations
#if HAVE_UNORDERED
   using conn_implementation_map = std::map<connection, connection_objRef>;
#else

   /// Sorter for connection
   struct ConnectionSorter
   {
      /**
       * Compare position of two connections
       * @param x is the first connection
       * @param y is the second connection
       * @return true if index of x is less than y
       */
      bool operator()(const connection& x, const connection& y) const;
   };

   using conn_implementation_map = std::map<connection, connection_objRef, ConnectionSorter>;
#endif

   /// definition of the key to deal with constant parameters
   using const_param = std::tuple<std::string, std::string, bool>;

   /// definition of target of a connection
   struct ConnectionTarget : public std::tuple<generic_objRef, unsigned int, unsigned int>
   {
      /**
       * Constructor
       * @param tgt is the target of the connection
       * @param tgt_port is the target port of the connection
       * @param tgt_port_index is the index of the target port
       */
      ConnectionTarget(generic_objRef tgt, unsigned int tgt_port, unsigned int tgt_port_index);

#if !HAVE_UNORDERED
      /**
       * @param other is the second operand
       * @return this < y
       */
      bool operator<(const ConnectionTarget& other) const;
#endif
   };

   /// definition of sources of a connection
#if HAVE_UNORDERED
   using ConnectionSources = CustomUnorderedMap<generic_objRef, CustomOrderedSet<data_transfer>>;
#else
   using ConnectionSources = std::map<generic_objRef, CustomOrderedSet<data_transfer>, GenericObjSorter>;
#endif

 protected:
   /// The set of input parameters
   const ParameterConstRef parameters;

   /// control the verbosity during the debugging
   int debug_level;

 private:
   /// control the output verbosity
   int output_level;

 private:
   /// reference to the behavioral helper associated with the specification
   const BehavioralHelperConstRef BH;

   /// map between a vertex and the corresponding activation signal
   std::map<gc_vertex_descriptor, std::map<unsigned int, generic_objRef>> activation_ports;

   /// map between input port variable and generic object
   std::map<unsigned int, generic_objRef> input_ports;

   /// map between output port variable and generic object
   std::map<unsigned int, generic_objRef> output_ports;

   /// constant values
   std::map<const_param, generic_objRef> constant_values;

   /// data type converters
   std::map<std::string, structural_objectRef> converters;

   /// map between command input port (operation vertex and command type) and generic object
   std::map<std::pair<gc_vertex_descriptor, unsigned int>, generic_objRef> command_input_ports;

   /// map between output port variable and generic object
   std::map<gc_vertex_descriptor, generic_objRef> command_output_ports;

   /// map between a call operation and the datapath endpoint carrying its predicate value
   std::map<gc_vertex_descriptor, generic_objRef> command_predicates;

   /// selector ports
#if HAVE_UNORDERED
   using Selectors = std::map<std::pair<generic_objRef, unsigned int>, generic_objRef>;
#else
   using Selectors = std::map<std::pair<generic_objRef, unsigned int>, generic_objRef, GenericObjUnsignedIntSorter>;
#endif
   std::map<unsigned int, Selectors> selectors;

   /// set containing all the sparse logic contained into the datapath
#if HAVE_UNORDERED
   CustomUnorderedSet<generic_objRef> sparse_logic;
#else
   CustomOrderedSet<generic_objRef, GenericObjSorter> sparse_logic;
#endif

   /// map between the input of the unit and the corresponding incoming connections.
   /// The key <tgt, tgt_port, tgt_port_index> is the target of the connection, while the value is a set of pairs <src,
   /// variable>
   std::map<ConnectionTarget, ConnectionSources> conn_variables;

   /// map between the connection <src, tgt, tgt_port, tgt_port_index> and the corresponding object
   conn_implementation_map conn_implementation;

   static unsigned unique_id;

   /**
    * Specialize a multiplexer according to the type of the variables crossing it.
    * @param mux is the multiplexer
    * @param bits_tgt is the bitwidth of the target port
    */
   void specialise_mux(const generic_objRef mux, unsigned int bits_tgt) const;

   /**
    * Add the mux-based interconnection
    */
   void mux_connection(const hlsRef HLS, const structural_managerRef SM);

   /**
    * Add sparse logic to the datapath
    */
   void add_sparse_logic_dp(const hlsRef HLS, const structural_managerRef SM, const HLS_managerRef HLSMgr);

   /**
    * Add signals from/to controller
    */
   void add_command_ports(const HLS_managerRef HLSMgr, const hlsRef HLS, const structural_managerRef SM);

   /**
    * Add multiplexers to the structural representation of the datapath
    */
   virtual void mux_allocation(const hlsRef HLS, const structural_managerRef SM, structural_objectRef src,
                               structural_objectRef tgt, connection_objRef conn);

   /**
    * Add a data converter, if needed, between two objects of the structural representation of the datapath
    */
   void add_datapath_connection(const technology_managerRef TM, const structural_managerRef SM,
                                const structural_objectRef src, const structural_objectRef port_tgt,
                                unsigned int conn_type);

   /**
    * check if a port vector has its port bounded to something
    * @param port_i
    * @return true in case all ports are bounded to something, false otherwise.
    */
   bool check_pv_allconnected(structural_objectRef port_i);

 public:
   /**
    * Constructor.
    */
   conn_binding(const BehavioralHelperConstRef BH, const ParameterConstRef parameters);

   virtual ~conn_binding() = default;

   /**
    * Bind variable to a port object
    * @param var is the variable to be associated to the port
    * @param dir is the direction to be associated to the port
    */
   generic_objRef bind_port(unsigned int var, direction_type dir);

   /**
    * Bind vertex to a command port object
    * @param ver is the vertex to be associated to the port
    * @param dir is the direction to be associated to the port
    * @param mode is command mode (as defined into commandport_obj::command_type)
    * @param g is graph where vertex ver is stored
    */
   generic_objRef bind_command_port(gc_vertex_descriptor ver, direction_type dir, unsigned int mode, const OpGraph& g);

   generic_objRef bind_selector_port(direction_type dir, unsigned int mode, gc_vertex_descriptor cond,
                                     const OpGraph& data);

   generic_objRef bind_selector_port(direction_type dir, unsigned int mode, const generic_objRef elem, unsigned int op);

   void bind_command_predicate(gc_vertex_descriptor ver, const generic_objRef& predicate_port)
   {
      command_predicates[ver] = predicate_port;
   }

   generic_objRef get_command_predicate(gc_vertex_descriptor ver) const
   {
      const auto it = command_predicates.find(ver);
      return it == command_predicates.end() ? generic_objRef() : it->second;
   }

   /**
    * Returns reference to generic object associated to a given variable, for a specific port direction
    * @param var is variable associated with the port
    * @param dir is port direction
    * @return reference to the generic object of the port associated to given variable, into given direction
    */
   generic_objRef get_port(unsigned int var, direction_type dir);

   /**
    * Function that prints the interconnection binding
    */
   virtual void print() const;

   /**
    * Adds a data transfer between two objects
    * @param op1 is reference to first object
    * @param op2 is reference to second object
    * @param operand is i-th operand for second object, where first one is connected
    * @param port_index is i-th port associated with the operand (different from 0 when multi-channels components are
    * considered)
    * @param data is the data to be transferred
    */
   void add_data_transfer(const generic_objRef op1, const generic_objRef op2, unsigned int operand,
                          unsigned int port_index, data_transfer data);

   /**
    * Creates a connection between two objects
    * @param op1 is reference to first object
    * @param op2 is reference to second object
    * @param operand is i-th operand for second object, where first one is connected
    * @param port_index is i-th port associated with the operand (different from 0 when multi-channels components are
    * considered)
    * @param conn is the reference to the implemented connection
    */
   void AddConnectionCB(const generic_objRef op1, const generic_objRef op2, unsigned int operand,
                        unsigned int port_index, connection_objRef conn);

   /**
    * Returns the map containing all the data transfers
    */
   const std::map<ConnectionTarget, ConnectionSources>& get_data_transfers() const;

   /**
    * Returns the number of bit-level multiplexers
    */
   unsigned long long determine_bit_level_mux() const;

   const std::map<unsigned int, Selectors>& GetSelectors() const
   {
      return selectors;
   }

   void add_sparse_logic(const generic_objRef so)
   {
      sparse_logic.insert(so);
   }

   /**
    * Add the interconnection to the structural representation of the datapath
    */
   virtual void add_to_SM(const HLS_managerRef HLSMgr, const hlsRef HLS, const structural_managerRef SM);

   generic_objRef get_constant_obj(const std::string& value, const std::string& param, unsigned int precision,
                                   bool signedP);

   const std::map<const_param, generic_objRef>& get_constant_objs() const;

   /**
    * @brief factory method to create the right conn_binding depending on the flow
    * @param _HLSMgr
    * @param _HLS
    * @param _BH
    * @param _parameters
    * @return
    */
   static conn_bindingRef create_conn_binding(const HLS_managerRef _HLSMgr, const hlsRef _HLS,
                                              const BehavioralHelperConstRef _BH, const ParameterConstRef _parameters);

   void cleanInternals();
};
/// Refcount definition of the class
using conn_bindingRef = refcount<conn_binding>;

#endif
