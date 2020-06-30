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
 *              Copyright (C) 2004-2020 Politecnico di Milano
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
 * @file string_cst_fix.cpp
 * @brief Pre-analysis step fixing readonly initializations and string_cst references.
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *
 */
/// Header include
#include "string_cst_fix.hpp"

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
#include "tree_manager.hpp"
#include "tree_manipulation.hpp"
#include "tree_node.hpp"
#include "tree_reindex.hpp"

/// Utility include
#include "dbgPrintHelper.hpp"
#include "exceptions.hpp"
#include "string_manipulation.hpp" // for GET_CLASS

string_cst_fix::string_cst_fix(const application_managerRef _AppM, const DesignFlowManagerConstRef _design_flow_manager, const ParameterConstRef _parameters) : ApplicationFrontendFlowStep(_AppM, STRING_CST_FIX, _design_flow_manager, _parameters)
{
   debug_level = parameters->get_class_debug_level(GET_CLASS(*this), DEBUG_LEVEL_NONE);
}

string_cst_fix::~string_cst_fix() = default;

const CustomUnorderedSet<std::pair<FrontendFlowStepType, FrontendFlowStep::FunctionRelationship>> string_cst_fix::ComputeFrontendRelationships(const DesignFlowStep::RelationshipType relationship_type) const
{
   CustomUnorderedSet<std::pair<FrontendFlowStepType, FunctionRelationship>> relationships;
   switch(relationship_type)
   {
      case(DEPENDENCE_RELATIONSHIP):
      {
         relationships.insert(std::make_pair(FUNCTION_ANALYSIS, WHOLE_APPLICATION));
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

DesignFlowStep_Status string_cst_fix::Exec()
{
   const CallGraphManagerConstRef CG = AppM->CGetCallGraphManager();
   const tree_managerRef TM = AppM->get_tree_manager();
   CustomOrderedSet<unsigned int> reached_body_fun_ids = CG->GetReachedBodyFunctions();

   for(unsigned int function_id : reached_body_fun_ids)
   {
      const tree_nodeRef curr_tn = TM->GetTreeNode(function_id);
      auto* fd = GetPointer<function_decl>(curr_tn);
      auto* sl = GetPointer<statement_list>(GET_NODE(fd->body));
      const std::string srcp_default = fd->include_name + ":" + STR(fd->line_number) + ":" + STR(fd->column_number);

      for(auto arg : fd->list_of_args)
         recursive_analysis(arg, srcp_default);

      std::map<unsigned int, blocRef>& blocks = sl->list_of_bloc;
      std::map<unsigned int, blocRef>::iterator it, it_end;

      it_end = blocks.end();
      for(it = blocks.begin(); it != it_end; ++it)
      {
         for(auto stmt : it->second->CGetStmtList())
         {
            recursive_analysis(stmt, srcp_default);
         }
         for(auto phi : it->second->CGetPhiList())
         {
            recursive_analysis(phi, srcp_default);
         }
      }
   }
   already_visited_ae.clear();
   string_cst_map.clear();
   return DesignFlowStep_Status::SUCCESS;
}

void string_cst_fix::recursive_analysis(tree_nodeRef& tn, const std::string& srcp)
{
   THROW_ASSERT(tn->get_kind() == tree_reindex_K, "Node is not a tree reindex");
   const tree_managerRef TM = AppM->get_tree_manager();
   const tree_nodeRef curr_tn = GET_NODE(tn);
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Analyzing recursively " + curr_tn->get_kind_text() + " " + STR(GET_INDEX_NODE(tn)) + ": " + curr_tn->ToString());
   switch(curr_tn->get_kind())
   {
      case call_expr_K:
      case aggr_init_expr_K:
      {
         auto* ce = GetPointer<call_expr>(curr_tn);
         for(auto& arg : ce->args)
         {
            recursive_analysis(arg, srcp);
         }
         break;
      }
      case gimple_call_K:
      {
         auto* ce = GetPointer<gimple_call>(curr_tn);
         for(auto& arg : ce->args)
         {
            recursive_analysis(arg, srcp);
         }
         break;
      }
      case gimple_assign_K:
      {
         auto* gm = GetPointer<gimple_assign>(curr_tn);
         if(!gm->clobber)
         {
            if(GET_NODE(gm->op0)->get_kind() == var_decl_K && (GET_NODE(gm->op1)->get_kind() == string_cst_K || GET_NODE(gm->op1)->get_kind() == constructor_K))
            {
               auto* vd = GetPointer<var_decl>(GET_NODE(gm->op0));
               THROW_ASSERT(vd, "not valid variable");
               if(vd->readonly_flag)
               {
                  vd->init = gm->op1;
                  gm->init_assignment = true; /// Removing statements create more problems than what it may solve.
                  /// So the solution for bambu is to not synthesize them.
               }
            }
            else if(GET_NODE(gm->op0)->get_kind() == var_decl_K && GET_NODE(gm->op1)->get_kind() == var_decl_K && GetPointer<var_decl>(GET_NODE(gm->op1))->init && GetPointer<var_decl>(GET_NODE(gm->op1))->used == 0)
            {
               auto* vd = GetPointer<var_decl>(GET_NODE(gm->op0));
               THROW_ASSERT(vd, "not valid variable");
               if(vd->readonly_flag)
               {
                  vd->init = GetPointer<var_decl>(GET_NODE(gm->op1))->init;
                  gm->init_assignment = true;
               }
               else
               {
                  /// makes the var_decl visible
                  auto* vd1 = GetPointer<var_decl>(GET_NODE(gm->op1));
                  vd1->include_name = gm->include_name;
                  vd1->line_number = gm->line_number;
                  vd1->column_number = gm->column_number;
               }
            }
            if(!gm->init_assignment)
            {
               if(GET_NODE(gm->op0)->get_kind() == var_decl_K && GetPointer<var_decl>(GET_NODE(gm->op0))->readonly_flag && GET_NODE(gm->op1)->get_kind() == ssa_name_K)
                  GetPointer<var_decl>(GET_NODE(gm->op0))->readonly_flag = false;
               recursive_analysis(gm->op0, srcp);
               recursive_analysis(gm->op1, srcp);
               if(gm->predicate)
                  recursive_analysis(gm->predicate, srcp);
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
         auto* sn = GetPointer<ssa_name>(curr_tn);
         if(sn->var)
            recursive_analysis(sn->var, srcp);
         break;
      }
      case tree_list_K:
      {
         tree_nodeRef current = tn;
         while(current)
         {
            recursive_analysis(GetPointer<tree_list>(GET_NODE(current))->valu, srcp);
            current = GetPointer<tree_list>(GET_NODE(current))->chan;
         }
         break;
      }
      case CASE_UNARY_EXPRESSION:
      {
         if(curr_tn->get_kind() == addr_expr_K)
         {
            if(already_visited_ae.find(GET_INDEX_NODE(tn)) != already_visited_ae.end())
            {
               break;
            }
            already_visited_ae.insert(GET_INDEX_NODE(tn));
         }
         auto* ue = GetPointer<unary_expr>(curr_tn);
         recursive_analysis(ue->op, srcp);
         break;
      }
      case CASE_BINARY_EXPRESSION:
      {
         auto* be = GetPointer<binary_expr>(curr_tn);
         recursive_analysis(be->op0, srcp);
         recursive_analysis(be->op1, srcp);
         break;
      }
      case CASE_TERNARY_EXPRESSION:
      {
         auto* te = GetPointer<ternary_expr>(curr_tn);
         recursive_analysis(te->op0, srcp);
         if(te->op1)
            recursive_analysis(te->op1, srcp);
         if(te->op2)
            recursive_analysis(te->op2, srcp);
         break;
      }
      case CASE_QUATERNARY_EXPRESSION:
      {
         auto* qe = GetPointer<quaternary_expr>(curr_tn);
         recursive_analysis(qe->op0, srcp);
         if(qe->op1)
            recursive_analysis(qe->op1, srcp);
         if(qe->op2)
            recursive_analysis(qe->op2, srcp);
         if(qe->op3)
            recursive_analysis(qe->op3, srcp);
         break;
      }
      case lut_expr_K:
      {
         auto* le = GetPointer<lut_expr>(curr_tn);
         recursive_analysis(le->op0, srcp);
         recursive_analysis(le->op1, srcp);
         if(le->op2)
            recursive_analysis(le->op2, srcp);
         if(le->op3)
            recursive_analysis(le->op3, srcp);
         if(le->op4)
            recursive_analysis(le->op4, srcp);
         if(le->op5)
            recursive_analysis(le->op5, srcp);
         if(le->op6)
            recursive_analysis(le->op6, srcp);
         if(le->op7)
            recursive_analysis(le->op7, srcp);
         if(le->op8)
            recursive_analysis(le->op8, srcp);
         break;
      }
      case constructor_K:
      {
         auto* co = GetPointer<constructor>(curr_tn);
         std::vector<std::pair<tree_nodeRef, tree_nodeRef>>& list_of_idx_valu = co->list_of_idx_valu;
         std::vector<std::pair<tree_nodeRef, tree_nodeRef>>::iterator it, it_end = list_of_idx_valu.end();
         for(it = list_of_idx_valu.begin(); it != it_end; ++it)
         {
            recursive_analysis(it->second, srcp);
         }
         break;
      }
      case gimple_cond_K:
      {
         auto* gc = GetPointer<gimple_cond>(curr_tn);
         recursive_analysis(gc->op0, srcp);
         break;
      }
      case gimple_switch_K:
      {
         auto* se = GetPointer<gimple_switch>(curr_tn);
         recursive_analysis(se->op0, srcp);
         break;
      }
      case gimple_multi_way_if_K:
      {
         auto* gmwi = GetPointer<gimple_multi_way_if>(curr_tn);
         for(auto cond : gmwi->list_of_cond)
            if(cond.first)
               recursive_analysis(cond.first, srcp);
         break;
      }
      case gimple_return_K:
      {
         auto* re = GetPointer<gimple_return>(curr_tn);
         if(re->op)
            recursive_analysis(re->op, srcp);
         break;
      }
      case gimple_for_K:
      {
         auto* fe = GetPointer<gimple_for>(curr_tn);
         recursive_analysis(fe->op0, srcp);
         recursive_analysis(fe->op1, srcp);
         recursive_analysis(fe->op2, srcp);
         break;
      }
      case gimple_while_K:
      {
         auto* we = GetPointer<gimple_while>(curr_tn);
         recursive_analysis(we->op0, srcp);
         break;
      }
      case gimple_phi_K:
      {
         auto* gp = GetPointer<gimple_phi>(curr_tn);
         for(auto def_edge_pair : gp->list_of_def_edge)
         {
            recursive_analysis(def_edge_pair.first, srcp);
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
         auto* tmr = GetPointer<target_mem_ref>(curr_tn);
         if(tmr->symbol)
            recursive_analysis(tmr->symbol, srcp);
         if(tmr->base)
            recursive_analysis(tmr->base, srcp);
         if(tmr->idx)
            recursive_analysis(tmr->idx, srcp);
         break;
      }
      case target_mem_ref461_K:
      {
         auto* tmr = GetPointer<target_mem_ref461>(curr_tn);
         if(tmr->base)
            recursive_analysis(tmr->base, srcp);
         if(tmr->idx)
            recursive_analysis(tmr->idx, srcp);
         if(tmr->idx2)
            recursive_analysis(tmr->idx2, srcp);
         break;
      }
      case string_cst_K:
      {
         if(string_cst_map.find(GET_INDEX_NODE(tn)) == string_cst_map.end())
         {
            auto* sc = GetPointer<string_cst>(curr_tn);
            const tree_manipulationRef tree_man = tree_manipulationRef(new tree_manipulation(TM, parameters));
            const auto* type_sc = GetPointer<const type_node>(GET_NODE(sc->type));
            const std::string local_var_name = "__bambu_artificial_var_string_cst_" + STR(GET_INDEX_NODE(tn));
            auto local_var_identifier = tree_man->create_identifier_node(local_var_name);
            auto global_scpe = tree_man->create_translation_unit_decl();
            auto new_var_decl = tree_man->create_var_decl(local_var_identifier, TM->CGetTreeReindex(GET_INDEX_NODE(sc->type)), global_scpe, TM->CGetTreeReindex(GET_INDEX_NODE(type_sc->size)), tree_nodeRef(), TM->CGetTreeReindex(GET_INDEX_NODE(tn)), srcp,
                                                          type_sc->algn, 1, true, -1, false, false, true, false, true);
            string_cst_map[GET_INDEX_NODE(tn)] = new_var_decl;
            tn = new_var_decl;
         }
         else
            tn = string_cst_map.find(GET_INDEX_NODE(tn))->second;
         break;
      }
      case real_cst_K:
      case complex_cst_K:
      case integer_cst_K:
      case field_decl_K:
      case function_decl_K:
      case label_decl_K:
      case result_decl_K:
      case template_decl_K:
      case vector_cst_K:
      case void_cst_K:
      case tree_vec_K:
      case case_label_expr_K:
      case gimple_label_K:
      case gimple_asm_K:
      case gimple_goto_K:
      case gimple_pragma_K:
      case gimple_resx_K:
      case CASE_PRAGMA_NODES:
         break;
      case binfo_K:
      case block_K:
      case const_decl_K:
      case CASE_CPP_NODES:
      case gimple_bind_K:
      case gimple_predict_K:
      case identifier_node_K:
      case last_tree_K:
      case namespace_decl_K:
      case none_K:
      case placeholder_expr_K:
      case statement_list_K:
      case translation_unit_decl_K:
      case error_mark_K:
      case using_decl_K:
      case tree_reindex_K:
      case target_expr_K:
      {
         THROW_ERROR_CODE(NODE_NOT_YET_SUPPORTED_EC, "Not supported node: " + std::string(curr_tn->get_kind_text()));
         break;
      }
      default:
         THROW_UNREACHABLE("");
   }
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Analyzed recursively " + STR(GET_INDEX_NODE(tn)) + ": " + STR(tn));
   return;
}
