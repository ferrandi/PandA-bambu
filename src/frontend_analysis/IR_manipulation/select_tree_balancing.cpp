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
 *              Copyright (C) 2015-2026 Politecnico di Milano
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
 * @file select_tree_balancing.cpp
 * @brief Analysis step balancing select_node trees to reduce critical path delay
 *
 * @author Marco Lattuada <marco.lattuada@polimi.it>
 *
 */
#include "select_tree_balancing.hpp"

#include "Parameter.hpp"
#include "allocation_constants.hpp"
#include "allocation_information.hpp"
#include "application_manager.hpp"
#include "dbgPrintHelper.hpp"
#include "design_flow_graph.hpp"
#include "design_flow_manager.hpp"
#include "fu_binding.hpp"
#include "function_behavior.hpp"
#include "hls.hpp"
#include "hls_device.hpp"
#include "hls_manager.hpp"
#include "hls_step.hpp"
#include "ir_basic_block.hpp"
#include "ir_helper.hpp"
#include "ir_manager.hpp"
#include "ir_manipulation.hpp"
#include "ir_node.hpp"
#include "schedule.hpp"
#include "string_manipulation.hpp"

#define EPSILON 0.0001

SelectTreeBalancing::SelectTreeBalancing(const application_managerRef _AppM, unsigned int _function_id,
                                         const DesignFlowManager& _design_flow_manager,
                                         const ParameterConstRef _parameters)
    : FunctionFrontendFlowStep(_AppM, _function_id, SELECT_TREE_BALANCING, _design_flow_manager, _parameters)
{
   debug_level = parameters->get_class_debug_level(GET_CLASS(*this), DEBUG_LEVEL_NONE);
}

CustomUnorderedSet<std::pair<FrontendFlowStepType, FrontendFlowStep::FunctionRelationship>>
SelectTreeBalancing::ComputeFrontendRelationships(const DesignFlowStep::RelationshipType relationship_type) const
{
   CustomUnorderedSet<std::pair<FrontendFlowStepType, FunctionRelationship>> relationships;
   switch(relationship_type)
   {
      case(DEPENDENCE_RELATIONSHIP):
      {
         break;
      }
      case(INVALIDATION_RELATIONSHIP):
      {
         /// Not executed
         if(GetStatus() != DesignFlowStep_Status::SUCCESS)
         {
            if(parameters->getOption<HLSFlowStep_Type>(OPT_scheduling_algorithm) == HLSFlowStep_Type::SDC_SCHEDULING &&
               GetPointer<const HLS_manager>(AppM) && GetPointerS<const HLS_manager>(AppM)->get_HLS(function_id) and
               GetPointerS<const HLS_manager>(AppM)->get_HLS(function_id)->Rsch)
            {
               /// If schedule is not up to date, do not execute this step and invalidate UpdateSchedule
               const auto update_schedule = design_flow_manager.GetDesignFlowStep(
                   FunctionFrontendFlowStep::ComputeSignature(FrontendFlowStepType::UPDATE_SCHEDULE, function_id));
               if(update_schedule != DesignFlowGraph::null_vertex())
               {
                  const auto design_flow_graph = design_flow_manager.CGetDesignFlowGraph();
                  const auto design_flow_step = design_flow_graph->CGetNodeInfo(update_schedule)->design_flow_step;
                  if(GetPointerS<const FunctionFrontendFlowStep>(design_flow_step)->CGetBBVersion() !=
                     function_behavior->GetBBVersion())
                  {
                     relationships.insert(std::make_pair(UPDATE_SCHEDULE, SAME_FUNCTION));
                     break;
                  }
               }
            }
         }
         break;
      }
      case(PRECEDENCE_RELATIONSHIP):
      {
         relationships.insert(std::make_pair(COMMUTATIVE_EXPR_RESTRUCTURING, SAME_FUNCTION));
         relationships.insert(std::make_pair(COMPLETE_BB_GRAPH, SAME_FUNCTION));
         relationships.insert(std::make_pair(FUNCTION_CALL_OPT, SAME_FUNCTION));
         relationships.insert(std::make_pair(MULTI_WAY_IF, SAME_FUNCTION));
         break;
      }
      default:
      {
         THROW_UNREACHABLE("");
      }
   }
   return relationships;
}

bool SelectTreeBalancing::HasToBeExecuted() const
{
   if(!FunctionFrontendFlowStep::HasToBeExecuted())
   {
      return false;
   }
   if(parameters->getOption<HLSFlowStep_Type>(OPT_scheduling_algorithm) == HLSFlowStep_Type::SDC_SCHEDULING &&
      GetPointer<const HLS_manager>(AppM) && GetPointerS<const HLS_manager>(AppM)->get_HLS(function_id) and
      GetPointerS<const HLS_manager>(AppM)->get_HLS(function_id)->Rsch)
   {
      /// If schedule is not up to date, do not execute this step and invalidate UpdateSchedule
      const auto update_schedule = design_flow_manager.GetDesignFlowStep(
          FunctionFrontendFlowStep::ComputeSignature(FrontendFlowStepType::UPDATE_SCHEDULE, function_id));
      if(update_schedule != DesignFlowGraph::null_vertex())
      {
         const auto design_flow_graph = design_flow_manager.CGetDesignFlowGraph();
         const auto design_flow_step = design_flow_graph->CGetNodeInfo(update_schedule)->design_flow_step;
         if(GetPointerS<const FunctionFrontendFlowStep>(design_flow_step)->CGetBBVersion() !=
            function_behavior->GetBBVersion())
         {
            return false;
         }
         else
         {
            return true;
         }
      }
      else
      {
         return false;
      }
   }
   else
   {
      return false;
   }
}

void SelectTreeBalancing::Initialize()
{
   FunctionFrontendFlowStep::Initialize();
   TM = AppM->get_ir_manager();
   if(GetPointer<HLS_manager>(AppM) && GetPointer<HLS_manager>(AppM)->get_HLS(function_id))
   {
      schedule = GetPointer<HLS_manager>(AppM)->get_HLS(function_id)->Rsch;
      allocation_information = GetPointer<HLS_manager>(AppM)->get_HLS(function_id)->allocation_information;
   }
}

DesignFlowStep_Status SelectTreeBalancing::InternalExec()
{
   bool modified = false;
   static size_t counter = 0;

   THROW_ASSERT(GetPointer<const HLS_manager>(AppM)->get_HLS_device(), "unexpected condition");
   const auto hls_d = GetPointerS<const HLS_manager>(AppM)->get_HLS_device();
   const auto max_lut_size = AppM->GetRequiredParameterFromParameterOrDevice<size_t>("max_lut_size", hls_d);
   if(max_lut_size == 0)
   {
      THROW_ERROR("Invalid parameter \"max_lut_size\": expected value > 0");
   }

   const ir_manipulationConstRef ir_man(new ir_manipulation(TM, parameters, AppM));
   const auto fd = GetPointerS<const function_val_node>(TM->GetIRNode(function_id));
   const auto sl = GetPointerS<const statement_list_node>(fd->body);
   for(const auto& block : sl->list_of_bloc)
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Examining BB" + STR(block.first));
      const auto& list_of_stmt = block.second->CGetStmtList();
      for(auto stmt = list_of_stmt.begin(); stmt != list_of_stmt.end(); stmt++)
      {
         if(!AppM->ApplyNewTransformation())
         {
            break;
         }

         std::list<ir_nodeRef> new_ir_nodes;

         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Examining " + (*stmt)->ToString());
         auto next_stmt_ptr = std::next(stmt);
         ir_nodeRef next_stmt = std::next(stmt) != list_of_stmt.end() ? *(std::next(stmt)) : ir_nodeRef();
         ir_nodeRef first_stmt = *stmt;
         ir_nodeRef second_stmt = ir_nodeRef();
         ir_nodeRef third_stmt = ir_nodeRef();
         if(!IsSelectStmt(*stmt))
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Not a select_node");
            continue;
         }
         bool first_operand_of_first = true;
         bool first_operand_of_second = true;
         second_stmt = FindSelectChain(*stmt, true, false);
         if(!second_stmt)
         {
            second_stmt = FindSelectChain(*stmt, false, false);
            first_operand_of_first = false;
         }
         if(second_stmt)
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                           "---Chained with a second select_node: " + STR(second_stmt));
            third_stmt = FindSelectChain(second_stmt, true, true);
            if(!third_stmt)
            {
               third_stmt = FindSelectChain(second_stmt, false, true);
               first_operand_of_second = false;
            }
         }
         if(!third_stmt)
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Not chained with two select nodes");
            continue;
         }
         const auto first_ga = GetPointer<const assign_stmt>(*stmt);
         const auto first_ce = GetPointer<const select_node>(first_ga->op1);

         const auto second_ga = GetPointer<const assign_stmt>(second_stmt);
         const auto second_ce = GetPointer<const select_node>(second_ga->op1);

         const auto third_ga = GetPointer<const assign_stmt>(third_stmt);
         const auto third_ce = GetPointer<const select_node>(third_ga->op1);

         const double old_time = schedule->GetEndingTime(first_ga->index);

         /// Check if new ending time would not be larger
         /// The time in which last operand is ready
         double operand_ready_time = 0.0;

         const auto other_operand_of_first = first_operand_of_first ? first_ce->op2 : first_ce->op1;
         const auto other_operand_of_second = first_operand_of_second ? second_ce->op2 : second_ce->op1;
         CustomSet<std::pair<ir_nodeRef, ir_nodeRef>> operands;
         operands.insert(std::make_pair(other_operand_of_first, *stmt));
         operands.insert(std::make_pair(other_operand_of_second, second_stmt));
         operands.insert(std::make_pair(third_ce->op1, third_stmt));
         operands.insert(std::make_pair(third_ce->op2, third_stmt));
         operands.insert(std::make_pair(first_ce->op0, *stmt));
         operands.insert(std::make_pair(second_ce->op0, second_stmt));
         operands.insert(std::make_pair(third_ce->op0, third_stmt));
         for(const auto& operand : operands)
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                           "---Analyzing when " + STR(operand.first) + " used in " + STR(operand.second) + " is ready");
            const auto sn = GetPointer<const ssa_node>(operand.first);
            if(sn)
            {
               const auto def_operand = GetPointer<const node_stmt>(sn->GetDefStmt());
               if(def_operand->bb_index == block.first)
               {
                  const auto def_stmt = sn->GetDefStmt();
                  const auto ending_time = schedule->GetEndingTime(def_stmt->index);
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Ending time is " + STR(ending_time));
                  const auto connection_time =
                      allocation_information->GetConnectionTime(def_stmt->index, operand.second->index);
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                 "---Connection time is " + STR(connection_time));
                  const auto current_operand_ready_time = ending_time + connection_time;
                  operand_ready_time = std::max(operand_ready_time, current_operand_ready_time);
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                 "---New ready time is " + STR(operand_ready_time));
               }
            }
         }

         /// For the select-node time we consider the worst latency among the existing operations in the chain.
         const auto select_node_time1 =
             allocation_information->GetTimeLatency((*stmt)->index, fu_binding::UNKNOWN).first;
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                        "---Delay of first operation is " + STR(select_node_time1));
         const auto select_node_time2 =
             allocation_information->GetTimeLatency(second_stmt->index, fu_binding::UNKNOWN).first;
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                        "---Delay of second operation is " + STR(select_node_time2));
         const auto select_node_time3 =
             allocation_information->GetTimeLatency(third_stmt->index, fu_binding::UNKNOWN).first;
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                        "---Delay of third operation is " + STR(select_node_time3));
         const auto new_ending_time = operand_ready_time + std::max(select_node_time1, select_node_time3) +
                                      select_node_time2 +
                                      allocation_information->GetConnectionTime(third_ga->index, second_ga->index);
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                        "---Operand ready time " + STR(operand_ready_time) + " - New ending time " +
                            STR(new_ending_time) + " - Old ending time " + STR(old_time));
         if(new_ending_time + EPSILON >= old_time)
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Increased execution time");
            continue;
         }
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Chained with a third select_node");
         THROW_ASSERT(third_stmt && second_stmt, "");
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---" + STR(third_stmt));
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---" + STR(second_stmt));
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---" + STR(*stmt));

         /// Insert the first select node after the last one.
         const auto type_node = ir_helper::CGetType(*stmt);
         auto first_value = first_operand_of_second ? second_ce->op2 : second_ce->op1;
         auto second_value = first_operand_of_first ? first_ce->op2 : first_ce->op1;
         if(!first_operand_of_first)
         {
            std::swap(first_value, second_value);
         }
         const auto select_node_node = ir_man->create_ternary_operation(type_node, first_ce->op0, first_value,
                                                                        second_value, BUILTIN_LOCINFO, select_node_K);

         /// Create the ssa in the left part
         ir_nodeRef var = nullptr;
         if(first_value->get_kind() == ssa_node_K && second_value->get_kind() == ssa_node_K)
         {
            const auto sn1 = GetPointer<const ssa_node>(first_value);
            const auto sn2 = GetPointer<const ssa_node>(second_value);
            if(sn1->var && sn2->var && sn1->var->index == sn2->var->index)
            {
               var = sn1->var;
            }
         }
         const auto first_ga_op0 = GetPointer<const ssa_node>(first_ga->op0);
         const auto result_ssa_ref = ir_man->create_ssa_name(var, type_node, first_ga_op0->min, first_ga_op0->max);
         GetPointerS<ssa_node>(result_ssa_ref)->bit_values = first_ga_op0->bit_values;

         /// Create the assign
         const auto curr_stmt =
             ir_man->create_assign_stmt(result_ssa_ref, select_node_node, function_id, BUILTIN_LOCINFO);
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Created " + curr_stmt->ToString());
         /// Set the bit value for the intermediate ssa to correctly update execution time
         block.second->PushBefore(curr_stmt, *stmt, AppM);
         new_ir_nodes.push_back(curr_stmt);

         ir_nodeRef and_first_cond;
         if(first_ce->op0->index == second_ce->op0->index)
         {
            /// simplified version
            if(!first_operand_of_first && !first_operand_of_second)
            {
               const auto boolType = ir_man->GetBooleanType();
               const auto DefaultUnsignedLongLongInt = ir_man->GetUnsignedLongLongType();
               const auto lut_constant_node = TM->CreateUniqueIntegerCst(1, DefaultUnsignedLongLongInt);
               const auto new_op1 =
                   ir_man->create_lut_node(boolType, lut_constant_node, first_ce->op0, nullptr, nullptr, nullptr,
                                           nullptr, nullptr, nullptr, nullptr, BUILTIN_LOCINFO);
               const auto ga = ir_man->CreateAssignStmt(boolType, TM->CreateUniqueIntegerCst(0, boolType),
                                                        TM->CreateUniqueIntegerCst(1, boolType), new_op1, function_id,
                                                        BUILTIN_LOCINFO);
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Created LUT NOT " + STR(ga));
               block.second->PushBefore(ga, *stmt, AppM);
               new_ir_nodes.push_back(ga);
               and_first_cond = GetPointerS<assign_stmt>(ga)->op0;
            }
            else if(first_operand_of_first && first_operand_of_second)
            {
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Simplified to identity");
               and_first_cond = first_ce->op0;
            }
            else
            {
               const auto boolType = ir_man->GetBooleanType();
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Simplified to 0");
               and_first_cond = TM->CreateUniqueIntegerCst(0, boolType);
            }
         }
         else
         {
            const auto boolType = ir_man->GetBooleanType();
            /// we are going to create a LUT
            const auto DefaultUnsignedLongLongInt = ir_man->GetUnsignedLongLongType();
            long long int lut_val;
            if(!first_operand_of_first && !first_operand_of_second)
            {
               lut_val = 1;
            }
            else if(!first_operand_of_first && first_operand_of_second)
            {
               lut_val = 2;
            }
            else if(first_operand_of_first && !first_operand_of_second)
            {
               lut_val = 4;
            }
            else
            {
               lut_val = 8;
            }
            const auto lut_constant_node = TM->CreateUniqueIntegerCst(lut_val, DefaultUnsignedLongLongInt);
            const auto new_op1 =
                ir_man->create_lut_node(boolType, lut_constant_node, second_ce->op0, first_ce->op0, nullptr, nullptr,
                                        nullptr, nullptr, nullptr, nullptr, BUILTIN_LOCINFO);
            const auto ga = ir_man->CreateAssignStmt(boolType, TM->CreateUniqueIntegerCst(0, boolType),
                                                     TM->CreateUniqueIntegerCst(1, boolType), new_op1, function_id,
                                                     BUILTIN_LOCINFO);
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Created LUT STD " + STR(ga));
            block.second->PushBefore(ga, *stmt, AppM);
            new_ir_nodes.push_back(ga);
            and_first_cond = GetPointerS<assign_stmt>(ga)->op0;
         }

         /// Insert the last select node.
         const auto root_select_node = ir_man->create_ternary_operation(
             type_node, and_first_cond, first_operand_of_second ? second_ce->op1 : second_ce->op2, result_ssa_ref,
             BUILTIN_LOCINFO, select_node_K);

         /// Create the assign
         const auto root_node_stmt =
             ir_man->create_assign_stmt(first_ga->op0, root_select_node, function_id, BUILTIN_LOCINFO);
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Created " + root_node_stmt->ToString());
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Before " + (*stmt)->ToString());
         block.second->Replace(*stmt, root_node_stmt, true, AppM);
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---After " + (*stmt)->ToString());
         new_ir_nodes.push_back(root_node_stmt);
         AppM->RegisterTransformation(GetName(), root_node_stmt);

         /// Check that the second select node is actually dead.
         THROW_ASSERT(GetPointer<const ssa_node>(second_ga->op0)->CGetUseStmts().size() == 0, "");

         /// Remove the intermediate select_node
         block.second->RemoveStmt(second_stmt, AppM);

         for(const auto& temp_stmt : list_of_stmt)
         {
            schedule->UpdateTime(temp_stmt->index);
         }

         if(debug_level >= DEBUG_LEVEL_VERY_PEDANTIC &&
            (!parameters->IsParameter("print-dot-FF") || parameters->GetParameter<unsigned int>("print-dot-FF")))
         {
            WriteBBGraphDot("BB_Inside_" + GetName() + "_" + STR(counter) + ".dot");
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                           "---Written BB_Inside_" + GetName() + "_" + STR(counter) + ".dot");
            counter++;
         }
         const double new_time = schedule->GetEndingTime(root_node_stmt->index);
         if(new_time + EPSILON > old_time)
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Error in estimation");
            /// Removing added statements
            for(const auto& to_be_removed : new_ir_nodes)
            {
               block.second->RemoveStmt(to_be_removed, AppM);
            }

            /// Adding old statements
            next_stmt ? block.second->PushBefore(second_stmt, next_stmt, AppM) :
                        block.second->PushBack(second_stmt, AppM);
            next_stmt ? block.second->PushBefore(first_stmt, next_stmt, AppM) :
                        block.second->PushBack(first_stmt, AppM);

            /// Recomputing schedule
            for(const auto& temp_stmt : list_of_stmt)
            {
               schedule->UpdateTime(temp_stmt->index);
            }

            /// Setting pointer to the previous element
            stmt = std::prev(next_stmt_ptr);
            continue;
         }
         /// Restarting
         modified = true;
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Examined");
         stmt = list_of_stmt.begin();
      }
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Examined BB" + STR(block.first));
   }

   if(modified)
   {
      function_behavior->UpdateBBVersion();
   }
   return modified ? DesignFlowStep_Status::SUCCESS : DesignFlowStep_Status::UNCHANGED;
}

bool SelectTreeBalancing::IsSelectStmt(const ir_nodeConstRef tn) const
{
   const auto ga = GetPointer<const assign_stmt>(tn);
   if(!ga)
   {
      return false;
   }
   return ga->op1->get_kind() == select_node_K;
}

ir_nodeRef SelectTreeBalancing::FindSelectChain(const ir_nodeConstRef tn, const bool first, bool is_third_node) const
{
   const auto ga = GetPointer<const assign_stmt>(tn);
   const auto ce = GetPointer<const select_node>(ga->op1);
   const auto operand = first ? ce->op1 : ce->op2;
   const auto other_operand = first ? ce->op2 : ce->op1;
   const auto sn = GetPointer<const ssa_node>(operand);
   if(ir_helper::IsConstant(other_operand))
   {
      return ir_nodeRef();
   }
   if(!sn)
   {
      return ir_nodeRef();
   }
   if(!is_third_node && sn->CGetNumberUses() > 1)
   {
      return ir_nodeRef();
   }
   const auto def = GetPointer<const assign_stmt>(sn->GetDefStmt());
   if(!def)
   {
      return ir_nodeRef();
   }
   if(def->bb_index != ga->bb_index)
   {
      return ir_nodeRef();
   }
   const auto chain_ce = GetPointer<const select_node>(def->op1);
   if(!chain_ce)
   {
      return ir_nodeRef();
   }
   if(schedule->GetStartingTime(ga->index) == schedule->GetEndingTime(def->index) or
      (schedule->get_cstep_end(def->index).second + 1) == schedule->get_cstep(ga->index).second)
   {
      return sn->GetDefStmt();
   }
   else
   {
      return ir_nodeRef();
   }
}
