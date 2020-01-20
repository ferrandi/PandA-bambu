/*
 *
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
 * @file conn_binding_cs.cpp
 * @brief Connect the new component instantiated by the flow context_switch
 *
 * @author Nicola Saporetti <nicola.saporetti@gmail.com>
 */

#include "conn_binding_cs.hpp"
#include "Parameter.hpp"
#include "hls.hpp"
#include "hls_manager.hpp"
#include "hls_target.hpp"
#include "omp_functions.hpp"
#include "structural_manager.hpp"
#include "structural_objects.hpp"
#include "technology_manager.hpp"
#include "technology_node.hpp"

/// STD include
#include <string>

/// utility include
#include "dbgPrintHelper.hpp"
#include "utility.hpp"

conn_binding_cs::conn_binding_cs(const BehavioralHelperConstRef _BH, const ParameterConstRef _parameters) : conn_binding(_BH, _parameters)
{
   debug_level = _parameters->get_class_debug_level(GET_CLASS(*this));
}

conn_binding_cs::~conn_binding_cs()
{
}

void conn_binding_cs::add_to_SM(const HLS_managerRef HLSMgr, const hlsRef HLS, const structural_managerRef SM)
{
   conn_binding::add_to_SM(HLSMgr, HLS, SM);
   instantiate_suspension_component(HLSMgr, HLS);
}

void conn_binding_cs::instantiate_suspension_component(const HLS_managerRef HLSMgr, const hlsRef HLS)
{
   structural_type_descriptorRef bool_type = structural_type_descriptorRef(new structural_type_descriptor("bool", 0));
   const structural_managerRef SM = HLS->datapath;
   const structural_objectRef circuit = SM->get_circ();

   bool addedLoad = false;
   bool addedStore = false;
   bool andStartMemOp_required = false;

   for(unsigned int j = 0; j < GetPointer<module>(circuit)->get_in_port_size(); j++)
   {
      structural_objectRef port_i = GetPointer<module>(circuit)->get_in_port(j);
      std::string port_name = GetPointer<port_o>(port_i)->get_id();
      std::size_t found = port_name.find("LOAD");
      if(found != std::string::npos)
      {
         addedLoad = true;
      }
      found = port_name.find("STORE");
      if(found != std::string::npos)
      {
         addedStore = true;
      }
   }
   for(unsigned int j = 0; j < GetPointer<module>(circuit)->get_internal_objects_size(); j++)
   {
      structural_objectRef curr_gate = GetPointer<module>(circuit)->get_internal_object(j);
      if(GET_TYPE_NAME(curr_gate) == "mem_ctrl_kernel")
      {
         structural_objectRef portStart = curr_gate->find_member(STR(START_PORT_NAME), port_o_K, curr_gate);
         structural_objectRef startMemOp = GetPointer<port_o>(portStart)->find_bounded_object();
         THROW_ASSERT(startMemOp != NULL, "No start port for mem_ctrl_found");
         andStartMemOp_required = true;
         break;
      }
   }
   // search in module and find one with suspension
   unsigned int num_suspension = 0;
   unsigned int n_elements = GetPointer<module>(circuit)->get_internal_objects_size();
   unsigned int i = 0;
   for(i = 0; i < n_elements; i++)
   {
      structural_objectRef curr_gate = GetPointer<module>(circuit)->get_internal_object(i);
      if(curr_gate->find_member(STR(SUSPENSION), port_o_K, curr_gate) != nullptr and curr_gate->get_id() != "scheduler_kernel")
         ++num_suspension;
   }

   if(num_suspension == 0 && !addedLoad && !addedStore && !andStartMemOp_required)
   {
      structural_objectRef suspension_datapath = circuit->find_member(STR(SUSPENSION), port_o_K, circuit);
      structural_objectRef constantFalse(new constant_o(debug_level, SM->get_circ(), "0"));
      constantFalse->set_type(bool_type);
      GetPointer<module>(circuit)->add_internal_object(constantFalse);
      SM->add_connection(constantFalse, suspension_datapath);
      return;
   }
   structural_objectRef suspensionOr = SM->add_module_from_technology_library("suspensionOr", OR_GATE_STD, HLS->HLS_T->get_technology_manager()->get_library(OR_GATE_STD), circuit, HLS->HLS_T->get_technology_manager());
   structural_objectRef port_in_or = suspensionOr->find_member("in", port_vector_o_K, suspensionOr);
   structural_objectRef port_out_or = suspensionOr->find_member("out1", port_o_K, suspensionOr);
   structural_objectRef out_or_sign = SM->add_sign("out_or_signal", circuit, bool_type);
   SM->add_connection(port_out_or, out_or_sign);

   if(GetPointer<port_o>(port_in_or)->get_ports_size() != 0)
      THROW_ERROR("Or start with more than 0 input port");
   else
      GetPointer<port_o>(port_in_or)->add_n_ports(2, port_in_or);
   for(unsigned int j = 0; j < GetPointer<module>(circuit)->get_in_port_size(); j++)
   {
      structural_objectRef port_i = GetPointer<module>(circuit)->get_in_port(j);
      std::string port_name = GetPointer<port_o>(port_i)->get_id();
      std::size_t found = port_name.find("LOAD");
      if(found != std::string::npos)
      {
         SM->add_connection(port_i, GetPointer<port_o>(port_in_or)->get_port(0));
      }
      found = port_name.find("STORE");
      if(found != std::string::npos)
      {
         SM->add_connection(port_i, GetPointer<port_o>(port_in_or)->get_port(1));
      }
   }

   PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, " - Added or_suspension local");

   structural_objectRef out_and_sign = SM->add_sign("out_and_signal", circuit, bool_type);
   structural_objectRef andStartMemOp = SM->add_module_from_technology_library("andStartMemOp", AND_GATE_STD, HLS->HLS_T->get_technology_manager()->get_library(OR_GATE_STD), circuit, HLS->HLS_T->get_technology_manager());
   structural_objectRef port_in_and = andStartMemOp->find_member("in", port_vector_o_K, andStartMemOp);
   structural_objectRef port_out_and = andStartMemOp->find_member("out1", port_o_K, andStartMemOp);
   SM->add_connection(port_out_and, out_and_sign);

   if(GetPointer<port_o>(port_in_and)->get_ports_size() != 0)
      THROW_ERROR("And start with more than 0 input port");
   else
      GetPointer<port_o>(port_in_and)->add_n_ports(2, port_in_and);

   SM->add_connection(out_or_sign, GetPointer<port_o>(port_in_and)->get_port(0)); // connected out or

   for(unsigned int j = 0; j < GetPointer<module>(circuit)->get_internal_objects_size(); j++)
   {
      structural_objectRef curr_gate = GetPointer<module>(circuit)->get_internal_object(j);
      if(GET_TYPE_NAME(curr_gate) == "mem_ctrl_kernel")
      {
         structural_objectRef portStart = curr_gate->find_member(STR(START_PORT_NAME), port_o_K, curr_gate);
         structural_objectRef startMemOp = GetPointer<port_o>(portStart)->find_bounded_object();
         THROW_ASSERT(startMemOp != NULL, "No start port for mem_ctrl_found");
         SM->add_connection(startMemOp, GetPointer<port_o>(port_in_and)->get_port(1));
         break;
      }
   }
   PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, " - Added and_suspension");

   structural_objectRef suspensionOrGlo = SM->add_module_from_technology_library("suspensionOrGlobal", OR_GATE_STD, HLS->HLS_T->get_technology_manager()->get_library(OR_GATE_STD), circuit, HLS->HLS_T->get_technology_manager());
   structural_objectRef port_in_or_glo = suspensionOrGlo->find_member("in", port_vector_o_K, suspensionOrGlo);
   structural_objectRef port_out_or_glo = suspensionOrGlo->find_member("out1", port_o_K, suspensionOrGlo);

   // search in module and find one with suspension
   if(GetPointer<port_o>(port_in_or_glo)->get_ports_size() != 0)
      THROW_ERROR("Or start with more than 0 input port");
   else
      GetPointer<port_o>(port_in_or_glo)->add_n_ports(1 + num_suspension, port_in_or_glo);
   SM->add_connection(out_and_sign, GetPointer<port_o>(port_in_or_glo)->get_port(0));

   if(num_suspension > 0)
   {
      unsigned int num_signal_or = 0;
      for(i = 0; i < n_elements; i++)
      {
         structural_objectRef curr_gate = GetPointer<module>(circuit)->get_internal_object(i);
         structural_objectRef port_suspension_module = curr_gate->find_member(STR(SUSPENSION), port_o_K, curr_gate);
         if(port_suspension_module != nullptr and curr_gate->get_id() != "scheduler_kernel")
         {
            structural_objectRef suspension_sign = SM->add_sign(STR(SUSPENSION) + "_signal_" + STR(i), circuit, bool_type);
            SM->add_connection(port_suspension_module, suspension_sign);
            SM->add_connection(suspension_sign, GetPointer<port_o>(port_in_or_glo)->get_port(num_signal_or + 1));
            ++num_signal_or;
         }
      }
   }

   connectOutOr(HLSMgr, HLS, port_out_or_glo);
}

void conn_binding_cs::connectOutOr(const HLS_managerRef HLSMgr, const hlsRef HLS, structural_objectRef port_out_or)
{
   const structural_managerRef SM = HLS->datapath;
   const structural_objectRef circuit = SM->get_circ();
   auto omp_functions = GetPointer<OmpFunctions>(HLSMgr->Rfuns);
   if(omp_functions->kernel_functions.find(HLS->functionId) != omp_functions->kernel_functions.end())
   {
      PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, " - Connecting out or of kernel");
      structural_objectRef scheduler = circuit->find_member("scheduler_kernel", component_o_K, circuit);
      structural_type_descriptorRef bool_type = structural_type_descriptorRef(new structural_type_descriptor("bool", 0));
      structural_objectRef suspension_scheduler = scheduler->find_member(STR(SUSPENSION), port_o_K, scheduler);
      structural_objectRef suspension_sign_out = SM->add_sign(STR(SUSPENSION) + "_signal", circuit, bool_type);
      SM->add_connection(port_out_or, suspension_sign_out);
      SM->add_connection(suspension_sign_out, suspension_scheduler);
   }
   else
   {
      PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, " - Connecting out or");
      structural_objectRef suspension_datapath = circuit->find_member(STR(SUSPENSION), port_o_K, circuit);
      SM->add_connection(port_out_or, suspension_datapath);
   }
   PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, " - Suspension signal correctly connected!");
}
