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
 * @file design_flow_manager.hpp
 * @brief Wrapper of design_flow
 *
 * @author Marco Lattuada <marco.lattuada@polimi.it>
 * @author Michele Fiorito <michele.fiorito@polimi.it>
 *
 */
#ifndef DESIGN_FLOW_MANAGER_HPP
#define DESIGN_FLOW_MANAGER_HPP
#include "custom_map.hpp"
#include "custom_set.hpp"
#include "design_flow_graph.hpp"
#include "design_flow_step.hpp"
#include "graph.hpp"
#include "refcount.hpp"

#include <cstddef>
#include <set>
#include <string>

CONSTREF_FORWARD_DECL(DesignFlowGraph);
REF_FORWARD_DECL(DesignFlowGraph);
REF_FORWARD_DECL(DesignFlowGraphsCollection);
enum class DesignFlowStep_Status;
CONSTREF_FORWARD_DECL(DesignFlowStepFactory);
REF_FORWARD_DECL(DesignFlowStepInfo);
REF_FORWARD_DECL(Parameter);

class DesignFlowStepPrioritySet;

class DesignFlowManager final
{
 public:
   using vertex_descriptor = DesignFlowGraph::vertex_descriptor;
   using edge_descriptor = DesignFlowGraph::edge_descriptor;

 private:
   /// The graph of steps composing the design flow
   const DesignFlowGraphRef design_flow_graph;

#ifndef NDEBUG
   /// The design flow graph with feedback edges
   const DesignFlowGraphRef feedback_design_flow_graph;
   CustomUnorderedMap<vertex_descriptor, vertex_descriptor> dfg_to_feedback;
#endif

   /// The set of potentially ready steps; when a step is added to set is ready to be executed, but it can become
   /// unready because of new added vertices
   DesignFlowStepPrioritySet* possibly_ready;

   /// The registered factories
   CustomUnorderedMap<DesignFlowStep::StepClass, DesignFlowStepFactoryConstRef> design_flow_step_factories;

   /// Counter of current iteration
   size_t step_counter;

   struct StepProfilingInfo
   {
      const std::string name;
      long long accumulated_execution_time{0};
      size_t success{0};
      size_t unchanged{0};
      size_t skipped{0};
      size_t direct_invalidations{0};
      size_t total_invalidations{0};

      StepProfilingInfo(const std::string& _name) : name(_name)
      {
      }
   };

   /// The name of each vertex (we have to store since it is possible that it cannot be recomputed at the end - for
   /// example because the corresponding task graph has been deallocated)
   CustomMap<vertex_descriptor, StepProfilingInfo> step_prof_info;

   /// The set of input parameters
   const ParameterConstRef parameters;

   /// The output level
   const int output_level;

   /// The debug level
   const int debug_level;

   vertex_descriptor AddDesignFlowStep(const DesignFlowStepRef& design_flow_step, bool unnecessary);

   void AddDesignFlowDependence(vertex_descriptor src, vertex_descriptor tgt, DesignFlowEdge type);

   void RemoveDesignFlowDependence(DesignFlowGraph::edge_descriptor e);

   DesignFlowEdge AddType(edge_descriptor e, DesignFlowEdge type);

   DesignFlowEdge RemoveType(edge_descriptor e, DesignFlowEdge type);

   /**
    * Recursively add steps and corresponding dependencies to the design flow
    * @param steps is the set of steps to be added
    * @param unnecessary specify if the steps have to be added only as a possible precedence of other steps (i.e., they
    * could be not executed if no step depends on them)
    * @param already_visited stores the signatures already traversed during the recursive walk
    */
   void RecursivelyAddSteps(const DesignFlowStepSet& steps, const bool unnecessary,
                            CustomUnorderedSet<std::pair<DesignFlowStep::signature_t, bool>>& already_visited);

   inline void RecursivelyAddSteps(const DesignFlowStepSet& steps, const bool unnecessary)
   {
      CustomUnorderedSet<std::pair<DesignFlowStep::signature_t, bool>> already_visited;
      RecursivelyAddSteps(steps, unnecessary, already_visited);
   }

   /**
    * Recursively remove executed flag starting from a vertex
    * @param starting_vertex is the starting vertex
    * @param force_execution specifies if a skipped vertex has to be changed into a unexecuted
    * @param already_visited stores the vertices already traversed during the recursive walk
    * @return size_t de-executed steps
    */
   size_t DeExecute(const vertex_descriptor starting_vertex, bool force_execution,
                    CustomUnorderedSet<vertex_descriptor>& already_visited);

   inline size_t DeExecute(const vertex_descriptor starting_vertex, bool force_execution)
   {
      CustomUnorderedSet<vertex_descriptor> already_visited;
      return DeExecute(starting_vertex, force_execution, already_visited);
   }

#ifndef NDEBUG
   void WriteLoopDot() const;
#endif

 public:
   explicit DesignFlowManager(const ParameterConstRef parameters);

   ~DesignFlowManager();

   /**
    * Execute the design flow
    */
   void Exec();

   /**
    * Add step and corresponding dependencies to the design flow
    * @param step is the step to be added
    */
   inline void AddStep(const DesignFlowStepRef step)
   {
      DesignFlowStepSet steps;
      steps.insert(step);
      RecursivelyAddSteps(steps, false);
   }

   /**
    * Add steps and corresponding dependencies to the design flow
    * @param steps is the set of steps to be added
    */
   inline void AddSteps(const DesignFlowStepSet& steps)
   {
      RecursivelyAddSteps(steps, false);
   }

   /**
    * Return the design flow graph
    * @return the design flow graph
    */
   DesignFlowGraphConstRef CGetDesignFlowGraph() const;

   /**
    * Return the vertex associated with a design step if exists, NULL_VERTEX otherwise
    * @param signature is the signature of the design step
    */
   inline vertex_descriptor GetDesignFlowStep(DesignFlowStep::signature_t signature) const
   {
      return design_flow_graph->GetDesignFlowStep(signature);
   }

   /**
    * Return the status of a design step (if it does not exist return NONEXISTENT)
    * @param signature is the signature of the design step
    */
   DesignFlowStep_Status GetStatus(DesignFlowStep::signature_t signature) const;

   /**
    * Return the factory which can create design flow step with given step class
    * @param step_class is step class of the factory
    * @return the corresponding factory
    */
   DesignFlowStepFactoryConstRef CGetDesignFlowStepFactory(DesignFlowStep::StepClass step_class) const;

   /**
    * Register a design flow step factory
    * @param factory is the factory to be registered
    */
   void RegisterFactory(const DesignFlowStepFactoryConstRef factory);

   /**
    * Create a design flow step
    * @param signature is the signature of the step to be created
    * @return the created design flow step
    */
   DesignFlowStepRef CreateFlowStep(DesignFlowStep::signature_t signature) const;
};
#endif
