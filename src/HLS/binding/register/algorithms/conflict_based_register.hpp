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
 * @file conflict_based_register.hpp
 * @brief Base class specification for register allocation algorithm based on a conflict graph
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *
 */
#ifndef CONFLICT_BASED_REGISTER_HPP
#define CONFLICT_BASED_REGISTER_HPP

#include "reg_binding_creator.hpp"

#include <boost/graph/adjacency_list.hpp>

class conflict_based_register : public reg_binding_creator
{
 protected:
   typedef boost::adjacency_list<boost::vecS, boost::vecS, boost::undirectedS> conflict_graph;
   typedef boost::graph_traits<conflict_graph>::vertex_descriptor cg_vertex_descriptor;
   typedef boost::graph_traits<conflict_graph>::vertices_size_type cg_vertices_size_type;
   typedef boost::property_map<conflict_graph, boost::vertex_index_t>::const_type cg_vertex_index_map;

   /// conflict graph
   conflict_graph cg;

   boost::iterator_property_map<cg_vertices_size_type*, cg_vertex_index_map, cg_vertices_size_type, cg_vertices_size_type&> color;

 private:
   std::vector<cg_vertices_size_type> color_vec;

 public:
   /**
    * Constructor of the class.
    * @param design_flow_manager is the design flow manager
    * @param hls_flow_step_type is the register binding algorithm
    */
   conflict_based_register(const ParameterConstRef Param, const HLS_managerRef HLSMgr, unsigned int funId, const DesignFlowManagerConstRef design_flow_manager, const HLSFlowStep_Type hls_flow_step_type);

   /**
    * Destructor of the class.
    */
   ~conflict_based_register() override;

   /**
    * Create the conflict graph
    */
   void create_conflict_graph();
};

#endif
