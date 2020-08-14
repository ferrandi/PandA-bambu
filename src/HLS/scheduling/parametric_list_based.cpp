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
 * @file parametric_list_based.cpp
 * @brief Class implementation of the parametric_list_based structure.
 *
 * This file implements some of the parametric_list_based member functions.
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 * @author Marco Lattuada <lattuada@elet.polimi.it>
 *
 */

#include "parametric_list_based.hpp"

#include "config_HAVE_ASSERTS.hpp"

#include <utility>
// #include "call_graph.hpp"
#include "exceptions.hpp"
#include "function_behavior.hpp"
#include "utility.hpp"

#include "ASLAP.hpp"
#include "allocation.hpp"
#include "fu_binding.hpp"
#include "hls.hpp"
#include "hls_constraints.hpp"
#include "schedule.hpp"
#include "technology_node.hpp"

#include "basic_block.hpp"
#include "tree_basic_block.hpp"
#include "tree_helper.hpp"
#include "tree_manager.hpp"
#include "tree_node.hpp"

#include "BambuParameter.hpp"
#include "cpu_time.hpp"
#include "memory.hpp"

/// circuit include
#include "structural_objects.hpp"

/// design_flows includes
#include "design_flow_graph.hpp"
#include "design_flow_manager.hpp"

/// frontend_flow include
#include "frontend_flow_step_factory.hpp"
#include "function_frontend_flow_step.hpp"

/// HLS/module_allocation include
#include "allocation_information.hpp"

/// polixml include
#include "xml_document.hpp"

#include "behavioral_helper.hpp"
#include "call_graph_manager.hpp"  // for CallGraphManager, CallGrap...
#include "string_manipulation.hpp" // for GET_CLASS

#if !HAVE_UNORDERED
PrioritySorter::PrioritySorter(refcount<priority_data<int>> _priority, const OpGraphConstRef _op_graph) : priority(std::move(_priority)), op_graph(_op_graph)
{
}

bool PrioritySorter::operator()(const vertex x, const vertex y) const
{
   const auto x_priority = (*priority)(x);
   const auto y_priority = (*priority)(y);
   if(x_priority != y_priority)
      return x_priority > y_priority;
   return GET_NAME(op_graph, x) < GET_NAME(op_graph, y);
}
#endif

/**
 * Functor used to compare two vertices with respect to an order based on the control steps associated with the vertices.
 */
struct cs_ordering_functor
{
 private:
   /// copy of the order: control step associated with the vertices.
   const OpVertexMap<double>& order;
   const OpGraphConstRef& op_graph;

 public:
   /**
    * functor function used to compare two vertices with respect to the control step associated with the vertices.
    * @param a is the first vertex
    * @param bis the second vertex
    * @return true when order(a) < order(b)
    */
   bool operator()(const vertex& a, const vertex& b) const
   {
      return order.find(a)->second < order.find(b)->second || (order.find(a)->second == order.find(b)->second && GET_NAME(op_graph, a) < GET_NAME(op_graph, b));
   }

   /**
    * Constructor
    * @param o is the order.
    * @param opGraph is the operation graph.
    */
   cs_ordering_functor(const OpVertexMap<double>& o, const OpGraphConstRef& opGraph) : order(o), op_graph(opGraph)
   {
   }

   /**
    * Destructor
    */
   ~cs_ordering_functor() = default;
};

/**
 * Functor used to compare which of two resources has to be considered first in the scheduling
 */
struct resource_ordering_functor
{
 private:
   /// copy of the order: control step associated with the vertices.
   const AllocationInformationConstRef all;

 public:
   /**
    * functor function used to compare two resources with respect to their performances
    * @param a is the first vertex
    * @param bis the second vertex
    * @return true when a is faster than b
    */
   bool operator()(const unsigned int& a, const unsigned int& b) const
   {
      unsigned char wm_a = all->is_indirect_access_memory_unit(a) ? 1 : 0;
      unsigned char wm_b = all->is_indirect_access_memory_unit(b) ? 1 : 0;
      double we_a = all->get_worst_execution_time(a);
      double we_b = all->get_worst_execution_time(b);
      double wa_a = all->get_area(a) + all->get_DSPs(a);
      double wa_b = all->get_area(b) + all->get_DSPs(b);

      std::string prefix(PROXY_PREFIX);
      std::string aFunName = all->get_fu_name(a).first + STR(a);
      std::string bFunName = all->get_fu_name(b).first + STR(b);
      if(aFunName.find(prefix) != std::string::npos)
         aFunName = aFunName.substr(prefix.length());
      if(bFunName.find(prefix) != std::string::npos)
         bFunName = bFunName.substr(prefix.length());

      return ((wm_a > wm_b) || (wm_a == wm_b && we_a < we_b) || (wm_a == wm_b && we_a == we_b && wa_a < wa_b) || (wm_a == wm_b && we_a == we_b && wa_a == wa_b && aFunName < bFunName));
   }

   /**
    * Constructor
    * @param o is the order.
    */
   explicit resource_ordering_functor(const AllocationInformationConstRef _all) : all(_all)
   {
   }

   /**
    * Destructor
    */
   ~resource_ordering_functor() = default;
};

/**
 * Functor to sort vertex by node name
 */
class compare_vertex_by_name : std::binary_function<vertex, vertex, bool>
{
   const OpGraphConstRef& op_graph;

 public:
   /**
    * Constructor.
    *
    * @param G is the operation graph.
    */
   explicit compare_vertex_by_name(const OpGraphConstRef& G) : op_graph(G)
   {
   }

   bool operator()(const vertex& a, const vertex& b) const
   {
      return GET_NAME(op_graph, a) < GET_NAME(op_graph, b);
   }
};

/**
 * The key comparison function for edge-integer set based on levels
 */
class edge_integer_order_by_map : std::binary_function<vertex, vertex, bool>
{
 private:
   /// Topological sorted vertices
   const std::map<vertex, unsigned int>& ref;

/// Graph
#if HAVE_ASSERTS
   const graph* g;
#endif

 public:
   /**
    * Constructor
    * @param ref is the map with the topological sort of vertices
    * @param g is a graph used only for debugging purpose to print name of vertex
    */
   edge_integer_order_by_map(const std::map<vertex, unsigned int>& _ref, const graph*
#if HAVE_ASSERTS
                                                                             _g)
       : ref(_ref), g(_g)
#else
                             )
       : ref(_ref)
#endif
   {
   }

   /**
    * Compare position of two vertices in topological sorted
    * @param x is the first pair of vertex
    * @param y is the second pair of vertex
    * @return true if x precedes y in topological sort, false otherwise
    */
   bool operator()(const std::pair<std::pair<vertex, vertex>, unsigned int>& x, const std::pair<std::pair<vertex, vertex>, unsigned int>& y) const
   {
      THROW_ASSERT(ref.find(x.first.first) != ref.end(), "Vertex " + GET_NAME(g, x.first.first) + " is not in topological_sort");
      THROW_ASSERT(ref.find(y.first.first) != ref.end(), "Vertex " + GET_NAME(g, y.first.first) + " is not in topological_sort");
      THROW_ASSERT(ref.find(x.first.second) != ref.end(), "Vertex " + GET_NAME(g, x.first.second) + " is not in topological_sort");
      THROW_ASSERT(ref.find(y.first.second) != ref.end(), "Vertex " + GET_NAME(g, y.first.second) + " is not in topological_sort");
      if(ref.find(x.first.first)->second != ref.find(y.first.first)->second)
         return ref.find(x.first.first)->second < ref.find(y.first.first)->second;
      else if(ref.find(x.first.second)->second != ref.find(y.first.second)->second)
         return ref.find(x.first.second)->second < ref.find(y.first.second)->second;
      else
         return x.second < y.second;
   }
};

static bool check_if_is_live_in_next_cycle(const std::set<vertex, cs_ordering_functor>& live_vertices, const ControlStep current_cycle, const OpVertexMap<double>& ending_time, double clock_cycle)
{
   auto live_vertex_it = live_vertices.begin();
   while(live_vertex_it != live_vertices.end())
   {
      if(ending_time.find(*live_vertex_it)->second > (from_strongtype_cast<double>(current_cycle) + 1) * clock_cycle)
      {
         return true;
      }
      ++live_vertex_it;
   }
   return false;
}

ParametricListBasedSpecialization::ParametricListBasedSpecialization(const ParametricListBased_Metric _parametric_list_based_metric) : parametric_list_based_metric(_parametric_list_based_metric)
{
}

const std::string ParametricListBasedSpecialization::GetKindText() const
{
   switch(parametric_list_based_metric)
   {
      case ParametricListBased_Metric::DYNAMIC_MOBILITY:
         return "DynamicMobility";
      case ParametricListBased_Metric::STATIC_FIXED:
         return "StaticFixed";
      case ParametricListBased_Metric::STATIC_MOBILITY:
         return "StaticMobility";
      default:
         THROW_UNREACHABLE("");
   }
   return "";
}

const std::string ParametricListBasedSpecialization::GetSignature() const
{
   return GetKindText();
}

parametric_list_based::parametric_list_based(const ParameterConstRef _parameters, const HLS_managerRef _HLSMgr, unsigned int _funId, const DesignFlowManagerConstRef _design_flow_manager,
                                             const HLSFlowStepSpecializationConstRef _hls_flow_step_specialization)
    : Scheduling(_parameters, _HLSMgr, _funId, _design_flow_manager, HLSFlowStep_Type::LIST_BASED_SCHEDULING,
                 _hls_flow_step_specialization ? _hls_flow_step_specialization :
                                                 HLSFlowStepSpecializationConstRef(new ParametricListBasedSpecialization(static_cast<ParametricListBased_Metric>(_parameters->getOption<unsigned int>(OPT_scheduling_priority))))),
      parametric_list_based_metric(GetPointer<const ParametricListBasedSpecialization>(hls_flow_step_specialization)->parametric_list_based_metric),
      ending_time(OpGraphConstRef()),
      clock_cycle(0.0),
      executions_number(0)
{
   debug_level = parameters->get_class_debug_level(GET_CLASS(*this));
}

parametric_list_based::~parametric_list_based() = default;

void parametric_list_based::ComputeRelationships(DesignFlowStepSet& relationship, const DesignFlowStep::RelationshipType relationship_type)
{
#if 0
   if(relationship_type == INVALIDATION_RELATIONSHIP and (static_cast<HLSFlowStep_Type>(parameters->getOption<unsigned int>(OPT_scheduling_algorithm))) == HLSFlowStep_Type::SDC_SCHEDULING and executions_number == 1)
   {
      vertex frontend_step = design_flow_manager.lock()->GetDesignFlowStep(FunctionFrontendFlowStep::ComputeSignature(FrontendFlowStepType::MULTI_WAY_IF, funId));
      const auto design_flow_graph = design_flow_manager.lock()->CGetDesignFlowGraph();
      const auto design_flow_step = frontend_step != NULL_VERTEX ? design_flow_graph->CGetDesignFlowStepInfo(frontend_step)->design_flow_step : GetPointer<const FrontendFlowStepFactory>(design_flow_manager.lock()->CGetDesignFlowStepFactory("Frontend"))->CreateFunctionFrontendFlowStep(FrontendFlowStepType::MULTI_WAY_IF, funId);
      relationship.insert(design_flow_step);
   }
#endif
   Scheduling::ComputeRelationships(relationship, relationship_type);
}

void parametric_list_based::Initialize()
{
   Scheduling::Initialize();
   const FunctionBehaviorConstRef FB = HLSMgr->CGetFunctionBehavior(funId);
   ending_time = OpVertexMap<double>(FB->CGetOpGraph(FunctionBehavior::CFG));
}

void parametric_list_based::CheckSchedulabilityConditions(const vertex& current_vertex, ControlStep current_cycle, double& current_starting_time, double& current_ending_time, double& current_stage_period,
                                                          CustomMap<std::pair<unsigned int, unsigned int>, double>& local_connection_map, double current_cycle_starting_time, double current_cycle_ending_time, double setup_hold_time, double& phi_extra_time,
                                                          double scheduling_mux_margins, bool unbounded, bool cstep_has_RET_conflict, unsigned int fu_type, const vertex2obj<ControlStep>& current_ASAP, const fu_bindingRef res_binding,
                                                          const ScheduleRef schedule, bool& predecessorsCond, bool& pipeliningCond, bool& cannotBeChained0, bool& chainingRetCond, bool& cannotBeChained1, bool& asyncCond, bool& cannotBeChained2,
                                                          bool& MultiCond0, bool& MultiCond1, bool& nonDirectMemCond)
{
   predecessorsCond = current_ASAP.find(current_vertex) != current_ASAP.end() and current_ASAP.find(current_vertex)->second > current_cycle;
   if(predecessorsCond)
      return;
   compute_starting_ending_time_asap(current_vertex, fu_type, current_cycle, current_starting_time, current_ending_time, current_stage_period, cannotBeChained1, res_binding, schedule, phi_extra_time, setup_hold_time, local_connection_map);
   bool is_pipelined = HLS->allocation_information->get_initiation_time(fu_type, current_vertex) != 0;
   auto n_cycles = HLS->allocation_information->get_cycles(fu_type, current_vertex, flow_graph);
   pipeliningCond = is_pipelined and (current_starting_time > current_cycle_starting_time) and ((current_stage_period + current_starting_time + setup_hold_time + phi_extra_time + scheduling_mux_margins > (current_cycle_ending_time) || unbounded));
   if(pipeliningCond)
      return;
   cannotBeChained0 = (current_starting_time >= current_cycle_ending_time) || ((!is_pipelined && !(GET_TYPE(flow_graph, current_vertex) & TYPE_RET) && n_cycles == 0 && current_starting_time > (current_cycle_starting_time)) &&
                                                                               current_ending_time + setup_hold_time + phi_extra_time + scheduling_mux_margins > current_cycle_ending_time);
   if(cannotBeChained0)
      return;
   chainingRetCond = (unbounded || (cstep_has_RET_conflict && current_starting_time > (current_cycle_starting_time))) && (GET_TYPE(flow_graph, current_vertex) & TYPE_RET);
   if(chainingRetCond)
      return;
   if(cannotBeChained1)
      return;
   asyncCond = (current_starting_time > (EPSILON + current_cycle_starting_time)) and (GET_TYPE(flow_graph, current_vertex) & TYPE_LOAD) and HLS->allocation_information->is_one_cycle_direct_access_memory_unit(fu_type) and
               (!HLS->allocation_information->is_readonly_memory_unit(fu_type) || (!HLS->Param->isOption(OPT_rom_duplication) || !parameters->getOption<bool>(OPT_rom_duplication))) and
               ((HLSMgr->Rmem->get_maximum_references(HLS->allocation_information->is_memory_unit(fu_type) ? HLS->allocation_information->get_memory_var(fu_type) : HLS->allocation_information->get_proxy_memory_var(fu_type))) >
                HLS->allocation_information->get_number_channels(fu_type));
   if(asyncCond)
      return;
   cannotBeChained2 = (current_starting_time > (current_cycle_starting_time)) && !parameters->getOption<bool>(OPT_chaining);
   if(cannotBeChained2)
      return;
   MultiCond0 = (!is_pipelined && n_cycles > 0 && current_starting_time > (current_cycle_starting_time)) && current_ending_time - (n_cycles - 1) * clock_cycle + setup_hold_time + phi_extra_time + scheduling_mux_margins > current_cycle_ending_time;
   if(MultiCond0)
      return;
   MultiCond1 = current_ending_time + setup_hold_time + phi_extra_time + scheduling_mux_margins > current_cycle_ending_time && unbounded;
   if(MultiCond1)
      return;
   nonDirectMemCond = (GET_TYPE(flow_graph, current_vertex) & (TYPE_LOAD | TYPE_STORE)) && !HLS->allocation_information->is_direct_access_memory_unit(fu_type) && unbounded;
   if(nonDirectMemCond)
      return;
}
const double parametric_list_based::EPSILON = 0.000000001;

#define CTRL_STEP_MULTIPLIER 1000

void parametric_list_based::exec(const OpVertexSet& operations, ControlStep current_cycle)
{
   PRINT_DBG_MEX(DEBUG_LEVEL_VERBOSE, debug_level, "Executing parametric_list_based::exec...");
   THROW_ASSERT(operations.size(), "At least one vertex is expected");
   const FunctionBehaviorConstRef FB = HLSMgr->CGetFunctionBehavior(funId);
   const OpGraphConstRef op_graph = FB->CGetOpGraph(FunctionBehavior::CFG);
   const auto top_function_ids = HLSMgr->CGetCallGraphManager()->GetRootFunctions();
   const unsigned int return_type_index = FB->CGetBehavioralHelper()->GetFunctionReturnType(funId);
   auto registering_output_p = top_function_ids.find(funId) != top_function_ids.end() && return_type_index && parameters->getOption<std::string>(OPT_registered_inputs) == "top";

   /// The scheduling
   const ScheduleRef schedule = HLS->Rsch;

   /// Current ASAP
   vertex2obj<ControlStep> current_ASAP;

   /// The binding
   const fu_bindingRef res_binding = HLS->Rfu;

   /// Number of predecessors of a vertex already scheduled
   vertex2obj<size_t> scheduled_predecessors;

   double clock_period_resource_fraction = HLS->HLS_C->get_clock_period_resource_fraction();

   double scheduling_mux_margins = parameters->getOption<double>(OPT_scheduling_mux_margins) * HLS->allocation_information->mux_time_unit(32);

   /// The clock cycle
   clock_cycle = clock_period_resource_fraction * HLS->HLS_C->get_clock_period();
   if(clock_cycle < scheduling_mux_margins)
      THROW_ERROR("Mux margins too constrained");

   /// The setup+hold time delay
   double setup_hold_time = HLS->allocation_information->get_setup_hold_time();

   resource_ordering_functor r_functor(HLS->allocation_information);
   /// The set of resources for which the list of ready operations is not empty
   std::set<unsigned int, resource_ordering_functor> ready_resources(r_functor);

   /// Set of ready operations (needed for update priority)
   OpVertexSet ready_vertices(op_graph);

   /// vertices still live at the beginning of the current control step
   cs_ordering_functor et_order(ending_time, op_graph);
   std::set<vertex, cs_ordering_functor> live_vertices(et_order);

   /// select the type of graph
   PRINT_DBG_MEX(DEBUG_LEVEL_VERBOSE, debug_level, "   Selecting the type of the graph...");

   if(speculation)
   {
      flow_graph = FB->CGetOpGraph(FunctionBehavior::SG, operations);
      flow_graph_with_feedbacks = FB->CGetOpGraph(FunctionBehavior::SG);
   }
   else
   {
      flow_graph = FB->CGetOpGraph(FunctionBehavior::FLSAODG, operations);
      flow_graph_with_feedbacks = FB->CGetOpGraph(FunctionBehavior::FFLSAODG);
   }

   /// Number of operation to be scheduled
   size_t operations_number = operations.size();

   //   long int cpu_time;
   /// compute asap and alap
   PRINT_DBG_MEX(DEBUG_LEVEL_VERBOSE, debug_level, "   Computing asap and alap...");
   //   START_TIME(cpu_time);
   ASLAPRef aslap;
   if(parametric_list_based_metric == ParametricListBased_Metric::STATIC_MOBILITY or parametric_list_based_metric == ParametricListBased_Metric::DYNAMIC_MOBILITY)
   {
      aslap = ASLAPRef(new ASLAP(HLSMgr, HLS, speculation, operations, parameters, CTRL_STEP_MULTIPLIER));
      if(HLS->Rsch->num_scheduled() == 0)
      {
         aslap->compute_ASAP();
         aslap->compute_ALAP(ASLAP::ALAP_fast);
      }
      else
      {
         aslap->compute_ASAP(HLS->Rsch);
         ControlStep est_upper_bound = ControlStep(static_cast<unsigned int>(operations_number));
         aslap->compute_ALAP(ASLAP::ALAP_with_partial_scheduling, HLS->Rsch, nullptr, est_upper_bound);
      }
   }
   //   STOP_TIME(cpu_time);
   //   if(output_level >= OUTPUT_LEVEL_VERY_PEDANTIC)
   //      INDENT_OUT_MEX(OUTPUT_LEVEL_VERY_PEDANTIC, output_level, "---Time to perform ASAP+ALAP scheduling: " + print_cpu_time(cpu_time) + " seconds");

   PRINT_DBG_MEX(DEBUG_LEVEL_VERBOSE, debug_level, "   Computing free input vertices...");
   /// compute the set of vertices without input edges.
   /// At least one vertex is expected
   for(auto operation : operations)
   {
      /// Updating structure for already scheduled operations
      if(schedule->is_scheduled(operation))
      {
         live_vertices.insert(operation);
      }
      else
      {
         /// Check if all its predecessors have been scheduled. In this case the vertex is ready
         InEdgeIterator ei, ei_end;
         for(boost::tie(ei, ei_end) = boost::in_edges(operation, *flow_graph); ei != ei_end; ei++)
         {
            vertex source = boost::source(*ei, *flow_graph);
            if(!schedule->is_scheduled(source))
               break;
         }
         if(ei == ei_end)
         {
            ready_vertices.insert(operation);
         }
      }
   }

   THROW_ASSERT(ready_vertices.size(), "At least one vertex is expected");
   PRINT_DBG_MEX(DEBUG_LEVEL_VERBOSE, debug_level, "   Number of vertices: " + STR(ready_vertices.size()));

   /// Setting priority
   PRINT_DBG_MEX(DEBUG_LEVEL_VERBOSE, debug_level, "   Setting priority...");
   refcount<priority_data<int>> Priority;
   switch(parametric_list_based_metric)
   {
      case ParametricListBased_Metric::STATIC_MOBILITY:
         Priority = refcount<priority_data<int>>(new priority_static_mobility(aslap));
         break;
      case ParametricListBased_Metric::DYNAMIC_MOBILITY:
      {
         Priority = refcount<priority_data<int>>(new priority_dynamic_mobility(aslap, ready_vertices, CTRL_STEP_MULTIPLIER));
         break;
      }
      case ParametricListBased_Metric::STATIC_FIXED:
      {
         CustomUnorderedMapUnstable<vertex, int> priority_value;
         CustomMap<std::string, int> string_priority_value = HLS->HLS_C->get_scheduling_priority();
         VertexIterator ki, ki_end;
         for(boost::tie(ki, ki_end) = boost::vertices(*flow_graph); ki != ki_end; ki++)
         {
            priority_value[*ki] = string_priority_value[GET_NAME(flow_graph, *ki)];
         }
         Priority = refcount<priority_data<int>>(new priority_fixed(priority_value));
         break;
      }
      default:
         THROW_UNREACHABLE("");
   }

   /// priory queues set up
   unsigned int n_resources = HLS->allocation_information->get_number_fu_types();
#if HAVE_UNORDERED
   priority_compare_functor<int> priority_functors(Priority);
   PriorityQueues priority_queues(n_resources, priority_functors);
#else
   PriorityQueues priority_queues(n_resources, std::set<vertex, PrioritySorter>(PrioritySorter(Priority, flow_graph)));
#endif
   unsigned int cstep_vuses_ARRAYs = 0;
   unsigned int cstep_vuses_others = 0;
   bool cstep_has_RET_conflict = registering_output_p;
   bool seen_cstep_has_RET_conflict = registering_output_p;

   OpVertexSet::const_iterator rv, rv_end = ready_vertices.end();

   for(rv = ready_vertices.begin(); rv != rv_end; ++rv)
      add_to_priority_queues(priority_queues, ready_resources, *rv);

   const auto TM = HLSMgr->get_tree_manager();
   auto fnode = TM->get_tree_node_const(funId);
   auto fd = GetPointer<function_decl>(fnode);
   std::string fname;
   tree_helper::get_mangled_fname(fd, fname);
   CustomUnorderedSet<vertex> RW_stmts;
   if(HLSMgr->design_interface_loads.find(fname) != HLSMgr->design_interface_loads.end())
   {
      for(auto bb2arg2stmtsR : HLSMgr->design_interface_loads.find(fname)->second)
      {
         for(auto arg2stms : bb2arg2stmtsR.second)
         {
            if(arg2stms.second.size() > 0)
            {
               for(auto stmt : arg2stms.second)
               {
                  THROW_ASSERT(flow_graph->CGetOpGraphInfo()->tree_node_to_operation.find(stmt) != flow_graph->CGetOpGraphInfo()->tree_node_to_operation.end(), "unexpected condition: STMT=" + STR(stmt));
                  RW_stmts.insert(flow_graph->CGetOpGraphInfo()->tree_node_to_operation.find(stmt)->second);
               }
            }
         }
      }
   }
   if(HLSMgr->design_interface_stores.find(fname) != HLSMgr->design_interface_stores.end())
   {
      for(auto bb2arg2stmtsW : HLSMgr->design_interface_stores.find(fname)->second)
      {
         for(auto arg2stms : bb2arg2stmtsW.second)
         {
            if(arg2stms.second.size() > 0)
            {
               for(auto stmt : arg2stms.second)
               {
                  THROW_ASSERT(flow_graph->CGetOpGraphInfo()->tree_node_to_operation.find(stmt) != flow_graph->CGetOpGraphInfo()->tree_node_to_operation.end(), "unexpected condition");
                  RW_stmts.insert(flow_graph->CGetOpGraphInfo()->tree_node_to_operation.find(stmt)->second);
               }
            }
         }
      }
   }

   PRINT_DBG_MEX(DEBUG_LEVEL_VERBOSE, debug_level, "   Starting scheduling...");
   unsigned int already_sch = schedule->num_scheduled();
   while((schedule->num_scheduled() - already_sch) != operations_number)
   {
      bool unbounded = false;
      bool unbounded_RW = false;
      bool store_unbounded_check = false;
      unsigned int n_scheduled_ops = 0;
      PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "      schedule->num_scheduled() " + std::to_string(schedule->num_scheduled()));
      PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "      already_sch " + std::to_string(already_sch));
      PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "      operations_number " + std::to_string(operations_number));
      PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "      Scheduling in control step " + STR(current_cycle) + " (Time: " + STR(from_strongtype_cast<double>(current_cycle) * clock_cycle) + ")");
      /// definition of the data structure used to check if a resource is available given a vertex
      /// in case a vertex is not included in a map this mean that the used resources are zero.
      /// First index is the functional unit type, second index is the controller node, third index is the condition
      CustomMap<unsigned int, unsigned int> used_resources;

      /// Operations which can be scheduled in this control step because precedences are satisfied, but they can't be scheduled in this control step for some reasons
      /// Index is the functional unit type
      CustomMap<unsigned int, OpVertexSet> black_list;

      /// Adding information about operation still live
      auto live_vertex_it = live_vertices.begin();
      PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "         Considering live vertices...");
      while(live_vertex_it != live_vertices.end())
      {
         if(ending_time.find(*live_vertex_it)->second <= from_strongtype_cast<double>(current_cycle) * clock_cycle)
         {
            PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "            Vertex " + GET_NAME(flow_graph, *live_vertex_it) + " dies");
            live_vertices.erase(*live_vertex_it);
            live_vertex_it = live_vertices.begin();
         }
         else
         {
            const auto II = HLS->allocation_information->get_initiation_time(res_binding->get_assign(*live_vertex_it), *live_vertex_it);
            if(FB->build_simple_pipeline() && (II > 1 || II == 0))
            {
               auto lat = ending_time[*live_vertex_it] - starting_time[*live_vertex_it];
               THROW_ERROR("Timing of Vertex " + GET_NAME(flow_graph, *live_vertex_it) + " is not compatible with II=1.\nActual vertex latency is " + STR(lat) + " greater than the clock period");
            }

            if(II == 0u || current_cycle < (II + static_cast<unsigned int>(floor(starting_time[*live_vertex_it] / clock_cycle))))
            {
               bool schedulable;
               if(used_resources.find(res_binding->get_assign(*live_vertex_it)) == used_resources.end())
                  used_resources[res_binding->get_assign(*live_vertex_it)] = 0;
               schedulable = BB_update_resources_use(used_resources[res_binding->get_assign(*live_vertex_it)], res_binding->get_assign(*live_vertex_it));
               if(!schedulable)
                  THROW_ERROR("Unfeasible scheduling");
            }
            PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "            Vertex " + GET_NAME(flow_graph, *live_vertex_it) + " lives until " + STR(ending_time.find(*live_vertex_it)->second));
            ++live_vertex_it;
         }
      }

      for(unsigned int fu_type = 0; fu_type < n_resources; ++fu_type)
      {
         if(priority_queues[fu_type].size())
            ready_resources.insert(fu_type);
      }

      CustomMap<unsigned int, OpVertexSet> postponed_resources;
      CustomMap<unsigned int, OpVertexSet> restarted_resources;
      bool do_again;

      auto prev_scheduled = schedule->num_scheduled();
      do
      {
         do_again = false;
         while(ready_resources.size())
         {
            unsigned int fu_type;
            auto best_res_it = ready_resources.begin();
            fu_type = *best_res_it;

            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                           "-->Considering functional unit type " + STR(fu_type) + "(" + HLS->allocation_information->get_fu_name(fu_type).first + "-" + HLS->allocation_information->get_fu_name(fu_type).second + ")" + " at clock cycle " +
                               STR(current_cycle));
            ready_resources.erase(best_res_it);

            auto& queue = priority_queues[fu_type];
            /// Ignore empty list
            while(queue.size())
            {
#if HAVE_UNORDERED
               auto current_vertex = queue.top();
#else
               auto current_vertex = *(queue.begin());
#endif
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---First operation ready for this unit is " + GET_NAME(flow_graph, current_vertex));
#if HAVE_UNORDERED
               queue.pop();
#else
               queue.erase(queue.begin());
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Other ready operations (" + STR(queue.size()) + ") are:");
#ifndef NDEBUG
               for(const auto temp_operation : queue)
               {
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---" + GET_NAME(flow_graph, temp_operation) + ": " + STR((*Priority)(temp_operation)));
               }
#endif
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--");
#endif
               /// Ignore already scheduled operation
               if(schedule->is_scheduled(current_vertex))
               {
                  PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "               Already scheduled");
                  /// remove current_vertex from the queue
                  continue;
               }
               /// true if operation is schedulable
               /// check if there exist enough resources available
               if(used_resources.find(fu_type) == used_resources.end())
                  used_resources[fu_type] = 0;
               bool schedulable = used_resources.at(fu_type) != HLS->allocation_information->get_number_fu(fu_type);
               if(!schedulable)
               {
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---No free resource");
                  if(black_list.find(fu_type) == black_list.end())
                     black_list.emplace(fu_type, OpVertexSet(flow_graph));
                  black_list.at(fu_type).insert(current_vertex);
                  continue;
               }
               /// check compatibility
               bool postponed = false;
               if(GET_TYPE(flow_graph, current_vertex) & (TYPE_LOAD | TYPE_STORE))
               {
                  bool is_array = HLS->allocation_information->is_direct_access_memory_unit(fu_type);
                  unsigned var = is_array ? (HLS->allocation_information->is_memory_unit(fu_type) ? HLS->allocation_information->get_memory_var(fu_type) : HLS->allocation_information->get_proxy_memory_var(fu_type)) : 0;
                  if(is_array && cstep_vuses_others && !HLSMgr->Rmem->is_private_memory(var))
                     postponed = true;
                  else if(!is_array && cstep_vuses_ARRAYs)
                     postponed = true;
                  /*
                  if(!postponed && !cstep_vuses_ARRAYs && !cstep_vuses_others)
                  {
                     for(auto cur_fu_type: ready_resources)
                     {
                        if(HLS->allocation_information->is_direct_access_memory_unit(cur_fu_type) || HLS->allocation_information->is_indirect_access_memory_unit(cur_fu_type))
                        {
                           vertex cand_vertex = priority_queues[cur_fu_type].top();
                           if((*Priority)(current_vertex)<(*Priority)(cand_vertex))
                              postponed = true;
                        }
                     }
                  }*/
               }
               if(postponed)
               {
                  PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                "                  MEMORY_CTRL cannot run together with BRAM direct accesses " + GET_NAME(flow_graph, current_vertex) + " mapped on " + HLS->allocation_information->get_fu_name(fu_type).first + "at cstep " +
                                    STR(current_cycle));
                  if(black_list.find(fu_type) == black_list.end())
                     black_list.emplace(fu_type, OpVertexSet(flow_graph));
                  black_list.at(fu_type).insert(current_vertex);
                  continue;
               }

               bool is_live = check_if_is_live_in_next_cycle(live_vertices, current_cycle, ending_time, clock_cycle);
               THROW_ASSERT(!(GET_TYPE(flow_graph, current_vertex) & (TYPE_WHILE | TYPE_FOR)), "not expected operation type");
               /// put these type of operations as last operation scheduled for the basic block
               if((GET_TYPE(flow_graph, current_vertex) & (TYPE_IF | TYPE_RET | TYPE_SWITCH | TYPE_MULTIIF | TYPE_GOTO)) && (unbounded || unbounded_RW || is_live))
               {
                  if(black_list.find(fu_type) == black_list.end())
                     black_list.emplace(fu_type, OpVertexSet(flow_graph));
                  black_list.at(fu_type).insert(current_vertex);
                  PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "            Scheduling of Control Vertex " + GET_NAME(flow_graph, current_vertex) + " postponed to the next cycle");
                  continue;
               }
               if((GET_TYPE(flow_graph, current_vertex) & (TYPE_IF | TYPE_RET | TYPE_SWITCH | TYPE_MULTIIF | TYPE_GOTO)) && ((schedule->num_scheduled() - already_sch) != operations_number - 1))
               {
                  if(postponed_resources.find(fu_type) == postponed_resources.end())
                     postponed_resources.emplace(fu_type, OpVertexSet(flow_graph));
                  postponed_resources.at(fu_type).insert(current_vertex);
                  PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "            Scheduling of Control Vertex " + GET_NAME(flow_graph, current_vertex) + " postponed ");
                  continue;
               }

               if((GET_TYPE(flow_graph, current_vertex) & TYPE_RET) && ((schedule->num_scheduled() - already_sch) == operations_number - 1) && n_scheduled_ops != 0 && registering_output_p)
               {
                  if(black_list.find(fu_type) == black_list.end())
                     black_list.emplace(fu_type, OpVertexSet(flow_graph));
                  black_list.at(fu_type).insert(current_vertex);

                  PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "            Scheduling of Control Vertex " + GET_NAME(flow_graph, current_vertex) + " postponed to the next cycle to register the output");
                  continue;
               }
               if(!HLS->allocation_information->is_operation_bounded(flow_graph, current_vertex, fu_type) && RW_stmts.find(current_vertex) == RW_stmts.end() && (unbounded || unbounded_RW || is_live || store_unbounded_check))
               {
                  if(black_list.find(fu_type) == black_list.end())
                     black_list.emplace(fu_type, OpVertexSet(flow_graph));
                  black_list.at(fu_type).insert(current_vertex);
                  PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "            Scheduling of unbounded " + GET_NAME(flow_graph, current_vertex) + " postponed to the next cycle");
                  continue;
               }
               if(!HLS->allocation_information->is_operation_bounded(flow_graph, current_vertex, fu_type) && RW_stmts.find(current_vertex) != RW_stmts.end() && unbounded)
               {
                  if(black_list.find(fu_type) == black_list.end())
                     black_list.emplace(fu_type, OpVertexSet(flow_graph));
                  black_list.at(fu_type).insert(current_vertex);
                  PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "            Scheduling of unbounded RW interface " + GET_NAME(flow_graph, current_vertex) + " postponed to the next cycle");
                  continue;
               }

               /// starting time of the operation
               double current_starting_time;

               /// ending time of the operation
               double current_ending_time;

               /// stage period for pipelined units
               double current_stage_period;

               const auto current_cycle_starting_time = from_strongtype_cast<double>(current_cycle) * clock_cycle;
               const auto current_cycle_ending_time = from_strongtype_cast<double>(current_cycle + 1) * clock_cycle;

               double phi_extra_time;
               CustomMap<std::pair<unsigned int, unsigned int>, double> local_connection_map;

               bool predecessorsCond, pipeliningCond, cannotBeChained0, chainingRetCond, cannotBeChained1, asyncCond, cannotBeChained2, MultiCond0, MultiCond1, nonDirectMemCond;

               CheckSchedulabilityConditions(current_vertex, current_cycle, current_starting_time, current_ending_time, current_stage_period, local_connection_map, current_cycle_starting_time, current_cycle_ending_time, setup_hold_time, phi_extra_time,
                                             scheduling_mux_margins, unbounded, cstep_has_RET_conflict, fu_type, current_ASAP, res_binding, schedule, predecessorsCond, pipeliningCond, cannotBeChained0, chainingRetCond, cannotBeChained1, asyncCond,
                                             cannotBeChained2, MultiCond0, MultiCond1, nonDirectMemCond);

               /// checking if predecessors have finished
               if(predecessorsCond)
               {
                  PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "                  Depends on a live operation");
                  if(black_list.find(fu_type) == black_list.end())
                     black_list.emplace(fu_type, OpVertexSet(flow_graph));
                  black_list.at(fu_type).insert(current_vertex);
                  continue;
               }

               if(pipeliningCond)
               {
                  PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                "                  Pipelined unit cannot be chained: starting time is " + boost::lexical_cast<std::string>(current_starting_time) + " stage period is " + boost::lexical_cast<std::string>(current_stage_period));
                  if(black_list.find(fu_type) == black_list.end())
                     black_list.emplace(fu_type, OpVertexSet(flow_graph));
                  black_list.at(fu_type).insert(current_vertex);
                  continue;
               }
               else if(cannotBeChained0)
               {
                  PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                "                  It cannot be chained: starting time is " + boost::lexical_cast<std::string>(current_starting_time) + " ending time is " + boost::lexical_cast<std::string>(current_ending_time));
                  if(black_list.find(fu_type) == black_list.end())
                     black_list.emplace(fu_type, OpVertexSet(flow_graph));
                  black_list.at(fu_type).insert(current_vertex);
                  continue;
               }
               else if(chainingRetCond)
               {
                  PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "                  Chaining of return expression operation is not allowed by construction" + (unbounded ? std::string("(unbounded)") : std::string("(bounded)")));
                  if(black_list.find(fu_type) == black_list.end())
                     black_list.emplace(fu_type, OpVertexSet(flow_graph));
                  black_list.at(fu_type).insert(current_vertex);
                  continue;
               }
               else if(cannotBeChained1)
               {
                  PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                "                  Chaining with a store or a load or an unbounded in input is not possible -> starting time " + boost::lexical_cast<std::string>(current_starting_time) +
                                    " ending time: " + boost::lexical_cast<std::string>(current_ending_time));
                  if(black_list.find(fu_type) == black_list.end())
                     black_list.emplace(fu_type, OpVertexSet(flow_graph));
                  black_list.at(fu_type).insert(current_vertex);
                  continue;
               }
               /*
               else if(has_read_cond_with_non_direct_operations)
               {
                  PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "                  READ_COND or MULTI_READ_COND cannot be chained with non-direct operations -> starting time " + boost::lexical_cast<std::string>(current_starting_time) + " ending
                                time: " + boost::lexical_cast<std::string>(current_ending_time));
                  if(black_list.find(fu_type) == black_list.end())
                     black_list.emplace(fu_type, OpVertexSet(flow_graph));
                  black_list.at(fu_type).insert(current_vertex);
                  continue;
               }*/
               else if(asyncCond)
               {
                  PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                "                  Chaining with an asynchronous load is not possible -> starting time " + boost::lexical_cast<std::string>(current_starting_time) + " ending time: " + boost::lexical_cast<std::string>(current_ending_time));
                  if(black_list.find(fu_type) == black_list.end())
                     black_list.emplace(fu_type, OpVertexSet(flow_graph));
                  black_list.at(fu_type).insert(current_vertex);
                  continue;
               }
               /*else if((current_starting_time > (current_cycle_starting_time)) && (GET_TYPE(flow_graph, current_v) & TYPE_STORE) && HLS->allocation_information->is_indirect_access_memory_unit(fu_type))
               {
                  PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "                  Chaining with a store is not possible -> starting time " + boost::lexical_cast<std::string>(current_starting_time) + " ending time: " +
                                boost::lexical_cast<std::string>(current_ending_time));
                  if(black_list.find(fu_type) == black_list.end())
                     black_list.emplace(fu_type, OpVertexSet(flow_graph));
                  black_list.at(fu_type).insert(current_vertex);
                  continue;
               }
               else if((current_starting_time > (current_cycle_starting_time)) && (GET_TYPE(flow_graph, current_vertex) & TYPE_LOAD) && HLS->allocation_information->is_indirect_access_memory_unit(fu_type))
               {
                  PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "                  Chaining with a load is not possible -> starting time " + boost::lexical_cast<std::string>(current_starting_time) + " ending time: " +
                                boost::lexical_cast<std::string>(current_ending_time));
                  if(black_list.find(fu_type) == black_list.end())
                     black_list.emplace(fu_type, OpVertexSet(flow_graph));
                  black_list.at(fu_type).insert(current_vertex);
                  continue;
               }*/
               else if(cannotBeChained2)
               {
                  PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                "                  Chaining is possible but not allowed -> starting time " + boost::lexical_cast<std::string>(current_starting_time) + " ending time: " + boost::lexical_cast<std::string>(current_ending_time));
                  if(black_list.find(fu_type) == black_list.end())
                     black_list.emplace(fu_type, OpVertexSet(flow_graph));
                  black_list.at(fu_type).insert(current_vertex);
                  continue;
               }
               else if(MultiCond0)
               {
                  PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "                  First cycle of a Multi-cycles operations does not fit in the first period");
                  if(black_list.find(fu_type) == black_list.end())
                     black_list.emplace(fu_type, OpVertexSet(flow_graph));
                  black_list.at(fu_type).insert(current_vertex);
                  continue;
               }
               else if(MultiCond1)
               {
                  PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "                  Multi-cycles operations cannot be scheduled together with unbounded operations");
                  if(black_list.find(fu_type) == black_list.end())
                     black_list.emplace(fu_type, OpVertexSet(flow_graph));
                  black_list.at(fu_type).insert(current_vertex);
                  continue;
               }
               else if(nonDirectMemCond)
               {
                  PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "                  Non-direct memory access operations may conflict with unbounded operations");
                  if(black_list.find(fu_type) == black_list.end())
                     black_list.emplace(fu_type, OpVertexSet(flow_graph));
                  black_list.at(fu_type).insert(current_vertex);
                  continue;
               }
               /*else if(store_in_chaining_with_load_in(current_cycle, current_vertex))
               {
                  PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "                  Load and store cannot be chained");
                  if(black_list.find(fu_type) == black_list.end())
                     black_list.emplace(fu_type, OpVertexSet(flow_graph));
                  black_list.at(fu_type).insert(current_vertex);
                  continue;
               }*/

               bool restarted = false;
               if(RW_stmts.find(current_vertex) != RW_stmts.end())
               {
                  auto bb_index = flow_graph->CGetOpNodeInfo(current_vertex)->bb_index;
                  if(HLSMgr->design_interface_loads.find(fname) != HLSMgr->design_interface_loads.end() && HLSMgr->design_interface_loads.find(fname)->second.find(bb_index) != HLSMgr->design_interface_loads.find(fname)->second.end())
                  {
                     for(auto par2stmts : HLSMgr->design_interface_loads.find(fname)->second.find(bb_index)->second)
                     {
                        CustomOrderedSet<vertex> OpCluster;
                        for(auto stmt : par2stmts.second)
                        {
                           THROW_ASSERT(flow_graph->CGetOpGraphInfo()->tree_node_to_operation.find(stmt) != flow_graph->CGetOpGraphInfo()->tree_node_to_operation.end(), "unexpected condition");
                           OpCluster.insert(flow_graph->CGetOpGraphInfo()->tree_node_to_operation.find(stmt)->second);
                        }
                        if(OpCluster.find(current_vertex) != OpCluster.end())
                        {
                           unsigned numReadyOrScheduled = 0;
                           for(auto op : OpCluster)
                           {
                              if(op == current_vertex)
                                 ++numReadyOrScheduled;
                              else if(schedule->is_scheduled(op))
                                 ++numReadyOrScheduled;
                              else if(std::find(queue.begin(), queue.end(), op) != queue.end())
                              {
                                 bool predecessorsCondLocal, pipeliningCondLocal, cannotBeChained0Local, chainingRetCondLocal, cannotBeChained1Local, asyncCondLocal, cannotBeChained2Local, MultiCond0Local, MultiCond1Local, nonDirectMemCondLocal;
                                 double current_starting_timeLocal, current_ending_timeLocal, current_stage_periodLocal, phi_extra_timeLocal;
                                 CustomMap<std::pair<unsigned int, unsigned int>, double> local_connection_mapLocal;
                                 CheckSchedulabilityConditions(op, current_cycle, current_starting_timeLocal, current_ending_timeLocal, current_stage_periodLocal, local_connection_mapLocal, current_cycle_starting_time, current_cycle_ending_time,
                                                               setup_hold_time, phi_extra_timeLocal, scheduling_mux_margins, unbounded, cstep_has_RET_conflict, fu_type, current_ASAP, res_binding, schedule, predecessorsCondLocal, pipeliningCondLocal,
                                                               cannotBeChained0Local, chainingRetCondLocal, cannotBeChained1Local, asyncCondLocal, cannotBeChained2Local, MultiCond0Local, MultiCond1Local, nonDirectMemCondLocal);
                                 if(!predecessorsCondLocal && !pipeliningCondLocal && !cannotBeChained0Local && !chainingRetCondLocal && !cannotBeChained1Local && !asyncCondLocal && !cannotBeChained2Local && !MultiCond0Local && !MultiCond1Local &&
                                    !nonDirectMemCondLocal)
                                 {
                                    ++numReadyOrScheduled;
                                 }
                              }
                           }
                           // std::cerr << "Rfu_type=" << fu_type << "numReady=" << numReadyOrScheduled << "vs ops=" << HLS->allocation_information->get_number_fu(fu_type) << "\n";
                           if(numReadyOrScheduled % HLS->allocation_information->get_number_fu(fu_type) && OpCluster.size() != numReadyOrScheduled)
                              restarted = true;
                           break;
                        }
                     }
                  }
                  if(HLSMgr->design_interface_stores.find(fname) != HLSMgr->design_interface_stores.end() && HLSMgr->design_interface_stores.find(fname)->second.find(bb_index) != HLSMgr->design_interface_stores.find(fname)->second.end())
                  {
                     for(auto par2stmts : HLSMgr->design_interface_stores.find(fname)->second.find(bb_index)->second)
                     {
                        CustomOrderedSet<vertex> OpCluster;
                        for(auto stmt : par2stmts.second)
                        {
                           THROW_ASSERT(flow_graph->CGetOpGraphInfo()->tree_node_to_operation.find(stmt) != flow_graph->CGetOpGraphInfo()->tree_node_to_operation.end(), "unexpected condition");
                           OpCluster.insert(flow_graph->CGetOpGraphInfo()->tree_node_to_operation.find(stmt)->second);
                        }
                        if(OpCluster.find(current_vertex) != OpCluster.end())
                        {
                           unsigned numReadyOrScheduled = 0;
                           for(auto op : OpCluster)
                           {
                              if(op == current_vertex)
                                 ++numReadyOrScheduled;
                              else if(schedule->is_scheduled(op))
                                 ++numReadyOrScheduled;
                              else if(std::find(queue.begin(), queue.end(), op) != queue.end())
                              {
                                 bool predecessorsCondLocal, pipeliningCondLocal, cannotBeChained0Local, chainingRetCondLocal, cannotBeChained1Local, asyncCondLocal, cannotBeChained2Local, MultiCond0Local, MultiCond1Local, nonDirectMemCondLocal;
                                 double current_starting_timeLocal, current_ending_timeLocal, current_stage_periodLocal, phi_extra_timeLocal;
                                 CustomMap<std::pair<unsigned int, unsigned int>, double> local_connection_mapLocal;
                                 CheckSchedulabilityConditions(op, current_cycle, current_starting_timeLocal, current_ending_timeLocal, current_stage_periodLocal, local_connection_mapLocal, current_cycle_starting_time, current_cycle_ending_time,
                                                               setup_hold_time, phi_extra_timeLocal, scheduling_mux_margins, unbounded, cstep_has_RET_conflict, fu_type, current_ASAP, res_binding, schedule, predecessorsCondLocal, pipeliningCondLocal,
                                                               cannotBeChained0Local, chainingRetCondLocal, cannotBeChained1Local, asyncCondLocal, cannotBeChained2Local, MultiCond0Local, MultiCond1Local, nonDirectMemCondLocal);
                                 if(!predecessorsCondLocal && !pipeliningCondLocal && !cannotBeChained0Local && !chainingRetCondLocal && !cannotBeChained1Local && !asyncCondLocal && !cannotBeChained2Local && !MultiCond0Local && !MultiCond1Local &&
                                    !nonDirectMemCondLocal)
                                 {
                                    ++numReadyOrScheduled;
                                 }
                              }
                           }
                           // std::cerr << "Wfu_type=" << fu_type << "numReady=" << numReadyOrScheduled << "vs ops=" << HLS->allocation_information->get_number_fu(fu_type) << "\n";
                           if(numReadyOrScheduled % HLS->allocation_information->get_number_fu(fu_type) && OpCluster.size() != numReadyOrScheduled)
                              restarted = true;
                           break;
                        }
                     }
                  }
               }
               if(restarted)
               {
                  //                  std::cerr<< "Restarted\n";
                  PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                "                  Interface operation restarted " + GET_NAME(flow_graph, current_vertex) + " mapped on " + HLS->allocation_information->get_fu_name(fu_type).first + "at cstep " + STR(current_cycle));
                  if(restarted_resources.find(fu_type) == restarted_resources.end())
                     restarted_resources.emplace(fu_type, OpVertexSet(flow_graph));
                  restarted_resources.at(fu_type).insert(current_vertex);
                  continue;
               }

               /// scheduling is now possible
               ++n_scheduled_ops;
               /// update resource usage
               used_resources[fu_type]++;
               /// check if there exist enough resources available
               if(!HLS->allocation_information->is_operation_bounded(flow_graph, current_vertex, fu_type) && RW_stmts.find(current_vertex) == RW_stmts.end())
               {
                  PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "                  " + GET_NAME(flow_graph, current_vertex) + " is unbounded");
                  THROW_ASSERT(!(unbounded || is_live || store_unbounded_check), "unexpected case");
                  double ex_time = HLS->allocation_information->get_execution_time(fu_type, current_vertex, flow_graph);
                  if(ex_time > clock_cycle)
                     THROW_WARNING("Operation execution time of the unbounded operation is greater than the clock period resource fraction (" + STR(clock_cycle) + ").\n\tExecution time " + STR(ex_time) + " of " + GET_NAME(flow_graph, current_vertex) +
                                   " of type " + flow_graph->CGetOpNodeInfo(current_vertex)->GetOperation() + "\nThis may prevent meeting the timing constraints.\n");
                  unbounded = true;
               }
               else if(!HLS->allocation_information->is_operation_bounded(flow_graph, current_vertex, fu_type) && RW_stmts.find(current_vertex) != RW_stmts.end())
               {
                  unbounded_RW = true;
               }
               else
               {
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---" + GET_NAME(flow_graph, current_vertex) + " is bounded");
               }

               /// update cstep_vuses
               if(GET_TYPE(flow_graph, current_vertex) & (TYPE_LOAD | TYPE_STORE))
               {
                  bool is_array = HLS->allocation_information->is_direct_access_memory_unit(fu_type);
                  unsigned var = is_array ? (HLS->allocation_information->is_memory_unit(fu_type) ? HLS->allocation_information->get_memory_var(fu_type) : HLS->allocation_information->get_proxy_memory_var(fu_type)) : 0;
                  if(!var || !HLSMgr->Rmem->is_private_memory(var))
                  {
                     seen_cstep_has_RET_conflict = cstep_has_RET_conflict = true;
                     if(HLS->allocation_information->is_direct_access_memory_unit(fu_type) && !cstep_vuses_others)
                        cstep_vuses_ARRAYs = 1;
                     else
                     {
                        cstep_vuses_others = 1 + ((parameters->getOption<std::string>(OPT_memory_controller_type) != "D00" && parameters->getOption<std::string>(OPT_memory_controller_type) != "D10") ? 1 : 0);
                     }
                  }
               }
               if((GET_TYPE(flow_graph, current_vertex) & (TYPE_LOAD | TYPE_STORE)) and not HLS->allocation_information->is_direct_access_memory_unit(fu_type))
               {
                  store_unbounded_check = true; /// even if it is bounded we would like to prevent non-direct memory accesses running together with unbounded operations
               }
               // if(GET_TYPE(flow_graph, current_vertex)&TYPE_EXTERNAL && !HLS->allocation_information->is_operation_bounded(flow_graph, current_vertex, fu_type))
               //   seen_cstep_has_RET_conflict=cstep_has_RET_conflict = true;
               /// set the schedule information
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---" + GET_NAME(flow_graph, current_vertex) + " scheduled at " + STR(current_cycle_starting_time));
               schedule->set_execution(current_vertex, current_cycle);
               starting_time[current_vertex] = current_starting_time;
               if(!HLS->allocation_information->is_operation_bounded(flow_graph, current_vertex, fu_type) && (current_ending_time + setup_hold_time + phi_extra_time + scheduling_mux_margins > current_cycle_ending_time))
               {
                  ending_time[current_vertex] = current_cycle_ending_time - setup_hold_time;
               }
               else
                  ending_time[current_vertex] = current_ending_time;
               auto current_statement = flow_graph->CGetOpNodeInfo(current_vertex)->GetNodeId();
               schedule->starting_times[current_statement] = starting_time[current_vertex];
               schedule->ending_times[current_statement] = ending_time[current_vertex];
               THROW_ASSERT(ending_time[current_vertex] >= starting_time[current_vertex], "unexpected condition");
               THROW_ASSERT(floor(ending_time[current_vertex] / clock_cycle + 0.0001) >= from_strongtype_cast<double>(current_cycle),
                            GET_NAME(flow_graph, current_vertex) + " " + STR(ending_time[current_vertex]) + " " + STR(from_strongtype_cast<double>(current_cycle)));
               schedule->set_execution_end(current_vertex, current_cycle + ControlStep(static_cast<unsigned int>(floor(ending_time[current_vertex] / clock_cycle) - from_strongtype_cast<double>(current_cycle))));
               for(auto edge_connection_pair : local_connection_map)
                  schedule->AddConnectionTimes(edge_connection_pair.first.first, edge_connection_pair.first.second, edge_connection_pair.second);
               if(phi_extra_time > 0.0)
                  schedule->AddConnectionTimes(current_statement, 0, phi_extra_time);
               ready_vertices.erase(current_vertex);

               /// set the binding information
               if(HLS->HLS_C->has_binding_to_fu(GET_NAME(flow_graph, current_vertex)))
                  res_binding->bind(current_vertex, fu_type, 0);
               else
                  res_binding->bind(current_vertex, fu_type);

               PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "                  Current cycles ends at " + STR(current_cycle_ending_time) + "  - operation ends at " + boost::lexical_cast<std::string>(ending_time[current_vertex]));
               if(ending_time[current_vertex] > current_cycle_ending_time)
                  live_vertices.insert(current_vertex);

               /// Check if some successors have become ready
               OutEdgeIterator eo, eo_end;

               std::list<std::pair<std::string, vertex>> successors;
               for(boost::tie(eo, eo_end) = boost::out_edges(current_vertex, *flow_graph); eo != eo_end; eo++)
               {
                  vertex target = boost::target(*eo, *flow_graph);
                  successors.push_back(std::make_pair(GET_NAME(flow_graph, target), target));
               }
               // successors.sort();

               for(auto s = successors.begin(); s != successors.end(); ++s)
               {
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Considering successor " + s->first);
                  scheduled_predecessors[s->second]++;
                  if(current_ASAP.find(s->second) != current_ASAP.end())
                  {
                     current_ASAP.at(s->second) = std::max(ControlStep(static_cast<unsigned int>(floor(ending_time.find(current_vertex)->second / clock_cycle))), current_ASAP.find(s->second)->second);
                  }
                  else
                  {
                     current_ASAP.emplace(s->second, ControlStep(static_cast<unsigned int>(floor(ending_time.find(current_vertex)->second / clock_cycle))));
                  }
                  /// check if to_v should be considered as ready
                  if(boost::in_degree(s->second, *flow_graph) == scheduled_predecessors(s->second))
                  {
                     INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Become ready");
                     ready_vertices.insert(s->second);
                     add_to_priority_queues(priority_queues, ready_resources, s->second);
                  }
               }
            }
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Finished with unit type " + STR(fu_type));
         }
         if(!restarted_resources.empty())
         {
            CustomMap<unsigned int, OpVertexSet>::const_iterator bl_end = restarted_resources.end();
            for(CustomMap<unsigned int, OpVertexSet>::const_iterator bl_it = restarted_resources.begin(); bl_it != bl_end; ++bl_it)
            {
               auto v_end = bl_it->second.end();
               for(auto v = bl_it->second.begin(); v != v_end; ++v)
               {
#if HAVE_UNORDERED
                  priority_queues[bl_it->first].push(*v);
#else
                  priority_queues[bl_it->first].insert(*v);
#endif
               }
               ready_resources.insert(bl_it->first);
            }
            restarted_resources.clear();
            if(prev_scheduled != schedule->num_scheduled())
            {
               do_again = true;
               PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "         Restarted the scheduling loop to accommodate restarted vertices: " + STR(prev_scheduled) + " vs " + STR(schedule->num_scheduled()));
               prev_scheduled = schedule->num_scheduled();
            }
         }
         if(!postponed_resources.empty())
         {
            CustomMap<unsigned int, OpVertexSet>::const_iterator bl_end = postponed_resources.end();
            for(CustomMap<unsigned int, OpVertexSet>::const_iterator bl_it = postponed_resources.begin(); bl_it != bl_end; ++bl_it)
            {
               auto v_end = bl_it->second.end();
               for(auto v = bl_it->second.begin(); v != v_end; ++v)
               {
#if HAVE_UNORDERED
                  priority_queues[bl_it->first].push(*v);
#else
                  priority_queues[bl_it->first].insert(*v);
#endif
               }
               ready_resources.insert(bl_it->first);
            }
            postponed_resources.clear();
            if(black_list.empty())
            {
               do_again = true;
               PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "         Restarted the scheduling loop to accommodate postponed vertices");
            }
         }
      } while(do_again);

      CustomMap<unsigned int, OpVertexSet>::const_iterator bl_end = black_list.end();
      for(CustomMap<unsigned int, OpVertexSet>::const_iterator bl_it = black_list.begin(); bl_it != bl_end; ++bl_it)
      {
         auto v_end = bl_it->second.end();
         for(auto v = bl_it->second.begin(); v != v_end; ++v)
         {
#if HAVE_UNORDERED
            priority_queues[bl_it->first].push(*v);
#else
            priority_queues[bl_it->first].insert(*v);
#endif
         }
      }
      /// update priorities and possibly rehash the queues
#if HAVE_UNORDERED
      const auto updated =
#endif
          Priority->update();
#if HAVE_UNORDERED
      if(updated)
         for(unsigned int i = 0; i < n_resources; i++)
            priority_queues[i].rehash();
#endif
      /// clear the vises
      cstep_vuses_ARRAYs = cstep_vuses_ARRAYs > 0 ? cstep_vuses_ARRAYs - 1 : 0;
      cstep_vuses_others = cstep_vuses_others > 0 ? cstep_vuses_others - 1 : 0;
      cstep_has_RET_conflict = registering_output_p;
      /// move to the next cycle
      ++current_cycle;
   }

   if(HLS->Param->isOption(OPT_post_rescheduling) && parameters->getOption<bool>(OPT_post_rescheduling))
   {
      /// update starting and ending time of operations
      /// by running them as late as possible
      PRINT_DBG_MEX(DEBUG_LEVEL_VERBOSE, debug_level, "   update starting ending time");
      const OpGraphConstRef opDFG = FB->CGetOpGraph(FunctionBehavior::DFG, operations);
      const std::deque<vertex>& levels = FB->get_levels();
      std::deque<vertex> sub_levels;
      for(auto rit = levels.rbegin(); rit != levels.rend(); ++rit)
      {
         vertex candidate_v = *rit;
         if(operations.find(candidate_v) == operations.end())
            continue;
         sub_levels.push_back(candidate_v);
         update_starting_ending_time(candidate_v, res_binding, opDFG, schedule);
      }
      PRINT_DBG_MEX(DEBUG_LEVEL_VERBOSE, debug_level, "   compute vertices slack");
      /// compute the vertices slack
      OpVertexSet vertices_analyzed(opDFG);
      for(std::deque<vertex>::const_iterator vi = sub_levels.begin(); vi != sub_levels.end(); ++vi)
      {
         if(vertices_analyzed.find(*vi) != vertices_analyzed.end())
         {
            continue;
         }
         update_vertices_slack(*vi, schedule, vertices_analyzed, setup_hold_time, opDFG);
      }

      do_balanced_scheduling1(sub_levels, schedule, res_binding, setup_hold_time, scheduling_mux_margins, opDFG, seen_cstep_has_RET_conflict);

      for(std::deque<vertex>::const_reverse_iterator vi = sub_levels.rbegin(); vi != sub_levels.rend(); ++vi)
      {
         update_starting_ending_time_asap(*vi, res_binding, opDFG, schedule);
         schedule->starting_times[flow_graph->CGetOpNodeInfo(*vi)->GetNodeId()] = starting_time[*vi];
         schedule->ending_times[flow_graph->CGetOpNodeInfo(*vi)->GetNodeId()] = ending_time[*vi];
      }
   }

   const auto steps = current_cycle;
   schedule->set_csteps(steps);
   PRINT_DBG_MEX(DEBUG_LEVEL_VERBOSE, debug_level, "   List Based finished");
}

void parametric_list_based::compute_starting_ending_time_asap(const vertex v, const unsigned int fu_type, const ControlStep cs, double& current_starting_time, double& current_ending_time, double& stage_period, bool& cannot_be_chained,
                                                              fu_bindingRef res_binding, const ScheduleConstRef schedule, double& phi_extra_time, double setup_hold_time, CustomMap<std::pair<unsigned int, unsigned int>, double>& local_connection_map)
{
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Computing starting and ending time " + GET_NAME(flow_graph, v));
   current_starting_time = from_strongtype_cast<double>(cs) * clock_cycle;
   bool is_load_store = GET_TYPE(flow_graph, v) & (TYPE_LOAD | TYPE_STORE);
   bool no_chaining_of_load_and_store = parameters->getOption<bool>(OPT_do_not_chain_memories) && (check_LOAD_chaining(v, cs, schedule) || is_load_store);
   cannot_be_chained = is_load_store && check_non_direct_operation_chaining(v, fu_type, cs, schedule, res_binding);
   PRINT_DBG_MEX(DEBUG_LEVEL_VERBOSE, debug_level, "                  Initial value of cannot_be_chained=" + (cannot_be_chained ? std::string("T") : std::string("F")));
   InEdgeIterator ei, ei_end;
   for(boost::tie(ei, ei_end) = boost::in_edges(v, *flow_graph); ei != ei_end; ei++)
   {
      vertex from_vertex = boost::source(*ei, *flow_graph);
      if(GET_TYPE(flow_graph, from_vertex) & (TYPE_PHI | TYPE_VPHI))
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Skipping phi predecessor " + GET_NAME(flow_graph, from_vertex));
         continue;
      }
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Considering predecessor " + GET_NAME(flow_graph, from_vertex));
      unsigned int from_fu_type = res_binding->get_assign(from_vertex);
      const auto cs_prev = schedule->get_cstep(from_vertex).second;
      const double fsm_correction = [&]() -> double {
         if(parameters->getOption<double>(OPT_scheduling_mux_margins) != 0.0)
            return HLS->allocation_information->EstimateControllerDelay();
         if(cs == cs_prev && HLS->allocation_information->is_one_cycle_direct_access_memory_unit(from_fu_type) &&
            (!HLS->allocation_information->is_readonly_memory_unit(from_fu_type) || (!HLS->Param->isOption(OPT_rom_duplication) || !parameters->getOption<bool>(OPT_rom_duplication))) &&
            HLSMgr->Rmem->get_maximum_references(HLS->allocation_information->is_memory_unit(from_fu_type) ? HLS->allocation_information->get_memory_var(from_fu_type) : HLS->allocation_information->get_proxy_memory_var(from_fu_type)) >
                HLS->allocation_information->get_number_channels(from_fu_type))
            return HLS->allocation_information->EstimateControllerDelay();
         return 0.0;
      }();
      auto from_statement = flow_graph->CGetOpNodeInfo(from_vertex)->GetNodeId();
      auto v_statement = flow_graph->CGetOpNodeInfo(v)->GetNodeId();
      const auto v_basic_block_index = flow_graph->CGetOpNodeInfo(v)->bb_index;
      auto edge_pair = std::make_pair(from_statement, v_statement);
      double connection_time = local_connection_map[edge_pair] = (schedule->get_cstep_end(from_vertex).second == cs) ? HLS->allocation_information->GetConnectionTime(from_statement, v_statement, AbsControlStep(v_basic_block_index, cs)) : 0.0;
      /// ending time is equal to the connection time plus the maximum between the controller time and the operation ending time
      double local_ending_time = connection_time + ((cs == cs_prev && starting_time(from_vertex) < (fsm_correction + (from_strongtype_cast<double>(cs) * clock_cycle))) ?
                                                        ending_time.find(from_vertex)->second + fsm_correction + (from_strongtype_cast<double>(cs) * clock_cycle) - starting_time(from_vertex) :
                                                        ending_time.find(from_vertex)->second);
      current_starting_time = std::max(current_starting_time, local_ending_time);
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "current_starting_time of " + STR(GET_NAME(flow_graph, v)) + " updated to " + STR(current_starting_time));

      /// Check for chaining
      if(schedule->get_cstep_end(from_vertex).second == cs)
      {
         if(no_chaining_of_load_and_store)
            cannot_be_chained = true;
         if(not HLS->allocation_information->CanBeChained(from_vertex, v))
            cannot_be_chained = true;
      }
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--");
   }

   double op_execution_time;
   compute_exec_stage_time(fu_type, stage_period, cs, flow_graph, v, op_execution_time, phi_extra_time, current_starting_time, setup_hold_time);

   current_ending_time = current_starting_time + op_execution_time;

   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                  "<--Operation " + GET_NAME(flow_graph, v) + " starts at " + STR(current_starting_time) + " and ends at " + STR(current_ending_time) + " execution time " + STR(op_execution_time) + " stage period " + STR(stage_period) +
                      " phi_extra_time " + STR(phi_extra_time));
}

void parametric_list_based::compute_exec_stage_time(const unsigned int fu_type, double& stage_period, const ControlStep cs, const OpGraphConstRef op_graph, vertex v, double& op_execution_time, double& phi_extra_time, double current_starting_time,
                                                    double setup_hold_time)
{
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Computing exec stage time of " + GET_NAME(flow_graph_with_feedbacks, v));
   const auto bb_index = op_graph->CGetOpNodeInfo(v)->bb_index;
   std::pair<double, double> timeLatency = HLS->allocation_information->GetTimeLatency(v, fu_type);
   op_execution_time = timeLatency.first;
   stage_period = timeLatency.second;

   /// check for PHIs attached to the output. They may require one or more muxes.
   phi_extra_time = HLS->allocation_information->GetConnectionTime(flow_graph_with_feedbacks->CGetOpNodeInfo(v)->GetNodeId(), 0, AbsControlStep(bb_index, cs));

   double scheduling_mux_margins = parameters->getOption<double>(OPT_scheduling_mux_margins) * HLS->allocation_information->mux_time_unit(32);

   /// corrections for the memory controllers
   unsigned int n_cycles = std::max(1u, HLS->allocation_information->get_cycles(fu_type, v, flow_graph));
   if((HLS->allocation_information->is_indirect_access_memory_unit(fu_type)) && op_execution_time + setup_hold_time >= clock_cycle * n_cycles)
      op_execution_time = clock_cycle * n_cycles - setup_hold_time - phi_extra_time - scheduling_mux_margins - EPSILON;
   if((HLS->allocation_information->is_indirect_access_memory_unit(fu_type)) && stage_period + setup_hold_time >= clock_cycle)
      stage_period = clock_cycle - setup_hold_time - phi_extra_time - scheduling_mux_margins - EPSILON;

   /// corrections in case the unit first fits in a clock period and then after all the additions does not fit anymore
   double initial_execution_time = HLS->allocation_information->get_execution_time(fu_type, v, flow_graph_with_feedbacks) -
                                   HLS->allocation_information->get_correction_time(fu_type, flow_graph_with_feedbacks->CGetOpNodeInfo(v)->GetOperation(),
                                                                                    static_cast<unsigned>(flow_graph_with_feedbacks->CGetOpNodeInfo(v)->GetVariables(FunctionBehavior_VariableType::SCALAR, FunctionBehavior_VariableAccessType::USE).size()));
   if(initial_execution_time + setup_hold_time < clock_cycle && op_execution_time + setup_hold_time + phi_extra_time + scheduling_mux_margins >= clock_cycle)
   {
      op_execution_time = clock_cycle - setup_hold_time - phi_extra_time - scheduling_mux_margins - EPSILON;
      if(op_execution_time < 0)
      {
         op_execution_time = clock_cycle - setup_hold_time - scheduling_mux_margins - EPSILON;
         phi_extra_time = 0.0;
      }
   }
   if(HLS->allocation_information->get_initiation_time(fu_type, v) > 0 && (stage_period + setup_hold_time + phi_extra_time + scheduling_mux_margins >= clock_cycle))
   {
      stage_period = clock_cycle - setup_hold_time - phi_extra_time - scheduling_mux_margins - EPSILON;
      if(stage_period < 0)
      {
         stage_period = clock_cycle - setup_hold_time - scheduling_mux_margins - EPSILON;
         phi_extra_time = 0.0;
      }
   }
   if(stage_period != 0.0)
   {
      /// recompute the execution time from scratch
      if(n_cycles > 1)
      {
         op_execution_time = clock_cycle * (n_cycles - 1);
         if(stage_period > clock_cycle)
            op_execution_time += clock_cycle - EPSILON;
         else
            op_execution_time += stage_period;
         op_execution_time -= current_starting_time - from_strongtype_cast<double>(cs) * clock_cycle;
      }
      else
         THROW_ERROR("unexpected case");
   }
   /// Stage period has already been used for computing ending time of operation
   /// If the operation has registered inputs, the stage period of first stage is 0 (mux delay is described by connection time)
   if(HLS->allocation_information->is_operation_PI_registered(op_graph, v, HLS->allocation_information->GetFuType(v)))
      stage_period = 0.0;
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Execution time is " + STR(op_execution_time));
}

void parametric_list_based::compute_starting_ending_time_alap(vertex v, const unsigned int fu_type, const ControlStep cs, double& current_starting_time, double& max_ending_time, double& op_execution_time, double& stage_period, unsigned int& n_cycles,
                                                              bool& cannot_be_chained, fu_bindingRef res_binding, const ScheduleConstRef schedule, double& phi_extra_time, double setup_hold_time,
                                                              CustomMap<std::pair<unsigned int, unsigned int>, double>& local_connection_map)
{
   InEdgeIterator ei, ei_end;
   current_starting_time = from_strongtype_cast<double>(cs) * clock_cycle;
   bool is_load_store = (GET_TYPE(flow_graph, v) & (TYPE_STORE | TYPE_LOAD));
   bool no_chaining_of_load_and_store = parameters->getOption<bool>(OPT_do_not_chain_memories) && (check_LOAD_chaining(v, cs, schedule) || is_load_store);
   cannot_be_chained = is_load_store && check_non_direct_operation_chaining(v, fu_type, cs, schedule, res_binding);
   bool is_operation_unbounded_and_registered = !HLS->allocation_information->is_operation_bounded(flow_graph, v, fu_type) && HLS->allocation_information->is_operation_PI_registered(flow_graph, v, fu_type);
   for(boost::tie(ei, ei_end) = boost::in_edges(v, *flow_graph); ei != ei_end; ei++)
   {
      vertex from_vertex = boost::source(*ei, *flow_graph);
      unsigned int from_fu_type = res_binding->get_assign(from_vertex);
      const auto cs_prev = schedule->get_cstep(from_vertex).second;
      const double fsm_correction = [&]() -> double {
         if(parameters->getOption<double>(OPT_scheduling_mux_margins) != 0.0)
            return HLS->allocation_information->EstimateControllerDelay();
         if(cs == cs_prev && HLS->allocation_information->is_one_cycle_direct_access_memory_unit(from_fu_type) &&
            (!HLS->allocation_information->is_readonly_memory_unit(from_fu_type) || (!HLS->Param->isOption(OPT_rom_duplication) || !parameters->getOption<bool>(OPT_rom_duplication))) &&
            HLSMgr->Rmem->get_maximum_references(HLS->allocation_information->is_memory_unit(from_fu_type) ? HLS->allocation_information->get_memory_var(from_fu_type) : HLS->allocation_information->get_proxy_memory_var(from_fu_type)) >
                HLS->allocation_information->get_number_channels(from_fu_type))
            return HLS->allocation_information->EstimateControllerDelay();
         return 0.0;
      }();
      auto from_statement = flow_graph->CGetOpNodeInfo(from_vertex)->GetNodeId();
      auto v_statement = flow_graph->CGetOpNodeInfo(v)->GetNodeId();
      const auto v_basic_block_index = flow_graph->CGetOpNodeInfo(v)->bb_index;
      std::pair<unsigned int, unsigned int> edge_pair = std::pair<unsigned int, unsigned int>(from_statement, v_statement);
      double connection_time = local_connection_map[edge_pair] = HLS->allocation_information->GetConnectionTime(from_statement, v_statement, AbsControlStep(v_basic_block_index, cs));
      /// ending time is equal to the connection time plus the maximum between the controller time and the operation ending time
      double local_ending_time = connection_time + ((cs == cs_prev && starting_time(from_vertex) < (fsm_correction + (from_strongtype_cast<double>(cs) * clock_cycle))) ?
                                                        ending_time.find(from_vertex)->second + fsm_correction + (from_strongtype_cast<double>(cs) * clock_cycle) - starting_time(from_vertex) :
                                                        ending_time.find(from_vertex)->second);
      current_starting_time = std::max(current_starting_time, local_ending_time);

      if((GET_TYPE(flow_graph, from_vertex) & TYPE_STORE) and schedule->get_cstep_end(from_vertex).second == cs and !is_operation_unbounded_and_registered)
         cannot_be_chained = true;
      if(no_chaining_of_load_and_store && schedule->get_cstep_end(from_vertex).second == cs)
         cannot_be_chained = true;
      if(!HLS->allocation_information->is_operation_bounded(flow_graph, from_vertex, from_fu_type) and schedule->get_cstep_end(from_vertex).second == cs)
         cannot_be_chained = true;
   }
   OutEdgeIterator oi, oi_end;
   max_ending_time = std::numeric_limits<double>::max();
   for(boost::tie(oi, oi_end) = boost::out_edges(v, *flow_graph); oi != oi_end; oi++)
   {
      vertex to_vertex = boost::target(*oi, *flow_graph);
      max_ending_time = std::min(max_ending_time, starting_time(to_vertex));
   }

   compute_exec_stage_time(fu_type, stage_period, cs, flow_graph, v, op_execution_time, phi_extra_time, current_starting_time, setup_hold_time);

   n_cycles = HLS->allocation_information->get_cycles(fu_type, v, flow_graph);

   PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                 "                  Operation " + GET_NAME(flow_graph, v) + " starts at " + boost::lexical_cast<std::string>(current_starting_time) + " and should end before of " + boost::lexical_cast<std::string>(max_ending_time) + " execution time " +
                     STR(op_execution_time) + " stage period " + STR(stage_period));
}

bool parametric_list_based::BB_update_resources_use(unsigned int& used_resources, const unsigned int fu_type) const
{
   if(used_resources == HLS->allocation_information->get_number_fu(fu_type))
      return false;
   else
   {
      used_resources++;
      return true;
   }
}

void parametric_list_based::add_to_priority_queues(PriorityQueues& priority_queue, std::set<unsigned int, resource_ordering_functor>& ready_resources, const vertex v) const
{
   unsigned int fu_name;
   if(HLS->allocation_information->is_vertex_bounded_with(v, fu_name))
   {
#if HAVE_UNORDERED
      priority_queue[fu_name].push(v);
#else
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---" + GET_NAME(flow_graph, v) + " bound to " + HLS->allocation_information->get_fu_name(fu_name).first);
      priority_queue[fu_name].insert(v);
#endif
      ready_resources.insert(fu_name);
   }
   else
   {
      const CustomOrderedSet<unsigned int>& fu_set = HLS->allocation_information->can_implement_set(v);
      const CustomOrderedSet<unsigned int>::const_iterator fu_set_it_end = fu_set.end();
      for(auto fu_set_it = fu_set.begin(); fu_set_it != fu_set_it_end; ++fu_set_it)
      {
#if HAVE_UNORDERED
         priority_queue[*fu_set_it].push(v);
#else
         priority_queue[*fu_set_it].insert(v);
#endif
         ready_resources.insert(*fu_set_it);
      }
   }
}

DesignFlowStep_Status parametric_list_based::InternalExec()
{
   executions_number++;
   long int step_time = 0;
   if(output_level >= OUTPUT_LEVEL_MINIMUM and output_level <= OUTPUT_LEVEL_PEDANTIC)
      START_TIME(step_time);
   const FunctionBehaviorConstRef FB = HLSMgr->CGetFunctionBehavior(funId);
   const BBGraphConstRef bbg = FB->CGetBBGraph();
   const OpGraphConstRef op_graph = FB->CGetOpGraph(FunctionBehavior::CFG);
   std::deque<vertex> vertices;
   boost::topological_sort(*bbg, std::front_inserter(vertices));
   std::deque<vertex>::const_iterator viend = vertices.end();
   ControlStep ctrl_steps = ControlStep(0u);
   for(std::deque<vertex>::const_iterator vi = vertices.begin(); vi != viend; ++vi)
   {
      OpVertexSet operations(op_graph);
      std::list<vertex> bb_operations = bbg->CGetBBNodeInfo(*vi)->statements_list;
      for(auto& bb_operation : bb_operations)
      {
         if(HLS->operations.find(bb_operation) != HLS->operations.end())
            operations.insert(bb_operation);
      }
      PRINT_DBG_MEX(DEBUG_LEVEL_VERBOSE, debug_level, "performing scheduling of basic block " + STR(bbg->CGetBBNodeInfo(*vi)->block->number));
      PRINT_DBG_MEX(DEBUG_LEVEL_VERBOSE, debug_level, "  .operations: " + STR(operations.size()));
#if 1
      exec(operations, ctrl_steps);
#else
      exec(operations, executions_number == 1 ? ControlStep(0) : ctrl_steps);
#endif
      ctrl_steps = HLS->Rsch->get_csteps();
   }
   HLS->Rsch->set_spec(get_spec());

   /// Find the minimum slack
   double min_slack = std::numeric_limits<double>::max();
   for(auto operation : ending_time)
   {
      double realClock = HLS->HLS_C->get_clock_period();

      double endingTime = operation.second;
      const auto endControlStep = HLS->Rsch->get_cstep_end(operation.first).second;
      double slack = (from_strongtype_cast<double>(endControlStep) + 1) * clock_cycle - endingTime + realClock - clock_cycle - HLS->allocation_information->get_setup_hold_time();
      if(slack < 0)
         slack = EPSILON / 2;
      if(min_slack > slack)
         PRINT_DBG_MEX(DEBUG_LEVEL_VERBOSE, debug_level, std::string("Operation constraining the maximum frequency:") + GET_NAME(flow_graph, operation.first) + " slack=" + STR(slack));
      min_slack = std::min(min_slack, slack);
   }
   HLS->allocation_information->setMinimumSlack(min_slack);

   if(output_level <= OUTPUT_LEVEL_PEDANTIC)
      INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, output_level, "");
   INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, output_level, "-->Scheduling Information of function " + FB->CGetBehavioralHelper()->get_function_name() + ":");
#if 0
   if(executions_number > 0)
#endif
   INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, output_level, "---Number of control steps: " + STR(ctrl_steps));

   if(output_level >= OUTPUT_LEVEL_VERY_PEDANTIC)
   {
      HLS->Rsch->print(HLS->Rfu);
   }

   double maxFrequency = 1000.0 / (HLS->HLS_C->get_clock_period() - HLS->allocation_information->getMinimumSlack());
   INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, output_level, "---Minimum slack: " + STR(HLS->allocation_information->getMinimumSlack()));
   INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, output_level, "---Estimated max frequency (MHz): " + STR(maxFrequency));

   INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, output_level, "<--");
   if(output_level >= OUTPUT_LEVEL_MINIMUM and output_level <= OUTPUT_LEVEL_PEDANTIC)
      STOP_TIME(step_time);
   if(output_level >= OUTPUT_LEVEL_MINIMUM and output_level <= OUTPUT_LEVEL_PEDANTIC)
      INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, output_level, "---Time to perform scheduling: " + print_cpu_time(step_time) + " seconds");
   if(output_level <= OUTPUT_LEVEL_PEDANTIC)
      INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, output_level, "");

   if(parameters->getOption<bool>(OPT_print_dot))
   {
      HLS->Rsch->WriteDot("HLS_scheduling.dot");
   }
   if(debug_level >= DEBUG_LEVEL_VERBOSE)
   {
      const std::string function_name = FB->CGetBehavioralHelper()->get_function_name();
      xml_document document;
      xml_element* nodeRoot = document.create_root_node("hls");
      HLS->xwrite(nodeRoot, FB->CGetOpGraph(FunctionBehavior::FDFG));
      document.write_to_file_formatted(function_name + "_scheduling.XML");
   }
   return DesignFlowStep_Status::SUCCESS;
}

double parametric_list_based::compute_slack(vertex current_op, const ControlStep current_vertex_cstep, double setup_hold_time)
{
   double real_clock_period = HLS->HLS_C->get_clock_period();
   double starting_time_current_vertex = starting_time(current_op);
   double slack = starting_time_current_vertex - from_strongtype_cast<double>(current_vertex_cstep) * clock_cycle + real_clock_period - clock_cycle - setup_hold_time;
   if(slack < 0)
      slack = EPSILON / 2;
   return slack;
}

void parametric_list_based::update_vertices_slack(vertex current_v, const ScheduleRef schedule, OpVertexSet& vertices_analyzed, double setup_hold_time, const OpGraphConstRef opDFG)
{
   /// compute the set of operations on the clock frontier
   const auto current_vertex_cstep = schedule->get_cstep(current_v).second;
   std::queue<vertex> fifo;
   std::list<vertex> curr_list;
   double worst_slack = compute_slack(current_v, current_vertex_cstep, setup_hold_time);
   InEdgeIterator eo, eo_end;
   for(boost::tie(eo, eo_end) = boost::in_edges(current_v, *opDFG); eo != eo_end; eo++)
      fifo.push(boost::source(*eo, *opDFG));
   while(!fifo.empty())
   {
      vertex current_op = fifo.front();
      fifo.pop();
      if(current_vertex_cstep == schedule->get_cstep(current_op).second && vertices_analyzed.find(current_op) == vertices_analyzed.end())
      {
         curr_list.push_back(current_op);
         worst_slack = std::min(worst_slack, compute_slack(current_op, current_vertex_cstep, setup_hold_time));
         for(boost::tie(eo, eo_end) = boost::in_edges(current_op, *opDFG); eo != eo_end; eo++)
            fifo.push(boost::source(*eo, *opDFG));
      }
   }
   const std::list<vertex>::const_iterator cl_it_end = curr_list.end();
   for(std::list<vertex>::const_iterator cl_it = curr_list.begin(); cl_it != cl_it_end; ++cl_it)
   {
      vertices_analyzed.insert(*cl_it);
      schedule->set_slack(*cl_it, worst_slack);
   }
   vertices_analyzed.insert(current_v);
   schedule->set_slack(current_v, worst_slack);
}

void parametric_list_based::update_vertices_timing(const ControlStep vertex_cstep, vertex current_v, const ScheduleConstRef schedule, std::list<vertex>& vertices_analyzed, fu_bindingRef res_binding, const OpGraphConstRef opDFG)
{
   /// compute the set of operations on the clock frontier
   std::queue<vertex> fifo;
   update_starting_ending_time(current_v, res_binding, opDFG, schedule);
   vertices_analyzed.push_back(current_v);
   InEdgeIterator eo, eo_end;
   for(boost::tie(eo, eo_end) = boost::in_edges(current_v, *opDFG); eo != eo_end; eo++)
      fifo.push(boost::source(*eo, *opDFG));
   while(!fifo.empty())
   {
      vertex current_op = fifo.front();
      fifo.pop();
      if(vertex_cstep - 1u <= schedule->get_cstep(current_op).second)
      {
         vertices_analyzed.push_back(current_op);
         update_starting_ending_time(current_op, res_binding, opDFG, schedule);
         for(boost::tie(eo, eo_end) = boost::in_edges(current_op, *opDFG); eo != eo_end; eo++)
            fifo.push(boost::source(*eo, *opDFG));
      }
   }
}

void parametric_list_based::update_starting_ending_time(vertex candidate_v, fu_bindingRef res_binding, OpGraphConstRef opDFG, const ScheduleConstRef schedule)
{
   unsigned int fu_type = res_binding->get_assign(candidate_v);
   if(!HLS->allocation_information->is_operation_bounded(opDFG, candidate_v, fu_type))
   {
      return;
   }
   PRINT_DBG_MEX(DEBUG_LEVEL_VERBOSE, debug_level, "      update starting ending time " + GET_NAME(opDFG, candidate_v));
   PRINT_DBG_MEX(DEBUG_LEVEL_VERBOSE, debug_level, "      initial starting time " + STR(starting_time(candidate_v)));
   PRINT_DBG_MEX(DEBUG_LEVEL_VERBOSE, debug_level, "      initial ending time " + STR(ending_time.find(candidate_v)->second));
   OutEdgeIterator eo, eo_end;
   const double DOUBLE_BIG_NUM = std::numeric_limits<double>::max();
   double current_ending_time;
   current_ending_time = DOUBLE_BIG_NUM;
   for(boost::tie(eo, eo_end) = boost::out_edges(candidate_v, *opDFG); eo != eo_end; eo++)
   {
      vertex target = boost::target(*eo, *opDFG);
      PRINT_DBG_MEX(DEBUG_LEVEL_VERBOSE, debug_level, "      starting time target " + GET_NAME(opDFG, target) + " = " + STR(starting_time(target)));
      current_ending_time = std::min(current_ending_time, starting_time(target));
   }
   PRINT_DBG_MEX(DEBUG_LEVEL_VERBOSE, debug_level, "      current_ending_time = " + STR(current_ending_time));

   double ctrl_step_border = (from_strongtype_cast<double>(schedule->get_cstep_end(candidate_v).second) + 1) * clock_cycle - EPSILON;
   PRINT_DBG_MEX(DEBUG_LEVEL_VERBOSE, debug_level, "      control step border = " + STR(ctrl_step_border));

   bool is_pipelined = HLS->allocation_information->get_initiation_time(fu_type, candidate_v) != 0;
   /// non-pipelined operations move towards the control step border
   if(!is_pipelined)
   {
      THROW_ASSERT(ending_time[candidate_v] >= starting_time[candidate_v], "unexpected starting/ending time");
      current_ending_time = std::min(current_ending_time, ctrl_step_border);
      THROW_ASSERT((current_ending_time - (ending_time[candidate_v] - starting_time[candidate_v])) / clock_cycle >= floor(starting_time[candidate_v] / clock_cycle), "unexpected starting time");
      starting_time[candidate_v] = current_ending_time - (ending_time[candidate_v] - starting_time[candidate_v]);
      ending_time[candidate_v] = current_ending_time;
      PRINT_DBG_MEX(DEBUG_LEVEL_VERBOSE, debug_level, "      non-pipelined op new starting time = " + STR(starting_time[candidate_v]));
      PRINT_DBG_MEX(DEBUG_LEVEL_VERBOSE, debug_level, "      non-pipelined op new ending time   = " + STR(ending_time[candidate_v]));
   }
   else
   {
      double stage_period = ending_time[candidate_v] - clock_cycle * floor(ending_time[candidate_v] / clock_cycle);
      starting_time[candidate_v] = (floor(starting_time[candidate_v] / clock_cycle) + 1) * clock_cycle - stage_period;
      PRINT_DBG_MEX(DEBUG_LEVEL_VERBOSE, debug_level, "      pipelined op new starting time = " + STR(starting_time[candidate_v]));
      PRINT_DBG_MEX(DEBUG_LEVEL_VERBOSE, debug_level, "      pipelined op same ending time   = " + STR(ending_time[candidate_v]));
   }
}

void parametric_list_based::update_starting_ending_time_asap(vertex candidate_v, fu_bindingRef res_binding, OpGraphConstRef opDFG, const ScheduleConstRef schedule)
{
   unsigned int fu_type = res_binding->get_assign(candidate_v);
   if(!HLS->allocation_information->is_operation_bounded(opDFG, candidate_v, fu_type))
   {
      return;
   }
   PRINT_DBG_MEX(DEBUG_LEVEL_VERBOSE, debug_level, "      asap update starting ending time " + GET_NAME(opDFG, candidate_v));
   PRINT_DBG_MEX(DEBUG_LEVEL_VERBOSE, debug_level, "      asap initial starting time " + STR(starting_time(candidate_v)));
   PRINT_DBG_MEX(DEBUG_LEVEL_VERBOSE, debug_level, "      asap initial ending time " + STR(ending_time.find(candidate_v)->second));
   InEdgeIterator eo, eo_end;
   double current_starting_time;
   current_starting_time = 0;
   for(boost::tie(eo, eo_end) = boost::in_edges(candidate_v, *opDFG); eo != eo_end; eo++)
   {
      vertex src = boost::source(*eo, *opDFG);
      PRINT_DBG_MEX(DEBUG_LEVEL_VERBOSE, debug_level, "      asap ending time src " + GET_NAME(opDFG, src) + " = " + STR(ending_time.find(src)->second));
      current_starting_time = std::max(current_starting_time, ending_time.find(src)->second);
   }
   PRINT_DBG_MEX(DEBUG_LEVEL_VERBOSE, debug_level, "      asap current_ending_time = " + STR(current_starting_time));

   double ctrl_step_border = from_strongtype_cast<double>(schedule->get_cstep(candidate_v).second) * clock_cycle;
   PRINT_DBG_MEX(DEBUG_LEVEL_VERBOSE, debug_level, "      asap control step border = " + STR(ctrl_step_border));

   bool is_pipelined = HLS->allocation_information->get_initiation_time(fu_type, candidate_v) != 0;
   current_starting_time = std::max(current_starting_time, ctrl_step_border);
   /// non-pipelined operations move towards the control step border
   if(!is_pipelined)
   {
      THROW_ASSERT(ending_time[candidate_v] >= starting_time[candidate_v], "unexpected starting/ending time");
      ending_time[candidate_v] = current_starting_time + (ending_time[candidate_v] - starting_time[candidate_v]);
      starting_time[candidate_v] = current_starting_time;
      PRINT_DBG_MEX(DEBUG_LEVEL_VERBOSE, debug_level, "      asap non-pipelined op new starting time = " + STR(starting_time[candidate_v]));
      PRINT_DBG_MEX(DEBUG_LEVEL_VERBOSE, debug_level, "      asap non-pipelined op new ending time   = " + STR(ending_time[candidate_v]));
   }
   else
   {
      starting_time[candidate_v] = current_starting_time;
      PRINT_DBG_MEX(DEBUG_LEVEL_VERBOSE, debug_level, "      asap pipelined op new starting time = " + STR(starting_time[candidate_v]));
      PRINT_DBG_MEX(DEBUG_LEVEL_VERBOSE, debug_level, "      asap pipelined op same ending time   = " + STR(ending_time[candidate_v]));
   }
}

bool parametric_list_based::store_in_chaining_with_load_in(unsigned int current_vertex_cstep, vertex v)
{
   const auto operation = flow_graph->CGetOpNodeInfo(v)->GetOperation();
   bool is_load_store = GET_TYPE(flow_graph, v) & (TYPE_LOAD | TYPE_STORE);
   if(not is_load_store)
      return false;
   std::queue<vertex> fifo;
   InEdgeIterator eo, eo_end;
   for(boost::tie(eo, eo_end) = boost::in_edges(v, *flow_graph); eo != eo_end; eo++)
      fifo.push(boost::source(*eo, *flow_graph));
   while(!fifo.empty())
   {
      vertex current_op = fifo.front();
      fifo.pop();
      if(current_vertex_cstep == static_cast<unsigned int>(floor(ending_time.find(current_op)->second / clock_cycle)))
      {
         if(GET_TYPE(flow_graph, current_op) & (TYPE_LOAD | TYPE_STORE))
            return true;
         for(boost::tie(eo, eo_end) = boost::in_edges(current_op, *flow_graph); eo != eo_end; eo++)
            fifo.push(boost::source(*eo, *flow_graph));
      }
   }
   return false;
}

bool parametric_list_based::store_in_chaining_with_load_out(unsigned int current_vertex_cstep, vertex v)
{
   const auto operation = flow_graph->CGetOpNodeInfo(v)->GetOperation();
   bool is_load_store = GET_TYPE(flow_graph, v) & (TYPE_LOAD | TYPE_STORE);
   if(not is_load_store)
      return false;
   std::queue<vertex> fifo;
   OutEdgeIterator eo, eo_end;
   for(boost::tie(eo, eo_end) = boost::out_edges(v, *flow_graph); eo != eo_end; eo++)
      fifo.push(boost::source(*eo, *flow_graph));
   while(!fifo.empty())
   {
      vertex current_op = fifo.front();
      fifo.pop();
      if(current_vertex_cstep == static_cast<unsigned int>(floor(starting_time(current_op) / clock_cycle)))
      {
         if(GET_TYPE(flow_graph, current_op) & (TYPE_LOAD | TYPE_STORE))
            return true;
         for(boost::tie(eo, eo_end) = boost::out_edges(current_op, *flow_graph); eo != eo_end; eo++)
            fifo.push(boost::source(*eo, *flow_graph));
      }
   }
   return false;
}

void parametric_list_based::do_balanced_scheduling(std::deque<vertex>& sub_levels, const ScheduleRef schedule, const fu_bindingRef res_binding, const double setup_hold_time, const double scheduling_mux_margins, const OpGraphConstRef opDFG)
{
   /// balanced scheduling
   /// step 1: dependencies computation
   compare_vertex_by_name vertex_by_name(opDFG);
   std::map<vertex, OpVertexSet, compare_vertex_by_name> DMAP(vertex_by_name);
   std::map<vertex, size_t, compare_vertex_by_name> D(vertex_by_name);
   for(std::deque<vertex>::const_reverse_iterator rit = sub_levels.rbegin(); rit != sub_levels.rend(); ++rit)
   {
      vertex candidate_v = *rit;
      InEdgeIterator ei, ei_end;
      for(boost::tie(ei, ei_end) = boost::in_edges(candidate_v, *opDFG); ei != ei_end; ++ei)
      {
         vertex src = boost::source(*ei, *opDFG);
         auto insertResult = DMAP.insert(std::make_pair(candidate_v, OpVertexSet(opDFG)));
         if(DMAP.find(src) != DMAP.end())
            insertResult.first->second.insert(DMAP.find(src)->second.begin(), DMAP.find(src)->second.end());
         insertResult.first->second.insert(src);
      }
      D[candidate_v] = DMAP.find(candidate_v)->second.size();
   }
   /// step 2: compute operation distribution
   std::map<ControlStep, std::deque<vertex>> T;
   std::map<ControlStep, double> T_area;
   ControlStep min_cycle = ControlStep(std::numeric_limits<unsigned int>::max());
   ControlStep max_cycle = ControlStep(0u);
   double total_resource_area = 0;
   for(std::deque<vertex>::const_iterator vi = sub_levels.begin(); vi != sub_levels.end(); ++vi)
   {
      vertex current_op = *vi;
      const auto curr_cs = schedule->get_cstep(current_op).second;
      min_cycle = std::min(min_cycle, curr_cs);
      max_cycle = std::max(max_cycle, curr_cs);
      T[curr_cs].push_back(current_op);
      unsigned int fu_type = res_binding->get_assign(current_op);
      double curr_area = HLS->allocation_information->get_area(fu_type) + 100 * HLS->allocation_information->get_DSPs(fu_type);
      if(T_area.find(curr_cs) == T_area.end())
         T_area[curr_cs] = curr_area;
      else
         T_area[curr_cs] += curr_area;
      total_resource_area += curr_area;
   }
   const auto cycles = max_cycle - min_cycle + 1u;
   double average_load = total_resource_area / from_strongtype_cast<double>(cycles); // static_cast<double>(operations.size())/cycles;//std::numeric_limits<double>::max();//
   const auto t_it_end = T.rend();
   for(auto t_it = T.rbegin(); t_it != t_it_end; ++t_it)
   {
      ControlStep time = t_it->first;
      if(time == 0)
         continue;
      if(static_cast<double>(T[time].size()) >= average_load)
         continue;
      auto ptime = time - 1u;
      /// check if time has unbounded vertices or the return statement
      bool has_unbounded = false;
      bool has_return = false;
      std::deque<vertex>::const_iterator time_it_end = T.find(time)->second.end();
      for(std::deque<vertex>::const_iterator time_it = T.find(time)->second.begin(); !has_unbounded && !has_return && time_it != time_it_end; ++time_it)
      {
         vertex candidate_v = *time_it;
         unsigned int fu_type = res_binding->get_assign(candidate_v);
         if(!HLS->allocation_information->is_operation_bounded(opDFG, candidate_v, fu_type))
            has_unbounded = true;
         if(GET_TYPE(opDFG, candidate_v) & TYPE_RET)
            has_return = true;
      }
      if(has_return)
         continue;

      while(T_area[time] < average_load && ptime >= min_cycle)
      {
         while(T.find(ptime) != T.end())
         {
            double best_starting_time = 0.0;
            double best_ending_time = 0.0;
            double best_starting_area = 0;
            size_t max_dependencies = 0;
            vertex best_node = NULL_VERTEX;
            std::deque<vertex>::const_iterator ptime_it_end = T.find(ptime)->second.end();
            for(std::deque<vertex>::const_iterator ptime_it = T.find(ptime)->second.begin(); ptime_it != ptime_it_end; ++ptime_it)
            {
               vertex candidate_v = *ptime_it;
               unsigned int fu_type = res_binding->get_assign(candidate_v);
               if(HLS->allocation_information->get_number_fu(fu_type) != INFINITE_UINT && !HLS->allocation_information->is_vertex_bounded(fu_type))
                  continue;
               /// only 1 unbounded operation is allowed in a control step
               if(!HLS->allocation_information->is_operation_bounded(opDFG, candidate_v, fu_type))
                  continue; /// unbounded operations could not be moved
               if(GET_TYPE(opDFG, candidate_v) & (TYPE_PHI | TYPE_VPHI | TYPE_INIT | TYPE_LABEL | TYPE_LOAD | TYPE_STORE))
                  continue;

               double candidate_starting_time;
               double max_ending_time;
               double candidate_stage_period;
               double candidate_op_execution_time;
               bool forward_node_p = true;
               bool cannot_be_chained = false;
               unsigned int n_cycles;
               double phi_extra_time;
               CustomMap<std::pair<unsigned int, unsigned int>, double> local_connection_map;
               update_starting_ending_time(candidate_v, res_binding, opDFG, schedule);
               compute_starting_ending_time_alap(candidate_v, fu_type, time, candidate_starting_time, max_ending_time, candidate_op_execution_time, candidate_stage_period, n_cycles, cannot_be_chained, res_binding, schedule, phi_extra_time, setup_hold_time,
                                                 local_connection_map);

               if((candidate_starting_time > (from_strongtype_cast<double>(time) * clock_cycle)) && !parameters->getOption<bool>(OPT_chaining))
                  continue; /// Chaining is possible but not allowed

               if((candidate_starting_time > (from_strongtype_cast<double>(time) * clock_cycle)) && cannot_be_chained)
                  continue; /// Chaining with STOREs/LOADs is not possible

               bool is_pipelined = HLS->allocation_information->get_initiation_time(fu_type, candidate_v) != 0;
               if(is_pipelined && (candidate_starting_time + setup_hold_time + phi_extra_time + scheduling_mux_margins + candidate_stage_period) > ((from_strongtype_cast<double>(time) + 1) * clock_cycle))
                  continue; /// pipelined operation does not fit at the begin

               if(is_pipelined && (max_ending_time - setup_hold_time - phi_extra_time - scheduling_mux_margins) < (from_strongtype_cast<double>(time + n_cycles - 1) * clock_cycle + candidate_stage_period))
                  continue; /// pipelined operation does not fit at the end

               if(!is_pipelined && (candidate_op_execution_time + setup_hold_time + phi_extra_time + scheduling_mux_margins) > clock_cycle && has_unbounded)
                  continue; /// Multi-cycles operations cannot be scheduled together with unbounded operations

               if(!is_pipelined && candidate_starting_time + setup_hold_time + phi_extra_time + scheduling_mux_margins + candidate_op_execution_time > max_ending_time)
                  continue; /// operation does not fit at the end

               if(!is_pipelined && setup_hold_time + phi_extra_time + candidate_op_execution_time < clock_cycle &&
                  setup_hold_time + phi_extra_time + scheduling_mux_margins + candidate_starting_time + candidate_op_execution_time > (from_strongtype_cast<double>(time) + 1) * clock_cycle)
                  continue; /// operation does not fit in the cycle

               // if(store_in_chaining_with_load_out(time,candidate_v)) continue; /// store and load cannot be chained
               // if(candidate_ending_time < (time + 1) * clock_cycle) continue; /// avoid chaining as much as possible

               double candidate_ending_time = candidate_starting_time + candidate_op_execution_time;

               /// try to keep the previous latency
               if(candidate_ending_time > (from_strongtype_cast<double>(max_cycle) + 1) * clock_cycle)
                  continue;

               OutEdgeIterator eo, eo_end;
               for(boost::tie(eo, eo_end) = boost::out_edges(candidate_v, *opDFG); forward_node_p && eo != eo_end; eo++)
               {
                  vertex target = boost::target(*eo, *opDFG);
                  if(candidate_ending_time > starting_time[target])
                     forward_node_p = false;
               }
               if(!forward_node_p)
                  continue;

               if(best_node == NULL_VERTEX || D[candidate_v] > max_dependencies || (D[candidate_v] == max_dependencies && best_starting_area < T_area[ptime]))
               {
                  max_dependencies = D[candidate_v];
                  best_node = candidate_v;
                  best_starting_time = candidate_starting_time;
                  best_ending_time = candidate_ending_time;
                  best_starting_area = T_area[ptime];
               }
            }
            if(best_node != NULL_VERTEX)
            {
               schedule->set_execution(best_node, time);
               starting_time[best_node] = best_starting_time;
               ending_time[best_node] = best_ending_time;
               schedule->starting_times[flow_graph->CGetOpNodeInfo(best_node)->GetNodeId()] = best_starting_time;
               schedule->ending_times[flow_graph->CGetOpNodeInfo(best_node)->GetNodeId()] = best_ending_time;
               THROW_ASSERT(ending_time[best_node] >= starting_time[best_node], "unexpected condition");
               THROW_ASSERT(floor(ending_time[best_node] / clock_cycle) >= from_strongtype_cast<double>(time), "unexpected condition");
               schedule->set_execution_end(best_node, time + ControlStep(static_cast<unsigned int>(floor(ending_time[best_node] / clock_cycle) - from_strongtype_cast<double>(time))));
               update_starting_ending_time(best_node, res_binding, opDFG, schedule);
               T[ptime].erase(std::find(T[ptime].begin(), T[ptime].end(), best_node));
               T[time].push_back(best_node);
               unsigned int fu_type = res_binding->get_assign(best_node);
               double curr_area = HLS->allocation_information->get_area(fu_type) + 100 * HLS->allocation_information->get_DSPs(fu_type);
               T_area[time] += curr_area;
               T_area[ptime] -= curr_area;
               InEdgeIterator ei, ei_end;
               for(boost::tie(ei, ei_end) = boost::in_edges(best_node, *opDFG); ei != ei_end; ++ei)
               {
                  vertex from_vertex = boost::source(*ei, *opDFG);
                  update_starting_ending_time(from_vertex, res_binding, opDFG, schedule);
               }
            }
            else
               break;
            if(T_area[time] >= average_load)
               break;
         }
         ptime = ptime - 1;
      }
   }
   OpVertexSet vertices_analyzed(opDFG);
   for(std::deque<vertex>::const_iterator vi = sub_levels.begin(); vi != sub_levels.end(); ++vi)
   {
      if(vertices_analyzed.find(*vi) != vertices_analyzed.end())
      {
         continue;
      }
      update_vertices_slack(*vi, schedule, vertices_analyzed, setup_hold_time, opDFG);
   }
}

void parametric_list_based::do_balanced_scheduling1(std::deque<vertex>& sub_levels, const ScheduleRef schedule, const fu_bindingRef res_binding, const double setup_hold_time, const double scheduling_mux_margins, const OpGraphConstRef opDFG,
                                                    bool seen_cstep_has_RET_conflict)
{
   //#define SLACK_BASED
   PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "do_balanced_scheduling1");

   PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "  dependencies computation");
   /// balanced scheduling
   /// step 1: dependencies computation
   compare_vertex_by_name vertex_by_name(opDFG);
   std::map<vertex, OpVertexSet, compare_vertex_by_name> DMAP(vertex_by_name);
   std::map<vertex, size_t, compare_vertex_by_name> D(vertex_by_name);
   for(std::deque<vertex>::const_reverse_iterator rit = sub_levels.rbegin(); rit != sub_levels.rend(); ++rit)
   {
      vertex candidate_v = *rit;
      InEdgeIterator ei, ei_end;
      for(boost::tie(ei, ei_end) = boost::in_edges(candidate_v, *opDFG); ei != ei_end; ++ei)
      {
         vertex src = boost::source(*ei, *opDFG);
         auto insertResult = DMAP.insert(std::make_pair(candidate_v, OpVertexSet(opDFG)));
         if(DMAP.find(src) != DMAP.end())
            insertResult.first->second.insert(DMAP.find(src)->second.begin(), DMAP.find(src)->second.end());
         insertResult.first->second.insert(src);
      }
      D[candidate_v] = DMAP.find(candidate_v)->second.size();
   }

   PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "  compute operation distribution");
   /// step 2: compute operation distribution
   std::map<ControlStep, std::deque<vertex>> T;

   std::map<unsigned int, double> total_obj;
   std::map<unsigned int, std::map<ControlStep, double>> T_obj;
   ControlStep min_cycle = ControlStep(std::numeric_limits<unsigned int>::max());
   ControlStep max_cycle = ControlStep(0u);
   for(std::deque<vertex>::const_iterator vi = sub_levels.begin(); vi != sub_levels.end(); ++vi)
   {
      vertex current_op = *vi;
      const auto curr_cs = schedule->get_cstep(current_op).second;
      min_cycle = std::min(min_cycle, curr_cs);
      max_cycle = std::max(max_cycle, curr_cs);
      T[curr_cs].push_back(current_op);
      unsigned int fu_type = res_binding->get_assign(current_op);
#ifdef SLACK_BASED
      if(T_obj.find(fu_type) == T_obj.end())
         T_obj[fu_type][curr_cs] = schedule->get_slack(current_op);
      if(T_obj.find(fu_type)->second.find(curr_cs) == T_obj.find(fu_type)->second.end())
         T_obj[fu_type][curr_cs] = schedule->get_slack(current_op);
      else
         T_obj[fu_type][curr_cs] = std::min(schedule->get_slack(current_op), T_obj[fu_type][curr_cs]);
#else
      double curr_area = 1;
      if(T_obj.find(fu_type) == T_obj.end())
         T_obj[fu_type][curr_cs] = curr_area;
      if(T_obj.find(fu_type)->second.find(curr_cs) == T_obj.find(fu_type)->second.end())
         T_obj[fu_type][curr_cs] = curr_area;
      else
         T_obj[fu_type][curr_cs] += curr_area;

      if(total_obj.find(fu_type) == total_obj.end())
         total_obj[fu_type] = curr_area;
      else
         total_obj[fu_type] += curr_area;
#endif
   }
#ifdef SLACK_BASED
   for(std::map<unsigned int, std::map<unsigned int, double>>::const_iterator to_it = T_obj.begin(); to_it != T_obj.end(); ++to_it)
   {
      total_obj[to_it->first] = 0;
      for(std::map<unsigned int, double>::const_iterator si = to_it->second.begin(); si != to_it->second.end(); ++si)
      {
         total_obj[to_it->first] += si->second;
      }
   }
   OpVertexSet vertices_analyzed(opDFG);
#endif
   PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "  let's start with the re-scheduling");
   ControlStep cycles = max_cycle - min_cycle + 1u;
   std::map<ControlStep, std::deque<vertex>>::const_reverse_iterator t_it_end = T.rend();
   for(std::map<ControlStep, std::deque<vertex>>::const_reverse_iterator t_it = T.rbegin(); t_it != t_it_end; ++t_it)
   {
      const auto time = t_it->first;
      const auto time_clock_cycle_starting = from_strongtype_cast<double>(time) * clock_cycle;
      const auto time_clock_cycle_ending = (from_strongtype_cast<double>(time) + 1) * clock_cycle;
      if(time == min_cycle)
         continue;
      ControlStep ptime = time - 1u;
      /// check if time has unbounded vertices, the return or the IF/SWITCH statements
      bool has_unbounded = false;
      std::deque<vertex>::const_iterator time_it_end = T.find(time)->second.end();
      for(std::deque<vertex>::const_iterator time_it = T.find(time)->second.begin(); !has_unbounded && time_it != time_it_end; ++time_it)
      {
         vertex candidate_v = *time_it;
         unsigned int fu_type = res_binding->get_assign(candidate_v);
         if(!HLS->allocation_information->is_operation_bounded(opDFG, candidate_v, fu_type))
            has_unbounded = true;
      }
      if(seen_cstep_has_RET_conflict)
      {
         /// recompute the number of cycles removing the return cycle
         --cycles;
         continue;
      }

      while(ptime >= min_cycle)
      {
         while(T.find(ptime) != T.end())
         {
            double best_starting_time = 0.0;
            double best_ending_time = 0.0;
#ifdef SLACK_BASED
            double best_objective = std::numeric_limits<double>::max();
            double best_candidate_slack = 0;
#else
            double best_objective = 0;
#endif
            size_t max_dependencies = 0;
            vertex best_node = NULL_VERTEX;
            std::deque<vertex>::const_iterator ptime_it_end = T.find(ptime)->second.end();
            for(std::deque<vertex>::const_iterator ptime_it = T.find(ptime)->second.begin(); ptime_it != ptime_it_end; ++ptime_it)
            {
               vertex candidate_v = *ptime_it;
               unsigned int fu_type = res_binding->get_assign(candidate_v);
               if(HLS->allocation_information->get_number_fu(fu_type) != INFINITE_UINT && !HLS->allocation_information->is_vertex_bounded(fu_type))
                  continue;
               /// only 1 unbounded operation is allowed in a control step
               if(!HLS->allocation_information->is_operation_bounded(opDFG, candidate_v, fu_type))
                  continue; /// unbounded operations could not be moved
               if(GET_TYPE(opDFG, candidate_v) & (TYPE_PHI | TYPE_VPHI | TYPE_INIT | TYPE_LABEL | TYPE_LOAD | TYPE_STORE))
                  continue;

               double candidate_starting_time;
               double max_ending_time;
               double candidate_stage_period;
               double candidate_op_execution_time;
               bool forward_node_p = true;
               bool cannot_be_chained = false;
               unsigned int n_cycles;
               double phi_extra_time;
               double normalized_area = HLS->allocation_information->compute_normalized_area(fu_type);
               CustomMap<std::pair<unsigned int, unsigned int>, double> local_connection_map;
               compute_starting_ending_time_alap(candidate_v, fu_type, time, candidate_starting_time, max_ending_time, candidate_op_execution_time, candidate_stage_period, n_cycles, cannot_be_chained, res_binding, schedule, phi_extra_time, setup_hold_time,
                                                 local_connection_map);

               if(T_obj.find(fu_type) != T_obj.end() && T_obj.find(fu_type)->second.find(time) != T_obj.find(fu_type)->second.end() &&
#ifdef SLACK_BASED
                  T_obj[fu_type][time] < (total_obj[fu_type] / cycles /*static_cast<double>(T_obj[fu_type].size())*/) //
#else
                  T_obj[fu_type][time] >= (total_obj[fu_type] / from_strongtype_cast<double>(cycles) /*static_cast<double>(T_obj[fu_type].size())*/) //
                  && normalized_area > 1.5
#endif
               )
               {
                  PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                "   no room for fu_type at time time " + HLS->allocation_information->get_fu_name(fu_type).first + " obj=" + STR(T_obj[fu_type][time]) + " tot_obj=" + STR(total_obj[fu_type] / from_strongtype_cast<double>(cycles)) +
                                    " cycles=" + STR(cycles) + " normalized_area=" + STR(normalized_area));
                  continue; /// no room for fu_type at time time
               }

               if((candidate_starting_time > time_clock_cycle_starting) && !parameters->getOption<bool>(OPT_chaining))
               {
                  PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "   Chaining is possible but not allowed");
                  continue; /// Chaining is possible but not allowed
               }

               if((candidate_starting_time > time_clock_cycle_starting) && cannot_be_chained)
               {
                  PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "   Chaining with STOREs/LOADs is not possible");
                  continue; /// Chaining with STOREs/LOADs is not possible
               }

               bool is_pipelined = HLS->allocation_information->get_initiation_time(fu_type, candidate_v) != 0;
               if(is_pipelined && (candidate_starting_time + setup_hold_time + phi_extra_time + scheduling_mux_margins + candidate_stage_period) > time_clock_cycle_ending)
               {
                  PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "   pipelined operation does not fit at the begin");
                  continue; /// pipelined operation does not fit at the begin
               }

               if(is_pipelined && (max_ending_time - setup_hold_time - phi_extra_time - scheduling_mux_margins) < ((from_strongtype_cast<double>(time) + n_cycles - 1) * clock_cycle + candidate_stage_period))
               {
                  PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "   pipelined operation does not fit at the end");
                  continue; /// pipelined operation does not fit at the end
               }

               if(!is_pipelined && (candidate_op_execution_time + setup_hold_time + phi_extra_time + scheduling_mux_margins) > clock_cycle && has_unbounded)
               {
                  PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "   Multi-cycles operations cannot be scheduled together with unbounded operations");
                  continue; /// Multi-cycles operations cannot be scheduled together with unbounded operations
               }

               if(!is_pipelined && candidate_starting_time + setup_hold_time + phi_extra_time + scheduling_mux_margins + candidate_op_execution_time > max_ending_time)
               {
                  PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "   operation does not fit at the end");
                  continue; /// operation does not fit at the end
               }

               if(!is_pipelined && setup_hold_time + phi_extra_time + scheduling_mux_margins + candidate_op_execution_time < clock_cycle &&
                  setup_hold_time + phi_extra_time + scheduling_mux_margins + candidate_starting_time + candidate_op_execution_time > time_clock_cycle_ending)
               {
                  PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "   operation does not fit in the cycle");
                  continue; /// operation does not fit in the cycle
               }

               // if(store_in_chaining_with_load_out(time,candidate_v)) continue; /// store and load cannot be chained
               // if(candidate_ending_time < (time + 1) * clock_cycle) continue; /// avoid chaining as much as possible

               double candidate_ending_time = candidate_starting_time + candidate_op_execution_time;

               /// try to keep the previous latency
               if(candidate_ending_time > (from_strongtype_cast<double>(max_cycle) + 1) * clock_cycle)
               {
                  PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "  break previous latency " + HLS->allocation_information->get_fu_name(fu_type).first);
                  continue;
               }

               /**/ if((candidate_ending_time < time_clock_cycle_ending - EPSILON) && (candidate_starting_time - time_clock_cycle_starting < clock_cycle / 3))
               {
                  continue; /// try to reduce as much as possible the chaining between operations
               }            /**/

#ifdef SLACK_BASED
               double candidate_slack = candidate_starting_time - time * clock_cycle + (HLS->HLS_C->get_clock_period() - clock_cycle);
               if(candidate_slack <= total_obj[fu_type] / cycles /*static_cast<double>(T_obj[fu_type].size())*/)
                  continue; /// no room for fu_type at time time
#else
               double curr_area = 1;
               if((T_obj[fu_type][time] + curr_area) > ceil(total_obj[fu_type] / from_strongtype_cast<double>(cycles)) && normalized_area > 1.5)
               {
                  PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                "  No room for fu_type at time time " + HLS->allocation_information->get_fu_name(fu_type).first + " obj=" + STR(T_obj[fu_type][time] + curr_area) +
                                    " tot_obj=" + STR((total_obj[fu_type] / from_strongtype_cast<double>(cycles))) + " normalized_area=" + STR(normalized_area));
                  continue; /// no room for fu_type at time time
               }
#endif

               OutEdgeIterator eo, eo_end;
               for(boost::tie(eo, eo_end) = boost::out_edges(candidate_v, *opDFG); forward_node_p && eo != eo_end; eo++)
               {
                  vertex target = boost::target(*eo, *opDFG);
                  if(candidate_ending_time > starting_time[target])
                     forward_node_p = false;
               }
               if(!forward_node_p)
               {
                  PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "  forwarding problem " + GET_NAME(opDFG, candidate_v));
                  continue;
               }

               PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                             "  candidate ending time=" + STR(candidate_ending_time) + " candidate starting time=" + STR(candidate_starting_time) + " functional unit " + HLS->allocation_information->get_fu_name(fu_type).first);

               if(best_node == NULL_VERTEX || D[candidate_v] > max_dependencies ||
                  (D[candidate_v] == max_dependencies &&
#ifdef SLACK_BASED
                   (best_objective > schedule->get_slack(candidate_v) || (best_objective == schedule->get_slack(candidate_v) && best_candidate_slack < candidate_slack))
#else
                   best_objective < 1
#endif
                       ))
               {
                  max_dependencies = D[candidate_v];
                  best_node = candidate_v;
                  best_starting_time = candidate_starting_time;
                  best_ending_time = candidate_ending_time;
#ifdef SLACK_BASED
                  best_objective = schedule->get_slack(candidate_v);
                  best_candidate_slack = candidate_slack;
#else
                  best_objective = 1;
#endif
               }
            }
            if(best_node != NULL_VERTEX)
            {
               schedule->set_execution(best_node, time);
               starting_time[best_node] = best_starting_time;
               ending_time[best_node] = best_ending_time;
               schedule->starting_times[flow_graph->CGetOpNodeInfo(best_node)->GetNodeId()] = best_starting_time;
               schedule->ending_times[flow_graph->CGetOpNodeInfo(best_node)->GetNodeId()] = best_ending_time;
               THROW_ASSERT(ending_time[best_node] >= starting_time[best_node], "unexpected condition");
               THROW_ASSERT(floor(ending_time[best_node] / clock_cycle) >= from_strongtype_cast<double>(time), "unexpected condition");
               schedule->set_execution_end(best_node, time + ControlStep(static_cast<unsigned int>(floor(ending_time[best_node] / clock_cycle) - from_strongtype_cast<double>(time))));
               PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "                  " + GET_NAME(opDFG, best_node) + " re-scheduled at " + STR(time_clock_cycle_starting));
               update_starting_ending_time(best_node, res_binding, opDFG, schedule);

               T[ptime].erase(std::find(T[ptime].begin(), T[ptime].end(), best_node));
               T[time].push_back(best_node);
#ifdef SLACK_BASED
               std::list<vertex> vertices_slack_analyzed;
               update_vertices_timing(ptime, best_node, LB, vertices_slack_analyzed, res_binding, opDFG);
               vertices_analyzed.clear();
               for(std::list<vertex>::const_iterator va_it = vertices_slack_analyzed.begin(); va_it != vertices_slack_analyzed.end(); ++va_it)
               {
                  if(vertices_analyzed.find(*va_it) != vertices_analyzed.end())
                  {
                     continue;
                  }
                  update_vertices_slack(*va_it, schedule, vertices_analyzed, setup_hold_time, opDFG);
               }
#endif

               unsigned int fu_type = res_binding->get_assign(best_node);
#ifdef SLACK_BASED
               if(T_obj.find(fu_type)->second.find(time) != T_obj.find(fu_type)->second.end() && T_obj[fu_type][time] != std::numeric_limits<double>::max())
                  total_obj[fu_type] -= T_obj[fu_type][time];
               T_obj[fu_type][time] = std::numeric_limits<double>::max();
               const std::deque<vertex>::const_iterator utime_it_end = T.find(time)->second.end();
               for(std::deque<vertex>::const_iterator utime_it = T.find(time)->second.begin(); utime_it != utime_it_end; ++utime_it)
               {
                  unsigned int cur_fu_type = res_binding->get_assign(*utime_it);
                  if(cur_fu_type == fu_type)
                     T_obj[fu_type][time] = std::min(schedule->get_slack(*utime_it), T_obj[fu_type][time]);
               }

               if(T_obj[fu_type][time] != std::numeric_limits<double>::max())
                  total_obj[fu_type] += T_obj[fu_type][time];
               else
                  T_obj[fu_type].erase(time);

               total_obj[fu_type] -= T_obj[fu_type][ptime];
               T_obj[fu_type][ptime] = std::numeric_limits<double>::max();
               const std::deque<vertex>::const_iterator uptime_it_end = T.find(ptime)->second.end();
               for(std::deque<vertex>::const_iterator uptime_it = T.find(ptime)->second.begin(); uptime_it != uptime_it_end; ++uptime_it)
               {
                  unsigned int cur_fu_type = res_binding->get_assign(*uptime_it);
                  if(cur_fu_type == fu_type)
                     T_obj[fu_type][ptime] = std::min(schedule->get_slack(*uptime_it), T_obj[fu_type][ptime]);
               }
               if(T_obj[fu_type][ptime] != std::numeric_limits<double>::max())
                  total_obj[fu_type] += T_obj[fu_type][ptime];
               else
                  T_obj[fu_type].erase(ptime);
#else
               double curr_area = 1;
               T_obj[fu_type][time] += curr_area;
               T_obj[fu_type][ptime] -= curr_area;
#endif
            }
            else
               break;
         }
         ptime = ptime - 1;
      }
   }
#ifdef SLACK_BASED
   vertices_analyzed.clear();
   for(std::deque<vertex>::const_iterator vi = sub_levels.begin(); vi != sub_levels.end(); ++vi)
   {
      if(vertices_analyzed.find(*vi) != vertices_analyzed.end())
      {
         continue;
      }
      update_vertices_slack(*vi, schedule, vertices_analyzed, setup_hold_time, opDFG);
   }
#endif
}

#define REMOVE_DIRECT_TO_INDIRECT 1
bool parametric_list_based::check_non_direct_operation_chaining(vertex current_v, unsigned int v_fu_type, const ControlStep cs, const ScheduleConstRef schedule, fu_bindingRef res_binding) const
{
   bool v_is_indirect = REMOVE_DIRECT_TO_INDIRECT && HLS->allocation_information->is_indirect_access_memory_unit(v_fu_type);
   bool v_is_one_cycle_direct_access = (HLS->allocation_information->is_one_cycle_direct_access_memory_unit(v_fu_type) &&
                                        (!HLS->allocation_information->is_readonly_memory_unit(v_fu_type) || (!HLS->Param->isOption(OPT_rom_duplication) || !parameters->getOption<bool>(OPT_rom_duplication)))) &&
                                       HLSMgr->Rmem->get_maximum_references(HLS->allocation_information->is_memory_unit(v_fu_type) ? HLS->allocation_information->get_memory_var(v_fu_type) : HLS->allocation_information->get_proxy_memory_var(v_fu_type)) >
                                           HLS->allocation_information->get_number_channels(v_fu_type);

   /// Set of already analyzed operations
   OpVertexSet already_analyzed_operations(flow_graph);

   /// compute the set of operations on the control step frontier
   OpVertexSet to_be_analyzed(flow_graph);
   InEdgeIterator ie, ie_end;
   for(boost::tie(ie, ie_end) = boost::in_edges(current_v, *flow_graph); ie != ie_end; ie++)
      to_be_analyzed.insert(boost::source(*ie, *flow_graph));
   while(!to_be_analyzed.empty())
   {
      vertex current_op = *(to_be_analyzed.begin());
      to_be_analyzed.erase(to_be_analyzed.begin());
      already_analyzed_operations.insert(current_op);
      if(cs == schedule->get_cstep_end(current_op).second)
      {
         unsigned int from_fu_type = res_binding->get_assign(current_op);
         if((GET_TYPE(flow_graph, current_op) & TYPE_LOAD) and
            (v_is_indirect or (v_is_one_cycle_direct_access and HLS->allocation_information->is_one_cycle_direct_access_memory_unit(from_fu_type)) or HLS->allocation_information->is_indirect_access_memory_unit(from_fu_type)))
         {
            return true;
         }
         for(boost::tie(ie, ie_end) = boost::in_edges(current_op, *flow_graph); ie != ie_end; ie++)
         {
            const auto source = boost::source(*ie, *flow_graph);
            if(already_analyzed_operations.find(source) == already_analyzed_operations.end())
            {
               to_be_analyzed.insert(source);
            }
         }
      }
   }
   return false;
}

bool parametric_list_based::check_direct_operation_chaining(vertex current_v, const ControlStep cs, const ScheduleConstRef schedule, fu_bindingRef res_binding) const
{
   /// compute the set of operations on the control step frontier
   std::queue<vertex> fifo;
   InEdgeIterator eo, eo_end;
   for(boost::tie(eo, eo_end) = boost::in_edges(current_v, *flow_graph); eo != eo_end; eo++)
      fifo.push(boost::source(*eo, *flow_graph));
   while(!fifo.empty())
   {
      vertex current_op = fifo.front();
      fifo.pop();
      if(cs == schedule->get_cstep_end(current_op).second)
      {
         unsigned int from_fu_type = res_binding->get_assign(current_op);
         if((GET_TYPE(flow_graph, current_op) & TYPE_LOAD) && HLS->allocation_information->is_direct_access_memory_unit(from_fu_type))
            return true;
         for(boost::tie(eo, eo_end) = boost::in_edges(current_op, *flow_graph); eo != eo_end; eo++)
            fifo.push(boost::source(*eo, *flow_graph));
      }
   }
   return false;
}

bool parametric_list_based::check_LOAD_chaining(vertex current_v, const ControlStep cs, const ScheduleConstRef schedule) const
{
   /// compute the set of operations on the control step frontier
   std::queue<vertex> fifo;
   InEdgeIterator eo, eo_end;
   for(boost::tie(eo, eo_end) = boost::in_edges(current_v, *flow_graph); eo != eo_end; eo++)
      fifo.push(boost::source(*eo, *flow_graph));
   while(!fifo.empty())
   {
      vertex current_op = fifo.front();
      fifo.pop();
      if(cs == schedule->get_cstep_end(current_op).second)
      {
         if(GET_TYPE(flow_graph, current_op) & TYPE_LOAD)
         {
            return true;
         }
         for(boost::tie(eo, eo_end) = boost::in_edges(current_op, *flow_graph); eo != eo_end; eo++)
            fifo.push(boost::source(*eo, *flow_graph));
      }
   }
   return false;
}

const CustomUnorderedSet<std::tuple<HLSFlowStep_Type, HLSFlowStepSpecializationConstRef, HLSFlowStep_Relationship>> parametric_list_based::ComputeHLSRelationships(const DesignFlowStep::RelationshipType relationship_type) const
{
   CustomUnorderedSet<std::tuple<HLSFlowStep_Type, HLSFlowStepSpecializationConstRef, HLSFlowStep_Relationship>> ret = Scheduling::ComputeHLSRelationships(relationship_type);
   switch(relationship_type)
   {
      case DEPENDENCE_RELATIONSHIP:
      {
#if HAVE_ILP_BUILT
         if(parameters->getOption<HLSFlowStep_Type>(OPT_scheduling_algorithm) == HLSFlowStep_Type::SDC_SCHEDULING
#if HAVE_FROM_PRAGMA_BUILT && HAVE_BAMBU_BUILT
            and HLSMgr->get_HLS(funId) and HLSMgr->CGetFunctionBehavior(funId)->CGetBehavioralHelper()->GetOmpForDegree() == 0
#endif
         )
         {
            ret.insert(std::make_tuple(HLSFlowStep_Type::SDC_SCHEDULING, HLSFlowStepSpecializationConstRef(), HLSFlowStep_Relationship::SAME_FUNCTION));
         }
#endif
         break;
      }
      case INVALIDATION_RELATIONSHIP:
      {
         break;
      }
      case PRECEDENCE_RELATIONSHIP:
      {
         break;
      }
      default:
         THROW_UNREACHABLE("");
   }
   return ret;
}
