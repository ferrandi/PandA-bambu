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
 *              Copyright (C) 2004-2023 Politecnico di Milano
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
 * @file network_flow.hpp
 * @brief
 *
 *  This class contains a network flow graph representation and min cost flow algorithm
 *
 * @author Fabrizio Castro <castro.fabrizio@gmail.com>
 * @author Simone   Froio  <simonefroio@hotmail.it>
 * @version $Revision:
 * @date
 */
#ifndef NETWORK_FLOW_HPP
#define NETWORK_FLOW_HPP

#include "custom_set.hpp"
#include "graph.hpp"
#include "refcount.hpp"
#include <iosfwd>
#include <vector>

#define I2S(number) std::to_string(number)

class network_flow
{
 private:
   /**vertex tags*/

   struct nf_name_t
   {
      using kind = boost::vertex_property_tag;
   };

   struct nf_index_t
   {
      using kind = boost::vertex_property_tag;
   };

   struct nf_distance_t
   {
      using kind = boost::vertex_property_tag;
   };

   struct nf_potential_t
   {
      using kind = boost::vertex_property_tag;
   };

   struct nf_imbalance_t
   {
      using kind = boost::vertex_property_tag;
   };

   struct nf_balance_t
   {
      using kind = boost::vertex_property_tag;
   };

   struct nf_vertex_description_t
   {
      using kind = boost::vertex_property_tag;
   };

   /**edge tags*/

   struct nf_cost_t
   {
      using kind = boost::edge_property_tag;
   };

   struct nf_capacity_t
   {
      using kind = boost::edge_property_tag;
   };

   struct nf_flow_t
   {
      using kind = boost::edge_property_tag;
   };

   struct nf_reduced_cost_t
   {
      using kind = boost::edge_property_tag;
   };

   struct nf_residual_capacity_t
   {
      using kind = boost::edge_property_tag;
   };

   struct nf_pseudo_flow_t
   {
      using kind = boost::edge_property_tag;
   };

   struct nf_edge_description_t
   {
      using kind = boost::edge_property_tag;
   };

   /**vertex properties*/

   using vertex_nf_description_property = boost::property<nf_vertex_description_t, std::string>;

   using vertex_nf_balance_property = boost::property<nf_balance_t, double, vertex_nf_description_property>;

   using vertex_nf_imbalance_property = boost::property<nf_imbalance_t, double, vertex_nf_balance_property>;

   using vertex_nf_potential_property = boost::property<nf_potential_t, double, vertex_nf_imbalance_property>;

   using vertex_nf_distance_property = boost::property<nf_distance_t, double, vertex_nf_potential_property>;

   using vertex_nf_index_property = boost::property<nf_index_t, unsigned long, vertex_nf_distance_property>;

   using vertex_nf_property = boost::property<nf_name_t, std::string, vertex_nf_index_property>;

   /**edge properties*/

   using edge_nf_description_property = boost::property<nf_edge_description_t, std::string>;

   using edge_nf_pseudo_flow_property = boost::property<nf_pseudo_flow_t, double, edge_nf_description_property>;

   using edge_nf_residual_capacity_property =
       boost::property<nf_residual_capacity_t, double, edge_nf_pseudo_flow_property>;

   using edge_nf_reduced_cost_property = boost::property<nf_reduced_cost_t, double, edge_nf_residual_capacity_property>;

   using edge_nf_flow_property = boost::property<nf_flow_t, double, edge_nf_reduced_cost_property>;

   using edge_nf_capacity_property = boost::property<nf_capacity_t, double, edge_nf_flow_property>;

   using edge_nf_property = boost::property<nf_cost_t, double, edge_nf_capacity_property>;

 public:
   /**definition of the Network Flow Graph*/
   using network_flow_graph_type =
       boost::adjacency_list<boost::vecS, boost::vecS, boost::bidirectionalS, vertex_nf_property, edge_nf_property>;

 private:
   /**vertex maps*/

   using pmap_nf_vertex_description_t = boost::property_map<network_flow_graph_type, nf_vertex_description_t>::type;

   using pmap_nf_balance_t = boost::property_map<network_flow_graph_type, nf_balance_t>::type;

   using pmap_nf_imbalance_t = boost::property_map<network_flow_graph_type, nf_imbalance_t>::type;

   using pmap_nf_potential_t = boost::property_map<network_flow_graph_type, nf_potential_t>::type;

   using pmap_nf_distance_t = boost::property_map<network_flow_graph_type, nf_distance_t>::type;

   using pmap_nf_index_t = boost::property_map<network_flow_graph_type, nf_index_t>::type;

   using pmap_nf_name_t = boost::property_map<network_flow_graph_type, nf_name_t>::type;

   /**edge maps*/

   using pmap_nf_edge_description_t = boost::property_map<network_flow_graph_type, nf_edge_description_t>::type;

   using pmap_nf_pseudo_flow_t = boost::property_map<network_flow_graph_type, nf_pseudo_flow_t>::type;

   using pmap_nf_residual_capacity_t = boost::property_map<network_flow_graph_type, nf_residual_capacity_t>::type;

   using pmap_nf_reduced_cost_t = boost::property_map<network_flow_graph_type, nf_reduced_cost_t>::type;

   using pmap_nf_flow_t = boost::property_map<network_flow_graph_type, nf_flow_t>::type;

   using pmap_nf_capacity_t = boost::property_map<network_flow_graph_type, nf_capacity_t>::type;

   using pmap_nf_cost_t = boost::property_map<network_flow_graph_type, nf_cost_t>::type;

 public:
   /**Network flow graph pointer*/
   network_flow_graph_type network_flow_graph;

   /**Vertex maps variables*/

   pmap_nf_vertex_description_t p_nf_vertex_description;

   pmap_nf_balance_t p_nf_balance;

   pmap_nf_imbalance_t p_nf_imbalance;

   pmap_nf_potential_t p_nf_potential;

   pmap_nf_distance_t p_nf_distance;

   pmap_nf_index_t p_nf_index;

   pmap_nf_name_t p_nf_name;

   /**Edge maps variables*/

   pmap_nf_edge_description_t p_nf_edge_description;

   pmap_nf_pseudo_flow_t p_nf_pseudo_flow;

   pmap_nf_residual_capacity_t p_nf_residual_capacity;

   pmap_nf_reduced_cost_t p_nf_reduced_cost;

   pmap_nf_flow_t p_nf_flow;

   pmap_nf_capacity_t p_nf_capacity;

   pmap_nf_cost_t p_nf_cost;

   /**
    * Constructor of the class.
    * @param debug_level_ is the debug level
    */
   explicit network_flow(int _debug_level);

   /**
    * Destructor of the class.
    */
   ~network_flow() = default;

   /**
    * Computes the solution for the min cost flow problem with successive shortest path algorithm
    * @return true if it finds the solution, false otherwise
    */
   bool successive_shortest_path_algorithm();

   /**
    * Clears results of a previous computation
    */
   void clear_results();

   /**
    * Prints a textual description of the graph in a .dot file format.
    * @param file_name is the name of the text file
    * @return true if everything gone good, false otherwise
    */
   bool print_graph(const char* file_name = "network_flow.dot");

 private:
   using vertex_pair =
       std::pair<network_flow_graph_type::vertex_descriptor, network_flow_graph_type::vertex_descriptor>;
   CustomOrderedSet<vertex_pair> inserted_edges; // used to remember the edges added to represent the residual network

   int debug_level; // the debug level

   /**
    * Generates the description string of the vertex vd.
    * Called by print_graph function and used to fill the vertex_nf_description_property.
    * Useful to printing methods
    * @param vd is the vertex
    * @return the description string
    */
   std::string get_vertex_description(network_flow_graph_type::vertex_descriptor vd);

   /**
    * Generates the description string of the edge ed.
    * Called by print_graph function and used to fill the edge_nf_description_property.
    * Useful to printing methods
    * @param ed is the edge
    * @return the description string
    */
   std::string get_edge_description(network_flow_graph_type::edge_descriptor ed);

   /**
    * Updates the vertex imbalance
    * @param vd is the vertex
    */
   void update_vertex_imbalance(network_flow_graph_type::vertex_descriptor vd);

   /**
    * Implements the generic label correcting algorithm for vertex distances labelling
    * @param source is the source vertex
    * @param target is the target vertex
    * @param P is the shortest path from source to target
    */
   void generic_label_correcting_algorithm(network_flow_graph_type::vertex_descriptor source,
                                           network_flow_graph_type::vertex_descriptor target,
                                           std::vector<network_flow_graph_type::edge_descriptor>* P);
};

/// refcount definition of the class
using network_flowRef = refcount<network_flow>;

#endif
