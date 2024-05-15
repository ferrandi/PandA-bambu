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

DesignFlowStepPrioritySet::key_t DesignFlowStepPrioritySet::compute_key(const vertex_descriptor v) const
{
   key_t idx = 0;
   const auto info = _dfg->CGetNodeInfo(v);
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

bool DesignFlowStepPrioritySet::insert(const vertex_descriptor v)
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

void DesignFlowStepPrioritySet::insert_or_assign(const vertex_descriptor v)
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

bool DesignFlowStepPrioritySet::Update(const vertex_descriptor v)
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

DesignFlowStepPrioritySet::vertex_descriptor DesignFlowStepPrioritySet::Extract(map_t::size_type pos)
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
    : design_flow_graph(new DesignFlowGraph()),
#ifndef NDEBUG
      feedback_design_flow_graph(new DesignFlowGraph()),
#endif
      possibly_ready(design_flow_graph),
      step_counter(0),
      parameters(_parameters),
      output_level(_parameters->getOption<int>(OPT_output_level)),
      debug_level(_parameters->get_class_debug_level(GET_CLASS(*this)))
{
   const DesignFlowStepRef entry_step(
       new AuxDesignFlowStep(DESIGN_FLOW_ENTRY, DesignFlowManagerConstRef(this, null_deleter{}), parameters));
   const DesignFlowStepRef exit_step(
       new AuxDesignFlowStep(DESIGN_FLOW_EXIT, DesignFlowManagerConstRef(this, null_deleter{}), parameters));
   const auto& design_flow_graph_info = design_flow_graph->GetGraphInfo();
   design_flow_graph_info->entry = design_flow_graph->AddDesignFlowStep(entry_step, false);
   design_flow_graph->GetNodeInfo(design_flow_graph_info->entry)->status = DesignFlowStep_Status::EMPTY;
   design_flow_graph_info->exit = design_flow_graph->AddDesignFlowStep(exit_step, false);
#ifndef NDEBUG
   const auto& feedback_design_flow_graph_info = feedback_design_flow_graph->GetGraphInfo();
   feedback_design_flow_graph_info->entry = feedback_design_flow_graph->AddDesignFlowStep(entry_step, false);
   feedback_design_flow_graph->GetNodeInfo(feedback_design_flow_graph_info->entry)->status =
       DesignFlowStep_Status::EMPTY;
   feedback_design_flow_graph_info->exit = design_flow_graph->AddDesignFlowStep(exit_step, false);
   dfg_to_feedback[design_flow_graph_info->entry] = feedback_design_flow_graph_info->entry;
   dfg_to_feedback[design_flow_graph_info->exit] = feedback_design_flow_graph_info->exit;
#endif
   if(
#ifndef NDEBUG
       debug_level >= DEBUG_LEVEL_PARANOIC ||
#endif
       parameters->IsParameter("profile_steps"))
   {
      step_prof_info.emplace(design_flow_graph_info->entry, StepProfilingInfo(entry_step->GetName()));
      step_prof_info.emplace(design_flow_graph_info->exit, StepProfilingInfo(exit_step->GetName()));
   }
}

DesignFlowManager::vertex_descriptor DesignFlowManager::AddDesignFlowStep(const DesignFlowStepRef& design_flow_step,
                                                                          bool unnecessary)
{
   auto v = design_flow_graph->AddDesignFlowStep(design_flow_step, unnecessary);
#ifndef NDEBUG
   dfg_to_feedback[v] = feedback_design_flow_graph->AddDesignFlowStep(design_flow_step, unnecessary);
#endif
   return v;
}

void DesignFlowManager::AddDesignFlowDependence(vertex_descriptor src, vertex_descriptor tgt, DesignFlowEdge type)
{
#ifndef NDEBUG
   feedback_design_flow_graph->AddDesignFlowDependence(dfg_to_feedback.at(src), dfg_to_feedback.at(tgt), type);
   if(type & ~DesignFlowGraph::FEEDBACK)
   {
      design_flow_graph->AddDesignFlowDependence(src, tgt,
                                                 static_cast<DesignFlowEdge>(type & ~DesignFlowGraph::FEEDBACK));
   }
#else
   design_flow_graph->AddDesignFlowDependence(src, tgt, type);
#endif
}

void DesignFlowManager::RemoveDesignFlowDependence(edge_descriptor e)
{
#ifndef NDEBUG
   const auto [fe, found] =
       boost::edge(dfg_to_feedback.at(boost::source(e, *design_flow_graph)),
                   dfg_to_feedback.at(boost::target(e, *design_flow_graph)), *feedback_design_flow_graph);
   feedback_design_flow_graph->RemoveEdge(fe);
#endif
   design_flow_graph->RemoveEdge(e);
}

DesignFlowEdge DesignFlowManager::AddType(edge_descriptor e, DesignFlowEdge type)
{
#ifndef NDEBUG
   const auto [fe, found] =
       boost::edge(dfg_to_feedback.at(boost::source(e, *design_flow_graph)),
                   dfg_to_feedback.at(boost::target(e, *design_flow_graph)), *feedback_design_flow_graph);
   feedback_design_flow_graph->AddType(fe, type);
#endif
   return design_flow_graph->AddType(e, static_cast<DesignFlowEdge>(type & ~DesignFlowGraph::FEEDBACK));
}

DesignFlowEdge DesignFlowManager::RemoveType(edge_descriptor e, DesignFlowEdge type)
{
#ifndef NDEBUG
   const auto [fe, found] =
       boost::edge(dfg_to_feedback.at(boost::source(e, *design_flow_graph)),
                   dfg_to_feedback.at(boost::target(e, *design_flow_graph)), *feedback_design_flow_graph);
   feedback_design_flow_graph->RemoveType(fe, type);
#endif
   return design_flow_graph->RemoveType(e, type);
}

void DesignFlowManager::RecursivelyAddSteps(
    const DesignFlowStepSet& steps, const bool unnecessary,
    CustomUnorderedSet<std::pair<DesignFlowStep::signature_t, bool>>& already_visited)
{
#ifndef NDEBUG
   static size_t temp_counter = 0;
#endif
   const auto design_flow_graph_info = design_flow_graph->GetGraphInfo();
   for(const auto& design_flow_step : steps)
   {
      const auto signature = design_flow_step->GetSignature();
      if(!already_visited.emplace(signature, unnecessary).second)
      {
         continue;
      }
      INDENT_DBG_MEX(DEBUG_LEVEL_PARANOIC, debug_level,
                     "-->Adding design flow step " + design_flow_step->GetName() + " - Signature " + STR(signature));

      /// Get vertex from design flow graph; there are four cases
      auto step_vertex = GetDesignFlowStep(signature);
      /// The step already exists
      if(step_vertex != DesignFlowGraph::null_vertex())
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
            const auto& dfs_info = design_flow_graph->GetNodeInfo(step_vertex);
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
         step_vertex = AddDesignFlowStep(design_flow_step, unnecessary);
         if(
#ifndef NDEBUG
             debug_level >= DEBUG_LEVEL_PARANOIC ||
#endif
             parameters->IsParameter("profile_steps"))
         {
            step_prof_info.emplace(step_vertex, StepProfilingInfo(design_flow_step->GetName()));
         }
      }

      DesignFlowStepSet relationships;

      /// Add edges from dependencies
      INDENT_DBG_MEX(DEBUG_LEVEL_PARANOIC, debug_level, "-->Adding dependencies of " + design_flow_step->GetName());
      design_flow_step->ComputeRelationships(relationships, DesignFlowStep::DEPENDENCE_RELATIONSHIP);
      RecursivelyAddSteps(relationships, unnecessary, already_visited);
      for(const auto& relationship : relationships)
      {
         const auto relationship_signature = relationship->GetSignature();
         const auto relationship_vertex = GetDesignFlowStep(relationship_signature);
         AddDesignFlowDependence(relationship_vertex, step_vertex, DesignFlowGraph::DEPENDENCE);
#ifndef NDEBUG
         if(parameters->IsParameter("DebugDFM") && parameters->GetParameter<bool>("DebugDFM"))
         {
            std::list<vertex_descriptor> vertices;
            try
            {
               boost::topological_sort(*design_flow_graph, std::front_inserter(vertices));
            }
            catch(...)
            {
               WriteLoopDot();
               feedback_design_flow_graph->WriteDot(parameters->getOption<std::filesystem::path>(OPT_dot_directory) /
                                                    "Design_Flow_Error");
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
         const auto relationship_vertex = GetDesignFlowStep(relationship_signature);
         AddDesignFlowDependence(relationship_vertex, step_vertex, DesignFlowGraph::PRECEDENCE);
#ifndef NDEBUG
         if(parameters->IsParameter("DebugDFM") && parameters->GetParameter<bool>("DebugDFM"))
         {
            std::list<vertex_descriptor> vertices;
            try
            {
               boost::topological_sort(*design_flow_graph, std::front_inserter(vertices));
            }
            catch(...)
            {
               WriteLoopDot();
               feedback_design_flow_graph->WriteDot(parameters->getOption<std::filesystem::path>(OPT_dot_directory) /
                                                    "Design_Flow_Error");
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
         const auto dep_v = boost::source(ie, *design_flow_graph);
         const auto& pre_info = design_flow_graph->GetNodeInfo(dep_v);
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
#ifndef NDEBUG
      if(debug_level >= DEBUG_LEVEL_PARANOIC)
      {
         feedback_design_flow_graph->WriteDot(parameters->getOption<std::filesystem::path>(OPT_dot_directory) /
                                              ("Design_Flow_" + STR(step_counter) + "_" + STR(temp_counter)));
         temp_counter++;
      }
#endif
      if(boost::in_degree(step_vertex, *design_flow_graph) == 0)
      {
         AddDesignFlowDependence(design_flow_graph_info->entry, step_vertex, DesignFlowGraph::AUXILIARY);
      }
      CustomOrderedSet<DesignFlowGraph::edge_descriptor> to_be_removeds;
      for(const auto& ie : boost::make_iterator_range(boost::in_edges(step_vertex, *design_flow_graph)))
      {
         const auto source = boost::source(ie, *design_flow_graph);
         if(design_flow_graph->ExistsEdge(source, design_flow_graph_info->exit))
         {
            to_be_removeds.insert(ie);
         }
      }
      for(const auto& ie : to_be_removeds)
      {
         if(RemoveType(ie, DesignFlowGraph::AUXILIARY) == 0)
         {
            RemoveDesignFlowDependence(ie);
         }
      }
      if(boost::out_degree(step_vertex, *design_flow_graph) == 0)
      {
         AddDesignFlowDependence(step_vertex, design_flow_graph_info->exit, DesignFlowGraph::AUXILIARY);
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
#if !HAVE_UNORDERED && !defined(NDEBUG)
   INDENT_DBG_MEX(DEBUG_LEVEL_PARANOIC, debug_level,
                  "---Seed for design flow manager: " +
                      STR(parameters->isOption(OPT_test_single_non_deterministic_flow) ?
                              parameters->getOption<size_t>(OPT_test_single_non_deterministic_flow) :
                              0));
   std::mt19937 urng(parameters->isOption(OPT_test_single_non_deterministic_flow) ?
                         parameters->getOption<size_t>(OPT_test_single_non_deterministic_flow) :
                         0);
   std::uniform_int_distribution<size_t> uid(0, 10000);
#endif

   INDENT_DBG_MEX(DEBUG_LEVEL_PARANOIC, debug_level, "-->Started execution of design flow");
#ifndef NDEBUG
   if(debug_level >= DEBUG_LEVEL_PARANOIC)
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_PARANOIC, debug_level, "---Writing initial design flow graph");
      feedback_design_flow_graph->WriteDot(parameters->getOption<std::filesystem::path>(OPT_dot_directory) /
                                           ("Design_Flow_" + STR(step_counter)));
   }
#endif
   size_t executed_passes = 0;
   size_t skipped_passes = 0;
   size_t graph_changes = 0;
   long long design_flow_manager_time = 0;
   const auto profile_dfm = parameters->IsParameter("dfm_statistics");
   const auto profile_steps = parameters->IsParameter("profile_steps");
   const auto disable_invalidations = parameters->IsParameter("disable-invalidations");

   while(possibly_ready.size())
   {
      const auto initial_number_vertices = boost::num_vertices(*design_flow_graph);
      const auto initial_number_edges = boost::num_edges(*design_flow_graph);
#ifndef NDEBUG
      if(debug_level >= DEBUG_LEVEL_PARANOIC)
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_PARANOIC, debug_level, "-->Ready steps are");
         for(const auto& [step_key, ready_step] : possibly_ready)
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_PARANOIC, debug_level,
                           "---" + design_flow_graph->CGetNodeInfo(ready_step)->design_flow_step->GetName());
         }
         INDENT_DBG_MEX(DEBUG_LEVEL_PARANOIC, debug_level, "<--");
      }
#endif
      long before_time = 0;
      if(profile_dfm)
      {
         START_TIME(before_time);
      }
      step_counter++;
      const auto next = [&]() {
         size_t offset = 0;
#if !HAVE_UNORDERED
#ifndef NDEBUG
         if(parameters->isOption(OPT_test_single_non_deterministic_flow))
         {
            const auto random = uid(urng);
            offset = random % possibly_ready.size();
            INDENT_DBG_MEX(DEBUG_LEVEL_PARANOIC, debug_level,
                           "---Random is " + STR(random) + " - Offset is " + STR(offset) + " (Size is " +
                               STR(possibly_ready.size()) + ")");
         }
#endif
#endif
         return possibly_ready.Extract(offset);
      }();
      const auto& dfs_info = design_flow_graph->GetNodeInfo(next);
      const auto& step = dfs_info->design_flow_step;
      INDENT_DBG_MEX(DEBUG_LEVEL_PARANOIC, debug_level,
                     "-->Beginning iteration number " + STR(step_counter) + " - Considering step " + step->GetName());

      /// Now check if next is actually ready
      /// First of all check if there are new dependence to add
      INDENT_DBG_MEX(DEBUG_LEVEL_PARANOIC, debug_level, "-->Recomputing dependences");
      DesignFlowStepSet step_dependencies, step_precedence;
      CustomUnorderedSet<std::pair<DesignFlowStep::signature_t, bool>> already_addsteps;
      step->ComputeRelationships(step_dependencies, DesignFlowStep::DEPENDENCE_RELATIONSHIP);
      RecursivelyAddSteps(step_dependencies, dfs_info->status == DesignFlowStep_Status::UNNECESSARY, already_addsteps);
      INDENT_DBG_MEX(DEBUG_LEVEL_PARANOIC, debug_level, "<--Recomputed dependences");
      INDENT_DBG_MEX(DEBUG_LEVEL_PARANOIC, debug_level, "-->Recomputing precedences");
      step->ComputeRelationships(step_precedence, DesignFlowStep::PRECEDENCE_RELATIONSHIP);
      RecursivelyAddSteps(step_precedence, true, already_addsteps);
      INDENT_DBG_MEX(DEBUG_LEVEL_PARANOIC, debug_level, "<--Recomputed precedences");
      bool current_ready = true;
      for(const auto& dep : step_dependencies)
      {
         const auto dep_v = design_flow_graph->GetDesignFlowStep(dep->GetSignature());
         AddDesignFlowDependence(dep_v, next, DesignFlowGraph::DEPENDENCE);
         const auto& pre_info = design_flow_graph->GetNodeInfo(dep_v);
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
            default:
            {
               THROW_UNREACHABLE("");
            }
         }
      }
      /// Now iterate on ingoing precedence edge
      for(const auto& prec : step_precedence)
      {
         const auto prec_v = design_flow_graph->GetDesignFlowStep(prec->GetSignature());
         AddDesignFlowDependence(prec_v, next, DesignFlowGraph::PRECEDENCE);
         const auto& pre_info = design_flow_graph->GetNodeInfo(prec_v);
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
      if(profile_dfm)
      {
         STOP_TIME(before_time);
         design_flow_manager_time += before_time;
      }
      if(!current_ready)
      {
#ifndef NDEBUG
         if(debug_level >= DEBUG_LEVEL_PARANOIC)
         {
            feedback_design_flow_graph->WriteDot(parameters->getOption<std::filesystem::path>(OPT_dot_directory) /
                                                 ("Design_Flow_" + STR(step_counter)));
            INDENT_DBG_MEX(DEBUG_LEVEL_PARANOIC, debug_level, "---Writing Design_Flow_" + STR(step_counter));
         }
#endif
         INDENT_DBG_MEX(DEBUG_LEVEL_PARANOIC, debug_level, "<--Ended iteration number " + STR(step_counter));
         const auto final_number_vertices = boost::num_vertices(*design_flow_graph);
         const auto final_number_edges = boost::num_edges(*design_flow_graph);
         if(final_number_vertices > initial_number_vertices || final_number_edges > initial_number_edges)
         {
            graph_changes++;
         }
         continue;
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
         if(OUTPUT_LEVEL_VERY_PEDANTIC <= output_level || profile_steps)
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
         if(OUTPUT_LEVEL_VERY_PEDANTIC <= output_level || profile_steps)
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
         if(profile_steps)
         {
            auto& spi = step_prof_info.at(next);
            spi.accumulated_execution_time += step_execution_time;
            if(dfs_info->status == DesignFlowStep_Status::SUCCESS)
            {
               spi.success++;
            }
            else if(dfs_info->status == DesignFlowStep_Status::UNCHANGED)
            {
               spi.unchanged++;
            }
         }
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
         if(profile_steps)
         {
            step_prof_info.at(next).skipped++;
         }
      }
      long after_time = 0;
      if(profile_dfm)
      {
         START_TIME(after_time);
      }
      bool invalidations = false;
      if(!disable_invalidations)
      {
         /// Add steps and edges from post dependencies
         INDENT_DBG_MEX(DEBUG_LEVEL_PARANOIC, debug_level, "-->Adding post-dependencies of " + step->GetName());
         DesignFlowStepSet relationships;
         step->ComputeRelationships(relationships, DesignFlowStep::INVALIDATION_RELATIONSHIP);
         INDENT_DBG_MEX(DEBUG_LEVEL_PARANOIC, debug_level, "---Got steps");
         if(profile_steps)
         {
            step_prof_info.at(next).direct_invalidations += relationships.size();
         }
         invalidations = !relationships.empty();
         CustomUnorderedSet<vertex_descriptor> already_deexecute;
         for(const auto& relationship : relationships)
         {
            const auto relationship_signature = relationship->GetSignature();
            const auto relationship_vertex = GetDesignFlowStep(relationship_signature);
            THROW_ASSERT(relationship_vertex, "Missing vertex " + relationship->GetName());
            if(design_flow_graph->IsReachable(relationship_vertex, next))
            {
#ifndef NDEBUG
               AddDesignFlowDependence(next, relationship_vertex, DesignFlowGraph::FEEDBACK);
#endif
               const auto step_count = DeExecute(relationship_vertex, true, already_deexecute);
               if(profile_steps)
               {
                  step_prof_info.at(next).total_invalidations += step_count;
               }
            }
            else
            {
#ifndef NDEBUG
               feedback_design_flow_graph->WriteDot(parameters->getOption<std::filesystem::path>(OPT_dot_directory) /
                                                    "Design_Flow_Error");
#endif
               THROW_UNREACHABLE("Invalidating " +
                                 design_flow_graph->CGetNodeInfo(relationship_vertex)->design_flow_step->GetName() +
                                 " which is not before the current one");
            }
         }
         INDENT_DBG_MEX(DEBUG_LEVEL_PARANOIC, debug_level, "<--Added post-dependencies of " + step->GetName());
      }
      INDENT_DBG_MEX(DEBUG_LEVEL_PARANOIC, debug_level, "-->Starting checking of new ready steps");
      for(const auto& oe : boost::make_iterator_range(boost::out_edges(next, *design_flow_graph)))
      {
         const auto target = boost::target(oe, *design_flow_graph);
         auto& target_info = design_flow_graph->GetNodeInfo(target);
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
            default:
            {
               THROW_UNREACHABLE("");
            }
         }
         INDENT_DBG_MEX(DEBUG_LEVEL_PARANOIC, debug_level,
                        "-->Examining successor " + target_info->design_flow_step->GetName());
         bool target_ready = true;
         for(const auto& ie : boost::make_iterator_range(boost::in_edges(target, *design_flow_graph)))
         {
            const auto source = boost::source(ie, *design_flow_graph);
            const auto& source_info = design_flow_graph->GetNodeInfo(source);
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
                           "---Adding " + target_info->design_flow_step->GetName() + " to list of ready steps");
            possibly_ready.insert(target);
         }
         INDENT_DBG_MEX(DEBUG_LEVEL_PARANOIC, debug_level, "<--");
      }
      INDENT_DBG_MEX(DEBUG_LEVEL_PARANOIC, debug_level, "<--Checked new ready steps");
      CustomOrderedSet<DesignFlowGraph::edge_descriptor> to_be_removeds;
      for(const auto& ie :
          boost::make_iterator_range(boost::in_edges(design_flow_graph->CGetGraphInfo()->exit, *design_flow_graph)))
      {
         const auto source = boost::source(ie, *design_flow_graph);
         if(boost::out_degree(source, *design_flow_graph) > 1)
         {
            to_be_removeds.insert(ie);
         }
      }
      for(const auto& ie : to_be_removeds)
      {
         if(RemoveType(ie, DesignFlowGraph::AUXILIARY) == 0)
         {
            RemoveDesignFlowDependence(ie);
         }
      }
      if(profile_dfm)
      {
         STOP_TIME(after_time);
         design_flow_manager_time += after_time;
      }
#ifndef NDEBUG
      if(debug_level >= DEBUG_LEVEL_PARANOIC)
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_PARANOIC, debug_level, "---Writing Design_Flow_" + STR(step_counter));
         feedback_design_flow_graph->WriteDot(parameters->getOption<std::filesystem::path>(OPT_dot_directory) /
                                              ("Design_Flow_" + STR(step_counter)));
      }
#endif
      INDENT_DBG_MEX(DEBUG_LEVEL_PARANOIC, debug_level,
                     "<--Ended iteration number " + STR(step_counter) + " - Step " + step->GetName());
      if(profile_dfm)
      {
         const auto final_number_vertices = boost::num_vertices(*design_flow_graph);
         const auto final_number_edges = boost::num_edges(*design_flow_graph);
         if(final_number_vertices > initial_number_vertices || final_number_edges > initial_number_edges or
            invalidations)
         {
            graph_changes++;
         }
         if(parameters->GetParameter<size_t>("dfm_statistics") > 1)
         {
            size_t executed_vertices = 0;
            static size_t previous_executed_vertices = 0;
            for(const auto& v : boost::make_iterator_range(boost::vertices(*design_flow_graph)))
            {
               switch(design_flow_graph->GetNodeInfo(v)->status)
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
   if(design_flow_graph->CGetNodeInfo(design_flow_graph->CGetGraphInfo()->exit)->status != DesignFlowStep_Status::EMPTY)
   {
#ifndef NDEBUG
      WriteLoopDot();
      feedback_design_flow_graph->WriteDot(parameters->getOption<std::filesystem::path>(OPT_dot_directory) /
                                           "Design_Flow_Error");
#endif
      THROW_UNREACHABLE("Design flow didn't end");
   }
   if(profile_dfm)
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
   if(profile_steps)
   {
      const auto output_directory = parameters->getOption<std::filesystem::path>(OPT_output_directory) / "design_flow";
      std::filesystem::create_directories(output_directory);
      const auto csv_report = output_directory / "step_stats.csv";
      std::ofstream csv_report_stream(csv_report);
      csv_report_stream << "Step name,Accumulated execution time,Total runs,Success runs,Unchanged runs,Skipped "
                           "runs,Direct invalidated dependencies,Total invalidated dependencies\n";
      for(const auto& [v, spi] : step_prof_info)
      {
         csv_report_stream << spi.name << "," << print_cpu_time(spi.accumulated_execution_time) << ","
                           << (spi.success + spi.unchanged) << "," << spi.success << "," << spi.unchanged << ","
                           << spi.skipped << "," << spi.direct_invalidations << "," << spi.total_invalidations << "\n";
      }
      csv_report_stream.close();
      INDENT_OUT_MEX(OUTPUT_LEVEL_NONE, output_level, "---Steps execution statistics stored in " + csv_report.string());
   }
   INDENT_DBG_MEX(DEBUG_LEVEL_PARANOIC, debug_level, "---Total number of iterations: " + STR(step_counter));
   INDENT_DBG_MEX(DEBUG_LEVEL_PARANOIC, debug_level, "<--Ended execution of design flow");
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

size_t DesignFlowManager::DeExecute(const vertex_descriptor starting_vertex, const bool force_execution,
                                    CustomUnorderedSet<vertex_descriptor>& already_visited)
{
   size_t deex_count = 0;
   if(!already_visited.insert(starting_vertex).second)
   {
      return deex_count;
   }
   const auto& dfs_info = design_flow_graph->GetNodeInfo(starting_vertex);
   INDENT_DBG_MEX(DEBUG_LEVEL_PARANOIC, debug_level, "---DeExecuting " + dfs_info->design_flow_step->GetName());
   switch(dfs_info->status)
   {
      case DesignFlowStep_Status::SUCCESS:
      case DesignFlowStep_Status::ABORTED:
      case DesignFlowStep_Status::EMPTY:
      case DesignFlowStep_Status::UNCHANGED:
      {
         dfs_info->status = DesignFlowStep_Status::UNEXECUTED;
         deex_count++;
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
         deex_count++;
         break;
      }
      case DesignFlowStep_Status::UNNECESSARY:
      case DesignFlowStep_Status::UNEXECUTED:
      {
         break;
      }
      case DesignFlowStep_Status::NONEXISTENT:
      default:
      {
         THROW_UNREACHABLE("");
      }
   }

   /// Check if the vertex is already ready
   bool current_ready = true;
   for(const auto& ie : boost::make_iterator_range(boost::in_edges(starting_vertex, *design_flow_graph)))
   {
      const auto dep_v = boost::source(ie, *design_flow_graph);
      const auto& pre_info = design_flow_graph->GetNodeInfo(dep_v);
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

   /// Propagating to successor
   for(const auto& oe : boost::make_iterator_range(boost::out_edges(starting_vertex, *design_flow_graph)))
   {
      const auto target = boost::target(oe, *design_flow_graph);
      const auto& target_info = design_flow_graph->GetNodeInfo(target);
      switch(target_info->status)
      {
         case DesignFlowStep_Status::ABORTED:
         case DesignFlowStep_Status::EMPTY:
         case DesignFlowStep_Status::SUCCESS:
         case DesignFlowStep_Status::UNCHANGED:
         case DesignFlowStep_Status::SKIPPED:
         {
            deex_count += DeExecute(target, false, already_visited);
            break;
         }
         case DesignFlowStep_Status::UNNECESSARY:
         case DesignFlowStep_Status::UNEXECUTED:
         {
            break;
         }
         case DesignFlowStep_Status::NONEXISTENT:
         default:
         {
            THROW_UNREACHABLE("");
         }
      }
   }
   return deex_count;
}

DesignFlowStep_Status DesignFlowManager::GetStatus(DesignFlowStep::signature_t signature) const
{
   const auto step = GetDesignFlowStep(signature);
   return step == DesignFlowGraph::null_vertex() ? DesignFlowStep_Status::NONEXISTENT :
                                                   design_flow_graph->CGetNodeInfo(step)->status;
}

DesignFlowStepRef DesignFlowManager::CreateFlowStep(DesignFlowStep::signature_t signature) const
{
   return CGetDesignFlowStepFactory(DesignFlowStep::GetStepClass(signature))->CreateFlowStep(signature);
}

#ifndef NDEBUG
void DesignFlowManager::WriteLoopDot() const
{
   const auto sccs = feedback_design_flow_graph->GetStronglyConnectedComponents();
   size_t scc_id = 0;
   for(const auto& scc : sccs)
   {
      if(scc.size() > 1)
      {
         const auto file_name = parameters->getOption<std::filesystem::path>(OPT_dot_directory) /
                                ("DesignFlowLoop_" + STR(scc_id) + ".dot");
         std::filesystem::create_directories(file_name.parent_path());
         const DesignFlowStepWriter sw(feedback_design_flow_graph.get());
         const DesignFlowEdgeWriter ew(feedback_design_flow_graph.get());
         graph_base<boost::filtered_graph<DesignFlowGraph, boost::keep_all, SelectVertex<DesignFlowGraph>>>(
             *feedback_design_flow_graph, boost::keep_all(),
             SelectVertex<DesignFlowGraph>(CustomUnorderedSet<vertex_descriptor>(scc.begin(), scc.end())))
             .WriteDot(file_name, sw, ew);
      }
      ++scc_id;
   }
}
#endif
