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
 * @file simple_code_motion.cpp
 * @brief Analysis step that performs some simple code motions over the IR
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 *
 */
#include "simple_code_motion.hpp"

#include "Parameter.hpp"
#include "SemiNCADominance.hpp"
#include "application_manager.hpp"
#include "basic_block.hpp"
#include "behavioral_helper.hpp"
#include "dbgPrintHelper.hpp"
#include "design_flow_graph.hpp"
#include "design_flow_manager.hpp"
#include "function_behavior.hpp"
#include "hls.hpp"
#include "hls_manager.hpp"
#include "hls_step.hpp"
#include "ir_basic_block.hpp"
#include "ir_helper.hpp"
#include "ir_manager.hpp"
#include "ir_manipulation.hpp"
#include "ir_node.hpp"
#include "math_function.hpp"
#include "schedule.hpp"
#include "string_manipulation.hpp"
#include <fstream>

simple_code_motion::simple_code_motion(const ParameterConstRef _parameters, const application_managerRef _AppM,
                                       unsigned int _function_id, const DesignFlowManager& _design_flow_manager)
    : FunctionFrontendFlowStep(_AppM, _function_id, SIMPLE_CODE_MOTION, _design_flow_manager, _parameters),
      restart_ifmwi_opt(false),
      schedule(ScheduleRef()),
      conservative(
          (parameters->IsParameter("enable-conservative-sdc") &&
           parameters->GetParameter<bool>("enable-conservative-sdc") &&
           parameters->isOption(OPT_scheduling_algorithm) and
           parameters->getOption<HLSFlowStep_Type>(OPT_scheduling_algorithm) == HLSFlowStep_Type::SDC_SCHEDULING) ?
              true :
              false)
{
   debug_level = _parameters->get_class_debug_level(GET_CLASS(*this), DEBUG_LEVEL_NONE);
}

CustomUnorderedSet<std::pair<FrontendFlowStepType, FunctionFrontendFlowStep::FunctionRelationship>>
simple_code_motion::ComputeFrontendRelationships(const DesignFlowStep::RelationshipType relationship_type) const
{
   CustomUnorderedSet<std::pair<FrontendFlowStepType, FunctionRelationship>> relationships;
   switch(relationship_type)
   {
      case(DEPENDENCE_RELATIONSHIP):
      {
         relationships.insert(std::make_pair(PREDICATE_STATEMENTS, SAME_FUNCTION));
         relationships.insert(std::make_pair(USE_COUNTING, SAME_FUNCTION));
         break;
      }
      case(PRECEDENCE_RELATIONSHIP):
      {
         relationships.insert(std::make_pair(SDC_CODE_MOTION, SAME_FUNCTION));
         relationships.insert(std::make_pair(DEAD_CODE_ELIMINATION_IPA, WHOLE_APPLICATION));
         break;
      }
      case(INVALIDATION_RELATIONSHIP):
      {
         if(GetStatus() == DesignFlowStep_Status::SUCCESS)
         {
            if(restart_ifmwi_opt)
            {
               relationships.insert(std::make_pair(PHI_OPT, SAME_FUNCTION));
               relationships.insert(std::make_pair(MULTI_WAY_IF, SAME_FUNCTION));
            }
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

void simple_code_motion::Initialize()
{
   if(GetPointer<const HLS_manager>(AppM) && GetPointer<const HLS_manager>(AppM)->get_HLS(function_id) &&
      GetPointer<const HLS_manager>(AppM)->get_HLS(function_id)->Rsch)
   {
      if(parameters->isOption(OPT_scheduling_algorithm) &&
         parameters->getOption<HLSFlowStep_Type>(OPT_scheduling_algorithm) == HLSFlowStep_Type::SDC_SCHEDULING)
      {
         const auto TM = AppM->get_ir_manager();
         schedule = GetPointerS<const HLS_manager>(AppM)->get_HLS(function_id)->Rsch;
      }
   }
}

FunctionFrontendFlowStep_Movable simple_code_motion::CheckMovable(const unsigned int bb_index, ir_nodeRef tn,
                                                                  bool& zero_delay)
{
   if(AppM->CGetFunctionBehavior(function_id)->is_function_pipelined())
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Yes because we aim to full pipelining");
      return FunctionFrontendFlowStep_Movable::MOVABLE;
   }
   if(tn->get_kind() == nop_stmt_K)
   {
      return FunctionFrontendFlowStep_Movable::MOVABLE;
   }

   auto* ga = GetPointer<assign_stmt>(tn);
   auto* gc = GetPointer<call_stmt>(tn);
#if HAVE_ASSERTS || !defined(NDEBUG)
   auto* ns = GetPointer<node_stmt>(tn);
#endif
   THROW_ASSERT(ns, "unexpected condition");
#if HAVE_ASSERTS
   const bool is_assign = ga != nullptr;
#endif
   const bool is_call_stmt = gc != nullptr;

   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                  "-->Checking if " + STR(ns->index) + " - " + ns->ToString() + " can be moved in BB" + STR(bb_index));
   if(is_call_stmt)
   {
      if(schedule)
      {
         auto movable = schedule->CanBeMoved(gc->index, bb_index);
         if(movable == FunctionFrontendFlowStep_Movable::UNMOVABLE or
            movable == FunctionFrontendFlowStep_Movable::TIMING)
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--No because of timing");
         }
         else
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Yes because of timing");
         }
         return movable;
      }
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Yes because it is a call statement");
      return FunctionFrontendFlowStep_Movable::MOVABLE;
   }
   THROW_ASSERT(is_assign, "unexpected condition");
   ir_nodeRef left = ga->op0;
   const bool is_assign_call = ga->op1->get_kind() == call_node_K;
   if(is_assign_call)
   {
      if(schedule)
      {
         auto movable = schedule->CanBeMoved(ga->index, bb_index);
         if(movable == FunctionFrontendFlowStep_Movable::UNMOVABLE or
            movable == FunctionFrontendFlowStep_Movable::TIMING)
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--No because of timing");
         }
         else
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Yes because of timing");
         }
         return movable;
      }
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Yes because it is an assign call");
      return FunctionFrontendFlowStep_Movable::MOVABLE;
   }
   bool storeCanBePredicated = false;
   if(left->get_kind() == mem_access_node_K)
   {
      storeCanBePredicated = true;
   }
   if(storeCanBePredicated)
   {
      if(schedule)
      {
         auto movable = schedule->CanBeMoved(ga->index, bb_index);
         if(movable == FunctionFrontendFlowStep_Movable::UNMOVABLE or
            movable == FunctionFrontendFlowStep_Movable::TIMING)
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--No because of timing");
         }
         else
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Yes because of timing");
         }
         return movable;
      }
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Yes because it is a predicable store");
      return FunctionFrontendFlowStep_Movable::MOVABLE;
   }
   if(!GetPointer<ssa_node>(left))
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                     "<--No because of " + left->get_kind_text() + " in left part");
      return FunctionFrontendFlowStep_Movable::UNMOVABLE;
   }
   if(ir_helper::IsConstant(ga->op1))
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Yes because right part is constant");
      return FunctionFrontendFlowStep_Movable::MOVABLE;
   }
   if(ga->op0->get_kind() == ssa_node_K && ga->op1->get_kind() == constructor_node_K)
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                     "<--Yes because it is an assignment with a constructor_node");
      return FunctionFrontendFlowStep_Movable::MOVABLE;
   }
   if(ga->op0->get_kind() == ssa_node_K && ga->op1->get_kind() == ssa_node_K)
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Yes because it is an assignment");
      return FunctionFrontendFlowStep_Movable::MOVABLE;
   }
   CustomOrderedSet<const ssa_node*> rhs_ssa_uses;
   ir_helper::compute_ssa_uses_rec_ptr(ga->op1, rhs_ssa_uses);
   ir_nodeRef right = ga->op1;

   if(rhs_ssa_uses.empty() && right->get_kind() != call_node_K && right->get_kind() != variable_val_node_K &&
      right->get_kind() != mem_access_node_K)
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                     "<--Yes because there is not any use of ssa in right part");
      return FunctionFrontendFlowStep_Movable::MOVABLE;
   }

   /// If we have the ending time information use it
   if(schedule)
   {
      auto movable = schedule->CanBeMoved(ga->index, bb_index);
      if(movable == FunctionFrontendFlowStep_Movable::UNMOVABLE or movable == FunctionFrontendFlowStep_Movable::TIMING)
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--No because of timing");
      }
      else
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Yes because of timing");
      }
      return movable;
   }
   switch(right->get_kind())
   {
      case bitcast_node_K:
      case ssa_node_K:
      case constructor_node_K:
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Yes");
         return FunctionFrontendFlowStep_Movable::MOVABLE;
      }
      /// binary expressions
      case eq_node_K:
      case shl_node_K:
      case max_node_K:
      case min_node_K:
      case ne_node_K:
      case shr_node_K:
      {
         if(conservative)
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--No");
            return FunctionFrontendFlowStep_Movable::UNMOVABLE;
         }
         else
         {
            auto* be = GetPointer<binary_node>(right);
            auto n_bit = std::max(ir_helper::Size(be->op0), ir_helper::Size(be->op1));
            bool is_constant = ir_helper::IsConstant(be->op0) || ir_helper::IsConstant(be->op1);
            if(n_bit > 9 && !is_constant)
            {
               zero_delay = false;
            }
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Yes");
            return FunctionFrontendFlowStep_Movable::MOVABLE;
         }
      }
      case fshl_node_K:
      case fshr_node_K:
      {
         if(conservative)
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--No");
            return FunctionFrontendFlowStep_Movable::UNMOVABLE;
         }
         else
         {
            auto* te = GetPointer<ternary_node>(right);
            auto n_bit = ir_helper::Size(te->op0);
            bool is_constant = ir_helper::IsConstant(te->op1);
            if(n_bit > 9 && !is_constant)
            {
               zero_delay = false;
            }
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Yes");
            return FunctionFrontendFlowStep_Movable::MOVABLE;
         }
      }
      case mul_node_K:
      case widen_mul_node_K:
      {
         if(ir_helper::IsRealType(ga->op1))
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--No because floating point operations");
            return FunctionFrontendFlowStep_Movable::UNMOVABLE;
         }
         auto* be = GetPointer<binary_node>(right);
         if(ir_helper::IsConstant(be->op1))
         {
            const auto ic = GetPointer<constant_int_val_node>(be->op1);
            if(ic)
            {
               const auto v = ir_helper::GetConstValue(be->op1);
               if(!(v && !(v & (v - 1))))
               {
                  zero_delay = false;
               }
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Yes");
               return FunctionFrontendFlowStep_Movable::MOVABLE;
            }
            else
            {
               zero_delay = false;
            }
         }
         else
         {
            zero_delay = false;
         }
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                        "<--No because is a multiplication with non constant args");
         return FunctionFrontendFlowStep_Movable::UNMOVABLE;
      }
      case nop_node_K:
      {
         auto* ne = GetPointer<nop_node>(right);
         const auto left_type = ir_helper::CGetType(ga->op0);
         const auto right_type = ir_helper::CGetType(ne->op);
         const auto is_realR = ir_helper::IsRealType(right_type);
         const auto is_realL = ir_helper::IsRealType(left_type);
         if(is_realR || is_realL)
         {
            zero_delay = false;
         }

         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Yes");
         return FunctionFrontendFlowStep_Movable::MOVABLE;
      }
      case addr_node_K:
      {
         zero_delay = false;
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Yes");
         return FunctionFrontendFlowStep_Movable::MOVABLE;
      }
      case and_node_K:
      case or_node_K:
      case xor_node_K:
      case not_node_K:
      case select_node_K:
      case lut_node_K:
      {
         if(conservative)
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--No");
            return FunctionFrontendFlowStep_Movable::UNMOVABLE;
         }
         else
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Yes");
            return FunctionFrontendFlowStep_Movable::MOVABLE;
         }
      }
      case concat_bit_node_K:
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Yes");
         return FunctionFrontendFlowStep_Movable::MOVABLE;
      }
      case extractvalue_node_K:
      case insertvalue_node_K:
      case extract_bit_node_K:
      case extractelement_node_K:
      case insertelement_node_K:
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Yes");
         return FunctionFrontendFlowStep_Movable::MOVABLE;
      }
      case ge_node_K:
      case gt_node_K:
      case gep_node_K:
      {
         if(conservative)
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--No");
            return FunctionFrontendFlowStep_Movable::UNMOVABLE;
         }
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Yes");
         return FunctionFrontendFlowStep_Movable::MOVABLE;
      }
      case le_node_K:
      case lt_node_K:
      case sub_node_K:
      case add_node_K:
      case add_sat_node_K:
      case sub_sat_node_K:
      {
         if(ir_helper::IsRealType(ga->op1))
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--No because floating point operations");
            return FunctionFrontendFlowStep_Movable::UNMOVABLE;
         }
         if(conservative)
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--No");
            return FunctionFrontendFlowStep_Movable::UNMOVABLE;
         }
         else
         {
            auto* be = GetPointer<binary_node>(right);
            auto n_bit = std::max(ir_helper::Size(be->op0), ir_helper::Size(be->op1));
            auto n_bit_min = std::min(ir_helper::Size(be->op0), ir_helper::Size(be->op1));
            bool is_constant = ir_helper::IsConstant(be->op0) || ir_helper::IsConstant(be->op1);
            if((n_bit > 9 && !is_constant && n_bit_min != 1) || n_bit > 16)
            {
               zero_delay = false;
            }
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Yes");
            return FunctionFrontendFlowStep_Movable::MOVABLE;
         }
      }
      case ternary_add_node_K:
      case ternary_as_node_K:
      case ternary_sa_node_K:
      case ternary_ss_node_K:
      {
         if(conservative)
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--No");
            return FunctionFrontendFlowStep_Movable::UNMOVABLE;
         }
         else
         {
            auto* be = GetPointer<ternary_node>(right);
            auto n_bit =
                std::max(std::max(ir_helper::Size(be->op0), ir_helper::Size(be->op1)), ir_helper::Size(be->op2));
            auto n_bit_min =
                std::min(std::min(ir_helper::Size(be->op0), ir_helper::Size(be->op1)), ir_helper::Size(be->op2));
            bool is_constant = ir_helper::IsConstant(be->op0) || ir_helper::IsConstant(be->op1);
            if((n_bit > 9 && !is_constant && n_bit_min != 1) || n_bit > 16)
            {
               zero_delay = false;
            }
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Yes");
            return FunctionFrontendFlowStep_Movable::MOVABLE;
         }
      }
      case neg_node_K:
      {
         auto* ne = GetPointer<neg_node>(right);
         auto n_bit = ir_helper::Size(ne->op);
         bool is_constant = ir_helper::IsConstant(ne->op);
         if((n_bit > 9 && !is_constant) || n_bit > 16)
         {
            zero_delay = false;
         }
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Yes");
         return FunctionFrontendFlowStep_Movable::MOVABLE;
      }
      case itofp_node_K:
      {
         zero_delay = false;
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Yes");
         return FunctionFrontendFlowStep_Movable::MOVABLE;
      }
      case idiv_node_K:
      case irem_node_K:
      {
         auto* be = GetPointer<binary_node>(right);
         if(ir_helper::IsConstant(be->op1))
         {
            auto ic = GetPointer<constant_int_val_node>(be->op1);
            if(ic)
            {
               const auto v = ir_helper::GetConstValue(be->op1);
               if(v)
               {
                  if(!(v && !(v & (v - 1))))
                  {
                     zero_delay = false;
                  }
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Yes");
                  return FunctionFrontendFlowStep_Movable::MOVABLE;
               }
            }
         }
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--No because is a division by a non constant");
         return FunctionFrontendFlowStep_Movable::UNMOVABLE;
      }
      case abs_node_K:
      {
         if(conservative)
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--No");
            return FunctionFrontendFlowStep_Movable::UNMOVABLE;
         }
         else
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Yes");
            return FunctionFrontendFlowStep_Movable::MOVABLE;
         }
      }
      case mem_access_node_K:
      {
         zero_delay = false;
         if(schedule)
         {
            auto movable = schedule->CanBeMoved(ga->index, bb_index);
            if(movable == FunctionFrontendFlowStep_Movable::UNMOVABLE or
               movable == FunctionFrontendFlowStep_Movable::TIMING)
            {
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--No because of timing");
            }
            else
            {
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Yes because of timing");
            }
            return movable;
         }
         if(conservative)
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                           "<--No because conservative mode has no timing information for predicable loads");
            return FunctionFrontendFlowStep_Movable::UNMOVABLE;
         }
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--No because it is a predicable load");
         return FunctionFrontendFlowStep_Movable::UNMOVABLE;
      }
      case CASE_CST_NODES:
      case CASE_DECL_NODES:
      case CASE_NODE_STMTS:
      case shufflevector_node_K:
      case call_node_K:
      case CASE_FAKE_NODES:
      case fdiv_node_K:
      case frem_node_K:
      case identifier_node_K:
      case statement_list_node_K:
      case CASE_TYPE_NODES:
      case fptoi_node_K:
      case unaligned_mem_access_node_K:
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                        "<--No because right part is " + right->get_kind_text());
         return FunctionFrontendFlowStep_Movable::UNMOVABLE;
      }
      default:
      {
         THROW_UNREACHABLE("");
         return FunctionFrontendFlowStep_Movable::UNMOVABLE;
      }
   }
}

DesignFlowStep_Status simple_code_motion::InternalExec()
{
   const auto TM = AppM->get_ir_manager();
   bool modified = false;
   restart_ifmwi_opt = false;

   const auto fd = GetPointerS<const function_val_node>(TM->GetIRNode(function_id));
   const auto sl = GetPointerS<const statement_list_node>(fd->body);

   /// store the BB graph ala boost::graph
   BBGraphsCollection bb_graphs_collection(BBGraphInfo(AppM, function_id));
   auto& bb_graph_info = bb_graphs_collection.GetGraphInfo();
   BBGraph bb_graph(bb_graphs_collection, CFG_SELECTOR);
   CustomUnorderedMap<BBGraph::vertex_descriptor, unsigned int> direct_vertex_map;
   CustomUnorderedMap<unsigned int, BBGraph::vertex_descriptor> inverse_vertex_map;
   /// add vertices
   const auto& list_of_bloc = sl->list_of_bloc;
   for(const auto& block : list_of_bloc)
   {
      inverse_vertex_map[block.first] = bb_graphs_collection.AddVertex(BBNodeInfo(block.second));
      direct_vertex_map[inverse_vertex_map[block.first]] = block.first;
   }
   /// add edges
   for(const auto& bbi_bb : list_of_bloc)
   {
      const auto& bbi = bbi_bb.first;
      const auto& bb = bbi_bb.second;
      for(const auto& pred : bb->list_of_pred)
      {
         THROW_ASSERT(inverse_vertex_map.count(pred),
                      "BB" + STR(pred) + " (predecessor of " + STR(bbi) + ") does not exist");
         THROW_ASSERT(inverse_vertex_map.count(bbi), STR(bbi));
         bb_graphs_collection.AddEdge(inverse_vertex_map.at(pred), inverse_vertex_map.at(bbi), CFG_SELECTOR);
      }
      for(const auto& succ : bb->list_of_succ)
      {
         if(succ == bloc::EXIT_BLOCK_ID)
         {
            bb_graphs_collection.AddEdge(inverse_vertex_map.at(bbi), inverse_vertex_map.at(succ), CFG_SELECTOR);
         }
      }
      if(bb->list_of_succ.empty())
      {
         bb_graphs_collection.AddEdge(inverse_vertex_map.at(bbi), inverse_vertex_map.at(bloc::EXIT_BLOCK_ID),
                                      CFG_SELECTOR);
      }
   }
   bb_graph_info.entry_vertex = inverse_vertex_map.at(bloc::ENTRY_BLOCK_ID);
   bb_graph_info.exit_vertex = inverse_vertex_map.at(bloc::EXIT_BLOCK_ID);
   /// add a connection between entry and exit thus avoiding problems with non terminating code
   bb_graphs_collection.AddEdge(inverse_vertex_map.at(bloc::ENTRY_BLOCK_ID), inverse_vertex_map.at(bloc::EXIT_BLOCK_ID),
                                CFG_SELECTOR);
   /// sort basic block vertices from the entry till the exit
   std::list<BBGraph::vertex_descriptor> bb_sorted_vertices;
   struct LocalDFSVisitor : public boost::dfs_visitor<>
   {
      explicit LocalDFSVisitor(std::list<BBGraph::vertex_descriptor>& Out) : Lref(Out)
      {
      }
      void finish_vertex(const BBGraph::vertex_descriptor& u, const BBGraph&)
      {
         Lref.push_front(u);
      }
      std::list<BBGraph::vertex_descriptor>& Lref;
   };
   {
      LocalDFSVisitor vis(bb_sorted_vertices);
      std::vector<boost::default_color_type> color_storage(boost::num_vertices(bb_graph));
      const auto idmap = boost::get(boost::vertex_index_t(), bb_graph);
      auto color_map = boost::make_iterator_property_map(color_storage.begin(), idmap, color_storage[0]);
      boost::depth_first_search(bb_graph, boost::visitor(vis).color_map(color_map).vertex_index_map(idmap));
   }
   static size_t counter = 0;
   if(debug_level >= DEBUG_LEVEL_VERY_PEDANTIC &&
      (!parameters->IsParameter("print-dot-FF") || parameters->GetParameter<unsigned int>("print-dot-FF")))
   {
      bb_graph.writeDot(function_behavior->GetDotPath() / ("BB_simple_code_motion_" + STR(counter) + ".dot"));
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                     "---Written BB_simple_code_motion_" + STR(counter) + ".dot");
      counter++;
   }

   dominance<BBGraph> bb_dominators(bb_graph, inverse_vertex_map.at(bloc::ENTRY_BLOCK_ID),
                                    inverse_vertex_map.at(bloc::EXIT_BLOCK_ID));
   const ir_manipulationConstRef ir_man(new ir_manipulation(TM, parameters, AppM));

   for(const auto bb_vertex : bb_sorted_vertices)
   {
      const auto curr_bb = direct_vertex_map.at(bb_vertex);
      if(curr_bb == bloc::ENTRY_BLOCK_ID)
      {
         continue;
      }
      if(curr_bb == bloc::EXIT_BLOCK_ID)
      {
         continue;
      }
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Analyzing BB" + STR(curr_bb));
      bool restart_bb_code_motion = false;
      do
      {
         if(debug_level >= DEBUG_LEVEL_VERY_PEDANTIC &&
            (!parameters->IsParameter("print-dot-FF") || parameters->GetParameter<unsigned int>("print-dot-FF")))
         {
            bb_graph.writeDot(function_behavior->GetDotPath() / ("BB_simple_code_motion_" + STR(counter) + ".dot"));
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                           "---Written BB_simple_code_motion_" + STR(counter) + ".dot");
            counter++;
         }
         const auto& list_of_stmt = list_of_bloc.at(curr_bb)->CGetStmtList();
         std::list<ir_nodeRef> to_be_removed;
         CustomOrderedSet<unsigned int> zero_delay_stmts;
         std::list<ir_nodeRef> to_be_added_back;
         std::list<ir_nodeRef> to_be_added_front;
         /// We must use pointer since we are erasing elements in the list
         for(auto statement = list_of_stmt.begin(); statement != list_of_stmt.end(); statement++)
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Analyzing " + (*statement)->ToString());
            /// skip statements defining or using virtual operands
            ir_nodeRef tn = *statement;
            auto* gn = GetPointer<node_stmt>(tn);
            auto* ga = GetPointer<assign_stmt>(tn);
            auto* gc = GetPointer<call_stmt>(tn);
            const bool is_nop = GetPointer<nop_stmt>(tn) != nullptr;
            const bool is_assign = ga != nullptr;
            const bool is_call_stmt = gc != nullptr;
            const bool is_load_assign = is_assign && ga->op1->get_kind() == mem_access_node_K;
            const bool is_store_assign = is_assign && ga->op0->get_kind() == mem_access_node_K;
            const bool is_assign_call = is_assign && ga->op1->get_kind() == call_node_K;
            bool loadCanBePredicated = false;
            if(is_load_assign)
            {
               loadCanBePredicated = true;
            }
            bool storeCanBePredicated = false;
            if(is_store_assign)
            {
               storeCanBePredicated = true;
            }
            if(is_call_stmt || is_assign_call)
            {
               if(gn->vdef)
               {
                  storeCanBePredicated = true;
               }
               else if(gn->vuses.size())
               {
                  loadCanBePredicated = true;
               }
            }

            THROW_ASSERT(gn, "unexpected condition");
            if(!storeCanBePredicated && gn->vdef && !is_nop) /// load can be loop pipelined/predicated
            {
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Skipped because of memory store");
               continue;
            }

            /// only assign_stmt/call_stmt/nop_stmt are considered for code motion
            if(!is_assign && !is_call_stmt && !is_nop)
            {
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Skipped because " + tn->get_kind_text());
               continue;
            }

            /// compute the SSA variables used by stmt
            CustomOrderedSet<const ssa_node*> stmt_ssa_uses;
            ir_helper::compute_ssa_uses_rec_ptr(*statement, stmt_ssa_uses);
            for(const auto& vo : gn->vovers)
            {
               ir_helper::compute_ssa_uses_rec_ptr(vo, stmt_ssa_uses);
            }

            /// compute BB where the SSA variables are defined
            CustomOrderedSet<unsigned int> BB_def;
            /// check for anti-dependencies
            for(auto stmt0 = list_of_stmt.begin(); stmt0 != list_of_stmt.end() && *stmt0 != *statement && gn->vdef;
                stmt0++)
            {
               ir_nodeRef tn0 = *stmt0;
               const auto gn0 = GetPointerS<node_stmt>(tn0);
               if(gn0->vuses.find(gn->vdef) != gn0->vuses.end())
               {
                  BB_def.insert(curr_bb);
               }
            }

            const auto ssu_it_end = stmt_ssa_uses.cend();
            for(auto ssu_it = stmt_ssa_uses.cbegin(); ssu_it != ssu_it_end; ++ssu_it)
            {
               const auto sn = *ssu_it;
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---for variable " + sn->ToString());
               auto def_stmt = sn->GetDefStmt();
               auto* def_gn = GetPointer<node_stmt>(def_stmt);
               THROW_ASSERT(def_gn->get_kind() == nop_stmt_K or def_gn->index, sn->ToString() + " is defined in entry");
               THROW_ASSERT(def_gn->get_kind() == nop_stmt_K or def_gn->bb_index or sn->virtual_flag,
                            "Definition " + def_gn->ToString() + " of " + sn->ToString() + " is in BB" +
                                STR(def_gn->bb_index));
               if(statement == list_of_stmt.begin() && list_of_bloc.at(curr_bb)->list_of_pred.size() == 1 &&
                  def_gn->bb_index == curr_bb && def_gn->get_kind() != phi_stmt_K)
               {
                  /// allow to move first statements later overwritten in the same BB
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---  no constraint because is the first one");
               }
               else
               {
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                 "---  Adding BB" + STR(def_gn->bb_index) + " because of " + def_gn->ToString());
                  BB_def.insert(def_gn->bb_index);
               }
            }
            /// skip the statement in case it uses ssa variables defined in the current BB
            if(BB_def.find(curr_bb) != BB_def.end())
            {
               /// check if list of pred has a loop_id greater than the loop_id of curr_bb
               bool can_be_pipelined = list_of_bloc.at(curr_bb)->loop_id != 0;
               const auto Lop_it_end = list_of_bloc.at(curr_bb)->list_of_pred.end();
               for(auto Lop_it = list_of_bloc.at(curr_bb)->list_of_pred.begin();
                   Lop_it != Lop_it_end && can_be_pipelined; ++Lop_it)
               {
                  can_be_pipelined = list_of_bloc.at(curr_bb)->loop_id >= list_of_bloc.at(*Lop_it)->loop_id;
               }
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                              "<--Skipped because uses ssa defined in the same block");
               continue;
            }
            if((((gn->vuses.size() && !is_nop) || is_load_assign || is_assign_call || is_call_stmt)
                // && (!schedule)
                && !loadCanBePredicated && !is_assign_call && !is_call_stmt))
            {
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Skipped because of vuses");
               continue; /// load cannot be code moved
            }
            /// find in which BB can be moved
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Checking where it can be moved");
            auto dest_bb_index = curr_bb;
            auto prev_dest_bb_index = curr_bb;
            if(gn->vdef || gn->vuses.size() || is_load_assign || is_assign_call || is_call_stmt)
            {
               if(list_of_bloc.at(curr_bb)->list_of_pred.size() == 1 &&
                  list_of_bloc.at(curr_bb)->list_of_pred.front() != bloc::ENTRY_BLOCK_ID &&
                  (is_store_assign || is_load_assign || is_assign_call || is_call_stmt || is_nop) &&
                  list_of_bloc.at(list_of_bloc.at(curr_bb)->list_of_pred.front())->loop_id ==
                      list_of_bloc.at(curr_bb)->loop_id)
               {
                  dest_bb_index = list_of_bloc.at(curr_bb)->list_of_pred.front();
               }
               else
               {
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                 "<--Skipped because is not a predicable store/load/call or because we do not know the "
                                 "condition under which it is done");
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--");
                  continue;
               }
            }
            else
            {
               auto dom_bb = bb_vertex;
               auto parent_bb = bb_dominators.getImmediateDominator(dom_bb);
               if(parent_bb != dom_bb)
               {
                  dom_bb = parent_bb;
                  auto dom_bb_index = direct_vertex_map[dom_bb];
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                 "---Considering its dominator BB" + STR(dom_bb_index));
                  while(dom_bb_index != bloc::ENTRY_BLOCK_ID)
                  {
                     unsigned loop_idU = list_of_bloc.at(dom_bb_index)->loop_id;
                     unsigned loop_idC = list_of_bloc.at(curr_bb)->loop_id;
                     if(loop_idC >= loop_idU)
                     {
                        prev_dest_bb_index = dest_bb_index;
                        dest_bb_index = dom_bb_index;
                        bool internLoopDep = false;
                        for(auto BBdef : BB_def)
                        {
                           if(list_of_bloc.at(BBdef)->loop_id > loop_idU && list_of_bloc.at(BBdef)->loop_id <= loop_idC)
                           {
                              internLoopDep = true;
                           }
                        }
                        if(internLoopDep)
                        {
                           INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                          "---the statement depends on values defined at inner level than dest BB" +
                                              STR(dest_bb_index));
                           dest_bb_index = curr_bb;
                           break;
                        }
                        INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                       "---Going up one level. Considering now BB" + STR(dest_bb_index));
                     }
                     if(BB_def.find(dom_bb_index) != BB_def.end())
                     {
                        INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                       "---It contains the definition of one SSA used by the statement to be moved");
                        break;
                     }
                     parent_bb = bb_dominators.getImmediateDominator(dom_bb);
                     if(parent_bb == dom_bb)
                     {
                        break;
                     }
                     dom_bb = parent_bb;
                     dom_bb_index = direct_vertex_map[dom_bb];
                     INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                    "---Considering its dominator BB" + STR(dom_bb_index));
                  }
               }
            }
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--");

            /// check the result of the dominator tree analysis
            if(dest_bb_index == curr_bb)
            {
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                              "<--Skipped (1) because destination would be the same bb");
               continue;
            }
            bool zero_delay = true;
            auto check_movable = CheckMovable(dest_bb_index, tn, zero_delay);
            if(check_movable == FunctionFrontendFlowStep_Movable::UNMOVABLE)
            {
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Skipped because cannot be moved");
               continue;
            }
            if(!AppM->ApplyNewTransformation())
            {
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                              "<--Skipped because reached limit of CFG transformations");
               continue;
            }

            /// finally we found something of meaningful
            /// check if the current uses in dest_bb_index are due only to phis
            bool only_phis = true;
            for(const auto sn : stmt_ssa_uses)
            {
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Checking definition of " + sn->ToString());
               auto def_stmt = sn->GetDefStmt();

               auto* def_gn = GetPointer<node_stmt>(def_stmt);
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Checked definition " + def_gn->ToString());
               if(def_gn->bb_index == dest_bb_index && def_gn->get_kind() != phi_stmt_K &&
                  zero_delay_stmts.find(def_stmt->index) == zero_delay_stmts.end())
               {
                  bool def_zero_delay = true;
                  CheckMovable(dest_bb_index, def_stmt, def_zero_delay);
                  if(!def_zero_delay)
                  {
                     only_phis = false;
                     INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Not a phi or zero-delay stmt");
                  }
               }

               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--");
            }
            if(only_phis && zero_delay)
            {
               zero_delay_stmts.insert((*statement)->index);
            }
            if(check_movable == FunctionFrontendFlowStep_Movable::TIMING or (!only_phis && !zero_delay))
            {
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                              "---Going down of one level because of non-zero delay");
               dest_bb_index = prev_dest_bb_index;
            }

            /// check if the statement can be really moved
            if(dest_bb_index == curr_bb)
            {
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                              "<--Skipped (2) because destination would be the same bb");
               continue;
            }
            modified = true;
            AppM->RegisterTransformation(GetName(), *statement);

            /// add predication in case is required
            if(is_store_assign || is_load_assign || is_assign_call || is_call_stmt)
            {
               if(list_of_bloc.at(dest_bb_index)->CGetStmtList().empty())
               {
                  /// it may happen: two consecutive BBs without conditional jump
               }
               else
               {
                  const auto& lastStmt = *(list_of_bloc.at(dest_bb_index)->CGetStmtList().rbegin());
                  auto lastStmtNode = lastStmt;
                  const auto scheduleNewOp = [&](const ir_nodeRef& ssa_ref) {
                     if(schedule)
                     {
                        schedule->UpdateTime(GetPointerS<const ssa_node>(ssa_ref)->GetDefStmt()->index, true);
                     }
                  };
                  const auto updatePredicate = [&](const ir_nodeRef& new_cond) {
                     THROW_ASSERT(gn->predicate,
                                  "PredicateStatements did not initialize the predicate of " + STR(*statement));
                     if(gn->predicate->get_kind() == constant_int_val_node_K)
                     {
                        const auto cond = ir_helper::GetConstValue(gn->predicate);
                        if(cond != 0)
                        {
                           TM->ReplaceIRNode(*statement, gn->predicate, new_cond);
                        }
                     }
                     else
                     {
                        auto and_cond =
                            ir_man->CreateAndExpr(new_cond, gn->predicate, list_of_bloc.at(dest_bb_index), function_id);
                        scheduleNewOp(and_cond);
                        TM->ReplaceIRNode(*statement, gn->predicate, and_cond);
                     }
                  };
                  if(lastStmtNode->get_kind() == multi_way_if_stmt_K)
                  {
                     auto gmwi = GetPointer<multi_way_if_stmt>(lastStmtNode);
                     bool found_condition = false;
                     for(const auto& gmwicond : gmwi->list_of_cond)
                     {
                        if(gmwicond.second == curr_bb)
                        {
                           found_condition = true;
                           if(!gmwicond.first)
                           {
                              /// compute default condition
                              auto firstCond = true;
                              ir_nodeRef Cur;
                              for(const auto& gmwicond0 : gmwi->list_of_cond)
                              {
                                 if(gmwicond0.first)
                                 {
                                    if(firstCond)
                                    {
                                       Cur = gmwicond0.first;
                                       firstCond = false;
                                    }
                                    else
                                    {
                                       Cur = ir_man->CreateOrExpr(Cur, gmwicond0.first, list_of_bloc.at(dest_bb_index),
                                                                  function_id);
                                       scheduleNewOp(Cur);
                                    }
                                 }
                              }
                              Cur = ir_man->CreateNotExpr(Cur, list_of_bloc.at(dest_bb_index), function_id);
                              scheduleNewOp(Cur);
                              updatePredicate(Cur);
                           }
                           else
                           {
                              updatePredicate(gmwicond.first);
                           }
                           break;
                        }
                     }
                     if(!found_condition)
                     {
                        THROW_ERROR("node not supported: " + lastStmtNode->get_kind_text() + " " +
                                    lastStmtNode->ToString());
                     }
                  }
                  else if(list_of_bloc.at(dest_bb_index)->list_of_succ.size() == 1 &&
                          list_of_bloc.at(dest_bb_index)->loop_id == list_of_bloc.at(curr_bb)->loop_id)
                  {
                     /// it may happen: two consecutive BBs without conditional jump
                  }
                  else
                  {
                     THROW_ERROR("node not supported: " + lastStmtNode->get_kind_text() + " " +
                                 lastStmtNode->ToString());
                  }
               }
            }
            const auto temp_statement = *statement;
            /// Going one step step forward to avoid invalidation of the pointer
            auto tmp_it = statement;
            ++tmp_it;
            /// Moving statement
            list_of_bloc.at(curr_bb)->RemoveStmt(temp_statement, AppM);
            if(list_of_bloc.at(curr_bb)->CGetStmtList().empty() && list_of_bloc.at(curr_bb)->CGetPhiList().empty())
            {
               restart_ifmwi_opt = true;
            }
            list_of_bloc.at(dest_bb_index)->PushBack(temp_statement, AppM);
            if(schedule)
            {
               schedule->UpdateTime(temp_statement->index, true);
            }
            /// Going one step back since pointer is already increment in for loop
            --tmp_it;
            statement = tmp_it;
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Moved in BB" + STR(dest_bb_index));
         }

         for(const auto& removing : to_be_removed)
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                           "---Removing " + removing->ToString() + " from BB" + STR(curr_bb));
            list_of_bloc.at(curr_bb)->RemoveStmt(removing, AppM);
         }
         if(!to_be_removed.empty() && list_of_bloc.at(curr_bb)->CGetStmtList().empty() &&
            list_of_bloc.at(curr_bb)->CGetPhiList().empty())
         {
            restart_ifmwi_opt = true;
         }
         for(const auto& adding_back : to_be_added_back)
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                           "---Adding back " + adding_back->ToString() + " from BB" + STR(curr_bb));
            list_of_bloc.at(curr_bb)->PushBack(adding_back, AppM);
         }
         for(const auto& adding_front : to_be_added_front)
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                           "---Adding front " + adding_front->ToString() + " from BB" + STR(curr_bb));
            list_of_bloc.at(curr_bb)->PushFront(adding_front, AppM);
         }
         restart_bb_code_motion = (!to_be_added_back.empty()) or (!to_be_added_front.empty());
         if(debug_level >= DEBUG_LEVEL_VERY_PEDANTIC &&
            (!parameters->IsParameter("print-dot-FF") || parameters->GetParameter<unsigned int>("print-dot-FF")))
         {
            bb_graph.writeDot(function_behavior->GetDotPath() / ("BB_simple_code_motion_" + STR(counter) + ".dot"));
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                           "---Written BB_simple_code_motion_" + STR(counter) + ".dot");
            counter++;
         }
         if(restart_bb_code_motion)
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Restart Analyzing BB" + STR(curr_bb));
         }
      } while(restart_bb_code_motion);
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Analyzed BB" + STR(curr_bb));
   }

   modified ? function_behavior->UpdateBBVersion() : 0;
   return modified ? DesignFlowStep_Status::SUCCESS : DesignFlowStep_Status::UNCHANGED;
}

bool simple_code_motion::IsScheduleBased() const
{
   if(schedule)
   {
      return true;
   }
   else
   {
      return false;
   }
}
