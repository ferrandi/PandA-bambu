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
 * @file module_binding_check.hpp
 * @author Stefano Bodini, Federico Badini, Fabrizio Ferrandi
 *
 */
#ifndef MODULE_BINDING_CHECK_HPP
#define MODULE_BINDING_CHECK_HPP
#include "allocation_information.hpp"
#include "check_clique.hpp"
#include "clique_covering_graph.hpp"
#include "filter_clique.hpp"
#include "fu_binding.hpp"
#include "hls.hpp"
#include "liveness.hpp"

#include <boost/property_map/property_map.hpp>

/// STD include
#include <algorithm>
#include <limits>
#include <utility>
#include <vector>

#include "custom_map.hpp"
#include "custom_set.hpp"

class fu_binding;
class module_register_binding_spec
{
 private:
   typedef CustomUnorderedMap<unsigned int, std::size_t> tree_index_rank_t;
   typedef CustomUnorderedMapUnstable<unsigned int, unsigned int> tree_index_parent_t;
   typedef boost::associative_property_map<tree_index_rank_t> tree_index_rank_map_t;
   typedef boost::associative_property_map<tree_index_parent_t> tree_index_parent_map_t;
   typedef boost::disjoint_sets<tree_index_rank_map_t, tree_index_parent_map_t> tree_index_dsets_t;

   tree_index_rank_t tree_index_rank_map;
   tree_index_parent_t tree_index_parent_map;
   tree_index_rank_map_t tree_index_rank_pmap;
   tree_index_parent_map_t tree_index_parent_pmap;

 public:
   tree_index_dsets_t binding;

   module_register_binding_spec() : tree_index_rank_pmap(tree_index_rank_map), tree_index_parent_pmap(tree_index_parent_map), binding(tree_index_rank_pmap, tree_index_parent_pmap)
   {
   }
};

template <typename vertex_type>
struct module_binding_check : public check_clique<vertex_type>
{
 private:
   /**
    * slack associated with the Op
    */
   CustomUnorderedMap<C_vertex, double> opSlacks;

   /**
    * the set of input to every port of every clique.
    * inputvariables[vertex][port_index] gives the set of ssa variables
    * that are needed at port port_index by the clique represented by
    * the node vertex
    */
   CustomUnorderedMap<C_vertex, std::vector<CustomOrderedSet<unsigned int>>> input_variables;

   /// resource precision
   unsigned int fu_prec;

   /// area resource
   double area_resource;

   /// store the current state for binding
   module_register_binding_spec& tree_index_dsets;

   /// reference to HLS data structure
   const hlsRef HLS;

   /// reference to the HLS Manager
   const HLS_managerRef HLSMgr;

   /// reference to the vertex slack
   const CustomUnorderedMap<vertex, double>& slack_time;

   /// reference to the vertex starting time
   const CustomUnorderedMap<vertex, double>& starting_time;

   /// controller delay
   double controller_delay;

   /**
    Takes as input the vertex of an operation, and returns a vertex.
    Each element of the vertex is a set containing only the identifier of the variable that
    is required by the given operation on that port
    */
   std::vector<CustomOrderedSet<unsigned int>> getOperationVariablesAtPort(vertex& operationVertex) const
   {
      std::vector<CustomOrderedSet<unsigned int>> variableVector;

      std::vector<HLS_manager::io_binding_type> vars_read = HLSMgr->get_required_values(HLS->functionId, operationVertex);
      for(auto& port_index : vars_read)
      {
         unsigned int tree_var = std::get<0>(port_index);
         CustomOrderedSet<unsigned int> variablesAtPort;
         if(tree_var != 0)
            variablesAtPort.insert(tree_var);
         variableVector.push_back(variablesAtPort);
      }

      return variableVector;
   }

 protected:
   /**
    Takes as input the node representing the operation and returns the
    maximum number of muxes that can be connected before the fu that
    implements the operation without compromising the delay of the circuit
    */
   virtual double getOpSlack(vertex& operationVertex) const
   {
      if(starting_time.find(operationVertex)->second > controller_delay)
         return slack_time.find(operationVertex)->second;
      else if(controller_delay - starting_time.find(operationVertex)->second > slack_time.find(operationVertex)->second)
         return 0;
      else
         return (slack_time.find(operationVertex)->second - (controller_delay - starting_time.find(operationVertex)->second));
   }

   bool is_disabled_slack_based_binding;

 public:
   //-----------------------------------------------------------------------------------------------
   // methods

   module_binding_check(unsigned int _fu_prec, double _area_resource, const hlsRef _HLS, const HLS_managerRef _HLSMgr, const CustomUnorderedMap<vertex, double>& _slack_time, const CustomUnorderedMap<vertex, double>& _starting_time,
                        double _controller_delay, module_register_binding_spec& _tree_index_dsets)
       : fu_prec(_fu_prec), area_resource(_area_resource), tree_index_dsets(_tree_index_dsets), HLS(_HLS), HLSMgr(_HLSMgr), slack_time(_slack_time), starting_time(_starting_time), controller_delay(_controller_delay), is_disabled_slack_based_binding(false)
   {
   }

   module_binding_check(const module_binding_check& original)
       : opSlacks(original.opSlacks),
         input_variables(original.input_variables),
         fu_prec(original.fu_prec),
         area_resource(original.area_resource),
         tree_index_dsets(original.tree_index_dsets),
         HLS(original.HLS),
         HLSMgr(original.HLSMgr),
         slack_time(original.slack_time),
         starting_time(original.starting_time),
         controller_delay(original.controller_delay),
         is_disabled_slack_based_binding(original.is_disabled_slack_based_binding)
   {
   }

   module_binding_check* clone() const override
   {
      return new module_binding_check(*this);
   }

   module_binding_check& operator=(const module_binding_check&) = delete;

   ~module_binding_check() override = default;

   void initialize_structures(boost_cc_compatibility_graph& graph, std::map<C_vertex, vertex_type>& Ruv2v) override
   {
      BOOST_FOREACH(C_vertex tempVertex, boost::vertices(graph))
      {
         opSlacks[tempVertex] = getOpSlack(Ruv2v[tempVertex]);
         input_variables[tempVertex] = getOperationVariablesAtPort(Ruv2v[tempVertex]);
      }

      CustomOrderedSet<unsigned int> tree_index_set;
      CustomUnorderedMap<std::pair<unsigned int, unsigned int>, unsigned int> tree_var_resource_relation;
      for(auto input_var : input_variables)
      {
         for(auto vars : input_var.second)
         {
            for(auto tree_var : vars)
            {
               tree_index_set.insert(tree_var);
               tree_index_dsets.binding.make_set(tree_var);
               if(HLS->Rliv->has_op_where_defined(tree_var))
               {
                  vertex def_op = HLS->Rliv->get_op_where_defined(tree_var);
                  if(HLS->Rfu->is_assigned(def_op))
                  {
                     unsigned int fu_name = HLS->Rfu->get_assign(def_op);
                     unsigned int fu_index = HLS->Rfu->get_index(def_op);
                     if(fu_index != INFINITE_UINT)
                     {
                        if(tree_var_resource_relation.find(std::make_pair(fu_name, fu_index)) != tree_var_resource_relation.end())
                        {
                           tree_index_dsets.binding.union_set(tree_var, tree_var_resource_relation.find(std::make_pair(fu_name, fu_index))->second);
                        }
                        else
                           tree_var_resource_relation[std::make_pair(fu_name, fu_index)] = tree_var;
                     }
                  }
               }
            }
         }
      }
   }

   double cost(size_t clique_count) override
   {
      double area_muxes = 0;
      for(auto input_var : input_variables)
         for(auto vars : input_var.second)
         {
            if(vars.size() > 1)
               area_muxes += HLS->allocation_information->estimate_muxNto1_area(fu_prec, static_cast<unsigned int>(vars.size()));
         }
      // std::cerr << "n_muxes " << n_muxes << " area_mux " << area_mux << " area_resource " << area_resource << " clique_count " << clique_count << " Area " << n_muxes*area_mux+area_resource*static_cast<double>(clique_count) << std::endl;
      return area_muxes + area_resource * static_cast<double>(clique_count);
   }

   size_t num_mux() override
   {
      size_t n_muxes = 0;
      for(auto input_var : input_variables)
         for(auto vars : input_var.second)
         {
            if(vars.size() > 1)
               n_muxes += static_cast<size_t>(vars.size()) - 1;
         }
      return n_muxes;
   }

   void update_after_join(C_vertex& rep, C_vertex& child) override
   {
      // aggiungo tutte le variabili in ingresso a child all'ingresso di rep

      //  PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, 11, "prima di definizione di port number");
      size_t port_number = input_variables[child].size();
      //  PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, 11, "definita port number");
      std::vector<CustomOrderedSet<unsigned int>>& input_at_port = input_variables[rep];
      std::vector<CustomOrderedSet<unsigned int>>& input_at_child = input_variables[child];
      for(size_t i = 0; i < port_number; i++)
      {
         //  PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, 11, "considero nuova porta");
         BOOST_FOREACH(unsigned int temp_var, input_at_child[i])
         {
            //    PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, 11, "considero nuova variabile di porta");
            temp_var = tree_index_dsets.binding.find_set(temp_var);
            (input_at_port[i]).insert(temp_var);
         }
         input_at_child[i].clear();
      }
      //  PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, 11, "uscito dal doppio ciclo");
      // imposto R_opSlacks di rep al minimo tra quelle di rep e di child
      opSlacks[rep] = std::min(opSlacks[rep], opSlacks[child]);
      //  PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, 11, "impostato anche max multiplexer");
   }

   /*
    returns false if the two vertex are not compatible, true if the
    two vertex can be inserted in the same clique
    */
   bool check_edge_compatibility(C_vertex& rep, C_vertex& other) override
   {
      double minSlack = std::min(opSlacks[rep], opSlacks[other]);
      THROW_ASSERT(input_variables.find(rep) != input_variables.end(), "unexpected case");

      size_t port_number = input_variables[rep].size();
      CustomOrderedSet<unsigned int> port_inputs;
      CustomOrderedSet<unsigned int> port_inputs_rep, port_inputs_other;
      double total_area_muxes_rep = 0, total_area_muxes_other = 0, total_area_muxes = 0;

      for(decltype(port_number) i = 0; i < port_number; i++)
      {
         for(auto temp_var : input_variables[rep][i])
         {
            temp_var = tree_index_dsets.binding.find_set(temp_var);
            port_inputs.insert(temp_var);
            port_inputs_rep.insert(temp_var);
         }
         for(auto temp_var : input_variables[other][i])
         {
            temp_var = tree_index_dsets.binding.find_set(temp_var);
            port_inputs.insert(temp_var);
            port_inputs_other.insert(temp_var);
         }
         size_t n_mux_inputs = static_cast<size_t>(port_inputs.size());

         total_area_muxes += HLS->allocation_information->estimate_muxNto1_area(fu_prec, static_cast<unsigned int>(n_mux_inputs));
         total_area_muxes_rep += HLS->allocation_information->estimate_muxNto1_area(fu_prec, static_cast<unsigned int>(port_inputs_rep.size()));
         total_area_muxes_other += HLS->allocation_information->estimate_muxNto1_area(fu_prec, static_cast<unsigned int>(port_inputs_other.size()));

         if(!is_disabled_slack_based_binding && n_mux_inputs > 1 && HLS->allocation_information->estimate_muxNto1_area(fu_prec, static_cast<unsigned int>(n_mux_inputs)) > area_resource)
            return false;

         if(n_mux_inputs > 1 && HLS->allocation_information->estimate_muxNto1_delay(fu_prec, static_cast<unsigned int>(n_mux_inputs)) > minSlack)
            return false;

         port_inputs.clear();
         port_inputs_rep.clear();
         port_inputs_other.clear();
      }
      // if(total_area_muxes>0)
      // std::cerr << "total_area_muxes " << total_area_muxes << " total_area_muxes_rep " << total_area_muxes_rep << " total_area_muxes_other " << total_area_muxes_other << std::endl;
      if(!is_disabled_slack_based_binding && ((total_area_muxes + area_resource) > (total_area_muxes_rep + area_resource + total_area_muxes_other + area_resource)))
         return false;

      return true;
   }

   bool check_no_mux_needed(C_vertex& rep, C_vertex& other) override
   {
      size_t port_number = input_variables[other].size();
      CustomOrderedSet<size_t> port_inputs;

      for(decltype(port_number) i = 0; i < port_number; i++)
      {
         for(auto temp_var : input_variables[rep][i])
         {
            temp_var = tree_index_dsets.binding.find_set(temp_var);
            port_inputs.insert(temp_var);
         }
         for(auto temp_var : input_variables[other][i])
         {
            temp_var = tree_index_dsets.binding.find_set(temp_var);
            port_inputs.insert(temp_var);
         }
         if(port_inputs.size() > 1)
            return false;

         port_inputs.clear();
      }

      return true;
   }
};

template <typename vertex_type>
struct module_binding_check_no_filter : public module_binding_check<vertex_type>
{
   module_binding_check_no_filter(unsigned int _fu_prec, double _area_resource, const hlsRef _HLS, const HLS_managerRef _HLSMgr, const CustomUnorderedMap<vertex, double>& _slack_time, const CustomUnorderedMap<vertex, double>& _starting_time,
                                  double _controller_delay, module_register_binding_spec& _tree_index_dsets)
       : module_binding_check<vertex_type>(_fu_prec, _area_resource, _HLS, _HLSMgr, _slack_time, _starting_time, _controller_delay, _tree_index_dsets)
   {
      module_binding_check<vertex_type>::is_disabled_slack_based_binding = true;
   }

   module_binding_check_no_filter(const module_binding_check_no_filter& original) : module_binding_check<vertex_type>(original)
   {
   }

 protected:
   double getOpSlack(vertex&) const override
   {
      return std::numeric_limits<unsigned int>::max();
   }
};

#endif // MODULE_BINDING_CHECK_HPP
