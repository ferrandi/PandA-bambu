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
 * @file design_flow_manager.cpp
 * @brief Wrapper of design_flow
 *
 * @author Marco Lattuada <lattuada@elet.polimi.it>
 * @author Michele Fiorito <michele.fiorito@polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *
 */
#include "design_flow_manager.hpp"

#include "Parameter.hpp"
#include "cpu_stats.hpp"
#include "cpu_time.hpp"
#include "custom_set.hpp"
#include "dbgPrintHelper.hpp"
#include "design_flow_aux_step.hpp"
#include "design_flow_graph.hpp"
#include "design_flow_step.hpp"
#include "design_flow_step_factory.hpp"
#include "exceptions.hpp"
#include "string_manipulation.hpp"

#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/filtered_graph.hpp>
#include <boost/iterator/filter_iterator.hpp>
#include <boost/iterator/iterator_facade.hpp>
#include <boost/tuple/tuple.hpp>

#include <iterator>
#include <list>
#include <utility>

#include "config_HAVE_ASSERTS.hpp"
#include "config_HAVE_UNORDERED.hpp"

#if !HAVE_UNORDERED && !defined(NDEBUG)
#include <random>
#endif

DesignFlowStepPrioritySet::DesignFlowStepPrioritySet(const DesignFlowGraphRef& dfg) : _dfg(dfg)
{
}

DesignFlowStepPrioritySet::key_t DesignFlowStepPrioritySet::compute_key(const vertex v) const
{
   key_t idx = 0;
   const auto info = _dfg->CGetDesignFlowStepInfo(v);
   idx += info->design_flow_step->IsComposed() ? 0 : absl::MakeUint128(2, 0);
   idx += (info->status == DesignFlowStep_Status::SKIPPED || info->status == DesignFlowStep_Status::UNNECESSARY) ?
              absl::MakeUint128(1, 0) :
              0;
   idx +=
#if HAVE_UNORDERED
       static_cast<unsigned long>(v)
#else
       info->design_flow_step->GetSignature()
#endif
       ;
   return idx;
}

bool DesignFlowStepPrioritySet::insert(const vertex v)
{
   const auto k_it = _keys_map.find(v);
   if(k_it == _keys_map.end())
   {
      const auto v_key = compute_key(v);
      _keys_map.emplace(v, v_key);
      _steps_map.emplace(v_key, v);
      return true;
   }
   return false;
}

void DesignFlowStepPrioritySet::insert_or_assign(const vertex v)
{
   const auto k_it = _keys_map.find(v);
   const auto v_key = compute_key(v);
   if(k_it != _keys_map.end())
   {
      if(k_it->second == v_key)
      {
         return;
      }
      _steps_map.erase(k_it->second);
      _keys_map.erase(k_it);
   }
   _keys_map.emplace(v, v_key);
   _steps_map.emplace(v_key, v);
}

bool DesignFlowStepPrioritySet::Update(const vertex v)
{
   const auto k_it = _keys_map.find(v);
   if(k_it != _keys_map.end())
   {
      const auto new_key = compute_key(v);
      if(new_key != k_it->second)
      {
         _steps_map.erase(k_it->second);
         _keys_map.erase(k_it);
         _keys_map.emplace(v, new_key);
         _steps_map.emplace(new_key, v);
      }
      return true;
   }
   return false;
}

vertex DesignFlowStepPrioritySet::Extract(map_t::size_type pos)
{
   THROW_ASSERT(pos < size(), "");
   const auto it = std::next(_steps_map.begin(), static_cast<ptrdiff_t>(pos));
   const auto v = it->second;
   _keys_map.erase(v);
   _steps_map.erase(it);
   return v;
}

DesignFlowStepPrioritySet::map_t::size_type DesignFlowStepPrioritySet::size() const
{
   return _steps_map.size();
}

DesignFlowStepPrioritySet::map_t::const_iterator DesignFlowStepPrioritySet::begin() const
{
   return _steps_map.begin();
}

DesignFlowStepPrioritySet::map_t::const_iterator DesignFlowStepPrioritySet::end() const
{
   return _steps_map.end();
}

DesignFlowManager::DesignFlowManager(const ParameterConstRef _parameters)
    : design_flow_graphs_collection(new DesignFlowGraphsCollection(_parameters)),
      design_flow_graph(new DesignFlowGraph(design_flow_graphs_collection, DesignFlowGraph::DEPENDENCE_SELECTOR |
                                                                               DesignFlowGraph::PRECEDENCE_SELECTOR |
                                                                               DesignFlowGraph::AUX_SELECTOR)),
      feedback_design_flow_graph(
          new DesignFlowGraph(design_flow_graphs_collection,
                              DesignFlowGraph::DEPENDENCE_SELECTOR | DesignFlowGraph::PRECEDENCE_SELECTOR |
                                  DesignFlowGraph::AUX_SELECTOR | DesignFlowGraph::DEPENDENCE_FEEDBACK_SELECTOR)),
      possibly_ready(design_flow_graph),
      parameters(_parameters),
      output_level(_parameters->getOption<int>(OPT_output_level))
{
   debug_level = parameters->get_class_debug_level(GET_CLASS(*this));
   const auto design_flow_graph_info = design_flow_graph->GetDesignFlowGraphInfo();
   null_deleter nullDel;
   design_flow_graph_info->entry = design_flow_graphs_collection->AddDesignFlowStep(
       DesignFlowStepRef(
           new AuxDesignFlowStep("Entry", DESIGN_FLOW_ENTRY, DesignFlowManagerConstRef(this, nullDel), parameters)),
       false);
#ifndef NDEBUG
   if(debug_level >= DEBUG_LEVEL_PARANOIC || parameters->IsParameter("profile_steps"))
   {
      step_names[design_flow_graph_info->entry] = "Entry";
   }
#endif
   const auto entry_info = design_flow_graph->GetDesignFlowStepInfo(design_flow_graph_info->entry);
   entry_info->status = DesignFlowStep_Status::EMPTY;
   design_flow_graph_info->exit = design_flow_graphs_collection->AddDesignFlowStep(
       DesignFlowStepRef(
           new AuxDesignFlowStep("Exit", DESIGN_FLOW_EXIT, DesignFlowManagerConstRef(this, nullDel), parameters)),
       false);
#ifndef NDEBUG
   if(debug_level >= DEBUG_LEVEL_PARANOIC || parameters->IsParameter("profile_steps"))
   {
      step_names[design_flow_graph_info->exit] = "Exit";
   }
#endif
}

DesignFlowManager::~DesignFlowManager() = default;

size_t DesignFlowManager::step_counter = 0;

void DesignFlowManager::AddStep(const DesignFlowStepRef step)
{
   DesignFlowStepSet steps;
   steps.insert(step);
   RecursivelyAddSteps(steps, false);
}

void DesignFlowManager::AddSteps(const DesignFlowStepSet& steps)
{
   RecursivelyAddSteps(steps, false);
}

void DesignFlowManager::RecursivelyAddSteps(
    const DesignFlowStepSet& steps, const bool unnecessary,
    CustomUnorderedSet<std::pair<DesignFlowStep::signature_t, bool>>& already_visited)
{
   static size_t temp_counter = 0;
   const auto design_flow_graph_info = design_flow_graph->GetDesignFlowGraphInfo();
   for(const auto& design_flow_step : steps)
   {
      const auto signature = design_flow_step->GetSignature();
      if(already_visited.count({signature, unnecessary}))
      {
         continue;
      }
      already_visited.emplace(signature, unnecessary);
      INDENT_DBG_MEX(DEBUG_LEVEL_PARANOIC, debug_level,
                     "-->Adding design flow step " + design_flow_step->GetName() + " - Signature " + STR(signature));

      /// Get vertex from design flow graph; there are four cases
      vertex step_vertex = GetDesignFlowStep(signature);
      /// The step already exists
      if(step_vertex)
      {
         if(unnecessary)
         {
            /// The step already exists and we are trying to re-add as unnecessary; both if now it is unnecessary or
            /// not, nothing has to be done
            INDENT_DBG_MEX(DEBUG_LEVEL_PARANOIC, debug_level, "<--This step already exist (unnecessary)");
            continue;
         }
         else
         {
            const auto dfs_info = design_flow_graph->GetDesignFlowStepInfo(step_vertex);
            if(dfs_info->status == DesignFlowStep_Status::UNNECESSARY)
            {
               /// The step already exists and it was unnecessary; now we are switching to necessary; note that
               /// computation of relationships of this node is performed to propagate the necessity If design flow step
               /// was ready I have to reinsert it into the set because of the ordering; so the setting of the
               /// unnecessary flag can not be factorized
               dfs_info->status = DesignFlowStep_Status::UNEXECUTED;
               possibly_ready.Update(step_vertex);
               INDENT_DBG_MEX(DEBUG_LEVEL_PARANOIC, debug_level,
                              "---This step already exist but was unnecessary. Now it becomes necessary");
            }
            else
            {
               THROW_ASSERT(dfs_info->status != DesignFlowStep_Status::SKIPPED,
                            "Switching to necessary " + dfs_info->design_flow_step->GetName() +
                                " which has already skipped");
               /// The step already exists and it is already necessary; nothing to do
               INDENT_DBG_MEX(DEBUG_LEVEL_PARANOIC, debug_level, "<--This step already exist");
               continue;
            }
         }
      }
      else
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_PARANOIC, debug_level, "---This step does not exist");
         step_vertex = design_flow_graphs_collection->AddDesignFlowStep(design_flow_step, unnecessary);
#ifndef NDEBUG
         if(debug_level >= DEBUG_LEVEL_PARANOIC || parameters->IsParameter("profile_steps"))
         {
            step_names[step_vertex] = design_flow_step->GetName();
         }
#endif
      }

      DesignFlowStepSet relationships;

      /// Add edges from dependencies
      INDENT_DBG_MEX(DEBUG_LEVEL_PARANOIC, debug_level, "-->Adding dependencies of " + design_flow_step->GetName());
      design_flow_step->ComputeRelationships(relationships, DesignFlowStep::DEPENDENCE_RELATIONSHIP);
      RecursivelyAddSteps(relationships, unnecessary, already_visited);
      for(const auto& relationship : relationships)
      {
         const auto relationship_signature = relationship->GetSignature();
         vertex relationship_vertex = GetDesignFlowStep(relationship_signature);
         design_flow_graphs_collection->AddDesignFlowDependence(relationship_vertex, step_vertex,
                                                                DesignFlowGraph::DEPENDENCE_SELECTOR);
#ifndef NDEBUG
         if(parameters->IsParameter("DebugDFM") and parameters->GetParameter<bool>("DebugDFM"))
         {
            std::list<vertex> vertices;
            try
            {
               design_flow_graph->TopologicalSort(vertices);
            }
            catch(...)
            {
               WriteLoopDot();
               feedback_design_flow_graph->WriteDot("Design_Flow_Error");
               THROW_UNREACHABLE("Design flow graph is not anymore acyclic");
            }
         }
#endif
      }
      INDENT_DBG_MEX(DEBUG_LEVEL_PARANOIC, debug_level, "<--Added dependencies  of " + design_flow_step->GetName());

      /// Add steps from precedences
      INDENT_DBG_MEX(DEBUG_LEVEL_PARANOIC, debug_level, "-->Adding precedences of " + design_flow_step->GetName());
      relationships.clear();
      design_flow_step->ComputeRelationships(relationships, DesignFlowStep::PRECEDENCE_RELATIONSHIP);
      RecursivelyAddSteps(relationships, true, already_visited);
      for(const auto& relationship : relationships)
      {
         const auto relationship_signature = relationship->GetSignature();
         vertex relationship_vertex = GetDesignFlowStep(relationship_signature);
         design_flow_graphs_collection->AddDesignFlowDependence(relationship_vertex, step_vertex,
                                                                DesignFlowGraph::PRECEDENCE_SELECTOR);
#ifndef NDEBUG
         if(parameters->IsParameter("DebugDFM") and parameters->GetParameter<bool>("DebugDFM"))
         {
            try
            {
               std::list<vertex> vertices;
               design_flow_graph->TopologicalSort(vertices);
            }
            catch(...)
            {
               WriteLoopDot();
               feedback_design_flow_graph->WriteDot("Design_Flow_Error");
               THROW_UNREACHABLE("Design flow graph is not anymore acyclic");
            }
         }
#endif
      }
      INDENT_DBG_MEX(DEBUG_LEVEL_PARANOIC, debug_level, "<--Added precedences of " + design_flow_step->GetName());

      /// Check if the added step is already ready
      bool current_ready = true;
      for(const auto& ie : boost::make_iterator_range(boost::in_edges(step_vertex, *design_flow_graph)))
      {
         const auto pre_dependence_vertex = boost::source(ie, *design_flow_graph);
         const auto pre_info = design_flow_graph->GetDesignFlowStepInfo(pre_dependence_vertex);
         switch(pre_info->status)
         {
            case DesignFlowStep_Status::ABORTED:
            case DesignFlowStep_Status::EMPTY:
            case DesignFlowStep_Status::SKIPPED:
            case DesignFlowStep_Status::SUCCESS:
            case DesignFlowStep_Status::UNCHANGED:
            {
               break;
            }
            case DesignFlowStep_Status::UNNECESSARY:
            case DesignFlowStep_Status::UNEXECUTED:
            {
               current_ready = false;
               break;
            }
            case DesignFlowStep_Status::NONEXISTENT:
            {
               THROW_UNREACHABLE("Step with nonexitent status");
               break;
            }
            default:
            {
               THROW_UNREACHABLE("");
            }
         }
      }
      if(current_ready)
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_PARANOIC, debug_level,
                        "---Adding " + design_flow_step->GetName() + " to list of ready steps");
         possibly_ready.insert(step_vertex);
      }
      if(debug_level >= DEBUG_LEVEL_PARANOIC)
      {
         feedback_design_flow_graph->WriteDot("Design_Flow_" + STR(step_counter) + "_" + STR(temp_counter));
         temp_counter++;
      }
      if(boost::in_degree(step_vertex, *design_flow_graph) == 0)
      {
         design_flow_graphs_collection->AddDesignFlowDependence(design_flow_graph_info->entry, step_vertex,
                                                                DesignFlowGraph::AUX_SELECTOR);
      }
      CustomOrderedSet<EdgeDescriptor> to_be_removeds;
      for(const auto& ie : boost::make_iterator_range(boost::in_edges(step_vertex, *design_flow_graph)))
      {
         const auto source = boost::source(ie, *design_flow_graph);
         if(design_flow_graph->ExistsEdge(source, design_flow_graph_info->exit))
         {
            to_be_removeds.insert(ie);
         }
      }
      for(const auto& to_be_removed : to_be_removeds)
      {
         design_flow_graphs_collection->RemoveSelector(to_be_removed, DesignFlowGraph::AUX_SELECTOR);
      }
      if(boost::out_degree(step_vertex, *design_flow_graph) == 0)
      {
         design_flow_graphs_collection->AddDesignFlowDependence(step_vertex, design_flow_graph_info->exit,
                                                                DesignFlowGraph::AUX_SELECTOR);
      }
      INDENT_DBG_MEX(DEBUG_LEVEL_PARANOIC, debug_level,
                     "<--Added design flow step " + design_flow_step->GetName() + " - Signature " + STR(signature));
   }
}

DesignFlowGraphConstRef DesignFlowManager::CGetDesignFlowGraph() const
{
   return design_flow_graph;
}

void DesignFlowManager::Exec()
{
#if !HAVE_UNORDERED
#ifndef NDEBUG
   INDENT_DBG_MEX(DEBUG_LEVEL_PARANOIC, debug_level,
                  "---Seed for design flow manager: " +
                      STR(parameters->isOption(OPT_test_single_non_deterministic_flow) ?
                              parameters->getOption<size_t>(OPT_test_single_non_deterministic_flow) :
                              0));
   auto generator = std::bind(std::uniform_int_distribution<size_t>(0, 10000),
                              std::mt19937(parameters->isOption(OPT_test_single_non_deterministic_flow) ?
                                               parameters->getOption<size_t>(OPT_test_single_non_deterministic_flow) :
                                               0));
#endif
#endif

   INDENT_DBG_MEX(DEBUG_LEVEL_PARANOIC, debug_level, "-->Started execution of design flow");
#ifndef NDEBUG
   if(debug_level >= DEBUG_LEVEL_PARANOIC)
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_PARANOIC, debug_level, "---Writing initial design flow graph");
      feedback_design_flow_graph->WriteDot("Design_Flow_" + STR(step_counter));
   }
#endif
   size_t executed_passes = 0;
   size_t skipped_passes = 0;
   size_t graph_changes = 0;
   long design_flow_manager_time = 0;
   const auto dump_statistics = parameters->IsParameter("dfm_statistics");
   while(possibly_ready.size())
   {
      const auto initial_number_vertices = boost::num_vertices(*design_flow_graphs_collection);
      const auto initial_number_edges = boost::num_edges(*design_flow_graphs_collection);
      long before_time = 0;
      if(dump_statistics)
      {
         START_TIME(before_time);
      }
      step_counter++;
#ifndef NDEBUG
      if(debug_level >= DEBUG_LEVEL_PARANOIC)
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_PARANOIC, debug_level, "-->Ready steps are");
         for(const auto& [step_key, ready_step] : possibly_ready)
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_PARANOIC, debug_level,
                           "---" + design_flow_graph->CGetDesignFlowStepInfo(ready_step)->design_flow_step->GetName());
         }
         INDENT_DBG_MEX(DEBUG_LEVEL_PARANOIC, debug_level, "<--");
      }
#endif
      const vertex next = [&]() -> vertex {
         size_t offset = 0;
#if !HAVE_UNORDERED
#ifndef NDEBUG
         if(parameters->isOption(OPT_test_single_non_deterministic_flow))
         {
            const size_t random = generator();
            offset = random % possibly_ready.size();
            INDENT_DBG_MEX(DEBUG_LEVEL_PARANOIC, debug_level,
                           "---Random is " + STR(random) + " - Offset is " + STR(offset) + " (Size is " +
                               STR(possibly_ready.size()) + ")");
         }
#endif
#endif
         return possibly_ready.Extract(offset);
      }();
      const auto dfs_info = design_flow_graph->GetDesignFlowStepInfo(next);
      const auto& step = dfs_info->design_flow_step;
      INDENT_DBG_MEX(DEBUG_LEVEL_PARANOIC, debug_level,
                     "-->Beginning iteration number " + STR(step_counter) + " - Considering step " + step->GetName());
#ifndef NDEBUG
      if(debug_level >= DEBUG_LEVEL_PARANOIC)
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_PARANOIC, debug_level, "-->Other ready steps are");
         for(const auto& [step_key, ready_step] : possibly_ready)
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_PARANOIC, debug_level,
                           "---" + design_flow_graph->CGetDesignFlowStepInfo(ready_step)->design_flow_step->GetName());
         }
         INDENT_DBG_MEX(DEBUG_LEVEL_PARANOIC, debug_level, "<--");
      }
#endif

      /// Now check if next is actually ready
      /// First of all check if there are new dependence to add
      INDENT_DBG_MEX(DEBUG_LEVEL_PARANOIC, debug_level, "-->Recomputing dependences");
      DesignFlowStepSet pre_dependence_steps, pre_precedence_steps;
      step->ComputeRelationships(pre_dependence_steps, DesignFlowStep::DEPENDENCE_RELATIONSHIP);
      RecursivelyAddSteps(pre_dependence_steps, dfs_info->status == DesignFlowStep_Status::UNNECESSARY);
      INDENT_DBG_MEX(DEBUG_LEVEL_PARANOIC, debug_level, "<--Recomputed dependences");
      INDENT_DBG_MEX(DEBUG_LEVEL_PARANOIC, debug_level, "-->Recomputing precedences");
      step->ComputeRelationships(pre_precedence_steps, DesignFlowStep::PRECEDENCE_RELATIONSHIP);
      RecursivelyAddSteps(pre_precedence_steps, true);
      INDENT_DBG_MEX(DEBUG_LEVEL_PARANOIC, debug_level, "<--Recomputed precedences");
      bool current_ready = true;
      for(const auto& pre_dependence_step : pre_dependence_steps)
      {
         const vertex pre_dependence_vertex = design_flow_graph->GetDesignFlowStep(pre_dependence_step->GetSignature());
         design_flow_graphs_collection->AddDesignFlowDependence(pre_dependence_vertex, next,
                                                                DesignFlowGraph::DEPENDENCE_SELECTOR);
         const auto pre_info = design_flow_graph->GetDesignFlowStepInfo(pre_dependence_vertex);
         switch(pre_info->status)
         {
            case DesignFlowStep_Status::ABORTED:
            case DesignFlowStep_Status::EMPTY:
            case DesignFlowStep_Status::SKIPPED:
            case DesignFlowStep_Status::SUCCESS:
            case DesignFlowStep_Status::UNCHANGED:
            {
               break;
            }
            case DesignFlowStep_Status::UNNECESSARY:
            case DesignFlowStep_Status::UNEXECUTED:
            {
               current_ready = false;
               break;
            }
            case DesignFlowStep_Status::NONEXISTENT:
            {
               THROW_UNREACHABLE("Step with nonexitent status");
               break;
            }
            default:
            {
               THROW_UNREACHABLE("");
            }
         }
      }
      /// Now iterate on ingoing precedence edge
      for(const auto& pre_precedence_step : pre_precedence_steps)
      {
         const vertex pre_precedence_vertex = design_flow_graph->GetDesignFlowStep(pre_precedence_step->GetSignature());
         design_flow_graphs_collection->AddDesignFlowDependence(pre_precedence_vertex, next,
                                                                DesignFlowGraph::PRECEDENCE_SELECTOR);
         const auto pre_info = design_flow_graph->GetDesignFlowStepInfo(pre_precedence_vertex);
         switch(pre_info->status)
         {
            case DesignFlowStep_Status::ABORTED:
            case DesignFlowStep_Status::EMPTY:
            case DesignFlowStep_Status::SKIPPED:
            case DesignFlowStep_Status::SUCCESS:
            case DesignFlowStep_Status::UNCHANGED:
            {
               break;
            }
            case DesignFlowStep_Status::UNNECESSARY:
            case DesignFlowStep_Status::UNEXECUTED:
            {
               current_ready = false;
               break;
            }
            case DesignFlowStep_Status::NONEXISTENT:
            {
               THROW_UNREACHABLE("Step with nonexitent status");
               break;
            }
            default:
            {
               THROW_UNREACHABLE("");
            }
         }
      }
      if(!current_ready)
      {
#ifndef NDEBUG
         if(debug_level >= DEBUG_LEVEL_PARANOIC)
         {
            feedback_design_flow_graph->WriteDot("Design_Flow_" + STR(step_counter));
            INDENT_DBG_MEX(DEBUG_LEVEL_PARANOIC, debug_level, "---Writing Design_Flow_" + STR(step_counter));

            /// Save the current status of the graph in history
            for(const auto& temp_step : boost::make_iterator_range(boost::vertices(*design_flow_graph)))
            {
               const auto temp_design_flow_step_info = design_flow_graph->CGetDesignFlowStepInfo(temp_step);
               vertex_history[step_counter][temp_step] = temp_design_flow_step_info->status;
            }
            for(const auto& edge : boost::make_iterator_range(boost::edges(*design_flow_graph)))
            {
               edge_history[step_counter][edge] = design_flow_graph->GetSelector(edge);
            }
         }
#endif
         INDENT_DBG_MEX(DEBUG_LEVEL_PARANOIC, debug_level, "<--Ended iteration number " + STR(step_counter));
         const auto final_number_vertices = boost::num_vertices(*design_flow_graphs_collection);
         const auto final_number_edges = boost::num_edges(*design_flow_graphs_collection);
         if(final_number_vertices > initial_number_vertices || final_number_edges > initial_number_edges)
         {
            graph_changes++;
         }
         continue;
      }
      if(dump_statistics)
      {
         STOP_TIME(before_time);
         design_flow_manager_time += before_time;
      }
      if(dfs_info->status == DesignFlowStep_Status::UNNECESSARY)
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_PARANOIC, debug_level,
                        "---Skipping execution of " + step->GetName() + " since unnecessary");
         dfs_info->status = DesignFlowStep_Status::SKIPPED;
      }
      else if(step->HasToBeExecuted())
      {
#ifndef NDEBUG
         size_t indentation_before = indentation;
#endif
         INDENT_OUT_MEX(OUTPUT_LEVEL_VERY_PEDANTIC, output_level, "-->Starting execution of " + step->GetName());
         long step_execution_time = 0;
         if(OUTPUT_LEVEL_VERY_PEDANTIC <= output_level
#ifndef NDEBUG
            || parameters->IsParameter("profile_steps")
#endif
         )
         {
            START_TIME(step_execution_time);
         }
         step->Initialize();
         if(step->CGetDebugLevel() >= DEBUG_LEVEL_VERY_PEDANTIC)
         {
            step->PrintInitialIR();
         }
         dfs_info->status = step->Exec();
         executed_passes++;
         if(step->CGetDebugLevel() >= DEBUG_LEVEL_VERY_PEDANTIC)
         {
            step->PrintFinalIR();
         }
         if(OUTPUT_LEVEL_VERY_PEDANTIC <= output_level
#ifndef NDEBUG
            || parameters->IsParameter("profile_steps")
#endif
         )
         {
            STOP_TIME(step_execution_time);
         }
         const std::string memory_usage =
#ifndef NDEBUG
             std::string(" - Virtual Memory: ") + PrintVirtualDataMemoryUsage()
#else
             ""
#endif
             ;
#ifndef NDEBUG
         if(parameters->IsParameter("profile_steps"))
         {
            accumulated_execution_time[next] += step_execution_time;
            if(dfs_info->status == DesignFlowStep_Status::SUCCESS)
            {
               success_executions[next]++;
            }
            else if(dfs_info->status == DesignFlowStep_Status::UNCHANGED)
            {
               unchanged_executions[next]++;
            }
         }
#endif
         INDENT_OUT_MEX(OUTPUT_LEVEL_VERY_PEDANTIC, output_level,
                        "<--Ended execution of " + step->GetName() +
                            (dfs_info->status == DesignFlowStep_Status::UNCHANGED ?
                                 ":=" :
                                 (dfs_info->status == DesignFlowStep_Status::SUCCESS ? ":+" : "")) +
                            " in " + print_cpu_time(step_execution_time) + " seconds" + memory_usage);
#ifndef NDEBUG
         THROW_ASSERT(indentation_before == indentation, "Not closed indentation");
#endif
      }
      else
      {
         INDENT_OUT_MEX(DEBUG_LEVEL_PARANOIC, debug_level, "---Skipping execution of " + step->GetName());
         dfs_info->status = DesignFlowStep_Status::UNCHANGED;
         skipped_passes++;
#ifndef NDEBUG
         if(parameters->IsParameter("profile_steps"))
         {
            skipped_executions[next]++;
         }
#endif
      }
      long after_time = 0;
      if(dump_statistics)
      {
         START_TIME(after_time);
      }
      bool invalidations = false;
      if(!parameters->IsParameter("disable-invalidations"))
      {
         /// Add steps and edges from post dependencies
         INDENT_DBG_MEX(DEBUG_LEVEL_PARANOIC, debug_level, "-->Adding post-dependencies of " + step->GetName());
         DesignFlowStepSet relationships;
         step->ComputeRelationships(relationships, DesignFlowStep::INVALIDATION_RELATIONSHIP);
         INDENT_DBG_MEX(DEBUG_LEVEL_PARANOIC, debug_level, "---Got steps");
         invalidations = !relationships.empty();
         for(const auto& relationship : relationships)
         {
            const auto relationship_signature = relationship->GetSignature();
            vertex relationship_vertex = GetDesignFlowStep(relationship_signature);
            THROW_ASSERT(relationship_vertex, "Missing vertex " + relationship_signature);
            if(design_flow_graph->IsReachable(relationship_vertex, next))
            {
               design_flow_graphs_collection->AddDesignFlowDependence(next, relationship_vertex,
                                                                      DesignFlowGraph::DEPENDENCE_FEEDBACK_SELECTOR);
               DeExecute(relationship_vertex, true);
            }
            else
            {
               feedback_design_flow_graph->WriteDot("Design_Flow_Error");
               THROW_UNREACHABLE(
                   "Invalidating " +
                   design_flow_graph->CGetDesignFlowStepInfo(relationship_vertex)->design_flow_step->GetName() +
                   " which is not before the current one");
            }
         }
         INDENT_DBG_MEX(DEBUG_LEVEL_PARANOIC, debug_level, "<--Added post-dependencies of " + step->GetName());
      }
      INDENT_DBG_MEX(DEBUG_LEVEL_PARANOIC, debug_level, "-->Starting checking of new ready steps");
      for(const auto& oe : boost::make_iterator_range(boost::out_edges(next, *design_flow_graph)))
      {
         const vertex target = boost::target(oe, *design_flow_graph);
         auto target_info = design_flow_graph->GetDesignFlowStepInfo(target);
         switch(target_info->status)
         {
            case DesignFlowStep_Status::ABORTED:
            case DesignFlowStep_Status::EMPTY:
            case DesignFlowStep_Status::SKIPPED:
            case DesignFlowStep_Status::SUCCESS:
            case DesignFlowStep_Status::UNCHANGED:
            {
               /// Post dependence previously required and previously executed;
               /// Now it is not more required, otherwise execution flag should just invalidated
               continue;
            }
            case DesignFlowStep_Status::UNNECESSARY:
            case DesignFlowStep_Status::UNEXECUTED:
            {
               break;
            }
            case DesignFlowStep_Status::NONEXISTENT:
            {
               THROW_UNREACHABLE("Step with nonexitent status");
               break;
            }
            default:
            {
               THROW_UNREACHABLE("");
            }
         }
         INDENT_DBG_MEX(DEBUG_LEVEL_PARANOIC, debug_level,
                        "-->Examining successor " +
                            design_flow_graph->GetDesignFlowStepInfo(target)->design_flow_step->GetName());
         bool target_ready = true;
         for(const auto& ie : boost::make_iterator_range(boost::in_edges(target, *design_flow_graph)))
         {
            const vertex source = boost::source(ie, *design_flow_graph);
            const auto source_info = design_flow_graph->GetDesignFlowStepInfo(source);
            INDENT_DBG_MEX(DEBUG_LEVEL_PARANOIC, debug_level,
                           "-->Examining predecessor " + source_info->design_flow_step->GetName());
            switch(source_info->status)
            {
               case DesignFlowStep_Status::ABORTED:
               case DesignFlowStep_Status::EMPTY:
               case DesignFlowStep_Status::SKIPPED:
               case DesignFlowStep_Status::SUCCESS:
               case DesignFlowStep_Status::UNCHANGED:
               {
                  break;
               }
               case DesignFlowStep_Status::UNNECESSARY:
               case DesignFlowStep_Status::UNEXECUTED:
               {
                  target_ready = false;
                  break;
               }
               case DesignFlowStep_Status::NONEXISTENT:
               {
                  THROW_UNREACHABLE("Step with nonexitent status");
                  break;
               }
               default:
               {
                  THROW_UNREACHABLE("");
               }
            }
            if(!target_ready)
            {
               INDENT_DBG_MEX(DEBUG_LEVEL_PARANOIC, debug_level, "<--Not ready");
               break;
            }
            INDENT_DBG_MEX(DEBUG_LEVEL_PARANOIC, debug_level, "<--");
         }
         if(target_ready)
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_PARANOIC, debug_level,
                           "---Adding " +
                               design_flow_graph->CGetDesignFlowStepInfo(target)->design_flow_step->GetName() +
                               " to list of ready steps");
            possibly_ready.insert(target);
         }
         INDENT_DBG_MEX(DEBUG_LEVEL_PARANOIC, debug_level, "<--");
      }
      INDENT_DBG_MEX(DEBUG_LEVEL_PARANOIC, debug_level, "<--Checked new ready steps");
      CustomOrderedSet<EdgeDescriptor> to_be_removeds;
      for(const auto& ie : boost::make_iterator_range(
              boost::in_edges(design_flow_graph->CGetDesignFlowGraphInfo()->exit, *design_flow_graph)))
      {
         const auto source = boost::source(ie, *design_flow_graph);
         if(boost::out_degree(source, *design_flow_graph) > 1)
         {
            to_be_removeds.insert(ie);
         }
      }
      for(const auto& to_be_removed : to_be_removeds)
      {
         design_flow_graphs_collection->RemoveSelector(to_be_removed, DesignFlowGraph::AUX_SELECTOR);
      }
#ifndef NDEBUG
      if(debug_level >= DEBUG_LEVEL_PARANOIC)
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_PARANOIC, debug_level, "---Writing Design_Flow_" + STR(step_counter));
         feedback_design_flow_graph->WriteDot("Design_Flow_" + STR(step_counter));

         /// Save the current status of the graph in history
         for(const auto& temp_step : boost::make_iterator_range(boost::vertices(*design_flow_graph)))
         {
            const auto temp_design_flow_step_info = design_flow_graph->CGetDesignFlowStepInfo(temp_step);
            vertex_history[step_counter][temp_step] = temp_design_flow_step_info->status;
         }
         for(const auto& edge : boost::make_iterator_range(boost::edges(*design_flow_graph)))
         {
            edge_history[step_counter][edge] = design_flow_graph->GetSelector(edge);
         }
      }
#endif
      if(dump_statistics)
      {
         STOP_TIME(after_time);
         design_flow_manager_time += after_time;
      }
      INDENT_DBG_MEX(DEBUG_LEVEL_PARANOIC, debug_level,
                     "<--Ended iteration number " + STR(step_counter) + " - Step " + step->GetName());
      if(dump_statistics)
      {
         const auto final_number_vertices = boost::num_vertices(*design_flow_graphs_collection);
         const auto final_number_edges = boost::num_edges(*design_flow_graphs_collection);
         if(final_number_vertices > initial_number_vertices || final_number_edges > initial_number_edges or
            invalidations)
         {
            graph_changes++;
         }
         if(parameters->GetParameter<size_t>("dfm_statistics") > 1)
         {
            size_t executed_vertices = 0;
            static size_t previous_executed_vertices = 0;
            for(const auto& v_stat : boost::make_iterator_range(boost::vertices(*design_flow_graph)))
            {
               const auto local_design_flow_step_info = design_flow_graph->GetDesignFlowStepInfo(v_stat);
               switch(local_design_flow_step_info->status)
               {
                  case DesignFlowStep_Status::ABORTED:
                  case DesignFlowStep_Status::EMPTY:
                  case DesignFlowStep_Status::SUCCESS:
                  case DesignFlowStep_Status::UNCHANGED:
                  case DesignFlowStep_Status::SKIPPED:
                  {
                     executed_vertices++;
                     break;
                  }
                  case DesignFlowStep_Status::UNNECESSARY:
                  case DesignFlowStep_Status::UNEXECUTED:
                  {
                     break;
                  }
                  case DesignFlowStep_Status::NONEXISTENT:
                  {
                     THROW_UNREACHABLE("Step with nonexitent status");
                     break;
                  }
                  default:
                  {
                     THROW_UNREACHABLE("");
                  }
               }
            }
            INDENT_OUT_MEX(OUTPUT_LEVEL_NONE, output_level, "-->DesignFlowManager - Iteration " + STR(step_counter));
            INDENT_OUT_MEX(OUTPUT_LEVEL_NONE, output_level, "---Executed step: " + step->GetName());
            if(previous_executed_vertices > executed_vertices)
            {
               INDENT_OUT_MEX(OUTPUT_LEVEL_NONE, output_level,
                              "---Invalidated vertices: " + STR(previous_executed_vertices - executed_vertices));
            }
            INDENT_OUT_MEX(OUTPUT_LEVEL_NONE, output_level,
                           "---Executed    vertices: " + STR(executed_vertices) + " / " + STR(final_number_vertices));
            INDENT_OUT_MEX(OUTPUT_LEVEL_NONE, output_level, "---Edges: " + STR(final_number_edges));
            INDENT_OUT_MEX(OUTPUT_LEVEL_NONE, output_level, "<--");
            previous_executed_vertices = executed_vertices;
         }
      }
   }
#ifndef NDEBUG
   if(debug_level >= DEBUG_LEVEL_PARANOIC)
   {
      for(size_t writing_step_counter = 0; writing_step_counter < step_counter; writing_step_counter++)
      {
         if(edge_history.find(writing_step_counter) != edge_history.end())
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_PARANOIC, debug_level,
                           "-->Writing Design_Flow_History_" + STR(writing_step_counter));
            feedback_design_flow_graph->WriteDot("Design_Flow_History_" + STR(writing_step_counter), vertex_history,
                                                 edge_history, step_names, writing_step_counter);
            INDENT_DBG_MEX(DEBUG_LEVEL_PARANOIC, debug_level,
                           "<--Written Design_Flow_History_" + STR(writing_step_counter));
         }
      }
   }
#endif
   if(design_flow_graph->CGetDesignFlowStepInfo(design_flow_graph->CGetDesignFlowGraphInfo()->exit)->status !=
      DesignFlowStep_Status::EMPTY)
   {
#ifndef NDEBUG
      WriteLoopDot();
#endif
      feedback_design_flow_graph->WriteDot("Design_Flow_Error");
      THROW_UNREACHABLE("Design flow didn't end");
   }
   if(dump_statistics)
   {
      INDENT_OUT_MEX(OUTPUT_LEVEL_NONE, output_level, "-->DesignFlowManager - Final summary");
      INDENT_OUT_MEX(OUTPUT_LEVEL_NONE, output_level, "---Vertices: " + STR(boost::num_vertices(*design_flow_graph)));
      INDENT_OUT_MEX(OUTPUT_LEVEL_NONE, output_level, "---Edges   : " + STR(boost::num_edges(*design_flow_graph)));
      INDENT_OUT_MEX(OUTPUT_LEVEL_NONE, output_level, "---Passes executed : " + STR(executed_passes));
      INDENT_OUT_MEX(OUTPUT_LEVEL_NONE, output_level, "---Passes skipped  : " + STR(skipped_passes));
      INDENT_OUT_MEX(OUTPUT_LEVEL_NONE, output_level, "---Graph changes   : " + STR(graph_changes));
      INDENT_OUT_MEX(OUTPUT_LEVEL_NONE, output_level,
                     "---Pass engine time: " + print_cpu_time(design_flow_manager_time) + " seconds");
      INDENT_OUT_MEX(OUTPUT_LEVEL_NONE, output_level, "<--");
   }
#ifndef NDEBUG
   if(parameters->IsParameter("profile_steps"))
   {
      INDENT_OUT_MEX(OUTPUT_LEVEL_NONE, output_level, "-->Steps execution statistics");
      for(const auto& [step, ex_time] : accumulated_execution_time)
      {
         INDENT_OUT_MEX(OUTPUT_LEVEL_NONE, output_level,
                        "---" + step_names.at(step) + ": " + print_cpu_time(ex_time) + " seconds - Successes: " +
                            STR(success_executions[step]) + " - Unchanged: " + STR(unchanged_executions[step]) +
                            " - Skipped: " + STR(skipped_executions[step]));
      }
      INDENT_OUT_MEX(OUTPUT_LEVEL_NONE, output_level, "<--");
   }
#endif
   INDENT_DBG_MEX(DEBUG_LEVEL_PARANOIC, debug_level, "---Total number of iterations: " + STR(step_counter));
   INDENT_DBG_MEX(DEBUG_LEVEL_PARANOIC, debug_level, "<--Ended execution of design flow");
}

vertex DesignFlowManager::GetDesignFlowStep(DesignFlowStep::signature_t signature) const
{
   return design_flow_graphs_collection->GetDesignFlowStep(signature);
}

DesignFlowStepFactoryConstRef DesignFlowManager::CGetDesignFlowStepFactory(DesignFlowStep::StepClass step_class) const
{
   THROW_ASSERT(design_flow_step_factories.find(step_class) != design_flow_step_factories.end(),
                "No factory to create steps with class " + STR(step_class) + " found");
   return design_flow_step_factories.at(step_class);
}

void DesignFlowManager::RegisterFactory(const DesignFlowStepFactoryConstRef factory)
{
   design_flow_step_factories[factory->GetClass()] = factory;
}

void DesignFlowManager::DeExecute(const vertex starting_vertex, const bool force_execution,
                                  CustomUnorderedSet<vertex>& already_visited)
{
   /// Set not executed on the starting vertex
   const auto dfs_info = design_flow_graph->GetDesignFlowStepInfo(starting_vertex);
   INDENT_DBG_MEX(DEBUG_LEVEL_PARANOIC, debug_level, "---DeExecuting " + dfs_info->design_flow_step->GetName());
   switch(dfs_info->status)
   {
      case DesignFlowStep_Status::SUCCESS:
      {
         dfs_info->status = DesignFlowStep_Status::UNEXECUTED;
         break;
      }
      case DesignFlowStep_Status::ABORTED:
      case DesignFlowStep_Status::EMPTY:
      case DesignFlowStep_Status::UNCHANGED:
      {
         dfs_info->status = DesignFlowStep_Status::UNEXECUTED;
         break;
      }
      case DesignFlowStep_Status::SKIPPED:
      {
         if(force_execution)
         {
            dfs_info->status = DesignFlowStep_Status::UNEXECUTED;
            possibly_ready.Update(starting_vertex);
         }
         else
         {
            dfs_info->status = DesignFlowStep_Status::UNNECESSARY;
         }
         break;
      }
      case DesignFlowStep_Status::UNNECESSARY:
      case DesignFlowStep_Status::UNEXECUTED:
      {
         break;
      }
      case DesignFlowStep_Status::NONEXISTENT:
      {
         THROW_UNREACHABLE("Step with nonexitent status");
         break;
      }
      default:
      {
         THROW_UNREACHABLE("");
      }
   }

   /// Check if the vertex is already ready
   bool current_ready = true;
   for(const auto& ie : boost::make_iterator_range(boost::in_edges(starting_vertex, *design_flow_graph)))
   {
      vertex pre_dependence_vertex = boost::source(ie, *design_flow_graph);
      const auto pre_info = design_flow_graph->GetDesignFlowStepInfo(pre_dependence_vertex);
      switch(pre_info->status)
      {
         case DesignFlowStep_Status::ABORTED:
         case DesignFlowStep_Status::EMPTY:
         case DesignFlowStep_Status::SKIPPED:
         case DesignFlowStep_Status::SUCCESS:
         case DesignFlowStep_Status::UNCHANGED:
         {
            break;
         }
         case DesignFlowStep_Status::UNNECESSARY:
         case DesignFlowStep_Status::UNEXECUTED:
         {
            current_ready = false;
            break;
         }
         case DesignFlowStep_Status::NONEXISTENT:
         {
            THROW_UNREACHABLE("Step with nonexitent status");
            break;
         }
         default:
         {
            THROW_UNREACHABLE("");
         }
      }
   }
   if(current_ready)
   {
      possibly_ready.insert(starting_vertex);
   }

   already_visited.insert(starting_vertex);

   /// Propagating to successor
   for(const auto& oe : boost::make_iterator_range(boost::out_edges(starting_vertex, *design_flow_graph)))
   {
      const vertex target = boost::target(oe, *design_flow_graph);
      const auto target_info = design_flow_graph->GetDesignFlowStepInfo(target);
      switch(target_info->status)
      {
         case DesignFlowStep_Status::ABORTED:
         case DesignFlowStep_Status::EMPTY:
         case DesignFlowStep_Status::SUCCESS:
         case DesignFlowStep_Status::UNCHANGED:
         case DesignFlowStep_Status::SKIPPED:
         {
            if(!already_visited.count(target))
            {
               DeExecute(target, false, already_visited);
            }
            break;
         }
         case DesignFlowStep_Status::UNNECESSARY:
         case DesignFlowStep_Status::UNEXECUTED:
         {
            break;
         }
         case DesignFlowStep_Status::NONEXISTENT:
         {
            THROW_UNREACHABLE("Step with nonexitent status");
            break;
         }
         default:
         {
            THROW_UNREACHABLE("");
         }
      }
   }
}

DesignFlowStep_Status DesignFlowManager::GetStatus(DesignFlowStep::signature_t signature) const
{
   const vertex step = GetDesignFlowStep(signature);
   if(step == NULL_VERTEX)
   {
      return DesignFlowStep_Status::NONEXISTENT;
   }
   else
   {
      return design_flow_graph->CGetDesignFlowStepInfo(step)->status;
   }
}

DesignFlowStepRef DesignFlowManager::CreateFlowStep(DesignFlowStep::signature_t signature) const
{
   return CGetDesignFlowStepFactory(DesignFlowStep::GetStepClass(signature))->CreateFlowStep(signature);
}

#ifndef NDEBUG
void DesignFlowManager::WriteLoopDot() const
{
   std::map<size_t, UnorderedSetStdStable<vertex>> sccs;
   feedback_design_flow_graph->GetStronglyConnectedComponents(sccs);
   for(const auto& id_scc : sccs)
   {
      const auto& scc_id = id_scc.first;
      const auto& scc = id_scc.second;
      if(scc.size() > 1)
      {
         CustomUnorderedSet<vertex> vertices;
         for(const auto v : scc)
         {
            vertices.insert(v);
         }
         DesignFlowGraph(design_flow_graphs_collection,
                         DesignFlowGraph::DEPENDENCE_SELECTOR | DesignFlowGraph::PRECEDENCE_SELECTOR |
                             DesignFlowGraph::AUX_SELECTOR | DesignFlowGraph::DEPENDENCE_FEEDBACK_SELECTOR,
                         vertices)
             .WriteDot("DesignFlowLoop_" + STR(scc_id) + ".dot");
      }
   }
}
#endif
