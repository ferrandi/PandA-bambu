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
 * @file ir2fun.cpp
 * @brief Step that replace some IR node expression with function call
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 *
 */
#include "ir2fun.hpp"

#include "Parameter.hpp"
#include "application_manager.hpp"
#include "basic_block.hpp"
#include "behavioral_helper.hpp"
#include "call_graph.hpp"
#include "call_graph_manager.hpp"
#include "dbgPrintHelper.hpp"
#include "design_flow_graph.hpp"
#include "design_flow_manager.hpp"
#include "design_flow_step.hpp"
#include "exceptions.hpp"
#include "function_behavior.hpp"
#include "hls_manager.hpp"
#include "ir_basic_block.hpp"
#include "ir_helper.hpp"
#include "ir_manager.hpp"
#include "ir_manipulation.hpp"
#include "ir_node.hpp"
#include "math_function.hpp"
#include "string_manipulation.hpp"

ir2fun::ir2fun(const ParameterConstRef _parameters, const application_managerRef _AppM, unsigned int _function_id,
               const DesignFlowManager& _design_flow_manager)
    : FunctionFrontendFlowStep(_AppM, _function_id, IR2FUN, _design_flow_manager, _parameters),
      IRM(_AppM->get_ir_manager())
{
   debug_level = parameters->get_class_debug_level(GET_CLASS(*this), DEBUG_LEVEL_NONE);
}

CustomUnorderedSet<std::pair<FrontendFlowStepType, FrontendFlowStep::FunctionRelationship>>
ir2fun::ComputeFrontendRelationships(const DesignFlowStep::RelationshipType relationship_type) const
{
   CustomUnorderedSet<std::pair<FrontendFlowStepType, FunctionRelationship>> relationships;
   switch(relationship_type)
   {
      case(DEPENDENCE_RELATIONSHIP):
      {
         relationships.insert(std::make_pair(FUNCTION_CALL_TYPE_CLEANUP, SAME_FUNCTION));
         relationships.insert(std::make_pair(USE_COUNTING, SAME_FUNCTION));
         break;
      }
      case(PRECEDENCE_RELATIONSHIP):
      {
         break;
      }
      case(INVALIDATION_RELATIONSHIP):
      {
         if(GetStatus() == DesignFlowStep_Status::SUCCESS)
         {
            relationships.insert(std::make_pair(FUNCTION_CALL_TYPE_CLEANUP, SAME_FUNCTION));
         }
         break;
      }
      default:
      {
         THROW_UNREACHABLE("");
      }
   }
   return relationships;
}

DesignFlowStep_Status ir2fun::InternalExec()
{
   const ir_manipulationRef ir_man(new ir_manipulation(IRM, parameters, AppM));

   const auto curr_tn = IRM->GetIRNode(function_id);
   const auto fname = ir_helper::GetFunctionName(curr_tn);
   const auto fd = GetPointerS<function_val_node>(curr_tn);
   const auto sl = GetPointerS<statement_list_node>(fd->body);

   bool modified = false;
   for(const auto& idx_bb : sl->list_of_bloc)
   {
      const auto& BB = idx_bb.second;
      for(const auto& stmt : BB->CGetStmtList())
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                        "-->Examine " + STR(stmt->index) + " " + stmt->ToString());
         modified |= recursive_transform(stmt, stmt, ir_man);
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                        "<--Examined " + STR(stmt->index) + " " + stmt->ToString());
      }
   }

   if(modified)
   {
      function_behavior->UpdateBBVersion();
      return DesignFlowStep_Status::SUCCESS;
   }
   return DesignFlowStep_Status::UNCHANGED;
}

bool ir2fun::recursive_transform(const ir_nodeRef& curr_tn, const ir_nodeRef& current_statement,
                                 const ir_manipulationRef ir_man)
{
   bool modified = false;
   const auto get_current_locinfo = [curr_tn]() -> std::string {
      const auto loc_info_tn = GetPointer<const IR_LocInfo>(curr_tn);
      if(loc_info_tn)
      {
         return loc_info_tn->include_name + ":" + STR(loc_info_tn->line_number) + ":" + STR(loc_info_tn->column_number);
      }
      return "";
   };
   switch(curr_tn->get_kind())
   {
      case call_node_K:
      {
         break;
      }
      case assign_stmt_K:
      {
         const auto gm = GetPointerS<assign_stmt>(curr_tn);
         modified |= recursive_transform(gm->op0, current_statement, ir_man);
         modified |= recursive_transform(gm->op1, current_statement, ir_man);
         break;
      }
      case CASE_UNARY_NODES:
      {
         const auto ue = GetPointerS<unary_node>(curr_tn);
         modified |= recursive_transform(ue->op, current_statement, ir_man);
         break;
      }
      case CASE_BINARY_NODES:
      {
         const auto be = GetPointerS<binary_node>(curr_tn);
         const auto be_type = be->get_kind();
         modified |= recursive_transform(be->op0, current_statement, ir_man);
         modified |= recursive_transform(be->op1, current_statement, ir_man);
         if(be_type == frem_node_K && be->type->get_kind() == real_ty_node_K)
         {
            const auto expr_type = be->type;
            THROW_ASSERT(expr_type->get_kind() == real_ty_node_K, "unexpected case");
            const auto bitsize = ir_helper::Size(expr_type);
            const std::string fu_name = bitsize == 32 ? "fmodf" : "fmod";
            const auto called_function = IRM->GetFunction(fu_name);
            THROW_ASSERT(called_function, "Add option -lm to the command line for frem/fmod");
            THROW_ASSERT(ir_helper::IsFunctionImplemented(called_function), "inconsistent behavioral helper");
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Adding call to " + fu_name);
            const std::vector<ir_nodeRef> args = {be->op0, be->op1};
            const auto ce = ir_man->CreateCallExpr(called_function, args, get_current_locinfo());
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Replaced " + STR(current_statement));
            IRM->ReplaceIRNode(current_statement, curr_tn, ce);
            CallGraphManager::addCallPointAndExpand(already_visited, AppM, function_id, called_function->index,
                                                    current_statement->index, FunctionEdgeInfo::CallType::direct_call,
                                                    DEBUG_LEVEL_NONE);
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---      -> " + STR(current_statement));
            modified = true;
         }
         break;
      }
      case CASE_TERNARY_NODES:
      {
         const auto te = GetPointerS<ternary_node>(curr_tn);
         modified |= recursive_transform(te->op0, current_statement, ir_man);
         if(te->op1)
         {
            modified |= recursive_transform(te->op1, current_statement, ir_man);
         }
         if(te->op2)
         {
            modified |= recursive_transform(te->op2, current_statement, ir_man);
         }
         break;
      }
      case constructor_node_K:
      {
         const auto co = GetPointerS<constructor_node>(curr_tn);
         for(const auto& iv : co->list_of_idx_valu)
         {
            modified |= recursive_transform(iv.second, current_statement, ir_man);
         }
         break;
      }
      case call_stmt_K:
      {
         break;
      }
      case nop_stmt_K:
      case variable_val_node_K:
      case argument_val_node_K:
      case ssa_node_K:
      case lut_node_K:
      case multi_way_if_stmt_K:
      case return_stmt_K:
      case CASE_TYPE_NODES:
      case constant_fp_val_node_K:
      case constant_int_val_node_K:
      case field_val_node_K:
      case function_val_node_K:
      case constant_vector_val_node_K:
         break;
      case phi_stmt_K:
      case identifier_node_K:
      case last_ir_K:
      case statement_list_node_K:
      case module_unit_node_K:
      case ir_reindex_K:
      {
         THROW_ERROR_CODE(NODE_NOT_YET_SUPPORTED_EC, "Not supported node: " + curr_tn->get_kind_text());
         break;
      }
      default:
      {
         THROW_UNREACHABLE("");
      }
   }
   return modified;
}
