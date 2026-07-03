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
 * @file parametric_list_based.hpp
 * @brief Class definition of the list_based structure.
 *
 * This file defines the class performing the list_based scheduling.
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 * @author Marco Lattuada <lattuada@elet.polimi.it>
 *
 */

#ifndef PARAMETRIC_LIST_BASED_HPP
#define PARAMETRIC_LIST_BASED_HPP

#include <iosfwd>
#include <set>
#include <vector>

#include "SchedulingStep.hpp"
#include "Vertex.hpp"
#include "custom_map.hpp"
#include "custom_set.hpp"
#include "hash_helper.hpp"
#include "op_graph.hpp"
#include "refcount.hpp"
#include "rehashed_heap.hpp"

class resource_ordering_functor;
class Schedule;
REF_FORWARD_DECL(fu_binding);
REF_FORWARD_DECL(hls);

#if HAVE_UNORDERED
using PriorityQueues = std::vector<rehashed_heap<int>>;
#else
/// Sorter for connection
struct PrioritySorter
{
   /// The priority
   refcount<priority_data<int>> priority;

   /// The operation graph
   const OpGraph& op_graph;

   /**
    * Constructor
    * @param priority is the priority associated with each vertex
    * @param op_graph is the operation graph
    */
   PrioritySorter(const refcount<priority_data<int>> priority, const OpGraph& op_graph);

   /**
    * Compare position of two vertices
    * @param x is the first vertex
    * @param y is the second vertex
    * @return true if index of x is less than y
    */
   bool operator()(OpGraph::vertex_descriptor x, OpGraph::vertex_descriptor y) const;
};

using PriorityQueues = std::vector<std::set<OpGraph::vertex_descriptor, PrioritySorter>>;
#endif

enum class ParametricListBased_Metric
{
   DYNAMIC_MOBILITY = 0,
   STATIC_FIXED,
   STATIC_MOBILITY
};

/**
 * Information about specialization of parametric list based step
 */
class ParametricListBasedSpecialization : public HLSFlowStepSpecialization
{
 public:
   /// The metric used in list based
   const ParametricListBased_Metric parametric_list_based_metric;

   /**
    * Constructor
    * @param parametric_list_based_metric is the metric used in list based
    */
   explicit ParametricListBasedSpecialization(const ParametricListBased_Metric parametric_list_based_metric);

   std::string GetName() const override;

   context_t GetSignatureContext() const override;
};

/**
 * Class managing list based scheduling algorithms.
 */
class parametric_list_based : public SchedulingStep
{
 private:
   static const double EPSILON;

   void compute_exec_stage_time(const unsigned int fu_type, double& stage_period, const unsigned int cs,
                                const OpGraph& op_graph, OpGraph::vertex_descriptor v, double& op_execution_time,
                                double& phi_extra_time, double current_starting_time, double setup_hold_time);

   unsigned computeLatestStep(unsigned cs_vertex, const OpGraph& opDFG, OpGraph::vertex_descriptor first_vertex,
                              const OpVertexSet& Operations, Schedule& schedule, unsigned int level,
                              std::list<OpGraph::vertex_descriptor>& phi_list, double connectionOffset,
                              const CustomUnorderedSet<OpGraph::vertex_descriptor>& feedback_last_vertices,
                              bool& moved_feedback_last_vertex);

   /// The used metric
   const ParametricListBased_Metric parametric_list_based_metric;

   /// The dependence graph
   OpGraph flow_graph;

   /// The dependence graph with feedbacks
   OpGraph flow_graph_with_feedbacks;

   /// The starting time given the scheduling (used for chaining)
   vertex2float<> starting_time;

   /// The ending time given the scheduling (used for chaining)
   OpVertexMap<double> ending_time;

   /// The clock cycle
   double clock_cycle;

   /// memoization table used for connection estimation
   CustomUnorderedMapUnstable<std::pair<OpGraph::vertex_descriptor, unsigned int>, bool> is_complex;

   /// reachable proxy from a given function
   std::map<std::string, std::set<std::string>> reachable_proxy_functions;

   /**
    * Given the control step in which an operation is scheduled, compute the exact starting and ending time of an
    * operation
    * @param operations is the set of operations considered for chaining
    * @param v is the vertex of the operation
    * @param fu_type is the functional unit type
    * @param cs is the control step
    * @param current_starting_time is where starting_time will be stored
    * @param current_ending_time is where ending_time will be stored
    * @param stage_period is the minimum period of the pipelined unit fu_type
    * @param cannot_be_chained is set when chaining is forbidden
    * @param res_binding contains the binding information in use
    * @param schedule is the scheduling state used for chaining lookups
    * @param phi_extra_time captures extra latency introduced by phi nodes
    * @param setup_hold_time is the timing budget reserved for setup/hold constraints
    */
   void compute_starting_ending_time_asap(const CustomUnorderedSet<OpGraph::vertex_descriptor>& operations,
                                          OpGraph::vertex_descriptor v, const unsigned int fu_type,
                                          const unsigned int cs, double& current_starting_time,
                                          double& current_ending_time, double& stage_period, bool& cannot_be_chained,
                                          fu_bindingRef res_binding, const Schedule& schedule, double& phi_extra_time,
                                          double setup_hold_time);

   /**
    * Update the resource map
    * @param used_resources records how many units of each type are consumed
    * @param fu_type is the functional unit type on which operation is scheduled
    * @return true if the assignment is feasible
    */
   bool BB_update_resources_use(unsigned int& used_resources, const unsigned int fu_type) const;

   /**
    * Adds the vertex v to the priority queues
    * @param priority_queue stores ready operations ordered by priority
    * @param ready_resources is the set of resources which have at least one ready operation
    * @param v is the vertex.
    */
   void add_to_priority_queues(PriorityQueues& priority_queue,
                               std::set<unsigned int, resource_ordering_functor>& ready_resources,
                               OpGraph::vertex_descriptor v) const;

   /**
    * @brief store_in_chaining_with_load checks if a store is chained with a load operation or vice versa
    * @param operations is the set of operations considered for chaining
    * @param current_vertex_cstep control step of vertex v
    * @param v vertex considered
    * @return true in case vertex v is a store or a load operation and it is chained with a load or store operation
    */
   bool store_in_chaining_with_load_in(const CustomUnorderedSet<OpGraph::vertex_descriptor>& operations,
                                       unsigned int current_vertex_cstep, OpGraph::vertex_descriptor v);
   bool store_in_chaining_with_load_out(const CustomUnorderedSet<OpGraph::vertex_descriptor>& operations,
                                        unsigned int current_vertex_cstep, OpGraph::vertex_descriptor v);

   bool check_non_direct_operation_chaining(const CustomUnorderedSet<OpGraph::vertex_descriptor>& operations,
                                            OpGraph::vertex_descriptor current_v, unsigned int v_fu_type,
                                            const unsigned int cs, const Schedule& schedule,
                                            fu_bindingRef res_binding) const;

   bool check_direct_operation_chaining(const CustomUnorderedSet<OpGraph::vertex_descriptor>& operations,
                                        OpGraph::vertex_descriptor current_v, const unsigned int cs,
                                        const Schedule& schedule, fu_bindingRef res_binding) const;

   bool check_LOAD_chaining(const CustomUnorderedSet<OpGraph::vertex_descriptor>& operations,
                            OpGraph::vertex_descriptor current_v, const unsigned int cs,
                            const Schedule& schedule) const;

   void CheckSchedulabilityConditions(
       const CustomUnorderedSet<OpGraph::vertex_descriptor>& operations, OpGraph::vertex_descriptor& current_vertex,
       unsigned int current_cycle, double& current_starting_time, double& current_ending_time,
       double& current_stage_period, double current_cycle_starting_time, double current_cycle_ending_time,
       double setup_hold_time, double& phi_extra_time, double scheduling_mux_margins, bool unbounded,
       bool unbounded_chaining, bool unbounded_Functions, bool LoadStoreOp,
       const std::set<std::string>& proxy_functions_used, bool cstep_has_RET_conflict, unsigned int fu_type,
       const vertex2obj<unsigned int>& current_ASAP, const fu_bindingRef res_binding, Schedule& schedule,
       bool& predecessorsCond, bool& pipeliningCond, bool& cannotBeChained0, bool& chainingRetCond,
       bool& cannotBeChained1, bool& asyncCond, bool& cannotBeChained2, bool& cannotBeChained3, bool& MultiCond0,
       bool& MultiCond1, bool& LoadStoreFunctionConflict, bool& FunctionLoadStoreConflict, bool& proxyFunCond,
       bool unbounded_RW, bool seeMulticycle, bool is_current_vertex_bounded);

   HLSRelationships ComputeHLSRelationships(const DesignFlowStep::RelationshipType relationship_type) const override;

   /**
    * @brief compute_function_topological_order compute reachable function topological order
    */
   void compute_function_topological_order();

   bool compute_minmaxII(std::list<OpGraph::vertex_descriptor>& bb_operations, const OpVertexSet& Operations,
                         const unsigned int& ctrl_steps, unsigned int bb_index, unsigned& minII, unsigned& maxII,
                         std::vector<std::pair<OpGraph::vertex_descriptor, OpGraph::vertex_descriptor>>& toBeScheduled);

 public:
   parametric_list_based(const ParameterConstRef parameters, const HLS_managerRef HLSMgr, unsigned int _funId,
                         const DesignFlowManager& design_flow_manager,
                         const HLSFlowStepSpecializationConstRef hls_flow_step_specialization);

   DesignFlowStep_Status InternalExec() override;

   /**
    * Function that computes the List-Based scheduling of the graph.
    * @return true in case a solution exists.
    */
   template <bool LPBB_predicate>
   bool exec(const OpVertexSet& operations, unsigned int current_cycle, unsigned II,
             const std::vector<std::pair<OpGraph::vertex_descriptor, OpGraph::vertex_descriptor>>& toBeScheduled,
             unsigned max_iteration_latency, CustomUnorderedMap<OpGraph::vertex_descriptor, unsigned>& tabu_table);

   void Initialize() override;
};
#endif
