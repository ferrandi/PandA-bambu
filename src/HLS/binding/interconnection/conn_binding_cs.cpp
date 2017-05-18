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
 *              Copyright (c) 2004-2016 Politecnico di Milano
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
#include "hls_target.hpp"
#include "omp_functions.hpp"
#include "hls_manager.hpp"
#include "hls.hpp"
#include "structural_objects.hpp"
#include "structural_manager.hpp"
#include "Parameter.hpp"

conn_binding_cs::conn_binding_cs(const BehavioralHelperConstRef _BH, const ParameterConstRef _parameters) :
   conn_binding(_BH, _parameters)
{
}

conn_binding_cs::~conn_binding_cs()
{

}

void conn_binding_cs::add_to_SM(const HLS_managerRef HLSMgr, const hlsRef HLS, const structural_managerRef SM)
{
   conn_binding::add_to_SM(HLSMgr,HLS,SM);
   auto omp_functions = GetPointer<OmpFunctions>(HLSMgr->Rfuns);
   if(omp_functions->kernel_functions.find(HLS->functionId) != omp_functions->kernel_functions.end())
   {
      connect_scheduler_kernel(HLS);
      connect_suspension_component(HLSMgr,HLS);
   }
   else if((omp_functions->parallelized_functions.find(HLS->functionId) != omp_functions->parallelized_functions.end()) || (omp_functions->atomic_functions.find(HLS->functionId) != omp_functions->atomic_functions.end()))
   {
      connect_selector(HLS);
      connect_suspension_component(HLSMgr,HLS);
   }
   else
      THROW_ERROR("No other function should call conn_binding_cs");
}

void conn_binding_cs::connect_scheduler_kernel(const hlsRef HLS)
{
   const structural_managerRef SM = HLS->datapath;
   const structural_objectRef circuit = SM->get_circ();
   structural_objectRef scheduler_mod = circuit->find_member("scheduler_kernel", component_o_K, circuit);
   structural_type_descriptorRef bool_type = structural_type_descriptorRef(new structural_type_descriptor("bool", 0));
   unsigned int num_slots=static_cast<unsigned int>(ceil(log2(HLS->Param->getOption<unsigned int>(OPT_context_switch))));   //resize selector-port
   structural_objectRef port_selector = scheduler_mod->find_member(STR(SELECTOR_REGISTER_FILE),port_o_K,scheduler_mod);
   port_selector->type_resize(num_slots);
   structural_objectRef selector_regFile_scheduler = scheduler_mod->find_member(STR(SELECTOR_REGISTER_FILE),port_o_K,scheduler_mod);
   structural_objectRef selector_regFile_datapath = circuit->find_member(STR(SELECTOR_REGISTER_FILE),port_o_K,circuit);
   structural_type_descriptorRef port_type = structural_type_descriptorRef(new structural_type_descriptor("bool", num_slots));
   structural_objectRef selector_regFile_sign=SM->add_sign(STR(SELECTOR_REGISTER_FILE)+"_signal", circuit, port_type);
   SM->add_connection(selector_regFile_scheduler, selector_regFile_sign);
   SM->add_connection(selector_regFile_sign, selector_regFile_datapath);
   PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, " - Added selector");
   for(unsigned int i=0;i<GetPointer<module>(circuit)->get_internal_objects_size();i++)
   {
      structural_objectRef curr_gate = GetPointer<module>(circuit)->get_internal_object(i);
      const structural_objectRef port_selector_module=curr_gate->find_member(STR(SELECTOR_REGISTER_FILE), port_o_K, curr_gate);
      if(port_selector_module!=NULL)
         SM->add_connection(selector_regFile_sign, port_selector_module);
   }
   PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, " - Connected register");
   //suspension connected when or_port instantiated

   structural_objectRef done_scheduler = scheduler_mod->find_member(DONE_SCHEDULER,port_o_K,scheduler_mod);
   structural_objectRef done_datapath = circuit->find_member(DONE_SCHEDULER,port_o_K,circuit);
   structural_objectRef done_sign=SM->add_sign("done_scheduler_signal", circuit, bool_type);
   SM->add_connection(done_sign, done_datapath);
   SM->add_connection(done_sign, done_scheduler);
   PRINT_DBG_MEX(DEBUG_LEVEL_VERBOSE, debug_level, " - Added done sche");

   structural_objectRef task_pool_end_scheduler = scheduler_mod->find_member(STR(TASKS_POOL_END),port_o_K,scheduler_mod);
   structural_objectRef task_pool_end_datapath = circuit->find_member(STR(TASKS_POOL_END),port_o_K,circuit);
   structural_objectRef task_pool_end_sign=SM->add_sign(STR(TASKS_POOL_END)+"_signal", circuit, bool_type);
   SM->add_connection(task_pool_end_sign, task_pool_end_datapath);
   SM->add_connection(task_pool_end_sign, task_pool_end_scheduler);
   PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, " - Added task_pool_end sche");

   structural_objectRef done_request_scheduler = scheduler_mod->find_member(STR(DONE_REQUEST),port_o_K,scheduler_mod);
   structural_objectRef done_request_datapath = circuit->find_member(STR(DONE_REQUEST),port_o_K,circuit);
   structural_objectRef done_request_sign=SM->add_sign(STR(DONE_REQUEST)+"_signal", circuit, bool_type);
   SM->add_connection(done_request_sign, done_request_datapath);
   SM->add_connection(done_request_sign, done_request_scheduler);
   PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, " - Added done_request sche");
}

void conn_binding_cs::connect_selector(const hlsRef HLS)
{
   const structural_managerRef SM = HLS->datapath;
   const structural_objectRef circuit = SM->get_circ();
   structural_objectRef port_selector_datapath = circuit->find_member(SELECTOR_REGISTER_FILE, port_o_K, circuit);
   for(unsigned int i=0;i<GetPointer<module>(circuit)->get_internal_objects_size();i++)
   {
      structural_objectRef curr_gate = GetPointer<module>(circuit)->get_internal_object(i);
      const structural_objectRef port_selector_module=curr_gate->find_member(STR(SELECTOR_REGISTER_FILE), port_o_K, curr_gate);
      if(port_selector_module!=NULL)
         SM->add_connection(port_selector_datapath, port_selector_module);
   }
}

void conn_binding_cs::connect_suspension_component(const HLS_managerRef HLSMgr, const hlsRef HLS)
{
   structural_type_descriptorRef bool_type = structural_type_descriptorRef(new structural_type_descriptor("bool", 0));
   const structural_managerRef SM = HLS->datapath;
   const structural_objectRef circuit = SM->get_circ();
   structural_objectRef suspensionOr = circuit->find_member("suspensionOr", component_o_K, circuit);
   structural_objectRef port_in_or = suspensionOr->find_member("in", port_vector_o_K, suspensionOr);
   structural_objectRef port_out_or = suspensionOr->find_member("out1", port_o_K, suspensionOr);
   unsigned int n_elements = GetPointer<module>(circuit)->get_internal_objects_size();
   unsigned int num_suspension=0;
   unsigned int i=0;
   for(i=0;i<n_elements;i++)
   {
      structural_objectRef curr_gate = GetPointer<module>(circuit)->get_internal_object(i);
      if(curr_gate->find_member(STR(SUSPENSION), port_o_K, curr_gate)!=NULL && curr_gate->get_id()!="scheduler_kernel")
         ++num_suspension;
   }
   if(num_suspension>0)
   {
      std::cout<<"Num port is: 2+"<<num_suspension<<std::endl;
      GetPointer<port_o>(port_in_or)->add_n_ports(num_suspension, port_in_or);
      unsigned int num_signal_or=0;
      for(i=0;i<n_elements;i++)
      {
         structural_objectRef curr_gate = GetPointer<module>(circuit)->get_internal_object(i);
         if(curr_gate->get_id()!="scheduler_kernel")
         {
            structural_objectRef port_suspension_module = curr_gate->find_member(STR(SUSPENSION), port_o_K, curr_gate);
            if(port_suspension_module!=NULL)
            {
               structural_objectRef suspension_sign=SM->add_sign(STR(SUSPENSION)+"_signal_"+STR(i), circuit, bool_type);
               SM->add_connection(port_suspension_module, suspension_sign);
               SM->add_connection(suspension_sign, GetPointer<port_o>(port_in_or)->get_port(num_signal_or+2));
               ++num_signal_or;
            }
         }
      }
   }
   for(unsigned int j = 0; j < GetPointer<module>(circuit)->get_in_port_size(); j++)
   {
      structural_objectRef port_i = GetPointer<module>(circuit)->get_in_port(j);
      std::string port_name = GetPointer<port_o>(port_i)->get_id();
      std::cout<<"Port name: "<<port_name<<std::endl;
      std::size_t found = port_name.find("LOAD");
      if(found!=std::string::npos)
      {
         SM->add_connection(port_i, GetPointer<port_o>(port_in_or)->get_port(0));
      }
      found = port_name.find("STORE");
      if(found!=std::string::npos)
      {
         SM->add_connection(port_i, GetPointer<port_o>(port_in_or)->get_port(1));
      }
   }
   PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, " - Connected load and store");
   connectOutOr(HLSMgr, HLS, port_out_or);
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
      structural_objectRef suspension_scheduler = scheduler->find_member(STR(SUSPENSION),port_o_K,scheduler);
      structural_objectRef suspension_sign_out=SM->add_sign(STR(SUSPENSION)+"_signal", circuit, bool_type);
      SM->add_connection(port_out_or, suspension_sign_out);
      SM->add_connection(suspension_sign_out, suspension_scheduler);
   }
   else
   {
      PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, " - Connecting out or");
      structural_objectRef suspension_datapath = circuit->find_member(STR(SUSPENSION),port_o_K,circuit);
      SM->add_connection(port_out_or, suspension_datapath);
   }
   PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, " - Suspension signal correctly connected!");
}
