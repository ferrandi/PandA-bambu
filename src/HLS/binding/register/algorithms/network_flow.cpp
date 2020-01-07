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
 * @file network_flow.cpp
 * @brief
 *
 *  This class contains a network flow graph representation and min cost flow algorithm
 *
 * @author Fabrizio Castro <castro.fabrizio@gmail.com>
 * @author Simone   Froio  <simonefroio@hotmail.it>
 * @version $Revision:
 * @date
 */

#include "network_flow.hpp"
#include "Parameter.hpp"
#include "dbgPrintHelper.hpp"

#define initial_value 10000000
#pragma GCC diagnostic push
#pragma GCC diagnostic error "-Weffc++"

network_flow::network_flow(int _debug_level)
    : network_flow_graph(0),
      p_nf_vertex_description(boost::get(nf_vertex_description_t(), network_flow_graph)),
      p_nf_balance(boost::get(nf_balance_t(), network_flow_graph)),
      p_nf_imbalance(boost::get(nf_imbalance_t(), network_flow_graph)),
      p_nf_potential(boost::get(nf_potential_t(), network_flow_graph)),
      p_nf_distance(boost::get(nf_distance_t(), network_flow_graph)),
      p_nf_index(boost::get(nf_index_t(), network_flow_graph)),
      p_nf_name(boost::get(nf_name_t(), network_flow_graph)),
      p_nf_edge_description(boost::get(nf_edge_description_t(), network_flow_graph)),
      p_nf_pseudo_flow(boost::get(nf_pseudo_flow_t(), network_flow_graph)),
      p_nf_residual_capacity(boost::get(nf_residual_capacity_t(), network_flow_graph)),
      p_nf_reduced_cost(boost::get(nf_reduced_cost_t(), network_flow_graph)),
      p_nf_flow(boost::get(nf_flow_t(), network_flow_graph)),
      p_nf_capacity(boost::get(nf_capacity_t(), network_flow_graph)),
      p_nf_cost(boost::get(nf_cost_t(), network_flow_graph)),
      inserted_edges(),
      debug_level(_debug_level)
{
}

bool network_flow::successive_shortest_path_algorithm()
{
   network_flow_graph_type::vertex_iterator vi, vi_end;
   network_flow_graph_type::edge_iterator ei, ei_end;

   OrderedSetStd<network_flow_graph_type::vertex_descriptor> E, D;
   OrderedSetStd<network_flow_graph_type::vertex_descriptor>::iterator EDi;

   CustomOrderedSet<vertex_pair>::iterator inserted_edges_iterator;

   if(boost::num_vertices(network_flow_graph) == 0)
   {
      return false;
   }

   /*creation and initialization of the residual network*/
   // distance = inf, potential = 0, e(i) = b(i), E:={i:e(i)>0} D:={i:e(i)<0}
   E.clear();
   D.clear();
   // std::cerr << "number of vertices into the flow graph = " << boost::num_vertices(network_flow_graph) << std::endl;
   for(boost::tie(vi, vi_end) = boost::vertices(network_flow_graph); vi != vi_end; vi++)
   {
      p_nf_distance[*vi] = initial_value;
      p_nf_potential[*vi] = 0;
      p_nf_imbalance[*vi] = p_nf_balance[*vi];
      // filling E and D sets
      if(p_nf_imbalance[*vi] > 0)
      {
         E.insert(*vi);
      }
      if(p_nf_imbalance[*vi] < 0)
         D.insert(*vi);
   }
   // flow_ij = 0, pseudo_flow_ij = 0, rcij = cij, ruij = uij
   inserted_edges.clear();
   for(boost::tie(ei, ei_end) = boost::edges(network_flow_graph); ei != ei_end; ei++)
   {
      p_nf_flow[*ei] = 0;
      p_nf_pseudo_flow[*ei] = 0;
      p_nf_reduced_cost[*ei] = p_nf_cost[*ei];
      p_nf_residual_capacity[*ei] = p_nf_capacity[*ei];

      inserted_edges.insert(std::make_pair(boost::target(*ei, network_flow_graph), boost::source(*ei, network_flow_graph)));
   }

   if(debug_level >= DEBUG_LEVEL_VERBOSE)
      print_graph();

   // from (i,j) adding (j,i), cji = -cij, ruji = 0, flow_ji = 0, pseudo_flow_ji = 0, rcji = cji, uji = 0
   for(inserted_edges_iterator = inserted_edges.begin(); inserted_edges_iterator != inserted_edges.end(); ++inserted_edges_iterator)
   {
      network_flow_graph_type::edge_descriptor original, reverse;
      vertex_pair vertices;
      vertices = *inserted_edges_iterator;
      original = (boost::edge(vertices.second, vertices.first, network_flow_graph)).first;
      reverse = (boost::add_edge(vertices.first, vertices.second, network_flow_graph)).first;
      p_nf_cost[reverse] = -p_nf_cost[original];
      p_nf_residual_capacity[reverse] = 0;
      p_nf_flow[reverse] = 0;
      p_nf_pseudo_flow[reverse] = 0;
      p_nf_reduced_cost[reverse] = p_nf_cost[reverse];
      p_nf_capacity[reverse] = 0;
   }
   /*---------------------------------------------------------------------------*/

   int i = 1; // maintains iterations number

   while(!E.empty())
   {
      std::vector<network_flow_graph_type::edge_descriptor> P;
      std::vector<network_flow_graph_type::edge_descriptor>::iterator P_iterator;

      /*selection of K and I vertices*/
      network_flow_graph_type::vertex_descriptor k, I;

      k = *(E.begin());
      for(EDi = E.begin(); EDi != E.end(); ++EDi)
         if(p_nf_balance[*EDi] > p_nf_balance[k])
            k = *EDi;

      I = *(D.begin());
      for(EDi = D.begin(); EDi != D.end(); ++EDi)
         if(p_nf_balance[*EDi] < p_nf_balance[k])
            I = *EDi;
      /*---------------------------------------------------------------------------*/

      PRINT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "network_flow: K and I vertices selected ");

      /*shortest path distances computation and shortest path identification*/
      generic_label_correcting_algorithm(k, I, &P);
      if(P.empty())
      {
         clear_results();
         return false;
      }
      /*---------------------------------------------------------------------------*/

      PRINT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "network_flow: shortest path distances computed and shortest path identificated");

      /*update node potentials*/
      for(boost::tie(vi, vi_end) = boost::vertices(network_flow_graph); vi != vi_end; vi++)
         p_nf_potential[*vi] = p_nf_potential[*vi] - p_nf_distance[*vi];
      /*---------------------------------------------------------------------------*/

      PRINT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "network_flow: node potentials updated");

      /*update reduced costs*/
      for(boost::tie(ei, ei_end) = boost::edges(network_flow_graph); ei != ei_end; ei++)
      {
         p_nf_reduced_cost[*ei] = p_nf_cost[*ei] - p_nf_potential[boost::source(*ei, network_flow_graph)] + p_nf_potential[boost::target(*ei, network_flow_graph)];
      }
      /*---------------------------------------------------------------------------*/

      PRINT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "network_flow: reduced costs updated");

      /*sigma computation*/
      // sigma = min{e(k),-e(I),min{rij: (i,j) in P}}
      double sigma;

      sigma = initial_value;

      for(P_iterator = P.begin(); P_iterator != P.end(); ++P_iterator) // min{rij: (i,j) in P}
         if(p_nf_residual_capacity[*P_iterator] < sigma)
            sigma = p_nf_residual_capacity[*P_iterator];
      if(p_nf_imbalance[k] < sigma)
         sigma = p_nf_imbalance[k];

      if((-p_nf_imbalance[I]) < sigma)
         sigma = p_nf_imbalance[I];
      /*---------------------------------------------------------------------------*/

      PRINT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "network_flow: sigma computated");

      /*augment sigma units of pseudo flow along the path P, updating flows, residual capacity and imbalance*/
      for(P_iterator = P.begin(); P_iterator != P.end(); ++P_iterator)
      {
         network_flow_graph_type::vertex_descriptor source, target;
         network_flow_graph_type::out_edge_iterator oei, oei_end;

         source = boost::source(*P_iterator, network_flow_graph);
         target = boost::target(*P_iterator, network_flow_graph);

         p_nf_pseudo_flow[*P_iterator] += sigma; // pseudo flow update

         network_flow_graph_type::edge_descriptor ed;
         ed = boost::edge(target, source, network_flow_graph).first;

         if(inserted_edges.find(std::make_pair(source, target)) == inserted_edges.end())
         {
            // original edge
            p_nf_flow[*P_iterator] = p_nf_pseudo_flow[*P_iterator] - p_nf_pseudo_flow[ed];             // flow update
            p_nf_residual_capacity[*P_iterator] = p_nf_capacity[*P_iterator] - p_nf_flow[*P_iterator]; // residual capacity update
            p_nf_residual_capacity[ed] = p_nf_flow[*P_iterator];                                       // residual capacity update
         }
         else
         {
            // reversal edge
            p_nf_flow[ed] = p_nf_pseudo_flow[ed] - p_nf_pseudo_flow[*P_iterator]; // flow update
            p_nf_residual_capacity[ed] = p_nf_capacity[ed] - p_nf_flow[ed];       // residual capacity update
            p_nf_residual_capacity[*P_iterator] = p_nf_flow[ed];                  // residual capacity update
         }
      }

      for(boost::tie(vi, vi_end) = boost::vertices(network_flow_graph); vi != vi_end; vi++)
      {
         update_vertex_imbalance(*vi); // residual network update
      }
      /*---------------------------------------------------------------------------*/

      PRINT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "network_flow: updated path P, updated flows, residual capacity and imbalance");

      /*Update E and D sets*/
      E.clear();
      D.clear();
      for(boost::tie(vi, vi_end) = boost::vertices(network_flow_graph); vi != vi_end; vi++)
      {
         if(p_nf_imbalance[*vi] > 0)
         {
            E.insert(*vi);
         }
         if(p_nf_imbalance[*vi] < 0)
            D.insert(*vi);
      }
      /*---------------------------------------------------------------------------*/

      PRINT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "network_flow: E and D sets updated");

      if(debug_level >= DEBUG_LEVEL_VERY_PEDANTIC)
      {
         std::string file_name;
         file_name = "network_flow_graph_" + I2S(i) + ".dot";
         print_graph(file_name.data());
      }

      i++;
   }

   /*Removing reversal edges*/
   while(!inserted_edges.empty())
   {
      vertex_pair edge;
      edge = *inserted_edges.begin();
      boost::remove_edge(edge.first, edge.second, network_flow_graph);
      inserted_edges.erase(inserted_edges.begin());
   }
   /*---------------------------------------------------------------------------*/

   if(debug_level >= DEBUG_LEVEL_MINIMUM)
      print_graph("network_flow_min_cost_flow.dot");

   return true;
}

void network_flow::generic_label_correcting_algorithm(network_flow_graph_type::vertex_descriptor source, network_flow_graph_type::vertex_descriptor target, std::vector<network_flow_graph_type::edge_descriptor>* P)
{
   /*initialization step*/
   size_t n = boost::num_vertices(network_flow_graph);
   std::map<network_flow_graph_type::vertex_descriptor, network_flow_graph_type::vertex_descriptor> predecessors;
   network_flow_graph_type::vertex_iterator vi, vi_end;
   for(boost::tie(vi, vi_end) = boost::vertices(network_flow_graph); vi != vi_end; vi++)
   {
      p_nf_distance[*vi] = initial_value;
      predecessors[*vi] = n;
   }
   p_nf_distance[source] = 0;
   predecessors[source] = source;
   /*---------------------------------------------------------------------------*/

   /*distances computation, predecessors computation and vertex distance labelling*/
   network_flow_graph_type::edge_iterator ei, ei_end;
   bool condition_satisfied = true;
   while(condition_satisfied)
   {
      condition_satisfied = false;
      for(boost::tie(ei, ei_end) = boost::edges(network_flow_graph); ei != ei_end; ei++)
      {
         network_flow_graph_type::vertex_descriptor source_, target_;
         source_ = boost::source(*ei, network_flow_graph);
         target_ = boost::target(*ei, network_flow_graph);
         if(p_nf_distance[source_] != initial_value)
            if((p_nf_distance[target_] > p_nf_distance[source_] + p_nf_reduced_cost[*ei]) && p_nf_residual_capacity[*ei] > 0)
            {
               p_nf_distance[target_] = p_nf_distance[source_] + p_nf_reduced_cost[*ei];
               predecessors[target_] = source_;
               network_flow_graph_type::vertex_descriptor pred;
               pred = source_;
               while(pred != source)
               {
                  if(pred == target_)
                  {
                     P->clear();
                     return;
                  }
                  pred = predecessors[pred];
               }
               condition_satisfied = true;
            }
      }
   }
   /*---------------------------------------------------------------------------*/

   /*shortest path identification*/
   network_flow_graph_type::vertex_descriptor current_vertex, dest;
   P->clear();
   current_vertex = target;

   while(current_vertex != source)
   {
      bool exist;
      dest = current_vertex;
      current_vertex = predecessors[current_vertex];
      if(current_vertex == n)
      {
         P->clear();
         return;
      }
      network_flow_graph_type::edge_descriptor ed;
      tie(ed, exist) = boost::edge(current_vertex, dest, network_flow_graph);
      P->push_back(ed);
   }
   /*---------------------------------------------------------------------------*/
}

void network_flow::update_vertex_imbalance(network_flow_graph_type::vertex_descriptor vd)
{
   network_flow_graph_type::out_edge_iterator oei, oei_end;
   network_flow_graph_type::in_edge_iterator iei, iei_end;

   double outflow, inflow;

   inflow = 0;
   outflow = 0;

   // outflow
   for(boost::tie(oei, oei_end) = boost::out_edges(vd, network_flow_graph); oei != oei_end; oei++)
      outflow += p_nf_pseudo_flow[*oei];

   // inflow
   for(boost::tie(iei, iei_end) = boost::in_edges(vd, network_flow_graph); iei != iei_end; iei++)
      inflow += p_nf_pseudo_flow[*iei];

   p_nf_imbalance[vd] = p_nf_balance[vd] + inflow - outflow;
}

void network_flow::clear_results()
{
   /*Removing reversal edges*/
   while(!inserted_edges.empty())
   {
      vertex_pair edge;
      edge = *inserted_edges.begin();
      boost::remove_edge(edge.first, edge.second, network_flow_graph);
      inserted_edges.erase(inserted_edges.begin());
   }
   /*---------------------------------------------------------------------------*/

   /*Cleaning network_flow_graph vertices*/
   network_flow_graph_type::vertex_iterator vi, vi_end;
   for(boost::tie(vi, vi_end) = boost::vertices(network_flow_graph); vi != vi_end; vi++)
   {
      p_nf_vertex_description[*vi] = "";
      p_nf_imbalance[*vi] = 0;
      p_nf_potential[*vi] = 0;
      p_nf_distance[*vi] = initial_value;
   }
   /*---------------------------------------------------------------------------*/

   /*Cleaning network_flow_graph edges*/
   network_flow_graph_type::edge_iterator ei, ei_end;
   for(boost::tie(ei, ei_end) = boost::edges(network_flow_graph); ei != ei_end; ei++)
   {
      p_nf_edge_description[*ei] = "";
      p_nf_pseudo_flow[*ei] = 0;
      p_nf_residual_capacity[*ei] = 0;
      p_nf_reduced_cost[*ei] = p_nf_cost[*ei];
      p_nf_flow[*ei] = 0;
   }
   /*---------------------------------------------------------------------------*/
}

bool network_flow::print_graph(const char* file_name)
{
   network_flow_graph_type::vertex_iterator vi, vi_end;
   network_flow_graph_type::edge_iterator ei, ei_end;

   for(boost::tie(vi, vi_end) = boost::vertices(network_flow_graph); vi != vi_end; vi++)
   {
      p_nf_vertex_description[*vi] = get_vertex_description(*vi);
   }

   for(boost::tie(ei, ei_end) = boost::edges(network_flow_graph); ei != ei_end; ei++)
   {
      p_nf_edge_description[*ei] = get_edge_description(*ei);
   }

   std::ofstream dot_file(file_name);
   if(!dot_file)
      return false;
   boost::write_graphviz(dot_file, network_flow_graph, make_label_writer(p_nf_vertex_description), make_label_writer(p_nf_edge_description));
   dot_file.close();
   return true;
}

std::string network_flow::get_vertex_description(network_flow_graph_type::vertex_descriptor vd)
{
   std::string description;

   description = "name: " + p_nf_name[vd] + "\\nindex: " + I2S(p_nf_index[vd]) + "\\ndistance: " + I2S(p_nf_distance[vd]) + "\\npotential: " + I2S(p_nf_potential[vd]) + "\\nimbalance: " + I2S(p_nf_imbalance[vd]) + "\\nbalance: " + I2S(p_nf_balance[vd]);

   return description;
}

std::string network_flow::get_edge_description(network_flow_graph_type::edge_descriptor ed)
{
   std::string description;

   description = "cost: " + I2S(p_nf_cost[ed]) + "\\ncapacity: " + I2S(p_nf_capacity[ed]) + "\\nflow: " + I2S(p_nf_flow[ed]) + "\\nreduced cost: " + I2S(p_nf_reduced_cost[ed]) + "\\nresidual capacity: " + I2S(p_nf_residual_capacity[ed]) +
                 "\\npseudo flow: " + I2S(p_nf_pseudo_flow[ed]);

   return description;
}
#pragma GCC diagnostic pop
