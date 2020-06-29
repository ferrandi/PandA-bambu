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
 * @file design_flow_manager.cpp
 * @brief Wrapper of design_flow
 *
 * @author Marco Lattuada <lattuada@elet.polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *
 */
#include "design_flow_manager.hpp"

#include "config_HAVE_ASSERTS.hpp"   // for HAVE_ASSERTS
#include "config_HAVE_UNORDERED.hpp" // for HAVE_UNORDERED

#include <boost/graph/adjacency_list.hpp>     // for adjacency_list, source
#include <boost/graph/filtered_graph.hpp>     // for in_edges, num_vertices
#include <boost/iterator/filter_iterator.hpp> // for filter_iterator
#include <boost/iterator/iterator_facade.hpp> // for operator!=, operator++
#include <boost/lexical_cast.hpp>             // for lexical_cast
#include <boost/tuple/tuple.hpp>              // for tie
#include <iterator>                           // for advance
#include <list>                               // for list
#if !HAVE_UNORDERED
#ifndef NDEBUG
#include <random> // for uniform_int_distrib...
#endif
#endif
#include "Parameter.hpp" // for Parameter, OPT_test...
#include "cpu_stats.hpp" // for PrintVirtualDataMem...
#include "cpu_time.hpp"  // for START_TIME, STOP_TIME
#include "custom_set.hpp"
#include "dbgPrintHelper.hpp"           // for DEBUG_LEVEL_VERY_PE...
#include "design_flow_aux_step.hpp"     // for AuxDesignFlowStep
#include "design_flow_graph.hpp"        // for DesignFlowGraph
#include "design_flow_step.hpp"         // for DesignFlowStep_Status
#include "design_flow_step_factory.hpp" // for DesignFlowStepRef
#include "exceptions.hpp"               // for THROW_UNREACHABLE
#include "string_manipulation.hpp"      // for STR GET_CLASS
#include <utility>                      // for pair

DesignFlowStepNecessitySorter::DesignFlowStepNecessitySorter(const DesignFlowGraphConstRef _design_flow_graph) : design_flow_graph(_design_flow_graph)
{
}

bool DesignFlowStepNecessitySorter::operator()(const vertex x, const vertex y) const
{
   const DesignFlowStepInfoConstRef x_info = design_flow_graph->CGetDesignFlowStepInfo(x);
   const DesignFlowStepInfoConstRef y_info = design_flow_graph->CGetDesignFlowStepInfo(y);
   const bool x_composed = x_info->design_flow_step->IsComposed();
   const bool y_composed = y_info->design_flow_step->IsComposed();
   const bool x_unnecessary = x_info->status == DesignFlowStep_Status::SKIPPED or x_info->status == DesignFlowStep_Status::UNNECESSARY;
   const bool y_unnecessary = y_info->status == DesignFlowStep_Status::SKIPPED or y_info->status == DesignFlowStep_Status::UNNECESSARY;
   if(x_composed and not y_composed)
   {
      return true;
   }
   else if(not x_composed and y_composed)
   {
      return false;
   }
   else if(x_unnecessary and not y_unnecessary)
   {
      return false;
   }
   else if(not x_unnecessary and y_unnecessary)
   {
      return true;
   }
   else
   {
#if HAVE_UNORDERED
      return x < y;
#else
      return x_info->design_flow_step->GetName() < y_info->design_flow_step->GetName();
#endif
   }
}

DesignFlowManager::DesignFlowManager(const ParameterConstRef _parameters)
    : design_flow_graphs_collection(new DesignFlowGraphsCollection(_parameters)),
      design_flow_graph(new DesignFlowGraph(design_flow_graphs_collection, DesignFlowGraph::DEPENDENCE_SELECTOR | DesignFlowGraph::PRECEDENCE_SELECTOR | DesignFlowGraph::AUX_SELECTOR)),
      feedback_design_flow_graph(new DesignFlowGraph(design_flow_graphs_collection, DesignFlowGraph::DEPENDENCE_SELECTOR | DesignFlowGraph::PRECEDENCE_SELECTOR | DesignFlowGraph::AUX_SELECTOR | DesignFlowGraph::DEPENDENCE_FEEDBACK_SELECTOR)),
      possibly_ready(std::set<vertex, DesignFlowStepNecessitySorter>(DesignFlowStepNecessitySorter(design_flow_graph))),
      parameters(_parameters),
      output_level(_parameters->getOption<int>(OPT_output_level))
{
   debug_level = parameters->get_class_debug_level(GET_CLASS(*this));
   const DesignFlowGraphInfoRef design_flow_graph_info = design_flow_graph->GetDesignFlowGraphInfo();
   null_deleter nullDel;
   design_flow_graph_info->entry = design_flow_graphs_collection->AddDesignFlowStep(DesignFlowStepRef(new AuxDesignFlowStep("Entry", DESIGN_FLOW_ENTRY, DesignFlowManagerConstRef(this, nullDel), parameters)), false);
#ifndef NDEBUG
   if(debug_level >= DEBUG_LEVEL_VERY_PEDANTIC or parameters->IsParameter("profile_steps"))
   {
      step_names[design_flow_graph_info->entry] = "Entry";
   }
#endif
   const DesignFlowStepInfoRef entry_info = design_flow_graph->GetDesignFlowStepInfo(design_flow_graph_info->entry);
   entry_info->status = DesignFlowStep_Status::EMPTY;
   design_flow_graph_info->exit = design_flow_graphs_collection->AddDesignFlowStep(DesignFlowStepRef(new AuxDesignFlowStep("Exit", DESIGN_FLOW_EXIT, DesignFlowManagerConstRef(this, nullDel), parameters)), false);
#ifndef NDEBUG
   if(debug_level >= DEBUG_LEVEL_VERY_PEDANTIC or parameters->IsParameter("profile_steps"))
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

void DesignFlowManager::RecursivelyAddSteps(const DesignFlowStepSet& steps, const bool unnecessary)
{
   static size_t temp_counter = 0;
   const DesignFlowGraphInfoRef design_flow_graph_info = design_flow_graph->GetDesignFlowGraphInfo();
   DesignFlowStepSet steps_to_be_processed = steps;
   while(steps_to_be_processed.size())
   {
      const DesignFlowStepRef design_flow_step = *(steps_to_be_processed.begin());
      steps_to_be_processed.erase(design_flow_step);
      const std::string signature = design_flow_step->GetSignature();
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Adding design flow step " + design_flow_step->GetName() + " - Signature " + signature);

      /// Get vertex from design flow graph; there are four cases
      vertex step_vertex = GetDesignFlowStep(signature);
      /// The step already exists
      if(step_vertex)
      {
         if(unnecessary)
         {
            /// The step already exists and we are trying to readd as unnecessary; both if now it is unnecessary or not, nothing has to be done
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--This step already exist (unnecessary)");
            continue;
         }
         else
         {
            const DesignFlowStepInfoRef design_flow_step_info = design_flow_graph->GetDesignFlowStepInfo(step_vertex);
            if(design_flow_step_info->status == DesignFlowStep_Status::UNNECESSARY)
            {
               /// The step already exists and it was unnecessary; now we are switching to necessary; note that computation of relationships of this node is performed to propagate the necessity
               /// If design flow step was ready I have to reinsert it into the set because of the ordering; so the setting of the unnecessary flag can not be factorized
               bool it_belongs = possibly_ready.find(step_vertex) != possibly_ready.end();
               possibly_ready.erase(step_vertex);
               if(it_belongs)
               {
                  design_flow_step_info->status = DesignFlowStep_Status::UNEXECUTED;
                  possibly_ready.insert(step_vertex);
               }
               else
               {
                  design_flow_step_info->status = DesignFlowStep_Status::UNEXECUTED;
               }
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---This step already exist but was unnecessary. Now it becomes necessary");
            }
#if 0
            else if(design_flow_step_info->status == DesignFlowStep_Status::SKIPPED)
            {
               /// The step already exists and it is already necessary; nothing to do
               design_flow_step_info->status = DesignFlowStep_Status::UNEXECUTED;
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---This step already exist (skipped)");
            }
#endif
            else
            {
               THROW_ASSERT(design_flow_step_info->status != DesignFlowStep_Status::SKIPPED, "Switching to necessary " + design_flow_step_info->design_flow_step->GetName() + " which has already skipped");
               /// The step already exists and it is already necessary; nothing to do
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--This step already exist");
               continue;
            }
         }
      }
      else
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---This step does not exist");
         step_vertex = design_flow_graphs_collection->AddDesignFlowStep(design_flow_step, unnecessary);
#ifndef NDEBUG
         if(debug_level >= DEBUG_LEVEL_VERY_PEDANTIC or parameters->IsParameter("profile_steps"))
         {
            step_names[step_vertex] = design_flow_step->GetName();
         }
#endif
      }

      DesignFlowStepSet relationships;

      /// Add edges from dependencies
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Adding dependencies of " + design_flow_step->GetName());
      design_flow_step->ComputeRelationships(relationships, DesignFlowStep::DEPENDENCE_RELATIONSHIP);
      RecursivelyAddSteps(relationships, unnecessary);
      DesignFlowStepSet::const_iterator relationship, relationship_end = relationships.end();
      for(relationship = relationships.begin(); relationship != relationship_end; ++relationship)
      {
         const std::string relationship_signature = (*relationship)->GetSignature();
         vertex relationship_vertex = GetDesignFlowStep(relationship_signature);
         design_flow_graphs_collection->AddDesignFlowDependence(relationship_vertex, step_vertex, DesignFlowGraph::DEPENDENCE_SELECTOR);
#ifndef NDEBUG
         if(parameters->IsParameter("DebugDFM") and parameters->GetParameter<bool>("DebugDFM"))
         {
            try
            {
               std::list<vertex> vertices;
               design_flow_graph->TopologicalSort(vertices);
            }
            catch(const char* msg)
            {
               feedback_design_flow_graph->WriteDot("Design_Flow_Error");
               THROW_UNREACHABLE("Design flow graph is not anymore acyclic");
            }
            catch(const std::string& msg)
            {
               feedback_design_flow_graph->WriteDot("Design_Flow_Error");
               THROW_UNREACHABLE("Design flow graph is not anymore acyclic");
            }
            catch(const std::exception& ex)
            {
               feedback_design_flow_graph->WriteDot("Design_Flow_Error");
               THROW_UNREACHABLE("Design flow graph is not anymore acyclic");
            }
            catch(...)
            {
               feedback_design_flow_graph->WriteDot("Design_Flow_Error");
               THROW_UNREACHABLE("Design flow graph is not anymore acyclic");
            }
         }
#endif
      }
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Added dependencies  of " + design_flow_step->GetName());

      while(relationships.size())
      {
         relationships.erase(relationships.begin());
      }

      /// Add steps from precedences
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Adding precedences of " + design_flow_step->GetName());
      relationships.clear();
      design_flow_step->ComputeRelationships(relationships, DesignFlowStep::PRECEDENCE_RELATIONSHIP);
      RecursivelyAddSteps(relationships, true);
      relationship_end = relationships.end();
      for(relationship = relationships.begin(); relationship != relationship_end; ++relationship)
      {
         const std::string relationship_signature = (*relationship)->GetSignature();
         vertex relationship_vertex = GetDesignFlowStep(relationship_signature);
         design_flow_graphs_collection->AddDesignFlowDependence(relationship_vertex, step_vertex, DesignFlowGraph::PRECEDENCE_SELECTOR);
#ifndef NDEBUG
         if(parameters->IsParameter("DebugDFM") and parameters->GetParameter<bool>("DebugDFM"))
         {
            try
            {
               std::list<vertex> vertices;
               design_flow_graph->TopologicalSort(vertices);
            }
            catch(const char* msg)
            {
               feedback_design_flow_graph->WriteDot("Design_Flow_Error");
               THROW_UNREACHABLE("Design flow graph is not anymore acyclic");
            }
            catch(const std::string& msg)
            {
               feedback_design_flow_graph->WriteDot("Design_Flow_Error");
               THROW_UNREACHABLE("Design flow graph is not anymore acyclic");
            }
            catch(const std::exception& ex)
            {
               feedback_design_flow_graph->WriteDot("Design_Flow_Error");
               THROW_UNREACHABLE("Design flow graph is not anymore acyclic");
            }
            catch(...)
            {
               feedback_design_flow_graph->WriteDot("Design_Flow_Error");
               THROW_UNREACHABLE("Design flow graph is not anymore acyclic");
            }
         }
#endif
      }
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Added precedences of " + design_flow_step->GetName());

      /// Check if the added step is already ready
      bool current_ready = true;
      InEdgeIterator ie, ie_end;
      for(boost::tie(ie, ie_end) = boost::in_edges(step_vertex, *design_flow_graph); ie != ie_end; ie++)
      {
         vertex pre_dependence_vertex = boost::source(*ie, *design_flow_graph);
         const DesignFlowStepInfoRef pre_info = design_flow_graph->GetDesignFlowStepInfo(pre_dependence_vertex);
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
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Adding " + design_flow_step->GetName() + " to list of ready steps");
         possibly_ready.insert(step_vertex);
      }
      if(debug_level >= DEBUG_LEVEL_VERY_PEDANTIC)
      {
         feedback_design_flow_graph->WriteDot("Design_Flow_" + boost::lexical_cast<std::string>(step_counter) + "_" + boost::lexical_cast<std::string>(temp_counter));
         temp_counter++;
      }
      if(boost::in_degree(step_vertex, *design_flow_graph) == 0)
      {
         design_flow_graphs_collection->AddDesignFlowDependence(design_flow_graph_info->entry, step_vertex, DesignFlowGraph::AUX_SELECTOR);
      }
      for(boost::tie(ie, ie_end) = boost::in_edges(step_vertex, *design_flow_graph); ie != ie_end; ie++)
      {
         const auto source = boost::source(*ie, *design_flow_graph);
         if(design_flow_graph->ExistsEdge(source, design_flow_graph_info->exit))
         {
            design_flow_graphs_collection->RemoveSelector(source, design_flow_graph_info->exit, DesignFlowGraph::AUX_SELECTOR);
         }
      }
      if(boost::out_degree(step_vertex, *design_flow_graph) == 0)
      {
         design_flow_graphs_collection->AddDesignFlowDependence(step_vertex, design_flow_graph_info->exit, DesignFlowGraph::AUX_SELECTOR);
      }
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Added design flow step " + design_flow_step->GetName() + " - Signature " + signature);
   }
}

const DesignFlowGraphConstRef DesignFlowManager::CGetDesignFlowGraph() const
{
   return design_flow_graph;
}

void DesignFlowManager::Exec()
{
#if !HAVE_UNORDERED
#ifndef NDEBUG
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Seed for design flow manager: " + STR(parameters->isOption(OPT_test_single_non_deterministic_flow) ? parameters->getOption<size_t>(OPT_test_single_non_deterministic_flow) : 0));
   auto generator = std::bind(std::uniform_int_distribution<size_t>(0, 10000), std::mt19937(parameters->isOption(OPT_test_single_non_deterministic_flow) ? parameters->getOption<size_t>(OPT_test_single_non_deterministic_flow) : 0));
#endif
#endif

   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Started execution of design flow");
   if(debug_level >= DEBUG_LEVEL_VERY_PEDANTIC)
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Writing initial design flow graph");
      feedback_design_flow_graph->WriteDot("Design_Flow_" + boost::lexical_cast<std::string>(step_counter));
   }
   size_t executed_passes = 0;
   size_t skipped_passes = 0;
   size_t graph_changes = 0;
   long design_flow_manager_time = 0;
   while(possibly_ready.size())
   {
      const size_t initial_number_vertices = boost::num_vertices(*feedback_design_flow_graph);
      const size_t initial_number_edges = boost::num_vertices(*feedback_design_flow_graph);
      long before_time = 0;
      if(parameters->IsParameter("dfm_statistics"))
         START_TIME(before_time);
      step_counter++;
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Ready steps are");
#ifndef NDEBUG
      for(const auto ready_step : possibly_ready)
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---" + design_flow_graph->CGetDesignFlowStepInfo(ready_step)->design_flow_step->GetName());
      }
#endif
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--");
      const vertex next = [&]() -> vertex {
#if !HAVE_UNORDERED
#ifndef NDEBUG
         if(parameters->isOption(OPT_test_single_non_deterministic_flow))
         {
            const size_t random = generator();
            const size_t offset = random % possibly_ready.size();
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Random is " + STR(random) + " - Offset is " + STR(offset) + " (Size is " + STR(possibly_ready.size()) + ")");
            auto begin = possibly_ready.begin();
            std::advance(begin, offset);
            return *begin;
         }
#endif
#endif
         return *(possibly_ready.begin());
      }();
#if HAVE_ASSERTS
      const auto erased_elements =
#endif
          possibly_ready.erase(next);
      const DesignFlowStepInfoRef design_flow_step_info = design_flow_graph->GetDesignFlowStepInfo(next);
      const DesignFlowStepRef step = design_flow_step_info->design_flow_step;
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Beginning iteration number " + boost::lexical_cast<std::string>(step_counter) + " - Considering step " + step->GetName());
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Other ready steps are");
#ifndef NDEBUG
      for(const auto ready_step : possibly_ready)
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---" + design_flow_graph->CGetDesignFlowStepInfo(ready_step)->design_flow_step->GetName());
      }
#endif
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--");
      THROW_ASSERT(erased_elements == 1, "Number of erased elements is " + STR(erased_elements));

      /// Now check if next is actually ready
      /// First of all check if there are new dependence to add
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Recomputing dependences");
      DesignFlowStepSet pre_dependence_steps, pre_precedence_steps;
      step->ComputeRelationships(pre_dependence_steps, DesignFlowStep::DEPENDENCE_RELATIONSHIP);
      RecursivelyAddSteps(pre_dependence_steps, design_flow_step_info->status == DesignFlowStep_Status::UNNECESSARY);
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Recomputed dependences");
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Recomputing precedences");
      step->ComputeRelationships(pre_precedence_steps, DesignFlowStep::PRECEDENCE_RELATIONSHIP);
      RecursivelyAddSteps(pre_precedence_steps, true);
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Recomputed precedences");
      bool current_ready = true;
      DesignFlowStepSet::const_iterator pre_dependence_step, pre_dependence_step_end = pre_dependence_steps.end();
      for(pre_dependence_step = pre_dependence_steps.begin(); pre_dependence_step != pre_dependence_step_end; ++pre_dependence_step)
      {
         const vertex pre_dependence_vertex = design_flow_graph->GetDesignFlowStep((*pre_dependence_step)->GetSignature());
         design_flow_graphs_collection->AddDesignFlowDependence(pre_dependence_vertex, next, DesignFlowGraph::DEPENDENCE_SELECTOR);
         const DesignFlowStepInfoRef pre_info = design_flow_graph->GetDesignFlowStepInfo(pre_dependence_vertex);
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
      for(auto pre_precedence_step : pre_precedence_steps)
      {
         const vertex pre_precedence_vertex = design_flow_graph->GetDesignFlowStep((pre_precedence_step)->GetSignature());
         design_flow_graphs_collection->AddDesignFlowDependence(pre_precedence_vertex, next, DesignFlowGraph::PRECEDENCE_SELECTOR);
         const DesignFlowStepInfoRef pre_info = design_flow_graph->GetDesignFlowStepInfo(pre_precedence_vertex);
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
      if(not current_ready)
      {
         if(debug_level >= DEBUG_LEVEL_VERY_PEDANTIC)
         {
            feedback_design_flow_graph->WriteDot("Design_Flow_" + boost::lexical_cast<std::string>(step_counter));
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Writing Design_Flow_" + boost::lexical_cast<std::string>(step_counter));
         }
#ifndef NDEBUG
         if(debug_level >= DEBUG_LEVEL_VERY_PEDANTIC)
         {
            /// Save the current status of the graph in history
            VertexIterator temp_step, temp_step_end;
            for(boost::tie(temp_step, temp_step_end) = boost::vertices(*design_flow_graph); temp_step != temp_step_end; temp_step++)
            {
               const DesignFlowStepInfoConstRef temp_design_flow_step_info = design_flow_graph->CGetDesignFlowStepInfo(*temp_step);
               vertex_history[step_counter][*temp_step] = temp_design_flow_step_info->status;
            }
            EdgeIterator edge, edge_end;
            for(boost::tie(edge, edge_end) = boost::edges(*design_flow_graph); edge != edge_end; edge++)
            {
               edge_history[step_counter][*edge] = design_flow_graph->GetSelector(*edge);
            }
         }
#endif
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Ended iteration number " + boost::lexical_cast<std::string>(step_counter));
         const size_t final_number_vertices = boost::num_vertices(*feedback_design_flow_graph);
         const size_t final_number_edges = boost::num_vertices(*feedback_design_flow_graph);
         if(final_number_vertices > initial_number_vertices or final_number_edges > initial_number_edges)
         {
            graph_changes++;
         }
         continue;
      }
      if(parameters->IsParameter("dfm_statistics"))
      {
         STOP_TIME(before_time);
         design_flow_manager_time += before_time;
      }
      if(design_flow_step_info->status == DesignFlowStep_Status::UNNECESSARY)
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Skipping execution of " + step->GetName() + " since unnecessary");
         design_flow_step_info->status = DesignFlowStep_Status::SKIPPED;
      }
      else if(step->HasToBeExecuted())
      {
#ifndef NDEBUG
         size_t indentation_before = indentation;
#endif
         INDENT_OUT_MEX(OUTPUT_LEVEL_VERY_PEDANTIC, output_level, "-->Starting execution of " + step->GetName());
         long step_execution_time = 0;
         if(OUTPUT_LEVEL_VERY_PEDANTIC <= output_level || parameters->IsParameter("profile_steps"))
            START_TIME(step_execution_time);
         step->Initialize();
         if(step->CGetDebugLevel() >= DEBUG_LEVEL_VERY_PEDANTIC)
         {
            step->PrintInitialIR();
         }
         design_flow_step_info->status = step->Exec();
         executed_passes++;
         if(step->CGetDebugLevel() >= DEBUG_LEVEL_VERY_PEDANTIC)
         {
            step->PrintFinalIR();
         }
         if(OUTPUT_LEVEL_VERY_PEDANTIC <= output_level || parameters->IsParameter("profile_steps"))
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
            if(design_flow_step_info->status == DesignFlowStep_Status::SUCCESS)
            {
               success_executions[next]++;
            }
            else if(design_flow_step_info->status == DesignFlowStep_Status::UNCHANGED)
            {
               unchanged_executions[next]++;
            }
         }
#endif
         INDENT_OUT_MEX(OUTPUT_LEVEL_VERY_PEDANTIC, output_level, "<--Ended execution of " + step->GetName() + " in " + print_cpu_time(step_execution_time) + " seconds" + memory_usage);
#ifndef NDEBUG
         THROW_ASSERT(indentation_before == indentation, "Not closed indentation");
#endif
      }
      else
      {
         INDENT_OUT_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Skipping execution of " + step->GetName());
         design_flow_step_info->status = DesignFlowStep_Status::UNCHANGED;
         skipped_passes++;
#ifndef NDEBUG
         if(parameters->IsParameter("profile_steps"))
         {
            skipped_executions[next]++;
         }
#endif
      }
      long after_time = 0;
      if(parameters->IsParameter("dfm_statistics"))
         START_TIME(after_time);
      bool invalidations = false;
      if(not parameters->IsParameter("disable-invalidations"))
      {
         /// Add steps and edges from post dependencies
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Adding post-dependencies of " + step->GetName());
         DesignFlowStepSet relationships;
         step->ComputeRelationships(relationships, DesignFlowStep::INVALIDATION_RELATIONSHIP);
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Got steps");
         invalidations = not relationships.empty();
         DesignFlowStepSet::const_iterator relationship, relationship_end = relationships.end();
         for(relationship = relationships.begin(); relationship != relationship_end; ++relationship)
         {
            const std::string relationship_signature = (*relationship)->GetSignature();
            vertex relationship_vertex = GetDesignFlowStep(relationship_signature);
            THROW_ASSERT(relationship_vertex, "Missing vertex " + relationship_signature);
            if(design_flow_graph->IsReachable(relationship_vertex, next))
            {
               design_flow_graphs_collection->AddDesignFlowDependence(next, relationship_vertex, DesignFlowGraph::DEPENDENCE_FEEDBACK_SELECTOR);
               DeExecute(relationship_vertex, true);
            }
            else
            {
               feedback_design_flow_graph->WriteDot("Design_Flow_Error");
               THROW_UNREACHABLE("Invalidating " + design_flow_graph->CGetDesignFlowStepInfo(relationship_vertex)->design_flow_step->GetName() + " which is not before the current one");
            }
         }
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Added post-dependencies of " + step->GetName());
      }
      OutEdgeIterator oe, oe_end;
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Starting checking of new ready steps");
      for(boost::tie(oe, oe_end) = boost::out_edges(next, *design_flow_graph); oe != oe_end; oe++)
      {
         const vertex target = boost::target(*oe, *design_flow_graph);
         DesignFlowStepInfoRef target_info = design_flow_graph->GetDesignFlowStepInfo(target);
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
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Examining successor " + design_flow_graph->GetDesignFlowStepInfo(target)->design_flow_step->GetName());
         bool target_ready = true;
         InEdgeIterator ie, ie_end;
         for(boost::tie(ie, ie_end) = boost::in_edges(target, *design_flow_graph); ie != ie_end; ie++)
         {
            const vertex source = boost::source(*ie, *design_flow_graph);
            const DesignFlowStepInfoRef source_info = design_flow_graph->GetDesignFlowStepInfo(source);
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Examining predecessor " + source_info->design_flow_step->GetName());
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
            if(not target_ready)
            {
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Not ready");
               break;
            }
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--");
         }
         if(target_ready)
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Adding " + design_flow_graph->CGetDesignFlowStepInfo(target)->design_flow_step->GetName() + " to list of ready steps");
            possibly_ready.insert(target);
         }
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--");
      }
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Checked new ready steps");
      CustomOrderedSet<EdgeDescriptor> to_be_removeds;
      InEdgeIterator ie, ie_end;
      for(boost::tie(ie, ie_end) = boost::in_edges(design_flow_graph->CGetDesignFlowGraphInfo()->exit, *design_flow_graph); ie != ie_end; ie++)
      {
         const auto source = boost::source(*ie, *design_flow_graph);
         if(boost::out_degree(source, *design_flow_graph) > 1)
         {
            to_be_removeds.insert(*ie);
         }
      }
      for(const auto& to_be_removed : to_be_removeds)
      {
         design_flow_graphs_collection->RemoveSelector(to_be_removed, DesignFlowGraph::AUX_SELECTOR);
      }
      if(debug_level >= DEBUG_LEVEL_VERY_PEDANTIC)
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Writing Design_Flow_" + boost::lexical_cast<std::string>(step_counter));
         feedback_design_flow_graph->WriteDot("Design_Flow_" + boost::lexical_cast<std::string>(step_counter));

#ifndef NDEBUG
         /// Save the current status of the graph in history
         VertexIterator temp_step, temp_step_end;
         for(boost::tie(temp_step, temp_step_end) = boost::vertices(*design_flow_graph); temp_step != temp_step_end; temp_step++)
         {
            const DesignFlowStepInfoConstRef temp_design_flow_step_info = design_flow_graph->CGetDesignFlowStepInfo(*temp_step);
            vertex_history[step_counter][*temp_step] = temp_design_flow_step_info->status;
         }
         EdgeIterator edge, edge_end;
         for(boost::tie(edge, edge_end) = boost::edges(*design_flow_graph); edge != edge_end; edge++)
         {
            edge_history[step_counter][*edge] = design_flow_graph->GetSelector(*edge);
         }
#endif
      }
      if(parameters->IsParameter("dfm_statistics"))
      {
         STOP_TIME(after_time);
         design_flow_manager_time += after_time;
      }
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Ended iteration number " + boost::lexical_cast<std::string>(step_counter) + " - Step " + step->GetName());
      const size_t final_number_vertices = boost::num_vertices(*feedback_design_flow_graph);
      const size_t final_number_edges = boost::num_vertices(*feedback_design_flow_graph);
      if(parameters->IsParameter("dfm_statistics"))
      {
         if(final_number_vertices > initial_number_vertices or final_number_edges > initial_number_edges or invalidations)
         {
            graph_changes++;
         }
         if(parameters->GetParameter<size_t>("dfm_statistics") > 1)
         {
            VertexIterator v_stat, v_stat_end;
            size_t executed_vertices = 0;
            static size_t previous_executed_vertices = 0;
            for(boost::tie(v_stat, v_stat_end) = boost::vertices(*design_flow_graph); v_stat != v_stat_end; v_stat++)
            {
               const DesignFlowStepInfoRef local_design_flow_step_info = design_flow_graph->GetDesignFlowStepInfo(*v_stat);
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
            if(previous_executed_vertices > executed_vertices)
            {
               INDENT_OUT_MEX(OUTPUT_LEVEL_NONE, output_level, "dfm_statistics - number of invalidated vertices by " + step->GetName() + ": " + STR(previous_executed_vertices - executed_vertices));
            }
            INDENT_OUT_MEX(OUTPUT_LEVEL_NONE, output_level, "dfm_statistics - number of vertices at the end of iteration " + STR(step_counter) + ": " + STR(executed_vertices) + " / " + STR(final_number_vertices));
            INDENT_OUT_MEX(OUTPUT_LEVEL_NONE, output_level, "dfm_statistics - number of edges at the end of iteration " + STR(step_counter) + ": " + STR(final_number_edges));
            previous_executed_vertices = executed_vertices;
         }
      }
   }
#ifndef NDEBUG
   if(debug_level >= DEBUG_LEVEL_VERY_PEDANTIC)
   {
      for(size_t writing_step_counter = 0; writing_step_counter < step_counter; writing_step_counter++)
      {
         if(edge_history.find(writing_step_counter) != edge_history.end())
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Writing Design_Flow_History_" + boost::lexical_cast<std::string>(writing_step_counter));
            feedback_design_flow_graph->WriteDot("Design_Flow_History_" + boost::lexical_cast<std::string>(writing_step_counter), vertex_history, edge_history, step_names, writing_step_counter);
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Written Design_Flow_History_" + boost::lexical_cast<std::string>(writing_step_counter));
         }
      }
   }
#endif
   if(design_flow_graph->CGetDesignFlowStepInfo(design_flow_graph->CGetDesignFlowGraphInfo()->exit)->status != DesignFlowStep_Status::EMPTY)
   {
      feedback_design_flow_graph->WriteDot("Design_Flow_Error");
      THROW_UNREACHABLE("Design flow didn't end");
   }
   if(parameters->IsParameter("dfm_statistics"))
   {
      INDENT_OUT_MEX(OUTPUT_LEVEL_NONE, output_level, "dfm_statistics - number of vertices in final graph: " + STR(boost::num_vertices(*design_flow_graph)));
      INDENT_OUT_MEX(OUTPUT_LEVEL_NONE, output_level, "dfm_statistics - number of edges in final graph: " + STR(boost::num_edges(*design_flow_graph)));
      INDENT_OUT_MEX(OUTPUT_LEVEL_NONE, output_level, "dfm_statistics - number of executed passes: " + STR(executed_passes));
      INDENT_OUT_MEX(OUTPUT_LEVEL_NONE, output_level, "dfm_statistics - number of skipped passes: " + STR(skipped_passes));
      INDENT_OUT_MEX(OUTPUT_LEVEL_NONE, output_level, "dfm_statistics - number of graph changes: " + STR(graph_changes));
      INDENT_OUT_MEX(OUTPUT_LEVEL_NONE, output_level, "dfm_statistics - design flow manager time: " + print_cpu_time(design_flow_manager_time) + " seconds");
   }
#ifndef NDEBUG
   if(parameters->IsParameter("profile_steps"))
   {
      INDENT_OUT_MEX(OUTPUT_LEVEL_NONE, output_level, "-->Steps execution statistics");
      for(const auto step : accumulated_execution_time)
      {
         INDENT_OUT_MEX(OUTPUT_LEVEL_NONE, output_level,
                        "---" + step_names.at(step.first) + ": " + print_cpu_time(step.second) + " seconds - Successes: " + STR(success_executions[step.first]) + " - Unchanged: " + STR(unchanged_executions[step.first]) +
                            " - Skipped: " + STR(skipped_executions[step.first]));
      }
      INDENT_OUT_MEX(OUTPUT_LEVEL_NONE, output_level, "<--");
   }
#endif
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Total number of iterations: " + STR(step_counter));
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Ended execution of design flow");
}

vertex DesignFlowManager::GetDesignFlowStep(const std::string& signature) const
{
   return design_flow_graphs_collection->GetDesignFlowStep(signature);
}

const DesignFlowStepFactoryConstRef DesignFlowManager::CGetDesignFlowStepFactory(const std::string& prefix) const
{
   THROW_ASSERT(design_flow_step_factories.find(prefix) != design_flow_step_factories.end(), "No factory to create steps with prefix " + prefix + " found");
   return design_flow_step_factories.find(prefix)->second;
}

void DesignFlowManager::RegisterFactory(const DesignFlowStepFactoryConstRef factory)
{
   design_flow_step_factories[factory->GetPrefix()] = factory;
}

void DesignFlowManager::DeExecute(const vertex starting_vertex, const bool force_execution)
{
   /// Set not executed on the starting vertex
   const DesignFlowStepInfoRef design_flow_step_info = design_flow_graph->GetDesignFlowStepInfo(starting_vertex);
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---DeExecuting " + design_flow_step_info->design_flow_step->GetName());
   switch(design_flow_step_info->status)
   {
      case DesignFlowStep_Status::SUCCESS:
      {
         design_flow_step_info->status = DesignFlowStep_Status::UNEXECUTED;
         break;
      }
      case DesignFlowStep_Status::ABORTED:
      case DesignFlowStep_Status::EMPTY:
      case DesignFlowStep_Status::UNCHANGED:
      {
         design_flow_step_info->status = DesignFlowStep_Status::UNEXECUTED;
         break;
      }
      case DesignFlowStep_Status::SKIPPED:
      {
         if(force_execution)
            design_flow_step_info->status = DesignFlowStep_Status::UNEXECUTED;
         else
            design_flow_step_info->status = DesignFlowStep_Status::UNNECESSARY;
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
   InEdgeIterator ie, ie_end;
   for(boost::tie(ie, ie_end) = boost::in_edges(starting_vertex, *design_flow_graph); ie != ie_end; ie++)
   {
      vertex pre_dependence_vertex = boost::source(*ie, *design_flow_graph);
      const DesignFlowStepInfoRef pre_info = design_flow_graph->GetDesignFlowStepInfo(pre_dependence_vertex);
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

   /// Propagating to successor
   OutEdgeIterator oe, oe_end;
   for(boost::tie(oe, oe_end) = boost::out_edges(starting_vertex, *design_flow_graph); oe != oe_end; oe++)
   {
      const vertex target = boost::target(*oe, *design_flow_graph);
      const DesignFlowStepInfoRef target_info = design_flow_graph->GetDesignFlowStepInfo(target);
      switch(target_info->status)
      {
         case DesignFlowStep_Status::ABORTED:
         case DesignFlowStep_Status::EMPTY:
         case DesignFlowStep_Status::SUCCESS:
         case DesignFlowStep_Status::UNCHANGED:
         case DesignFlowStep_Status::SKIPPED:
         {
            DeExecute(target, false);
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

DesignFlowStep_Status DesignFlowManager::GetStatus(const std::string& signature) const
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

const DesignFlowStepRef DesignFlowManager::CreateFlowStep(const std::string& signature) const
{
   THROW_ASSERT(signature.find("::") != std::string::npos, signature);
   const auto prefix = signature.substr(0, signature.find("::"));
   return CGetDesignFlowStepFactory(prefix)->CreateFlowStep(signature);
}
