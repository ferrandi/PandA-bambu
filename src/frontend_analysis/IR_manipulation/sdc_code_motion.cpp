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
 * @file sdc_code_motion.cpp
 * @brief Analysis step performing code motion speculation on the basis of sdc results.
 *
 * @author Marco Lattuada <marco.lattuada@polimi.it>
 *
 */
#include "sdc_code_motion.hpp"

#include "Parameter.hpp"
#include "basic_block.hpp"
#include "dbgPrintHelper.hpp"
#include "design_flow_graph.hpp"
#include "design_flow_manager.hpp"
#include "frontend_flow_step_factory.hpp"
#include "function_behavior.hpp"
#include "hls.hpp"
#include "hls_flow_step_factory.hpp"
#include "hls_manager.hpp"
#include "ir_basic_block.hpp"
#include "ir_helper.hpp"
#include "ir_manager.hpp"
#include "ir_node.hpp"
#include "op_graph.hpp"
#include "sdc_scheduling_base.hpp"
#include "simple_code_motion.hpp"
#include "string_manipulation.hpp"

SDCCodeMotion::SDCCodeMotion(const application_managerRef _AppM, unsigned int _function_id,
                             const DesignFlowManager& _design_flow_manager, const ParameterConstRef _parameters)
    : FunctionFrontendFlowStep(_AppM, _function_id, SDC_CODE_MOTION, _design_flow_manager, _parameters),
      restart_ifmwi_opt(false)
{
   debug_level = parameters->get_class_debug_level(GET_CLASS(*this), DEBUG_LEVEL_NONE);
}

CustomUnorderedSet<std::pair<FrontendFlowStepType, FrontendFlowStep::FunctionRelationship>>
SDCCodeMotion::ComputeFrontendRelationships(const DesignFlowStep::RelationshipType relationship_type) const
{
   CustomUnorderedSet<std::pair<FrontendFlowStepType, FunctionRelationship>> relationships;
   switch(relationship_type)
   {
      case(DEPENDENCE_RELATIONSHIP):
      {
         relationships.insert(std::make_pair(DEAD_CODE_ELIMINATION_IPA, WHOLE_APPLICATION));
         break;
      }
      case(INVALIDATION_RELATIONSHIP):
      {
         if(GetStatus() == DesignFlowStep_Status::SUCCESS)
         {
            if(restart_ifmwi_opt)
            {
               relationships.insert(std::make_pair(PHI_OPT, SAME_FUNCTION));
               relationships.insert(std::make_pair(MULTI_WAY_IF, SAME_FUNCTION));
               relationships.insert(std::make_pair(UPDATE_SCHEDULE, SAME_FUNCTION));
            }
         }
         break;
      }
      case(PRECEDENCE_RELATIONSHIP):
      {
         relationships.insert(std::make_pair(PHI_OPT, SAME_FUNCTION));
         relationships.insert(std::make_pair(MULTI_WAY_IF, SAME_FUNCTION));
         relationships.insert(std::make_pair(UPDATE_SCHEDULE, SAME_FUNCTION));
         break;
      }
      default:
      {
         THROW_UNREACHABLE("");
      }
   }
   return relationships;
}

bool SDCCodeMotion::HasToBeExecuted() const
{
   return bb_version == 0 &&
          parameters->getOption<HLSFlowStep_Type>(OPT_scheduling_algorithm) == HLSFlowStep_Type::SDC_SCHEDULING &&
          GetPointer<const HLS_manager>(AppM) && GetPointerS<const HLS_manager>(AppM)->get_HLS(function_id) &&
          GetPointerS<const HLS_manager>(AppM)->get_HLS(function_id)->Rsch &&
          FunctionFrontendFlowStep::HasToBeExecuted();
}

DesignFlowStep_Status SDCCodeMotion::InternalExec()
{
   const auto design_flow_graph = design_flow_manager.CGetDesignFlowGraph();
   restart_ifmwi_opt = false;

   const ir_managerRef TM = AppM->get_ir_manager();
   auto* fd = GetPointer<function_val_node>(TM->GetIRNode(function_id));
   auto* sl = GetPointer<statement_list_node>(fd->body);
   std::map<unsigned int, blocRef>& list_of_bloc = sl->list_of_bloc;

   /// Retrieve result of sdc scheduling
   const auto sdc_scheduling_step = design_flow_manager.GetDesignFlowStep(HLSFunctionStep::ComputeSignature(
       HLSFlowStep_Type::SDC_SCHEDULING, HLSFlowStepSpecializationConstRef(), function_id));
   THROW_ASSERT(sdc_scheduling_step != DesignFlowGraph::null_vertex(), "SDC scheduling hls step not found");
   const auto sdc_scheduling =
       GetPointer<const SDCScheduling_base>(design_flow_graph->CGetNodeInfo(sdc_scheduling_step)->design_flow_step);
   const auto& movements_list = sdc_scheduling->movements_list;
   if(movements_list.empty())
   {
      return DesignFlowStep_Status::UNCHANGED;
   }
   for(const auto& movement : movements_list)
   {
      const auto statement_index = movement[0];
      const auto old_basic_block = movement[1];
      const auto new_basic_block = movement[2];
      THROW_ASSERT(list_of_bloc.find(old_basic_block) != list_of_bloc.end() &&
                       list_of_bloc.find(new_basic_block) != list_of_bloc.end(),
                   "unexpected condition: BB are missing");
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                     "-->Moving " + STR(TM->GetIRNode(statement_index)) + " from BB" + STR(old_basic_block) + " to BB" +
                         STR(new_basic_block));
      if(not AppM->ApplyNewTransformation())
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                        "<--Skipped because reached limit of cfg transformations");
         continue;
      }
      list_of_bloc.at(old_basic_block)->RemoveStmt(TM->GetIRNode(statement_index), AppM);
      if(list_of_bloc.at(old_basic_block)->CGetStmtList().empty() &&
         list_of_bloc.at(old_basic_block)->CGetPhiList().empty())
      {
         restart_ifmwi_opt = true;
      }
      list_of_bloc.at(new_basic_block)->PushBack(TM->GetIRNode(statement_index), AppM);
      AppM->RegisterTransformation(GetName(), TM->GetIRNode(statement_index));
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Moved " + STR(statement_index));
   }
   function_behavior->UpdateBBVersion();
   return DesignFlowStep_Status::SUCCESS;
}
