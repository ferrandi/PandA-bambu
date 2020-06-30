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
 * @file structural_manager.hpp
 * @brief Class implementation of the structural_manager.
 *
 * This class defines functions used to build a structural description.
 *
 * @author Matteo Barbati <mbarbati@gmail.com>
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *
 */
#ifndef STRUCTURAL_MANAGER_HPP
#define STRUCTURAL_MANAGER_HPP

#include "config_HAVE_BAMBU_BUILT.hpp"
#include "config_HAVE_EUCALYPTUS_BUILT.hpp"
#include "config_HAVE_KOALA_BUILT.hpp"
#include "config_HAVE_TUCANO_BUILT.hpp"

#include "NP_functionality.hpp"
#include "graph.hpp"
#include "refcount.hpp"
#include "structural_objects.hpp"
#include <iosfwd> // for ostream
#include <string>

#define TREENODE_CHECK -1

/**
 * @name Forward declarations.
 */
//@{
REF_FORWARD_DECL(structural_object);
CONSTREF_FORWARD_DECL(technology_manager);
REF_FORWARD_DECL(tree_manager);
CONSTREF_FORWARD_DECL(Parameter);
REF_FORWARD_DECL(technology_manager);
REF_FORWARD_DECL(technology_node);
struct graphs_collection;
struct graph;
/// forward decl of xml Element
class xml_element;
//@}

/**
 * This class manages the circuit structures.
 */
class structural_manager
{
 public:
   // no copy constructor
   structural_manager(const structural_manager& inst) = delete;

   enum circuit_graph_type
   {
      DATA_G,
      COMPLETE_G
   };

 private:
   /// class containing all the parameters
   const ParameterConstRef Param;

   /// debug level
   int debug_level;

   /**
    * Bulk graph used to represent all graphs.
    */
   graphs_collection* og;
   /**
    * Graph only composed by the data flow in the circuit.
    */
   graph* data_graph;
   /**
    * Graph containing all lines of the circuit.
    */
   graph* circuit_graph;

   /**
    * Structure that represent circuit
    */
   structural_objectRef circuit;

   /**
    * Check if two type structural object are consistent.
    * @param src_type is the first structural object.
    * @param dest_type is the second structural object.
    * @return true if the two structural objects are consistent.
    */
   bool check_type(structural_objectRef src_type, structural_objectRef dest_type);
   /**
    * Function that check if a signal (or port) is already bound on a port.
    * @param src is the reference to the port.
    * @param sign is the reference of the signal (or the second port).
    */
   bool check_bound(structural_objectRef src, structural_objectRef sign);

   /**
    * perform some check on the circuit manager.
    */
   void check_structure(structural_objectRef obj, bool permissive = false);

   /**
    * build a graph starting from a structural object.
    * @param top is the top component from which the graph is built.
    * @param og is the bulk graph where the graph is stored.
    */
   void build_graph(const structural_objectRef& top, graphs_collection* og);

 public:
   /**
    * This is the constructor of the structural_manager which initializes the data field.
    * @param Param is the reference to the class containing all parameters
    */
   explicit structural_manager(const ParameterConstRef Param);

#if HAVE_BAMBU_BUILT || HAVE_KOALA_BUILT
   void set_top_info(const std::string& id, const technology_managerRef& TM, const std::string& Library = "");
#endif

   /**
    * Destructor.
    */
   ~structural_manager();

   /**
    * Verify if the component is already associated with owner.
    * @param id is the name of the object.
    * @param so_kind is the type of the object.
    * @return true if the object is already associated with owner, false otherwise.
    */
   bool check_object(std::string id, structural_objectRef owner, so_kind type);

   /**
    * Return the objectRef given the id of the object and the c_object of the owner.
    * @param o_tn is the owner c_object.
    * @param tn is the treenode.
    * @return the c_object.
    */
   // structural_objectRef getc_object(const structural_objectRef &o_tn, int tn) const;

   /**
    * Set the characteristics of the Top object.
    * @param id is the name of the object.
    * @param module_type is the type of the top object.
    * @param treenode is the treenode of the object.
    */
   void set_top_info(std::string id, structural_type_descriptorRef module_type, unsigned int treenode = 0);
   /**
    * Create a new object of the circuit.
    * @param id is the name of the object.
    * @param ctype represent the type of the object (component or channel).
    * @param owner is the owner of the object.
    * @param obj_type is the type descriptor of the object.
    * @param treenode is the treenode of the object.
    */
   structural_objectRef create(std::string id, so_kind ctype, structural_objectRef owner, structural_type_descriptorRef obj_type, unsigned int treenode = 0);

#if HAVE_BAMBU_BUILT || HAVE_KOALA_BUILT || HAVE_EUCALYPTUS_BUILT
   /**
    * Create a new object starting from a library component.
    * @param id is the name of the object.
    * @param fu_name is the name of the type.
    * @param library_name is the name of the library.
    * @param owner is the owner of the object.
    * @param TM is the technology manager.
    * @return the object created.
    */
   structural_objectRef add_module_from_technology_library(const std::string& id, const std::string& fu_name, const std::string& library_name, const structural_objectRef owner, const technology_managerConstRef TM);
#endif

   void remove_module(structural_objectRef obj);

   void remove_connection(structural_objectRef src, structural_objectRef dest);

   void change_connection(structural_objectRef old_obj, structural_objectRef new_obj, structural_objectRef owner);

   /**
    * Create a new port.
    * @param id is the name of the port.
    * @param pdir represent the direction of the port (in, out, in-out, gen).
    * @param owner is the reference to the owner of the port.
    * @param port_type is the type of the port.
    * @param treenode is the treenode of the port.
    */
   structural_objectRef add_port(std::string id, port_o::port_direction pdir, structural_objectRef owner, structural_type_descriptorRef port_type, unsigned int treenode = 0);

   /**
    * Change the direction of the port
    * @param port_object is the port that will change the pdir
    * @param pdir represent the new direction of the port (in, out, in-out, gen).
    * @param owner is the reference to the owner of the port.
    */
   void change_port_direction(structural_objectRef port_object, port_o::port_direction pdir, structural_objectRef owner);

   /**
    * Create a new port_vector.
    * @param id is the name of the port_vector.
    * @param pdir represent the direction of the port_vector (in, out, in-out, gen).
    * @param n_ports is the number of port associated with the port_vector object. When the number of ports is not defined n_ports is equal to PARAMETRIC_PORT.
    * @param owner is the reference to the owner of the port_vector.
    * @param port_type is the type of the port_vector.
    * @param treenode is the treenode of the port_vector.
    */
   structural_objectRef add_port_vector(std::string id, port_o::port_direction pdir, unsigned int n_ports, structural_objectRef owner, structural_type_descriptorRef type_descr, unsigned int treenode = 0);
   /**
    * Create a new signal.
    * @param id is the name of the signal.
    * @param owner is the reference to the owner of the signal.
    * @param sign_type is the type of the signal.
    * @param treenode is the treenode of the signal.
    */
   structural_objectRef add_sign(std::string id, structural_objectRef owner, structural_type_descriptorRef sign_type, unsigned int treenode = 0);

   structural_objectRef add_sign_vector(std::string id, unsigned int n_signs, structural_objectRef owner, structural_type_descriptorRef sign_type, unsigned int treenode = 0);

   /**
    * Remove an existing signal from the SM.
    * The signal must not have any connected objects.
    */
   void remove_empty_signal(structural_objectRef& signal);

   /**
    * Disconnects a member from from_signal and reconnects it to to_signal.
    * Signals must be of compatible types and owned by the same object
    */
   void reconnect_signal_member(structural_objectRef& port, structural_objectRef& from_signal, structural_objectRef& to_signal);

   /**
    * Create a new constant;
    * @param id is the name of the constant;
    * @param owner is the reference to the owner of the constant;
    * @param type is the type of the constant;
    * @param value is the value of the constant in string form;
    * @param treenode is the treenode of the constant
    */
   structural_objectRef add_constant(std::string id, structural_objectRef owner, structural_type_descriptorRef type, std::string value, unsigned int treenode = 0);

#if HAVE_TUCANO_BUILT
   /**
    * Create a new local data.
    * @param id is the name of the local data.
    * @param owner is the reference to the owner of the local data.
    * @param data_type is the type of the local data.
    * @param treenode is the treenode of the local data.
    */
   structural_objectRef add_local_data(std::string id, structural_objectRef owner, unsigned int treenode, const tree_managerRef& treeM);

   /**
    * Add a new event to the owner.
    * @param id is the event name.
    * @param owner is the reference to the owner of the event.
    * @param treenode is the treenode of the event.
    */
   structural_objectRef add_event(std::string id, structural_objectRef owner, unsigned int treenode, const tree_managerRef& treeM);

   /**
    * Add a new process to the owner.
    * @param id is the process name.
    * @param owner is the reference to the owner of the process.
    * @param treenode is the treenode of the process.
    * @param scope is the scope of the process.
    * @param ft is the function type.
    */
   structural_objectRef add_process(std::string id, structural_objectRef owner, unsigned int treenode, const tree_managerRef& treeM, std::string scope, int ft);
   /**
    * Add a new service to the owner.
    * @param id is the service name.
    * @param interface is the interface id.
    * @param owner is the reference to the owner of the service.
    * @param treenode is the treenode of the service.
    * @param scope is the scope of the service.
    * @param ft is the function type.
    */
   structural_objectRef add_service(std::string id, std::string interface, structural_objectRef owner, unsigned int treenode, const tree_managerRef& treeM, std::string scope);
   /**
    * Add a parameter to the process.
    * @param id is the name of the parameter.
    * @param owner is the reference to the owner of the parameter.
    * @param treenode is the treenode of the parameter.
    */
   structural_objectRef add_process_param(std::string id, structural_objectRef owner, unsigned int treenode, const tree_managerRef& treeM);
   /**
    * Add a parameter to the service.
    * @param id is the name of the parameter.
    * @param owner is the reference to the owner of the parameter.
    * @param treenode is the treenode of the parameter.
    */
   structural_objectRef add_service_param(std::string id, structural_objectRef owner, unsigned int treenode, const tree_managerRef& treeM);
#endif

   /**
    * Add a not-parsed functionality.
    * @param owner is the reference to the owner of the functionality.
    * @param dt is the type of the not-parsed functionality.
    * @param descr is the description of the functionality.
    */
   void add_NP_functionality(structural_objectRef owner, NP_functionality::NP_functionaly_type dt, std::string functionality_description);

   /**
    * Specify a parameter for the top module
    * @param name is the parameter name
    * @param value is the parameter value
    */
   void SetParameter(const std::string& name, const std::string& value);

   /**
    * Add an object to the sensitivity list of process/service
    * @param obj is the object.
    * @param pr is the process.
    */
   void add_sensitivity(structural_objectRef obj, structural_objectRef pr);
   /**
    * Create a connection between a source structural object and a destination structural object.
    * A source port can be connected to another port, a signal and to a channel. Two primary ports cannot be connected, in this case a signal is needed.
    * A source signal can be connected to another port.
    * The other combinations are not allowed.
    * @param src is the source.
    * @param dest is the destination.
    */
   void add_connection(structural_objectRef src, structural_objectRef dest);

   /**
    * Function that prints the circuit data structure.
    * @param os is the output stream
    */
   void print(std::ostream& os) const;
   /**
    * Function that writes the dot file of the graph by using the AT&T dot format.
    */
   void WriteDot(const std::string& file_name, circuit_graph_type gt, graph* g = nullptr) const;

   /**
    * Get a reference to circ field
    * @return a reference to circ field
    */
   const structural_objectRef get_circ() const
   {
      return circuit;
   }

   /**
    * @name XML functions.
    */
   //@{
   /**
    * Load a structural manager from an xml file.
    * @param node is a node of the xml tree.
    * @param CM is the refcount version of this.
    */
   static void xload(const xml_element* node, structural_managerRef const& CM);
   /**
    * Add a component to an xml tree.
    * @param rootnode is the root node at which the xml representation of the operation is attached.
    */
   void xwrite(xml_element* rootnode, const technology_nodeRef& tn = technology_nodeRef()) const;
   //@}

   /**
    * Friend definition of the << operator.
    * @param os is the output stream
    * @param s is the circuit manager element
    */
   friend std::ostream& operator<<(std::ostream& os, structural_manager& s)
   {
      s.print(os);
      return os;
   }
   /**
    * Friend definition of the << operator. Pointer version.
    * @param os is the output stream
    * @param s is the circuit manager element
    */
   friend std::ostream& operator<<(std::ostream& os, const structural_managerRef s)
   {
      if(s)
         s->print(os);
      return os;
   }
   /**
    * Return the PI vertex of the circuit, at a specific level.
    * @param level represent the reference to the required level.
    */
   const vertex& get_PI(structural_objectRef level) const;

   void INIT(bool permissive = false);

   int get_debug_level() const
   {
      return debug_level;
   }
};

/**
 * RefCount type definition of the structural_manager class structure
 */
typedef refcount<structural_manager> structural_managerRef;

#endif
