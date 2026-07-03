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
 * @file hls_constraints.hpp
 * @brief Data structure definition for HLS constraints.
 *
 * This class contains all the information useful to constrain the synthesis flow.
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 * @author Christian Pilato <pilato@elet.polimi.it>
 *
 */
#ifndef HLS_CONSTRAINTS_HPP
#define HLS_CONSTRAINTS_HPP

#include "custom_map.hpp"
#include "refcount.hpp"

#include <iosfwd>
#include <limits>
#include <string>
#include <utility>

REF_FORWARD_DECL(HLS_constraints);
CONSTREF_FORWARD_DECL(Parameter);
REF_FORWARD_DECL(generic_device);
class xml_element;

/// macro used to convert the functional unit name and the library in an unique string.
#define ENCODE_FU_LIB(fu_name, library) ((fu_name) + ":" + (library))

/**
 * @class HLS_constraints
 * @ingroup HLS
 * Data structure used to store all the HLS constraints
 */
class HLS_constraints
{
   /// For each functional unit tech_constraints stores the number of resources.
   CustomMap<std::string, unsigned int> tech_constraints;

   /// put into relation the vertex name with the functional unit, its library and the functional unit index
   /// this map can be used to define a binding for a vertex.
   CustomMap<std::string, std::pair<std::string, std::pair<std::string, unsigned int>>> binding_constraints;

   /// map between an operation vertex and its given scheduling priority value
   CustomMap<std::string, int> scheduling_constraints;

   /// Clock period in ns.
   double clock_period;

   /// current value of the resource fraction
   double clock_period_resource_fraction;

   /// Variable storing the number of registers.
   unsigned int registers;

   /// name of the function which the constraints are associated with; emtpy string means that they are global
   /// constraints
   std::string fun_name;

   /// The set of input parameters
   const ParameterConstRef parameters;

 public:
   /// class that can directly access
   friend class allocation;

   /**
    * Constructor.
    */
   HLS_constraints(const ParameterConstRef& Param, std::string _fun_name);

   /**
    * Gets the name of the function which the constraints are associated with
    */
   std::string GetFunctionName() const;

   /**
    * Adds the builtin constraints
    */
   void add_builtin_constraints();

   /**
    * Reads an XML file describing the structural data structures.
    * @param f the input file name
    */
   void read_HLS_constraints_File(const std::string& f);

   /**
    * @brief read_HLS_CL_constraints reads command-line constraints
    * @param s is the input string
    */
   void read_HLS_CL_constraints(const std::string& s);

   /**
    * Writes an XML file describing the high level synthesis constraints data structure.
    * @param f the output file name
    */
   void write_HLS_constraints_File(const std::string& f);

   /**
    * Sets the maximum number of resources available in the design
    * @param name is the name of the functional unit.
    * @param library is the library of name.
    * @param n_resources is the maximum number of fu available (default is infinite).
    */
   void set_number_fu(const std::string& name, const std::string& library,
                      unsigned int n_resources = std::numeric_limits<unsigned int>::max());

   /**
    * This method returns the maximum number of functional units available for the design.
    * @param name is the name of the functional unit.
    * @param library is the library of name.
    * @return the maximum number of functional units available.
    */
   unsigned int get_number_fu(const std::string& name, const std::string& library) const;

   /**
    * This method returns the maximum number of functional units available for the design.
    * @param combined is the name of the functional unit in the form `unit_name:library_name`
    * @return the maximum number of functional units available.
    */
   unsigned int get_number_fu(const std::string& combined) const;

   /**
    * Binds a vertex to a functional unit.
    * @param vertex_name is the name of the vertex.
    * @param fu_name is the name of the functional unit.
    * @param fu_library is the name of the library where the functional unit fu_name is contained.
    * @param fu_index is the ith functional unit instance.
    */
   void bind_vertex_to_fu(const std::string& vertex_name, const std::string& fu_name, const std::string& fu_library,
                          const unsigned int fu_index);

   /**
    * Binds a vertex to a functional unit.
    * @param vertex_name is the name of the operation vertex
    */
   bool has_binding_to_fu(const std::string& vertex_name) const;

   /**
    * Stores the priority value for an operation vertex
    * @param vertex_name is the operation name
    * @param Priority is the priority value related to the operation vertex
    */
   void set_scheduling_priority(const std::string& vertex_name, int Priority);

   /**
    * Returns the data structure containing scheduling priority constraints
    * @return a map between the operation vertex and its priority value
    */
   CustomMap<std::string, int> get_scheduling_priority() const;

   /**
    * Sets the clock period.
    * @param period is the clock period (in ns)
    */
   void set_clock_period(double period);

   /**
    * Gets the clock period.
    * @return the clock period (in ns)
    */
   double get_clock_period() const
   {
      return clock_period;
   }

   /**
    * Gets the resource fraction of the clock period.
    * @return the value of the resource fraction
    */
   double get_clock_period_resource_fraction() const
   {
      return clock_period_resource_fraction;
   }

   /**
    * Sets the number of registers present in the function.
    * @param n_resources is the maximum number of registers available
    */
   void set_max_registers(unsigned int n_resources);

   /**
    * Returns the number of registers available.
    * @return the maximum number of registers
    */
   unsigned int get_max_registers() const
   {
      return registers;
   }

   /**
    * Loads a set of HLS constraints starting from an XML node.
    * @param Enode is a node of the XML tree where the constraints are stored.
    */
   void xload(const xml_element* Enode);

   /**
    * Writes the HLS constraints into an XML tree.
    * @param rootnode is the root node which the XML representation of the constraints is attached.
    */
   void xwrite(xml_element* rootnode);

   /**
    * Writes the generic synthesis constraints
    * @param Enode is the node of the XML tree where the constraints will be stored.
    * @param forDump controls whether additional dump metadata is produced
    */
   void xwriteHLSConstraints(xml_element* Enode, bool forDump = false);

   /**
    * Writes the constraints on the functional units
    * @param Enode is the node of the XML tree where the constraint will be stored.
    * @param fu_name is the name of the functional unit
    * @param fu_library is the name of the library where the functional unit fu_name is contained.
    * @param number_fu is the number of the functional unit instances
    * @param forDump controls whether additional dump metadata is produced
    */
   void xwriteFUConstraints(xml_element* Enode, const std::string& fu_name, const std::string& fu_library,
                            unsigned int number_fu, bool forDump = false);

   /**
    * Writes the binding constraints (i.e., functional unit, instance and library) associated with an operation
    * @param Enode is the node of the XML tree where the constraint will be stored.
    * @param vertex_name is the operation name
    * @param fu_name is the name of the functional unit
    * @param fu_library is the name of the library where the functional unit fu_name is contained.
    * @param fu_index is the index of the functional unit instance bound to the operation
    */
   void xwriteBindingConstraints(xml_element* Enode, const std::string& vertex_name, const std::string& fu_name,
                                 const std::string& fu_library, unsigned int fu_index);

   /**
    * Writes the scheduling constraints (i.e., priority value) associated with an operation
    * @param Enode is the node of the XML tree where the constraint will be stored.
    * @param vertex_name is the operation name
    * @param priority is the priority value for the scheduling
    */
   void xwriteSchedulingConstraints(xml_element* Enode, const std::string& vertex_name, int priority);

   /**
    * Function that prints the class HLS_constraints.
    * @param os is the output stream.
    */
   void print(std::ostream& os) const;

   /**
    * Friend definition of the << operator.
    */
   friend std::ostream& operator<<(std::ostream& os, const HLS_constraints& s)
   {
      s.print(os);
      return os;
   }

   /// definition of the get_kind_text method
   std::string get_kind_text() const
   {
      return std::string("HLS_constraints");
   }
};
/// refcount definition of the class
using HLS_constraintsRef = refcount<HLS_constraints>;

#endif
