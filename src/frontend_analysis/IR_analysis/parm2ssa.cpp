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
 *              Copyright (C) 2019-2024 Politecnico di Milano
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
 * @file parm2ssa.cpp
 * @brief Pre-analysis step computing the relation between parm_decl and the associated ssa_name.
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *
 */
/// Header include
#include "parm2ssa.hpp"

/// Behavior include
#include "application_manager.hpp"
#include "call_graph.hpp"
#include "call_graph_manager.hpp"
#include "function_behavior.hpp"

/// Graph include

/// Parameter include
#include "Parameter.hpp"

/// STD include
#include <fstream>

/// STL include
#include "custom_map.hpp"
#include <string>

/// Tree include
#include "behavioral_helper.hpp"
#include "ext_tree_node.hpp"
#include "tree_basic_block.hpp"
#include "tree_helper.hpp"
#include "tree_manager.hpp"
#include "tree_manipulation.hpp"
#include "tree_node.hpp"

/// Utility include
#include "dbgPrintHelper.hpp"
#include "exceptions.hpp"
#include "string_manipulation.hpp" // for GET_CLASS

parm2ssa::parm2ssa(const ParameterConstRef _parameters, const application_managerRef _AppM, unsigned int _function_id,
                   const DesignFlowManagerConstRef _design_flow_manager)
    : FunctionFrontendFlowStep(_AppM, _function_id, PARM2SSA, _design_flow_manager, _parameters)
{
   debug_level = parameters->get_class_debug_level(GET_CLASS(*this), DEBUG_LEVEL_NONE);
}

parm2ssa::~parm2ssa() = default;

CustomUnorderedSet<std::pair<FrontendFlowStepType, FrontendFlowStep::FunctionRelationship>>
parm2ssa::ComputeFrontendRelationships(const DesignFlowStep::RelationshipType relationship_type) const
{
   CustomUnorderedSet<std::pair<FrontendFlowStepType, FunctionRelationship>> relationships;
   switch(relationship_type)
   {
      case(DEPENDENCE_RELATIONSHIP):
      {
         relationships.insert(std::make_pair(PARM_DECL_TAKEN_ADDRESS, SAME_FUNCTION));
         relationships.insert(std::make_pair(FIX_STRUCTS_PASSED_BY_VALUE, SAME_FUNCTION));
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
   const auto TM = AppM->get_tree_manager();
   const tree_manipulationRef IRman(new tree_manipulation(TM, parameters, AppM));
   /// Already visited address expression (used to avoid infinite recursion)
   CustomUnorderedSet<unsigned int> already_visited_ae;

   const auto beforeParm2SSA = AppM->getACopyParm2SSA(function_id);
   AppM->clearParm2SSA(function_id);

   const auto curr_tn = TM->GetTreeNode(function_id);
   const auto fd = GetPointer<function_decl>(curr_tn);
   const auto sl = GetPointer<statement_list>(fd->body);
   const std::string srcp_default = fd->include_name + ":" + STR(fd->line_number) + ":" + STR(fd->column_number);
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                  "-->Analyzing function " + STR(function_id) + ": " + tree_helper::GetMangledFunctionName(fd));

   for(const auto& arg : fd->list_of_args)
   {
      recursive_analysis(arg, srcp_default, already_visited_ae);
   }

   for(const auto& bb : sl->list_of_bloc)
   {
      for(const auto& stmt : bb.second->CGetStmtList())
      {
         recursive_analysis(stmt, srcp_default, already_visited_ae);
      }
      for(const auto& phi : bb.second->CGetPhiList())
      {
         recursive_analysis(phi, srcp_default, already_visited_ae);
      }
   }

   // TODO: should not be requested, but removing this causes issues with BitValue Inference steps
   for(const auto& arg : fd->list_of_args)
   {
      if(!AppM->getSSAFromParm(function_id, arg->index))
      {
         const auto pd = GetPointer<const parm_decl>(arg);
         const auto ssa_par = IRman->create_ssa_name(arg, pd->type, tree_nodeRef(), tree_nodeRef());
         AppM->setSSAFromParm(function_id, arg->index, ssa_par->index);
      }
   }

   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                  "<--Analyzed function " + STR(function_id) + ": " + tree_helper::GetMangledFunctionName(fd));
   const auto afterParm2SSA = AppM->getACopyParm2SSA(function_id);
   const auto modified = afterParm2SSA != beforeParm2SSA;
   if(modified)
   {
      function_behavior->UpdateBBVersion();
   }
   return modified ? DesignFlowStep_Status::SUCCESS : DesignFlowStep_Status::UNCHANGED;
}

void parm2ssa::recursive_analysis(const tree_nodeRef& tn, const std::string& srcp,
                                  CustomUnorderedSet<unsigned int>& already_visited_ae)
{
   const auto TM = AppM->get_tree_manager();
   const auto curr_tn = tn;
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                  "-->Analyzing recursively " + curr_tn->get_kind_text() + " " + STR(tn->index) + ": " +
                      curr_tn->ToString());
   switch(curr_tn->get_kind())
   {
      case call_expr_K:
      case aggr_init_expr_K:
      {
         const auto ce = GetPointerS<call_expr>(curr_tn);
         for(const auto& arg : ce->args)
         {
            recursive_analysis(arg, srcp, already_visited_ae);
         }
         break;
      }
      case gimple_call_K:
      {
         const auto ce = GetPointerS<gimple_call>(curr_tn);
         for(const auto& arg : ce->args)
         {
            recursive_analysis(arg, srcp, already_visited_ae);
         }
         break;
      }
      case gimple_assign_K:
      {
         const auto gm = GetPointerS<gimple_assign>(curr_tn);
         if(!gm->clobber)
         {
            if(!gm->init_assignment)
            {
               recursive_analysis(gm->op0, srcp, already_visited_ae);
               recursive_analysis(gm->op1, srcp, already_visited_ae);
               if(gm->predicate)
               {
                  recursive_analysis(gm->predicate, srcp, already_visited_ae);
               }
            }
         }
         break;
      }
      case gimple_nop_K:
      {
         break;
      }
      case var_decl_K:
      case parm_decl_K:
      {
         break;
      }
      case ssa_name_K:
      {
         const auto sn = GetPointerS<ssa_name>(curr_tn);
         if(sn->var)
         {
            const auto defStmt = sn->CGetDefStmt();
            if(sn->var->get_kind() == parm_decl_K && defStmt->get_kind() == gimple_nop_K)
            {
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                              "---Setting " + STR(sn->var->index) + "-> " + STR(tn->index) + " " + STR(tn->index));
               AppM->setSSAFromParm(function_id, sn->var->index, tn->index);
            }
            recursive_analysis(sn->var, srcp, already_visited_ae);
         }
         break;
      }
      case tree_list_K:
      {
         auto current = tn;
         while(current)
         {
            recursive_analysis(GetPointerS<tree_list>(current)->valu, srcp, already_visited_ae);
            current = GetPointerS<tree_list>(current)->chan;
         }
         break;
      }
      case CASE_UNARY_EXPRESSION:
      {
         if(curr_tn->get_kind() == addr_expr_K)
         {
            if(already_visited_ae.find(tn->index) != already_visited_ae.end())
            {
               break;
            }
            already_visited_ae.insert(tn->index);
         }
         const auto ue = GetPointerS<unary_expr>(curr_tn);
         recursive_analysis(ue->op, srcp, already_visited_ae);
         break;
      }
      case CASE_BINARY_EXPRESSION:
      {
         const auto be = GetPointerS<binary_expr>(curr_tn);
         recursive_analysis(be->op0, srcp, already_visited_ae);
         recursive_analysis(be->op1, srcp, already_visited_ae);
         break;
      }
      case CASE_TERNARY_EXPRESSION:
      {
         const auto te = GetPointerS<ternary_expr>(curr_tn);
         recursive_analysis(te->op0, srcp, already_visited_ae);
         if(te->op1)
         {
            recursive_analysis(te->op1, srcp, already_visited_ae);
         }
         if(te->op2)
         {
            recursive_analysis(te->op2, srcp, already_visited_ae);
         }
         break;
      }
      case CASE_QUATERNARY_EXPRESSION:
      {
         const auto qe = GetPointerS<quaternary_expr>(curr_tn);
         recursive_analysis(qe->op0, srcp, already_visited_ae);
         if(qe->op1)
         {
            recursive_analysis(qe->op1, srcp, already_visited_ae);
         }
         if(qe->op2)
         {
            recursive_analysis(qe->op2, srcp, already_visited_ae);
         }
         if(qe->op3)
         {
            recursive_analysis(qe->op3, srcp, already_visited_ae);
         }
         break;
      }
      case lut_expr_K:
      {
         const auto le = GetPointerS<lut_expr>(curr_tn);
         recursive_analysis(le->op0, srcp, already_visited_ae);
         recursive_analysis(le->op1, srcp, already_visited_ae);
         if(le->op2)
         {
            recursive_analysis(le->op2, srcp, already_visited_ae);
         }
         if(le->op3)
         {
            recursive_analysis(le->op3, srcp, already_visited_ae);
         }
         if(le->op4)
         {
            recursive_analysis(le->op4, srcp, already_visited_ae);
         }
         if(le->op5)
         {
            recursive_analysis(le->op5, srcp, already_visited_ae);
         }
         if(le->op6)
         {
            recursive_analysis(le->op6, srcp, already_visited_ae);
         }
         if(le->op7)
         {
            recursive_analysis(le->op7, srcp, already_visited_ae);
         }
         if(le->op8)
         {
            recursive_analysis(le->op8, srcp, already_visited_ae);
         }
         break;
      }
      case constructor_K:
      {
         const auto co = GetPointerS<constructor>(curr_tn);
         for(const auto& idx_valu : co->list_of_idx_valu)
         {
            recursive_analysis(idx_valu.second, srcp, already_visited_ae);
         }
         break;
      }
      case gimple_cond_K:
      {
         const auto gc = GetPointerS<gimple_cond>(curr_tn);
         recursive_analysis(gc->op0, srcp, already_visited_ae);
         break;
      }
      case gimple_switch_K:
      {
         auto se = GetPointer<gimple_switch>(curr_tn);
         recursive_analysis(se->op0, srcp, already_visited_ae);
         break;
      }
      case gimple_multi_way_if_K:
      {
         const auto gmwi = GetPointerS<gimple_multi_way_if>(curr_tn);
         for(const auto& cond : gmwi->list_of_cond)
         {
            if(cond.first)
            {
               recursive_analysis(cond.first, srcp, already_visited_ae);
            }
         }
         break;
      }
      case gimple_return_K:
      {
         const auto re = GetPointerS<gimple_return>(curr_tn);
         if(re->op)
         {
            recursive_analysis(re->op, srcp, already_visited_ae);
         }
         break;
      }
      case gimple_for_K:
      {
         const auto fe = GetPointerS<gimple_for>(curr_tn);
         recursive_analysis(fe->op0, srcp, already_visited_ae);
         recursive_analysis(fe->op1, srcp, already_visited_ae);
         recursive_analysis(fe->op2, srcp, already_visited_ae);
         break;
      }
      case gimple_while_K:
      {
         const auto we = GetPointerS<gimple_while>(curr_tn);
         recursive_analysis(we->op0, srcp, already_visited_ae);
         break;
      }
      case gimple_phi_K:
      {
         const auto gp = GetPointerS<gimple_phi>(curr_tn);
         for(const auto& def_edge_pair : gp->list_of_def_edge)
         {
            recursive_analysis(def_edge_pair.first, srcp, already_visited_ae);
         }
         break;
      }
      case CASE_TYPE_NODES:
      case type_decl_K:
      {
         break;
      }
      case target_mem_ref_K:
      {
         const auto tmr = GetPointerS<target_mem_ref>(curr_tn);
         if(tmr->symbol)
         {
            recursive_analysis(tmr->symbol, srcp, already_visited_ae);
         }
         if(tmr->base)
         {
            recursive_analysis(tmr->base, srcp, already_visited_ae);
         }
         if(tmr->idx)
         {
            recursive_analysis(tmr->idx, srcp, already_visited_ae);
         }
         break;
      }
      case target_mem_ref461_K:
      {
         const auto tmr = GetPointerS<target_mem_ref461>(curr_tn);
         if(tmr->base)
         {
            recursive_analysis(tmr->base, srcp, already_visited_ae);
         }
         if(tmr->idx)
         {
            recursive_analysis(tmr->idx, srcp, already_visited_ae);
         }
         if(tmr->idx2)
         {
            recursive_analysis(tmr->idx2, srcp, already_visited_ae);
         }
         break;
      }
      case string_cst_K:
      {
         break;
      }
      case CASE_PRAGMA_NODES:
      case case_label_expr_K:
      case complex_cst_K:
      case field_decl_K:
      case function_decl_K:
      case gimple_asm_K:
      case gimple_goto_K:
      case gimple_label_K:
      case gimple_pragma_K:
      case gimple_resx_K:
      case integer_cst_K:
      case label_decl_K:
      case real_cst_K:
      case result_decl_K:
      case template_decl_K:
      case tree_vec_K:
      case vector_cst_K:
      case void_cst_K:
         break;
      case CASE_CPP_NODES:
      case CASE_FAKE_NODES:
      case binfo_K:
      case block_K:
      case const_decl_K:
      case error_mark_K:
      case gimple_bind_K:
      case gimple_predict_K:
      case identifier_node_K:
      case namespace_decl_K:
      case statement_list_K:
      case target_expr_K:
      case translation_unit_decl_K:
      case using_decl_K:
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
