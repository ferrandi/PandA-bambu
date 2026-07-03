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
 *              Copyright (C) 2004-2026 Politecnico di Milano
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
 * @file sched_based_chaining_computation.cpp
 * @brief chaining computation starting from the results of the scheduling step
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 */
#include "sched_based_chaining_computation.hpp"

#include "Parameter.hpp"
#include "chaining_information.hpp"
#include "custom_map.hpp"
#include "custom_set.hpp"
#include "dbgPrintHelper.hpp"
#include "exceptions.hpp"
#include "function_behavior.hpp"
#include "hls.hpp"
#include "hls_manager.hpp"
#include "schedule.hpp"
#include "string_manipulation.hpp"

sched_based_chaining_computation::sched_based_chaining_computation(const ParameterConstRef _Param,
                                                                   const HLS_managerRef _HLSMgr, unsigned int _funId,
                                                                   const DesignFlowManager& _design_flow_manager)
    : chaining(_Param, _HLSMgr, _funId, _design_flow_manager, HLSFlowStep_Type::SCHED_CHAINING)
{
   debug_level = parameters->get_class_debug_level(GET_CLASS(*this));
}

void sched_based_chaining_computation::Initialize()
{
   HLSFunctionStep::Initialize();
   HLS->chaining_information = ChainingInformationRef(new ChainingInformation(HLSMgr, funId));
   HLS->chaining_information->Initialize();
}

DesignFlowStep_Status sched_based_chaining_computation::InternalExec()
{
   const auto FB = HLSMgr->CGetFunctionBehavior(funId);
   const auto flow_graph = FB->GetOpGraph(FunctionBehavior::FLSAODG);

   for(const auto op : flow_graph.vertices())
   {
      const auto current_starting_cycle = HLS->Rsch->get_cstep(op);
      const auto current_ending_cycle = HLS->Rsch->get_cstep_end(op);
      if(current_starting_cycle == current_ending_cycle)
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                        "---Operations " + flow_graph.CGetNodeInfo(op).vertex_name + " and " +
                            flow_graph.CGetNodeInfo(op).vertex_name + " are chained in");
         HLS->chaining_information->add_chained_vertices_in(op, op);
      }
      bool is_chained_test = false;
      for(const auto& ei : flow_graph.in_edges(op))
      {
         auto src = flow_graph.source(ei);
         if(HLS->Rsch->get_cstep_end(src) == current_starting_cycle)
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level,
                           std::string("Operations ") + flow_graph.CGetNodeInfo(src).vertex_name + " and " +
                               flow_graph.CGetNodeInfo(op).vertex_name + " are chained in");
            HLS->chaining_information->add_chained_vertices_in(op, src);
            is_chained_test = true;
         }
      }
      if(is_chained_test)
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level,
                        std::string("Operations ") + flow_graph.CGetNodeInfo(op).vertex_name +
                            " is chained with something");
         HLS->chaining_information->is_chained_with.insert(op);
      }
      for(auto eo : flow_graph.out_edges(op))
      {
         auto tgt = flow_graph.target(eo);
         if(HLS->Rsch->get_cstep(tgt) == current_ending_cycle)
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level,
                           std::string("Operations ") + flow_graph.CGetNodeInfo(tgt).vertex_name + " and " +
                               flow_graph.CGetNodeInfo(op).vertex_name + " are chained out");
            HLS->chaining_information->add_chained_vertices_out(op, tgt);
         }
      }
   }
   return DesignFlowStep_Status::SUCCESS;
}
