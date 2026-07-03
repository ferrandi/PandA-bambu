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
 * @file structural_manager.hpp
 * @brief Class implementation of the structural_manager.
 *
 * This class defines functions used to build a structural description.
 *
 * @author Matteo Barbati <mbarbati@gmail.com>
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 *
 */
#ifndef STRUCTURAL_MANAGER_HPP
#define STRUCTURAL_MANAGER_HPP

#include "NP_functionality.hpp"
#include "cg_graph.hpp"
#include "refcount.hpp"
#include "structural_objects.hpp"

#include <iosfwd>
#include <string>

#define IR_NODE_ID_CHECK (-1)

REF_FORWARD_DECL(structural_object);
CONSTREF_FORWARD_DECL(technology_manager);
REF_FORWARD_DECL(ir_manager);
CONSTREF_FORWARD_DECL(Parameter);
REF_FORWARD_DECL(technology_manager);
REF_FORWARD_DECL(technology_node);
/// forward decl of xml Element
class xml_element;

/**
 * This class manages the circuit structures.
 */
class structural_manager : private CGGraphsCollection
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
    * Graph only composed by the data flow in the circuit.
    */
   CGGraph data_graph;
   /**
    * Graph containing all lines of the circuit.
    */
   CGGraph circuit_graph;

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
   bool check_type(const structural_objectRef& src_type, const structural_objectRef& dest_type);

   /**
    * Function that check if a signal (or port) is already bound on a port.
    * @param src is the reference to the port.
    * @param sign is the reference of the signal (or the second port).
    */
   bool check_bound(const structural_objectRef& src, const structural_objectRef& sign);

   /**
    * perform some check on the circuit manager.
    */
   void check_structure(const structural_objectRef& obj, bool permissive = false);

   /**
    * build a graph starting from a structural object.
    * @param top is the top component from which the graph is built.
    */
   void build_graph(const structural_objectRef& top);

   /**
    * Add a directed edge between the nodes associated with p1 and p2.
    * @param module_vertex_rel is the relation between modules and vertexes.
    * @param p1 is the first port.
    * @param p2 is the second object. It could be a port, a signal or a module.
    * @param en is the entry vertex.
    * @param ex is the exit vertex.
    * @param is_critical marks the edge as timing critical in the circuit graph.
    */
   void
   add_directed_edge(const std::map<structural_objectRef, CGGraphsCollection::vertex_descriptor>& module_vertex_rel,
                     const structural_objectRef& p1, const structural_objectRef& p2,
                     CGGraphsCollection::vertex_descriptor en, CGGraphsCollection::vertex_descriptor ex,
                     bool is_critical = false);

   void circuit_add_edge(CGGraphsCollection::vertex_descriptor A, CGGraphsCollection::vertex_descriptor B, int selector,
                         const structural_objectRef from1, structural_objectRef to1, bool is_critical = false);

   /**
    * Add a directed edge between the nodes associated with p1 and p2.
    * @param module_vertex_rel is the relation between modules and vertexes.
    * @param p1 is the first port.
    * @param p2 is the second port.
    * @param en is the entry vertex.
    * @param ex is the exit vertex.
    * @param is_critical marks the edge as timing critical in the circuit graph.
    */
   void add_directed_edge_single(
       const std::map<structural_objectRef, CGGraphsCollection::vertex_descriptor>& module_vertex_rel,
       const structural_objectRef& p1, const structural_objectRef& p2, CGGraphsCollection::vertex_descriptor en,
       CGGraphsCollection::vertex_descriptor ex, bool is_critical = false);

 public:
   /**
    * This is the constructor of the structural_manager which initializes the data field.
    * @param Param is the reference to the class containing all parameters
    */
   explicit structural_manager(ParameterConstRef Param);

   void set_top_info(const std::string& id, const technology_managerRef& LM, const std::string& Library = "");

   /**
    * Verify if the component is already associated with owner.
    * @param id is the name of the object.
    * @param owner is the structural object that should own the object identified by `id`.
    * @param type is the expected kind of the object.
    * @return true if the object is already associated with owner, false otherwise.
    */
   static bool check_object(std::string id, const structural_objectRef& owner, so_kind type);

   /**
    * Set the characteristics of the Top object.
    * @param id is the name of the object.
    * @param module_type is the type of the top object.
    * @param ir_node_id is the IR node id of the object.
    */
   void set_top_info(std::string id, structural_type_descriptorRef module_type, unsigned int ir_node_id = 0);

   /**
    * Create a new object of the circuit.
    * @param id is the name of the object.
    * @param ctype represent the type of the object.
    * @param owner is the owner of the object.
    * @param obj_type is the type descriptor of the object.
    * @param ir_node_id is the IR node id of the object.
    */
   structural_objectRef create(std::string id, so_kind ctype, structural_objectRef owner,
                               structural_type_descriptorRef obj_type, unsigned int ir_node_id = 0);

   /**
    * Create a new object starting from a library component.
    * @param id is the name of the object.
    * @param fu_name is the name of the type.
    * @param library_name is the name of the library.
    * @param owner is the owner of the object.
    * @param TM is the technology manager.
    * @return the object created.
    */
   structural_objectRef add_module_from_technology_library(const std::string& id, const std::string& fu_name,
                                                           const std::string& library_name,
                                                           const structural_objectRef owner,
                                                           const technology_managerConstRef TM);

   void remove_module(const structural_objectRef& obj);

   void remove_connection(const structural_objectRef& src, const structural_objectRef& dest);

   void change_connection(const structural_objectRef& old_obj, const structural_objectRef& new_obj,
                          const structural_objectRef& owner);

   /**
    * Create a new port.
    * @param id is the name of the port.
    * @param pdir represent the direction of the port (in, out, in-out, gen).
    * @param owner is the reference to the owner of the port.
    * @param type_descr is the type descriptor associated with the port.
    * @param ir_node_id is the IR node id of the port.
    */
   static structural_objectRef add_port(const std::string& id, port_o::port_direction pdir, structural_objectRef owner,
                                        structural_type_descriptorRef type_descr, unsigned int ir_node_id = 0);

   /**
    * Change the direction of the port
    * @param port_object is the port that will change the pdir
    * @param pdir represent the new direction of the port (in, out, in-out, gen).
    * @param owner is the reference to the owner of the port.
    */
   static void change_port_direction(const structural_objectRef& port_object, port_o::port_direction pdir,
                                     const structural_objectRef& owner);

   /**
    * Create a new port_vector.
    * @param id is the name of the port_vector.
    * @param pdir represent the direction of the port_vector (in, out, in-out, gen).
    * @param n_ports is the number of port associated with the port_vector object. When the number of ports is not
    * defined n_ports is equal to PARAMETRIC_PORT.
    * @param owner is the reference to the owner of the port_vector.
    * @param type_descr is the type descriptor associated with each port in the vector.
    * @param ir_node_id is the IR node id of the port_vector.
    */
   static structural_objectRef add_port_vector(std::string id, port_o::port_direction pdir, unsigned int n_ports,
                                               const structural_objectRef& owner,
                                               structural_type_descriptorRef type_descr, unsigned int ir_node_id = 0);
   /**
    * Create a new signal.
    * @param id is the name of the signal.
    * @param owner is the reference to the owner of the signal.
    * @param sign_type is the type of the signal.
    * @param ir_node_id is the IR node id of the signal.
    */
   static structural_objectRef add_sign(std::string id, const structural_objectRef& owner,
                                        structural_type_descriptorRef sign_type, unsigned int ir_node_id = 0);

   static structural_objectRef add_sign_vector(std::string id, unsigned int n_signs, const structural_objectRef& owner,
                                               structural_type_descriptorRef sign_type, unsigned int ir_node_id = 0);

   /**
    * Remove an existing signal from the SM.
    * The signal must not have any connected objects.
    */
   void remove_empty_signal(const structural_objectRef& signal);

   /**
    * Disconnects a member from from_signal and reconnects it to to_signal.
    * Signals must be of compatible types and owned by the same object
    */
   void reconnect_signal_member(const structural_objectRef& member, const structural_objectRef& from_signal,
                                const structural_objectRef& to_signal);

   /**
    * Create a new constant;
    * @param id is the name of the constant;
    * @param owner is the reference to the owner of the constant;
    * @param type is the type of the constant;
    * @param value is the value of the constant in string form;
    * @param ir_node_id is the IR node id of the constant
    */
   structural_objectRef add_constant(std::string id, const structural_objectRef& owner,
                                     structural_type_descriptorRef type, std::string value,
                                     unsigned int ir_node_id = 0);

   /**
    * Add a not-parsed functionality.
    * @param cir is the circuit object receiving the functionality annotation.
    * @param dt is the type of the not-parsed functionality.
    * @param functionality_description is the textual description of the functionality.
    */
   static void add_NP_functionality(const structural_objectRef& cir, NP_functionality::NP_functionaly_type dt,
                                    const std::string& functionality_description);

   /**
    * Append a not-parsed functionality. (created if not present)
    * @param cir is the circuit object receiving the functionality annotation.
    * @param dt is the type of the not-parsed functionality.
    * @param functionality_description is the textual description of the functionality to append.
    */
   static void append_NP_functionality(const structural_objectRef& cir, NP_functionality::NP_functionaly_type dt,
                                       const std::string& functionality_description);

   /**
    * Specify a parameter for the top module
    * @param name is the parameter name
    * @param value is the parameter value
    */
   void SetParameter(const std::string& name, const std::string& value);

   /**
    * Create a connection between a source structural object and a destination structural object.
    * A source port can be connected to another port, a signal and to a channel. Two primary ports cannot be connected,
    * in this case a signal is needed. A source signal can be connected to another port. The other combinations are not
    * allowed.
    * @param src is the source.
    * @param dest is the destination.
    */
   void add_connection(const structural_objectRef& src, const structural_objectRef& dest);

   /**
    * Function that prints the circuit data structure.
    * @param os is the output stream
    */
   void print(std::ostream& os) const;

   /**
    * Function that writes the dot file of the graph by using the AT&T dot format.
    */
   void writeDot(const std::filesystem::path& file_name, circuit_graph_type gt) const;

   /**
    * Get a reference to circ field
    * @return a reference to circ field
    */
   structural_objectRef get_circ() const
   {
      return circuit;
   }

   /**
    * Load a structural manager from an xml file.
    * @param node is a node of the xml tree.
    * @param CM is the refcount version of this.
    */
   static void xload(const xml_element* node, structural_managerRef const& CM);

   /**
    * Add a component to an xml tree.
    * @param rootnode is the root node at which the xml representation of the operation is attached.
    * @param tn optionally selects the technology node used to enrich the XML dump.
    */
   void xwrite(xml_element* rootnode, const technology_nodeRef& tn = nullptr) const;

   /**
    * Friend definition of the << operator.
    * @param os is the output stream
    * @param s is the circuit manager element
    */
   friend std::ostream& operator<<(std::ostream& os, const structural_manager& s)
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
      {
         s->print(os);
      }
      return os;
   }

   void INIT(bool permissive = false);

   int get_debug_level() const
   {
      return debug_level;
   }
};
using structural_managerRef = refcount<structural_manager>;
#endif
