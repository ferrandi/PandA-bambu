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
 *              Copyright (C) 2004-2024 Politecnico di Milano
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
 * @author Michele Fiorito <michele.fiorito@polimi.it>
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

#include "application_manager.hpp"

#include "dbgPrintHelper.hpp" // for DEBUG_LEVEL_
#include "function_behavior.hpp"
#include "string_manipulation.hpp" // for GET_CLASS

FunctionCallTypeCleanup::FunctionCallTypeCleanup(const ParameterConstRef Param, const application_managerRef _AppM,
                                                 unsigned int _function_id,
                                                 const DesignFlowManagerConstRef _design_flow_manager)
    : FunctionFrontendFlowStep(_AppM, _function_id, FUNCTION_CALL_TYPE_CLEANUP, _design_flow_manager, Param)
{
   debug_level = Param->get_class_debug_level(GET_CLASS(*this), DEBUG_LEVEL_NONE);
}

FunctionCallTypeCleanup::~FunctionCallTypeCleanup() = default;

void FunctionCallTypeCleanup::Initialize()
{
}

CustomUnorderedSet<std::pair<FrontendFlowStepType, FrontendFlowStep::FunctionRelationship>>
FunctionCallTypeCleanup::ComputeFrontendRelationships(const DesignFlowStep::RelationshipType relationship_type) const
{
   CustomUnorderedSet<std::pair<FrontendFlowStepType, FrontendFlowStep::FunctionRelationship>> relationships;
   switch(relationship_type)
   {
      case(DEPENDENCE_RELATIONSHIP):
      {
         relationships.insert(std::make_pair(CHECK_SYSTEM_TYPE, SAME_FUNCTION));
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
   const auto tree_man = tree_manipulationRef(new tree_manipulation(TM, parameters, AppM));
   const auto tn = TM->GetTreeNode(function_id);
   const auto fd = GetPointerS<const function_decl>(tn);
   THROW_ASSERT(fd && fd->body, "Node is not a function or it hasn't a body");
   const auto sl = GetPointerS<const statement_list>(fd->body);
   THROW_ASSERT(sl, "Body is not a statement_list");
   const auto CGMan = AppM->CGetCallGraphManager();
   const auto called_body_fun_ids = CGMan->GetReachedFunctionsFrom(function_id);

   for(const auto& block : sl->list_of_bloc)
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Examining BB" + STR(block.first));
      for(const auto& stmt : block.second->CGetStmtList())
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Examining statement " + stmt->ToString());
         if(stmt->get_kind() == gimple_assign_K)
         {
            const auto ga = GetPointerS<const gimple_assign>(stmt);
            const auto srcp_default = ga->include_name + ":" + STR(ga->line_number) + ":" + STR(ga->column_number);
            const auto code0 = ga->op0->get_kind();
            const auto code1 = ga->op1->get_kind();

            THROW_ASSERT(!ga->clobber, "");
            if(code1 == call_expr_K || code1 == aggr_init_expr_K)
            {
               const auto ce = GetPointerS<call_expr>(ga->op1);
               if(ce->fn->get_kind() == addr_expr_K)
               {
                  const auto addr_node = ce->fn;
                  const auto ae = GetPointerS<const addr_expr>(addr_node);
                  const auto fu_decl_node = ae->op;
                  THROW_ASSERT(fu_decl_node->get_kind() == function_decl_K, "node  " + STR(fu_decl_node) +
                                                                                " is not function_decl but " +
                                                                                fu_decl_node->get_kind_text());
                  const auto ret_type_node = tree_helper::GetFunctionReturnType(fu_decl_node);
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                 "---is a call_expr with LHS " + ga->op0->ToString());
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                 "---the called function returns type " + STR(ret_type_node));
                  if(code0 == ssa_name_K)
                  {
                     const auto assigned_ssa_type_node = tree_helper::CGetType(ga->op0);
                     INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                    "---the assigned ssa_name " + STR(ga->op0) + " has type " +
                                        STR(assigned_ssa_type_node));
                     if(!tree_helper::IsSameType(ret_type_node, assigned_ssa_type_node))
                     {
                        const auto new_ssa =
                            tree_man->create_ssa_name(tree_nodeRef(), ret_type_node, tree_nodeRef(), tree_nodeRef());
                        INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---create ssa " + new_ssa->ToString());

                        const auto ga_nop = tree_man->CreateNopExpr(new_ssa, assigned_ssa_type_node, tree_nodeRef(),
                                                                    tree_nodeRef(), function_id);
                        INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---create nop " + ga_nop->ToString());

                        const auto cast_ga = GetPointerS<const gimple_assign>(ga_nop);
                        TM->ReplaceTreeNode(ga_nop, cast_ga->op0, ga->op0);
                        INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---fix nop " + ga_nop->ToString());

                        TM->ReplaceTreeNode(stmt, ga->op0, new_ssa);
                        INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---update call " + stmt->ToString());

                        block.second->PushAfter(ga_nop, stmt, AppM);
                        INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---insert nop " + ga_nop->ToString());
                        changed = true;
                     }
                  }
                  const auto called_id = ae->op->index;
                  if(called_body_fun_ids.find(called_id) != called_body_fun_ids.end())
                  {
                     changed |= ParametersTypeCleanup(TM, tree_man, block.second, stmt, ce->args, srcp_default);
                  }
                  else
                  {
                     INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Function does not have a body");
                  }
               }
               else if(ce->fn->get_kind() != ssa_name_K)
               {
                  THROW_UNREACHABLE("call node  " + STR(ce->fn) + " is a " + ce->fn->get_kind_text());
               }
            }
         }
         else if(stmt->get_kind() == gimple_call_K)
         {
            const auto gc = GetPointerS<gimple_call>(stmt);
            if(gc->fn->get_kind() == addr_expr_K)
            {
               const auto addr_node = gc->fn;
               const auto ae = GetPointerS<const addr_expr>(addr_node);
               THROW_ASSERT(ae->op->get_kind() == function_decl_K,
                            "node  " + STR(ae->op) + " is not function_decl but " + ae->op->get_kind_text());
               const auto called_id = ae->op->index;
               if(called_body_fun_ids.find(called_id) != called_body_fun_ids.end())
               {
                  const auto srcp_default =
                      gc->include_name + ":" + STR(gc->line_number) + ":" + STR(gc->column_number);
                  changed |= ParametersTypeCleanup(TM, tree_man, block.second, stmt, gc->args, srcp_default);
               }
               else
               {
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Function does not have a body");
               }
            }
            else if(gc->fn->get_kind() != ssa_name_K)
            {
               THROW_UNREACHABLE("call node  " + STR(gc->fn) + " is a " + gc->fn->get_kind_text());
            }
         }
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Examined statement " + stmt->ToString());
      }
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Examined BB" + STR(block.first));
   }

   if(changed)
   {
      function_behavior->UpdateBBVersion();
   }
   return changed ? DesignFlowStep_Status::SUCCESS : DesignFlowStep_Status::UNCHANGED;
}

bool FunctionCallTypeCleanup::ParametersTypeCleanup(const tree_managerRef& TM, const tree_manipulationRef& tree_man,
                                                    const blocRef& block, const tree_nodeRef& stmt,
                                                    std::vector<tree_nodeRef>& args, const std::string& srcp) const
{
   bool changed = false;
   unsigned arg_n = 0;
   auto arg_it = args.cbegin();
   for(; arg_it != args.cend(); arg_it++, arg_n++)
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                     "-->Examining arg " + (*arg_it)->ToString() + " " + (*arg_it)->get_kind_text());
      const auto formal_type = tree_helper::GetFormalIth(stmt, arg_n);
      const auto actual_type = tree_helper::CGetType(*arg_it);
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                     "---formal type = " + formal_type->get_kind_text() + "\t" + STR(formal_type));
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                     "---actual type = " + actual_type->get_kind_text() + "\t" + STR(actual_type));
      tree_nodeRef ga_cleanup = nullptr;
      if(((*arg_it)->get_kind() == integer_cst_K || (*arg_it)->get_kind() == ssa_name_K) &&
         !tree_helper::IsSameType(formal_type, actual_type))
      {
         ga_cleanup = tree_man->CreateNopExpr(*arg_it, formal_type, tree_nodeRef(), tree_nodeRef(), function_id);
      }
      else if((*arg_it)->get_kind() == addr_expr_K || (*arg_it)->get_kind() == nop_expr_K ||
              (*arg_it)->get_kind() == view_convert_expr_K) /// required by CLANG/LLVM
      {
         const auto parm_ue = GetPointerS<const unary_expr>(*arg_it);
         const auto ue_expr =
             tree_man->create_unary_operation(formal_type, parm_ue->op, srcp,
                                              (*arg_it)->get_kind()); /// It is required to de-share some IR nodes
         ga_cleanup =
             tree_man->CreateGimpleAssign(formal_type, tree_nodeRef(), tree_nodeRef(), ue_expr, function_id, srcp);
      }
      if(ga_cleanup)
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---adding statement " + ga_cleanup->ToString());
         block->PushBefore(ga_cleanup, stmt, AppM);
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---old call statement " + stmt->ToString());
         const auto new_ssa = GetPointerS<const gimple_assign>(ga_cleanup)->op0;
         unsigned int k = 0;
         auto tmp_arg_it = args.begin();
         for(; tmp_arg_it != args.end(); tmp_arg_it++, k++)
         {
            if((*arg_it)->index == (*tmp_arg_it)->index &&
               tree_helper::GetFormalIth(stmt, k)->index == formal_type->index)
            {
               TM->RecursiveReplaceTreeNode(*tmp_arg_it, *tmp_arg_it, new_ssa, stmt, false);
               tmp_arg_it = std::next(args.begin(), static_cast<int>(k));
               arg_it = std::next(args.begin(), static_cast<int>(arg_n));
               continue;
            }
         }
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---new call statement " + stmt->ToString());
         THROW_ASSERT(k, "");
         changed = true;
      }
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--");
   }
   return changed;
}
