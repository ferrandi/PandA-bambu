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
 *              Copyright (C) 2004-2021 Politecnico di Milano
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
 * @file use_counting.cpp
 * @brief Analysis step counting how many times a ssa_name is used
 *
 * @author Marco Lattuada <lattuada@elet.polimi.it>
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 *
 */

/// Autoheader include
#include "config_HAVE_BAMBU_BUILT.hpp"

/// Header include
#include "use_counting.hpp"

/// Behavior include
#include "application_manager.hpp"

/// design_flow_manager includes
#include "design_flow_graph.hpp"
#include "design_flow_manager.hpp"

/// Parameter include
#include "Parameter.hpp"

/// STL includes
#include <list>
#include <utility>

/// tree include
#include "dbgPrintHelper.hpp" // for DEBUG_LEVEL_
#include "ext_tree_node.hpp"
#include "string_manipulation.hpp" // for GET_CLASS
#include "tree_basic_block.hpp"
#include "tree_helper.hpp"
#include "tree_manager.hpp"
#include "tree_node.hpp"
#include "tree_reindex.hpp"

use_counting::use_counting(const ParameterConstRef _parameters, const application_managerRef _AppM, unsigned int _function_id, const DesignFlowManagerConstRef _design_flow_manager)
    : FunctionFrontendFlowStep(_AppM, _function_id, USE_COUNTING, _design_flow_manager, _parameters)
{
   debug_level = parameters->get_class_debug_level(GET_CLASS(*this), DEBUG_LEVEL_NONE);
}

use_counting::~use_counting() = default;

const CustomUnorderedSet<std::pair<FrontendFlowStepType, FrontendFlowStep::FunctionRelationship>> use_counting::ComputeFrontendRelationships(const DesignFlowStep::RelationshipType relationship_type) const
{
   CustomUnorderedSet<std::pair<FrontendFlowStepType, FunctionRelationship>> relationships;
   switch(relationship_type)
   {
      case(DEPENDENCE_RELATIONSHIP):
      {
         relationships.insert(std::make_pair(REBUILD_INITIALIZATION2, SAME_FUNCTION));
         break;
      }
      case(PRECEDENCE_RELATIONSHIP):
      {
         relationships.insert(std::make_pair(REMOVE_CLOBBER_GA, SAME_FUNCTION));
         relationships.insert(std::make_pair(HWCALL_INJECTION, SAME_FUNCTION));
         relationships.insert(std::make_pair(SWITCH_FIX, SAME_FUNCTION));
         relationships.insert(std::make_pair(REBUILD_INITIALIZATION, SAME_FUNCTION));
         relationships.insert(std::make_pair(IR_LOWERING, SAME_FUNCTION));
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

DesignFlowStep_Status use_counting::InternalExec()
{
   const auto TM = AppM->get_tree_manager();
   const auto fd = GetPointerS<const function_decl>(TM->CGetTreeNode(function_id));
   const auto sl = GetPointerS<const statement_list>(GET_CONST_NODE(fd->body));
   for(const auto& bbi_bb : sl->list_of_bloc)
   {
      const auto& bb = bbi_bb.second;
      for(const auto& statement_node : bb->CGetStmtList())
      {
         /// [breadshe] This set contains the ssa_name nodes "used" by the statement
         std::list<tree_nodeRef> ssa_uses;
         analyze_node(statement_node, ssa_uses);
         /// [breadshe] Add current statement to the use_stmts corresponding to the ssa_name nodes contained in ssa_uses
         for(const auto& ssa_use : ssa_uses)
         {
            auto sn = GetPointerS<ssa_name>(GET_NODE(ssa_use));
            sn->AddUseStmt(statement_node);
         }
      }
      for(const auto& phi_node : bb->CGetPhiList())
      {
         std::list<tree_nodeRef> ssa_uses;
         analyze_node(phi_node, ssa_uses);
         for(const auto& ssa_use : ssa_uses)
         {
            auto sn = GetPointerS<ssa_name>(GET_NODE(ssa_use));
            sn->AddUseStmt(phi_node);
         }
         GetPointerS<gimple_phi>(GET_NODE(phi_node))->SetSSAUsesComputed();
      }
      bb->SetSSAUsesComputed();
   }

   // THROW_ASSERT(TM->check_ssa_uses(function_id), "Inconsistent ssa uses: post");
   return DesignFlowStep_Status::SUCCESS;
}

void use_counting::analyze_node(const tree_nodeRef& tn, std::list<tree_nodeRef>& ssa_uses)
{
   THROW_ASSERT(tn->get_kind() == tree_reindex_K, "Node is not a tree reindex");
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Analyzing node " + tn->ToString());
   const auto curr_tn = GET_CONST_NODE(tn);
   const auto gn = GetPointer<const gimple_node>(curr_tn);
   if(gn)
   {
      if(gn->memuse)
      {
         analyze_node(gn->memuse, ssa_uses);
      }
      if(gn->vuses.size())
      {
         for(const auto& vuse : gn->vuses)
         {
            analyze_node(vuse, ssa_uses);
         }
      }
   }

   switch(curr_tn->get_kind())
   {
      case gimple_return_K:
      {
         const auto re = GetPointerS<const gimple_return>(curr_tn);
         if(re->op)
         {
            analyze_node(re->op, ssa_uses);
         }
         break;
      }
      case gimple_assign_K:
      {
         const auto me = GetPointerS<const gimple_assign>(curr_tn);
         if(GET_CONST_NODE(me->op0)->get_kind() != ssa_name_K)
         {
            analyze_node(me->op0, ssa_uses);
         }
         analyze_node(me->op1, ssa_uses);
         if(me->predicate)
         {
            analyze_node(me->predicate, ssa_uses);
         }
         break;
      }
      case gimple_nop_K:
      {
         break;
      }
      case aggr_init_expr_K:
      case call_expr_K:
      {
         const auto ce = GetPointerS<const call_expr>(curr_tn);
         analyze_node(ce->fn, ssa_uses);
         for(const auto& arg : ce->args)
         {
            analyze_node(arg, ssa_uses);
         }
         break;
      }
      case gimple_call_K:
      {
         const auto ce = GetPointerS<const gimple_call>(curr_tn);
         analyze_node(ce->fn, ssa_uses);
         for(const auto& arg : ce->args)
         {
            analyze_node(arg, ssa_uses);
         }
         break;
      }
      case gimple_cond_K:
      {
         const auto gc = GetPointerS<const gimple_cond>(curr_tn);
         analyze_node(gc->op0, ssa_uses);
         break;
      }
      /* Unary expressions.  */
      case CASE_UNARY_EXPRESSION:
      {
         const auto ue = GetPointerS<const unary_expr>(curr_tn);
         if(GET_NODE(ue->op)->get_kind() != function_decl_K)
         {
            analyze_node(ue->op, ssa_uses);
         }
         break;
      }
      case CASE_BINARY_EXPRESSION:
      {
         const auto be = GetPointerS<const binary_expr>(curr_tn);
         analyze_node(be->op0, ssa_uses);
         analyze_node(be->op1, ssa_uses);
         break;
      }
      /*ternary expressions*/
      case gimple_switch_K:
      {
         const auto se = GetPointerS<const gimple_switch>(curr_tn);
         analyze_node(se->op0, ssa_uses);
         break;
      }
      case CASE_TERNARY_EXPRESSION:
      {
         const auto te = GetPointerS<const ternary_expr>(curr_tn);
         analyze_node(te->op0, ssa_uses);
         analyze_node(te->op1, ssa_uses);
         if(te->op2)
         {
            analyze_node(te->op2, ssa_uses);
         }
         break;
      }
      case CASE_QUATERNARY_EXPRESSION:
      {
         const auto qe = GetPointerS<const quaternary_expr>(curr_tn);
         analyze_node(qe->op0, ssa_uses);
         analyze_node(qe->op1, ssa_uses);
         if(qe->op2)
         {
            analyze_node(qe->op2, ssa_uses);
         }
         if(qe->op3)
         {
            analyze_node(qe->op3, ssa_uses);
         }
         break;
      }
      case lut_expr_K:
      {
         const auto le = GetPointerS<const lut_expr>(curr_tn);
         analyze_node(le->op0, ssa_uses);
         analyze_node(le->op1, ssa_uses);
         if(le->op2)
         {
            analyze_node(le->op2, ssa_uses);
         }
         if(le->op3)
         {
            analyze_node(le->op3, ssa_uses);
         }
         if(le->op4)
         {
            analyze_node(le->op4, ssa_uses);
         }
         if(le->op5)
         {
            analyze_node(le->op5, ssa_uses);
         }
         if(le->op6)
         {
            analyze_node(le->op6, ssa_uses);
         }
         if(le->op7)
         {
            analyze_node(le->op7, ssa_uses);
         }
         if(le->op8)
         {
            analyze_node(le->op8, ssa_uses);
         }
         break;
      }
      case constructor_K:
      {
         const auto c = GetPointerS<const constructor>(curr_tn);
         for(const auto& iv : c->list_of_idx_valu)
         {
            analyze_node(iv.second, ssa_uses);
         }
         break;
      }
      case var_decl_K:
      {
         /// var decl performs an assignment when init is not null
         // const auto vd = GetPointerS<const var_decl>(curr_tn);
         // if(vd->init)
         // {
         //    analyze_node(vd->init, ssa_uses);
         // }
         break;
      }
      case gimple_asm_K:
      {
         const auto ae = GetPointerS<const gimple_asm>(curr_tn);
         if(ae->out)
         {
            analyze_node(ae->out, ssa_uses);
         }
         if(ae->in)
         {
            analyze_node(ae->in, ssa_uses);
         }
         if(ae->clob)
         {
            analyze_node(ae->in, ssa_uses);
         }
         break;
      }
      case gimple_goto_K:
      {
         const auto ge = GetPointerS<const gimple_goto>(curr_tn);
         analyze_node(ge->op, ssa_uses);
         break;
      }
      case tree_list_K:
      {
         auto tl = GetPointerS<const tree_list>(curr_tn);
         while(tl)
         {
            if(tl->purp)
            {
               analyze_node(tl->purp, ssa_uses);
            }
            if(tl->valu)
            {
               analyze_node(tl->valu, ssa_uses);
            }
            tl = tl->chan ? GetPointerS<const tree_list>(GET_CONST_NODE(tl->chan)) : nullptr;
         }
         break;
      }
      case gimple_multi_way_if_K:
      {
         const auto gmwi = GetPointerS<const gimple_multi_way_if>(curr_tn);
         for(const auto& cond : gmwi->list_of_cond)
         {
            if(cond.first)
            {
               analyze_node(cond.first, ssa_uses);
            }
         }
         break;
      }
      case result_decl_K:
      case parm_decl_K:
      case function_decl_K:
      case integer_cst_K:
      case real_cst_K:
      case string_cst_K:
      case vector_cst_K:
      case void_cst_K:
      case complex_cst_K:
      case field_decl_K:
      case label_decl_K:
      case template_decl_K:
      case gimple_label_K:
      case gimple_pragma_K:
      case gimple_resx_K:
      case CASE_PRAGMA_NODES:
      {
         break;
      }
      case gimple_phi_K:
      {
         const auto gp = GetPointerS<const gimple_phi>(curr_tn);
         for(auto def_nodes : gp->CGetDefEdgesList())
         {
            analyze_node(def_nodes.first, ssa_uses);
         }
         break;
      }
      case target_mem_ref_K:
      {
         const auto tmr = GetPointerS<const target_mem_ref>(curr_tn);
         if(tmr->base)
         {
            analyze_node(tmr->base, ssa_uses);
         }
         if(tmr->symbol)
         {
            analyze_node(tmr->symbol, ssa_uses);
         }
         if(tmr->idx)
         {
            analyze_node(tmr->idx, ssa_uses);
         }
         /// step and offset are constants
         break;
      }
      case target_mem_ref461_K:
      {
         const auto tmr = GetPointerS<const target_mem_ref461>(curr_tn);
         if(tmr->base)
         {
            analyze_node(tmr->base, ssa_uses);
         }
         if(tmr->idx)
         {
            analyze_node(tmr->idx, ssa_uses);
         }
         if(tmr->idx2)
         {
            analyze_node(tmr->idx2, ssa_uses);
         }
         /// step and offset are constants
         break;
      }
      case ssa_name_K:
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Added use " + tn->ToString());
         ssa_uses.push_back(tn);
         break;
      }
      case binfo_K:
      case block_K:
      case case_label_expr_K:
      case CASE_CPP_NODES:
      case const_decl_K:
      case CASE_FAKE_NODES:
      case gimple_bind_K:
      case gimple_for_K:
      case gimple_predict_K:
      case gimple_while_K:
      case identifier_node_K:
      case namespace_decl_K:
      case statement_list_K:
      case translation_unit_decl_K:
      case error_mark_K:
      case using_decl_K:
      case tree_vec_K:
      case type_decl_K:
      case CASE_TYPE_NODES:
      case target_expr_K:
      {
         THROW_UNREACHABLE("Node is " + curr_tn->get_kind_text());
         break;
      }
      default:
         THROW_UNREACHABLE("");
   }
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Analyzed node " + tn->ToString());
}
