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
 *              Copyright (c) 2015-2020 Politecnico di Milano
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
 * @file parallel_memory_fu_binding.cpp
 * @brief Data structure used to store the functional-unit binding of the vertices.
 *
 * @author Marco Lattuada <marco.lattuada@polimi.it>
 *
 */
/// Header class
#include "parallel_memory_fu_binding.hpp"

///. include
#include "Parameter.hpp"

/// circuit include
#include "structural_manager.hpp"

/// HLS includes
#include "hls.hpp"
#include "hls_target.hpp"

/// HLS/functions include
#include "omp_functions.hpp"

/// HLS/module_allocation include
#include "allocation_information.hpp"

/// HLS/virtual_components
#include "generic_obj.hpp"

/// STD include
#include <string>

/// STL include
#include <list>

/// technology includes
#include "technology_manager.hpp"
#include "technology_node.hpp"

/// tree includes
#include "tree_manager.hpp"
#include "tree_node.hpp"

/// utility include
#include "dbgPrintHelper.hpp"
#include "utility.hpp"

ParallelMemoryFuBinding::ParallelMemoryFuBinding(const HLS_managerConstRef _HLS_mgr, const unsigned int _function_id, const ParameterConstRef _parameters) : fu_binding(_HLS_mgr, _function_id, _parameters)
{
   debug_level = parameters->get_class_debug_level(GET_CLASS(*this));
}

void ParallelMemoryFuBinding::add_to_SM(const HLS_managerRef HLSMgr, const hlsRef HLS, structural_objectRef clock_port, structural_objectRef reset_port)
{
   fu_binding::add_to_SM(HLSMgr, HLS, clock_port, reset_port);
   const auto& parallelized_functions = GetPointer<const OmpFunctions>(HLSMgr->Rfuns)->parallelized_functions;
   const auto& omp_for_wrappers = GetPointer<const OmpFunctions>(HLSMgr->Rfuns)->omp_for_wrappers;
   const auto TM = HLS->HLS_T->get_technology_manager();
   const auto memory_banks_number = parameters->getOption<unsigned int>(OPT_memory_banks_number);
   const structural_managerRef SM = HLS->datapath;
   const structural_objectRef circuit = SM->get_circ();
   if(parallelized_functions.find(HLS->functionId) != parallelized_functions.end())
   {
      std::list<structural_objectRef> memory_enableds;
      bool load_store_operation = [&]() -> bool {
         for(const auto fu_index : get_allocation_list())
         {
            std::string channels_type = GetPointer<const functional_unit>(allocation_information->get_fu(fu_index))->channels_type;
            if(channels_type == CHANNELS_TYPE_MEM_ACC_P1N)
            {
               return true;
            }
         }
         return false;
      }();
      /// Collecting memory enabled signals which have to be put in and
      for(unsigned int internal_object_id = 0; internal_object_id < GetPointer<const module>(circuit)->get_internal_objects_size(); internal_object_id++)
      {
         const auto internal_object = GetPointer<const module>(circuit)->get_internal_object(internal_object_id);
         const auto memory_enabled = internal_object->find_member("memory_enabled", port_o_K, internal_object);
         if(memory_enabled)
         {
            memory_enableds.push_back(memory_enabled);
         }
      }
      if(load_store_operation)
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Adding ALLOW_MEM_ACCESS_FU for memory operations");
         const auto allow_mem_access = SM->add_module_from_technology_library("allow_memory_access", "ALLOW_MEM_ACCESS_FU", LIBRARY_PC, circuit, TM);
         const auto clock = allow_mem_access->find_member(CLOCK_PORT_NAME, port_o_K, allow_mem_access);
         SM->add_connection(clock_port, clock);
         const auto reset = allow_mem_access->find_member(RESET_PORT_NAME, port_o_K, allow_mem_access);
         SM->add_connection(reset_port, reset);
         const auto amaf_access_allowed = allow_mem_access->find_member("access_allowed", port_o_K, allow_mem_access);
         GetPointer<port_o>(amaf_access_allowed)->add_n_ports(memory_banks_number, amaf_access_allowed);
         HLS->Rfu->manage_extern_global_port(HLSMgr, HLS, SM, amaf_access_allowed, port_o::IN, circuit, 0);

         // start_port and done_port are connected in ParallelMemoryConnBinding::add_to_SM

         const auto memory_enabled = allow_mem_access->find_member("memory_enabled", port_o_K, allow_mem_access);
         GetPointer<port_o>(memory_enabled)->add_n_ports(memory_banks_number, memory_enabled);
         memory_enableds.push_back(memory_enabled);

         for(unsigned int internal_object_id = 0; internal_object_id < GetPointer<const module>(circuit)->get_internal_objects_size(); internal_object_id++)
         {
            const auto internal_object = GetPointer<const module>(circuit)->get_internal_object(internal_object_id);
            if(internal_object->get_typeRef()->id_type == "MEMORY_CTRL_P1N")
            {
               component_to_allow_mem_access[internal_object] = allow_mem_access;
            }
         }
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Added ALLOW_MEM_ACCESS_FU for memory operations");
      }
      /// Considering atomic operations
      CustomSet<structural_objectRef> atomics;
      VertexIterator operation, operation_end;
      for(boost::tie(operation, operation_end) = boost::vertices(*op_graph); operation != operation_end; operation++)
      {
         if((GET_TYPE(op_graph, *operation) & TYPE_ATOMIC) != 0)
         {
            atomics.insert(get(*operation)->get_structural_obj());
         }
      }
      for(const auto& atomic : atomics)
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Adding ALLOW_MEM_ACCESS_FU for " + atomic->get_path());
         const auto allow_mem_access = SM->add_module_from_technology_library("allow_memory_access_" + atomic->get_id(), "ALLOW_MEM_ACCESS_FU", LIBRARY_PC, circuit, TM);
         component_to_allow_mem_access[atomic] = allow_mem_access;

         const auto clock = allow_mem_access->find_member(CLOCK_PORT_NAME, port_o_K, allow_mem_access);
         SM->add_connection(clock_port, clock);
         const auto reset = allow_mem_access->find_member(RESET_PORT_NAME, port_o_K, allow_mem_access);
         SM->add_connection(reset_port, reset);
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Connected clock and reset");

         const auto amaf_access_allowed = allow_mem_access->find_member("access_allowed", port_o_K, allow_mem_access);
         GetPointer<port_o>(amaf_access_allowed)->add_n_ports(memory_banks_number, amaf_access_allowed);
         HLS->Rfu->manage_extern_global_port(HLSMgr, HLS, SM, amaf_access_allowed, port_o::IN, circuit, 0);
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Connected access allowed");

         // Done is connected in ParallelMemoryConnBinding::add_to_SM

         const auto memory_enabled = allow_mem_access->find_member("memory_enabled", port_o_K, allow_mem_access);
         GetPointer<port_o>(memory_enabled)->add_n_ports(memory_banks_number, memory_enabled);
         memory_enableds.push_back(memory_enabled);
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Resized memory enabled");

         // Op is connected in ParallelMemoryConnBinding::add_to_SM

         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Added ALLOW_MEM_ACCESS_FU for " + atomic->get_path());
      }
      /// Check if there is at least a memory enabled to be connected
      if(memory_enableds.size())
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->" + STR(memory_enableds.size()) + " memory enableds to be put in and");
         size_t and_counter = 0;
         size_t and_counter_sig = 0;
         while(memory_enableds.size() > 1)
         {
            const auto first = memory_enableds.front();
            memory_enableds.pop_front();
            const auto first_signal = SM->add_sign_vector("memory_enabled_and_sign_" + STR(and_counter_sig), memory_banks_number, circuit, first->get_typeRef());
            SM->add_connection(first, first_signal);
            and_counter_sig++;

            const auto second = memory_enableds.front();
            memory_enableds.pop_front();
            const auto second_signal = SM->add_sign_vector("memory_enabled_and_sign_" + STR(and_counter_sig), memory_banks_number, circuit, second->get_typeRef());
            SM->add_connection(second, second_signal);
            and_counter_sig++;

            const auto and_port = SM->add_module_from_technology_library("memory_enabled_and_" + STR(and_counter), "MEMORY_ENABLED_AND", LIBRARY_PC, circuit, TM);
            const auto out_memory_enabled = and_port->find_member("memory_enabled", port_o_K, and_port);
            GetPointer<port_o>(out_memory_enabled)->add_n_ports(memory_banks_number, out_memory_enabled);
            and_counter++;

            const auto first_port = and_port->find_member("in1", port_o_K, and_port);
            GetPointer<port_o>(first_port)->add_n_ports(memory_banks_number, first_port);
            SM->add_connection(first_signal, first_port);

            const auto second_port = and_port->find_member("in2", port_o_K, and_port);
            GetPointer<port_o>(second_port)->add_n_ports(memory_banks_number, second_port);
            SM->add_connection(second_signal, second_port);

            memory_enableds.push_back(out_memory_enabled);
         }
         GetPointer<port_o>(memory_enableds.front())->set_is_memory(true);
         GetPointer<port_o>(memory_enableds.front())->set_is_global(true);
         GetPointer<port_o>(memory_enableds.front())->set_is_extern(true);
         HLS->Rfu->manage_extern_global_port(HLSMgr, HLS, SM, memory_enableds.front(), port_o::OUT, circuit, 0);
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Generated memory enabled and");
      }
      else
      {
         THROW_UNREACHABLE("");
      }
   }
   else if(omp_for_wrappers.find(HLS->functionId) != omp_for_wrappers.end())
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Adding PT_FU");
      std::list<structural_objectRef> slaves;
      std::list<structural_objectRef> memory_enableds;
      /// Collecting ports of internal objects
      for(unsigned int internal_object_id = 0; internal_object_id < GetPointer<const module>(circuit)->get_internal_objects_size(); internal_object_id++)
      {
         const auto internal_object = GetPointer<const module>(circuit)->get_internal_object(internal_object_id);
         const auto access_request = internal_object->find_member("access_request", port_o_K, internal_object);
         if(access_request)
         {
            slaves.push_back(internal_object);
            memory_enableds.push_back(internal_object->find_member("memory_enabled", port_o_K, internal_object));
         }
      }
      size_t and_counter = 0;
      size_t and_counter_sig = 0;
      while(memory_enableds.size() > 1)
      {
         const auto first = memory_enableds.front();
         memory_enableds.pop_front();
         const auto first_signal = SM->add_sign_vector("memory_enabled_and_sign_" + STR(and_counter_sig), memory_banks_number, circuit, first->get_typeRef());
         SM->add_connection(first, first_signal);
         and_counter_sig++;

         const auto second = memory_enableds.front();
         memory_enableds.pop_front();
         const auto second_signal = SM->add_sign_vector("memory_enabled_and_sign_" + STR(and_counter_sig), memory_banks_number, circuit, second->get_typeRef());
         SM->add_connection(second, second_signal);
         and_counter_sig++;

         const auto and_port = SM->add_module_from_technology_library("memory_enabled_and_" + STR(and_counter), "MEMORY_ENABLED_AND", LIBRARY_PC, circuit, TM);
         const auto out_memory_enabled = and_port->find_member("memory_enabled", port_o_K, and_port);
         GetPointer<port_o>(out_memory_enabled)->add_n_ports(memory_banks_number, out_memory_enabled);
         and_counter++;

         const auto first_port = and_port->find_member("in1", port_o_K, and_port);
         GetPointer<port_o>(first_port)->add_n_ports(memory_banks_number, first_port);
         SM->add_connection(first_signal, first_port);

         const auto second_port = and_port->find_member("in2", port_o_K, and_port);
         GetPointer<port_o>(second_port)->add_n_ports(memory_banks_number, second_port);
         SM->add_connection(second_signal, second_port);

         memory_enableds.push_back(out_memory_enabled);
      }
      for(unsigned int memory_bank_index = 0; memory_bank_index < memory_banks_number; memory_bank_index++)
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Considering memory bank " + STR(memory_bank_index));
         const auto pt_rr_fu = SM->add_module_from_technology_library("pt_rr_fu_" + STR(memory_bank_index), "PT_FU_MEM", LIBRARY_PC, circuit, TM);
         // const auto clock = pt_rr_fu->find_member(CLOCK_PORT_NAME, port_o_K, pt_rr_fu);
         // SM->add_connection(clock_port, clock);
         // const auto reset = pt_rr_fu->find_member(RESET_PORT_NAME, port_o_K, pt_rr_fu);
         // SM->add_connection(reset_port, reset);
         const auto rops_so = pt_rr_fu->find_member("rops", port_o_K, pt_rr_fu);
         THROW_ASSERT(rops_so, "");
         const auto rops = GetPointer<port_o>(rops_so);
         rops->add_n_ports(static_cast<unsigned int>(slaves.size()), rops_so);
         const auto ops_so = pt_rr_fu->find_member("ops", port_o_K, pt_rr_fu);
         THROW_ASSERT(ops_so, "");
         const auto ops = GetPointer<port_o>(ops_so);
         ops->add_n_ports(static_cast<unsigned int>(slaves.size()), ops_so);
         const auto enable_so = pt_rr_fu->find_member("ENABLE", port_o_K, pt_rr_fu);
         THROW_ASSERT(enable_so, "");

         unsigned int internal_object_id = 0;
         for(const auto& slave : slaves)
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Considering component " + slave->get_path());
            THROW_ASSERT(slave->find_member("access_request", port_o_K, slave), "");
            const auto int_access_request = GetPointer<port_o>(slave->find_member("access_request", port_o_K, slave))->get_port(memory_bank_index);
            const auto access_request_sign = SM->add_sign("access_request_" + STR(internal_object_id) + "_" + STR(memory_bank_index), circuit, int_access_request->get_typeRef());
            const auto rop = rops->get_port(internal_object_id);
            SM->add_connection(int_access_request, access_request_sign);
            SM->add_connection(access_request_sign, rop);
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Connected access request");

            THROW_ASSERT(slave->find_member("access_allowed", port_o_K, slave), "");
            const auto int_access_allowed = GetPointer<port_o>(slave->find_member("access_allowed", port_o_K, slave))->get_port(memory_bank_index);
            const auto access_allowed_sign = SM->add_sign("access_allowed_" + STR(internal_object_id) + "_" + STR(memory_bank_index), circuit, int_access_allowed->get_typeRef());
            const auto op = ops->get_port(internal_object_id);
            SM->add_connection(int_access_allowed, access_allowed_sign);
            SM->add_connection(access_allowed_sign, op);
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Connected access allowed");

            internal_object_id++;
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Considered component " + slave->get_path());
         }
         const auto int_enable = GetPointer<port_o>(memory_enableds.front())->get_port(memory_bank_index);
         const auto enable_sign = SM->add_sign("enable_" + STR(memory_bank_index), circuit, int_enable->get_typeRef());
         SM->add_connection(int_enable, enable_sign);
         SM->add_connection(enable_sign, enable_so);
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Connected enabled");
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Considered memory bank " + STR(memory_bank_index));
      }
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Added PT_FU");
   }
   else
   {
      for(unsigned int internal_object_id = 0; internal_object_id < GetPointer<const module>(circuit)->get_internal_objects_size(); internal_object_id++)
      {
         const auto internal_object = GetPointer<const module>(circuit)->get_internal_object(internal_object_id);
         if(internal_object->get_typeRef()->id_type == "MEMORY_CTRL_P1N")
         {
            const auto access_allowed = internal_object->find_member("access_allowed", port_o_K, internal_object);
            for(unsigned int memory_bank_index = 0; memory_bank_index < memory_banks_number; memory_bank_index++)
            {
               const auto constant = SM->add_constant("access_allowed_enabled_" + STR(memory_bank_index), circuit, access_allowed->get_typeRef(), "1");
               SM->add_connection(constant, GetPointer<port_o>(access_allowed)->get_port(memory_bank_index));
            }
         }
         else
         {
            const auto access_allowed = internal_object->find_member("access_allowed", port_o_K, internal_object);
            if(access_allowed)
            {
               for(unsigned int memory_bank_index = 0; memory_bank_index < memory_banks_number; memory_bank_index++)
               {
                  const auto constant = SM->add_constant("access_allowed_enabled_" + internal_object->get_id() + "_" + STR(memory_bank_index), circuit, access_allowed->get_typeRef(), "1");
                  SM->add_connection(constant, GetPointer<port_o>(access_allowed)->get_port(memory_bank_index));
               }
            }
         }
      }
   }
}

bool ParallelMemoryFuBinding::manage_module_ports(const HLS_managerRef HLSMgr, const hlsRef HLS, const structural_managerRef SM, const structural_objectRef curr_gate, unsigned int num)
{
   const auto TM = HLSMgr->get_tree_manager();
   const auto memory_enabled = curr_gate->find_member("memory_enabled", port_o_K, curr_gate);
   if(memory_enabled)
   {
      GetPointer<port_o>(memory_enabled)->set_is_memory(false);
      GetPointer<port_o>(memory_enabled)->set_is_global(false);
      GetPointer<port_o>(memory_enabled)->set_is_extern(false);
   }
   const auto& parallelized_functions = GetPointer<const OmpFunctions>(HLSMgr->Rfuns)->parallelized_functions;
   if(TM->function_index(curr_gate->get_typeRef()->id_type))
   {
      const auto fd = GetPointer<const function_decl>(TM->get_tree_node_const(TM->function_index(curr_gate->get_typeRef()->id_type)));
      if(fd->omp_atomic)
      {
         const auto access_allowed = curr_gate->find_member("access_allowed", port_o_K, curr_gate);
         if(access_allowed)
         {
            access_allowed_killeds.insert(curr_gate);
            GetPointer<port_o>(access_allowed)->set_is_memory(false);
            GetPointer<port_o>(access_allowed)->set_is_global(false);
            GetPointer<port_o>(access_allowed)->set_is_extern(false);
         }
      }
   }
   if(parallelized_functions.find(HLS->functionId) == parallelized_functions.end())
   {
      const auto access_request = curr_gate->find_member("access_request", port_o_K, curr_gate);
      if(access_request)
      {
         GetPointer<port_o>(access_request)->set_is_memory(false);
         GetPointer<port_o>(access_request)->set_is_global(false);
         GetPointer<port_o>(access_request)->set_is_extern(false);
      }
      const auto access_allowed = curr_gate->find_member("access_allowed", port_o_K, curr_gate);
      if(access_allowed)
      {
         access_allowed_killeds.insert(curr_gate);
         GetPointer<port_o>(access_allowed)->set_is_memory(false);
         GetPointer<port_o>(access_allowed)->set_is_global(false);
         GetPointer<port_o>(access_allowed)->set_is_extern(false);
      }
   }
   return fu_binding::manage_module_ports(HLSMgr, HLS, SM, curr_gate, num);
}
