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
 * @file conflict_based_register.cpp
 * @brief Base class specification for register allocation algorithm based on a conflict graph
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *
 */
#include "conflict_based_register.hpp"

#include "hls.hpp"

#include "liveness.hpp"
#include "op_graph.hpp"
#include "tree_helper.hpp"

#include "Parameter.hpp"
#include "dbgPrintHelper.hpp"

#include <boost/lexical_cast.hpp>
#include <vector>

/// HLS/binding/storage_value_insertion includes
#include "storage_value_information.hpp"

conflict_based_register::conflict_based_register(const ParameterConstRef _Param, const HLS_managerRef _HLSMgr, unsigned int _funId, const DesignFlowManagerConstRef _design_flow_manager, const HLSFlowStep_Type _hls_flow_step_type)
    : reg_binding_creator(_Param, _HLSMgr, _funId, _design_flow_manager, _hls_flow_step_type)
{
}

conflict_based_register::~conflict_based_register() = default;

void conflict_based_register::create_conflict_graph()
{
   THROW_ASSERT(HLS->Rliv, "Liveness analysis not yet computed");

   unsigned int cg_num_vertices = HLS->storage_value_information->get_number_of_storage_values();
   for(unsigned int vi = 0; vi < cg_num_vertices; ++vi)
      boost::add_vertex(cg);
   color_vec.resize(cg_num_vertices);
   color = boost::iterator_property_map<cg_vertices_size_type*, cg_vertex_index_map, cg_vertices_size_type, cg_vertices_size_type&>(&color_vec.front(), boost::get(boost::vertex_index, cg));
   /// conflict graph creation
   const std::list<vertex>& support = HLS->Rliv->get_support();

   const std::list<vertex>::const_iterator vEnd = support.end();
   for(auto vIt = support.begin(); vIt != vEnd; ++vIt)
   {
      const CustomOrderedSet<unsigned int>& live = HLS->Rliv->get_live_in(*vIt);
      register_lower_bound = std::max(static_cast<unsigned int>(live.size()), register_lower_bound);
      const CustomOrderedSet<unsigned int>::const_iterator k_end = live.end();
      for(auto k = live.begin(); k != k_end; ++k)
      {
         auto k_inner = k;
         ++k_inner;
         while(k_inner != k_end)
         {
            boost::graph_traits<conflict_graph>::edge_descriptor e1;
            bool in1;
            unsigned int tail = HLS->storage_value_information->get_storage_value_index(*vIt, *k);
            THROW_ASSERT(tail < cg_num_vertices, "wrong conflict graph index");
            unsigned int head = HLS->storage_value_information->get_storage_value_index(*vIt, *k_inner);
            THROW_ASSERT(head < cg_num_vertices, "wrong conflict graph index");
            tie(e1, in1) = boost::add_edge(boost::vertex(tail, cg), boost::vertex(head, cg), cg);
            THROW_ASSERT(in1, "unable to add edge");
            ++k_inner;
         }
      }
   }
   /// variables of different size are in conflict
   for(unsigned int vj = 1; vj < cg_num_vertices; ++vj)
      for(unsigned int vi = 0; vi < vj; ++vi)
      {
         if(!HLS->storage_value_information->are_value_bitsize_compatible(vi, vj))
         {
            boost::graph_traits<conflict_graph>::edge_descriptor e1;
            bool in1;
            tie(e1, in1) = boost::add_edge(boost::vertex(vi, cg), boost::vertex(vj, cg), cg);
            THROW_ASSERT(in1, "unable to add edge");
         }
      }
}
