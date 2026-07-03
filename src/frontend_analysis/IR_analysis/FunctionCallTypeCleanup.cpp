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
 * @file FunctionCallTypeCleanup.cpp
 *
 * @author Pietro Fezzardi <pietrofezzardi@gmail.com>
 * @author Michele Fiorito <michele.fiorito@polimi.it>
 *
 */
#include "FunctionCallTypeCleanup.hpp"

#include "Parameter.hpp"
#include "application_manager.hpp"
#include "call_graph_manager.hpp"
#include "dbgPrintHelper.hpp"
#include "function_behavior.hpp"
#include "ir_basic_block.hpp"
#include "ir_helper.hpp"
#include "ir_manager.hpp"
#include "ir_manipulation.hpp"
#include "ir_node.hpp"
#include "string_manipulation.hpp"

FunctionCallTypeCleanup::FunctionCallTypeCleanup(const ParameterConstRef Param, const application_managerRef _AppM,
                                                 unsigned int _function_id,
                                                 const DesignFlowManager& _design_flow_manager)
    : FunctionFrontendFlowStep(_AppM, _function_id, FUNCTION_CALL_TYPE_CLEANUP, _design_flow_manager, Param)
{
   debug_level = Param->get_class_debug_level(GET_CLASS(*this), DEBUG_LEVEL_NONE);
}

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
         relationships.insert(std::make_pair(FUNCTION_ANALYSIS, WHOLE_APPLICATION));
         relationships.insert(std::make_pair(USE_COUNTING, SAME_FUNCTION));
         break;
      }
      case(PRECEDENCE_RELATIONSHIP):
      {
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
   const auto TM = AppM->get_ir_manager();
   const ir_manipulationRef ir_man(new ir_manipulation(TM, parameters, AppM));
   const auto tn = TM->GetIRNode(function_id);
   const auto* fd = GetPointerS<const function_val_node>(tn);
   THROW_ASSERT(fd && fd->body, "Node is not a function or it hasn't a body");
   const auto* sl = GetPointerS<const statement_list_node>(fd->body);
   THROW_ASSERT(sl, "Body is not a statement_list_node");
   const auto& CGM = AppM->CGetCallGraphManager();
   const auto called_body_fun_ids = CGM.GetReachedFunctionsFrom(function_id);

   for(const auto& [bbi, bb] : sl->list_of_bloc)
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Examining BB" + STR(bbi));
      for(const auto& stmt : bb->CGetStmtList())
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Examining statement " + stmt->ToString());
         if(stmt->get_kind() == assign_stmt_K)
         {
            const auto* ga = GetPointerS<const assign_stmt>(stmt);
            const auto op1_kind = ga->op1->get_kind();

            if(op1_kind == call_node_K)
            {
               auto* ce = GetPointerS<call_node>(ga->op1);
               if(ce->fn->get_kind() == addr_node_K)
               {
                  const auto* ae = GetPointerS<const addr_node>(ce->fn);
                  THROW_ASSERT(ae->op->get_kind() == function_val_node_K,
                               "node  " + STR(ae->op) + " is not function_val_node but " + ae->op->get_kind_text());
                  const auto lhs_type = ir_helper::CGetType(ga->op0);
                  const auto rhs_type = ir_helper::GetFunctionReturnType(ae->op);
                  THROW_ASSERT(rhs_type, "Function return type is void?");
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---LHS: " + STR(ga->op0));
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---LHS type: " + STR(lhs_type));
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---RHS: " + STR(ga->op1));
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---RHS type: " + STR(rhs_type));
                  const auto loc_info_default =
                      ga->include_name + ":" + STR(ga->line_number) + ":" + STR(ga->column_number);
                  if(ga->op0->get_kind() == ssa_node_K && !ir_helper::IsSameType(rhs_type, lhs_type))
                  {
                     INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->");
                     const auto ret_val = ir_man->create_ssa_name(nullptr, rhs_type, nullptr, nullptr);
                     const auto needs_bitcast = ir_helper::IsRealType(rhs_type) != ir_helper::IsRealType(lhs_type);
                     const auto ue = ir_man->create_unary_operation(lhs_type, ret_val, loc_info_default,
                                                                    needs_bitcast ? bitcast_node_K : nop_node_K);

                     const auto ga_nop =
                         ir_man->CreateAssignStmt(lhs_type, nullptr, nullptr, ue, function_id, loc_info_default);

                     const auto* cast_ga = GetPointerS<const assign_stmt>(ga_nop);
                     TM->ReplaceIRNode(ga_nop, cast_ga->op0, ga->op0);

                     TM->ReplaceIRNode(stmt, ga->op0, ret_val);
                     INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---update call " + stmt->ToString());

                     bb->PushAfter(ga_nop, stmt, AppM);
                     INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---insert nop  " + ga_nop->ToString());
                     changed = true;
                     INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--");
                  }
                  const auto called_id = ae->op->index;
                  if(called_body_fun_ids.find(called_id) != called_body_fun_ids.end())
                  {
                     changed |= ParametersTypeCleanup(TM, ir_man, bb, stmt, ce->args, loc_info_default);
                  }
                  else
                  {
                     INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Function does not have a body");
                  }
               }
               else if(ce->fn->get_kind() != ssa_node_K)
               {
                  THROW_UNREACHABLE("call node  " + STR(ce->fn) + " is a " + ce->fn->get_kind_text());
               }
            }
         }
         else if(stmt->get_kind() == call_stmt_K)
         {
            auto* gc = GetPointerS<call_stmt>(stmt);
            if(gc->fn->get_kind() == addr_node_K)
            {
               const auto* ae = GetPointerS<const addr_node>(gc->fn);
               THROW_ASSERT(ae->op->get_kind() == function_val_node_K,
                            "node  " + STR(ae->op) + " is not function_val_node but " + ae->op->get_kind_text());
               const auto called_id = ae->op->index;
               if(called_body_fun_ids.find(called_id) != called_body_fun_ids.end())
               {
                  const auto loc_info_default =
                      gc->include_name + ":" + STR(gc->line_number) + ":" + STR(gc->column_number);
                  changed |= ParametersTypeCleanup(TM, ir_man, bb, stmt, gc->args, loc_info_default);
               }
               else
               {
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Function does not have a body");
               }
            }
            else if(gc->fn->get_kind() != ssa_node_K)
            {
               THROW_UNREACHABLE("call node  " + STR(gc->fn) + " is a " + gc->fn->get_kind_text());
            }
         }
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Examined statement " + stmt->ToString());
      }
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Examined BB" + STR(bbi));
   }

   if(changed)
   {
      function_behavior->UpdateBBVersion();
   }
   return changed ? DesignFlowStep_Status::SUCCESS : DesignFlowStep_Status::UNCHANGED;
}

bool FunctionCallTypeCleanup::ParametersTypeCleanup(const ir_managerRef& TM, const ir_manipulationRef& ir_man,
                                                    const blocRef& block, const ir_nodeRef& stmt,
                                                    std::vector<ir_nodeRef>& args, const std::string& loc_info) const
{
   bool changed = false;
   unsigned arg_n = 0;
   auto arg_it = args.cbegin();
   for(; arg_it != args.cend(); arg_it++, arg_n++)
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Examining arg " + STR(*arg_it));
      const auto formal_type = ir_helper::GetFormalIth(stmt, arg_n);
      const auto actual_type = ir_helper::CGetType(*arg_it);
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                     "---Formal type = " + formal_type->get_kind_text() + "\t" + formal_type->ToString());
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                     "---Actual type = " + actual_type->get_kind_text() + "\t" + actual_type->ToString());
      ir_nodeRef ga_cleanup = nullptr;
      if(((*arg_it)->get_kind() == constant_int_val_node_K || (*arg_it)->get_kind() == ssa_node_K) &&
         !ir_helper::IsSameType(formal_type, actual_type))
      {
         const auto needs_bitcast = ir_helper::IsRealType(actual_type) != ir_helper::IsRealType(formal_type);
         const auto ue = ir_man->create_unary_operation(formal_type, *arg_it, loc_info,
                                                        needs_bitcast ? bitcast_node_K : nop_node_K);
         ga_cleanup = ir_man->CreateAssignStmt(formal_type, nullptr, nullptr, ue, function_id, loc_info);
      }
      else if((*arg_it)->get_kind() == addr_node_K || (*arg_it)->get_kind() == nop_node_K ||
              (*arg_it)->get_kind() == bitcast_node_K) /// required by CLANG/LLVM
      {
         const auto* parm_ue = GetPointerS<const unary_node>(*arg_it);
         const auto ue =
             ir_man->create_unary_operation(formal_type, parm_ue->op, loc_info,
                                            (*arg_it)->get_kind()); /// It is required to de-share some IR nodes
         ga_cleanup = ir_man->CreateAssignStmt(formal_type, nullptr, nullptr, ue, function_id, loc_info);
      }
      if(ga_cleanup)
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---insert nop  " + ga_cleanup->ToString());
         block->PushBefore(ga_cleanup, stmt, AppM);
         const auto new_ssa = GetPointerS<const assign_stmt>(ga_cleanup)->op0;
         unsigned int k = 0;
         auto tmp_arg_it = args.begin();
         for(; tmp_arg_it != args.end(); tmp_arg_it++, k++)
         {
            if((*arg_it)->index == (*tmp_arg_it)->index &&
               ir_helper::GetFormalIth(stmt, k)->index == formal_type->index)
            {
               TM->RecursiveReplaceIRNode(*tmp_arg_it, *tmp_arg_it, new_ssa, stmt, false, true);
               tmp_arg_it = std::next(args.begin(), static_cast<int>(k));
               arg_it = std::next(args.begin(), static_cast<int>(arg_n));
               continue;
            }
         }
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---update call " + stmt->ToString());
         THROW_ASSERT(k, "");
         changed = true;
      }
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--");
   }
   return changed;
}
