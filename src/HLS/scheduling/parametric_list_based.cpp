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

#include "ASLAP.hpp"
#include "Parameter.hpp"
#include "allocation.hpp"
#include "allocation_information.hpp"
#include "basic_block.hpp"
#include "behavioral_helper.hpp"
#include "call_graph_manager.hpp"
#include "cpu_time.hpp"
#include "design_flow_graph.hpp"
#include "design_flow_manager.hpp"
#include "exceptions.hpp"
#include "fileIO.hpp"
#include "fu_binding.hpp"
#include "function_behavior.hpp"
#include "function_frontend_flow_step.hpp"
#include "functions.hpp"
#include "hls.hpp"
#include "hls_constraints.hpp"
#include "memory.hpp"
#include "schedule.hpp"
#include "string_manipulation.hpp"
#include "structural_objects.hpp"
#include "technology_node.hpp"
#include "tree_basic_block.hpp"
#include "tree_helper.hpp"
#include "tree_manager.hpp"
#include "tree_node.hpp"
#include "utility.hpp"
#include "xml_document.hpp"

#include <utility>

#if !HAVE_UNORDERED
PrioritySorter::PrioritySorter(refcount<priority_data<int>> _priority, const OpGraphConstRef _op_graph)
    : priority(std::move(_priority)), op_graph(_op_graph)
{
}

bool PrioritySorter::operator()(const vertex x, const vertex y) const
{
   const auto x_priority = (*priority)(x);
   const auto y_priority = (*priority)(y);
   if(x_priority != y_priority)
   {
      return x_priority > y_priority;
   }
   return op_graph->CGetOpNodeInfo(x)->vertex_name < op_graph->CGetOpNodeInfo(y)->vertex_name;
}
#endif

/**
 * Functor used to compare two vertices with respect to an order based on the control steps associated with the
 * vertices.
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
      return order.find(a)->second < order.find(b)->second ||
             (order.find(a)->second == order.find(b)->second &&
              op_graph->CGetOpNodeInfo(a)->vertex_name < op_graph->CGetOpNodeInfo(b)->vertex_name);
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
      {
         aFunName = aFunName.substr(prefix.length());
      }
      if(bFunName.find(prefix) != std::string::npos)
      {
         bFunName = bFunName.substr(prefix.length());
      }

      return ((wm_a > wm_b) || (wm_a == wm_b && we_a < we_b) || (wm_a == wm_b && we_a == we_b && wa_a < wa_b) ||
              (wm_a == wm_b && we_a == we_b && wa_a == wa_b && aFunName < bFunName));
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
   bool operator()(const std::pair<std::pair<vertex, vertex>, unsigned int>& x,
                   const std::pair<std::pair<vertex, vertex>, unsigned int>& y) const
   {
      THROW_ASSERT(ref.find(x.first.first) != ref.end(),
                   "Vertex " + GET_NAME(g, x.first.first) + " is not in topological_sort");
      THROW_ASSERT(ref.find(y.first.first) != ref.end(),
                   "Vertex " + GET_NAME(g, y.first.first) + " is not in topological_sort");
      THROW_ASSERT(ref.find(x.first.second) != ref.end(),
                   "Vertex " + GET_NAME(g, x.first.second) + " is not in topological_sort");
      THROW_ASSERT(ref.find(y.first.second) != ref.end(),
                   "Vertex " + GET_NAME(g, y.first.second) + " is not in topological_sort");
      if(ref.find(x.first.first)->second != ref.find(y.first.first)->second)
      {
         return ref.find(x.first.first)->second < ref.find(y.first.first)->second;
      }
      else if(ref.find(x.first.second)->second != ref.find(y.first.second)->second)
      {
         return ref.find(x.first.second)->second < ref.find(y.first.second)->second;
      }
      else
      {
         return x.second < y.second;
      }
   }
};

static bool check_if_is_live_in_next_cycle(const std::set<vertex, cs_ordering_functor>& live_vertices,
                                           const ControlStep current_cycle, const OpVertexMap<double>& ending_time,
                                           double clock_cycle)
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

ParametricListBasedSpecialization::ParametricListBasedSpecialization(
    const ParametricListBased_Metric _parametric_list_based_metric)
    : parametric_list_based_metric(_parametric_list_based_metric)
{
}

std::string ParametricListBasedSpecialization::GetName() const
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

HLSFlowStepSpecialization::context_t ParametricListBasedSpecialization::GetSignatureContext() const
{
   return ComputeSignatureContext(PARAMETRIC_LIST_BASED, static_cast<unsigned char>(parametric_list_based_metric));
}

parametric_list_based::parametric_list_based(const ParameterConstRef _parameters, const HLS_managerRef _HLSMgr,
                                             unsigned int _funId, const DesignFlowManagerConstRef _design_flow_manager,
                                             const HLSFlowStepSpecializationConstRef _hls_flow_step_specialization)
    : Scheduling(_parameters, _HLSMgr, _funId, _design_flow_manager, HLSFlowStep_Type::LIST_BASED_SCHEDULING,
                 _hls_flow_step_specialization ?
                     _hls_flow_step_specialization :
                     HLSFlowStepSpecializationConstRef(
                         new ParametricListBasedSpecialization(static_cast<ParametricListBased_Metric>(
                             _parameters->getOption<unsigned int>(OPT_scheduling_priority))))),
      parametric_list_based_metric(GetPointer<const ParametricListBasedSpecialization>(hls_flow_step_specialization)
                                       ->parametric_list_based_metric),
      ending_time(OpGraphConstRef()),
      clock_cycle(0.0)
{
   debug_level = parameters->get_class_debug_level(GET_CLASS(*this));
}

parametric_list_based::~parametric_list_based() = default;

void parametric_list_based::Initialize()
{
   Scheduling::Initialize();
   const FunctionBehaviorConstRef FB = HLSMgr->CGetFunctionBehavior(funId);
   ending_time = OpVertexMap<double>(FB->CGetOpGraph(FunctionBehavior::CFG));
}

static bool has_element_in_common(const std::set<std::string>& set1, const std::set<std::string>& set2)
{
   auto first1 = set1.begin(), last1 = set1.end(), first2 = set2.begin(), last2 = set2.end();
   while(first1 != last1 and first2 != last2)
   {
      if(*first1 < *first2)
      {
         ++first1;
      }
      else if(*first2 < *first1)
      {
         ++first2;
      }
      else
      {
         return true;
      }
   }
   return false;
}

void parametric_list_based::CheckSchedulabilityConditions(
    const CustomUnorderedSet<vertex>& operations, const vertex& current_vertex, ControlStep current_cycle,
    double& current_starting_time, double& current_ending_time, double& current_stage_period,
    CustomMap<std::pair<unsigned int, unsigned int>, double>& local_connection_map, double current_cycle_starting_time,
    double current_cycle_ending_time, double setup_hold_time, double& phi_extra_time, double scheduling_mux_margins,
    bool unbounded, bool RWFunctions, bool LoadStoreOp, const std::set<std::string>& proxy_functions_used,
    bool cstep_has_RET_conflict, unsigned int fu_type, const vertex2obj<ControlStep>& current_ASAP,
    const fu_bindingRef res_binding, const ScheduleRef schedule, bool& predecessorsCond, bool& pipeliningCond,
    bool& cannotBeChained0, bool& chainingRetCond, bool& cannotBeChained1, bool& asyncCond, bool& cannotBeChained2,
    bool& cannotBeChained3, bool& MultiCond0, bool& MultiCond1, bool& LoadStoreFunctionConflict,
    bool& FunctionLoadStoreConflict, bool& proxyFunCond, bool unbounded_RW, bool seeMulticycle)
{
   predecessorsCond = current_ASAP.find(current_vertex) != current_ASAP.end() and
                      current_ASAP.find(current_vertex)->second > current_cycle;
   if(predecessorsCond)
   {
      return;
   }
   compute_starting_ending_time_asap(operations, current_vertex, fu_type, current_cycle, current_starting_time,
                                     current_ending_time, current_stage_period, cannotBeChained1, res_binding, schedule,
                                     phi_extra_time, setup_hold_time, local_connection_map);
   const auto complex_op = (current_ending_time - current_starting_time) > setup_hold_time;
   const auto is_pipelined = HLS->allocation_information->get_initiation_time(fu_type, current_vertex) != 0;
   const auto n_cycles = HLS->allocation_information->get_cycles(fu_type, current_vertex, flow_graph);
   pipeliningCond = is_pipelined and (current_starting_time > current_cycle_starting_time) and
                    ((current_stage_period + current_starting_time + setup_hold_time + phi_extra_time +
                              (complex_op ? scheduling_mux_margins : 0) >
                          (current_cycle_ending_time) ||
                      unbounded));
   if(pipeliningCond)
   {
      return;
   }
   const auto curr_vertex_type = GET_TYPE(flow_graph, current_vertex);
   cannotBeChained0 =
       (current_starting_time >= current_cycle_ending_time) ||
       ((!is_pipelined && !(curr_vertex_type & TYPE_RET) && n_cycles == 0 &&
         current_starting_time > (current_cycle_starting_time + EPSILON)) &&
        current_ending_time + setup_hold_time + phi_extra_time + (complex_op ? scheduling_mux_margins : 0) >
            current_cycle_ending_time);
   if(cannotBeChained0)
   {
      return;
   }
   chainingRetCond = (unbounded || cstep_has_RET_conflict) && (curr_vertex_type & TYPE_RET);
   if(chainingRetCond)
   {
      return;
   }
   if(cannotBeChained1)
   {
      return;
   }
   asyncCond = (current_starting_time > (EPSILON + current_cycle_starting_time)) and (curr_vertex_type & TYPE_LOAD) and
               HLS->allocation_information->is_one_cycle_direct_access_memory_unit(fu_type) and
               (!HLS->allocation_information->is_readonly_memory_unit(fu_type) ||
                (!HLS->Param->isOption(OPT_rom_duplication) || !parameters->getOption<bool>(OPT_rom_duplication))) and
               ((HLSMgr->Rmem->get_maximum_references(HLS->allocation_information->is_memory_unit(fu_type) ?
                                                          HLS->allocation_information->get_memory_var(fu_type) :
                                                          HLS->allocation_information->get_proxy_memory_var(fu_type))) >
                HLS->allocation_information->get_number_channels(fu_type));
   if(asyncCond)
   {
      return;
   }
   cannotBeChained2 =
       (current_starting_time > (current_cycle_starting_time)) && !parameters->getOption<bool>(OPT_chaining);
   if(cannotBeChained2)
   {
      return;
   }
   MultiCond0 =
       (n_cycles > 1 && (unbounded_RW || unbounded)) ||
       (seeMulticycle && !HLS->allocation_information->is_operation_bounded(flow_graph, current_vertex, fu_type)) ||
       ((!is_pipelined && n_cycles > 0 && current_starting_time > (current_cycle_starting_time)) &&
        current_ending_time - (n_cycles - 1) * clock_cycle + setup_hold_time + phi_extra_time +
                (complex_op ? scheduling_mux_margins : 0) >
            current_cycle_ending_time);
   if(MultiCond0)
   {
      return;
   }
   MultiCond1 = current_ending_time + setup_hold_time + phi_extra_time + (complex_op ? scheduling_mux_margins : 0) >
                    current_cycle_ending_time &&
                unbounded && HLS->allocation_information->is_operation_bounded(flow_graph, current_vertex, fu_type);
   if(MultiCond1)
   {
      return;
   }
   const auto curr_node = flow_graph->CGetOpNodeInfo(current_vertex);
   const auto curr_node_name = curr_node->GetOperation();
   const auto fid = curr_node->called.empty() ? 0U : *curr_node->called.begin();
   if(!HLSMgr->CGetCallGraphManager()->GetRootFunctions().count(fid))
   {
      LoadStoreFunctionConflict = (curr_vertex_type & (TYPE_LOAD | TYPE_STORE)) && RWFunctions;
      if(LoadStoreFunctionConflict)
      {
         return;
      }
      FunctionLoadStoreConflict =
          (curr_vertex_type & TYPE_EXTERNAL) && (curr_vertex_type & TYPE_RW) && (LoadStoreOp || RWFunctions);
      if(FunctionLoadStoreConflict)
      {
         return;
      }
      proxyFunCond = (curr_vertex_type & TYPE_EXTERNAL) &&
                     (proxy_functions_used.count(curr_node_name) ||
                      (reachable_proxy_functions.count(curr_node_name) &&
                       has_element_in_common(proxy_functions_used, reachable_proxy_functions.at(curr_node_name))));
      if(proxyFunCond)
      {
         return;
      }
   }
   cannotBeChained3 = (curr_vertex_type & TYPE_EXTERNAL) && !is_pipelined && n_cycles > 1 &&
                      !HLS->allocation_information->is_operation_PI_registered(flow_graph, current_vertex, fu_type);
   if(cannotBeChained3)
   {
      bool is_chained = false;
      BOOST_FOREACH(EdgeDescriptor e, boost::in_edges(current_vertex, *flow_graph))
      {
         const auto from_vertex = boost::source(e, *flow_graph);
         if(operations.find(from_vertex) == operations.end())
         {
            continue;
         }
         if(GET_TYPE(flow_graph, from_vertex) & (TYPE_PHI | TYPE_VPHI))
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                           "---Skipping phi predecessor " + GET_NAME(flow_graph, from_vertex));
            continue;
         }
         const auto cs_prev = schedule->get_cstep(from_vertex).second;
         if(cs_prev == current_cycle)
         {
            is_chained = true;
            break;
         }
      }
      cannotBeChained3 = false;
      if(is_chained)
      {
         cannotBeChained3 = current_ending_time - (n_cycles - 1) * clock_cycle + setup_hold_time + phi_extra_time +
                                HLS->allocation_information->EstimateControllerDelay() >
                            current_cycle_ending_time;
      }
   }
   if(cannotBeChained3)
   {
      return;
   }
}

const double parametric_list_based::EPSILON = 0.000000001;

#define CTRL_STEP_MULTIPLIER 1000

void parametric_list_based::exec(const OpVertexSet& Operations, ControlStep current_cycle)
{
   PRINT_DBG_MEX(DEBUG_LEVEL_VERBOSE, debug_level, "Executing parametric_list_based::exec...");
   THROW_ASSERT(Operations.size(), "At least one vertex is expected");
   const FunctionBehaviorConstRef FB = HLSMgr->CGetFunctionBehavior(funId);
   const OpGraphConstRef op_graph = FB->CGetOpGraph(FunctionBehavior::CFG);
   const unsigned int return_type_index = FB->CGetBehavioralHelper()->GetFunctionReturnType(funId);
   auto registering_output_p = HLSMgr->CGetCallGraphManager()->GetRootFunctions().count(funId) && return_type_index &&
                               parameters->getOption<std::string>(OPT_registered_inputs) == "top";
   CustomUnorderedSet<vertex> operations;
   for(auto op : Operations)
   {
      operations.insert(op);
   }

   /// The scheduling
   const ScheduleRef schedule = HLS->Rsch;

   /// Current ASAP
   vertex2obj<ControlStep> current_ASAP;

   /// The binding
   const fu_bindingRef res_binding = HLS->Rfu;

   /// Number of predecessors of a vertex already scheduled
   vertex2obj<size_t> scheduled_predecessors;

   double clock_period_resource_fraction = HLS->HLS_C->get_clock_period_resource_fraction();

   double scheduling_mux_margins =
       parameters->getOption<double>(OPT_scheduling_mux_margins) * HLS->allocation_information->mux_time_unit(32);

   /// The clock cycle
   clock_cycle = clock_period_resource_fraction * HLS->HLS_C->get_clock_period();
   if(clock_cycle < scheduling_mux_margins)
   {
      THROW_ERROR("Mux margins too constrained");
   }

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
      flow_graph = FB->CGetOpGraph(FunctionBehavior::SG);
      flow_graph_with_feedbacks = FB->CGetOpGraph(FunctionBehavior::SG);
   }
   else
   {
      flow_graph = FB->CGetOpGraph(FunctionBehavior::FLSAODG);
      flow_graph_with_feedbacks = FB->CGetOpGraph(FunctionBehavior::FFLSAODG);
   }

   /// Number of operation to be scheduled
   size_t operations_number = Operations.size();

   // long int cpu_time;
   /// compute asap and alap
   PRINT_DBG_MEX(DEBUG_LEVEL_VERBOSE, debug_level, "   Computing asap and alap...");
   // START_TIME(cpu_time);
   ASLAPRef aslap;
   if(parametric_list_based_metric == ParametricListBased_Metric::STATIC_MOBILITY or
      parametric_list_based_metric == ParametricListBased_Metric::DYNAMIC_MOBILITY)
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
         auto est_upper_bound = ControlStep(static_cast<unsigned int>(operations_number));
         aslap->compute_ALAP(ASLAP::ALAP_with_partial_scheduling, HLS->Rsch, nullptr, est_upper_bound);
      }
   }
   //   STOP_TIME(cpu_time);
   //   if(output_level >= OUTPUT_LEVEL_VERY_PEDANTIC)
   //      INDENT_OUT_MEX(OUTPUT_LEVEL_VERY_PEDANTIC, output_level, "---Time to perform ASAP+ALAP scheduling: " +
   //      print_cpu_time(cpu_time) + " seconds");

   PRINT_DBG_MEX(DEBUG_LEVEL_VERBOSE, debug_level, "   Computing free input vertices...");
   /// compute the set of vertices without input edges.
   /// At least one vertex is expected
   for(auto operation : Operations)
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
            if(operations.find(source) != operations.end() && !schedule->is_scheduled(source))
            {
               break;
            }
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
         Priority =
             refcount<priority_data<int>>(new priority_dynamic_mobility(aslap, ready_vertices, CTRL_STEP_MULTIPLIER));
         break;
      }
      case ParametricListBased_Metric::STATIC_FIXED:
      {
         CustomUnorderedMapUnstable<vertex, int> priority_value;
         CustomMap<std::string, int> string_priority_value = HLS->HLS_C->get_scheduling_priority();
         VertexIterator ki, ki_end;
         for(auto op : Operations)
         {
            priority_value[op] = string_priority_value[GET_NAME(flow_graph, op)];
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
   bool cstep_has_RET_conflict = false;

   OpVertexSet::const_iterator rv, rv_end = ready_vertices.end();

   for(rv = ready_vertices.begin(); rv != rv_end; ++rv)
   {
      add_to_priority_queues(priority_queues, ready_resources, *rv);
   }

   const auto TM = HLSMgr->get_tree_manager();
   auto fnode = TM->GetTreeNode(funId);
   auto fd = GetPointer<function_decl>(fnode);
   const auto fname = tree_helper::GetMangledFunctionName(fd);
   CustomUnorderedSet<vertex> RW_stmts;
   if(HLSMgr->design_interface_io.find(fname) != HLSMgr->design_interface_io.end())
   {
      for(const auto& bb2arg2stmtsR : HLSMgr->design_interface_io.at(fname))
      {
         for(const auto& arg2stms : bb2arg2stmtsR.second)
         {
            if(arg2stms.second.size() > 0)
            {
               for(const auto& stmt : arg2stms.second)
               {
                  const auto op_it = flow_graph->CGetOpGraphInfo()->tree_node_to_operation.find(stmt);
                  if(op_it != flow_graph->CGetOpGraphInfo()->tree_node_to_operation.end())
                  {
                     RW_stmts.insert(op_it->second);
                  }
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
      bool RWFunctions = false;
      bool unbounded_RW = false;
      bool LoadStoreOp = false;
      bool seeMulticycle = false;
      unsigned int n_scheduled_ops = 0;
      const auto current_cycle_starting_time = from_strongtype_cast<double>(current_cycle) * clock_cycle;
      const auto current_cycle_ending_time = from_strongtype_cast<double>(current_cycle + 1) * clock_cycle;
      cstep_has_RET_conflict =
          ((schedule->num_scheduled() - already_sch) != operations_number - 1) ? registering_output_p : false;
      std::set<std::string> proxy_functions_used;
      PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                    "      schedule->num_scheduled() " + std::to_string(schedule->num_scheduled()));
      PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "      already_sch " + std::to_string(already_sch));
      PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                    "      operations_number " + std::to_string(operations_number));
      PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                    "      Scheduling in control step " + STR(current_cycle) +
                        " (Time: " + STR(current_cycle_starting_time) + ")");
      /// definition of the data structure used to check if a resource is available given a vertex
      /// in case a vertex is not included in a map this mean that the used resources are zero.
      /// First index is the functional unit type, second index is the controller node, third index is the condition
      CustomMap<unsigned int, unsigned int> used_resources;

      /// Operations which can be scheduled in this control step because precedences are satisfied, but they can't be
      /// scheduled in this control step for some reasons Index is the functional unit type
      CustomMap<unsigned int, OpVertexSet> black_list;

      /// Adding information about operation still live
      auto live_vertex_it = live_vertices.begin();
      PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "         Considering live vertices...");
      while(live_vertex_it != live_vertices.end())
      {
         if(ending_time.find(*live_vertex_it)->second > current_cycle_ending_time)
         {
            seeMulticycle = true;
         }
         if(ending_time.find(*live_vertex_it)->second <= current_cycle_starting_time)
         {
            PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                          "            Vertex " + GET_NAME(flow_graph, *live_vertex_it) + " dies");
            live_vertices.erase(*live_vertex_it);
            live_vertex_it = live_vertices.begin();
         }
         else
         {
            auto live_vertex_type = GET_TYPE(flow_graph, *live_vertex_it);
            if(live_vertex_type & TYPE_STORE)
            {
               cstep_has_RET_conflict = true;
            }
            if(live_vertex_type & (TYPE_LOAD | TYPE_STORE))
            {
               LoadStoreOp = true;
            }
            if((live_vertex_type & TYPE_EXTERNAL) && (live_vertex_type & TYPE_RW) &&
               RW_stmts.find(*live_vertex_it) == RW_stmts.end())
            {
               RWFunctions = true;
            }
            const auto II = HLS->allocation_information->get_initiation_time(res_binding->get_assign(*live_vertex_it),
                                                                             *live_vertex_it);
            if(FB->is_simple_pipeline() && (II > 1 || II == 0))
            {
               auto lat = ending_time[*live_vertex_it] - starting_time[*live_vertex_it];
               THROW_ERROR("Timing of Vertex " + GET_NAME(flow_graph, *live_vertex_it) +
                           " is not compatible with II=1.\nActual vertex latency is " + STR(lat) +
                           " greater than the clock period");
            }

            if(II == 0u ||
               current_cycle < (II + static_cast<unsigned int>(floor(starting_time[*live_vertex_it] / clock_cycle))))
            {
               bool schedulable;
               if(used_resources.find(res_binding->get_assign(*live_vertex_it)) == used_resources.end())
               {
                  used_resources[res_binding->get_assign(*live_vertex_it)] = 0;
               }
               schedulable = BB_update_resources_use(used_resources[res_binding->get_assign(*live_vertex_it)],
                                                     res_binding->get_assign(*live_vertex_it));
               if(!schedulable)
               {
                  THROW_ERROR("Unfeasible scheduling");
               }
            }
            PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                          "            Vertex " + GET_NAME(flow_graph, *live_vertex_it) + " lives until " +
                              STR(ending_time.find(*live_vertex_it)->second));
            ++live_vertex_it;
         }
      }

      for(unsigned int fu_type = 0; fu_type < n_resources; ++fu_type)
      {
         if(priority_queues[fu_type].size())
         {
            ready_resources.insert(fu_type);
         }
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
            const auto fu_type = *ready_resources.begin();
            ready_resources.erase(ready_resources.begin());
            THROW_ASSERT(fu_type != static_cast<unsigned>(-1), "");

            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                           "-->Considering functional unit type " + STR(fu_type) + "(" +
                               HLS->allocation_information->get_fu_name(fu_type).first + "-" +
                               HLS->allocation_information->get_fu_name(fu_type).second + ")" + " at clock cycle " +
                               STR(current_cycle));

            auto& queue = priority_queues[fu_type];
            /// Ignore empty list
            while(queue.size())
            {
#if HAVE_UNORDERED
               auto current_vertex = queue.top();
#else
               auto current_vertex = *(queue.begin());
#endif
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                              "---First operation ready for this unit is " + GET_NAME(flow_graph, current_vertex));
#if HAVE_UNORDERED
               queue.pop();
#else
               queue.erase(queue.begin());
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                              "-->Other ready operations (" + STR(queue.size()) + ") are:");
#ifndef NDEBUG
               for(const auto temp_operation : queue)
               {
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                 "---" + GET_NAME(flow_graph, temp_operation) + ": " +
                                     STR((*Priority)(temp_operation)));
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
               {
                  used_resources[fu_type] = 0;
               }
               bool schedulable = used_resources.at(fu_type) != HLS->allocation_information->get_number_fu(fu_type);
               if(!schedulable)
               {
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---No free resource");
                  if(black_list.find(fu_type) == black_list.end())
                  {
                     black_list.emplace(fu_type, OpVertexSet(flow_graph));
                  }
                  black_list.at(fu_type).insert(current_vertex);
                  continue;
               }
               /// check compatibility
               bool postponed = false;
               auto curr_vertex_type = GET_TYPE(flow_graph, current_vertex);
               if(curr_vertex_type & (TYPE_LOAD | TYPE_STORE))
               {
                  bool is_array = HLS->allocation_information->is_direct_access_memory_unit(fu_type);
                  unsigned var = is_array ? (HLS->allocation_information->is_memory_unit(fu_type) ?
                                                 HLS->allocation_information->get_memory_var(fu_type) :
                                                 HLS->allocation_information->get_proxy_memory_var(fu_type)) :
                                            0;
                  if(is_array && cstep_vuses_others && !HLSMgr->Rmem->is_private_memory(var))
                  {
                     postponed = true;
                  }
                  else if(!is_array && cstep_vuses_ARRAYs)
                  {
                     postponed = true;
                  }
                  /*
                  if(!postponed && !cstep_vuses_ARRAYs && !cstep_vuses_others)
                  {
                     for(auto cur_fu_type: ready_resources)
                     {
                        if(HLS->allocation_information->is_direct_access_memory_unit(cur_fu_type) ||
                  HLS->allocation_information->is_indirect_access_memory_unit(cur_fu_type))
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
                                "                  MEMORY_CTRL cannot run together with BRAM direct accesses " +
                                    GET_NAME(flow_graph, current_vertex) + " mapped on " +
                                    HLS->allocation_information->get_fu_name(fu_type).first + "at cstep " +
                                    STR(current_cycle));
                  if(black_list.find(fu_type) == black_list.end())
                  {
                     black_list.emplace(fu_type, OpVertexSet(flow_graph));
                  }
                  black_list.at(fu_type).insert(current_vertex);
                  continue;
               }

               bool is_live = check_if_is_live_in_next_cycle(live_vertices, current_cycle, ending_time, clock_cycle);
               THROW_ASSERT(!(curr_vertex_type & (TYPE_WHILE | TYPE_FOR)), "not expected operation type");
               /// put these type of operations as last operation scheduled for the basic block
               if((curr_vertex_type & (TYPE_IF | TYPE_RET | TYPE_SWITCH | TYPE_MULTIIF | TYPE_GOTO)) &&
                  (unbounded || unbounded_RW || is_live))
               {
                  if(black_list.find(fu_type) == black_list.end())
                  {
                     black_list.emplace(fu_type, OpVertexSet(flow_graph));
                  }
                  black_list.at(fu_type).insert(current_vertex);
                  PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                "            Scheduling of Control Vertex " + GET_NAME(flow_graph, current_vertex) +
                                    " postponed to the next cycle");
                  continue;
               }
               if((curr_vertex_type & (TYPE_IF | TYPE_RET | TYPE_SWITCH | TYPE_MULTIIF | TYPE_GOTO)) &&
                  ((schedule->num_scheduled() - already_sch) != operations_number - 1))
               {
                  if(postponed_resources.find(fu_type) == postponed_resources.end())
                  {
                     postponed_resources.emplace(fu_type, OpVertexSet(flow_graph));
                  }
                  postponed_resources.at(fu_type).insert(current_vertex);
                  PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                "            Scheduling of Control Vertex " + GET_NAME(flow_graph, current_vertex) +
                                    " postponed ");
                  continue;
               }

               if((curr_vertex_type & TYPE_RET) &&
                  ((schedule->num_scheduled() - already_sch) == operations_number - 1) && n_scheduled_ops != 0 &&
                  registering_output_p)
               {
                  if(black_list.find(fu_type) == black_list.end())
                  {
                     black_list.emplace(fu_type, OpVertexSet(flow_graph));
                  }
                  black_list.at(fu_type).insert(current_vertex);

                  PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                "            Scheduling of Control Vertex " + GET_NAME(flow_graph, current_vertex) +
                                    " postponed to the next cycle to register the output");
                  continue;
               }
               if(!HLS->allocation_information->is_operation_bounded(flow_graph, current_vertex, fu_type) &&
                  RW_stmts.find(current_vertex) == RW_stmts.end() && (unbounded_RW || is_live))
               {
                  if(black_list.find(fu_type) == black_list.end())
                  {
                     black_list.emplace(fu_type, OpVertexSet(flow_graph));
                  }
                  black_list.at(fu_type).insert(current_vertex);
                  PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                "            Scheduling of unbounded " + GET_NAME(flow_graph, current_vertex) +
                                    " postponed to the next cycle");
                  continue;
               }
               if(!HLS->allocation_information->is_operation_bounded(flow_graph, current_vertex, fu_type) &&
                  RW_stmts.find(current_vertex) != RW_stmts.end() && unbounded)
               {
                  if(black_list.find(fu_type) == black_list.end())
                  {
                     black_list.emplace(fu_type, OpVertexSet(flow_graph));
                  }
                  black_list.at(fu_type).insert(current_vertex);
                  PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                "            Scheduling of unbounded RW interface " +
                                    GET_NAME(flow_graph, current_vertex) + " postponed to the next cycle");
                  continue;
               }

               /// starting time of the operation
               double current_starting_time;

               /// ending time of the operation
               double current_ending_time;

               /// stage period for pipelined units
               double current_stage_period;

               double phi_extra_time;
               CustomMap<std::pair<unsigned int, unsigned int>, double> local_connection_map;

               bool predecessorsCond = false, pipeliningCond = false, cannotBeChained0 = false, chainingRetCond = false,
                    cannotBeChained1 = false, asyncCond = false, cannotBeChained2 = false, cannotBeChained3 = false,
                    MultiCond0 = false, MultiCond1 = false, LoadStoreFunctionConflict = false,
                    FunctionLoadStoreConflict = false, proxyFunCond = false;

               CheckSchedulabilityConditions(
                   operations, current_vertex, current_cycle, current_starting_time, current_ending_time,
                   current_stage_period, local_connection_map, current_cycle_starting_time, current_cycle_ending_time,
                   setup_hold_time, phi_extra_time, scheduling_mux_margins, unbounded, RWFunctions, LoadStoreOp,
                   proxy_functions_used, cstep_has_RET_conflict, fu_type, current_ASAP, res_binding, schedule,
                   predecessorsCond, pipeliningCond, cannotBeChained0, chainingRetCond, cannotBeChained1, asyncCond,
                   cannotBeChained2, cannotBeChained3, MultiCond0, MultiCond1, LoadStoreFunctionConflict,
                   FunctionLoadStoreConflict, proxyFunCond, unbounded_RW, seeMulticycle);

               /// checking if predecessors have finished
               if(predecessorsCond)
               {
                  PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                "                  Depends on a live operation");
                  if(black_list.find(fu_type) == black_list.end())
                  {
                     black_list.emplace(fu_type, OpVertexSet(flow_graph));
                  }
                  black_list.at(fu_type).insert(current_vertex);
                  continue;
               }

               if(pipeliningCond)
               {
                  PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                "                  Pipelined unit cannot be chained: starting time is " +
                                    std::to_string(current_starting_time) + " stage period is " +
                                    std::to_string(current_stage_period));
                  if(black_list.find(fu_type) == black_list.end())
                  {
                     black_list.emplace(fu_type, OpVertexSet(flow_graph));
                  }
                  black_list.at(fu_type).insert(current_vertex);
                  continue;
               }
               else if(cannotBeChained0)
               {
                  PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                "                  It cannot be chained: starting time is " +
                                    std::to_string(current_starting_time) + " ending time is " +
                                    std::to_string(current_ending_time));
                  if(black_list.find(fu_type) == black_list.end())
                  {
                     black_list.emplace(fu_type, OpVertexSet(flow_graph));
                  }
                  black_list.at(fu_type).insert(current_vertex);
                  continue;
               }
               else if(chainingRetCond)
               {
                  PRINT_DBG_MEX(
                      DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                      "                  Chaining of return expression operation is not allowed by construction" +
                          (unbounded ? std::string("(unbounded)") : std::string("(bounded)")));
                  if(black_list.find(fu_type) == black_list.end())
                  {
                     black_list.emplace(fu_type, OpVertexSet(flow_graph));
                  }
                  black_list.at(fu_type).insert(current_vertex);
                  continue;
               }
               else if(cannotBeChained1)
               {
                  PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                "                  Chaining with a store or a load or an unbounded in input is not "
                                "possible -> starting time " +
                                    std::to_string(current_starting_time) +
                                    " ending time: " + std::to_string(current_ending_time));
                  if(black_list.find(fu_type) == black_list.end())
                  {
                     black_list.emplace(fu_type, OpVertexSet(flow_graph));
                  }
                  black_list.at(fu_type).insert(current_vertex);
                  continue;
               }
               /*
               else if(has_read_cond_with_non_direct_operations)
               {
                  PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "                  READ_COND or MULTI_READ_COND
               cannot be chained with non-direct operations -> starting time " +
               std::to_string(current_starting_time) + " ending time: " +
               std::to_string(current_ending_time)); if(black_list.find(fu_type) == black_list.end())
                     black_list.emplace(fu_type, OpVertexSet(flow_graph));
                  black_list.at(fu_type).insert(current_vertex);
                  continue;
               }*/
               else if(asyncCond)
               {
                  PRINT_DBG_MEX(
                      DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                      "                  Chaining with an asynchronous load is not possible -> starting time " +
                          std::to_string(current_starting_time) +
                          " ending time: " + std::to_string(current_ending_time));
                  if(black_list.find(fu_type) == black_list.end())
                  {
                     black_list.emplace(fu_type, OpVertexSet(flow_graph));
                  }
                  black_list.at(fu_type).insert(current_vertex);
                  continue;
               }
               /*else if((current_starting_time > (current_cycle_starting_time)) && (GET_TYPE(flow_graph, current_v) &
               TYPE_STORE) && HLS->allocation_information->is_indirect_access_memory_unit(fu_type))
               {
                  PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "                  Chaining with a store is not
               possible -> starting time " + std::to_string(current_starting_time) + " ending time: "
               + std::to_string(current_ending_time)); if(black_list.find(fu_type) ==
               black_list.end()) black_list.emplace(fu_type, OpVertexSet(flow_graph));
                  black_list.at(fu_type).insert(current_vertex);
                  continue;
               }
               else if((current_starting_time > (current_cycle_starting_time)) && (curr_vertex_type & TYPE_LOAD) &&
               HLS->allocation_information->is_indirect_access_memory_unit(fu_type))
               {
                  PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "                  Chaining with a load is not
               possible -> starting time " + std::to_string(current_starting_time) + " ending time: "
               + std::to_string(current_ending_time)); if(black_list.find(fu_type) ==
               black_list.end()) black_list.emplace(fu_type, OpVertexSet(flow_graph));
                  black_list.at(fu_type).insert(current_vertex);
                  continue;
               }*/
               else if(cannotBeChained2)
               {
                  PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                "                  Chaining is possible but not allowed -> starting time " +
                                    std::to_string(current_starting_time) +
                                    " ending time: " + std::to_string(current_ending_time));
                  if(black_list.find(fu_type) == black_list.end())
                  {
                     black_list.emplace(fu_type, OpVertexSet(flow_graph));
                  }
                  black_list.at(fu_type).insert(current_vertex);
                  continue;
               }
               else if(MultiCond0)
               {
                  PRINT_DBG_MEX(
                      DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                      "                  First cycle of a Multi-cycles operations does not fit in the first period");
                  if(black_list.find(fu_type) == black_list.end())
                  {
                     black_list.emplace(fu_type, OpVertexSet(flow_graph));
                  }
                  black_list.at(fu_type).insert(current_vertex);
                  continue;
               }
               else if(MultiCond1)
               {
                  PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                "                  Multi-cycles operations cannot be scheduled together with unbounded "
                                "operations");
                  if(black_list.find(fu_type) == black_list.end())
                  {
                     black_list.emplace(fu_type, OpVertexSet(flow_graph));
                  }
                  black_list.at(fu_type).insert(current_vertex);
                  continue;
               }
               else if(LoadStoreFunctionConflict)
               {
                  PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                "                  Memory access operations may conflict with function calls");
                  if(black_list.find(fu_type) == black_list.end())
                  {
                     black_list.emplace(fu_type, OpVertexSet(flow_graph));
                  }
                  black_list.at(fu_type).insert(current_vertex);
                  continue;
               }
               else if(FunctionLoadStoreConflict)
               {
                  PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                "                  function calls may conflict with memory access operations ");
                  if(black_list.find(fu_type) == black_list.end())
                  {
                     black_list.emplace(fu_type, OpVertexSet(flow_graph));
                  }
                  black_list.at(fu_type).insert(current_vertex);
                  continue;
               }
               else if(proxyFunCond)
               {
                  PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                "                  proxy function may conflict with other functions calling the same "
                                "proxy function ");
                  if(black_list.find(fu_type) == black_list.end())
                  {
                     black_list.emplace(fu_type, OpVertexSet(flow_graph));
                  }
                  black_list.at(fu_type).insert(current_vertex);
                  continue;
               }
               /*else if(store_in_chaining_with_load_in(current_cycle, current_vertex))
               {
                  PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "                  Load and store cannot be
               chained"); if(black_list.find(fu_type) == black_list.end()) black_list.emplace(fu_type,
               OpVertexSet(flow_graph)); black_list.at(fu_type).insert(current_vertex); continue;
               }*/
               else if(cannotBeChained3)
               {
                  PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                "                  cannot be chained because critical path of the current function "
                                "plus the one of the called function are not compatible with the clock ");
                  if(black_list.find(fu_type) == black_list.end())
                  {
                     black_list.emplace(fu_type, OpVertexSet(flow_graph));
                  }
                  black_list.at(fu_type).insert(current_vertex);
                  continue;
               }

               /// scheduling is now possible
               ++n_scheduled_ops;
               /// update resource usage
               used_resources[fu_type]++;

               /// check if we have functions accessing the memory
               if((curr_vertex_type & TYPE_EXTERNAL) && (curr_vertex_type & TYPE_RW) &&
                  RW_stmts.find(current_vertex) == RW_stmts.end())
               {
                  RWFunctions = true;
               }

               if((curr_vertex_type & TYPE_EXTERNAL))
               {
                  auto curr_op_name = flow_graph->CGetOpNodeInfo(current_vertex)->GetOperation();
                  if(HLSMgr->Rfuns->is_a_proxied_function(curr_op_name))
                  {
                     proxy_functions_used.insert(curr_op_name);
                  }
                  if(reachable_proxy_functions.find(curr_op_name) != reachable_proxy_functions.end())
                  {
                     for(const auto& p : reachable_proxy_functions.at(curr_op_name))
                     {
                        proxy_functions_used.insert(p);
                     }
                  }
               }

               /// check if we have unbounded resources
               if(!HLS->allocation_information->is_operation_bounded(flow_graph, current_vertex, fu_type) &&
                  RW_stmts.find(current_vertex) == RW_stmts.end())
               {
                  PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                "                  " + GET_NAME(flow_graph, current_vertex) + " is unbounded");
                  THROW_ASSERT(!(is_live), "unexpected case");
                  double ex_time = HLS->allocation_information->get_execution_time(fu_type, current_vertex, flow_graph);
                  if(ex_time > clock_cycle)
                  {
                     THROW_WARNING("Operation execution time of the unbounded operation is greater than the clock "
                                   "period resource fraction (" +
                                   STR(clock_cycle) + ").\n\tExecution time " + STR(ex_time) + " of " +
                                   GET_NAME(flow_graph, current_vertex) + " of type " +
                                   flow_graph->CGetOpNodeInfo(current_vertex)->GetOperation() +
                                   "\nThis may prevent meeting the timing constraints.\n");
                  }
                  unbounded = true;
               }
               else if(!HLS->allocation_information->is_operation_bounded(flow_graph, current_vertex, fu_type) &&
                       RW_stmts.find(current_vertex) != RW_stmts.end())
               {
                  unbounded_RW = true;
               }
               else
               {
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                 "---" + GET_NAME(flow_graph, current_vertex) + " is bounded");
               }
               /// check if we have memory accesses
               if((curr_vertex_type & (TYPE_LOAD | TYPE_STORE)))
               {
                  LoadStoreOp = true;
               }

               /// update cstep_vuses
               if(curr_vertex_type & (TYPE_LOAD | TYPE_STORE))
               {
                  cstep_has_RET_conflict = (curr_vertex_type & (TYPE_STORE)) != 0;

                  bool is_array = HLS->allocation_information->is_direct_access_memory_unit(fu_type);
                  unsigned var = is_array ? (HLS->allocation_information->is_memory_unit(fu_type) ?
                                                 HLS->allocation_information->get_memory_var(fu_type) :
                                                 HLS->allocation_information->get_proxy_memory_var(fu_type)) :
                                            0;
                  if(!var || !HLSMgr->Rmem->is_private_memory(var))
                  {
                     if(HLS->allocation_information->is_direct_access_memory_unit(fu_type) && !cstep_vuses_others)
                     {
                        cstep_vuses_ARRAYs = 1;
                     }
                     else
                     {
                        cstep_vuses_others =
                            1 + ((parameters->getOption<std::string>(OPT_memory_controller_type) != "D00" &&
                                  parameters->getOption<std::string>(OPT_memory_controller_type) != "D10") ?
                                     1 :
                                     0);
                     }
                  }
               }
               if((curr_vertex_type & TYPE_EXTERNAL) && (curr_vertex_type & TYPE_RW))
               {
                  cstep_has_RET_conflict = true;
               }
               /// set the schedule information
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                              "---" + GET_NAME(flow_graph, current_vertex) + " scheduled at " +
                                  STR(current_cycle_starting_time));
               schedule->set_execution(current_vertex, current_cycle);
               starting_time[current_vertex] = current_starting_time;
               if(!HLS->allocation_information->is_operation_bounded(flow_graph, current_vertex, fu_type) &&
                  (current_ending_time + setup_hold_time + phi_extra_time + scheduling_mux_margins >
                   current_cycle_ending_time))
               {
                  ending_time[current_vertex] = current_cycle_ending_time - setup_hold_time;
               }
               else
               {
                  ending_time[current_vertex] = current_ending_time;
               }
               auto current_statement = flow_graph->CGetOpNodeInfo(current_vertex)->GetNodeId();
               schedule->starting_times[current_statement] = starting_time[current_vertex];
               schedule->ending_times[current_statement] = ending_time[current_vertex];
               THROW_ASSERT(ending_time[current_vertex] >= starting_time[current_vertex], "unexpected condition");
               THROW_ASSERT(floor(ending_time[current_vertex] / clock_cycle + 0.0001) >=
                                from_strongtype_cast<double>(current_cycle),
                            GET_NAME(flow_graph, current_vertex) + " " + STR(ending_time[current_vertex]) + " " +
                                STR(from_strongtype_cast<double>(current_cycle)));
               schedule->set_execution_end(current_vertex,
                                           current_cycle + ControlStep(static_cast<unsigned int>(
                                                               floor(ending_time[current_vertex] / clock_cycle) -
                                                               from_strongtype_cast<double>(current_cycle))));
               /// check if we have operations taking more than one cycle
               if(HLS->allocation_information->is_operation_bounded(flow_graph, current_vertex, fu_type) &&
                  ending_time[current_vertex] > current_cycle_ending_time)
               {
                  seeMulticycle = true;
               }

               for(auto edge_connection_pair : local_connection_map)
               {
                  schedule->AddConnectionTimes(edge_connection_pair.first.first, edge_connection_pair.first.second,
                                               edge_connection_pair.second);
               }
               if(phi_extra_time > 0.0)
               {
                  schedule->AddConnectionTimes(current_statement, 0, phi_extra_time);
               }
               ready_vertices.erase(current_vertex);

               /// set the binding information
               if(HLS->HLS_C->has_binding_to_fu(GET_NAME(flow_graph, current_vertex)))
               {
                  res_binding->bind(current_vertex, fu_type, 0);
               }
               else
               {
                  res_binding->bind(current_vertex, fu_type);
               }

               PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                             "                  Current cycles ends at " + STR(current_cycle_ending_time) +
                                 "  - operation ends at " + STR(ending_time[current_vertex]));
               if(ending_time[current_vertex] > current_cycle_ending_time)
               {
                  live_vertices.insert(current_vertex);
               }

               /// Check if some successors have become ready
               OutEdgeIterator eo, eo_end;

               std::list<std::pair<std::string, vertex>> successors;
               for(boost::tie(eo, eo_end) = boost::out_edges(current_vertex, *flow_graph); eo != eo_end; eo++)
               {
                  vertex target = boost::target(*eo, *flow_graph);
                  if(operations.find(target) != operations.end())
                  {
                     successors.push_back(std::make_pair(GET_NAME(flow_graph, target), target));
                  }
               }
               // successors.sort();

               for(auto& successor : successors)
               {
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Considering successor " + successor.first);
                  scheduled_predecessors[successor.second]++;
                  if(current_ASAP.find(successor.second) != current_ASAP.end())
                  {
                     current_ASAP.at(successor.second) =
                         std::max(ControlStep(static_cast<unsigned int>(
                                      floor(ending_time.find(current_vertex)->second / clock_cycle))),
                                  current_ASAP.find(successor.second)->second);
                  }
                  else
                  {
                     current_ASAP.emplace(successor.second,
                                          ControlStep(static_cast<unsigned int>(
                                              floor(ending_time.find(current_vertex)->second / clock_cycle))));
                  }
                  /// check if to_v should be considered as ready
                  CustomUnorderedSet<vertex> in_set;
                  InEdgeIterator ei, ei_end;
                  for(boost::tie(ei, ei_end) = boost::in_edges(successor.second, *flow_graph); ei != ei_end; ei++)
                  {
                     vertex source = boost::source(*ei, *flow_graph);
                     if(operations.find(source) != operations.end())
                     {
                        in_set.insert(source);
                     }
                  }
                  if(in_set.size() == scheduled_predecessors(successor.second))
                  {
                     INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Become ready");
                     ready_vertices.insert(successor.second);
                     add_to_priority_queues(priority_queues, ready_resources, successor.second);
                  }
               }
            }
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Finished with unit type " + STR(fu_type));
         }
         if(!restarted_resources.empty())
         {
            CustomMap<unsigned int, OpVertexSet>::const_iterator bl_end = restarted_resources.end();
            for(CustomMap<unsigned int, OpVertexSet>::const_iterator bl_it = restarted_resources.begin();
                bl_it != bl_end; ++bl_it)
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
               PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                             "         Restarted the scheduling loop to accommodate restarted vertices: " +
                                 STR(prev_scheduled) + " vs " + STR(schedule->num_scheduled()));
               prev_scheduled = schedule->num_scheduled();
            }
         }
         if(!postponed_resources.empty())
         {
            CustomMap<unsigned int, OpVertexSet>::const_iterator bl_end = postponed_resources.end();
            for(CustomMap<unsigned int, OpVertexSet>::const_iterator bl_it = postponed_resources.begin();
                bl_it != bl_end; ++bl_it)
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
               PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                             "         Restarted the scheduling loop to accommodate postponed vertices");
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
      {
         for(unsigned int i = 0; i < n_resources; i++)
         {
            priority_queues[i].rehash();
         }
      }
#endif
      /// clear the vuses
      cstep_vuses_ARRAYs = cstep_vuses_ARRAYs > 0 ? cstep_vuses_ARRAYs - 1 : 0;
      cstep_vuses_others = cstep_vuses_others > 0 ? cstep_vuses_others - 1 : 0;
      /// move to the next cycle
      ++current_cycle;
   }

   const auto steps = current_cycle;
   schedule->set_csteps(steps);
   PRINT_DBG_MEX(DEBUG_LEVEL_VERBOSE, debug_level, "   List Based finished");
}

void parametric_list_based::compute_starting_ending_time_asap(
    const CustomUnorderedSet<vertex>& operations, const vertex v, const unsigned int fu_type, const ControlStep cs,
    double& current_starting_time, double& current_ending_time, double& stage_period, bool& cannot_be_chained,
    fu_bindingRef res_binding, const ScheduleConstRef schedule, double& phi_extra_time, double setup_hold_time,
    CustomMap<std::pair<unsigned int, unsigned int>, double>& local_connection_map)
{
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                  "-->Computing starting and ending time " + GET_NAME(flow_graph, v));
   current_starting_time = from_strongtype_cast<double>(cs) * clock_cycle;
   bool is_load_store = GET_TYPE(flow_graph, v) & (TYPE_LOAD | TYPE_STORE);
   bool is_cond_op = GET_TYPE(flow_graph, v) & (TYPE_IF | TYPE_MULTIIF | TYPE_SWITCH);
   bool no_chaining_of_load_and_store = parameters->getOption<bool>(OPT_do_not_chain_memories) &&
                                        (check_LOAD_chaining(operations, v, cs, schedule) || is_load_store);
   cannot_be_chained =
       is_load_store && check_non_direct_operation_chaining(operations, v, fu_type, cs, schedule, res_binding);
   PRINT_DBG_MEX(DEBUG_LEVEL_VERBOSE, debug_level,
                 "                  Initial value of cannot_be_chained=" +
                     (cannot_be_chained ? std::string("T") : std::string("F")));
   InEdgeIterator ei, ei_end;
   for(boost::tie(ei, ei_end) = boost::in_edges(v, *flow_graph); ei != ei_end; ei++)
   {
      vertex from_vertex = boost::source(*ei, *flow_graph);
      if(operations.find(from_vertex) == operations.end())
      {
         continue;
      }
      if(GET_TYPE(flow_graph, from_vertex) & (TYPE_PHI | TYPE_VPHI))
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                        "---Skipping phi predecessor " + GET_NAME(flow_graph, from_vertex));
         continue;
      }
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                     "-->Considering predecessor " + GET_NAME(flow_graph, from_vertex));
      unsigned int from_fu_type = res_binding->get_assign(from_vertex);
      const auto cs_prev = schedule->get_cstep(from_vertex).second;
      const double fsm_correction = [&]() -> double {
         if(is_cond_op)
         {
            return HLS->allocation_information->estimate_controller_delay_fb();
         }
         if(parameters->getOption<double>(OPT_scheduling_mux_margins) != 0.0)
         {
            return HLS->allocation_information->EstimateControllerDelay();
         }
         if(cs == cs_prev && HLS->allocation_information->is_one_cycle_direct_access_memory_unit(from_fu_type) &&
            (!HLS->allocation_information->is_readonly_memory_unit(from_fu_type) ||
             (!parameters->isOption(OPT_rom_duplication) || !parameters->getOption<bool>(OPT_rom_duplication))) &&
            HLSMgr->Rmem->get_maximum_references(HLS->allocation_information->is_memory_unit(from_fu_type) ?
                                                     HLS->allocation_information->get_memory_var(from_fu_type) :
                                                     HLS->allocation_information->get_proxy_memory_var(from_fu_type)) >
                HLS->allocation_information->get_number_channels(from_fu_type))
         {
            return HLS->allocation_information->EstimateControllerDelay();
         }
         return 0.0;
      }();
      auto from_statement = flow_graph->CGetOpNodeInfo(from_vertex)->GetNodeId();
      auto v_statement = flow_graph->CGetOpNodeInfo(v)->GetNodeId();
      const auto v_basic_block_index = flow_graph->CGetOpNodeInfo(v)->bb_index;
      auto edge_pair = std::make_pair(from_statement, v_statement);
      double connection_time = local_connection_map[edge_pair] =
          (schedule->get_cstep_end(from_vertex).second == cs) ?
              HLS->allocation_information->GetConnectionTime(from_statement, v_statement,
                                                             AbsControlStep(v_basic_block_index, cs)) :
              0.0;
      /// ending time is equal to the connection time plus the maximum between the controller time and the operation
      /// ending time
      double local_ending_time =
          connection_time +
          ((cs == cs_prev &&
            starting_time(from_vertex) < (fsm_correction + (from_strongtype_cast<double>(cs) * clock_cycle))) ?
               ending_time.find(from_vertex)->second + fsm_correction +
                   (from_strongtype_cast<double>(cs) * clock_cycle) - starting_time(from_vertex) :
               ending_time.find(from_vertex)->second);
      current_starting_time = std::max(current_starting_time, local_ending_time);
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                     "current_starting_time of " + STR(GET_NAME(flow_graph, v)) + " updated to " +
                         STR(current_starting_time));

      /// Check for chaining
      if(schedule->get_cstep_end(from_vertex).second == cs)
      {
         if(no_chaining_of_load_and_store)
         {
            cannot_be_chained = true;
         }
         if(not HLS->allocation_information->CanBeChained(from_vertex, v))
         {
            cannot_be_chained = true;
         }
      }
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--");
   }

   double op_execution_time;
   compute_exec_stage_time(fu_type, stage_period, cs, flow_graph, v, op_execution_time, phi_extra_time,
                           current_starting_time, setup_hold_time);

   current_ending_time = current_starting_time + op_execution_time;

   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                  "<--Operation " + GET_NAME(flow_graph, v) + " starts at " + STR(current_starting_time) +
                      " and ends at " + STR(current_ending_time) + " execution time " + STR(op_execution_time) +
                      " stage period " + STR(stage_period) + " phi_extra_time " + STR(phi_extra_time));
}

void parametric_list_based::compute_exec_stage_time(const unsigned int fu_type, double& stage_period,
                                                    const ControlStep cs, const OpGraphConstRef op_graph, vertex v,
                                                    double& op_execution_time, double& phi_extra_time,
                                                    double current_starting_time, double setup_hold_time)
{
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                  "-->Computing exec stage time of " + GET_NAME(flow_graph_with_feedbacks, v));
   const auto bb_index = op_graph->CGetOpNodeInfo(v)->bb_index;
   std::pair<double, double> timeLatency = HLS->allocation_information->GetTimeLatency(v, fu_type);
   op_execution_time = timeLatency.first;
   stage_period = timeLatency.second;

   /// check for PHIs attached to the output. They may require one or more muxes.
   phi_extra_time = HLS->allocation_information->GetConnectionTime(
       flow_graph_with_feedbacks->CGetOpNodeInfo(v)->GetNodeId(), 0, AbsControlStep(bb_index, cs));

   double scheduling_mux_margins =
       parameters->getOption<double>(OPT_scheduling_mux_margins) * HLS->allocation_information->mux_time_unit(32);

   /// corrections for the memory controllers
   unsigned int n_cycles = std::max(1u, HLS->allocation_information->get_cycles(fu_type, v, flow_graph));
   if((HLS->allocation_information->is_indirect_access_memory_unit(fu_type)) &&
      op_execution_time + setup_hold_time >= clock_cycle * n_cycles)
   {
      op_execution_time = clock_cycle * n_cycles - setup_hold_time - phi_extra_time - scheduling_mux_margins - EPSILON;
   }
   if((HLS->allocation_information->is_indirect_access_memory_unit(fu_type)) &&
      stage_period + setup_hold_time >= clock_cycle)
   {
      stage_period = clock_cycle - setup_hold_time - phi_extra_time - scheduling_mux_margins - EPSILON;
   }

   /// corrections in case the unit first fits in a clock period and then after all the additions does not fit anymore
   double initial_execution_time =
       HLS->allocation_information->get_execution_time(fu_type, v, flow_graph_with_feedbacks) -
       HLS->allocation_information->get_correction_time(
           fu_type, flow_graph_with_feedbacks->CGetOpNodeInfo(v)->GetOperation(),
           static_cast<unsigned>(
               flow_graph_with_feedbacks->CGetOpNodeInfo(v)
                   ->GetVariables(FunctionBehavior_VariableType::SCALAR, FunctionBehavior_VariableAccessType::USE)
                   .size()));
   if(initial_execution_time + setup_hold_time < clock_cycle &&
      op_execution_time + setup_hold_time + phi_extra_time + scheduling_mux_margins >= clock_cycle)
   {
      op_execution_time = clock_cycle - setup_hold_time - phi_extra_time - scheduling_mux_margins - EPSILON;
      if(op_execution_time < 0)
      {
         op_execution_time = clock_cycle - setup_hold_time - scheduling_mux_margins - EPSILON;
         phi_extra_time = 0.0;
      }
   }
   if(HLS->allocation_information->get_initiation_time(fu_type, v) > 0 &&
      (stage_period + setup_hold_time + phi_extra_time + scheduling_mux_margins >= clock_cycle))
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
         {
            op_execution_time += clock_cycle - EPSILON;
         }
         else
         {
            op_execution_time += stage_period;
         }
         op_execution_time -= current_starting_time - from_strongtype_cast<double>(cs) * clock_cycle;
      }
      else
      {
         THROW_ERROR("unexpected case");
      }
   }
   /// Stage period has already been used for computing ending time of operation
   /// If the operation has registered inputs, the stage period of first stage is 0 (mux delay is described by
   /// connection time)
   if(HLS->allocation_information->is_operation_PI_registered(op_graph, v, HLS->allocation_information->GetFuType(v)))
   {
      stage_period = 0.0;
   }
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Execution time is " + STR(op_execution_time));
}

bool parametric_list_based::BB_update_resources_use(unsigned int& used_resources, const unsigned int fu_type) const
{
   if(used_resources == HLS->allocation_information->get_number_fu(fu_type))
   {
      return false;
   }
   else
   {
      used_resources++;
      return true;
   }
}

void parametric_list_based::add_to_priority_queues(PriorityQueues& priority_queue,
                                                   std::set<unsigned int, resource_ordering_functor>& ready_resources,
                                                   const vertex v) const
{
   unsigned int fu_name;
   if(HLS->allocation_information->is_vertex_bounded_with(v, fu_name))
   {
#if HAVE_UNORDERED
      priority_queue[fu_name].push(v);
#else
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                     "---" + GET_NAME(flow_graph, v) + " bound to " +
                         HLS->allocation_information->get_fu_name(fu_name).first);
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

void parametric_list_based::compute_function_topological_order()
{
   PRINT_DBG_MEX(DEBUG_LEVEL_VERBOSE, debug_level, "compute function topological order...");
   std::list<vertex> topology_sorted_vertex;
   const CallGraphManagerConstRef CG = HLSMgr->CGetCallGraphManager();
   CG->CGetCallGraph()->TopologicalSort(topology_sorted_vertex);

   CustomOrderedSet<unsigned> reachable_functions = CG->GetReachedFunctionsFrom(funId);

   std::list<unsigned int> topological_sorted_functions;
   for(auto v : topology_sorted_vertex)
   {
      auto fun_id = CG->get_function(v);
      if(reachable_functions.find(fun_id) != reachable_functions.end())
      {
         topological_sorted_functions.push_front(fun_id);
      }
   }
   reachable_proxy_functions.clear();
   for(auto v : topological_sorted_functions)
   {
      auto curr_v_name = HLSMgr->CGetFunctionBehavior(v)->CGetBehavioralHelper()->get_function_name();
      if(HLSMgr->Rfuns->has_proxied_shared_functions(v))
      {
         for(const auto& p : HLSMgr->Rfuns->get_proxied_shared_functions(v))
         {
            reachable_proxy_functions[curr_v_name].insert(p);
         }
      }

      for(auto c : CG->get_called_by(v))
      {
         auto called_name = HLSMgr->CGetFunctionBehavior(c)->CGetBehavioralHelper()->get_function_name();
         if(reachable_proxy_functions.find(called_name) != reachable_proxy_functions.end())
         {
            for(const auto& cp : reachable_proxy_functions.at(called_name))
            {
               reachable_proxy_functions[curr_v_name].insert(cp);
            }
         }
      }
   }
}

DesignFlowStep_Status parametric_list_based::InternalExec()
{
   long int step_time = 0;
   if(output_level >= OUTPUT_LEVEL_MINIMUM and output_level <= OUTPUT_LEVEL_PEDANTIC)
   {
      START_TIME(step_time);
   }
   const FunctionBehaviorConstRef FB = HLSMgr->CGetFunctionBehavior(funId);
   const BBGraphConstRef bbg = FB->CGetBBGraph();
   const OpGraphConstRef op_graph = FB->CGetOpGraph(FunctionBehavior::CFG);
   std::deque<vertex> vertices;
   boost::topological_sort(*bbg, std::front_inserter(vertices));
   auto viend = vertices.end();
   auto ctrl_steps = ControlStep(0u);

   if(output_level <= OUTPUT_LEVEL_PEDANTIC)
   {
      INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, output_level, "");
   }
   INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, output_level,
                  "-->Scheduling Information of function " + FB->CGetBehavioralHelper()->get_function_name() + ":");
   /// initialize topological_sorted_functions
   compute_function_topological_order();
   for(auto vi = vertices.begin(); vi != viend; ++vi)
   {
      OpVertexSet operations(op_graph);
      std::list<vertex> bb_operations = bbg->CGetBBNodeInfo(*vi)->statements_list;
      for(auto& bb_operation : bb_operations)
      {
         if(HLS->operations.find(bb_operation) != HLS->operations.end())
         {
            operations.insert(bb_operation);
         }
      }
      PRINT_DBG_MEX(DEBUG_LEVEL_VERBOSE, debug_level,
                    "performing scheduling of basic block " + STR(bbg->CGetBBNodeInfo(*vi)->block->number));
      PRINT_DBG_MEX(DEBUG_LEVEL_VERBOSE, debug_level, "  .operations: " + STR(operations.size()));

      exec(operations, ctrl_steps);

      ctrl_steps = HLS->Rsch->get_csteps();
      if(parameters->getOption<bool>(OPT_print_dot))
      {
         auto subgraph = FB->CGetOpGraph(FunctionBehavior::FSDG, operations);
         HLS->Rsch->WriteDot("HLS_scheduling_BB" + STR(bbg->CGetBBNodeInfo(*vi)->block->number) + ".dot", subgraph,
                             &operations);
      }
   }
   HLS->Rsch->set_spec(get_spec());

   /// Find the minimum slack
   double min_slack = std::numeric_limits<double>::max();
   for(auto operation : ending_time)
   {
      double realClock = HLS->HLS_C->get_clock_period();

      double endingTime = operation.second;
      const auto endControlStep = HLS->Rsch->get_cstep_end(operation.first).second;
      double slack = (from_strongtype_cast<double>(endControlStep) + 1) * clock_cycle - endingTime + realClock -
                     clock_cycle - HLS->allocation_information->get_setup_hold_time();
      if(slack < 0)
      {
         slack = EPSILON / 2;
      }
      if(min_slack > slack)
      {
         PRINT_DBG_MEX(DEBUG_LEVEL_VERBOSE, debug_level,
                       std::string("Operation constraining the maximum frequency:") +
                           GET_NAME(flow_graph, operation.first) + " slack=" + STR(slack));
      }
      min_slack = std::min(min_slack, slack);
   }
   HLS->allocation_information->setMinimumSlack(min_slack);
   INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, output_level, "---Number of control steps: " + STR(ctrl_steps));

   if(output_level >= OUTPUT_LEVEL_VERY_PEDANTIC)
   {
      HLS->Rsch->print(HLS->Rfu);
   }

   double maxFrequency = 1000.0 / (HLS->HLS_C->get_clock_period() - HLS->allocation_information->getMinimumSlack());
   INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, output_level,
                  "---Minimum slack: " + STR(HLS->allocation_information->getMinimumSlack()));
   INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, output_level, "---Estimated max frequency (MHz): " + STR(maxFrequency));

   INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, output_level, "<--");
   if(output_level >= OUTPUT_LEVEL_MINIMUM and output_level <= OUTPUT_LEVEL_PEDANTIC)
   {
      STOP_TIME(step_time);
   }
   if(output_level >= OUTPUT_LEVEL_MINIMUM and output_level <= OUTPUT_LEVEL_PEDANTIC)
   {
      INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, output_level,
                     "---Time to perform scheduling: " + print_cpu_time(step_time) + " seconds");
   }
   if(output_level <= OUTPUT_LEVEL_PEDANTIC)
   {
      INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, output_level, "");
   }

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
      document.write_to_file_formatted(function_name + "_scheduling.xml");
   }
   return DesignFlowStep_Status::SUCCESS;
}

bool parametric_list_based::store_in_chaining_with_load_in(const CustomUnorderedSet<vertex>& operations,
                                                           unsigned int current_vertex_cstep, vertex v)
{
   const auto operation = flow_graph->CGetOpNodeInfo(v)->GetOperation();
   bool is_load_store = GET_TYPE(flow_graph, v) & (TYPE_LOAD | TYPE_STORE);
   if(not is_load_store)
   {
      return false;
   }
   std::queue<vertex> fifo;
   InEdgeIterator eo, eo_end;
   for(boost::tie(eo, eo_end) = boost::in_edges(v, *flow_graph); eo != eo_end; eo++)
   {
      auto s = boost::source(*eo, *flow_graph);
      if(operations.find(s) != operations.end())
      {
         fifo.push(s);
      }
   }
   while(!fifo.empty())
   {
      vertex current_op = fifo.front();
      fifo.pop();
      if(current_vertex_cstep == static_cast<unsigned int>(floor(ending_time.find(current_op)->second / clock_cycle)))
      {
         if(GET_TYPE(flow_graph, current_op) & (TYPE_LOAD | TYPE_STORE))
         {
            return true;
         }
         for(boost::tie(eo, eo_end) = boost::in_edges(current_op, *flow_graph); eo != eo_end; eo++)
         {
            auto s = boost::source(*eo, *flow_graph);
            if(operations.find(s) != operations.end())
            {
               fifo.push(s);
            }
         }
      }
   }
   return false;
}

bool parametric_list_based::store_in_chaining_with_load_out(const CustomUnorderedSet<vertex>& operations,
                                                            unsigned int current_vertex_cstep, vertex v)
{
   const auto operation = flow_graph->CGetOpNodeInfo(v)->GetOperation();
   bool is_load_store = GET_TYPE(flow_graph, v) & (TYPE_LOAD | TYPE_STORE);
   if(not is_load_store)
   {
      return false;
   }
   std::queue<vertex> fifo;
   OutEdgeIterator eo, eo_end;
   for(boost::tie(eo, eo_end) = boost::out_edges(v, *flow_graph); eo != eo_end; eo++)
   {
      auto s = boost::source(*eo, *flow_graph);
      if(operations.find(s) != operations.end())
      {
         fifo.push(s);
      }
   }
   while(!fifo.empty())
   {
      vertex current_op = fifo.front();
      fifo.pop();
      if(current_vertex_cstep == static_cast<unsigned int>(floor(starting_time(current_op) / clock_cycle)))
      {
         if(GET_TYPE(flow_graph, current_op) & (TYPE_LOAD | TYPE_STORE))
         {
            return true;
         }
         for(boost::tie(eo, eo_end) = boost::out_edges(current_op, *flow_graph); eo != eo_end; eo++)
         {
            auto s = boost::source(*eo, *flow_graph);
            if(operations.find(s) != operations.end())
            {
               fifo.push(s);
            }
         }
      }
   }
   return false;
}

#define REMOVE_DIRECT_TO_INDIRECT 1
bool parametric_list_based::check_non_direct_operation_chaining(const CustomUnorderedSet<vertex>& operations,
                                                                vertex current_v, unsigned int v_fu_type,
                                                                const ControlStep cs, const ScheduleConstRef schedule,
                                                                fu_bindingRef res_binding) const
{
   bool v_is_indirect =
       REMOVE_DIRECT_TO_INDIRECT && HLS->allocation_information->is_indirect_access_memory_unit(v_fu_type);
   bool v_is_one_cycle_direct_access =
       (HLS->allocation_information->is_one_cycle_direct_access_memory_unit(v_fu_type) &&
        (!HLS->allocation_information->is_readonly_memory_unit(v_fu_type) ||
         (!HLS->Param->isOption(OPT_rom_duplication) || !parameters->getOption<bool>(OPT_rom_duplication)))) &&
       HLSMgr->Rmem->get_maximum_references(HLS->allocation_information->is_memory_unit(v_fu_type) ?
                                                HLS->allocation_information->get_memory_var(v_fu_type) :
                                                HLS->allocation_information->get_proxy_memory_var(v_fu_type)) >
           HLS->allocation_information->get_number_channels(v_fu_type);

   /// Set of already analyzed operations
   CustomUnorderedSet<vertex> already_analyzed_operations;

   /// compute the set of operations on the control step frontier
   OpVertexSet to_be_analyzed(flow_graph);
   InEdgeIterator ie, ie_end;
   for(boost::tie(ie, ie_end) = boost::in_edges(current_v, *flow_graph); ie != ie_end; ie++)
   {
      auto v = boost::source(*ie, *flow_graph);
      if(operations.find(v) != operations.end())
      {
         to_be_analyzed.insert(v);
      }
   }
   while(!to_be_analyzed.empty())
   {
      vertex current_op = *(to_be_analyzed.begin());
      to_be_analyzed.erase(to_be_analyzed.begin());
      already_analyzed_operations.insert(current_op);
      if(cs == schedule->get_cstep_end(current_op).second)
      {
         unsigned int from_fu_type = res_binding->get_assign(current_op);
         if((GET_TYPE(flow_graph, current_op) & TYPE_LOAD) and
            (v_is_indirect or
             (v_is_one_cycle_direct_access and
              HLS->allocation_information->is_one_cycle_direct_access_memory_unit(from_fu_type)) or
             HLS->allocation_information->is_indirect_access_memory_unit(from_fu_type)))
         {
            return true;
         }
         for(boost::tie(ie, ie_end) = boost::in_edges(current_op, *flow_graph); ie != ie_end; ie++)
         {
            const auto source = boost::source(*ie, *flow_graph);
            if(operations.find(source) != operations.end() &&
               already_analyzed_operations.find(source) == already_analyzed_operations.end())
            {
               to_be_analyzed.insert(source);
            }
         }
      }
   }
   return false;
}

bool parametric_list_based::check_direct_operation_chaining(const CustomUnorderedSet<vertex>& operations,
                                                            vertex current_v, const ControlStep cs,
                                                            const ScheduleConstRef schedule,
                                                            fu_bindingRef res_binding) const
{
   /// compute the set of operations on the control step frontier
   std::queue<vertex> fifo;
   InEdgeIterator eo, eo_end;
   for(boost::tie(eo, eo_end) = boost::in_edges(current_v, *flow_graph); eo != eo_end; eo++)
   {
      auto v = boost::source(*eo, *flow_graph);
      if(operations.find(v) != operations.end())
      {
         fifo.push(v);
      }
   }
   while(!fifo.empty())
   {
      vertex current_op = fifo.front();
      fifo.pop();
      if(cs == schedule->get_cstep_end(current_op).second)
      {
         unsigned int from_fu_type = res_binding->get_assign(current_op);
         if((GET_TYPE(flow_graph, current_op) & TYPE_LOAD) &&
            HLS->allocation_information->is_direct_access_memory_unit(from_fu_type))
         {
            return true;
         }
         for(boost::tie(eo, eo_end) = boost::in_edges(current_op, *flow_graph); eo != eo_end; eo++)
         {
            auto v = boost::source(*eo, *flow_graph);
            if(operations.find(v) != operations.end())
            {
               fifo.push(v);
            }
         }
      }
   }
   return false;
}

bool parametric_list_based::check_LOAD_chaining(const CustomUnorderedSet<vertex>& operations, vertex current_v,
                                                const ControlStep cs, const ScheduleConstRef schedule) const
{
   /// compute the set of operations on the control step frontier
   std::queue<vertex> fifo;
   InEdgeIterator eo, eo_end;
   for(boost::tie(eo, eo_end) = boost::in_edges(current_v, *flow_graph); eo != eo_end; eo++)
   {
      auto v = boost::source(*eo, *flow_graph);
      if(operations.find(v) != operations.end())
      {
         fifo.push(v);
      }
   }
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
         {
            auto v = boost::source(*eo, *flow_graph);
            if(operations.find(v) != operations.end())
            {
               fifo.push(v);
            }
         }
      }
   }
   return false;
}

HLS_step::HLSRelationships
parametric_list_based::ComputeHLSRelationships(const DesignFlowStep::RelationshipType relationship_type) const
{
   HLSRelationships ret = Scheduling::ComputeHLSRelationships(relationship_type);
   switch(relationship_type)
   {
      case DEPENDENCE_RELATIONSHIP:
      {
#if HAVE_ILP_BUILT
         if(parameters->getOption<HLSFlowStep_Type>(OPT_scheduling_algorithm) == HLSFlowStep_Type::SDC_SCHEDULING
#if HAVE_FROM_PRAGMA_BUILT
            and HLSMgr->get_HLS(funId) and
            HLSMgr->CGetFunctionBehavior(funId)->CGetBehavioralHelper()->GetOmpForDegree() == 0
#endif
         )
         {
            ret.insert(std::make_tuple(HLSFlowStep_Type::SDC_SCHEDULING, HLSFlowStepSpecializationConstRef(),
                                       HLSFlowStep_Relationship::SAME_FUNCTION));
         }
#endif
         ret.insert(std::make_tuple(HLSFlowStep_Type::DOMINATOR_ALLOCATION, HLSFlowStepSpecializationConstRef(),
                                    HLSFlowStep_Relationship::WHOLE_APPLICATION));
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
