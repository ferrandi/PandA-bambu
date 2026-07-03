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
 * @file determine_memory_accesses.cpp
 * @brief Class to determine the variable to be stored in memory
 *
 * @author Christian Pilato <pilato@elet.polimi.it>
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 *
 */
#include "determine_memory_accesses.hpp"

#include "Parameter.hpp"
#include "application_manager.hpp"
#include "behavioral_helper.hpp"
#include "call_graph.hpp"
#include "call_graph_manager.hpp"
#include "constant_strings.hpp"
#include "dbgPrintHelper.hpp"
#include "function_behavior.hpp"
#include "ir_basic_block.hpp"
#include "ir_helper.hpp"
#include "ir_manager.hpp"
#include "ir_node.hpp"
#include "kmp_bambu_names.h"
#include "module_interface.hpp"
#include "op_graph.hpp"
#include "string_manipulation.hpp"

#define VAL(str) #str
#define TOSTRING(str) VAL(str)

static std::vector<std::string> pointer_safe_function_prefix = {STR_CST_interface_parameter_keyword,
                                                                TOSTRING(KMP_FORK_CALL), TOSTRING(KMP_SET_REDUCE_DATA),
                                                                TOSTRING(KMP_GET_REDUCE_DATA)};

static bool is_pointer_safe(const std::string& fname)
{
   for(const auto& fprefix : pointer_safe_function_prefix)
   {
      if(fname.find(fprefix) != std::string::npos)
      {
         return true;
      }
   }
   return false;
}

determine_memory_accesses::determine_memory_accesses(const ParameterConstRef _parameters,
                                                     const application_managerRef _AppM, unsigned int _function_id,
                                                     const DesignFlowManager& _design_flow_manager)
    : FunctionFrontendFlowStep(_AppM, _function_id, DETERMINE_MEMORY_ACCESSES, _design_flow_manager, _parameters),
      behavioral_helper(function_behavior->CGetBehavioralHelper()),
      TM(_AppM->get_ir_manager())
{
   debug_level = parameters->get_class_debug_level(GET_CLASS(*this), DEBUG_LEVEL_NONE);
}

CustomUnorderedSet<std::pair<FrontendFlowStepType, FrontendFlowStep::FunctionRelationship>>
determine_memory_accesses::ComputeFrontendRelationships(const DesignFlowStep::RelationshipType relationship_type) const
{
   CustomUnorderedSet<std::pair<FrontendFlowStepType, FunctionRelationship>> relationships;
   switch(relationship_type)
   {
      case(DEPENDENCE_RELATIONSHIP):
      {
         relationships.insert(std::make_pair(DETERMINE_MEMORY_ACCESSES, CALLED_FUNCTIONS));
         relationships.insert(std::make_pair(FUNCTION_CALL_TYPE_CLEANUP, SAME_FUNCTION));
         relationships.insert(std::make_pair(PARM2SSA, SAME_FUNCTION));
         relationships.insert(std::make_pair(USE_COUNTING, SAME_FUNCTION));
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

DesignFlowStep_Status determine_memory_accesses::InternalExec()
{
   const auto tn = TM->GetIRNode(function_id);
   const auto fd = GetPointer<const function_val_node>(tn);
   if(!fd || !fd->body)
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Node is not a function or it hasn't a body");
      return DesignFlowStep_Status::UNCHANGED;
   }
   const CustomOrderedSet<unsigned int> before_function_mem = function_behavior->get_function_mem();
   const bool before_has_globals = function_behavior->get_has_globals();
   const CustomOrderedSet<unsigned int> before_state_variables = function_behavior->get_state_variables();
   const CustomOrderedSet<unsigned int> before_dynamic_address = function_behavior->get_dynamic_address();
   const CustomOrderedSet<unsigned int> before_parm_decl_copied = function_behavior->get_parm_decl_copied();
   const CustomOrderedSet<unsigned int> before_parm_decl_loaded = function_behavior->get_parm_decl_loaded();
   const CustomOrderedSet<unsigned int> before_parm_decl_stored = function_behavior->get_parm_decl_stored();
   const bool before_dereference_unknown_addr = function_behavior->get_dereference_unknown_addr();
   const bool before_has_undefined_function_receiving_pointers =
       function_behavior->get_has_undefined_function_receiving_pointers();

   /// cleanup data structure
   function_behavior->clean_function_mem();
   function_behavior->set_has_globals(false);
   function_behavior->clean_state_variable();
   function_behavior->clean_dynamic_address();
   function_behavior->clean_parm_decl_copied();
   function_behavior->clean_parm_decl_loaded();
   function_behavior->clean_parm_decl_stored();
   function_behavior->set_dereference_unknown_addr(false);
   function_behavior->set_has_undefined_function_receiveing_pointers(false);

   /// analyze formal parameters
   for(const auto& formal : fd->list_of_args)
   {
      analyze_node(formal, false, false, false);
   }

   const auto sl = GetPointer<const statement_list_node>(fd->body);
   THROW_ASSERT(sl, "Body is not a statement_list_node");
   for(const auto& bb : sl->list_of_bloc)
   {
      if(bb.second->number == BB_ENTRY || bb.second->number == BB_EXIT)
      {
         continue;
      }
      for(const auto& phi : bb.second->CGetPhiList())
      {
         analyze_node(phi, false, false, false);
      }
      for(const auto& stmt : bb.second->CGetStmtList())
      {
         analyze_node(stmt, false, false, false);
      }
   }

   if(debug_level >= DEBUG_LEVEL_PEDANTIC && parameters->getOption<bool>(OPT_print_dot))
   {
      AppM->CGetCallGraphManager().GetCallGraph().writeDot(
          parameters->getOption<std::filesystem::path>(OPT_dot_directory) / "call_graph_memory_analysis.dot");
   }
   /// mem clean up
   already_visited_ae.clear();
   already_visited.clear();

   bool changed = before_function_mem != function_behavior->get_function_mem() ||
                  before_has_globals != function_behavior->get_has_globals() ||
                  before_state_variables != function_behavior->get_state_variables() ||
                  before_dynamic_address != function_behavior->get_dynamic_address() ||
                  before_parm_decl_copied != function_behavior->get_parm_decl_copied() ||
                  before_parm_decl_loaded != function_behavior->get_parm_decl_loaded() ||
                  before_parm_decl_stored != function_behavior->get_parm_decl_stored() ||
                  before_dereference_unknown_addr != function_behavior->get_dereference_unknown_addr() ||
                  before_has_undefined_function_receiving_pointers !=
                      function_behavior->get_has_undefined_function_receiving_pointers();
   return changed ? DesignFlowStep_Status::SUCCESS : DesignFlowStep_Status::UNCHANGED;
}

void determine_memory_accesses::analyze_call(const ir_nodeConstRef& tn, const ir_nodeRef& fn,
                                             const std::vector<ir_nodeRef>& args)
{
   // The first parameter of a call_node can be a ssa_node in
   // case of function pointer usage.  When it happens skip the
   // following analysis.
   if(fn->get_kind() != addr_node_K)
   {
      return;
   }
   const auto fnode = GetPointerS<const addr_node>(fn)->op;
   THROW_ASSERT(fnode->get_kind() == function_val_node_K, "expected a function_val_node");
   if(ir_helper::GetFunctionName(fnode) == BUILTIN_WAIT_CALL)
   {
      function_behavior->add_function_mem(tn->index);
      AppM->add_written_object(tn->index);
   }
   const auto fd = GetPointerS<const function_val_node>(fnode);
   const auto is_var_args_p = GetPointerS<const function_ty_node>(fd->type)->varargs_flag;
   const auto has_pointers_as_actual_parameters = [&]() {
      for(const auto& arg : args)
      {
         if(ir_helper::IsPointerType(arg))
            return true;
      }
      return false;
   }();
   if(ir_helper::IsFunctionImplemented(fnode))
   {
      if(AppM->GetFunctionBehavior(fnode->index)->get_unaligned_accesses())
      {
         function_behavior->set_unaligned_accesses(true);
      }
      if(!(is_var_args_p || fd->list_of_args.size() == args.size()))
      {
         THROW_ERROR("In function " + behavioral_helper->GetFunctionName() +
                     " a different number of formal and actual parameters is found when function " +
                     ir_helper::GetFunctionName(fnode) + " is called: " + STR(fd->list_of_args.size()) + " - " +
                     STR(args.size()) +
                     "\n Check the C source code since an actual parameter is passed to a function that does "
                     "have the associated formal parameter");
      }
      auto formal_it = fd->list_of_args.cbegin();
      const auto formal_it_end = fd->list_of_args.cend();
      auto arg = args.cbegin();
      const auto arg_end = args.cend();
      for(; arg != arg_end && formal_it != formal_it_end; ++arg, ++formal_it)
      {
         auto actual_par = *arg;
         const auto formal_par = *formal_it;
         if(ir_helper::IsPointerType(actual_par) && ir_helper::GetBaseVariable(actual_par))
         {
            actual_par = ir_helper::GetBaseVariable(actual_par);
         }
         const auto FBcalled = AppM->GetFunctionBehavior(fnode->index);
         /// check if the actual parameter has been allocated in memory
         const auto formal_ssa_index = AppM->getSSAFromParm(fnode->index, formal_par->index);
         if(formal_ssa_index && function_behavior->is_variable_mem(actual_par->index))
         {
            const auto formal_ssa_node = TM->GetIRNode(formal_ssa_index);
            const auto formal_ssa = GetPointerS<const ssa_node>(formal_ssa_node);
            const auto is_singleton = formal_ssa->use_set.is_a_singleton() &&
                                      actual_par->index == formal_ssa->use_set.variables.front()->index;
            if(!is_singleton)
            {
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                              "---Variable for which the dynamic address is used-5: " +
                                  behavioral_helper->PrintVariable(actual_par->index));
               function_behavior->add_dynamic_address(actual_par->index);
               AppM->add_written_object(actual_par->index);
               /// if the formal parameter has not been allocated in memory then it has to be initialized
               if(!FBcalled->is_variable_mem(formal_par->index) && (*arg)->index == actual_par->index)
               {
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                 "---actual parameter loaded " + STR(actual_par));
                  function_behavior->add_parm_decl_loaded(actual_par->index);
               }
            }
         }
         else if(!formal_ssa_index)
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Parameter is not used in the function body.");
         }

         /// check if the formal parameter has been allocated in memory.
         if(FBcalled->is_variable_mem(formal_par->index))
         {
            /// If the actual has not been allocated in memory then the formal parameter storage has to be
            /// initialized with the actual value with a MEMSTORE_STD
            const auto actual_par_node = *arg;
            switch(actual_par->get_kind())
            {
               case ssa_node_K:
               {
                  if(!function_behavior->is_variable_mem(actual_par->index))
                  {
                     INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                    "---formal parameter stored " + STR(formal_par->index));
                     FBcalled->add_parm_decl_stored(formal_par->index);
                     FBcalled->add_dynamic_address(formal_par->index);
                     AppM->add_written_object(formal_par->index);
                  }
                  break;
               }
               case constant_fp_val_node_K:
               case constant_int_val_node_K:
               case addr_node_K:
               {
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                 "---formal parameter stored " + STR(formal_par->index));
                  FBcalled->add_parm_decl_stored(formal_par->index);
                  FBcalled->add_dynamic_address(formal_par->index);
                  AppM->add_written_object(formal_par->index);
                  break;
               }
               case unaligned_mem_access_node_K:
               {
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                 "---formal parameter copied " + STR(formal_par->index));
                  FBcalled->add_parm_decl_copied(formal_par->index);
                  FBcalled->add_dynamic_address(formal_par->index);
                  AppM->add_written_object(formal_par->index);
                  const auto arg_op_type = ir_helper::CGetType(actual_par_node);
                  // records have to be allocated
                  if(arg_op_type->get_kind() == struct_ty_node_K)
                  {
                     analyze_node(actual_par, false, true, false);
                  }
                  break;
               }
               case mem_access_node_K:
               case call_node_K:
               case constructor_node_K:
               case identifier_node_K:
               case statement_list_node_K:
               case abs_node_K:
               case not_node_K:
               case fptoi_node_K:
               case itofp_node_K:
               case neg_node_K:
               case nop_node_K:
               case bitcast_node_K:
               case select_node_K:
               case shufflevector_node_K:
               case ternary_add_node_K:
               case ternary_as_node_K:
               case ternary_sa_node_K:
               case ternary_ss_node_K:
               case fshl_node_K:
               case fshr_node_K:
               case concat_bit_node_K:
               case constant_vector_val_node_K:
               case lut_node_K:
               case insertvalue_node_K:
               case insertelement_node_K:
               case CASE_BINARY_NODES:
               case CASE_DECL_NODES:
               case CASE_FAKE_NODES:
               case CASE_NODE_STMTS:
               case CASE_TYPE_NODES:
               default:
               {
                  THROW_ASSERT(function_behavior->is_variable_mem(actual_par->index),
                               "actual parameter non allocated in memory: calling @" + STR(fnode->index) + " actual " +
                                   actual_par->ToString());
                  break;
               }
            }
         }
      }
   }
   else
   {
      if(has_pointers_as_actual_parameters && !is_pointer_safe(ir_helper::GetFunctionName(fnode)))
      {
         function_behavior->set_has_undefined_function_receiveing_pointers(true);
         function_behavior->set_unaligned_accesses(true);
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---set unaligned access for current function");
      }
   }
}

void determine_memory_accesses::analyze_node(const ir_nodeConstRef& tn, bool left_p, bool dynamic_address,
                                             bool no_dynamic_address)
{
   const auto node_id = tn->index;
   const auto tn_kind = tn->get_kind();
   if(tn_kind != addr_node_K && tn_kind != variable_val_node_K)
   {
      if(already_visited.find(node_id) != already_visited.end())
      {
         return;
      }
      else
      {
         already_visited.insert(node_id);
      }
   }
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                  "-->Analyzing node " + tn->ToString() +
                      " - Dynamic address: " + (dynamic_address ? " true" : "false") +
                      " - No dynamic address: " + (no_dynamic_address ? "true" : "false"));
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Operation type: " + tn->get_kind_text());
   const auto function_name = behavioral_helper->GetFunctionName();

   switch(tn_kind)
   {
      case assign_stmt_K:
      {
         const auto gm = GetPointerS<const assign_stmt>(tn);
         // std::cerr << "assign statement " << node_id << " " << tn << std::endl;
         analyze_node(gm->op0, true, false, false);
         analyze_node(gm->op1, false, false, gm->temporary_address);
         if(gm->predicate)
         {
            analyze_node(gm->predicate, false, true, false);
         }

         /// check for implicit memcpy calls
         const auto op0 = gm->op0;
         const auto op1 = gm->op1;
         const auto op0_kind = op0->get_kind();
         const auto op1_kind = op1->get_kind();
         const auto op0_type = ir_helper::CGetType(op0);
         const auto op1_type = ir_helper::CGetType(op1);

         auto load_candidate = op1_kind == unaligned_mem_access_node_K || op1_kind == mem_access_node_K;
         auto store_candidate = op0_kind == unaligned_mem_access_node_K || op0_kind == mem_access_node_K;
         if(op0_type && op1_type && op1->get_kind() != insertvalue_node_K && op1->get_kind() != extractvalue_node_K &&
            ((op0_type->get_kind() == struct_ty_node_K && op1_type->get_kind() == struct_ty_node_K &&
              op1_kind != bitcast_node_K) ||
             (op0_type->get_kind() == array_ty_node_K) ||
             (function_behavior->is_variable_mem(gm->op0->index) &&
              function_behavior->is_variable_mem(gm->op1->index)) ||
             (function_behavior->is_variable_mem(gm->op0->index) && load_candidate) ||
             (store_candidate && function_behavior->is_variable_mem(gm->op1->index))))
         {
            if(op0_kind == mem_access_node_K)
            {
               const auto mr = GetPointerS<const mem_access_node>(op0);
               analyze_node(mr->op, true, true, false);
            }
            else
            {
               analyze_node(gm->op0, true, true, false);
            }

            if(op1_kind == mem_access_node_K)
            {
               const auto mr = GetPointerS<const mem_access_node>(op1);
               analyze_node(mr->op, true, true, false);
            }
            else
            {
               analyze_node(gm->op1, false, true, false);
            }
            if(op1_kind == constructor_node_K && GetPointerS<const constructor_node>(op1) &&
               GetPointerS<const constructor_node>(op1)->list_of_idx_valu.empty())
            {
               /// manage temporary addresses
               const auto ref_var = ir_helper::GetBaseVariable(gm->op0);
               if(ref_var)
               {
                  analyze_node(ref_var, true, true, false);
               }
            }
            else
            {
               /// manage temporary addresses
               auto ref_var = ir_helper::GetBaseVariable(gm->op0);
               if(ref_var)
               {
                  analyze_node(ref_var, true, true, false);
               }
               ref_var = ir_helper::GetBaseVariable(gm->op1);
               if(ref_var)
               {
                  analyze_node(ref_var, false, true, false);
               }
            }
         }
         break;
      }
      case CASE_UNARY_NODES:
      {
         const auto ue = GetPointerS<const unary_node>(tn);
         const auto ue_op_kind = ue->op->get_kind();
         if(tn_kind == addr_node_K)
         {
            if(ue_op_kind == variable_val_node_K)
            {
               const auto vd = GetPointerS<const variable_val_node>(ue->op);
               bool address_externally_used = false;

               function_behavior->add_function_mem(vd->index);
               function_behavior->add_state_variable(vd->index);
               if((((!vd->parent || vd->parent->get_kind() == module_unit_node_K) && !vd->static_flag)))
               {
                  if(parameters->isOption(OPT_expose_globals) && parameters->getOption<bool>(OPT_expose_globals))
                  {
                     address_externally_used = true;
                     INDENT_OUT_MEX(OUTPUT_LEVEL_VERBOSE, output_level,
                                    "---Global variable externally accessible found: " +
                                        behavioral_helper->PrintVariable(ue->op->index));
                  }
                  function_behavior->set_has_globals(true);
               }

               if((!no_dynamic_address || address_externally_used))
               {
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                 "---Variable for which the dynamic address is used-1: " +
                                     behavioral_helper->PrintVariable(vd->index) +
                                     (function_behavior->is_variable_mem(vd->index) ? "T" : "F") + " " +
                                     STR(vd->index));
                  function_behavior->add_dynamic_address(vd->index);
                  if(!vd->readonly_flag)
                  {
                     AppM->add_written_object(vd->index);
                  }
               }
               if(left_p && !vd->readonly_flag)
               {
                  AppM->add_written_object(vd->index);
               }
               if(already_visited_ae.insert(node_id).second)
               {
                  if(vd->init)
                  {
                     analyze_node(vd->init, left_p, false, false);
                  }
               }
            }
            else if(ue_op_kind == argument_val_node_K)
            {
               function_behavior->add_function_mem(ue->op->index);
               if(!no_dynamic_address)
               {
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                 "---Variable for which the dynamic address is used-2: " +
                                     behavioral_helper->PrintVariable(ue->op->index));
                  function_behavior->add_dynamic_address(ue->op->index);
               }
               /// an address of a parm decl may be used in writing so it has to be copied
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                              "Analyzing node: formal parameter copied " + STR(ue->op->index));
               function_behavior->add_parm_decl_copied(ue->op->index);
               AppM->add_written_object(ue->op->index);
            }
            else if(ue_op_kind == function_val_node_K)
            {
               analyze_node(ue->op, false, !no_dynamic_address, no_dynamic_address);
            }
            else if(ue_op_kind == mem_access_node_K)
            {
               const auto mr = GetPointerS<const mem_access_node>(ue->op);
               analyze_node(mr->op, left_p, !no_dynamic_address, no_dynamic_address);
            }
            else
            {
               THROW_ERROR_CODE(NODE_NOT_YET_SUPPORTED_EC,
                                "determine_memory_accesses addressing currently not supported: " +
                                    ue->op->get_kind_text() + " @" + STR(node_id) + " in function " + function_name);
            }
         }
         else if(tn_kind == bitcast_node_K)
         {
            const auto bitcast_expr = GetPointerS<const bitcast_node>(tn);
            analyze_node(bitcast_expr->op, left_p, dynamic_address, no_dynamic_address);
         }
         else if(tn_kind == mem_access_node_K)
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---It is a mem ref");
            if(ir_helper::IsConstant(ue->op))
            {
               function_behavior->set_dereference_unknown_addr(true);
            }
            if(!dynamic_address)
            {
               dynamic_address = false;
               no_dynamic_address = true;
            }
            if(left_p)
            {
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Left part");
            }
            const auto ref_var = ir_helper::GetBaseVariable(ue->op);
            if(ref_var)
            {
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Referenced variable is " + STR(ref_var));
               const bool is_variable_mem = [&]() {
                  if(function_behavior->is_variable_mem(ref_var->index))
                  {
                     INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Already classified as memory variable");
                     return true;
                  }
                  const auto vd = GetPointer<const variable_val_node>(ref_var);
                  if(!vd)
                  {
                     INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Not a variable");
                     return false;
                  }
                  if(vd->readonly_flag && !left_p)
                  {
                     INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---ReadOnly variable");
                     THROW_ERROR("ReadOnly variable on lhs");
                     return false;
                  }
                  if(vd->static_flag)
                  {
                     function_behavior->add_function_mem(ref_var->index);
                     INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Static variable");
                     return true;
                  }
                  if(!vd->parent || vd->parent->get_kind() == module_unit_node_K)
                  {
                     function_behavior->add_function_mem(ref_var->index);
                     INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Not local");
                     return true;
                  }
                  if(vd->parent)
                  {
                     function_behavior->add_function_mem(ref_var->index);
                     INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---local variable of another function");
                     return true;
                  }
                  return false;
               }();
               if(is_variable_mem && left_p)
               {
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---It is written");
                  AppM->add_written_object(ref_var->index);
               }
            }
            analyze_node(ue->op, left_p, dynamic_address, no_dynamic_address);
         }
         else
         {
            analyze_node(ue->op, left_p, dynamic_address, no_dynamic_address);
         }
         break;
      }
      case CASE_BINARY_NODES:
      {
         const auto be = GetPointerS<const binary_node>(tn);
         analyze_node(be->op0, left_p, dynamic_address, no_dynamic_address);
         analyze_node(be->op1, left_p, dynamic_address, no_dynamic_address);
         break;
      }
      case multi_way_if_stmt_K:
      {
         const auto gmwi = GetPointerS<const multi_way_if_stmt>(tn);
         for(const auto& cond : gmwi->list_of_cond)
         {
            if(cond.first)
            {
               analyze_node(cond.first, left_p, dynamic_address, no_dynamic_address);
            }
         }
         break;
      }
      case phi_stmt_K:
      {
         const auto gp = GetPointerS<const phi_stmt>(tn);
         for(const auto& def_edge : gp->CGetDefEdgesList())
         {
            analyze_node(def_edge.first, left_p, dynamic_address, no_dynamic_address);
         }
         break;
      }
      case CASE_TERNARY_NODES:
      {
         const auto te = GetPointerS<const ternary_node>(tn);

         if(te->op0)
         {
            analyze_node(te->op0, left_p, dynamic_address, no_dynamic_address);
         }
         if(te->op1)
         {
            analyze_node(te->op1, left_p, dynamic_address, no_dynamic_address);
         }
         if(te->op2)
         {
            analyze_node(te->op2, left_p, dynamic_address, no_dynamic_address);
         }
         break;
      }
      case lut_node_K:
      {
         const auto le = GetPointerS<const lut_node>(tn);
         analyze_node(le->op0, left_p, dynamic_address, no_dynamic_address);
         analyze_node(le->op1, left_p, dynamic_address, no_dynamic_address);
         if(le->op2)
         {
            analyze_node(le->op2, left_p, dynamic_address, no_dynamic_address);
         }
         if(le->op3)
         {
            analyze_node(le->op3, left_p, dynamic_address, no_dynamic_address);
         }
         if(le->op4)
         {
            analyze_node(le->op4, left_p, dynamic_address, no_dynamic_address);
         }
         if(le->op5)
         {
            analyze_node(le->op5, left_p, dynamic_address, no_dynamic_address);
         }
         if(le->op6)
         {
            analyze_node(le->op6, left_p, dynamic_address, no_dynamic_address);
         }
         if(le->op7)
         {
            analyze_node(le->op7, left_p, dynamic_address, no_dynamic_address);
         }
         if(le->op8)
         {
            analyze_node(le->op8, left_p, dynamic_address, no_dynamic_address);
         }
         break;
      }
      case return_stmt_K:
      {
         const auto re = GetPointerS<const return_stmt>(tn);
         if(re->op)
         {
            const auto res_type = ir_helper::CGetType(re->op);
            // records have to be allocated
            if(res_type->get_kind() == struct_ty_node_K)
            {
               THROW_ERROR_CODE(NODE_NOT_YET_SUPPORTED_EC,
                                "structs or unions returned by copy are not yet supported: @" + STR(node_id) +
                                    " in function " + function_name);
               function_behavior->add_function_mem(node_id);
               function_behavior->add_parm_decl_copied(node_id);
               AppM->add_written_object(node_id);
            }
            analyze_node(re->op, left_p, dynamic_address, no_dynamic_address);
         }
         break;
      }
      case call_node_K:
      {
         const auto ce = GetPointerS<const call_node>(tn);
         analyze_call(tn, ce->fn, ce->args);
         break;
      }
      case call_stmt_K:
      {
         const auto ce = GetPointerS<const call_stmt>(tn);
         analyze_call(tn, ce->fn, ce->args);
         if(ce->predicate)
         {
            analyze_node(ce->predicate, false, true, false);
         }
         break;
      }
      case ssa_node_K:
      {
         const auto sn = GetPointerS<const ssa_node>(tn);
         if(sn->use_set.is_fully_resolved())
         {
            for(const auto& var : sn->use_set.variables)
            {
               if(var->get_kind() != function_val_node_K)
               {
                  function_behavior->add_function_mem(var->index);
               }
            }
         }
         break;
      }
      case constant_vector_val_node_K:
      case constant_fp_val_node_K:
      case constant_int_val_node_K:
      {
         break;
      }
      case argument_val_node_K:
      {
         const auto pd = GetPointerS<const argument_val_node>(tn);
         // records have to be allocated
         if(pd->type->get_kind() == struct_ty_node_K)
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                           "---Variable for which the dynamic address is used-8: " +
                               behavioral_helper->PrintVariable(node_id));
            function_behavior->add_function_mem(node_id);
            function_behavior->add_dynamic_address(node_id);
            AppM->add_written_object(node_id);
            if(left_p)
            {
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                              "Analyzing node: formal parameter copied " + STR(node_id));
               function_behavior->add_parm_decl_copied(node_id);
            }
         }
         break;
      }
      case variable_val_node_K:
      {
         const auto vd = GetPointerS<const variable_val_node>(tn);
         if(vd->extern_flag)
         {
            THROW_ERROR_CODE(C_EC, "Extern symbols not yet supported " + behavioral_helper->PrintVariable(node_id));
         }
         if(!vd->parent ||
            vd->parent->get_kind() == module_unit_node_K) // memory has to be allocated in case of global variables
         {
            function_behavior->add_function_mem(node_id);
            bool address_externally_used = false;
            if((!vd->static_flag))
            {
               if(parameters->isOption(OPT_expose_globals) && parameters->getOption<bool>(OPT_expose_globals))
               {
                  address_externally_used = true;
                  INDENT_OUT_MEX(OUTPUT_LEVEL_VERBOSE, output_level,
                                 "---Global variable externally accessible found: " +
                                     behavioral_helper->PrintVariable(node_id));
               }
               function_behavior->set_has_globals(true);
            }
            function_behavior->add_state_variable(node_id);
            if((dynamic_address && !no_dynamic_address && !vd->addr_not_taken) || address_externally_used)
            {
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                              "---Variable for which the dynamic address is used-9: " +
                                  behavioral_helper->PrintVariable(node_id));
               function_behavior->add_dynamic_address(node_id);
               if(!vd->readonly_flag)
               {
                  AppM->add_written_object(node_id);
               }
            }
            if(left_p && !vd->readonly_flag)
            {
               AppM->add_written_object(node_id);
            }
            if(vd->init)
            {
               analyze_node(vd->init, false, false, false);
            }
         }
         else
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Local variable");
            THROW_ASSERT(vd->parent->get_kind() != module_unit_node_K,
                         "module_unit_node not expected a translation unit in this point @" + STR(node_id));
            if(vd->static_flag || // memory has to be allocated in case of local static variables
               vd->type->get_kind() == array_ty_node_K || // arrays have to be allocated
               vd->type->get_kind() == struct_ty_node_K   // records have to be allocated
            )
            {
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---It has to be allocated");
               bool address_externally_used = false;
               if(vd->static_flag)
               {
                  function_behavior->add_state_variable(node_id);
               }
               function_behavior->add_function_mem(node_id);
               if((dynamic_address && !no_dynamic_address && !vd->addr_not_taken) || address_externally_used)
               {
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                 "---Variable for which the dynamic address is used-10: " +
                                     behavioral_helper->PrintVariable(node_id));
                  function_behavior->add_dynamic_address(node_id);
                  if(!vd->readonly_flag)
                  {
                     AppM->add_written_object(node_id);
                  }
               }
               if(left_p && !vd->readonly_flag)
               {
                  AppM->add_written_object(node_id);
               }
            }
            else
            {
               // nothing have to be allocated for the variable
               // maybe something has to be allocated for its initialization
               if(vd->init)
               {
                  analyze_node(vd->init, left_p, false, false);
               }
            }
         }
         break;
      }
      case constructor_node_K:
      {
         const auto con = GetPointerS<const constructor_node>(tn);
         for(const auto& [idx, valu] : con->list_of_idx_valu)
         {
            if(idx)
            {
               analyze_node(idx, left_p, dynamic_address, no_dynamic_address);
            }
            if(valu)
            {
               analyze_node(valu, left_p, dynamic_address, no_dynamic_address);
            }
         }
         break;
      }
      case nop_stmt_K:
      case field_val_node_K:
      {
         break;
      }
      case function_val_node_K:
      {
         break;
      }
      case CASE_FAKE_NODES:
      case identifier_node_K:
      case statement_list_node_K:
      case module_unit_node_K:
      case CASE_TYPE_NODES:
      {
         THROW_ERROR_CODE(NODE_NOT_YET_SUPPORTED_EC, "Not supported node (@" + STR(node_id) + ") of type " +
                                                         std::string(tn->get_kind_text()) + " in function " +
                                                         function_name);
         break;
      }
      default:
      {
         THROW_UNREACHABLE("");
      }
   }
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Analyzed node " + tn->ToString());
}
