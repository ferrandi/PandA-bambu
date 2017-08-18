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
 *              Copyright (c) 2004-2017 Politecnico di Milano
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
 * @file hls_div_cg_ext.cpp
 * @brief Step that extends the call graph with the soft-float calls where appropriate.
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 *
*/

///Header include
#include "hls_div_cg_ext.hpp"

///design_flows include
#include "design_flow_graph.hpp"
#include "design_flow_manager.hpp"
#include "design_flow_step.hpp"

///frontend_analysis
#include "symbolic_application_frontend_flow_step.hpp"

///Behavior include
#include "application_manager.hpp"
#include "call_graph.hpp"
#include "call_graph_manager.hpp"
#include "function_behavior.hpp"
#include "behavioral_helper.hpp"

///Graph include
#include "basic_block.hpp"
#include "basic_blocks_graph_constructor.hpp"

///Parameter include
#include "Parameter.hpp"

///STD include
#include <fstream>

///STL include
#include <map>
#include <string>

///Tree include
#include "ext_tree_node.hpp"
#include "tree_basic_block.hpp"
#include "tree_manager.hpp"
#include "tree_node.hpp"
#include "tree_reindex.hpp"
#include "tree_helper.hpp"
#include "tree_manipulation.hpp"

///Utility include
#include "dbgPrintHelper.hpp"
#include "exceptions.hpp"
#include "math_function.hpp"


hls_div_cg_ext::hls_div_cg_ext(const ParameterConstRef _parameters, const application_managerRef _AppM, unsigned int _function_id, const DesignFlowManagerConstRef _design_flow_manager) :
   FunctionFrontendFlowStep(_AppM, _function_id, HLS_DIV_CG_EXT, _design_flow_manager, _parameters),
   TreeM(_AppM->get_tree_manager()),
   tree_man(new tree_manipulation(TreeM, parameters)),
   already_executed(false),
   changed_call_graph(false)
{
   debug_level = parameters->get_class_debug_level(GET_CLASS(*this), DEBUG_LEVEL_NONE);
}

void hls_div_cg_ext::Initialize()
{
   changed_call_graph = false;
}

hls_div_cg_ext::~hls_div_cg_ext()
{}

const std::unordered_set<std::pair<FrontendFlowStepType, FrontendFlowStep::FunctionRelationship> > hls_div_cg_ext::ComputeFrontendRelationships(const DesignFlowStep::RelationshipType relationship_type) const
{
   std::unordered_set<std::pair<FrontendFlowStepType, FunctionRelationship> > relationships;
   switch (relationship_type)
   {
      case (PRECEDENCE_RELATIONSHIP):
      {
         break;
      }
      case DEPENDENCE_RELATIONSHIP:
      {
         relationships.insert(std::make_pair(BIT_VALUE, SAME_FUNCTION));
         relationships.insert(std::make_pair(IR_LOWERING, SAME_FUNCTION));
         relationships.insert(std::make_pair(USE_COUNTING, SAME_FUNCTION));
         break;
      }
      case(INVALIDATION_RELATIONSHIP) :
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

DesignFlowStep_Status hls_div_cg_ext::InternalExec()
{
   const tree_nodeRef curr_tn = TreeM->GetTreeNode(function_id);
   function_decl * fd = GetPointer<function_decl>(curr_tn);
   statement_list * sl = GetPointer<statement_list>(GET_NODE(fd->body));

   std::map<unsigned int, blocRef> &blocks = sl->list_of_bloc;
   std::map<unsigned int, blocRef>::iterator it, it_end;

   it_end = blocks.end();
   for(it = blocks.begin(); it != it_end; it++)
   {
      for(const auto stmt : it->second->CGetStmtList())
      {
         recursive_examinate(stmt, stmt);
      }
   }
   already_executed = true;
   changed_call_graph ? function_behavior->UpdateBBVersion() : 0;
   return changed_call_graph ? DesignFlowStep_Status::SUCCESS : DesignFlowStep_Status::UNCHANGED;
}

void hls_div_cg_ext::recursive_examinate(const tree_nodeRef & current_tree_node, const tree_nodeRef & current_statement)
{
   THROW_ASSERT(current_tree_node->get_kind() == tree_reindex_K, "Node is not a tree reindex");
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Update recursively " + boost::lexical_cast<std::string>(GET_INDEX_NODE(current_tree_node)) + " " + GET_NODE(current_tree_node)->ToString());
   const tree_nodeRef curr_tn = GET_NODE(current_tree_node);
   const std::string current_srcp = [curr_tn] () -> std::string
   {
      const auto srcp_tn = GetPointer<const srcp>(curr_tn);
      if(srcp_tn)
      {
         return srcp_tn->include_name + ":" + STR(srcp_tn->line_number) + ":" + STR(srcp_tn->column_number);
      }
      return "";
   }();
   switch(curr_tn->get_kind())
   {
      case call_expr_K:
      case aggr_init_expr_K:
      {
         const call_expr * ce = GetPointer<call_expr>(curr_tn);
         const std::vector<tree_nodeRef> & args = ce->args;
         std::vector<tree_nodeRef>::const_iterator arg, arg_end = args.end();
         unsigned int parm_index = 0;
         for(arg = args.begin(); arg != arg_end; arg++)
         {
            recursive_examinate(*arg, current_statement);
            ++parm_index;
         }
         break;
      }
      case gimple_call_K:
      {
         const gimple_call * ce = GetPointer<gimple_call>(curr_tn);
         const std::vector<tree_nodeRef> & args = ce->args;
         std::vector<tree_nodeRef>::const_iterator arg, arg_end = args.end();
         unsigned int parm_index = 0;
         for(arg = args.begin(); arg != arg_end; arg++)
         {
            recursive_examinate(*arg, current_statement);
            ++parm_index;
         }
         break;
      }
      case gimple_assign_K:
      {
         gimple_assign * gm = GetPointer<gimple_assign>(curr_tn);
         recursive_examinate(gm->op0, current_statement);
         recursive_examinate(gm->op1, current_statement);
         if(gm->predicate)
            recursive_examinate(gm->predicate, current_statement);
         break;
      }
      case gimple_nop_K:
      {
         break;
      }
      case var_decl_K:
      case parm_decl_K:
      case ssa_name_K:
      {
         break;
      }
      case tree_list_K:
      {
         tree_nodeRef current = current_tree_node;
         while(current)
         {
            recursive_examinate(GetPointer<tree_list>(GET_NODE(current))->valu, current_statement);
            current = GetPointer<tree_list>(GET_NODE(current))->chan;
         }
         break;
      }
      case CASE_UNARY_EXPRESSION :
      {
         const unary_expr * ue = GetPointer<unary_expr>(curr_tn);
         recursive_examinate(ue->op, current_statement);
         break;
      }
      case CASE_BINARY_EXPRESSION :
      {
         const binary_expr * be = GetPointer<binary_expr>(curr_tn);
         recursive_examinate(be->op0, current_statement);
         recursive_examinate(be->op1, current_statement);
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wswitch-enum"
         switch(curr_tn->get_kind())
         {
            case exact_div_expr_K:
            case trunc_div_expr_K:
            case trunc_mod_expr_K:
            {
               std::string fu_suffix;
               unsigned int expr_type_index;
               tree_nodeRef expr_type = tree_helper::get_type_node(GET_NODE(be->op0), expr_type_index);
               unsigned int bitsize0 = resize_to_1_8_16_32_64_128_256_512(tree_helper::Size(GET_NODE(be->op0)));
               unsigned int bitsize1 = resize_to_1_8_16_32_64_128_256_512(tree_helper::Size(GET_NODE(be->op1)));
               unsigned int bitsize = std::max(bitsize0,bitsize1);

               bool is_constant_second_par = false;

               if(GetPointer<integer_cst>(GET_NODE(be->op1)))
               {
                  integer_cst * ic = GetPointer<integer_cst>(GET_NODE(be->op1));
                  if((ic->value & (ic->value - 1)) == 0)
                     is_constant_second_par = true;
               }

               if(!is_constant_second_par && expr_type->get_kind() == integer_type_K && (bitsize == 32 || bitsize == 64))
               {
                  switch(curr_tn->get_kind())
                  {
                     case exact_div_expr_K:
                     case trunc_div_expr_K:
                     {
                        fu_suffix = "div";
                        break;
                     }
                     case trunc_mod_expr_K:
                     {
                        fu_suffix = "mod";
                        break;
                     }
                     default:
                        break;
                  }
                  std::string bitsize_str = bitsize == 32 ? "s" : "d";
                  bool unsignedp = tree_helper::is_unsigned(TreeM, expr_type_index);
                  std::string fu_name = STR("__")+(unsignedp?"u":"")+fu_suffix+bitsize_str+"i3"+((bitsize0==64&&bitsize1==32)?"6432":"");
                  unsigned int called_function_id = TreeM->function_index(fu_name);
                  THROW_ASSERT(called_function_id, "The library miss this function " + fu_name);
                  THROW_ASSERT(AppM->GetFunctionBehavior(called_function_id)->GetBehavioralHelper()->has_implementation(), "inconsistent behavioral helper");
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Adding call to " + fu_name);
                  std::vector<tree_nodeRef> args;
                  args.push_back(be->op0);
                  args.push_back(be->op1);
                  TreeM->ReplaceTreeNode(current_statement, current_tree_node, tree_man->CreateCallExpr(TreeM->GetTreeReindex(called_function_id), args, current_srcp));

                  AppM->GetCallGraphManager()->AddCallPoint(function_id, called_function_id, current_statement->index, FunctionEdgeInfo::CallType::direct_call);
                  const std::set<unsigned int> called_by_set = AppM->CGetCallGraphManager()->get_called_by(function_id);
                  if(called_by_set.find(called_function_id) == called_by_set.end())
                  {
                     changed_call_graph = true;
                  }
               }
               break;
            }
            default:
               break;
         }
#pragma GCC diagnostic pop
         break;
      }
      case CASE_TERNARY_EXPRESSION :
      {
         const ternary_expr * te = GetPointer<ternary_expr>(curr_tn);
         recursive_examinate(te->op0, current_statement);
         if(te->op1)
            recursive_examinate(te->op1, current_statement);
         if(te->op2)
            recursive_examinate(te->op2, current_statement);
         break;
      }
      case CASE_QUATERNARY_EXPRESSION :
      {
         const quaternary_expr * qe = GetPointer<quaternary_expr>(curr_tn);
         recursive_examinate(qe->op0, current_statement);
         if(qe->op1)
            recursive_examinate(qe->op1, current_statement);
         if(qe->op2)
            recursive_examinate(qe->op2, current_statement);
         if(qe->op3)
            recursive_examinate(qe->op3, current_statement);
         break;
      }
      case constructor_K:
      {
         const constructor * co = GetPointer<constructor>(curr_tn);
         const std::vector<std::pair< tree_nodeRef, tree_nodeRef> > & list_of_idx_valu = co->list_of_idx_valu;
         std::vector<std::pair< tree_nodeRef, tree_nodeRef> >::const_iterator it, it_end = list_of_idx_valu.end();
         for(it = list_of_idx_valu.begin(); it != it_end; it++)
         {
            recursive_examinate(it->second, current_statement);
         }
         break;
      }
      case gimple_cond_K:
      {
         const gimple_cond * gc = GetPointer<gimple_cond>(curr_tn);
         recursive_examinate(gc->op0, current_statement);
         break;
      }
      case gimple_switch_K:
      {
         const gimple_switch * se = GetPointer<gimple_switch>(curr_tn);
         recursive_examinate(se->op0, current_statement);
         break;
      }
      case gimple_multi_way_if_K:
      {
         gimple_multi_way_if* gmwi=GetPointer<gimple_multi_way_if>(curr_tn);
         for(auto cond : gmwi->list_of_cond)
            if(cond.first)
               recursive_examinate(cond.first, current_statement);
         break;
      }
      case gimple_return_K:
      {
         const gimple_return * re = GetPointer<gimple_return>(curr_tn);
         if(re->op)
            recursive_examinate(re->op, current_statement);
         break;
      }
      case gimple_for_K:
      {
         const gimple_for * gf = GetPointer<const gimple_for>(curr_tn);
         recursive_examinate(gf->op0, current_statement);
         recursive_examinate(gf->op1, current_statement);
         recursive_examinate(gf->op2, current_statement);
         break;
      }
      case gimple_while_K:
      {
         const gimple_while * gw = GetPointer<gimple_while>(curr_tn);
         recursive_examinate(gw->op0, current_statement);
         break;
      }
      case CASE_TYPE_NODES:
      case type_decl_K:
      {
         break;
      }
      case target_mem_ref_K:
      {
         const target_mem_ref * tmr = GetPointer<target_mem_ref>(curr_tn);
         if(tmr->symbol)
            recursive_examinate(tmr->symbol, current_statement);
         if(tmr->base)
            recursive_examinate(tmr->base, current_statement);
         if(tmr->idx)
            recursive_examinate(tmr->idx, current_statement);
         break;
      }
      case target_mem_ref461_K:
      {
         const target_mem_ref461 * tmr = GetPointer<target_mem_ref461>(curr_tn);
         if(tmr->base)
            recursive_examinate(tmr->base, current_statement);
         if(tmr->idx)
            recursive_examinate(tmr->idx, current_statement);
         if(tmr->idx2)
            recursive_examinate(tmr->idx2, current_statement);
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
      case vector_cst_K:
      case void_cst_K:
      case tree_vec_K:
      case case_label_expr_K:
      case gimple_label_K:
      case gimple_asm_K:
      case gimple_goto_K:
      case CASE_PRAGMA_NODES:
      case gimple_pragma_K:
         break;
      case binfo_K:
      case block_K:
      case const_decl_K:
      case CASE_CPP_NODES:
      case gimple_bind_K:
      case gimple_phi_K:
      case gimple_predict_K:
      case gimple_resx_K:
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
      {
         THROW_UNREACHABLE("");
      }
   }
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Updated recursively " + boost::lexical_cast<std::string>(GET_INDEX_NODE(current_tree_node)) + " " + GET_NODE(current_tree_node)->ToString());
   return;
}

bool hls_div_cg_ext::HasToBeExecuted() const
{
   return not already_executed;
}

