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
 * @file port_swapping.hpp
 * @brief Implementation of the port swapping algorithm described in the following paper:
 *   Hao Cong, Song Chen and T. Yoshimura, "Port assignment for interconnect reduction in high-level synthesis," Proceedings of Technical Program of 2012 VLSI Design, Automation and Test, Hsinchu, 2012, pp. 1-4.
 *
 * @author Alessandro Comodi <alessandro.comodi@mail.polimi.it>
 * @author Davide Conficconi <davide.conficconi@mail.polimi.it>
 */

#ifndef PORT_SWAPPING_HPP
#define PORT_SWAPPING_HPP

/// superclass include
#include "hls_function_step.hpp"
#include <boost/graph/adjacency_list.hpp>

/**
 * Class managing a partial module binding based on simple conditions
 */
class port_swapping : public HLSFunctionStep
{
 private:
   typedef boost::adjacency_list<boost::vecS, boost::vecS, boost::undirectedS, boost::no_property, boost::property<boost::edge_color_t, boost::default_color_type>> PSGraph;
   typedef boost::graph_traits<PSGraph>::edge_descriptor PSEdge;
   typedef boost::graph_traits<PSGraph>::vertex_descriptor PSVertex;
   typedef std::pair<unsigned int, unsigned int> PSE;
   typedef struct
   {
      PSVertex v;
      int belongs;
      int level;
   } PSVSet;
   typedef struct
   {
      std::vector<PSVSet> vset;
      std::vector<PSE> spt_vector_edges;
      std::vector<PSE> co_tree_vector_edges;
      std::vector<PSE> odd_co_tree_vector_edges;
      long unsigned int cardinality;
   } PSMultiStart;

 protected:
   /**
    * Compute the relationship of this step
    * @param relationship_type is the type of relationship to be considered
    * @return the steps in relationship with this
    */
   const CustomUnorderedSet<std::tuple<HLSFlowStep_Type, HLSFlowStepSpecializationConstRef, HLSFlowStep_Relationship>> ComputeHLSRelationships(const DesignFlowStep::RelationshipType relationship_type) const override;

 public:
   /**
    * Constructor
    * @param design_flow_manager is the design flow manager
    */
   port_swapping(const ParameterConstRef Param, const HLS_managerRef HLSMgr, unsigned int funId, const DesignFlowManagerConstRef design_flow_manager);

   /**
    * Destructor.
    */
   ~port_swapping() override;

   /**
    * This function calculates the levels of all te vertices starting from root
    * @param spt_edges edges of the spanning tree
    * @param root starting vertex
    * @param num_vertices_g number of vertices in the graph
    * @param vset structure that contains the vertices and the relative level and the belongings
    */
   void vertex_levels(std::vector<PSE>& spt_edges, PSVertex root, size_t num_vertices_g, std::vector<PSVSet>& vset);

   /**
    * This function calculates the distances between two vertices in a spanning tree
    * @param spt_edges edges of the spanning tree
    * @param root starting vertex
    * @param num_vertices_g number of vertices in the graph
    * @param vset structure that contains the vertices and the relative level and the belongings
    */
   int vertex_distance(std::vector<PSE>& spt_edges, PSVertex root, PSVertex dest, std::vector<PSVSet>& vset);

   /**
    * This function finds the maximum degree (out_edges) of a vertex
    * @param dSet structure with all the vertices and relative degrees
    * @return Vertex which has the max degree
    */
   PSVertex find_max_degree(CustomOrderedSet<std::pair<PSVertex, unsigned int>>& dSet);

   /**
    * This function updates the degree of all the vertices
    * @param g2 graph on which the degree of the vertices must be calculated
    * @param dSet set to be updated with all the vertices and relative degree
    */
   void update_degree(PSGraph g2, CustomOrderedSet<std::pair<PSVertex, unsigned int>>& dSet);

   /**
    * This function selects the right vertex from the co_tree_edges vector
    * @param vertex
    * @param edges vector of edges from which is taken the right one and from that the right vertex
    * @return chosen vertex
    */
   PSVertex get_co_tree_vertex(PSVertex vertex, std::vector<PSE>& edges);

   /**
    * This function computes the port_swapping
    * @param g graph on which the algorithm is computed
    * @param vector_sets vector that will contain all the runs of the algorithm. It will contain the best solution
    * @param num_vertices_g number of vertices in the graph
    * @param root vertex from which the spanning tree is calculated
    */
   void port_swapping_algorithm(PSGraph g, std::vector<PSMultiStart>& vector_sets, size_t num_vertices_g, PSVertex root);

   /**
    * This function is the wrapper that executes port_swapping_algorithms and extracts the best solution
    * @param g starting graph
    * @return vector of vertices and relative results. Each vertex will contain the belonging set (A, B, AB)
    */
   std::vector<std::pair<PSVertex, unsigned int>> p_swap(PSGraph g);

   /**
    * This function checks if an operation is commutative or not
    * @param operation name
    * @return true if it is a commutative binary operation, false otherwise
    */
   bool is_commutative_op(const std::string& operation);

   /**
    * This function returns the belonging set of a given vertex
    * @param operand which represents a vertex in the graph
    * @param results vector containing all the vertices and relative belonging sets
    * @return belonging set of the operand
    */
   unsigned int get_results(PSVertex operand, std::vector<std::pair<PSVertex, unsigned int>> results);

   /**
    * Execute the step
    * @return the exit status of this step
    */
   DesignFlowStep_Status InternalExec() override;
};
#endif
