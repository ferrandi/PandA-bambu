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
 * @file SchedulingStep.cpp
 * @brief scheduling base class.
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 *
 */
#include "SchedulingStep.hpp"

#include "Parameter.hpp"
#include "behavioral_helper.hpp"
#include "fu_binding.hpp"
#include "function_behavior.hpp"
#include "hls.hpp"
#include "hls_manager.hpp"
#include "ir_helper.hpp"
#include "ir_manager.hpp"
#include "op_graph.hpp"
#include "schedule.hpp"
#include "utility.hpp"

SchedulingStep::SchedulingStep(const ParameterConstRef _Param, const HLS_managerRef _HLSMgr, unsigned int _funId,
                               const DesignFlowManager& _design_flow_manager,
                               const HLSFlowStep_Type _hls_flow_step_type,
                               const HLSFlowStepSpecializationConstRef _hls_flow_step_specialization)
    : HLSFunctionStep(_Param, _HLSMgr, _funId, _design_flow_manager, _hls_flow_step_type, _hls_flow_step_specialization)
{
}

void SchedulingStep::Initialize()
{
   HLSFunctionStep::Initialize();
   if(HLS->Rsch)
   {
      HLS->Rsch->Initialize();
   }
   else
   {
      HLS->Rsch = ScheduleRef(new Schedule(HLSMgr, funId, parameters));
   }
   HLS->Rfu = fu_bindingRef(fu_binding::create_fu_binding(HLSMgr, funId, parameters));
}

HLSFunctionStep::HLSRelationships
SchedulingStep::ComputeHLSRelationships(const DesignFlowStep::RelationshipType relationship_type) const
{
   HLSRelationships ret;
   switch(relationship_type)
   {
      case(DEPENDENCE_RELATIONSHIP):
      {
         ret.insert(std::make_tuple(HLSFlowStep_Type::ALLOCATION, HLSFlowStepSpecializationConstRef(),
                                    HLSFlowStep_Relationship::SAME_FUNCTION));
         ret.insert(std::make_tuple(HLSFlowStep_Type::INITIALIZE_HLS, HLSFlowStepSpecializationConstRef(),
                                    HLSFlowStep_Relationship::SAME_FUNCTION));
         break;
      }
      case(INVALIDATION_RELATIONSHIP):
      {
         break;
      }
      case(PRECEDENCE_RELATIONSHIP):
      {
         break;
      }
      default:
         THROW_UNREACHABLE("");
   }
   return ret;
}

void SchedulingStep::compute_RW_stmts(CustomUnorderedSet<OpGraph::vertex_descriptor>& RW_stmts,
                                      const OpGraph& flow_graph, const HLS_managerRef HLSMgr, unsigned function_id)
{
   const auto TM = HLSMgr->get_ir_manager();
   const auto fnode = TM->GetIRNode(function_id);
   const auto fname = ir_helper::GetFunctionName(fnode);
   if(HLSMgr->design_interface_io.find(fname) != HLSMgr->design_interface_io.end())
   {
      for(const auto& bb2arg2stmtsR : HLSMgr->design_interface_io.at(fname))
      {
         for(const auto& arg2stms : bb2arg2stmtsR.second)
         {
            if(arg2stms.second.size() > 0)
            {
               for(const auto& stmt : arg2stms.second)
               {
                  const auto op_it = flow_graph.CGetGraphInfo().ir_node_to_operation.find(stmt);
                  if(op_it != flow_graph.CGetGraphInfo().ir_node_to_operation.end())
                  {
                     RW_stmts.insert(op_it->second);
                  }
               }
            }
         }
      }
   }
}
