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
 * @file hls_div_cg_ext.cpp
 * @brief Step that extends the call graph with the soft-float calls where appropriate.
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 *
 */

/// Header include
#include "hls_div_cg_ext.hpp"

/// design_flows include
#include "design_flow_graph.hpp"
#include "design_flow_manager.hpp"
#include "design_flow_step.hpp"

/// frontend_analysis
#include "symbolic_application_frontend_flow_step.hpp"

/// Behavior include
#include "application_manager.hpp"
#include "behavioral_helper.hpp"
#include "call_graph.hpp"
#include "call_graph_manager.hpp"
#include "function_behavior.hpp"
#include "hls_manager.hpp"
#include "hls_target.hpp"

/// Graph include
#include "basic_block.hpp"
#include "basic_blocks_graph_constructor.hpp"

/// Parameter include
#include "Parameter.hpp"

/// STD include
#include <fstream>

/// STL include
#include "custom_map.hpp"
#include <string>

/// Tree include
#include "ext_tree_node.hpp"
#include "token_interface.hpp"
#include "tree_basic_block.hpp"
#include "tree_helper.hpp"
#include "tree_manager.hpp"
#include "tree_manipulation.hpp"
#include "tree_node.hpp"
#include "tree_reindex.hpp"

/// Utility include
#include "dbgPrintHelper.hpp"
#include "exceptions.hpp"
#include "math_function.hpp"
#include "string_manipulation.hpp" // for GET_CLASS

hls_div_cg_ext::hls_div_cg_ext(const ParameterConstRef _parameters, const application_managerRef _AppM, unsigned int _function_id, const DesignFlowManagerConstRef _design_flow_manager)
    : FunctionFrontendFlowStep(_AppM, _function_id, HLS_DIV_CG_EXT, _design_flow_manager, _parameters), TreeM(_AppM->get_tree_manager()), already_executed(false), changed_call_graph(false), fix_nop(false), use64bitMul(false)
{
   debug_level = parameters->get_class_debug_level(GET_CLASS(*this), DEBUG_LEVEL_NONE);
}

void hls_div_cg_ext::Initialize()
{
   changed_call_graph = false;
   fix_nop = false;
}

hls_div_cg_ext::~hls_div_cg_ext() = default;

const CustomUnorderedSet<std::pair<FrontendFlowStepType, FrontendFlowStep::FunctionRelationship>> hls_div_cg_ext::ComputeFrontendRelationships(const DesignFlowStep::RelationshipType relationship_type) const
{
   CustomUnorderedSet<std::pair<FrontendFlowStepType, FunctionRelationship>> relationships;
   switch(relationship_type)
   {
      case(PRECEDENCE_RELATIONSHIP):
      {
         relationships.insert(std::make_pair(MEM_CG_EXT, SAME_FUNCTION));
         break;
      }
      case DEPENDENCE_RELATIONSHIP:
      {
         if(not parameters->getOption<int>(OPT_gcc_openmp_simd))
            relationships.insert(std::make_pair(BIT_VALUE, SAME_FUNCTION));
         relationships.insert(std::make_pair(IR_LOWERING, SAME_FUNCTION));
         relationships.insert(std::make_pair(USE_COUNTING, SAME_FUNCTION));
         break;
      }
      case(INVALIDATION_RELATIONSHIP):
      {
         switch(GetStatus())
         {
            case DesignFlowStep_Status::SUCCESS:
            {
               relationships.insert(std::make_pair(MEM_CG_EXT, SAME_FUNCTION));
               break;
            }
            case DesignFlowStep_Status::SKIPPED:
            case DesignFlowStep_Status::UNCHANGED:
            case DesignFlowStep_Status::UNEXECUTED:
            case DesignFlowStep_Status::UNNECESSARY:
            {
               break;
            }
            case DesignFlowStep_Status::ABORTED:
            case DesignFlowStep_Status::EMPTY:
            case DesignFlowStep_Status::NONEXISTENT:
            default:
               THROW_UNREACHABLE("");
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

DesignFlowStep_Status hls_div_cg_ext::InternalExec()
{
   const tree_manipulationRef tree_man(new tree_manipulation(TreeM, parameters));

   const tree_nodeRef curr_tn = TreeM->GetTreeNode(function_id);
   auto* fd = GetPointer<function_decl>(curr_tn);
   auto* sl = GetPointer<statement_list>(GET_NODE(fd->body));

   std::map<unsigned int, blocRef>& blocks = sl->list_of_bloc;
   std::map<unsigned int, blocRef>::iterator it, it_end;

   THROW_ASSERT(GetPointer<const HLS_manager>(AppM)->get_HLS_target(), "unexpected condition");
   const auto hls_target = GetPointer<const HLS_manager>(AppM)->get_HLS_target();
   if(hls_target->get_target_device()->has_parameter("use_soft_64_mul") && hls_target->get_target_device()->get_parameter<size_t>("use_soft_64_mul"))
   {
      use64bitMul = true;
   }

   it_end = blocks.end();
   for(it = blocks.begin(); it != it_end; ++it)
   {
      for(const auto& stmt : it->second->CGetStmtList())
      {
         recursive_examinate(stmt, stmt, tree_man);
      }
   }

   if(fix_nop)
   {
      for(it = blocks.begin(); it != it_end; ++it)
      {
         const auto& list_of_stmt = it->second->CGetStmtList();
         auto it_los_end = list_of_stmt.end();
         auto it_los = list_of_stmt.begin();
         while(it_los != it_los_end)
         {
            if(GET_NODE(*it_los)->get_kind() == gimple_assign_K)
            {
               auto* ga = GetPointer<gimple_assign>(GET_NODE(*it_los));
               const std::string srcp_default = ga->include_name + ":" + STR(ga->line_number) + ":" + STR(ga->column_number);
               if(GET_NODE(ga->op1)->get_kind() == nop_expr_K)
               {
                  auto* ue = GetPointer<unary_expr>(GET_NODE(ga->op1));
                  if(GET_NODE(ue->op)->get_kind() == call_expr_K)
                  {
                     unsigned int arg_n = 0;
                     auto ce = GetPointer<call_expr>(GET_NODE(ue->op));
                     auto arg_it = ce->args.begin();
                     for(; arg_it != ce->args.end(); arg_it++, arg_n++)
                     {
                        if(GET_NODE(*arg_it)->get_kind() == integer_cst_K or GET_NODE(*arg_it)->get_kind() == ssa_name_K)
                        {
                           unsigned int formal_type_id = tree_helper::get_formal_ith(TreeM, ce->index, arg_n);
                           const tree_nodeConstRef formal_type_node = TreeM->get_tree_node_const(formal_type_id);
                           const tree_nodeConstRef actual_type_node = tree_helper::CGetType(GET_NODE(*arg_it));
                           if(formal_type_id != actual_type_node->index)
                           {
                              const auto ga_nop = tree_man->CreateNopExpr(*arg_it, TreeM->CGetTreeReindex(formal_type_node->index), tree_nodeRef(), tree_nodeRef());
                              INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---adding statement " + GET_NODE(ga_nop)->ToString());
                              it->second->PushBefore(ga_nop, *it_los);
                              INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---old call statement " + GET_NODE(*it_los)->ToString());
                              unsigned int k = 0;
                              auto new_ssa = GetPointer<gimple_assign>(GET_NODE(ga_nop))->op0;
                              auto tmp_arg_it = ce->args.begin();
                              for(; tmp_arg_it != ce->args.end(); tmp_arg_it++, k++)
                              {
                                 if(GET_INDEX_NODE(*arg_it) == GET_INDEX_NODE(*tmp_arg_it) and tree_helper::get_formal_ith(TreeM, ce->index, k) == formal_type_id)
                                 {
                                    TreeM->RecursiveReplaceTreeNode(*tmp_arg_it, *tmp_arg_it, new_ssa, *it_los, false);
                                    tmp_arg_it = std::next(ce->args.begin(), static_cast<int>(k));
                                    arg_it = std::next(ce->args.begin(), static_cast<int>(arg_n));
                                    continue;
                                 }
                              }
                              INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---new call statement " + GET_NODE(*it_los)->ToString());
                              THROW_ASSERT(k, "");
                           }
                        }
                     }
                     unsigned int type_index = tree_helper::get_type_index(TreeM, GET_INDEX_NODE(ue->op));
                     tree_nodeRef op_type = TreeM->GetTreeReindex(type_index);
                     tree_nodeRef op_ga = tree_man->CreateGimpleAssign(op_type, tree_nodeRef(), tree_nodeRef(), ue->op, it->first, srcp_default);
                     tree_nodeRef op_vd = GetPointer<gimple_assign>(GET_NODE(op_ga))->op0;
                     it->second->PushBefore(op_ga, *it_los);
                     INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---adding statement " + GET_NODE(op_ga)->ToString());
                     TreeM->ReplaceTreeNode(*it_los, ue->op, op_vd);
                     unsigned int called_function_id = GET_INDEX_NODE(GetPointer<addr_expr>(GET_NODE(ce->fn))->op);
                     AppM->GetCallGraphManager()->RemoveCallPoint(function_id, called_function_id, ga->index);
                     AppM->GetCallGraphManager()->AddCallPoint(function_id, called_function_id, op_ga->index, FunctionEdgeInfo::CallType::direct_call);
                  }
               }
            }
            it_los++;
         }
      }
   }
   already_executed = true;
   changed_call_graph ? function_behavior->UpdateBBVersion() : 0;
   return changed_call_graph ? DesignFlowStep_Status::SUCCESS : DesignFlowStep_Status::UNCHANGED;
}

void hls_div_cg_ext::recursive_examinate(const tree_nodeRef& current_tree_node, const tree_nodeRef& current_statement, const tree_manipulationRef tree_man)
{
   THROW_ASSERT(current_tree_node->get_kind() == tree_reindex_K, "Node is not a tree reindex");
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Update recursively " + boost::lexical_cast<std::string>(GET_INDEX_NODE(current_tree_node)) + " " + GET_NODE(current_tree_node)->ToString());
   const tree_nodeRef curr_tn = GET_NODE(current_tree_node);
   const std::string current_srcp = [curr_tn]() -> std::string {
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
         const call_expr* ce = GetPointer<call_expr>(curr_tn);
         const std::vector<tree_nodeRef>& args = ce->args;
         std::vector<tree_nodeRef>::const_iterator arg, arg_end = args.end();
         unsigned int parm_index = 0;
         for(arg = args.begin(); arg != arg_end; ++arg)
         {
            recursive_examinate(*arg, current_statement, tree_man);
            ++parm_index;
         }
         break;
      }
      case gimple_call_K:
      {
         const gimple_call* ce = GetPointer<gimple_call>(curr_tn);
         const std::vector<tree_nodeRef>& args = ce->args;
         std::vector<tree_nodeRef>::const_iterator arg, arg_end = args.end();
         unsigned int parm_index = 0;
         for(arg = args.begin(); arg != arg_end; ++arg)
         {
            recursive_examinate(*arg, current_statement, tree_man);
            ++parm_index;
         }
         break;
      }
      case gimple_assign_K:
      {
         auto* gm = GetPointer<gimple_assign>(curr_tn);
         recursive_examinate(gm->op0, current_statement, tree_man);
         recursive_examinate(gm->op1, current_statement, tree_man);
         if(gm->predicate)
            recursive_examinate(gm->predicate, current_statement, tree_man);
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
            recursive_examinate(GetPointer<tree_list>(GET_NODE(current))->valu, current_statement, tree_man);
            current = GetPointer<tree_list>(GET_NODE(current))->chan;
         }
         break;
      }
      case CASE_UNARY_EXPRESSION:
      {
         const unary_expr* ue = GetPointer<unary_expr>(curr_tn);
         recursive_examinate(ue->op, current_statement, tree_man);
         break;
      }
      case CASE_BINARY_EXPRESSION:
      {
         const binary_expr* be = GetPointer<binary_expr>(curr_tn);
         recursive_examinate(be->op0, current_statement, tree_man);
         recursive_examinate(be->op1, current_statement, tree_man);
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
               unsigned int bitsize = std::max(bitsize0, bitsize1);

               bool is_constant_second_par = false;

               if(GetPointer<integer_cst>(GET_NODE(be->op1)))
               {
                  auto* ic = GetPointer<integer_cst>(GET_NODE(be->op1));
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
                  std::string fu_name = STR("__") + (unsignedp ? "u" : "") + fu_suffix + bitsize_str + "i3" + ((bitsize0 == 64 && bitsize1 == 32) ? "6432" : "");
                  unsigned int called_function_id = TreeM->function_index(fu_name);
                  THROW_ASSERT(called_function_id, "The library miss this function " + fu_name);
                  THROW_ASSERT(AppM->GetFunctionBehavior(called_function_id)->GetBehavioralHelper()->has_implementation(), "inconsistent behavioral helper");
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Adding call to " + fu_name);
                  std::vector<tree_nodeRef> args;
                  args.push_back(be->op0);
                  args.push_back(be->op1);
                  auto callExpr = tree_man->CreateCallExpr(TreeM->GetTreeReindex(called_function_id), args, current_srcp);
                  bool CEunsignedp = tree_helper::is_unsigned(TreeM, GET_INDEX_NODE(callExpr));
                  if(CEunsignedp != unsignedp)
                  {
                     std::map<TreeVocabularyTokenTypes_TokenEnum, std::string> ne_schema, ga_schema;
                     ne_schema[TOK(TOK_TYPE)] = STR(expr_type_index);
                     ne_schema[TOK(TOK_SRCP)] = "<built-in>:0:0";
                     ne_schema[TOK(TOK_OP)] = STR(callExpr->index);
                     const auto ne_id = TreeM->new_tree_node_id();
                     TreeM->create_tree_node(ne_id, nop_expr_K, ne_schema);
                     callExpr = TreeM->GetTreeReindex(ne_id);
                     fix_nop = true;
                  }
                  TreeM->ReplaceTreeNode(current_statement, current_tree_node, callExpr);

                  const CustomOrderedSet<unsigned int> called_by_set = AppM->CGetCallGraphManager()->get_called_by(function_id);
                  if(called_by_set.find(called_function_id) == called_by_set.end())
                  {
                     changed_call_graph = true;
                  }
                  AppM->GetCallGraphManager()->AddCallPoint(function_id, called_function_id, current_statement->index, FunctionEdgeInfo::CallType::direct_call);
               }
               break;
            }
            case mult_expr_K:
            {
               if(use64bitMul)
               {
                  unsigned int expr_type_index;
                  tree_nodeRef expr_type = tree_helper::get_type_node(GET_NODE(be->op0), expr_type_index);
                  unsigned int bitsize0 = resize_to_1_8_16_32_64_128_256_512(tree_helper::Size(GET_NODE(be->op0)));
                  unsigned int bitsize1 = resize_to_1_8_16_32_64_128_256_512(tree_helper::Size(GET_NODE(be->op1)));
                  unsigned int bitsize = std::max(bitsize0, bitsize1);
                  if(expr_type->get_kind() == integer_type_K && bitsize == 64)
                  {
                     bool unsignedp = tree_helper::is_unsigned(TreeM, expr_type_index);
                     std::string fu_name = "__umul64";
                     unsigned int called_function_id = TreeM->function_index(fu_name);
                     THROW_ASSERT(called_function_id, "The library miss this function " + fu_name);
                     THROW_ASSERT(AppM->GetFunctionBehavior(called_function_id)->GetBehavioralHelper()->has_implementation(), "inconsistent behavioral helper");
                     INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Adding call to " + fu_name);
                     std::vector<tree_nodeRef> args;
                     args.push_back(be->op0);
                     args.push_back(be->op1);
                     auto callExpr = tree_man->CreateCallExpr(TreeM->GetTreeReindex(called_function_id), args, current_srcp);
                     bool CEunsignedp = tree_helper::is_unsigned(TreeM, GET_INDEX_NODE(callExpr));
                     if(CEunsignedp != unsignedp)
                     {
                        std::map<TreeVocabularyTokenTypes_TokenEnum, std::string> ne_schema, ga_schema;
                        ne_schema[TOK(TOK_TYPE)] = STR(expr_type_index);
                        ne_schema[TOK(TOK_SRCP)] = "<built-in>:0:0";
                        ne_schema[TOK(TOK_OP)] = STR(callExpr->index);
                        const auto ne_id = TreeM->new_tree_node_id();
                        TreeM->create_tree_node(ne_id, nop_expr_K, ne_schema);
                        callExpr = TreeM->GetTreeReindex(ne_id);
                        fix_nop = true;
                     }
                     TreeM->ReplaceTreeNode(current_statement, current_tree_node, callExpr);

                     const CustomOrderedSet<unsigned int> called_by_set = AppM->CGetCallGraphManager()->get_called_by(function_id);
                     if(called_by_set.find(called_function_id) == called_by_set.end())
                     {
                        changed_call_graph = true;
                     }
                     AppM->GetCallGraphManager()->AddCallPoint(function_id, called_function_id, current_statement->index, FunctionEdgeInfo::CallType::direct_call);
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
      case CASE_TERNARY_EXPRESSION:
      {
         const ternary_expr* te = GetPointer<ternary_expr>(curr_tn);
         recursive_examinate(te->op0, current_statement, tree_man);
         if(te->op1)
            recursive_examinate(te->op1, current_statement, tree_man);
         if(te->op2)
            recursive_examinate(te->op2, current_statement, tree_man);
         break;
      }
      case CASE_QUATERNARY_EXPRESSION:
      {
         const quaternary_expr* qe = GetPointer<quaternary_expr>(curr_tn);
         recursive_examinate(qe->op0, current_statement, tree_man);
         if(qe->op1)
            recursive_examinate(qe->op1, current_statement, tree_man);
         if(qe->op2)
            recursive_examinate(qe->op2, current_statement, tree_man);
         if(qe->op3)
            recursive_examinate(qe->op3, current_statement, tree_man);
         break;
      }
      case lut_expr_K:
      {
         auto* le = GetPointer<lut_expr>(curr_tn);
         recursive_examinate(le->op0, current_statement, tree_man);
         recursive_examinate(le->op1, current_statement, tree_man);
         if(le->op2)
            recursive_examinate(le->op2, current_statement, tree_man);
         if(le->op3)
            recursive_examinate(le->op3, current_statement, tree_man);
         if(le->op4)
            recursive_examinate(le->op4, current_statement, tree_man);
         if(le->op5)
            recursive_examinate(le->op5, current_statement, tree_man);
         if(le->op6)
            recursive_examinate(le->op6, current_statement, tree_man);
         if(le->op7)
            recursive_examinate(le->op7, current_statement, tree_man);
         if(le->op8)
            recursive_examinate(le->op8, current_statement, tree_man);
         break;
      }
      case constructor_K:
      {
         const constructor* co = GetPointer<constructor>(curr_tn);
         const std::vector<std::pair<tree_nodeRef, tree_nodeRef>>& list_of_idx_valu = co->list_of_idx_valu;
         std::vector<std::pair<tree_nodeRef, tree_nodeRef>>::const_iterator it, it_end = list_of_idx_valu.end();
         for(it = list_of_idx_valu.begin(); it != it_end; ++it)
         {
            recursive_examinate(it->second, current_statement, tree_man);
         }
         break;
      }
      case gimple_cond_K:
      {
         const gimple_cond* gc = GetPointer<gimple_cond>(curr_tn);
         recursive_examinate(gc->op0, current_statement, tree_man);
         break;
      }
      case gimple_switch_K:
      {
         const gimple_switch* se = GetPointer<gimple_switch>(curr_tn);
         recursive_examinate(se->op0, current_statement, tree_man);
         break;
      }
      case gimple_multi_way_if_K:
      {
         auto* gmwi = GetPointer<gimple_multi_way_if>(curr_tn);
         for(auto cond : gmwi->list_of_cond)
            if(cond.first)
               recursive_examinate(cond.first, current_statement, tree_man);
         break;
      }
      case gimple_return_K:
      {
         const gimple_return* re = GetPointer<gimple_return>(curr_tn);
         if(re->op)
            recursive_examinate(re->op, current_statement, tree_man);
         break;
      }
      case gimple_for_K:
      {
         const auto* gf = GetPointer<const gimple_for>(curr_tn);
         recursive_examinate(gf->op0, current_statement, tree_man);
         recursive_examinate(gf->op1, current_statement, tree_man);
         recursive_examinate(gf->op2, current_statement, tree_man);
         break;
      }
      case gimple_while_K:
      {
         const gimple_while* gw = GetPointer<gimple_while>(curr_tn);
         recursive_examinate(gw->op0, current_statement, tree_man);
         break;
      }
      case CASE_TYPE_NODES:
      case type_decl_K:
      case template_decl_K:
      {
         break;
      }
      case target_mem_ref_K:
      {
         const target_mem_ref* tmr = GetPointer<target_mem_ref>(curr_tn);
         if(tmr->symbol)
            recursive_examinate(tmr->symbol, current_statement, tree_man);
         if(tmr->base)
            recursive_examinate(tmr->base, current_statement, tree_man);
         if(tmr->idx)
            recursive_examinate(tmr->idx, current_statement, tree_man);
         break;
      }
      case target_mem_ref461_K:
      {
         const target_mem_ref461* tmr = GetPointer<target_mem_ref461>(curr_tn);
         if(tmr->base)
            recursive_examinate(tmr->base, current_statement, tree_man);
         if(tmr->idx)
            recursive_examinate(tmr->idx, current_statement, tree_man);
         if(tmr->idx2)
            recursive_examinate(tmr->idx2, current_statement, tree_man);
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
