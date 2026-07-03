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
 * @file bus_interface.cpp
 * @brief Class to generate the interface for the bus
 *
 * @author Giovanni Gozzi <giovanni.gozzi@polimi.it>
 *
 */
#include "bus_interface.hpp"

#include "BambuParameter.hpp"
#include "behavioral_helper.hpp"
#include "call_graph_manager.hpp"
#include "copyrights_strings.hpp"
#include "dbgPrintHelper.hpp"
#include "function_behavior.hpp"
#include "hls.hpp"
#include "hls_device.hpp"
#include "hls_manager.hpp"
#include "ir_helper.hpp"
#include "ir_manager.hpp"
#include "ir_node.hpp"
#include "ir_reindex.hpp"
#include "math_function.hpp"
#include "memory.hpp"
#include "memory_allocation.hpp"
#include "string_manipulation.hpp"
#include "structural_manager.hpp"
#include "structural_objects.hpp"
#include "technology_manager.hpp"

#include <string>

bus_interface::bus_interface(const ParameterConstRef _Param, const HLS_managerRef _HLSMgr, unsigned int _funId,
                             const DesignFlowManager& _design_flow_manager, const HLSFlowStep_Type _hls_flow_step_type)
    : module_interface(_Param, _HLSMgr, _funId, _design_flow_manager, _hls_flow_step_type)
{
   debug_level = parameters->get_class_debug_level(GET_CLASS(*this));
   THROW_ASSERT(funId, "Function not set in minimal interface");
}

HLS_step::HLSRelationships
bus_interface::ComputeHLSRelationships(const DesignFlowStep::RelationshipType relationship_type) const
{
   auto reletionship = module_interface::ComputeHLSRelationships(relationship_type);
   switch(relationship_type)
   {
      case(PRECEDENCE_RELATIONSHIP):
      {
         reletionship.insert(std::make_tuple(HLSFlowStep_Type::MINIMAL_INTERFACE_GENERATION,
                                             HLSFlowStepSpecializationConstRef(),
                                             HLSFlowStep_Relationship::SAME_FUNCTION));
         break;
      }
      case(INVALIDATION_RELATIONSHIP):
      case(DEPENDENCE_RELATIONSHIP):
         break;
      default:
         THROW_UNREACHABLE("");
   }
   return reletionship;
}

structural_objectRef bus_interface::get_smart_signal(const structural_managerRef SM, std::string name,
                                                     so_kind port_type, const structural_type_descriptorRef& type,
                                                     unsigned int vector_size = 0U)
{
   const structural_objectRef circuit = SM->get_circ();
   structural_objectRef result = circuit->find_member(name, port_type, circuit);
   return result ? result :
                   (port_type == signal_o_K ? SM->add_sign(name, circuit, type) :
                                              SM->add_sign_vector(name, vector_size, circuit, type));
}

DesignFlowStep_Status bus_interface::InternalExec()
{
   const structural_managerRef SM = HLS->top;
   if(!SM)
   {
      THROW_ERROR("Top component has not been created yet!");
   }
   const auto TM = HLSMgr->get_ir_manager();

   const auto top_fb = HLSMgr->CGetFunctionBehavior(funId);
   const auto top_bh = top_fb->CGetBehavioralHelper();
   const auto fname = top_bh->GetFunctionName();
   const auto func_arch = HLSMgr->module_arch->GetArchitecture(fname);
   bool is_banked = false;
   bool has_bus_pragma = false;
   if(func_arch && func_arch->ifaces.find("bus") != func_arch->ifaces.end())
   {
      has_bus_pragma = true;
      const auto& bus_if = func_arch->ifaces.at("bus");
      if(bus_if.find(FunctionArchitecture::iface_bank_number) != bus_if.end())
      {
         is_banked = std::stoul((bus_if.find(FunctionArchitecture::iface_bank_number)->second)) > 0U;
      }
   }

   if(has_bus_pragma)
   {
      structural_objectRef wrappedObj = SM->get_circ();
      if(!wrappedObj->find_member("Mout_addr_ram", port_vector_o_K, wrappedObj))
      {
         THROW_ERROR("Function architecture declares a bus interface for " + fname + ", but top component " +
                     wrappedObj->get_path() +
                     " does not expose Mout_addr_ram. Check interface pragma analysis and architecture.xml.");
      }
      std::string module_name = wrappedObj->get_id();

      structural_managerRef SM_bus_interface = structural_managerRef(new structural_manager(parameters));
      const structural_type_descriptorRef internal_type(
          new structural_type_descriptor(module_name + "_minimal_interface"));
      structural_type_descriptorRef module_type =
          structural_type_descriptorRef(new structural_type_descriptor(module_name));
      SM_bus_interface->set_top_info(module_name, module_type);
      wrappedObj->set_type(internal_type);
      structural_objectRef interfaceObj = SM_bus_interface->get_circ();

      // add the core to the wrapper
      wrappedObj->set_owner(interfaceObj);
      wrappedObj->set_id(wrappedObj->get_id() + "_i0");
      GetPointer<module_o>(interfaceObj)->add_internal_object(wrappedObj);
      /// Set some descriptions and legal stuff
      GetPointer<module_o>(interfaceObj)
          ->set_description("Minimal interface for top component: " + wrappedObj->get_typeRef()->id_type);
      GetPointer<module_o>(interfaceObj)->set_copyright(GENERATED_COPYRIGHT);
      GetPointer<module_o>(interfaceObj)->set_authors("Component automatically generated by bambu");
      GetPointer<module_o>(interfaceObj)->set_license(GENERATED_LICENSE);

      add_parameter_port(SM_bus_interface, interfaceObj, wrappedObj); // add ports to external module

      initialize_bmi(wrappedObj);

      if(parameters->getOption<MemoryAllocation_Policy>(OPT_memory_allocation_policy) ==
             MemoryAllocation_Policy::ALL_BRAM ||
         parameters->getOption<MemoryAllocation_Policy>(OPT_memory_allocation_policy) ==
             MemoryAllocation_Policy::EXT_PIPELINED_BRAM)
      {
         if(is_banked)
         {
            THROW_ERROR("The memory allocation policy ALL_BRAM and EXT_PIPELINED_BRAM are not compatible with the "
                        "banked memory. Please use a different policy.");
         }
         if(bi.use_axi)
         {
            THROW_ERROR("The memory allocation policy ALL_BRAM and EXT_PIPELINED_BRAM are not compatible with the "
                        "AXI protocol. Please use a different policy.");
         }
      }

      if(is_banked)
      {
         instantiate_noc(SM_bus_interface, wrappedObj);
      }
      else if(bi.use_caches)
      {
         create_iob_caches(wrappedObj, wrappedObj, SM_bus_interface);
      }
      else if(bi.use_axi)
      {
         create_axi_adapter(wrappedObj, wrappedObj, SM_bus_interface);
      }

      memory::propagate_memory_parameters(HLS->top->get_circ(), SM_bus_interface);
      // Generation completed, the new created module substitutes the current top-level one
      HLS->top = SM_bus_interface;
   }
   return DesignFlowStep_Status::SUCCESS;
}

void bus_interface::add_parameter_port(const structural_managerRef SM, structural_objectRef circuit,
                                       structural_objectRef top_module)
{
   PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, " - Connecting parameter ports");
   for(unsigned int j = 0; j < GetPointer<module_o>(top_module)->get_in_port_size(); j++) // connect in
   {
      auto port_i = GetPointer<module_o>(top_module)->get_in_port(j);
      auto port_i_o = GetPointer<port_o>(port_i);
      if(!port_i_o->get_is_memory())
      {
         std::string port_name = port_i_o->get_id();
         structural_objectRef port_obj;
         if(port_i->get_kind() == so_kind::port_o_K)
         {
            port_obj = SM->add_port(port_name, port_o::IN, circuit, port_i->get_typeRef());
            port_o::resize_std_port(STD_GET_SIZE(port_i->get_typeRef()), 0U, DEBUG_LEVEL_NONE, port_obj);
         }
         else
         {
            port_obj =
                SM->add_port_vector(port_name, port_o::IN, port_i_o->get_ports_size(), circuit, port_i->get_typeRef());
            port_o::resize_std_port(port_i_o->get_ports_size() * STD_GET_SIZE(port_i->get_typeRef()), 0U,
                                    DEBUG_LEVEL_NONE, port_obj);
         }
         port_o::fix_port_properties(port_i, port_obj);
         SM->add_connection(port_obj, port_i);
         PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                       " - Connected port: " + port_obj->get_path() + " to port: " + port_i->get_path());
         PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                       " - Connected port size: " + STR(STD_GET_SIZE(port_obj->get_typeRef())) +
                           " to port size: " + STR(STD_GET_SIZE(port_i->get_typeRef())));
      }
   }
   for(unsigned int j = 0; j < GetPointer<module_o>(top_module)->get_out_port_size(); j++) // connect out
   {
      auto port_i = GetPointer<module_o>(top_module)->get_out_port(j);
      auto port_i_o = GetPointer<port_o>(port_i);
      if(!port_i_o->get_is_memory() && !(port_i->get_id() == DONE_PORT_NAME))
      {
         std::string port_name = port_i_o->get_id();
         structural_objectRef port_obj;
         if(port_i->get_kind() == so_kind::port_o_K)
         {
            port_obj = SM->add_port(port_name, port_o::OUT, circuit, port_i->get_typeRef());
            port_o::resize_std_port(STD_GET_SIZE(port_i->get_typeRef()), 0U, DEBUG_LEVEL_NONE, port_obj);
         }
         else
         {
            port_obj =
                SM->add_port_vector(port_name, port_o::OUT, port_i_o->get_ports_size(), circuit, port_i->get_typeRef());
            port_o::resize_std_port(port_i_o->get_ports_size() * STD_GET_SIZE(port_i->get_typeRef()), 0U,
                                    DEBUG_LEVEL_NONE, port_obj);
         }
         port_o::fix_port_properties(port_i, port_obj);
         SM->add_connection(port_i, port_obj);
         PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                       " - Connected port: " + port_obj->get_path() + " to port: " + port_i->get_path());
         PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                       " - Connected port size: " + STR(STD_GET_SIZE(port_obj->get_typeRef())) +
                           " to port size: " + STR(STD_GET_SIZE(port_i->get_typeRef())));
      }
   }
}

void bus_interface::initialize_bmi(const structural_objectRef wrappedObj)
{
   const auto TM = HLSMgr->get_ir_manager();
   const auto fnode = TM->GetIRNode(funId);
   bi.fname = ir_helper::GetFunctionName(fnode);

   bi.const_use_profiling_modules = parameters->isOption(OPT_noc_profiling);
   const auto FB = HLSMgr->CGetFunctionBehavior(funId);
   const auto BH = FB->CGetBehavioralHelper();
   const auto& parm_attrs = HLSMgr->module_arch->GetArchitecture(bi.fname)->parms;
   const auto params = BH->GetParameters();
   for(auto par_idx = 0U; par_idx < params.size(); ++par_idx)
   {
      const auto& par = params.at(par_idx);
      const auto param = BH->PrintVariable(par->index);
      const auto param_attr = parm_attrs.find(param);
      if(param_attr != parm_attrs.end())
      {
         const auto param_attrs = param_attr->second;
         if(param_attrs.find(FunctionArchitecture::parm_bank_allocation) != param_attrs.end())
         {
            bi.const_use_bank_allocation = true;
         }
      }
   }

   bi.bank_index = 1;
   const auto& ifaces = HLSMgr->module_arch->GetArchitecture(bi.fname)->ifaces;
   THROW_ASSERT(ifaces.find("bus") != ifaces.end(), "bus interface must exist");
   const auto& iface_attrs = ifaces.at("bus");
   bi.use_caches = iface_attrs.find(FunctionArchitecture::iface_cache_line_count) != iface_attrs.end();
   bi.use_axi = iface_attrs.at(FunctionArchitecture::iface_mode) == "m_axi";
   auto bank_number = iface_attrs.find(FunctionArchitecture::iface_bank_number);
   if(bank_number != iface_attrs.end())
   {
      bi.bank_number = static_cast<unsigned int>(std::stoul((bank_number->second)));
   }
   auto bank_index = iface_attrs.find(FunctionArchitecture::iface_chunk_size);
   if(bank_index != iface_attrs.end())
   {
      bi.bank_index = ceil_log2(static_cast<unsigned int>(std::stoul((bank_index->second))));
   }

   bi.channel_number = parameters->getOption<unsigned int>(OPT_channels_number);
   bi.bank_index_bits_used = bi.bank_number > 0 ? ceil_log2(bi.bank_number) : 0;
   bi.noc_nodes = bi.bank_number > 0 ? std::max(bi.channel_number, bi.bank_number) : 0;

   INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, output_level, "-->");
   INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, output_level, "---Bus interface information:");
   INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, output_level, "---Use NoC: " + STR(bi.bank_number > 0 ? "yes" : "no"));
   INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, output_level, "---Use Caches: " + STR(bi.use_caches ? "yes" : "no"));
   INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, output_level, "---Use AXI: " + STR(bi.use_axi > 0 ? "yes" : "no"));
   INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, output_level, "---Number of nodes: " + STR(bi.noc_nodes));
   INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, output_level, "---Number of channels: " + STR(bi.channel_number));
   INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, output_level, "---Bank number: " + STR(bi.bank_number));
   INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, output_level, "---Chunk size: " + STR(1 << bi.bank_index));
   INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, output_level,
                  "---BANK_ID first bit: " + STR(bi.bank_index + bi.bank_index_bits_used));
   INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, output_level, "---BANK_ID bitsize: " + STR(bi.bank_index_bits_used));
   INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, output_level, "---BANK_ID last bit: " + STR(bi.bank_index));
   INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, output_level, "<--");
   bi.noc_id_size = bi.bank_number > 0 ? ceil_log2(bi.noc_nodes) : 0;
   bi.use_registered_noc =
       parameters->IsParameter("registered_noc") ? parameters->GetParameter<bool>("registered_noc") : false;

   structural_objectRef wrapped_addr_ram_port = wrappedObj->find_member("Mout_addr_ram", port_vector_o_K, wrappedObj);
   THROW_ASSERT(wrapped_addr_ram_port, "Mout_addr_ram port must be present in " + wrappedObj->get_path());
   bi.addr_bus_bitsize = wrapped_addr_ram_port->get_typeRef()->vector_size;

   structural_objectRef wrapped_Wdata_ram_port = wrappedObj->find_member("Mout_Wdata_ram", port_vector_o_K, wrappedObj);
   THROW_ASSERT(wrapped_Wdata_ram_port, "Mout_Wdata_ram port must be present in " + wrappedObj->get_path());
   bi.data_bus_bitsize = wrapped_Wdata_ram_port->get_typeRef()->vector_size;

   structural_objectRef wrapped_data_ram_size_port =
       wrappedObj->find_member("Mout_data_ram_size", port_vector_o_K, wrappedObj);
   THROW_ASSERT(wrapped_data_ram_size_port, "Mout_data_ram_size port must be present in " + wrappedObj->get_path());
   bi.size_bus_bitsize = wrapped_data_ram_size_port->get_typeRef()->vector_size;

   structural_objectRef wrapped_tag_port = wrappedObj->find_member("Mout_tag", port_vector_o_K, wrappedObj);
   bi.context_bus_bitsize = wrapped_tag_port ? wrapped_tag_port->get_typeRef()->vector_size : 1U;
   bi.addr_banked_bitsize = bi.addr_bus_bitsize - bi.bank_index_bits_used;
}

void bus_interface::create_noc_memory_mapping_unit(const structural_objectRef noc_module,
                                                   const structural_objectRef wrappedObj,
                                                   const structural_managerRef SM)
{
   structural_objectRef wrapped_addr_ram_port = wrappedObj->find_member("Mout_addr_ram", port_vector_o_K, wrappedObj);
   const auto wrapped_addr_ram_port_o = GetPointer<port_o>(wrapped_addr_ram_port);

   structural_objectRef noc_addr_port = noc_module->find_member("Mout_addr_ram", port_vector_o_K, noc_module);
   const auto noc_addr_port_o = GetPointer<port_o>(noc_addr_port);
   noc_addr_port_o->add_n_ports(static_cast<unsigned int>(bi.noc_nodes), noc_addr_port);
   port_o::resize_std_port(bi.addr_banked_bitsize, 0U, 0, noc_addr_port);

   structural_objectRef noc_id_dest_port = noc_module->find_member("node_id_dest", port_vector_o_K, noc_module);
   const auto noc_id_dest_port_o = GetPointer<port_o>(noc_id_dest_port);
   noc_id_dest_port_o->add_n_ports(static_cast<unsigned int>(bi.noc_nodes), noc_id_dest_port);
   port_o::resize_std_port(bi.noc_id_size, 0U, 0, noc_id_dest_port);

   std::string noc_mapping_unit_library = HLS->HLS_D->get_technology_manager()->get_library("noc_mapping_unit");
   std::string noc_addr_adapter_library = HLS->HLS_D->get_technology_manager()->get_library("noc_addr_adapter");

   std::string bank_allocation_default_string;
   std::set<std::string> inserted_bundles;

   const auto SplitString = [&](std::string s, std::string delimiter) -> std::vector<std::string> {
      size_t pos_start = 0, pos_end, delim_len = delimiter.length();
      std::string token;
      std::vector<std::string> res;

      while((pos_end = s.find(delimiter, pos_start)) != std::string::npos)
      {
         token = s.substr(pos_start, pos_end - pos_start);
         pos_start = pos_end + delim_len;
         res.push_back(token);
      }

      res.push_back(s.substr(pos_start));
      return res;
   };

   for(unsigned int i = 0; i < bi.bank_number; i++)
   {
      if(i > 0)
      {
         bank_allocation_default_string += ",";
      }
      bank_allocation_default_string += STR(i);
   }
   HLSMgr->bundle_required.push_back(
       std::string(bank_allocation_default_string.begin(), bank_allocation_default_string.end()));
   inserted_bundles.insert(bank_allocation_default_string);

   const structural_objectRef circuit = SM->get_circ();
   if(bi.const_use_bank_allocation)
   {
      const auto& func_arch = HLSMgr->module_arch->GetArchitecture(bi.fname);
      THROW_ASSERT(func_arch, "Expected interface architecture for function " + bi.fname);

      const ir_managerConstRef TM = HLSMgr->get_ir_manager();
      const auto top_function_ids = HLSMgr->CGetCallGraphManager().GetRootFunctions();
      THROW_ASSERT(top_function_ids.size() == 1, "Multiple top functions");
      const auto function_id = *(top_function_ids.begin());
      const BehavioralHelperConstRef behavioral_helper =
          HLSMgr->GetFunctionBehavior(function_id)->GetBehavioralHelper();
      const std::list<unsigned int>& func_parameters = behavioral_helper->get_parameters();

      for(auto& p : func_parameters)
      {
         std::string param_name = behavioral_helper->PrintVariable(p);
         std::string bank_allocation_string;

         const auto& param = func_arch->parms.find(param_name);
         if(param != func_arch->parms.end())
         {
            const auto& parm_attrs = param->second;
            const auto& iface_attrs = func_arch->ifaces.at(parm_attrs.at(FunctionArchitecture::parm_bundle));
            const auto iface_mode = iface_attrs.at(FunctionArchitecture::iface_mode);
            const auto arg_typename = parm_attrs.at(FunctionArchitecture::parm_original_typename);
            THROW_ASSERT(arg_typename != "", "Param " + param_name + " has no type");
            const auto is_pointer_type = arg_typename.back() == '*';
            const auto is_reference_type = arg_typename.back() == '&';
            // param is allocated on the bus
            if(iface_mode == "default" && (is_pointer_type || is_reference_type))
            {
               const auto& bank_allocation_value = parm_attrs.find(FunctionArchitecture::parm_bank_allocation);
               if(bank_allocation_value != parm_attrs.end())
               {
                  bank_allocation_string = bank_allocation_value->second;
               }
               else
               {
                  bank_allocation_string = bank_allocation_default_string;
               }
               if(inserted_bundles.find(bank_allocation_string) == inserted_bundles.end())
               {
                  HLSMgr->bundle_required.push_back(bank_allocation_string);
                  INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, output_level, "Added bundle " + bank_allocation_string);
                  inserted_bundles.insert(bank_allocation_string);
               }
               std::deque<std::string>::iterator itr;
               itr = std::find(HLSMgr->bundle_required.begin(), HLSMgr->bundle_required.end(), bank_allocation_string);
               HLSMgr->bundle_map[param_name] =
                   static_cast<unsigned int>(std::distance(HLSMgr->bundle_required.begin(), itr));

               INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, output_level,
                              "Bank allocation: " + param_name + " -> banks: " + bank_allocation_string);
            }
         }
         else
         {
            THROW_UNREACHABLE("This point should not be reached.");
         }
      }
   }

   const auto bundle_number = static_cast<unsigned int>(HLSMgr->bundle_required.size());
   const auto bundle_bitsize = std::max(1U, ceil_log2(bundle_number));
   const auto external_base_addr = parameters->getOption<unsigned long long int>(OPT_base_address);
   const auto external_bit = ceil_log2(external_base_addr);
   const auto bundle_bit = external_bit - bundle_bitsize;

   INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, output_level, "-->");
   INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, output_level, "---Address encode information:");
   INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, output_level, "---Address size: " + STR(bi.addr_bus_bitsize));
   INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, output_level, "---External bit: " + STR(external_bit));
   INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, output_level, "---Bundle bit: " + STR(bundle_bit));
   INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, output_level, "---Bundle bitsize: " + STR(bundle_bitsize));
   INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, output_level, "---Bundle number: " + STR(bundle_number));
   INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, output_level, "<--");

   for(auto i = 0U; i < bi.noc_nodes; i++)
   {
      if(i < bi.channel_number)
      {
         structural_objectRef node_id_dest_signal_out =
             get_smart_signal(SM, "node_id_dest_signal_" + STR(i), signal_o_K, noc_id_dest_port->get_typeRef());
         structural_objectRef addr_signal_out =
             get_smart_signal(SM, "sig_in_Mout_addr_ram_signal_" + STR(i), signal_o_K, noc_addr_port->get_typeRef());
         SM->add_connection(addr_signal_out, noc_addr_port_o->get_port(i));
         structural_objectRef addr_signal =
             get_smart_signal(SM, "sig_in_Mout_addr_ram_" + STR(i), signal_o_K, wrapped_addr_ram_port->get_typeRef());
         SM->add_connection(addr_signal,
                            bi.channel_number > 1 ? wrapped_addr_ram_port_o->get_port(i) : wrapped_addr_ram_port);

         structural_objectRef noc_mapping_unit_in_port, noc_addr_map_in_port;

         if(bundle_number > 1)
         {
            // There are multiple bundle so we need to choose between them using a noc_mapping_unit
            std::string noc_mapping_unit_name = "top_level_mapping_unit_id_dest_" + STR(i);
            structural_objectRef noc_mapping_unit_module = SM->add_module_from_technology_library(
                noc_mapping_unit_name, "noc_mapping_unit", noc_mapping_unit_library, circuit,
                HLS->HLS_D->get_technology_manager());

            noc_mapping_unit_module->SetParameter("SELECTOR_START", STR(bundle_bit + ceil_log2(bundle_number)));
            noc_mapping_unit_module->SetParameter("SELECTOR_END", STR(bundle_bit));

            structural_objectRef selector_port =
                noc_mapping_unit_module->find_member("selector", port_o_K, noc_mapping_unit_module);
            port_o::resize_std_port(bi.addr_bus_bitsize, 0U, 0, selector_port);
            SM->add_connection(addr_signal, selector_port);

            structural_objectRef out_port =
                noc_mapping_unit_module->find_member("out1", port_o_K, noc_mapping_unit_module);
            port_o::resize_std_port(bi.noc_id_size, 0U, 0, out_port);
            SM->add_connection(node_id_dest_signal_out, noc_id_dest_port_o->get_port(i));
            SM->add_connection(node_id_dest_signal_out, out_port);

            noc_mapping_unit_in_port =
                noc_mapping_unit_module->find_member("in1", port_vector_o_K, noc_mapping_unit_module);
            const auto noc_mapping_unit_in_port_o = GetPointer<port_o>(noc_mapping_unit_in_port);
            noc_mapping_unit_in_port_o->add_n_ports(bundle_number, noc_mapping_unit_in_port);
            port_o::resize_std_port(bi.noc_id_size, 0U, 0, noc_mapping_unit_in_port);

            // We also need to choose the right bundle_adapter using a noc_mapping_unit
            std::string noc_bundle_adapter_map_name = "top_level_addr_adapter_map_" + STR(i);
            structural_objectRef noc_bundle_adapter_map_module = SM->add_module_from_technology_library(
                noc_bundle_adapter_map_name, "noc_mapping_unit", noc_mapping_unit_library, circuit,
                HLS->HLS_D->get_technology_manager());

            noc_bundle_adapter_map_module->SetParameter("SELECTOR_START", STR(bundle_bit + ceil_log2(bundle_number)));
            noc_bundle_adapter_map_module->SetParameter("SELECTOR_END", STR(bundle_bit));

            selector_port =
                noc_bundle_adapter_map_module->find_member("selector", port_o_K, noc_bundle_adapter_map_module);
            port_o::resize_std_port(bi.addr_bus_bitsize, 0U, 0, selector_port);
            SM->add_connection(addr_signal, selector_port);

            out_port = noc_bundle_adapter_map_module->find_member("out1", port_o_K, noc_bundle_adapter_map_module);
            port_o::resize_std_port(bi.addr_banked_bitsize, 0U, 0, out_port);
            SM->add_connection(addr_signal_out, out_port);

            noc_addr_map_in_port =
                noc_bundle_adapter_map_module->find_member("in1", port_vector_o_K, noc_bundle_adapter_map_module);
            const auto noc_addr_map_in_port_o = GetPointer<port_o>(noc_addr_map_in_port);
            noc_addr_map_in_port_o->add_n_ports(bundle_number, noc_addr_map_in_port);
            port_o::resize_std_port(bi.addr_banked_bitsize, 0U, 0, noc_addr_map_in_port);
         }
         unsigned int index = 0U;
         for(const auto& p : HLSMgr->bundle_required)
         {
            const auto splitted_list = SplitString(p, ",");
            std::vector<unsigned int> bank_identifier;
            std::transform(splitted_list.begin(), splitted_list.end(), std::back_inserter(bank_identifier),
                           [](const std::string& str) { return std::stoul(str); });
            const auto bundle_bank_number = static_cast<unsigned int>(bank_identifier.size());
            const auto bundle_bank_bitsize = bundle_bank_number == 1 ? 0U : ceil_log2(bundle_bank_number);
            const auto bundle_bank_number_effected = pow(2, bundle_bank_bitsize);

            if(bundle_bank_number == 1)
            {
               structural_objectRef const_obj =
                   SM->add_constant("const_node_bank_" + STR(i) + "_" + STR(index), circuit,
                                    noc_id_dest_port->get_typeRef(), STR(bank_identifier[0]));
               SM->add_connection(bundle_number > 1 ? GetPointer<port_o>(noc_mapping_unit_in_port)->get_port(index) :
                                                      noc_id_dest_port_o->get_port(i),
                                  const_obj);
            }
            else
            {
               std::string noc_mapping_unit_name = "second_level_mapping_unit_id_dest_" + STR(i) + "_" + STR(index);
               structural_objectRef noc_mapping_unit_module = SM->add_module_from_technology_library(
                   noc_mapping_unit_name, "noc_mapping_unit", noc_mapping_unit_library, circuit,
                   HLS->HLS_D->get_technology_manager());

               noc_mapping_unit_module->SetParameter("SELECTOR_START",
                                                     STR(bi.bank_index + ceil_log2(bundle_bank_number)));
               noc_mapping_unit_module->SetParameter("SELECTOR_END", STR(bi.bank_index));

               structural_objectRef selector_port =
                   noc_mapping_unit_module->find_member("selector", port_o_K, noc_mapping_unit_module);
               port_o::resize_std_port(bi.addr_bus_bitsize, 0U, 0, selector_port);
               SM->add_connection(addr_signal, selector_port);

               structural_objectRef out_port =
                   noc_mapping_unit_module->find_member("out1", port_o_K, noc_mapping_unit_module);
               port_o::resize_std_port(bi.noc_id_size, 0U, 0, out_port);
               if(bundle_number > 1)
               {
                  structural_objectRef signal_second_level_noc_out =
                      SM->add_sign("signal_second_level_noc_mapping_unit_out_" + STR(i) + "_" + STR(index), circuit,
                                   out_port->get_typeRef());
                  SM->add_connection(signal_second_level_noc_out, out_port);
                  SM->add_connection(signal_second_level_noc_out,
                                     GetPointer<port_o>(noc_mapping_unit_in_port)->get_port(index));
               }
               else
               {
                  SM->add_connection(node_id_dest_signal_out, noc_id_dest_port_o->get_port(i));
                  SM->add_connection(node_id_dest_signal_out, out_port);
               }

               const auto inner_level_noc_mapping_unit_in_port =
                   noc_mapping_unit_module->find_member("in1", port_vector_o_K, noc_mapping_unit_module);
               const auto inner_level_noc_mapping_unit_in_port_o =
                   GetPointer<port_o>(inner_level_noc_mapping_unit_in_port);
               inner_level_noc_mapping_unit_in_port_o->add_n_ports(bundle_bank_number,
                                                                   inner_level_noc_mapping_unit_in_port);
               port_o::resize_std_port(bi.noc_id_size, 0U, 0, inner_level_noc_mapping_unit_in_port);

               for(unsigned int j = 0; j < bundle_bank_number_effected; j++)
               {
                  structural_objectRef const_obj =
                      SM->add_constant("const_node_bank_" + STR(i) + "_" + STR(index) + "_" + STR(j), circuit,
                                       noc_id_dest_port->get_typeRef(), STR(bank_identifier[j % bundle_bank_number]));
                  SM->add_connection(inner_level_noc_mapping_unit_in_port_o->get_port(j), const_obj);
               }
            }

            std::string noc_addr_adapter_name = "noc_addr_adapter_" + STR(i) + "_" + STR(index);
            structural_objectRef noc_addr_adapter_module = SM->add_module_from_technology_library(
                noc_addr_adapter_name, "noc_addr_adapter", noc_addr_adapter_library, circuit,
                HLS->HLS_D->get_technology_manager());
            noc_addr_adapter_module->SetParameter("BANK_START", STR(bi.bank_index + bundle_bank_bitsize));
            noc_addr_adapter_module->SetParameter("BANK_END", STR(bi.bank_index));
            noc_addr_adapter_module->SetParameter("BUNDLE_START", STR(bundle_bit));
            noc_addr_adapter_module->SetParameter("BUNDLE_END",
                                                  STR(bundle_bit - (bi.bank_index_bits_used - bundle_bank_bitsize)));

            structural_objectRef out_port =
                noc_addr_adapter_module->find_member("out1", port_o_K, noc_addr_adapter_module);
            port_o::resize_std_port(bi.addr_banked_bitsize, 0U, 0, out_port);
            if(bundle_number == 1)
            {
               SM->add_connection(addr_signal_out, out_port);
            }
            else
            {
               structural_objectRef out_signal =
                   get_smart_signal(SM, "sig_in_Mout_addr_ram_signal_" + STR(i) + "_" + STR(index), signal_o_K,
                                    noc_addr_port->get_typeRef());
               SM->add_connection(out_signal, out_port);
               SM->add_connection(out_signal, GetPointer<port_o>(noc_addr_map_in_port)->get_port(index));
            }

            structural_objectRef in_port =
                noc_addr_adapter_module->find_member("in1", port_vector_o_K, noc_addr_adapter_module);
            port_o::resize_std_port(bi.addr_bus_bitsize, 0U, 0, in_port);
            SM->add_connection(addr_signal, in_port);
            index++;
         }
      }
      else
      {
         structural_objectRef const_obj =
             SM->add_constant("const_node_id_" + STR(i), circuit, noc_id_dest_port->get_typeRef(), STR(0));
         SM->add_connection(noc_id_dest_port_o->get_port(i), const_obj);
         const_obj = SM->add_constant("const_node_addr_" + STR(i), circuit, noc_addr_port->get_typeRef(), STR(0));
         SM->add_connection(noc_addr_port_o->get_port(i), const_obj);
      }
   }
}

void bus_interface::create_iob_caches(const structural_objectRef input_module, const structural_objectRef wrappedObj,
                                      const structural_managerRef SM)
{
   const structural_objectRef circuit = SM->get_circ();
   const std::vector<std::tuple<std::string, std::string, unsigned int>> noc_adapter_ports = {
       {"Mout_oe_ram_out", "Mout_oe_ram", 1U},
       {"Mout_we_ram_out", "Mout_we_ram", 1U},
       {"Mout_addr_ram_out", "Mout_addr_ram", bi.addr_banked_bitsize},
       {"Mout_Wdata_ram_out", "Mout_Wdata_ram", bi.data_bus_bitsize},
       {"Mout_data_ram_size_out", "Mout_data_ram_size", bi.size_bus_bitsize},
       {"Mout_tag_out", "Mout_tag", bi.context_bus_bitsize + bi.noc_id_size},
       {"Mout_back_pressure_out", "Mout_back_pressure", 1U},
       {"M_back_pressure", "M_back_pressure", 1U},
       {"M_DataRdy", "M_DataRdy", 1U},
       {"M_Rdata_ram", "M_Rdata_ram", bi.data_bus_bitsize},
       {"M_tag", "M_tag", bi.context_bus_bitsize + bi.noc_id_size}};

   const std::vector<std::tuple<std::string, std::string, unsigned int>> minimal_adapter_ports = {
       {"Mout_oe_ram", "Mout_oe_ram", 1U},
       {"Mout_we_ram", "Mout_we_ram", 1U},
       {"Mout_addr_ram", "Mout_addr_ram", bi.addr_banked_bitsize},
       {"Mout_Wdata_ram", "Mout_Wdata_ram", bi.data_bus_bitsize},
       {"Mout_data_ram_size", "Mout_data_ram_size", bi.size_bus_bitsize},
       {"Mout_back_pressure", "Mout_back_pressure", 1U},
       {"M_DataRdy", "M_DataRdy", 1U},
       {"M_Rdata_ram", "M_Rdata_ram", bi.data_bus_bitsize}};

   const std::vector<std::tuple<std::string, std::string, unsigned int>> adapter_cache_ports = {
       {"dirty", "dirty", 1U},
       {"flush", "flush", 1U},
       {"valid", "valid", 1U},
       {"addr", "addr", bi.addr_banked_bitsize - ceil_log2(bi.data_bus_bitsize / 8)},
       {"wdata", "wdata", bi.data_bus_bitsize},
       {"wstrb", "wstrb", bi.data_bus_bitsize / 8},
       {"rdata", "rdata", bi.data_bus_bitsize},
       {"ready", "ready", 1U}};

   const bool is_banked = bi.bank_number > 0;
   INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level,
                  "Creating IOB caches for" + STR(is_banked ? " banked" : "") + " bus with " +
                      STR(is_banked ? bi.bank_number : bi.channel_number) + " channels");

   auto done_port_obj = wrappedObj->find_member(DONE_PORT_NAME, port_o_K, wrappedObj);
   THROW_ASSERT(done_port_obj, "Must be present.\n");
   auto done_signal_input = get_smart_signal(SM, "sig_in_done_component", signal_o_K, done_port_obj->get_typeRef());
   SM->add_connection(done_port_obj, done_signal_input);

   std::string join_name = "join_done";
   std::string join_library = HLS->HLS_D->get_technology_manager()->get_library("SIMPLEJOIN2_FU");
   structural_objectRef join_module = SM->add_module_from_technology_library(
       join_name, "SIMPLEJOIN2_FU", join_library, circuit, HLS->HLS_D->get_technology_manager());
   THROW_ASSERT(join_module, "Must be present.\n");

   SM->add_connection(circuit->find_member(CLOCK_PORT_NAME, port_o_K, circuit),
                      join_module->find_member(CLOCK_PORT_NAME, port_o_K, join_module));
   SM->add_connection(circuit->find_member(RESET_PORT_NAME, port_o_K, circuit),
                      join_module->find_member(RESET_PORT_NAME, port_o_K, join_module));
   auto join_input_port = join_module->find_member("in1", port_o_K, join_module);
   THROW_ASSERT(join_input_port, "Must be present.\n");
   auto join_input_port_o = GetPointer<port_o>(join_input_port);
   join_input_port_o->add_n_ports(bi.bank_number, join_input_port);
   port_o::resize_std_port(1U, 0U, 0, join_input_port);

   auto join_output_port = join_module->find_member("out1", port_o_K, join_module);
   THROW_ASSERT(join_output_port, "Must be present.\n");
   port_o::resize_std_port(1U, 0U, 0, join_output_port);

   structural_objectRef done_external_port = SM->add_port(
       DONE_PORT_NAME, port_o::OUT, circuit, structural_type_descriptorRef(new structural_type_descriptor("bool", 1U)));
   port_o::resize_std_port(1U, 0U, 0, done_external_port);

   auto done_output_sig = SM->add_sign("sig_out_done", circuit, join_output_port->get_typeRef());
   SM->add_connection(done_output_sig, join_output_port);
   SM->add_connection(done_output_sig, done_external_port);

   const auto adapter_number = is_banked ? bi.bank_number : bi.channel_number;
   for(unsigned int i = 0; i < adapter_number; i = i + 1)
   {
      std::string adapter_name = "adapter_" + STR(i);
      std::string adapter_library = HLS->HLS_D->get_technology_manager()->get_library("MinimalIOBAdapter");
      structural_objectRef adapter_module = SM->add_module_from_technology_library(
          adapter_name, "MinimalIOBAdapter", adapter_library, circuit, HLS->HLS_D->get_technology_manager());
      SM->add_connection(circuit->find_member(CLOCK_PORT_NAME, port_o_K, circuit),
                         adapter_module->find_member(CLOCK_PORT_NAME, port_o_K, adapter_module));
      SM->add_connection(circuit->find_member(RESET_PORT_NAME, port_o_K, circuit),
                         adapter_module->find_member(RESET_PORT_NAME, port_o_K, adapter_module));

      for(const auto& p : (is_banked ? noc_adapter_ports : minimal_adapter_ports))
      {
         const auto pname_input = std::get<0>(p);
         const auto pname_adapter = std::get<1>(p);
         const auto size = std::get<2>(p);

         structural_objectRef input_port = input_module->find_member(pname_input, port_vector_o_K, input_module);
         THROW_ASSERT(input_port, pname_input + " port must be present");
         const auto input_port_o = GetPointer<port_o>(input_port);
         if((i == 0) && is_banked)
         {
            // Noc output ports must be resized
            input_port_o->add_n_ports(bi.noc_nodes, input_port);
            port_o::resize_std_port(size, 0U, 0, input_port);
         }

         structural_objectRef adapter_port = adapter_module->find_member(pname_adapter, port_o_K, adapter_module);
         THROW_ASSERT(adapter_port, pname_adapter + " port must be present");
         port_o::resize_std_port(size, 0U, 0, adapter_port);

         auto sig = SM->add_sign("sig_out_" + pname_input + "_" + STR(i), circuit, adapter_port->get_typeRef());
         SM->add_connection(sig, (adapter_number > 1 ? input_port_o->get_port(i) : input_port));
         SM->add_connection(sig, adapter_port);
      }

      if(!is_banked)
      {
         structural_objectRef bp_port = adapter_module->find_member("M_back_pressure", port_vector_o_K, adapter_module);
         if(bp_port)
         {
            structural_objectRef const_obj_bp =
                SM->add_constant("null_value_M_back_pressure_" + STR(i), circuit, bp_port->get_typeRef(), STR(0));
            SM->add_connection(bp_port, const_obj_bp);
         }
         structural_objectRef ac_port = adapter_module->find_member("Mout_tag", port_vector_o_K, adapter_module);
         if(ac_port)
         {
            structural_objectRef const_obj_ac =
                SM->add_constant("null_value_Mout_tag_" + STR(i), circuit, ac_port->get_typeRef(), STR(0));
            SM->add_connection(ac_port, const_obj_ac);
         }
      }

      std::string cache_name = "cache_" + STR(i);
      std::string cache_library = HLS->HLS_D->get_technology_manager()->get_library("IOB_cache_axi");
      structural_objectRef cache_module = SM->add_module_from_technology_library(
          cache_name, "IOB_cache_axi", cache_library, circuit, HLS->HLS_D->get_technology_manager());
      SM->add_connection(circuit->find_member(CLOCK_PORT_NAME, port_o_K, circuit),
                         cache_module->find_member(CLOCK_PORT_NAME, port_o_K, cache_module));
      SM->add_connection(circuit->find_member(RESET_PORT_NAME, port_o_K, circuit),
                         cache_module->find_member(RESET_PORT_NAME, port_o_K, cache_module));

      auto cache_module_mod = GetPointer<module_o>(cache_module);

      std::string line_count = "1";
      std::string bus_size = "0";
      std::string n_ways = "1";
      std::string word_off_w = "1";
      long long unsigned int wtbuf_depth_w = 1ULL;
      std::string rep_policy;
      std::string write_policy;

      const auto& iface_attrs = HLSMgr->module_arch->GetArchitecture(bi.fname)->ifaces.at("bus");
      if(auto it = iface_attrs.find(FunctionArchitecture::iface_cache_line_count); it != iface_attrs.end())
      {
         line_count = std::to_string(ceil_log2(std::stoull(it->second)));
      }
      if(auto it = iface_attrs.find(FunctionArchitecture::iface_cache_line_size); it != iface_attrs.end())
      {
         word_off_w = std::to_string(ceil_log2(std::stoull(it->second)));
      }
      if(auto it = iface_attrs.find(FunctionArchitecture::iface_cache_bus_size); it != iface_attrs.end())
      {
         bus_size = it->second;
      }
      if(auto it = iface_attrs.find(FunctionArchitecture::iface_cache_ways); it != iface_attrs.end())
      {
         n_ways = it->second;
      }
      if(auto it = iface_attrs.find(FunctionArchitecture::iface_cache_num_write_outstanding); it != iface_attrs.end())
      {
         wtbuf_depth_w = ceil_log2(std::stoull(it->second));
         if(wtbuf_depth_w < 1)
         {
            wtbuf_depth_w = 1;
         }
      }
      if(auto it = iface_attrs.find(FunctionArchitecture::iface_cache_rep_policy); it != iface_attrs.end())
      {
         const auto rp_name = boost::to_upper_copy(it->second);
         if(rp_name == "LRU")
         {
            rep_policy = "0";
         }
         else if(rp_name == "MRU")
         {
            rep_policy = "1";
         }
         else if(rp_name == "TREE")
         {
            rep_policy = "2";
         }
         else
         {
            THROW_ERROR("Unexpected cache replacement policy: " + it->second);
         }
      }
      if(auto it = iface_attrs.find(FunctionArchitecture::iface_cache_write_policy); it != iface_attrs.end())
      {
         const auto wp_name = boost::to_upper_copy(it->second);
         if(wp_name == "WT")
         {
            write_policy = "0";
         }
         else if(wp_name == "WB")
         {
            write_policy = "1";
         }
         else
         {
            THROW_ERROR("Unexpected cache write policy: " + it->second);
         }
      }

      THROW_ASSERT((1ULL << std::stoull(word_off_w)) * bi.data_bus_bitsize >= std::stoull(bus_size),
                   "ERROR: Cache line of " + STR((1ULL << std::stoull(word_off_w)) * bi.data_bus_bitsize) +
                       " bits is smaller than bus size (" + STR(bus_size) + ")");

      cache_module_mod->SetParameter("FE_ADDR_W", STR(bi.addr_banked_bitsize));
      cache_module_mod->SetParameter("BE_ADDR_W", STR(bi.addr_banked_bitsize));
      cache_module_mod->SetParameter("BE_DATA_W", bus_size);
      cache_module_mod->SetParameter("N_WAYS", n_ways);
      cache_module_mod->SetParameter("LINE_OFF_W", line_count);
      cache_module_mod->SetParameter("WORD_OFF_W", word_off_w);
      cache_module_mod->SetParameter("WTBUF_DEPTH_W", STR(wtbuf_depth_w));
      cache_module_mod->SetParameter("REP_POLICY", STR(strcmp(rep_policy.c_str(), "tree") == 0 ? 2 :
                                                       strcmp(rep_policy.c_str(), "mru") == 0  ? 1 :
                                                                                                 0));
      cache_module_mod->SetParameter("WRITE_POL", STR(strcmp(write_policy.c_str(), "wb") == 0 ? 1 : 0));
      cache_module_mod->SetParameter("AXI_ID", STR(0));
      cache_module_mod->SetParameter("CTRL_CACHE", STR(1));
      cache_module_mod->SetParameter("CTRL_CNT", STR(1));

      for(const auto& p : adapter_cache_ports)
      {
         const auto pname_adapter = std::get<0>(p);
         const auto pname_cache = std::get<1>(p);
         const auto size = std::get<2>(p);

         structural_objectRef adapter_port =
             adapter_module->find_member(pname_adapter, port_vector_o_K, adapter_module);
         THROW_ASSERT(adapter_port, pname_adapter + " port must be present");
         port_o::resize_std_port(size, 0U, 0, adapter_port);

         structural_objectRef cache_port = cache_module->find_member(pname_cache, port_o_K, cache_module);
         THROW_ASSERT(cache_port, pname_cache + " port must be present");
         port_o::resize_std_port(size, 0U, 0, cache_port);

         auto sig = SM->add_sign("sig_out_" + pname_cache + "_" + STR(i), circuit, adapter_port->get_typeRef());
         SM->add_connection(sig, cache_port);
         SM->add_connection(sig, adapter_port);
      }

      structural_objectRef m_axi_awaddr_port = cache_module->find_member("m_axi_awaddr", port_o_K, cache_module);
      THROW_ASSERT(m_axi_awaddr_port, "m_axi_awaddr port must be present");
      port_o::resize_std_port(bi.addr_banked_bitsize, 0U, 0, m_axi_awaddr_port);

      structural_objectRef m_axi_awlen_port = cache_module->find_member("m_axi_awlen", port_o_K, cache_module);
      THROW_ASSERT(m_axi_awlen_port, "m_axi_awlen port must be present");
      port_o::resize_std_port(8U, 0U, 0, m_axi_awlen_port);

      structural_objectRef m_axi_wdata_port = cache_module->find_member("m_axi_wdata", port_o_K, cache_module);
      THROW_ASSERT(m_axi_wdata_port, "m_axi_wdata port must be present");
      port_o::resize_std_port(std::stoull(bus_size), 0U, 0, m_axi_wdata_port);

      structural_objectRef m_axi_wstrb_port = cache_module->find_member("m_axi_wstrb", port_o_K, cache_module);
      THROW_ASSERT(m_axi_wstrb_port, "m_axi_wstrb port must be present");
      port_o::resize_std_port(std::stoull(bus_size) / 8, 0U, 0, m_axi_wstrb_port);

      structural_objectRef m_axi_araddr_port = cache_module->find_member("m_axi_araddr", port_o_K, cache_module);
      THROW_ASSERT(m_axi_araddr_port, "m_axi_araddr port must be present");
      port_o::resize_std_port(bi.addr_banked_bitsize, 0U, 0, m_axi_araddr_port);

      structural_objectRef m_axi_arlen_port = cache_module->find_member("m_axi_arlen", port_o_K, cache_module);
      THROW_ASSERT(m_axi_arlen_port, "m_axi_arlen port must be present");
      port_o::resize_std_port(8U, 0U, 0, m_axi_arlen_port);

      structural_objectRef m_axi_rdata_port = cache_module->find_member("m_axi_rdata", port_o_K, cache_module);
      THROW_ASSERT(m_axi_rdata_port, "m_axi_wdata port must be present");
      port_o::resize_std_port(std::stoull(bus_size), 0U, 0, m_axi_rdata_port);

      for(unsigned int index = 0; index < cache_module_mod->get_in_port_size(); index++)
      {
         const auto port = cache_module_mod->get_in_port(index);
         const auto port_obj = GetPointer<port_o>(port);
         const auto pname = port_obj->get_id();

         if(!port_obj->get_connections_size())
         {
            structural_objectRef external_port;
            std::string external_port_name = "m_axi_" + STR(i) + pname.substr(5);
            external_port = SM->add_port(external_port_name, port_o::IN, circuit, port->get_typeRef());
            port_o::fix_port_properties(port, external_port);
            auto external_port_o = GetPointer<port_o>(external_port);
            external_port_o->set_is_memory(true);

            SM->add_connection(external_port, port);
         }
      }

      for(unsigned int index = 0; index < cache_module_mod->get_out_port_size(); index++)
      {
         const auto port = cache_module_mod->get_out_port(index);
         const auto port_obj = GetPointer<port_o>(port);
         const auto pname = port_obj->get_id();

         if(!port_obj->get_connections_size())
         {
            structural_objectRef external_port;
            std::string external_port_name = "m_axi_" + STR(i) + pname.substr(5);
            external_port = SM->add_port(external_port_name, port_o::OUT, circuit, port->get_typeRef());
            port_o::fix_port_properties(port, external_port);
            auto external_port_o = GetPointer<port_o>(external_port);
            external_port_o->set_is_memory(true);

            SM->add_connection(external_port, port);
         }
      }

      auto done_port_cache_input = adapter_module->find_member("done_input", port_o_K, adapter_module);
      THROW_ASSERT(done_port_cache_input, "Must be present.\n");
      SM->add_connection(done_signal_input, done_port_cache_input);

      auto done_port_cache_output = adapter_module->find_member("done", port_o_K, adapter_module);
      THROW_ASSERT(done_port_cache_output, "Must be present.\n");

      auto done_out_sig = SM->add_sign("sig_loc_done_cache_output_" + STR(i), circuit, join_input_port->get_typeRef());
      SM->add_connection(done_out_sig, join_input_port_o->get_port(i));
      SM->add_connection(done_out_sig, done_port_cache_output);
   }

   for(unsigned int j = bi.bank_number; j < bi.noc_nodes; j++)
   {
      for(const auto& p : noc_adapter_ports)
      {
         const auto pname_noc = std::get<0>(p);

         structural_objectRef noc_port = input_module->find_member(pname_noc, port_vector_o_K, input_module);
         THROW_ASSERT(noc_port, pname_noc + " port must be present");
         const auto noc_port_o = GetPointer<port_o>(noc_port);

         if(noc_port_o->get_port_direction() == port_o::IN)
         {
            structural_objectRef const_obj =
                SM->add_constant("null_value_" + pname_noc + "_" + STR(j), circuit, noc_port->get_typeRef(), STR(0));
            SM->add_connection(noc_port_o->get_port(j), const_obj);
         }
         else
         {
            auto signal = SM->add_sign(pname_noc + "_signal_" + STR(j), circuit, noc_port->get_typeRef());
            SM->add_connection(signal, noc_port_o->get_port(j));
         }
      }
   }
}

void bus_interface::create_axi_adapter(const structural_objectRef input_module, const structural_objectRef wrappedObj,
                                       const structural_managerRef SM)
{
   const structural_objectRef circuit = SM->get_circ();
   const std::vector<std::tuple<std::string, std::string, unsigned int>> noc_adapter_ports = {
       {"Mout_oe_ram_out", "Mout_oe_ram", 1U},
       {"Mout_we_ram_out", "Mout_we_ram", 1U},
       {"Mout_addr_ram_out", "Mout_addr_ram", bi.addr_banked_bitsize},
       {"Mout_Wdata_ram_out", "Mout_Wdata_ram", bi.data_bus_bitsize},
       {"Mout_data_ram_size_out", "Mout_data_ram_size", bi.size_bus_bitsize},
       {"Mout_tag_out", "Mout_tag", bi.context_bus_bitsize + bi.noc_id_size},
       {"Mout_back_pressure_out", "Mout_back_pressure", 1U},
       {"M_back_pressure", "M_back_pressure", 1U},
       {"M_DataRdy", "M_DataRdy", 1U},
       {"M_Rdata_ram", "M_Rdata_ram", bi.data_bus_bitsize},
       {"M_tag", "M_tag", bi.context_bus_bitsize + bi.noc_id_size}};

   const std::vector<std::tuple<std::string, std::string, unsigned int>> minimal_adapter_ports = {
       {"Mout_oe_ram", "Mout_oe_ram", 1U},
       {"Mout_we_ram", "Mout_we_ram", 1U},
       {"Mout_addr_ram", "Mout_addr_ram", bi.addr_banked_bitsize},
       {"Mout_Wdata_ram", "Mout_Wdata_ram", bi.data_bus_bitsize},
       {"Mout_data_ram_size", "Mout_data_ram_size", bi.size_bus_bitsize},
       {"Mout_back_pressure", "Mout_back_pressure", 1U},
       {"M_DataRdy", "M_DataRdy", 1U},
       {"M_Rdata_ram", "M_Rdata_ram", bi.data_bus_bitsize}};

   const bool is_banked = bi.bank_number > 0;
   INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level,
                  "Creating AXI adapter for" + STR(is_banked ? " banked" : "") + " bus with " +
                      STR(is_banked ? bi.bank_number : bi.channel_number) + " channels");

   auto AXI_conversion = [&](unsigned int type) -> std::string {
      if(type == 0)
      {
         return "FIXED";
      }
      else if(type == 1)
      {
         return "INCREMENTAL";
      }
      else
      {
         return "unsupported AXI burst type";
      }
   };

   unsigned int axi_burst_type = 0U;

   const auto use_specific_axi_burst_type = parameters->isOption(OPT_axi_burst_type);
   const unsigned int requested_axi_burst_type =
       use_specific_axi_burst_type ? parameters->getOption<unsigned int>(OPT_axi_burst_type) : 0U;
   if(use_specific_axi_burst_type)
   {
      axi_burst_type = requested_axi_burst_type;
   }

   const auto hls_d = HLSMgr->get_HLS_device();
   unsigned int device_axi_burst_type = 0U;
   const auto use_device_axi_burst_type =
       HLSMgr->TryGetParameterFromParameterOrDevice<unsigned int>("axi_burst_type", hls_d, device_axi_burst_type);
   if(use_device_axi_burst_type)
   {
      axi_burst_type = device_axi_burst_type;
   }

   if(use_device_axi_burst_type && use_specific_axi_burst_type && device_axi_burst_type != requested_axi_burst_type)
   {
      THROW_WARNING("User required " + AXI_conversion(requested_axi_burst_type) +
                    " axi burst but the requested board needs " + AXI_conversion(device_axi_burst_type));
   }
   else if(use_specific_axi_burst_type)
   {
      THROW_WARNING("The requested board needs " + AXI_conversion(device_axi_burst_type) + " AXI burst type");
   }

   auto done_port_obj = wrappedObj->find_member(DONE_PORT_NAME, port_o_K, wrappedObj);
   THROW_ASSERT(done_port_obj, "Must be present.\n");
   auto done_signal = get_smart_signal(SM, "sig_done", signal_o_K, done_port_obj->get_typeRef());
   SM->add_connection(done_port_obj, done_signal);

   structural_objectRef done_external_port = SM->add_port(
       DONE_PORT_NAME, port_o::OUT, circuit, structural_type_descriptorRef(new structural_type_descriptor("bool", 1U)));
   port_o::resize_std_port(1U, 0U, 0, done_external_port);
   SM->add_connection(done_signal, done_external_port);

   const auto adapter_number = is_banked ? bi.bank_number : bi.channel_number;
   for(unsigned int i = 0; i < adapter_number; i = i + 1)
   {
      std::string adapter_name = "adapter_" + STR(i);
      std::string adapter_library = HLS->HLS_D->get_technology_manager()->get_library("MinimalAXI4Adapter");
      structural_objectRef adapter_module = SM->add_module_from_technology_library(
          adapter_name, "MinimalAXI4Adapter", adapter_library, circuit, HLS->HLS_D->get_technology_manager());
      THROW_ASSERT(adapter_module, "MinimalAXI4Adapter not present in component library");
      SM->add_connection(circuit->find_member(CLOCK_PORT_NAME, port_o_K, circuit),
                         adapter_module->find_member(CLOCK_PORT_NAME, port_o_K, adapter_module));
      SM->add_connection(circuit->find_member(RESET_PORT_NAME, port_o_K, circuit),
                         adapter_module->find_member(RESET_PORT_NAME, port_o_K, adapter_module));
      auto adapter_module_mod = GetPointer<module_o>(adapter_module);
      adapter_module_mod->SetParameter("BURST_TYPE", STR(axi_burst_type));

      for(const auto& p : (is_banked ? noc_adapter_ports : minimal_adapter_ports))
      {
         const auto pname_input = std::get<0>(p);
         const auto pname_adapter = std::get<1>(p);
         const auto size = std::get<2>(p);

         structural_objectRef input_port = input_module->find_member(pname_input, port_vector_o_K, input_module);
         THROW_ASSERT(input_port, pname_input + " port must be present");
         const auto input_port_o = GetPointer<port_o>(input_port);
         if((i == 0) && is_banked)
         {
            input_port_o->add_n_ports(bi.noc_nodes, input_port);
            port_o::resize_std_port(size, 0U, 0, input_port);
         }

         structural_objectRef adapter_port = adapter_module->find_member(pname_adapter, port_o_K, adapter_module);
         THROW_ASSERT(adapter_port, pname_adapter + " port must be present");
         port_o::resize_std_port(size, 0U, 0, adapter_port);

         auto sig = SM->add_sign("sig_out_" + pname_input + "_" + STR(i), circuit, adapter_port->get_typeRef());
         SM->add_connection(sig, (adapter_number > 1 ? input_port_o->get_port(i) : input_port));
         SM->add_connection(sig, adapter_port);
      }

      if(!is_banked)
      {
         structural_objectRef bp_port = adapter_module->find_member("M_back_pressure", port_vector_o_K, adapter_module);
         if(bp_port)
         {
            structural_objectRef const_obj_bp =
                SM->add_constant("null_value_M_back_pressure_" + STR(i), circuit, bp_port->get_typeRef(), STR(0));
            SM->add_connection(bp_port, const_obj_bp);
         }
         structural_objectRef ac_port = adapter_module->find_member("Mout_tag", port_vector_o_K, adapter_module);
         if(ac_port)
         {
            structural_objectRef const_obj_ac =
                SM->add_constant("null_value_Mout_tag_" + STR(i), circuit, ac_port->get_typeRef(), STR(0));
            SM->add_connection(ac_port, const_obj_ac);
         }
      }

      structural_objectRef m_axi_awaddr_port = adapter_module->find_member("m_axi_awaddr", port_o_K, adapter_module);
      THROW_ASSERT(m_axi_awaddr_port, "m_axi_awaddr port must be present");
      port_o::resize_std_port(bi.addr_banked_bitsize, 0U, 0, m_axi_awaddr_port);

      structural_objectRef m_axi_awlen_port = adapter_module->find_member("m_axi_awlen", port_o_K, adapter_module);
      THROW_ASSERT(m_axi_awlen_port, "m_axi_awlen port must be present");
      port_o::resize_std_port(8U, 0U, 0, m_axi_awlen_port);

      structural_objectRef m_axi_wdata_port = adapter_module->find_member("m_axi_wdata", port_o_K, adapter_module);
      THROW_ASSERT(m_axi_wdata_port, "m_axi_wdata port must be present");
      port_o::resize_std_port(bi.data_bus_bitsize, 0U, 0, m_axi_wdata_port);

      structural_objectRef m_axi_wstrb_port = adapter_module->find_member("m_axi_wstrb", port_o_K, adapter_module);
      THROW_ASSERT(m_axi_wstrb_port, "m_axi_wstrb port must be present");
      port_o::resize_std_port(bi.data_bus_bitsize / 8, 0U, 0, m_axi_wstrb_port);

      structural_objectRef m_axi_araddr_port = adapter_module->find_member("m_axi_araddr", port_o_K, adapter_module);
      THROW_ASSERT(m_axi_araddr_port, "m_axi_araddr port must be present");
      port_o::resize_std_port(bi.addr_banked_bitsize, 0U, 0, m_axi_araddr_port);

      structural_objectRef m_axi_arlen_port = adapter_module->find_member("m_axi_arlen", port_o_K, adapter_module);
      THROW_ASSERT(m_axi_arlen_port, "m_axi_arlen port must be present");
      port_o::resize_std_port(8U, 0U, 0, m_axi_arlen_port);

      structural_objectRef m_axi_rdata_port = adapter_module->find_member("m_axi_rdata", port_o_K, adapter_module);
      THROW_ASSERT(m_axi_rdata_port, "m_axi_wdata port must be present");
      port_o::resize_std_port(bi.data_bus_bitsize, 0U, 0, m_axi_rdata_port);

      if(is_banked)
      {
         // Resize signals for IDs
         structural_objectRef m_axi_awid_port = adapter_module->find_member("m_axi_awid", port_o_K, adapter_module);
         THROW_ASSERT(m_axi_awid_port, "m_axi_awid port must be present");
         port_o::resize_std_port(bi.context_bus_bitsize + bi.noc_id_size, 0U, 0, m_axi_awid_port);

         structural_objectRef m_axi_bid_port = adapter_module->find_member("m_axi_bid", port_o_K, adapter_module);
         THROW_ASSERT(m_axi_bid_port, "m_axi_bid port must be present");
         port_o::resize_std_port(bi.context_bus_bitsize + bi.noc_id_size, 0U, 0, m_axi_bid_port);

         structural_objectRef m_axi_arid_port = adapter_module->find_member("m_axi_arid", port_o_K, adapter_module);
         THROW_ASSERT(m_axi_arid_port, "m_axi_arid port must be present");
         port_o::resize_std_port(bi.context_bus_bitsize + bi.noc_id_size, 0U, 0, m_axi_arid_port);

         structural_objectRef m_axi_rid_port = adapter_module->find_member("m_axi_rid", port_o_K, adapter_module);
         THROW_ASSERT(m_axi_rid_port, "m_axi_rid port must be present");
         port_o::resize_std_port(bi.context_bus_bitsize + bi.noc_id_size, 0U, 0, m_axi_rid_port);
      }

      for(unsigned int index = 0; index < adapter_module_mod->get_in_port_size(); index++)
      {
         const auto port = adapter_module_mod->get_in_port(index);
         const auto port_obj = GetPointer<port_o>(port);
         const auto pname = port_obj->get_id();

         if(!port_obj->get_connections_size())
         {
            structural_objectRef external_port;
            std::string external_port_name = "m_axi_" + STR(i) + pname.substr(5);
            external_port = SM->add_port(external_port_name, port_o::IN, circuit, port->get_typeRef());
            port_o::fix_port_properties(port, external_port);
            auto external_port_o = GetPointer<port_o>(external_port);
            external_port_o->set_is_memory(true);

            SM->add_connection(external_port, port);
         }
      }

      for(unsigned int index = 0; index < adapter_module_mod->get_out_port_size(); index++)
      {
         const auto port = adapter_module_mod->get_out_port(index);
         const auto port_obj = GetPointer<port_o>(port);
         const auto pname = port_obj->get_id();

         if(!port_obj->get_connections_size())
         {
            structural_objectRef external_port;
            std::string external_port_name = "m_axi_" + STR(i) + pname.substr(5);
            external_port = SM->add_port(external_port_name, port_o::OUT, circuit, port->get_typeRef());
            port_o::fix_port_properties(port, external_port);
            auto external_port_o = GetPointer<port_o>(external_port);
            external_port_o->set_is_memory(true);

            SM->add_connection(external_port, port);
         }
      }
   }

   for(unsigned int j = bi.bank_number; j < bi.noc_nodes; j++)
   {
      for(const auto& p : noc_adapter_ports)
      {
         const auto pname_noc = std::get<0>(p);

         structural_objectRef noc_port = input_module->find_member(pname_noc, port_vector_o_K, input_module);
         THROW_ASSERT(noc_port, pname_noc + " port must be present");
         const auto noc_port_o = GetPointer<port_o>(noc_port);

         if(noc_port_o->get_port_direction() == port_o::IN)
         {
            structural_objectRef const_obj =
                SM->add_constant("null_value_" + pname_noc + "_" + STR(j), circuit, noc_port->get_typeRef(), STR(0));
            SM->add_connection(noc_port_o->get_port(j), const_obj);
         }
         else
         {
            auto signal = SM->add_sign(pname_noc + "_signal_" + STR(j), circuit, noc_port->get_typeRef());
            SM->add_connection(signal, noc_port_o->get_port(j));
         }
      }
   }
   INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "Created AXI adapters");
}

void bus_interface::add_profiling_modules(const structural_managerRef SM)
{
   const structural_objectRef circuit = SM->get_circ();
   for(auto i = 0U; i < bi.channel_number; i++)
   {
      std::string profiling_module_name = "profiling_module_pre_noc" + STR(i);
      std::string profiling_module_library =
          HLS->HLS_D->get_technology_manager()->get_library("PROFILING_MODULE_PRE_NOC");
      structural_objectRef profiling_module = SM->add_module_from_technology_library(
          profiling_module_name, "PROFILING_MODULE_PRE_NOC", profiling_module_library, circuit,
          HLS->HLS_D->get_technology_manager());
      SM->add_connection(circuit->find_member(CLOCK_PORT_NAME, port_o_K, circuit),
                         profiling_module->find_member(CLOCK_PORT_NAME, port_o_K, profiling_module));

      auto profiling_module_mod = GetPointer<module_o>(profiling_module);
      profiling_module_mod->SetParameter("CHANNEL_NUMBER", STR(i));
      profiling_module_mod->SetParameter("DATA_file", "\"\"profiling_data_pre_noc" + STR(i) + ".csv\"\"");

      auto oe_port = profiling_module->find_member("Mout_oe_ram", port_o_K, profiling_module);
      auto oe_signal = circuit->find_member("sig_in_Mout_oe_ram_" + STR(i), signal_o_K, circuit);
      SM->add_connection(oe_port, oe_signal);

      auto we_port = profiling_module->find_member("Mout_we_ram", port_o_K, profiling_module);
      auto we_signal = circuit->find_member("sig_in_Mout_we_ram_" + STR(i), signal_o_K, circuit);
      SM->add_connection(we_port, we_signal);

      auto addr_ram_port = profiling_module->find_member("Mout_addr_ram", port_o_K, profiling_module);
      port_o::resize_std_port(bi.addr_bus_bitsize, 0U, 0, addr_ram_port);
      structural_objectRef addr_signal =
          get_smart_signal(SM, "sig_in_Mout_addr_ram_" + STR(i), signal_o_K, addr_ram_port->get_typeRef());
      SM->add_connection(addr_ram_port, addr_signal);

      auto node_id_dest_port = profiling_module->find_member("node_id_dest", port_o_K, profiling_module);
      port_o::resize_std_port(bi.noc_id_size, 0U, 0, node_id_dest_port);
      auto node_id_dest_signal = circuit->find_member("node_id_dest_signal_" + STR(i), signal_o_K, circuit);
      SM->add_connection(node_id_dest_port, node_id_dest_signal);

      auto M_back_pressure_port = profiling_module->find_member("Mout_back_pressure", port_o_K, profiling_module);
      auto M_back_pressure_signal = circuit->find_member("sig_in_Mout_back_pressure_" + STR(i), signal_o_K, circuit);
      SM->add_connection(M_back_pressure_port, M_back_pressure_signal);
   }

   for(auto i = 0U; i < bi.bank_number; i++)
   {
      std::string profiling_module_name = "profiling_module_post_noc" + STR(i);
      std::string profiling_module_library =
          HLS->HLS_D->get_technology_manager()->get_library("PROFILING_MODULE_POST_NOC");
      structural_objectRef profiling_module = SM->add_module_from_technology_library(
          profiling_module_name, "PROFILING_MODULE_POST_NOC", profiling_module_library, circuit,
          HLS->HLS_D->get_technology_manager());
      SM->add_connection(circuit->find_member(CLOCK_PORT_NAME, port_o_K, circuit),
                         profiling_module->find_member(CLOCK_PORT_NAME, port_o_K, profiling_module));

      auto profiling_module_mod = GetPointer<module_o>(profiling_module);
      profiling_module_mod->SetParameter("BANK_NUMBER", STR(i));
      profiling_module_mod->SetParameter("NODE_ID_SIZE", STR(bi.noc_id_size));
      profiling_module_mod->SetParameter("DATA_file", "\"\"profiling_data_post_noc" + STR(i) + ".csv\"\"");

      auto oe_port = profiling_module->find_member("Mout_oe_ram", port_o_K, profiling_module);
      auto oe_signal = circuit->find_member("sig_out_Mout_oe_ram_out_" + STR(i), signal_o_K, circuit);
      SM->add_connection(oe_port, oe_signal);

      auto we_port = profiling_module->find_member("Mout_we_ram", port_o_K, profiling_module);
      auto we_signal = circuit->find_member("sig_out_Mout_we_ram_out_" + STR(i), signal_o_K, circuit);
      SM->add_connection(we_port, we_signal);

      auto addr_ram_port = profiling_module->find_member("Mout_addr_ram", port_o_K, profiling_module);
      port_o::resize_std_port(bi.addr_banked_bitsize, 0U, 0, addr_ram_port);
      auto addr_signal = circuit->find_member("sig_out_Mout_addr_ram_out_" + STR(i), signal_o_K, circuit);
      SM->add_connection(addr_ram_port, addr_signal);

      auto node_id_dest_port = profiling_module->find_member("M_tag", port_o_K, profiling_module);
      port_o::resize_std_port(bi.noc_id_size, 0U, 0, node_id_dest_port);
      auto node_id_dest_signal = circuit->find_member("sig_out_M_tag_" + STR(i), signal_o_K, circuit);
      SM->add_connection(node_id_dest_port, node_id_dest_signal);

      auto Mout_back_pressure_port = profiling_module->find_member("Mout_back_pressure", port_o_K, profiling_module);
      auto Mout_back_pressure_signal =
          circuit->find_member("sig_out_Mout_back_pressure_out_" + STR(i), signal_o_K, circuit);
      SM->add_connection(Mout_back_pressure_port, Mout_back_pressure_signal);
   }
}

void bus_interface::instantiate_noc(const structural_managerRef SM, const structural_objectRef wrappedObj)
{
   const structural_objectRef circuit = SM->get_circ();
   PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Start to instantiate banked_memory_noc");
   std::string noc_name = "noc_top";
   std::string noc_library =
       HLS->HLS_D->get_technology_manager()->get_library("Noc_banked_memory_connection_interface");
   structural_objectRef noc_module = SM->add_module_from_technology_library(
       noc_name, "Noc_banked_memory_connection_interface", noc_library, circuit, HLS->HLS_D->get_technology_manager());

   structural_objectRef clock_noc = noc_module->find_member(CLOCK_PORT_NAME, port_o_K, noc_module);
   structural_objectRef clock_port = circuit->find_member(CLOCK_PORT_NAME, port_o_K, circuit);
   SM->add_connection(clock_noc, clock_port);

   structural_objectRef reset_noc = noc_module->find_member(RESET_PORT_NAME, port_o_K, noc_module);
   structural_objectRef reset_port = circuit->find_member(RESET_PORT_NAME, port_o_K, circuit);
   SM->add_connection(reset_noc, reset_port);
   PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Instantiate banked_memory_noc");

   PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Starting setting parameter banked_memory_noc");
   unsigned int internal_portsize;
   structural_objectRef wrapped_addr_ram_port = wrappedObj->find_member("Mout_addr_ram", port_vector_o_K, wrappedObj);

   if(bi.channel_number == 1)
   {
      THROW_ASSERT(wrapped_addr_ram_port->get_kind() == port_o_K,
                   "Mout_data_ram_size port must be present in " + wrappedObj->get_path());
      internal_portsize = 1;
   }
   else
   {
      internal_portsize = GetPointer<port_o>(wrapped_addr_ram_port)->get_ports_size();
   }
   if(internal_portsize > bi.channel_number)
   {
      THROW_ERROR(
          "The number of memory channels requested is greater than the number of channels between the NoC and the "
          "accelerator. This is not a valid configuration\n .");
   }

   auto noc_rows = 1U;
   while((noc_rows * (1ULL << noc_rows)) < bi.noc_nodes)
   {
      noc_rows++;
   }
   const auto noc_cols = 1ULL << noc_rows;
   const auto noc_addr_ext_size = ceil_log2(bi.noc_nodes / noc_cols + (bi.noc_nodes % noc_cols != 0));
   const auto noc_data_size_to_data = 1ULL + bi.size_bus_bitsize + bi.data_bus_bitsize + bi.addr_banked_bitsize +
                                      bi.context_bus_bitsize + bi.noc_id_size;
   const auto noc_data_size_from_data = bi.data_bus_bitsize + bi.context_bus_bitsize;

   auto noc_module_mod = GetPointer<module_o>(noc_module);

   noc_module_mod->SetParameter("REGISTERED", STR(bi.use_registered_noc ? 1 : 0));
   noc_module_mod->SetParameter("NOC_NODES", STR(bi.noc_nodes));
   noc_module_mod->SetParameter("NOC_COLUMNS", STR(noc_cols));
   noc_module_mod->SetParameter("ADDRESS_SIZE_NOC", STR(noc_rows));
   noc_module_mod->SetParameter("ADDRESS_EXT_SIZE_NOC", STR(noc_addr_ext_size));
   noc_module_mod->SetParameter("NODE_ID_SIZE", STR(bi.noc_id_size));
   noc_module_mod->SetParameter("DATA_SIZE_NOC_TO", STR(noc_data_size_to_data));
   noc_module_mod->SetParameter("DATA_SIZE_NOC_FROM", STR(noc_data_size_from_data));
   noc_module_mod->SetParameter("BASE_ADDR",
                                STR(parameters->getOption<unsigned long long int>(OPT_base_address) / bi.bank_number));

   PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Starting connecting banked_memory_noc ports");

   create_noc_memory_mapping_unit(noc_module, wrappedObj, SM);

   const std::vector<std::tuple<std::string, std::string, unsigned int>> internal_module_ports = {
       {"Mout_oe_ram", "Mout_oe_ram", 1U},
       {"Mout_we_ram", "Mout_we_ram", 1U},
       {"Mout_Wdata_ram", "Mout_Wdata_ram", bi.data_bus_bitsize},
       {"Mout_data_ram_size", "Mout_data_ram_size", bi.size_bus_bitsize},
       {"Mout_tag", "Mout_tag", bi.context_bus_bitsize},
       {"Mout_back_pressure", "Mout_back_pressure", 1U},
       {"M_DataRdy_out", "M_DataRdy", 1U},
       {"M_Rdata_ram_out", "M_Rdata_ram", bi.data_bus_bitsize},
       {"M_tag_out", "M_tag", bi.context_bus_bitsize}};

   for(const auto& p : internal_module_ports)
   {
      const auto pname_noc = std::get<0>(p);
      const auto pname_module = std::get<1>(p);
      const auto size = std::get<2>(p);

      structural_objectRef internal_port = wrappedObj->find_member(pname_module, port_vector_o_K, wrappedObj);
      if(internal_port)
      {
         auto internal_port_o = GetPointer<port_o>(internal_port);
         const auto internal_port_size =
             internal_port_o->get_kind() == port_vector_o_K ? internal_port_o->get_ports_size() : 1U;
         structural_objectRef noc_port = noc_module->find_member(pname_noc, port_vector_o_K, noc_module);
         const auto noc_port_o = GetPointer<port_o>(noc_port);
         noc_port_o->add_n_ports(static_cast<unsigned int>(bi.noc_nodes), noc_port);
         port_o::resize_std_port(size, 0U, 0, noc_port);

         for(unsigned int i = 0; i < bi.noc_nodes; i++)
         {
            structural_objectRef signal =
                get_smart_signal(SM, "sig_in_" + pname_module + "_" + STR(i), signal_o_K, internal_port->get_typeRef());

            // internal_portsize is the size of the bus, internal_port_size is the size of the specific_port (Mout_tag
            // can have smaller size as it is generated by OpenMP requests and not by memory ctrl)
            if(internal_portsize > i && internal_port_size > i)
            {
               SM->add_connection(signal, noc_port_o->get_port(i));
               SM->add_connection(signal,
                                  (bi.channel_number == 1 && i == 0) ? internal_port : internal_port_o->get_port(i));
            }
            else if(noc_port_o->get_port_direction() == port_o::IN)
            {
               structural_objectRef const_obj =
                   SM->add_constant("zero_const_" + pname_noc + "_" + STR(i), circuit, noc_port->get_typeRef(), STR(0));
               SM->add_connection(noc_port_o->get_port(i), const_obj);
            }
            else
            {
               SM->add_connection(signal, noc_port_o->get_port(i));
            }
         }
      }
      else
      {
         THROW_ASSERT(pname_module == "Mout_tag" || pname_module == "M_tag",
                      "Only Mout_tag or M_tag can not be present. " + pname_module);
         structural_objectRef noc_port = noc_module->find_member(pname_noc, port_vector_o_K, noc_module);
         const auto noc_port_o = GetPointer<port_o>(noc_port);
         noc_port_o->add_n_ports(static_cast<unsigned int>(bi.noc_nodes), noc_port);
         port_o::resize_std_port(size, 0U, 0, noc_port);
         if(pname_noc == "Mout_tag")
         {
            for(unsigned int i = 0; i < bi.noc_nodes; i = i + 1)
            {
               structural_objectRef const_obj =
                   SM->add_constant("null_value_" + pname_noc + "_" + STR(i), circuit, noc_port->get_typeRef(), STR(0));
               SM->add_connection(noc_port_o->get_port(i), const_obj);
            }
         }
         else if(pname_noc == "M_tag_out")
         {
            auto signal = SM->add_sign_vector(pname_module + "_signal", bi.noc_nodes, circuit, noc_port->get_typeRef());
            SM->add_connection(signal, noc_port);
         }
      }
   }

   if(bi.use_caches)
   {
      create_iob_caches(noc_module, wrappedObj, SM);
   }
   else if(bi.use_axi)
   {
      create_axi_adapter(noc_module, wrappedObj, SM);
   }
   else
   {
      const std::vector<std::tuple<std::string, std::string, unsigned int, port_o::port_direction>>
          external_module_ports = {{"Mout_oe_ram_out", "Mout_oe_ram", 1U, port_o::OUT},
                                   {"Mout_we_ram_out", "Mout_we_ram", 1U, port_o::OUT},
                                   {"Mout_addr_ram_out", "Mout_addr_ram", bi.addr_banked_bitsize, port_o::OUT},
                                   {"Mout_Wdata_ram_out", "Mout_Wdata_ram", bi.data_bus_bitsize, port_o::OUT},
                                   {"Mout_data_ram_size_out", "Mout_data_ram_size", bi.size_bus_bitsize, port_o::OUT},
                                   {"Mout_tag_out", "Mout_tag", bi.context_bus_bitsize + bi.noc_id_size, port_o::OUT},
                                   {"M_back_pressure", "M_back_pressure", 1U, port_o::OUT},
                                   {"Mout_back_pressure_out", "Mout_back_pressure", 1U, port_o::IN},
                                   {"M_DataRdy", "M_DataRdy", 1U, port_o::IN},
                                   {"M_Rdata_ram", "M_Rdata_ram", bi.data_bus_bitsize, port_o::IN},
                                   {"M_tag", "M_tag", bi.context_bus_bitsize + bi.noc_id_size, port_o::IN}};

      for(const auto& p : external_module_ports)
      {
         const auto pname_noc = std::get<0>(p);
         const auto pname_module = std::get<1>(p);
         const auto size = std::get<2>(p);
         const auto direction = std::get<3>(p);

         structural_objectRef noc_port = noc_module->find_member(pname_noc, port_vector_o_K, noc_module);
         const auto noc_port_o = GetPointer<port_o>(noc_port);
         noc_port_o->add_n_ports(bi.noc_nodes, noc_port);
         port_o::resize_std_port(size, 0U, 0, noc_port);

         structural_objectRef external_port =
             SM->add_port_vector(pname_module, direction, bi.bank_number, circuit,
                                 structural_type_descriptorRef(new structural_type_descriptor("bool", size)));
         port_o::resize_std_port(size, 0U, 0, external_port);
         auto external_port_o = GetPointer<port_o>(external_port);
         external_port_o->set_is_memory(true);

         if((bi.noc_nodes > bi.bank_number) && noc_port_o->get_port_direction() == port_o::IN)
         {
            for(unsigned int i = 0; i < bi.bank_number; i = i + 1)
            {
               structural_objectRef signal =
                   SM->add_sign("sig_out_" + pname_noc + "_" + STR(i), circuit, external_port->get_typeRef());
               SM->add_connection(noc_port_o->get_port(i), signal);
               SM->add_connection(external_port_o->get_port(i), signal);
            }

            for(unsigned int i = bi.bank_number; i < bi.noc_nodes; i = i + 1)
            {
               structural_objectRef const_obj =
                   SM->add_constant("null_value_" + pname_noc + "_" + STR(i), circuit, noc_port->get_typeRef(), STR(0));
               SM->add_connection(noc_port_o->get_port(i), const_obj);
            }
         }
         else
         {
            for(unsigned int i = 0; i < bi.noc_nodes; i = i + 1)
            {
               structural_objectRef signal =
                   SM->add_sign("sig_out_" + pname_noc + "_" + STR(i), circuit, external_port->get_typeRef());
               SM->add_connection(noc_port_o->get_port(i), signal);
               if(i < bi.bank_number)
               {
                  SM->add_connection(external_port_o->get_port(i), signal);
               }
            }
         }
      }
      auto done_port_obj = wrappedObj->find_member(DONE_PORT_NAME, port_o_K, wrappedObj);
      structural_objectRef external_port =
          SM->add_port(DONE_PORT_NAME, port_o::OUT, circuit,
                       structural_type_descriptorRef(new structural_type_descriptor("bool", 1U)));
      SM->add_connection(done_port_obj, external_port);
   }

   if(bi.const_use_profiling_modules)
   {
      add_profiling_modules(SM);
   }
}
