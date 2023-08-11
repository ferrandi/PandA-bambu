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
 *              Copyright (C) 2004-2023 Politecnico di Milano
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
 * @file hls_manager.hpp
 * @brief Data structure representing the entire HLS information
 *
 * @author Christian Pilato <pilato@elet.polimi.it>
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *
 */
#ifndef HLS_MANAGER_HPP
#define HLS_MANAGER_HPP

/// Superclass include
#include "application_manager.hpp"

/// Autoheader include
#include "config_HAVE_TASTE.hpp"

/// utility include
#include "custom_map.hpp"

REF_FORWARD_DECL(AadlInformation);
REF_FORWARD_DECL(hls);
REF_FORWARD_DECL(HLS_target);
REF_FORWARD_DECL(HLS_manager);
REF_FORWARD_DECL(functions);
REF_FORWARD_DECL(memory);
REF_FORWARD_DECL(SimulationInformation);
REF_FORWARD_DECL(BackendFlow);

enum interface_attributes
{
   attr_interface_type,
   attr_interface_dir,
   attr_interface_bitwidth,
   attr_interface_alignment,
   attr_size,
   attr_size_in_bytes,
   attr_offset,
   attr_bundle_name,
   attr_way_lines,
   attr_line_size,
   attr_bus_size,
   attr_n_ways,
   attr_buf_size,
   attr_rep_pol,
   attr_wr_pol,
   attr_typename
};
class HLS_manager : public application_manager
{
 public:
   /// tuple set used to represent the required values or the constant default value associated with the inputs of a
   /// node
   using io_binding_type = std::tuple<unsigned int, unsigned int>;

 private:
   /// information about the target device/technology for the synthesis
   HLS_targetRef HLS_T;

   /// map between the function id and the corresponding HLS datastructure
   std::map<unsigned int, hlsRef> hlsMap;

   /// reference to the datastructure implementing the backend flow
   BackendFlowRef back_flow;

   /// The version of memory representation on which this step was applied
   unsigned int memory_version;

 public:
   /// base address for memory space addressing
   unsigned long long int base_address;

   /// HLS execution time
   long HLS_execution_time;

   /// information about function allocation
   functionsRef Rfuns;

   /// information about memory allocation
   memoryRef Rmem;

   /// information about the simulation
   SimulationInformationRef RSim;

   /// Evaluations
   CustomMap<std::string, std::vector<double>> evaluations;

   /// The auxiliary files
   std::list<std::string> aux_files;

   /// The HDL files
   std::list<std::string> hdl_files;

#if HAVE_TASTE
   /// The information collected from aadl files
   const AadlInformationRef aadl_information;
#endif
   /** store the design interface attributes coming from an xml file:
    * function_name->parameter_name->attribute_name->value
    */
   std::map<std::string, std::map<std::string, std::map<interface_attributes, std::string>>> design_attributes;
   /// store the design interface signature coming from an xml file: function_name->typename_signature
   std::map<std::string, std::vector<std::string>> design_interface_typename_signature;
   /// store the design interface original signature coming from an xml file: function_name->typename_signature
   std::map<std::string, std::vector<std::string>> design_interface_typename_orig_signature;
   /// store the design interface typename includes coming from an xml file:
   /// function_name->parameter_name->interface_typenameinclude
   std::map<std::string, std::map<std::string, std::string>> design_interface_typenameinclude;
   /// store the design interface read/write references of parameters:
   /// function_name->bb_index->parameter_name->list_of_loads
   std::map<std::string, std::map<unsigned, std::map<std::string, std::list<unsigned>>>> design_interface_io;

   /// store the constraints on resources added to manage the I/O interfaces:
   /// function_id->library_name->resource_function_name->number of resources
   std::map<unsigned, std::map<std::string, std::map<std::string, unsigned int>>> design_interface_constraints;

   /// global resource constraints
   std::map<std::pair<std::string, std::string>, unsigned> global_resource_constraints;

   /**
    * Constructor.
    */
   HLS_manager(const ParameterConstRef Param, const HLS_targetRef HLS_T);

   /**
    * Destructor.
    */
   ~HLS_manager() override;

   /**
    * Returns the HLS datastructure associated with a specific function
    */
   hlsRef get_HLS(unsigned int funId) const;

   /**
    * Creates the HLS flow starting from the given specification
    */
   static hlsRef create_HLS(const HLS_managerRef HLSMgr, unsigned int functionId);

   /**
    * Returns the datastructure associated with the HLS target
    */
   HLS_targetRef get_HLS_target() const;

   /**
    * Returns the backend flow
    */
   const BackendFlowRef get_backend_flow();

   /**
    * Return the specified constant in string format
    */
   std::string get_constant_string(unsigned int node, unsigned long long precision);

   /**
    * Writes the current HLS project into an XML file
    */
   void xwrite(const std::string& filename);

   /**
    * Returns the values required by a vertex
    */
   std::vector<io_binding_type> get_required_values(unsigned int fun_id, const vertex& v) const;

   /**
    * helper function that return true in case the variable is register compatible
    * @param var is the variable
    * @return true in case var is register compatible
    */
   bool is_register_compatible(unsigned int var) const;

   /**
    * @brief is_reading_writing_function
    * @param funID is the function identifier
    * @return true in case the function performs at least a load or a store
    */
   bool is_reading_writing_function(unsigned funID) const;

   /**
    * Returns all the implementations resulting from the synthesis
    */
   CustomOrderedSet<hlsRef> GetAllImplementations() const;

   /**
    * Return if single write memory is exploited
    */
   bool IsSingleWriteMemory() const;

   /**
    * Return the version of the memory intermediate representation
    * @return bb_version
    */
   unsigned int GetMemVersion() const;

   /**
    * Update the version of the memory intermediate representation
    * @return the new version
    */
   unsigned int UpdateMemVersion();

   /// check if the maximum bitwidth used for registers, busses, muxes, etc. is compatible with prec
   static void check_bitwidth(unsigned long long prec);
};
/// refcount definition of the class
using HLS_managerRef = refcount<HLS_manager>;
using HLS_managerConstRef = refcount<const HLS_manager>;

#endif
