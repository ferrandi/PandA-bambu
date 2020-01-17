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
 * @file mem_dominator_allocation_CS.hpp
 * @brief Class to allocate memories in HLS based on dominators, add tag for context switch
 *
 * @author Nicola Saporetti <nicola.saporetti@gmail.com>
 */
#include "mem_dominator_allocation_cs.hpp"
#include "Parameter.hpp"
#include "behavioral_helper.hpp"
#include "call_graph_manager.hpp"
#include "cmath"
#include "function_behavior.hpp"
#include "hls_manager.hpp"
#include "memory_cs.hpp"
#include "omp_functions.hpp"

/// utility include
#include "math_function.hpp"
#include "utility.hpp"

mem_dominator_allocation_cs::mem_dominator_allocation_cs(const ParameterConstRef _parameters, const HLS_managerRef _HLSMgr, const DesignFlowManagerConstRef _design_flow_manager, const HLSFlowStepSpecializationConstRef _hls_flow_step_specialization,
                                                         const HLSFlowStep_Type _hls_flow_step_type)
    : mem_dominator_allocation(_parameters, _HLSMgr, _design_flow_manager, _hls_flow_step_specialization, _hls_flow_step_type)
{
   debug_level = _parameters->get_class_debug_level(GET_CLASS(*this));
}

mem_dominator_allocation_cs::~mem_dominator_allocation_cs()
{
}

DesignFlowStep_Status mem_dominator_allocation_cs::Exec()
{
   mem_dominator_allocation::Exec(); // exec of hierarchical class
   int tag_index = 0;
   int context_switch = ceil_log2(parameters->getOption<unsigned long long int>(OPT_context_switch));
   if(!context_switch)
      context_switch = 1;
   int num_bits_acc = ceil_log2(parameters->getOption<unsigned long long>(OPT_num_accelerators));
   if(!num_bits_acc)
      num_bits_acc = 1;
   tag_index = context_switch + num_bits_acc + 2; // tag_index is log2(switch)+log2(thread)+2
   GetPointer<memory_cs>(HLSMgr->Rmem)->set_bus_tag_bitsize(static_cast<unsigned>(tag_index));
   return DesignFlowStep_Status::SUCCESS;
}
