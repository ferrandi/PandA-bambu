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
 *              Copyright (c) 2016-2020 Politecnico di Milano
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
 * @file fu_binding_cs.cpp
 * @brief Derived class to add module scheduler, mem_ctrl_parallel and bind correctly the channels
 *
 * @author Nicola Saporetti <nicola.saporetti@gmail.com>
 */
#include "fu_binding_cs.hpp"
#include "Parameter.hpp"
#include "hls.hpp"
#include "hls_manager.hpp"
#include "hls_target.hpp"
#include "memory.hpp"
#include "memory_cs.hpp"
#include "omp_functions.hpp"
#include "reg_binding_cs.hpp"
#include "structural_manager.hpp"
#include "structural_objects.hpp"
#include "technology_manager.hpp"
/// STD include
#include <cmath>
#include <string>

/// STL include
#include "custom_map.hpp"
#include "custom_set.hpp"

/// utility include
#include "dbgPrintHelper.hpp"
#include "math_function.hpp"

fu_binding_cs::fu_binding_cs(const HLS_managerConstRef _HLSMgr, const unsigned int _function_id, const ParameterConstRef _parameters) : fu_binding(_HLSMgr, _function_id, _parameters)
{
   debug_level = _parameters->get_class_debug_level(GET_CLASS(*this));
}

fu_binding_cs::~fu_binding_cs()
{
}

void fu_binding_cs::add_to_SM(const HLS_managerRef HLSMgr, const hlsRef HLS, structural_objectRef clock_port, structural_objectRef reset_port)
{
   auto omp_functions = GetPointer<OmpFunctions>(HLSMgr->Rfuns);
   if(omp_functions->kernel_functions.find(HLS->functionId) != omp_functions->kernel_functions.end())
   {
      instantiate_component_kernel(HLSMgr, HLS, clock_port, reset_port);
   }
   fu_binding::add_to_SM(HLSMgr, HLS, clock_port, reset_port);
   if(omp_functions->kernel_functions.find(HLS->functionId) != omp_functions->kernel_functions.end())
   {
      connect_selector_kernel(HLS);
   }
   else if(omp_functions->atomic_functions.find(HLS->functionId) != omp_functions->atomic_functions.end() || omp_functions->parallelized_functions.find(HLS->functionId) != omp_functions->parallelized_functions.end())
   {
      connect_selector(HLS);
      set_atomic_memory_parameter(HLS);
   }
}

void fu_binding_cs::instantiate_component_kernel(const HLS_managerRef HLSMgr, const hlsRef HLS, structural_objectRef clock_port, structural_objectRef reset_port)
{
   const structural_managerRef SM = HLS->datapath;
   const structural_objectRef circuit = SM->get_circ();
   std::string scheduler_model = "scheduler";
   std::string scheduler_name = "scheduler_kernel";
   PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, " - Start adding scheduler");
   std::string sche_library = HLS->HLS_T->get_technology_manager()->get_library(scheduler_model);
   structural_objectRef scheduler_mod = SM->add_module_from_technology_library(scheduler_name, scheduler_model, sche_library, circuit, HLS->HLS_T->get_technology_manager());
   PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, " - Added Scheduler");

   PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Starting setting parameter scheduler!");
   GetPointer<module>(scheduler_mod)->SetParameter("NUM_TASKS", STR(parameters->getOption<unsigned int>(OPT_context_switch)));
   int addr_acc = ceil_log2(parameters->getOption<unsigned long long>(OPT_num_accelerators));
   if(!addr_acc)
      addr_acc = 1;
   GetPointer<module>(scheduler_mod)->SetParameter("ADDR_ACC", STR(addr_acc));
   GetPointer<module>(scheduler_mod)->SetParameter("KERN_NUM", "KERN_NUM"); // taken from datapath
   PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Parameter scheduler setted!");

   structural_objectRef clock_scheduler = scheduler_mod->find_member(CLOCK_PORT_NAME, port_o_K, scheduler_mod);
   SM->add_connection(clock_port, clock_scheduler);
   PRINT_DBG_MEX(DEBUG_LEVEL_VERBOSE, debug_level, " - Added clock sche");

   structural_objectRef reset_scheduler = scheduler_mod->find_member(RESET_PORT_NAME, port_o_K, scheduler_mod);
   SM->add_connection(reset_port, reset_scheduler);
   PRINT_DBG_MEX(DEBUG_LEVEL_VERBOSE, debug_level, " - Added reset sche");

   structural_objectRef done_scheduler = scheduler_mod->find_member(DONE_SCHEDULER, port_o_K, scheduler_mod);
   structural_objectRef done_datapath = circuit->find_member(DONE_SCHEDULER, port_o_K, circuit);
   SM->add_connection(done_datapath, done_scheduler);
   PRINT_DBG_MEX(DEBUG_LEVEL_VERBOSE, debug_level, " - Added done sche");

   structural_objectRef start_port_scheduler = scheduler_mod->find_member(STR(START_PORT_NAME) + "_task", port_o_K, scheduler_mod);
   structural_objectRef start_port_datapath = circuit->find_member(STR(START_PORT_NAME) + "_task", port_o_K, circuit);
   SM->add_connection(start_port_datapath, start_port_scheduler);
   PRINT_DBG_MEX(DEBUG_LEVEL_VERBOSE, debug_level, " - Added start sche");

   structural_objectRef task_pool_end_scheduler = scheduler_mod->find_member(STR(TASKS_POOL_END), port_o_K, scheduler_mod);
   structural_objectRef task_pool_end_datapath = circuit->find_member(STR(TASKS_POOL_END), port_o_K, circuit);
   SM->add_connection(task_pool_end_datapath, task_pool_end_scheduler);
   PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, " - Added task_pool_end sche");

   structural_objectRef done_request_scheduler = scheduler_mod->find_member(STR(DONE_REQUEST), port_o_K, scheduler_mod);
   structural_objectRef done_request_datapath = circuit->find_member(STR(DONE_REQUEST), port_o_K, circuit);
   SM->add_connection(done_request_scheduler, done_request_datapath);
   PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, " - Added done_request sche");

   resize_scheduler_ports(HLSMgr, scheduler_mod);
}

void fu_binding_cs::resize_scheduler_ports(const HLS_managerRef HLSMgr, structural_objectRef scheduler_mod)
{
   for(unsigned int j = 0; j < GetPointer<module>(scheduler_mod)->get_in_port_size(); j++) // resize input port
   {
      structural_objectRef port_i = GetPointer<module>(scheduler_mod)->get_in_port(j);
      if(GetPointer<port_o>(port_i)->get_is_memory())
         resize_dimension_bus_port(HLSMgr, port_i);
   }
   for(unsigned int j = 0; j < GetPointer<module>(scheduler_mod)->get_out_port_size(); j++) // resize output port
   {
      structural_objectRef port_i = GetPointer<module>(scheduler_mod)->get_out_port(j);
      if(GetPointer<port_o>(port_i)->get_is_memory())
         resize_dimension_bus_port(HLSMgr, port_i);
   }
}

void fu_binding_cs::resize_dimension_bus_port(const HLS_managerRef HLSMgr, structural_objectRef port)
{
   unsigned int bus_data_bitsize = HLSMgr->Rmem->get_bus_data_bitsize();
   unsigned int bus_addr_bitsize = HLSMgr->get_address_bitsize();
   unsigned int bus_size_bitsize = HLSMgr->Rmem->get_bus_size_bitsize();
   unsigned int bus_tag_bitsize = GetPointer<memory_cs>(HLSMgr->Rmem)->get_bus_tag_bitsize();

   if(GetPointer<port_o>(port)->get_is_data_bus())
      port->type_resize(bus_data_bitsize);
   else if(GetPointer<port_o>(port)->get_is_addr_bus())
      port->type_resize(bus_addr_bitsize);
   else if(GetPointer<port_o>(port)->get_is_size_bus())
      port->type_resize(bus_size_bitsize);
   else if(GetPointer<port_o>(port)->get_is_tag_bus())
      port->type_resize(bus_tag_bitsize);
}

void fu_binding_cs::connect_selector(const hlsRef HLS)
{
   const structural_managerRef SM = HLS->datapath;
   const structural_objectRef circuit = SM->get_circ();
   structural_objectRef port_selector_datapath = circuit->find_member(SELECTOR_REGISTER_FILE, port_o_K, circuit);
   for(unsigned int i = 0; i < GetPointer<module>(circuit)->get_internal_objects_size(); i++)
   {
      structural_objectRef curr_gate = GetPointer<module>(circuit)->get_internal_object(i);
      const structural_objectRef port_selector_module = curr_gate->find_member(STR(SELECTOR_REGISTER_FILE), port_o_K, curr_gate);
      if(port_selector_module != nullptr)
         SM->add_connection(port_selector_datapath, port_selector_module);
   }
}

void fu_binding_cs::connect_selector_kernel(const hlsRef HLS)
{
   const structural_managerRef SM = HLS->datapath;
   const structural_objectRef circuit = SM->get_circ();
   int num_slots = ceil_log2(parameters->getOption<unsigned long long int>(OPT_context_switch)); // resize selector-port
   if(!num_slots)
      num_slots = 1;
   structural_type_descriptorRef port_type = structural_type_descriptorRef(new structural_type_descriptor("bool", static_cast<unsigned>(num_slots)));

   structural_objectRef scheduler_mod = circuit->find_member("scheduler_kernel", component_o_K, circuit);
   structural_objectRef port_selector = scheduler_mod->find_member(STR(SELECTOR_REGISTER_FILE), port_o_K, scheduler_mod);
   port_selector->type_resize(static_cast<unsigned>(num_slots));
   structural_objectRef selector_regFile_scheduler = scheduler_mod->find_member(STR(SELECTOR_REGISTER_FILE), port_o_K, scheduler_mod);
   structural_objectRef selector_regFile_datapath = circuit->find_member(STR(SELECTOR_REGISTER_FILE), port_o_K, circuit);
   structural_objectRef selector_regFile_sign = SM->add_sign(STR(SELECTOR_REGISTER_FILE) + "_signal", circuit, port_type);
   SM->add_connection(selector_regFile_scheduler, selector_regFile_sign);
   SM->add_connection(selector_regFile_sign, selector_regFile_datapath);

   PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, " - Added selector");
   for(unsigned int i = 0; i < GetPointer<module>(circuit)->get_internal_objects_size(); i++)
   {
      structural_objectRef curr_gate = GetPointer<module>(circuit)->get_internal_object(i);
      if(curr_gate->get_id() != "scheduler_kernel")
      {
         const structural_objectRef port_selector_module = curr_gate->find_member(STR(SELECTOR_REGISTER_FILE), port_o_K, curr_gate);
         if(port_selector_module != nullptr)
            SM->add_connection(selector_regFile_sign, port_selector_module);
      }
   }
   PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, " - Connected register");
}

void fu_binding_cs::set_atomic_memory_parameter(const hlsRef HLS)
{
   const structural_managerRef SM = HLS->datapath;
   const structural_objectRef circuit = SM->get_circ();
   for(unsigned int i = 0; i < GetPointer<module>(circuit)->get_internal_objects_size(); i++)
   {
      structural_objectRef curr_gate = GetPointer<module>(circuit)->get_internal_object(i);
      if(curr_gate->ExistsParameter("TAG_MEM_REQ"))
      {
         unsigned long long int tag_num = 0;
         int addr_tasks = ceil_log2(parameters->getOption<unsigned long long int>(OPT_context_switch));
         if(!addr_tasks)
            addr_tasks = 1;
         int addr_acc = ceil_log2(parameters->getOption<unsigned long long>(OPT_num_accelerators));
         if(!addr_acc)
            addr_acc = 1;
         int bit_atomic = addr_tasks + addr_acc;
         if(bit_atomic >= 64)
            THROW_ERROR("too large tag value for TAG_MEM_REQ");
         tag_num = 1ULL << bit_atomic;
         curr_gate->SetParameter("TAG_MEM_REQ", STR(tag_num));
      }
   }
}

void fu_binding_cs::manage_memory_ports_parallel_chained(const HLS_managerRef HLSMgr, const structural_managerRef SM, const std::list<structural_objectRef>& memory_modules, const structural_objectRef circuit, const hlsRef HLS, unsigned int& _unique_id)
{
   auto omp_functions = GetPointer<OmpFunctions>(HLSMgr->Rfuns);
   if(omp_functions->kernel_functions.find(HLS->functionId) != omp_functions->kernel_functions.end())
   {
      manage_memory_port_kernel(SM, memory_modules, circuit, HLS, _unique_id);
   }
   else if(omp_functions->hierarchical_functions.find(HLS->functionId) != omp_functions->hierarchical_functions.end())
   {
      manage_memory_port_hierarchical(SM, memory_modules, circuit, HLS, _unique_id);
   }
   else
      fu_binding::manage_memory_ports_parallel_chained(HLSMgr, SM, memory_modules, circuit, HLS, _unique_id);
}

void fu_binding_cs::manage_memory_port_kernel(const structural_managerRef SM, const std::list<structural_objectRef>& memory_modules, const structural_objectRef circuit, const hlsRef HLS, unsigned int& _unique_id)
{
   std::map<structural_objectRef, std::list<structural_objectRef>, jms_sorter> primary_outs;
   structural_objectRef cir_port;
   structural_objectRef sche_port;
   structural_objectRef scheduler = circuit->find_member("scheduler_kernel", component_o_K, circuit);
   PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, " - Connecting memory_port of scheduler");
   for(unsigned int j = 0; j < GetPointer<module>(scheduler)->get_in_port_size(); j++) // connect input scheduler with datapath input
   {
      structural_objectRef port_i = GetPointer<module>(scheduler)->get_in_port(j);
      std::string port_name = GetPointer<port_o>(port_i)->get_id();
      if(GetPointer<port_o>(port_i)->get_is_memory() && GetPointer<port_o>(port_i)->get_is_global() && GetPointer<port_o>(port_i)->get_is_extern() && port_name.substr(0, 3) == "IN_")
      {
         cir_port = circuit->find_member(port_name.erase(0, 3), port_i->get_kind(), circuit); // erase IN_ from port name
         THROW_ASSERT(!cir_port || GetPointer<port_o>(cir_port), "should be a port or null");
         if(!cir_port)
         {
            if(port_i->get_kind() == port_vector_o_K)
               cir_port = SM->add_port_vector(port_name, port_o::IN, GetPointer<port_o>(port_i)->get_ports_size(), circuit, port_i->get_typeRef());
            else
               cir_port = SM->add_port(port_name, port_o::IN, circuit, port_i->get_typeRef());
            port_o::fix_port_properties(port_i, cir_port);
            SM->add_connection(cir_port, port_i);
         }
         else
         {
            SM->add_connection(cir_port, port_i);
         }
      }
   }

   for(const auto& memory_module : memory_modules) // connect scheduler with memory_modules
   {
      for(unsigned int j = 0; j < GetPointer<const module>(memory_module)->get_in_port_size(); j++)
      {
         structural_objectRef scheMemorySign;
         structural_objectRef port_i = GetPointer<module>(memory_module)->get_in_port(j);
         if(GetPointer<port_o>(port_i)->get_is_memory() && GetPointer<port_o>(port_i)->get_is_global() && GetPointer<port_o>(port_i)->get_is_extern())
         {
            std::string port_name = GetPointer<port_o>(port_i)->get_id();
            sche_port = scheduler->find_member(port_name, port_i->get_kind(), scheduler);
            scheMemorySign = GetPointer<port_o>(sche_port)->get_connected_signal();
            if(scheMemorySign == nullptr)
            {
               scheMemorySign = SM->add_sign(port_name + "_signal", circuit, port_i->get_typeRef());
               SM->add_connection(sche_port, scheMemorySign); // connect port scheduler with signal only one time
            }
            THROW_ASSERT(GetPointer<port_o>(sche_port), "should be a port");
            SM->add_connection(scheMemorySign, port_i);
         }
      }

      for(unsigned int j = 0; j < GetPointer<const module>(memory_module)->get_out_port_size(); j++) // connect memory_modules out with scheduler by jms
      {
         structural_objectRef port_i = GetPointer<const module>(memory_module)->get_out_port(j);
         if(GetPointer<port_o>(port_i)->get_is_memory() && !GetPointer<port_o>(port_i)->get_is_global() && !GetPointer<port_o>(port_i)->get_is_extern())
         {
            std::string port_name = GetPointer<port_o>(port_i)->get_id();
            sche_port = scheduler->find_member(port_name, port_i->get_kind(), scheduler);
            THROW_ASSERT(!sche_port || GetPointer<port_o>(sche_port), "should be a port");
            if(std::find(primary_outs[sche_port].begin(), primary_outs[sche_port].end(), port_i) == primary_outs[sche_port].end())
               primary_outs[sche_port].push_back(port_i);
         }
      }
   }
   join_merge_split(SM, HLS, primary_outs, circuit, _unique_id);

   for(unsigned int j = 0; j < GetPointer<module>(scheduler)->get_out_port_size(); j++) // connect scheduler out with datapath out
   {
      structural_objectRef port_i = GetPointer<module>(scheduler)->get_out_port(j);
      std::string port_name = GetPointer<port_o>(port_i)->get_id();
      if(GetPointer<port_o>(port_i)->get_is_memory() && !GetPointer<port_o>(port_i)->get_is_global() && !GetPointer<port_o>(port_i)->get_is_extern() && port_name.substr(0, 4) == "OUT_")
      {
         cir_port = circuit->find_member(port_name.erase(0, 4), port_i->get_kind(), circuit); // erase OUT_ from port name
         THROW_ASSERT(!cir_port || GetPointer<port_o>(cir_port), "should be a port or null");
         if(!cir_port)
         {
            if(port_i->get_kind() == port_vector_o_K)
               cir_port = SM->add_port_vector(port_name, port_o::OUT, GetPointer<port_o>(port_i)->get_ports_size(), circuit, port_i->get_typeRef());
            else
               cir_port = SM->add_port(port_name, port_o::OUT, circuit, port_i->get_typeRef());
            port_o::fix_port_properties(port_i, cir_port);
         }
         SM->add_connection(port_i, cir_port);
      }
   }
   PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, " - Connected memory_port of scheduler");
}

void fu_binding_cs::manage_memory_port_hierarchical(const structural_managerRef SM, const std::list<structural_objectRef>& memory_modules, const structural_objectRef circuit, const hlsRef HLS, unsigned int& _unique_id)
{
   std::map<structural_objectRef, std::list<structural_objectRef>, jms_sorter> primary_outs;
   structural_objectRef cir_port;
   PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, " - Start merging, splitting for hierarchical");
   for(const auto& memory_module : memory_modules)
   {
      for(unsigned int j = 0; j < GetPointer<const module>(memory_module)->get_in_port_size(); j++)
      {
         structural_objectRef port_i = GetPointer<const module>(memory_module)->get_in_port(j);
         if(GetPointer<port_o>(port_i)->get_is_memory() && GetPointer<port_o>(port_i)->get_is_global() && GetPointer<port_o>(port_i)->get_is_extern())
         {
            std::string port_name = GetPointer<port_o>(port_i)->get_id();
            cir_port = circuit->find_member(port_name, port_i->get_kind(), circuit);
            THROW_ASSERT(!cir_port || GetPointer<port_o>(cir_port), "should be a port or null");
            if(!cir_port)
            {
               cir_port = SM->add_port_vector(port_name, port_o::IN, parameters->getOption<unsigned int>(OPT_channels_number), circuit, port_i->get_typeRef());
               port_o::fix_port_properties(port_i, cir_port);
            }
            if(port_i->get_kind() == port_vector_o_K) // connecting a port vector
            {
               SM->add_connection(GetPointer<port_o>(cir_port)->get_port(0), GetPointer<port_o>(port_i)->get_port(0)); // connect first port
               if(GetPointer<port_o>(port_i)->get_ports_size() > 1)                                                    // More than 1 channel
               {
                  for(unsigned int num_chan = 1; num_chan < parameters->getOption<unsigned int>(OPT_channels_number); num_chan++)
                  {
                     SM->add_connection(GetPointer<port_o>(cir_port)->get_port(num_chan), GetPointer<port_o>(port_i)->get_port(num_chan)); // connect other port one by one
                  }
               }
            }
            else
               SM->add_connection(GetPointer<port_o>(cir_port)->get_port(0), port_i); // connect a normal port
         }
      }

      for(unsigned int j = 0; j < GetPointer<const module>(memory_module)->get_out_port_size(); j++)
      {
         structural_objectRef port_i = GetPointer<const module>(memory_module)->get_out_port(j);
         if(GetPointer<port_o>(port_i)->get_is_memory() && !GetPointer<port_o>(port_i)->get_is_global() && !GetPointer<port_o>(port_i)->get_is_extern())
         {
            std::string port_name = GetPointer<port_o>(port_i)->get_id();
            cir_port = circuit->find_member(port_name, port_i->get_kind(), circuit);
            THROW_ASSERT(!cir_port || GetPointer<port_o>(cir_port), "should be a port or null");
            if(!cir_port)
            {
               cir_port = SM->add_port_vector(port_name, port_o::OUT, parameters->getOption<unsigned int>(OPT_channels_number), circuit, port_i->get_typeRef());
               port_o::fix_port_properties(port_i, cir_port);
            }
            if(port_i->get_kind() == port_vector_o_K) // connecting a port vector
            {
               if(std::find(primary_outs[GetPointer<port_o>(cir_port)->get_port(0)].begin(), primary_outs[GetPointer<port_o>(cir_port)->get_port(0)].end(), GetPointer<port_o>(port_i)->get_port(0)) ==
                  primary_outs[GetPointer<port_o>(cir_port)->get_port(0)].end())
                  primary_outs[GetPointer<port_o>(cir_port)->get_port(0)].push_back(GetPointer<port_o>(port_i)->get_port(0)); // merge first cell of vector
               if(GetPointer<port_o>(port_i)->get_ports_size() > 1)                                                           // More than 1 channel
               {
                  for(unsigned int num_chan = 1; num_chan < parameters->getOption<unsigned int>(OPT_channels_number); num_chan++)
                  {
                     SM->add_connection(GetPointer<port_o>(port_i)->get_port(num_chan), GetPointer<port_o>(cir_port)->get_port(num_chan)); // connect other port one by one
                  }
               }
            }
            else if(std::find(primary_outs[GetPointer<port_o>(cir_port)->get_port(0)].begin(), primary_outs[GetPointer<port_o>(cir_port)->get_port(0)].end(), port_i) == primary_outs[GetPointer<port_o>(cir_port)->get_port(0)].end())
               primary_outs[GetPointer<port_o>(cir_port)->get_port(0)].push_back(port_i); // merge normal port
         }
      }
   }
   join_merge_split(SM, HLS, primary_outs, circuit, _unique_id);
   PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, " - End merging, splitting for hierarchical");
}

void fu_binding_cs::manage_extern_global_port(const HLS_managerRef HLSMgr, const hlsRef HLS, const structural_managerRef SM, structural_objectRef port_in, unsigned int _dir, structural_objectRef circuit, unsigned int num)
{
   if(_dir == port_o::IO)
   {
      THROW_ERROR("port declared as IO not accepted in context switch");
   }
   else
   {
      auto omp_functions = GetPointer<OmpFunctions>(HLSMgr->Rfuns);
      if(omp_functions->kernel_functions.find(HLS->functionId) != omp_functions->kernel_functions.end() || omp_functions->hierarchical_functions.find(HLS->functionId) != omp_functions->hierarchical_functions.end())
      {
         if(!GetPointer<port_o>(port_in)->get_is_memory()) // if not a memory port then use standard method
            fu_binding::manage_extern_global_port(HLSMgr, HLS, SM, port_in, _dir, circuit, num);
         // otherwise do nothing because memory port are managed by method manage_memory_port called after
      }
      else
         fu_binding::manage_extern_global_port(HLSMgr, HLS, SM, port_in, _dir, circuit, num);
   }
}
