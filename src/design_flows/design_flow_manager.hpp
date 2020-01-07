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
 * @file design_flow_manager.hpp
 * @brief Wrapper of design_flow
 *
 * @author Marco Lattuada <marco.lattuada@polimi.it>
 *
 */

#ifndef DESIGN_FLOW_MANAGER_HPP
#define DESIGN_FLOW_MANAGER_HPP

#include "config_HAVE_STDCXX_11.hpp" // for HAVE_STDCXX_11

#include "custom_map.hpp"
#include "graph.hpp"    // for vertex, Paramete...
#include "refcount.hpp" // for REF_FORWARD_DECL
#include <cstddef>      // for size_t
#include <functional>   // for binary_function
#include <set>          // for set
#include <string>       // for string

class DesignFlowStepSet;
CONSTREF_FORWARD_DECL(DesignFlowGraph);
REF_FORWARD_DECL(DesignFlowGraph);
REF_FORWARD_DECL(DesignFlowGraphsCollection);
REF_FORWARD_DECL(DesignFlowStep);
enum class DesignFlowStep_Status;
CONSTREF_FORWARD_DECL(DesignFlowStepFactory);
REF_FORWARD_DECL(DesignFlowStepInfo);
REF_FORWARD_DECL(Parameter);

/**
 * The key comparison functor for design flow step; it puts necessary steps before unnecessary ones;
 * in this way steps which depend on unnecessary steps are executed later
 */
class DesignFlowStepNecessitySorter : std::binary_function<vertex, vertex, bool>
{
 private:
   /// The design flow graph
   const DesignFlowGraphConstRef design_flow_graph;

 public:
   /**
    * Constructor
    * @param design_flow_graph is the graph to which design flow steps belong
    */
   explicit DesignFlowStepNecessitySorter(const DesignFlowGraphConstRef design_flow_graph_);

   /**
    * Compare position of two vertices
    * @param x is the first vertex
    * @param y is the second vertex
    * @return true if x is necessary and y is unnecessary
    */
   bool operator()(const vertex x, const vertex y) const;
};

class DesignFlowManager
#if HAVE_STDCXX_11
    final
#endif
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

   /// The set of potentially ready steps; when a step is added to set is ready to be executed, but it can become unready because of new added vertices
   std::set<vertex, DesignFlowStepNecessitySorter> possibly_ready;

   /// The registered factories
   CustomUnorderedMap<std::string, DesignFlowStepFactoryConstRef> design_flow_step_factories;

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

   /// The name of each vertex (we have to store since it is possible that it cannot be recomputed at the end - for example because the corresponding task graph has been deallocated)
   CustomMap<vertex, std::string> step_names;

   /// The accumulated times of each step
   CustomMap<vertex, long> accumulated_execution_time;

   /// The number of times each step is executed with success
   CustomMap<vertex, size_t> success_executions;

   /// The number of times each step is executed with unchanged exit
   CustomMap<vertex, size_t> unchanged_executions;

   /// The number of times the execution of a step is skipped
   CustomMap<vertex, size_t> skipped_executions;
#endif

   /// The set of input parameters
   const ParameterConstRef parameters;

   /// The output level
   const int output_level;

   /// The debug level
   int debug_level;

   /**
    * Recursively add steps and corresponding dependencies to the design flow
    * @param steps is the set of steps to be added
    * @param unnecessary specify if the steps have to be added only as a possible precedence of other steps (i.e., they could be not executed if no step depends on them)
    */
   void RecursivelyAddSteps(const DesignFlowStepSet& steps, const bool unnecessary);

   /**
    * Recursively remove executed flag starting from a vertex
    * @param starting_vertex is the starting vertex
    * @param force_execution specifies if a skipped vertex has to be changed into a unexecuted
    */
   void DeExecute(const vertex starting_vertex, bool force_execution);

   /**
    * Connect source and sink vertices to entry and exit
    */
   void Consolidate();

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
   void virtual Exec()
#if HAVE_STDCXX_11
       final
#endif
       ;

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
   const DesignFlowGraphConstRef CGetDesignFlowGraph() const;

   /**
    * Return the vertex associated with a design step if exists, NULL_VERTEX otherwise
    * @param signature is the signature of the design step
    */
   vertex GetDesignFlowStep(const std::string& signature) const;

   /**
    * Return the status of a design step (if it does not exist return NONEXISTENT)
    * @param signature is the signature of the design step
    */
   DesignFlowStep_Status GetStatus(const std::string& signature) const;

   /**
    * Return the factory which can create design flow step with signature beginning with prefix
    * @param prefix is the beginning of the steps that the factory should be created
    * @return the corresponding factory
    */
   const DesignFlowStepFactoryConstRef CGetDesignFlowStepFactory(const std::string& prefix) const;

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
   const DesignFlowStepRef CreateFlowStep(const std::string& signature) const;
};

typedef refcount<DesignFlowManager> DesignFlowManagerRef;
#endif
