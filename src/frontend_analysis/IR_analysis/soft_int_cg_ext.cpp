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
 * @file soft_int_cg_ext.cpp
 * @brief Step that extends the call graph with software implementation of integer operators.
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 *
 */

#include "soft_int_cg_ext.hpp"

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
#include "function_frontend_flow_step.hpp"
#include "hls_manager.hpp"
#include "ir_basic_block.hpp"
#include "ir_helper.hpp"
#include "ir_manager.hpp"
#include "ir_manipulation.hpp"
#include "ir_node.hpp"
#include "math_function.hpp"
#include "string_manipulation.hpp"

soft_int_cg_ext::soft_int_cg_ext(const application_managerRef AM, const DesignFlowManager& dfm,
                                 const ParameterConstRef par)
    : ApplicationFrontendFlowStep(AM, SOFT_INT_CG_EXT, dfm, par),
      IRM(AM->get_ir_manager()),
      doSoftDiv(par->isOption(OPT_hls_div) && par->getOption<std::string>(OPT_hls_div) != "none")
{
   debug_level = parameters->get_class_debug_level(GET_CLASS(*this), DEBUG_LEVEL_NONE);
}

CustomUnorderedSet<std::pair<FrontendFlowStepType, FrontendFlowStep::FunctionRelationship>>
soft_int_cg_ext::ComputeFrontendRelationships(const DesignFlowStep::RelationshipType relationship_type) const
{
   CustomUnorderedSet<std::pair<FrontendFlowStepType, FunctionRelationship>> relationships;
   switch(relationship_type)
   {
      case(DEPENDENCE_RELATIONSHIP):
      {
         relationships.insert(std::make_pair(FUNCTION_CALL_TYPE_CLEANUP, ALL_FUNCTIONS));
         relationships.insert(std::make_pair(USE_COUNTING, ALL_FUNCTIONS));
         relationships.insert(std::make_pair(CSE_STEP, ALL_FUNCTIONS));
         relationships.insert(std::make_pair(SOFT_FLOAT_CG_EXT, ALL_FUNCTIONS));
         break;
      }
      case(PRECEDENCE_RELATIONSHIP):
      {
         relationships.insert(std::make_pair(DEAD_CODE_ELIMINATION_IPA, WHOLE_APPLICATION));
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

void soft_int_cg_ext::ComputeRelationships(DesignFlowStepSet& relationships,
                                           const DesignFlowStep::RelationshipType relationship_type)
{
   if(relationship_type == INVALIDATION_RELATIONSHIP)
   {
      const auto DFG = design_flow_manager.CGetDesignFlowGraph();
      for(const auto& i : fun_id_to_restart)
      {
         const auto step_signature =
             FunctionFrontendFlowStep::ComputeSignature(FrontendFlowStepType::FUNCTION_CALL_TYPE_CLEANUP, i);
         const auto frontend_step = design_flow_manager.GetDesignFlowStep(step_signature);
         THROW_ASSERT(frontend_step != DesignFlowGraph::null_vertex(), "step is not present");
         const auto design_flow_step = DFG->CGetNodeInfo(frontend_step)->design_flow_step;
         relationships.insert(design_flow_step);
      }
      fun_id_to_restart.clear();
   }
   ApplicationFrontendFlowStep::ComputeRelationships(relationships, relationship_type);
}

DesignFlowStep_Status soft_int_cg_ext::Exec()
{
   if(!doSoftDiv)
   {
      return DesignFlowStep_Status::UNCHANGED;
   }
   const ir_manipulationRef ir_man(new ir_manipulation(IRM, parameters, AppM));

   const auto& CGM = AppM->CGetCallGraphManager();
   const auto reached_body_fun_ids = CGM.GetReachedBodyFunctions();

   for(const auto& function_id : reached_body_fun_ids)
   {
      const auto curr_tn = IRM->GetIRNode(function_id);
      const auto fname = ir_helper::GetFunctionName(curr_tn);
      INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level,
                     "-->Analyzing function \"" + fname + "\": id = " + STR(function_id));
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
            modified |= recursive_transform(function_id, stmt, stmt, ir_man);
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                           "<--Examined " + STR(stmt->index) + " " + stmt->ToString());
         }
      }

      if(modified)
      {
         fun_id_to_restart.insert(function_id);
      }
      INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level,
                     "<--Analyzed function \"" + fname + "\": id = " + STR(function_id));
   }
   for(const auto& i : fun_id_to_restart)
   {
      const auto FB = AppM->GetFunctionBehavior(i);
      FB->UpdateBBVersion();
   }
   return fun_id_to_restart.empty() ? DesignFlowStep_Status::UNCHANGED : DesignFlowStep_Status::SUCCESS;
}

bool soft_int_cg_ext::recursive_transform(unsigned int function_id, const ir_nodeRef& curr_tn,
                                          const ir_nodeRef& current_statement, const ir_manipulationRef ir_man)
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
         modified |= recursive_transform(function_id, gm->op0, current_statement, ir_man);
         modified |= recursive_transform(function_id, gm->op1, current_statement, ir_man);
         break;
      }
      case CASE_UNARY_NODES:
      {
         const auto ue = GetPointerS<unary_node>(curr_tn);
         modified |= recursive_transform(function_id, ue->op, current_statement, ir_man);
         break;
      }
      case CASE_BINARY_NODES:
      {
         const auto be = GetPointerS<binary_node>(curr_tn);
         const auto be_type = be->get_kind();
         modified |= recursive_transform(function_id, be->op0, current_statement, ir_man);
         modified |= recursive_transform(function_id, be->op1, current_statement, ir_man);

         if(be_type == idiv_node_K || be_type == irem_node_K)
         {
            const auto expr_type = ir_helper::CGetType(be->op0);
            const auto bitsize0 = ceil_pow2(ir_helper::Size(be->op0));
            const auto bitsize1 = ceil_pow2(ir_helper::Size(be->op1));
            const auto bitsize = std::max(bitsize0, bitsize1);

            const auto div_by_constant = [&]() {
               if(GetPointer<const constant_int_val_node>(be->op1))
               {
                  const auto cst_val = ir_helper::GetConstValue(be->op1);
                  if((cst_val & (cst_val - 1)) == 0)
                  {
                     return true;
                  }
               }
               return false;
            }();

            if(!div_by_constant && expr_type->get_kind() == integer_ty_node_K && (bitsize > 16 && bitsize <= 64))
            {
               const auto fu_suffix = be_type == irem_node_K ? "mod" : "div";
               const auto bitsize_str = bitsize <= 32 ? "s" : "d";
               const auto unsignedp = ir_helper::IsUnsignedIntegerType(expr_type);
               const std::string fu_name = "__" + STR(unsignedp ? "u" : "") + fu_suffix + bitsize_str + "i3" +
                                           ((bitsize0 > 32 && bitsize1 <= 32) ? "6432" : "");
               const auto called_function = IRM->GetFunction(fu_name);
               THROW_ASSERT(called_function, "The library miss this function " + fu_name);
               THROW_ASSERT(ir_helper::IsFunctionImplemented(called_function), "inconsistent behavioral helper");
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Adding call to " + fu_name);
               const std::vector<ir_nodeRef> args = {be->op0, be->op1};
               const auto ce = ir_man->CreateCallExpr(called_function, args, get_current_locinfo());
               const auto stmt = GetPointer<node_stmt>(current_statement);
               if(stmt && !stmt->predicate)
               {
                  stmt->predicate = IRM->CreateUniqueIntegerCst(1, ir_man->GetBooleanType());
               }
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Replaced " + STR(current_statement));
               IRM->ReplaceIRNode(current_statement, curr_tn, ce);
               CallGraphManager::addCallPointAndExpand(already_visited, AppM, function_id, called_function->index,
                                                       current_statement->index,
                                                       FunctionEdgeInfo::CallType::direct_call, DEBUG_LEVEL_NONE);
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---      -> " + STR(current_statement));
               modified = true;
            }
         }
         break;
      }
      case CASE_TERNARY_NODES:
      {
         const ternary_node* te = GetPointer<ternary_node>(curr_tn);
         modified |= recursive_transform(function_id, te->op0, current_statement, ir_man);
         if(te->op1)
         {
            modified |= recursive_transform(function_id, te->op1, current_statement, ir_man);
         }
         if(te->op2)
         {
            modified |= recursive_transform(function_id, te->op2, current_statement, ir_man);
         }
         break;
      }
      case constructor_node_K:
      {
         const constructor_node* co = GetPointer<constructor_node>(curr_tn);
         for(const auto& iv : co->list_of_idx_valu)
         {
            modified |= recursive_transform(function_id, iv.second, current_statement, ir_man);
         }
         break;
      }
      case CASE_TYPE_NODES:
      case field_val_node_K:
      case function_val_node_K:
      case call_stmt_K:
      case multi_way_if_stmt_K:
      case nop_stmt_K:
      case return_stmt_K:
      case constant_int_val_node_K:
      case lut_node_K:
      case argument_val_node_K:
      case constant_fp_val_node_K:
      case ssa_node_K:
      case variable_val_node_K:
      case constant_vector_val_node_K:
         break;
      case CASE_FAKE_NODES:
      case phi_stmt_K:
      case identifier_node_K:
      case statement_list_node_K:
      case module_unit_node_K:
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
