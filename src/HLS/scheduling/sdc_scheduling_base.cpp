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
 * @file sdc_scheduling_base.cpp
 * @brief SDC scheduling base class
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 *
 */
#include "sdc_scheduling_base.hpp"

#include "basic_block.hpp"
#include "function_behavior.hpp"
#include "op_graph.hpp"

SDCScheduling_base::SDCScheduling_base(const ParameterConstRef _parameters, const HLS_managerRef _HLSMgr,
                                       unsigned int _funId, const DesignFlowManager& _design_flow_manager,
                                       const HLSFlowStep_Type _hls_flow_step_type,
                                       const HLSFlowStepSpecializationConstRef _hls_flow_step_specialization)
    : SchedulingStep(_parameters, _HLSMgr, _funId, _design_flow_manager, _hls_flow_step_type,
                     _hls_flow_step_specialization)
{
}

SDCSorter::SDCSorter(const FunctionBehaviorConstRef _function_behavior, const OpGraph& _op_graph)
    : function_behavior(_function_behavior),
      op_graph(_op_graph),
      bb_index_map(_function_behavior->GetBBGraph(FunctionBehavior::BB).GetGraphInfo().bb_index_map)
{
}

bool SDCSorter::operator()(OpGraph::vertex_descriptor x, OpGraph::vertex_descriptor y) const
{
   const ComparisonKey key(x, y);
   const auto cached = comparison_cache.find(key);
   if(cached != comparison_cache.end())
   {
      return cached->second;
   }
   const auto first_bb_index = op_graph.CGetNodeInfo(x).bb_index;
   const auto second_bb_index = op_graph.CGetNodeInfo(y).bb_index;
   bool result = false;
   if(first_bb_index != second_bb_index)
   {
      const auto first_bb_vertex = bb_index_map.at(first_bb_index);
      const auto second_bb_vertex = bb_index_map.at(second_bb_index);
      if(function_behavior->CheckBBReachability(first_bb_vertex, second_bb_vertex))
      {
         result = true;
         comparison_cache.emplace(key, result);
         return result;
      }
      if(function_behavior->CheckBBReachability(second_bb_vertex, first_bb_vertex))
      {
         comparison_cache.emplace(key, result);
         return result;
      }
   }
   if(x != y)
   {
      if(function_behavior->CheckReachability(x, y))
      {
         result = true;
         comparison_cache.emplace(key, result);
         return result;
      }
      if(function_behavior->CheckReachability(y, x))
      {
         comparison_cache.emplace(key, result);
         return result;
      }
   }
   result = x < y;
   comparison_cache.emplace(key, result);
   return result;
}
