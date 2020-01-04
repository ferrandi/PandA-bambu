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
#include "dbgPrintHelper.hpp" // for DEBUG_LEVEL_
#include "function_behavior.hpp"
#include "string_manipulation.hpp" // for GET_CLASS
#include "tree_basic_block.hpp"
#include "tree_helper.hpp"
#include "tree_manager.hpp"
#include "tree_manipulation.hpp"
#include "tree_node.hpp"
#include "tree_reindex.hpp"

FixStructsPassedByValue::FixStructsPassedByValue(const ParameterConstRef params, const application_managerRef AM, unsigned int fun_id, const DesignFlowManagerConstRef dfm) : FunctionFrontendFlowStep(AM, fun_id, FIX_STRUCTS_PASSED_BY_VALUE, dfm, params)
{
   debug_level = parameters->get_class_debug_level(GET_CLASS(*this), DEBUG_LEVEL_NONE);
}

FixStructsPassedByValue::~FixStructsPassedByValue() = default;

static bool cannot_have_struct_parameters(const function_decl* const fd, const function_type* const ft,
                                          const tree_managerConstRef
#if HAVE_ASSERTS
                                              TM
#endif
                                          ,
                                          const std::string&
#if HAVE_ASSERTS
                                              fu_name
#endif
)
{
   auto p_type_head = ft->prms;
   if(p_type_head and GetPointer<const void_type>(GET_CONST_NODE(p_type_head)))
   {
      // if the function_type takes void argument there's nothing to do
      THROW_ASSERT(fd->list_of_args.empty(), "function " + fu_name + " has void parameter type but has a parm_decl " + STR(GET_NODE(fd->list_of_args.front())));
      return true;
   }
   if(p_type_head and fd->list_of_args.empty())
   {
      while(p_type_head)
      {
         const auto* const p = GetPointer<const tree_list>(GET_CONST_NODE(p_type_head));
         p_type_head = p->chan;
         /*
          * from what I figured out from gcc, if the function_decl has no
          * no parm_decl this means that there is no explicit declaration
          * of that function in the source code. in principle it should
          * not even compile, but this is not true for certain builtin
          * functions like exit() or others. in these cases, even if the
          * function is not declared and just used, gcc compiles and maps
          * it to its own builtins and infers its own function_type that
          * can have types for parameters (p_type_head is not null).
          * anyways in all these cases it should never happen that these
          * kinds of functions take structs passed by values as arguments.
          * hence we can safely 'continue' without worries.
          * if the assertion fails some of these assumptions are wrong
          */
         THROW_ASSERT(not tree_helper::is_a_struct(TM, p->valu->index) and not tree_helper::is_an_union(TM, p->valu->index), "function " + fu_name +
                                                                                                                                 " has no parm_decl but in its "
                                                                                                                                 "function_type it takes a struct type " +
                                                                                                                                 STR(GET_NODE(p->valu)));
      }
      return true;
   }
   return false;
}

DesignFlowStep_Status FixStructsPassedByValue::InternalExec()
{
   bool changed = false;
   const tree_managerRef TM = AppM->get_tree_manager();
   const tree_manipulationRef tree_man = tree_manipulationRef(new tree_manipulation(TM, parameters));
   const tree_nodeRef tn = TM->get_tree_node_const(function_id);
   auto* fd = GetPointer<function_decl>(tn);
   THROW_ASSERT(fd and fd->body, "Node " + STR(tn) + "is not a function_decl or has no body");
   const auto* sl = GetPointer<const statement_list>(GET_NODE(fd->body));
   THROW_ASSERT(sl, "Body is not a statement_list");
   const std::string fu_name = tree_helper::name_function(TM, function_id);
   const auto* fu_type = GetPointer<const function_type>(tree_helper::CGetType(tn));
   THROW_ASSERT(not fu_type->varargs_flag, "function " + fu_name + " is varargs");
   // fix declaration
   if(not cannot_have_struct_parameters(fd, fu_type, TM, fu_name))
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Fixing declaration of function " + fu_name);
      THROW_ASSERT(tn->get_kind() != tree_reindex_K, "function declaration " + STR(tn) + " is a tree_reindex");
      unsigned int param_n = 0;
      auto p_decl_it = fd->list_of_args.begin();
      auto p_type_head = fu_type->prms;
      auto has_param_types = static_cast<bool>(p_type_head);
      for(; p_decl_it != fd->list_of_args.cend(); p_decl_it++, param_n++)
      {
         const tree_nodeConstRef p_decl = GET_NODE(*p_decl_it);
         const tree_nodeConstRef p_type = tree_helper::CGetType(p_decl);
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Analyzing parameter " + STR(p_decl) + " with type " + STR(p_type));
         THROW_ASSERT(static_cast<bool>(has_param_types) == static_cast<bool>(p_type_head),
                      "function " + fu_name + " has " + STR(fd->list_of_args.size()) + " parameters, but argument " + STR(param_n) + " (" + STR(p_decl) + ") has not a corresponding underlying type in function_type");

         if(tree_helper::is_an_union(TM, p_type->index) or tree_helper::is_a_struct(TM, p_type->index))
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "function " + fu_name + " has a struct parameter: " + STR(p_decl) + " with type " + STR(p_type));
            // initialize some general stuff useful later
            const auto* pd = GetPointer<const parm_decl>(p_decl);
            const std::string srcp = pd->include_name + ":" + STR(pd->line_number) + ":" + STR(pd->column_number);
            const std::string original_param_name = pd->name ? GetPointer<const identifier_node>(GET_NODE(pd->name))->strg : "";

            unsigned int ptd_type_size = tree_helper::Size(p_type);
            if(ptd_type_size % 8)
               ptd_type_size += 8;
            ptd_type_size /= 8;

            // create new var_decl
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Creating new local var_decl");
            const std::string local_var_name = "bambu_artificial_local_param_copy_" + original_param_name;
            const auto local_var_identifier = tree_man->create_identifier_node(local_var_name);
            const auto new_local_var_decl = tree_man->create_var_decl(local_var_identifier, TM->CGetTreeReindex(p_type->index), pd->scpe, pd->size, tree_nodeRef(), tree_nodeRef(), srcp, GetPointer<const type_node>(p_type)->algn, pd->used,
                                                                      false); // artificial flag (should be true???)
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Created new local var_decl");

            // substitute var_decl to parm_decl in all the statements of the function
            {
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Substituting var_decl " + STR(p_decl) + " to parm_decl " + STR(new_local_var_decl) + " in all statements");
               for(auto& block : sl->list_of_bloc)
               {
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Examining BB" + STR(block.first));
                  for(const auto& stmt : block.second->CGetStmtList())
                  {
                     INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Examining statement " + GET_NODE(stmt)->ToString());
                     TM->ReplaceTreeNode(stmt, TM->CGetTreeReindex(p_decl->index), new_local_var_decl);
                     INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Examined statement " + GET_NODE(stmt)->ToString());
                  }
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Examined BB" + STR(block.first));
               }
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Substituted var_decl " + STR(p_decl) + " to parm_decl " + STR(new_local_var_decl) + " in all statements");
            }

            // create pointer type for the new pointer-to-struct parameter
            const auto ptr_type = tree_man->create_pointer_type(p_type, ALGN_POINTER);

            // substitute parameter type in function_type if necessary
            if(has_param_types)
            {
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Substituting type of parameter " + STR(p_decl));
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Changing type from " + STR(p_type) + " to " + STR(GET_NODE(ptr_type)));
               GetPointer<tree_list>(GET_NODE(p_type_head))->valu = ptr_type;
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Substituted type of parameter " + STR(p_decl));
            }

            // create and substitute new pointer-to-struct parm_decl in function_decl
            {
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Substituting parm_decl of " + STR(p_decl));
               const std::string ptr_p_name = "bambu_artificial_ptr_param_" + original_param_name;
               const auto ptr_p_identifier = tree_man->create_identifier_node(ptr_p_name);
               const auto ptr_p_decl =
                   tree_man->create_parm_decl(ptr_p_identifier, ptr_type, pd->scpe, GetPointer<type_node>(GET_NODE(ptr_type))->size, ptr_type, tree_nodeRef(), tree_nodeRef(), srcp, GetPointer<const type_node>(GET_NODE(ptr_type))->algn, 1, false, true);
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Changing parm_decl from " + STR(p_decl) + " to " + STR(GET_NODE(ptr_p_decl)));
               *p_decl_it = ptr_p_decl;
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Substituted parm_decl of " + STR(p_decl) + " with " + STR(GET_NODE(*p_decl_it)));
            }

            /*
             * find the first basic block of the function. it is the
             * successor of the entry basic block that is not the exit basic
             * block.
             */
            unsigned int bb_index = BB_ENTRY;
            {
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Selecting first basic block of " + fu_name);
               const auto entry_block = sl->list_of_bloc.at(BB_ENTRY);
               const auto succ_blocks = entry_block->list_of_succ;
               THROW_ASSERT(succ_blocks.size() == 1, "entry basic block of function " + fu_name + " has " + STR(succ_blocks.size()) + " successors");
               bb_index = *(succ_blocks.begin());
               THROW_ASSERT(bb_index != BB_ENTRY and bb_index != BB_EXIT, "first basic block of function " + fu_name + " not found");
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Selected first basic block of " + fu_name + ": " + STR(bb_index));
            }
            const auto first_block = sl->list_of_bloc.at(bb_index);

            // create the call to memcpy
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Creating new call to memcpy");
            unsigned int memcpy_function_id = TM->function_index("__internal_bambu_memcpy");
            THROW_ASSERT(AppM->GetFunctionBehavior(memcpy_function_id)->GetBehavioralHelper()->has_implementation(), "inconsistent behavioral helper");
            unsigned int size_formal_type_id = tree_helper::get_formal_ith(TM, memcpy_function_id, 2);
            const auto formal_type_node = TM->CGetTreeReindex(size_formal_type_id);
            const std::vector<tree_nodeRef> args = {// & new_local_var_decl
                                                    tree_man->CreateAddrExpr(GET_NODE(new_local_var_decl), srcp),
                                                    // src is the new pointer-to-struct parm_decl
                                                    tree_man->create_ssa_name(*p_decl_it, ptr_type, tree_nodeRef(), tree_nodeRef()),
                                                    // sizeof(var_decl)
                                                    tree_man->CreateIntegerCst(formal_type_node, static_cast<long long>(ptd_type_size), TM->new_tree_node_id())};
            const auto gimple_call_memcpy = tree_man->create_gimple_call(TM->CGetTreeReindex(memcpy_function_id), args, srcp, bb_index);
            auto* gn = GetPointer<gimple_node>(GET_NODE(gimple_call_memcpy));
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
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Created new call to memcpy: " + STR(GET_NODE(gimple_call_memcpy)));
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Updating basic block");
            first_block->PushFront(gimple_call_memcpy);
            // add call to memcpy to the call graph and refresh the reached functions
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Updating call graph");
            AppM->GetCallGraphManager()->AddCallPoint(function_id, memcpy_function_id, gimple_call_memcpy->index, FunctionEdgeInfo::CallType::direct_call);
            changed = true;
         }

         if(has_param_types)
            p_type_head = GetPointer<const tree_list>(GET_CONST_NODE(p_type_head))->chan;

         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Analyzed parameter " + STR(p_decl) + " with type " + STR(p_type));
      }
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Fixed declaration of function " + fu_name);
   }
   // fix calls to other functions that accept structs passed by value as parameters
   {
      for(auto& block : sl->list_of_bloc)
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Examining BB" + STR(block.first));
         for(const auto& stmt : block.second->CGetStmtList())
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Examining statement " + GET_NODE(stmt)->ToString());
            const auto* gn = GetPointer<const gimple_node>(GET_NODE(stmt));
            const std::string srcp_default = gn->include_name + ":" + STR(gn->line_number) + ":" + STR(gn->column_number);
            if(GET_NODE(stmt)->get_kind() == gimple_assign_K or GET_NODE(stmt)->get_kind() == gimple_call_K)
            {
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Is a " + GET_NODE(stmt)->get_kind_text());
               tree_nodeConstRef called_node;
               std::vector<tree_nodeRef>* arguments;
               unsigned int call_tree_node_id = 0;

               if(GET_NODE(stmt)->get_kind() == gimple_assign_K)
               {
                  const auto* ga = GetPointer<const gimple_assign>(GET_NODE(stmt));
                  if(GET_NODE(ga->op1)->get_kind() != call_expr_K && GET_NODE(ga->op1)->get_kind() != aggr_init_expr_K)
                  {
                     INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--RHS is not a call_expr");
                     continue;
                  }

                  auto* ce = GetPointer<call_expr>(GET_NODE(ga->op1));
                  called_node = GET_NODE(ce->fn);
                  arguments = &ce->args;
                  call_tree_node_id = ce->index;
               }
               else // GET_NODE(stmt)->get_kind() == gimple_call_K
               {
                  auto* gc = GetPointer<gimple_call>(GET_NODE(stmt));
                  called_node = GET_NODE(gc->fn);
                  arguments = &gc->args;
                  call_tree_node_id = gc->index;
               }
               if(not called_node)
               {
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Not a gimple_call nor a call_expr");
                  continue;
               }
               if(called_node->get_kind() == ssa_name_K)
               {
                  const std::string called_ssa_name = STR(called_node);
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Indirect function call through ssa " + called_ssa_name);
                  const auto* f_ptr = GetPointer<const pointer_type>(tree_helper::CGetType(called_node));
                  THROW_ASSERT(f_ptr, "");
                  const auto* ft = GetPointer<const function_type>(GET_CONST_NODE(f_ptr->ptd));
                  THROW_ASSERT(ft, "");
                  unsigned int param_n = 0;
                  auto p_type_head = ft->prms;
                  while(p_type_head)
                  {
                     const auto* const p = GetPointer<const tree_list>(GET_CONST_NODE(p_type_head));
                     INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Analyzing parameter type" + STR(GET_CONST_NODE(p->valu)));
                     if(tree_helper::is_an_union(TM, p->valu->index) or tree_helper::is_a_struct(TM, p->valu->index))
                     {
                        INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "function ssa " + called_ssa_name + " has a struct parameter with type: " + STR(GET_CONST_NODE(p->valu)));
                        if(ft->varargs_flag)
                        {
                           THROW_ERROR("op: " + STR(stmt) + " id: " + STR(call_tree_node_id) + " calls function pointer " + called_ssa_name + ": varargs function taking structs argument not supported");
                        }
                        const auto& actual_argument_node = GET_NODE(arguments->at(param_n));
#if HAVE_ASSERTS
                        const auto& arg_id = actual_argument_node->index;
#endif
                        INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Actual argument " + STR(actual_argument_node) + " is " + actual_argument_node->get_kind_text());
                        THROW_ASSERT(tree_helper::is_an_union(TM, arg_id) or tree_helper::is_a_struct(TM, arg_id), "op: " + STR(stmt) + " id: " + STR(call_tree_node_id) + " passes argument " + STR(actual_argument_node) + " to a call to function " +
                                                                                                                       called_ssa_name + " which has a struct/union parameter with type: " + STR(GET_CONST_NODE(p->valu)) + " but " +
                                                                                                                       STR(actual_argument_node) + " is a " + STR(tree_helper::CGetType(actual_argument_node)));
                        auto new_ga_node = tree_man->CreateGimpleAssignAddrExpr(actual_argument_node, gn->bb_index, srcp_default);
                        INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Changing parameter: creating pointer " + STR(GET_NODE(new_ga_node)));
                        block.second->PushBefore(new_ga_node, stmt);
                        const auto* new_ga = GetPointer<const gimple_assign>(GET_NODE(new_ga_node));
                        arguments->at(param_n) = new_ga->op0;
                        changed = true;
                     }
                     INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Analyzed parameter type" + STR(GET_CONST_NODE(p->valu)));
                     p_type_head = p->chan;
                  }
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Analyzed indirect call to ssa " + called_ssa_name);
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Examined statement " + GET_NODE(stmt)->ToString());
                  continue;
               }
               THROW_ASSERT(called_node->get_kind() == addr_expr_K, "called_node = " + STR(called_node) + " is a " + called_node->get_kind_text());
               const auto* ae = GetPointer<const addr_expr>(called_node);
               const auto called_fu_decl_node = GET_NODE(ae->op);
               THROW_ASSERT(called_fu_decl_node->get_kind() == function_decl_K, "node  " + STR(called_fu_decl_node) + " is not function_decl but " + called_fu_decl_node->get_kind_text());
               const std::string called_fu_name = tree_helper::name_function(TM, called_fu_decl_node->index);
               const auto* called_fd = GetPointer<const function_decl>(called_fu_decl_node);
               const auto* called_fu_type = GetPointer<const function_type>(tree_helper::CGetType(called_fu_decl_node));
               /*
                * if there is a call to a function without body we don't turn
                * structs parameters into pointers, because we would also need
                * to change the body of the function to alter how the parameter
                * is used.
                */
               if(not fd->body)
               {
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Function " + called_fu_name + " is varargs but has no body");
                  continue;
               }
               if(cannot_have_struct_parameters(called_fd, called_fu_type, TM, called_fu_name))
               {
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Cannot have struct parameters");
                  continue;
               }
               auto p_type_head = called_fu_type->prms;
               auto has_param_types = static_cast<bool>(p_type_head);
               unsigned int param_n = 0;
               auto p_decl_it = called_fd->list_of_args.begin();
               for(; p_decl_it != called_fd->list_of_args.cend(); p_decl_it++, param_n++)
               {
                  const tree_nodeConstRef p_decl = GET_NODE(*p_decl_it);
                  const tree_nodeConstRef p_type = tree_helper::CGetType(p_decl);
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Analyzing parameter " + STR(p_decl) + " with type " + STR(p_type));

                  THROW_ASSERT(static_cast<bool>(has_param_types) == static_cast<bool>(p_type_head),
                               "function " + called_fu_name + " has " + STR(called_fd->list_of_args.size()) + " parameters, but argument " + STR(param_n) + " (" + STR(p_decl) + ") has not a corresponding underlying type in function_type");
                  if(has_param_types)
                     p_type_head = GetPointer<const tree_list>(GET_CONST_NODE(p_type_head))->chan;

                  if(tree_helper::is_an_union(TM, p_type->index) or tree_helper::is_a_struct(TM, p_type->index))
                  {
                     INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "function " + called_fu_name + " has a struct parameter: " + STR(p_decl) + " with type " + STR(p_type));
                     if(called_fu_type->varargs_flag)
                     {
                        THROW_ERROR("op: " + STR(stmt) + " id: " + STR(call_tree_node_id) + " calls function " + called_fu_name + ": varargs function taking structs argument not supported");
                     }
                     const auto& actual_argument_node = GET_NODE(arguments->at(param_n));
#if HAVE_ASSERTS
                     const auto& arg_id = actual_argument_node->index;
#endif
                     INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Actual argument " + STR(actual_argument_node) + " is " + actual_argument_node->get_kind_text());
                     THROW_ASSERT(tree_helper::is_an_union(TM, arg_id) or tree_helper::is_a_struct(TM, arg_id), "op: " + STR(stmt) + " id: " + STR(call_tree_node_id) + " passes argument " + STR(actual_argument_node) + " to a call to function " +
                                                                                                                    called_fu_name + " which has a struct/union parameter: " + STR(p_decl) + " but " + STR(actual_argument_node) + " is a " +
                                                                                                                    STR(tree_helper::CGetType(actual_argument_node)));
                     auto new_ga_node = tree_man->CreateGimpleAssignAddrExpr(actual_argument_node, gn->bb_index, srcp_default);
                     INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Changing parameter: creating pointer " + STR(GET_NODE(new_ga_node)));
                     block.second->PushBefore(new_ga_node, stmt);
                     const auto* new_ga = GetPointer<const gimple_assign>(GET_NODE(new_ga_node));
                     arguments->at(param_n) = new_ga->op0;
                     changed = true;
                  }
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Analyzed parameter " + STR(p_decl) + " with type " + STR(p_type));
               }
            }
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Examined statement " + GET_NODE(stmt)->ToString());
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

const CustomUnorderedSet<std::pair<FrontendFlowStepType, FrontendFlowStep::FunctionRelationship>> FixStructsPassedByValue::ComputeFrontendRelationships(const DesignFlowStep::RelationshipType relationship_type) const
{
   CustomUnorderedSet<std::pair<FrontendFlowStepType, FunctionRelationship>> relationships;
   switch(relationship_type)
   {
      case(DEPENDENCE_RELATIONSHIP):
      {
         relationships.insert(std::make_pair(BLOCK_FIX, SAME_FUNCTION));
         relationships.insert(std::make_pair(STRING_CST_FIX, WHOLE_APPLICATION));
         relationships.insert(std::make_pair(FIX_STRUCTS_PASSED_BY_VALUE, CALLING_FUNCTIONS));
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
