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
 * @file chaining_information.cpp
 * @brief class containing information about chaining
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 *
 */
#include "chaining_information.hpp"

#include "function_behavior.hpp"
#include "hls.hpp"
#include "hls_manager.hpp"

#include <boost/pending/disjoint_sets.hpp>

struct ChainingSet
{
   using const_vertex_index_pmap_t = boost::property_map<OpGraph::Base, boost::vertex_index_t>::const_type;
   using rank_pmap_type = boost::iterator_property_map<std::vector<std::size_t>::iterator, boost::identity_property_map,
                                                       std::vector<std::size_t>::value_type>;
   using pred_pmap_type = boost::iterator_property_map<std::vector<std::size_t>::iterator, boost::identity_property_map,
                                                       std::vector<std::size_t>::value_type>;

   const_vertex_index_pmap_t cindex_pmap;

   OpGraph::vertices_size_type n_vert;
   std::vector<std::size_t> rank_map;
   std::vector<std::size_t> pred_map;
   rank_pmap_type rank_pmap;
   pred_pmap_type pred_pmap;
   boost::disjoint_sets<rank_pmap_type, pred_pmap_type> ds;

   ChainingSet(const OpGraph& flow_graph)
       : cindex_pmap(boost::get(boost::vertex_index_t(), flow_graph)),
         n_vert(flow_graph.num_vertices()),
         rank_map(2 * n_vert),
         pred_map(2 * n_vert),
         rank_pmap(rank_map.begin()),
         pred_pmap(pred_map.begin()),
         ds(boost::disjoint_sets<rank_pmap_type, pred_pmap_type>(rank_pmap, pred_pmap))
   {
   }

   std::size_t get_index0(OpGraph::vertex_descriptor v) const
   {
      return cindex_pmap[v] * 2;
   }
   std::size_t get_index1(OpGraph::vertex_descriptor v) const
   {
      return cindex_pmap[v] * 2 + 1;
   }
};

ChainingInformation::ChainingInformation(const HLS_managerConstRef _HLS_mgr, const unsigned int _function_id)
    : HLS_mgr(_HLS_mgr), function_id(_function_id)
{
}

size_t ChainingInformation::get_representative_in(OpGraph::vertex_descriptor op1) const
{
   return chaining_relation->ds.find_set(chaining_relation->get_index0(op1));
}

size_t ChainingInformation::get_representative_out(OpGraph::vertex_descriptor op1) const
{
   return chaining_relation->ds.find_set(chaining_relation->get_index1(op1));
}

void ChainingInformation::Initialize()
{
   const auto flow_graph = HLS_mgr.lock()->CGetFunctionBehavior(function_id)->GetOpGraph(FunctionBehavior::FLSAODG);
   const auto HLS = HLS_mgr.lock()->get_HLS(function_id);

   HLS->chaining_information->chaining_relation = ChainingSetRef(new ChainingSet(flow_graph));
   for(const auto v : flow_graph.vertices())
   {
      HLS->chaining_information->chaining_relation->ds.make_set(chaining_relation->get_index0(v));
      HLS->chaining_information->chaining_relation->ds.make_set(chaining_relation->get_index1(v));
   }
}

bool ChainingInformation::is_chained_vertex(OpGraph::vertex_descriptor v) const
{
   return is_chained_with.find(v) != is_chained_with.end();
}

bool ChainingInformation::may_be_chained_ops(OpGraph::vertex_descriptor tgt, OpGraph::vertex_descriptor src) const
{
   return (chaining_relation->ds.find_set(chaining_relation->get_index0(tgt)) ==
               chaining_relation->ds.find_set(chaining_relation->get_index1(src)) ||
           chaining_relation->ds.find_set(chaining_relation->get_index1(tgt)) ==
               chaining_relation->ds.find_set(chaining_relation->get_index0(src)));
}

void ChainingInformation::add_chained_vertices_in(OpGraph::vertex_descriptor op1, OpGraph::vertex_descriptor src)
{
   chaining_relation->ds.union_set(chaining_relation->get_index0(op1), chaining_relation->get_index1(src));
}

void ChainingInformation::add_chained_vertices_out(OpGraph::vertex_descriptor op1, OpGraph::vertex_descriptor tgt)
{
   chaining_relation->ds.union_set(chaining_relation->get_index1(op1), chaining_relation->get_index0(tgt));
}
