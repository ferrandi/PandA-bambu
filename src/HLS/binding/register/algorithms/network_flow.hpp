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

#include "refcount.hpp"

#include "custom_map.hpp"
#include "custom_set.hpp"
#include <iosfwd>
#include <vector>

#include "graph.hpp"

#include "boost/lexical_cast.hpp"

#define I2S(number) std::to_string(number)

class network_flow
{
 private:
   /**vertex tags*/

   struct nf_name_t
   {
      typedef boost::vertex_property_tag kind;
   };

   struct nf_index_t
   {
      typedef boost::vertex_property_tag kind;
   };

   struct nf_distance_t
   {
      typedef boost::vertex_property_tag kind;
   };

   struct nf_potential_t
   {
      typedef boost::vertex_property_tag kind;
   };

   struct nf_imbalance_t
   {
      typedef boost::vertex_property_tag kind;
   };

   struct nf_balance_t
   {
      typedef boost::vertex_property_tag kind;
   };

   struct nf_vertex_description_t
   {
      typedef boost::vertex_property_tag kind;
   };

   /**edge tags*/

   struct nf_cost_t
   {
      typedef boost::edge_property_tag kind;
   };

   struct nf_capacity_t
   {
      typedef boost::edge_property_tag kind;
   };

   struct nf_flow_t
   {
      typedef boost::edge_property_tag kind;
   };

   struct nf_reduced_cost_t
   {
      typedef boost::edge_property_tag kind;
   };

   struct nf_residual_capacity_t
   {
      typedef boost::edge_property_tag kind;
   };

   struct nf_pseudo_flow_t
   {
      typedef boost::edge_property_tag kind;
   };

   struct nf_edge_description_t
   {
      typedef boost::edge_property_tag kind;
   };

   /**vertex properties*/

   typedef boost::property<nf_vertex_description_t, std::string> vertex_nf_description_property;

   typedef boost::property<nf_balance_t, double, vertex_nf_description_property> vertex_nf_balance_property;

   typedef boost::property<nf_imbalance_t, double, vertex_nf_balance_property> vertex_nf_imbalance_property;

   typedef boost::property<nf_potential_t, double, vertex_nf_imbalance_property> vertex_nf_potential_property;

   typedef boost::property<nf_distance_t, double, vertex_nf_potential_property> vertex_nf_distance_property;

   typedef boost::property<nf_index_t, long unsigned int, vertex_nf_distance_property> vertex_nf_index_property;

   typedef boost::property<nf_name_t, std::string, vertex_nf_index_property> vertex_nf_property;

   /**edge properties*/

   typedef boost::property<nf_edge_description_t, std::string> edge_nf_description_property;

   typedef boost::property<nf_pseudo_flow_t, double, edge_nf_description_property> edge_nf_pseudo_flow_property;

   typedef boost::property<nf_residual_capacity_t, double, edge_nf_pseudo_flow_property> edge_nf_residual_capacity_property;

   typedef boost::property<nf_reduced_cost_t, double, edge_nf_residual_capacity_property> edge_nf_reduced_cost_property;

   typedef boost::property<nf_flow_t, double, edge_nf_reduced_cost_property> edge_nf_flow_property;

   typedef boost::property<nf_capacity_t, double, edge_nf_flow_property> edge_nf_capacity_property;

   typedef boost::property<nf_cost_t, double, edge_nf_capacity_property> edge_nf_property;

 public:
   /**definition of the Network Flow Graph*/
   typedef boost::adjacency_list<boost::vecS, boost::vecS, boost::bidirectionalS, vertex_nf_property, edge_nf_property> network_flow_graph_type;

 private:
   /**vertex maps*/

   typedef boost::property_map<network_flow_graph_type, nf_vertex_description_t>::type pmap_nf_vertex_description_t;

   typedef boost::property_map<network_flow_graph_type, nf_balance_t>::type pmap_nf_balance_t;

   typedef boost::property_map<network_flow_graph_type, nf_imbalance_t>::type pmap_nf_imbalance_t;

   typedef boost::property_map<network_flow_graph_type, nf_potential_t>::type pmap_nf_potential_t;

   typedef boost::property_map<network_flow_graph_type, nf_distance_t>::type pmap_nf_distance_t;

   typedef boost::property_map<network_flow_graph_type, nf_index_t>::type pmap_nf_index_t;

   typedef boost::property_map<network_flow_graph_type, nf_name_t>::type pmap_nf_name_t;

   /**edge maps*/

   typedef boost::property_map<network_flow_graph_type, nf_edge_description_t>::type pmap_nf_edge_description_t;

   typedef boost::property_map<network_flow_graph_type, nf_pseudo_flow_t>::type pmap_nf_pseudo_flow_t;

   typedef boost::property_map<network_flow_graph_type, nf_residual_capacity_t>::type pmap_nf_residual_capacity_t;

   typedef boost::property_map<network_flow_graph_type, nf_reduced_cost_t>::type pmap_nf_reduced_cost_t;

   typedef boost::property_map<network_flow_graph_type, nf_flow_t>::type pmap_nf_flow_t;

   typedef boost::property_map<network_flow_graph_type, nf_capacity_t>::type pmap_nf_capacity_t;

   typedef boost::property_map<network_flow_graph_type, nf_cost_t>::type pmap_nf_cost_t;

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
   explicit network_flow(int debug_level_);

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
   typedef std::pair<network_flow_graph_type::vertex_descriptor, network_flow_graph_type::vertex_descriptor> vertex_pair;
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
   void generic_label_correcting_algorithm(network_flow_graph_type::vertex_descriptor source, network_flow_graph_type::vertex_descriptor target, std::vector<network_flow_graph_type::edge_descriptor>* P);
};

/// refcount definition of the class
typedef refcount<network_flow> network_flowRef;

#endif
