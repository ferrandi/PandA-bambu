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
 * @file compatibility_based_register.cpp
 * @brief Base class specification for register allocation algorithm based on a compatibility graph
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *
 */
#include "compatibility_based_register.hpp"

#include "hls.hpp"

#include "liveness.hpp"

#include "Parameter.hpp"
#include "dbgPrintHelper.hpp"

#include <boost/lexical_cast.hpp>
#if BOOST_VERSION >= 106400
#include <boost/serialization/array_wrapper.hpp>
#endif
#include <boost/numeric/ublas/matrix.hpp>

/// HLS/binding/storage_value_insertion includes
#include "storage_value_information.hpp"

compatibility_based_register::compatibility_based_register(
    const ParameterConstRef _Param, const HLS_managerRef _HLSMgr, unsigned int _funId,
    const DesignFlowManagerConstRef _design_flow_manager, const HLSFlowStep_Type _hls_flow_step_type,
    const HLSFlowStepSpecializationConstRef _hls_flow_step_specialization)
    : reg_binding_creator(_Param, _HLSMgr, _funId, _design_flow_manager, _hls_flow_step_type,
                          _hls_flow_step_specialization),
      CG(nullptr)
{
}

compatibility_based_register::~compatibility_based_register() = default;

void compatibility_based_register::create_compatibility_graph()
{
   verts.clear();
   THROW_ASSERT(HLS->Rliv, "Liveness analysis not yet computed");
   const auto CG_num_vertices = HLS->storage_value_information->get_number_of_storage_values();
   CG = new compatibility_graph(CG_num_vertices);
   boost::numeric::ublas::matrix<bool> conflict_map(CG_num_vertices, CG_num_vertices);

   boost::numeric::ublas::noalias(conflict_map) =
       boost::numeric::ublas::zero_matrix<bool>(CG_num_vertices, CG_num_vertices);
   for(auto vi = 0U; vi < CG_num_vertices; ++vi)
   {
      verts.push_back(boost::vertex(vi, *CG));
   }

   /// compatibility graph creation
   const auto& support = HLS->Rliv->get_support();
   for(const auto v : support)
   {
      const auto& live = HLS->Rliv->get_live_in(v);
      register_lower_bound = std::max(static_cast<unsigned int>(live.size()), register_lower_bound);
      const auto k_end = live.cend();
      for(auto k = live.cbegin(); k != k_end; ++k)
      {
         auto k_inner = k;
         ++k_inner;
         while(k_inner != k_end)
         {
            const auto tail = HLS->storage_value_information->get_storage_value_index(v, k->first, k->second);
            THROW_ASSERT(tail < CG_num_vertices, "wrong compatibility graph index");
            const auto head =
                HLS->storage_value_information->get_storage_value_index(v, k_inner->first, k_inner->second);
            THROW_ASSERT(head < CG_num_vertices, "wrong compatibility graph index");
            if(tail < head)
            {
               conflict_map(tail, head) = true;
            }
            else if(tail > head)
            {
               conflict_map(head, tail) = true;
            }
            ++k_inner;
         }
      }
   }
   for(auto vj = 1U; vj < CG_num_vertices; ++vj)
   {
      for(auto vi = 0U; vi < vj; ++vi)
      {
         if(!conflict_map(vi, vj) && HLS->storage_value_information->are_storage_value_compatible(vi, vj))
         {
            boost::graph_traits<compatibility_graph>::edge_descriptor e1;
            const auto edge_weight = HLS->storage_value_information->get_compatibility_weight(vi, vj);
            /// we consider only valuable sharing between registers
            if(edge_weight > 1)
            {
               bool in1;
               boost::tie(e1, in1) =
                   boost::add_edge(verts[vi], verts[vj], edge_compatibility_property(edge_weight), *CG);
               THROW_ASSERT(in1, "unable to add edge");
            }
         }
      }
   }
}

bool compatibility_based_register::is_compatible(unsigned int sv1, unsigned int sv2) const
{
   std::pair<boost::graph_traits<compatibility_graph>::edge_descriptor, bool> edge =
       boost::edge(verts[sv1], verts[sv2], *CG);
   return edge.second;
}
