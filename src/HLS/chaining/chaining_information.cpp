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
 * @file chaining_information.cpp
 * @brief class containing information about chaining
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 *
 */
/// Header include
#include "chaining_information.hpp"

/// behavior include
#include "op_graph.hpp"

/// boost include
#include <boost/pending/disjoint_sets.hpp>

/// HLS includes
#include "hls.hpp"
#include "hls_manager.hpp"

struct ChainingSet
{
   typedef boost::property_map<OpGraph, boost::vertex_index_t>::const_type const_vertex_index_pmap_t;
   typedef boost::iterator_property_map<std::vector<std::size_t>::iterator, boost::identity_property_map, std::vector<std::size_t>::value_type> rank_pmap_type;
   typedef boost::iterator_property_map<std::vector<std::size_t>::iterator, boost::identity_property_map, std::vector<std::size_t>::value_type> pred_pmap_type;

   const_vertex_index_pmap_t cindex_pmap;

   boost::graph_traits<graph>::vertices_size_type n_vert;
   std::vector<std::size_t> rank_map;
   std::vector<std::size_t> pred_map;
   rank_pmap_type rank_pmap;
   pred_pmap_type pred_pmap;
   boost::disjoint_sets<rank_pmap_type, pred_pmap_type> ds;

   explicit ChainingSet(const OpGraphConstRef flow_graph)
       : cindex_pmap(boost::get(boost::vertex_index_t(), *flow_graph)),
         n_vert(boost::num_vertices(*flow_graph)),
         rank_map(2 * n_vert),
         pred_map(2 * n_vert),
         rank_pmap(rank_map.begin()),
         pred_pmap(pred_map.begin()),
         ds(boost::disjoint_sets<rank_pmap_type, pred_pmap_type>(rank_pmap, pred_pmap))
   {
   }

   std::size_t get_index0(vertex v) const
   {
      return cindex_pmap[v] * 2;
   }
   std::size_t get_index1(vertex v) const
   {
      return cindex_pmap[v] * 2 + 1;
   }
};

ChainingInformation::ChainingInformation(const HLS_managerConstRef _HLS_mgr, const unsigned int _function_id) : HLS_mgr(_HLS_mgr), function_id(_function_id)
{
}

size_t ChainingInformation::get_representative_in(vertex op1) const
{
   return chaining_relation->ds.find_set(chaining_relation->get_index0(op1));
}

size_t ChainingInformation::get_representative_out(vertex op1) const
{
   return chaining_relation->ds.find_set(chaining_relation->get_index1(op1));
}

void ChainingInformation::Initialize()
{
   const OpGraphConstRef flow_graph = HLS_mgr.lock()->CGetFunctionBehavior(function_id)->CGetOpGraph(FunctionBehavior::FLSAODG);
   const hlsRef HLS = HLS_mgr.lock()->get_HLS(function_id);

   HLS->chaining_information->chaining_relation = ChainingSetRef(new ChainingSet(flow_graph));
   VertexIterator vi, vi_end;
   for(boost::tie(vi, vi_end) = boost::vertices(*flow_graph); vi != vi_end; ++vi)
   {
      vertex s = *vi;
      HLS->chaining_information->chaining_relation->ds.make_set(chaining_relation->get_index0(s));
      HLS->chaining_information->chaining_relation->ds.make_set(chaining_relation->get_index1(s));
   }
}

bool ChainingInformation::is_chained_vertex(vertex v) const
{
   return is_chained_with.find(v) != is_chained_with.end();
}

bool ChainingInformation::may_be_chained_ops(vertex tgt, vertex src) const
{
   return (chaining_relation->ds.find_set(chaining_relation->get_index0(tgt)) == chaining_relation->ds.find_set(chaining_relation->get_index1(src)) ||
           chaining_relation->ds.find_set(chaining_relation->get_index1(tgt)) == chaining_relation->ds.find_set(chaining_relation->get_index0(src)));
}

void ChainingInformation::add_chained_vertices_in(vertex op1, vertex src)
{
   chaining_relation->ds.union_set(chaining_relation->get_index0(op1), chaining_relation->get_index1(src));
}

void ChainingInformation::add_chained_vertices_out(vertex op1, vertex tgt)
{
   chaining_relation->ds.union_set(chaining_relation->get_index1(op1), chaining_relation->get_index0(tgt));
}
