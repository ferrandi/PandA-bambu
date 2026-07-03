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
 * @file omp_fork_fu_binding.hpp
 * @brief
 *
 * @author Giovanni Gozzi <giovanni.gozzi@polimi.it>
 *
 */
#ifndef OMP_FORK_FU_BINDING_HPP
#define OMP_FORK_FU_BINDING_HPP

#include "fu_binding.hpp"

#include "custom_map.hpp"
#include "custom_set.hpp"
#include "refcount.hpp"

#include <list>
#include <vector>

REF_FORWARD_DECL(ir_node);

class omp_fork_fu_binding : public fu_binding
{
   static constexpr const char* GENERAL_MEMORY_IF_NAME = "kmp_bambu_general_memory_interface";
   static constexpr const char* GROUPED_CHANNEL_IF_NAME = "Grouped_channel_connection_interface";
   static constexpr const char* BANKED_MEMORY_IF_NAME = "Banked_memory_connection_interface";
   static constexpr const char* SIMPLE_ARBITER_IF_NAME = "Simple_arbiter_connection_interface";
   static constexpr const char* ARBITER_IF_NAME = "Arbiter_connection_interface";
   static constexpr const char* SINGLE_SOURCE_IF_NAME = "Single_source_connection_interface";
   static constexpr const char* BUS_MERGER_NAME = "bus_merger";
   static constexpr const char* REDUCE_MANAGER_NAME = "kmp_bambu_reduce_manager";
   static constexpr const char* CRITICAL_MANAGER_NAME = "kmp_bambu_fork_call_critical_manager";
   static constexpr const char* BARRIER_MANAGER_NAME = "kmp_bambu_barrier_register";

   CustomMap<structural_objectRef, ir_nodeRef> obj_fnode;
   struct fork_call_infos
   {
      bool const_use_reduce_manager;
      bool const_use_critical;
      bool const_use_reduction;
      bool const_use_grouped_channels;
      bool has_wait_all_thread;
      bool context_switch;
      bool use_banked_memory;
      bool is_top_level;
      bool is_bus_pipelined;
      int debug_level;
      unsigned int fork_id;
      unsigned int channel_number;
      unsigned int accelerator_number;
      unsigned int accelerator_number_size;
      unsigned int inner_tag_bitsize;
      unsigned int outer_tag_bitsize;
      unsigned int threads_size;
      unsigned int context_switch_number;
      unsigned int nproc;
      unsigned int tid_size;
      unsigned int channel_per_group;
   } fci;

   static constexpr std::array<const char*, 28> internalizable_port_name = {"S_oe_ram",
                                                                            "S_we_ram",
                                                                            "S_addr_ram",
                                                                            "S_Wdata_ram",
                                                                            "S_data_ram_size",
                                                                            "Sout_Rdata_ram",
                                                                            "Sout_DataRdy",
                                                                            "Mout_oe_ram",
                                                                            "Mout_we_ram",
                                                                            "Mout_addr_ram",
                                                                            "Mout_Wdata_ram",
                                                                            "Mout_data_ram_size",
                                                                            "Mout_tag",
                                                                            "M_Rdata_ram",
                                                                            "M_DataRdy",
                                                                            "M_tag",
                                                                            "Min_oe_ram",
                                                                            "Min_we_ram",
                                                                            "Min_addr_ram",
                                                                            "Min_Wdata_ram",
                                                                            "Min_data_ram_size",
                                                                            "Sin_Rdata_ram",
                                                                            "Sin_DataRdy",
                                                                            "kmp_bambu_wait_all_threads_rd",
                                                                            "Mout_back_pressure",
                                                                            "kmp_bambu_barrier_reached_tid",
                                                                            "kmp_bambu_barrier_reached_we",
                                                                            "selector_register_file"};

   ir_nodeRef get_outlined_fnode(const structural_objectRef& obj, const HLS_managerRef& HLSMgr);

   const structural_objectRef add_module(std::string fu_name, std::string name, const structural_managerRef SM,
                                         const hlsRef& HLS, const structural_objectRef& circuit) const;

   void bind_ports(
       const std::list<structural_objectRef>& memory_modules, CustomSet<structural_objectRef>& already_connected,
       const structural_managerRef& SM, const structural_objectRef& circuit, unsigned int& _unique_id,
       std::map<structural_objectRef, std::list<structural_objectRef>, jms_sorter>* primary_outs = nullptr) const;

   void add_memory_ports_to_fork_call_module(structural_objectRef& fork_call_module,
                                             std::list<structural_objectRef>& outlined_modules_list,
                                             const structural_objectRef& circuit,
                                             const structural_managerRef& SM) const;

   void propagate_ports_to_fork_call_module(structural_objectRef& fork_call_module,
                                            std::list<structural_objectRef> outlined_modules,
                                            const structural_objectRef& circuit, const structural_managerRef& SM) const;

   void generate_mem_if(std::list<structural_objectRef>& outlined_modules_list,
                        std::map<structural_objectRef, std::list<structural_objectRef>, jms_sorter>& connections,
                        const HLS_managerRef& HLSMgr, const hlsRef& HLS, const structural_objectRef& circuit,
                        const structural_managerRef& SM);

   void generate_thread_to_memory_if(
       std::list<structural_objectRef>& general_memory_if_list, std::list<structural_objectRef>& outlined_modules_list,
       std::map<structural_objectRef, std::list<structural_objectRef>, jms_sorter>& connections,
       const HLS_managerRef& HLSMgr, const hlsRef& HLS, const structural_objectRef& circuit,
       const structural_managerRef& SM) const;

   void generate_thread_to_thread_if(
       std::list<structural_objectRef>& general_memory_if_list, std::list<structural_objectRef>& outlined_modules_list,
       std::map<structural_objectRef, std::list<structural_objectRef>, jms_sorter>& connections,
       const HLS_managerRef& HLSMgr, const hlsRef& HLS, const structural_objectRef& circuit,
       const structural_managerRef& SM) const;

   void
   generate_reduce_manager_if(std::list<structural_objectRef>& outlined_modules_list,
                              std::map<structural_objectRef, std::list<structural_objectRef>, jms_sorter>& connections,
                              const HLS_managerRef& HLSMgr, const hlsRef& HLS, const structural_objectRef& circuit,
                              const structural_managerRef SM) const;

   void
   generate_barrier_manager_if(std::list<structural_objectRef>& outlined_modules_list,
                               std::map<structural_objectRef, std::list<structural_objectRef>, jms_sorter>& connections,
                               const structural_managerRef SM, const hlsRef& HLS,
                               const structural_objectRef& circuit) const;

   void generate_critical_manager_if(
       std::list<structural_objectRef>& outlined_modules_list,
       std::map<structural_objectRef, std::list<structural_objectRef>, jms_sorter>& connections,
       const structural_managerRef SM, const hlsRef& HLS, const structural_objectRef& circuit) const;

   void create_connections(const structural_managerRef SM, const hlsRef HLS,
                           std::map<structural_objectRef, std::list<structural_objectRef>, jms_sorter> connections,
                           const structural_objectRef& circuit, unsigned int& _unique_id) const;

 public:
   omp_fork_fu_binding(const HLS_managerConstRef _HLSMgr, const unsigned int function_id,
                       const ParameterConstRef parameters);

   void manage_extern_global_port(const HLS_managerRef, const hlsRef, const structural_managerRef SM,
                                  structural_objectRef port_in, unsigned int _dir, structural_objectRef circuit,
                                  unsigned int num) final;

   void manage_memory_ports_parallel_chained(const HLS_managerRef, const structural_managerRef SM,
                                             const std::list<structural_objectRef>& memory_modules,
                                             const structural_objectRef circuit, const hlsRef HLS,
                                             unsigned int& _unique_id) final;
};
#endif
