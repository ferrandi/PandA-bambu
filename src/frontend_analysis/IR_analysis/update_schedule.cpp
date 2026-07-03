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
 *              Copyright (C) 2016-2026 Politecnico di Milano
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
 * @file update_schedule.cpp
 * @brief Analysis step which updates the schedule of all the instructions
 *
 * @author Marco Lattuada <marco.lattuada@polimi.it>
 *
 */
#include "update_schedule.hpp"

#include "Parameter.hpp"
#include "application_manager.hpp"
#include "basic_block.hpp"
#include "dbgPrintHelper.hpp"
#include "function_behavior.hpp"
#include "hls.hpp"
#include "hls_manager.hpp"
#include "hls_step.hpp"
#include "ir_basic_block.hpp"
#include "ir_manager.hpp"
#include "schedule.hpp"
#include "string_manipulation.hpp"

UpdateSchedule::UpdateSchedule(const application_managerRef _AppM, unsigned int _function_id,
                               const DesignFlowManager& _design_flow_manager, const ParameterConstRef _parameters)
    : FunctionFrontendFlowStep(_AppM, _function_id, UPDATE_SCHEDULE, _design_flow_manager, _parameters)
{
   debug_level = parameters->get_class_debug_level(GET_CLASS(*this), DEBUG_LEVEL_NONE);
}

CustomUnorderedSet<std::pair<FrontendFlowStepType, FrontendFlowStep::FunctionRelationship>>
UpdateSchedule::ComputeFrontendRelationships(const DesignFlowStep::RelationshipType relationship_type) const
{
   CustomUnorderedSet<std::pair<FrontendFlowStepType, FunctionRelationship>> relationships;
   switch(relationship_type)
   {
      case(DEPENDENCE_RELATIONSHIP):
      {
         break;
      }
      case(PRECEDENCE_RELATIONSHIP):
      {
         break;
      }
      case(INVALIDATION_RELATIONSHIP):
      {
         break;
      }
      default:
      {
         THROW_UNREACHABLE("");
      }
   }
   return relationships;
}

void UpdateSchedule::Initialize()
{
   if(GetPointer<const HLS_manager>(AppM) and GetPointer<const HLS_manager>(AppM)->get_HLS(function_id) and
      GetPointer<const HLS_manager>(AppM)->get_HLS(function_id)->Rsch)
   {
      schedule = GetPointer<const HLS_manager>(AppM)->get_HLS(function_id)->Rsch;
      /// Set reference to schedule in basic blocks
      auto basic_block_graph = function_behavior->GetBBGraph(FunctionBehavior::BB);
      for(const auto& basic_block : basic_block_graph.vertices())
      {
         basic_block_graph.GetNodeInfo(basic_block).block->schedule = schedule;
      }
   }
}

bool UpdateSchedule::HasToBeExecuted() const
{
   if(parameters->getOption<HLSFlowStep_Type>(OPT_scheduling_algorithm) == HLSFlowStep_Type::SDC_SCHEDULING and
      GetPointer<const HLS_manager>(AppM) and GetPointer<const HLS_manager>(AppM)->get_HLS(function_id) and
      GetPointer<const HLS_manager>(AppM)->get_HLS(function_id)->Rsch)
   {
      return FunctionFrontendFlowStep::HasToBeExecuted();
   }
   else
   {
      return false;
   }
}

DesignFlowStep_Status UpdateSchedule::InternalExec()
{
   const auto TM = AppM->get_ir_manager();
   auto* fd = GetPointer<function_val_node>(TM->GetIRNode(function_id));
   auto* sl = GetPointer<statement_list_node>(fd->body);
   for(const auto& block : sl->list_of_bloc)
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->BB" + STR(block.first));
      for(const auto& phi : block.second->CGetPhiList())
      {
         schedule->UpdateTime(phi->index);
      }
      for(const auto& statement : block.second->CGetStmtList())
      {
         schedule->UpdateTime(statement->index);
      }
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--");
   }
   return DesignFlowStep_Status::SUCCESS;
}
