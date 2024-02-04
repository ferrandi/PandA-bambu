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
 * @file compatibility_based_register.hpp
 * @brief Base class specification for register allocation algorithm based on a compatibility graph
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *
 */
#ifndef COMPATIBILITY_BASED_REGISTER_HPP
#define COMPATIBILITY_BASED_REGISTER_HPP

#include "reg_binding_creator.hpp"

#include <boost/graph/adjacency_matrix.hpp>
#include <vector>

class compatibility_based_register : public reg_binding_creator
{
 protected:
   /// edge property
   struct edge_compatibility_property
   {
      /// edge weight
      int weight;

      edge_compatibility_property() : weight(0)
      {
      }
      /**
       * Constructor with selector
       * @param _weight is the weight to be associated with the edge
       */
      explicit edge_compatibility_property(int _weight) : weight(_weight)
      {
      }
   };

   using compatibility_graph =
       boost::adjacency_matrix<boost::undirectedS, boost::no_property, edge_compatibility_property>;
   using CG_vertex_descriptor = boost::graph_traits<compatibility_graph>::vertex_descriptor;
   using CG_vertices_size_type = boost::graph_traits<compatibility_graph>::vertices_size_type;

   /// compatibility graph
   compatibility_graph* CG;

   /// ordered vector containing the vertices of the compatibility graph
   std::vector<CG_vertex_descriptor> verts;

 public:
   /**
    * Constructor
    * @param design_flow_manager is the design flow manager
    */
   compatibility_based_register(
       const ParameterConstRef Param, const HLS_managerRef HLSMgr, unsigned int funId,
       const DesignFlowManagerConstRef design_flow_manager, const HLSFlowStep_Type hls_flow_step_type,
       const HLSFlowStepSpecializationConstRef hls_flow_step_specialization = HLSFlowStepSpecializationConstRef());

   ~compatibility_based_register() override;

   /**
    * Creates the compatibility graph
    */
   void create_compatibility_graph();

   /**
    * Checks if two storage values are compatible
    */
   bool is_compatible(unsigned int sv1, unsigned int sv2) const;
};
/// refcount definition of the class
using compatibility_based_registerRef = refcount<compatibility_based_register>;

#endif
