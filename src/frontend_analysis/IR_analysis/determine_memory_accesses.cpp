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
#include "tree_reindex.hpp"

determine_memory_accesses::determine_memory_accesses(const ParameterConstRef _parameters, const application_managerRef _AppM, unsigned int _function_id, const DesignFlowManagerConstRef _design_flow_manager)
    : FunctionFrontendFlowStep(_AppM, _function_id, DETERMINE_MEMORY_ACCESSES, _design_flow_manager, _parameters), behavioral_helper(function_behavior->CGetBehavioralHelper()), TM(_AppM->get_tree_manager()), already_executed(false)
{
   debug_level = parameters->get_class_debug_level(GET_CLASS(*this), DEBUG_LEVEL_NONE);
}

determine_memory_accesses::~determine_memory_accesses() = default;

const CustomUnorderedSet<std::pair<FrontendFlowStepType, FrontendFlowStep::FunctionRelationship>> determine_memory_accesses::ComputeFrontendRelationships(const DesignFlowStep::RelationshipType relationship_type) const
{
   CustomUnorderedSet<std::pair<FrontendFlowStepType, FunctionRelationship>> relationships;
   switch(relationship_type)
   {
      case(DEPENDENCE_RELATIONSHIP):
      {
         relationships.insert(std::make_pair(DETERMINE_MEMORY_ACCESSES, CALLED_FUNCTIONS));
#if HAVE_BAMBU_BUILT
         if(parameters->isOption(OPT_writer_language))
         {
            relationships.insert(std::make_pair(HDL_VAR_DECL_FIX, SAME_FUNCTION));
         }
         else
#endif
         {
            relationships.insert(std::make_pair(VAR_DECL_FIX, SAME_FUNCTION));
         }
         relationships.insert(std::make_pair(CALL_GRAPH_BUILTIN_CALL, SAME_FUNCTION));
         relationships.insert(std::make_pair(PARM_DECL_TAKEN_ADDRESS, SAME_FUNCTION));
         relationships.insert(std::make_pair(PARM2SSA, WHOLE_APPLICATION));
         break;
      }
      case(INVALIDATION_RELATIONSHIP):
      {
         break;
      }
      case(PRECEDENCE_RELATIONSHIP):
      {
         relationships.insert(std::make_pair(FIX_STRUCTS_PASSED_BY_VALUE, SAME_FUNCTION));
         relationships.insert(std::make_pair(FUNCTION_CALL_TYPE_CLEANUP, SAME_FUNCTION));
         relationships.insert(std::make_pair(UN_COMPARISON_LOWERING, SAME_FUNCTION));
         relationships.insert(std::make_pair(IR_LOWERING, SAME_FUNCTION));
         relationships.insert(std::make_pair(REBUILD_INITIALIZATION, SAME_FUNCTION));
         relationships.insert(std::make_pair(REBUILD_INITIALIZATION2, SAME_FUNCTION));
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
   tree_nodeRef tn = TM->get_tree_node_const(function_id);
   auto* fd = GetPointer<function_decl>(tn);
   if(!fd || !fd->body)
   {
      PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Node is not a function or it hasn't a body");
      return DesignFlowStep_Status::UNCHANGED;
   }

   /// analyze formal parameters
   std::vector<tree_nodeRef>::const_iterator formal_it_end = fd->list_of_args.end();
   for(std::vector<tree_nodeRef>::const_iterator formal_it = fd->list_of_args.begin(); formal_it != formal_it_end; ++formal_it)
      analyze_node(GET_INDEX_NODE(*formal_it), false, false, false);

   auto* sl = GetPointer<statement_list>(GET_NODE(fd->body));
   THROW_ASSERT(sl, "Body is not a statement_list");
   std::map<unsigned int, blocRef>::iterator it_bb, it_bb_end = sl->list_of_bloc.end();
   for(it_bb = sl->list_of_bloc.begin(); it_bb != it_bb_end; ++it_bb)
   {
      if(it_bb->second->number == BB_ENTRY || it_bb->second->number == BB_EXIT)
         continue;
      for(const auto& phi : it_bb->second->CGetPhiList())
      {
         analyze_node(phi->index, false, false, false);
      }
      for(const auto& stmt : it_bb->second->CGetStmtList())
      {
         analyze_node(stmt->index, false, false, false);
      }
   }

   if(debug_level >= DEBUG_LEVEL_PEDANTIC && parameters->getOption<bool>(OPT_print_dot))
      AppM->CGetCallGraphManager()->CGetCallGraph()->WriteDot("call_graph_memory_analysis.dot");
   already_executed = true;
   /// mem clean up
   already_visited_ae.clear();
   already_visited.clear();
   return DesignFlowStep_Status::SUCCESS;
}

void determine_memory_accesses::analyze_node(unsigned int node_id, bool left_p, bool dynamic_address, bool no_dynamic_address)
{
   const tree_nodeRef tn = TM->get_tree_node_const(node_id);
   auto tnKind = tn->get_kind();
   if(tnKind != addr_expr_K && tnKind != var_decl_K)
   {
      if(already_visited.find(node_id) != already_visited.end())
         return;
      else
         already_visited.insert(node_id);
   }
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Analyzing node " + tn->ToString() + " - Dynamic address: " + (dynamic_address ? " true" : "false") + " - No dynamic address: " + (no_dynamic_address ? "true" : "false"));
   std::string function_name = behavioral_helper->get_function_name();

   if(GetPointer<gimple_node>(tn))
   {
      auto* gn = GetPointer<gimple_node>(tn);
      if(gn->use_set)
      {
         const std::vector<tree_nodeRef>::const_iterator usv_it_end = gn->use_set->variables.end();
         for(std::vector<tree_nodeRef>::const_iterator usv_it = gn->use_set->variables.begin(); usv_it != usv_it_end; ++usv_it)
         {
            analyze_node(GET_INDEX_NODE(*usv_it), false, true, false);
         }
      }
   }

   switch(tnKind)
   {
      case gimple_assign_K:
      {
         auto* gm = GetPointer<gimple_assign>(tn);
         if(!gm->init_assignment)
         {
            // std::cerr << "gimple assign " << node_id << " " << tn << std::endl;
            analyze_node(GET_INDEX_NODE(gm->op0), true, false, false);
            analyze_node(GET_INDEX_NODE(gm->op1), false, false, gm->temporary_address);

            /// check for implicit memcpy calls
            tree_nodeRef op0 = GET_NODE(gm->op0);
            tree_nodeRef op1 = GET_NODE(gm->op1);
            unsigned int op0_type_index, op1_type_index;
            tree_nodeRef op0_type = tree_helper::get_type_node(op0, op0_type_index);
            tree_nodeRef op1_type = tree_helper::get_type_node(op1, op1_type_index);

            bool is_a_vector_bitfield = false;
            if(op1->get_kind() == bit_field_ref_K)
            {
               auto* bfr = GetPointer<bit_field_ref>(op1);
               if(tree_helper::is_a_vector(TM, GET_INDEX_NODE(bfr->op0)))
                  is_a_vector_bitfield = true;
            }

            bool load_candidate = (op1->get_kind() == bit_field_ref_K && !is_a_vector_bitfield) || op1->get_kind() == component_ref_K || op1->get_kind() == indirect_ref_K || op1->get_kind() == misaligned_indirect_ref_K || op1->get_kind() == mem_ref_K ||
                                  op1->get_kind() == array_ref_K || op1->get_kind() == target_mem_ref_K || op1->get_kind() == target_mem_ref461_K;
            if(op1->get_kind() == realpart_expr_K || op1->get_kind() == imagpart_expr_K)
            {
               enum kind code1 = GET_NODE(GetPointer<unary_expr>(op1)->op)->get_kind();
               if((code1 == bit_field_ref_K && !is_a_vector_bitfield) || code1 == component_ref_K || code1 == indirect_ref_K || code1 == bit_field_ref_K || code1 == misaligned_indirect_ref_K || code1 == mem_ref_K || code1 == array_ref_K ||
                  code1 == target_mem_ref_K || code1 == target_mem_ref461_K)
                  load_candidate = true;
               if(code1 == var_decl_K && function_behavior->is_variable_mem(GET_INDEX_NODE(GetPointer<unary_expr>(op1)->op)))
                  load_candidate = true;
            }
            bool store_candidate = op0->get_kind() == bit_field_ref_K || op0->get_kind() == component_ref_K || op0->get_kind() == indirect_ref_K || op0->get_kind() == misaligned_indirect_ref_K || op0->get_kind() == mem_ref_K ||
                                   op0->get_kind() == array_ref_K || op0->get_kind() == target_mem_ref_K || op0->get_kind() == target_mem_ref461_K;
            if(op0->get_kind() == realpart_expr_K || op0->get_kind() == imagpart_expr_K)
            {
               enum kind code0 = GET_NODE(GetPointer<unary_expr>(op0)->op)->get_kind();
               if((code0 == bit_field_ref_K) || code0 == component_ref_K || code0 == indirect_ref_K || code0 == bit_field_ref_K || code0 == misaligned_indirect_ref_K || code0 == mem_ref_K || code0 == array_ref_K || code0 == target_mem_ref_K ||
                  code0 == target_mem_ref461_K)
                  store_candidate = true;
               if(code0 == var_decl_K && function_behavior->is_variable_mem(GET_INDEX_NODE(GetPointer<unary_expr>(op0)->op)))
                  store_candidate = true;
            }
            if(!gm->clobber && !gm->init_assignment && op0_type && op1_type &&
               ((op0_type->get_kind() == record_type_K && op1_type->get_kind() == record_type_K && op1->get_kind() != view_convert_expr_K) ||
                (op0_type->get_kind() == union_type_K && op1_type->get_kind() == union_type_K && op1->get_kind() != view_convert_expr_K) || (op0_type->get_kind() == array_type_K) ||
                (function_behavior->is_variable_mem(GET_INDEX_NODE(gm->op0)) && function_behavior->is_variable_mem(GET_INDEX_NODE(gm->op1))) || (function_behavior->is_variable_mem(GET_INDEX_NODE(gm->op0)) && load_candidate) ||
                (store_candidate && function_behavior->is_variable_mem(GET_INDEX_NODE(gm->op1)))))
            {
               if(op0->get_kind() == mem_ref_K)
               {
                  auto* mr = GetPointer<mem_ref>(op0);
                  analyze_node(GET_INDEX_NODE(mr->op0), true, true, false);
               }
               else if(op0->get_kind() == target_mem_ref461_K)
               {
                  auto* tmr = GetPointer<target_mem_ref461>(op0);
                  if(tmr->base)
                     analyze_node(GET_INDEX_NODE(tmr->base), true, true, false);
                  else
                     analyze_node(GET_INDEX_NODE(gm->op0), true, true, false);
               }
               else
                  analyze_node(GET_INDEX_NODE(gm->op0), true, true, false);

               if(op1->get_kind() == mem_ref_K)
               {
                  auto* mr = GetPointer<mem_ref>(op1);
                  analyze_node(GET_INDEX_NODE(mr->op0), true, true, false);
               }
               else if(op1->get_kind() == target_mem_ref461_K)
               {
                  auto* tmr = GetPointer<target_mem_ref461>(op1);
                  if(tmr->base)
                     analyze_node(GET_INDEX_NODE(tmr->base), true, true, false);
                  else
                     analyze_node(GET_INDEX_NODE(gm->op1), true, true, false);
               }
               else
                  analyze_node(GET_INDEX_NODE(gm->op1), false, true, false);
               if(gm->predicate)
                  analyze_node(GET_INDEX_NODE(gm->predicate), false, true, false);

               if(op1->get_kind() == constructor_K && GetPointer<constructor>(op1) && GetPointer<constructor>(op1)->list_of_idx_valu.size() == 0)
               {
                  /// manage temporary addresses
                  unsigned int ref_var = tree_helper::get_base_index(TM, GET_INDEX_NODE(gm->op0));
                  if(ref_var)
                     analyze_node(ref_var, true, true, false);
               }
               else
               {
                  /// manage temporary addresses
                  unsigned int ref_var = tree_helper::get_base_index(TM, GET_INDEX_NODE(gm->op0));
                  if(ref_var)
                     analyze_node(ref_var, true, true, false);
                  ref_var = tree_helper::get_base_index(TM, GET_INDEX_NODE(gm->op1));
                  if(ref_var)
                     analyze_node(ref_var, false, true, false);
               }
            }
         }
         break;
      }
      case CASE_UNARY_EXPRESSION:
      {
         auto* ue = GetPointer<unary_expr>(tn);
         if(GetPointer<addr_expr>(tn))
         {
            if(GetPointer<var_decl>(GET_NODE(ue->op)))
            {
               bool address_externally_used = false;
               auto* vd = GetPointer<var_decl>(GET_NODE(ue->op));
               function_behavior->add_function_mem(GET_INDEX_NODE(ue->op));
               if((((!vd->scpe || GET_NODE(vd->scpe)->get_kind() == translation_unit_decl_K) && !vd->static_flag) || tree_helper::is_volatile(TM, node_id)))
               {
                  if(!parameters->isOption(OPT_do_not_expose_globals) || !parameters->getOption<bool>(OPT_do_not_expose_globals))
                     address_externally_used = true;
                  function_behavior->set_has_globals(true);
                  function_behavior->add_state_variable(GET_INDEX_NODE(ue->op));
                  if(address_externally_used)
                     INDENT_OUT_MEX(OUTPUT_LEVEL_VERBOSE, output_level, "---Global variable externally accessible found: " + behavioral_helper->PrintVariable(GET_INDEX_NODE(ue->op)));
               }
               else
                  function_behavior->add_state_variable(GET_INDEX_NODE(ue->op));

               if((!no_dynamic_address || address_externally_used))
               {
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Variable for which the dynamic address is used-1: " + behavioral_helper->PrintVariable(GET_INDEX_NODE(ue->op)));
                  function_behavior->add_dynamic_address(GET_INDEX_NODE(ue->op));
                  if(!vd->readonly_flag)
                     AppM->add_written_object(GET_INDEX_NODE(ue->op));
               }
               if(left_p && !vd->readonly_flag)
                  AppM->add_written_object(GET_INDEX_NODE(ue->op));
               if(already_visited_ae.find(node_id) == already_visited_ae.end())
               {
                  already_visited_ae.insert(node_id);

                  if((GetPointer<var_decl>(GET_NODE(ue->op)))->init && !GetPointer<string_cst>(GET_NODE((GetPointer<var_decl>(GET_NODE(ue->op)))->init)))
                     analyze_node(GET_INDEX_NODE((GetPointer<var_decl>(GET_NODE(ue->op)))->init), left_p, false, false);
               }
            }
            else if(GetPointer<parm_decl>(GET_NODE(ue->op)))
            {
               function_behavior->add_function_mem(GET_INDEX_NODE(ue->op));
               if(!no_dynamic_address)
               {
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Variable for which the dynamic address is used-2: " + behavioral_helper->PrintVariable(GET_INDEX_NODE(ue->op)));
                  function_behavior->add_dynamic_address(GET_INDEX_NODE(ue->op));
               }
               /// an address of a parm decl may be used in writing so it has to be copied
               PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "  Analyzing node: formal parameter copied " + STR(GET_INDEX_NODE(ue->op)));
               function_behavior->add_parm_decl_copied(GET_INDEX_NODE(ue->op));
               AppM->add_written_object(GET_INDEX_NODE(ue->op));
            }
            else if(GetPointer<string_cst>(GET_NODE(ue->op)))
            {
               function_behavior->add_function_mem(GET_INDEX_NODE(ue->op));
               if(!no_dynamic_address)
               {
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Variable for which the dynamic address is used-3: " + behavioral_helper->PrintVariable(GET_INDEX_NODE(ue->op)));
                  function_behavior->add_dynamic_address(GET_INDEX_NODE(ue->op));
                  AppM->add_written_object(GET_INDEX_NODE(ue->op));
               }
            }
            else if(GetPointer<result_decl>(GET_NODE(ue->op)))
            {
               function_behavior->add_function_mem(GET_INDEX_NODE(ue->op));
               if(!no_dynamic_address)
               {
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Variable for which the dynamic address is used-4: " + behavioral_helper->PrintVariable(GET_INDEX_NODE(ue->op)));
                  function_behavior->add_dynamic_address(GET_INDEX_NODE(ue->op));
                  AppM->add_written_object(GET_INDEX_NODE(ue->op));
               }
               INDENT_OUT_MEX(OUTPUT_LEVEL_VERBOSE, output_level, "---result_decl variable added to memory: " + behavioral_helper->PrintVariable(GET_INDEX_NODE(ue->op)));
            }
            else if(GetPointer<component_ref>(GET_NODE(ue->op)) || GetPointer<realpart_expr>(GET_NODE(ue->op)) || GetPointer<imagpart_expr>(GET_NODE(ue->op)) || GetPointer<array_ref>(GET_NODE(ue->op)))
               analyze_node(GET_INDEX_NODE(ue->op), true, !no_dynamic_address, no_dynamic_address);
            else if(GetPointer<function_decl>(GET_NODE(ue->op)))
            {
               analyze_node(GET_INDEX_NODE(ue->op), false, !no_dynamic_address, no_dynamic_address);
            }
            else if(GetPointer<mem_ref>(GET_NODE(ue->op)))
            {
               auto* mr = GetPointer<mem_ref>(GET_NODE(ue->op));
               analyze_node(GET_INDEX_NODE(mr->op0), left_p, !no_dynamic_address, no_dynamic_address);
            }
            else if(GetPointer<target_mem_ref461>(GET_NODE(ue->op)))
            {
               auto* tmr = GetPointer<target_mem_ref461>(GET_NODE(ue->op));
               if(tmr->base)
                  analyze_node(GET_INDEX_NODE(tmr->base), left_p, !no_dynamic_address, no_dynamic_address);
               else
                  analyze_node(GET_INDEX_NODE(ue->op), left_p, !no_dynamic_address, no_dynamic_address);
            }
            else
               THROW_ERROR_CODE(NODE_NOT_YET_SUPPORTED_EC, "determine_memory_accesses addressing currently not supported: " + GET_NODE(ue->op)->get_kind_text() + " @" + STR(node_id) + " in function " + function_name);
         }
         else if(GetPointer<view_convert_expr>(tn))
         {
            auto* vc = GetPointer<view_convert_expr>(tn);
            analyze_node(GET_INDEX_NODE(vc->op), left_p, dynamic_address, no_dynamic_address);
         }
         else if(GetPointer<indirect_ref>(tn))
         {
            auto* ir = GetPointer<indirect_ref>(tn);
            if(GetPointer<integer_cst>(GET_NODE(ir->op)))
            {
               function_behavior->set_dereference_unknown_addr(true);
            }
            if(!dynamic_address)
            {
               dynamic_address = false;
               no_dynamic_address = true;
            }
            analyze_node(GET_INDEX_NODE(ir->op), left_p, dynamic_address, no_dynamic_address);
         }
         else
            analyze_node(GET_INDEX_NODE(ue->op), left_p, dynamic_address, no_dynamic_address);
         break;
      }
      case CASE_BINARY_EXPRESSION:
      {
         auto* be = GetPointer<binary_expr>(tn);
         if(GetPointer<mem_ref>(tn))
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---It is a mem ref");
            auto* mr = GetPointer<mem_ref>(tn);
            if(GetPointer<integer_cst>(GET_NODE(mr->op0)))
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
               unsigned int ref_var = tree_helper::get_base_index(TM, GET_INDEX_NODE(mr->op0));
               if(ref_var)
               {
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Referenced variable is " + TM->get_tree_node_const(ref_var)->ToString());
                  const bool is_variable_mem = [&]() {
                     if(function_behavior->is_variable_mem(ref_var))
                     {
                        INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Already classified as memory variable");
                        return true;
                     }
                     const auto vd = GetPointer<const var_decl>(TM->get_tree_node_const(ref_var));
                     if(not vd)
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
                     if(not vd->scpe or GET_NODE(vd->scpe)->get_kind() == translation_unit_decl_K)
                     {
                        INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Not local");
                        return false;
                     }
                     if(vd->scpe)
                     {
                        INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---local variable of another function");
                        return true;
                     }
                     if(tree_helper::is_volatile(TM, ref_var))
                     {
                        INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Volatile");
                        return true;
                     }
                     const auto type_kind = GET_NODE(vd->type)->get_kind();
                     /*
                      * TODO: the following if should look like this
                      *     if (type_kind == array_type_K or type_kind == record_type_K or type_kind == union_type_K)
                      * because the complex types should not be memory allocated
                      * anymore. however changing it results in failures during
                      * allocation due to missing complex components in the
                      * technology library.
                      * This issue should be further investigated.
                      */
                     if(type_kind == array_type_K or type_kind == complex_type_K or type_kind == record_type_K or type_kind == union_type_K)
                     {
                        INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Memory allocated");
                        return true;
                     }
                     return false;
                  }();
                  if(is_variable_mem)
                  {
                     INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---It is written");
                     AppM->add_written_object(ref_var);
                  }
               }
            }
         }
         analyze_node(GET_INDEX_NODE(be->op0), left_p, dynamic_address, no_dynamic_address);
         analyze_node(GET_INDEX_NODE(be->op1), left_p, dynamic_address, no_dynamic_address);
         break;
      }
      case gimple_cond_K:
      {
         auto* gc = GetPointer<gimple_cond>(tn);
         analyze_node(GET_INDEX_NODE(gc->op0), false, false, true);
         break;
      }
      case gimple_switch_K:
      {
         auto* se = GetPointer<gimple_switch>(tn);
         if(se->op0)
            analyze_node(GET_INDEX_NODE(se->op0), left_p, dynamic_address, no_dynamic_address);
         break;
      }
      case gimple_multi_way_if_K:
      {
         auto* gmwi = GetPointer<gimple_multi_way_if>(tn);
         for(auto cond : gmwi->list_of_cond)
            if(cond.first)
               analyze_node(cond.first->index, left_p, dynamic_address, no_dynamic_address);
         break;
      }
      case gimple_phi_K:
      {
         auto* gp = GetPointer<gimple_phi>(tn);
         for(const auto& def_edge : gp->CGetDefEdgesList())
         {
            analyze_node(GET_INDEX_NODE(def_edge.first), left_p, dynamic_address, no_dynamic_address);
         }
         break;
      }
      case CASE_TERNARY_EXPRESSION:
      {
         auto* te = GetPointer<ternary_expr>(tn);
         if(GetPointer<component_ref>(tn))
            left_p = true;
         if(te->op0)
            analyze_node(GET_INDEX_NODE(te->op0), left_p, dynamic_address, no_dynamic_address);
         if(te->op1)
            analyze_node(GET_INDEX_NODE(te->op1), left_p, dynamic_address, no_dynamic_address);
         if(te->op2)
            analyze_node(GET_INDEX_NODE(te->op2), left_p, dynamic_address, no_dynamic_address);
         break;
      }
      case CASE_QUATERNARY_EXPRESSION:
      {
         auto* qe = GetPointer<quaternary_expr>(tn);
         if(qe->op0)
            analyze_node(GET_INDEX_NODE(qe->op0), left_p, dynamic_address, no_dynamic_address);
         if(qe->op1)
            analyze_node(GET_INDEX_NODE(qe->op1), left_p, dynamic_address, no_dynamic_address);
         if(qe->op2)
            analyze_node(GET_INDEX_NODE(qe->op2), left_p, dynamic_address, no_dynamic_address);
         if(qe->op3)
            analyze_node(GET_INDEX_NODE(qe->op3), left_p, dynamic_address, no_dynamic_address);
         break;
      }
      case lut_expr_K:
      {
         auto* le = GetPointer<lut_expr>(tn);
         analyze_node(GET_INDEX_NODE(le->op0), left_p, dynamic_address, no_dynamic_address);
         analyze_node(GET_INDEX_NODE(le->op1), left_p, dynamic_address, no_dynamic_address);
         if(le->op2)
            analyze_node(GET_INDEX_NODE(le->op2), left_p, dynamic_address, no_dynamic_address);
         if(le->op3)
            analyze_node(GET_INDEX_NODE(le->op3), left_p, dynamic_address, no_dynamic_address);
         if(le->op4)
            analyze_node(GET_INDEX_NODE(le->op4), left_p, dynamic_address, no_dynamic_address);
         if(le->op5)
            analyze_node(GET_INDEX_NODE(le->op5), left_p, dynamic_address, no_dynamic_address);
         if(le->op6)
            analyze_node(GET_INDEX_NODE(le->op6), left_p, dynamic_address, no_dynamic_address);
         if(le->op7)
            analyze_node(GET_INDEX_NODE(le->op7), left_p, dynamic_address, no_dynamic_address);
         if(le->op8)
            analyze_node(GET_INDEX_NODE(le->op8), left_p, dynamic_address, no_dynamic_address);
         break;
      }
      case gimple_return_K:
      {
         auto* re = GetPointer<gimple_return>(tn);
         if(re->op)
         {
            tree_nodeRef res = GET_NODE(re->op);
            tree_nodeRef res_type = tree_helper::get_type_node(res);
            if(res_type->get_kind() == record_type_K || // records have to be allocated
               res_type->get_kind() == union_type_K     // unions have to be allocated
            )
            {
               THROW_ERROR_CODE(NODE_NOT_YET_SUPPORTED_EC, "structs or unions returned by copy are not yet supported: @" + STR(node_id) + " in function " + function_name);
               function_behavior->add_function_mem(node_id);
               function_behavior->add_parm_decl_copied(node_id);
               AppM->add_written_object(node_id);
            }
            analyze_node(GET_INDEX_NODE(re->op), left_p, dynamic_address, no_dynamic_address);
         }
         break;
      }
      case call_expr_K:
      case aggr_init_expr_K:
      {
         auto* ce = GetPointer<call_expr>(tn);
         std::vector<tree_nodeRef>& args = ce->args;
         auto* ae = GetPointer<addr_expr>(GET_NODE(ce->fn));

         // The first parameter of a call_expr can be a ssa_name in
         // case of function pointer usage.  When it happens skip the
         // following analysis.
         if(!ae)
            break;

         if(AppM->GetFunctionBehavior(GET_INDEX_NODE(ae->op))->get_unaligned_accesses())
         {
            function_behavior->set_unaligned_accesses(true);
         }
         auto* fd = GetPointer<function_decl>(GET_NODE(ae->op));
         bool is_var_args_p = GetPointer<function_type>(GET_NODE(fd->type))->varargs_flag;
         THROW_ASSERT(fd, "expected a function_decl");
         std::vector<tree_nodeRef>::iterator arg, arg_end = args.end();
         bool has_pointers_as_actual_parameters = false;
         for(arg = args.begin(); arg != arg_end; ++arg)
         {
            analyze_node(GET_INDEX_NODE(*arg), left_p, dynamic_address, no_dynamic_address);
            has_pointers_as_actual_parameters |= tree_helper::is_a_pointer(TM, GET_INDEX_NODE(*arg));
         }
         if(!fd->undefined_flag)
         {
            std::vector<tree_nodeRef>::const_iterator formal_it = fd->list_of_args.begin();
            std::vector<tree_nodeRef>::const_iterator formal_it_end = fd->list_of_args.end();
            if(!(is_var_args_p || fd->list_of_args.size() == args.size()))
               THROW_ERROR("In function " + function_name + " a different number of formal and actual parameters is found when function " + tree_helper::print_function_name(TM, fd) + " is called: " + STR(fd->list_of_args.size()) + " - " +
                           STR(args.size()) + "\n Check the C source code since an actual parameter is passed to a function that does have the associated formal parameter");
            for(arg = args.begin(); arg != arg_end && formal_it != formal_it_end; ++arg, ++formal_it)
            {
               unsigned int actual_par_index = GET_INDEX_NODE(*arg);
               unsigned int formal_par_index = GET_INDEX_NODE(*formal_it);
               unsigned int calledFundID = GET_INDEX_NODE(ae->op);
               if(tree_helper::is_a_pointer(TM, actual_par_index) && tree_helper::get_base_index(TM, actual_par_index))
                  actual_par_index = tree_helper::get_base_index(TM, actual_par_index);
               const FunctionBehaviorRef FBcalled = AppM->GetFunctionBehavior(calledFundID);
               /// check if the actual parameter has been allocated in memory
               if(function_behavior->is_variable_mem(actual_par_index) && AppM->isParmUsed(formal_par_index))
               {
                  auto formal_ssa_index = AppM->getSSAFromParm(formal_par_index);
                  auto formal_ssa_node = TM->get_tree_node_const(formal_ssa_index);
                  auto formal_ssa = GetPointer<ssa_name>(formal_ssa_node);
                  auto is_singleton = formal_ssa->use_set->is_a_singleton() && actual_par_index == formal_ssa->use_set->variables.front()->index;
                  if(!is_singleton)
                  {
                     INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Variable for which the dynamic address is used-5: " + behavioral_helper->PrintVariable(actual_par_index));
                     function_behavior->add_dynamic_address(actual_par_index);
                     AppM->add_written_object(actual_par_index);
                     /// if the formal parameter has not been allocated in memory then it has to be initialized
                     if(!FBcalled->is_variable_mem(formal_par_index) && GET_INDEX_NODE(*arg) == actual_par_index)
                     {
                        PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "  Analyzing node: actual parameter loaded " + STR(actual_par_index));
                        function_behavior->add_parm_decl_loaded(actual_par_index);
                     }
                  }
               }
               else if(!AppM->isParmUsed(formal_par_index))
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Parameter is not used in the function body.");

               /// check if the formal parameter has been allocated in memory.
               if(FBcalled->is_variable_mem(formal_par_index))
               {
                  /// If the actual has not been allocated in memory then the formal parameter storage has to be initialized with the actual value with a MEMSTORE_STD
                  tree_nodeRef actual_par = GET_NODE(*arg);
                  switch(actual_par->get_kind())
                  {
                     case ssa_name_K:
                     {
                        if(!function_behavior->is_variable_mem(actual_par_index))
                        {
                           PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "  Analyzing node: formal parameter stored " + STR(formal_par_index));
                           FBcalled->add_parm_decl_stored(formal_par_index);
                           FBcalled->add_dynamic_address(formal_par_index);
                           AppM->add_written_object(formal_par_index);
                        }
                        break;
                     }
                     case real_cst_K:
                     case string_cst_K:
                     case integer_cst_K:
                     case addr_expr_K:
                     {
                        PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "  Analyzing node: formal parameter stored " + STR(formal_par_index));
                        FBcalled->add_parm_decl_stored(formal_par_index);
                        FBcalled->add_dynamic_address(formal_par_index);
                        AppM->add_written_object(formal_par_index);
                        if(actual_par->get_kind() == string_cst_K)
                        {
                           INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Variable for which the dynamic address is used-6: " + behavioral_helper->PrintVariable(actual_par_index));
                           function_behavior->add_dynamic_address(actual_par_index);
                           AppM->add_written_object(actual_par_index);
                        }
                        break;
                     }
                     case misaligned_indirect_ref_K:
                     case indirect_ref_K:
                     case array_ref_K:
                     case component_ref_K:
                     {
                        PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "  Analyzing node: formal parameter copied " + STR(formal_par_index));
                        FBcalled->add_parm_decl_copied(formal_par_index);
                        FBcalled->add_dynamic_address(formal_par_index);
                        AppM->add_written_object(formal_par_index);
                        tree_nodeRef arg_op = GET_NODE(*arg);
                        tree_nodeRef arg_op_type = tree_helper::get_type_node(arg_op);
                        if(arg_op_type->get_kind() == record_type_K || // records have to be allocated
                           arg_op_type->get_kind() == union_type_K     // unions have to be allocated
                        )
                           analyze_node(actual_par_index, left_p, true, false);
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
                     case bit_ior_concat_expr_K:
                     case complex_cst_K:
                     case vector_cst_K:
                     case void_cst_K:
                     case array_range_ref_K:
                     case target_expr_K:
                     case error_mark_K:
                     case lut_expr_K:
                     case CASE_BINARY_EXPRESSION:
                     case CASE_CPP_NODES:
                     case CASE_DECL_NODES:
                     case CASE_FAKE_NODES:
                     case CASE_GIMPLE_NODES:
                     case CASE_PRAGMA_NODES:
                     case CASE_TYPE_NODES:
                     default:
                     {
                        THROW_ASSERT(function_behavior->is_variable_mem(actual_par_index), "actual parameter non allocated in memory: calling @" + STR(calledFundID) + " actual @" + STR(actual_par_index));
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
         auto* ce = GetPointer<gimple_call>(tn);
         std::vector<tree_nodeRef>& args = ce->args;
         auto* ae = GetPointer<addr_expr>(GET_NODE(ce->fn));
         // The first parameter of a call_expr can be a ssa_name in
         // case of function pointer usage.  When it happens skip the
         // following analysis.
         if(!ae)
            break;

         auto* fd = GetPointer<function_decl>(GET_NODE(ae->op));
         if(tree_helper::print_function_name(TM, fd) == BUILTIN_WAIT_CALL)
         {
            function_behavior->add_function_mem(node_id);
            AppM->add_written_object(node_id);
         }

         bool is_var_args_p = GetPointer<function_type>(GET_NODE(fd->type))->varargs_flag;
         THROW_ASSERT(fd, "expected a function_decl");
         std::vector<tree_nodeRef>::iterator arg, arg_end = args.end();
         bool has_pointers_as_actual_parameters = false;
         for(arg = args.begin(); arg != arg_end; ++arg)
         {
            analyze_node(GET_INDEX_NODE(*arg), left_p, dynamic_address, no_dynamic_address);
            has_pointers_as_actual_parameters |= tree_helper::is_a_pointer(TM, GET_INDEX_NODE(*arg));
         }
         if(!fd->undefined_flag)
         {
            std::vector<tree_nodeRef>::const_iterator formal_it = fd->list_of_args.begin();
            std::vector<tree_nodeRef>::const_iterator formal_it_end = fd->list_of_args.end();
            if(!(is_var_args_p || fd->list_of_args.size() == args.size()))
               THROW_ERROR("In function " + function_name + " a different number of formal and actual parameters is found when function " + tree_helper::print_function_name(TM, fd) + " is called: " + STR(fd->list_of_args.size()) + " - " +
                           STR(args.size()) + "\n Check the C source code since an actual parameter is passed to a function that does have the associated formal parameter");
            for(arg = args.begin(); arg != arg_end && formal_it != formal_it_end; ++arg, ++formal_it)
            {
               unsigned int actual_par_index = GET_INDEX_NODE(*arg);
               unsigned int formal_par_index = GET_INDEX_NODE(*formal_it);
               unsigned int calledFundID = GET_INDEX_NODE(ae->op);
               if(tree_helper::is_a_pointer(TM, actual_par_index) && tree_helper::get_base_index(TM, actual_par_index))
                  actual_par_index = tree_helper::get_base_index(TM, actual_par_index);
               const FunctionBehaviorRef FBcalled = AppM->GetFunctionBehavior(calledFundID);
               /// check if the actual parameter has been allocated in memory
               if(function_behavior->is_variable_mem(actual_par_index) && AppM->isParmUsed(formal_par_index))
               {
                  auto formal_ssa_index = AppM->getSSAFromParm(formal_par_index);
                  auto formal_ssa_node = TM->get_tree_node_const(formal_ssa_index);
                  auto formal_ssa = GetPointer<ssa_name>(formal_ssa_node);
                  auto is_singleton = formal_ssa->use_set->is_a_singleton() && actual_par_index == formal_ssa->use_set->variables.front()->index;
                  if(!is_singleton)
                  {
                     INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Variable for which the dynamic address is used-7: " + behavioral_helper->PrintVariable(actual_par_index));
                     function_behavior->add_dynamic_address(actual_par_index);
                     AppM->add_written_object(actual_par_index);
                     /// if the formal parameter has not been allocated in memory then it has to be initialized
                     if(!FBcalled->is_variable_mem(formal_par_index) && GET_INDEX_NODE(*arg) == actual_par_index)
                     {
                        INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---actual parameter loaded " + STR(actual_par_index));
                        function_behavior->add_parm_decl_loaded(actual_par_index);
                     }
                  }
               }
               else if(!AppM->isParmUsed(formal_par_index))
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Parameter is not used in the function body.");
               /// check if the formal parameter has been allocated in memory.
               if(FBcalled->is_variable_mem(formal_par_index))
               {
                  /// If the actual has not been allocated in memory then the formal parameter storage has to be initialized with the actual value with a MEMSTORE_STD
                  tree_nodeRef actual_par = GET_NODE(*arg);
                  switch(actual_par->get_kind())
                  {
                     case ssa_name_K:
                     {
                        if(!function_behavior->is_variable_mem(actual_par_index))
                        {
                           INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---formal parameter stored " + STR(formal_par_index));
                           FBcalled->add_parm_decl_stored(formal_par_index);
                           FBcalled->add_dynamic_address(formal_par_index);
                           AppM->add_written_object(formal_par_index);
                        }
                        break;
                     }
                     case real_cst_K:
                     case string_cst_K:
                     case integer_cst_K:
                     case addr_expr_K:
                     {
                        INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---formal parameter stored " + STR(formal_par_index));
                        FBcalled->add_parm_decl_stored(formal_par_index);
                        FBcalled->add_dynamic_address(formal_par_index);
                        AppM->add_written_object(formal_par_index);
                        if(actual_par->get_kind() == string_cst_K)
                        {
                           function_behavior->add_dynamic_address(actual_par_index);
                           AppM->add_written_object(actual_par_index);
                        }
                        break;
                     }
                     case misaligned_indirect_ref_K:
                     case indirect_ref_K:
                     case array_ref_K:
                     case component_ref_K:
                     {
                        INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---formal parameter copied " + STR(formal_par_index));
                        FBcalled->add_parm_decl_copied(formal_par_index);
                        FBcalled->add_dynamic_address(formal_par_index);
                        AppM->add_written_object(formal_par_index);
                        tree_nodeRef arg_op = GET_NODE(*arg);
                        tree_nodeRef arg_op_type = tree_helper::get_type_node(arg_op);
                        if(arg_op_type->get_kind() == record_type_K || // records have to be allocated
                           arg_op_type->get_kind() == union_type_K     // unions have to be allocated
                        )
                           analyze_node(actual_par_index, left_p, true, false);
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
                     case CASE_BINARY_EXPRESSION:
                     case CASE_CPP_NODES:
                     case CASE_DECL_NODES:
                     case CASE_FAKE_NODES:
                     case CASE_GIMPLE_NODES:
                     case CASE_PRAGMA_NODES:
                     case CASE_TYPE_NODES:
                     default:
                     {
                        THROW_ASSERT(function_behavior->is_variable_mem(actual_par_index), "actual parameter non allocated in memory: calling @" + STR(calledFundID) + " actual @" + STR(actual_par_index));
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
         auto* sn = GetPointer<ssa_name>(tn);
         if(sn->use_set->is_fully_resolved())
            for(auto var : sn->use_set->variables)
            {
               function_behavior->add_function_mem(GET_INDEX_NODE(var));
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
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Variable for which the dynamic address is used-8: " + behavioral_helper->PrintVariable(node_id));
            function_behavior->add_dynamic_address(node_id);
            AppM->add_written_object(node_id);
         }
         if(left_p)
            AppM->add_written_object(node_id);
         break;
      }
      case parm_decl_K:
      {
         auto* pd = GetPointer<parm_decl>(tn);
         if(GET_NODE(pd->type)->get_kind() == record_type_K || // records have to be allocated
            GET_NODE(pd->type)->get_kind() == union_type_K     // unions have to be allocated
         )
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Variable for which the dynamic address is used-9: " + behavioral_helper->PrintVariable(node_id));
            function_behavior->add_function_mem(node_id);
            function_behavior->add_dynamic_address(node_id);
            AppM->add_written_object(node_id);
            if(left_p)
            {
               PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "  Analyzing node: formal parameter copied " + STR(node_id));
               function_behavior->add_parm_decl_copied(node_id);
            }
         }
         break;
      }
      case result_decl_K:
      {
         auto* rd = GetPointer<result_decl>(tn);
         if(GET_NODE(rd->type)->get_kind() == record_type_K || // records have to be allocated
            GET_NODE(rd->type)->get_kind() == union_type_K     // unions have to be allocated
         )
         {
            THROW_ERROR_CODE(C_EC, "structs or unions returned by copy are not yet supported: @" + STR(node_id) + " in function " + function_name);
            function_behavior->add_function_mem(node_id);
            function_behavior->add_parm_decl_copied(node_id);
            AppM->add_written_object(node_id);
         }
         break;
      }
      case tree_list_K:
      {
         auto* tl = GetPointer<tree_list>(tn);
         while(tl)
         {
            analyze_node(GET_INDEX_NODE(tl->valu), left_p, dynamic_address, no_dynamic_address);
            tl = tl->chan ? GetPointer<tree_list>(GET_NODE(tl->chan)) : nullptr;
         }
         break;
      }
      case var_decl_K:
      {
         auto* vd = GetPointer<var_decl>(tn);
         if(vd->extern_flag)
            THROW_ERROR_CODE(C_EC, "Extern symbols not yet supported " + behavioral_helper->PrintVariable(node_id));
         if(!vd->scpe || GET_NODE(vd->scpe)->get_kind() == translation_unit_decl_K) // memory has to be allocated in case of global variables
         {
            function_behavior->add_function_mem(node_id);
            bool address_externally_used = false;
            if((!vd->static_flag || tree_helper::is_volatile(TM, node_id)))
            {
               if(!parameters->isOption(OPT_do_not_expose_globals) || !parameters->getOption<bool>(OPT_do_not_expose_globals))
                  address_externally_used = true;
               function_behavior->set_has_globals(true);
               if(address_externally_used)
                  INDENT_OUT_MEX(OUTPUT_LEVEL_VERBOSE, output_level, "---Global variable externally accessible found: " + behavioral_helper->PrintVariable(node_id));
            }
            function_behavior->add_state_variable(node_id);
            if((dynamic_address && !no_dynamic_address && !vd->addr_not_taken) || address_externally_used || vd->addr_taken)
            {
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Variable for which the dynamic address is used-10: " + behavioral_helper->PrintVariable(node_id));
               function_behavior->add_dynamic_address(node_id);
               if(!vd->readonly_flag)
                  AppM->add_written_object(node_id);
            }
            if(left_p && !vd->readonly_flag)
               AppM->add_written_object(node_id);
            if(vd->init && !GetPointer<string_cst>(GET_NODE(vd->init)))
               analyze_node(GET_INDEX_NODE(vd->init), false, false, false);
         }
         else
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Local variable");
            THROW_ASSERT(GET_NODE(vd->scpe)->get_kind() != translation_unit_decl_K, "translation_unit_decl not expected a translation unit in this point @" + STR(node_id));
            if(vd->static_flag ||                                // memory has to be allocated in case of local static variables
               tree_helper::is_volatile(TM, node_id) ||          // volatile vars have to be allocated
               GET_NODE(vd->type)->get_kind() == array_type_K || // arrays have to be allocated
               /*
                * TODO: initially complexes were like structs and so they were allocated
                * this should not happen anymore but removing the next line
                * caused failures in the allocation due to missing complex
                * components in the technology library.
                * This issue should be further investigated.
                */
               GET_NODE(vd->type)->get_kind() == complex_type_K || GET_NODE(vd->type)->get_kind() == record_type_K || // records have to be allocated
               GET_NODE(vd->type)->get_kind() == union_type_K)
            {
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---It has to be allocated");
               bool address_externally_used = false;
               if(tree_helper::is_volatile(TM, node_id))
               {
                  if((!parameters->isOption(OPT_do_not_expose_globals) || !parameters->getOption<bool>(OPT_do_not_expose_globals)))
                     address_externally_used = true;
                  function_behavior->set_has_globals(true);
                  function_behavior->add_state_variable(node_id);
                  if(address_externally_used)
                     INDENT_OUT_MEX(OUTPUT_LEVEL_VERBOSE, output_level, "---Global variable externally accessible found: " + behavioral_helper->PrintVariable(node_id));
               }
               else if(vd->static_flag)
                  function_behavior->add_state_variable(node_id);
               function_behavior->add_function_mem(node_id);
               if((dynamic_address && !no_dynamic_address && !vd->addr_not_taken) || address_externally_used || vd->addr_taken)
               {
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Variable for which the dynamic address is used-11: " + behavioral_helper->PrintVariable(node_id));
                  function_behavior->add_dynamic_address(node_id);
                  if(!vd->readonly_flag)
                     AppM->add_written_object(node_id);
               }
               if(left_p && !vd->readonly_flag)
                  AppM->add_written_object(node_id);
            }
            else
            {
               // nothing have to be allocated for the variable
               // maybe something has to be allocated for its initialization
               if(vd->init && !GetPointer<string_cst>(GET_NODE(vd->init)))
                  analyze_node(GET_INDEX_NODE(vd->init), left_p, false, false);
            }
         }
         break;
      }
      case constructor_K:
      {
         auto* con = GetPointer<constructor>(tn);
         std::vector<std::pair<tree_nodeRef, tree_nodeRef>>& list_of_idx_valu = con->list_of_idx_valu;
         for(auto el = list_of_idx_valu.begin(); el != list_of_idx_valu.end(); ++el)
         {
            if(el->first)
               analyze_node(GET_INDEX_NODE(el->first), left_p, dynamic_address, no_dynamic_address);
            if(el->second)
               analyze_node(GET_INDEX_NODE(el->second), left_p, dynamic_address, no_dynamic_address);
         }
         break;
      }
      case gimple_goto_K:
      {
         auto* ge = GetPointer<gimple_goto>(tn);
         analyze_node(GET_INDEX_NODE(ge->op), left_p, dynamic_address, no_dynamic_address);
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
         auto* tmr = GetPointer<target_mem_ref>(tn);
         if(tmr->symbol)
            analyze_node(GET_INDEX_NODE(tmr->symbol), left_p, false, true);
         if(tmr->base)
            analyze_node(GET_INDEX_NODE(tmr->base), left_p, false, true);
         if(tmr->idx)
            analyze_node(GET_INDEX_NODE(tmr->idx), left_p, false, false);
         break;
      }
      case target_mem_ref461_K:
      {
         auto* tmr = GetPointer<target_mem_ref461>(tn);
         if(tmr->base)
         {
            tree_nodeRef operand = GET_NODE(tmr->base);
            if(operand->get_kind() == addr_expr_K)
            {
               /// skip the &
               analyze_node(GET_INDEX_NODE(GetPointer<addr_expr>(operand)->op), left_p, false, true);
            }
            else
               analyze_node(GET_INDEX_NODE(tmr->base), left_p, false, true);
         }
         if(tmr->idx)
            analyze_node(GET_INDEX_NODE(tmr->idx), left_p, false, false);
         if(tmr->idx2)
            analyze_node(GET_INDEX_NODE(tmr->idx2), left_p, false, false);

         /// check for unaligned accesses
         if(tmr->base)
         {
            tree_nodeRef type_base = tree_helper::get_type_node(GET_NODE(tmr->base));
            auto* t_base_ptr = GetPointer<type_node>(type_base);
            if(t_base_ptr->algn != 8)
            {
            }
         }
         break;
      }
      case function_decl_K:
      case template_decl_K:
      {
         break;
      }
      case gimple_asm_K:
      {
         auto* ga = GetPointer<gimple_asm>(tn);
         if(ga->in)
            analyze_node(GET_INDEX_NODE(ga->in), false, false, false);
         if(ga->out)
            analyze_node(GET_INDEX_NODE(ga->out), true, false, false);
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
         THROW_ERROR_CODE(NODE_NOT_YET_SUPPORTED_EC, "Not supported node (@" + STR(node_id) + ") of type " + std::string(tn->get_kind_text()) + " in function " + function_name);
         break;
      }
      default:
      {
         THROW_UNREACHABLE("");
      }
   }
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Analyzed node " + tn->ToString());
}

bool determine_memory_accesses::HasToBeExecuted() const
{
   return not already_executed;
}
