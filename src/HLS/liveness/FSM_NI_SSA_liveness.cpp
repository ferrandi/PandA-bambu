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
 * @file FSM_NI_SSA_liveness.cpp
 * @brief liveness analysis exploiting the SSA form of the IR
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 * @author Pietro Fezzardi <pietrofezzardi@gmail.com>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *
 */
#include "FSM_NI_SSA_liveness.hpp"

#include "Parameter.hpp"
#include "basic_block.hpp"
#include "behavioral_helper.hpp"
#include "dbgPrintHelper.hpp"
#include "function_behavior.hpp"
#include "hls.hpp"
#include "hls_constraints.hpp"
#include "hls_manager.hpp"
#include "liveness.hpp"
#include "memory.hpp"
#include "op_graph.hpp"
#include "state_transition_graph.hpp"
#include "state_transition_graph_manager.hpp"
#include "string_manipulation.hpp"
#include "tree_helper.hpp"
#include "tree_manager.hpp"
#include "tree_node.hpp"

#include <boost/foreach.hpp>
#include <boost/range/adaptor/reversed.hpp>

#include <algorithm>

FSM_NI_SSA_liveness::FSM_NI_SSA_liveness(const ParameterConstRef _parameters, const HLS_managerRef _HLSMgr,
                                         unsigned int _funId, const DesignFlowManagerConstRef _design_flow_manager)
    : liveness_computer(_parameters, _HLSMgr, _funId, _design_flow_manager, HLSFlowStep_Type::FSM_NI_SSA_LIVENESS)
{
   debug_level = parameters->get_class_debug_level(GET_CLASS(*this));
}

FSM_NI_SSA_liveness::~FSM_NI_SSA_liveness() = default;

void FSM_NI_SSA_liveness::Initialize()
{
   HLSFunctionStep::Initialize();
   HLS->Rliv = livenessRef(new liveness(HLSMgr, parameters));
}

static inline unsigned int get_bb_index_from_state_info(const OpGraphConstRef data, StateInfoConstRef state_info)
{
   if(state_info->executing_operations.empty())
   {
      if(state_info->ending_operations.empty())
      {
         return *state_info->BB_ids.begin(); // it may happen with pipelined operations
      }
      else
      {
         return GET_BB_INDEX(data, *(state_info->ending_operations).rbegin());
      }
   }
   else
   {
      return GET_BB_INDEX(data, *(state_info->executing_operations.rbegin()));
   }
}

static void update_liveout_with_prev(const HLS_managerRef HLSMgr, hlsRef HLS, const StateTransitionGraphConstRef stg,
                                     const OpGraphConstRef data, vertex current_state, unsigned bb_index,
                                     vertex prev_state, bool is_lp_feedback, unsigned LP_II, unsigned int funId,
                                     int
#ifndef NDEBUG
                                         debug_level
#endif
)
{
   const auto state_info = stg->CGetStateInfo(prev_state);
   for(const auto& exec_op : state_info->executing_operations)
   {
      auto isAPhi = GET_TYPE(data, exec_op) & TYPE_PHI;
      auto manage_scalar_use = [&](unsigned scalar_use) {
         if(HLSMgr->is_register_compatible(scalar_use))
         {
            THROW_ASSERT(HLS->Rliv->has_op_where_defined(scalar_use), "unexpected");
            auto def_op = HLS->Rliv->get_op_where_defined(scalar_use);
            auto not_have_def = std::find(state_info->ending_operations.begin(), state_info->ending_operations.end(),
                                          def_op) == state_info->ending_operations.end();
            if(isAPhi || not_have_def || HLS->STG->not_same_step(prev_state, def_op, exec_op))
            {
               unsigned int step = isAPhi ? HLS->Rliv->GetStepPhiOut(exec_op, scalar_use) :
                                            HLS->Rliv->GetStep(prev_state, exec_op, scalar_use, true);
               bool repeat = false;
               do
               {
                  INDENT_DBG_MEX(
                      DEBUG_LEVEL_PEDANTIC, debug_level,
                      HLSMgr->CGetFunctionBehavior(funId)->CGetBehavioralHelper()->PrintVariable(scalar_use) + "-" +
                          STR(step));
                  HLS->Rliv->set_live_out(current_state, scalar_use, step);
                  repeat = is_lp_feedback && step > LP_II;
                  if(repeat)
                  {
                     auto prev_pair = HLS->Rliv->GetPrevStep(bb_index, scalar_use, step, LP_II);
                     if(!prev_pair.first)
                     {
                        repeat = false;
                     }
                     else
                     {
                        if(prev_pair.second != step)
                        {
                           step = prev_pair.second;
                        }
                        else
                        {
                           repeat = false;
                        }
                     }
                  }
               } while(repeat);
            }
         }
         else if(HLSMgr->CGetFunctionBehavior(funId)->is_function_pipelined() &&
                 tree_helper::is_parameter(HLSMgr->get_tree_manager(), scalar_use))
         {
            auto parStep = HLS->Rliv->GetStepOp(prev_state, exec_op);
            if(parStep)
            {
               std::pair<bool, unsigned> prev_pair;
               do
               {
                  INDENT_DBG_MEX(
                      DEBUG_LEVEL_PEDANTIC, debug_level,
                      HLSMgr->CGetFunctionBehavior(funId)->CGetBehavioralHelper()->PrintVariable(scalar_use) + "-" +
                          STR(parStep));
                  HLS->Rliv->set_live_out(current_state, scalar_use, parStep);
                  prev_pair = HLS->Rliv->GetPrevStep(2, scalar_use, parStep, 1);
                  if(prev_pair.second && parStep == prev_pair.second)
                  {
                     break;
                  }
                  parStep = prev_pair.second;
               } while(prev_pair.first);
            }
         }
      };
      if(isAPhi)
      {
         const auto curr_state_info = stg->CGetStateInfo(current_state);
         const auto phi_node =
             HLSMgr->get_tree_manager()->get_tree_node_const(data->CGetOpNodeInfo(exec_op)->GetNodeId());
         for(const auto& def_edge : GetPointer<const gimple_phi>(phi_node)->CGetDefEdgesList())
         {
            auto phi_in = def_edge.first->index;
            if((curr_state_info->is_prologue.count(exec_op) && bb_index != def_edge.second) ||
               (!curr_state_info->is_prologue.count(exec_op) && bb_index == def_edge.second))
            {
               manage_scalar_use(phi_in);
            }
         }
      }
      else
      {
         const CustomSet<unsigned int>& scalar_uses = data->CGetOpNodeInfo(exec_op)->GetVariables(
             FunctionBehavior_VariableType::SCALAR, FunctionBehavior_VariableAccessType::USE);

         for(const auto scalar_use : scalar_uses)
         {
            manage_scalar_use(scalar_use);
         }
      }
   }
   for(const auto& end_op : state_info->ending_operations)
   {
      const CustomSet<unsigned int>& scalar_defs = data->CGetOpNodeInfo(end_op)->GetVariables(
          FunctionBehavior_VariableType::SCALAR, FunctionBehavior_VariableAccessType::DEFINITION);

      for(const auto scalar_def : scalar_defs)
      {
         if(HLSMgr->is_register_compatible(scalar_def))
         {
            unsigned int step = HLS->Rliv->GetStep(prev_state, end_op, scalar_def, false);
            INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level,
                           "erase " +
                               HLSMgr->CGetFunctionBehavior(funId)->CGetBehavioralHelper()->PrintVariable(scalar_def) +
                               "-" + STR(step));
            HLS->Rliv->erase_el_live_out(current_state, scalar_def, step);
         }
      }
   }

   if(not state_info->moved_op_use_set.empty())
   {
      for(auto v : stg->CGetStateInfo(prev_state)->moved_op_use_set)
      {
         HLS->Rliv->set_live_out(current_state, v, 0);
      }
   }
}

DesignFlowStep_Status FSM_NI_SSA_liveness::InternalExec()
{
   const StateTransitionGraphConstRef astg = HLS->STG->CGetAstg();
   const StateTransitionGraphConstRef stg = HLS->STG->CGetStg();
   const FunctionBehaviorConstRef FB = HLSMgr->CGetFunctionBehavior(funId);
   const BBGraphConstRef fbb = FB->CGetBBGraph(FunctionBehavior::FBB);
   const OpGraphConstRef data = FB->CGetOpGraph(FunctionBehavior::DFG);
   auto is_function_pipelined = FB->is_function_pipelined();
   auto has_pipelined_states = is_function_pipelined;

   /// Map between basic block node index and vertices
   CustomUnorderedMap<unsigned int, vertex> bb_index_map;
   BOOST_FOREACH(vertex v, boost::vertices(*fbb))
   {
      bb_index_map[fbb->CGetBBNodeInfo(v)->get_bb_index()] = v;
   }

   /*
    * vertex of the STG analyzed in reverse order. N.B.: the reversed list is
    * taken from the astg (Acyclic State Transition Graph), so this may affect
    * the position of certain states in the list, depending on the presence of
    * feedback edges
    */
   std::deque<vertex> reverse_order_state_list;
   astg->ReverseTopologicalSort(reverse_order_state_list);
   vertex exit_state = HLS->STG->get_exit_state();
   std::deque<vertex> dummy_states;

   /// compute which operation defines a variable (add_op_definition)
   /// compute in which states an operation complete the computation (add_state_for_ending_op)
   /// compute the set of supporting states
   /// compute the set of dummy states
   INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "-->Computing preliminary information for liveness computation");
   for(const auto& rosl : reverse_order_state_list)
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "-->Setting up state " + HLS->STG->get_state_name(rosl));
      HLS->Rliv->add_name(rosl, HLS->STG->get_state_name(rosl));
      HLS->Rliv->add_support_state(rosl);
      const StateInfoConstRef state_info = stg->CGetStateInfo(rosl);
      unsigned int bb_index = get_bb_index_from_state_info(data, state_info);
      HLS->Rliv->set_max_step(bb_index, state_info->is_pipelined_state ?
                                            stg->CGetStateTransitionGraphInfo()->vertex_to_max_step.at(rosl) :
                                            0);
      for(auto op : state_info->ending_operations)
      {
         HLS->Rliv->set_BB(op, GET_BB_INDEX(data, op));
         if(GET_TYPE(data, op) & TYPE_PHI)
         {
            HLS->Rliv->add_phi_vertices(op);
         }
         THROW_ASSERT((GET_TYPE(data, op) & TYPE_VPHI) == 0, "unexpected condition");
      }
      for(auto op : state_info->executing_operations)
      {
         HLS->Rliv->set_BB(op, GET_BB_INDEX(data, op));
         HLS->Rliv->set_II(GET_BB_INDEX(data, op), state_info->LP_II);
         THROW_ASSERT((GET_TYPE(data, op) & TYPE_VPHI) == 0, "unexpected condition");
      }
      // compute which operation defines a variable (add_op_definition)
      // compute in which states an operation complete the computation (add_state_for_ending_op)
      for(const auto& eoc : state_info->ending_operations) // these also include the moved ending operations
      {
         const CustomSet<unsigned int>& scalar_defs = data->CGetOpNodeInfo(eoc)->GetVariables(
             FunctionBehavior_VariableType::SCALAR, FunctionBehavior_VariableAccessType::DEFINITION);

         for(const auto scalar_def : scalar_defs)
         {
            if(HLSMgr->is_register_compatible(scalar_def))
            {
               HLS->Rliv->add_op_definition(scalar_def, eoc);
            }
         }

         HLS->Rliv->add_state_for_ending_op(eoc, rosl);
      }

      // add dummy state
      if(state_info->is_dummy)
      {
         dummy_states.push_back(rosl);
         HLS->Rliv->add_dummy_state(rosl);
      }

      // add pipelined state
      if(state_info->is_pipelined_state)
      {
         has_pipelined_states = true;
         for(auto op : state_info->step_in)
         {
            HLS->Rliv->set_step(rosl, op.first, op.second, true);
         }
         for(auto op : state_info->step_out)
         {
            HLS->Rliv->set_step(rosl, op.first, op.second, false);
            HLS->Rliv->add_op_step(op.first, op.second);
         }
         INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "<--Pipelined state");
         continue;
      }

      if(state_info->is_dummy)
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "<--Is a dummy state");
         continue;
      }

      /*
       * handle states that contain moved operations coming from other states
       * (possibly coming from other BBs as well)
       */
      if(not state_info->moved_op_use_set.empty())
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "-->Contains moved operations");
#ifndef NDEBUG
         const BehavioralHelperConstRef BH = FB->CGetBehavioralHelper();
         if(debug_level >= DEBUG_LEVEL_PEDANTIC)
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "---Used by moved operations:");
            for(const auto& var_id : state_info->moved_op_use_set)
            {
               INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "--- * " + BH->PrintVariable(var_id));
            }
         }
#endif
         /*
          * if rosl contains a moved operation, then the use set of the moved
          * operation must be in the live in of the state
          */
         INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level,
                        "Adding to the live in of state:" + HLS->STG->get_state_name(rosl));
         for(const auto v : state_info->moved_op_use_set)
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level,
                           "---" + FB->CGetBehavioralHelper()->PrintVariable(v) + "-m-" + STR(0));
            HLS->Rliv->set_live_in(rosl, v, 0);
         }
         /*
          * if rosl contains a moved operation, we have to add the used set to
          * the live out of the predecessors of rosl
          */
         BOOST_FOREACH(EdgeDescriptor e, boost::in_edges(rosl, *stg))
         {
            vertex src_state = boost::source(e, *stg);
            INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level,
                           "Adding to the live out of predecessor state:" + HLS->STG->get_state_name(src_state));
            for(const auto v : state_info->moved_op_use_set)
            {
               HLS->Rliv->set_live_out(src_state, v, 0);
            }
         }
         INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "<--");
      }
      INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "<--Set up state " + HLS->STG->get_state_name(rosl));
   }
   INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "<--Computed preliminary information for liveness computation");

   /// compute live_out
   INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "-->Computing live_out");
   const vertex bb_exit = fbb->CGetBBGraphInfo()->exit_vertex;
   unsigned int prev_bb_index = fbb->CGetBBNodeInfo(bb_exit)->get_bb_index();
   vertex prev_state = exit_state;
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                  "---prev_state: " + stg->CGetStateInfo(prev_state)->name + " prev_bb_index: " + STR(prev_bb_index));
   for(auto ind = (has_pipelined_states ? 0 : 1); ind < 2; ++ind)
   {
      for(const auto& rosl : reverse_order_state_list)
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level,
                        "-->Computing live out for state " + HLS->STG->get_state_name(rosl));
         // skip exit state
         if(rosl == exit_state && !is_function_pipelined)
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "<--Is an exit state");
            continue;
         }

         StateInfoConstRef state_info = stg->CGetStateInfo(rosl);
         // skip dummy states
         if(state_info->is_dummy)
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "<--Is a dummy state");
            continue;
         }

         unsigned int bb_index = get_bb_index_from_state_info(data, state_info);
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---bb_index: " + STR(bb_index));
         if(state_info->is_pipelined_state)
         {
            OutEdgeIterator o_e_it, o_e_end;
            for(boost::tie(o_e_it, o_e_end) = boost::out_edges(rosl, *stg); o_e_it != o_e_end; ++o_e_it)
            {
               vertex target_state = boost::target(*o_e_it, *stg);
               prev_state = target_state;
               StateInfoConstRef tgt_state_info = stg->CGetStateInfo(target_state);
               prev_bb_index = get_bb_index_from_state_info(data, tgt_state_info);
               if(prev_bb_index != bb_index)
               {
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                 "---prev_bb_index " + STR(prev_bb_index) + " != bb_index " + STR(bb_index));
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                 "---adding live out of BB " + STR(bb_index) + " to live out of state " +
                                     state_info->name);
                  for(const auto& lo : fbb->CGetBBNodeInfo(bb_index_map[bb_index])->get_live_out())
                  {
                     if(HLSMgr->is_register_compatible(lo))
                     {
                        auto step = HLS->Rliv->GetStepOut(lo);
                        INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                       "---" + FB->CGetBehavioralHelper()->PrintVariable(lo) + "-" + STR(step));
                        HLS->Rliv->set_live_out(rosl, lo, step);
                     }
                  }
               }
               else
               {
                  bool is_feedback = stg->GetSelector(*o_e_it) & TransitionInfo::StateTransitionType::ST_EDGE_FEEDBACK;
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                 "---adding live out of " + stg->CGetStateInfo(target_state)->name +
                                     " to live out of state " + state_info->name);
                  const auto prev_live_out = HLS->Rliv->get_live_out(rosl);
                  if(target_state != rosl)
                  {
                     for(const auto& lo : HLS->Rliv->get_live_out(target_state))
                     {
                        auto pre_pair = HLS->Rliv->GetPrevStep(bb_index, lo.first, lo.second, 1);
                        // std::cerr << "checking " << FB->CGetBehavioralHelper()->PrintVariable(lo.first) << "\n";
                        if(pre_pair.first)
                        {
                           INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                          "---" + FB->CGetBehavioralHelper()->PrintVariable(lo.first) + "-" +
                                              STR(pre_pair.second));
                           HLS->Rliv->set_live_out(rosl, lo.first, pre_pair.second);
                        }
                        else if(state_info->is_last_state)
                        {
                           if(!tree_helper::is_parameter(HLSMgr->get_tree_manager(), lo.first) &&
                              std::find(
                                  tgt_state_info->ending_operations.begin(), tgt_state_info->ending_operations.end(),
                                  HLS->Rliv->get_op_where_defined(lo.first)) == tgt_state_info->ending_operations.end())
                           {
                              INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "is_last_state");
                              INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                             "---" + FB->CGetBehavioralHelper()->PrintVariable(lo.first) + "-" +
                                                 STR(pre_pair.second));
                              HLS->Rliv->set_live_out(rosl, lo.first, pre_pair.second);
                           }
                        }
                     }
                  }
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "update_liveout_with_prev");
                  update_liveout_with_prev(HLSMgr, HLS, stg, data, rosl, bb_index, target_state, is_feedback,
                                           state_info->LP_II, funId, debug_level);
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "add back initial liveout");
                  for(const auto& lo : prev_live_out)
                  {
                     INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                    "---" + FB->CGetBehavioralHelper()->PrintVariable(lo.first) + "-" + STR(lo.second));
                     HLS->Rliv->set_live_out(rosl, lo.first, lo.second);
                  }
               }
            }
         }
         // handle duplicated states
         else if(state_info->is_duplicated)
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "Is duplicated state");
            if(state_info->isOriginalState)
            {
               INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "---Is the original copy");
               StateInfoConstRef cloned_state_info = stg->CGetStateInfo(rosl);
               unsigned int cloned_bb_index = *cloned_state_info->BB_ids.begin();
               prev_state = rosl;
               prev_bb_index = cloned_bb_index;
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                              "---update prev_state: " + stg->CGetStateInfo(prev_state)->name +
                                  " prev_bb_index: " + STR(prev_bb_index));
            }
            else
            {
               INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "---Is the cloned copy");
               THROW_ASSERT(state_info->clonedState != NULL_VERTEX, "");
               StateInfoConstRef cloned_state_info = stg->CGetStateInfo(state_info->clonedState);
               unsigned int cloned_bb_index = *cloned_state_info->BB_ids.begin();
               bool found = false;
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---cloned_bb_index: " + STR(cloned_bb_index));
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                              "-->Considering successors of: " + cloned_state_info->name);
               BOOST_FOREACH(EdgeDescriptor oe, boost::out_edges(state_info->clonedState, *astg))
               {
                  vertex target_state = boost::target(oe, *astg);
                  StateInfoConstRef target_state_info = astg->CGetStateInfo(target_state);
                  unsigned int target_bb_index = *target_state_info->BB_ids.begin();
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "successor: " + target_state_info->name);
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "target_bb_index: " + STR(target_bb_index));
                  if(cloned_bb_index == target_bb_index and target_state != rosl)
                  {
                     INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                    "---updating live out of : " + state_info->name + ", " + cloned_state_info->name);
                     INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                    "---adding live out of : " + target_state_info->name);
                     HLS->Rliv->set_live_out(rosl, HLS->Rliv->get_live_out(target_state));
                     HLS->Rliv->set_live_out(state_info->clonedState, HLS->Rliv->get_live_out(target_state));
                     update_liveout_with_prev(HLSMgr, HLS, stg, data, state_info->clonedState, cloned_bb_index,
                                              target_state, false, 0, funId, debug_level);
                     update_liveout_with_prev(HLSMgr, HLS, stg, data, rosl, bb_index, target_state, false, 0, funId,
                                              debug_level);
                     found = true;
                  }
               }
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                              "<--Considered successors of: " + cloned_state_info->name);

               if(not found)
               {
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "no valid successors found");
                  for(const auto lo : fbb->CGetBBNodeInfo(bb_index_map[cloned_bb_index])->get_live_out())
                  {
                     if(HLSMgr->is_register_compatible(lo))
                     {
#ifndef NDEBUG
                        const BehavioralHelperConstRef BH = FB->CGetBehavioralHelper();
                        INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                       "---adding to live out of " + state_info->name + " var " +
                                           BH->PrintVariable(lo));
                        INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                       "---adding to live out of " + cloned_state_info->name + " var " +
                                           BH->PrintVariable(lo));
#endif
                        HLS->Rliv->set_live_out(rosl, lo, 0);
                        HLS->Rliv->set_live_out(state_info->clonedState, lo, 0);
                     }
                  }
               }
               prev_state = rosl;
               prev_bb_index = cloned_bb_index;
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                              "---update prev_state: " + stg->CGetStateInfo(prev_state)->name +
                                  " prev_bb_index: " + STR(prev_bb_index));
            }
            INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "<--");
            continue;
         }
         else if(prev_bb_index != bb_index)
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                           "---prev_bb_index " + STR(prev_bb_index) + " != bb_index " + STR(bb_index));
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                           "---adding live out of BB " + STR(bb_index) + " to live out of state " + state_info->name);
            for(const auto lo : fbb->CGetBBNodeInfo(bb_index_map[bb_index])->get_live_out())
            {
               if(HLSMgr->is_register_compatible(lo))
               {
                  auto step = HLS->Rliv->GetStepOut(lo);
                  INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level,
                                 "---" + FB->CGetBehavioralHelper()->PrintVariable(lo) + "-" + STR(step));
                  HLS->Rliv->set_live_out(rosl, lo, step);
               }
            }
         }
         else
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                           "---adding live out of " + stg->CGetStateInfo(prev_state)->name + " to live out of state " +
                               state_info->name);
            for(const auto& lo : HLS->Rliv->get_live_out(prev_state))
            {
               auto pre_pair = HLS->Rliv->GetPrevStep(bb_index, lo.first, lo.second, 1);
               if(pre_pair.first)
               {
                  const BehavioralHelperConstRef BH = FB->CGetBehavioralHelper();
                  INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level,
                                 BH->PrintVariable(lo.first) + "-" + STR(pre_pair.second));
                  HLS->Rliv->set_live_out(rosl, lo.first, pre_pair.second);
               }
            }
            update_liveout_with_prev(HLSMgr, HLS, stg, data, rosl, bb_index, prev_state, false, 0, funId, debug_level);
         }
         prev_state = rosl;
         prev_bb_index = bb_index;
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                        "---update prev_state: " + stg->CGetStateInfo(prev_state)->name +
                            " prev_bb_index: " + STR(prev_bb_index));
         INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level,
                        "<--Computed live out for state " + HLS->STG->get_state_name(rosl));
      }
   }
   INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "<--Computed live out");

   /// compute the live in of a state by traversing the state list in topological order
   INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "-->Computing live in");
   vertex entry_state = HLS->STG->get_entry_state();
   prev_state = entry_state;
   CustomOrderedSet<vertex> state_to_skip;
   for(const auto& osl : boost::adaptors::reverse(reverse_order_state_list))
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level,
                     "---Computing live in for state " + HLS->STG->get_state_name(osl));
      const StateInfoConstRef state_info = stg->CGetStateInfo(osl);
      if(state_to_skip.find(osl) != state_to_skip.end())
      {
         prev_state = osl;
         prev_bb_index = *state_info->BB_ids.begin();
         continue;
      }

      // skip exit state
      if(osl == exit_state && !is_function_pipelined)
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "---Is an exit state");
         continue;
      }
      if(state_info->is_dummy)
      {
         continue;
      }

      unsigned int bb_index = get_bb_index_from_state_info(data, state_info);
      const auto live_in_bb_index = fbb->CGetBBNodeInfo(bb_index_map[bb_index])->get_live_in();
      if(state_info->is_pipelined_state)
      {
         InEdgeIterator i_e_it, i_e_end;
         for(boost::tie(i_e_it, i_e_end) = boost::in_edges(osl, *stg); i_e_it != i_e_end; ++i_e_it)
         {
            auto src_state = boost::source(*i_e_it, *stg);
            auto src_state_info = stg->CGetStateInfo(src_state);
            prev_bb_index = get_bb_index_from_state_info(data, src_state_info);
            auto adjust_phi = [&] {
               for(const auto& exec_op : state_info->executing_operations)
               {
                  if((GET_TYPE(data, exec_op) & TYPE_PHI) != 0)
                  {
                     const auto phi_node =
                         HLSMgr->get_tree_manager()->get_tree_node_const(data->CGetOpNodeInfo(exec_op)->GetNodeId());
                     for(const auto& def_edge : GetPointer<const gimple_phi>(phi_node)->CGetDefEdgesList())
                     {
                        auto phi_in = def_edge.first->index;
                        if(HLSMgr->is_register_compatible(phi_in) && !live_in_bb_index.contains(phi_in))
                        {
                           unsigned int step = HLS->Rliv->GetStepPhiOut(exec_op, phi_in);
                           INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level,
                                          "---erase phi in " + FB->CGetBehavioralHelper()->PrintVariable(phi_in) + "-" +
                                              STR(step));
                           HLS->Rliv->erase_el_live_in(osl, phi_in, step);
                        }
                     }
                     auto phi_res = GetPointer<const gimple_phi>(phi_node)->res->index;
                     unsigned int step = HLS->Rliv->GetStep(osl, exec_op, phi_res, false);
                     INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level,
                                    "---" + FB->CGetBehavioralHelper()->PrintVariable(phi_res) + "-" + STR(step));
                     HLS->Rliv->set_live_in(osl, phi_res, step);
                  }
               }
            };
            if(prev_bb_index != bb_index)
            {
               for(const auto li : fbb->CGetBBNodeInfo(bb_index_map[prev_bb_index])->get_live_out())
               {
                  if(HLSMgr->is_register_compatible(li))
                  {
                     auto step_pair = HLS->Rliv->GetStepIn(bb_index, li, osl);
                     if(step_pair.first)
                     {
                        INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level,
                                       "---" + FB->CGetBehavioralHelper()->PrintVariable(li) + "--" +
                                           STR(step_pair.second));
                        HLS->Rliv->set_live_in(osl, li, step_pair.second);
                     }
                  }
               }
               adjust_phi();
            }
            else
            {
               for(const auto& li : HLS->Rliv->get_live_out(src_state))
               {
                  INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level,
                                 "---" + FB->CGetBehavioralHelper()->PrintVariable(li.first) + "-" + STR(li.second));
                  HLS->Rliv->set_live_in(osl, li.first, li.second);
               }
               adjust_phi();
               for(const auto& li : HLS->Rliv->get_live_out(osl))
               {
                  auto pre_pair = HLS->Rliv->GetPrevStep(bb_index, li.first, li.second, 1);
                  if(pre_pair.first)
                  {
                     INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level,
                                    "---" + FB->CGetBehavioralHelper()->PrintVariable(li.first) + "-" +
                                        STR(pre_pair.second));
                     HLS->Rliv->set_live_in(osl, li.first, pre_pair.second);
                  }
               }
            }
         }
      }
      else if(state_info->is_duplicated)
      {
         auto l_bb_index = *state_info->BB_ids.begin();
         for(const auto li : fbb->CGetBBNodeInfo(bb_index_map[l_bb_index])->get_live_in())
         {
            if(HLSMgr->is_register_compatible(li))
            {
               INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level,
                              "---" + FB->CGetBehavioralHelper()->PrintVariable(li) + "-d-" + STR(0));
               HLS->Rliv->set_live_in(osl, li, 0);
            }
         }

         BOOST_FOREACH(EdgeDescriptor oe, boost::out_edges(osl, *astg))
         {
            vertex target_state = boost::target(oe, *astg);
            StateInfoConstRef target_state_info = astg->CGetStateInfo(target_state);
            unsigned int target_bb_index = *target_state_info->BB_ids.begin();
            if(l_bb_index == target_bb_index && !target_state_info->is_duplicated)
            {
               for(const auto& lo : HLS->Rliv->get_live_out(osl))
               {
                  INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level,
                                 "---" + FB->CGetBehavioralHelper()->PrintVariable(lo.first) + "-d-" + STR(lo.second));
                  HLS->Rliv->set_live_in(target_state, lo.first, lo.second);
               }
               state_to_skip.insert(target_state);
            }
         }
         prev_state = osl;
         prev_bb_index = l_bb_index;
         continue;
      }
      else if(prev_bb_index != bb_index)
      {
         for(const auto li : live_in_bb_index)
         {
            if(HLSMgr->is_register_compatible(li))
            {
               auto step_pair = HLS->Rliv->GetStepIn(bb_index, li, osl);
               if(step_pair.first)
               {
                  INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level,
                                 "---" + FB->CGetBehavioralHelper()->PrintVariable(li) + "--" + STR(step_pair.second));
                  HLS->Rliv->set_live_in(osl, li, step_pair.second);
               }
            }
         }
      }
      else
      {
         for(const auto& lo : HLS->Rliv->get_live_out(prev_state))
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level,
                           "---" + FB->CGetBehavioralHelper()->PrintVariable(lo.first) + "--" + STR(lo.second));
            HLS->Rliv->set_live_in(osl, lo.first, lo.second);
         }
      }
      prev_state = osl;
      prev_bb_index = bb_index;
   }

   INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "<--Computed live in");

   /// fix the live in/out of dummy states
   INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "-->Adjusting liveness of dummy states");
   for(const auto& ds : dummy_states)
   {
      const StateInfoConstRef state_info = astg->CGetStateInfo(ds);
      for(const auto& eo : state_info->executing_operations)
      {
         const CustomSet<unsigned int>& scalar_defs = data->CGetOpNodeInfo(eo)->GetVariables(
             FunctionBehavior_VariableType::SCALAR, FunctionBehavior_VariableAccessType::DEFINITION);
         for(const auto scalar_def : scalar_defs)
         {
            if(HLSMgr->is_register_compatible(scalar_def))
            {
               unsigned int step = HLS->Rliv->GetStepWrite(ds, eo);
               HLS->Rliv->set_live_out(ds, scalar_def, step);
            }
         }
      }
      BOOST_FOREACH(EdgeDescriptor e, boost::in_edges(ds, *astg))
      {
         vertex src_state = boost::source(e, *astg);
         // HLS->Rliv->set_live_out(ds, HLS->Rliv->get_live_out(src_state));

         for(const auto& li : HLS->Rliv->get_live_out(src_state))
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level,
                           "---" + FB->CGetBehavioralHelper()->PrintVariable(li.first) + "-d-" + STR(li.second));
            HLS->Rliv->set_live_in(ds, li.first, li.second);
         }
         /// add all the uses of ds to src_state
         for(const auto& eo : state_info->executing_operations)
         {
            const CustomSet<unsigned int>& scalar_uses = data->CGetOpNodeInfo(eo)->GetVariables(
                FunctionBehavior_VariableType::SCALAR, FunctionBehavior_VariableAccessType::USE);
            for(const auto scalar_use : scalar_uses)
            {
               if(HLSMgr->is_register_compatible(scalar_use))
               {
                  auto step = HLS->Rliv->GetStep(ds, eo, scalar_use, true);
                  HLS->Rliv->add_state_out_for_var(scalar_use, HLS->Rliv->get_op_where_defined(scalar_use), src_state,
                                                   ds);
                  INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level,
                                 "---" + FB->CGetBehavioralHelper()->PrintVariable(scalar_use) + "-a-" + STR(step));
                  HLS->Rliv->set_live_in(ds, scalar_use, step);
                  HLS->Rliv->set_live_out(src_state, scalar_use, step);
                  /// extend the lifetime of used variable to reduce the critical path
                  BOOST_FOREACH(EdgeDescriptor oe, boost::out_edges(ds, *astg))
                  {
                     vertex target_state = boost::target(oe, *astg);
                     const StateInfoConstRef tgt_state_info = astg->CGetStateInfo(target_state);
                     INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level,
                                    "---" + FB->CGetBehavioralHelper()->PrintVariable(scalar_use) + "-a-" +
                                        STR(tgt_state_info->is_pipelined_state ? step : 0));
                     HLS->Rliv->set_live_in(target_state, scalar_use, tgt_state_info->is_pipelined_state ? step : 0);
                  }
               }
            }
         }
      }
   }
   INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "<--Adjusted liveness of dummy states");

   /// compute in which state an operation is in execution
   /// compute state in relation: on which transition a variable is live in
   /// compute state out relation: on which transition a variable is live out
   for(const auto& rosl : reverse_order_state_list)
   {
      const StateInfoConstRef state_info = stg->CGetStateInfo(rosl);
      INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "-->Analyzing state " + state_info->name);
      for(const auto& roc : state_info->executing_operations) // these also include the moved execution operations
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "-->Analyzing running operation " + GET_NAME(data, roc));
         HLS->Rliv->add_state_for_running_op(roc, rosl);
         if((GET_TYPE(data, roc) & TYPE_NOP) != 0)
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level,
                           "<--Analyzed running operation of type NOP" + GET_NAME(data, roc));
            continue;
         }
         if((GET_TYPE(data, roc) & TYPE_PHI) != 0)
         {
            const auto phi_node =
                HLSMgr->get_tree_manager()->get_tree_node_const(data->CGetOpNodeInfo(roc)->GetNodeId());
            for(const auto& def_edge : GetPointer<const gimple_phi>(phi_node)->CGetDefEdgesList())
            {
               unsigned int tree_var = def_edge.first->index;
               unsigned int bb_index = def_edge.second;
               if(state_info->is_duplicated && !state_info->all_paths)
               {
                  if(!state_info->isOriginalState && bb_index != state_info->sourceBb)
                  {
                     continue;
                  }
                  if(state_info->isOriginalState && bb_index == state_info->sourceBb)
                  {
                     continue;
                  }
                  unsigned int written_phi = HLSMgr->get_produced_value(HLS->functionId, roc);
                  if(state_info->moved_op_def_set.find(tree_var) != state_info->moved_op_def_set.end() or
                     state_info->moved_op_use_set.find(written_phi) != state_info->moved_op_use_set.end())
                  {
                     HLS->Rliv->set_live_out(rosl, HLS->Rliv->get_live_out(stg->CGetStateInfo(rosl)->clonedState));

                     if(state_info->moved_op_use_set.find(written_phi) != state_info->moved_op_use_set.end())
                     {
                        INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level,
                                       "---" + FB->CGetBehavioralHelper()->PrintVariable(tree_var) + "-m-" + STR(0));
                        HLS->Rliv->set_live_in(rosl, tree_var, 0);
                        BOOST_FOREACH(EdgeDescriptor ie, boost::in_edges(rosl, *stg))
                        {
                           vertex src_vrtx = boost::source(ie, *stg);
                           HLS->Rliv->set_live_out(src_vrtx, tree_var, 0);
                        }
                     }
                     HLS->Rliv->set_live_out(rosl, written_phi, 0);
                     BOOST_FOREACH(EdgeDescriptor e, boost::out_edges(rosl, *stg))
                     {
                        vertex tgt_vrtx = boost::target(e, *stg);
                        INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level,
                                       "---" + FB->CGetBehavioralHelper()->PrintVariable(written_phi) + "-m-" + STR(0));
                        HLS->Rliv->set_live_in(tgt_vrtx, written_phi, 0);
                     }
                  }
               }
               /// now we look for the last state with operations belonging to basic block bb_index
#if HAVE_ASSERTS
               bool found_state = false;
#endif

               BOOST_FOREACH(EdgeDescriptor ie, boost::in_edges(rosl, *stg))
               {
                  vertex src_state = boost::source(ie, *stg);
                  const StateInfoConstRef source_state_info = stg->CGetStateInfo(src_state);
                  INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level,
                                 "---rosl " + state_info->name + " src " + source_state_info->name + " " +
                                     (source_state_info->is_prologue.count(roc) ? "T" : "F") + " " +
                                     FB->CGetBehavioralHelper()->PrintVariable(tree_var));
                  const CustomOrderedSet<unsigned int>& BB_ids = source_state_info->BB_ids;
                  auto same_bb = BB_ids.find(bb_index) != BB_ids.end();
                  if(((!source_state_info->is_pipelined_state || !state_info->is_pipelined_state) && same_bb) ||
                     ((state_info->is_pipelined_state && source_state_info->is_pipelined_state && same_bb &&
                       !source_state_info->is_prologue.count(roc)) ||
                      (state_info->is_pipelined_state && source_state_info->is_pipelined_state && !same_bb &&
                       source_state_info->is_prologue.count(roc))))
                  {
                     // THROW_ASSERT((HLS->Rliv->get_live_out(src_state).find(tree_var) !=
                     // HLS->Rliv->get_live_out(src_state).end()), "unexpected live out condition");
                     THROW_ASSERT(src_state != entry_state,
                                  "Source state for phi " + STR(data->CGetOpNodeInfo(roc)->GetNodeId()) + " not found");
                     INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level,
                                    "---Adding state in " + source_state_info->name + " " +
                                        FB->CGetBehavioralHelper()->PrintVariable(tree_var) + " in state " +
                                        state_info->name);
                     HLS->Rliv->add_state_in_for_var(tree_var, roc, rosl, src_state);
#if HAVE_ASSERTS
                     found_state = true;
#endif
                  }
               }
               THROW_ASSERT(found_state || state_info->is_pipelined_state,
                            "Not found source for phi " + GET_NAME(data, roc) + " in state " + state_info->name +
                                " coming from BB" + STR(bb_index));
            }
         }
         else
         {
            BOOST_FOREACH(EdgeDescriptor oe, boost::out_edges(rosl, *stg))
            {
               vertex tgt_state = boost::target(oe, *stg);
               const auto& scalar_defs = data->CGetOpNodeInfo(roc)->GetVariables(
                   FunctionBehavior_VariableType::SCALAR, FunctionBehavior_VariableAccessType::DEFINITION);
               for(const auto def : data->CGetOpNodeInfo(roc)->cited_variables)
               {
                  if(scalar_defs.count(def))
                  {
                     continue;
                  }
                  if(HLSMgr->is_register_compatible(def))
                  {
                     HLS->Rliv->add_state_in_for_var(def, roc, rosl, tgt_state);
                  }
                  else if(HLSMgr->Rmem->has_base_address(def) ||
                          tree_helper::is_parameter(HLSMgr->get_tree_manager(), def))
                  {
                     HLS->Rliv->add_state_in_for_var(def, roc, rosl, tgt_state);
                  }
                  else if(tree_helper::is_ssa_name(HLSMgr->get_tree_manager(), def) &&
                          tree_helper::is_virtual(HLSMgr->get_tree_manager(), def))
                  {
                     ;
                  }
                  else
                  {
                     THROW_ERROR("unexpected condition " + STR(def));
                  }
               }
            }
         }
         INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "<--Analyzed running operation " + GET_NAME(data, roc));
      }
      for(const auto& eoc : state_info->ending_operations) // these also include the moved ending operations
      {
         if((GET_TYPE(data, eoc) & TYPE_PHI) != 0)
         {
            if(state_info->is_duplicated && state_info->clonedState != NULL_VERTEX && !state_info->all_paths)
            {
               unsigned int written_phi = HLSMgr->get_produced_value(HLS->functionId, eoc);
               bool postponed_phi = false;
               const auto phi_node =
                   HLSMgr->get_tree_manager()->get_tree_node_const(data->CGetOpNodeInfo(eoc)->GetNodeId());
               for(const auto& def_edge : GetPointer<const gimple_phi>(phi_node)->CGetDefEdgesList())
               {
                  unsigned int tree_temp = def_edge.first->index;
                  unsigned int bbID = def_edge.second;
                  if(bbID != state_info->sourceBb)
                  {
                     continue;
                  }
                  else if(state_info->moved_op_def_set.find(tree_temp) != state_info->moved_op_def_set.end())
                  {
                     postponed_phi = true;
                  }
                  else if(state_info->moved_op_use_set.find(written_phi) != state_info->moved_op_use_set.end())
                  {
                     postponed_phi = true;
                  }
                  break;
               }
               if(not postponed_phi)
               {
                  continue;
               }
            }
            else
            {
               continue;
            }
         }

         const CustomSet<unsigned int>& scalar_defs = data->CGetOpNodeInfo(eoc)->GetVariables(
             FunctionBehavior_VariableType::SCALAR, FunctionBehavior_VariableAccessType::DEFINITION);
         for(const auto scalar_def : scalar_defs)
         {
            if(HLSMgr->is_register_compatible(scalar_def))
            {
               BOOST_FOREACH(EdgeDescriptor e, boost::out_edges(rosl, *stg))
               {
                  unsigned int step = HLS->Rliv->GetStepWrite(rosl, eoc);

                  vertex tgt_state = boost::target(e, *stg);
                  if(HLS->Rliv->get_live_in(tgt_state).find(std::make_pair(scalar_def, step)) !=
                     HLS->Rliv->get_live_in(tgt_state).end())
                  {
                     HLS->Rliv->add_state_out_for_var(scalar_def, eoc, rosl, tgt_state);
                  }
               }
            }
         }
      }
      INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "<--Analyzed state " + state_info->name);
   }

#ifndef NDEBUG
   if(debug_level >= DEBUG_LEVEL_PEDANTIC)
   {
      /// print the analysis result
      const BehavioralHelperConstRef BH = FB->CGetBehavioralHelper();
      PRINT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "STG Liveness for function " + BH->get_function_name());
      BOOST_FOREACH(vertex v, boost::vertices(*stg))
      {
         PRINT_DBG_STRING(DEBUG_LEVEL_PEDANTIC, debug_level, "Live In for state " + HLS->STG->get_state_name(v) + ": ");
         for(const auto& li : HLS->Rliv->get_live_in(v))
         {
            PRINT_DBG_STRING(DEBUG_LEVEL_PEDANTIC, debug_level,
                             BH->PrintVariable(li.first) + "(" + STR(li.second) + ") ");
         }
         PRINT_DBG_STRING(DEBUG_LEVEL_PEDANTIC, debug_level, "\n");

         PRINT_DBG_STRING(DEBUG_LEVEL_PEDANTIC, debug_level,
                          "Live Out for state " + HLS->STG->get_state_name(v) + ": ");
         for(const auto& lo : HLS->Rliv->get_live_out(v))
         {
            PRINT_DBG_STRING(DEBUG_LEVEL_PEDANTIC, debug_level,
                             BH->PrintVariable(lo.first) + "(" + STR(lo.second) + ") ");
         }
         PRINT_DBG_STRING(DEBUG_LEVEL_PEDANTIC, debug_level, "\n");
      }
   }
#endif
   return DesignFlowStep_Status::SUCCESS;
}
