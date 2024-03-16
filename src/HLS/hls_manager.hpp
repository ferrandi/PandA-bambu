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
 *              Copyright (C) 2004-2024 Politecnico di Milano
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

#include "application_manager.hpp"
#include "custom_map.hpp"

#include "config_HAVE_TASTE.hpp"

#include <boost/preprocessor/seq/for_each.hpp>

#include <map>
#include <string>

REF_FORWARD_DECL(AadlInformation);
REF_FORWARD_DECL(hls);
REF_FORWARD_DECL(HLS_device);
REF_FORWARD_DECL(HLS_manager);
REF_FORWARD_DECL(functions);
REF_FORWARD_DECL(memory);
REF_FORWARD_DECL(SimulationInformation);
REF_FORWARD_DECL(BackendFlow);

#define ENUM_ID(r, data, elem) elem,
#define FUNC_ARCH_ATTR_ENUM \
   (func_symbol)(func_name)(func_inline)(func_dataflow_top)(func_dataflow_module)(func_pipeline_style)(func_pipeline_ii)
#define FUNC_ARCH_PARM_ATTR_ENUM                                                                            \
   (parm_port)(parm_index)(parm_bundle)(parm_offset)(parm_includes)(parm_typename)(parm_original_typename)( \
       parm_elem_count)(parm_size_in_bytes)
#define FUNC_ARCH_IFACE_ATTR_ENUM                                                                           \
   (iface_name)(iface_mode)(iface_direction)(iface_bitwidth)(iface_alignment)(iface_depth)(iface_register)( \
       iface_cache_ways)(iface_cache_line_count)(iface_cache_line_size)(iface_cache_num_write_outstanding)( \
       iface_cache_rep_policy)(iface_cache_bus_size)(iface_cache_write_policy)

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
   ~ModuleArchitecture();

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

 private:
   /// information about the target device/technology for the synthesis
   HLS_deviceRef HLS_D;

   /// map between the function id and the corresponding HLS data-structure
   std::map<unsigned int, hlsRef> hlsMap;

   /// reference to the data-structure implementing the backend flow
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
   CustomMap<std::string, double> evaluations;

   /// The auxiliary files
   std::list<std::string> aux_files;

   /// The HDL files
   std::list<std::string> hdl_files;

#if HAVE_TASTE
   /// The information collected from aadl files
   const AadlInformationRef aadl_information;
#endif

   ModuleArchitectureRef module_arch;

   /// store the design interface read/write references of parameters:
   /// function_name->bb_index->parameter_name->list_of_loads
   std::map<std::string, std::map<unsigned, std::map<std::string, std::list<unsigned>>>> design_interface_io;

   /// global resource constraints
   std::map<std::pair<std::string, std::string>, std::pair<unsigned, unsigned>> global_resource_constraints;

   /**
    * Constructor.
    */
   HLS_manager(const ParameterConstRef Param, const HLS_deviceRef HLS_D);

   /**
    * Destructor.
    */
   ~HLS_manager() override;

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
