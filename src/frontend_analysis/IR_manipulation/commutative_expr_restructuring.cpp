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
 *              Copyright (C) 2018-2024 Politecnico di Milano
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
 * @file commutative_expr_restructuring.cpp
 * @brief Analysis step restructuring tree of commutative expressions to reduce the critical path delay.
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 *
 */

/// Header include
#include "commutative_expr_restructuring.hpp"

///. include
#include "Parameter.hpp"

/// behavior include
#include "application_manager.hpp"

/// design_flows includes
#include "design_flow_graph.hpp"
#include "design_flow_manager.hpp"

/// HLS includes
#include "hls.hpp"
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
               const auto update_schedule = design_flow_manager.lock()->GetDesignFlowStep(
                   FunctionFrontendFlowStep::ComputeSignature(FrontendFlowStepType::UPDATE_SCHEDULE, function_id));
               if(update_schedule != DesignFlowGraph::null_vertex())
               {
                  const DesignFlowGraphConstRef design_flow_graph = design_flow_manager.lock()->CGetDesignFlowGraph();
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

commutative_expr_restructuring::commutative_expr_restructuring(const application_managerRef _AppM,
                                                               unsigned int _function_id,
                                                               const DesignFlowManagerConstRef _design_flow_manager,
                                                               const ParameterConstRef _parameters)
    : FunctionFrontendFlowStep(_AppM, _function_id, COMMUTATIVE_EXPR_RESTRUCTURING, _design_flow_manager, _parameters)
{
   debug_level = parameters->get_class_debug_level(GET_CLASS(*this), DEBUG_LEVEL_NONE);
}

commutative_expr_restructuring::~commutative_expr_restructuring() = default;

bool commutative_expr_restructuring::IsCommExprGimple(const tree_nodeConstRef tn) const
{
   const auto ga = GetPointer<const gimple_assign>(tn);
   if(!ga)
   {
      return false;
   }
   auto opKind = ga->op1->get_kind();
   auto Type = tree_helper::CGetType(ga->op0);
   if(!GetPointer<const integer_type>(Type))
   {
      return false;
   }
   return opKind == mult_expr_K || opKind == widen_mult_expr_K || opKind == widen_sum_expr_K ||
          opKind == bit_ior_expr_K || opKind == bit_xor_expr_K || opKind == bit_and_expr_K || opKind == eq_expr_K ||
          opKind == ne_expr_K || opKind == plus_expr_K;
}

tree_nodeRef commutative_expr_restructuring::IsCommExprChain(const tree_nodeConstRef tn, const bool first,
                                                             bool is_third_node) const
{
   const auto ga = GetPointer<const gimple_assign>(tn);
   const auto be = GetPointer<const binary_expr>(ga->op1);
   const auto operand = first ? be->op0 : be->op1;
   const auto other_operand = first ? be->op1 : be->op0;
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
   if(def->op1->get_kind() != ga->op1->get_kind())
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

DesignFlowStep_Status commutative_expr_restructuring::InternalExec()
{
   bool modified = false;
   static size_t counter = 0;

   const tree_manipulationConstRef tree_man = tree_manipulationConstRef(new tree_manipulation(TM, parameters, AppM));
   auto* fd = GetPointer<function_decl>(TM->GetTreeNode(function_id));
   auto* sl = GetPointer<statement_list>(fd->body);
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
         if(!IsCommExprGimple(*stmt))
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
         const auto first_ga = GetPointer<const gimple_assign>(*stmt);
         auto comm_expr_kind = first_ga->op1->get_kind();
         auto comm_expr_kind_text = first_ga->op1->get_kind_text();
         const auto first_be = GetPointer<const binary_expr>(first_ga->op1);

         const auto second_ga = GetPointer<const gimple_assign>(second_stmt);
         THROW_ASSERT(second_ga->op1->get_kind() == comm_expr_kind, "unexpected condition");
         const auto second_be = GetPointer<const binary_expr>(second_ga->op1);

         const auto third_ga = GetPointer<const gimple_assign>(third_stmt);
         THROW_ASSERT(third_ga->op1->get_kind() == comm_expr_kind, "unexpected condition");
         const auto third_be = GetPointer<const binary_expr>(third_ga->op1);

         const double old_time = schedule->GetEndingTime(first_ga->index);

         /// Check if new ending time would not be larger
         /// The time in which last operand is ready
         double operand_ready_time = 0.0;

         const auto other_operand_of_first = first_operand_of_first ? first_be->op1 : first_be->op0;
         const auto other_operand_of_second = first_operand_of_second ? second_be->op1 : second_be->op0;
         CustomSet<std::pair<tree_nodeRef, tree_nodeRef>> operands;
         operands.insert(std::make_pair(other_operand_of_first, *stmt));
         operands.insert(std::make_pair(other_operand_of_second, second_stmt));
         operands.insert(std::make_pair(third_be->op0, third_stmt));
         operands.insert(std::make_pair(third_be->op1, third_stmt));
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
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Chained with a third commutative expression");
         THROW_ASSERT(third_stmt && second_stmt, "");
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---" + STR(third_stmt));
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---" + STR(second_stmt));
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---" + STR(*stmt));

         /// Inserting first commutative expression after the last one
         const auto type_node = tree_helper::CGetType(*stmt);
         auto first_value = first_operand_of_second ? second_be->op1 : second_be->op0;
         auto second_value = first_operand_of_first ? first_be->op1 : first_be->op0;
         if(!first_operand_of_first)
         {
            std::swap(first_value, second_value);
         }
         const auto comm_expr_node =
             tree_man->create_binary_operation(type_node, first_value, second_value, BUILTIN_SRCP, comm_expr_kind);

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
         const auto first_ga_op0 = GetPointer<ssa_name>(first_ga->op0);
         const auto ssa_node = tree_man->create_ssa_name(var, type_node, first_ga_op0->min, first_ga_op0->max);
         GetPointer<ssa_name>(ssa_node)->bit_values = first_ga_op0->bit_values;

         /// Create the assign
         const auto gimple_assign_node =
             tree_man->create_gimple_modify_stmt(ssa_node, comm_expr_node, function_id, BUILTIN_SRCP);
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Created " + gimple_assign_node->ToString());
         /// Set the bit value for the intermediate ssa to correctly update execution time
         block.second->PushBefore(gimple_assign_node, *stmt, AppM);
         new_tree_nodes.push_back(gimple_assign_node);

         /// Inserting last commutative expression
         const auto root_comm_expr =
             tree_man->create_binary_operation(type_node, first_operand_of_second ? second_be->op0 : second_be->op1,
                                               ssa_node, BUILTIN_SRCP, comm_expr_kind);

         /// Create the assign
         const auto root_gimple_node =
             tree_man->create_gimple_modify_stmt(first_ga->op0, root_comm_expr, function_id, BUILTIN_SRCP);
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Created " + root_gimple_node->ToString());
         block.second->Replace(*stmt, root_gimple_node, true, AppM);
         new_tree_nodes.push_back(root_gimple_node);
         AppM->RegisterTransformation(GetName(), root_gimple_node);

         /// Check that the second commutative expression is actually dead
         THROW_ASSERT(GetPointer<const ssa_name>(second_ga->op0)->CGetUseStmts().size() == 0, "");

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
   TM = AppM->get_tree_manager();
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
#if HAVE_ILP_BUILT
   if(parameters->getOption<HLSFlowStep_Type>(OPT_scheduling_algorithm) == HLSFlowStep_Type::SDC_SCHEDULING &&
      GetPointer<const HLS_manager>(AppM) && GetPointer<const HLS_manager>(AppM)->get_HLS(function_id) and
      GetPointer<const HLS_manager>(AppM)->get_HLS(function_id)->Rsch)
   {
      /// If schedule is not up to date, do not execute this step and invalidate UpdateSchedule
      const auto update_schedule = design_flow_manager.lock()->GetDesignFlowStep(
          FunctionFrontendFlowStep::ComputeSignature(FrontendFlowStepType::UPDATE_SCHEDULE, function_id));
      if(update_schedule != DesignFlowGraph::null_vertex())
      {
         const DesignFlowGraphConstRef design_flow_graph = design_flow_manager.lock()->CGetDesignFlowGraph();
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
#endif
   {
      return false;
   }
}
