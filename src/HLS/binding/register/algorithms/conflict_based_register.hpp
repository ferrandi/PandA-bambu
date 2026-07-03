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
 *              Copyright (C) 2004-2026 Politecnico di Milano
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 *
 *   This file is part of the PandA framework.
 *
 *   Licensed under the Apache License, Version 2.0, with BAMBU exceptions (the "License");
 *   you may not use this file except in compliance with the License.
 *   You may obtain a copy of the License at
 *
 *       http://www.apache.org/licenses/LICENSE-2.0
 *
 *   Unless required by applicable law or agreed to in writing, software
 *   distributed under the License is distributed on an "AS IS" BASIS,
 *   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *   See the License for the specific language governing permissions and
 *   limitations under the License.
 *
 */
/**
 * @file conflict_based_register.hpp
 * @brief Base class specification for register allocation algorithm based on a conflict graph
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 *
 */
#ifndef CONFLICT_BASED_REGISTER_HPP
#define CONFLICT_BASED_REGISTER_HPP

#include "reg_binding_creator.hpp"

#include <boost/graph/adjacency_list.hpp>

class conflict_based_register : public reg_binding_creator
{
 protected:
   using conflict_graph = boost::adjacency_list<boost::vecS, boost::vecS, boost::undirectedS, unsigned>;
   using cg_vertex_descriptor = boost::graph_traits<conflict_graph>::vertex_descriptor;
   using cg_vertices_size_type = boost::graph_traits<conflict_graph>::vertices_size_type;
   using cg_vertex_index_map = boost::property_map<conflict_graph, boost::vertex_index_t>::const_type;

   boost::iterator_property_map<cg_vertices_size_type*, cg_vertex_index_map, cg_vertices_size_type,
                                cg_vertices_size_type&>
       color;
   std::vector<cg_vertices_size_type> color_vec;

 public:
   /**
    * Constructor of the class.
    * @param Param is the set of input parameters
    * @param HLSMgr is the HLS manager
    * @param funId is the identifier of the function being processed
    * @param design_flow_manager is the design flow manager
    * @param hls_flow_step_type is the register binding algorithm
    */
   conflict_based_register(const ParameterConstRef Param, const HLS_managerRef HLSMgr, unsigned int funId,
                           const DesignFlowManager& design_flow_manager, const HLSFlowStep_Type hls_flow_step_type);

   void create_conflict_graph(conflict_graph& cg);
};

#endif
