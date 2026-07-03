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
 * @file reg_binding_creator.cpp
 * @brief Base class for all register allocation algorithms
 *
 *
 * @author Christian Pilato <pilato@elet.polimi.it>
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 * @author Michele Fiorito <michele.fiorito@polimi.it>
 *
 */
#include "reg_binding_creator.hpp"

#include "Parameter.hpp"
#include "behavioral_helper.hpp"
#include "call_graph_manager.hpp"
#include "cdfc_module_binding.hpp"
#include "dbgPrintHelper.hpp"
#include "function_behavior.hpp"
#include "functions.hpp"
#include "hls.hpp"
#include "hls_manager.hpp"
#include "ir_helper.hpp"
#include "ir_manager.hpp"
#include "liveVariables.hpp"
#include "refcount.hpp"
#include "reg_binding.hpp"
#include "storage_value_insertion.hpp"
#include "utility.hpp"

#include <boost/version.hpp>
#include <iosfwd>

#include "kmp_bambu_names.h"

#define VAL(str) #str
#define TOSTRING(str) VAL(str)

reg_binding_creator::reg_binding_creator(const ParameterConstRef _Param, const HLS_managerRef _HLSMgr,
                                         unsigned int _funId, const DesignFlowManager& _design_flow_manager,
                                         const HLSFlowStep_Type _hls_flow_step_type,
                                         const HLSFlowStepSpecializationConstRef _hls_flow_step_specialization)
    : HLSFunctionStep(_Param, _HLSMgr, _funId, _design_flow_manager, _hls_flow_step_type,
                      _hls_flow_step_specialization),
      register_lower_bound(0),
      cs_modified(false)
{
   debug_level = parameters->get_class_debug_level(GET_CLASS(*this));
}

HLS_step::HLSRelationships
reg_binding_creator::ComputeHLSRelationships(const DesignFlowStep::RelationshipType relationship_type) const
{
   HLSRelationships ret;
   switch(relationship_type)
   {
      case(DEPENDENCE_RELATIONSHIP):
      {
         ret.insert(std::make_tuple(HLSFlowStep_Type::ALLOCATION, HLSFlowStepSpecializationConstRef(),
                                    HLSFlowStep_Relationship::SAME_FUNCTION));

         ret.insert(std::make_tuple(HLSFlowStep_Type::EASY_MODULE_BINDING, HLSFlowStepSpecializationConstRef(),
                                    HLSFlowStep_Relationship::SAME_FUNCTION));

         ret.insert(std::make_tuple(HLSFlowStep_Type::BUILD_FSM, HLSFlowStepSpecializationConstRef(),
                                    HLSFlowStep_Relationship::SAME_FUNCTION));
         if(HLSMgr->get_HLS(funId))
         {
            ret.insert(std::make_tuple(HLSMgr->get_HLS(funId)->chaining_algorithm, HLSFlowStepSpecializationConstRef(),
                                       HLSFlowStep_Relationship::SAME_FUNCTION));
            ret.insert(std::make_tuple(HLSMgr->get_HLS(funId)->liveVariableAlgorithm,
                                       HLSFlowStepSpecializationConstRef(), HLSFlowStep_Relationship::SAME_FUNCTION));
            ret.insert(std::make_tuple(HLSMgr->get_HLS(funId)->module_binding_algorithm,
                                       HLSFlowStepSpecializationConstRef(), HLSFlowStep_Relationship::SAME_FUNCTION));
         }
         break;
      }
      case(INVALIDATION_RELATIONSHIP):
      {
         if(GetStatus() == DesignFlowStep_Status::SUCCESS && cs_modified)
         {
            ret.insert(std::make_tuple(hls_flow_step_type, hls_flow_step_specialization,
                                       HLSFlowStep_Relationship::CALLED_FUNCTIONS));
         }
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

void reg_binding_creator::Initialize()
{
   HLSFunctionStep::Initialize();
   cs_modified = false;
}

DesignFlowStep_Status reg_binding_creator::InternalExec()
{
   const auto status = RegisterBinding();
   THROW_ASSERT(HLS->Rreg, "");

   const auto& old_cs_states = HLS->Rreg->context_switch_states;
   const auto cs_states = ComputeContextSwitchStates();
   cs_modified = [&]() {
      for(const auto& state_cs : cs_states)
      {
         if(!old_cs_states.count(state_cs.first))
         {
            return true;
         }
      }
      return false;
   }();
   if(cs_modified)
   {
      HLS->Rreg->context_switch_states = cs_states;
      return DesignFlowStep_Status::SUCCESS;
   }
   return status;
}

CustomMap<FSMInfo::state_descriptor, bool> reg_binding_creator::ComputeContextSwitchStates()
{
   CustomMap<FSMInfo::state_descriptor, bool> context_switch_states;

   const auto FB = HLSMgr->CGetFunctionBehavior(funId);
   const auto omp_info = FB->GetOMPInfo();
   if(omp_info && omp_info->context_count > 1U)
   {
      const auto& fsm_info = HLS->fsm_info;
      const auto is_bounded = fsm_info->bounded;
      const auto& support = HLS->fsm_info->vertices();
      const auto depth_vertices = fsm_info->depthMap();

      const auto& CGM = HLSMgr->CGetCallGraphManager();
      const auto& CG = CGM.GetCallGraph();
      const auto function_v = CGM.GetVertex(HLS->functionId);

      // Compute weak cs states from caller functions
      for(const auto& e : CG.in_edges(function_v))
      {
         const auto caller_id = CGM.get_function(CG.source(e));
         const auto caller_FB = HLSMgr->CGetFunctionBehavior(caller_id);
         const auto caller_omp_info = caller_FB->GetOMPInfo();
         const auto caller_HLS = HLSMgr->get_HLS(caller_id);
         if(caller_omp_info && caller_omp_info->context_count > 1U && caller_HLS && caller_HLS->fsm_info)
         {
            const auto caller_op_graph = caller_FB->GetOpGraph(FunctionBehavior::CFG);

            CustomUnorderedSet<gc_vertex_descriptor> caller_ops;
            const auto& stmt_to_op = caller_op_graph.CGetGraphInfo().ir_node_to_operation;
            for(const auto call_point : CG.CGetEdgeInfo(e).direct_call_points)
            {
               THROW_ASSERT(stmt_to_op.count(call_point),
                            "Call point is not an operation. " + STR(call_point) + " - " +
                                HLSMgr->get_ir_manager()->GetIRNode(call_point)->ToString());
               const auto call_op = stmt_to_op.at(call_point);
               caller_ops.insert(call_op);
            }

            CustomSet<FSMInfo::state_descriptor> caller_states;
            for(const auto state : caller_HLS->fsm_info->vertices())
            {
               const auto& state_ops = caller_HLS->fsm_info->getState(state).startingOperations;
               if(std::count_if(state_ops.cbegin(), state_ops.cend(),
                                [&](const gc_vertex_descriptor op) { return caller_ops.count(op); }))
               {
                  caller_states.insert(state);
               }
            }
            THROW_ASSERT(caller_states.size(), "Call points not matching with caller states.");

            const auto caller_depth_vertices = caller_HLS->fsm_info->depthMap();
            const auto& caller_cs_states = caller_HLS->Rreg->context_switch_states;
            const auto entry_state = HLS->fsm_info->entryNode;
            for(const auto caller_state : caller_states)
            {
               if(caller_cs_states.count(caller_state))
               {
                  context_switch_states.insert(std::make_pair(entry_state, false));
                  std::cout << "Function called from context switch state\n";
               }
               const auto caller_depth = static_cast<unsigned int>(
                   std::distance(caller_depth_vertices.cbegin(),
                                 std::find_if(caller_depth_vertices.cbegin(), caller_depth_vertices.cend(),
                                              [&](const std::set<FSMInfo::state_descriptor>& dv) {
                                                 return dv.count(caller_state);
                                              })));
               if(is_bounded)
               {
                  if(depth_vertices.size() > 1U)
                  {
                     for(auto depth = 1U; depth < depth_vertices.size(); ++depth)
                     {
                        const auto& depth_states = caller_depth_vertices.at(caller_depth + depth);
                        for(const auto depth_state : depth_states)
                        {
                           if(caller_cs_states.count(depth_state))
                           {
                              for(const auto state : depth_vertices.at(depth))
                              {
                                 context_switch_states.insert(std::make_pair(state, false));
                              }
                              break;
                           }
                        }
                     }
                  }
               }
               else
               {
                  THROW_ASSERT(caller_depth_vertices.size() >= caller_depth + 1U, "");
                  const auto& depth_states = caller_depth_vertices.at(caller_depth + 1U);
                  for(const auto depth_state : depth_states)
                  {
                     if(caller_cs_states.count(depth_state))
                     {
                        for(const auto state : support)
                        {
                           context_switch_states.insert(std::make_pair(state, false));
                        }
                        break;
                     }
                  }
               }
            }
         }
      }

      // Compute hard cs states from operations and weak cs states from called functions
      const auto op_graph = FB->GetOpGraph(FunctionBehavior::FLSAODG);
      const auto& function_mem = FB->get_function_mem();
      for(const auto state : support)
      {
         const auto& state_ops = HLS->fsm_info->getState(state).startingOperations;
         for(const auto op_v : state_ops)
         {
            const auto op = op_graph.CGetNodeInfo(op_v).node;
            if(op)
            {
               if(ir_helper::IsLoad(op, function_mem) || ir_helper::IsStore(op, function_mem))
               {
                  context_switch_states[state] = true;
               }
               else
               {
                  const auto called_fnode = [&]() -> ir_nodeRef {
                     const auto op_kind = op->get_kind();
                     const auto extract_fnode = [](const ir_nodeRef& tn) -> ir_nodeRef {
                        if(tn->get_kind() == addr_node_K)
                        {
                           return GetPointerS<const unary_node>(tn)->op;
                        }
                        THROW_ASSERT(tn->get_kind() == function_val_node_K, "");
                        return tn;
                     };
                     if(op_kind == call_stmt_K)
                     {
                        const auto gc = GetPointerS<const call_stmt>(op);
                        return extract_fnode(gc->fn);
                     }
                     else if(op_kind == assign_stmt_K)
                     {
                        const auto ga = GetPointerS<const assign_stmt>(op);
                        if(ga->op1->get_kind() == call_node_K)
                        {
                           return extract_fnode(GetPointerS<const call_node>(ga->op1)->fn);
                        }
                     }
                     return nullptr;
                  }();
                  if(called_fnode)
                  {
                     const auto called_id = called_fnode->index;
                     const auto called_fname = ir_helper::GetFunctionName(called_fnode);
                     const auto called_omp_info = HLSMgr->CGetFunctionBehavior(called_id)->GetOMPInfo();
                     if(boost::starts_with(called_fname, TOSTRING(KMP_CRITICAL)) ||
                        boost::starts_with(called_fname, TOSTRING(KMP_WAIT_ALL_THREADS)))
                     {
                        context_switch_states[state] = true;
                     }
                     else if(GetPointerS<const function_val_node>(called_fnode)->body == nullptr)
                     {
                        context_switch_states[state] |= false;
                     }
                     else if(called_omp_info && called_omp_info->context_count > 1U)
                     {
                        const auto called_HLS = HLSMgr->get_HLS(called_id);
                        THROW_ASSERT(
                            called_HLS,
                            "HLS not available for " +
                                HLSMgr->CGetFunctionBehavior(called_id)->CGetBehavioralHelper()->GetFunctionName());
                        const auto called_entry_state = called_HLS->fsm_info->entryNode;
                        const auto& called_cs_states = called_HLS->Rreg->context_switch_states;

                        const auto css_it = called_cs_states.find(called_entry_state);
                        if(css_it != called_cs_states.end() && css_it->second)
                        {
                           context_switch_states[state] |= false;
                        }
                        if(called_HLS->fsm_info->bounded)
                        {
                           const auto calling_depth = static_cast<unsigned int>(
                               std::distance(depth_vertices.cbegin(),
                                             std::find_if(depth_vertices.cbegin(), depth_vertices.cend(),
                                                          [&](const std::set<FSMInfo::state_descriptor>& dv) {
                                                             return dv.count(state);
                                                          })));
                           const auto called_depth_vertices = called_HLS->fsm_info->depthMap();
                           for(auto depth = 0U; depth < called_depth_vertices.size(); ++depth)
                           {
                              const auto& depth_set = called_depth_vertices.at(depth);
                              for(const auto called_state : depth_set)
                              {
                                 const auto it = called_cs_states.find(called_state);
                                 if(it != called_cs_states.end() && it->second)
                                 {
                                    for(const auto v : depth_vertices.at(calling_depth + depth))
                                    {
                                       context_switch_states[v] |= false;
                                    }
                                    break;
                                 }
                              }
                           }
                        }
                        else
                        {
                           if(std::count_if(called_cs_states.cbegin(), called_cs_states.cend(),
                                            [&](const std::pair<FSMInfo::state_descriptor, bool>& sh) {
                                               return sh.first != called_entry_state && sh.second;
                                            }))
                           {
                              for(const auto tgt : HLS->fsm_info->successors(state))
                              {
                                 context_switch_states.insert_or_assign(tgt, true);
                              }
                           }
                        }
                     }
                  }
               }
            }
         }
      }
      INDENT_OUT_MEX(OUTPUT_LEVEL_VERBOSE, HLS->output_level,
                     "---Context switch states out of total FSM states: " + STR(context_switch_states.size()) + "/" +
                         STR(support.size()));
   }
   return context_switch_states;
}
