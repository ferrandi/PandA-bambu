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
 * @file determine_memory_accesses.cpp
 * @brief Class to determine the variable to be stored in memory
 *
 * @author Christian Pilato <pilato@elet.polimi.it>
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *
 */

/// Header include
#include "determine_memory_accesses.hpp"
#include "config_HAVE_PRAGMA_BUILT.hpp"

/// Behavior include
#include "application_manager.hpp"
#include "call_graph.hpp"
#include "call_graph_manager.hpp"
#include "function_behavior.hpp"

/// Parameter include
#include "Parameter.hpp"
#include "module_interface.hpp"

/// Tree include
#include "behavioral_helper.hpp"
#include "dbgPrintHelper.hpp" // for DEBUG_LEVEL_
#include "ext_tree_node.hpp"
#include "op_graph.hpp"
#include "string_manipulation.hpp" // for GET_CLASS
#include "tree_basic_block.hpp"
#include "tree_helper.hpp"
#include "tree_manager.hpp"
#include "tree_node.hpp"

determine_memory_accesses::determine_memory_accesses(const ParameterConstRef _parameters,
                                                     const application_managerRef _AppM, unsigned int _function_id,
                                                     const DesignFlowManagerConstRef _design_flow_manager)
    : FunctionFrontendFlowStep(_AppM, _function_id, DETERMINE_MEMORY_ACCESSES, _design_flow_manager, _parameters),
      behavioral_helper(function_behavior->CGetBehavioralHelper()),
      TM(_AppM->get_tree_manager())
{
   debug_level = parameters->get_class_debug_level(GET_CLASS(*this), DEBUG_LEVEL_NONE);
}

determine_memory_accesses::~determine_memory_accesses() = default;

CustomUnorderedSet<std::pair<FrontendFlowStepType, FrontendFlowStep::FunctionRelationship>>
determine_memory_accesses::ComputeFrontendRelationships(const DesignFlowStep::RelationshipType relationship_type) const
{
   CustomUnorderedSet<std::pair<FrontendFlowStepType, FunctionRelationship>> relationships;
   switch(relationship_type)
   {
      case(DEPENDENCE_RELATIONSHIP):
      {
         if(parameters->isOption(OPT_writer_language))
         {
            relationships.insert(std::make_pair(HDL_VAR_DECL_FIX, SAME_FUNCTION));
         }
         else
         {
            relationships.insert(std::make_pair(VAR_DECL_FIX, SAME_FUNCTION));
         }
         relationships.insert(std::make_pair(CALL_GRAPH_BUILTIN_CALL, SAME_FUNCTION));
         relationships.insert(std::make_pair(DETERMINE_MEMORY_ACCESSES, CALLED_FUNCTIONS));
         relationships.insert(std::make_pair(FIX_STRUCTS_PASSED_BY_VALUE, SAME_FUNCTION));
         relationships.insert(std::make_pair(FUNCTION_CALL_TYPE_CLEANUP, SAME_FUNCTION));
         relationships.insert(std::make_pair(IR_LOWERING, SAME_FUNCTION));
         relationships.insert(std::make_pair(PARM_DECL_TAKEN_ADDRESS, SAME_FUNCTION));
         relationships.insert(std::make_pair(PARM2SSA, SAME_FUNCTION));
         relationships.insert(std::make_pair(REBUILD_INITIALIZATION, SAME_FUNCTION));
         relationships.insert(std::make_pair(REBUILD_INITIALIZATION2, SAME_FUNCTION));
         relationships.insert(std::make_pair(UN_COMPARISON_LOWERING, SAME_FUNCTION));
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
   const auto tn = TM->GetTreeNode(function_id);
   const auto fd = GetPointer<const function_decl>(tn);
   if(!fd || !fd->body)
   {
      PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Node is not a function or it hasn't a body");
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

   const auto sl = GetPointer<const statement_list>(fd->body);
   THROW_ASSERT(sl, "Body is not a statement_list");
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
      AppM->CGetCallGraphManager()->CGetCallGraph()->WriteDot("call_graph_memory_analysis.dot");
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

void determine_memory_accesses::analyze_node(const tree_nodeConstRef& tn, bool left_p, bool dynamic_address,
                                             bool no_dynamic_address)
{
   const auto node_id = tn->index;
   const auto tn_kind = tn->get_kind();
   if(tn_kind != addr_expr_K && tn_kind != var_decl_K)
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
   std::string function_name = behavioral_helper->get_function_name();

   const auto gn = GetPointer<const gimple_node>(tn);
   if(gn && gn->use_set)
   {
      for(const auto& usv : gn->use_set->variables)
      {
         analyze_node(usv, false, true, false);
      }
   }

   switch(tn_kind)
   {
      case gimple_assign_K:
      {
         const auto gm = GetPointerS<const gimple_assign>(tn);
         if(!gm->init_assignment)
         {
            // std::cerr << "gimple assign " << node_id << " " << tn << std::endl;
            analyze_node(gm->op0, true, false, false);
            analyze_node(gm->op1, false, false, gm->temporary_address);

            /// check for implicit memcpy calls
            const auto op0 = gm->op0;
            const auto op1 = gm->op1;
            const auto op0_kind = op0->get_kind();
            const auto op1_kind = op1->get_kind();
            const auto op0_type = tree_helper::CGetType(op0);
            const auto op1_type = tree_helper::CGetType(op1);

            bool is_a_vector_bitfield = false;
            if(op1_kind == bit_field_ref_K)
            {
               const auto bfr = GetPointerS<const bit_field_ref>(op1);
               if(tree_helper::IsVectorType(bfr->op0))
               {
                  is_a_vector_bitfield = true;
               }
            }

            auto load_candidate =
                (op1_kind == bit_field_ref_K && !is_a_vector_bitfield) || op1_kind == component_ref_K ||
                op1_kind == indirect_ref_K || op1_kind == misaligned_indirect_ref_K || op1_kind == mem_ref_K ||
                op1_kind == array_ref_K || op1_kind == target_mem_ref_K || op1_kind == target_mem_ref461_K;
            if(op1_kind == realpart_expr_K || op1_kind == imagpart_expr_K)
            {
               const auto code1 = GetPointerS<const unary_expr>(op1)->op->get_kind();
               if((code1 == bit_field_ref_K && !is_a_vector_bitfield) || code1 == component_ref_K ||
                  code1 == indirect_ref_K || code1 == bit_field_ref_K || code1 == misaligned_indirect_ref_K ||
                  code1 == mem_ref_K || code1 == array_ref_K || code1 == target_mem_ref_K ||
                  code1 == target_mem_ref461_K)
               {
                  load_candidate = true;
               }
               if(code1 == var_decl_K &&
                  function_behavior->is_variable_mem(GetPointerS<const unary_expr>(op1)->op->index))
               {
                  load_candidate = true;
               }
            }
            auto store_candidate = op0_kind == bit_field_ref_K || op0_kind == component_ref_K ||
                                   op0_kind == indirect_ref_K || op0_kind == misaligned_indirect_ref_K ||
                                   op0_kind == mem_ref_K || op0_kind == array_ref_K || op0_kind == target_mem_ref_K ||
                                   op0_kind == target_mem_ref461_K;
            if(op0_kind == realpart_expr_K || op0_kind == imagpart_expr_K)
            {
               const auto code0 = GetPointerS<const unary_expr>(op0)->op->get_kind();
               if((code0 == bit_field_ref_K) || code0 == component_ref_K || code0 == indirect_ref_K ||
                  code0 == misaligned_indirect_ref_K || code0 == mem_ref_K || code0 == array_ref_K ||
                  code0 == target_mem_ref_K || code0 == target_mem_ref461_K)
               {
                  store_candidate = true;
               }
               if(code0 == var_decl_K &&
                  function_behavior->is_variable_mem(GetPointerS<const unary_expr>(op0)->op->index))
               {
                  store_candidate = true;
               }
            }
            if(!gm->clobber && !gm->init_assignment && op0_type && op1_type && op1->get_kind() != insertvalue_expr_K &&
               op1->get_kind() != extractvalue_expr_K &&
               ((op0_type->get_kind() == record_type_K && op1_type->get_kind() == record_type_K &&
                 op1_kind != view_convert_expr_K) ||
                (op0_type->get_kind() == union_type_K && op1_type->get_kind() == union_type_K &&
                 op1_kind != view_convert_expr_K) ||
                (op0_type->get_kind() == array_type_K) ||
                (function_behavior->is_variable_mem(gm->op0->index) &&
                 function_behavior->is_variable_mem(gm->op1->index)) ||
                (function_behavior->is_variable_mem(gm->op0->index) && load_candidate) ||
                (store_candidate && function_behavior->is_variable_mem(gm->op1->index))))
            {
               if(op0_kind == mem_ref_K)
               {
                  const auto mr = GetPointerS<const mem_ref>(op0);
                  analyze_node(mr->op0, true, true, false);
               }
               else if(op0_kind == target_mem_ref461_K)
               {
                  const auto tmr = GetPointerS<const target_mem_ref461>(op0);
                  if(tmr->base)
                  {
                     analyze_node(tmr->base, true, true, false);
                  }
                  else
                  {
                     analyze_node(gm->op0, true, true, false);
                  }
               }
               else
               {
                  analyze_node(gm->op0, true, true, false);
               }

               if(op1_kind == mem_ref_K)
               {
                  const auto mr = GetPointerS<const mem_ref>(op1);
                  analyze_node(mr->op0, true, true, false);
               }
               else if(op1_kind == target_mem_ref461_K)
               {
                  const auto tmr = GetPointerS<const target_mem_ref461>(op1);
                  if(tmr->base)
                  {
                     analyze_node(tmr->base, true, true, false);
                  }
                  else
                  {
                     analyze_node(gm->op1, true, true, false);
                  }
               }
               else
               {
                  analyze_node(gm->op1, false, true, false);
               }
               if(gm->predicate)
               {
                  analyze_node(gm->predicate, false, true, false);
               }

               if(op1_kind == constructor_K && GetPointerS<const constructor>(op1) &&
                  GetPointerS<const constructor>(op1)->list_of_idx_valu.empty())
               {
                  /// manage temporary addresses
                  const auto ref_var = tree_helper::GetBaseVariable(gm->op0);
                  if(ref_var)
                  {
                     analyze_node(ref_var, true, true, false);
                  }
               }
               else
               {
                  /// manage temporary addresses
                  auto ref_var = tree_helper::GetBaseVariable(gm->op0);
                  if(ref_var)
                  {
                     analyze_node(ref_var, true, true, false);
                  }
                  ref_var = tree_helper::GetBaseVariable(gm->op1);
                  if(ref_var)
                  {
                     analyze_node(ref_var, false, true, false);
                  }
               }
            }
         }
         break;
      }
      case CASE_UNARY_EXPRESSION:
      {
         const auto ue = GetPointerS<const unary_expr>(tn);
         const auto ue_op_kind = ue->op->get_kind();
         if(GetPointer<const addr_expr>(tn))
         {
            if(ue_op_kind == var_decl_K)
            {
               const auto vd = GetPointerS<const var_decl>(ue->op);
               bool address_externally_used = false;
               function_behavior->add_function_mem(ue->op->index);
               if((((!vd->scpe || vd->scpe->get_kind() == translation_unit_decl_K) && !vd->static_flag) ||
                   tree_helper::IsVolatile(tn)))
               {
                  if(parameters->isOption(OPT_expose_globals) && parameters->getOption<bool>(OPT_expose_globals))
                  {
                     address_externally_used = true;
                  }
                  function_behavior->set_has_globals(true);
                  function_behavior->add_state_variable(ue->op->index);
                  if(address_externally_used)
                  {
                     INDENT_OUT_MEX(OUTPUT_LEVEL_VERBOSE, output_level,
                                    "---Global variable externally accessible found: " +
                                        behavioral_helper->PrintVariable(ue->op->index));
                  }
               }
               else
               {
                  function_behavior->add_state_variable(ue->op->index);
               }

               if((!no_dynamic_address || address_externally_used))
               {
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                 "---Variable for which the dynamic address is used-1: " +
                                     behavioral_helper->PrintVariable(ue->op->index));
                  function_behavior->add_dynamic_address(ue->op->index);
                  if(!vd->readonly_flag)
                  {
                     AppM->add_written_object(ue->op->index);
                  }
               }
               if(left_p && !vd->readonly_flag)
               {
                  AppM->add_written_object(ue->op->index);
               }
               if(already_visited_ae.find(node_id) == already_visited_ae.end())
               {
                  already_visited_ae.insert(node_id);

                  if(vd->init && vd->init->get_kind() != string_cst_K)
                  {
                     analyze_node(vd->init, left_p, false, false);
                  }
               }
            }
            else if(ue_op_kind == parm_decl_K)
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
               PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                             "  Analyzing node: formal parameter copied " + STR(ue->op->index));
               function_behavior->add_parm_decl_copied(ue->op->index);
               AppM->add_written_object(ue->op->index);
            }
            else if(ue_op_kind == string_cst_K)
            {
               function_behavior->add_function_mem(ue->op->index);
               if(!no_dynamic_address)
               {
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                 "---Variable for which the dynamic address is used-3: " +
                                     behavioral_helper->PrintVariable(ue->op->index));
                  function_behavior->add_dynamic_address(ue->op->index);
                  AppM->add_written_object(ue->op->index);
               }
            }
            else if(ue_op_kind == result_decl_K)
            {
               function_behavior->add_function_mem(ue->op->index);
               if(!no_dynamic_address)
               {
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                 "---Variable for which the dynamic address is used-4: " +
                                     behavioral_helper->PrintVariable(ue->op->index));
                  function_behavior->add_dynamic_address(ue->op->index);
                  AppM->add_written_object(ue->op->index);
               }
               INDENT_OUT_MEX(OUTPUT_LEVEL_VERBOSE, output_level,
                              "---result_decl variable added to memory: " +
                                  behavioral_helper->PrintVariable(ue->op->index));
            }
            else if(ue_op_kind == component_ref_K || ue_op_kind == realpart_expr_K || ue_op_kind == imagpart_expr_K ||
                    ue_op_kind == array_ref_K)
            {
               analyze_node(ue->op, true, !no_dynamic_address, no_dynamic_address);
            }
            else if(ue_op_kind == function_decl_K)
            {
               analyze_node(ue->op, false, !no_dynamic_address, no_dynamic_address);
            }
            else if(ue_op_kind == mem_ref_K)
            {
               const auto mr = GetPointerS<const mem_ref>(ue->op);
               analyze_node(mr->op0, left_p, !no_dynamic_address, no_dynamic_address);
            }
            else if(ue_op_kind == target_mem_ref461_K)
            {
               const auto tmr = GetPointerS<const target_mem_ref461>(ue->op);
               if(tmr->base)
               {
                  analyze_node(tmr->base, left_p, !no_dynamic_address, no_dynamic_address);
               }
               else
               {
                  analyze_node(ue->op, left_p, !no_dynamic_address, no_dynamic_address);
               }
            }
            else
            {
               THROW_ERROR_CODE(NODE_NOT_YET_SUPPORTED_EC,
                                "determine_memory_accesses addressing currently not supported: " +
                                    ue->op->get_kind_text() + " @" + STR(node_id) + " in function " + function_name);
            }
         }
         else if(tn_kind == view_convert_expr_K)
         {
            const auto vc = GetPointerS<const view_convert_expr>(tn);
            analyze_node(vc->op, left_p, dynamic_address, no_dynamic_address);
         }
         else if(tn_kind == indirect_ref_K)
         {
            const auto ir = GetPointerS<const indirect_ref>(tn);
            if(GetPointer<const integer_cst>(ir->op))
            {
               function_behavior->set_dereference_unknown_addr(true);
            }
            if(!dynamic_address)
            {
               dynamic_address = false;
               no_dynamic_address = true;
            }
            analyze_node(ir->op, left_p, dynamic_address, no_dynamic_address);
         }
         else
         {
            analyze_node(ue->op, left_p, dynamic_address, no_dynamic_address);
         }
         break;
      }
      case CASE_BINARY_EXPRESSION:
      {
         const auto be = GetPointerS<const binary_expr>(tn);
         if(tn_kind == mem_ref_K)
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---It is a mem ref");
            const auto mr = GetPointerS<const mem_ref>(tn);
            if(GetPointer<const integer_cst>(mr->op0))
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
               const auto ref_var = tree_helper::GetBaseVariable(mr->op0);
               if(ref_var)
               {
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Referenced variable is " + STR(ref_var));
                  const bool is_variable_mem = [&]() {
                     if(function_behavior->is_variable_mem(ref_var->index))
                     {
                        INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                       "---Already classified as memory variable");
                        return true;
                     }
                     const auto vd = GetPointer<const var_decl>(ref_var);
                     if(!vd)
                     {
                        INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Not a variable");
                        return false;
                     }
                     if(vd->readonly_flag)
                     {
                        INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---ReadOnly variable");
                        THROW_ERROR("ReadOnly variable on lhs");
                        return false;
                     }
                     if(vd->static_flag)
                     {
                        INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Static variable");
                        return true;
                     }
                     if(!vd->scpe or vd->scpe->get_kind() == translation_unit_decl_K)
                     {
                        INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Not local");
                        return true;
                     }
                     if(vd->scpe)
                     {
                        INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---local variable of another function");
                        return true;
                     }
                     if(tree_helper::IsVolatile(ref_var))
                     {
                        INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Volatile");
                        return true;
                     }
                     const auto type_kind = vd->type->get_kind();
                     /*
                      * TODO: the following if should look like this
                      *     if (type_kind == array_type_K or type_kind == record_type_K or type_kind == union_type_K)
                      * because the complex types should not be memory allocated
                      * anymore. however changing it results in failures during
                      * allocation due to missing complex components in the
                      * technology library.
                      * This issue should be further investigated.
                      */
                     if(type_kind == array_type_K or type_kind == complex_type_K or type_kind == record_type_K or
                        type_kind == union_type_K)
                     {
                        INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Memory allocated");
                        return true;
                     }
                     return false;
                  }();
                  if(is_variable_mem)
                  {
                     INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---It is written");
                     AppM->add_written_object(ref_var->index);
                  }
               }
            }
         }
         analyze_node(be->op0, left_p, dynamic_address, no_dynamic_address);
         analyze_node(be->op1, left_p, dynamic_address, no_dynamic_address);
         break;
      }
      case gimple_cond_K:
      {
         const auto gc = GetPointerS<const gimple_cond>(tn);
         analyze_node(gc->op0, false, false, true);
         break;
      }
      case gimple_switch_K:
      {
         const auto se = GetPointerS<const gimple_switch>(tn);
         if(se->op0)
         {
            analyze_node(se->op0, left_p, dynamic_address, no_dynamic_address);
         }
         break;
      }
      case gimple_multi_way_if_K:
      {
         const auto gmwi = GetPointerS<const gimple_multi_way_if>(tn);
         for(const auto& cond : gmwi->list_of_cond)
         {
            if(cond.first)
            {
               analyze_node(cond.first, left_p, dynamic_address, no_dynamic_address);
            }
         }
         break;
      }
      case gimple_phi_K:
      {
         const auto gp = GetPointerS<const gimple_phi>(tn);
         for(const auto& def_edge : gp->CGetDefEdgesList())
         {
            analyze_node(def_edge.first, left_p, dynamic_address, no_dynamic_address);
         }
         break;
      }
      case CASE_TERNARY_EXPRESSION:
      {
         const auto te = GetPointerS<const ternary_expr>(tn);
         if(tn_kind == component_ref_K)
         {
            left_p = true;
         }
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
      case CASE_QUATERNARY_EXPRESSION:
      {
         const auto qe = GetPointerS<const quaternary_expr>(tn);
         if(qe->op0)
         {
            analyze_node(qe->op0, left_p, dynamic_address, no_dynamic_address);
         }
         if(qe->op1)
         {
            analyze_node(qe->op1, left_p, dynamic_address, no_dynamic_address);
         }
         if(qe->op2)
         {
            analyze_node(qe->op2, left_p, dynamic_address, no_dynamic_address);
         }
         if(qe->op3)
         {
            analyze_node(qe->op3, left_p, dynamic_address, no_dynamic_address);
         }
         break;
      }
      case lut_expr_K:
      {
         const auto le = GetPointerS<const lut_expr>(tn);
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
      case gimple_return_K:
      {
         const auto re = GetPointerS<const gimple_return>(tn);
         if(re->op)
         {
            const auto res_type = tree_helper::CGetType(re->op);
            if(res_type->get_kind() == record_type_K || // records have to be allocated
               res_type->get_kind() == union_type_K     // unions have to be allocated
            )
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
      case call_expr_K:
      case aggr_init_expr_K:
      {
         const auto ce = GetPointerS<const call_expr>(tn);
         const auto& args = ce->args;
         const auto ae = GetPointerS<const addr_expr>(ce->fn);

         // The first parameter of a call_expr can be a ssa_name in
         // case of function pointer usage.  When it happens skip the
         // following analysis.
         if(!ae)
         {
            break;
         }

         if(AppM->GetFunctionBehavior(ae->op->index)->get_unaligned_accesses())
         {
            function_behavior->set_unaligned_accesses(true);
         }
         const auto fd = GetPointer<function_decl>(ae->op);
         bool is_var_args_p = GetPointer<function_type>(fd->type)->varargs_flag;
         THROW_ASSERT(fd, "expected a function_decl");
         bool has_pointers_as_actual_parameters = false;
         for(const auto& arg : ce->args)
         {
            analyze_node(arg, left_p, dynamic_address, no_dynamic_address);
            has_pointers_as_actual_parameters |= tree_helper::IsPointerType(arg);
         }
         if(!fd->undefined_flag)
         {
            if(!(is_var_args_p || fd->list_of_args.size() == args.size()))
            {
               THROW_ERROR("In function " + function_name +
                           " a different number of formal and actual parameters is found when function " +
                           tree_helper::GetMangledFunctionName(fd) + " is called: " + STR(fd->list_of_args.size()) +
                           " - " + STR(args.size()) +
                           "\n Check the C source code since an actual parameter is passed to a function that does "
                           "have the associated formal parameter");
            }
            auto formal_it = fd->list_of_args.cbegin();
            const auto formal_it_end = fd->list_of_args.cend();
            auto arg = ce->args.cbegin();
            const auto arg_end = ce->args.cend();
            for(; arg != arg_end && formal_it != formal_it_end; ++arg, ++formal_it)
            {
               auto actual_par = *arg;
               const auto formal_par = *formal_it;
               unsigned int calledFundID = ae->op->index;
               if(tree_helper::IsPointerType(actual_par) && tree_helper::GetBaseVariable(actual_par))
               {
                  actual_par = tree_helper::GetBaseVariable(actual_par);
               }
               const auto FBcalled = AppM->GetFunctionBehavior(calledFundID);
               /// check if the actual parameter has been allocated in memory
               const auto formal_ssa_index = AppM->getSSAFromParm(calledFundID, formal_par->index);
               if(function_behavior->is_variable_mem(actual_par->index) && formal_ssa_index)
               {
                  const auto formal_ssa_node = TM->GetTreeNode(formal_ssa_index);
                  const auto formal_ssa = GetPointerS<const ssa_name>(formal_ssa_node);
                  const auto is_singleton = formal_ssa->use_set->is_a_singleton() &&
                                            actual_par->index == formal_ssa->use_set->variables.front()->index;
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
                        PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                      "  Analyzing node: actual parameter loaded " + STR(actual_par));
                        function_behavior->add_parm_decl_loaded(actual_par->index);
                     }
                  }
               }
               else if(!formal_ssa_index)
               {
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                 "---Parameter is not used in the function body.");
               }

               /// check if the formal parameter has been allocated in memory.
               if(FBcalled->is_variable_mem(formal_par->index))
               {
                  /// If the actual has not been allocated in memory then the formal parameter storage has to be
                  /// initialized with the actual value with a MEMSTORE_STD
                  const auto actual_par_node = *arg;
                  switch(actual_par->get_kind())
                  {
                     case ssa_name_K:
                     {
                        if(!function_behavior->is_variable_mem(actual_par->index))
                        {
                           PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                         "  Analyzing node: formal parameter stored " + STR(formal_par->index));
                           FBcalled->add_parm_decl_stored(formal_par->index);
                           FBcalled->add_dynamic_address(formal_par->index);
                           AppM->add_written_object(formal_par->index);
                        }
                        break;
                     }
                     case real_cst_K:
                     case string_cst_K:
                     case integer_cst_K:
                     case addr_expr_K:
                     {
                        PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                      "  Analyzing node: formal parameter stored " + STR(formal_par->index));
                        FBcalled->add_parm_decl_stored(formal_par->index);
                        FBcalled->add_dynamic_address(formal_par->index);
                        AppM->add_written_object(formal_par->index);
                        if(actual_par->get_kind() == string_cst_K)
                        {
                           INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                          "---Variable for which the dynamic address is used-6: " +
                                              behavioral_helper->PrintVariable(actual_par->index));
                           function_behavior->add_dynamic_address(actual_par->index);
                           AppM->add_written_object(actual_par->index);
                        }
                        break;
                     }
                     case misaligned_indirect_ref_K:
                     case indirect_ref_K:
                     case array_ref_K:
                     case component_ref_K:
                     {
                        PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                      "  Analyzing node: formal parameter copied " + STR(formal_par->index));
                        FBcalled->add_parm_decl_copied(formal_par->index);
                        FBcalled->add_dynamic_address(formal_par->index);
                        AppM->add_written_object(formal_par->index);
                        const auto arg_op_type = tree_helper::CGetType(actual_par_node);
                        if(arg_op_type->get_kind() == record_type_K || // records have to be allocated
                           arg_op_type->get_kind() == union_type_K     // unions have to be allocated
                        )
                        {
                           analyze_node(actual_par, left_p, true, false);
                        }
                        break;
                     }
                     case binfo_K:
                     case block_K:
                     case call_expr_K:
                     case aggr_init_expr_K:
                     case case_label_expr_K:
                     case constructor_K:
                     case identifier_node_K:
                     case statement_list_K:
                     case target_mem_ref_K:
                     case target_mem_ref461_K:
                     case tree_list_K:
                     case tree_vec_K:
                     case abs_expr_K:
                     case alignof_expr_K:
                     case arrow_expr_K:
                     case bit_not_expr_K:
                     case buffer_ref_K:
                     case card_expr_K:
                     case cleanup_point_expr_K:
                     case conj_expr_K:
                     case convert_expr_K:
                     case exit_expr_K:
                     case fix_ceil_expr_K:
                     case fix_floor_expr_K:
                     case fix_round_expr_K:
                     case fix_trunc_expr_K:
                     case float_expr_K:
                     case imagpart_expr_K:
                     case loop_expr_K:
                     case negate_expr_K:
                     case non_lvalue_expr_K:
                     case nop_expr_K:
                     case paren_expr_K:
                     case realpart_expr_K:
                     case reference_expr_K:
                     case reinterpret_cast_expr_K:
                     case sizeof_expr_K:
                     case static_cast_expr_K:
                     case throw_expr_K:
                     case truth_not_expr_K:
                     case unsave_expr_K:
                     case va_arg_expr_K:
                     case view_convert_expr_K:
                     case reduc_max_expr_K:
                     case reduc_min_expr_K:
                     case reduc_plus_expr_K:
                     case vec_unpack_hi_expr_K:
                     case vec_unpack_lo_expr_K:
                     case vec_unpack_float_hi_expr_K:
                     case vec_unpack_float_lo_expr_K:
                     case bit_field_ref_K:
                     case vtable_ref_K:
                     case with_cleanup_expr_K:
                     case obj_type_ref_K:
                     case save_expr_K:
                     case cond_expr_K:
                     case vec_cond_expr_K:
                     case vec_perm_expr_K:
                     case dot_prod_expr_K:
                     case ternary_plus_expr_K:
                     case ternary_pm_expr_K:
                     case ternary_mp_expr_K:
                     case ternary_mm_expr_K:
                     case fshl_expr_K:
                     case fshr_expr_K:
                     case bit_ior_concat_expr_K:
                     case complex_cst_K:
                     case vector_cst_K:
                     case void_cst_K:
                     case array_range_ref_K:
                     case target_expr_K:
                     case error_mark_K:
                     case lut_expr_K:
                     case insertvalue_expr_K:
                     case insertelement_expr_K:
                     case CASE_BINARY_EXPRESSION:
                     case CASE_CPP_NODES:
                     case CASE_DECL_NODES:
                     case CASE_FAKE_NODES:
                     case CASE_GIMPLE_NODES:
                     case CASE_PRAGMA_NODES:
                     case CASE_TYPE_NODES:
                     default:
                     {
                        THROW_ASSERT(function_behavior->is_variable_mem(actual_par->index),
                                     "actual parameter non allocated in memory: calling @" + STR(calledFundID) +
                                         " actual " + actual_par->ToString());
                        break;
                     }
                  }
               }
            }
         }
         else
         {
            if(has_pointers_as_actual_parameters)
            {
               function_behavior->set_has_undefined_function_receiveing_pointers(true);
               function_behavior->set_unaligned_accesses(true);
            }
         }
         break;
      }
      case gimple_call_K:
      {
         const auto ce = GetPointerS<const gimple_call>(tn);
         const auto ae = GetPointerS<const addr_expr>(ce->fn);
         // The first parameter of a call_expr can be a ssa_name in
         // case of function pointer usage.  When it happens skip the
         // following analysis.
         if(!ae)
         {
            break;
         }

         const auto fd = GetPointerS<const function_decl>(ae->op);
         if(tree_helper::print_function_name(TM, fd) == BUILTIN_WAIT_CALL)
         {
            function_behavior->add_function_mem(node_id);
            AppM->add_written_object(node_id);
         }

         bool is_var_args_p = GetPointer<function_type>(fd->type)->varargs_flag;
         THROW_ASSERT(fd, "expected a function_decl");
         bool has_pointers_as_actual_parameters = false;
         for(const auto& arg : ce->args)
         {
            analyze_node(arg, left_p, dynamic_address, no_dynamic_address);
            has_pointers_as_actual_parameters |= tree_helper::IsPointerType(arg);
         }
         if(!fd->undefined_flag)
         {
            if(!(is_var_args_p || fd->list_of_args.size() == ce->args.size()))
            {
               THROW_ERROR("In function " + function_name +
                           " a different number of formal and actual parameters is found when function " +
                           tree_helper::GetMangledFunctionName(fd) + " is called: " + STR(fd->list_of_args.size()) +
                           " - " + STR(ce->args.size()) +
                           "\n Check the C source code since an actual parameter is passed to a function that does "
                           "have the associated formal parameter");
            }
            auto formal_it = fd->list_of_args.cbegin();
            const auto formal_it_end = fd->list_of_args.cend();
            auto arg = ce->args.cbegin();
            const auto arg_end = ce->args.cend();
            for(; arg != arg_end && formal_it != formal_it_end; ++arg, ++formal_it)
            {
               auto actual_par = *arg;
               const auto formal_par = *formal_it;
               unsigned int calledFundID = ae->op->index;
               if(tree_helper::IsPointerType(actual_par) && tree_helper::GetBaseVariable(actual_par))
               {
                  actual_par = tree_helper::GetBaseVariable(actual_par);
               }
               const auto FBcalled = AppM->GetFunctionBehavior(calledFundID);
               /// check if the actual parameter has been allocated in memory
               const auto formal_ssa_index = AppM->getSSAFromParm(calledFundID, formal_par->index);
               if(function_behavior->is_variable_mem(actual_par->index) && formal_ssa_index)
               {
                  const auto formal_ssa_node = TM->GetTreeNode(formal_ssa_index);
                  const auto formal_ssa = GetPointer<const ssa_name>(formal_ssa_node);
                  const auto is_singleton = formal_ssa->use_set->is_a_singleton() &&
                                            actual_par->index == formal_ssa->use_set->variables.front()->index;
                  if(!is_singleton)
                  {
                     INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                    "---Variable for which the dynamic address is used-7: " +
                                        behavioral_helper->PrintVariable(actual_par->index));
                     function_behavior->add_dynamic_address(actual_par->index);
                     AppM->add_written_object(actual_par->index);
                     /// if the formal parameter has not been allocated in memory then it has to be initialized
                     if(!FBcalled->is_variable_mem(formal_par->index) && (*arg)->index == actual_par->index)
                     {
                        INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                       "---actual parameter loaded " + STR(actual_par->index));
                        function_behavior->add_parm_decl_loaded(actual_par->index);
                     }
                  }
               }
               else if(!formal_ssa_index)
               {
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                 "---Parameter is not used in the function body.");
               }
               /// check if the formal parameter has been allocated in memory.
               if(FBcalled->is_variable_mem(formal_par->index))
               {
                  /// If the actual has not been allocated in memory then the formal parameter storage has to be
                  /// initialized with the actual value with a MEMSTORE_STD
                  const auto actual_par_node = *arg;
                  switch(actual_par->get_kind())
                  {
                     case ssa_name_K:
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
                     case real_cst_K:
                     case string_cst_K:
                     case integer_cst_K:
                     case addr_expr_K:
                     {
                        INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                       "---formal parameter stored " + STR(formal_par->index));
                        FBcalled->add_parm_decl_stored(formal_par->index);
                        FBcalled->add_dynamic_address(formal_par->index);
                        AppM->add_written_object(formal_par->index);
                        if(actual_par->get_kind() == string_cst_K)
                        {
                           function_behavior->add_dynamic_address(actual_par->index);
                           AppM->add_written_object(actual_par->index);
                        }
                        break;
                     }
                     case misaligned_indirect_ref_K:
                     case indirect_ref_K:
                     case array_ref_K:
                     case component_ref_K:
                     {
                        INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                       "---formal parameter copied " + STR(formal_par->index));
                        FBcalled->add_parm_decl_copied(formal_par->index);
                        FBcalled->add_dynamic_address(formal_par->index);
                        AppM->add_written_object(formal_par->index);
                        const auto arg_op_type = tree_helper::CGetType(actual_par_node);
                        if(arg_op_type->get_kind() == record_type_K || // records have to be allocated
                           arg_op_type->get_kind() == union_type_K     // unions have to be allocated
                        )
                        {
                           analyze_node(actual_par, left_p, true, false);
                        }
                        break;
                     }
                     case binfo_K:
                     case block_K:
                     case call_expr_K:
                     case aggr_init_expr_K:
                     case case_label_expr_K:
                     case constructor_K:
                     case identifier_node_K:
                     case statement_list_K:
                     case target_mem_ref_K:
                     case target_mem_ref461_K:
                     case tree_list_K:
                     case tree_vec_K:
                     case abs_expr_K:
                     case alignof_expr_K:
                     case arrow_expr_K:
                     case bit_not_expr_K:
                     case buffer_ref_K:
                     case card_expr_K:
                     case cleanup_point_expr_K:
                     case conj_expr_K:
                     case convert_expr_K:
                     case exit_expr_K:
                     case fix_ceil_expr_K:
                     case fix_floor_expr_K:
                     case fix_round_expr_K:
                     case fix_trunc_expr_K:
                     case float_expr_K:
                     case imagpart_expr_K:
                     case loop_expr_K:
                     case negate_expr_K:
                     case non_lvalue_expr_K:
                     case nop_expr_K:
                     case paren_expr_K:
                     case realpart_expr_K:
                     case reference_expr_K:
                     case reinterpret_cast_expr_K:
                     case sizeof_expr_K:
                     case static_cast_expr_K:
                     case throw_expr_K:
                     case truth_not_expr_K:
                     case unsave_expr_K:
                     case va_arg_expr_K:
                     case view_convert_expr_K:
                     case reduc_max_expr_K:
                     case reduc_min_expr_K:
                     case reduc_plus_expr_K:
                     case vec_unpack_hi_expr_K:
                     case vec_unpack_lo_expr_K:
                     case vec_unpack_float_hi_expr_K:
                     case vec_unpack_float_lo_expr_K:
                     case bit_field_ref_K:
                     case vtable_ref_K:
                     case with_cleanup_expr_K:
                     case obj_type_ref_K:
                     case save_expr_K:
                     case cond_expr_K:
                     case dot_prod_expr_K:
                     case ternary_plus_expr_K:
                     case ternary_pm_expr_K:
                     case ternary_mp_expr_K:
                     case ternary_mm_expr_K:
                     case fshl_expr_K:
                     case fshr_expr_K:
                     case bit_ior_concat_expr_K:
                     case vec_cond_expr_K:
                     case vec_perm_expr_K:
                     case complex_cst_K:
                     case vector_cst_K:
                     case void_cst_K:
                     case array_range_ref_K:
                     case target_expr_K:
                     case error_mark_K:
                     case lut_expr_K:
                     case insertvalue_expr_K:
                     case insertelement_expr_K:
                     case CASE_BINARY_EXPRESSION:
                     case CASE_CPP_NODES:
                     case CASE_DECL_NODES:
                     case CASE_FAKE_NODES:
                     case CASE_GIMPLE_NODES:
                     case CASE_PRAGMA_NODES:
                     case CASE_TYPE_NODES:
                     default:
                     {
                        THROW_ASSERT(function_behavior->is_variable_mem(actual_par->index),
                                     "actual parameter non allocated in memory: calling @" + STR(calledFundID) +
                                         " actual " + actual_par->ToString());
                        break;
                     }
                  }
               }
            }
         }
         else
         {
            if(has_pointers_as_actual_parameters)
            {
               function_behavior->set_has_undefined_function_receiveing_pointers(true);
               function_behavior->set_unaligned_accesses(true);
            }
         }
         break;
      }
      case ssa_name_K:
      {
         const auto sn = GetPointerS<const ssa_name>(tn);
         if(sn->use_set->is_fully_resolved())
         {
            for(const auto& var : sn->use_set->variables)
            {
               function_behavior->add_function_mem(var->index);
            }
         }
         break;
      }
      case vector_cst_K:
      case void_cst_K:
      case real_cst_K:
      case integer_cst_K:
      case gimple_label_K:
      case label_decl_K:
      case complex_cst_K:
      {
         break;
      }
      case string_cst_K:
      {
         function_behavior->add_function_mem(node_id);
         if(dynamic_address && !no_dynamic_address)
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                           "---Variable for which the dynamic address is used-8: " +
                               behavioral_helper->PrintVariable(node_id));
            function_behavior->add_dynamic_address(node_id);
            AppM->add_written_object(node_id);
         }
         if(left_p)
         {
            AppM->add_written_object(node_id);
         }
         break;
      }
      case parm_decl_K:
      {
         const auto pd = GetPointerS<const parm_decl>(tn);
         if(pd->type->get_kind() == record_type_K || // records have to be allocated
            pd->type->get_kind() == union_type_K     // unions have to be allocated
         )
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                           "---Variable for which the dynamic address is used-9: " +
                               behavioral_helper->PrintVariable(node_id));
            function_behavior->add_function_mem(node_id);
            function_behavior->add_dynamic_address(node_id);
            AppM->add_written_object(node_id);
            if(left_p)
            {
               PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                             "  Analyzing node: formal parameter copied " + STR(node_id));
               function_behavior->add_parm_decl_copied(node_id);
            }
         }
         break;
      }
      case result_decl_K:
      {
         const auto rd = GetPointerS<const result_decl>(tn);
         if(rd->type->get_kind() == record_type_K || // records have to be allocated
            rd->type->get_kind() == union_type_K     // unions have to be allocated
         )
         {
            THROW_ERROR_CODE(C_EC, "structs or unions returned by copy are not yet supported: @" + STR(node_id) +
                                       " in function " + function_name);
            function_behavior->add_function_mem(node_id);
            function_behavior->add_parm_decl_copied(node_id);
            AppM->add_written_object(node_id);
         }
         break;
      }
      case tree_list_K:
      {
         auto tl = GetPointerS<const tree_list>(tn);
         while(tl)
         {
            analyze_node(tl->valu, left_p, dynamic_address, no_dynamic_address);
            tl = tl->chan ? GetPointerS<const tree_list>(tl->chan) : nullptr;
         }
         break;
      }
      case var_decl_K:
      {
         const auto vd = GetPointerS<const var_decl>(tn);
         if(vd->extern_flag)
         {
            THROW_ERROR_CODE(C_EC, "Extern symbols not yet supported " + behavioral_helper->PrintVariable(node_id));
         }
         if(!vd->scpe ||
            vd->scpe->get_kind() == translation_unit_decl_K) // memory has to be allocated in case of global variables
         {
            function_behavior->add_function_mem(node_id);
            bool address_externally_used = false;
            if((!vd->static_flag || tree_helper::IsVolatile(tn)))
            {
               if(parameters->isOption(OPT_expose_globals) && parameters->getOption<bool>(OPT_expose_globals))
               {
                  address_externally_used = true;
               }
               function_behavior->set_has_globals(true);
               if(address_externally_used)
               {
                  INDENT_OUT_MEX(OUTPUT_LEVEL_VERBOSE, output_level,
                                 "---Global variable externally accessible found: " +
                                     behavioral_helper->PrintVariable(node_id));
               }
            }
            function_behavior->add_state_variable(node_id);
            if((dynamic_address && !no_dynamic_address && !vd->addr_not_taken) || address_externally_used ||
               vd->addr_taken)
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
            if(vd->init && (vd->init->get_kind() != string_cst_K))
            {
               analyze_node(vd->init, false, false, false);
            }
         }
         else
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Local variable");
            THROW_ASSERT(vd->scpe->get_kind() != translation_unit_decl_K,
                         "translation_unit_decl not expected a translation unit in this point @" + STR(node_id));
            if(vd->static_flag ||                      // memory has to be allocated in case of local static variables
               tree_helper::IsVolatile(tn) ||          // volatile vars have to be allocated
               vd->type->get_kind() == array_type_K || // arrays have to be allocated
               /*
                * TODO: initially complexes were like structs and so they were allocated
                * this should not happen anymore but removing the next line
                * caused failures in the allocation due to missing complex
                * components in the technology library.
                * This issue should be further investigated.
                */
               vd->type->get_kind() == complex_type_K ||
               vd->type->get_kind() == record_type_K || // records have to be allocated
               vd->type->get_kind() == union_type_K)
            {
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---It has to be allocated");
               bool address_externally_used = false;
               if(tree_helper::IsVolatile(tn))
               {
                  if(parameters->isOption(OPT_expose_globals) && parameters->getOption<bool>(OPT_expose_globals))
                  {
                     address_externally_used = true;
                  }
                  function_behavior->set_has_globals(true);
                  function_behavior->add_state_variable(node_id);
                  if(address_externally_used)
                  {
                     INDENT_OUT_MEX(OUTPUT_LEVEL_VERBOSE, output_level,
                                    "---Global variable externally accessible found: " +
                                        behavioral_helper->PrintVariable(node_id));
                  }
               }
               else if(vd->static_flag)
               {
                  function_behavior->add_state_variable(node_id);
               }
               function_behavior->add_function_mem(node_id);
               if((dynamic_address && !no_dynamic_address && !vd->addr_not_taken) || address_externally_used ||
                  vd->addr_taken)
               {
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                 "---Variable for which the dynamic address is used-11: " +
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
               if(vd->init && vd->init->get_kind() != string_cst_K)
               {
                  analyze_node(vd->init, left_p, false, false);
               }
            }
         }
         break;
      }
      case constructor_K:
      {
         const auto con = GetPointerS<const constructor>(tn);
         for(const auto& el : con->list_of_idx_valu)
         {
            if(el.first)
            {
               analyze_node(el.first, left_p, dynamic_address, no_dynamic_address);
            }
            if(el.second)
            {
               analyze_node(el.second, left_p, dynamic_address, no_dynamic_address);
            }
         }
         break;
      }
      case gimple_goto_K:
      {
         const auto ge = GetPointerS<const gimple_goto>(tn);
         analyze_node(ge->op, left_p, dynamic_address, no_dynamic_address);
         break;
      }
      case gimple_nop_K:
      case field_decl_K:
      case gimple_pragma_K:
      case CASE_PRAGMA_NODES:
      {
         break;
      }
      case target_mem_ref_K:
      {
         const auto tmr = GetPointerS<const target_mem_ref>(tn);
         if(tmr->symbol)
         {
            analyze_node(tmr->symbol, left_p, false, true);
         }
         if(tmr->base)
         {
            analyze_node(tmr->base, left_p, false, true);
         }
         if(tmr->idx)
         {
            analyze_node(tmr->idx, left_p, false, false);
         }
         break;
      }
      case target_mem_ref461_K:
      {
         const auto tmr = GetPointerS<const target_mem_ref461>(tn);
         if(tmr->base)
         {
            const auto operand = tmr->base;
            if(operand->get_kind() == addr_expr_K)
            {
               /// skip the &
               analyze_node(GetPointerS<const addr_expr>(operand)->op, left_p, false, true);
            }
            else
            {
               analyze_node(tmr->base, left_p, false, true);
            }
         }
         if(tmr->idx)
         {
            analyze_node(tmr->idx, left_p, false, false);
         }
         if(tmr->idx2)
         {
            analyze_node(tmr->idx2, left_p, false, false);
         }

         /// check for unaligned accesses
         // if(tmr->base)
         // {
         //    const auto type_base = tree_helper::CGetType(tmr->base);
         //    const auto t_base_ptr = GetPointer<const type_node>(type_base);
         //    if(t_base_ptr->algn != 8)
         //    {
         //    }
         // }
         break;
      }
      case function_decl_K:
      case template_decl_K:
      {
         break;
      }
      case gimple_asm_K:
      {
         const auto ga = GetPointerS<const gimple_asm>(tn);
         if(ga->in)
         {
            analyze_node(ga->in, false, false, false);
         }
         if(ga->out)
         {
            analyze_node(ga->out, true, false, false);
         }
         break;
      }
      case binfo_K:
      case block_K:
      case case_label_expr_K:
      case const_decl_K:
      case CASE_CPP_NODES:
      case CASE_FAKE_NODES:
      case gimple_for_K:
      case gimple_bind_K:
      case gimple_predict_K:
      case gimple_resx_K:
      case gimple_while_K:
      case identifier_node_K:
      case namespace_decl_K:
      case statement_list_K:
      case translation_unit_decl_K:
      case error_mark_K:
      case using_decl_K:
      case tree_vec_K:
      case type_decl_K:
      case CASE_TYPE_NODES:
      case target_expr_K:
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
