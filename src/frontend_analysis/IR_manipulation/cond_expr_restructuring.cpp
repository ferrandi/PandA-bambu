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
 *              Copyright (C) 2015-2024 Politecnico di Milano
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
 * @file cond_expr_restructuring.cpp
 * @brief Analysis step restructuring tree of cond_expr to reduce critical path delay
 *
 * @author Marco Lattuada <marco.lattuada@polimi.it>
 *
 */

/// Header include
#include "cond_expr_restructuring.hpp"

///. include
#include "Parameter.hpp"

/// behavior include
#include "application_manager.hpp"

/// design_flows includes
#include "design_flow_graph.hpp"
#include "design_flow_manager.hpp"

/// HLS includes
#include "hls.hpp"
#include "hls_device.hpp"
#include "hls_manager.hpp"
#include "hls_step.hpp"

/// HLS/binding/module
#include "fu_binding.hpp"

/// HLS/module_allocation
#include "allocation_information.hpp"

/// HLS/scheduling include
#include "schedule.hpp"

/// parser/compiler include
#include "token_interface.hpp"

/// tree includes
#include "dbgPrintHelper.hpp"      // for DEBUG_LEVEL_
#include "string_manipulation.hpp" // for GET_CLASS
#include "tree_basic_block.hpp"
#include "tree_helper.hpp"
#include "tree_manager.hpp"
#include "tree_manipulation.hpp"
#include "tree_node.hpp"

#define EPSILON 0.0001

CondExprRestructuring::CondExprRestructuring(const application_managerRef _AppM, unsigned int _function_id,
                                             const DesignFlowManagerConstRef _design_flow_manager,
                                             const ParameterConstRef _parameters)
    : FunctionFrontendFlowStep(_AppM, _function_id, COND_EXPR_RESTRUCTURING, _design_flow_manager, _parameters)
{
   debug_level = parameters->get_class_debug_level(GET_CLASS(*this), DEBUG_LEVEL_NONE);
}

CondExprRestructuring::~CondExprRestructuring() = default;

CustomUnorderedSet<std::pair<FrontendFlowStepType, FrontendFlowStep::FunctionRelationship>>
CondExprRestructuring::ComputeFrontendRelationships(const DesignFlowStep::RelationshipType relationship_type) const
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
               const auto update_schedule = design_flow_manager.lock()->GetDesignFlowStep(
                   FunctionFrontendFlowStep::ComputeSignature(FrontendFlowStepType::UPDATE_SCHEDULE, function_id));
               if(update_schedule != DesignFlowGraph::null_vertex())
               {
                  const auto design_flow_graph = design_flow_manager.lock()->CGetDesignFlowGraph();
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
         relationships.insert(std::make_pair(SHORT_CIRCUIT_TAF, SAME_FUNCTION));
         break;
      }
      default:
      {
         THROW_UNREACHABLE("");
      }
   }
   return relationships;
}

bool CondExprRestructuring::HasToBeExecuted() const
{
   if(!FunctionFrontendFlowStep::HasToBeExecuted())
   {
      return false;
   }
#if HAVE_ILP_BUILT
   if(parameters->getOption<HLSFlowStep_Type>(OPT_scheduling_algorithm) == HLSFlowStep_Type::SDC_SCHEDULING &&
      GetPointer<const HLS_manager>(AppM) && GetPointerS<const HLS_manager>(AppM)->get_HLS(function_id) and
      GetPointerS<const HLS_manager>(AppM)->get_HLS(function_id)->Rsch)
   {
      /// If schedule is not up to date, do not execute this step and invalidate UpdateSchedule
      const auto update_schedule = design_flow_manager.lock()->GetDesignFlowStep(
          FunctionFrontendFlowStep::ComputeSignature(FrontendFlowStepType::UPDATE_SCHEDULE, function_id));
      if(update_schedule != DesignFlowGraph::null_vertex())
      {
         const auto design_flow_graph = design_flow_manager.lock()->CGetDesignFlowGraph();
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
#endif
   {
      return false;
   }
}

void CondExprRestructuring::Initialize()
{
   FunctionFrontendFlowStep::Initialize();
   TM = AppM->get_tree_manager();
   if(GetPointer<HLS_manager>(AppM) && GetPointer<HLS_manager>(AppM)->get_HLS(function_id))
   {
      schedule = GetPointer<HLS_manager>(AppM)->get_HLS(function_id)->Rsch;
      allocation_information = GetPointer<HLS_manager>(AppM)->get_HLS(function_id)->allocation_information;
   }
}

DesignFlowStep_Status CondExprRestructuring::InternalExec()
{
   bool modified = false;
   static size_t counter = 0;

   THROW_ASSERT(GetPointer<const HLS_manager>(AppM)->get_HLS_device(), "unexpected condition");
   const auto hls_d = GetPointerS<const HLS_manager>(AppM)->get_HLS_device();
   THROW_ASSERT(hls_d->has_parameter("max_lut_size"), "unexpected condition");
   const auto max_lut_size = hls_d->get_parameter<size_t>("max_lut_size");

   const tree_manipulationConstRef tree_man(new tree_manipulation(TM, parameters, AppM));
   const auto fd = GetPointerS<const function_decl>(TM->GetTreeNode(function_id));
   const auto sl = GetPointerS<const statement_list>(fd->body);
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

         std::list<tree_nodeRef> new_tree_nodes;

         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Examining " + (*stmt)->ToString());
         auto next_stmt_ptr = std::next(stmt);
         tree_nodeRef next_stmt = std::next(stmt) != list_of_stmt.end() ? *(std::next(stmt)) : tree_nodeRef();
         tree_nodeRef first_stmt = *stmt;
         tree_nodeRef second_stmt = tree_nodeRef();
         tree_nodeRef third_stmt = tree_nodeRef();
         if(!IsCondExprGimple(*stmt))
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Not a cond_expr");
            continue;
         }
         bool first_operand_of_first = true;
         bool first_operand_of_second = true;
         second_stmt = IsCondExprChain(*stmt, true, false);
         if(!second_stmt)
         {
            second_stmt = IsCondExprChain(*stmt, false, false);
            first_operand_of_first = false;
         }
         if(second_stmt)
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                           "---Chained with a second cond_expr: " + STR(second_stmt));
            third_stmt = IsCondExprChain(second_stmt, true, true);
            if(!third_stmt)
            {
               third_stmt = IsCondExprChain(second_stmt, false, true);
               first_operand_of_second = false;
            }
         }
         if(!third_stmt)
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Not chained with two cond_exprs");
            continue;
         }
         const auto first_ga = GetPointer<const gimple_assign>(*stmt);
         const auto first_ce = GetPointer<const cond_expr>(first_ga->op1);

         const auto second_ga = GetPointer<const gimple_assign>(second_stmt);
         const auto second_ce = GetPointer<const cond_expr>(second_ga->op1);

         const auto third_ga = GetPointer<const gimple_assign>(third_stmt);
         const auto third_ce = GetPointer<const cond_expr>(third_ga->op1);

         const double old_time = schedule->GetEndingTime(first_ga->index);

         /// Check if new ending time would not be larger
         /// The time in which last operand is ready
         double operand_ready_time = 0.0;

         const auto other_operand_of_first = first_operand_of_first ? first_ce->op2 : first_ce->op1;
         const auto other_operand_of_second = first_operand_of_second ? second_ce->op2 : second_ce->op1;
         CustomSet<std::pair<tree_nodeRef, tree_nodeRef>> operands;
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
            const auto sn = GetPointer<const ssa_name>(operand.first);
            if(sn)
            {
               const auto def_operand = GetPointer<const gimple_node>(sn->CGetDefStmt());
               if(def_operand->bb_index == block.first)
               {
                  const auto def_stmt = sn->CGetDefStmt();
                  const auto ending_time = schedule->GetEndingTime(def_stmt->index);
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Ending time is " + STR(ending_time));
                  const auto connection_time = allocation_information->GetConnectionTime(
                      def_stmt->index, operand.second->index, schedule->get_cstep(def_stmt->index));
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                 "---Connection time is " + STR(connection_time));
                  const auto current_operand_ready_time = ending_time + connection_time;
                  operand_ready_time = std::max(operand_ready_time, current_operand_ready_time);
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                 "---New ready time is " + STR(operand_ready_time));
               }
            }
         }

         /// As cond expr time we consider the worst among the existing operation in the chain
         const auto cond_expr_time1 = allocation_information->GetTimeLatency((*stmt)->index, fu_binding::UNKNOWN).first;
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                        "---Delay of first operation is " + STR(cond_expr_time1));
         const auto cond_expr_time2 =
             allocation_information->GetTimeLatency(second_stmt->index, fu_binding::UNKNOWN).first;
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                        "---Delay of second operation is " + STR(cond_expr_time2));
         const auto cond_expr_time3 =
             allocation_information->GetTimeLatency(third_stmt->index, fu_binding::UNKNOWN).first;
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                        "---Delay of third operation is " + STR(cond_expr_time3));
         const auto new_ending_time = operand_ready_time + std::max(cond_expr_time1, cond_expr_time3) +
                                      cond_expr_time2 +
                                      allocation_information->GetConnectionTime(third_ga->index, second_ga->index,
                                                                                schedule->get_cstep(third_ga->index));
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                        "---Operand ready time " + STR(operand_ready_time) + " - New ending time " +
                            STR(new_ending_time) + " - Old ending time " + STR(old_time));
         if(new_ending_time + EPSILON >= old_time)
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Increased execution time");
            continue;
         }
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Chained with a third cond_expr");
         THROW_ASSERT(third_stmt && second_stmt, "");
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---" + STR(third_stmt));
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---" + STR(second_stmt));
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---" + STR(*stmt));

         /// Inserting first cond expr after the last one
         const auto type_node = tree_helper::CGetType(*stmt);
         auto first_value = first_operand_of_second ? second_ce->op2 : second_ce->op1;
         auto second_value = first_operand_of_first ? first_ce->op2 : first_ce->op1;
         if(!first_operand_of_first)
         {
            std::swap(first_value, second_value);
         }
         const auto cond_expr_node = tree_man->create_ternary_operation(type_node, first_ce->op0, first_value,
                                                                        second_value, BUILTIN_SRCP, cond_expr_K);

         /// Create the ssa in the left part
         tree_nodeRef var = nullptr;
         if(first_value->get_kind() == ssa_name_K && second_value->get_kind() == ssa_name_K)
         {
            const auto sn1 = GetPointer<const ssa_name>(first_value);
            const auto sn2 = GetPointer<const ssa_name>(second_value);
            if(sn1->var && sn2->var && sn1->var->index == sn2->var->index)
            {
               var = sn1->var;
            }
         }
         const auto first_ga_op0 = GetPointer<const ssa_name>(first_ga->op0);
         const auto ssa_node = tree_man->create_ssa_name(var, type_node, first_ga_op0->min, first_ga_op0->max);
         GetPointerS<ssa_name>(ssa_node)->bit_values = first_ga_op0->bit_values;

         /// Create the assign
         const auto curr_stmt =
             tree_man->create_gimple_modify_stmt(ssa_node, cond_expr_node, function_id, BUILTIN_SRCP);
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Created " + curr_stmt->ToString());
         /// Set the bit value for the intermediate ssa to correctly update execution time
         block.second->PushBefore(curr_stmt, *stmt, AppM);
         new_tree_nodes.push_back(curr_stmt);

         tree_nodeRef and_first_cond;
         if(first_ce->op0->index == second_ce->op0->index)
         {
            /// simplified version
            if(!first_operand_of_first && !first_operand_of_second)
            {
               const auto boolType = tree_man->GetBooleanType();
               tree_nodeRef ga;
               if(max_lut_size > 0)
               {
                  const auto DefaultUnsignedLongLongInt = tree_man->GetUnsignedLongLongType();
                  const auto lut_constant_node = TM->CreateUniqueIntegerCst(1, DefaultUnsignedLongLongInt);
                  const auto new_op1 =
                      tree_man->create_lut_expr(boolType, lut_constant_node, first_ce->op0, nullptr, nullptr, nullptr,
                                                nullptr, nullptr, nullptr, nullptr, BUILTIN_SRCP);
                  ga = tree_man->CreateGimpleAssign(boolType, TM->CreateUniqueIntegerCst(0, boolType),
                                                    TM->CreateUniqueIntegerCst(1, boolType), new_op1, function_id,
                                                    BUILTIN_SRCP);
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Created LUT NOT " + STR(ga));
               }
               else
               {
                  const auto not_expr =
                      tree_man->create_unary_operation(boolType, first_ce->op0, BUILTIN_SRCP, truth_not_expr_K);
                  ga = tree_man->CreateGimpleAssign(boolType, TM->CreateUniqueIntegerCst(0, boolType),
                                                    TM->CreateUniqueIntegerCst(1, boolType), not_expr, function_id,
                                                    BUILTIN_SRCP);
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Created NOT " + STR(ga));
               }
               block.second->PushBefore(ga, *stmt, AppM);
               new_tree_nodes.push_back(ga);
               and_first_cond = GetPointerS<gimple_assign>(ga)->op0;
            }
            else if(first_operand_of_first && first_operand_of_second)
            {
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Simplified to identity");
               and_first_cond = first_ce->op0;
            }
            else
            {
               const auto boolType = tree_man->GetBooleanType();
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Simplified to 0");
               and_first_cond = TM->CreateUniqueIntegerCst(0, boolType);
            }
         }
         else
         {
            const auto boolType = tree_man->GetBooleanType();
            tree_nodeRef ga;
            if(max_lut_size > 0)
            {
               /// we are going to create a LUT
               const auto DefaultUnsignedLongLongInt = tree_man->GetUnsignedLongLongType();
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
                   tree_man->create_lut_expr(boolType, lut_constant_node, second_ce->op0, first_ce->op0, nullptr,
                                             nullptr, nullptr, nullptr, nullptr, nullptr, BUILTIN_SRCP);
               ga = tree_man->CreateGimpleAssign(boolType, TM->CreateUniqueIntegerCst(0, boolType),
                                                 TM->CreateUniqueIntegerCst(1, boolType), new_op1, function_id,
                                                 BUILTIN_SRCP);
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Created LUT STD " + STR(ga));
            }
            else
            {
               if(!first_operand_of_first && !first_operand_of_second)
               {
                  const auto not_expr0 =
                      tree_man->create_unary_operation(boolType, second_ce->op0, BUILTIN_SRCP, truth_not_expr_K);
                  const auto op0_not_expr = tree_man->CreateGimpleAssign(
                      boolType, TM->CreateUniqueIntegerCst(0, boolType), TM->CreateUniqueIntegerCst(1, boolType),
                      not_expr0, function_id, BUILTIN_SRCP);
                  block.second->PushBefore(op0_not_expr, *stmt, AppM);
                  new_tree_nodes.push_back(op0_not_expr);
                  const auto not_expr1 =
                      tree_man->create_unary_operation(boolType, first_ce->op0, BUILTIN_SRCP, truth_not_expr_K);
                  const auto op1_not_expr = tree_man->CreateGimpleAssign(
                      boolType, TM->CreateUniqueIntegerCst(0, boolType), TM->CreateUniqueIntegerCst(1, boolType),
                      not_expr1, function_id, BUILTIN_SRCP);
                  block.second->PushBefore(op1_not_expr, *stmt, AppM);
                  new_tree_nodes.push_back(op1_not_expr);
                  const auto and_expr = tree_man->create_binary_operation(
                      boolType, GetPointerS<gimple_assign>(op0_not_expr)->op0,
                      GetPointerS<gimple_assign>(op1_not_expr)->op0, BUILTIN_SRCP, truth_and_expr_K);
                  ga = tree_man->CreateGimpleAssign(boolType, TM->CreateUniqueIntegerCst(0, boolType),
                                                    TM->CreateUniqueIntegerCst(1, boolType), and_expr, function_id,
                                                    BUILTIN_SRCP);
               }
               else if(!first_operand_of_first && first_operand_of_second)
               {
                  const auto not_expr1 =
                      tree_man->create_unary_operation(boolType, first_ce->op0, BUILTIN_SRCP, truth_not_expr_K);
                  const auto op1_not_expr = tree_man->CreateGimpleAssign(
                      boolType, TM->CreateUniqueIntegerCst(0, boolType), TM->CreateUniqueIntegerCst(1, boolType),
                      not_expr1, function_id, BUILTIN_SRCP);
                  block.second->PushBefore(op1_not_expr, *stmt, AppM);
                  new_tree_nodes.push_back(op1_not_expr);
                  const auto and_expr = tree_man->create_binary_operation(boolType, second_ce->op0,
                                                                          GetPointerS<gimple_assign>(op1_not_expr)->op0,
                                                                          BUILTIN_SRCP, truth_and_expr_K);
                  ga = tree_man->CreateGimpleAssign(boolType, TM->CreateUniqueIntegerCst(0, boolType),
                                                    TM->CreateUniqueIntegerCst(1, boolType), and_expr, function_id,
                                                    BUILTIN_SRCP);
               }
               else if(first_operand_of_first && !first_operand_of_second)
               {
                  const auto not_expr0 =
                      tree_man->create_unary_operation(boolType, second_ce->op0, BUILTIN_SRCP, truth_not_expr_K);
                  const auto op0_not_expr = tree_man->CreateGimpleAssign(
                      boolType, TM->CreateUniqueIntegerCst(0, boolType), TM->CreateUniqueIntegerCst(1, boolType),
                      not_expr0, function_id, BUILTIN_SRCP);
                  block.second->PushBefore(op0_not_expr, *stmt, AppM);
                  new_tree_nodes.push_back(op0_not_expr);
                  const auto and_expr =
                      tree_man->create_binary_operation(boolType, GetPointerS<gimple_assign>(op0_not_expr)->op0,
                                                        first_ce->op0, BUILTIN_SRCP, truth_and_expr_K);
                  ga = tree_man->CreateGimpleAssign(boolType, TM->CreateUniqueIntegerCst(0, boolType),
                                                    TM->CreateUniqueIntegerCst(1, boolType), and_expr, function_id,
                                                    BUILTIN_SRCP);
               }
               else
               {
                  const auto and_expr = tree_man->create_binary_operation(boolType, second_ce->op0, first_ce->op0,
                                                                          BUILTIN_SRCP, truth_and_expr_K);
                  ga = tree_man->CreateGimpleAssign(boolType, TM->CreateUniqueIntegerCst(0, boolType),
                                                    TM->CreateUniqueIntegerCst(1, boolType), and_expr, function_id,
                                                    BUILTIN_SRCP);
               }
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Created AND " + STR(ga));
            }
            block.second->PushBefore(ga, *stmt, AppM);
            new_tree_nodes.push_back(ga);
            and_first_cond = GetPointerS<gimple_assign>(ga)->op0;
         }

         /// Inserting last cond expr
         const auto root_cond_expr = tree_man->create_ternary_operation(
             type_node, and_first_cond, first_operand_of_second ? second_ce->op1 : second_ce->op2, ssa_node,
             BUILTIN_SRCP, cond_expr_K);

         /// Create the assign
         const auto root_gimple_node =
             tree_man->create_gimple_modify_stmt(first_ga->op0, root_cond_expr, function_id, BUILTIN_SRCP);
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Created " + root_gimple_node->ToString());
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Before " + (*stmt)->ToString());
         block.second->Replace(*stmt, root_gimple_node, true, AppM);
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---After " + (*stmt)->ToString());
         new_tree_nodes.push_back(root_gimple_node);
         AppM->RegisterTransformation(GetName(), root_gimple_node);

         /// Check that the second cond expr is actually dead
         THROW_ASSERT(GetPointer<const ssa_name>(second_ga->op0)->CGetUseStmts().size() == 0, "");

         /// Remove the intermediate cond_expr
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
         const double new_time = schedule->GetEndingTime(root_gimple_node->index);
         if(new_time + EPSILON > old_time)
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Error in estimation");
            /// Removing added statements
            for(const auto& to_be_removed : new_tree_nodes)
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

bool CondExprRestructuring::IsCondExprGimple(const tree_nodeConstRef tn) const
{
   const auto ga = GetPointer<const gimple_assign>(tn);
   if(!ga)
   {
      return false;
   }
   return ga->op1->get_kind() == cond_expr_K;
}

tree_nodeRef CondExprRestructuring::IsCondExprChain(const tree_nodeConstRef tn, const bool first,
                                                    bool is_third_node) const
{
   const auto ga = GetPointer<const gimple_assign>(tn);
   const auto ce = GetPointer<const cond_expr>(ga->op1);
   const auto operand = first ? ce->op1 : ce->op2;
   const auto other_operand = first ? ce->op2 : ce->op1;
   const auto sn = GetPointer<const ssa_name>(operand);
   if(tree_helper::is_constant(TM, other_operand->index))
   {
      return tree_nodeRef();
   }
   if(!sn)
   {
      return tree_nodeRef();
   }
   if(!is_third_node && sn->CGetNumberUses() > 1)
   {
      return tree_nodeRef();
   }
   const auto def = GetPointer<const gimple_assign>(sn->CGetDefStmt());
   if(!def)
   {
      return tree_nodeRef();
   }
   if(def->bb_index != ga->bb_index)
   {
      return tree_nodeRef();
   }
   const auto chain_ce = GetPointer<const cond_expr>(def->op1);
   if(!chain_ce)
   {
      return tree_nodeRef();
   }
   if(schedule->GetStartingTime(ga->index) == schedule->GetEndingTime(def->index) or
      (schedule->get_cstep_end(def->index).second + 1) == schedule->get_cstep(ga->index).second)
   {
      return sn->CGetDefStmt();
   }
   else
   {
      return tree_nodeRef();
   }
}
