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
 *              Copyright (C) 2016-2020 Politecnico di Milano
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
 * @file mem_cg_ext.cpp
 *
 * Created on: July 18, 2016
 *
 * @author Pietro Fezzardi <pietrofezzardi@gmail.com>
 * @author Marco Lattuada <marco.lattuada@polimi.it>
 *
 */

// include class header
#include "mem_cg_ext.hpp"

// include from src/
#include "Parameter.hpp"

// include from src/behavior/
#include "call_graph.hpp"
#include "call_graph_manager.hpp"
#include "function_behavior.hpp"

#include "application_manager.hpp"

// include from src/tree/
#include "behavioral_helper.hpp"
#include "dbgPrintHelper.hpp"      // for DEBUG_LEVEL_
#include "string_manipulation.hpp" // for GET_CLASS
#include "tree_basic_block.hpp"
#include "tree_helper.hpp"
#include "tree_manager.hpp"
#include "tree_manipulation.hpp"
#include "tree_node.hpp"
#include "tree_reindex.hpp"

mem_cg_ext::mem_cg_ext(const application_managerRef _AppM, const unsigned int _function_id, const DesignFlowManagerConstRef _design_flow_manager, const ParameterConstRef _parameters)
    : FunctionFrontendFlowStep(_AppM, _function_id, MEM_CG_EXT, _design_flow_manager, _parameters)
{
   debug_level = parameters->get_class_debug_level(GET_CLASS(*this), DEBUG_LEVEL_NONE);
}

mem_cg_ext::~mem_cg_ext() = default;

const CustomUnorderedSet<std::pair<FrontendFlowStepType, FrontendFlowStep::FunctionRelationship>> mem_cg_ext::ComputeFrontendRelationships(const DesignFlowStep::RelationshipType relationship_type) const
{
   CustomUnorderedSet<std::pair<FrontendFlowStepType, FunctionRelationship>> relationships;
   switch(relationship_type)
   {
      case DEPENDENCE_RELATIONSHIP:
      {
         relationships.insert(std::make_pair(COMPUTE_IMPLICIT_CALLS, ALL_FUNCTIONS));
         relationships.insert(std::make_pair(IR_LOWERING, ALL_FUNCTIONS));
         relationships.insert(std::make_pair(FUNCTION_ANALYSIS, WHOLE_APPLICATION));
         relationships.insert(std::make_pair(USE_COUNTING, ALL_FUNCTIONS));
         relationships.insert(std::make_pair(FIX_STRUCTS_PASSED_BY_VALUE, ALL_FUNCTIONS));
         /// Workaround: this should be a precedence, but when this is added later for functions added during call graph extension is added as unnecessary and it is not updated before its execution
         if(parameters->isOption(OPT_soft_float) and parameters->getOption<bool>(OPT_soft_float))
         {
            relationships.insert(std::make_pair(SOFT_FLOAT_CG_EXT, ALL_FUNCTIONS));
         }
         break;
      }
      case PRECEDENCE_RELATIONSHIP:
      {
         relationships.insert(std::make_pair(UN_COMPARISON_LOWERING, ALL_FUNCTIONS));
         break;
      }
      case INVALIDATION_RELATIONSHIP:
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

DesignFlowStep_Status mem_cg_ext::InternalExec()
{
   const CallGraphManagerRef CGMan = AppM->GetCallGraphManager();
   const CallGraphConstRef cg = CGMan->CGetCallGraph();
   const tree_managerRef TM = AppM->get_tree_manager();
   const tree_manipulationRef tree_man = tree_manipulationRef(new tree_manipulation(TM, parameters));
   CustomOrderedSet<unsigned int> changed_fu_ids;
   const auto reached_body_fun_ids = CGMan->GetReachedBodyFunctions();
   for(unsigned int fu_id : reached_body_fun_ids)
   {
      const std::string fu_name = AppM->CGetFunctionBehavior(fu_id)->CGetBehavioralHelper()->get_function_name();
      if(fu_name != "__internal_bambu_memcpy" and fu_name != "__internal_bambu_memset")
      {
         continue;
      }
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->analyzing function " + fu_name);
      const vertex fu_cgv = CGMan->GetVertex(fu_id);
      InEdgeIterator ie_it, ie_end;
      boost::tie(ie_it, ie_end) = boost::in_edges(fu_cgv, *cg);
      for(; ie_it != ie_end; ie_it++)
      {
         const unsigned int caller_id = CGMan->get_function(boost::source(*ie_it, *cg));
         if(caller_id != function_id)
            continue;
         const auto tmp_it = reached_body_fun_ids.find(caller_id);
         if(tmp_it == reached_body_fun_ids.cend())
            continue;

         const FunctionBehaviorConstRef FB = AppM->CGetFunctionBehavior(caller_id);
         const std::string caller_name = FB->CGetBehavioralHelper()->get_function_name();
         const tree_nodeConstRef caller_tn = TM->get_tree_node_const(caller_id);
         const auto* caller_fd = GetPointer<const function_decl>(caller_tn);
         THROW_ASSERT(caller_fd, "");
         const auto* sl = GetPointer<const statement_list>(GET_NODE(caller_fd->body));
         THROW_ASSERT(sl, "");
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->analyzing caller function " + caller_name);

         const FunctionEdgeInfoConstRef call_edge_info = cg->CGetFunctionEdgeInfo(*ie_it);
         bool changed;
         do
         {
            changed = false;
            /*
             * this loop cannot be made ranged, because it calls
             * replace_call_point(), which changes call_edge_info->direct_call_points
             */
            auto call_it = call_edge_info->direct_call_points.begin();
            auto call_it_end = call_edge_info->direct_call_points.end();
            for(; call_it != call_it_end; call_it++)
            {
               unsigned int i = *call_it;
               if(i == 0)
                  continue;
               const tree_nodeRef call_node = TM->get_tree_node_const(i);
               const tree_nodeRef call_node_reindex = TM->CGetTreeReindex(i);
               if(call_node->get_kind() != gimple_assign_K)
                  continue;
               const auto* ga = GetPointer<const gimple_assign>(call_node);
               if(GET_NODE(ga->op1)->get_kind() != call_expr_K && GET_NODE(ga->op1)->get_kind() != aggr_init_expr_K)
               {
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->analyzing hidden call " + STR(ga) + " id " + STR(i));

                  const auto lhs_node = GET_NODE(ga->op0);
                  THROW_ASSERT(lhs_node->get_kind() == mem_ref_K,
                               "unexpected condition: " + caller_name + " calls function " + fu_name + " in operation " + ga->ToString() + " but lhs " + lhs_node->ToString() + " is not a mem_ref: it's a " + lhs_node->get_kind_text());
                  const auto* mr_lhs = GetPointer<const mem_ref>(lhs_node);
                  THROW_ASSERT(GetPointer<const ssa_name>(GET_NODE(mr_lhs->op0)), "");
#if HAVE_ASSERTS
                  const auto* dst_offset = GetPointer<const integer_cst>(GET_NODE(mr_lhs->op1));
#endif
                  THROW_ASSERT(dst_offset, "");
                  THROW_ASSERT(dst_offset->value == 0, "");
                  const auto rhs_node = GET_NODE(ga->op1);
                  const auto rhs_kind = rhs_node->get_kind();

                  const auto* s = GetPointer<const srcp>(call_node);
                  const std::string current_srcp = s ? (s->include_name + ":" + STR(s->line_number) + ":" + STR(s->column_number)) : "";
                  // node for the new gimple_call
                  tree_nodeRef new_gimple_call;
                  unsigned long int copy_byte_size = 0;
                  // args to be filled before the creation of the gimple call
                  std::vector<tree_nodeRef> args;
                  // dst is always the ssa on the rhs
                  args.push_back(mr_lhs->op0);
                  if(fu_name == "__internal_bambu_memcpy")
                  {
                     THROW_ASSERT(rhs_kind == mem_ref_K or rhs_kind == parm_decl_K or rhs_kind == string_cst_K,
                                  "unexpected condition: " + caller_name + " calls function " + fu_name + " in operation " + ga->ToString() + " but rhs " + rhs_node->ToString() + " is not a mem_ref: it's a " + rhs_node->get_kind_text());

                     if(rhs_kind == mem_ref_K)
                     {
                        const auto* mr_rhs = GetPointer<const mem_ref>(rhs_node);
                        THROW_ASSERT(GetPointer<const ssa_name>(GET_NODE(mr_rhs->op0)), "");
#if HAVE_ASSERTS
                        const auto* src_offset = GetPointer<const integer_cst>(GET_NODE(mr_rhs->op1));
#endif
                        THROW_ASSERT(src_offset, "");
                        THROW_ASSERT(src_offset->value == 0, "");

                        // src
                        args.push_back(mr_rhs->op0);

                        // compute the size in bytes of the copied memory
                        const auto dst_type = tree_helper::CGetType(GET_NODE(mr_lhs->op0));
                        const auto src_type = tree_helper::CGetType(GET_NODE(mr_rhs->op0));
                        const auto* dst_ptr_t = GetPointer<const pointer_type>(dst_type);
                        const auto* src_ptr_t = GetPointer<const pointer_type>(src_type);
                        unsigned int dst_size;
                        if(dst_ptr_t)
                           dst_size = tree_helper::Size(dst_ptr_t->ptd);
                        else
                        {
                           const auto* dst_rptr_t = GetPointer<const reference_type>(dst_type);
                           dst_size = tree_helper::Size(dst_rptr_t->refd);
                        }
                        unsigned int src_size;
                        if(src_ptr_t)
                           src_size = tree_helper::Size(src_ptr_t->ptd);
                        else
                        {
                           const auto* src_rptr_t = GetPointer<const reference_type>(src_type);
                           src_size = tree_helper::Size(src_rptr_t->refd);
                        }
                        if(src_size != dst_size)
                        {
                           INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---WARNING: src_size = " + STR(src_size) + "; dst_size = " + STR(dst_size));
                        }
                        THROW_ASSERT(src_size % 8 == 0, "");
                        copy_byte_size = src_size / 8;
                     }
                     else if(rhs_kind == string_cst_K)
                     {
                        // compute src param
                        auto memcpy_src_ga = tree_man->CreateGimpleAssignAddrExpr(rhs_node, ga->bb_index, current_srcp);
                        // push the new gimple_assign with lhs = addr_expr(param_decl) before the call
                        THROW_ASSERT(not sl->list_of_bloc.empty(), "");
                        THROW_ASSERT(sl->list_of_bloc.find(ga->bb_index) != sl->list_of_bloc.end(), "");
                        blocRef block = sl->list_of_bloc.at(ga->bb_index);
                        block->PushBefore(memcpy_src_ga, call_node_reindex);
                        // push back src param
                        const auto* new_ga = GetPointer<const gimple_assign>(GET_NODE(memcpy_src_ga));
                        args.push_back(new_ga->op0);

                        // compute the size in bytes of the copied memory
                        const auto dst_type = tree_helper::CGetType(GET_NODE(mr_lhs->op0));
                        const auto* dst_ptr_t = GetPointer<const pointer_type>(dst_type);
                        THROW_ASSERT(dst_ptr_t, "");
                        const auto dst_bitsize = tree_helper::Size(dst_ptr_t->ptd);
                        THROW_ASSERT(dst_bitsize % 8 == 0, "");
                        const auto dst_size = dst_bitsize / 8;
                        const auto* sc = GetPointer<const string_cst>(rhs_node);
                        const auto src_strlen = sc->strg.length();
                        if((src_strlen + 1) != dst_size)
                        {
                           INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---WARNING: src_strlen = " + STR(src_strlen) + "; dst_size = " + STR(dst_size));
                        }
                        copy_byte_size = src_strlen;
                     }
                     else // rhs_kind == parm_decl_K
                     {
                        // compute src param
                        auto memcpy_src_ga = tree_man->CreateGimpleAssignAddrExpr(rhs_node, ga->bb_index, current_srcp);
                        // push the new gimple_assign with lhs = addr_expr(param_decl) before the call
                        THROW_ASSERT(not sl->list_of_bloc.empty(), "");
                        THROW_ASSERT(sl->list_of_bloc.find(ga->bb_index) != sl->list_of_bloc.end(), "");
                        blocRef block = sl->list_of_bloc.at(ga->bb_index);
                        block->PushBefore(memcpy_src_ga, call_node_reindex);
                        // push back src param
                        const auto* new_ga = GetPointer<const gimple_assign>(GET_NODE(memcpy_src_ga));
                        args.push_back(new_ga->op0);

                        // compute the size in bytes of the copied memory
                        const auto dst_type = tree_helper::CGetType(GET_NODE(mr_lhs->op0));
                        const auto src_type = GET_NODE(tree_man->create_pointer_type(tree_helper::CGetType(rhs_node), 8));
                        const auto* dst_ptr_t = GetPointer<const pointer_type>(dst_type);
                        const auto* src_ptr_t = GetPointer<const pointer_type>(src_type);
                        THROW_ASSERT(dst_ptr_t, "");
                        THROW_ASSERT(src_ptr_t, "");
                        const auto dst_size = tree_helper::Size(dst_ptr_t->ptd);
                        const auto src_size = tree_helper::Size(src_ptr_t->ptd);
                        if(src_size != dst_size)
                        {
                           INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---WARNING: src_size = " + STR(src_size) + "; dst_size = " + STR(dst_size));
                        }
                        THROW_ASSERT(src_size % 8 == 0, "");
                        copy_byte_size = src_size / 8;
                     }
                  }
                  else if(fu_name == "__internal_bambu_memset")
                  {
                     THROW_ASSERT(GetPointer<const constructor>(rhs_node)->list_of_idx_valu.empty(), "");

                     // create the second argument of memset
                     unsigned int memset_val_formal_type_id = tree_helper::get_formal_ith(TM, fu_id, 1);
                     const auto formal_type_node = TM->CGetTreeReindex(memset_val_formal_type_id);
                     args.push_back(tree_man->CreateIntegerCst(formal_type_node, 0, TM->new_tree_node_id()));

                     // compute the size of memory to be set with memset
                     const auto dst_type = tree_helper::CGetType(GET_NODE(mr_lhs->op0));
                     const auto* dst_ptr_t = GetPointer<const pointer_type>(dst_type);
                     THROW_ASSERT(dst_ptr_t, "");
                     const auto dst_size = tree_helper::Size(dst_ptr_t->ptd);
                     THROW_ASSERT(dst_size % 8 == 0, "");
                     copy_byte_size = dst_size / 8;
                  }
                  THROW_ASSERT(copy_byte_size <= std::numeric_limits<long long>::max(), "");
                  // add size to arguments
                  unsigned int size_formal_type_id = tree_helper::get_formal_ith(TM, fu_id, 2);
                  const auto formal_type_node = TM->CGetTreeReindex(size_formal_type_id);
                  args.push_back(tree_man->CreateIntegerCst(formal_type_node, static_cast<long long>(copy_byte_size), TM->new_tree_node_id()));
                  // create the new gimple call
                  new_gimple_call = tree_man->create_gimple_call(TM->CGetTreeReindex(fu_id), args, current_srcp, ga->bb_index);
                  // replace the gimple_assign with the new gimple_call
                  THROW_ASSERT(not sl->list_of_bloc.empty(), "");
                  THROW_ASSERT(sl->list_of_bloc.find(ga->bb_index) != sl->list_of_bloc.end(), "");
                  blocRef block = sl->list_of_bloc.at(ga->bb_index);
                  block->Replace(call_node_reindex, new_gimple_call, true);
                  CGMan->ReplaceCallPoint(*ie_it, ga->index, GET_INDEX_NODE(new_gimple_call));
                  changed_fu_ids.insert(caller_id);
                  changed = true;
                  /*
                   * break out of the for loop on the direct call points,
                   * because replace_call_point invalidates the iterators and we
                   * have to restart
                   */
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--replaced hidden call " + STR(ga) + " id: " + STR(i) + " with call " + STR(new_gimple_call) + " id: " + STR(new_gimple_call->index));
                  break;
               }
            }
         } while(changed);
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--analyzed caller function " + caller_name);
      }
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--analyzed function " + fu_name);
   }

   for(auto i : changed_fu_ids)
   {
      AppM->GetFunctionBehavior(i)->UpdateBBVersion();
   }

   if(changed_fu_ids.empty())
      return DesignFlowStep_Status::UNCHANGED;
   else
      return DesignFlowStep_Status::SUCCESS;
}

void mem_cg_ext::Initialize()
{
}
