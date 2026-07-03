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
 *              Copyright (c) 2016-2026 Politecnico di Milano
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
 * @file bus_interface.hpp
 * @brief Class to generate the interface for the context switch project
 *
 * @author Giovanni Gozzi <giovanni.gozzi@polimi.it>
 *
 */
#ifndef BUS_INTERFACE_H
#define BUS_INTERFACE_H
#include "module_interface.hpp"

#include "hls_function_step.hpp"
#include "refcount.hpp"
#include "structural_objects.hpp"

REF_FORWARD_DECL(structural_manager);
REF_FORWARD_DECL(structural_object);

class bus_interface : public module_interface
{
   struct bus_interface_info
   {
      bool const_use_profiling_modules;
      bool const_use_bank_allocation;
      bool use_registered_noc;
      bool use_caches;
      bool use_axi;
      unsigned int channel_number;
      unsigned int bank_number;
      unsigned int bank_index;
      unsigned int bank_index_bits_used;
      unsigned int noc_nodes;
      unsigned int noc_id_size;
      long long unsigned int addr_bus_bitsize;
      long long unsigned int addr_banked_bitsize;
      long long unsigned int data_bus_bitsize;
      long long unsigned int size_bus_bitsize;
      long long unsigned int context_bus_bitsize;
      std::string fname;
   } bi;

 public:
   bus_interface(const ParameterConstRef Param, const HLS_managerRef HLSMgr, unsigned int funId,
                 const DesignFlowManager& design_flow_manager,
                 const HLSFlowStep_Type hls_flow_step_type = HLSFlowStep_Type::BUS_INTERFACE_GENERATION);

   void build_wrapper(structural_objectRef wrappedObj, structural_objectRef interfaceObj,
                      structural_managerRef SM_minimal_interface);

   DesignFlowStep_Status InternalExec() override;

   /**
    * @brief instantiate_component_parallel
    * @param SM structural manager where the NoC wrapper is instantiated
    * @param wrappedObj wrapped component connected to the generated interface
    */
   void instantiate_noc(const structural_managerRef SM, const structural_objectRef wrappedObj);

   /**
    * @brief add_parameter_port
    * @param SM
    * @param circuit
    * @param top_module
    */
   void add_parameter_port(const structural_managerRef SM, structural_objectRef circuit,
                           structural_objectRef top_module);

   void initialize_bmi(const structural_objectRef wrappedObj);

   structural_objectRef get_smart_signal(const structural_managerRef SM, std::string name, so_kind port_type,
                                         const structural_type_descriptorRef& type, unsigned int vector_size);

   void create_noc_memory_mapping_unit(const structural_objectRef noc_module, const structural_objectRef wrappedObj,
                                       const structural_managerRef SM);

   void create_iob_caches(const structural_objectRef noc_module, const structural_objectRef wrappedObj,
                          const structural_managerRef SM);

   void create_axi_adapter(const structural_objectRef noc_module, const structural_objectRef wrappedObj,
                           const structural_managerRef SM);

   void add_profiling_modules(const structural_managerRef SM);

   HLSRelationships ComputeHLSRelationships(const DesignFlowStep::RelationshipType relationship_type) const override;
};
#endif // BUS_INTERFACE_H
