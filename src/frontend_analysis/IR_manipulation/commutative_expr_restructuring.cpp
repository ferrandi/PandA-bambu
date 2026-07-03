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
 *              Copyright (C) 2018-2026 Politecnico di Milano
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
 * @file commutative_expr_restructuring.cpp
 * @brief Analysis step restructuring tree of commutative expressions to reduce the critical path delay.
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 *
 */
#include "commutative_expr_restructuring.hpp"

#include "Parameter.hpp"
#include "allocation_information.hpp"
#include "application_manager.hpp"
#include "dbgPrintHelper.hpp"
#include "design_flow_graph.hpp"
#include "design_flow_manager.hpp"
#include "fu_binding.hpp"
#include "function_behavior.hpp"
#include "hls.hpp"
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

CustomUnorderedSet<std::pair<FrontendFlowStepType, FrontendFlowStep::FunctionRelationship>>
commutative_expr_restructuring::ComputeFrontendRelationships(
    const DesignFlowStep::RelationshipType relationship_type) const
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
               GetPointer<const HLS_manager>(AppM) && GetPointer<const HLS_manager>(AppM)->get_HLS(function_id) and
               GetPointer<const HLS_manager>(AppM)->get_HLS(function_id)->Rsch)
            {
               /// If schedule is not up to date, do not execute this step and invalidate UpdateSchedule
               const auto update_schedule = design_flow_manager.GetDesignFlowStep(
                   FunctionFrontendFlowStep::ComputeSignature(FrontendFlowStepType::UPDATE_SCHEDULE, function_id));
               if(update_schedule != DesignFlowGraph::null_vertex())
               {
                  const DesignFlowGraphConstRef design_flow_graph = design_flow_manager.CGetDesignFlowGraph();
                  const DesignFlowStepRef design_flow_step =
                      design_flow_graph->CGetNodeInfo(update_schedule)->design_flow_step;
                  if(GetPointer<const FunctionFrontendFlowStep>(design_flow_step)->CGetBBVersion() !=
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

commutative_expr_restructuring::commutative_expr_restructuring(const application_managerRef _AppM,
                                                               unsigned int _function_id,
                                                               const DesignFlowManager& _design_flow_manager,
                                                               const ParameterConstRef _parameters)
    : FunctionFrontendFlowStep(_AppM, _function_id, COMMUTATIVE_EXPR_RESTRUCTURING, _design_flow_manager, _parameters)
{
   debug_level = parameters->get_class_debug_level(GET_CLASS(*this), DEBUG_LEVEL_NONE);
}

bool commutative_expr_restructuring::IsCommExpr(const ir_nodeConstRef tn) const
{
   const auto ga = GetPointer<const assign_stmt>(tn);
   if(!ga)
   {
      return false;
   }
   auto opKind = ga->op1->get_kind();
   auto Type = ir_helper::CGetType(ga->op0);
   if(!GetPointer<const integer_ty_node>(Type))
   {
      return false;
   }
   return opKind == mul_node_K || opKind == widen_mul_node_K || opKind == or_node_K || opKind == xor_node_K ||
          opKind == and_node_K || opKind == eq_node_K || opKind == ne_node_K || opKind == add_node_K;
}

ir_nodeRef commutative_expr_restructuring::IsCommExprChain(const ir_nodeConstRef tn, const bool first,
                                                           bool is_third_node) const
{
   const auto ga = GetPointer<const assign_stmt>(tn);
   const auto be = GetPointer<const binary_node>(ga->op1);
   const auto operand = first ? be->op0 : be->op1;
   const auto other_operand = first ? be->op1 : be->op0;
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
   if(def->op1->get_kind() != ga->op1->get_kind())
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

DesignFlowStep_Status commutative_expr_restructuring::InternalExec()
{
   bool modified = false;
   static size_t counter = 0;

   const ir_manipulationConstRef ir_man = ir_manipulationConstRef(new ir_manipulation(TM, parameters, AppM));
   auto* fd = GetPointer<function_val_node>(TM->GetIRNode(function_id));
   auto* sl = GetPointer<statement_list_node>(fd->body);
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
         if(!IsCommExpr(*stmt))
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Not a commutative_expr");
            continue;
         }
         bool first_operand_of_first = true;
         bool first_operand_of_second = true;
         second_stmt = IsCommExprChain(*stmt, true, false);
         if(!second_stmt)
         {
            second_stmt = IsCommExprChain(*stmt, false, false);
            first_operand_of_first = false;
         }
         if(second_stmt)
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                           "---Chained with a second commutative expression: " + STR(second_stmt));
            third_stmt = IsCommExprChain(second_stmt, true, true);
            if(!third_stmt)
            {
               third_stmt = IsCommExprChain(second_stmt, false, true);
               first_operand_of_second = false;
            }
         }
         if(!third_stmt)
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Not chained with two commutative expression");
            continue;
         }
         const auto first_ga = GetPointer<const assign_stmt>(*stmt);
         auto comm_expr_kind = first_ga->op1->get_kind();
         auto comm_expr_kind_text = first_ga->op1->get_kind_text();
         const auto first_be = GetPointer<const binary_node>(first_ga->op1);

         const auto second_ga = GetPointer<const assign_stmt>(second_stmt);
         THROW_ASSERT(second_ga->op1->get_kind() == comm_expr_kind, "unexpected condition");
         const auto second_be = GetPointer<const binary_node>(second_ga->op1);

         const auto third_ga = GetPointer<const assign_stmt>(third_stmt);
         THROW_ASSERT(third_ga->op1->get_kind() == comm_expr_kind, "unexpected condition");
         const auto third_be = GetPointer<const binary_node>(third_ga->op1);

         const double old_time = schedule->GetEndingTime(first_ga->index);

         /// Check if new ending time would not be larger
         /// The time in which last operand is ready
         double operand_ready_time = 0.0;

         const auto other_operand_of_first = first_operand_of_first ? first_be->op1 : first_be->op0;
         const auto other_operand_of_second = first_operand_of_second ? second_be->op1 : second_be->op0;
         CustomSet<std::pair<ir_nodeRef, ir_nodeRef>> operands;
         operands.insert(std::make_pair(other_operand_of_first, *stmt));
         operands.insert(std::make_pair(other_operand_of_second, second_stmt));
         operands.insert(std::make_pair(third_be->op0, third_stmt));
         operands.insert(std::make_pair(third_be->op1, third_stmt));
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

         const auto comm_expr_time1 = allocation_information->GetTimeLatency((*stmt)->index, fu_binding::UNKNOWN).first;
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                        "---Delay of first operation is " + STR(comm_expr_time1));
         const auto comm_expr_time2 =
             allocation_information->GetTimeLatency(second_stmt->index, fu_binding::UNKNOWN).first;
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                        "---Delay of second operation is " + STR(comm_expr_time2));
         const auto comm_expr_time3 =
             allocation_information->GetTimeLatency(third_stmt->index, fu_binding::UNKNOWN).first;
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                        "---Delay of third operation is " + STR(comm_expr_time3));
         const auto new_ending_time = operand_ready_time + std::max(comm_expr_time1, comm_expr_time3) +
                                      comm_expr_time2 +
                                      allocation_information->GetConnectionTime(third_ga->index, second_ga->index);
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                        "---Operand ready time " + STR(operand_ready_time) + " - New ending time " +
                            STR(new_ending_time) + " - Old ending time " + STR(old_time));
         if(new_ending_time + EPSILON >= old_time)
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Increased execution time");
            continue;
         }
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Chained with a third commutative expression");
         THROW_ASSERT(third_stmt && second_stmt, "");
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---" + STR(third_stmt));
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---" + STR(second_stmt));
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---" + STR(*stmt));

         /// Inserting first commutative expression after the last one
         const auto type_node = ir_helper::CGetType(*stmt);
         auto first_value = first_operand_of_second ? second_be->op1 : second_be->op0;
         auto second_value = first_operand_of_first ? first_be->op1 : first_be->op0;
         if(!first_operand_of_first)
         {
            std::swap(first_value, second_value);
         }
         const auto comm_expr_node =
             ir_man->create_binary_operation(type_node, first_value, second_value, BUILTIN_LOCINFO, comm_expr_kind);

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
         const auto first_ga_op0 = GetPointer<ssa_node>(first_ga->op0);
         const auto result_ssa_ref = ir_man->create_ssa_name(var, type_node, first_ga_op0->min, first_ga_op0->max);
         GetPointer<ssa_node>(result_ssa_ref)->bit_values = first_ga_op0->bit_values;

         /// Create the assign
         const auto assign_stmt_node =
             ir_man->create_assign_stmt(result_ssa_ref, comm_expr_node, function_id, BUILTIN_LOCINFO);
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Created " + assign_stmt_node->ToString());
         /// Set the bit value for the intermediate ssa to correctly update execution time
         block.second->PushBefore(assign_stmt_node, *stmt, AppM);
         new_ir_nodes.push_back(assign_stmt_node);

         /// Inserting last commutative expression
         const auto root_comm_expr =
             ir_man->create_binary_operation(type_node, first_operand_of_second ? second_be->op0 : second_be->op1,
                                             result_ssa_ref, BUILTIN_LOCINFO, comm_expr_kind);

         /// Create the assign
         const auto root_node_stmt =
             ir_man->create_assign_stmt(first_ga->op0, root_comm_expr, function_id, BUILTIN_LOCINFO);
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Created " + root_node_stmt->ToString());
         block.second->Replace(*stmt, root_node_stmt, true, AppM);
         new_ir_nodes.push_back(root_node_stmt);
         AppM->RegisterTransformation(GetName(), root_node_stmt);

         /// Check that the second commutative expression is actually dead
         THROW_ASSERT(GetPointer<const ssa_node>(second_ga->op0)->CGetUseStmts().size() == 0, "");

         /// Remove the intermediate commutative expression
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
         INDENT_OUT_MEX(OUTPUT_LEVEL_VERBOSE, output_level,
                        "-->Commutative expression restructuring applied on three " + comm_expr_kind_text +
                            " operations");
         INDENT_OUT_MEX(OUTPUT_LEVEL_VERBOSE, output_level, "<--");
         /// Restarting
         modified = true;
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Examined statement");
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

void commutative_expr_restructuring::Initialize()
{
   FunctionFrontendFlowStep::Initialize();
   TM = AppM->get_ir_manager();
   if(GetPointer<HLS_manager>(AppM) && GetPointer<HLS_manager>(AppM)->get_HLS(function_id))
   {
      schedule = GetPointer<HLS_manager>(AppM)->get_HLS(function_id)->Rsch;
      allocation_information = GetPointer<HLS_manager>(AppM)->get_HLS(function_id)->allocation_information;
   }
}

bool commutative_expr_restructuring::HasToBeExecuted() const
{
   if(!FunctionFrontendFlowStep::HasToBeExecuted())
   {
      return false;
   }
   if(parameters->getOption<HLSFlowStep_Type>(OPT_scheduling_algorithm) == HLSFlowStep_Type::SDC_SCHEDULING &&
      GetPointer<const HLS_manager>(AppM) && GetPointer<const HLS_manager>(AppM)->get_HLS(function_id) and
      GetPointer<const HLS_manager>(AppM)->get_HLS(function_id)->Rsch)
   {
      /// If schedule is not up to date, do not execute this step and invalidate UpdateSchedule
      const auto update_schedule = design_flow_manager.GetDesignFlowStep(
          FunctionFrontendFlowStep::ComputeSignature(FrontendFlowStepType::UPDATE_SCHEDULE, function_id));
      if(update_schedule != DesignFlowGraph::null_vertex())
      {
         const DesignFlowGraphConstRef design_flow_graph = design_flow_manager.CGetDesignFlowGraph();
         const DesignFlowStepRef design_flow_step = design_flow_graph->CGetNodeInfo(update_schedule)->design_flow_step;
         if(GetPointer<const FunctionFrontendFlowStep>(design_flow_step)->CGetBBVersion() !=
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
