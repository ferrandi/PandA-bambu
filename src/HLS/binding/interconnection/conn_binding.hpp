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
 * @file conn_binding.hpp
 * @brief Data structure used to store the interconnection binding of datapath elements
 *
 *
 * @author Christian Pilato <pilato@elet.polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *
 */
#ifndef CONN_BINDING_HPP
#define CONN_BINDING_HPP

/// Autoheader include
#include "config_HAVE_UNORDERED.hpp"

#include <iosfwd>
#include <string>

#include "refcount.hpp"
REF_FORWARD_DECL(HLS_manager);
REF_FORWARD_DECL(generic_obj);
REF_FORWARD_DECL(connection_obj);
CONSTREF_FORWARD_DECL(BehavioralHelper);
REF_FORWARD_DECL(structural_manager);
REF_FORWARD_DECL(structural_object);
REF_FORWARD_DECL(technology_manager);
REF_FORWARD_DECL(hls);
REF_FORWARD_DECL(conn_binding);
CONSTREF_FORWARD_DECL(OpGraph);
class GenericObjUnsignedIntSorter;

#include "custom_map.hpp"
#include "generic_obj.hpp"
#include "graph.hpp"

/// definition of the data transfer (tree_node, precision, from, to, data_transferred, current_op). Note that from/to can represent either chained vertices or STG states
typedef std::tuple<unsigned int, unsigned int, vertex, vertex, vertex> data_transfer;

/**
 * @class conn_binding
 * Class managing the interconnection binding.
 */
class conn_binding
{
 public:
   /// direction port identifier
   typedef enum
   {
      IN = 0,
      OUT
   } direction_type;

   /// type of the datastructure
   typedef enum
   {
      STG = 0
   } type_t;

   /// connection between two objects (<src, tgt, tgt_port, tgt_port_index>)
   typedef std::tuple<generic_objRef, generic_objRef, unsigned int, unsigned int> connection;

   /// definition of the connection implementations
#if HAVE_UNORDERED
   typedef std::map<connection, connection_objRef> conn_implementation_map;
#else

   /// Sorter for connection
   struct ConnectionSorter : public std::binary_function<connection, connection, bool>
   {
      /**
       * Compare position of two connections
       * @param x is the first connection
       * @param y is the second connection
       * @return true if index of x is less than y
       */
      bool operator()(const connection& x, const connection& y) const;
   };

   typedef std::map<connection, connection_objRef, ConnectionSorter> conn_implementation_map;
#endif

   /// definition of the key to deal with constant parameters
   typedef std::tuple<std::string, std::string> const_param;

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
   typedef CustomUnorderedMap<generic_objRef, CustomOrderedSet<data_transfer>> ConnectionSources;
#else
   typedef std::map<generic_objRef, CustomOrderedSet<data_transfer>, GenericObjSorter> ConnectionSources;
#endif

 protected:
   /// The set of input parameters
   const ParameterConstRef parameters;

   /// control the verbosity during the debugging
   int debug_level;

   /// control the output verbosity
   int output_level;

   /// reference to the behavioral helper associated with the specification
   const BehavioralHelperConstRef BH;

   /// map between a vertex and the corresponding activation signal
   std::map<vertex, std::map<unsigned int, generic_objRef>> activation_ports;

   /// map between input port variable and generic object
   std::map<unsigned int, generic_objRef> input_ports;

   /// map between output port variable and generic object
   std::map<unsigned int, generic_objRef> output_ports;

   /// constant values
   std::map<const_param, generic_objRef> constant_values;

   /// data type converters
   std::map<std::string, structural_objectRef> converters;

   /// map between command input port (operation vertex and command type) and generic object
   std::map<std::pair<vertex, unsigned int>, generic_objRef> command_input_ports;

   /// map between output port variable and generic object
   std::map<vertex, generic_objRef> command_output_ports;

   /// map between interconnection elements and their number
   std::map<std::pair<std::string, unsigned int>, unsigned int> interconnection_elements;

   /// set of other command ports
   CustomOrderedSet<generic_objRef> command_ports;

   /// selector ports
#if HAVE_UNORDERED
   typedef std::map<std::pair<generic_objRef, unsigned int>, generic_objRef> Selectors;
#else
   typedef std::map<std::pair<generic_objRef, unsigned int>, generic_objRef, GenericObjUnsignedIntSorter> Selectors;
#endif
   std::map<unsigned int, Selectors> selectors;

   /// map between a resource name and the corresponding number of instances inside the datapath
   std::map<std::string, unsigned int> component_num;

   /// set containing all the sparse logic contained into the datapath
#if HAVE_UNORDERED
   CustomUnorderedSet<generic_objRef> sparse_logic;
#else
   CustomOrderedSet<generic_objRef, GenericObjSorter> sparse_logic;
#endif

   /// map between the input of the unit and the corresponding incoming connections.
   /// The key <tgt, tgt_port, tgt_port_index> is the target of the connection, while the value is a set of pairs <src, variable>
   std::map<ConnectionTarget, ConnectionSources> conn_variables;

   /// map between the connection <src, tgt, tgt_port, tgt_port_index> and the corresponding object
   conn_implementation_map conn_implementation;

   static unsigned unique_id;

   /**
    * Specialise a multiplexer according to the type of the variables crossing it.
    * @param mux is the multiplexer
    * @param bits_tgt is the bitwidth of the target port
    */
   void specialise_mux(const generic_objRef mux, unsigned int bits_tgt) const;

   /**
    * Specialise a multiplexer according to the type of the variables crossing it.
    * @param mux is the multiplexer
    */
   unsigned int get_mux_size(const generic_objRef mux) const;

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
   virtual void mux_allocation(const hlsRef HLS, const structural_managerRef SM, structural_objectRef src, structural_objectRef tgt, connection_objRef conn);

   /**
    * Add a data converter, if needed, between two objects of the structural representation of the datapath
    */
   void add_datapath_connection(const technology_managerRef TM, const structural_managerRef SM, const structural_objectRef conn_out, const structural_objectRef port_tgt, unsigned int conn_type);

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

   /**
    * Destructor.
    */
   virtual ~conn_binding();

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
   generic_objRef bind_command_port(const vertex& ver, direction_type dir, unsigned int mode, const OpGraphConstRef g);

   generic_objRef bind_selector_port(direction_type dir, unsigned int mode, const vertex& ver, const OpGraphConstRef data);

   generic_objRef bind_selector_port(direction_type dir, unsigned int mode, const generic_objRef elem, unsigned int op);

   /**
    * Returns reference to generic object associated to a given variable, for a specific port direction
    * @param var is variable associated with the port
    * @param dir is port direction
    * @return reference to the generic object of the port associated to given variable, into given direction
    */
   generic_objRef get_port(unsigned int var, direction_type dir);

   /**
    * Returns reference to generic object associated to a given vertex, for a specific port direction
    * @param ver is vertex associated to desired port
    * @param dir is port direction
    * @param mode is command mode (OPERATION or CONDITION)
    * @return reference to the generic object of the port associated to given vertex, into given direction
    */
   generic_objRef get_command_port(const vertex& ver, direction_type dir, unsigned int mode);

   /**
    * Returns the number of connections from the datapath to the controller
    */
   unsigned int get_to_controller_ports() const;

   /**
    * Returns the list of connections from the controller to the datapath
    */
   std::map<std::pair<vertex, unsigned int>, generic_objRef> get_command_input_ports() const;

   /**
    * Returns the number of connections from the controller to the datapath
    */
   unsigned int get_from_controller_ports() const;

   /**
    * Function that prints the interconnection binding
    */
   virtual void print() const;

   /**
    * Adds a data transfer between two objects
    * @param op1 is reference to first object
    * @param op2 is reference to second object
    * @param operand is i-th operand for second object, where first one is connected
    * @param port_index is i-th port associated with the operand (different from 0 when multi-channels components are considered)
    * @param data is the data to be transferred
    */
   void add_data_transfer(const generic_objRef op1, const generic_objRef op2, unsigned int operand, unsigned int port_index, data_transfer data);

   /**
    * Creates a connection between two objects
    * @param op1 is reference to first object
    * @param op2 is reference to second object
    * @param operand is i-th operand for second object, where first one is connected
    * @param port_index is i-th port associated with the operand (different from 0 when multi-channels components are considered)
    * @param conn is the reference to the implemented connection
    */
   void add_connection(const generic_objRef op1, const generic_objRef op2, unsigned int operand, unsigned int port_index, connection_objRef conn);

   /**
    * Return the current connection between two objects, if any
    * @param op1 is reference to first object
    * @param op2 is reference to second object
    * @param operand is i-th operand for second object, where first one is connected
    * @param port_index is i-th port associated with the operand (different from 0 when multi-channels components are considered)
    * @return conn is the reference to the implemented connection
    */
   connection_objRef get_connection(const generic_objRef op1, const generic_objRef op2, unsigned int operand, unsigned int port_index) const;

   /**
    * Checks if there is a connection between two objects
    * @param op1 is reference to first object
    * @param op2 is reference to second object
    * @param operand is i-th operand for second object, where first one is connected
    * @param port_index is i-th port associated with the operand (different from 0 when multi-channels components are considered)
    */
   bool is_connection(const generic_objRef op1, const generic_objRef op2, unsigned int operand, unsigned int port_index) const;

   /**
    * Returns the map containing all the data transfers
    */
   const std::map<ConnectionTarget, ConnectionSources>& get_data_transfers() const;

   /**
    * Returns the number of bit-level multiplexers
    */
   unsigned int determine_bit_level_mux() const;

   /**
    * Returns the map containing all the connection implementations
    */
   const conn_implementation_map& get_connection_implementations() const;

   /**
    * Adds a generic command port to proper set
    * @param obj is refcount to generic_obj class associated with port to be added
    */
   void add_command_port(const generic_objRef obj)
   {
      command_ports.insert(obj);
   }

   /**
    * Returns set of generic command ports
    * @return a set of all refcount to generic_obj's associated to ports
    */
   CustomOrderedSet<generic_objRef> get_command_ports() const
   {
      return command_ports;
   }

   const std::map<unsigned int, Selectors>& GetSelectors() const
   {
      return selectors;
   }

   unsigned int get_component_num(const std::string& type) const;

   void add_component(const std::string& type, unsigned int num = 1);

   void add_sparse_logic(const generic_objRef so)
   {
      sparse_logic.insert(so);
   }

   const std::map<std::string, unsigned int>& get_components() const;

   /**
    * Add the interconnection to the structural representation of the datapath
    */
   virtual void add_to_SM(const HLS_managerRef HLSMgr, const hlsRef HLS, const structural_managerRef SM);

   generic_objRef get_constant_obj(const std::string& value, const std::string& param, unsigned int precision);

   std::map<const_param, generic_objRef> get_constant_objs() const;

   /**
    * @brief factory method to create the right conn_binding depending on the flow
    * @param _HLSMgr
    * @param _HLS
    * @param _BH
    * @param _parameters
    * @return
    */
   static conn_bindingRef create_conn_binding(const HLS_managerRef _HLSMgr, const hlsRef _HLS, const BehavioralHelperConstRef _BH, const ParameterConstRef _parameters);
};
/// Refcount definition of the class
typedef refcount<conn_binding> conn_bindingRef;

#endif
