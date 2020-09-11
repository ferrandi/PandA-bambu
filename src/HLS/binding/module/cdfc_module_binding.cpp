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
 * @file cdfc_module_binding.cpp
 * @brief Class implementation of the module binding algorithm.
 * *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *
 */
#include "cdfc_module_binding.hpp"

#include "config_HAVE_EXPERIMENTAL.hpp"

#include <boost/filesystem/operations.hpp>

///. include
#include "Parameter.hpp"

/// algorithms/clique_covering include
#include "clique_covering.hpp"

/// behavior includes
#include "behavioral_writer_helper.hpp"
#include "function_behavior.hpp"
#include "op_graph.hpp"

/// boost include
#include <boost/range/adaptor/reversed.hpp>

/// Graph include
#include "graph.hpp"

/// HLS includes
#include "hls.hpp"
#include "hls_constraints.hpp"
#include "hls_manager.hpp"
#include "hls_target.hpp"

/// HLS/binding/interconnection
#include "mux_connection_binding.hpp"

/// HLS/binding/module include
#include "fu_binding.hpp"
#include "parallel_memory_fu_binding.hpp"

/// HLS/binding/module include
#include "module_binding_check.hpp"

/// HLS/binding/register include
#include "reg_binding.hpp"
/// HLS/binding/register/algorithms include
#include "weighted_clique_register.hpp"
/// required by register binding
#include "design_flow_manager.hpp"
#include "design_flow_step.hpp"
#include "hls_flow_step_factory.hpp"

/// HLS/binding/storage_value_insertion includes
#include "storage_value_information.hpp"
#include "storage_value_insertion.hpp"

/// HLS/chaining include
#include "chaining_information.hpp"

/// HLS/liveness include
#include "liveness.hpp"

/// HLS/memory include
#include "memory.hpp"

/// HLS/module_allocation include
#include "allocation_information.hpp"

/// HLS/scheduling include
#include "schedule.hpp"

/// HLS/stg include
#include "state_transition_graph_manager.hpp"

/// STD includes
#include <cmath>
#include <iosfwd>
#include <limits>
#include <string>

/// STL includes
#include "custom_map.hpp"
#include "custom_set.hpp"
#include <algorithm>
#include <deque>
#include <list>
#include <utility>
#include <vector>

/// technology include
#include "technology_manager.hpp"

/// technology/physical_library include
#include "technology_node.hpp"

/// technology/physical_library/models include
#include "time_model.hpp"

/// tree includes
#include "behavioral_helper.hpp"
#include "tree_helper.hpp"
#include "tree_manager.hpp"
#include "tree_node.hpp"

/// utility include
#include "cpu_time.hpp"
#include "dbgPrintHelper.hpp"
#include "hash_helper.hpp"

#ifdef HC_APPROACH
#include "hierarchical_clustering.hpp"
struct spec_hierarchical_clustering : public hierarchical_clustering<>
{
};
#endif
#include "string_manipulation.hpp" // for GET_CLASS

#define TOOSMALL_NORMALIZED_RESOURCE_AREA 0.9
#define DEFAULT_SMALL_NORMALIZED_RESOURCE_AREA 1.0
#define MODULE_BINDING_MUX_MARGIN 1.0
#define DSP_MARGIN 1.0
#define CLOCK_MARGIN 0.97

template <typename OutputIterator>
struct topological_based_sorting_visitor : public boost::dfs_visitor<>
{
   topological_based_sorting_visitor(std::vector<vertex>& _c2s, const OpGraphConstRef _sdg, OutputIterator _iter) : m_iter(_iter), c2s(_c2s), sdg(_sdg)
   {
   }

   template <typename Vertex, typename Graph>
   void finish_vertex(const Vertex& u, Graph& g)
   {
      *m_iter++ = boost::get(boost::vertex_index, g, u);
   }
   template <typename Edge, typename Graph>
   void back_edge(const Edge& e, Graph& g)
   {
      std::cerr << GET_NAME(sdg, c2s[boost::get(boost::vertex_index, g, boost::source(e, g))]) << "(" << sdg->CGetOpNodeInfo(c2s[boost::get(boost::vertex_index, g, boost::source(e, g))])->GetOperation() << ")-"
                << GET_NAME(sdg, c2s[boost::get(boost::vertex_index, g, boost::target(e, g))]) << "(" << sdg->CGetOpNodeInfo(c2s[boost::get(boost::vertex_index, g, boost::target(e, g))])->GetOperation() << ")" << std::endl;
      BOOST_THROW_EXCEPTION(boost::not_a_dag());
   }

   OutputIterator m_iter;
   std::vector<vertex>& c2s;
   const OpGraphConstRef sdg;
};

template <typename VertexListGraph, typename OutputIterator, typename P, typename T, typename R>
void topological_based_sorting(const VertexListGraph& g, std::vector<vertex>& c2s, const OpGraphConstRef sdg, OutputIterator result, const boost::bgl_named_params<P, T, R>& params)
{
   typedef topological_based_sorting_visitor<OutputIterator> TopoVisitor;
   boost::depth_first_search(g, params.visitor(TopoVisitor(c2s, sdg, result)));
}

template <typename VertexListGraph, typename OutputIterator>
void topological_based_sorting(const VertexListGraph& g, std::vector<vertex>& c2s, const OpGraphConstRef sdg, OutputIterator result)
{
   topological_based_sorting(g, c2s, sdg, result, boost::bgl_named_params<int, boost::buffer_param_t>(0));
}

/**
 * Functor used to compare which of two resources has to be considered first during the binding
 */
struct cdfc_resource_ordering_functor
{
 private:
   /// copy of the order: control step associated with the vertices.
   const AllocationInformationConstRef allocation_information;

 public:
   /**
    * functor function used to compare two resources with respect to their area
    * @param a is the first vertex
    * @param bis the second vertex
    * @return true when a is faster than b
    */
   bool operator()(const unsigned int& a, const unsigned int& b) const
   {
      unsigned char wm_a = (allocation_information->is_direct_access_memory_unit(a) || allocation_information->is_indirect_access_memory_unit(a)) ? 1 : 0;
      unsigned char wm_b = (allocation_information->is_direct_access_memory_unit(b) || allocation_information->is_indirect_access_memory_unit(b)) ? 1 : 0;
      double wd_a = allocation_information->get_DSPs(a);
      double wd_b = allocation_information->get_DSPs(b);
      double wa_a = allocation_information->get_area(a);
      double wa_b = allocation_information->get_area(b);
      return ((wm_a > wm_b) || (wm_a == wm_b && wd_a > wd_b) || (wm_a == wm_b && wd_a == wd_b && wa_a > wa_b) || (wm_a == wm_b && wd_a == wd_b && wa_a == wa_b && a < b));
   }

   /**
    * Constructor
    * @param o is the order.
    */
   explicit cdfc_resource_ordering_functor(const AllocationInformationConstRef _allocation_soluton) : allocation_information(_allocation_soluton)
   {
   }

   /**
    * Destructor
    */
   ~cdfc_resource_ordering_functor() = default;
};

CDFCModuleBindingSpecialization::CDFCModuleBindingSpecialization(const CliqueCovering_Algorithm _clique_covering_algorithm) : clique_covering_algorithm(_clique_covering_algorithm)
{
}

const std::string CDFCModuleBindingSpecialization::GetKindText() const
{
   return CliqueCovering_AlgorithmToString(clique_covering_algorithm);
}

const std::string CDFCModuleBindingSpecialization::GetSignature() const
{
   return STR(static_cast<unsigned int>(clique_covering_algorithm));
}

void cdfc_module_binding::initialize_connection_relation(connection_relation& con_rel, OpVertexSet& all_candidate_vertices)
{
   const tree_managerRef TreeM = HLSMgr->get_tree_manager();
   const FunctionBehaviorConstRef FB = HLSMgr->CGetFunctionBehavior(funId);
   const OpGraphConstRef data = FB->CGetOpGraph(FunctionBehavior::FDFG);
   for(auto current_v : all_candidate_vertices)
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Considering operation " + GET_NAME(data, current_v) + " " + STR(TreeM->CGetTreeNode(data->CGetOpNodeInfo(current_v)->GetNodeId())));
      std::vector<HLS_manager::io_binding_type> vars_read = HLSMgr->get_required_values(HLS->functionId, current_v);
      size_t n_ports = vars_read.size();
      con_rel[current_v].resize(n_ports);
      for(unsigned int port_index = 0; port_index < n_ports; ++port_index)
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Considering port " + STR(port_index));
         unsigned int tree_var = std::get<0>(vars_read[port_index]);
         if(tree_var != 0)
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---" + STR(TreeM->CGetTreeNode(tree_var)));
            CustomOrderedSet<std::pair<conn_code, std::pair<unsigned int, vertex>>>& con_rel_per_vertex_per_port_index = con_rel[current_v][port_index];
            const CustomOrderedSet<vertex>& running_states = HLS->Rliv->get_state_where_run(current_v);
            const CustomOrderedSet<vertex>::const_iterator rs_it_end = running_states.end();
            for(auto rs_it = running_states.begin(); rs_it != rs_it_end; ++rs_it)
            {
               vertex state = *rs_it;
               if(tree_helper::is_parameter(TreeM, tree_var) || !HLS->Rliv->has_op_where_defined(tree_var))
               {
                  con_rel_per_vertex_per_port_index.insert(std::make_pair(no_def, std::make_pair(tree_var, NULL_VERTEX)));
               }
               else
               {
                  vertex def_op = HLS->Rliv->get_op_where_defined(tree_var);
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Variable is defined in " + GET_NAME(data, def_op));
                  const CustomOrderedSet<vertex>& def_op_ending_states = HLS->Rliv->get_state_where_end(def_op);
                  if((GET_TYPE(data, def_op) & TYPE_PHI) == 0)
                  {
                     if(def_op_ending_states.find(state) != def_op_ending_states.end())
                     {
                        con_rel_per_vertex_per_port_index.insert(std::make_pair(no_phi_chained, std::make_pair(tree_var, def_op)));
                     }
                     else if(HLS->storage_value_information->is_a_storage_value(state, tree_var))
                     {
                        unsigned int storage_value = HLS->storage_value_information->get_storage_value_index(state, tree_var);
                        con_rel_per_vertex_per_port_index.insert(std::make_pair(no_phi_no_chained, std::make_pair(storage_value, def_op)));
                     }
                     else
                        THROW_UNREACHABLE("unexpected");
                  }
                  else
                  {
                     unsigned int storage_value = HLS->storage_value_information->get_storage_value_index(state, tree_var);
                     THROW_ASSERT(HLS->storage_value_information->is_a_storage_value(state, tree_var), "unxpected case");
                     con_rel_per_vertex_per_port_index.insert(std::make_pair(phi, std::make_pair(storage_value, def_op)));
                  }
               }
            }
         }
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Considered port " + STR(port_index));
      }
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Considered operation " + GET_NAME(data, current_v));
   }
}

template <bool do_estimation, bool do_conversion, typename vertex_type, class cluster_type, bool IS_DEBUGGING = true>
void estimate_muxes(const connection_relation& con_rel, unsigned int mux_prec, double& tot_mux_delay, double& tot_mux_area, const cluster_type& cluster, unsigned int& total_muxes, unsigned int& n_shared,
                    const typename std::map<vertex_type, vertex>& converter, const HLS_managerRef HLSMgr, const hlsRef HLS,
                    int
#ifndef NDEBUG
                        debug_level
#endif
)
{
   const fu_bindingRef fu = HLS->Rfu;
   const FunctionBehaviorConstRef FB = HLSMgr->CGetFunctionBehavior(HLS->functionId);
   const OpGraphConstRef data = FB->CGetOpGraph(FunctionBehavior::FDFG);
   bool has_register_done = HLS->Rreg && HLS->Rreg->size() != 0;
   std::vector<CustomOrderedSet<unsigned int>> regs_in;
   std::vector<CustomOrderedSet<unsigned int>> chained_in;
   std::vector<CustomOrderedSet<std::pair<unsigned int, unsigned int>>> module_in;
   std::vector<CustomOrderedSet<std::pair<unsigned int, unsigned int>>> module_in_reg;
   CustomOrderedSet<unsigned int> regs_out;
   CustomOrderedSet<vertex> chained_out;
   CustomOrderedSet<std::pair<unsigned int, unsigned int>> module_out;
   unsigned int n_tot_outgoing_edges = 0, n_tot_outgoing_unbound_operations = 0, n_tot_shared = 0;
   unsigned int max_port_index = 0;
   for(auto cv : cluster)
   {
      vertex current_v;
      if(std::is_same<vertex, vertex_type>::value && !do_conversion)
         current_v = reinterpret_cast<vertex>(cv);
      else
         current_v = converter.find(cv)->second;
      max_port_index = std::max(max_port_index, static_cast<unsigned int>(con_rel.find(current_v)->second.size()));
   }
   regs_in.resize(max_port_index);
   chained_in.resize(max_port_index);
   module_in.resize(max_port_index);
   module_in_reg.resize(max_port_index);

   for(auto cv : cluster)
   {
      vertex current_v;
      if(std::is_same<vertex, vertex_type>::value && !do_conversion)
         current_v = reinterpret_cast<vertex>(cv);
      else
         current_v = converter.find(cv)->second;
      THROW_ASSERT(con_rel.find(current_v) != con_rel.end(), "missing vertex from con_rel data structure");
      for(unsigned int port_index_actual = 0; port_index_actual < con_rel.find(current_v)->second.size(); ++port_index_actual)
      {
         unsigned int port_index = port_index_actual;
         const CustomOrderedSet<std::pair<conn_code, std::pair<unsigned int, vertex>>>& con_rel_per_vertex_per_port_index = con_rel.find(current_v)->second[port_index];
         if(fu->get_ports_are_swapped(current_v))
         {
            if(port_index_actual == 0)
               port_index = 1;
            else
               port_index = 0;
         }
         for(auto triple : con_rel_per_vertex_per_port_index)
         {
            switch(triple.first)
            {
               case no_def:
               {
                  unsigned int tree_var = triple.second.first;
                  chained_in[port_index].insert(tree_var); /// it is not chained but from the mux binding it counts as input to the mux tree
                  break;
               }
               case no_phi_chained:
               {
                  unsigned int tree_var = triple.second.first;
                  vertex def_op = triple.second.second;
                  if(fu->get_index(def_op) != INFINITE_UINT)
                     module_in[port_index].insert(std::make_pair(fu->get_assign(def_op), fu->get_index(def_op)));
                  else
                     chained_in[port_index].insert(tree_var);
                  break;
               }
               case no_phi_no_chained:
               {
                  unsigned int storage_value = triple.second.first;
                  vertex def_op = triple.second.second;
                  if(has_register_done)
                     regs_in[port_index].insert(HLS->Rreg->get_register(storage_value));
                  else if(fu->get_index(def_op) != INFINITE_UINT)
                     module_in_reg[port_index].insert(std::make_pair(fu->get_assign(def_op), fu->get_index(def_op)));
                  else
                     regs_in[port_index].insert(storage_value);
                  break;
               }
               case phi:
               {
                  unsigned int storage_value = triple.second.first;
                  vertex def_op = triple.second.second;
                  if(has_register_done)
                     regs_in[port_index].insert(HLS->Rreg->get_register(storage_value));
                  else if(fu->get_index(def_op) != INFINITE_UINT)
                     module_in_reg[port_index].insert(std::make_pair(fu->get_assign(def_op), fu->get_index(def_op)));
                  else
                     regs_in[port_index].insert(storage_value);
                  break;
               }
               default:
               {
                  THROW_ERROR("unexpected connection code");
               }
            }
         }
      }

      if(IS_DEBUGGING && has_register_done)
      {
         unsigned int var_written = HLSMgr->get_produced_value(HLS->functionId, current_v);
         if(var_written)
         {
            const CustomOrderedSet<vertex>& end = HLS->Rliv->get_state_where_end(current_v);
            const CustomOrderedSet<vertex>::const_iterator e_it_end = end.end();
            for(auto e_it = end.begin(); e_it != e_it_end; ++e_it)
            {
               vertex state = *e_it;
               if(HLS->storage_value_information->is_a_storage_value(state, var_written))
               {
                  regs_out.insert(HLS->Rreg->get_register(HLS->storage_value_information->get_storage_value_index(state, var_written)));
               }
               else
                  chained_out.insert(state);
            }
         }
      }
      for(const auto& oe : data->CGetOutEdges(current_v))
      {
         if((data->GetSelector(oe) & (DFG_SCA_SELECTOR | FB_DFG_SCA_SELECTOR)))
         {
            if(IS_DEBUGGING)
               ++n_tot_outgoing_edges;
            vertex tgt = boost::target(oe, *data);
            if(fu->get_index(tgt) != INFINITE_UINT)
            {
               if(module_out.find(std::make_pair(fu->get_assign(tgt), fu->get_index(tgt))) != module_out.end())
                  ++n_tot_shared;
               else
                  module_out.insert(std::make_pair(fu->get_assign(tgt), fu->get_index(tgt)));
            }
            else if(IS_DEBUGGING)
               ++n_tot_outgoing_unbound_operations;
         }
      }
   }

   /// compute the maximum number of mux ins
   total_muxes = 0;
   tot_mux_area = 0;
   tot_mux_delay = 0;
   for(unsigned int port_index = 0; port_index < max_port_index; ++port_index)
   {
      if(IS_DEBUGGING)
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Ports " + STR(port_index));
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Reg in ports: " + STR(regs_in[port_index].size()));
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Chained in ports: " + STR(chained_in[port_index].size()));
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Module in ports: " + STR(module_in[port_index].size()));
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Module in reg ports: " + STR(module_in_reg[port_index].size()));
      }
      unsigned int current_muxs = 0;
      current_muxs += static_cast<unsigned int>(regs_in[port_index].size());
      current_muxs += static_cast<unsigned int>(chained_in[port_index].size());
      current_muxs += static_cast<unsigned int>(module_in[port_index].size());
      current_muxs += static_cast<unsigned int>(module_in_reg[port_index].size());
      if(current_muxs > 1)
      {
         total_muxes += current_muxs - 1;
      }
      if(do_estimation)
      {
         tot_mux_area += HLS->allocation_information->estimate_muxNto1_area(mux_prec, current_muxs);
         tot_mux_delay = std::max(tot_mux_delay, HLS->allocation_information->estimate_muxNto1_delay(mux_prec, current_muxs));
      }
   }
   if(IS_DEBUGGING)
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---total number of outgoing edges: " + STR(n_tot_outgoing_edges));
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---total number of outgoing unbound operations: " + STR(n_tot_outgoing_unbound_operations));
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---total number of outgoing bound modules: " + STR(module_out.size()));
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---total number of modules shared: " + STR(n_tot_shared));
      if(has_register_done)
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Reg out ports: " + STR(regs_out.size()));
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Chained out ports: " + STR(chained_out.size()));
      }
   }
   n_shared = n_tot_shared;
}

struct slack_based_filtering : public filter_clique<vertex>
{
   slack_based_filtering(const CustomUnorderedMap<vertex, double>& _slack_time, const CustomUnorderedMap<vertex, double>& _starting_time, double _controller_delay, unsigned int _mux_prec, const hlsRef _HLS, const HLS_managerRef _HLSMgr,
                         const double _area_resource, const connection_relation& _con_rel)
       : slack_time(_slack_time),
         starting_time(_starting_time),
         controller_delay(_controller_delay),
         mux_prec(_mux_prec),
         HLS(_HLS),
         HLSMgr(_HLSMgr),
         data(_HLSMgr->CGetFunctionBehavior(_HLS->functionId)->CGetOpGraph(FunctionBehavior::FDFG)),
         area_resource(_area_resource),
         con_rel(_con_rel)
   {
   }

   bool select_candidate_to_remove(const CustomOrderedSet<C_vertex>& candidate_clique, C_vertex& v, const std::map<C_vertex, vertex>& converter, const cc_compatibility_graph& cg) const override
   {
      THROW_ASSERT(!candidate_clique.empty(), "candidate clique cannot be empty");
      double min_slack = std::numeric_limits<double>::max();
      C_vertex min_slack_vertex = *candidate_clique.begin();
      vertex current_v;
      unsigned int total_muxes;
      unsigned int n_shared;
      double mux_area_estimation, mux_time_estimation;
      estimate_muxes<true, true, C_vertex>(con_rel, mux_prec, mux_time_estimation, mux_area_estimation, candidate_clique, total_muxes, n_shared, converter, HLSMgr, HLS, HLS->debug_level);

      double max_starting_time = 0.0;
      for(auto current_c : candidate_clique)
      {
         current_v = converter.find(current_c)->second;
         max_starting_time = std::max(max_starting_time, starting_time.find(current_v)->second);
      }
      if(max_starting_time < controller_delay && total_muxes > 0)
         max_starting_time = controller_delay;
      auto vert_it_end = candidate_clique.end();
      for(auto vert_it = candidate_clique.begin(); vert_it != vert_it_end; ++vert_it)
      {
         THROW_ASSERT(converter.find(*vert_it) != converter.end(), "non-existing vertex");
         current_v = converter.find(*vert_it)->second;
         double curr_slack = slack_time.find(current_v)->second - (max_starting_time - starting_time.find(current_v)->second);

         // THROW_ASSERT(curr_slack>=0.0, "negative slack not allowed");
         // std::cerr << "Current slack " << curr_slack << " for vertex " << GET_NAME(data, current_v) << std::endl;
         if(curr_slack < min_slack)
         {
            min_slack_vertex = *vert_it;
            min_slack = curr_slack;
         }
         else if(curr_slack == min_slack && min_slack_vertex != *vert_it)
         {
            int weight1 = 0;
            boost::graph_traits<cc_compatibility_graph>::out_edge_iterator ei, ei_end;
            boost::tie(ei, ei_end) = boost::out_edges(*vert_it, cg);
            for(; ei != ei_end; ++ei)
               if(candidate_clique.find(boost::target(*ei, cg)) != candidate_clique.end())
                  weight1 += cg[*ei].weight;
            int weight2 = 0;
            boost::tie(ei, ei_end) = boost::out_edges(min_slack_vertex, cg);
            for(; ei != ei_end; ++ei)
               if(candidate_clique.find(boost::target(*ei, cg)) != candidate_clique.end())
                  weight2 += cg[*ei].weight;

            if(weight1 < weight2)
               min_slack_vertex = *vert_it;
         }
      }
      // std::cerr << "Min_slack " << min_slack << " mux_delay " << mux_delay << std::endl;
      /// special case
      if(total_muxes > 0 && min_slack < 0)
      {
         v = min_slack_vertex;
         // std::cerr << "Removed0 " << GET_NAME(data, converter.find(v)->second) << " Slack "<< min_slack << " mux_delay " << mux_delay << std::endl;
         return true;
      }
      /// we accept solutions sharing resources without introducing muxes

      if(total_muxes > 0 && ((mux_area_estimation + area_resource) >= (static_cast<double>(candidate_clique.size() + (total_muxes <= n_shared ? n_shared : 0)) * area_resource)))
      {
         v = min_slack_vertex;
         // std::cerr << "Removed1 " << GET_NAME(data, converter.find(v)->second) << " Slack "<< min_slack << " mux_delay " << mux_delay << std::endl;
         return true;
      }

      if(total_muxes > 0 && mux_time_estimation > min_slack)
      {
         v = min_slack_vertex;
         // std::cerr << "Removed " << GET_NAME(data, converter.find(v)->second) << " Slack "<< min_slack << " levels " << levels_in << " mux_delay " << mux_delay << std::endl;
         return true;
      }
      else
      {
         // std::cerr << "No vertex removed " << " Slack "<< min_slack << " levels " << levels_in << " mux_delay " << mux_delay  << std::endl;
         return false;
      }
   }

   size_t clique_cost(const CustomOrderedSet<C_vertex>& candidate_clique, const std::map<C_vertex, vertex>& converter) const
   {
      unsigned int total_muxes;
      unsigned int n_shared;
      double mux_area_estimation, mux_time_estimation;
      estimate_muxes<false, true, C_vertex>(con_rel, mux_prec, mux_time_estimation, mux_area_estimation, candidate_clique, total_muxes, n_shared, converter, HLSMgr, HLS, HLS->debug_level);
      return static_cast<size_t>(total_muxes);
   }

 private:
   const CustomUnorderedMap<vertex, double>& slack_time;
   const CustomUnorderedMap<vertex, double>& starting_time;
   double controller_delay;
   const unsigned int mux_prec;
   const hlsRef HLS;
   const HLS_managerRef HLSMgr;
   const OpGraphConstRef data;
   const double area_resource;
   const connection_relation& con_rel;
};

CdfcEdgeInfo::CdfcEdgeInfo(const int _edge_weight) : edge_weight(_edge_weight)
{
}

class CdfcWriter : public VertexWriter
{
 private:
   /// The info associated with the graph to be printed
   const CdfcGraphInfo* cdfc_graph_info;

   const std::map<vertex, vertex>& c2s;

   /// The functor used to print labels which correspond to vertices of the graph to be printed
   const OpWriter operation_writer;

 public:
   /**
    * Constructor
    * @param _printing_graph is the graph to be printed
    */
   explicit CdfcWriter(const CdfcGraph* _printing_graph)
       : VertexWriter(_printing_graph, 0), cdfc_graph_info(GetPointer<const CdfcGraphInfo>(_printing_graph->CGetGraphInfo())), c2s(cdfc_graph_info->c2s), operation_writer(cdfc_graph_info->operation_graph.get(), 0)
   {
   }

   /**
    * Operator used to print the label of a vertex
    * @param out is the stream where label has to printed
    * @param v is the vertex to be printed
    */
   void operator()(std::ostream& out, const vertex& v) const override
   {
      operation_writer(out, c2s.find(v)->second);
   }
};

class CdfcEdgeWriter : public EdgeWriter
{
 public:
   /**
    * Constructor
    * @param _printing_graph is the graph to be printed
    * @param _selector is the selector of the graph to be printed
    */
   explicit CdfcEdgeWriter(const CdfcGraph* _printing_graph) : EdgeWriter(_printing_graph, 0)
   {
   }

   /**
    * Operator which print label of an EdgeDescriptor
    * @param out is the stream
    * @param e is the edge
    */
   void operator()(std::ostream& out, const EdgeDescriptor& e) const override
   {
      const auto* edge_info = Cget_edge_info<CdfcEdgeInfo>(e, *printing_graph);
      if(edge_info)
         out << "[label=\"" << edge_info->edge_weight << "\"]";
      else
         out << "[color=red3]";
   }
};

CdfcGraphInfo::CdfcGraphInfo(const std::map<vertex, vertex>& _c2s, const OpGraphConstRef _operation_graph) : c2s(_c2s), operation_graph(_operation_graph)
{
}

CdfcGraphsCollection::CdfcGraphsCollection(const CdfcGraphInfoRef cdfc_graph_info, const ParameterConstRef _parameters) : graphs_collection(RefcountCast<GraphInfo>(cdfc_graph_info), _parameters)
{
}

CdfcGraphsCollection::~CdfcGraphsCollection() = default;

CdfcGraph::CdfcGraph(const CdfcGraphsCollectionRef cdfc_graphs_collection, const int _selector) : graph(cdfc_graphs_collection.get(), _selector)
{
}

CdfcGraph::CdfcGraph(const CdfcGraphsCollectionRef cdfc_graphs_collection, const int _selector, const CustomUnorderedSet<vertex>& vertices) : graph(cdfc_graphs_collection.get(), _selector, vertices)
{
}

CdfcGraph::~CdfcGraph() = default;

void CdfcGraph::WriteDot(const std::string& file_name, const int) const
{
   const auto* cdfc_graph_info = GetPointer<const CdfcGraphInfo>(CGetGraphInfo());
   const BehavioralHelperConstRef behavioral_helper = cdfc_graph_info->operation_graph->CGetOpGraphInfo()->BH;
   const std::string output_directory = collection->parameters->getOption<std::string>(OPT_dot_directory) + "/" + behavioral_helper->get_function_name() + "/";
   if(!boost::filesystem::exists(output_directory))
      boost::filesystem::create_directories(output_directory);
   const std::string full_name = output_directory + file_name;
   const VertexWriterConstRef cdfc_writer(new CdfcWriter(this));
   const EdgeWriterConstRef cdfc_edge_writer(new CdfcEdgeWriter(this));
   InternalWriteDot<const CdfcWriter, const CdfcEdgeWriter>(full_name, cdfc_writer, cdfc_edge_writer);
}

cdfc_module_binding::cdfc_module_binding(const ParameterConstRef _parameters, const HLS_managerRef _HLSMgr, unsigned int _funId, const DesignFlowManagerConstRef _design_flow_manager, const HLSFlowStepSpecializationConstRef _hls_flow_step_specialization)
    : fu_binding_creator(_parameters, _HLSMgr, _funId, _design_flow_manager, HLSFlowStep_Type::CDFC_MODULE_BINDING,
                         _hls_flow_step_specialization ? _hls_flow_step_specialization : HLSFlowStepSpecializationConstRef(new CDFCModuleBindingSpecialization(_parameters->getOption<CliqueCovering_Algorithm>(OPT_cdfc_module_binding_algorithm)))),
      small_normalized_resource_area(_parameters->IsParameter("small_normalized_resource_area") ? _parameters->GetParameter<double>("small_normalized_resource_area") : DEFAULT_SMALL_NORMALIZED_RESOURCE_AREA)
{
   debug_level = _parameters->get_class_debug_level(GET_CLASS(*this));
}

cdfc_module_binding::~cdfc_module_binding() = default;

void cdfc_module_binding::update_slack_starting_time(const OpGraphConstRef fdfg, OpVertexSet& sorted_vertices, CustomUnorderedMap<vertex, double>& slack_time, CustomUnorderedMap<vertex, double>& starting_time, bool update_starting_time, bool only_backward,
                                                     bool only_forward)
{
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Updating slack starting time");
   while(!sorted_vertices.empty())
   {
      vertex curr_vertex = *sorted_vertices.begin();
      INDENT_OUT_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Considering " + GET_NAME(fdfg, curr_vertex));
      sorted_vertices.erase(curr_vertex);
      double current_budget = slack_time[curr_vertex];
      double new_current_budget = current_budget;
      if(!only_forward)
      {
         for(const auto& ie : fdfg->CGetInEdges(curr_vertex))
         {
            if(fdfg->GetSelector(ie) & DFG_SCA_SELECTOR)
            {
               vertex src = boost::source(ie, *fdfg);
               if(HLS->chaining_information->may_be_chained_ops(curr_vertex, src) && HLS->chaining_information->get_representative_in(src) == HLS->chaining_information->get_representative_in(curr_vertex))
               {
                  if(slack_time[src] > new_current_budget)
                  {
                     slack_time[src] = new_current_budget;
                     INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Reducing slack time of " + GET_NAME(fdfg, src) + " to " + STR(slack_time[src]) + " because of " + GET_NAME(fdfg, curr_vertex));
                     sorted_vertices.insert(src);
                  }
                  else if(slack_time[src] < new_current_budget)
                     new_current_budget = slack_time[src];
               }
            }
         }
      }
      if(!only_backward)
      {
         for(const auto& oe : fdfg->CGetOutEdges(curr_vertex))
         {
            if(fdfg->GetSelector(oe) & DFG_SCA_SELECTOR)
            {
               vertex tgt = boost::target(oe, *fdfg);
               if(HLS->chaining_information->may_be_chained_ops(curr_vertex, tgt))
               {
                  if(HLS->chaining_information->get_representative_in(curr_vertex) == HLS->chaining_information->get_representative_in(tgt))
                  {
                     if(slack_time[tgt] > new_current_budget)
                     {
                        if(update_starting_time)
                        {
                           starting_time[tgt] += slack_time[tgt] - new_current_budget;
                           INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Updating starting time of " + GET_NAME(fdfg, tgt) + " to " + STR(starting_time[tgt]) + " because of " + GET_NAME(fdfg, curr_vertex));
                        }
                        slack_time[tgt] = new_current_budget;
                        INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Reducing slack time of " + GET_NAME(fdfg, tgt) + " to " + STR(slack_time[tgt]) + " because of " + GET_NAME(fdfg, curr_vertex));
                        sorted_vertices.insert(tgt);
                     }
                     else if(slack_time[tgt] < new_current_budget)
                        new_current_budget = slack_time[tgt];
                  }
               }
            }
         }
      }
      if(new_current_budget < current_budget)
      {
         sorted_vertices.insert(curr_vertex);
         slack_time[curr_vertex] = new_current_budget;
      }
      INDENT_OUT_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--");
   }
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Updated slack starting time");
}

#ifdef HC_APPROACH
static vertex get_src_vertex(unsigned int var_written, vertex tgt, const HLS_managerRef HLSMgr, unsigned int functionId, const OpGraphConstRef fsdg)
{
   InEdgeIterator ie, ie_end;
   for(boost::tie(ie, ie_end) = boost::in_edges(tgt, *fsdg); ie != ie_end; ++ie)
   {
      vertex src = boost::source(*ie, *fsdg);
      unsigned int vw = HLSMgr->get_produced_value(functionId, src);
      if(var_written == vw)
         return src;
   }
   return NULL_VERTEX;
}
#endif

static inline bool compute_condition1(const std::string& lib_name, const AllocationInformationConstRef allocation_information, double local_mux_time, unsigned int fu_s1)
{
   bool cond1 = local_mux_time > 0 && lib_name != WORK_LIBRARY && lib_name != PROXY_LIBRARY && allocation_information->get_number_channels(fu_s1) < 1 && allocation_information->get_worst_number_of_cycles(fu_s1) <= 1 &&
                allocation_information->get_number_fu(fu_s1) == INFINITE_UINT;

   return cond1;
}

static inline bool compute_condition2(bool cond1, unsigned int fu_prec, double resource_area, const double small_normalized_resource_area)
{
   bool cond2 = cond1 && (fu_prec <= 8 || resource_area <= small_normalized_resource_area);

   return cond2;
}

DesignFlowStep_Status cdfc_module_binding::InternalExec()
{
   long step_time = 0;
   if(output_level >= OUTPUT_LEVEL_MINIMUM and output_level <= OUTPUT_LEVEL_PEDANTIC)
      START_TIME(step_time);
   const tree_managerRef TreeM = HLSMgr->get_tree_manager();
   HLS->Rliv->set_HLS(HLS);

   // resource binding and allocation  info
   fu_bindingRef fu = HLS->Rfu;
   const AllocationInformationConstRef allocation_information = HLS->allocation_information;

   cdfc_resource_ordering_functor r_functor(allocation_information);
   double setup_hold_time = allocation_information->get_setup_hold_time();

   // pointer to a Control, Data dependence and anti-dependence graph graph
   const FunctionBehaviorConstRef FB = HLSMgr->CGetFunctionBehavior(funId);
   const OpGraphConstRef sdg = FB->CGetOpGraph(FunctionBehavior::SDG);
#ifdef HC_APPROACH
   const OpGraphConstRef fsdg = FB->CGetOpGraph(FunctionBehavior::FSDG);
#endif
   const OpGraphConstRef fdfg = FB->CGetOpGraph(FunctionBehavior::FDFG);
   size_t total_modules_allocated = 0;
   double total_resource_area = 0, total_DSPs = 0;
   double total_area_muxes = 0;

   unsigned int fu_unit;
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Computing non-shared resources");
   /// compute non-shared resources
   std::map<unsigned int, unsigned int> n_shared_fu;
   // HLS->Rliv->compute_conflicts_with_reachability(HLS);
   for(const auto operation : fdfg->CGetOperations())
   {
      fu_unit = fu->get_assign(operation);
      if(allocation_information->is_vertex_bounded(fu_unit))
         continue;
      if(n_shared_fu.find(fu_unit) == n_shared_fu.end())
         n_shared_fu[fu_unit] = 1;
      else
         n_shared_fu[fu_unit] = 1 + n_shared_fu[fu_unit];
   }
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Computed non-shared resources");
   /// check easy binding and compute the list of vertices for which a sharing is possible
   std::map<unsigned int, OpVertexSet, cdfc_resource_ordering_functor> candidate_vertices(r_functor);
   OpVertexSet all_candidate_vertices(fdfg);
   CustomUnorderedMap<unsigned int, CustomOrderedSet<vertex>> easy_bound_vertices;
   for(const auto operation : fdfg->CGetOperations())
   {
      fu_unit = fu->get_assign(operation);
      if(fu->get_index(operation) != INFINITE_UINT)
      {
         ++total_modules_allocated;
         total_resource_area += allocation_information->get_area(fu_unit);
         total_DSPs += allocation_information->get_DSPs(fu_unit);
         if(!allocation_information->is_vertex_bounded(fu_unit) && n_shared_fu.find(fu_unit)->second != 1)
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Easy binding for -> " + GET_NAME(sdg, operation) + "-" + sdg->CGetOpNodeInfo(operation)->GetOperation() + "(" + allocation_information->get_string_name(fu_unit) + ")");
            easy_bound_vertices[fu_unit].insert(operation);
         }
         else
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Non-easy binding for -> " + GET_NAME(sdg, operation) + "-" + sdg->CGetOpNodeInfo(operation)->GetOperation() + "(" + allocation_information->get_string_name(fu_unit) + ")");
         }
      }
      else
      {
         if(allocation_information->is_vertex_bounded(fu_unit) ||
            (allocation_information->is_memory_unit(fu_unit) &&
             (!allocation_information->is_readonly_memory_unit(fu_unit) || (!allocation_information->is_one_cycle_direct_access_memory_unit(fu_unit) && (!parameters->isOption(OPT_rom_duplication) || !parameters->getOption<bool>(OPT_rom_duplication)))) &&
             allocation_information->get_number_channels(fu_unit) == 1) ||
            n_shared_fu.find(fu_unit)->second == 1)
         {
            ++total_modules_allocated;
            total_resource_area += allocation_information->get_area(fu_unit);
            total_DSPs += allocation_information->get_DSPs(fu_unit);
            fu->bind(operation, fu_unit, 0);
            if(allocation_information->is_memory_unit(fu_unit))
            {
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Easy binding for -> " + GET_NAME(sdg, operation) + "-" + sdg->CGetOpNodeInfo(operation)->GetOperation() + "(" + allocation_information->get_string_name(fu_unit) + ")");
               easy_bound_vertices[fu_unit].insert(operation);
            }
            else
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Non-easy binding for -> " + GET_NAME(sdg, operation) + "-" + sdg->CGetOpNodeInfo(operation)->GetOperation() + "(" + allocation_information->get_string_name(fu_unit) + ")");
         }
         else
         {
            if(candidate_vertices.find(fu_unit) == candidate_vertices.end())
               candidate_vertices.insert(std::make_pair(fu_unit, OpVertexSet(sdg)));
            candidate_vertices.find(fu_unit)->second.insert(operation);
            all_candidate_vertices.insert(operation);
         }
      }
   }
   long clique_cputime = 0;
   long falseloop_cputime = 0;
   long weight_cputime = 0;
   long slack_cputime = 0;
   CustomMap<unsigned int, long> clique_iteration_cputime;
   bool clique_covering_executed = false;
   unsigned int n_performance_conflicts = 0;

   /// in case no vertices was left we have done
   if(!candidate_vertices.empty())
   {
      std::vector<vertex> c2s;
      c2s.reserve(boost::num_vertices(*fdfg));
      std::map<vertex, cdfc_vertex> s2c;

      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Creating the cdfc for the module binding...");
      connection_relation con_rel;
      initialize_connection_relation(con_rel, all_candidate_vertices);
      boost_cdfc_graphRef cdfc_bulk_graph = boost_cdfc_graphRef(new boost_cdfc_graph());
      if(output_level >= OUTPUT_LEVEL_MINIMUM)
         START_TIME(slack_cputime);
      // std::map<size_t,CustomOrderedSet<vertex> >chained_relation;
      std::list<vertex> sorted_vertices;
      sdg->TopologicalSort(sorted_vertices);
      CustomUnorderedMap<vertex, double> starting_time;
      CustomUnorderedMap<vertex, double> ending_time;
      CustomUnorderedMap<vertex, double> slack_time;
      double clock_period_resource_fraction = HLS->HLS_C->get_clock_period_resource_fraction();
      double actual_scheduling_clock_budget = HLS->HLS_C->get_clock_period() * clock_period_resource_fraction;
      const double clock_budget = CLOCK_MARGIN * actual_scheduling_clock_budget;
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---clock_budget=" + STR(clock_budget) + " setup_hold_time=" + STR(setup_hold_time));
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Computing starting times");
      for(const auto operation : sorted_vertices)
      {
         if(GET_TYPE(fdfg, operation) & (TYPE_ENTRY | TYPE_EXIT))
            starting_time[operation] = 0.0;
         else
         {
            const auto statement_index = fdfg->CGetOpNodeInfo(operation)->GetNodeId();
            starting_time[operation] = HLS->Rsch->GetStartingTime(statement_index) - (from_strongtype_cast<double>(HLS->Rsch->get_cstep(statement_index).second) * actual_scheduling_clock_budget);
         }
      }
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Computed starting times");
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Computing Latest ending times");
      for(const auto operation : boost::adaptors::reverse(sorted_vertices))
      {
         /// check for PHIs attached to the ouput. They may require one or more muxes.
         double delay = HLS->Rsch->get_fo_correction(fdfg->CGetOpNodeInfo(operation)->GetNodeId(), 0);
         double current_ending_time;
         if(GET_TYPE(fdfg, operation) & (TYPE_ENTRY | TYPE_EXIT))
            current_ending_time = 0;
         else
         {
            const auto statement_index = fdfg->CGetOpNodeInfo(operation)->GetNodeId();
            unsigned int fu_type = fu->get_assign(operation);
            const auto ii_time = allocation_information->get_initiation_time(fu_type, statement_index);
            const auto n_cycles = allocation_information->get_cycles(fu_type, statement_index);
            if(ii_time != (0u))
               current_ending_time = delay + HLS->Rsch->GetEndingTime(statement_index) - (from_strongtype_cast<double>(HLS->Rsch->get_cstep_end(statement_index).second) * actual_scheduling_clock_budget) + starting_time.find(operation)->second;
            else if(n_cycles > 1)
               current_ending_time = clock_budget - setup_hold_time;
            else
               current_ending_time = delay + HLS->Rsch->GetEndingTime(statement_index) - (from_strongtype_cast<double>(HLS->Rsch->get_cstep_end(statement_index).second) * actual_scheduling_clock_budget);
         }
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---current_ending_time for " + GET_NAME(sdg, operation) + "=" + STR(current_ending_time));
         double current_budget = clock_budget - current_ending_time - setup_hold_time;
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Initial Current_budget/Slack for " + GET_NAME(sdg, operation) + "=" + STR(current_budget));
         if(current_budget < 0.0)
            current_budget = 0.0;
         OutEdgeIterator oe, oe_end;
         for(boost::tie(oe, oe_end) = boost::out_edges(operation, *fdfg); oe != oe_end; ++oe)
         {
            if(fdfg->GetSelector(*oe) & DFG_SCA_SELECTOR)
            {
               vertex tgt = boost::target(*oe, *fdfg);
               if(HLS->chaining_information->may_be_chained_ops(operation, tgt))
               {
                  if(HLS->chaining_information->get_representative_in(operation) == HLS->chaining_information->get_representative_in(tgt))
                  {
                     current_ending_time = std::max(ending_time[tgt], current_ending_time);
                     current_budget = std::min(current_budget, slack_time[tgt]);
                  }
               }
            }
         }
         ending_time[operation] = current_ending_time;
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Current_budget/Slack for " + GET_NAME(sdg, operation) + "=" + STR(current_budget));
         slack_time[operation] = current_budget;
      }
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Computed Latest ending times and slacks");
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Starting time *** Latest ending time *** Slacks");
#ifndef NDEBUG
      for(const auto operation : sdg->CGetOperations())
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                        "---" + GET_NAME(sdg, operation) + " *** starting_time=" + STR(starting_time.find(operation)->second) + " *** latest_ending_time=" + STR(ending_time.find(operation)->second) +
                            " *** slack_time=" + STR(slack_time.find(operation)->second));
         THROW_ASSERT(ending_time.find(operation)->second >= starting_time.find(operation)->second, "wrong starting/ending for operation " + GET_NAME(sdg, operation));
      }
#endif
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--");
      OpVertexSet to_update(fdfg);
      to_update.insert(sorted_vertices.begin(), sorted_vertices.end());

      update_slack_starting_time(fdfg, to_update, slack_time, starting_time, false, false, false);

      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Updated Starting time *** Latest ending time *** Slacks");
#ifndef NDEBUG
      for(const auto operation : sdg->CGetOperations())
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                        "---" + GET_NAME(sdg, operation) + " *** starting_time=" + STR(starting_time.find(operation)->second) + " *** latest_ending_time=" + STR(ending_time.find(operation)->second) +
                            " *** slack_time=" + STR(slack_time.find(operation)->second));
      }
#endif
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--");

      if(output_level >= OUTPUT_LEVEL_MINIMUM)
         STOP_TIME(slack_cputime);

      boost::graph_traits<graph>::vertices_size_type n_vert = boost::num_vertices(*fdfg);
      std::vector<boost::graph_traits<OpGraph>::vertices_size_type> rank_map(n_vert);
      std::vector<vertex> pred_map(n_vert);
      typedef boost::property_map<OpGraph, boost::vertex_index_t>::const_type const_vertex_index_pmap_t;
      const_vertex_index_pmap_t cindex_pmap = boost::get(boost::vertex_index_t(), *fdfg);
      /// rank property map definition
      typedef boost::iterator_property_map<std::vector<boost::graph_traits<OpGraph>::vertices_size_type>::iterator, const_vertex_index_pmap_t> op_rank_pmap_type;
      op_rank_pmap_type rank_pmap = boost::make_iterator_property_map(rank_map.begin(), cindex_pmap, rank_map[0]);
      /// parent property map definition
      typedef boost::iterator_property_map<std::vector<vertex>::iterator, const_vertex_index_pmap_t> op_pred_pmap_type;
      op_pred_pmap_type pred_pmap = boost::make_iterator_property_map(pred_map.begin(), cindex_pmap, pred_map[0]);
      boost::disjoint_sets<op_rank_pmap_type, op_pred_pmap_type> ds(rank_pmap, pred_pmap);
      VertexIterator vi, vi_end;
      for(boost::tie(vi, vi_end) = boost::vertices(*fdfg); vi != vi_end; ++vi)
      {
         vertex s = *vi;
         ds.make_set(s);
      }

      /// merge easy bound vertices
      for(const auto& fu_eb : easy_bound_vertices)
      {
         std::map<unsigned int, vertex> rep_vertex;
         for(const auto& cur_v : fu_eb.second)
         {
            unsigned int vertex_index = fu->get_index(cur_v);
            if(rep_vertex.find(vertex_index) == rep_vertex.end())
               rep_vertex[vertex_index] = cur_v;
            else
            {
               vertex rep = rep_vertex.find(vertex_index)->second;
               ds.union_set(cur_v, rep);
               starting_time[cur_v] = starting_time[rep] = std::max(starting_time[rep], starting_time[cur_v]);
               ending_time[cur_v] = ending_time[rep] = std::max(ending_time[rep], ending_time[cur_v]);
            }
         }
      }

      /// add the vertices to the cdfc graph
      for(boost::tie(vi, vi_end) = boost::vertices(*fdfg); vi != vi_end; ++vi)
      {
         vertex s = *vi;
         vertex rep = ds.find_set(s);
         cdfc_vertex C;
         if(rep == s && s2c.find(rep) == s2c.end())
         {
            C = boost::add_vertex(*cdfc_bulk_graph);
            THROW_ASSERT(boost::get(boost::vertex_index, *cdfc_bulk_graph, C) == c2s.size(), "unexpected case");
            c2s.push_back(rep);
         }
         else if(s2c.find(rep) == s2c.end())
         {
            C = boost::add_vertex(*cdfc_bulk_graph);
            THROW_ASSERT(boost::get(boost::vertex_index, *cdfc_bulk_graph, C) == c2s.size(), "unexpected case");
            c2s.push_back(rep);
            s2c[rep] = C;
         }
         else
            C = s2c[rep];
         s2c[s] = C;
      }

      /// add the control dependencies edges and the chained edges to the cdfc graph
      const OpGraphConstRef dfg = FB->CGetOpGraph(FunctionBehavior::DFG);
      EdgeIterator ei, ei_end;
      for(boost::tie(ei, ei_end) = boost::edges(*sdg); ei != ei_end; ++ei)
      {
         vertex src = boost::source(*ei, *sdg);
         unsigned int fu_unit_src = fu->get_assign(src);
         const auto II_src = allocation_information->get_initiation_time(fu_unit_src, src);
         vertex tgt = boost::target(*ei, *sdg);
         if(HLS->chaining_information->may_be_chained_ops(tgt, src) /// only the chained operations are relevant
            && II_src == 0                                          /// pipelined operations break false loops
         )
         {
            cdfc_vertex cdfc_src = s2c[src];
            cdfc_vertex cdfc_tgt = s2c[tgt];
            if(cdfc_src != cdfc_tgt)
            {
               bool exists;
               cdfc_edge E;
               boost::tie(E, exists) = boost::edge(cdfc_src, cdfc_tgt, *cdfc_bulk_graph);
               // std::cerr << "Chained " << cdfc_src << "-" << cdfc_tgt << " -- " << GET_NAME(sdg, src)  << "-" << GET_NAME(sdg, tgt) << std::endl;
               if(!exists)
               {
                  boost::tie(E, exists) = boost::add_edge(cdfc_src, cdfc_tgt, edge_cdfc_selector(CD_EDGE), *cdfc_bulk_graph);
                  THROW_ASSERT(exists, "already inserted edge");
               }
            }
            else
               THROW_ERROR(std::string(GET_NAME(sdg, src)) + "(" + sdg->CGetOpNodeInfo(src)->GetOperation() + ")--" + GET_NAME(sdg, tgt) + "(" + sdg->CGetOpNodeInfo(tgt)->GetOperation() + ")");
         }
      }

      double clock_cycle = HLS->HLS_C->get_clock_period() * CLOCK_MARGIN * clock_period_resource_fraction;

#ifdef HC_APPROACH
      spec_hierarchical_clustering hc;

      /// add the vertices to the clustering graph graph
      for(const auto& fu_cv : candidate_vertices)
      {
         for(const auto& cv : fu_cv.second)
         {
            unsigned int fu_s1 = fu_cv.first;
            const double mux_time = MODULE_BINDING_MUX_MARGIN * allocation_information->estimate_mux_time(fu_s1);
            if(!can_be_clustered(cv, fsdg, fu, slack_time, mux_time))
            {
               // fu_s1 = fu->get_assign(cv);
               // double exec_time = allocation_information->get_worst_execution_time(fu_s1);
               // double stage_time = allocation_information->get_worst_stage_period(fu_s1);
               // double resource_area = allocation_information->compute_normalized_area(fu_s1, vars_read1.size());
               // std::cerr << "NOPERATION=" << fsdg->CGetOpNodeInfo(cv)->GetOperation() << " area " << resource_area  << "-" << has_a_constant_in << " e=" << exec_time << " s=" << stage_time << std::endl;
               continue;
            }
            double resource_area = allocation_information->compute_normalized_area(fu_s1);
            resource_area += allocation_information->get_DSPs(fu_s1);

            // double exec_time = allocation_information->get_worst_execution_time(fu_s1);
            // double stage_time = allocation_information->get_worst_stage_period(fu_s1);
            hc.add_vertex(boost::get(boost::vertex_index, *fsdg, cv), GET_NAME(fsdg, cv), fu->get_assign(cv), resource_area);
         }
      }
      /// add tabu for each pair of vertices in conflict: vertices concurrently running
      for(const auto& fu_cv : candidate_vertices)
      {
         const CustomOrderedSet<vertex>::const_iterator cv_it_end = fu_cv.second.end();
         for(CustomOrderedSet<vertex>::const_iterator cv_it = fu_cv.second.begin(); cv_it != cv_it_end;)
         {
            CustomOrderedSet<vertex>::const_iterator cv1_it = cv_it;
            ++cv_it;
            for(CustomOrderedSet<vertex>::const_iterator cv2_it = cv_it; cv2_it != cv_it_end; ++cv2_it)
            {
               if(!can_be_clustered(*cv1_it, fsdg, fu, slack_time, 0))
                  continue;
               if(!can_be_clustered(*cv2_it, fsdg, fu, slack_time, 0))
                  continue;
               if(clock_cycle < (setup_hold_time + starting_time[*cv1_it] + ending_time[*cv2_it] - starting_time[*cv2_it]))
                  hc.add_tabu_pair(boost::get(boost::vertex_index, *fsdg, *cv1_it), boost::get(boost::vertex_index, *fsdg, *cv2_it));
               if(clock_cycle < (setup_hold_time + starting_time[*cv2_it] + ending_time[*cv1_it] - starting_time[*cv1_it]))
                  hc.add_tabu_pair(boost::get(boost::vertex_index, *fsdg, *cv1_it), boost::get(boost::vertex_index, *fsdg, *cv2_it));
               if(HLS->Rliv->are_in_conflict(*cv1_it, *cv2_it))
                  hc.add_tabu_pair(boost::get(boost::vertex_index, *fsdg, *cv1_it), boost::get(boost::vertex_index, *fsdg, *cv2_it));
               // else if(allocation_information->get_worst_execution_time(fu_s1)==0)
               // hc.add_tabu_pair(boost::get(boost::vertex_index, *fsdg, *cv1_it), boost::get(boost::vertex_index, *fsdg, *cv2_it));
            }
         }
      }
      /// add the control dependencies edges and the chained edges to the HC graph
      for(const auto& fu_cv : candidate_vertices)
      {
         for(const auto& cv : fu_cv.second)
         {
            unsigned int fu_s1 = fu_cv.first;
            const double mux_time = MODULE_BINDING_MUX_MARGIN * allocation_information->estimate_mux_time(fu_s1);
            if(!can_be_clustered(cv, fsdg, fu, slack_time, mux_time))
               continue;
            std::vector<HLS_manager::io_binding_type> vars_read1 = HLSMgr->get_required_values(HLS->functionId, cv);
            unsigned int index = 0;
            for(auto var_pair : vars_read1)
            {
               unsigned int var_written = std::get<0>(var_pair);
               vertex tgt = cv;
               vertex src = get_src_vertex(var_written, tgt, HLSMgr, HLS->functionId, fsdg);
               if(src != NULL_VERTEX)
               {
                  if(all_candidate_vertices.find(src) != all_candidate_vertices.end() && can_be_clustered(src, fsdg, fu, slack_time, 0) && !HLS->chaining_information->may_be_chained_ops(src, tgt))
                     hc.add_edge(boost::get(boost::vertex_index, *fsdg, src), boost::get(boost::vertex_index, *fsdg, tgt), 2 * index + (HLS->chaining_information->may_be_chained_ops(src, tgt) ? 1 : 0));
               }
               ++index;
            }
         }
      }
#if 0
      for(tie(ei, ei_end) = boost::edges(*fsdg); ei != ei_end; ++ei)
      {
         vertex src = boost::source(*ei, *fsdg);
         if(!can_be_clustered(src, fsdg, fu, slack_time, 0)) continue;
         if(all_candidate_vertices.find(src) == all_candidate_vertices.end()) continue;
         vertex tgt = boost::target(*ei, *fsdg);
         if(!can_be_clustered(tgt, fsdg, fu, slack_time, 0)) continue;
         if(all_candidate_vertices.find(tgt) == all_candidate_vertices.end()) continue;
         if(!HLS->chaining_information->may_be_chained_ops(src, tgt))
            hc.add_edge(boost::get(boost::vertex_index, *fsdg, src), boost::get(boost::vertex_index, *fsdg, tgt), 2*get_src_vertex(src, tgt, HLSMgr, HLS->functionId)+(HLS->chaining_information->may_be_chained_ops(src, tgt)?1:0));
      }
#endif

      hc.exec();
#endif

      if(output_level >= OUTPUT_LEVEL_MINIMUM)
         START_TIME(weight_cputime);

      int _w = 1;

      // Do a preliminary register binding to help the sharing of complex operations
      {
         DesignFlowStepRef regb;
         regb = GetPointer<const HLSFlowStepFactory>(design_flow_manager.lock()->CGetDesignFlowStepFactory("HLS"))
                    ->CreateHLSFlowStep(HLSFlowStep_Type::WEIGHTED_CLIQUE_REGISTER_BINDING, funId, HLSFlowStepSpecializationConstRef(new WeightedCliqueRegisterBindingSpecialization(CliqueCovering_Algorithm::TS_WEIGHTED_CLIQUE_COVERING)));
         regb->Initialize();
         regb->Exec();
      }

      for(const auto& fu_cv : candidate_vertices)
      {
         unsigned int fu_s1 = fu_cv.first;
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Considering fu " + allocation_information->get_fu_name(fu_s1).first);

         std::string res_name = allocation_information->get_fu_name(fu_s1).first;
         std::string lib_name = HLS->HLS_T->get_technology_manager()->get_library(res_name);
         const double mux_time = MODULE_BINDING_MUX_MARGIN * allocation_information->estimate_mux_time(fu_s1);
         double controller_delay = allocation_information->EstimateControllerDelay();
         double resource_area = allocation_information->compute_normalized_area(fu_s1);
         bool disabling_slack_based_binding =
             ((allocation_information->get_number_channels(fu_s1) >= 1) and (!allocation_information->is_readonly_memory_unit(fu_s1) || (!parameters->isOption(OPT_rom_duplication) || !parameters->getOption<bool>(OPT_rom_duplication)))) ||
             lib_name == WORK_LIBRARY || lib_name == PROXY_LIBRARY || allocation_information->get_number_fu(fu_s1) != INFINITE_UINT;
         double local_mux_time = (disabling_slack_based_binding ? -std::numeric_limits<double>::infinity() : mux_time);
         unsigned int fu_prec = allocation_information->get_prec(fu_s1);
         bool cond1 = compute_condition1(lib_name, allocation_information, local_mux_time, fu_s1);
         bool cond2 = compute_condition2(cond1, fu_prec, resource_area, small_normalized_resource_area);

         const auto cv_it_end = fu_cv.second.end();
         for(auto cv_it = fu_cv.second.begin(); cv_it != cv_it_end;)
         {
            auto cv1_it = cv_it;
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->First operation is " + GET_NAME(sdg, *cv1_it));
            ++cv_it;
            for(auto cv2_it = cv_it; cv2_it != cv_it_end; ++cv2_it)
            {
               if(HLS->Rliv->are_in_conflict(*cv1_it, *cv2_it))
               {
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                 "---Conflict between operations " + GET_NAME(sdg, *cv1_it) + "(" + sdg->CGetOpNodeInfo(*cv1_it)->GetOperation() + ")" + GET_NAME(sdg, *cv2_it) + "(" + sdg->CGetOpNodeInfo(*cv2_it)->GetOperation() + ") (" +
                                     allocation_information->get_string_name(fu_s1) + ")");
                  continue;
               }
               if(!disabling_slack_based_binding && !allocation_information->is_proxy_unit(fu_s1) && allocation_information->get_number_channels(fu_s1) < 2 && allocation_information->get_cycles(fu_s1, *cv1_it, sdg) <= 1 &&
                  allocation_information->get_cycles(fu_s1, *cv2_it, sdg) <= 1)
               {
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Checking for execution frames");
                  if(clock_cycle < (setup_hold_time + starting_time[*cv1_it] + ending_time[*cv2_it] - starting_time[*cv2_it]))
                  {
                     INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                    "---Performance based problem in sharing " + GET_NAME(sdg, *cv1_it) + "(" + sdg->CGetOpNodeInfo(*cv1_it)->GetOperation() + ")" + GET_NAME(sdg, *cv2_it) + "(" + sdg->CGetOpNodeInfo(*cv2_it)->GetOperation() +
                                        ")=" + STR(clock_cycle) + "<" + STR(setup_hold_time + starting_time[*cv1_it] + ending_time[*cv2_it] - starting_time[*cv2_it]));
                     ++n_performance_conflicts;
                     continue;
                  }
                  if(clock_cycle < (setup_hold_time + starting_time[*cv2_it] + ending_time[*cv1_it] - starting_time[*cv1_it]))
                  {
                     INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                    "---Performance based problem in sharing " + GET_NAME(sdg, *cv1_it) + "(" + sdg->CGetOpNodeInfo(*cv1_it)->GetOperation() + ")" + GET_NAME(sdg, *cv2_it) + "(" + sdg->CGetOpNodeInfo(*cv2_it)->GetOperation() +
                                        ")=" + STR(clock_cycle) + "<" + STR(setup_hold_time + starting_time[*cv2_it] + ending_time[*cv1_it] - starting_time[*cv1_it]));
                     ++n_performance_conflicts;
                     continue;
                  }
               }
               if(GET_TYPE(dfg, *cv1_it) & (TYPE_ENTRY | TYPE_EXIT | TYPE_PHI | TYPE_VPHI | TYPE_GOTO | TYPE_LABEL | TYPE_RET | TYPE_SWITCH))
                  continue;
               if(GET_TYPE(dfg, *cv2_it) & (TYPE_ENTRY | TYPE_EXIT | TYPE_PHI | TYPE_VPHI | TYPE_GOTO | TYPE_LABEL | TYPE_RET | TYPE_SWITCH))
                  continue;

               _w = weight_computation(cond1, cond2, *cv1_it, *cv2_it, local_mux_time, dfg, fu, slack_time, starting_time,
#ifdef HC_APPROACH
                                       hc,
#endif
                                       con_rel, controller_delay, fu_prec);
               // std::cerr << "possible sharing between " << GET_NAME(sdg, *cv1_it) << " and " << GET_NAME(sdg, *cv2_it) << ", but.... maybe next time " << _w << " " << mux_time  << " -- " + STR(s2c[*cv1_it]) + "->" + STR(s2c[*cv2_it])<<  std::endl;

               /// add compatibility edge with computed weight on both the directions
               if(_w > 0)
               {
                  cdfc_edge E;
                  bool exists;
                  boost::tie(E, exists) = boost::edge(s2c[*cv1_it], s2c[*cv2_it], *cdfc_bulk_graph);
                  if(!exists)
                  {
                     boost::tie(E, exists) = boost::add_edge(s2c[*cv1_it], s2c[*cv2_it], edge_cdfc_selector(COMPATIBILITY_EDGE, _w), *cdfc_bulk_graph);
                     THROW_ASSERT(exists, "already inserted edge " + GET_NAME(sdg, *cv1_it) + " - " + GET_NAME(sdg, *cv2_it) + " -- " + STR(s2c[*cv1_it]) + "->" + STR(s2c[*cv2_it]));
                  }
                  else
                     THROW_ERROR("already inserted edge " + GET_NAME(sdg, *cv1_it) + " - " + GET_NAME(sdg, *cv2_it) + " -- " + STR(s2c[*cv1_it]) + "->" + STR(s2c[*cv2_it]));

                  boost::tie(E, exists) = boost::edge(s2c[*cv2_it], s2c[*cv1_it], *cdfc_bulk_graph);
                  if(!exists)
                  {
                     boost::tie(E, exists) = boost::add_edge(s2c[*cv2_it], s2c[*cv1_it], edge_cdfc_selector(COMPATIBILITY_EDGE, _w), *cdfc_bulk_graph);
                     THROW_ASSERT(exists, "already inserted edge " + GET_NAME(sdg, *cv2_it) + " - " + GET_NAME(sdg, *cv1_it) + " -- " + STR(s2c[*cv2_it]) + "->" + STR(s2c[*cv1_it]));
                  }
                  else
                     THROW_ERROR("already inserted edge " + GET_NAME(sdg, *cv2_it) + " - " + GET_NAME(sdg, *cv1_it) + " -- " + STR(s2c[*cv2_it]) + "->" + STR(s2c[*cv1_it]));
               }
               else
               {
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                 "Null compatibility weight for " + GET_NAME(sdg, *cv1_it) + "(" + sdg->CGetOpNodeInfo(*cv1_it)->GetOperation() + ")" + GET_NAME(sdg, *cv2_it) + "(" + sdg->CGetOpNodeInfo(*cv2_it)->GetOperation() + ") (" +
                                     allocation_information->get_string_name(fu_s1) + ")");
               }
            }
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--");
         }
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Considered fu " + allocation_information->get_fu_name(fu_s1).first);
      }
      if(output_level >= OUTPUT_LEVEL_MINIMUM)
         STOP_TIME(weight_cputime);

      const cdfc_graphRef CG = cdfc_graphRef(new cdfc_graph(*cdfc_bulk_graph, cdfc_graph_edge_selector<boost_cdfc_graph>(COMPATIBILITY_EDGE, &*cdfc_bulk_graph), cdfc_graph_vertex_selector<boost_cdfc_graph>()));
      const cdfc_graphConstRef CD_chained_graph = cdfc_graphConstRef(new cdfc_graph(*cdfc_bulk_graph, cdfc_graph_edge_selector<boost_cdfc_graph>(CD_EDGE, &*cdfc_bulk_graph), cdfc_graph_vertex_selector<boost_cdfc_graph>()));
      std::string functionName = FB->CGetBehavioralHelper()->get_function_name();
      if(parameters->getOption<bool>(OPT_print_dot))
      {
         // CD_chained_graph->WriteDot("HLS_CD_CHAINED.dot");
      }
      /// compute levels
      std::vector<int> cd_levels;
      cd_levels.resize(boost::num_vertices(*CD_chained_graph));
      /// topologically sort vertex of CD_EDGE based graph
      std::deque<size_t> Csorted_vertices;
      topological_based_sorting(*CD_chained_graph, c2s, sdg, std::front_inserter(Csorted_vertices));
      std::deque<size_t>::const_iterator sv_it_end = Csorted_vertices.end();
      for(std::deque<size_t>::const_iterator sv_it = Csorted_vertices.begin(); sv_it != sv_it_end; ++sv_it)
      {
         cd_levels[*sv_it] = 0;
         cdfc_in_edge_iterator ie, ie_end;
         for(boost::tie(ie, ie_end) = boost::in_edges(*sv_it, *CD_chained_graph); ie != ie_end; ++ie)
            if(boost::in_degree(boost::source(*ie, *CD_chained_graph), *CG) != 0)
               cd_levels[*sv_it] = std::max(cd_levels[*sv_it], 1 + cd_levels[boost::get(boost::vertex_index, *CD_chained_graph, boost::source(*ie, *CD_chained_graph))]);
            else
               cd_levels[*sv_it] = std::max(cd_levels[*sv_it], cd_levels[boost::get(boost::vertex_index, *CD_chained_graph, boost::source(*ie, *CD_chained_graph))]);
      }

      /// remove all cycles from the cdfc graph
      const cdfc_graphConstRef cdfc = cdfc_graphConstRef(new cdfc_graph(*cdfc_bulk_graph, cdfc_graph_edge_selector<boost_cdfc_graph>(CD_EDGE | COMPATIBILITY_EDGE, &*cdfc_bulk_graph), cdfc_graph_vertex_selector<boost_cdfc_graph>()));

      if(output_level >= OUTPUT_LEVEL_MINIMUM)
         START_TIME(falseloop_cputime);

      unsigned int k = 2;
      std::deque<cdfc_edge> candidate_edges;
      CustomUnorderedSet<vertex> no_cycles;
      bool restart;

      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Looking for loops in compatibility graphs");
      do
      {
         restart = false;
         for(const auto candidate : all_candidate_vertices)
         {
            if(no_cycles.find(candidate) != no_cycles.end())
               continue;
            cdfc_vertex start = s2c[candidate];

            if(cd_levels[boost::get(boost::vertex_index, *CG, start)] != 0 && boost::in_degree(start, *CG) != 0)
            {
               bool found_a_loop;
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Search loops starting from -> " + GET_NAME(sdg, candidate) + " iteration " + STR(k));
               found_a_loop = false_loop_search(start, k, cdfc, CG, candidate_edges);
               if(!found_a_loop)
                  no_cycles.insert(candidate);
               restart |= found_a_loop;
               THROW_ASSERT(!found_a_loop || candidate_edges.size() >= 1, "something of unexpected happen");
               while(found_a_loop)
               {
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Found a loop");
                  /// remove the loop
                  const std::deque<cdfc_edge>::const_iterator ce_it_end = candidate_edges.end();
                  std::deque<cdfc_edge>::const_iterator ce_it = candidate_edges.begin();
                  cdfc_edge cand_e = *ce_it;
                  ++ce_it;
                  cdfc_vertex cand_src = boost::source(cand_e, *CG);
                  cdfc_vertex cand_tgt = boost::target(cand_e, *CG);
                  int cand_level_difference = std::abs(cd_levels[boost::get(boost::vertex_index, *CG, cand_src)] - cd_levels[boost::get(boost::vertex_index, *CG, cand_tgt)]);
                  size_t cand_out_degree = boost::out_degree(cand_src, *CG) + boost::out_degree(cand_tgt, *CG);
                  int cand_edge_weight = (*CG)[cand_e].weight;
                  if(allocation_information->get_number_channels(fu->get_assign(c2s[boost::get(boost::vertex_index, *CG, cand_src)])) >= 1)
                  {
                     if(allocation_information->is_readonly_memory_unit(fu->get_assign(c2s[boost::get(boost::vertex_index, *CG, cand_src)])))
                        cand_level_difference = -2;
                     else
                        cand_level_difference = -1;
                  }
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                 "-->Analyzing compatibility between operations " + GET_NAME(sdg, c2s[boost::get(boost::vertex_index, *CG, cand_src)]) + " and " + GET_NAME(sdg, c2s[boost::get(boost::vertex_index, *CG, cand_tgt)]) +
                                     " - ld = " + STR(cand_level_difference) + " - d= " + STR(cand_out_degree) + " - w = " + STR(cand_edge_weight));

                  for(; ce_it != ce_it_end; ++ce_it)
                  {
                     cdfc_edge e = *ce_it;
                     cdfc_vertex src = boost::source(e, *CG);
                     cdfc_vertex tgt = boost::target(e, *CG);
                     int level_difference = std::abs(cd_levels[boost::get(boost::vertex_index, *CG, src)] - cd_levels[boost::get(boost::vertex_index, *CG, tgt)]);
                     size_t out_degree = boost::out_degree(src, *CG) + boost::out_degree(tgt, *CG);
                     int edge_weight = (*CG)[e].weight;
                     if(allocation_information->get_number_channels(fu->get_assign(c2s[boost::get(boost::vertex_index, *CG, src)])) >= 1)
                        level_difference = -1;
                     INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                    "---Analyzing compatibility between operations " + GET_NAME(sdg, c2s[boost::get(boost::vertex_index, *CG, src)]) + " and " + GET_NAME(sdg, c2s[boost::get(boost::vertex_index, *CG, tgt)]) +
                                        " - ld = " + STR(level_difference) + " - d= " + STR(out_degree) + " - w = " + STR(edge_weight));
                     if(level_difference > cand_level_difference || (level_difference == cand_level_difference && out_degree > cand_out_degree) ||
                        (level_difference == cand_level_difference && out_degree == cand_out_degree && edge_weight < cand_edge_weight))
                     {
                        cand_src = src;
                        cand_tgt = tgt;
                        cand_e = e;
                        cand_level_difference = level_difference;
                        cand_out_degree = out_degree;
                        cand_edge_weight = edge_weight;
                     }
                  }
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--");
                  /// remove both the compatibility edges
                  // boost::remove_edge(cand_e, *cdfc_bulk_graph);
                  (*cdfc_bulk_graph)[cand_e].selector = 0;
                  bool exists;
                  boost::tie(cand_e, exists) = boost::edge(cand_tgt, cand_src, *CG);
                  THROW_ASSERT(exists, "edge already removed");
                  (*cdfc_bulk_graph)[cand_e].selector = 0;
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                 "---Removed compatibility between operations " + GET_NAME(sdg, c2s[boost::get(boost::vertex_index, *CG, cand_src)]) + "(" + sdg->CGetOpNodeInfo(c2s[boost::get(boost::vertex_index, *CG, cand_src)])->GetOperation() + ")" +
                                     " and " + GET_NAME(sdg, c2s[boost::get(boost::vertex_index, *CG, cand_tgt)]));
                  candidate_edges.clear();

                  /// search another loop
                  found_a_loop = false_loop_search(start, k, cdfc, CG, candidate_edges);

                  // std::cerr << "2 Search loops starting from -> " + GET_NAME(sdg, candidate) + " iteration " + STR(k) << std::endl;
               }
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Searched loops starting from -> " + GET_NAME(sdg, candidate) + " iteration " + STR(k));
               THROW_ASSERT(candidate_edges.empty(), "candidate_cycle has to be empty");
            }
            else
               no_cycles.insert(candidate);
         }
         ++k;
      } while(restart);
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Looked for loops in compatibility graphs");

      if(output_level >= OUTPUT_LEVEL_MINIMUM)
         STOP_TIME(falseloop_cputime);

      if(parameters->getOption<bool>(OPT_print_dot))
      {
         // cdfc->WriteDot("HLS_CD_COMP.dot");
      }

      std::map<vertex, vertex> identity_converter;

      /// partition vertices for clique covering or bind the easy functional units
      std::map<unsigned int, unsigned int> numModule;
      std::map<unsigned int, CustomSet<cdfc_vertex>, cdfc_resource_ordering_functor> partitions(r_functor);
      for(const auto& fu_cv : candidate_vertices)
      {
         fu_unit = fu_cv.first;
         for(const auto cv : fu_cv.second)
         {
            /// check easy binding
            if(boost::out_degree(s2c[cv], *CG) == 0)
            {
               unsigned int num = 0;
               if(numModule.find(fu_unit) == numModule.end())
                  numModule[fu_unit] = 1;
               else
                  num = numModule[fu_unit]++;
               ++total_modules_allocated;
               total_resource_area += allocation_information->get_area(fu_unit);
               total_DSPs += allocation_information->get_DSPs(fu_unit);
               fu->bind(cv, fu_unit, num);
            }
            else
            {
               partitions[fu_unit].insert(s2c[cv]);
               identity_converter[cv] = cv;
            }
         }
      }

      if(output_level >= OUTPUT_LEVEL_MINIMUM)
         START_TIME(clique_cputime);

      /// solve the binding problem for all the partitions
      const unsigned int number_of_iterations = 10;
      const std::map<unsigned int, unsigned int> numModule_initial = numModule;
      const size_t total_modules_allocated_initial = total_modules_allocated;
      const double total_resource_area_initial = total_resource_area;
      double total_resource_area_prev = total_resource_area;
      const double total_DSPs_initial = total_DSPs;
      double total_DSPs_prev = total_DSPs;
      const double total_area_muxes_initial = total_area_muxes;
      double total_area_muxes_prev = total_area_muxes;
      const CustomUnorderedMap<vertex, double> slack_time_initial = slack_time;
      const CustomUnorderedMap<vertex, double> starting_time_initial = starting_time;

      fu_bindingRef fu_best;
      if(parameters->getOption<int>(OPT_memory_banks_number) > 1 && !parameters->isOption(OPT_context_switch))
      {
         fu_best = fu_bindingRef(new ParallelMemoryFuBinding(HLSMgr, funId, parameters));
      }
      else
      {
         fu_best = fu_bindingRef(fu_binding::create_fu_binding(HLSMgr, funId, parameters));
      }
      double total_area_best = 0;
      size_t total_modules_allocated_best = 0;
      double total_resource_area_best = 0;
      double total_area_muxes_best = 0;
      double total_DSPs_best = 0;

      for(unsigned int iteration = 0; iteration < number_of_iterations; ++iteration)
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Running iteration " + STR(iteration));
         if(iteration > 0)
         {
            if(iteration > 1 && total_resource_area == total_resource_area_prev && total_DSPs == total_DSPs_prev && total_area_muxes == total_area_muxes_prev)
               break;
            numModule = numModule_initial;
            total_modules_allocated = total_modules_allocated_initial;
            total_resource_area_prev = total_resource_area;
            total_resource_area = total_resource_area_initial;
            total_DSPs_prev = total_DSPs;
            total_DSPs = total_DSPs_initial;
            slack_time = slack_time_initial;
            starting_time = starting_time_initial;
            total_area_muxes_prev = total_area_muxes;
            total_area_muxes = total_area_muxes_initial;

            DesignFlowStepRef regb;
            // if(iteration%2)
            regb = GetPointer<const HLSFlowStepFactory>(design_flow_manager.lock()->CGetDesignFlowStepFactory("HLS"))
                       ->CreateHLSFlowStep(HLSFlowStep_Type::WEIGHTED_CLIQUE_REGISTER_BINDING, funId, HLSFlowStepSpecializationConstRef(new WeightedCliqueRegisterBindingSpecialization(CliqueCovering_Algorithm::TS_WEIGHTED_CLIQUE_COVERING)));
            // else
            //   regb = GetPointer<const HLSFlowStepFactory>(design_flow_manager.lock()->CGetDesignFlowStepFactory("HLS"))->CreateHLSFlowStep(HLSFlowStep_Type::WEIGHTED_CLIQUE_REGISTER_BINDING, funId, HLSFlowStepSpecializationConstRef(new
            //   WeightedCliqueRegisterBindingSpecialization(CliqueCovering_Algorithm::BIPARTITE_MATCHING)));
            regb->Initialize();
            regb->Exec();
         }

         clique_iteration_cputime[iteration] = 0;
         if(output_level >= OUTPUT_LEVEL_VERBOSE)
            START_TIME(clique_iteration_cputime[iteration]);
         for(const auto& partition : partitions)
         {
            THROW_ASSERT(partition.second.size() > 1, "bad projection");
            auto vert_it_end = partition.second.end();
            const double mux_time = MODULE_BINDING_MUX_MARGIN * allocation_information->estimate_mux_time(partition.first);
            double controller_delay = allocation_information->EstimateControllerDelay();
            double resource_area = allocation_information->compute_normalized_area(partition.first);
            unsigned int fu_prec = allocation_information->get_prec(partition.first);

            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---controller_delay: " + STR(controller_delay) + " resource normalized area=" + STR(resource_area));
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---mux_time: " + STR(mux_time) + " area_mux=" + STR(allocation_information->estimate_mux_area(partition.first)));

            CliqueCovering_Algorithm clique_covering_algorithm = GetPointer<const CDFCModuleBindingSpecialization>(hls_flow_step_specialization)->clique_covering_algorithm;
            bool disabling_slack_cond0 = ((allocation_information->get_number_channels(partition.first) >= 1) and
                                          (!allocation_information->is_readonly_memory_unit(partition.first) || (!parameters->isOption(OPT_rom_duplication) || !parameters->getOption<bool>(OPT_rom_duplication))));
            if(disabling_slack_cond0)
            {
               clique_covering_algorithm = CliqueCovering_Algorithm::BIPARTITE_MATCHING;
            }

            const CliqueCovering_Algorithm clique_covering_method_used = clique_covering_algorithm;
            std::string res_name = allocation_information->get_fu_name(partition.first).first;
            std::string lib_name = HLS->HLS_T->get_technology_manager()->get_library(res_name);
            bool disabling_slack_based_binding = disabling_slack_cond0 || lib_name == WORK_LIBRARY || lib_name == PROXY_LIBRARY || allocation_information->get_number_fu(partition.first) != INFINITE_UINT;

            THROW_ASSERT(lib_name != PROXY_LIBRARY || 1 == allocation_information->get_number_fu(partition.first), "unexpected condition");

            /// build the clique covering solver
            refcount<clique_covering<vertex>> module_clique(clique_covering<vertex>::create_solver(clique_covering_method_used));
            /// add vertex to the clique covering solver
            for(auto vert_it = partition.second.begin(); vert_it != vert_it_end; ++vert_it)
            {
               std::string el1_name = GET_NAME(sdg, c2s[boost::get(boost::vertex_index, *CG, *vert_it)]) + "(" + sdg->CGetOpNodeInfo(c2s[boost::get(boost::vertex_index, *CG, *vert_it)])->GetOperation() + ")";
               module_clique->add_vertex(c2s[boost::get(boost::vertex_index, *CG, *vert_it)], el1_name);
            }

            if(clique_covering_method_used == CliqueCovering_Algorithm::BIPARTITE_MATCHING)
            {
               std::map<vertex, size_t> v2id;
               size_t max_id = 0, curr_id;
               for(auto vert_it = partition.second.begin(); vert_it != vert_it_end; ++vert_it)
               {
                  const CustomOrderedSet<vertex>& running_states = HLS->Rliv->get_state_where_run(c2s[boost::get(boost::vertex_index, *CG, *vert_it)]);
                  const CustomOrderedSet<vertex>::const_iterator rs_it_end = running_states.end();
                  for(auto rs_it = running_states.begin(); rs_it != rs_it_end; ++rs_it)
                  {
                     if(v2id.find(*rs_it) == v2id.end())
                     {
                        curr_id = max_id;
                        v2id[*rs_it] = max_id;
                        ++max_id;
                     }
                     else
                        curr_id = v2id.find(*rs_it)->second;
                     module_clique->add_subpartitions(curr_id, c2s[boost::get(boost::vertex_index, *CG, *vert_it)]);
                  }
               }
            }
            double local_mux_time = (disabling_slack_based_binding ? -std::numeric_limits<double>::infinity() : mux_time);
            bool cond1 = compute_condition1(lib_name, allocation_information, local_mux_time, partition.first);
            bool cond2 = compute_condition2(cond1, fu_prec, resource_area, small_normalized_resource_area);

            /// add the edges
            cdfc_edge_iterator cg_ei, cg_ei_end;
            const cdfc_graphConstRef CG_subgraph(new cdfc_graph(*cdfc_bulk_graph, cdfc_graph_edge_selector<boost_cdfc_graph>(COMPATIBILITY_EDGE, &*cdfc_bulk_graph), cdfc_graph_vertex_selector<boost_cdfc_graph>(&partition.second)));
            for(boost::tie(cg_ei, cg_ei_end) = boost::edges(*CG_subgraph); cg_ei != cg_ei_end; ++cg_ei)
            {
               vertex src = c2s[boost::get(boost::vertex_index, *CG_subgraph, boost::source(*cg_ei, *CG_subgraph))];
               vertex tgt = c2s[boost::get(boost::vertex_index, *CG_subgraph, boost::target(*cg_ei, *CG_subgraph))];
#if HAVE_UNORDERED
               if(src > tgt)
#else
               if(GET_NAME(dfg, src) > GET_NAME(dfg, tgt))
#endif
                  continue; /// only one edge is needed to build the undirected compatibility graph
               _w = weight_computation(cond1, cond2, src, tgt, local_mux_time, dfg, fu, slack_time, starting_time,
#ifdef HC_APPROACH
                                       hc,
#endif
                                       con_rel, controller_delay, fu_prec);
               if(_w > 0)
                  module_clique->add_edge(src, tgt, _w);
            }
            if(parameters->getOption<bool>(OPT_print_dot))
            {
               const auto output_directory = parameters->getOption<std::string>(OPT_dot_directory) + "/" + functionName + "/";
               if(!boost::filesystem::exists(output_directory))
                  boost::filesystem::create_directories(output_directory);
               const auto file_name = output_directory + "MB_" + allocation_information->get_string_name(partition.first) + ".dot";
               module_clique->writeDot(file_name);
            }

            if(allocation_information->get_number_fu(partition.first) != INFINITE_UINT)
            {
               THROW_ASSERT(allocation_information->get_number_channels(partition.first) == 0 || allocation_information->get_number_channels(partition.first) == allocation_information->get_number_fu(partition.first), "unexpected condition");
               PRINT_DBG_MEX(DEBUG_LEVEL_VERBOSE, debug_level, "Defining resource constraints for  : " + allocation_information->get_string_name(partition.first) + " to " + STR(allocation_information->get_number_fu(partition.first)));
               module_clique->suggest_min_resources(allocation_information->get_number_channels(partition.first));
               if(allocation_information->get_number_channels(partition.first) > 0)
                  module_clique->max_resources(allocation_information->get_number_channels(partition.first));
            }

            /// Specify the minimum number of resources in case we have to use all the memory ports.
            /// That is relevant for memories attached to the bus
            /// Private memories should use the minimum number of ports to minimize the total area.
            unsigned var = allocation_information->is_direct_access_memory_unit(partition.first) ?
                               (allocation_information->is_memory_unit(partition.first) ? allocation_information->get_memory_var(partition.first) : allocation_information->get_proxy_memory_var(partition.first)) :
                               0;
            if(var && !HLSMgr->Rmem->is_private_memory(var))
               module_clique->min_resources(allocation_information->get_number_channels(partition.first));

            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Starting clique covering on a graph with " + STR(partition.second.size()) + " vertices for " + allocation_information->get_string_name(partition.first));

            /// performing clique covering
            if(disabling_slack_based_binding)
            {
               PRINT_DBG_MEX(DEBUG_LEVEL_VERBOSE, debug_level, "Disabled slack based clique covering for: " + res_name);
#if HAVE_EXPERIMENTAL
               if(clique_covering_method_used == CliqueCovering_Algorithm::RANDOMIZED)
               {
                  double area_resource = allocation_information->get_area(partition.first) + 100 * allocation_information->get_DSPs(partition.first);
                  module_register_binding_spec mrbs;
                  module_binding_check_no_filter<vertex> cq(fu_prec, area_resource, HLS, HLSMgr, slack_time, starting_time, controller_delay, mrbs);
                  module_clique->exec(no_filter_clique<vertex>(), cq);
               }
               else
#endif
               {
                  no_check_clique<vertex> cq;
                  module_clique->exec(no_filter_clique<vertex>(), cq);
               }
               INDENT_DBG_MEX(DEBUG_LEVEL_VERBOSE, debug_level, "Number of cliques covering the graph: " + STR(module_clique->num_vertices()) + " for " + allocation_information->get_string_name(partition.first));
               if(module_clique->num_vertices() == 0 || (allocation_information->get_number_channels(partition.first) >= 1 && module_clique->num_vertices() > allocation_information->get_number_channels(partition.first)))
               {
                  PRINT_DBG_MEX(DEBUG_LEVEL_VERBOSE, debug_level, "Restarting with BIPARTITE_MATCHING: " + res_name);
                  module_clique = clique_covering<vertex>::create_solver(CliqueCovering_Algorithm::BIPARTITE_MATCHING);
                  for(auto vert_it = partition.second.begin(); vert_it != vert_it_end; ++vert_it)
                  {
                     std::string el1_name = GET_NAME(sdg, c2s[boost::get(boost::vertex_index, *CG, *vert_it)]) + "(" + sdg->CGetOpNodeInfo(c2s[boost::get(boost::vertex_index, *CG, *vert_it)])->GetOperation() + ")";
                     module_clique->add_vertex(c2s[boost::get(boost::vertex_index, *CG, *vert_it)], el1_name);
                  }
                  std::map<vertex, size_t> v2id;
                  size_t max_id = 0, curr_id;
                  for(auto vert_it = partition.second.begin(); vert_it != vert_it_end; ++vert_it)
                  {
                     const CustomOrderedSet<vertex>& running_states = HLS->Rliv->get_state_where_run(c2s[boost::get(boost::vertex_index, *CG, *vert_it)]);
                     const CustomOrderedSet<vertex>::const_iterator rs_it_end = running_states.end();
                     for(auto rs_it = running_states.begin(); rs_it != rs_it_end; ++rs_it)
                     {
                        if(v2id.find(*rs_it) == v2id.end())
                        {
                           curr_id = max_id;
                           v2id[*rs_it] = max_id;
                           ++max_id;
                        }
                        else
                           curr_id = v2id.find(*rs_it)->second;
                        module_clique->add_subpartitions(curr_id, c2s[boost::get(boost::vertex_index, *CG, *vert_it)]);
                     }
                  }
                  const cdfc_graphConstRef CG_subgraph0(new cdfc_graph(*cdfc_bulk_graph, cdfc_graph_edge_selector<boost_cdfc_graph>(COMPATIBILITY_EDGE, &*cdfc_bulk_graph), cdfc_graph_vertex_selector<boost_cdfc_graph>(&partition.second)));
                  for(boost::tie(cg_ei, cg_ei_end) = boost::edges(*CG_subgraph0); cg_ei != cg_ei_end; ++cg_ei)
                  {
                     vertex src = c2s[boost::get(boost::vertex_index, *CG_subgraph0, boost::source(*cg_ei, *CG_subgraph0))];
                     vertex tgt = c2s[boost::get(boost::vertex_index, *CG_subgraph0, boost::target(*cg_ei, *CG_subgraph0))];
#if HAVE_UNORDERED
                     if(src > tgt)
#else
                     if(GET_NAME(dfg, src) > GET_NAME(dfg, tgt))
#endif
                        continue; /// only one edge is needed to build the undirected compatibility graph
                     _w = weight_computation(cond1, cond2, src, tgt, local_mux_time, dfg, fu, slack_time, starting_time,
#ifdef HC_APPROACH
                                             hc,
#endif
                                             con_rel, controller_delay, fu_prec);
                     if(_w > 0)
                        module_clique->add_edge(src, tgt, _w);
                  }
                  if(allocation_information->get_number_fu(partition.first) != INFINITE_UINT)
                  {
                     THROW_ASSERT(allocation_information->get_number_channels(partition.first) == 0 || allocation_information->get_number_channels(partition.first) == allocation_information->get_number_fu(partition.first), "unexpected condition");

                     module_clique->suggest_min_resources(allocation_information->get_number_channels(partition.first));
                     if(allocation_information->get_number_channels(partition.first) > 0)
                        module_clique->max_resources(allocation_information->get_number_channels(partition.first));
                  }

                  /// Specify the minimum number of resources in case we have to use all the memory ports.
                  /// That is relevant for memories attached to the bus
                  /// Private memories should use the minimum number of ports to minimize the total area.
                  if(var && !HLSMgr->Rmem->is_private_memory(var))
                     module_clique->min_resources(allocation_information->get_number_channels(partition.first));
                  no_check_clique<vertex> cq;
                  module_clique->exec(no_filter_clique<vertex>(), cq);
                  if(allocation_information->get_number_fu(partition.first) != INFINITE_UINT)
                  {
                     THROW_ASSERT(allocation_information->get_number_channels(partition.first) == 0 || allocation_information->get_number_channels(partition.first) == allocation_information->get_number_fu(partition.first), "unexpected condition");
                     if(allocation_information->get_number_channels(partition.first) > 0 && module_clique->num_vertices() > allocation_information->get_number_channels(partition.first) && !allocation_information->is_readonly_memory_unit(partition.first))
                     {
                        THROW_ERROR("Something of wrong happen: no feasible solution exist for module binding: " + res_name + "[" + STR(module_clique->num_vertices()) + "]");
                     }
                  }
               }
            }
#if HAVE_EXPERIMENTAL
            else if(clique_covering_method_used == CliqueCovering_Algorithm::RANDOMIZED)
            {
               double area_resource = allocation_information->get_area(partition.first) + 100 * allocation_information->get_DSPs(partition.first);
               module_register_binding_spec mrbs;
               module_binding_check<vertex> cq(fu_prec, area_resource, HLS, HLSMgr, slack_time, starting_time, controller_delay, mrbs);
               module_clique->exec(no_filter_clique<vertex>(), cq);
            }
#endif
            else
            {
               double area_resource = allocation_information->get_area(partition.first) + 100 * allocation_information->get_DSPs(partition.first);
               module_register_binding_spec mrbs;
               module_binding_check<vertex> cq(fu_prec, area_resource, HLS, HLSMgr, slack_time, starting_time, controller_delay, mrbs);
               module_clique->exec(slack_based_filtering(slack_time, starting_time, controller_delay, fu_prec, HLS, HLSMgr, area_resource, con_rel), cq);
            }
            INDENT_DBG_MEX(DEBUG_LEVEL_VERBOSE, debug_level, "Number of cliques covering the graph: " + STR(module_clique->num_vertices()) + " for " + allocation_information->get_string_name(partition.first));
            total_modules_allocated += module_clique->num_vertices();
            to_update.clear();

            unsigned int Tot_mux = 0;

            /// retrieve the solution
            unsigned int delta_nclique = 0;
            for(unsigned int i = 0; i < module_clique->num_vertices(); ++i)
            {
               const auto clique_temp = module_clique->get_clique(i);
               if(clique_temp.empty())
                  continue;

#if HAVE_UNORDERED
               const auto clique = clique_temp;
#else
               OpVertexSet clique(sdg);
               clique.insert(clique_temp.begin(), clique_temp.end());
#endif

               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Analyzing clique");
               fu_unit = fu->get_assign(*(clique.begin()));
               THROW_ASSERT(fu_unit == partition.first, "unexpected case");
               unsigned int num = 0;
               if(numModule.find(fu_unit) == numModule.end())
                  numModule[fu_unit] = 1;
               else
                  num = numModule[fu_unit]++;
               total_resource_area += allocation_information->get_area(fu_unit);
               total_DSPs += allocation_information->get_DSPs(fu_unit);

               unsigned int total_muxes;
               unsigned int n_shared;

               double mux_area_estimation, mux_time_estimation;
               estimate_muxes<true, false>(con_rel, fu_prec, mux_time_estimation, mux_area_estimation, clique, total_muxes, n_shared, identity_converter, HLSMgr, HLS, debug_level);
               Tot_mux += total_muxes;
               total_area_muxes += mux_area_estimation;
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---estimate area muxes=" + STR(mux_area_estimation));
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---estimate delay muxes=" + STR(mux_time_estimation));

               INDENT_DBG_MEX(DEBUG_LEVEL_VERBOSE, debug_level, "---Sharing degree for " + allocation_information->get_string_name(fu_unit) + "_" + STR(num) + " = " + STR(clique.size()));
               /// compute maximum starting time
               double max_starting_time = 0.0;
               for(auto current_vert : clique)
               {
                  max_starting_time = std::max(max_starting_time, starting_time[current_vert]);
               }
               if(max_starting_time < controller_delay && total_muxes > 0)
                  max_starting_time = controller_delay;
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---max_starting_time=" + STR(max_starting_time));
               bool first_vertex = true;
               bool first_vertex_has_negative_slack = false;

               for(auto current_vert : clique)
               {
                  const auto node_id = sdg->CGetOpNodeInfo(current_vert)->GetNodeId();

                  if(!first_vertex && (!disabling_slack_based_binding && ((slack_time[current_vert] - (max_starting_time - starting_time[current_vert]) - mux_time_estimation) < 0 || first_vertex_has_negative_slack) && clique.size() > 1))
                  {
                     INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, " negative slack: solution is not feasible");
                     fu->bind(current_vert, fu_unit, numModule[fu_unit]);
                     if(node_id)
                        INDENT_OUT_MEX(OUTPUT_LEVEL_VERY_PEDANTIC, output_level,
                                       "---" + GET_NAME(sdg, current_vert) + "(" + TreeM->get_tree_node_const(node_id)->ToString() + ") bound to " + allocation_information->get_string_name(fu_unit) + "(" + STR(numModule[fu_unit]) + ")");
                     numModule[fu_unit]++;
                     total_resource_area += allocation_information->get_area(fu_unit);
                     total_DSPs += allocation_information->get_DSPs(fu_unit);
                     total_modules_allocated++;
                     delta_nclique++;
                     continue;
                  }
                  else
                  {
                     first_vertex = false;
                     /// storing new binding results
                     fu->bind(current_vert, fu_unit, num);
                     if(node_id)
                        INDENT_OUT_MEX(OUTPUT_LEVEL_VERY_PEDANTIC, output_level,
                                       "---" + GET_NAME(sdg, current_vert) + "(" + TreeM->get_tree_node_const(node_id)->ToString() + ") bound to " + allocation_information->get_string_name(fu_unit) + "(" + STR(num) + ")");
                  }
                  slack_time[current_vert] = slack_time[current_vert] - (max_starting_time - starting_time[current_vert]) - mux_time_estimation;
                  if(slack_time[current_vert] < 0)
                     first_vertex_has_negative_slack = true;
                  starting_time[current_vert] = max_starting_time;
                  to_update.insert(current_vert);
                  update_slack_starting_time(fdfg, to_update, slack_time, starting_time, false, true, false);
                  /*
                                    INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Updated Starting time *** Latest ending time *** Slacks");
                  #ifndef NDEBUG
                                    for(const auto operation : sdg->CGetOperations())
                                    {
                                       INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---" + GET_NAME(sdg, operation) + " *** starting_time=" + STR(starting_time.find(operation)->second) + " *** latest_ending_time=" +
                  STR(ending_time.find(operation)->second) + " *** slack_time="+ STR(slack_time.find(operation)->second));
                                    }
                  #endif
                                    INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--");
                  */
                  to_update.insert(current_vert);
                  update_slack_starting_time(fdfg, to_update, slack_time, starting_time, true, false, true);
                  /*                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Updated Starting time *** Latest ending time *** Slacks");
                  #ifndef NDEBUG
                                    for(const auto operation : sdg->CGetOperations())
                                    {
                                       INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---" + GET_NAME(sdg, operation) + " *** starting_time=" + STR(starting_time.find(operation)->second) + " *** latest_ending_time=" +
                  STR(ending_time.find(operation)->second) + " *** slack_time="+ STR(slack_time.find(operation)->second));
                                    }
                  #endif
                                    INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--");
                  */
               }
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Analyzed clique");
            }

            INDENT_OUT_MEX(OUTPUT_LEVEL_PEDANTIC, output_level,
                           "---cdfc mux estimation " + STR(Tot_mux) + " -- Number of cliques covering the graph: " + STR(module_clique->num_vertices() + delta_nclique) + " " + functionName + "_" + allocation_information->get_string_name(partition.first) +
                               " with " + STR(partition.second.size()) + " vertices");
         }
         if(iteration == 0 || total_area_best > total_area_muxes + total_resource_area)
         {
            fu_best = fu;
            total_area_best = total_area_muxes + total_resource_area;
            total_modules_allocated_best = total_modules_allocated;
            total_resource_area_best = total_resource_area;
            total_area_muxes_best = total_area_muxes;
            total_DSPs_best = total_DSPs;
         }
         if(output_level >= OUTPUT_LEVEL_VERBOSE)
            STOP_TIME(clique_iteration_cputime[iteration]);
      }
      std::swap(fu_best, fu);
      std::swap(total_modules_allocated_best, total_modules_allocated);
      std::swap(total_resource_area_best, total_resource_area);
      std::swap(total_area_muxes_best, total_area_muxes);
      std::swap(total_DSPs_best, total_DSPs);
      clique_covering_executed = true;
      if(output_level >= OUTPUT_LEVEL_MINIMUM)
         STOP_TIME(clique_cputime);
   }

   if(output_level <= OUTPUT_LEVEL_PEDANTIC)
      INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, output_level, "");
   INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, output_level, "-->Module binding information for function " + HLSMgr->CGetFunctionBehavior(funId)->CGetBehavioralHelper()->get_function_name() + ":");
   INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, output_level, "---Number of modules instantiated: " + STR(total_modules_allocated));
   INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, output_level, "---Number of possible conflicts for possible false paths introduced by resource sharing: " + STR(n_performance_conflicts));
   INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, output_level, "---Estimated resources area (no Muxes and address logic): " + STR(total_resource_area));
   INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, output_level, "---Estimated area of MUX21: " + STR(total_area_muxes));
   INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, output_level, "---Total estimated area: " + STR(total_area_muxes + total_resource_area));
   INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, output_level, "---Estimated number of DSPs: " + STR(total_DSPs));
   INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, output_level, "---Slack computed in " + print_cpu_time(slack_cputime) + " seconds");
   if(clique_covering_executed)
   {
      INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, output_level, "---False-loop computation completed in " + print_cpu_time(falseloop_cputime) + " seconds");
      INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, output_level, "---Weight computation completed in " + print_cpu_time(weight_cputime) + " seconds");
      if(output_level == OUTPUT_LEVEL_MINIMUM)
      {
         INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, output_level, "---Clique covering computation completed in " + print_cpu_time(clique_cputime) + " seconds");
      }
      else
      {
         INDENT_OUT_MEX(OUTPUT_LEVEL_VERBOSE, output_level, "---Clique covering computation:");
         INDENT_OUT_MEX(OUTPUT_LEVEL_VERBOSE, output_level, "-->");
         for(const auto& iteration : clique_iteration_cputime)
         {
            INDENT_OUT_MEX(OUTPUT_LEVEL_VERBOSE, output_level, "---Iteration " + STR(iteration.first) + " completed in " + print_cpu_time(iteration.second) + " seconds");
         }
         INDENT_OUT_MEX(OUTPUT_LEVEL_VERBOSE, output_level, "<--");
      }
   }
   if(output_level >= OUTPUT_LEVEL_MINIMUM and output_level <= OUTPUT_LEVEL_PEDANTIC)
      STOP_TIME(step_time);
   if(output_level >= OUTPUT_LEVEL_MINIMUM and output_level <= OUTPUT_LEVEL_PEDANTIC)
      INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, output_level, "Time to perform module binding: " + print_cpu_time(step_time) + " seconds");
   INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, output_level, "<--");
   if(output_level <= OUTPUT_LEVEL_PEDANTIC)
      INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, output_level, "");
   return DesignFlowStep_Status::SUCCESS;
}

bool cdfc_module_binding::false_loop_search(cdfc_vertex start, unsigned k, const cdfc_graphConstRef& cdfc, const cdfc_graphConstRef& cg, std::deque<cdfc_edge>& candidate_edges)
{
   std::vector<bool> visited(boost::num_vertices(*cdfc), false);
   std::vector<bool> cg_visited(boost::num_vertices(*cg), false);
   std::vector<bool> cdfc_visited(boost::num_vertices(*cg), false);
   cg_visited[boost::get(boost::vertex_index, *cg, start)] = true;
   cdfc_out_edge_iterator oe_cg, oe_end_cg;
   for(boost::tie(oe_cg, oe_end_cg) = boost::out_edges(start, *cg); oe_cg != oe_end_cg; ++oe_cg)
   {
      cdfc_vertex tgt = boost::target(*oe_cg, *cg);
      visited[boost::get(boost::vertex_index, *cdfc, tgt)] = true;
      if(false_loop_search_cdfc_1(tgt, 1, k, start, cdfc, cg, candidate_edges, visited, cg_visited, cdfc_visited))
      {
         candidate_edges.push_front(*oe_cg);
         return true;
      }
      visited[boost::get(boost::vertex_index, *cdfc, tgt)] = false;
   }
   return false;
}

bool cdfc_module_binding::false_loop_search_cdfc_1(cdfc_vertex src, unsigned int level, unsigned k, cdfc_vertex start, const cdfc_graphConstRef& cdfc, const cdfc_graphConstRef& cg, std::deque<cdfc_edge>& candidate_edges, std::vector<bool>& visited,
                                                   std::vector<bool>& cg_visited, std::vector<bool>& cdfc_visited)
{
   cdfc_out_edge_iterator oe_cdfc, oe_end_cdfc;
   if(level > k)
      return false;
   if(cdfc_visited[boost::get(boost::vertex_index, *cdfc, src)])
      return false;
   cdfc_visited[boost::get(boost::vertex_index, *cdfc, src)] = true;
   for(boost::tie(oe_cdfc, oe_end_cdfc) = boost::out_edges(src, *cdfc); oe_cdfc != oe_end_cdfc; ++oe_cdfc)
   {
      cdfc_vertex tgt = boost::target(*oe_cdfc, *cdfc);
      if(!visited[boost::get(boost::vertex_index, *cdfc, tgt)])
      {
         visited[boost::get(boost::vertex_index, *cdfc, tgt)] = true;
         bool is_cg_edge;
         cdfc_edge cg_e;
         boost::tie(cg_e, is_cg_edge) = boost::edge(src, tgt, *cg);
         if(!is_cg_edge && false_loop_search_cdfc_more(tgt, level, k, start, cdfc, cg, candidate_edges, visited, cg_visited, cdfc_visited))
            return true;
         visited[boost::get(boost::vertex_index, *cdfc, tgt)] = false;
      }
   }
   return false;
}

bool cdfc_module_binding::false_loop_search_cdfc_more(cdfc_vertex src, unsigned int level, unsigned k, cdfc_vertex start, const cdfc_graphConstRef& cdfc, const cdfc_graphConstRef& cg, std::deque<cdfc_edge>& candidate_edges, std::vector<bool>& visited,
                                                      std::vector<bool>& cg_visited, std::vector<bool>& cdfc_visited)
{
   cdfc_out_edge_iterator oe_cdfc, oe_end_cdfc;
   if(start == src)
      return true;
   if(cg_visited[boost::get(boost::vertex_index, *cg, src)])
      return false;
   cg_visited[boost::get(boost::vertex_index, *cg, src)] = true;

   for(boost::tie(oe_cdfc, oe_end_cdfc) = boost::out_edges(src, *cdfc); oe_cdfc != oe_end_cdfc; ++oe_cdfc)
   {
      cdfc_vertex tgt = boost::target(*oe_cdfc, *cdfc);
      if(!visited[boost::get(boost::vertex_index, *cdfc, tgt)])
      {
         visited[boost::get(boost::vertex_index, *cdfc, tgt)] = true;
         cdfc_edge cg_e = *oe_cdfc;
         if((*cdfc)[cg_e].selector & COMPATIBILITY_EDGE)
         {
            if(false_loop_search_cdfc_1(tgt, level + 1, k, start, cdfc, cg, candidate_edges, visited, cg_visited, cdfc_visited))
            {
               candidate_edges.push_front(cg_e);
               return true;
            }
         }
         else
         {
            if(false_loop_search_cdfc_more(tgt, level, k, start, cdfc, cg, candidate_edges, visited, cg_visited, cdfc_visited))
               return true;
         }
         visited[boost::get(boost::vertex_index, *cdfc, tgt)] = false;
      }
   }
   return false;
}

bool cdfc_module_binding::can_be_clustered(vertex v, OpGraphConstRef fsdg, fu_bindingConstRef fu, const CustomUnorderedMap<vertex, double>& slack_time, const double mux_time)
{
   const AllocationInformationConstRef allocation_information = HLS->allocation_information;
   if(can_be_clustered_table.find(v) != can_be_clustered_table.end())
      return can_be_clustered_table.find(v)->second;
   if(GET_TYPE(fsdg, v) & (TYPE_ENTRY | TYPE_EXIT | TYPE_PHI | TYPE_VPHI | TYPE_GOTO | TYPE_LABEL | TYPE_RET | TYPE_SWITCH | TYPE_MULTIIF | TYPE_IF | TYPE_EXTERNAL))
   {
      can_be_clustered_table[v] = false;
      return false;
   }
   if(slack_time.find(v)->second < 2 * mux_time)
   {
      can_be_clustered_table[v] = false;
      return false;
   }
   unsigned int fu_s1 = fu->get_assign(v);
   /*
   HLS->Rliv->set_HLS(HLS);
   std::string res_name = HLS->allocation_information->get_fu_name(fu_s1).first;
   std::string lib_name = HLS->HLS_T->get_technology_manager()->get_library(res_name);
   bool disabling_slack_based_binding = (HLS->allocation_information->get_number_channels(fu_s1) >= 1) ||
                                        lib_name  == WORK_LIBRARY || lib_name == PROXY_LIBRARY ||
                                        (HLS->allocation_information->get_number_fu(fu_s1) != INFINITE_UINT && !HLS->allocation_information->is_indirect_access_memory_unit(fu_s1));
   if(disabling_slack_based_binding)
   {
      can_be_clustered_table[v] = false;
      return false;
   }
    if(GET_TYPE(fsdg, v) & (TYPE_ENTRY|TYPE_EXIT)) return false;
   if((GET_TYPE(fsdg, v) & (TYPE_LOAD  | TYPE_STORE)) ||
         fsdg->CGetOpNodeInfo(v)->GetOperation() == STORE ||
         fsdg->CGetOpNodeInfo(v)->GetOperation() == MEMSET ||
         fsdg->CGetOpNodeInfo(v)->GetOperation() == MEMCPY ||
         fsdg->CGetOpNodeInfo(v)->GetOperation() == MEMCMP)
   {
      can_be_clustered_table[v] = false;
      return false;
   }

   OutEdgeIterator oe_fsdg, oe_end_fsdg;
   bool is_first=true;
   bool conflict_p = false;
   for(tie(oe_fsdg, oe_end_fsdg) = boost::out_edges(v, *fsdg); oe_fsdg != oe_end_fsdg; ++oe_fsdg)
   {
      if(GET_TYPE(fsdg, boost::target(*oe_fsdg, *fsdg)) & (TYPE_ENTRY|TYPE_EXIT|TYPE_PHI|TYPE_VPHI|TYPE_GOTO|TYPE_LABEL|TYPE_RET|TYPE_SWITCH|TYPE_MULTIIF|TYPE_IF|TYPE_EXTERNAL)) continue;
      if(is_first)
      {
         conflict_p = HLS->Rliv->are_in_conflict(v, boost::target(*oe_fsdg, *fsdg));
         is_first = false;
      }
      else
      {
         bool curr_conflict_p = HLS->Rliv->are_in_conflict(v, boost::target(*oe_fsdg, *fsdg));
         if(curr_conflict_p != conflict_p)
         {
            can_be_clustered_table[v] = false;
            return false;
         }
      }
   }
*/

   std::vector<HLS_manager::io_binding_type> vars_read1 = HLSMgr->get_required_values(HLS->functionId, v);
   if(vars_read1.size() > 1)
   {
      double resource_area = allocation_information->compute_normalized_area(fu_s1);
      double exec_time =
          allocation_information->get_worst_execution_time(fu_s1) -
          allocation_information->get_correction_time(fu_s1, fsdg->CGetOpNodeInfo(v)->GetOperation(), static_cast<unsigned>(fsdg->CGetOpNodeInfo(v)->GetVariables(FunctionBehavior_VariableType::SCALAR, FunctionBehavior_VariableAccessType::USE).size()));
      // double stage_time = allocation_information->get_worst_stage_period(fu_s1);
      // if(exec_time == 0.0 && stage_time == 0.0) return true;
      if(exec_time < 1.0 || resource_area < 0.5)
      {
         can_be_clustered_table[v] = false;
         return false;
      }
   }
   can_be_clustered_table[v] = true;
   return true;
}

int cdfc_module_binding::weight_computation(bool cond1, bool cond2, vertex v1, vertex v2, const double mux_time,
                                            const OpGraphConstRef
#ifndef NDEBUG
                                                fsdg
#endif
                                            ,
                                            fu_bindingConstRef
#ifdef HC_APPROACH
                                                fu
#endif
                                            ,
                                            const CustomUnorderedMap<vertex, double>& slack_time, CustomUnorderedMap<vertex, double>& starting_time,
#ifdef HC_APPROACH
                                            spec_hierarchical_clustering& hc,
#endif
                                            connection_relation& con_rel, double controller_delay, unsigned int prec)
{
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Weight computation of " + GET_NAME(fsdg, v1) + "-->" + GET_NAME(fsdg, v2));
   size_t _w = 1;
   size_t threshold1, threshold2;
   size_t in1 = con_rel.find(v1)->second.size();
   size_t in2 = con_rel.find(v2)->second.size();
   unsigned int total_muxes = 0;
   if(in1 == in2)
   {
      // std::cerr << "same number of operand" << std::endl;
      size_t n_inputs = in1;
      threshold1 = 2 * n_inputs;
      std::map<vertex, vertex> converter;
      std::vector<vertex> cluster(2);
      converter[v1] = v1;
      cluster[0] = v1;
      converter[v2] = v2;
      cluster[1] = v2;
      unsigned int n_shared;
      double mux_area_estimation, mux_time_estimation;
      estimate_muxes<false, false>(con_rel, prec, mux_time_estimation, mux_area_estimation, cluster, total_muxes, n_shared, converter, HLSMgr, HLS, debug_level);
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---total_muxes=" + STR(total_muxes) + " n_shared=" + STR(n_shared) + " n_inputs=" + STR(n_inputs));
      // std::cerr << "total_muxes " << total_muxes << " n_shared " << n_shared << " n_inputs " << n_inputs << " " << GET_OP(fsdg, v1) + "-" + GET_OP(fsdg, v2) << std::endl;

      if(total_muxes > n_inputs)
         _w = n_inputs;
      else
         _w = 1 + n_inputs + n_inputs - total_muxes;
      _w += n_shared;
      threshold2 = threshold1 + n_shared;
   }
   else
   {
      threshold1 = 2 * std::max(in1, in2);
      threshold2 = threshold1;
   }
   // std::cerr << "_w=" << _w << std::endl;

   double max_starting_time = std::max(starting_time.find(v1)->second, starting_time.find(v2)->second);
   if(max_starting_time < controller_delay && total_muxes > 0)
      max_starting_time = controller_delay;
   double v1_slack, v2_slack;
   v1_slack = slack_time.find(v1)->second - (max_starting_time - starting_time.find(v1)->second);
   v2_slack = slack_time.find(v2)->second - (max_starting_time - starting_time.find(v2)->second);

   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---cond1=" + (cond1 ? std::string("T") : std::string("F")) + " cond2=" + (cond2 ? std::string("T") : std::string("F")));
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                  "---Weight before=" + STR(_w) + " v1_slack=" + STR(v1_slack) + " v2_slack=" + STR(v2_slack) + " threshold=" + STR(threshold1) + " resource_type=" + " mux_time=" + STR(mux_time) + " prec=" + STR(prec));

   if(cond1 && _w <= threshold1 && ((v1_slack < mux_time) || (v2_slack < mux_time)))
      _w = 0;
#ifdef HC_APPROACH
   int _w_saved = _w;
   if(can_be_clustered(v1, fsdg, fu, slack_time, mux_time) && can_be_clustered(v2, fsdg, fu, slack_time, mux_time))
   {
      double p_weight = hc.pair_weight(boost::get(boost::vertex_index, *fsdg, v1), boost::get(boost::vertex_index, *fsdg, v2));
      int delta = static_cast<int>(static_cast<double>(threshold1) * p_weight);
      if(p_weight >= 1.0)
         _w += delta;
   }
   if(_w != _w_saved)
   {
      std::cerr << "Before " << _w_saved << " " << GET_NAME(fsdg, v1) << "(" << fsdg->CGetOpNodeInfo(v1)->GetOperation() << ")-"
                << "-" << GET_NAME(fsdg, v2) << std::endl;
      std::cerr << "After " << _w << std::endl;
   }
#endif
   else if((cond2 && _w <= threshold2))
      _w = 0;

   if(_w > 31)
      _w = 31;
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Weight of " + GET_NAME(fsdg, v1) + "-->" + GET_NAME(fsdg, v2) + " is " + STR(_w));
   return static_cast<int>(_w);
}
