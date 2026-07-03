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
 * @file FixStructsPassedByValue.cpp
 *
 * @author Pietro Fezzardi <pietrofezzardi@gmail.com>
 *
 */
#include "FixStructsPassedByValue.hpp"

#include "Parameter.hpp"
#include "application_manager.hpp"
#include "behavioral_helper.hpp"
#include "call_graph_manager.hpp"
#include "dbgPrintHelper.hpp"
#include "function_behavior.hpp"
#include "hls_manager.hpp"
#include "ir_basic_block.hpp"
#include "ir_helper.hpp"
#include "ir_manager.hpp"
#include "ir_manipulation.hpp"
#include "ir_node.hpp"
#include "op_graph.hpp"
#include "string_manipulation.hpp"

FixStructsPassedByValue::FixStructsPassedByValue(const ParameterConstRef params, const application_managerRef AM,
                                                 unsigned int fun_id, const DesignFlowManager& dfm)
    : FunctionFrontendFlowStep(AM, fun_id, FIX_STRUCTS_PASSED_BY_VALUE, dfm, params)
{
   debug_level = parameters->get_class_debug_level(GET_CLASS(*this), DEBUG_LEVEL_NONE);
}

static bool cannot_have_struct_parameters(const ir_nodeRef& fnode, const function_ty_node* const ft)
{
   const auto fd = GetPointerS<const function_val_node>(fnode);
   auto p_type_head = ft->list_of_args_type;
   if(p_type_head.empty() || (p_type_head.size() == 1 && p_type_head.front()->get_kind() == void_ty_node_K))
   {
      // if the function_ty_node takes void argument there's nothing to do
      THROW_ASSERT(fd->list_of_args.empty(), "function " + ir_helper::GetFunctionName(fnode) +
                                                 " has void parameter type but has a argument_val_node " +
                                                 STR(fd->list_of_args.front()));
      return true;
   }
   if(fd->list_of_args.empty())
   {
      THROW_ERROR("unexpected pattern");
      return false;
   }
   return false;
}

CustomUnorderedSet<std::pair<FrontendFlowStepType, FrontendFlowStep::FunctionRelationship>>
FixStructsPassedByValue::ComputeFrontendRelationships(const DesignFlowStep::RelationshipType relationship_type) const
{
   CustomUnorderedSet<std::pair<FrontendFlowStepType, FunctionRelationship>> relationships;
   switch(relationship_type)
   {
      case(DEPENDENCE_RELATIONSHIP):
      {
         relationships.insert(std::make_pair(BLOCK_FIX, SAME_FUNCTION));
         relationships.insert(std::make_pair(FIX_STRUCTS_PASSED_BY_VALUE, CALLING_FUNCTIONS));
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

DesignFlowStep_Status FixStructsPassedByValue::InternalExec()
{
   bool changed = false;
   const auto TM = AppM->get_ir_manager();
   const auto ir_man = ir_manipulationRef(new ir_manipulation(TM, parameters, AppM));
   const auto tn = TM->GetIRNode(function_id);
   const auto fd = GetPointer<function_val_node>(tn);
   THROW_ASSERT(fd && fd->body, "Node " + STR(tn) + "is not a function_val_node or has no body");
   const auto sl = GetPointer<const statement_list_node>(fd->body);
   THROW_ASSERT(sl, "Body is not a statement_list_node");
   const auto fname = function_behavior->GetBehavioralHelper()->GetFunctionName();
   auto ftype = GetPointer<function_ty_node>(fd->type);
   THROW_ASSERT(!ftype->varargs_flag, "function " + fname + " is varargs");
   const auto HLSMgr = GetPointer<HLS_manager>(AppM);
   const auto func_arch = HLSMgr ? HLSMgr->module_arch->GetArchitecture(fname) : nullptr;
   // fix declaration
   if(!cannot_have_struct_parameters(tn, ftype))
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Fixing declaration of function " + fname);
      unsigned int param_n = 0;
      auto p_decl_it = fd->list_of_args.begin();
      auto p_type_head = ftype->list_of_args_type.begin();
      auto p_type_last = ftype->list_of_args_type.end();
      const auto has_param_types = p_type_head != p_type_last;
      for(; p_decl_it != fd->list_of_args.cend(); p_decl_it++, param_n++)
      {
         const auto p_decl = *p_decl_it;
         const auto p_type = ir_helper::CGetType(p_decl);
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                        "-->Analyzing parameter " + STR(p_decl) + " with type " + STR(p_type));
         THROW_ASSERT(has_param_types == (p_type_head != p_type_last),
                      "function " + fname + " has " + STR(fd->list_of_args.size()) + " parameters, but argument " +
                          STR(param_n) + " (" + STR(p_decl) +
                          ") has not a corresponding underlying type in function_ty_node");

         if(ir_helper::IsStructType(p_type))
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                           "function " + fname + " has a struct parameter: " + STR(p_decl) + " with type " +
                               STR(p_type));
            // initialize some general stuff useful later
            const auto pd = GetPointerS<const argument_val_node>(p_decl);
            const auto loc_info = pd->include_name + ":" + STR(pd->line_number) + ":" + STR(pd->column_number);
            const auto original_param_name = pd->name ? GetPointerS<const identifier_node>(pd->name)->strg : "";

            auto ptd_type_size = ir_helper::SizeAlloc(p_type);
            if(ptd_type_size % 8)
            {
               ptd_type_size += 8;
            }
            ptd_type_size /= 8;

            // create new variable_val_node
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Creating new local variable_val_node");
            const auto local_var_name = "bambu_artificial_local_param_copy_" + original_param_name;
            const auto local_var_identifier = ir_man->create_identifier_node(local_var_name);
            const auto new_local_var_decl =
                ir_man->create_var_decl(local_var_identifier, p_type, pd->parent, pd->bitsizealloc, ir_nodeRef(),
                                        loc_info, GetPointerS<const type_node>(p_type)->algn,
                                        false); // artificial flag (should be true???)
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Created new local variable_val_node");

            // substitute variable_val_node to argument_val_node in all the statements of the function
            {
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                              "-->Substituting variable_val_node " + STR(p_decl) + " to argument_val_node " +
                                  STR(new_local_var_decl) + " in all statements");
               for(const auto& block : sl->list_of_bloc)
               {
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Examining BB" + STR(block.first));
                  for(const auto& stmt : block.second->CGetStmtList())
                  {
                     INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                    "-->Examining statement " + stmt->ToString());
                     TM->ReplaceIRNode(stmt, p_decl, new_local_var_decl);
                     INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                    "<--Examined statement " + stmt->ToString());
                  }
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Examined BB" + STR(block.first));
               }
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                              "<--Substituted variable_val_node " + STR(p_decl) + " to argument_val_node " +
                                  STR(new_local_var_decl) + " in all statements");
            }

            // create pointer type for the new pointer-to-struct parameter
            const auto ptr_type = ir_man->GetPointerType(p_type);

            // substitute parameter type in function_ty_node if necessary
            if(has_param_types)
            {
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                              "-->Substituting type of parameter " + STR(p_decl));
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                              "---Changing type from " + STR(p_type) + " to " + STR(ptr_type));
               *p_type_head = ptr_type;
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                              "<--Substituted type of parameter " + STR(p_decl));
            }

            // create and substitute new pointer-to-struct argument_val_node in function_val_node
            {
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                              "-->Substituting argument_val_node of " + STR(p_decl));
               const auto ptr_p_name = "bambu_artificial_ptr_param_" + original_param_name;
               const auto ptr_p_identifier = ir_man->create_identifier_node(ptr_p_name);
               const auto ptr_p_decl =
                   ir_man->create_parm_decl(ptr_p_identifier, ptr_type, pd->parent, ir_nodeRef(), loc_info, true);
               if(func_arch)
               {
                  const auto parm_it = func_arch->parms.find(original_param_name);
                  if(parm_it != func_arch->parms.end())
                  {
                     func_arch->parms[ptr_p_name] = parm_it->second;
                     func_arch->parms[ptr_p_name].at(FunctionArchitecture::parm_port) = ptr_p_name;
                     func_arch->parms.erase(parm_it);

                     // NOTE: should also update HLS_manager::design_interface_io, but passed-by-value parameters cannot
                     // have associated I/O operations
                  }
               }
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                              "---Changing argument_val_node from " + STR(p_decl) + " to " + STR(ptr_p_decl));
               *p_decl_it = ptr_p_decl;
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                              "<--Substituted argument_val_node of " + STR(p_decl) + " with " + STR(*p_decl_it));
            }

            /*
             * find the first basic block of the function. it is the
             * successor of the entry basic block that is not the exit basic
             * block.
             */
            unsigned int bb_index = BB_ENTRY;
            {
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Selecting first basic block of " + fname);
               const auto entry_block = sl->list_of_bloc.at(BB_ENTRY);
               const auto succ_blocks = entry_block->list_of_succ;
               THROW_ASSERT(succ_blocks.size() == 1, "entry basic block of function " + fname + " has " +
                                                         STR(succ_blocks.size()) + " successors");
               bb_index = *(succ_blocks.begin());
               THROW_ASSERT(bb_index != BB_ENTRY and bb_index != BB_EXIT,
                            "first basic block of function " + fname + " not found");
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                              "<--Selected first basic block of " + fname + ": " + STR(bb_index));
            }
            const auto first_block = sl->list_of_bloc.at(bb_index);

            // create the call to memcpy
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Creating new call to memcpy");
            const auto memcpy_function = TM->GetFunction(MEMCPY);
            THROW_ASSERT(ir_helper::IsFunctionImplemented(memcpy_function), "inconsistent behavioral helper");
            const auto formal_type_node = ir_helper::GetFormalIth(memcpy_function, 2);
            const std::vector<ir_nodeRef> args = {
                // & new_local_var_decl
                ir_man->CreateAddrExpr(new_local_var_decl, loc_info),
                // src is the new pointer-to-struct argument_val_node
                ir_man->create_ssa_name(*p_decl_it, ptr_type, ir_nodeRef(), ir_nodeRef()),
                // sizeof(variable_val_node)
                TM->CreateUniqueIntegerCst(static_cast<long long>(ptd_type_size), formal_type_node)};
            const auto call_stmt_memcpy = ir_man->create_call_stmt(memcpy_function, args, function_id, loc_info);
            auto gn = GetPointer<node_stmt>(call_stmt_memcpy);
            /*
             * the call is artificial. this is necessary because this memcpy
             * should not be moved around by code motion or other steps. this
             * call should always be performed as first operation of the
             * function, before any other. this could be achieved in theory
             * adding vdefs/vuses to force dependencies, but it becomes
             * tricky to get it right when the address of the struct passed
             * by value is taken, stored and used somewhere else. instead we
             * set the call to artificial so that the other passes will not
             * move it around
             */
            gn->artificial = true;
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                           "<--Created new call to memcpy: " + STR(call_stmt_memcpy));
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Updating basic block");
            first_block->PushFront(call_stmt_memcpy, AppM);
            changed = true;
         }

         if(has_param_types)
         {
            ++p_type_head;
         }

         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                        "<--Analyzed parameter " + STR(p_decl) + " with type " + STR(p_type));
      }
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Fixed declaration of function " + fname);
   }
   // fix calls to other functions that accept structs passed by value as parameters
   {
      for(const auto& block : sl->list_of_bloc)
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Examining BB" + STR(block.first));
         for(const auto& stmt : block.second->CGetStmtList())
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Examining statement " + stmt->ToString());
            const auto gn = GetPointer<const node_stmt>(stmt);
            const auto loc_info_default = gn->include_name + ":" + STR(gn->line_number) + ":" + STR(gn->column_number);
            const auto stmt_kind = stmt->get_kind();
            if(stmt_kind == assign_stmt_K or stmt_kind == call_stmt_K)
            {
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Is a " + ir_node::GetString(stmt_kind));
               ir_nodeConstRef called_node;
               std::vector<ir_nodeRef>* arguments;
               unsigned int call_ir_node_id = 0;

               if(stmt_kind == assign_stmt_K)
               {
                  const auto ga = GetPointer<const assign_stmt>(stmt);
                  if(ga->op1->get_kind() != call_node_K)
                  {
                     INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--RHS is not a call_node");
                     continue;
                  }

                  auto ce = GetPointer<call_node>(ga->op1);
                  called_node = ce->fn;
                  arguments = &ce->args;
                  call_ir_node_id = ce->index;
               }
               else // stmt->get_kind() == call_stmt_K
               {
                  auto gc = GetPointer<call_stmt>(stmt);
                  called_node = gc->fn;
                  arguments = &gc->args;
                  call_ir_node_id = gc->index;
               }
               if(!called_node)
               {
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Not a call_stmt nor a call_node");
                  continue;
               }
               if(called_node->get_kind() == ssa_node_K)
               {
                  const auto called_ssa_name = STR(called_node);
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                 "-->Indirect function call through ssa " + called_ssa_name);
                  const auto f_ptr = GetPointer<const pointer_ty_node>(ir_helper::CGetType(called_node));
                  THROW_ASSERT(f_ptr, "");
                  const auto ft = GetPointer<const function_ty_node>(f_ptr->ptd);
                  THROW_ASSERT(ft, "");
                  unsigned int param_n = 0;
                  for(const auto& p : ft->list_of_args_type)
                  {
                     INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Analyzing parameter type" + STR(p));
                     if(ir_helper::IsStructType(p))
                     {
                        INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                       "function ssa " + called_ssa_name +
                                           " has a struct parameter with type: " + STR(p));
                        if(ft->varargs_flag)
                        {
                           THROW_ERROR("op: " + STR(stmt) + " id: " + STR(call_ir_node_id) +
                                       " calls function pointer " + called_ssa_name +
                                       ": varargs function taking structs argument not supported");
                        }
                        const auto& actual_argument_node = arguments->at(param_n);
                        INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                       "---Actual argument " + STR(actual_argument_node) + " is " +
                                           actual_argument_node->get_kind_text());
                        THROW_ASSERT(ir_helper::IsStructType(actual_argument_node),
                                     "op: " + STR(stmt) + " id: " + STR(call_ir_node_id) + " passes argument " +
                                         STR(actual_argument_node) + " to a call to function " + called_ssa_name +
                                         " which has a struct/union parameter with type: " + STR(p) + " but " +
                                         STR(actual_argument_node) + " is a " +
                                         STR(ir_helper::CGetType(actual_argument_node)));
                        auto new_ga_node =
                            ir_man->CreateAssignStmtAddrExpr(actual_argument_node, function_id, loc_info_default);
                        INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                       "---Changing parameter: creating pointer " + STR(new_ga_node));
                        block.second->PushBefore(new_ga_node, stmt, AppM);
                        const auto new_ga = GetPointer<const assign_stmt>(new_ga_node);
                        arguments->at(param_n) = new_ga->op0;
                        changed = true;
                     }
                     INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Analyzed parameter type" + STR(p));
                  }
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                 "<--Analyzed indirect call to ssa " + called_ssa_name);
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Examined statement " + stmt->ToString());
                  continue;
               }
               THROW_ASSERT(called_node->get_kind() == addr_node_K,
                            "called_node = " + STR(called_node) + " is a " + called_node->get_kind_text());
               const auto called_fu_decl_node = GetPointerS<const addr_node>(called_node)->op;
               THROW_ASSERT(called_fu_decl_node->get_kind() == function_val_node_K,
                            "node  " + STR(called_fu_decl_node) + " is not function_val_node but " +
                                called_fu_decl_node->get_kind_text());
               const auto called_fname = ir_helper::GetFunctionName(called_fu_decl_node);
               const auto called_ftype = GetPointer<const function_ty_node>(ir_helper::CGetType(called_fu_decl_node));
               /*
                * if there is a call to a function without body we don't turn
                * structs parameters into pointers, because we would also need
                * to change the body of the function to alter how the parameter
                * is used.
                */
               if(!fd->body)
               {
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                 "<--Function " + called_fname + " is varargs but has no body");
                  continue;
               }
               if(cannot_have_struct_parameters(called_fu_decl_node, called_ftype))
               {
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Cannot have struct parameters");
                  continue;
               }
               auto p_type_head = called_ftype->list_of_args_type.begin();
               auto p_type_last = ftype->list_of_args_type.end();
               const auto has_param_types = p_type_head != p_type_last;
               unsigned int param_n = 0;
               const auto called_fd = GetPointer<const function_val_node>(called_fu_decl_node);
               auto p_decl_it = called_fd->list_of_args.begin();
               for(; p_decl_it != called_fd->list_of_args.cend(); p_decl_it++, param_n++)
               {
                  const auto& p_decl = *p_decl_it;
                  const auto p_type = ir_helper::CGetType(p_decl);
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                 "-->Analyzing parameter " + STR(p_decl) + " with type " + STR(p_type));

                  THROW_ASSERT(static_cast<bool>(has_param_types) == (p_type_head != p_type_last),
                               "function " + called_fname + " has " + STR(called_fd->list_of_args.size()) +
                                   " parameters, but argument " + STR(param_n) + " (" + STR(p_decl) +
                                   ") has not a corresponding underlying type in function_ty_node");
                  if(has_param_types)
                  {
                     ++p_type_head;
                  }

                  if(ir_helper::IsStructType(p_type))
                  {
                     INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                    "function " + called_fname + " has a struct parameter: " + STR(p_decl) +
                                        " with type " + STR(p_type));
                     if(called_ftype->varargs_flag)
                     {
                        THROW_ERROR("op: " + STR(stmt) + " id: " + STR(call_ir_node_id) + " calls function " +
                                    called_fname + ": varargs function taking structs argument not supported");
                     }
                     const auto& actual_argument_node = arguments->at(param_n);
                     INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                    "---Actual argument " + STR(actual_argument_node) + " is " +
                                        actual_argument_node->get_kind_text());
                     THROW_ASSERT(ir_helper::IsStructType(actual_argument_node),
                                  "op: " + STR(stmt) + " id: " + STR(call_ir_node_id) + " passes argument " +
                                      STR(actual_argument_node) + " to a call to function " + called_fname +
                                      " which has a struct/union parameter: " + STR(p_decl) + " but " +
                                      STR(actual_argument_node) + " is a " +
                                      STR(ir_helper::CGetType(actual_argument_node)));
                     const auto new_ga_node =
                         ir_man->CreateAssignStmtAddrExpr(actual_argument_node, function_id, loc_info_default);
                     INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                    "---Changing parameter: creating pointer " + STR(new_ga_node));
                     block.second->PushBefore(new_ga_node, stmt, AppM);
                     const auto* new_ga = GetPointer<const assign_stmt>(new_ga_node);
                     arguments->at(param_n) = new_ga->op0;
                     changed = true;
                  }
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                 "<--Analyzed parameter " + STR(p_decl) + " with type " + STR(p_type));
               }
            }
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Examined statement " + stmt->ToString());
         }
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Examined BB" + STR(block.first));
      }
   }

   if(changed)
   {
      function_behavior->UpdateBBVersion();
      return DesignFlowStep_Status::SUCCESS;
   }
   return DesignFlowStep_Status::UNCHANGED;
}
