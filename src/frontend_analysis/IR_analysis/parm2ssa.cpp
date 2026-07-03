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
 *              Copyright (C) 2019-2026 Politecnico di Milano
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
 * @file parm2ssa.cpp
 * @brief Pre-analysis step computing the relation between argument_val_node and the associated ssa_node.
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 *
 */
#include "parm2ssa.hpp"

#include "Parameter.hpp"
#include "application_manager.hpp"
#include "behavioral_helper.hpp"
#include "call_graph.hpp"
#include "call_graph_manager.hpp"
#include "custom_map.hpp"
#include "dbgPrintHelper.hpp"
#include "exceptions.hpp"
#include "function_behavior.hpp"
#include "ir_basic_block.hpp"
#include "ir_helper.hpp"
#include "ir_manager.hpp"
#include "ir_manipulation.hpp"
#include "ir_node.hpp"
#include "string_manipulation.hpp"

#include <fstream>
#include <string>

parm2ssa::parm2ssa(const ParameterConstRef _parameters, const application_managerRef _AppM, unsigned int _function_id,
                   const DesignFlowManager& _design_flow_manager)
    : FunctionFrontendFlowStep(_AppM, _function_id, PARM2SSA, _design_flow_manager, _parameters)
{
   debug_level = parameters->get_class_debug_level(GET_CLASS(*this), DEBUG_LEVEL_NONE);
}

CustomUnorderedSet<std::pair<FrontendFlowStepType, FrontendFlowStep::FunctionRelationship>>
parm2ssa::ComputeFrontendRelationships(const DesignFlowStep::RelationshipType relationship_type) const
{
   CustomUnorderedSet<std::pair<FrontendFlowStepType, FunctionRelationship>> relationships;
   switch(relationship_type)
   {
      case(DEPENDENCE_RELATIONSHIP):
      {
         relationships.insert(std::make_pair(USE_COUNTING, SAME_FUNCTION));
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
      {
         THROW_UNREACHABLE("");
      }
   }
   return relationships;
}

DesignFlowStep_Status parm2ssa::InternalExec()
{
   const auto TM = AppM->get_ir_manager();
   const ir_manipulationRef IRman(new ir_manipulation(TM, parameters, AppM));
   /// Already visited address expression (used to avoid infinite recursion)
   CustomUnorderedSet<unsigned int> already_visited_ae;

   const auto beforeParm2SSA = AppM->getACopyParm2SSA(function_id);
   AppM->clearParm2SSA(function_id);

   const auto fnode = TM->GetIRNode(function_id);
   const auto fd = GetPointer<function_val_node>(fnode);
   const auto sl = GetPointer<statement_list_node>(fd->body);
   const std::string loc_info_default = fd->include_name + ":" + STR(fd->line_number) + ":" + STR(fd->column_number);
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                  "-->Analyzing function " + STR(function_id) + ": " + ir_helper::GetFunctionName(fnode));

   for(const auto& arg : fd->list_of_args)
   {
      recursive_analysis(arg, loc_info_default, already_visited_ae);
   }

   for(const auto& bb : sl->list_of_bloc)
   {
      for(const auto& stmt : bb.second->CGetStmtList())
      {
         recursive_analysis(stmt, loc_info_default, already_visited_ae);
      }
      for(const auto& phi : bb.second->CGetPhiList())
      {
         recursive_analysis(phi, loc_info_default, already_visited_ae);
      }
   }

   // TODO: should not be requested, but removing this causes issues with BitValue Inference steps
   for(const auto& arg : fd->list_of_args)
   {
      if(!AppM->getSSAFromParm(function_id, arg->index))
      {
         const auto pd = GetPointer<const argument_val_node>(arg);
         const auto ssa_par = IRman->create_ssa_name(arg, pd->type, ir_nodeRef(), ir_nodeRef());
         AppM->setSSAFromParm(function_id, arg->index, ssa_par->index);
      }
   }

   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                  "<--Analyzed function " + STR(function_id) + ": " + ir_helper::GetFunctionName(fnode));
   const auto afterParm2SSA = AppM->getACopyParm2SSA(function_id);
   const auto modified = afterParm2SSA != beforeParm2SSA;
   if(modified)
   {
      function_behavior->UpdateBBVersion();
   }
   return modified ? DesignFlowStep_Status::SUCCESS : DesignFlowStep_Status::UNCHANGED;
}

void parm2ssa::recursive_analysis(const ir_nodeRef& tn, const std::string& loc_info,
                                  CustomUnorderedSet<unsigned int>& already_visited_ae)
{
   const auto TM = AppM->get_ir_manager();
   const auto curr_tn = tn;
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                  "-->Analyzing recursively " + curr_tn->get_kind_text() + " " + STR(tn->index) + ": " +
                      curr_tn->ToString());
   switch(curr_tn->get_kind())
   {
      case call_node_K:
      {
         const auto ce = GetPointerS<call_node>(curr_tn);
         for(const auto& arg : ce->args)
         {
            recursive_analysis(arg, loc_info, already_visited_ae);
         }
         break;
      }
      case call_stmt_K:
      {
         const auto ce = GetPointerS<call_stmt>(curr_tn);
         for(const auto& arg : ce->args)
         {
            recursive_analysis(arg, loc_info, already_visited_ae);
         }
         if(ce->predicate)
         {
            recursive_analysis(ce->predicate, loc_info, already_visited_ae);
         }
         break;
      }
      case assign_stmt_K:
      {
         const auto gm = GetPointerS<assign_stmt>(curr_tn);

         recursive_analysis(gm->op0, loc_info, already_visited_ae);
         recursive_analysis(gm->op1, loc_info, already_visited_ae);
         if(gm->predicate)
         {
            recursive_analysis(gm->predicate, loc_info, already_visited_ae);
         }

         break;
      }
      case nop_stmt_K:
      {
         break;
      }
      case variable_val_node_K:
      case argument_val_node_K:
      {
         break;
      }
      case ssa_node_K:
      {
         const auto sn = GetPointerS<ssa_node>(curr_tn);
         if(sn->var)
         {
            const auto defStmt = sn->GetDefStmt();
            if(sn->var->get_kind() == argument_val_node_K && defStmt->get_kind() == nop_stmt_K)
            {
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                              "---Setting " + STR(sn->var->index) + "-> " + STR(tn->index) + " " + STR(tn->index));
               AppM->setSSAFromParm(function_id, sn->var->index, tn->index);
            }
            recursive_analysis(sn->var, loc_info, already_visited_ae);
         }
         break;
      }
      case CASE_UNARY_NODES:
      {
         if(curr_tn->get_kind() == addr_node_K)
         {
            if(already_visited_ae.find(tn->index) != already_visited_ae.end())
            {
               break;
            }
            already_visited_ae.insert(tn->index);
         }
         const auto ue = GetPointerS<unary_node>(curr_tn);
         recursive_analysis(ue->op, loc_info, already_visited_ae);
         break;
      }
      case CASE_BINARY_NODES:
      {
         const auto be = GetPointerS<binary_node>(curr_tn);
         recursive_analysis(be->op0, loc_info, already_visited_ae);
         recursive_analysis(be->op1, loc_info, already_visited_ae);
         break;
      }
      case CASE_TERNARY_NODES:
      {
         const auto te = GetPointerS<ternary_node>(curr_tn);
         recursive_analysis(te->op0, loc_info, already_visited_ae);
         if(te->op1)
         {
            recursive_analysis(te->op1, loc_info, already_visited_ae);
         }
         if(te->op2)
         {
            recursive_analysis(te->op2, loc_info, already_visited_ae);
         }
         break;
      }
      case lut_node_K:
      {
         const auto le = GetPointerS<lut_node>(curr_tn);
         recursive_analysis(le->op0, loc_info, already_visited_ae);
         recursive_analysis(le->op1, loc_info, already_visited_ae);
         if(le->op2)
         {
            recursive_analysis(le->op2, loc_info, already_visited_ae);
         }
         if(le->op3)
         {
            recursive_analysis(le->op3, loc_info, already_visited_ae);
         }
         if(le->op4)
         {
            recursive_analysis(le->op4, loc_info, already_visited_ae);
         }
         if(le->op5)
         {
            recursive_analysis(le->op5, loc_info, already_visited_ae);
         }
         if(le->op6)
         {
            recursive_analysis(le->op6, loc_info, already_visited_ae);
         }
         if(le->op7)
         {
            recursive_analysis(le->op7, loc_info, already_visited_ae);
         }
         if(le->op8)
         {
            recursive_analysis(le->op8, loc_info, already_visited_ae);
         }
         break;
      }
      case constructor_node_K:
      {
         const auto co = GetPointerS<constructor_node>(curr_tn);
         for(const auto& idx_valu : co->list_of_idx_valu)
         {
            recursive_analysis(idx_valu.second, loc_info, already_visited_ae);
         }
         break;
      }
      case multi_way_if_stmt_K:
      {
         const auto gmwi = GetPointerS<multi_way_if_stmt>(curr_tn);
         for(const auto& cond : gmwi->list_of_cond)
         {
            if(cond.first)
            {
               recursive_analysis(cond.first, loc_info, already_visited_ae);
            }
         }
         break;
      }
      case return_stmt_K:
      {
         const auto re = GetPointerS<return_stmt>(curr_tn);
         if(re->op)
         {
            recursive_analysis(re->op, loc_info, already_visited_ae);
         }
         break;
      }
      case phi_stmt_K:
      {
         const auto gp = GetPointerS<phi_stmt>(curr_tn);
         for(const auto& def_edge_pair : gp->list_of_def_edge)
         {
            recursive_analysis(def_edge_pair.first, loc_info, already_visited_ae);
         }
         break;
      }
      case CASE_TYPE_NODES:
      {
         break;
      }
      case field_val_node_K:
      case function_val_node_K:
      case constant_int_val_node_K:
      case constant_fp_val_node_K:
      case constant_vector_val_node_K:
         break;
      case CASE_FAKE_NODES:
      case identifier_node_K:
      case statement_list_node_K:
      case module_unit_node_K:
      {
         THROW_ERROR_CODE(NODE_NOT_YET_SUPPORTED_EC, "Not supported node: " + curr_tn->get_kind_text());
         break;
      }
      default:
         THROW_UNREACHABLE("");
   }
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Analyzed recursively " + STR(tn->index) + ": " + STR(tn));
   return;
}
