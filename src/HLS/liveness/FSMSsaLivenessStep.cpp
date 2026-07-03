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
 * @file FSMSsaLivenessStep.cpp
 * @brief liveness analysis exploiting the SSA form of the IR and supporting pipelining
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 *
 */
#include "FSMSsaLivenessStep.hpp"

#include "Parameter.hpp"
#include "basic_block.hpp"
#include "behavioral_helper.hpp"
#include "dbgPrintHelper.hpp"
#include "fsm/FSMInfo.hpp"
#include "function_behavior.hpp"
#include "hls.hpp"
#include "hls_constraints.hpp"
#include "hls_manager.hpp"
#include "ir_helper.hpp"
#include "ir_manager.hpp"
#include "ir_node.hpp"
#include "liveVariables.hpp"
#include "memory.hpp"
#include "op_graph.hpp"
#include "string_manipulation.hpp"
#include <algorithm>
#include <list>

HLS_step::HLSRelationships
FSMSsaLivenessStep::ComputeHLSRelationships(const DesignFlowStep::RelationshipType relationship_type) const
{
   HLSRelationships ret;
   switch(relationship_type)
   {
      case(DEPENDENCE_RELATIONSHIP):
      {
         ret.insert(std::make_tuple(HLSFlowStep_Type::BUILD_FSM, HLSFlowStepSpecializationConstRef(),
                                    HLSFlowStep_Relationship::SAME_FUNCTION));
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
         THROW_UNREACHABLE("");
   }
   return ret;
}

FSMSsaLivenessStep::FSMSsaLivenessStep(const ParameterConstRef _parameters, const HLS_managerRef _HLSMgr,
                                       unsigned int _funId, const DesignFlowManager& _design_flow_manager)
    : HLSFunctionStep(_parameters, _HLSMgr, _funId, _design_flow_manager, HLSFlowStep_Type::FSM_NI_SSA_LIVENESS)
{
   debug_level = parameters->get_class_debug_level(GET_CLASS(*this));
}

void FSMSsaLivenessStep::Initialize()
{
   HLSFunctionStep::Initialize();
   HLS->Rliv = liveVariablesRef(new liveVariables());
}

static void update_liveout_with_prev(const HLS_managerRef HLSMgr, hlsRef HLS, const OpGraph& data,
                                     FSMInfo::state_descriptor current_state, unsigned bb_index,
                                     FSMInfo::state_descriptor prevState, bool is_lp_feedback, unsigned LP_II,
                                     unsigned int funId,
                                     int
#ifndef NDEBUG
                                         debug_level
#endif
)
{
   auto fsm_info = HLS->fsm_info;
   const auto& state_info = fsm_info->getState(prevState);
   for(const auto& exec_op : state_info.executingOperations)
   {
      const auto& op_info = data.CGetNodeInfo(exec_op);
      auto isAPhi = op_info.node_type & TYPE_PHI;
      auto manage_scalar_use = [&](unsigned scalar_use) {
         const bool scalar_is_register_compatible = HLSMgr->is_register_compatible(scalar_use);
         if(scalar_is_register_compatible)
         {
            auto def_op = getDefOp(data, scalar_use);
            auto not_have_def = std::find(state_info.endingOperations.begin(), state_info.endingOperations.end(),
                                          def_op) == state_info.endingOperations.end();
            if(isAPhi || not_have_def || fsm_info->notSameStep(prevState, def_op, exec_op))
            {
               unsigned int step = isAPhi ? fsm_info->GetStepPhiOut(data, exec_op, scalar_use, HLS->Rsch) :
                                            fsm_info->GetStep(data, prevState, exec_op, scalar_use, true,
                                                              scalar_is_register_compatible);
               bool repeat = false;
               do
               {
                  INDENT_DBG_MEX(
                      DEBUG_LEVEL_PEDANTIC, debug_level,
                      HLSMgr->CGetFunctionBehavior(funId)->CGetBehavioralHelper()->PrintVariable(scalar_use) + "-" +
                          STR(step));
                  HLS->Rliv->addLiveOutVariable(current_state, scalar_use, step);
                  repeat = is_lp_feedback && step > LP_II;
                  if(repeat)
                  {
                     auto prev_pair =
                         fsm_info->GetPrevStep(data, bb_index, scalar_use, step, LP_II, scalar_is_register_compatible);
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
                 ir_helper::IsParameter(HLSMgr->get_ir_manager()->GetIRNode(scalar_use)))
         {
            auto parStep = fsm_info->GetStepOp(data, prevState, exec_op);
            if(parStep)
            {
               std::pair<bool, unsigned> prev_pair;
               do
               {
                  INDENT_DBG_MEX(
                      DEBUG_LEVEL_PEDANTIC, debug_level,
                      HLSMgr->CGetFunctionBehavior(funId)->CGetBehavioralHelper()->PrintVariable(scalar_use) + "-" +
                          STR(parStep));
                  HLS->Rliv->addLiveOutVariable(current_state, scalar_use, parStep);
                  prev_pair = fsm_info->GetPrevStep(data, 2, scalar_use, parStep, 1, scalar_is_register_compatible);
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
         const auto& curr_state_info = fsm_info->getState(current_state);
         const auto phi_node = HLSMgr->get_ir_manager()->GetIRNode(op_info.GetNodeId());
         for(const auto& def_edge : GetPointer<const phi_stmt>(phi_node)->CGetDefEdgesList())
         {
            auto phi_in = def_edge.first->index;
            if((curr_state_info.isPrologue.count(exec_op) && bb_index != def_edge.second) ||
               (!curr_state_info.isPrologue.count(exec_op) && bb_index == def_edge.second))
            {
               manage_scalar_use(phi_in);
            }
         }
      }
      else
      {
         const auto& scalar_uses = getVariablesScalarUse(data, exec_op);

         for(const auto scalar_use : scalar_uses)
         {
            manage_scalar_use(scalar_use);
         }
      }
   }
   for(const auto& end_op : state_info.endingOperations)
   {
      const auto& scalar_defs = getVariablesScalarDef(data, end_op);

      for(const auto scalar_def : scalar_defs)
      {
         const bool scalar_def_register_compatible = HLSMgr->is_register_compatible(scalar_def);
         if(scalar_def_register_compatible)
         {
            unsigned int step =
                fsm_info->GetStep(data, prevState, end_op, scalar_def, false, scalar_def_register_compatible);
            INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level,
                           "erase " +
                               HLSMgr->CGetFunctionBehavior(funId)->CGetBehavioralHelper()->PrintVariable(scalar_def) +
                               "-" + STR(step));
            HLS->Rliv->removeLiveOutVariable(current_state, scalar_def, step);
         }
      }
   }
}

DesignFlowStep_Status FSMSsaLivenessStep::InternalExec()
{
   const auto FB = HLSMgr->CGetFunctionBehavior(funId);
   const auto fbb = FB->GetBBGraph(FunctionBehavior::FBB);
   const auto data = FB->GetOpGraph(FunctionBehavior::DFG);
   auto isFunctionPipelined = FB->is_function_pipelined();
   auto hasPipelinedStates = isFunctionPipelined;

   /// Map between basic block node index and vertices
   CustomUnorderedMap<unsigned int, BBGraph::vertex_descriptor> BBIndexMap;
   for(const auto v : fbb.vertices())
   {
      BBIndexMap[fbb.CGetNodeInfo(v).get_bb_index()] = v;
   }

   auto fsm_info = HLS->fsm_info;
   /// vertices of the FSM in reverse order.
   std::list<FSMInfo::state_descriptor> reverseStates;
   fsm_info->reverseTopologicalOrder(reverseStates);
   auto exitState = fsm_info->exitNode;
   std::deque<FSMInfo::state_descriptor> dummyStates;

   INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "-->Collect dummy states and set hasPipelinedStates");
   for(const auto& rosl : reverseStates)
   {
      const auto& state_info = fsm_info->getState(rosl);
      if(state_info.isDummy)
      {
         dummyStates.push_back(rosl);
      }
      if(state_info.isPipelinedState)
      {
         hasPipelinedStates = true;
      }
   }
   INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "<--");

   /// compute live out
   INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "-->Computing live out");
   const auto BBExit = fbb.CGetGraphInfo().exit_vertex;
   unsigned int prevBBindex = fbb.CGetNodeInfo(BBExit).get_bb_index();
   auto prevState = exitState;
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                  "---prevState: " + fsm_info->getState(prevState).name + " prevBBindex: " + STR(prevBBindex));
   for(auto ind = (hasPipelinedStates ? 0 : 1); ind < 2 || hasPipelinedStates; ++ind)
   {
      size_t liveOutSizeBefore = 0;
      if(hasPipelinedStates)
      {
         for(const auto& rosl : reverseStates)
         {
            liveOutSizeBefore += HLS->Rliv->getLiveOutFsmVariables(rosl).size();
         }
      }
      for(const auto& rosl : reverseStates)
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "-->Live out for state " + fsm_info->getState(rosl).name);
         if(rosl == exitState && !isFunctionPipelined)
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "<--");
            continue;
         }
         const auto& state_info = fsm_info->getState(rosl);
         if(state_info.isDummy)
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "<--");
            continue;
         }

         unsigned int bb_index = state_info.bbId;
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---BB: " + STR(bb_index));
         if(state_info.isPipelinedState)
         {
            for(const auto targetState : fsm_info->successors(rosl))
            {
               prevState = targetState;
               const auto& tgt_state_info = fsm_info->getState(targetState);
               prevBBindex = tgt_state_info.bbId;
               if(prevBBindex != bb_index)
               {
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                 "---prevBBindex " + STR(prevBBindex) + " != bb_index " + STR(bb_index));
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                 "---adding live out of BB " + STR(bb_index) + " to live out of state " +
                                     state_info.name);
                  for(const auto& lo : fbb.CGetNodeInfo(BBIndexMap[bb_index]).getLiveOutBbVariables())
                  {
                     const bool lo_register_compatible = HLSMgr->is_register_compatible(lo);
                     if(lo_register_compatible)
                     {
                        auto step = fsm_info->GetStepOut(data, lo);
                        INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                       "---" + FB->CGetBehavioralHelper()->PrintVariable(lo) + "-" + STR(step));
                        HLS->Rliv->addLiveOutVariable(rosl, lo, step);
                     }
                  }
               }
               else
               {
                  bool is_feedback = fsm_info->getEdge(rosl, targetState).edgeSelector == stEdgeFeedback;
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                 "---adding live out of " + fsm_info->getState(targetState).name +
                                     " to live out of state " + state_info.name);
                  const auto prev_live_out = HLS->Rliv->getLiveOutFsmVariables(rosl);
                  if(targetState != rosl)
                  {
                     for(const auto& lo : HLS->Rliv->getLiveOutFsmVariables(targetState))
                     {
                        const bool lo_register_compatible = HLSMgr->is_register_compatible(lo.first);
                        auto pre_pair =
                            fsm_info->GetPrevStep(data, bb_index, lo.first, lo.second, 1, lo_register_compatible);
                        if(pre_pair.first)
                        {
                           INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                          "---" + FB->CGetBehavioralHelper()->PrintVariable(lo.first) + "-" +
                                              STR(pre_pair.second));
                           HLS->Rliv->addLiveOutVariable(rosl, lo.first, pre_pair.second);
                        }
                        else if(state_info.isLastState)
                        {
                           if(!ir_helper::IsParameter(HLSMgr->get_ir_manager()->GetIRNode(lo.first)) &&
                              std::find(tgt_state_info.endingOperations.begin(), tgt_state_info.endingOperations.end(),
                                        getDefOp(data, lo.first)) == tgt_state_info.endingOperations.end())
                           {
                              INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "is_last_state");
                              INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                             "---" + FB->CGetBehavioralHelper()->PrintVariable(lo.first) + "-" +
                                                 STR(pre_pair.second));
                              HLS->Rliv->addLiveOutVariable(rosl, lo.first, pre_pair.second);
                           }
                        }
                     }
                  }
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "update_liveout_with_prev");
                  update_liveout_with_prev(HLSMgr, HLS, data, rosl, bb_index, targetState, is_feedback, state_info.lpII,
                                           funId, debug_level);
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "add back initial liveout");
                  for(const auto& lo : prev_live_out)
                  {
                     INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                    "---" + FB->CGetBehavioralHelper()->PrintVariable(lo.first) + "-" + STR(lo.second));
                     HLS->Rliv->addLiveOutVariable(rosl, lo.first, lo.second);
                  }
               }
            }
         }
         else if(prevBBindex != bb_index)
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                           "---PrevBB" + STR(prevBBindex) + " != BB" + STR(bb_index));
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                           "---copying BB live out " + STR(bb_index) + " to state " + state_info.name);
            for(const auto lo : fbb.CGetNodeInfo(BBIndexMap[bb_index]).getLiveOutBbVariables())
            {
               const bool lo_register_compatible = HLSMgr->is_register_compatible(lo);
               if(lo_register_compatible)
               {
                  auto step = fsm_info->GetStepOut(data, lo);
                  INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level,
                                 "---" + FB->CGetBehavioralHelper()->PrintVariable(lo) + "-" + STR(step));
                  HLS->Rliv->addLiveOutVariable(rosl, lo, step);
               }
            }
         }
         else
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                           "---copying state live out of " + fsm_info->getState(prevState).name + " to state " +
                               state_info.name);
            for(const auto& lo : HLS->Rliv->getLiveOutFsmVariables(prevState))
            {
               const bool lo_register_compatible = HLSMgr->is_register_compatible(lo.first);
               auto pre_pair = fsm_info->GetPrevStep(data, bb_index, lo.first, lo.second, 1, lo_register_compatible);
               if(pre_pair.first)
               {
                  const BehavioralHelperConstRef BH = FB->CGetBehavioralHelper();
                  INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level,
                                 BH->PrintVariable(lo.first) + "-" + STR(pre_pair.second));
                  HLS->Rliv->addLiveOutVariable(rosl, lo.first, pre_pair.second);
               }
            }
            update_liveout_with_prev(HLSMgr, HLS, data, rosl, bb_index, prevState, false, 0, funId, debug_level);
         }
         prevState = rosl;
         prevBBindex = bb_index;
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                        "---new prevState: " + fsm_info->getState(prevState).name +
                            " new prevBBindex: " + STR(prevBBindex));
         INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "<--");
      }
      // For pipelined designs, iterate until live-out sets converge.
      // The backward propagation through GetPrevStep needs enough
      // passes to cover the full pipeline depth.
      if(hasPipelinedStates && ind >= 1)
      {
         size_t liveOutSizeAfter = 0;
         for(const auto& rosl : reverseStates)
         {
            liveOutSizeAfter += HLS->Rliv->getLiveOutFsmVariables(rosl).size();
         }
         if(liveOutSizeAfter == liveOutSizeBefore)
         {
            break;
         }
      }
   }
   INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "<--");

   /// compute the live in of a state by traversing the state list in topological order
   INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "-->Computing live in");
   auto entryState = fsm_info->entryNode;
   prevState = entryState;
   for(auto it = reverseStates.rbegin(); it != reverseStates.rend(); ++it)
   {
      const auto& osl = *it;
      INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "-->Live in for state " + fsm_info->getState(osl).name);

      if(osl == exitState && !isFunctionPipelined)
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "<--");
         continue;
      }
      const auto& state_info = fsm_info->getState(osl);
      if(state_info.isDummy)
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "<--");
         continue;
      }

      unsigned int bb_index = state_info.bbId;
      const auto live_in_bb_index = fbb.CGetNodeInfo(BBIndexMap[bb_index]).getLiveInBbVariables();
      if(state_info.isPipelinedState)
      {
         for(const auto& src_state : fsm_info->predecessors(osl))
         {
            const auto& src_state_info = fsm_info->getState(src_state);
            prevBBindex = src_state_info.bbId;
            auto adjust_phi = [&] {
               for(const auto& exec_op : state_info.executingOperations)
               {
                  const auto& op_info = data.CGetNodeInfo(exec_op);
                  if((op_info.node_type & TYPE_PHI) != 0)
                  {
                     const auto phi_node = HLSMgr->get_ir_manager()->GetIRNode(op_info.GetNodeId());
                     for(const auto& def_edge : GetPointer<const phi_stmt>(phi_node)->CGetDefEdgesList())
                     {
                        auto phi_in = def_edge.first->index;
                        if(HLSMgr->is_register_compatible(phi_in) && !live_in_bb_index.contains(phi_in))
                        {
                           // Do not remove the phi input from live-in if it is also used by a
                           // non-phi operation in the same pipelined state. Removing it would
                           // break the destination-state computation for the register write-enable
                           // of the non-phi consumer's storage register.
                           bool used_by_non_phi = false;
                           for(const auto& other_op : state_info.executingOperations)
                           {
                              const auto& other_info = data.CGetNodeInfo(other_op);
                              if((other_info.node_type & TYPE_PHI) == 0)
                              {
                                 const auto& other_uses = getVariablesScalarUse(data, other_op);
                                 if(other_uses.count(phi_in))
                                 {
                                    used_by_non_phi = true;
                                    break;
                                 }
                              }
                           }
                           if(!used_by_non_phi)
                           {
                              unsigned int step = fsm_info->GetStepPhiOut(data, exec_op, phi_in, HLS->Rsch);
                              INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level,
                                             "---erase phi in " + FB->CGetBehavioralHelper()->PrintVariable(phi_in) +
                                                 "-" + STR(step));
                              HLS->Rliv->removeLiveInVariable(osl, phi_in, step);
                           }
                        }
                     }
                     auto phi_res = GetPointer<const phi_stmt>(phi_node)->res->index;
                     const bool phi_res_register_compatible = HLSMgr->is_register_compatible(phi_res);
                     unsigned int step =
                         fsm_info->GetStep(data, osl, exec_op, phi_res, false, phi_res_register_compatible);
                     INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level,
                                    "---" + FB->CGetBehavioralHelper()->PrintVariable(phi_res) + "-" + STR(step));
                     HLS->Rliv->addLiveInVariable(osl, phi_res, step);
                  }
               }
            };
            if(prevBBindex != bb_index)
            {
               for(const auto li : fbb.CGetNodeInfo(BBIndexMap[prevBBindex]).getLiveOutBbVariables())
               {
                  if(HLSMgr->is_register_compatible(li))
                  {
                     auto step_in = fsm_info->GetStepIn(data, bb_index, li);
                     INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level,
                                    "---" + FB->CGetBehavioralHelper()->PrintVariable(li) + "--" + STR(step_in));
                     HLS->Rliv->addLiveInVariable(osl, li, step_in);
                  }
               }
               adjust_phi();
            }
            else
            {
               for(const auto& li : HLS->Rliv->getLiveOutFsmVariables(src_state))
               {
                  INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level,
                                 "---" + FB->CGetBehavioralHelper()->PrintVariable(li.first) + "-" + STR(li.second));
                  HLS->Rliv->addLiveInVariable(osl, li.first, li.second);
               }
               adjust_phi();
               for(const auto& li : HLS->Rliv->getLiveOutFsmVariables(osl))
               {
                  const bool li_register_compatible = HLSMgr->is_register_compatible(li.first);
                  auto pre_pair = fsm_info->GetPrevStep(data, bb_index, li.first, li.second, 1, li_register_compatible);
                  if(pre_pair.first)
                  {
                     INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level,
                                    "---" + FB->CGetBehavioralHelper()->PrintVariable(li.first) + "-" +
                                        STR(pre_pair.second));
                     HLS->Rliv->addLiveInVariable(osl, li.first, pre_pair.second);
                  }
               }
            }
         }
      }
      else if(prevBBindex != bb_index)
      {
         for(const auto li : live_in_bb_index)
         {
            if(HLSMgr->is_register_compatible(li))
            {
               auto step_in = fsm_info->GetStepIn(data, bb_index, li);
               INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level,
                              "---" + FB->CGetBehavioralHelper()->PrintVariable(li) + "--" + STR(step_in));
               HLS->Rliv->addLiveInVariable(osl, li, step_in);
            }
         }
      }
      else
      {
         for(const auto& lo : HLS->Rliv->getLiveOutFsmVariables(prevState))
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level,
                           "---" + FB->CGetBehavioralHelper()->PrintVariable(lo.first) + "--" + STR(lo.second));
            HLS->Rliv->addLiveInVariable(osl, lo.first, lo.second);
         }
      }
      prevState = osl;
      prevBBindex = bb_index;
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                     "---new prevState: " + fsm_info->getState(prevState).name +
                         " new prevBBindex: " + STR(prevBBindex));
      INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "<--");
   }

   INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "<--");

   /// fix the live in/out of dummy states
   INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "-->Updating dummy states liveness");
   for(const auto& ds : dummyStates)
   {
      const auto& state_info = fsm_info->getState(ds);
      for(const auto& eo : state_info.executingOperations)
      {
         const auto& scalar_defs = getVariablesScalarDef(data, eo);
         for(const auto scalar_def : scalar_defs)
         {
            if(HLSMgr->is_register_compatible(scalar_def))
            {
               unsigned int step = fsm_info->GetStepWrite(data, eo);
               HLS->Rliv->addLiveOutVariable(ds, scalar_def, step);
            }
         }
      }
      for(const auto& src_state : fsm_info->predecessors(ds, true))
      {
         for(const auto& li : HLS->Rliv->getLiveOutFsmVariables(src_state))
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level,
                           "---" + FB->CGetBehavioralHelper()->PrintVariable(li.first) + "-d-" + STR(li.second));
            HLS->Rliv->addLiveInVariable(ds, li.first, li.second);
         }
         /// add all the uses of ds to src_state
         for(const auto& eo : state_info.executingOperations)
         {
            const auto& scalar_uses = getVariablesScalarUse(data, eo);
            for(const auto scalar_use : scalar_uses)
            {
               if(HLSMgr->is_register_compatible(scalar_use))
               {
                  const bool scalar_use_register_compatible = HLSMgr->is_register_compatible(scalar_use);
                  auto step = fsm_info->GetStep(data, ds, eo, scalar_use, true, scalar_use_register_compatible);
                  HLS->Rliv->addVariableDestinationState(scalar_use, getDefOp(data, scalar_use), src_state, ds);
                  INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level,
                                 "---" + FB->CGetBehavioralHelper()->PrintVariable(scalar_use) + "-a-" + STR(step));
                  HLS->Rliv->addLiveInVariable(ds, scalar_use, step);
                  HLS->Rliv->addLiveOutVariable(src_state, scalar_use, step);
                  /// extend the lifetime of used variable to reduce the critical path
                  for(const auto& targetState : fsm_info->successors(ds))
                  {
                     const auto& tgt_state_info = fsm_info->getState(targetState);
                     INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level,
                                    "---" + FB->CGetBehavioralHelper()->PrintVariable(scalar_use) + "-a-" +
                                        STR(tgt_state_info.isPipelinedState ? step : 0));
                     HLS->Rliv->addLiveInVariable(targetState, scalar_use, tgt_state_info.isPipelinedState ? step : 0);
                  }
               }
            }
         }
      }
   }
   INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "<--");

   /// compute transitions on which a variable is live out
   for(const auto& rosl : reverseStates)
   {
      const auto& state_info = fsm_info->getState(rosl);
      INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "-->Analyzing state " + state_info.name);

      for(const auto& eoc : state_info.endingOperations)
      {
         const auto& scalar_defs = getVariablesScalarDef(data, eoc);
         for(const auto scalar_def : scalar_defs)
         {
            if(HLSMgr->is_register_compatible(scalar_def))
            {
               for(const auto tgt_state : fsm_info->successors(rosl))
               {
                  auto step = fsm_info->GetStepWrite(data, eoc);

                  if(HLS->Rliv->getLiveInFsmVariables(tgt_state).find(std::make_pair(scalar_def, step)) !=
                     HLS->Rliv->getLiveInFsmVariables(tgt_state).end())
                  {
                     HLS->Rliv->addVariableDestinationState(scalar_def, eoc, rosl, tgt_state);
                  }
               }
            }
         }
      }
      INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "<--");
   }

#ifndef NDEBUG
   if(debug_level >= DEBUG_LEVEL_PEDANTIC)
   {
      /// print the analysis result
      const BehavioralHelperConstRef BH = FB->CGetBehavioralHelper();
      PRINT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "FSM Liveness for function " + BH->GetFunctionName());
      for(const auto v : fsm_info->vertices())
      {
         const auto& st = fsm_info->getState(v);
         PRINT_DBG_STRING(DEBUG_LEVEL_PEDANTIC, debug_level, "Live In for state " + st.name + ": ");
         for(const auto& liveIn : HLS->Rliv->getLiveInFsmVariables(v))
         {
            PRINT_DBG_STRING(DEBUG_LEVEL_PEDANTIC, debug_level,
                             BH->PrintVariable(liveIn.first) + "(" + STR(liveIn.second) + ") ");
         }
         PRINT_DBG_STRING(DEBUG_LEVEL_PEDANTIC, debug_level, "\n");

         PRINT_DBG_STRING(DEBUG_LEVEL_PEDANTIC, debug_level, "Live Out for state " + fsm_info->getState(v).name + ": ");
         for(const auto& liveOut : HLS->Rliv->getLiveOutFsmVariables(v))
         {
            PRINT_DBG_STRING(DEBUG_LEVEL_PEDANTIC, debug_level,
                             BH->PrintVariable(liveOut.first) + "(" + STR(liveOut.second) + ") ");
         }
         PRINT_DBG_STRING(DEBUG_LEVEL_PEDANTIC, debug_level, "\n");
      }
   }
#endif
   return DesignFlowStep_Status::SUCCESS;
}
