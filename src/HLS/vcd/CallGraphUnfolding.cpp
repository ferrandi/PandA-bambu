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
 *              Copyright (C) 2015-2026 Politecnico di Milano
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
 * @author Pietro Fezzardi <pietrofezzardi@gmail.com>
 */
#include "CallGraphUnfolding.hpp"

#include "Discrepancy.hpp"
#include "Parameter.hpp"
#include "UnfoldedCallGraph.hpp"
#include "call_graph.hpp"
#include "call_graph_manager.hpp"
#include "dbgPrintHelper.hpp"
#include "function_behavior.hpp"
#include "hls_manager.hpp"
#include "op_graph.hpp"
#include "string_manipulation.hpp"

namespace
{
   class CallSitesCollectorVisitor : public boost::default_dfs_visitor
   {
      /// A refcount to the HLSMgr
      const HLS_managerRef HLSMgr;

    public:
      CallSitesCollectorVisitor(const HLS_managerRef& _HLSMgr) : HLSMgr(_HLSMgr)
      {
      }

      void start_vertex(CallGraph::vertex_descriptor, const CallGraph&)
      {
         THROW_ASSERT(HLSMgr->RDiscr, "Discrepancy data structure not initialized");
      }

      void back_edge(const CallGraph::edge_descriptor&, const CallGraph&)
      {
         THROW_ERROR("Recursive functions not supported");
      }

      void examine_edge(const CallGraph::edge_descriptor& e, const CallGraph& g)
      {
         const auto called_id = HLSMgr->CGetCallGraphManager().get_function(g.target(e));
         const auto caller_id = HLSMgr->CGetCallGraphManager().get_function(g.source(e));
         for(const auto callid : g.CGetEdgeInfo(e).direct_call_points)
         {
            HLSMgr->RDiscr->call_sites_info.fu_id_to_call_ids[caller_id].insert(callid);
            THROW_ASSERT(HLSMgr->RDiscr->call_sites_info.call_id_to_called_id[callid].empty() or callid == 0,
                         "direct call " + STR(callid) + " calls more than one function");
            HLSMgr->RDiscr->call_sites_info.call_id_to_called_id[callid].insert(called_id);
         }
         for(const auto callid : g.CGetEdgeInfo(e).indirect_call_points)
         {
            HLSMgr->RDiscr->call_sites_info.fu_id_to_call_ids[caller_id].insert(callid);
            HLSMgr->RDiscr->call_sites_info.call_id_to_called_id[callid].insert(called_id);
            HLSMgr->RDiscr->call_sites_info.indirect_calls.insert(callid);
         }
      }

      void discover_vertex(CallGraph::vertex_descriptor v, const CallGraph&)
      {
         const auto this_fun_id = HLSMgr->CGetCallGraphManager().get_function(v);
         HLSMgr->RDiscr->call_sites_info.fu_id_to_call_ids[this_fun_id];
      }
   };

   void RecursivelyUnfold(UnfoldedCallGraph::vertex_descriptor caller_v, UnfoldedCallGraph& ucg, const CallGraph& CG,
                          const CallSitesInfo& call_sites_info)
   {
      const auto caller_id = ucg.CGetNodeInfo(caller_v).f_id;
      // if this function does not call other functions we're done
      const auto caller = call_sites_info.fu_id_to_call_ids.find(caller_id);
      if(caller == call_sites_info.fu_id_to_call_ids.cend())
      {
         return;
      }

      for(const auto& call_id : caller->second) // loop on the calls performed by function caller_id
      {
         if(call_id == 0)
         { // this should happen only for artificial calls
            continue;
         }
         const auto is_direct =
             !call_sites_info.indirect_calls.count(call_id) && !call_sites_info.taken_addresses.count(call_id);
         for(const auto& called_id :
             call_sites_info.call_id_to_called_id.at(call_id)) // loop on the function called by call_id
         {
            // compute the behavior of the new vertex
            const auto& behaviors = CG.CGetGraphInfo().behaviors;
            const auto b = behaviors.find(called_id);
            // add a new copy of the vertex representing the called function
            const auto called_v = ucg.AddVertex(
                UnfoldedFunctionNodeInfo(called_id, (b != behaviors.end()) ? b->second : FunctionBehaviorConstRef()));
            // add an edge between the caller and the called
            ucg.AddEdge(caller_v, called_v, UnfoldedCallEdgeInfo(call_id, is_direct));
            RecursivelyUnfold(called_v, ucg, CG, call_sites_info);
         }
      }
   }

   void Unfold(const HLS_managerRef& HLSMgr)
   {
      // check that there is only one root function
      const auto& CGM = HLSMgr->CGetCallGraphManager();
      const auto& root_functions = CGM.GetRootFunctions();
      THROW_ASSERT(root_functions.size() == 1, STR(root_functions.size()));
      const auto root_function = *(root_functions.begin());
      const auto& CG = CGM.GetCallGraph();
      {
         /*
          * Use a visitor to analyze the call graph in HLSMgr and to initialize the
          * CallSitesInfo in the Discrepancy data.
          */
         std::vector<boost::default_color_type> csc_color(CG.num_vertices());
         boost::depth_first_visit(CG, CGM.GetVertex(root_function), CallSitesCollectorVisitor(HLSMgr),
                                  boost::make_iterator_property_map(
                                      csc_color.begin(), boost::get(boost::vertex_index_t(), CG), boost::white_color));
      }
      /*
       * After the collection of the data on the call sites we can actually start
       * to unfold the call graph
       */
      const auto FB = HLSMgr->CGetFunctionBehavior(root_function);
      for(const auto fun_id : CGM.GetReachedFunctionsFrom(root_function))
      {
         const auto op_graph = HLSMgr->CGetFunctionBehavior(fun_id)->GetOpGraph(FunctionBehavior::FCFG);
         THROW_ASSERT(op_graph.num_vertices() >= 2,
                      "at least ENTRY and EXIT node must exist for op graph of function " + STR(fun_id));
         HLSMgr->RDiscr->n_total_operations += op_graph.num_vertices() - 2;
      }
      // insert in the unfolded call graph the root function node
      HLSMgr->RDiscr->unfolded_root_v =
          HLSMgr->RDiscr->DiscrepancyCallGraph.AddVertex(UnfoldedFunctionNodeInfo(root_function, FB));
      RecursivelyUnfold(HLSMgr->RDiscr->unfolded_root_v, HLSMgr->RDiscr->DiscrepancyCallGraph, CG,
                        HLSMgr->RDiscr->call_sites_info);
   }
} // namespace

CallGraphUnfolding::CallGraphUnfolding(const ParameterConstRef _Param, const HLS_managerRef _HLSMgr,
                                       const DesignFlowManager& _design_flow_manager)
    : HLS_step(_Param, _HLSMgr, _design_flow_manager, HLSFlowStep_Type::CALL_GRAPH_UNFOLDING)
{
}

HLS_step::HLSRelationships
CallGraphUnfolding::ComputeHLSRelationships(const DesignFlowStep::RelationshipType relationship_type) const
{
   HLSRelationships ret;

   switch(relationship_type)
   {
      case(DEPENDENCE_RELATIONSHIP):
      {
         ret.insert(std::make_tuple(HLSFlowStep_Type::HLS_SYNTHESIS_FLOW, HLSFlowStepSpecializationConstRef(),
                                    HLSFlowStep_Relationship::TOP_FUNCTION));
         break;
      }
      case(INVALIDATION_RELATIONSHIP):
      case(PRECEDENCE_RELATIONSHIP):
      {
         break;
      }
      default:
      {
         THROW_UNREACHABLE("");
      }
   }
   return ret;
}

bool CallGraphUnfolding::HasToBeExecuted() const
{
   return true;
}

DesignFlowStep_Status CallGraphUnfolding::Exec()
{
   // cleanup data structure if this is not the first execution
   HLSMgr->RDiscr->clear();
   /* unfold the call graph and compute data structures used for discrepancy analysis*/
   INDENT_OUT_MEX(OUTPUT_LEVEL_VERBOSE, output_level, "-->Unfolding call graph");
   Unfold(HLSMgr);
   INDENT_OUT_MEX(OUTPUT_LEVEL_VERBOSE, output_level, "<--Unfolded call graph");
   return DesignFlowStep_Status::SUCCESS;
}
