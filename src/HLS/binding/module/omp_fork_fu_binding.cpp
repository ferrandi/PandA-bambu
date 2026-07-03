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
 *              Copyright (C) 2022-2026 Politecnico di Milano
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
 * @file omp_fork_fu_binding.cpp
 * @brief
 *
 * @author Giovanni Gozzi <giovanni.gozzi@polimi.it>
 *
 */

#include "omp_fork_fu_binding.hpp"

#include "Parameter.hpp"
#include "behavioral_helper.hpp"
#include "fileIO.hpp"
#include "fu_binding.hpp"
#include "function_behavior.hpp"
#include "hls.hpp"
#include "hls_device.hpp"
#include "hls_manager.hpp"
#include "hls_step.hpp"
#include "interface/module_interface.hpp"
#include "ir_manager.hpp"
#include "ir_node.hpp"
#include "ir_reindex.hpp"
#include "language_writer.hpp"
#include "math_function.hpp"
#include "memory.hpp"
#include "memory_allocation.hpp"
#include "string_manipulation.hpp"
#include "structural_manager.hpp"
#include "structural_objects.hpp"
#include "technology_manager.hpp"
#include "technology_node.hpp"
#include <algorithm>
#include <tuple>

omp_fork_fu_binding::omp_fork_fu_binding(const HLS_managerConstRef _HLSMgr, const unsigned int _function_id,
                                         const ParameterConstRef _parameters)
    : fu_binding(_HLSMgr, _function_id, _parameters)
{
}

static bool try_get_resized_port_list(const std::string& port_name, const std::list<structural_objectRef>& modules,
                                      const unsigned int size, so_kind port_type, const unsigned int port_number,
                                      std::list<structural_objectRef>& target_list)
{
   bool result = false;
   for(auto& id_module : modules)
   {
      structural_objectRef port_obj;
      const auto has_port = fu_binding::try_find_port(port_name, id_module, port_type, port_obj);
      if(has_port)
      {
         const auto port = GetPointerS<port_o>(port_obj);
         if(port_type == port_vector_o_K)
         {
            port->add_n_ports(port_number, port_obj);
         }
         port_o::resize_std_port(size, 0U, 0, port_obj);
         target_list.push_back(port_obj);
         result = true;
      }
   }
   return result;
}

static void
prepare_resized_connection(std::map<structural_objectRef, std::list<structural_objectRef>, jms_sorter>& connections,
                           const std::string& port_name_source, const structural_objectRef& source,
                           const std::string& port_name_destination,
                           const std::list<structural_objectRef>& destinations, const unsigned int size,
                           so_kind port_source_type, const int DEBUG_PARAMETER(debug_level),
                           const unsigned int port_destination_number = 1U, so_kind port_dest_type = port_o_K)
{
   std::list<structural_objectRef> destination_ports;
   const bool has_destination_ports = try_get_resized_port_list(
       port_name_destination, destinations, size, port_dest_type, port_destination_number, destination_ports);
   if(has_destination_ports)
   {
      structural_objectRef port_source_obj;
      const auto has_port = fu_binding::try_find_port(port_name_source, source, port_source_type, port_source_obj);
      if(has_port)
      {
         if(port_source_type == port_vector_o_K)
         {
            const auto port_source = GetPointerS<port_o>(port_source_obj);
            port_source->add_n_ports(static_cast<unsigned int>(destination_ports.size() * port_destination_number),
                                     port_source_obj);
            port_o::resize_std_port(size, 0U, 0, port_source_obj);
         }
         else
         {
            port_o::resize_std_port(size *
                                        static_cast<unsigned int>(destination_ports.size() * port_destination_number),
                                    0U, 0, port_source_obj);
         }
         connections[port_source_obj] = destination_ports;
         PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                       "Added connection between " + port_name_source + " of module " + source->get_path() + " and " +
                           port_name_destination);
      }
   }
}

const structural_objectRef omp_fork_fu_binding::add_module(std::string fu_name, std::string name,
                                                           const structural_managerRef SM, const hlsRef& HLS,
                                                           const structural_objectRef& circuit) const
{
   const auto TechM = HLS->HLS_D->get_technology_manager();
   const auto clock_port = circuit->find_member(CLOCK_PORT_NAME, port_o_K, circuit);
   const auto reset_port = circuit->find_member(RESET_PORT_NAME, port_o_K, circuit);
   const auto module_library = HLS->HLS_D->get_technology_manager()->get_library(fu_name);
   const auto module_obj = SM->add_module_from_technology_library(name, fu_name, module_library, circuit, TechM);
   const auto clock_port_mod = module_obj->find_member(CLOCK_PORT_NAME, port_o_K, module_obj);
   const auto reset_port_mod = module_obj->find_member(RESET_PORT_NAME, port_o_K, module_obj);
   add_smart_connection(reset_port, reset_port_mod, 0, circuit, SM);
   add_smart_connection(clock_port, clock_port_mod, 0, circuit, SM);
   return module_obj;
}

ir_nodeRef omp_fork_fu_binding::get_outlined_fnode(const structural_objectRef& obj, const HLS_managerRef& HLSMgr)
{
   const auto fnode_it = obj_fnode.find(obj);
   ir_nodeRef fnode = nullptr;
   if(fnode_it == obj_fnode.end())
   {
      if(obj->get_id().find("fu___kmp_bambu_fork_call") != std::string::npos)
      {
         const auto last_us = obj->get_id().find_last_of("_");
         const auto tn_id = obj->get_id().substr(last_us + 1U);
         const auto stmt = HLSMgr->get_ir_manager()->GetIRNode(boost::lexical_cast<unsigned int>(tn_id));
         if(stmt->get_kind() == call_stmt_K)
         {
            const auto gc = GetPointerS<const call_stmt>(stmt);
            THROW_ASSERT(gc->fn->get_kind() == addr_node_K, "Expected an addr_node");
            fnode = GetPointerS<const addr_node>(gc->fn)->op;
            if(HLSMgr->isOmpLambdaFunction(fnode->index))
            {
               obj_fnode[obj] = fnode;
            }
         }
      }
   }
   else
   {
      fnode = fnode_it->second;
   }
   return fnode;
}

void omp_fork_fu_binding::add_memory_ports_to_fork_call_module(structural_objectRef& fork_call_module,
                                                               std::list<structural_objectRef>& outlined_modules_list,
                                                               const structural_objectRef& circuit,
                                                               const structural_managerRef& SM) const
{
   structural_objectRef fork_call_port;
   auto outlined_master = outlined_modules_list.front();
   const std::vector<std::tuple<std::string, port_o::port_direction>> M_ports = {
       {"Mout_oe_ram", port_o::OUT},    {"Mout_we_ram", port_o::OUT},        {"Mout_addr_ram", port_o::OUT},
       {"Mout_Wdata_ram", port_o::OUT}, {"Mout_data_ram_size", port_o::OUT}, {"M_Rdata_ram", port_o::IN},
       {"M_DataRdy", port_o::IN}};

   INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, fci.debug_level, "Adding ports to module: " + fork_call_module->get_path());
   for(const auto& mp : M_ports)
   {
      const auto pname = std::get<0>(mp);
      const auto direction = std::get<1>(mp);
      structural_objectRef port_outlined = outlined_master->find_member(pname, port_o_K, outlined_master);
      THROW_ASSERT(port_outlined, "Port " + pname + " of master thread should not be null");
      fork_call_port =
          fork_call_module->find_member(pname, fci.channel_number == 1U ? port_o_K : port_vector_o_K, fork_call_module);
      THROW_ASSERT(!fork_call_port, "should be null");
      if(!fork_call_port)
      {
         add_smart_port(fork_call_port, port_outlined, fci.channel_number, pname, direction, circuit, SM);
         THROW_ASSERT(fork_call_port, "should not be null");
         GetPointer<port_o>(fork_call_port)->set_is_memory(true);
         INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, fci.debug_level, "Added port " + pname);
      }
   }

   if(fci.is_bus_pipelined)
   {
      const std::vector<std::tuple<std::string, port_o::port_direction>> M_tag_ports = {{"Mout_tag", port_o::OUT},
                                                                                        {"M_tag", port_o::IN}};

      for(const auto& mp : M_tag_ports)
      {
         const auto pname = std::get<0>(mp);
         const auto direction = std::get<1>(mp);

         structural_objectRef port = fork_call_module->find_member(
             pname, fci.channel_number == 1U ? port_o_K : port_vector_o_K, fork_call_module);
         if(!port)
         {
            if(fci.channel_number == 1U)
            {
               port = SM->add_port(pname, direction, circuit,
                                   structural_type_descriptorRef(new structural_type_descriptor("bool", 1U)));
            }
            else
            {
               port = SM->add_port_vector(pname, direction, fci.channel_number, circuit,
                                          structural_type_descriptorRef(new structural_type_descriptor("bool", 1U)));
            }
            port_o::resize_std_port(fci.outer_tag_bitsize, 0U, 0, port);
            GetPointer<port_o>(port)->set_is_memory(true);
            INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, fci.debug_level, "Added port " + pname);
         }
      }
   }

   if(fci.is_bus_pipelined || fci.use_banked_memory || !fci.is_top_level)
   {
      structural_objectRef port = fork_call_module->find_member(
          "Mout_back_pressure", fci.channel_number == 1U ? port_o_K : port_vector_o_K, fork_call_module);
      THROW_ASSERT(!port, "should be null");
      structural_objectRef port_outlined =
          outlined_master->find_member("Mout_back_pressure", port_o_K, outlined_master);
      THROW_ASSERT(port_outlined, "Port Mout_back_pressure of master thread should not be null");
      if(!port)
      {
         add_smart_port(port, port_outlined, fci.channel_number, "Mout_back_pressure", port_o::IN, circuit, SM);
         THROW_ASSERT(port, "should not be null");
         GetPointer<port_o>(port)->set_is_memory(true);
         INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, fci.debug_level, "Added port Mout_back_pressure");
      }
   }
   INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, fci.debug_level, "Added ports to module: " + fork_call_module->get_path());
}

void omp_fork_fu_binding::propagate_ports_to_fork_call_module(structural_objectRef& fork_call_module,
                                                              std::list<structural_objectRef> outlined_modules,
                                                              const structural_objectRef& circuit,
                                                              const structural_managerRef& SM) const
{
   structural_objectRef fork_call_port;
   for(const auto& om : outlined_modules)
   {
      for(unsigned int j = 0; j < GetPointer<module_o>(om)->get_in_port_size(); ++j)
      {
         structural_objectRef port_i = GetPointer<module_o>(om)->get_in_port(j);
         if(GetPointer<port_o>(port_i)->get_is_memory())
         {
            std::string port_name = GetPointer<port_o>(port_i)->get_id();
            fork_call_port = fork_call_module->find_member(port_name, port_i->get_kind(), fork_call_module);
            THROW_ASSERT(!fork_call_port || GetPointer<port_o>(fork_call_port), "should be a port or null");
            if(!fork_call_port)
            {
               add_smart_port(
                   fork_call_port, port_i,
                   (port_i->get_kind() == port_vector_o_K ? GetPointer<port_o>(port_i)->get_ports_size() : 1U),
                   port_name, port_o::IN, circuit, SM);
               SM->add_connection(port_i, fork_call_port);
            }
         }
      }

      for(unsigned int j = 0; j < GetPointer<module_o>(om)->get_out_port_size(); j++)
      {
         structural_objectRef port_i = GetPointer<module_o>(om)->get_out_port(j);
         if(GetPointer<port_o>(port_i)->get_is_memory())
         {
            const auto port_name = GetPointerS<port_o>(port_i)->get_id();
            fork_call_port = fork_call_module->find_member(port_name, port_i->get_kind(), fork_call_module);
            THROW_ASSERT(!fork_call_port || GetPointer<port_o>(fork_call_port), "should be a port or null");
            if(!fork_call_port)
            {
               add_smart_port(
                   fork_call_port, port_i,
                   (port_i->get_kind() == port_vector_o_K ? GetPointer<port_o>(port_i)->get_ports_size() : 1U),
                   port_name, port_o::OUT, circuit, SM);
               SM->add_connection(port_i, fork_call_port);
            }
         }
      }
   }
}

void omp_fork_fu_binding::generate_mem_if(
    std::list<structural_objectRef>& outlined_modules_list,
    std::map<structural_objectRef, std::list<structural_objectRef>, jms_sorter>& connections,
    const HLS_managerRef& HLSMgr, const hlsRef& HLS, const structural_objectRef& circuit,
    const structural_managerRef& SM)
{
   INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, fci.debug_level, "Generating mem_interface modules");
   const auto addr_bus_bitsize = HLSMgr->get_address_bitsize();
   const auto data_bus_bitsize = HLSMgr->Rmem->get_bus_data_bitsize();
   const auto size_bus_bitsize = HLSMgr->Rmem->get_bus_size_bitsize();
   const auto outlined_master = outlined_modules_list.front();
   auto fork_call_module = outlined_modules_list.front()->get_owner();

   const auto root_space_alignment = HLSMgr->Rmem->get_root_space_alignment();
   const auto external_base_address = parameters->getOption<unsigned long long int>(OPT_base_address);
   const auto omp_page_id_start = HLSMgr->Rmem->get_fork_page_id_start();
   const auto omp_page_id_end = HLSMgr->Rmem->get_fork_page_id_end();
   const auto& omp_info = HLSMgr->Rmem->get_omp_allocation_info(fci.fork_id);
   const auto fork_number = omp_info.fork_number;

   unsigned int i = 0U;
   std::list<structural_objectRef> general_memory_if_list;

   for(auto& o : outlined_modules_list)
   {
      unsigned int outlined_threads = 1U;
      unsigned int outlined_threads_size = 1U;

      if(i < outlined_modules_list.size())
      {
         INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, fci.debug_level, "Considering core: " + STR(i));
         auto outlined_fnode = get_outlined_fnode(o, HLSMgr);
         auto outlined_fid = outlined_fnode->index;
         outlined_threads = HLSMgr->GetOMPThreadsCount(outlined_fid);
         outlined_threads_size = std::max(ceil_log2(outlined_threads), 1U);
         fci.nproc += outlined_threads;
         fci.context_switch_number = std::max(fci.context_switch_number, outlined_threads);
         fci.context_switch = fci.context_switch || outlined_threads > 1U;
         fci.threads_size = std::max(fci.threads_size, outlined_threads_size);
         INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, fci.debug_level, "Number of threads: " + STR(fci.nproc));
         INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, fci.debug_level,
                        "Context switch: " + STR(fci.context_switch ? "yes" : "no"));
         INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, fci.debug_level, "Number of context: " + STR(fci.context_switch_number));
      }
      fci.tid_size = std::max(ceil_log2(fci.nproc), 1U);

      const std::vector<std::tuple<std::string, unsigned int, so_kind>> general_memory_if_ports = {
          {"Mout_oe_ram", 1U, port_o_K},
          {"Mout_we_ram", 1U, port_o_K},
          {"Mout_addr_ram", addr_bus_bitsize, port_o_K},
          {"Mout_Wdata_ram", data_bus_bitsize, port_o_K},
          {"Mout_data_ram_size", size_bus_bitsize, port_o_K},
          {"Mout_back_pressure", 1U, port_o_K},
          {"M_DataRdy", 1U, port_o_K},
          {"M_Rdata_ram", data_bus_bitsize, port_o_K},
          {"M_tag", std::max(fci.inner_tag_bitsize, 1U), port_o_K},
          {"Mout_tag", std::max(fci.inner_tag_bitsize, 1U), port_o_K}};

      std::list<structural_objectRef> list_o;
      list_o.push_back(o);
      const auto general_memory_if = add_module(
          GENERAL_MEMORY_IF_NAME, STR("general_memory_if_") + STR(fci.fork_id) + "_" + STR(i), SM, HLS, circuit);

      const bool const_use_filters = (i == 0U && fci.const_use_reduction);
      general_memory_if_list.push_back(general_memory_if);
      general_memory_if->SetParameter("ROOT_SPACE_ALIGNMENT", STR(root_space_alignment));
      general_memory_if->SetParameter("EXTERNAL_BASE_ADDRESS", STR(external_base_address));
      general_memory_if->SetParameter("FORK_CALL_ID_START", STR(omp_page_id_start));
      general_memory_if->SetParameter("FORK_CALL_ID_END", STR(omp_page_id_end));
      general_memory_if->SetParameter("FORK_NUMBER", STR(fork_number));
      general_memory_if->SetParameter("USE_FILTER", STR(const_use_filters));
      general_memory_if->SetParameter("INNER_TAG", STR(fci.inner_tag_bitsize > 0U));
      general_memory_if->SetParameter("USE_CS", STR(fci.context_switch));
      general_memory_if->SetParameter(
          "CONTEXT_NUMBER_SIZE",
          STR((fci.use_banked_memory && fci.is_top_level && !fci.context_switch) ? 1 : fci.threads_size));
      INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, fci.debug_level, "Added general_mem_if for core " + STR(i));
      for(const auto& fop : general_memory_if_ports)
      {
         const auto port_name = std::get<0>(fop);
         const auto size = std::get<1>(fop);
         const auto source_port_type = std::get<2>(fop);
         prepare_resized_connection(connections, port_name, general_memory_if, port_name, list_o, size,
                                    source_port_type, fci.debug_level);
      }
      i++;
   }
   INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, fci.debug_level, "Generated mem_interface modules");

   if(fci.const_use_reduction)
   {
      generate_thread_to_thread_if(general_memory_if_list, outlined_modules_list, connections, HLSMgr, HLS, circuit,
                                   SM);
   }
   generate_thread_to_memory_if(general_memory_if_list, outlined_modules_list, connections, HLSMgr, HLS, circuit, SM);
}

void omp_fork_fu_binding::generate_thread_to_memory_if(
    std::list<structural_objectRef>& general_memory_if_list, std::list<structural_objectRef>& outlined_modules_list,
    std::map<structural_objectRef, std::list<structural_objectRef>, jms_sorter>& connections,
    const HLS_managerRef& HLSMgr, const hlsRef& HLS, const structural_objectRef& circuit,
    const structural_managerRef& SM) const
{
   INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, fci.debug_level, "Generating Thread to memory connection module");
   THROW_ASSERT(outlined_modules_list.size() > 0, "outlined_modules_list can't be empty");
   const auto fork_call_module = outlined_modules_list.front()->get_owner();
   const auto addr_bus_bitsize = HLSMgr->get_address_bitsize();
   const auto data_bus_bitsize = HLSMgr->Rmem->get_bus_data_bitsize();
   const auto size_bus_bitsize = HLSMgr->Rmem->get_bus_size_bitsize();

   std::string conn_interface_name;
   if(fci.const_use_grouped_channels)
   {
      conn_interface_name = GROUPED_CHANNEL_IF_NAME;
      THROW_ASSERT(fci.channel_number <= fci.accelerator_number, "The number of channels " + STR(fci.channel_number) +
                                                                     " is greater than the number of accelerators " +
                                                                     STR(fci.accelerator_number));
   }
   else if(fci.use_banked_memory && fci.is_top_level)
   {
      conn_interface_name = BANKED_MEMORY_IF_NAME;
   }
   else if(fci.channel_number == 1)
   {
      conn_interface_name = SIMPLE_ARBITER_IF_NAME;
   }
   else
   {
      conn_interface_name = ARBITER_IF_NAME;
      THROW_ASSERT(fci.channel_number <= fci.accelerator_number, "The number of channels " + STR(fci.channel_number) +
                                                                     " is greater than the number of accelerators " +
                                                                     STR(fci.accelerator_number));
   }
   const auto conn_interface =
       add_module(conn_interface_name, "Thread_to_Memory_connection_interface_" + STR(fci.fork_id), SM, HLS, circuit);
   INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, fci.debug_level, "Added module " + STR(conn_interface_name));
   INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, fci.debug_level,
                  "Arbiter type: " + parameters->getOption<std::string>(OPT_bus_arbiter_type));

   if(fci.const_use_grouped_channels)
   {
      const auto channel_group_size = std::max(ceil_log2(fci.channel_per_group), 1U);
      conn_interface->SetParameter("CHANNEL_GROUP_NUMBER", STR(fci.channel_number));
      conn_interface->SetParameter("CHANNEL_PER_GROUP", STR(fci.channel_per_group));
      conn_interface->SetParameter("CHANNEL_GROUP_SIZE", STR(channel_group_size));
   }
   else
   {
      conn_interface->SetParameter("SELECTOR_SIZE", STR(fci.accelerator_number_size));
   }

   const auto arbiter_type = parameters->getOption<std::string>(OPT_bus_arbiter_type);
   if(arbiter_type == "RR")
   {
      conn_interface->SetParameter("ARBITER_TYPE", STR(1));
   }
   else if(arbiter_type == "LP")
   {
      conn_interface->SetParameter("ARBITER_TYPE", STR(2));
   }
   if(fci.inner_tag_bitsize > 0U)
   {
      conn_interface->SetParameter("INNER_TAG", STR(1));
   }
   if(fci.is_bus_pipelined)
   {
      conn_interface->SetParameter("IS_PIPELINED", STR(1));
   }

   const std::vector<std::tuple<std::string, std::string, unsigned int, so_kind>> conn_interface_ports = {
       {"Mout_oe_ram", "Mout_oe_ram_memory", 1U, port_vector_o_K},
       {"Mout_we_ram", "Mout_we_ram_memory", 1U, port_vector_o_K},
       {"Mout_addr_ram", "Mout_addr_ram_memory", addr_bus_bitsize, port_vector_o_K},
       {"Mout_Wdata_ram", "Mout_Wdata_ram_memory", data_bus_bitsize, port_vector_o_K},
       {"Mout_data_ram_size", "Mout_data_ram_size_memory", size_bus_bitsize, port_vector_o_K},
       {"Mout_tag", "Mout_tag_memory", std::max(fci.inner_tag_bitsize, 1U), port_vector_o_K},
       {"Mout_back_pressure", "Mout_back_pressure_memory", 1U, port_vector_o_K},
       {"M_DataRdy_out", "M_DataRdy_memory", 1U, port_vector_o_K},
       {"M_Rdata_ram_out", "M_Rdata_ram_memory", data_bus_bitsize, port_vector_o_K},
       {"M_tag_out", "M_tag_memory", std::max(fci.inner_tag_bitsize, 1U), port_vector_o_K}};
   for(const auto& fop : conn_interface_ports)
   {
      const auto pname_conn = std::get<0>(fop);
      const auto pname_filt = std::get<1>(fop);
      const auto size = std::get<2>(fop);
      const auto source_port_type = std::get<3>(fop);
      prepare_resized_connection(connections, pname_conn, conn_interface, pname_filt, general_memory_if_list, size,
                                 source_port_type, fci.debug_level);
   }
   std::list<structural_objectRef> fork_call_list;
   fork_call_list.push_back(fork_call_module);

   const std::vector<std::tuple<std::string, std::string, unsigned int, so_kind>> conn_interface_fork_call_ports = {
       {"Mout_oe_ram_out", "Mout_oe_ram", 1U, port_vector_o_K},
       {"Mout_we_ram_out", "Mout_we_ram", 1U, port_vector_o_K},
       {"Mout_addr_ram_out", "Mout_addr_ram", addr_bus_bitsize, port_vector_o_K},
       {"Mout_Wdata_ram_out", "Mout_Wdata_ram", data_bus_bitsize, port_vector_o_K},
       {"Mout_data_ram_size_out", "Mout_data_ram_size", size_bus_bitsize, port_vector_o_K},
       {"Mout_tag_out", "Mout_tag", fci.outer_tag_bitsize, port_vector_o_K},
       {"Mout_back_pressure_out", "Mout_back_pressure", 1U, port_vector_o_K},
       {"M_DataRdy", "M_DataRdy", 1U, port_vector_o_K},
       {"M_Rdata_ram", "M_Rdata_ram", data_bus_bitsize, port_vector_o_K},
       {"M_tag", "M_tag", fci.outer_tag_bitsize, port_vector_o_K}};

   for(const auto& tp : conn_interface_fork_call_ports)
   {
      const auto pname_conn = std::get<0>(tp);
      const auto pname_filt = std::get<1>(tp);
      const auto size = std::get<2>(tp);
      const auto source_port_type = std::get<3>(tp);
      prepare_resized_connection(connections, pname_conn, conn_interface, pname_filt, fork_call_list, size,
                                 source_port_type, fci.debug_level, fci.channel_number);
   }

   // If there are no Mout_back_pressure_out ports, we add a constant port to avoid undefined signals
   if(!(fci.is_bus_pipelined || fci.use_banked_memory || !fci.is_top_level))
   {
      structural_objectRef port = conn_interface->find_member(
          "Mout_back_pressure_out", fci.channel_number == 1U ? port_o_K : port_vector_o_K, conn_interface);
      if(port)
      {
         const auto port_source = GetPointerS<port_o>(port);
         port_source->add_n_ports(fci.channel_number, port);
         port_o::resize_std_port(1U, 0U, 0, port);
         for(auto index = 0U; index < fci.channel_number; ++index)
         {
            structural_objectRef const_obj =
                SM->add_constant("const_Mout_back_pressure_out_" + STR(index), circuit, port->get_typeRef(), STR(0));
            SM->add_connection(const_obj, port_source->get_port(index));
         }
      }
   }

   INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, fci.debug_level, "Generated Thread to memory connection module");
}

void omp_fork_fu_binding::generate_thread_to_thread_if(
    std::list<structural_objectRef>& general_memory_if_list, std::list<structural_objectRef>& outlined_modules_list,
    std::map<structural_objectRef, std::list<structural_objectRef>, jms_sorter>& connections,
    const HLS_managerRef& HLSMgr, const hlsRef& HLS, const structural_objectRef& circuit,
    const structural_managerRef& SM) const
{
   INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, fci.debug_level, "Generating Thread to Thread connection modules");
   const auto& omp_info = HLSMgr->Rmem->get_omp_allocation_info(fci.fork_id);
   const auto addr_bus_bitsize = HLSMgr->get_address_bitsize();
   const auto data_bus_bitsize = HLSMgr->Rmem->get_bus_data_bitsize();
   const auto size_bus_bitsize = HLSMgr->Rmem->get_bus_size_bitsize();

   const auto conn_interface_name = SINGLE_SOURCE_IF_NAME;
   const auto conn_interface =
       add_module(conn_interface_name, "Thread_to_Thread_connection_interface_" + STR(fci.fork_id), SM, HLS, circuit);
   INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, fci.debug_level, "Added " + STR(conn_interface_name) + " module");

   conn_interface->SetParameter("INNER_TAG", STR((fci.inner_tag_bitsize > 0U)));
   conn_interface->SetParameter("SUBMASK_START", STR(omp_info.proc_id_bitsize + omp_info.proc_addr_bitsize));
   conn_interface->SetParameter("SUBMASK_END",
                                STR(omp_info.proc_addr_bitsize + (fci.context_switch ? fci.threads_size : 0)));

   const std::vector<std::tuple<std::string, std::string, unsigned int, so_kind>> conn_interface_ports = {
       {"Mout_oe_ram", "Mout_oe_ram_local", 1U, port_vector_o_K},
       {"Mout_we_ram", "Mout_we_ram_local", 1U, port_vector_o_K},
       {"Mout_addr_ram", "Mout_addr_ram_local", addr_bus_bitsize, port_vector_o_K},
       {"Mout_Wdata_ram", "Mout_Wdata_ram_local", data_bus_bitsize, port_vector_o_K},
       {"Mout_data_ram_size", "Mout_data_ram_size_local", size_bus_bitsize, port_vector_o_K},
       {"Mout_tag", "Mout_tag_local", std::max(fci.inner_tag_bitsize, 1U), port_vector_o_K},
       {"M_back_pressure_out", "M_back_pressure_local", 1U, port_vector_o_K},
       {"M_DataRdy_out", "M_DataRdy_local", 1U, port_vector_o_K},
       {"M_Rdata_ram_out", "M_Rdata_ram_local", data_bus_bitsize, port_vector_o_K},
       {"M_tag_out", "M_tag_local", std::max(fci.inner_tag_bitsize, 1U), port_vector_o_K}};

   for(const auto& fop : conn_interface_ports)
   {
      const auto pname_conn = std::get<0>(fop);
      const auto pname_filt = std::get<1>(fop);
      const auto size = std::get<2>(fop);
      const auto source_port_type = std::get<3>(fop);
      prepare_resized_connection(connections, pname_conn, conn_interface, pname_filt, general_memory_if_list, size,
                                 source_port_type, fci.debug_level);
   }
   const std::vector<std::tuple<std::string, std::string, unsigned int, so_kind>> conn_interface_threads_ports = {
       {"Mout_oe_ram_out", "S_oe_ram", 1U, port_vector_o_K},
       {"Mout_we_ram_out", "S_we_ram", 1U, port_vector_o_K},
       {"Mout_addr_ram_out", "S_addr_ram", addr_bus_bitsize, port_vector_o_K},
       {"Mout_Wdata_ram_out", "S_Wdata_ram", data_bus_bitsize, port_vector_o_K},
       {"Mout_data_ram_size_out", "S_data_ram_size", size_bus_bitsize, port_vector_o_K},
       {"M_DataRdy", "Sout_DataRdy", 1U, port_vector_o_K},
       {"M_Rdata_ram", "Sout_Rdata_ram", data_bus_bitsize, port_vector_o_K}};
   for(const auto& tp : conn_interface_threads_ports)
   {
      const auto pname_conn = std::get<0>(tp);
      const auto pname_thread = std::get<1>(tp);
      const auto size = std::get<2>(tp);
      const auto source_port_type = std::get<3>(tp);
      prepare_resized_connection(connections, pname_conn, conn_interface, pname_thread, outlined_modules_list, size,
                                 source_port_type, fci.debug_level);
   }
   INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, fci.debug_level, "Generated Thread to Thread connection modules");
}

void omp_fork_fu_binding::generate_reduce_manager_if(
    std::list<structural_objectRef>& outlined_modules_list,
    std::map<structural_objectRef, std::list<structural_objectRef>, jms_sorter>& connections,
    const HLS_managerRef& HLSMgr, const hlsRef& HLS, const structural_objectRef& circuit,
    const structural_managerRef SM) const
{
   INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, fci.debug_level, "Generating Reduce Manager module");
   const auto addr_bus_bitsize = HLSMgr->get_address_bitsize();

   const auto rm = add_module(REDUCE_MANAGER_NAME, STR(REDUCE_MANAGER_NAME) + STR(fci.fork_id), SM, HLS, circuit);
   THROW_ASSERT(rm, "Missing kmp_bambu_reduce_manager module from " + LIBRARY_STD_FU);
   const std::vector<std::tuple<std::string, unsigned int, so_kind>> std_ports = {
       {"kmp_bambu_get_th_local_reduce_data_tid", 32U, port_vector_o_K},
       {"kmp_bambu_get_th_local_reduce_data_ld", 1U, port_vector_o_K},
       {"kmp_bambu_set_th_local_reduce_data_tid", 32U, port_vector_o_K},
       {"kmp_bambu_set_th_local_reduce_data_val", addr_bus_bitsize, port_vector_o_K},
       {"kmp_bambu_set_th_local_reduce_data_we", 1U, port_vector_o_K},
       {"kmp_bambu_set_th_local_reduce_data_ack", 1U, port_vector_o_K},
       {"kmp_bambu_get_th_local_reduce_data_ack", 1U, port_vector_o_K},
       {"kmp_bambu_get_th_local_reduce_data_val", addr_bus_bitsize, port_vector_o_K}};
   for(const auto& p : std_ports)
   {
      const auto pname = std::get<0>(p);
      const auto size = std::get<1>(p);
      const auto source_port_type = std::get<2>(p);
      prepare_resized_connection(connections, pname, rm, pname, outlined_modules_list, size, source_port_type,
                                 fci.debug_level);
   }
   rm->SetParameter("THREAD_NUMBER", STR(fci.nproc));
   INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, fci.debug_level, "Generated Reduce Manager module");
}

void omp_fork_fu_binding::generate_critical_manager_if(
    std::list<structural_objectRef>& outlined_modules_list,
    std::map<structural_objectRef, std::list<structural_objectRef>, jms_sorter>& connections,
    const structural_managerRef SM, const hlsRef& HLS, const structural_objectRef& circuit) const
{
   INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, fci.debug_level, "Generating Critical Manager module");

   const auto cm = add_module(CRITICAL_MANAGER_NAME, STR(CRITICAL_MANAGER_NAME) + STR(fci.fork_id), SM, HLS, circuit);
   THROW_ASSERT(cm, "Missing kmp_bambu_reduce_manager module from " + LIBRARY_STD_FU);

   cm->SetParameter("USE_CS", STR(fci.context_switch ? 1 : 0));
   cm->SetParameter("ACCELERATOR_NUMBER", STR(fci.accelerator_number));
   cm->SetParameter("CONTEXT_NUMBER", STR(fci.context_switch ? fci.context_switch_number : 1));
   const std::vector<std::tuple<std::string, std::string, unsigned int, so_kind>> std_ports = {
       {"kmp_lock_acquired", "kmp_lock_acquired", 1U, port_vector_o_K},
       {"kmp_lock_acquired_tag", "kmp_lock_acquired_tag", fci.threads_size, port_vector_o_K},
       {"kmp_lock_acquired_ack", "kmp_lock_acquired_ack", 1U, port_vector_o_K},
       {"kmp_lock_release", "kmp_lock_release", 1U, port_vector_o_K},
       {"kmp_lock_acquired_ack_tag", "kmp_lock_acquired_ack_tag", fci.threads_size, port_vector_o_K}};
   for(const auto& p : std_ports)
   {
      const auto outlined_name = std::get<0>(p);
      const auto critical_manager_name = std::get<1>(p);
      const auto size = std::get<2>(p);
      const auto source_port_type = std::get<3>(p);
      prepare_resized_connection(connections, critical_manager_name, cm, outlined_name, outlined_modules_list, size,
                                 source_port_type, fci.debug_level);
   }
   INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, fci.debug_level, "Generated Critical Manager module");
}

void omp_fork_fu_binding::generate_barrier_manager_if(
    std::list<structural_objectRef>& outlined_modules_list,
    std::map<structural_objectRef, std::list<structural_objectRef>, jms_sorter>& connections,
    const structural_managerRef SM, const hlsRef& HLS, const structural_objectRef& circuit) const
{
   INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, fci.debug_level, "Generating Barrier Manager module");
   const auto outlined_master = outlined_modules_list.front();

   const auto barrier = add_module(BARRIER_MANAGER_NAME, "kmp_barrier_" + STR(fci.fork_id), SM, HLS, circuit);

   barrier->SetParameter("CONTEXT_NUMBER", STR(fci.context_switch ? fci.context_switch_number : 1));
   barrier->SetParameter("TID_BITSIZE", STR(fci.tid_size));

   std::vector<std::tuple<std::string, unsigned int, so_kind>> std_ports = {
       {"kmp_bambu_barrier_reached_we", 1U, port_vector_o_K}};

   for(const auto& p : std_ports)
   {
      const auto pname = std::get<0>(p);
      const auto size = std::get<1>(p);
      const auto source_port_type = std::get<2>(p);
      prepare_resized_connection(connections, pname, barrier, pname, outlined_modules_list, size, source_port_type,
                                 fci.debug_level);
   }
   if(fci.has_wait_all_thread == 1U)
   {
      const auto wait_all_thread_barrier_port_obj =
          barrier->find_member("kmp_bambu_wait_all_threads_rd", port_o_K, barrier);
      std::list<structural_objectRef> destinations;
      for(const auto& o : outlined_modules_list)
      {
         structural_objectRef port_obj;
         const auto has_port = fu_binding::try_find_port("kmp_bambu_wait_all_threads_rd", o, port_o_K, port_obj);
         if(has_port)
         {
            destinations.push_back(port_obj);
         }
      }
      connections[wait_all_thread_barrier_port_obj] = destinations;
      PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, fci.debug_level,
                    "Added connection between kmp_bambu_wait_all_threads_rd of module " + barrier->get_path() +
                        " and kmp_bambu_wait_all_threads_rd");
   }
   INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, fci.debug_level, "Generated Barrier Manager module");
}

void omp_fork_fu_binding::manage_extern_global_port(const HLS_managerRef HLSMgr, const hlsRef HLS,
                                                    const structural_managerRef SM, structural_objectRef port_in,
                                                    unsigned int _dir, structural_objectRef circuit, unsigned int num)
{
   if(!get_outlined_fnode(port_in->get_owner(), HLSMgr))
   {
      fu_binding::manage_extern_global_port(HLSMgr, HLS, SM, port_in, _dir, circuit, num);
   }
}

void omp_fork_fu_binding::create_connections(
    const structural_managerRef SM, const hlsRef HLS,
    std::map<structural_objectRef, std::list<structural_objectRef>, jms_sorter> connections,
    const structural_objectRef& circuit, unsigned int& _unique_id) const
{
   const auto js_library = HLS->HLS_D->get_technology_manager()->get_library(BUS_MERGER_NAME);

   auto is_vectorizable = [&](std::list<structural_objectRef> ports, structural_objectRef src_port) -> bool {
      bool compatible_portsize = (ports.size() == GetPointerS<port_o>(src_port)->get_ports_size() ? true : false);
      bool compatible_bitsize = (src_port->get_typeRef()->vector_size == ports.front()->get_typeRef()->vector_size) ||
                                (src_port->get_typeRef()->vector_size == 1 &&
                                 ports.front()->get_typeRef()->type == structural_type_descriptor::BOOL);
      for(const auto& p : ports)
      {
         if(p->get_kind() == port_vector_o_K)
         {
            compatible_portsize = false;
         }
      }
      return compatible_portsize && compatible_bitsize;
   };

   for(auto& vc : connections)
   {
      auto& src_port = vc.first;
      auto& ports = vc.second;
      auto& sample_port = ports.front();

      const bool is_same_bitsize_and_portsize =
          src_port->get_typeRef()->vector_size == sample_port->get_typeRef()->vector_size &&
          ((src_port->get_kind() == port_o_K && sample_port->get_kind() == port_o_K) ||
           ((src_port->get_kind() == port_vector_o_K && sample_port->get_kind() == port_vector_o_K) &&
            GetPointerS<port_o>(src_port)->get_port_size() == GetPointerS<port_o>(sample_port)->get_port_size()));

      if(ports.size() == 1 && is_same_bitsize_and_portsize)
      {
         add_smart_connection(src_port, sample_port, _unique_id, circuit, SM);
         _unique_id++;
      }
      else if(src_port->get_kind() == port_vector_o_K && is_vectorizable(ports, src_port))
      {
         unsigned int i = 0;
         for(const auto& p : ports)
         {
            add_smart_connection(GetPointerS<port_o>(src_port)->get_port(i), p, _unique_id, circuit, SM);
            _unique_id++;
            i++;
         }
      }
      else if(GetPointerS<port_o>(src_port)->get_port_direction() == port_o::OUT && is_same_bitsize_and_portsize)
      {
         structural_objectRef sig = add_smart_signal(src_port, _unique_id, circuit, SM);
         SM->add_connection(sig, src_port);
         for(const auto& p : ports)
         {
            SM->add_connection(sig, p);
         }
      }
      else if(GetPointerS<port_o>(src_port)->get_port_direction() == port_o::IN && is_same_bitsize_and_portsize)
      {
         for(unsigned int j = 0U;
             j < (src_port->get_kind() == port_vector_o_K ? GetPointerS<port_o>(src_port)->get_port_size() : 1U); j++)
         {
            const auto js_mod = SM->add_module_from_technology_library(
                STR(BUS_MERGER_NAME) + "_" + src_port->get_id() + "_" + STR(j) + "_" + STR(_unique_id), BUS_MERGER_NAME,
                js_library, circuit, HLS->HLS_D->get_technology_manager());
            const auto js_in_port = GetPointerS<module_o>(js_mod)->get_in_port(0U);
            GetPointerS<port_o>(js_in_port)->add_n_ports(static_cast<unsigned int>(ports.size()), js_in_port);
            port_o::resize_std_port(src_port->get_typeRef()->vector_size, 0U, fci.debug_level, js_in_port);

            unsigned int i = 0;
            for(auto& p : ports)
            {
               add_smart_connection(p->get_kind() == port_o_K ? p : GetPointerS<port_o>(p)->get_port(j),
                                    GetPointerS<port_o>(js_in_port)->get_port(i), _unique_id, circuit, SM);
               _unique_id++;
               i++;
            }

            const auto js_out_port = GetPointerS<module_o>(js_mod)->get_out_port(0U);
            port_o::resize_std_port(src_port->get_typeRef()->vector_size, 0U, fci.debug_level, js_out_port);

            add_smart_connection(
                js_out_port, src_port->get_kind() == port_o_K ? src_port : GetPointerS<port_o>(src_port)->get_port(j),
                _unique_id, circuit, SM);
            _unique_id++;
         }
      }
      else
      {
         THROW_UNREACHABLE("Port size not matching: port " + src_port->get_path() + " : " +
                           GetPointerS<port_o>(src_port)->get_kind_text() + ", " +
                           STR(GetPointerS<port_o>(src_port)->get_typeRef()) + ", " +
                           (GetPointerS<port_o>(src_port)->get_kind() == port_vector_o_K ?
                                STR(GetPointerS<port_o>(src_port)->get_ports_size()) :
                                "0") +
                           " -> sample_port " + sample_port->get_path() + " : " +
                           GetPointerS<port_o>(sample_port)->get_kind_text() + ", " +
                           STR(GetPointerS<port_o>(sample_port)->get_typeRef()) + ", " + STR(ports.size()));
      }
   }
}

void omp_fork_fu_binding::manage_memory_ports_parallel_chained(const HLS_managerRef HLSMgr,
                                                               const structural_managerRef SM,
                                                               const std::list<structural_objectRef>& memory_modules,
                                                               const structural_objectRef circuit, const hlsRef HLS,
                                                               unsigned int& _unique_id)
{
   fci.debug_level = parameters->get_class_debug_level(GET_CLASS(*this));
   INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, fci.debug_level, "Creation of fork_call datapath");
   std::list<structural_objectRef> outlined_modules_list;
   std::map<structural_objectRef, std::list<structural_objectRef>, jms_sorter> connections;

   THROW_ASSERT(memory_modules.size() > 0, "memory module must not be empty");
   structural_objectRef fork_call_module = memory_modules.front()->get_owner();
   fci.fork_id = 0U;

   const auto internalize_port = [](const structural_objectRef& obj) {
      auto port = GetPointer<port_o>(obj);
      port->set_is_global(false);
      port->set_is_extern(false);
      port->set_is_memory(false);
   };

   for(unsigned int n = 0; n < GetPointerS<module_o>(fork_call_module)->get_internal_objects_size(); n++)
   {
      auto mm = GetPointerS<module_o>(fork_call_module)->get_internal_object(n);
      const auto outlined_fnode = get_outlined_fnode(mm, HLSMgr);
      if(outlined_fnode)
      {
         THROW_ASSERT(HLSMgr->isOmpLambdaFunction(outlined_fnode->index),
                      "Statement should be an omp outlined function call. " + STR(outlined_fnode));
         INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, fci.debug_level, "Internalizing ports of module: " + mm->get_path());
         // The list of signals that must remain inside the fork_call
         for(const auto& pname : internalizable_port_name)
         {
            const auto port = mm->find_member(pname, port_o_K, mm);
            if(port)
            {
               internalize_port(port);
               INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, fci.debug_level, "Internalizing port: " + port->get_path());
            }
         }
         INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, fci.debug_level, "Internalized ports");
         outlined_modules_list.push_back(mm);
         const auto last_us = mm->get_id().find_last_of("_");
         const auto prev_us = mm->get_id().find_last_of("_", last_us - 1U);
         const auto fork_id_str = mm->get_id().substr(prev_us + 1U, last_us - prev_us - 1U);
         THROW_ASSERT(!fci.fork_id || fci.fork_id == boost::lexical_cast<unsigned int>(fork_id_str),
                      "All threads must have the same fork_id");
         fci.fork_id = boost::lexical_cast<unsigned int>(fork_id_str);
      }
      else
      {
         THROW_ERROR("This point should not be reached. Module " + mm->get_path() +
                     " is not an outlined and is inside a fork_call module");
      }
   }

   if(outlined_modules_list.size() > 1U)
   {
      struct sort_by_name
      {
         bool operator()(const structural_objectRef& a, const structural_objectRef& b)
         {
            return a->get_id() < b->get_id();
         }
      };
      outlined_modules_list.sort(sort_by_name());
   }

   auto outlined_master = GetPointer<module_o>(outlined_modules_list.front());
   const auto& omp_info = HLSMgr->Rmem->get_omp_allocation_info(fci.fork_id);
   fci.const_use_reduce_manager = outlined_master->has_port("kmp_bambu_get_th_local_reduce_data_ld");
   fci.const_use_critical = outlined_master->has_port("kmp_lock_acquired");
   fci.const_use_reduction = outlined_master->has_port("S_oe_ram");
   fci.const_use_grouped_channels = parameters->isOption(OPT_bus_architecture) &&
                                    parameters->getOption<std::string>(OPT_bus_architecture) == "GROUPED_CHANNEL";
   fci.accelerator_number = static_cast<unsigned int>(outlined_modules_list.size());
   fci.accelerator_number_size = std::max(ceil_log2(fci.accelerator_number), 1U);
   const auto top_symbols = parameters->getOption<std::vector<std::string>>(OPT_top_functions_names);
   THROW_ASSERT(top_symbols.size() == 1, "Expected single top function name");
   const auto top_fnode = HLSMgr->get_ir_manager()->GetFunction(top_symbols.front());
   const auto func_arch = HLSMgr->module_arch->GetArchitecture(
       HLSMgr->CGetFunctionBehavior(top_fnode->index)->CGetBehavioralHelper()->GetFunctionName());
   fci.use_banked_memory = false;
   // Use the banked memory if the top function uses the banked interface.
   if(func_arch->ifaces.find("bus") != func_arch->ifaces.end())
   {
      const auto& bus_if = func_arch->ifaces.at("bus");
      if(bus_if.find(FunctionArchitecture::iface_bank_number) != bus_if.end())
      {
         fci.use_banked_memory = std::stoul((bus_if.find(FunctionArchitecture::iface_bank_number)->second)) > 0U;
      }
   }
   fci.is_top_level = HLSMgr->CGetFunctionBehavior(fci.fork_id)->GetOMPInfo() == nullptr;
   fci.channel_number = std::max(HLSMgr->CGetFunctionBehavior(fci.fork_id)->GetChannelsNumber(), 1U);
   if(fci.use_banked_memory && fci.is_top_level)
   {
      THROW_ASSERT(fci.channel_number >= fci.accelerator_number,
                   "The number of channels " + STR(fci.channel_number) + " is less than the number of accelerators " +
                       STR(fci.accelerator_number) + " and this is not a valid configuration");
   }
   if(fci.const_use_grouped_channels)
   {
      THROW_ASSERT(fci.accelerator_number % fci.channel_number == 0,
                   " The number of accelerators " + STR(fci.accelerator_number) +
                       " is not divisible by the number of required channels " + STR(fci.channel_number));
      fci.channel_per_group = fci.accelerator_number / fci.channel_number;
   }
   fci.has_wait_all_thread = outlined_master->has_port("kmp_bambu_wait_all_threads_rd");
   fci.context_switch = false;
   fci.context_switch_number = 0U;
   fci.threads_size = 0U;
   fci.nproc = 0U;
   fci.inner_tag_bitsize = 0U;
   const auto Mout_tag_port =
       outlined_modules_list.front()->find_member("Mout_tag", port_o_K, outlined_modules_list.front());
   if(Mout_tag_port)
   {
      fci.inner_tag_bitsize = static_cast<unsigned int>(Mout_tag_port->get_typeRef()->vector_size);
   }
   fci.outer_tag_bitsize = fci.inner_tag_bitsize;
   fci.is_bus_pipelined = parameters->isOption(OPT_bus_pipelined) && parameters->getOption<bool>(OPT_bus_pipelined);
   if(!(fci.use_banked_memory && fci.is_top_level) && fci.is_bus_pipelined)
   {
      INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, fci.debug_level, "Size of inner tag: " + STR(fci.accelerator_number_size));
      fci.outer_tag_bitsize +=
          fci.const_use_grouped_channels ? std::max(ceil_log2(fci.channel_per_group), 1U) : fci.accelerator_number_size;
   }

   INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, fci.debug_level, "Number of bus channel: " + STR(fci.channel_number));
   INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, fci.debug_level, "Number of accelerator: " + STR(fci.accelerator_number));
   INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, fci.debug_level, "Size of inner tag: " + STR(fci.inner_tag_bitsize));
   INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, fci.debug_level, "Size of outer tag: " + STR(fci.outer_tag_bitsize));

   add_memory_ports_to_fork_call_module(fork_call_module, outlined_modules_list, circuit, SM);
   propagate_ports_to_fork_call_module(fork_call_module, outlined_modules_list, circuit, SM);

   generate_mem_if(outlined_modules_list, connections, HLSMgr, HLS, circuit, SM);

   generate_barrier_manager_if(outlined_modules_list, connections, SM, HLS, circuit);

   if(fci.const_use_reduce_manager)
   {
      generate_reduce_manager_if(outlined_modules_list, connections, HLSMgr, HLS, circuit, SM);
   }

   if(fci.const_use_critical)
   {
      generate_critical_manager_if(outlined_modules_list, connections, SM, HLS, circuit);
   }

   for(auto o : outlined_modules_list)
   {
      if(o->ExistsParameter("CONTEXT_BIT_START") && fci.context_switch && fci.const_use_reduction)
      {
         o->SetParameter("CONTEXT_BIT_START", STR(omp_info.proc_addr_bitsize + fci.threads_size));
         o->SetParameter("CONTEXT_BIT_END", STR(omp_info.proc_addr_bitsize));
      }
   }

   create_connections(SM, HLS, connections, circuit, _unique_id);
}
