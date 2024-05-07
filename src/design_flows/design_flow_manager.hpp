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
#include "design_flow_step.hpp"
#include "graph.hpp"
#include "refcount.hpp"

#include <absl/numeric/int128.h>

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

class DesignFlowStepPrioritySet
{
 public:
   using key_t = absl::uint128;
   using map_t = CustomMap<key_t, vertex>;

 private:
   using reverse_map_t = CustomUnorderedMap<vertex, key_t>;

   map_t _steps_map;

   reverse_map_t _keys_map;

   const DesignFlowGraphRef _dfg;

   /**
    * @brief Compute vertex ordering key
    *
    * Key is computed such that necessary steps before unnecessary ones. Thus, steps which depend on unnecessary steps
    * are executed later.
    *
    * @param v Vertex to compute the key for
    * @return key_t Computed ordering key
    */
   key_t compute_key(const vertex v) const;

 public:
   DesignFlowStepPrioritySet(const DesignFlowGraphRef& dfg);

   /**
    * @brief Insert vertex into the set
    *
    * Insert vertex v into the set. If vertex is already present, key is not updated.
    *
    * @param v Vertex to add
    * @return true If vertex has been inserted
    * @return false If vertex was already present
    */
   bool insert(const vertex v);

   /**
    * @brief Insert vertex into the set
    *
    * Insert vertex v into the set. If vertex is already present, key is updated.
    *
    * @param v Vertex to add
    */
   void insert_or_assign(const vertex v);

   /**
    * @brief Update vertex ordering
    *
    * Update vertex key based on design flow step information. Update is performed only if the vertex is already present
    * in the list and if the key has changed.
    *
    * @param v Vertex to update
    * @return true If vertex was present in the set
    * @return false If vertex was not found
    */
   bool Update(const vertex v);

   /**
    * @brief Extract vertex at position pos from the set
    *
    * @param pos Position of the vertex to extract
    * @return vertex Vertex removed from the set
    */
   vertex Extract(map_t::size_type pos = 0);

   map_t::size_type size() const;

   map_t::const_iterator begin() const;

   map_t::const_iterator end() const;
};

class DesignFlowManager final
{
 private:
   /// NOTE: static should be removed when all the design flow managers will be merged
   /// Counter of current iteration
   static size_t step_counter;

   /// The bulk graph of steps composing the design flow
   const DesignFlowGraphsCollectionRef design_flow_graphs_collection;

   /// The graph of steps composing the design flow
   const DesignFlowGraphRef design_flow_graph;

   /// The design flow graph with feedback edges
   const DesignFlowGraphConstRef feedback_design_flow_graph;

   /// The set of potentially ready steps; when a step is added to set is ready to be executed, but it can become
   /// unready because of new added vertices
   DesignFlowStepPrioritySet possibly_ready;

   /// The registered factories
   CustomUnorderedMap<DesignFlowStep::StepClass, DesignFlowStepFactoryConstRef> design_flow_step_factories;

#ifndef NDEBUG
   /// This structure stores "history of design flow graph manager - vertices"
   /// First key is the iteration
   /// Second key is the vertex
   /// If a vertex is not present in a iteration, it was not yet been created
   /// The first value is the status
   CustomMap<size_t, CustomMap<vertex, DesignFlowStep_Status>> vertex_history;

   /// This structure stores "history of design flow graph manager - edges"
   /// First key is the iteration
   /// Second key is the edge
   /// Value is the selector
   CustomMap<size_t, CustomUnorderedMapStable<EdgeDescriptor, int>> edge_history;
#endif

   /// The name of each vertex (we have to store since it is possible that it cannot be recomputed at the end - for
   /// example because the corresponding task graph has been deallocated)
   CustomMap<vertex, std::string> step_names;

   /// The accumulated times of each step
   CustomMap<vertex, long long> accumulated_execution_time;

   /// The number of times each step is executed with success
   CustomMap<vertex, size_t> success_executions;

   /// The number of times each step is executed with unchanged exit
   CustomMap<vertex, size_t> unchanged_executions;

   /// The number of times the execution of a step is skipped
   CustomMap<vertex, size_t> skipped_executions;

   /// The set of input parameters
   const ParameterConstRef parameters;

   /// The output level
   const int output_level;

   /// The debug level
   int debug_level;

   /**
    * Recursively add steps and corresponding dependencies to the design flow
    * @param steps is the set of steps to be added
    * @param unnecessary specify if the steps have to be added only as a possible precedence of other steps (i.e., they
    * could be not executed if no step depends on them)
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
    */
   void DeExecute(const vertex starting_vertex, bool force_execution, CustomUnorderedSet<vertex>& already_visited);

   inline void DeExecute(const vertex starting_vertex, bool force_execution)
   {
      CustomUnorderedSet<vertex> already_visited;
      DeExecute(starting_vertex, force_execution, already_visited);
   }

   /**
    * Connect source and sink vertices to entry and exit
    */
   void Consolidate();

#ifndef NDEBUG
   void WriteLoopDot() const;
#endif

 public:
   /**
    * Constructor
    */
   explicit DesignFlowManager(const ParameterConstRef parameters);

   /**
    * Destructor
    */
   virtual ~DesignFlowManager();

   /**
    * Execute the design flow
    */
   void virtual Exec() final;

   /**
    * Add step and corresponding dependencies to the design flow
    * @param step is the step to be added
    */
   void AddStep(const DesignFlowStepRef step);

   /**
    * Add steps and corresponding dependencies to the design flow
    * @param steps is the set of steps to be added
    */
   void AddSteps(const DesignFlowStepSet& steps);

   /**
    * Return the design flow graph
    * @return the design flow graph
    */
   DesignFlowGraphConstRef CGetDesignFlowGraph() const;

   /**
    * Return the vertex associated with a design step if exists, NULL_VERTEX otherwise
    * @param signature is the signature of the design step
    */
   vertex GetDesignFlowStep(DesignFlowStep::signature_t signature) const;

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

using DesignFlowManagerRef = refcount<DesignFlowManager>;
#endif
