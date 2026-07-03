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
 *              Copyright (C) 2022-2026 Politecnico di Milano
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
 * @file OMPLowering.cpp
 *
 * @author Michele Fiorito <michele.fiorito@polimi.it>
 *
 */

#include "OMPLowering.hpp"

#include "Parameter.hpp"
#include "Range.hpp"
#include "application_manager.hpp"
#include "basic_block.hpp"
#include "behavioral_helper.hpp"
#include "bit_lattice.hpp"
#include "call_graph_manager.hpp"
#include "dbgPrintHelper.hpp"
#include "function_behavior.hpp"
#include "functions.hpp"
#include "hls_device.hpp"
#include "hls_manager.hpp"
#include "ir_basic_block.hpp"
#include "ir_helper.hpp"
#include "ir_manager.hpp"
#include "ir_manipulation.hpp"
#include "ir_node.hpp"
#include "library_manager.hpp"
#include "op_graph.hpp"
#include "string_manipulation.hpp"
#include "structural_manager.hpp"
#include "structural_objects.hpp"
#include "technology_manager.hpp"
#include "technology_node.hpp"
#include "var_pp_functor.hpp"

#include "kmp_bambu_names.h"

#define VAL(str) #str
#define TOSTRING(str) VAL(str)

std::map<unsigned int, CustomSet<unsigned int>> OMPLowering::shared_infos;

OMPLowering::OMPLowering(const ParameterConstRef _parameters, const application_managerRef _AppM,
                         unsigned int _function_id, const DesignFlowManager& _design_flow_manager)
    : OMPCGExt(_parameters, _AppM, _function_id, OMP_LOWERING, _design_flow_manager)
{
   debug_level = _parameters->get_class_debug_level(GET_CLASS(*this), DEBUG_LEVEL_NONE);
}

CustomUnorderedSet<std::pair<FrontendFlowStepType, FunctionFrontendFlowStep::FunctionRelationship>>
OMPLowering::ComputeFrontendRelationships(const DesignFlowStep::RelationshipType relationship_type) const
{
   CustomUnorderedSet<std::pair<FrontendFlowStepType, FunctionRelationship>> relationships;
   switch(relationship_type)
   {
      case(DEPENDENCE_RELATIONSHIP):
      {
         relationships.insert(std::make_pair(OMP_CG_EXT, ALL_FUNCTIONS));
         relationships.insert(std::make_pair(OMP_LOWERING, CALLING_FUNCTIONS));
         relationships.insert(std::make_pair(SOFT_FLOAT_CG_EXT, SAME_FUNCTION));
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

bool OMPLowering::HasToBeExecuted() const
{
   return bb_version == 0 && FunctionFrontendFlowStep::HasToBeExecuted();
}

DesignFlowStep_Status OMPLowering::InternalExec()
{
   if(OMPCGExt::fork_infos.count(function_id))
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Generated fork call lowering not needed");
      return DesignFlowStep_Status::UNCHANGED;
   }

   const auto omp_info = function_behavior->GetOMPInfo();
   if(omp_info)
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Fork call id : " + STR(omp_info->fork_call_id));
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Context count: " + STR(omp_info->context_count));
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Local index  : " + STR(omp_info->local_idx));
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Global index : " + STR(omp_info->global_idx));
   }
   else
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Missing OMP info");
   }
   const auto& shared_functions = shared_infos[omp_info ? omp_info->fork_call_id : 0U];
   const auto HLSMgr = RefcountCast<HLS_manager>(AppM);
   auto& CGM = AppM->GetCallGraphManager();
   const auto extract_fnode = [](const ir_nodeRef& tn) {
      if(tn->get_kind() == addr_node_K)
      {
         return GetPointerS<const unary_node>(tn)->op;
      }
      THROW_ASSERT(tn->get_kind() == function_val_node_K, "");
      return tn;
   };

   bool modified = false;
   CustomUnorderedSet<unsigned int> already_visited;
   const auto bb_topological = OMPCGExt::DominatorTopologicalSort(function_id, AppM);
   for(const auto& BB : bb_topological)
   {
      const auto stmt_list = BB->CGetStmtList();
      for(const auto& stmt : stmt_list)
      {
         if(stmt->get_kind() == call_stmt_K)
         {
            const auto gc = GetPointerS<const call_stmt>(stmt);
            const auto called_fnode = extract_fnode(gc->fn);
            const auto called_fname = ir_helper::GetFunctionName(called_fnode);
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Analizing call_stmt_K " + called_fname);
            const auto replace_call = [&](const ir_nodeRef& version_fnode, const std::vector<ir_nodeRef>& args) {
               const auto version_call = ir_man->create_call_stmt(version_fnode, args, function_id, BUILTIN_LOCINFO);
               BB->Replace(stmt, version_call, true, AppM);
               const auto fu_name = functions::GetFUName(version_fnode->index, HLSMgr);
               HLSMgr->global_resource_constraints[std::make_pair(fu_name, LIBRARY_STD_FU)] = std::make_pair(1U, 1U);
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Replaced OMP " + STR(version_call));
            };

            if(shared_functions.count(called_fnode->index))
            {
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                              "---Function " + called_fname + " shared between threads.");
            }
            else if(called_fname == TOSTRING(KMP_SET_REDUCE_DATA) || called_fname == TOSTRING(KMP_BARRIER_REACHED) ||
                    called_fname == TOSTRING(KMP_WAIT_ALL_THREADS) || called_fname == TOSTRING(KMP_GET_REDUCE_DATA) ||
                    called_fname == TOSTRING(KMP_GET_TID_FROM_GTID))
            {
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Replacing OMP " + STR(stmt));
               const auto version_fnode = ForkFunction(called_fname, omp_info);
               replace_call(version_fnode, gc->args);
               modified = true;
            }
            else if(called_fname == TOSTRING(KMP_CRITICAL) || called_fname == TOSTRING(KMP_END_CRITICAL))
            {
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Replacing OMP " + STR(stmt));
               THROW_ASSERT(omp_info, "Critical section must be within parallel section");
               THROW_ASSERT(gc->args.size() >= 1, "Expected at least 1 argument for " + called_fname + " call.");
               const auto loc = ir_helper::GetBaseVariable(gc->args.at(0));
               const auto loc_id = loc->index;
               const auto lock = called_fname == TOSTRING(KMP_CRITICAL);
               const std::string fname = lock ? TOSTRING(KMP_CRITICAL) : TOSTRING(KMP_END_CRITICAL);
#ifndef NDEBUG
               const auto critical_name = [&]() {
                  const auto loc_name = STR(loc);
                  const auto t =
                      loc_name.substr(loc_name.find("gomp_critical_user_") + sizeof("gomp_critical_user_") - 1U);
                  return t.substr(0, t.find('.'));
               }();
#endif
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                              "---Critical section " + STR(lock ? "" : "end ") + critical_name + " (" + STR(loc_id) +
                                  ")");
               if(lock)
               {
                  omp_info->critical.push(loc_id);
               }
               else
               {
                  THROW_ASSERT(loc_id == omp_info->critical.top(), "Unordered critical section end: " + STR(loc_id) +
                                                                       " != " + STR(omp_info->critical.top()));
                  omp_info->critical.pop();
               }
               const auto version_fnode = [&]() {
                  const auto loc_fname =
                      fname + "_" + STR(loc_id) + OMPCGExt::OMPForkSuffix(omp_info->fork_call_id, omp_info->core_id);
                  const auto loc_fnode = TM->GetFunction(loc_fname);
                  if(loc_fnode)
                  {
                     return loc_fnode;
                  }
                  THROW_ASSERT(OMPCGExt::fork_infos.count(omp_info->fork_call_id), "");
                  OMPCGExt::GenerateCriticalAccessors(loc_id, OMPCGExt::fork_infos.at(omp_info->fork_call_id));
                  return TM->GetFunction(loc_fname);
               }();
               replace_call(version_fnode, std::vector<ir_nodeRef>());
               modified = true;
            }
            else if(OMPCGExt::fork_infos.count(called_fnode->index))
            {
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Generated fork call already expanded");
            }
            else if(omp_info)
            {
               const auto version_fnode = OMPCGExt::CloneFunction(called_fnode, omp_info);
               if(version_fnode->index != called_fnode->index)
               {
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Replacing " + STR(stmt));
                  const auto version_call =
                      ir_man->create_call_stmt(version_fnode, gc->args, function_id, BUILTIN_LOCINFO);
                  BB->Replace(stmt, version_call, true, AppM);
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Replaced " + STR(version_call));
                  modified = true;
               }
            }
         }
         else if(stmt->get_kind() == assign_stmt_K)
         {
            const auto ga = GetPointerS<assign_stmt>(stmt);
            const auto ce = GetPointer<const call_node>(ga->op1);
            if(ce)
            {
               const auto called_fnode = extract_fnode(ce->fn);
               const auto called_fname = ir_helper::GetFunctionName(called_fnode);
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Analizing assign_stmt_K " + called_fname);
               const auto replace_call = [&] {
                  const auto version_fnode = ForkFunction(called_fname, omp_info);
                  const auto version_ce = ir_man->CreateCallExpr(version_fnode, ce->args, BUILTIN_LOCINFO);
                  CGM.RemoveCallPoint(function_id, called_fnode->index, stmt->index);
                  TM->ReplaceIRNode(stmt, ga->op1, version_ce);
                  CallGraphManager::addCallPointAndExpand(already_visited, AppM, function_id, version_fnode->index,
                                                          stmt->index, FunctionEdgeInfo::CallType::direct_call,
                                                          DEBUG_LEVEL_NONE);
                  const auto fu_name = functions::GetFUName(version_fnode->index, HLSMgr);
                  HLSMgr->global_resource_constraints[std::make_pair(fu_name, LIBRARY_STD_FU)] = std::make_pair(1U, 1U);
               };
               if(shared_functions.count(called_fnode->index))
               {
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                 "---Function " + called_fname + " shared between threads.");
               }
               else if(called_fname == TOSTRING(KMP_GET_REDUCE_DATA) || called_fname == TOSTRING(KMP_GET_TID_FROM_GTID))
               {
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Replacing OMP " + STR(stmt));
                  replace_call();
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Replaced OMP " + STR(stmt));
                  modified = true;
               }
               else if(called_fname == TOSTRING(KMP_CS_GET_GTID) || called_fname == TOSTRING(KMP_CS_GET_TID))
               {
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Replacing OMP " + STR(stmt));
                  if(!omp_info || omp_info->context_count == 1U)
                  {
                     const auto rtype = ir_helper::GetFunctionReturnType(called_fnode);
                     const auto cval = omp_info ? (called_fname == TOSTRING(KMP_CS_GET_GTID) ? omp_info->global_idx :
                                                                                               omp_info->local_idx) :
                                                  0U;
                     CGM.RemoveCallPoint(function_id, called_fnode->index, stmt->index);
                     if(ga->predicate && ga->predicate->get_kind() != constant_int_val_node_K)
                     {
                        TM->ReplaceIRNode(stmt, ga->predicate, TM->CreateUniqueIntegerCst(1, ir_man->GetBooleanType()));
                        ga->predicate = nullptr;
                     }
                     TM->ReplaceIRNode(stmt, ga->op1, TM->CreateUniqueIntegerCst(cval, rtype));
                  }
                  else
                  {
                     replace_call();
                  }
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Replaced OMP " + STR(stmt));
                  modified = true;
               }
               else if(omp_info)
               {
                  const auto version_fnode = OMPCGExt::CloneFunction(called_fnode, omp_info);
                  if(version_fnode->index != called_fnode->index)
                  {
                     INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Replacing " + STR(stmt));
                     const auto version_ce = ir_man->CreateCallExpr(version_fnode, ce->args, BUILTIN_LOCINFO);
                     CGM.RemoveCallPoint(function_id, called_fnode->index, stmt->index);
                     TM->ReplaceIRNode(stmt, ga->op1, version_ce);
                     CallGraphManager::addCallPointAndExpand(already_visited, AppM, function_id, version_fnode->index,
                                                             stmt->index, FunctionEdgeInfo::CallType::direct_call,
                                                             DEBUG_LEVEL_NONE);
                     INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Replaced " + STR(stmt));
                     modified = true;
                  }
               }
            }
         }
      }
   }
   if(modified)
   {
      AppM->GetFunctionBehavior(function_id)->UpdateBBVersion();
      return DesignFlowStep_Status::SUCCESS;
   }
   return DesignFlowStep_Status::UNCHANGED;
}

ir_nodeRef OMPLowering::ForkFunction(const std::string& fname, const OMPInfoRef& omp_info) const
{
   const auto core_fname = fname + OMPCGExt::OMPForkSuffix(omp_info->fork_call_id, omp_info->core_id);
   auto core_fnode = TM->GetFunction(core_fname);
   if(!core_fnode)
   {
      const auto fork_fname = fname + OMPCGExt::OMPForkSuffix(omp_info->fork_call_id, 0);
      core_fnode = TM->GetFunction(fork_fname);
      THROW_ASSERT(core_fnode, "Expected " + fork_fname + " function to be present.");
      THROW_ASSERT(shared_infos[omp_info->fork_call_id].count(core_fnode->index),
                   "Expected function " + fname + " to be shared in fork call " + STR(omp_info->fork_call_id) + ".");
   }
   return core_fnode;
}
