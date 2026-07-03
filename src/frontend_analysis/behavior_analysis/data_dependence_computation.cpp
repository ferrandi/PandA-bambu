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
 * @file data_dependence_computation.cpp
 * @brief Base class for different data dependence computation
 *
 * @author Marco Lattuada <marco.lattuada@polimi.it>
 *
 */
#include "data_dependence_computation.hpp"

#include "Parameter.hpp"
#include "application_manager.hpp"
#include "basic_block.hpp"
#include "behavioral_helper.hpp"
#include "custom_map.hpp"
#include "custom_set.hpp"
#include "dbgPrintHelper.hpp"
#include "function_behavior.hpp"
#include "ir_helper.hpp"
#include "ir_manager.hpp"
#include "ir_node.hpp"
#include "op_graph.hpp"
#include "string_manipulation.hpp"

#include <utility>

DataDependenceComputation::DataDependenceComputation(const application_managerRef _AppM, unsigned int _function_id,
                                                     const FrontendFlowStepType _frontend_flow_step_type,
                                                     const DesignFlowManager& _design_flow_manager,
                                                     const ParameterConstRef _parameters)
    : FunctionFrontendFlowStep(_AppM, _function_id, _frontend_flow_step_type, _design_flow_manager, _parameters)
{
}

DesignFlowStep_Status DataDependenceComputation::InternalExec()
{
   if(frontend_flow_step_type == SCALAR_SSA_DATA_FLOW_ANALYSIS)
   {
      return Computedependencies(DFG_SCA_SELECTOR, FB_DFG_SCA_SELECTOR, ADG_SCA_SELECTOR, FB_ADG_SCA_SELECTOR);
   }
   else if(frontend_flow_step_type == VIRTUAL_AGGREGATE_DATA_FLOW_ANALYSIS)
   {
      auto res = Computedependencies(DFG_AGG_SELECTOR, FB_DFG_AGG_SELECTOR, ADG_AGG_SELECTOR, FB_ADG_AGG_SELECTOR);
      do_dependence_reduction();
      return res;
   }
   else
   {
      THROW_UNREACHABLE("Unexpected data flow analysis type");
   }
   return DesignFlowStep_Status::ABORTED;
}

static void ordered_dfs(unsigned u, const OpGraph& avg, CustomUnorderedMap<OpGraph::vertex_descriptor, unsigned>& pos,
                        std::vector<OpGraph::vertex_descriptor>& rev_pos, std::vector<bool>& vis,
                        CustomUnorderedSet<std::pair<unsigned, unsigned>>& keep)
{
   vis[u] = true;
   CustomOrderedSet<unsigned> to;
   auto statement = rev_pos.at(u);
   for(const auto& ei : avg.out_edges(statement))
   {
      auto vi = avg.target(ei);
      if(pos.find(vi) != pos.end())
      {
         to.insert(pos.find(vi)->second);
      }
   }
   for(auto dest : to)
   {
      if(!vis[dest])
      {
         keep.insert(std::make_pair(u, dest));
         ordered_dfs(dest, avg, pos, rev_pos, vis, keep);
      }
   }
}

void DataDependenceComputation::do_dependence_reduction()
{
   const auto bb_fcfg = function_behavior->GetBBGraph(FunctionBehavior::BB);
   const auto avg = function_behavior->GetOpGraph(FunctionBehavior::AGG_VIRTUALG);
   if(parameters->getOption<bool>(OPT_print_dot))
   {
      function_behavior->GetOpGraph(FunctionBehavior::AGG_VIRTUALG)
          .writeDot(function_behavior->GetDotPath() / "AGG_VIRTUALG.dot", 1);
   }
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---do_dependence_reduction");
   for(const auto basic_block : bb_fcfg.vertices())
   {
      const auto& bb_node_info = bb_fcfg.CGetNodeInfo(basic_block);
      CustomUnorderedMap<BBGraph::vertex_descriptor, unsigned> pos;
      std::vector<BBGraph::vertex_descriptor> rev_pos;
      unsigned posIndex = 0;
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Analyzing BB" + STR(bb_node_info.get_bb_index()));
      for(const auto statement : bb_node_info.statements_list)
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                        "---Analyzing operation " + avg.CGetNodeInfo(statement).vertex_name);
         pos[statement] = posIndex;
         ++posIndex;
         rev_pos.push_back(statement);
      }
      std::vector<bool> vis(posIndex, false);
      const auto n_stmts = bb_node_info.statements_list.size();
      CustomUnorderedSet<std::pair<unsigned, unsigned>> keep;
      for(posIndex = 0; posIndex < n_stmts; ++posIndex)
      {
         if(!vis.at(posIndex))
         {
            ordered_dfs(posIndex, avg, pos, rev_pos, vis, keep);
            for(unsigned posIndex0 = posIndex + 1; posIndex0 < n_stmts; ++posIndex0)
            {
               if(vis.at(posIndex0))
               {
                  vis[posIndex0] = false;
               }
            }
         }
      }
      for(const auto statement : bb_node_info.statements_list)
      {
         std::list<OpGraph::edge_descriptor> to_be_removed;
         for(const auto ei : avg.out_edges(statement))
         {
            auto vi = avg.target(ei);
            if(pos.find(vi) != pos.end())
            {
               auto key = std::make_pair(pos.at(statement), pos.at(vi));
               if(keep.find(key) == keep.end())
               {
                  to_be_removed.push_back(ei);
               }
            }
         }
         for(auto e0 : to_be_removed)
         {
            function_behavior->ogc->RemoveSelector(e0, DFG_AGG_SELECTOR);
            function_behavior->ogc->RemoveSelector(e0, ADG_AGG_SELECTOR);
         }
      }
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Analyzed " + STR(bb_node_info.get_bb_index()));
   }
   function_behavior->ogc->CompressEdges();
   if(parameters->getOption<bool>(OPT_print_dot))
   {
      const auto dot_path = function_behavior->GetDotPath();
      function_behavior->GetOpGraph(FunctionBehavior::AGG_VIRTUALG).writeDot(dot_path / "AGG_VIRTUALG-post.dot", 1);
      function_behavior->GetOpGraph(FunctionBehavior::FFLSAODG).writeDot(dot_path / "OP_FFLSAODG2.dot", 1);
   }
}

DesignFlowStep_Status DataDependenceComputation::Computedependencies(const int dfg_selector, const int fb_dfg_selector,
                                                                     const int adg_selector, const int fb_adg_selector)
{
   const auto TM = AppM->get_ir_manager();
   const auto cfg = function_behavior->GetOpGraph(FunctionBehavior::CFG);
   const auto behavioral_helper = function_behavior->CGetBehavioralHelper();
#ifndef NDEBUG
   const std::string function_name = behavioral_helper->GetFunctionName();
#endif
   // Maps between a variable and its definitions
   std::map<unsigned int, CustomOrderedSet<OpGraph::vertex_descriptor>> defs, overs;
   using ReachabilityKey = std::pair<OpGraph::vertex_descriptor, OpGraph::vertex_descriptor>;
   CustomUnorderedMapUnstable<ReachabilityKey, bool> reachability_cache;
   const auto check_reachability_cached = [&](const OpGraph::vertex_descriptor from,
                                              const OpGraph::vertex_descriptor to) -> bool {
      const ReachabilityKey key(from, to);
      const auto cached = reachability_cache.find(key);
      if(cached != reachability_cache.end())
      {
         return cached->second;
      }
      const bool result = function_behavior->CheckReachability(from, to);
      reachability_cache.emplace(key, result);
      return result;
   };
   CustomUnorderedMapUnstable<ReachabilityKey, bool> feedback_reachability_cache;
   const auto check_feedback_reachability_cached = [&](const OpGraph::vertex_descriptor from,
                                                       const OpGraph::vertex_descriptor to) -> bool {
      const ReachabilityKey key(from, to);
      const auto cached = feedback_reachability_cache.find(key);
      if(cached != feedback_reachability_cache.end())
      {
         return cached->second;
      }
      const bool result = function_behavior->CheckFeedbackReachability(from, to);
      feedback_reachability_cache.emplace(key, result);
      return result;
   };
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Computing definitions");
   for(const auto& v : cfg.vertices())
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Definitions in " + cfg.CGetNodeInfo(v).vertex_name);
      const auto& local_defs = getVariables(v, VariableAccessType::DEFINITION);
      for(auto local_def : local_defs)
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---" + TM->GetIRNode(local_def)->ToString());
         defs[local_def].insert(v);
      }
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--");
   }
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Computed definitions");
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Computing overwritings");
   for(const auto& v : cfg.vertices())
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Overwritings in " + cfg.CGetNodeInfo(v).vertex_name);
      const auto& local_overs = getVariables(v, VariableAccessType::OVER);
      for(auto local_over : local_overs)
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---" + TM->GetIRNode(local_over)->ToString());
         overs[local_over].insert(v);
      }
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--");
   }
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Computed overwritings");

   // Build cited-variable-to-virtual-overwriting-vertices map for cross-chain
   // anti-dependency detection. After function inlining, independent VSSA
   // chains may exist for the same physical memory variable. Since virtual SSA
   // nodes carry no var field, we match operations through cited_variables
   // (the physical variables each operation accesses).
   std::map<unsigned int, CustomOrderedSet<OpGraph::vertex_descriptor>> cited_var_to_virtual_over_vertices;
   for(const auto& v : cfg.vertices())
   {
      const auto& v_virtual_overs = cfg.CGetNodeInfo(v).getVariables(VariableType::VIRTUAL, VariableAccessType::OVER);
      if(!v_virtual_overs.empty())
      {
         for(const auto cited_var : cfg.CGetNodeInfo(v).cited_variables)
         {
            cited_var_to_virtual_over_vertices[cited_var].insert(v);
         }
      }
   }

   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Computing dependencies");
   for(const auto& v : cfg.vertices())
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                     "-->Computing anti and data dependencies of vertex " + cfg.CGetNodeInfo(v).vertex_name);
      for(auto local_use : getVariables(v, VariableAccessType::USE))
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                        "-->Considering use of " + TM->GetIRNode(local_use)->ToString());
         auto local_use_node = TM->GetIRNode(local_use);
         if(defs.find(local_use) != defs.end())
         {
            // TODO: iteration of vertex set may cause non-determinism
            for(const auto this_def : defs.find(local_use)->second)
            {
               const bool forward_dependence = check_reachability_cached(this_def, v);
               const bool feedback_dependence = check_reachability_cached(v, this_def);
               THROW_ASSERT(!(forward_dependence and feedback_dependence),
                            "Dependence between operation " + cfg.CGetNodeInfo(this_def).vertex_name + " and " +
                                cfg.CGetNodeInfo(v).vertex_name + " is in both the direction");
               if(forward_dependence)
               {
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                 "---Adding data dependence " + cfg.CGetNodeInfo(this_def).vertex_name + "-->" +
                                     cfg.CGetNodeInfo(v).vertex_name);
                  function_behavior->ogc->AddEdge(this_def, v, dfg_selector);
                  function_behavior->ogc->add_edge_info(this_def, v, DFG_SELECTOR, local_use);
                  if(ir_helper::IsVirtual(local_use_node) && check_feedback_reachability_cached(v, this_def))
                  {
                     INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                    "---Adding fb_adg_selector dependence " + cfg.CGetNodeInfo(v).vertex_name + "-->" +
                                        cfg.CGetNodeInfo(this_def).vertex_name);
                     function_behavior->ogc->AddEdge(v, this_def, fb_adg_selector);
                     /// NOTE: label associated with forward selector also on feedback edge
                     function_behavior->ogc->add_edge_info(v, this_def, ADG_SELECTOR, local_use);
                  }
               }

               if(feedback_dependence)
               {
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                 "---Checking for feedback data dependence " + cfg.CGetNodeInfo(this_def).vertex_name +
                                     "-->" + cfg.CGetNodeInfo(v).vertex_name);
                  if(v != this_def && ir_helper::IsVirtual(local_use_node))
                  {
                     INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                    "---Adding adg_selector dependence " + cfg.CGetNodeInfo(v).vertex_name + "-->" +
                                        cfg.CGetNodeInfo(this_def).vertex_name);
                     function_behavior->ogc->AddEdge(v, this_def, adg_selector);
                     function_behavior->ogc->add_edge_info(v, this_def, ADG_SELECTOR, local_use);
                  }
                  if(check_feedback_reachability_cached(this_def, v))
                  {
                     INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                    "---Adding fb_dfg_selector dependence " + cfg.CGetNodeInfo(this_def).vertex_name +
                                        "-->" + cfg.CGetNodeInfo(v).vertex_name);
                     function_behavior->ogc->AddEdge(this_def, v, fb_dfg_selector);
                     /// NOTE: label associated with forward selector also on feedback edgeADG_SELECTOR
                     /// (ADG_SCA_SELECTADG_SELECTOR (ADG_SCA_SELECTOR | ADG_AGG_SELECTOR) FeedOR | ADG_AGG_SELECTOR)
                     /// Feed
                     function_behavior->ogc->add_edge_info(this_def, v, DFG_SELECTOR, local_use);
                  }
               }

               if(v == this_def)
               {
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                 "---Adding2 fb_dfg_selector dependence " + cfg.CGetNodeInfo(v).vertex_name + "-->" +
                                     cfg.CGetNodeInfo(v).vertex_name);
                  function_behavior->ogc->AddEdge(v, v, fb_dfg_selector);
                  function_behavior->ogc->add_edge_info(v, v, DFG_SELECTOR, local_use);
               }
            }
         }
         if(ir_helper::IsVirtual(local_use_node))
         {
            // Standard VSSA-based anti-dependency: USE -> OVER of the same VSSA
            if(overs.count(local_use))
            {
               for(const auto this_over : overs.at(local_use))
               {
                  const bool dependence = check_reachability_cached(v, this_over);
                  if(dependence)
                  {
                     INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                    "---Adding adg_selector dependence " + cfg.CGetNodeInfo(v).vertex_name + "-->" +
                                        cfg.CGetNodeInfo(this_over).vertex_name);
                     function_behavior->ogc->AddEdge(v, this_over, adg_selector);
                     function_behavior->ogc->add_edge_info(v, this_over, ADG_SELECTOR, local_use);
                  }
               }
            }
            // Cross-chain anti-dependency: after function inlining, independent
            // VSSA chains for the same physical memory may exist. Match operations
            // through cited_variables (the physical variables each operation accesses).
            for(const auto cited_var : cfg.CGetNodeInfo(v).cited_variables)
            {
               const auto cvit = cited_var_to_virtual_over_vertices.find(cited_var);
               if(cvit != cited_var_to_virtual_over_vertices.end())
               {
                  for(const auto this_over : cvit->second)
                  {
                     if(this_over == v)
                     {
                        continue;
                     }
                     const bool dependence = check_reachability_cached(v, this_over);
                     if(dependence)
                     {
                        INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                       "---Adding cross-chain adg_selector dependence " +
                                           cfg.CGetNodeInfo(v).vertex_name + "-->" +
                                           cfg.CGetNodeInfo(this_over).vertex_name);
                        function_behavior->ogc->AddEdge(v, this_over, adg_selector);
                        function_behavior->ogc->add_edge_info(v, this_over, ADG_SELECTOR, local_use);
                     }
                  }
               }
            }
         }
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--");
      }
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                     "<--Computed anti and data dependencies of vertex " + cfg.CGetNodeInfo(v).vertex_name);
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                     "-->Computing output dependencies of vertex " + cfg.CGetNodeInfo(v).vertex_name);
      for(auto local_over : getVariables(v, VariableAccessType::OVER))
      {
         if(defs.find(local_over) != defs.end())
         {
            // TODO: iteration of vertex set may cause non-determinism
            for(const auto this_def : defs.find(local_over)->second)
            {
               const bool forward_dependence = check_reachability_cached(this_def, v);
               if(forward_dependence)
               {
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                 "---Adding output dependence " + cfg.CGetNodeInfo(this_def).vertex_name + "-->" +
                                     cfg.CGetNodeInfo(v).vertex_name);
                  function_behavior->ogc->AddEdge(this_def, v, ODG_AGG_SELECTOR);
                  function_behavior->ogc->add_edge_info(this_def, v, ODG_SELECTOR, local_over);
                  if(check_feedback_reachability_cached(v, this_def))
                  {
                     INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                    "---Adding FB_ODG_AGG_SELECTOR dependence " + cfg.CGetNodeInfo(v).vertex_name +
                                        "-->" + cfg.CGetNodeInfo(this_def).vertex_name);
                     function_behavior->ogc->AddEdge(v, this_def, FB_ODG_AGG_SELECTOR);
                     /// NOTE: label associated with forward selector also on feedback edge
                     function_behavior->ogc->add_edge_info(v, this_def, ODG_SELECTOR, local_over);
                  }
               }
            }
         }
         auto local_over_node = TM->GetIRNode(local_over);
         if(overs.find(local_over) != overs.end() && ir_helper::IsVirtual(local_over_node))
         {
            for(const auto this_over : overs.find(local_over)->second)
            {
               if(v != this_over && check_feedback_reachability_cached(v, this_over))
               {
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                 "---Adding FB_ODG_AGG_SELECTOR dependence " + cfg.CGetNodeInfo(v).vertex_name + "-->" +
                                     cfg.CGetNodeInfo(this_over).vertex_name);
                  function_behavior->ogc->AddEdge(v, this_over, FB_ODG_AGG_SELECTOR);
               }
            }
         }
      }
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                     "<--Computed output dependencies of vertex " + cfg.CGetNodeInfo(v).vertex_name);
   }
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Computed dependencies");
   if(parameters->getOption<bool>(OPT_print_dot))
   {
      const auto dot_path = function_behavior->GetDotPath();
      function_behavior->GetOpGraph(FunctionBehavior::DFG).writeDot(dot_path / "OP_DFG.dot", 1);
      function_behavior->GetOpGraph(FunctionBehavior::FDFG).writeDot(dot_path / "OP_FDFG.dot", 1);
      function_behavior->GetOpGraph(FunctionBehavior::ADG).writeDot(dot_path / "OP_ADG.dot", 1);
      function_behavior->GetOpGraph(FunctionBehavior::FADG).writeDot(dot_path / "OP_FADG.dot", 1);
      function_behavior->GetOpGraph(FunctionBehavior::ODG).writeDot(dot_path / "OP_ODG.dot", 1);
      function_behavior->GetOpGraph(FunctionBehavior::FODG).writeDot(dot_path / "OP_FODG.dot", 1);
      function_behavior->GetOpGraph(FunctionBehavior::SAODG).writeDot(dot_path / "OP_SAODG1.dot", 1);
   }
#ifndef NDEBUG
   try
   {
      const auto dfg = function_behavior->GetOpGraph(FunctionBehavior::DFG);
      std::list<OpGraph::vertex_descriptor> vertices;
      dfg.TopologicalSort(vertices);
   }
   catch(...)
   {
      THROW_UNREACHABLE("dfg graph of function " + function_name + " is not acyclic");
   }
#endif
   return DesignFlowStep_Status::SUCCESS;
}

CustomSet<unsigned int> DataDependenceComputation::getVariables(gc_vertex_descriptor statement,
                                                                const VariableAccessType variable_access_type) const
{
   VariableType variable_type = VariableType::UNKNOWN;
   if(frontend_flow_step_type == VIRTUAL_AGGREGATE_DATA_FLOW_ANALYSIS)
   {
      variable_type = VariableType::VIRTUAL;
   }
   else if(frontend_flow_step_type == SCALAR_SSA_DATA_FLOW_ANALYSIS)
   {
      variable_type = VariableType::SCALAR;
   }
   else
   {
      THROW_UNREACHABLE("Unexpected data flow analysis type");
   }
   return function_behavior->GetOpGraph(FunctionBehavior::CFG)
       .CGetNodeInfo(statement)
       .getVariables(variable_type, variable_access_type);
}
