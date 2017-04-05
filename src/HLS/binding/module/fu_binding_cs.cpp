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
 * @file fu_binding_cs.cpp
 * @brief Derived class to add module scheduler, mem_ctrl_parallel and bind correctly the channels
 *
 * @author Nicola Saporetti <nicola.saporetti@gmail.com>
*/
#include "fu_binding_cs.hpp"
#include "omp_functions.hpp"
#include "structural_objects.hpp"

fu_binding_cs::fu_binding_cs(const HLS_managerConstRef _HLSMgr, const unsigned int _function_id, const ParameterConstRef _parameters) :
   fu_binding(_HLSMgr, _function_id, _parameters)
{
}

void fu_binding::add_to_SM(const HLS_managerRef HLSMgr, const hlsRef HLS, structural_objectRef clock_port, structural_objectRef reset_port)
{
   auto omp_functions = GetPointer<OmpFunctions>(HLSMgr->Rfuns);
   if(omp_functions->kernel_functions.find(_function_id) != omp_functions->kernel_functions.end())
   {
      const structural_objectRef circuit = SM->get_circ();
      std::string scheduler_model = "scheduler";
      std::string scheduler_name = "scheduler_kernel";
      std::string sche_library = HLS->HLS_T->get_technology_manager()->get_library(scheduler_model);
      structural_objectRef scheduler_mod = SM->add_module_from_technology_library(scheduler_name, scheduler_model, sche_library, circuit, HLS->HLS_T->get_technology_manager());
      fu_binding::add_to_SM(HLSMgr,HLS, clock_port,reset_port);

      structural_objectRef clock_scheduler = scheduler_mod->find_member("clock",port_o,scheduler_mod);
      SM->add_connection(clock_port,clock_scheduler);
      structural_objectRef reset_scheduler = scheduler_mod->find_member("reset",port_o,scheduler_mod);
      SM->add_connection(reset_port,reset_scheduler);
      structural_objectRef done_port_task_scheduler = scheduler_mod->find_member("done_port_task",port_o,scheduler_mod);
      structural_objectRef done_port_task_datapath = circuit->find_member("done_port_task",port_o,circuit);
      SM->add_connection(done_port_task_datapath,done_port_task_scheduler);
      structural_objectRef request_accepted_scheduler = scheduler_mod->find_member("request_accepted",port_o,scheduler_mod);
      structural_objectRef done_port_task_datapath = circuit->find_member("done_port_task",port_o,circuit);
      SM->add_connection(done_port_task_datapath,done_port_task_scheduler);
request_accepted
      task_finished
      unsigned int num_slots=static_cast<unsigned int>(log2(HLS->Param->getOption<unsigned int>(OPT_context_switch)));
      structural_type_descriptorRef port_type = structural_type_descriptorRef(new structural_type_descriptor("bool", num_slots));
      SM->add_port(SELECTOR_REGISTER_FILE, port_o::IN, circuit, port_type);
      structural_type_descriptorRef bool_type = structural_type_descriptorRef(new structural_type_descriptor("bool", 0));
      SM->add_port(SUSPENSION, port_o::OUT, circuit, bool_type);

      structural_objectRef sign_out = SM->add_sign("sig_out_"+bus_merger_inst_name, circuit, sig_type);
   }

}

void fu_binding_cs::manage_memory_ports_parallel_chained(const structural_managerRef SM, const std::set<structural_objectRef> &memory_modules, const structural_objectRef circuit, const hlsRef HLS, unsigned int & _unique_id)
{
   auto omp_functions = GetPointer<OmpFunctions>(HLSMgr->Rfuns);
   bool found=false;
   if(omp_functions->hierarchical_functions.find(_function_id) != omp_functions->hierarchical_functions.end())
   {
      manage_memory_ports_parallel_chained_hierarchical(SM, memory_modules, circuit, HLS, _unique_id);
   }
   else
   {
      fu_binding::manage_memory_ports_parallel_chained(SM, memory_modules, circuit, HLS, _unique_id);
   }
}

void fu_binding_cs::manage_memory_ports_parallel_chained_hierarchical(const structural_managerRef SM, const std::set<structural_objectRef> &memory_modules, const structural_objectRef circuit, const hlsRef HLS, unsigned int & _unique_id)
{
   std::map<structural_objectRef, std::set<structural_objectRef> > primary_outs;
   structural_objectRef cir_port;
   for (std::set<structural_objectRef>::iterator i = memory_modules.begin(); i != memory_modules.end(); i++)
   {
      for(unsigned int j = 0; j < GetPointer<module>(*i)->get_in_port_size(); j++)
      {
         structural_objectRef port_i = GetPointer<module>(*i)->get_in_port(j);
         if(GetPointer<port_o>(port_i)->get_is_memory() && (!GetPointer<port_o>(port_i)->get_is_global()) && (!GetPointer<port_o>(port_i)->get_is_extern()))
         {
            std::string port_name = GetPointer<port_o>(port_i)->get_id();
            cir_port = circuit->find_member(port_name, port_i->get_kind(), circuit);
            THROW_ASSERT(!cir_port || GetPointer<port_o>(cir_port), "should be a port or null");
            if(!cir_port)
            {
               if(port_i->get_kind() == port_vector_o_K)
                  cir_port = SM->add_port_vector(port_name, port_o::IN, GetPointer<port_o>(port_i)->get_ports_size(), circuit, port_i->get_typeRef());
               else
                  cir_port = SM->add_port(port_name, port_o::IN, circuit, port_i->get_typeRef());
               port_o::fix_port_properties(port_i, cir_port);
               SM->add_connection(cir_port,port_i);
            }
            else
            {
               SM->add_connection(cir_port,port_i);
            }
         }
      }
      for(unsigned int j = 0; j < GetPointer<module>(*i)->get_out_port_size(); j++)
      {
         structural_objectRef port_i = GetPointer<module>(*i)->get_out_port(j);
         if(GetPointer<port_o>(port_i)->get_is_memory() && (!GetPointer<port_o>(port_i)->get_is_global()) && (!GetPointer<port_o>(port_i)->get_is_extern()))
         {
            std::string port_name = GetPointer<port_o>(port_i)->get_id();
            cir_port = circuit->find_member(port_name, port_i->get_kind(), circuit);
            THROW_ASSERT(!cir_port || GetPointer<port_o>(cir_port), "should be a port or null");
            if(!cir_port)
            {
               if(port_i->get_kind() == port_vector_o_K)
                  cir_port = SM->add_port_vector(port_name, port_o::OUT, GetPointer<port_o>(port_i)->get_ports_size(), circuit, port_i->get_typeRef());
               else
                  cir_port = SM->add_port(port_name, port_o::OUT, circuit, port_i->get_typeRef());
               port_o::fix_port_properties(port_i, cir_port);
            }
            primary_outs[cir_port].insert(port_i);
         }
      }
   }
   call_version_of_jms(SM, HLS, primary_outs, circuit, _unique_id);
}

void fu_binding_cs::join_merge_split_cs_parallel(const structural_managerRef SM, const hlsRef HLS, std::map<structural_objectRef, std::set<structural_objectRef> > &primary_outs, const structural_objectRef circuit, unsigned int & _unique_id)
{
}

void fu_binding_cs::call_version_of_jms(const structural_managerRef SM, const hlsRef HLS, std::map<structural_objectRef, std::set<structural_objectRef> > &primary_outs, const structural_objectRef circuit, unsigned int &_unique_id)
{
   auto omp_functions = GetPointer<OmpFunctions>(HLSMgr->Rfuns);
   if(omp_functions->kernel_functions.find(_function_id) != omp_functions->kernel_functions.end())
      fu_binding::join_merge_split(SM, HLS, primary_outs, circuit, _unique_id);
   else if(omp_functions->omp_for_wrappers.find(_function_id) != omp_functions->omp_for_wrappers.end())
      join_merge_split_cs_parallel();
   else
      THROW_UNREACHABLE("Only kernel and parallel have to call this method");
}

void fu_binding_cs::connectSplitsToDatapath(std::map::const_iterator po, const structural_objectRef circuit, const structural_managerRef SM, std::__cxx11::string bus_merger_inst_name, structural_objectRef ss_out_port)
{
   //instantiate scheduler
   //connect input to scheduler
   //connect scheduler to datapath
}
