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
 * @file parametric_list_based.hpp
 * @brief Class definition of the list_based structure.
 *
 * This file defines the class performing the list_based scheduling.
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 * @author Marco Lattuada <lattuada@elet.polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *
 */

#ifndef PARAMETRIC_LIST_BASED_HPP
#define PARAMETRIC_LIST_BASED_HPP

#include <iosfwd>
#include <set>
#include <vector>

#include "Vertex.hpp"
#include "custom_map.hpp"
#include "custom_set.hpp"
#include "hash_helper.hpp"
#include "op_graph.hpp"
#include "refcount.hpp"
#include "rehashed_heap.hpp"
#include "scheduling.hpp"

/**
 * @name forward declarations
 */
//@{
REF_FORWARD_DECL(hls);
CONSTREF_FORWARD_DECL(OpGraph);
CONSTREF_FORWARD_DECL(Schedule);
REF_FORWARD_DECL(Schedule);
class graph;
class resource_ordering_functor;
REF_FORWARD_DECL(fu_binding);
class OpVertexSet;
//@}

#if HAVE_UNORDERED
typedef std::vector<rehashed_heap<int>> PriorityQueues;
#else
/// Sorter for connection
struct PrioritySorter : public std::binary_function<vertex, vertex, bool>
{
   /// The priority
   refcount<priority_data<int>> priority;

   /// The operation graph
   const OpGraphConstRef op_graph;

   /**
    * Constructor
    * @param priority is the priority associated with each vertex
    * @param op_graph is the operation graph
    */
   PrioritySorter(const refcount<priority_data<int>> priority, const OpGraphConstRef op_graph);

   /**
    * Compare position of two vertices
    * @param x is the first vertex
    * @param y is the second vertex
    * @return true if index of x is less than y
    */
   bool operator()(const vertex x, const vertex y) const;
};

typedef std::vector<std::set<vertex, PrioritySorter>> PriorityQueues;
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

   /**
    * Return the string representation of this
    */
   const std::string GetKindText() const override;

   /**
    * Return the contribution to the signature of a step given by the specialization
    */
   const std::string GetSignature() const override;
};

/**
 * Class managing list based scheduling algorithms.
 */
class parametric_list_based : public Scheduling
{
 private:
   static const double EPSILON;

   void compute_exec_stage_time(const unsigned int fu_type, double& stage_period, const ControlStep cs, const OpGraphConstRef op_graph, vertex v, double& op_execution_time, double& phi_extra_time, double current_starting_time, double setup_hold_time);

   /// The used metric
   const ParametricListBased_Metric parametric_list_based_metric;

   /// The dependence graph
   OpGraphConstRef flow_graph;

   /// The dependence graph with feedbacks
   OpGraphConstRef flow_graph_with_feedbacks;

   /// The starting time given the scheduling (used for chaining)
   vertex2float starting_time;

   /// The ending time given the scheduling (used for chaining)
   OpVertexMap<double> ending_time;

   /// The clock cycle
   double clock_cycle;

   /// memoization table used for connection estimation
   CustomUnorderedMapUnstable<std::pair<vertex, unsigned int>, bool> is_complex;

   /// Number of executions
   size_t executions_number;

   /**
    * Given the control step in which an operation is scheduled, compute the exact starting and ending time of an operation
    * @param v is the vertex of the operation
    * @param fu_type is the functional unit type
    * @param cs is the control step
    * @param current_starting_time is where starting_time will be stored
    * @param current_ending_time is where ending_time will be stored
    * @param stage_period is the minimum period of the pipelined unit fu_type
    */
   void compute_starting_ending_time_asap(const vertex v, const unsigned int fu_type, const ControlStep cs, double& current_starting_time, double& current_ending_time, double& stage_period, bool& cannot_be_chained, fu_bindingRef res_binding,
                                          const ScheduleConstRef schedule, double& phi_extra_time, double setup_hold_time, CustomMap<std::pair<unsigned int, unsigned int>, double>& local_fo_correction_map);
   void compute_starting_ending_time_alap(const vertex v, const unsigned int fu_type, const ControlStep cs, double& starting_time, double& ending_time, double& op_execution_time, double& stage_period, unsigned int& n_cycles,
                                          bool& has_a_store_unbounded_as_in, fu_bindingRef res_binding, const ScheduleConstRef schedule, double& phi_extra_time, double setup_hold_time,
                                          CustomMap<std::pair<unsigned int, unsigned int>, double>& local_connection_map);

   /**
    * update starting and ending time by moving candidate_v as late as possible without increasing the whole latency
    */
   void update_starting_ending_time(vertex candidate_v, fu_bindingRef res_binding, OpGraphConstRef op_dfg_graph, const ScheduleConstRef schedule);

   void update_starting_ending_time_asap(vertex candidate_v, fu_bindingRef res_binding, OpGraphConstRef opDFG, const ScheduleConstRef schedule);

   /**
    * @brief update the starting and ending time of the vertices scheduled in the same cs of current_v
    * @param vertex_cstep is the control step of vertex current_v before its rescheduling
    * @param current_v is the current vertex
    * @param schedule is the current scheduling
    * @param vertices_analyzed is the set of vertices updated
    * @param res_binding is the current resource binding
    */
   void update_vertices_timing(const ControlStep vertex_cstep, vertex current_v, const ScheduleConstRef schedule, std::list<vertex>& vertices_analyzed, fu_bindingRef res_binding, const OpGraphConstRef opDFG);

   /**
    * Update the resource map
    * @param used_resources_fu is the resource map.
    * @param fu_type is the functional unit type on which operation is scheduled
    * @return true if the assignment is feasible
    */
   bool BB_update_resources_use(unsigned int& used_resources, const unsigned int fu_type) const;

   /**
    * Adds the vertex v to the priority queues
    * @param priority_trees are the priority_queues
    * @param ready_resources is the set of resources which have at least one ready operation
    * @param v is the vertex.
    */
   void add_to_priority_queues(PriorityQueues& priority_queue, std::set<unsigned int, resource_ordering_functor>& ready_resources, const vertex v) const;

   /**
    * compute the slack of a given operation and in case specify if is pipelined
    */
   double compute_slack(vertex current_op, const ControlStep current_vertex_cstep, double setup_hold_time);

   /**
    * @brief update the slack of vertices of current_v and of all the vertices in chaining with it
    * @param current_v is the considered vertex
    * @param schedule is the scheduling data structure
    * @param vertices_analyzed is the set of vertices having the slack updated
    */
   void update_vertices_slack(vertex current_v, const ScheduleRef schedule, OpVertexSet& vertices_analyzed, double setup_hold_time, const OpGraphConstRef opDFG);

   /**
    * @brief store_in_chaining_with_load checks if a store is chained with a load operation or vice versa
    * @param current_vertex_cstep control step of vertex v
    * @param v vertex considered
    * @return true in case vertex v is a store or a load operation and it is chained with a load or store operation
    */
   bool store_in_chaining_with_load_in(unsigned int current_vertex_cstep, vertex v);
   bool store_in_chaining_with_load_out(unsigned int current_vertex_cstep, vertex v);

   /**
    * perform balanced scheduling on an existing solution
    * @param sub_levels is the list of sorted vertices
    * @param schedule is the current scheduling
    * @param res_binding is the current resource binding
    * @param setup_hold_time is the delay for the setup and hold of registers
    */
   void do_balanced_scheduling(std::deque<vertex>& sub_levels, const ScheduleRef schedule, const fu_bindingRef res_binding, const double setup_hold_time, const double scheduling_mux_margins, const OpGraphConstRef opDFG);

   /**
    * different version of balanced scheduling. It re-schedules the operation trying to improve their slack.
    * @param sub_levels is the list of sorted vertices
    * @param schedule is the current scheduling
    * @param res_binding is the current resource binding
    * @param setup_hold_time is the delay for the setup and hold of registers
    */
   void do_balanced_scheduling1(std::deque<vertex>& sub_levels, const ScheduleRef schedule, const fu_bindingRef res_binding, const double setup_hold_time, const double scheduling_mux_margins, const OpGraphConstRef opDFG, bool seen_cstep_has_RET_conflict);

   bool check_non_direct_operation_chaining(vertex current_v, unsigned int v_fu_type, const ControlStep cs, const ScheduleConstRef schedule, fu_bindingRef res_binding) const;

   bool check_direct_operation_chaining(vertex current_v, const ControlStep cs, const ScheduleConstRef schedule, fu_bindingRef res_binding) const;

   bool check_LOAD_chaining(vertex current_v, const ControlStep cs, const ScheduleConstRef schedule) const;

   void CheckSchedulabilityConditions(const vertex& current_vertex, ControlStep current_cycle, double& current_starting_time, double& current_ending_time, double& current_stage_period,
                                      CustomMap<std::pair<unsigned int, unsigned int>, double>& local_connection_map, double current_cycle_starting_time, double current_cycle_ending_time, double setup_hold_time, double& phi_extra_time,
                                      double scheduling_mux_margins, bool unbounded, bool cstep_has_RET_conflict, unsigned int fu_type, const vertex2obj<ControlStep>& current_ASAP, const fu_bindingRef res_binding, const ScheduleRef schedule,
                                      bool& predecessorsCond, bool& pipeliningCond, bool& cannotBeChained0, bool& chainingRetCond, bool& cannotBeChained1, bool& asyncCond, bool& cannotBeChained2, bool& MultiCond0, bool& MultiCond1, bool& nonDirectMemCond);
   /**
    * Compute the relationship of this step
    * @param relationship_type is the type of relationship to be considered
    * @return the steps in relationship with this
    */
   const CustomUnorderedSet<std::tuple<HLSFlowStep_Type, HLSFlowStepSpecializationConstRef, HLSFlowStep_Relationship>> ComputeHLSRelationships(const DesignFlowStep::RelationshipType relationship_type) const override;

 public:
   /**
    * This is the constructor of the list_based.
    * @param parameters is the set of input parameters
    * @param HLSmgr is the HLS manager
    * @param function_id is the function index of the function
    * @param design_flow_manager is the hls design flow
    * @param hls_flow_step_specialization specifies how specialize this step
    */
   parametric_list_based(const ParameterConstRef parameters, const HLS_managerRef HLSMgr, unsigned int function_id, const DesignFlowManagerConstRef design_flow_manager, const HLSFlowStepSpecializationConstRef hls_flow_step_specialization);

   /**
    * Destructor.
    */
   ~parametric_list_based() override;

   /**
    * Execute the step
    * @return the exit status of this step
    */
   DesignFlowStep_Status InternalExec() override;

   /**
    * Function that computes the List-Based scheduling of the graph.
    */
   void exec(const OpVertexSet& operations, ControlStep current_cycle);

   /**
    * Initialize the step (i.e., like a constructor, but executed just before exec
    */
   void Initialize() override;

   /**
    * Compute the relationships of a step with other steps
    * @param dependencies is where relationships will be stored
    * @param relationship_type is the type of relationship to be computed
    */
   void ComputeRelationships(DesignFlowStepSet& relationship, const DesignFlowStep::RelationshipType relationship_type) override;
};
#endif
