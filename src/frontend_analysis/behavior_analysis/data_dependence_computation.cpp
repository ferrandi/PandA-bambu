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
 * @file data_dependence_computation.cpp
 * @brief Base class for different data dependence computation
 *
 * @author Marco Lattuada <marco.lattuada@polimi.it>
 *
 */

/// Autoheader include
#include "config_HAVE_EXPERIMENTAL.hpp"

/// Header include
#include "data_dependence_computation.hpp"

///. include
#include "Parameter.hpp"

/// behavior include
#include "application_manager.hpp"
#include "basic_block.hpp"
#include "function_behavior.hpp"
#include "op_graph.hpp"
#include "operations_graph_constructor.hpp"

/// tree include
#include "behavioral_helper.hpp"
#include "dbgPrintHelper.hpp"      // for DEBUG_LEVEL_
#include "string_manipulation.hpp" // for GET_CLASS
#include "tree_manager.hpp"
#include "tree_node.hpp"

#include "custom_map.hpp"
#include "custom_set.hpp"

DataDependenceComputation::DataDependenceComputation(const application_managerRef _AppM, unsigned int _function_id, const FrontendFlowStepType _frontend_flow_step_type, const DesignFlowManagerConstRef _design_flow_manager,
                                                     const ParameterConstRef _parameters)
    : FunctionFrontendFlowStep(_AppM, _function_id, _frontend_flow_step_type, _design_flow_manager, _parameters)
{
}

DataDependenceComputation::~DataDependenceComputation() = default;

DesignFlowStep_Status DataDependenceComputation::InternalExec()
{
   if(frontend_flow_step_type == SCALAR_SSA_DATA_FLOW_ANALYSIS)
   {
      return Computedependencies<unsigned int>(DFG_SCA_SELECTOR, FB_DFG_SCA_SELECTOR, ADG_SCA_SELECTOR, FB_ADG_SCA_SELECTOR);
   }
   else if(frontend_flow_step_type == VIRTUAL_AGGREGATE_DATA_FLOW_ANALYSIS or frontend_flow_step_type == MEMORY_DATA_FLOW_ANALYSIS
#if HAVE_EXPERIMENTAL && HAVE_ZEBU_BUILT
           or frontend_flow_step_type == REFINED_AGGREGATE_DATA_FLOW_ANALYSIS
#endif
   )
   {
      auto res = Computedependencies<unsigned int>(DFG_AGG_SELECTOR, FB_DFG_AGG_SELECTOR, ADG_AGG_SELECTOR, FB_ADG_AGG_SELECTOR);
      do_dependence_reduction();
      return res;
   }
#if HAVE_ZEBU_BUILT && HAVE_EXPERIMENTAL
   else if(frontend_flow_step_type == DYNAMIC_AGGREGATE_DATA_FLOW_ANALYSIS)
   {
      return Computedependencies<unsigned int>(DFG_AGG_SELECTOR, FB_DFG_AGG_SELECTOR, ADG_AGG_SELECTOR, FB_ADG_AGG_SELECTOR);
   }
#endif
   else
   {
      THROW_UNREACHABLE("Unexpected data flow analysis type");
   }
   return DesignFlowStep_Status::ABORTED;
}

static void ordered_dfs(unsigned u, const OpGraphConstRef avg, CustomUnorderedMap<vertex, unsigned>& pos, std::vector<vertex>& rev_pos, std::vector<bool>& vis, CustomUnorderedSet<std::pair<unsigned, unsigned>>& keep)
{
   vis[u] = true;
   CustomOrderedSet<unsigned> to;
   OutEdgeIterator ei, ei_end;
   auto statement = rev_pos.at(u);
   for(boost::tie(ei, ei_end) = boost::out_edges(statement, *avg); ei != ei_end; ei++)
   {
      auto vi = boost::target(*ei, *avg);
      if(pos.find(vi) != pos.end())
         to.insert(pos.find(vi)->second);
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
   const BBGraphRef bb_fcfg = function_behavior->GetBBGraph(FunctionBehavior::BB);
   const OpGraphConstRef avg = function_behavior->CGetOpGraph(FunctionBehavior::AGG_VIRTUALG);
   if(parameters->getOption<bool>(OPT_print_dot))
   {
      std::string file_name;
      file_name = "AGG_VIRTUALG.dot";
      function_behavior->CGetOpGraph(FunctionBehavior::AGG_VIRTUALG)->WriteDot(file_name, 1);
   }
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---do_dependence_reduction");
   VertexIterator basic_block, basic_block_end;
   for(boost::tie(basic_block, basic_block_end) = boost::vertices(*bb_fcfg); basic_block != basic_block_end; basic_block++)
   {
      const auto bb_node_info = bb_fcfg->CGetBBNodeInfo(*basic_block);
      CustomUnorderedMap<vertex, unsigned> pos;
      std::vector<vertex> rev_pos;
      unsigned posIndex = 0;
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Analyzing BB" + STR(bb_node_info->get_bb_index()));
      for(const auto statement : bb_node_info->statements_list)
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Analyzing operation " + GET_NAME(avg, statement));
         pos[statement] = posIndex;
         ++posIndex;
         rev_pos.push_back(statement);
      }
      std::vector<bool> vis(posIndex, false);
      const auto n_stmts = bb_node_info->statements_list.size();
      CustomUnorderedSet<std::pair<unsigned, unsigned>> keep;
      for(posIndex = 0; posIndex < n_stmts; ++posIndex)
      {
         if(!vis.at(posIndex))
         {
            ordered_dfs(posIndex, avg, pos, rev_pos, vis, keep);
            for(unsigned posIndex0 = posIndex + 1; posIndex0 < n_stmts; ++posIndex0)
               if(vis.at(posIndex0))
                  vis[posIndex0] = false;
         }
      }
      for(const auto statement : bb_node_info->statements_list)
      {
         vertex vi;
         OutEdgeIterator ei, ei_end;
         std::list<EdgeDescriptor> to_be_removed;
         for(boost::tie(ei, ei_end) = boost::out_edges(statement, *avg); ei != ei_end; ei++)
         {
            vi = boost::target(*ei, *avg);
            if(pos.find(vi) != pos.end())
            {
               auto key = std::make_pair(pos.at(statement), pos.at(vi));
               if(keep.find(key) == keep.end())
               {
                  to_be_removed.push_back(*ei);
               }
            }
         }
         for(auto e0 : to_be_removed)
         {
            function_behavior->ogc->RemoveSelector(e0, DFG_AGG_SELECTOR);
            function_behavior->ogc->RemoveSelector(e0, ADG_AGG_SELECTOR);
         }
      }
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Analyzed " + STR(bb_node_info->get_bb_index()));
   }
   function_behavior->ogc->CompressEdges();
   if(parameters->getOption<bool>(OPT_print_dot))
   {
      std::string file_name;
      file_name = "AGG_VIRTUALG-post.dot";
      function_behavior->CGetOpGraph(FunctionBehavior::AGG_VIRTUALG)->WriteDot(file_name, 1);
      file_name = "OP_FFLSAODG2.dot";
      function_behavior->CGetOpGraph(FunctionBehavior::FFLSAODG)->WriteDot(file_name, 1);
   }
}

template <typename type>
DesignFlowStep_Status DataDependenceComputation::Computedependencies(const int dfg_selector, const int fb_dfg_selector, const int adg_selector, const int fb_adg_selector)
{
   const auto TM = AppM->get_tree_manager();
   const OpGraphConstRef cfg = function_behavior->CGetOpGraph(FunctionBehavior::CFG);
   const BehavioralHelperConstRef behavioral_helper = function_behavior->CGetBehavioralHelper();
#ifndef NDEBUG
   const std::string function_name = behavioral_helper->get_function_name();
#endif
   // Maps between a variable and its definitions
   std::map<type, CustomOrderedSet<vertex>> defs, overs;
   VertexIterator vi, vi_end;
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Computing definitions");
   for(boost::tie(vi, vi_end) = boost::vertices(*cfg); vi != vi_end; vi++)
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Definitions in " + GET_NAME(cfg, *vi));
      const auto& local_defs = GetVariables<type>(*vi, FunctionBehavior_VariableAccessType::DEFINITION);
      for(auto local_def : local_defs)
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---" + TM->get_tree_node_const(local_def)->ToString());
         defs[local_def].insert(*vi);
      }
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--");
   }
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Computed definitions");
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Computing overwritings");
   for(boost::tie(vi, vi_end) = boost::vertices(*cfg); vi != vi_end; vi++)
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Overwritings in " + GET_NAME(cfg, *vi));
      const auto& local_overs = GetVariables<type>(*vi, FunctionBehavior_VariableAccessType::OVER);
      for(auto local_over : local_overs)
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---" + TM->get_tree_node_const(local_over)->ToString());
         overs[local_over].insert(*vi);
      }
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--");
   }
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Computed overwritings");
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Computing dependencies");
   for(boost::tie(vi, vi_end) = boost::vertices(*cfg); vi != vi_end; vi++)
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Computing anti and data dependencies of vertex " + GET_NAME(cfg, *vi));
      for(auto local_use : GetVariables<type>(*vi, FunctionBehavior_VariableAccessType::USE))
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Considering use of " + TM->get_tree_node_const(local_use)->ToString());
         if(defs.find(local_use) != defs.end())
         {
            for(const auto this_def : defs.find(local_use)->second)
            {
               const bool forward_dependence = function_behavior->CheckReachability(this_def, *vi);
               const bool feedback_dependence = function_behavior->CheckReachability(*vi, this_def);
               THROW_ASSERT(!(forward_dependence and feedback_dependence), "Dependence between operation " + GET_NAME(cfg, this_def) + " and " + GET_NAME(cfg, *vi) + " is in both the direction");
               if(forward_dependence)
               {
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Adding data dependence " + GET_NAME(cfg, this_def) + "-->" + GET_NAME(cfg, *vi));
                  function_behavior->ogc->AddEdge(this_def, *vi, dfg_selector);
                  function_behavior->ogc->add_edge_info(this_def, *vi, DFG_SELECTOR, local_use);
                  if(function_behavior->CheckFeedbackReachability(*vi, this_def))
                  {
                     function_behavior->ogc->AddEdge(*vi, this_def, fb_adg_selector);
                     /// NOTE: label associated with forward selector also on feedback edge
                     function_behavior->ogc->add_edge_info(this_def, *vi, ADG_SELECTOR, local_use);
                  }
               }

               if(feedback_dependence)
               {
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Checking for feedback data dependence " + GET_NAME(cfg, this_def) + "-->" + GET_NAME(cfg, *vi));
                  if(*vi != this_def)
                  {
                     function_behavior->ogc->AddEdge(*vi, this_def, adg_selector);
                     function_behavior->ogc->add_edge_info(*vi, this_def, ADG_SELECTOR, local_use);
                  }
                  if(function_behavior->CheckFeedbackReachability(this_def, *vi))
                  {
                     function_behavior->ogc->AddEdge(this_def, *vi, fb_dfg_selector);
                     /// NOTE: label associated with forward selector also on feedback edge
                     function_behavior->ogc->add_edge_info(this_def, *vi, DFG_SELECTOR, local_use);
                  }
               }

               if(*vi == this_def)
               {
                  function_behavior->ogc->AddEdge(*vi, *vi, fb_dfg_selector);
                  function_behavior->ogc->add_edge_info(*vi, *vi, DFG_SELECTOR, local_use);
               }
            }
         }
         if(overs.find(local_use) != overs.end())
         {
            for(const auto this_over : overs.find(local_use)->second)
            {
               const bool dependence = function_behavior->CheckReachability(*vi, this_over);
               if(dependence)
               {
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Adding anti dependence " + GET_NAME(cfg, *vi) + "-->" + GET_NAME(cfg, this_over));
                  function_behavior->ogc->AddEdge(*vi, this_over, adg_selector);
                  function_behavior->ogc->add_edge_info(*vi, this_over, ADG_SELECTOR, local_use);
               }
            }
         }
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--");
      }
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Computed anti and data dependencies of vertex " + GET_NAME(cfg, *vi));
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Computing output dependencies of vertex " + GET_NAME(cfg, *vi));
      for(auto local_def : GetVariables<type>(*vi, FunctionBehavior_VariableAccessType::OVER))
      {
         if(defs.find(local_def) != defs.end())
         {
            for(const auto this_def : defs.find(local_def)->second)
            {
               const bool forward_dependence = function_behavior->CheckReachability(this_def, *vi);
               if(forward_dependence)
               {
                  function_behavior->ogc->AddEdge(this_def, *vi, ODG_AGG_SELECTOR);
                  function_behavior->ogc->add_edge_info(this_def, *vi, ODG_SELECTOR, local_def);
                  if(function_behavior->CheckFeedbackReachability(*vi, this_def))
                  {
                     function_behavior->ogc->AddEdge(*vi, this_def, FB_ODG_AGG_SELECTOR);
                     /// NOTE: label associated with forward selector also on feedback edge
                     function_behavior->ogc->add_edge_info(*vi, this_def, ODG_SELECTOR, local_def);
                  }
               }
            }
         }
      }
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Computed output dependencies of vertex " + GET_NAME(cfg, *vi));
   }
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Computed dependencies");
   if(parameters->getOption<bool>(OPT_print_dot))
   {
      std::string file_name;
      file_name = "OP_DFG.dot";
      function_behavior->CGetOpGraph(FunctionBehavior::DFG)->WriteDot(file_name, 1);
      file_name = "OP_FDFG.dot";
      function_behavior->CGetOpGraph(FunctionBehavior::FDFG)->WriteDot(file_name, 1);
      file_name = "OP_ADG.dot";
      function_behavior->CGetOpGraph(FunctionBehavior::ADG)->WriteDot(file_name, 1);
      file_name = "OP_FADG.dot";
      function_behavior->CGetOpGraph(FunctionBehavior::FADG)->WriteDot(file_name, 1);
      file_name = "OP_ODG.dot";
      function_behavior->CGetOpGraph(FunctionBehavior::ODG)->WriteDot(file_name, 1);
      file_name = "OP_FODG.dot";
      function_behavior->CGetOpGraph(FunctionBehavior::FODG)->WriteDot(file_name, 1);
      file_name = "OP_SAODG1.dot";
      function_behavior->CGetOpGraph(FunctionBehavior::SAODG)->WriteDot(file_name, 1);
   }
#ifndef NDEBUG
   try
   {
      const OpGraphConstRef dfg = function_behavior->CGetOpGraph(FunctionBehavior::DFG);
      std::deque<vertex> vertices;
      boost::topological_sort(*dfg, std::front_inserter(vertices));
   }
   catch(const char* msg)
   {
      THROW_UNREACHABLE("dfg graph of function " + function_name + " is not acyclic");
   }
   catch(const std::string& msg)
   {
      THROW_UNREACHABLE("dfg graph of function " + function_name + " is not acyclic");
   }
   catch(const std::exception& ex)
   {
      THROW_UNREACHABLE("dfg graph of function " + function_name + " is not acyclic");
   }
   catch(...)
   {
      THROW_UNREACHABLE("dfg graph of function " + function_name + " is not acyclic");
   }
#endif
   return DesignFlowStep_Status::SUCCESS;
}

template <>
CustomSet<unsigned int> DataDependenceComputation::GetVariables(const vertex statement, const FunctionBehavior_VariableAccessType variable_access_type) const
{
   FunctionBehavior_VariableType variable_type = FunctionBehavior_VariableType::UNKNOWN;
   if(frontend_flow_step_type == VIRTUAL_AGGREGATE_DATA_FLOW_ANALYSIS)
   {
      variable_type = FunctionBehavior_VariableType::VIRTUAL;
   }
   else if(frontend_flow_step_type == SCALAR_SSA_DATA_FLOW_ANALYSIS)
   {
      variable_type = FunctionBehavior_VariableType::SCALAR;
   }
   else if(frontend_flow_step_type == MEMORY_DATA_FLOW_ANALYSIS)
   {
      variable_type = FunctionBehavior_VariableType::MEMORY;
   }
#if HAVE_EXPERIMENTAL && HAVE_ZEBU_BUILT
   else if(frontend_flow_step_type == REFINED_AGGREGATE_DATA_FLOW_ANALYSIS)
   {
      variable_type = FunctionBehavior_VariableType::AGGREGATE;
   }
   else if(frontend_flow_step_type == DYNAMIC_AGGREGATE_DATA_FLOW_ANALYSIS)
   {
      THROW_UNREACHABLE("Unexpected dynamic aggregate data flow analysis");
   }
#endif
   else
   {
      THROW_UNREACHABLE("Unexpected data flow analysis type");
   }
   return function_behavior->CGetOpGraph(FunctionBehavior::CFG)->CGetOpNodeInfo(statement)->GetVariables(variable_type, variable_access_type);
}

#if HAVE_EXPERIMENTAL && HAVE_ZEBU_BUILT
template <>
CustomSet<MemoryAddress> DataDependenceComputation::GetVariables(const vertex statement, const FunctionBehavior_VariableAccessType variable_access_type) const
{
   THROW_ASSERT(frontend_flow_step_type == DYNAMIC_AGGREGATE_DATA_FLOW_ANALYSIS, "Unexpected data flow analysis type");
   return function_behavior->CGetOpGraph(FunctionBehavior::CFG)->CGetOpNodeInfo(statement)->GetDynamicMemoryLocations(variable_access_type);
}
#endif
