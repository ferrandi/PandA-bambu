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
 * @file mem_dominator_allocation_CS.hpp
 * @brief Class to allocate memories in HLS based on dominators, add tag for context switch
 *
 * @author Nicola Saporetti <nicola.saporetti@gmail.com>
*/
#include "omp_functions.hpp"
#include "memory_cs.hpp"
#include "mem_dominator_allocation_cs.hpp"
#include "cmath"
#include "Parameter.hpp"
#include "hls_manager.hpp"
#include "call_graph_manager.hpp"
#include "function_behavior.hpp"
#include "behavioral_helper.hpp"

mem_dominator_allocation_cs::mem_dominator_allocation_cs(const ParameterConstRef _parameters, const HLS_managerRef _HLSMgr, const DesignFlowManagerConstRef _design_flow_manager, const HLSFlowStepSpecializationConstRef _hls_flow_step_specialization, const HLSFlowStep_Type _hls_flow_step_type) :
   mem_dominator_allocation (_parameters, _HLSMgr, _design_flow_manager, _hls_flow_step_specialization, _hls_flow_step_type)
{
   debug_level = _parameters->get_class_debug_level(GET_CLASS(*this));
}

mem_dominator_allocation_cs::~mem_dominator_allocation_cs()
{
}

DesignFlowStep_Status mem_dominator_allocation_cs::Exec()
{
    mem_dominator_allocation::Exec();    //exec of hierarchical class
    const auto call_graph_manager = HLSMgr->CGetCallGraphManager();
    auto root_functions = call_graph_manager->GetRootFunctions();
    std::set<unsigned int> reached_fu_ids;
    for(const auto f_id : root_functions)
       for (const auto reached_f_id : call_graph_manager->GetReachedBodyFunctionsFrom(f_id))
          reached_fu_ids.insert(reached_f_id);
    auto omp_functions = GetPointer<OmpFunctions>(HLSMgr->Rfuns);
    unsigned int tag_index=0;
    bool noMemory=false;
    for (const auto & funID : reached_fu_ids)
    {
       std::cout<<HLSMgr->CGetFunctionBehavior(funID)->CGetBehavioralHelper()->get_function_name()<<" has to be syntetized"<<std::endl;
       GetPointer<memory_cs>(HLSMgr->Rmem)->set_tag_memory_number(funID,tag_index);
       if(omp_functions->kernel_functions.find(funID) != omp_functions->kernel_functions.end()) noMemory=true;
       if(omp_functions->parallelized_functions.find(funID) != omp_functions->parallelized_functions.end()) noMemory=true;
       if(omp_functions->atomic_functions.find(funID) != omp_functions->atomic_functions.end()) noMemory=true;
       if(noMemory) tag_index++;    //only parallel and other function have memory_ctrl
       noMemory=false;  //reset counter
    }
    tag_index=static_cast<unsigned int>(ceil(log2(tag_index)));    //reduce into log2 and take the upper bound
    tag_index+=2;   //bit for parallel and atomic
    unsigned int context_switch=static_cast<unsigned int>(log2(parameters->getOption<int>(OPT_context_switch)));
    unsigned int num_threads=static_cast<unsigned int>(log2(parameters->getOption<int>(OPT_num_threads)));
    tag_index=tag_index+context_switch+num_threads; //tag_index final
    GetPointer<memory_cs>(HLSMgr->Rmem)->set_bus_tag_bitsize(tag_index);

    unsigned int iterator=0;
    unsigned int base_address=static_cast<unsigned int>(pow((num_threads+context_switch),2));
    for (const auto & funID : reached_fu_ids)
    {
       unsigned int tag_number=0;   //number of tag to assign
       if(omp_functions->atomic_functions.find(funID) != omp_functions->atomic_functions.end())
           tag_number+=static_cast<unsigned int>(pow((tag_index-2),2));     //second bit 1
       tag_number+=(iterator*base_address);    //add number
       if(omp_functions->omp_for_wrappers.find(funID) != omp_functions->omp_for_wrappers.end())
           tag_number=static_cast<unsigned int>(pow((tag_index-1),2));  // first bit 1
       GetPointer<memory_cs>(HLSMgr->Rmem)->set_tag_memory_number(funID,tag_number);
       iterator++;
    }
    return DesignFlowStep_Status::SUCCESS;
}
