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
 *              Copyright (c) 2015-2018 Politecnico di Milano
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
 * @author Pietro Fezzardi <pietrofezzardi@gmail.com>
 */

#include "CallGraphUnfolder.hpp"

// includes from behavior/
#include "call_graph.hpp"
#include "call_graph_manager.hpp"
#include "op_graph.hpp"

// include from HLS/
#include "hls_manager.hpp"

// includes from HLS/vcd/
#include "CallGraphUnfolder.hpp"
#include "CallSitesCollectorVisitor.hpp"
#include "Discrepancy.hpp"
#include "HWCallPathCalculator.hpp"
#include "UnfoldedCallGraph.hpp"
#include "UnfoldedCallInfo.hpp"
#include "UnfoldedFunctionInfo.hpp"
#include "string_manipulation.hpp" // for STR

static void RecursivelyUnfold(const UnfoldedVertexDescriptor caller_v, UnfoldedCallGraph& ucg, const CallGraphConstRef& cg, std::unordered_map<unsigned int, std::unordered_set<unsigned int>>& caller_to_call_id,
                              std::unordered_map<unsigned int, std::unordered_set<unsigned int>>& call_to_called_id, std::unordered_set<unsigned int>& indirect_calls)
{
   const unsigned int caller_id = get_node_info<UnfoldedFunctionInfo>(caller_v, ucg)->f_id;
   // if this function does not call other functions we're done
   auto caller = caller_to_call_id.find(caller_id);
   if(caller == caller_to_call_id.cend())
      return;

   for(const unsigned int call_id : caller->second) // loop on the calls performed by function caller_id
   {
      if(call_id == 0) // this should happen only for artificial calls
         continue;
      bool is_direct = indirect_calls.find(call_id) == indirect_calls.end();
      for(auto called_id : call_to_called_id.at(call_id)) // loop on the function called by call_id
      {
         // compute the behavior of the new vertex
         const std::map<unsigned int, FunctionBehaviorRef>& behaviors = cg->CGetCallGraphInfo()->behaviors;
         const auto b = behaviors.find(called_id);
         // add a new copy of the vertex representing the called function
         UnfoldedVertexDescriptor called_v = ucg.AddVertex(NodeInfoRef(new UnfoldedFunctionInfo(called_id, (b != behaviors.end()) ? b->second : FunctionBehaviorConstRef())));
         // add an edge between the caller and the called
         ucg.AddEdge(caller_v, called_v, EdgeInfoRef(new UnfoldedCallInfo(call_id, is_direct)));
         RecursivelyUnfold(called_v, ucg, cg, caller_to_call_id, call_to_called_id, indirect_calls);
      }
   }
}

void CallGraphUnfolder::Unfold(const HLS_managerRef& HLSMgr, const ParameterConstRef& params, std::unordered_map<unsigned int, std::unordered_set<unsigned int>>& caller_to_call_id,
                               std::unordered_map<unsigned int, std::unordered_set<unsigned int>>& call_to_called_id)
{
   // check that there is only one root function
   const CallGraphManagerConstRef cgman = HLSMgr->CGetCallGraphManager();
   THROW_ASSERT(cgman->GetRootFunctions().size() == 1, STR(cgman->GetRootFunctions().size()));
   const CallGraphConstRef cg = cgman->CGetCallGraph();
   // initialize the maps with data from the call graph, using a visitor to collect data
   std::unordered_set<unsigned int> indirect_calls;
   CallSitesCollectorVisitor csc_vis(cgman, caller_to_call_id, call_to_called_id, indirect_calls);
   std::vector<boost::default_color_type> csc_color(boost::num_vertices(*cg));
   const auto root_functions = cgman->GetRootFunctions();
   THROW_ASSERT(root_functions.size() == 1, STR(root_functions.size()));
   const auto root_function = *(root_functions.begin());
   boost::depth_first_visit(*cg, cgman->GetVertex(root_function), csc_vis, boost::make_iterator_property_map(csc_color.begin(), boost::get(boost::vertex_index_t(), *cg), boost::white_color));
   // insert in the unfolded call graph the root function node
   const unsigned int root_fun_id = *(cgman->GetRootFunctions().begin());
   const auto b = cg->CGetCallGraphInfo()->behaviors.find(root_fun_id);
   THROW_ASSERT(b != cg->CGetCallGraphInfo()->behaviors.end(), "no behavior for root function " + STR(root_fun_id));
   HLSMgr->RDiscr->unfolded_root_v = HLSMgr->RDiscr->DiscrepancyCallGraph.AddVertex(NodeInfoRef(new UnfoldedFunctionInfo(root_fun_id, b->second)));
   RecursivelyUnfold(HLSMgr->RDiscr->unfolded_root_v, HLSMgr->RDiscr->DiscrepancyCallGraph, cg, caller_to_call_id, call_to_called_id, indirect_calls);
   /* fill the map used by the HWCallPathCalculator */
   std::map<unsigned int, vertex> call_id_to_OpVertex;
   std::set<unsigned int> reached_body_fun_ids = cgman->GetReachedBodyFunctions();
   VertexIterator cg_vi, cg_vi_end;
   boost::tie(cg_vi, cg_vi_end) = boost::vertices(*cg);
   HLSMgr->RDiscr->n_total_operations = 0;
   for(unsigned int fun_id : reached_body_fun_ids)
   {
      const FunctionBehaviorConstRef function_behavior = HLSMgr->CGetFunctionBehavior(fun_id);
      const OpGraphConstRef op_graph = function_behavior->CGetOpGraph(FunctionBehavior::FCFG);
      THROW_ASSERT(boost::num_vertices(*op_graph) >= 2, "at least ENTRY and EXIT vertices must exist");
      HLSMgr->RDiscr->n_total_operations += boost::num_vertices(*op_graph) - 2;
      VertexIterator vi, vi_end;
      boost::tie(vi, vi_end) = boost::vertices(*op_graph);
      for(; vi != vi_end; vi++)
         call_id_to_OpVertex[op_graph->CGetOpNodeInfo(*vi)->GetNodeId()] = *vi;
   }
   // Calculate the HW paths and store them in Discrepancy
   HWCallPathCalculator sig_sel_v(HLSMgr, params, call_id_to_OpVertex);
   std::vector<boost::default_color_type> sig_sel_color(boost::num_vertices(HLSMgr->RDiscr->DiscrepancyCallGraph), boost::white_color);
   boost::depth_first_visit(HLSMgr->RDiscr->DiscrepancyCallGraph, HLSMgr->RDiscr->unfolded_root_v, sig_sel_v,
                            boost::make_iterator_property_map(sig_sel_color.begin(), boost::get(boost::vertex_index_t(), HLSMgr->RDiscr->DiscrepancyCallGraph), boost::white_color));
}
