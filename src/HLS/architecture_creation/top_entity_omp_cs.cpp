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
 * @file top_entity_omp_cs.cpp
 *
 * @author Giovanni Gozzi <giovanni.gozzi@polimi.it>
 *
 */
#include "top_entity_omp_cs.hpp"

#include "Parameter.hpp"
#include "behavioral_helper.hpp"
#include "call_graph_manager.hpp"
#include "copyrights_strings.hpp"
#include "custom_map.hpp"
#include "fileIO.hpp"
#include "fu_binding.hpp"
#include "function_behavior.hpp"
#include "functions.hpp"
#include "hls.hpp"
#include "hls_device.hpp"
#include "hls_manager.hpp"
#include "ir_helper.hpp"
#include "math_function.hpp"
#include "memory.hpp"
#include "memory_allocation.hpp"
#include "memory_symbol.hpp"
#include "omp_fork_fu_binding.hpp"
#include "structural_manager.hpp"
#include "structural_objects.hpp"
#include "technology_manager.hpp"
#include "technology_node.hpp"

#include <string>

top_entity_omp_cs::top_entity_omp_cs(const ParameterConstRef _parameters, const HLS_managerRef _HLSMgr,
                                     unsigned int _funId, const DesignFlowManager& _design_flow_manager,
                                     const HLSFlowStep_Type _hls_flow_step_type)
    : top_entity(_parameters, _HLSMgr, _funId, _design_flow_manager, _hls_flow_step_type)
{
   debug_level = parameters->get_class_debug_level(GET_CLASS(*this));
}

static void propagateInterface(structural_managerRef SM, structural_objectRef internalObj, unsigned int Nthreads_size)
{
   auto externalObj = SM->get_circ();
   const auto internalModule = GetPointer<module_o>(internalObj);

   for(unsigned int i = 0U; i < internalModule->get_num_ports(); i++)
   {
      const auto port_i = internalModule->get_positional_port(i);
      const auto port_i_Obj = GetPointer<port_o>(port_i);
      const auto port_i_ID = port_i_Obj->get_id();
      structural_objectRef port_e;
      if(port_i_ID == "selector_register_file")
      {
         port_e = SM->add_port("M_tag", port_o::IN, externalObj,
                               structural_type_descriptorRef(new structural_type_descriptor("bool", 1U)));
         port_o::resize_std_port(Nthreads_size, 0U, 0, port_e);
         port_o::fix_port_properties(port_i, port_e);
      }
      else
      {
         port_e = SM->add_port(port_i_ID, port_i_Obj->get_port_direction(), externalObj, port_i_Obj->get_typeRef());
         port_o::resize_std_port(
             port_i_Obj->get_typeRef()->vector_size == 0 ? 1 : port_i_Obj->get_typeRef()->vector_size,
             (port_i->get_kind() == port_vector_o_K ? port_i_Obj->get_ports_size() : 0U), 0, port_e);
         port_o::fix_port_properties(port_i, port_e);
      }
      if(port_i_ID != "selector_register_file" && port_i_ID != "start_port" && port_i_ID != "done_port" &&
         port_i_ID != "Mout_oe_ram" && port_i_ID != "Mout_we_ram" && port_i_ID != "M_DataRdy" &&
         port_i_ID != "M_Rdata_ram" && port_i_ID != "kmp_bambu_wait_all_threads_ld" &&
         port_i_ID != "kmp_bambu_wait_all_threads_rd" && !(boost::starts_with(port_i_ID, "kmp_lock_acquired")) &&
         !(boost::starts_with(port_i_ID, "kmp_lock_acquired_ack")))
      {
         fu_binding::add_smart_connection(port_i, port_e, 0, externalObj, SM);
      }
   }

   auto port_srf = internalObj->find_member("selector_register_file", port_o_K, internalObj);
   auto port_srf_out = SM->add_port("Mout_tag", port_o::OUT, externalObj,
                                    structural_type_descriptorRef(new structural_type_descriptor("bool", 1U)));
   port_o::resize_std_port(Nthreads_size, 0U, 0, port_srf_out);
   port_o::fix_port_properties(port_srf, port_srf_out);

   auto port_critical = internalObj->find_member("kmp_lock_acquired", port_o_K, internalObj);

   if(port_critical)
   {
      auto port_crtical_ac_out =
          SM->add_port("kmp_lock_acquired_tag", port_o::OUT, externalObj,
                       structural_type_descriptorRef(new structural_type_descriptor("bool", 1U)));
      port_o::resize_std_port(Nthreads_size, 0U, 0, port_crtical_ac_out);
      auto port_crtical_ac_in = SM->add_port("kmp_lock_acquired_ack_tag", port_o::IN, externalObj,
                                             structural_type_descriptorRef(new structural_type_descriptor("bool", 1U)));
      port_o::resize_std_port(Nthreads_size, 0U, 0, port_crtical_ac_in);
   }

   for(auto p : internalObj->GetParameters())
   {
      const auto name = p.first;
      const auto value = p.second;
      externalObj->AddParameter(name, "0");
      externalObj->SetParameter(name, value);
      if(!boost::starts_with(name, "MEM"))
      {
         internalObj->SetParameter(name, name);
      }
   }
}

DesignFlowStep_Status top_entity_omp_cs::InternalExec()
{
   top_entity::InternalExec();
   THROW_ASSERT(HLS->top, "component must exist");
   const auto outlined_obj = HLS->top->get_circ();
   THROW_ASSERT(outlined_obj, "component must exist");
   const auto outlined_name = outlined_obj->get_id();
   outlined_obj->set_id(outlined_name + "_int");
   const structural_managerRef omp_cs_interface(new structural_manager(parameters));

   const structural_type_descriptorRef internal_type(new structural_type_descriptor(outlined_name + "_int"));
   omp_cs_interface->set_top_info(outlined_name, outlined_obj->get_typeRef());
   outlined_obj->set_type(internal_type);
   HLS->top->set_top_info(outlined_name + "_int", internal_type);
   auto wrapper_obj = omp_cs_interface->get_circ();

   // add the core to the wrapper
   outlined_obj->set_owner(wrapper_obj);
   outlined_obj->set_id(outlined_obj->get_id() + "_i0");
   GetPointerS<module_o>(wrapper_obj)->add_internal_object(outlined_obj);

   // Set some descriptions and legal stuff
   GetPointerS<module_o>(wrapper_obj)
       ->set_description("Memory mapped interface for top component: " + outlined_obj->get_typeRef()->id_type);
   GetPointerS<module_o>(wrapper_obj)->set_copyright(GENERATED_COPYRIGHT);
   GetPointerS<module_o>(wrapper_obj)->set_authors("Component automatically generated by bambu");
   GetPointerS<module_o>(wrapper_obj)->set_license(GENERATED_LICENSE);

   unsigned int _unique_id = 0U;
   const auto omp_info = HLSMgr->CGetFunctionBehavior(funId)->GetOMPInfo();
   THROW_ASSERT(omp_info, "");
   const auto Nthreads = omp_info->context_count;
   auto Nthreads_size = std::max(ceil_log2(Nthreads), 1U);

   propagateInterface(omp_cs_interface, outlined_obj, Nthreads_size);

   auto datapath_module = GetPointer<module_o>(outlined_obj->find_member("Datapath_i", module_o_K, outlined_obj));
   auto selector_register_size = std::max(1U, ceil_log2(parameters->getOption<unsigned int>(OPT_context_switch)));
   for(unsigned int i = 0; i < datapath_module->get_internal_objects_size(); i++)
   {
      auto component = datapath_module->get_internal_object(i);
      auto component_module = GetPointer<module_o>(component);
      if(component_module)
      {
         if(component_module->has_port("selector_register_file"))
         {
            auto port = component->find_member("selector_register_file", port_o_K, component);
            port_o::resize_std_port(selector_register_size, 0U, 0, port);
         }
      }
   }

   HLS->top = omp_cs_interface;
   const auto TechM = HLS->HLS_D->get_technology_manager();

   const auto done_cs_name = "kmp_bambu_omp_done_cs";
   const auto done_cs_library = TechM->get_library(done_cs_name);
   const auto done_cs = omp_cs_interface->add_module_from_technology_library("omp_done_cs", done_cs_name,
                                                                             done_cs_library, wrapper_obj, TechM);

   const std::vector<std::string> omp_done_cs_wrapper_ports = {"clock", "reset"};

   for(const auto& name : omp_done_cs_wrapper_ports)
   {
      const auto wrapper_port = wrapper_obj->find_member(name, port_o_K, wrapper_obj);
      THROW_ASSERT(wrapper_port, "port must exist");
      const auto done_port = done_cs->find_member(name, port_o_K, done_cs);
      fu_binding::add_smart_connection(wrapper_port, done_port, _unique_id, wrapper_obj, omp_cs_interface);
      _unique_id++;
   }
   done_cs->SetParameter("THREAD_NUMBER", STR(Nthreads));
   const auto done_srf_port = done_cs->find_member("selector_register_file", port_o_K, done_cs);
   port_o::resize_std_port(Nthreads_size, 0U, 0, done_srf_port);
   const auto srf_signal = fu_binding::add_smart_signal(done_srf_port, _unique_id, wrapper_obj, omp_cs_interface);
   omp_cs_interface->add_connection(srf_signal, done_srf_port);

   const auto done_done_port = done_cs->find_member("done_port", port_o_K, done_cs);
   const auto wrapper_done_port = wrapper_obj->find_member("done_port", port_o_K, wrapper_obj);
   const auto done_signal = fu_binding::add_smart_signal(done_done_port, _unique_id, wrapper_obj, omp_cs_interface);
   omp_cs_interface->add_connection(done_signal, done_done_port);
   omp_cs_interface->add_connection(done_signal, wrapper_done_port);
   _unique_id++;

   const auto done_port_in = done_cs->find_member("done_port_in", port_o_K, done_cs);
   const auto done_port_outlined = outlined_obj->find_member("done_port", port_o_K, outlined_obj);
   THROW_ASSERT(done_port_outlined, "port must exist");
   const auto done_signal_outlined =
       fu_binding::add_smart_signal(done_port_outlined, _unique_id, wrapper_obj, omp_cs_interface);
   omp_cs_interface->add_connection(done_signal_outlined, done_port_in);
   omp_cs_interface->add_connection(done_signal_outlined, done_port_outlined);
   _unique_id++;

   const auto start_cs_name = "kmp_bambu_omp_start_cs";
   const auto start_cs_library = TechM->get_library(start_cs_name);
   const auto start_cs = omp_cs_interface->add_module_from_technology_library("omp_start_cs", start_cs_name,
                                                                              start_cs_library, wrapper_obj, TechM);

   const std::vector<std::string> omp_start_cs_wrapper_ports = {"clock", "reset", "start_port"};

   for(const auto& name : omp_start_cs_wrapper_ports)
   {
      const auto wrapper_port = wrapper_obj->find_member(name, port_o_K, wrapper_obj);
      THROW_ASSERT(wrapper_port, "port must exist");
      const auto start_port = start_cs->find_member(name, port_o_K, start_cs);
      fu_binding::add_smart_connection(wrapper_port, start_port, _unique_id, wrapper_obj, omp_cs_interface);
      _unique_id++;
   }
   start_cs->SetParameter("THREAD_NUMBER", STR(Nthreads));
   const auto start_srf_port = start_cs->find_member("selector_register_file", port_o_K, start_cs);
   port_o::resize_std_port(Nthreads_size, 0U, 0, start_srf_port);
   omp_cs_interface->add_connection(srf_signal, start_srf_port);

   const auto start_port_out = start_cs->find_member("start_port_out", port_o_K, start_cs);
   const auto start_port_outlined = outlined_obj->find_member("start_port", port_o_K, outlined_obj);
   THROW_ASSERT(start_port_outlined, "port must exist");
   fu_binding::add_smart_connection(start_port_out, start_port_outlined, _unique_id, wrapper_obj, omp_cs_interface);
   _unique_id++;

   const auto start_done_port = start_cs->find_member("component_done", port_o_K, start_cs);
   omp_cs_interface->add_connection(done_signal, start_done_port);

   const auto cs_name = "kmp_bambu_cs_manager";
   const auto cs_library = TechM->get_library(cs_name);
   const auto cs =
       omp_cs_interface->add_module_from_technology_library("cs_manager", cs_name, cs_library, wrapper_obj, TechM);
   cs->SetParameter("THREAD_NUMBER", STR(Nthreads));
   cs->SetParameter("THREAD_NUMBER_SIZE", STR(Nthreads_size));

   const std::vector<std::pair<std::string, std::string>> cs_wrapper_ports = {
       {"clock", "clock"},
       {"reset", "reset"},
       {"start_port", "start_port"},
       {"M_DataRdy", "M_DataRdy_in"},
       {"M_Rdata_ram", "M_Rdata_ram_in"},
       {"kmp_lock_acquired_ack", "kmp_lock_acquired_ack_in"},
       {"kmp_lock_acquired_ack_tag", "kmp_lock_acquired_ack_tag"},
       {"kmp_bambu_wait_all_threads_rd", "kmp_bambu_wait_all_threads_rd_in"},
       {"M_tag", "selector_register_file_in"}};

   for(const auto& p : cs_wrapper_ports)
   {
      const auto wrapper_port_name = p.first;
      const auto cs_port_name = p.second;
      structural_objectRef wrapper_port;
      const bool result = fu_binding::try_find_port(wrapper_port_name, wrapper_obj, port_o_K, wrapper_port);
      if(result)
      {
         const auto cs_port = cs->find_member(cs_port_name, port_o_K, cs);
         const auto wrapper_port_Obj = GetPointer<port_o>(wrapper_port);
         port_o::resize_std_port(
             wrapper_port_Obj->get_typeRef()->vector_size == 0 ? 1U : wrapper_port_Obj->get_typeRef()->vector_size, 0U,
             0, cs_port);
         fu_binding::add_smart_connection(wrapper_port, cs_port, _unique_id, wrapper_obj, omp_cs_interface);
         _unique_id++;
      }
      else
      {
         const auto cs_port = cs->find_member(cs_port_name, port_o_K, cs);
         structural_objectRef const_obj =
             SM->add_constant("const_node_" + cs_port_name, wrapper_obj, cs_port->get_typeRef(), STR(0));
         SM->add_connection(cs_port, const_obj);
      }
   }

   const std::vector<std::string> cs_direct_outlined_ports = {"M_DataRdy", "M_Rdata_ram", "kmp_lock_acquired_ack",
                                                              "kmp_bambu_wait_all_threads_rd"};

   for(const auto& p : cs_direct_outlined_ports)
   {
      const auto outlined_port = outlined_obj->find_member(p, port_o_K, outlined_obj);
      if(outlined_port)
      {
         const auto cs_port = cs->find_member(p, port_o_K, cs);
         const auto outlined_port_Obj = GetPointer<port_o>(outlined_port);
         port_o::resize_std_port(
             outlined_port_Obj->get_typeRef()->vector_size == 0U ? 1U : outlined_port_Obj->get_typeRef()->vector_size,
             0U, 0, cs_port);
         fu_binding::add_smart_connection(outlined_port, cs_port, _unique_id, wrapper_obj, omp_cs_interface);
         _unique_id++;
      }
   }

   const std::vector<std::string> cs_outlined_ports = {"Mout_oe_ram", "Mout_we_ram", "kmp_lock_acquired",
                                                       "kmp_bambu_wait_all_threads_ld"};

   for(const auto& p : cs_outlined_ports)
   {
      structural_objectRef outlined_port;
      const bool result1 = fu_binding::try_find_port(p, outlined_obj, port_o_K, outlined_port);
      structural_objectRef wrapper_port;
      const bool result2 = fu_binding::try_find_port(p, wrapper_obj, port_o_K, wrapper_port);
      if(result1 && result2)
      {
         const auto cs_port = cs->find_member(p, port_o_K, cs);
         const auto outlined_port_Obj = GetPointer<port_o>(outlined_port);
         port_o::resize_std_port(
             outlined_port_Obj->get_typeRef()->vector_size == 0U ? 1U : outlined_port_Obj->get_typeRef()->vector_size,
             0U, 0, cs_port);
         const auto sig_loc = fu_binding::add_smart_signal(outlined_port, _unique_id, wrapper_obj, omp_cs_interface);
         _unique_id++;
         omp_cs_interface->add_connection(sig_loc, outlined_port);
         omp_cs_interface->add_connection(sig_loc, wrapper_port);
         omp_cs_interface->add_connection(sig_loc, cs_port);
      }
      else
      {
         const auto cs_port = cs->find_member(p, port_o_K, cs);
         structural_objectRef const_obj =
             SM->add_constant("const_node_" + p, wrapper_obj, cs_port->get_typeRef(), STR(0));
         SM->add_connection(cs_port, const_obj);
      }
   }

   const auto cs_srf_port = cs->find_member("selector_register_file", port_o_K, cs);
   port_o::resize_std_port(Nthreads_size, 0U, 0, cs_srf_port);
   omp_cs_interface->add_connection(srf_signal, cs_srf_port);

   const auto outlined_srf_port = outlined_obj->find_member("selector_register_file", port_o_K, outlined_obj);
   THROW_ASSERT(outlined_srf_port, "port must exist");
   omp_cs_interface->add_connection(srf_signal, outlined_srf_port);

   const auto wrapper_srf_port = wrapper_obj->find_member("Mout_tag", port_o_K, wrapper_obj);
   THROW_ASSERT(wrapper_srf_port, "port must exist");
   omp_cs_interface->add_connection(srf_signal, wrapper_srf_port);

   auto port_critical = outlined_obj->find_member("kmp_lock_acquired", port_o_K, outlined_obj);

   if(port_critical)
   {
      const auto port_critical_ac_out = wrapper_obj->find_member("kmp_lock_acquired_tag", port_o_K, wrapper_obj);
      THROW_ASSERT(port_critical_ac_out, "port must exist");
      omp_cs_interface->add_connection(srf_signal, port_critical_ac_out);
   }

   const auto cs_done_port = cs->find_member("component_done", port_o_K, cs);
   omp_cs_interface->add_connection(done_signal, cs_done_port);

   const auto cs_done_port_in = cs->find_member("done_port", port_o_K, cs);
   omp_cs_interface->add_connection(done_signal_outlined, cs_done_port_in);

   return DesignFlowStep_Status::SUCCESS;
}
