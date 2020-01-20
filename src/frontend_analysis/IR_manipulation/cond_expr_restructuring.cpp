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
 *              Copyright (C) 2015-2020 Politecnico di Milano
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
#include "hls_manager.hpp"
#include "hls_step.hpp"

/// HLS/binding/module
#include "fu_binding.hpp"

/// HLS/module_allocation
#include "allocation_information.hpp"

/// HLS/scheduling include
#include "schedule.hpp"

/// parser/treegcc include
#include "token_interface.hpp"

/// tree includes
#include "dbgPrintHelper.hpp"      // for DEBUG_LEVEL_
#include "string_manipulation.hpp" // for GET_CLASS
#include "tree_basic_block.hpp"
#include "tree_helper.hpp"
#include "tree_manager.hpp"
#include "tree_manipulation.hpp"
#include "tree_node.hpp"
#include "tree_reindex.hpp"

#define EPSILON 0.0001

const CustomUnorderedSet<std::pair<FrontendFlowStepType, FrontendFlowStep::FunctionRelationship>> CondExprRestructuring::ComputeFrontendRelationships(const DesignFlowStep::RelationshipType relationship_type) const
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
            if(parameters->getOption<HLSFlowStep_Type>(OPT_scheduling_algorithm) == HLSFlowStep_Type::SDC_SCHEDULING and GetPointer<const HLS_manager>(AppM) and GetPointer<const HLS_manager>(AppM)->get_HLS(function_id) and
               GetPointer<const HLS_manager>(AppM)->get_HLS(function_id)->Rsch)
            {
               /// If schedule is not update, do not execute this step and invalidate UpdateSchedule
               const auto update_schedule = design_flow_manager.lock()->GetDesignFlowStep(FunctionFrontendFlowStep::ComputeSignature(FrontendFlowStepType::UPDATE_SCHEDULE, function_id));
               if(update_schedule)
               {
                  const DesignFlowGraphConstRef design_flow_graph = design_flow_manager.lock()->CGetDesignFlowGraph();
                  const DesignFlowStepRef design_flow_step = design_flow_graph->CGetDesignFlowStepInfo(update_schedule)->design_flow_step;
                  if(GetPointer<const FunctionFrontendFlowStep>(design_flow_step)->CGetBBVersion() != function_behavior->GetBBVersion())
                  {
                     relationships.insert(std::pair<FrontendFlowStepType, FunctionRelationship>(UPDATE_SCHEDULE, SAME_FUNCTION));
                     break;
                  }
               }
            }
         }
         break;
      }
      case(PRECEDENCE_RELATIONSHIP):
      {
         relationships.insert(std::pair<FrontendFlowStepType, FunctionRelationship>(MULTI_WAY_IF, SAME_FUNCTION));
         relationships.insert(std::pair<FrontendFlowStepType, FunctionRelationship>(COMPLETE_BB_GRAPH, SAME_FUNCTION));
         relationships.insert(std::pair<FrontendFlowStepType, FunctionRelationship>(COMMUTATIVE_EXPR_RESTRUCTURING, SAME_FUNCTION));
         break;
      }
      default:
      {
         THROW_UNREACHABLE("");
      }
   }
   return relationships;
}

CondExprRestructuring::CondExprRestructuring(const application_managerRef _AppM, unsigned int _function_id, const DesignFlowManagerConstRef _design_flow_manager, const ParameterConstRef _parameters)
    : FunctionFrontendFlowStep(_AppM, _function_id, COND_EXPR_RESTRUCTURING, _design_flow_manager, _parameters)
{
   debug_level = parameters->get_class_debug_level(GET_CLASS(*this), DEBUG_LEVEL_NONE);
}

CondExprRestructuring::~CondExprRestructuring() = default;

bool CondExprRestructuring::IsCondExprGimple(const tree_nodeConstRef tn) const
{
   const auto ga = GetPointer<const gimple_assign>(GET_CONST_NODE(tn));
   if(not ga)
      return false;
   return GET_NODE(ga->op1)->get_kind() == cond_expr_K;
}

tree_nodeRef CondExprRestructuring::IsCondExprChain(const tree_nodeConstRef tn, const bool first) const
{
   const auto ga = GetPointer<const gimple_assign>(GET_CONST_NODE(tn));
   const auto ce = GetPointer<const cond_expr>(GET_NODE(ga->op1));
   const auto operand = first ? GET_NODE(ce->op1) : GET_NODE(ce->op2);
   const auto other_operand = first ? GET_NODE(ce->op2) : GET_NODE(ce->op1);
   const auto sn = GetPointer<const ssa_name>(operand);
   if(tree_helper::is_constant(TM, other_operand->index))
      return tree_nodeRef();
   if(not sn)
      return tree_nodeRef();
   if(sn->CGetNumberUses() > 1)
      return tree_nodeRef();
   const auto def = GetPointer<const gimple_assign>(GET_NODE(sn->CGetDefStmt()));
   if(not def)
      return tree_nodeRef();
   if(def->bb_index != ga->bb_index)
      return tree_nodeRef();
   const auto chain_ce = GetPointer<const cond_expr>(GET_NODE(def->op1));
   if(not chain_ce)
      return tree_nodeRef();
   if(schedule->GetStartingTime(ga->index) == schedule->GetEndingTime(def->index) or (schedule->get_cstep_end(def->index).second + 1) == schedule->get_cstep(ga->index).second)
      return sn->CGetDefStmt();
   else
      return tree_nodeRef();
}

DesignFlowStep_Status CondExprRestructuring::InternalExec()
{
   bool modified = false;
   static size_t counter = 0;

   const tree_manipulationConstRef tree_man = tree_manipulationConstRef(new tree_manipulation(TM, parameters));
   auto* fd = GetPointer<function_decl>(TM->get_tree_node_const(function_id));
   auto* sl = GetPointer<statement_list>(GET_NODE(fd->body));
   for(const auto& block : sl->list_of_bloc)
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Examining BB" + STR(block.first));
      const auto& list_of_stmt = block.second->CGetStmtList();
      for(auto stmt = list_of_stmt.begin(); stmt != list_of_stmt.end(); stmt++)
      {
#ifndef NDEBUG
         if(not AppM->ApplyNewTransformation())
         {
            break;
         }
#endif

         std::list<tree_nodeRef> new_tree_nodes;

         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Examining " + (*stmt)->ToString());
         auto next_stmt_ptr = std::next(stmt);
         tree_nodeRef next_stmt = std::next(stmt) != list_of_stmt.end() ? *(std::next(stmt)) : tree_nodeRef();
         tree_nodeRef first_stmt = *stmt;
         tree_nodeRef second_stmt = tree_nodeRef();
         tree_nodeRef third_stmt = tree_nodeRef();
         if(not IsCondExprGimple(*stmt))
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Not a cond_expr");
            continue;
         }
         bool first_operand_of_first = true;
         bool first_operand_of_second = true;
         second_stmt = IsCondExprChain(*stmt, true);
         if(not second_stmt)
         {
            second_stmt = IsCondExprChain(*stmt, false);
            first_operand_of_first = false;
         }
         if(second_stmt)
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Chained with a second cond_expr: " + STR(second_stmt));
            third_stmt = IsCondExprChain(second_stmt, true);
            if(not third_stmt)
            {
               third_stmt = IsCondExprChain(second_stmt, false);
               first_operand_of_second = false;
            }
         }
         if(not third_stmt)
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Not chained with two cond_exprs");
            continue;
         }
         const auto first_ga = GetPointer<const gimple_assign>(GET_NODE(*stmt));
         const auto first_ce = GetPointer<const cond_expr>(GET_NODE(first_ga->op1));

         const auto second_ga = GetPointer<const gimple_assign>(GET_NODE(second_stmt));
         const auto second_ce = GetPointer<const cond_expr>(GET_NODE(second_ga->op1));

         const auto third_ga = GetPointer<const gimple_assign>(GET_NODE(third_stmt));
         const auto third_ce = GetPointer<const cond_expr>(GET_NODE(third_ga->op1));

         const double old_time = schedule->GetEndingTime(first_ga->index);

         /// Check if new ending time would not be larger
         /// The time in which last operand is ready
         double operand_ready_time = 0.0;

         const auto other_operand_of_first = first_operand_of_first ? first_ce->op2 : first_ce->op1;
         const auto other_operand_of_second = first_operand_of_second ? second_ce->op2 : second_ce->op1;
         CustomSet<std::pair<tree_nodeRef, tree_nodeRef>> operands;
         operands.insert(std::pair<tree_nodeRef, tree_nodeRef>(other_operand_of_first, *stmt));
         operands.insert(std::pair<tree_nodeRef, tree_nodeRef>(other_operand_of_second, second_stmt));
         operands.insert(std::pair<tree_nodeRef, tree_nodeRef>(third_ce->op1, third_stmt));
         operands.insert(std::pair<tree_nodeRef, tree_nodeRef>(third_ce->op2, third_stmt));
         operands.insert(std::pair<tree_nodeRef, tree_nodeRef>(first_ce->op0, *stmt));
         operands.insert(std::pair<tree_nodeRef, tree_nodeRef>(second_ce->op0, second_stmt));
         operands.insert(std::pair<tree_nodeRef, tree_nodeRef>(third_ce->op0, third_stmt));
         for(const auto& operand : operands)
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Analyzing when " + STR(operand.first) + " used in " + STR(operand.second) + " is ready");
            const auto sn = GetPointer<const ssa_name>(GET_NODE(operand.first));
            if(sn)
            {
               const auto def_operand = GetPointer<const gimple_node>(GET_NODE(sn->CGetDefStmt()));
               if(def_operand->bb_index == block.first)
               {
                  const auto def_stmt = sn->CGetDefStmt();
                  const auto ending_time = schedule->GetEndingTime(def_stmt->index);
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Ending time is " + STR(ending_time));
                  const auto connection_time = allocation_information->GetConnectionTime(def_stmt->index, operand.second->index, schedule->get_cstep(def_stmt->index));
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Connection time is " + STR(connection_time));
                  const auto current_operand_ready_time = ending_time + connection_time;
                  operand_ready_time = std::max(operand_ready_time, current_operand_ready_time);
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---New ready time is " + STR(operand_ready_time));
               }
            }
         }

         /// As cond expr time we consider the worst among the existing operation in the chain
         const auto cond_expr_time1 = allocation_information->GetTimeLatency((*stmt)->index, fu_binding::UNKNOWN).first;
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Delay of first operation is " + STR(cond_expr_time1));
         const auto cond_expr_time2 = allocation_information->GetTimeLatency(second_stmt->index, fu_binding::UNKNOWN).first;
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Delay of second operation is " + STR(cond_expr_time2));
         const auto cond_expr_time3 = allocation_information->GetTimeLatency(third_stmt->index, fu_binding::UNKNOWN).first;
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Delay of third operation is " + STR(cond_expr_time3));
         const auto mux_time = std::max(cond_expr_time1, std::max(cond_expr_time2, cond_expr_time3));
         const auto new_ending_time = operand_ready_time + 2 * mux_time;
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Operand ready time " + STR(operand_ready_time) + " - Mux time " + STR(mux_time) + " - New ending time " + STR(new_ending_time) + " - Old ending time " + STR(old_time));
         if(new_ending_time + EPSILON >= old_time)
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Increased execution time");
            continue;
         }
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Chained with a third cond_expr");
         THROW_ASSERT(third_stmt and second_stmt, "");
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---" + STR(third_stmt));
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---" + STR(second_stmt));
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---" + STR(*stmt));

         /// Inserting first cond expr after the last one
         std::map<TreeVocabularyTokenTypes_TokenEnum, std::string> cond_expr_schema, gimple_assign_schema, ssa_schema;
         const auto type_index = tree_helper::get_type_index(TM, (*stmt)->index);
         const auto cond_expr_id = TM->new_tree_node_id();
         auto first_value = first_operand_of_second ? second_ce->op2 : second_ce->op1;
         auto second_value = first_operand_of_first ? first_ce->op2 : first_ce->op1;
         if(not first_operand_of_first)
            std::swap(first_value, second_value);
         cond_expr_schema[TOK(TOK_SRCP)] = "<built-in>:0:0";
         cond_expr_schema[TOK(TOK_TYPE)] = boost::lexical_cast<std::string>(type_index);
         cond_expr_schema[TOK(TOK_OP0)] = boost::lexical_cast<std::string>(first_ce->op0->index);
         cond_expr_schema[TOK(TOK_OP1)] = boost::lexical_cast<std::string>(first_value->index);
         cond_expr_schema[TOK(TOK_OP2)] = boost::lexical_cast<std::string>(second_value->index);
         TM->create_tree_node(cond_expr_id, cond_expr_K, cond_expr_schema);

         /// Create the ssa in the left part
         auto ssa_vers = TM->get_next_vers();
         auto ssa_node_nid = TM->new_tree_node_id();
         ssa_schema[TOK(TOK_TYPE)] = STR(type_index);
         ssa_schema[TOK(TOK_VERS)] = STR(ssa_vers);
         ssa_schema[TOK(TOK_VOLATILE)] = STR(false);
         ssa_schema[TOK(TOK_VIRTUAL)] = STR(false);
         if(GET_NODE(first_value)->get_kind() == ssa_name_K and GET_NODE(second_value)->get_kind() == ssa_name_K)
         {
            const auto sn1 = GetPointer<const ssa_name>(GET_NODE(first_value));
            const auto sn2 = GetPointer<const ssa_name>(GET_NODE(second_value));
            if(sn1->var and sn2->var and sn1->var->index == sn2->var->index)
            {
               ssa_schema[TOK(TOK_VAR)] = STR(sn1->var->index);
            }
         }
         TM->create_tree_node(ssa_node_nid, ssa_name_K, ssa_schema);

         /// Create the assign
         const auto gimple_node_id = TM->new_tree_node_id();
         gimple_assign_schema[TOK(TOK_SRCP)] = "<built-in>:0:0";
         gimple_assign_schema[TOK(TOK_TYPE)] = STR(type_index);
         gimple_assign_schema[TOK(TOK_OP0)] = STR(ssa_node_nid);
         gimple_assign_schema[TOK(TOK_OP1)] = STR(cond_expr_id);
         TM->create_tree_node(gimple_node_id, gimple_assign_K, gimple_assign_schema);
         auto curr_stmt = TM->GetTreeReindex(gimple_node_id);
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Created " + STR(curr_stmt));
         /// Set the bit value for the intermediate ssa to correctly update execution time
         GetPointer<ssa_name>(TM->get_tree_node_const(ssa_node_nid))->bit_values = GetPointer<ssa_name>(GET_NODE(first_ga->op0))->bit_values;
         block.second->PushBefore(curr_stmt, *stmt);
         new_tree_nodes.push_back(curr_stmt);

         tree_nodeRef and_first_cond;
         if(GET_INDEX_NODE(first_ce->op0) == GET_INDEX_NODE(second_ce->op0))
         {
            /// simplified version
            if(!first_operand_of_first && !first_operand_of_second)
            {
               tree_nodeRef lut_constant_node;
               auto DefaultUnsignedLongLongInt = tree_man->CreateDefaultUnsignedLongLongInt();
               lut_constant_node = TM->CreateUniqueIntegerCst(1, GET_INDEX_NODE(DefaultUnsignedLongLongInt));
               auto boolType = tree_man->create_boolean_type();
               tree_nodeRef op2, op3, op4, op5, op6, op7, op8;
               const std::string srcp_default = "<built-in>:0:0";
               tree_nodeRef new_op1 = tree_man->create_lut_expr(boolType, lut_constant_node, first_ce->op0, op2, op3, op4, op5, op6, op7, op8, srcp_default);
               auto lut_ga = tree_man->CreateGimpleAssign(boolType, TM->CreateUniqueIntegerCst(0, boolType->index), TM->CreateUniqueIntegerCst(1, boolType->index), new_op1, block.first, srcp_default);
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Created LUT NOT " + STR(lut_ga));
               block.second->PushBefore(lut_ga, *stmt);
               new_tree_nodes.push_back(lut_ga);
               and_first_cond = GetPointer<gimple_assign>(GET_NODE(lut_ga))->op0;
            }
            else if(first_operand_of_first && first_operand_of_second)
            {
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Simplified to identity");
               and_first_cond = first_ce->op0;
            }
            else
            {
               auto boolType = tree_man->create_boolean_type();
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Simplified to 0");
               and_first_cond = TM->CreateUniqueIntegerCst(0, boolType->index);
            }
         }
         else
         {
            /// we are going to create a LUT
            tree_nodeRef lut_constant_node;
            auto DefaultUnsignedLongLongInt = tree_man->CreateDefaultUnsignedLongLongInt();
            long long int lut_val;
            if(!first_operand_of_first && !first_operand_of_second)
               lut_val = 1;
            else if(!first_operand_of_first && first_operand_of_second)
               lut_val = 2;
            else if(first_operand_of_first && !first_operand_of_second)
               lut_val = 4;
            else
               lut_val = 8;
            lut_constant_node = TM->CreateUniqueIntegerCst(lut_val, GET_INDEX_NODE(DefaultUnsignedLongLongInt));
            auto boolType = tree_man->create_boolean_type();
            tree_nodeRef op3, op4, op5, op6, op7, op8;
            const std::string srcp_default = "<built-in>:0:0";
            tree_nodeRef new_op1 = tree_man->create_lut_expr(boolType, lut_constant_node, second_ce->op0, first_ce->op0, op3, op4, op5, op6, op7, op8, srcp_default);
            auto lut_ga = tree_man->CreateGimpleAssign(boolType, TM->CreateUniqueIntegerCst(0, boolType->index), TM->CreateUniqueIntegerCst(1, boolType->index), new_op1, block.first, srcp_default);
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Created LUT STD " + STR(lut_ga));
            block.second->PushBefore(lut_ga, *stmt);
            new_tree_nodes.push_back(lut_ga);
            and_first_cond = GetPointer<gimple_assign>(GET_NODE(lut_ga))->op0;
         }

         /// Inserting last cond expr
         cond_expr_schema.clear();
         const auto root_cond_expr_id = TM->new_tree_node_id();
         cond_expr_schema[TOK(TOK_SRCP)] = "<built-in>:0:0";
         cond_expr_schema[TOK(TOK_TYPE)] = boost::lexical_cast<std::string>(type_index);
         cond_expr_schema[TOK(TOK_OP0)] = boost::lexical_cast<std::string>(and_first_cond->index);
         cond_expr_schema[TOK(TOK_OP1)] = boost::lexical_cast<std::string>(first_operand_of_second ? second_ce->op1->index : second_ce->op2->index);
         cond_expr_schema[TOK(TOK_OP2)] = boost::lexical_cast<std::string>(ssa_node_nid);
         TM->create_tree_node(root_cond_expr_id, cond_expr_K, cond_expr_schema);

         /// Create the assign
         const auto root_gimple_node_id = TM->new_tree_node_id();
         gimple_assign_schema[TOK(TOK_SRCP)] = "<built-in>:0:0";
         gimple_assign_schema[TOK(TOK_TYPE)] = STR(type_index);
         gimple_assign_schema[TOK(TOK_OP0)] = STR(first_ga->op0->index);
         gimple_assign_schema[TOK(TOK_OP1)] = STR(root_cond_expr_id);
         TM->create_tree_node(root_gimple_node_id, gimple_assign_K, gimple_assign_schema);
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Created " + TM->CGetTreeNode(root_gimple_node_id)->ToString());
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Before " + (*stmt)->ToString());
         block.second->Replace(*stmt, TM->GetTreeReindex(root_gimple_node_id), true);
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---After " + (*stmt)->ToString());
         new_tree_nodes.push_back(TM->GetTreeReindex(root_gimple_node_id));
#ifndef NDEBUG
         AppM->RegisterTransformation(GetName(), TM->CGetTreeNode(root_gimple_node_id));
#endif

         /// Check that the second cond expr is actually dead
         THROW_ASSERT(GetPointer<const ssa_name>(GET_CONST_NODE(second_ga->op0))->CGetUseStmts().size() == 0, "");

         /// Remove the intermediate cond_expr
         block.second->RemoveStmt(second_stmt);

         for(const auto& temp_stmt : list_of_stmt)
         {
            schedule->UpdateTime(temp_stmt->index);
         }

         if(debug_level >= DEBUG_LEVEL_VERY_PEDANTIC)
         {
            WriteBBGraphDot("BB_Inside_" + GetName() + "_" + STR(counter) + ".dot");
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Written BB_Inside_" + GetName() + "_" + STR(counter) + ".dot");
            counter++;
         }
         const double new_time = schedule->GetEndingTime(root_gimple_node_id);
         if(new_time + EPSILON > old_time)
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Error in estimation");
            /// Removing added statements
            for(const auto& to_be_removed : new_tree_nodes)
            {
               block.second->RemoveStmt(to_be_removed);
            }

            /// Adding old statements
            next_stmt ? block.second->PushBefore(second_stmt, next_stmt) : block.second->PushBack(second_stmt);
            next_stmt ? block.second->PushBefore(first_stmt, next_stmt) : block.second->PushBack(first_stmt);

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

void CondExprRestructuring::Initialize()
{
   FunctionFrontendFlowStep::Initialize();
   TM = AppM->get_tree_manager();
   if(GetPointer<HLS_manager>(AppM) and GetPointer<HLS_manager>(AppM)->get_HLS(function_id))
   {
      schedule = GetPointer<HLS_manager>(AppM)->get_HLS(function_id)->Rsch;
      allocation_information = GetPointer<HLS_manager>(AppM)->get_HLS(function_id)->allocation_information;
   }
}

bool CondExprRestructuring::HasToBeExecuted() const
{
   if(!FunctionFrontendFlowStep::HasToBeExecuted())
      return false;
#if HAVE_ILP_BUILT
   if(parameters->getOption<HLSFlowStep_Type>(OPT_scheduling_algorithm) == HLSFlowStep_Type::SDC_SCHEDULING and GetPointer<const HLS_manager>(AppM) and GetPointer<const HLS_manager>(AppM)->get_HLS(function_id) and
      GetPointer<const HLS_manager>(AppM)->get_HLS(function_id)->Rsch)
   {
      /// If schedule is not update, do not execute this step and invalidate UpdateSchedule
      const auto update_schedule = design_flow_manager.lock()->GetDesignFlowStep(FunctionFrontendFlowStep::ComputeSignature(FrontendFlowStepType::UPDATE_SCHEDULE, function_id));
      if(update_schedule)
      {
         const DesignFlowGraphConstRef design_flow_graph = design_flow_manager.lock()->CGetDesignFlowGraph();
         const DesignFlowStepRef design_flow_step = design_flow_graph->CGetDesignFlowStepInfo(update_schedule)->design_flow_step;
         if(GetPointer<const FunctionFrontendFlowStep>(design_flow_step)->CGetBBVersion() != function_behavior->GetBBVersion())
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
