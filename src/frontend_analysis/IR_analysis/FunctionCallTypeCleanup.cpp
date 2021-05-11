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
 * @file FunctionCallTypeCleanup.cpp
 *
 * @author Pietro Fezzardi <pietrofezzardi@gmail.com>
 *
 */

#include "FunctionCallTypeCleanup.hpp"

#include "Parameter.hpp"

#include "call_graph_manager.hpp"

#include "tree_basic_block.hpp"
#include "tree_helper.hpp"
#include "tree_manager.hpp"
#include "tree_manipulation.hpp"
#include "tree_node.hpp"
#include "tree_reindex.hpp"

#include "application_manager.hpp"

#include "dbgPrintHelper.hpp" // for DEBUG_LEVEL_
#include "function_behavior.hpp"
#include "string_manipulation.hpp" // for GET_CLASS

static inline bool conv_really_needed(const tree_managerConstRef TM, unsigned int op0, unsigned int op1)
{
   if(op0 != op1 and ((tree_helper::is_real(TM, op0) and tree_helper::is_real(TM, op1)) || (tree_helper::is_a_complex(TM, op0) and tree_helper::is_a_complex(TM, op1))))
   {
      return tree_helper::size(TM, op0) != tree_helper::size(TM, op1);
   }
   else
   {
      return op0 != op1;
   }
}

FunctionCallTypeCleanup::FunctionCallTypeCleanup(const ParameterConstRef Param, const application_managerRef _AppM, unsigned int _function_id, const DesignFlowManagerConstRef _design_flow_manager)
    : FunctionFrontendFlowStep(_AppM, _function_id, FUNCTION_CALL_TYPE_CLEANUP, _design_flow_manager, Param)
{
   debug_level = Param->get_class_debug_level(GET_CLASS(*this), DEBUG_LEVEL_NONE);
}

FunctionCallTypeCleanup::~FunctionCallTypeCleanup() = default;

void FunctionCallTypeCleanup::Initialize()
{
}

const CustomUnorderedSet<std::pair<FrontendFlowStepType, FrontendFlowStep::FunctionRelationship>> FunctionCallTypeCleanup::ComputeFrontendRelationships(const DesignFlowStep::RelationshipType relationship_type) const
{
   CustomUnorderedSet<std::pair<FrontendFlowStepType, FrontendFlowStep::FunctionRelationship>> relationships;
   switch(relationship_type)
   {
      case(DEPENDENCE_RELATIONSHIP):
      {
         relationships.insert(std::make_pair(COMPUTE_IMPLICIT_CALLS, SAME_FUNCTION));
         relationships.insert(std::make_pair(FIX_STRUCTS_PASSED_BY_VALUE, CALLED_FUNCTIONS));
         relationships.insert(std::make_pair(FUNCTION_ANALYSIS, WHOLE_APPLICATION));
         relationships.insert(std::make_pair(IR_LOWERING, SAME_FUNCTION));
         relationships.insert(std::make_pair(USE_COUNTING, SAME_FUNCTION));
         break;
      }
      case(PRECEDENCE_RELATIONSHIP):
      {
         relationships.insert(std::make_pair(REMOVE_CLOBBER_GA, SAME_FUNCTION));
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

DesignFlowStep_Status FunctionCallTypeCleanup::InternalExec()
{
   bool changed = false;
   const auto TM = AppM->get_tree_manager();
   const auto tree_man = tree_manipulationRef(new tree_manipulation(TM, parameters));
   const auto tn = TM->get_tree_node_const(function_id);
   const auto fd = GetPointer<const function_decl>(tn);
   THROW_ASSERT(fd && fd->body, "Node is not a function or it hasn't a body");
   const auto sl = GetPointer<const statement_list>(GET_CONST_NODE(fd->body));
   THROW_ASSERT(sl, "Body is not a statement_list");
   const auto CGMan = AppM->CGetCallGraphManager();
   const auto called_body_fun_ids = CGMan->GetReachedBodyFunctionsFrom(function_id);

   for(const auto& block : sl->list_of_bloc)
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Examining BB" + STR(block.first));
      for(const auto& stmt : block.second->CGetStmtList())
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Examining statement " + GET_NODE(stmt)->ToString());
         if(GET_NODE(stmt)->get_kind() == gimple_assign_K)
         {
            const auto ga = GetPointerS<gimple_assign>(GET_NODE(stmt));
            const auto srcp_default = ga->include_name + ":" + STR(ga->line_number) + ":" + STR(ga->column_number);
            const auto code0 = GET_NODE(ga->op0)->get_kind();
            const auto code1 = GET_NODE(ga->op1)->get_kind();

            THROW_ASSERT(not ga->clobber, "");
            if(code1 == call_expr_K || code1 == aggr_init_expr_K)
            {
               const auto ce = GetPointerS<call_expr>(GET_NODE(ga->op1));
               if(GET_NODE(ce->fn)->get_kind() == addr_expr_K)
               {
                  const auto addr_node = GET_NODE(ce->fn);
                  const auto ae = GetPointerS<const addr_expr>(addr_node);
                  const auto fu_decl_node = GET_NODE(ae->op);
                  THROW_ASSERT(fu_decl_node->get_kind() == function_decl_K, "node  " + STR(fu_decl_node) + " is not function_decl but " + fu_decl_node->get_kind_text());
                  const auto ret_type_node = tree_helper::GetFunctionReturnType(fu_decl_node);
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---is a call_expr with LHS " + GET_NODE(ga->op0)->ToString());
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---the called function returns type " + ret_type_node->ToString());
                  if(code0 == ssa_name_K)
                  {
                     const auto assigned_ssa_type_node = tree_helper::CGetType(GET_NODE(ga->op0));
                     INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---the assigned ssa_name " + STR(GET_NODE(ga->op0)) + " has type " + assigned_ssa_type_node->ToString());
                     if(conv_really_needed(TM, ret_type_node->index, assigned_ssa_type_node->index))
                     {
                        const auto new_ssa = tree_man->create_ssa_name(tree_nodeRef(), ret_type_node, tree_nodeRef(), tree_nodeRef());
                        INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---create ssa " + GET_NODE(new_ssa)->ToString());

                        const auto ga_nop = tree_man->CreateNopExpr(new_ssa, TM->CGetTreeReindex(assigned_ssa_type_node->index), tree_nodeRef(), tree_nodeRef());
                        INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---create nop " + GET_NODE(ga_nop)->ToString());

                        const auto cast_ga = GetPointerS<const gimple_assign>(GET_NODE(ga_nop));
                        TM->ReplaceTreeNode(ga_nop, cast_ga->op0, ga->op0);
                        INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---fix nop " + GET_NODE(ga_nop)->ToString());

                        TM->ReplaceTreeNode(stmt, ga->op0, new_ssa);
                        INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---update call " + GET_NODE(stmt)->ToString());

                        block.second->PushAfter(ga_nop, stmt);
                        INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---insert nop " + GET_NODE(ga_nop)->ToString());
                        changed = true;
                     }
                  }
                  const auto called_id = GET_INDEX_NODE(ae->op);
                  if(called_body_fun_ids.find(called_id) != called_body_fun_ids.end())
                  {
                     unsigned int arg_n = 0;
                     auto arg_it = ce->args.begin();
                     for(; arg_it != ce->args.end(); arg_it++, arg_n++)
                     {
                        if(GET_NODE(*arg_it)->get_kind() == integer_cst_K or GET_NODE(*arg_it)->get_kind() == ssa_name_K)
                        {
                           const auto formal_type_id = tree_helper::get_formal_ith(TM, ce->index, arg_n);
                           const auto formal_type_node = TM->get_tree_node_const(formal_type_id);
                           const auto actual_type_node = tree_helper::CGetType(GET_NODE(*arg_it));
                           if(conv_really_needed(TM, formal_type_id, actual_type_node->index))
                           {
                              const auto ga_nop = tree_man->CreateNopExpr(*arg_it, TM->CGetTreeReindex(formal_type_node->index), tree_nodeRef(), tree_nodeRef());
                              INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---adding statement " + GET_NODE(ga_nop)->ToString());
                              block.second->PushBefore(ga_nop, stmt);
                              INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---old call statement " + GET_NODE(stmt)->ToString());
                              unsigned int k = 0;
                              const auto new_ssa = GetPointerS<gimple_assign>(GET_NODE(ga_nop))->op0;
                              auto tmp_arg_it = ce->args.begin();
                              for(; tmp_arg_it != ce->args.end(); tmp_arg_it++, k++)
                              {
                                 if(GET_INDEX_NODE(*arg_it) == GET_INDEX_NODE(*tmp_arg_it) and tree_helper::get_formal_ith(TM, ce->index, k) == formal_type_id)
                                 {
                                    TM->RecursiveReplaceTreeNode(*tmp_arg_it, *tmp_arg_it, new_ssa, stmt, false);
                                    tmp_arg_it = std::next(ce->args.begin(), static_cast<int>(k));
                                    arg_it = std::next(ce->args.begin(), static_cast<int>(arg_n));
                                    continue;
                                 }
                              }
                              INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---new call statement " + GET_NODE(stmt)->ToString());
                              THROW_ASSERT(k, "");
                              changed = true;
                           }
                        }
                        else if(GET_NODE(*arg_it)->get_kind() == addr_expr_K || GET_NODE(*arg_it)->get_kind() == nop_expr_K || /// required by CLANG/LLVM
                                GET_NODE(*arg_it)->get_kind() == view_convert_expr_K)                                          /// required by CLANG/LLVM
                        {
                           const auto formal_type_id = tree_helper::get_formal_ith(TM, ce->index, arg_n);
                           const auto formal_type_reindex = TM->CGetTreeReindex(formal_type_id);
                           const auto parm_ue = GetPointerS<unary_expr>(GET_NODE(*arg_it));
                           const auto ue_expr = tree_man->create_unary_operation(formal_type_reindex, parm_ue->op, srcp_default, GET_NODE(*arg_it)->get_kind()); /// It is required to de-share some IR nodes
                           const auto ue_ga = tree_man->CreateGimpleAssign(formal_type_reindex, tree_nodeRef(), tree_nodeRef(), ue_expr, block.first, srcp_default);
                           INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---adding statement " + GET_NODE(ue_ga)->ToString());
                           block.second->PushBefore(ue_ga, stmt);
                           INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---old call statement " + GET_NODE(stmt)->ToString());
                           unsigned int k = 0;
                           const auto new_ssa = GetPointerS<gimple_assign>(GET_NODE(ue_ga))->op0;
                           auto tmp_arg_it = ce->args.begin();
                           for(; tmp_arg_it != ce->args.end(); tmp_arg_it++, k++)
                           {
                              if(GET_INDEX_NODE(*arg_it) == GET_INDEX_NODE(*tmp_arg_it) and tree_helper::get_formal_ith(TM, ce->index, k) == formal_type_id)
                              {
                                 TM->RecursiveReplaceTreeNode(*tmp_arg_it, *tmp_arg_it, new_ssa, stmt, false);
                                 tmp_arg_it = std::next(ce->args.begin(), static_cast<int>(k));
                                 arg_it = std::next(ce->args.begin(), static_cast<int>(arg_n));
                                 continue;
                              }
                           }
                           INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---new call statement " + GET_NODE(stmt)->ToString());
                           THROW_ASSERT(k, "");
                           changed = true;
                        }
                     }
                  }
                  else
                  {
                     INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Function " + STR(called_id) + " does not have a body");
                  }
               }
               else if(GET_NODE(ce->fn)->get_kind() != ssa_name_K)
               {
                  THROW_UNREACHABLE("call node  " + STR(GET_NODE(ce->fn)) + " is a " + GET_NODE(ce->fn)->get_kind_text());
               }
            }
         }
         else if(GET_NODE(stmt)->get_kind() == gimple_call_K)
         {
            const auto gc = GetPointerS<gimple_call>(GET_NODE(stmt));
            const auto srcp_default = gc->include_name + ":" + STR(gc->line_number) + ":" + STR(gc->column_number);
            if(GET_NODE(gc->fn)->get_kind() == addr_expr_K)
            {
               const auto addr_node = GET_NODE(gc->fn);
               const auto ae = GetPointerS<const addr_expr>(addr_node);
               THROW_ASSERT(GET_NODE(ae->op)->get_kind() == function_decl_K, "node  " + STR(GET_NODE(ae->op)) + " is not function_decl but " + GET_NODE(ae->op)->get_kind_text());
               const auto called_id = GET_INDEX_NODE(ae->op);
               if(called_body_fun_ids.find(called_id) != called_body_fun_ids.end())
               {
                  unsigned int arg_n = 0;
                  auto arg_it = gc->args.begin();
                  for(; arg_it != gc->args.end(); arg_it++, arg_n++)
                  {
                     if(GET_NODE(*arg_it)->get_kind() == integer_cst_K or GET_NODE(*arg_it)->get_kind() == ssa_name_K)
                     {
                        const auto formal_type_id = tree_helper::get_formal_ith(TM, gc->index, arg_n);
                        const auto formal_type_node = TM->get_tree_node_const(formal_type_id);
                        const auto actual_type_node = tree_helper::CGetType(GET_NODE(*arg_it));
                        if(conv_really_needed(TM, formal_type_id, actual_type_node->index))
                        {
                           const auto ga_nop = tree_man->CreateNopExpr(*arg_it, TM->CGetTreeReindex(formal_type_node->index), tree_nodeRef(), tree_nodeRef());
                           INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---adding statement " + GET_NODE(ga_nop)->ToString());
                           block.second->PushBefore(ga_nop, stmt);
                           INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---old call statement " + GET_NODE(stmt)->ToString());
                           unsigned int k = 0;
                           const auto new_ssa = GetPointerS<gimple_assign>(GET_NODE(ga_nop))->op0;
                           auto tmp_arg_it = gc->args.begin();
                           for(; tmp_arg_it != gc->args.end(); tmp_arg_it++, k++)
                           {
                              if(GET_INDEX_NODE(*arg_it) == GET_INDEX_NODE(*tmp_arg_it) and tree_helper::get_formal_ith(TM, gc->index, k) == formal_type_id)
                              {
                                 TM->RecursiveReplaceTreeNode(*tmp_arg_it, *tmp_arg_it, new_ssa, stmt, false);
                                 tmp_arg_it = std::next(gc->args.begin(), static_cast<int>(k));
                                 arg_it = std::next(gc->args.begin(), static_cast<int>(arg_n));
                                 continue;
                              }
                           }
                           INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---new call statement " + GET_NODE(stmt)->ToString());
                           THROW_ASSERT(k, "");
                           changed = true;
                        }
                     }
                     else if(GET_NODE(*arg_it)->get_kind() == addr_expr_K || GET_NODE(*arg_it)->get_kind() == nop_expr_K || /// required by CLANG/LLVM
                             GET_NODE(*arg_it)->get_kind() == view_convert_expr_K)                                          /// required by CLANG/LLVM
                     {
                        const auto formal_type_id = tree_helper::get_formal_ith(TM, gc->index, arg_n);
                        const auto formal_type_reindex = TM->CGetTreeReindex(formal_type_id);
                        const auto parm_ue = GetPointerS<unary_expr>(GET_NODE(*arg_it));
                        const auto ue_expr = tree_man->create_unary_operation(formal_type_reindex, parm_ue->op, srcp_default, GET_NODE(*arg_it)->get_kind()); /// It is required to de-share some IR nodes
                        const auto ue_ga = tree_man->CreateGimpleAssign(formal_type_reindex, tree_nodeRef(), tree_nodeRef(), ue_expr, block.first, srcp_default);
                        const auto ue_vd = GetPointerS<gimple_assign>(GET_NODE(ue_ga))->op0;
                        INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---adding statement " + GET_NODE(ue_ga)->ToString());
                        block.second->PushBefore(ue_ga, stmt);
                        INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---old call statement " + GET_NODE(stmt)->ToString());
                        unsigned int k = 0;
                        const auto new_ssa = GetPointerS<gimple_assign>(GET_NODE(ue_ga))->op0;
                        auto tmp_arg_it = gc->args.begin();
                        for(; tmp_arg_it != gc->args.end(); tmp_arg_it++, k++)
                        {
                           if(GET_INDEX_NODE(*arg_it) == GET_INDEX_NODE(*tmp_arg_it) and tree_helper::get_formal_ith(TM, gc->index, k) == formal_type_id)
                           {
                              TM->RecursiveReplaceTreeNode(*tmp_arg_it, *tmp_arg_it, new_ssa, stmt, false);
                              tmp_arg_it = std::next(gc->args.begin(), static_cast<int>(k));
                              arg_it = std::next(gc->args.begin(), static_cast<int>(arg_n));
                              continue;
                           }
                        }
                        INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---new call statement " + GET_NODE(stmt)->ToString());
                        THROW_ASSERT(k, "");
                        changed = true;
                     }
                  }
               }
            }
            else if(GET_NODE(gc->fn)->get_kind() != ssa_name_K)
            {
               THROW_UNREACHABLE("call node  " + STR(GET_NODE(gc->fn)) + " is a " + GET_NODE(gc->fn)->get_kind_text());
            }
         }
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Examined statement " + GET_NODE(stmt)->ToString());
      }
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Examined BB" + STR(block.first));
   }

   if(changed)
   {
      function_behavior->UpdateBBVersion();
   }
   return changed ? DesignFlowStep_Status::SUCCESS : DesignFlowStep_Status::UNCHANGED;
}
