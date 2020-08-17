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
 * @file var_decl_fix.cpp
 * @brief Pre-analysis step fixing var_decl duplication.
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 * @author Marco Lattuada <lattuada@elet.polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *
 */

/// Autoheader include
#include "config_HAVE_BAMBU_BUILT.hpp"

/// Header include
#include "var_decl_fix.hpp"

/// Behavior include
#include "application_manager.hpp"
#include "function_behavior.hpp"

/// Parameter include
#include "Parameter.hpp"

/// parser/treegcc include
#include "token_interface.hpp"

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
#include "tree_node.hpp"
#include "tree_reindex.hpp"

/// Utility include
#include "dbgPrintHelper.hpp"
#include "exceptions.hpp"
#include "string_manipulation.hpp" // for GET_CLASS

VarDeclFix::VarDeclFix(const application_managerRef _AppM, unsigned int _function_id, const DesignFlowManagerConstRef _design_flow_manager, const ParameterConstRef _parameters, const FrontendFlowStepType _frontend_flow_step_type)
    : FunctionFrontendFlowStep(_AppM, _function_id, _frontend_flow_step_type, _design_flow_manager, _parameters)
{
   debug_level = parameters->get_class_debug_level(GET_CLASS(*this), DEBUG_LEVEL_NONE);
}

VarDeclFix::~VarDeclFix() = default;

const CustomUnorderedSet<std::pair<FrontendFlowStepType, FrontendFlowStep::FunctionRelationship>> VarDeclFix::ComputeFrontendRelationships(const DesignFlowStep::RelationshipType relationship_type) const
{
   CustomUnorderedSet<std::pair<FrontendFlowStepType, FunctionRelationship>> relationships;
   switch(relationship_type)
   {
      case(DEPENDENCE_RELATIONSHIP):
      {
         relationships.insert(std::make_pair(CHECK_SYSTEM_TYPE, SAME_FUNCTION));
         break;
      }
      case(INVALIDATION_RELATIONSHIP):
      {
         break;
      }
      case(PRECEDENCE_RELATIONSHIP):
      {
#if HAVE_BAMBU_BUILT
         relationships.insert(std::make_pair(PARM_DECL_TAKEN_ADDRESS, SAME_FUNCTION));
         relationships.insert(std::make_pair(INTERFACE_INFER, SAME_FUNCTION));
#endif
         break;
      }
      default:
      {
         THROW_UNREACHABLE("");
      }
   }
   return relationships;
}

DesignFlowStep_Status VarDeclFix::InternalExec()
{
   const tree_managerRef TM = AppM->get_tree_manager();
   const tree_nodeRef curr_tn = TM->GetTreeNode(function_id);
   auto* fd = GetPointer<function_decl>(curr_tn);
   auto* sl = GetPointer<statement_list>(GET_NODE(fd->body));
   /// Already considered decl_node
   CustomUnorderedSet<unsigned int> already_examinated_decls;

   /// Already found variable and parameter names
   CustomUnorderedSet<std::string> already_examinated_names;

   /// Already found type names
   CustomUnorderedSet<std::string> already_examinated_type_names;

   /// Already visited address expression (used to avoid infinite recursion)
   CustomUnorderedSet<unsigned int> already_visited_ae;

   for(const auto& arg : fd->list_of_args)
      recursive_examinate(arg, already_examinated_decls, already_examinated_names, already_examinated_type_names, already_visited_ae);

   std::map<unsigned int, blocRef>& blocks = sl->list_of_bloc;
   std::map<unsigned int, blocRef>::iterator it, it_end;

   it_end = blocks.end();
   for(it = blocks.begin(); it != it_end; ++it)
   {
      for(const auto& stmt : it->second->CGetStmtList())
      {
         recursive_examinate(stmt, already_examinated_decls, already_examinated_names, already_examinated_type_names, already_visited_ae);
      }
   }
   return DesignFlowStep_Status::SUCCESS;
}

void VarDeclFix::recursive_examinate(const tree_nodeRef& tn, CustomUnorderedSet<unsigned int>& already_examinated_decls, CustomUnorderedSet<std::string>& already_examinated_names, CustomUnorderedSet<std::string>& already_examinated_type_names,
                                     CustomUnorderedSet<unsigned int>& already_visited_ae)
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
         const call_expr* ce = GetPointer<call_expr>(curr_tn);
         const std::vector<tree_nodeRef>& args = ce->args;
         std::vector<tree_nodeRef>::const_iterator arg, arg_end = args.end();
         for(arg = args.begin(); arg != arg_end; ++arg)
         {
            recursive_examinate(*arg, already_examinated_decls, already_examinated_names, already_examinated_type_names, already_visited_ae);
         }
         break;
      }
      case gimple_call_K:
      {
         const gimple_call* ce = GetPointer<gimple_call>(curr_tn);
         const std::vector<tree_nodeRef>& args = ce->args;
         std::vector<tree_nodeRef>::const_iterator arg, arg_end = args.end();
         for(arg = args.begin(); arg != arg_end; ++arg)
         {
            recursive_examinate(*arg, already_examinated_decls, already_examinated_names, already_examinated_type_names, already_visited_ae);
         }
         break;
      }
      case gimple_assign_K:
      {
         auto* gm = GetPointer<gimple_assign>(curr_tn);
         recursive_examinate(gm->op0, already_examinated_decls, already_examinated_names, already_examinated_type_names, already_visited_ae);
         recursive_examinate(gm->op1, already_examinated_decls, already_examinated_names, already_examinated_type_names, already_visited_ae);
         if(gm->predicate)
            recursive_examinate(gm->predicate, already_examinated_decls, already_examinated_names, already_examinated_type_names, already_visited_ae);
         break;
      }
      case gimple_nop_K:
      {
         break;
      }
      case var_decl_K:
      case parm_decl_K:
      {
         if(already_examinated_decls.find(GET_INDEX_NODE(tn)) == already_examinated_decls.end())
         {
            already_examinated_decls.insert(GET_INDEX_NODE(tn));
            auto* dn = GetPointer<decl_node>(GET_NODE(tn));
            recursive_examinate(dn->type, already_examinated_decls, already_examinated_names, already_examinated_type_names, already_visited_ae);
            if(dn->name)
            {
               // check if the var_decl
               if(curr_tn->get_kind() == var_decl_K)
               { /* this is a variable declaration */
                  auto* cast_res = GetPointer<var_decl>(curr_tn);
                  if(cast_res->static_flag)
                  {
                     PRINT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "Found a static variable with identifier <" + GetPointer<identifier_node>(GET_NODE(dn->name))->strg + "> within function #" + STR(function_id));
                  }
               }

               std::string original_name = Normalize(GetPointer<identifier_node>(GET_NODE(dn->name))->strg);
               std::string name_id = tree_helper::normalized_ID(original_name);
               if(already_examinated_names.find(original_name) == already_examinated_names.end())
                  already_examinated_names.insert(original_name);
               else
               {
                  PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, name_id + " is a duplicated var_decl");
                  /// create a new identifier_node tree node
                  std::map<TreeVocabularyTokenTypes_TokenEnum, std::string> IR_schema;
                  unsigned int var_decl_name_nid_test;
                  unsigned var_decl_unique_id = 0;
                  do
                  {
                     IR_schema[TOK(TOK_STRG)] = name_id + STR(var_decl_unique_id++);
                     var_decl_name_nid_test = TM->find(identifier_node_K, IR_schema);
                  } while(var_decl_name_nid_test);
                  unsigned int var_decl_name_nid = TM->new_tree_node_id();
                  TM->create_tree_node(var_decl_name_nid, identifier_node_K, IR_schema);
                  IR_schema.clear();
                  tree_nodeRef tr_new_id = TM->GetTreeReindex(var_decl_name_nid);
                  dn->name = tr_new_id;
                  function_behavior->GetBehavioralHelper()->InvaildateVariableName(dn->index);
               }
            }
         }
         break;
      }
      case ssa_name_K:
      {
         const ssa_name* sn = GetPointer<ssa_name>(curr_tn);
         if(sn->var)
            recursive_examinate(sn->var, already_examinated_decls, already_examinated_names, already_examinated_type_names, already_visited_ae);
         break;
      }
      case tree_list_K:
      {
         tree_nodeRef current = tn;
         while(current)
         {
            recursive_examinate(GetPointer<tree_list>(GET_NODE(current))->valu, already_examinated_decls, already_examinated_names, already_examinated_type_names, already_visited_ae);
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
         const unary_expr* ue = GetPointer<unary_expr>(curr_tn);
         recursive_examinate(ue->op, already_examinated_decls, already_examinated_names, already_examinated_type_names, already_visited_ae);
         break;
      }
      case CASE_BINARY_EXPRESSION:
      {
         const binary_expr* be = GetPointer<binary_expr>(curr_tn);
         recursive_examinate(be->op0, already_examinated_decls, already_examinated_names, already_examinated_type_names, already_visited_ae);
         recursive_examinate(be->op1, already_examinated_decls, already_examinated_names, already_examinated_type_names, already_visited_ae);
         break;
      }
      case CASE_TERNARY_EXPRESSION:
      {
         const ternary_expr* te = GetPointer<ternary_expr>(curr_tn);
         recursive_examinate(te->op0, already_examinated_decls, already_examinated_names, already_examinated_type_names, already_visited_ae);
         if(te->op1)
            recursive_examinate(te->op1, already_examinated_decls, already_examinated_names, already_examinated_type_names, already_visited_ae);
         if(te->op2)
            recursive_examinate(te->op2, already_examinated_decls, already_examinated_names, already_examinated_type_names, already_visited_ae);
         break;
      }
      case CASE_QUATERNARY_EXPRESSION:
      {
         const quaternary_expr* qe = GetPointer<quaternary_expr>(curr_tn);
         recursive_examinate(qe->op0, already_examinated_decls, already_examinated_names, already_examinated_type_names, already_visited_ae);
         if(qe->op1)
            recursive_examinate(qe->op1, already_examinated_decls, already_examinated_names, already_examinated_type_names, already_visited_ae);
         if(qe->op2)
            recursive_examinate(qe->op2, already_examinated_decls, already_examinated_names, already_examinated_type_names, already_visited_ae);
         if(qe->op3)
            recursive_examinate(qe->op3, already_examinated_decls, already_examinated_names, already_examinated_type_names, already_visited_ae);
         break;
      }
      case lut_expr_K:
      {
         auto* le = GetPointer<lut_expr>(curr_tn);
         recursive_examinate(le->op0, already_examinated_decls, already_examinated_names, already_examinated_type_names, already_visited_ae);
         recursive_examinate(le->op1, already_examinated_decls, already_examinated_names, already_examinated_type_names, already_visited_ae);
         if(le->op2)
            recursive_examinate(le->op2, already_examinated_decls, already_examinated_names, already_examinated_type_names, already_visited_ae);
         if(le->op3)
            recursive_examinate(le->op3, already_examinated_decls, already_examinated_names, already_examinated_type_names, already_visited_ae);
         if(le->op4)
            recursive_examinate(le->op4, already_examinated_decls, already_examinated_names, already_examinated_type_names, already_visited_ae);
         if(le->op5)
            recursive_examinate(le->op5, already_examinated_decls, already_examinated_names, already_examinated_type_names, already_visited_ae);
         if(le->op6)
            recursive_examinate(le->op6, already_examinated_decls, already_examinated_names, already_examinated_type_names, already_visited_ae);
         if(le->op7)
            recursive_examinate(le->op7, already_examinated_decls, already_examinated_names, already_examinated_type_names, already_visited_ae);
         if(le->op8)
            recursive_examinate(le->op8, already_examinated_decls, already_examinated_names, already_examinated_type_names, already_visited_ae);
         break;
      }
      case constructor_K:
      {
         const constructor* co = GetPointer<constructor>(curr_tn);
         const std::vector<std::pair<tree_nodeRef, tree_nodeRef>>& list_of_idx_valu = co->list_of_idx_valu;
         std::vector<std::pair<tree_nodeRef, tree_nodeRef>>::const_iterator it, it_end = list_of_idx_valu.end();
         for(it = list_of_idx_valu.begin(); it != it_end; ++it)
         {
            recursive_examinate(it->second, already_examinated_decls, already_examinated_names, already_examinated_type_names, already_visited_ae);
         }
         break;
      }
      case gimple_cond_K:
      {
         const gimple_cond* gc = GetPointer<gimple_cond>(curr_tn);
         recursive_examinate(gc->op0, already_examinated_decls, already_examinated_names, already_examinated_type_names, already_visited_ae);
         break;
      }
      case gimple_switch_K:
      {
         const gimple_switch* se = GetPointer<gimple_switch>(curr_tn);
         recursive_examinate(se->op0, already_examinated_decls, already_examinated_names, already_examinated_type_names, already_visited_ae);
         break;
      }
      case gimple_multi_way_if_K:
      {
         auto* gmwi = GetPointer<gimple_multi_way_if>(curr_tn);
         for(const auto& cond : gmwi->list_of_cond)
            if(cond.first)
               recursive_examinate(cond.first, already_examinated_decls, already_examinated_names, already_examinated_type_names, already_visited_ae);
         break;
      }
      case gimple_return_K:
      {
         const gimple_return* re = GetPointer<gimple_return>(curr_tn);
         if(re->op)
            recursive_examinate(re->op, already_examinated_decls, already_examinated_names, already_examinated_type_names, already_visited_ae);
         break;
      }
      case gimple_for_K:
      {
         const gimple_for* fe = GetPointer<gimple_for>(curr_tn);
         recursive_examinate(fe->op0, already_examinated_decls, already_examinated_names, already_examinated_type_names, already_visited_ae);
         recursive_examinate(fe->op1, already_examinated_decls, already_examinated_names, already_examinated_type_names, already_visited_ae);
         recursive_examinate(fe->op2, already_examinated_decls, already_examinated_names, already_examinated_type_names, already_visited_ae);
         break;
      }
      case gimple_while_K:
      {
         const gimple_while* we = GetPointer<gimple_while>(curr_tn);
         recursive_examinate(we->op0, already_examinated_decls, already_examinated_names, already_examinated_type_names, already_visited_ae);
         break;
      }
      case CASE_TYPE_NODES:
      {
         if(already_examinated_decls.find(GET_INDEX_NODE(tn)) == already_examinated_decls.end())
         {
            already_examinated_decls.insert(GET_INDEX_NODE(tn));
            auto* ty = GetPointer<type_node>(GET_NODE(tn));
            if(ty && ty->name && GET_NODE(ty->name)->get_kind() == type_decl_K)
            {
               recursive_examinate(ty->name, already_examinated_decls, already_examinated_names, already_examinated_type_names, already_visited_ae);
            }
            else if(ty and (not ty->system_flag) and ty->name and GET_NODE(ty->name)->get_kind() == identifier_node_K)
            {
               std::string name_id = GetPointer<identifier_node>(GET_NODE(ty->name))->strg;
               if(already_examinated_type_names.find(name_id) == already_examinated_type_names.end())
               {
                  already_examinated_type_names.insert(name_id);
               }
               else
               {
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---" + name_id + " is a duplicated type");
                  /// create a new identifier_node tree node
                  std::map<TreeVocabularyTokenTypes_TokenEnum, std::string> IR_schema;
                  unsigned int var_decl_name_nid_test;
                  unsigned var_decl_unique_id = 0;
                  do
                  {
                     IR_schema[TOK(TOK_STRG)] = name_id + STR(var_decl_unique_id++);
                     var_decl_name_nid_test = TM->find(identifier_node_K, IR_schema);
                  } while(var_decl_name_nid_test);
                  unsigned int var_decl_name_nid = TM->new_tree_node_id();
                  TM->create_tree_node(var_decl_name_nid, identifier_node_K, IR_schema);
                  IR_schema.clear();
                  tree_nodeRef tr_new_id = TM->GetTreeReindex(var_decl_name_nid);
                  ty->name = tr_new_id;
               }
            }
         }
         break;
      }
      case type_decl_K:
      {
         auto* td = GetPointer<type_decl>(GET_NODE(tn));
         if(td and td->name and td->include_name != "<built-in>" and (not td->operating_system_flag) and (not td->library_system_flag))
         {
            std::string name_id = GetPointer<identifier_node>(GET_NODE(td->name))->strg;
            if(already_examinated_type_names.find(name_id) == already_examinated_type_names.end())
               already_examinated_type_names.insert(name_id);
            else
            {
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---" + name_id + " is a duplicated type");
               /// create a new identifier_node tree node
               std::map<TreeVocabularyTokenTypes_TokenEnum, std::string> IR_schema;
               unsigned int var_decl_name_nid_test;
               unsigned var_decl_unique_id = 0;
               do
               {
                  IR_schema[TOK(TOK_STRG)] = name_id + STR(var_decl_unique_id++);
                  var_decl_name_nid_test = TM->find(identifier_node_K, IR_schema);
               } while(var_decl_name_nid_test);
               unsigned int var_decl_name_nid = TM->new_tree_node_id();
               TM->create_tree_node(var_decl_name_nid, identifier_node_K, IR_schema);
               IR_schema.clear();
               tree_nodeRef tr_new_id = TM->GetTreeReindex(var_decl_name_nid);
               td->name = tr_new_id;
            }
         }
         break;
      }
      case target_mem_ref_K:
      {
         const target_mem_ref* tmr = GetPointer<target_mem_ref>(curr_tn);
         if(tmr->symbol)
            recursive_examinate(tmr->symbol, already_examinated_decls, already_examinated_names, already_examinated_type_names, already_visited_ae);
         if(tmr->base)
            recursive_examinate(tmr->base, already_examinated_decls, already_examinated_names, already_examinated_type_names, already_visited_ae);
         if(tmr->idx)
            recursive_examinate(tmr->idx, already_examinated_decls, already_examinated_names, already_examinated_type_names, already_visited_ae);
         break;
      }
      case target_mem_ref461_K:
      {
         const target_mem_ref461* tmr = GetPointer<target_mem_ref461>(curr_tn);
         if(tmr->base)
            recursive_examinate(tmr->base, already_examinated_decls, already_examinated_names, already_examinated_type_names, already_visited_ae);
         if(tmr->idx)
            recursive_examinate(tmr->idx, already_examinated_decls, already_examinated_names, already_examinated_type_names, already_visited_ae);
         if(tmr->idx2)
            recursive_examinate(tmr->idx2, already_examinated_decls, already_examinated_names, already_examinated_type_names, already_visited_ae);
         break;
      }
      case real_cst_K:
      case complex_cst_K:
      case string_cst_K:
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
      case gimple_phi_K:
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

const std::string VarDeclFix::Normalize(const std::string& identifier) const
{
   return identifier;
}
