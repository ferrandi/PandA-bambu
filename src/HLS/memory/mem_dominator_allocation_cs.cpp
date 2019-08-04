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
    unsigned int tag_index=0;
    unsigned int context_switch=static_cast<unsigned int>(log2(parameters->getOption<int>(OPT_context_switch)));
    if(!context_switch) context_switch=1;
    unsigned int num_threads=static_cast<unsigned int>(log2(parameters->getOption<int>(OPT_num_threads)));
    if(!num_threads) num_threads=1;
    tag_index=context_switch+num_threads+2; //tag_index is log2(switch)+log2(thread)+2
    GetPointer<memory_cs>(HLSMgr->Rmem)->set_bus_tag_bitsize(tag_index);
    return DesignFlowStep_Status::SUCCESS;
}
