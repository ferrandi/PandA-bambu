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
 * @file conflict_based_register.cpp
 * @brief Base class specification for register allocation algorithm based on a conflict graph
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 *
 */
#include "conflict_based_register.hpp"

#include "Parameter.hpp"
#include "dbgPrintHelper.hpp"
#include "hls.hpp"
#include "ir_helper.hpp"
#include "liveVariables.hpp"
#include "op_graph.hpp"
#include "storage_value_information.hpp"

#include <vector>

conflict_based_register::conflict_based_register(const ParameterConstRef _Param, const HLS_managerRef _HLSMgr,
                                                 unsigned int _funId, const DesignFlowManager& _design_flow_manager,
                                                 const HLSFlowStep_Type _hls_flow_step_type)
    : reg_binding_creator(_Param, _HLSMgr, _funId, _design_flow_manager, _hls_flow_step_type)
{
}

void conflict_based_register::create_conflict_graph(conflict_graph& cg)
{
   THROW_ASSERT(HLS->Rliv, "Liveness analysis not yet computed");

   const auto cg_num_vertices = HLS->storage_value_information->get_number_of_storage_values();
   color_vec.resize(cg_num_vertices);
   color =
       boost::iterator_property_map<cg_vertices_size_type*, cg_vertex_index_map, cg_vertices_size_type,
                                    cg_vertices_size_type&>(&color_vec.front(), boost::get(boost::vertex_index, cg));
   HLS->storage_value_information->Initialize();
   /// conflict graph creation
   const auto states = HLS->fsm_info->vertices();
   for(const auto v : states)
   {
      const auto& live = HLS->Rliv->getLiveInFsmVariables(v);
      register_lower_bound = std::max(static_cast<unsigned int>(live.size()), register_lower_bound);
      const auto k_end = live.end();
      for(auto k = live.begin(); k != k_end; ++k)
      {
         auto k_inner = k;
         ++k_inner;
         while(k_inner != k_end)
         {
            const auto tail = HLS->storage_value_information->get_storage_value_index(v, k->first, k->second);
            THROW_ASSERT(tail < cg_num_vertices, "wrong conflict graph index");
            const auto head =
                HLS->storage_value_information->get_storage_value_index(v, k_inner->first, k_inner->second);
            THROW_ASSERT(head < cg_num_vertices, "wrong conflict graph index");
            boost::add_edge(boost::vertex(tail, cg), boost::vertex(head, cg), cg);
            ++k_inner;
         }
      }
   }
   /// variables of different size are in conflict
   for(unsigned int vj = 1; vj < cg_num_vertices; ++vj)
   {
      for(unsigned int vi = 0; vi < vj; ++vi)
      {
         if(!HLS->storage_value_information->are_storage_value_compatible(vi, vj))
         {
            boost::add_edge(boost::vertex(vi, cg), boost::vertex(vj, cg), cg);
         }
      }
   }
}
