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
 * @file hls_manager.hpp
 * @brief Data structure representing the entire HLS information
 *
 * @author Christian Pilato <pilato@elet.polimi.it>
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 *
 */
#ifndef HLS_MANAGER_HPP
#define HLS_MANAGER_HPP

#include "application_manager.hpp"
#include "custom_map.hpp"

#include <boost/preprocessor/seq/for_each.hpp>

#include <map>
#include <set>
#include <string>
#include <vector>

REF_FORWARD_DECL(hls);
REF_FORWARD_DECL(HLS_device);
REF_FORWARD_DECL(HLS_manager);
class BackendWrapper;
class functions;
class memory;
class SimulationInformation;

#define ENUM_ID(r, data, elem) elem,
#define FUNC_ARCH_ATTR_ENUM                                                                             \
   (func_symbol)(func_name)(func_inline)(func_dataflow_top)(func_dataflow_module)(func_pipeline_style)( \
       func_pipeline_ii)(func_csroa)(func_original)
#define FUNC_ARCH_PARM_ATTR_ENUM                                                                                \
   (parm_port)(parm_index)(parm_bundle)(parm_offset)(parm_includes)(parm_typename)(parm_original_typename)(     \
       parm_elem_count)(parm_size_in_bytes)(parm_bank_allocation)(parm_array_dims)(parm_array_partition_types)( \
       parm_array_partition_factors)
#define FUNC_ARCH_IFACE_ATTR_ENUM                                                                                  \
   (iface_name)(iface_global)(iface_mode)(iface_direction)(iface_bitwidth)(iface_alignment)(iface_depth)(          \
       iface_register)(iface_cache_ways)(iface_cache_line_count)(iface_cache_line_size)(                           \
       iface_cache_num_write_outstanding)(iface_cache_rep_policy)(iface_cache_bus_size)(iface_cache_write_policy)( \
       iface_cache_word_size)(iface_bank_number)(iface_chunk_size)

REF_FORWARD_DECL(FunctionArchitecture);

class FunctionArchitecture
{
 public:
   enum func_attr
   {
      BOOST_PP_SEQ_FOR_EACH(ENUM_ID, BOOST_PP_EMPTY, FUNC_ARCH_ATTR_ENUM)
   };
   static enum func_attr to_func_attr(const std::string& attr);

   enum parm_attr
   {
      BOOST_PP_SEQ_FOR_EACH(ENUM_ID, BOOST_PP_EMPTY, FUNC_ARCH_PARM_ATTR_ENUM)
   };
   static enum parm_attr to_parm_attr(const std::string& attr);

   enum iface_attr
   {
      BOOST_PP_SEQ_FOR_EACH(ENUM_ID, BOOST_PP_EMPTY, FUNC_ARCH_IFACE_ATTR_ENUM)
   };
   static enum iface_attr to_iface_attr(const std::string& attr);

   using func_attrs = std::map<enum func_attr, std::string>;
   using parm_attrs = std::map<enum parm_attr, std::string>;
   using iface_attrs = std::map<enum iface_attr, std::string>;

   func_attrs attrs;
   std::map<std::string, parm_attrs> parms;
   std::map<std::string, iface_attrs> ifaces;
};

REF_FORWARD_DECL(ModuleArchitecture);
class ModuleArchitecture
{
 public:
   using FunctionArchitectures = std::map<std::string, FunctionArchitectureRef>;

 private:
   FunctionArchitectures _funcArchs;

 public:
   ModuleArchitecture(const std::string& filename);

   FunctionArchitectures::const_iterator cbegin() const
   {
      return _funcArchs.cbegin();
   }

   FunctionArchitectures::const_iterator cend() const
   {
      return _funcArchs.cend();
   }

   FunctionArchitectures::const_iterator begin() const
   {
      return _funcArchs.begin();
   }

   FunctionArchitectures::const_iterator end() const
   {
      return _funcArchs.end();
   }

   FunctionArchitectures::iterator erase(FunctionArchitectures::const_iterator it)
   {
      return _funcArchs.erase(it);
   }

   void AddArchitecture(const std::string& symbol, FunctionArchitectureRef arch);

   FunctionArchitectureRef GetArchitecture(const std::string& funcSymbol) const;

   void RemoveArchitecture(const std::string& funcSymbol);
};

class HLS_manager : public application_manager
{
 public:
   /// tuple set used to represent the required values or the constant default value associated with the inputs of a
   /// node
   using io_binding_type = std::tuple<unsigned int, unsigned int>;

   struct ding_dong_buffer_info
   {
      unsigned int producer_function_id;
      unsigned int consumer_function_id;
      std::set<unsigned int> producer_function_ids;
      std::set<unsigned int> consumer_function_ids;
      std::set<std::string> bundle_names;
   };

 private:
   /// information about the target device/technology for the synthesis
   HLS_deviceRef HLS_D;

   /// map between the function id and the corresponding HLS data-structure
   std::map<unsigned int, hlsRef> hlsMap;

   /// reference to the data-structure implementing the backend flow
   std::unique_ptr<BackendWrapper> back_flow;

   /// The version of memory representation on which this step was applied
   unsigned int memory_version;

   /// candidate shared dataflow-top local arrays eligible for ding-dong buffering
   std::map<unsigned int, std::map<unsigned int, ding_dong_buffer_info>> ding_dong_buffer_candidates_;

 public:
   /// base address for memory space addressing
   unsigned long long int base_address;

   /// HLS execution time
   long HLS_execution_time;

   /// information about function allocation
   std::unique_ptr<functions> Rfuns;

   /// information about memory allocation
   std::unique_ptr<memory> Rmem;

   /// unused port interface
   std::map<unsigned, std::set<std::pair<std::string, std::string>>> unused_interfaces;

   /// information about the simulation
   std::unique_ptr<SimulationInformation> RSim;

   /// The auxiliary files
   std::list<std::string> aux_files;

   /// The HDL files
   std::list<std::string> hdl_files;

   ModuleArchitectureRef module_arch;

   /// store the design interface read/write references of parameters:
   /// function_name->bb_index->parameter_name->list_of_loads
   std::map<std::string, std::map<unsigned, std::map<std::string, std::list<unsigned>>>> design_interface_io;

   /// global resource constraints
   std::map<std::pair<std::string, std::string>, std::pair<unsigned, unsigned>> global_resource_constraints;

   /// ordered set of bundles required for the banked memory allocation
   std::deque<std::string> bundle_required;

   /// maps each parameter to the corresponding bundle id
   std::map<std::string, unsigned int> bundle_map;

   /**
    * Constructor.
    */
   HLS_manager(const ParameterConstRef Param, const HLS_deviceRef HLS_D);

   /**
    * Returns the HLS data-structure associated with a specific function
    */
   hlsRef get_HLS(unsigned int funId) const;

   /**
    * Creates the HLS flow starting from the given specification
    */
   static hlsRef create_HLS(const HLS_managerRef HLSMgr, unsigned int functionId);

   /**
    * Returns the data-structure associated with the HLS target
    */
   HLS_deviceRef get_HLS_device() const;

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
   std::vector<io_binding_type> get_required_values(unsigned int fun_id, gc_vertex_descriptor v) const;

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
    * Returns all the implementations resulting from the synthesis ordered by functionId.
    */
   std::vector<hlsRef> GetAllImplementations() const;

   /**
    * Return if single write memory is exploited
    */
   bool IsSingleWriteMemory() const;

   /**
    * Return if SDS private BRAMs should use the single-port wrapper.
    */
   bool UseSinglePortSdsMemory() const;

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

   /// returns a pair with the nth element of bundle_required and true if the element exist, false otherwise
   std::pair<std::string, bool> bundle_required_get_nth_element(unsigned int index) const;

   /// clears the cached ding-dong buffer candidates
   void clear_ding_dong_buffer_candidates();

   /// stores a ding-dong candidate for a dataflow top local array
   void add_ding_dong_buffer_candidate(unsigned int top_id, unsigned int var_id, const ding_dong_buffer_info& info);

   /// returns all ding-dong buffer candidates (full map)
   const std::map<unsigned int, std::map<unsigned int, HLS_manager::ding_dong_buffer_info>>&
   get_ding_dong_buffer_candidates() const;

   /// returns the ding-dong candidates associated with the given dataflow top
   const std::map<unsigned int, ding_dong_buffer_info>& get_ding_dong_buffer_candidates(unsigned int top_id) const;

   /// returns the ding-dong candidate for a given top/variable pair, or null if absent
   const ding_dong_buffer_info* get_ding_dong_buffer_candidate(unsigned int top_id, unsigned int var_id) const;
};
/// refcount definition of the class
using HLS_managerRef = refcount<HLS_manager>;
using HLS_managerConstRef = refcount<const HLS_manager>;

#endif
