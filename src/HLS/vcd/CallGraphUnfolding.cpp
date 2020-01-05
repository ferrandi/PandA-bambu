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
 *              Copyright (C) 2015-2020 Politecnico di Milano
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

#include "CallGraphUnfolding.hpp"

// include from ./
#include "Parameter.hpp"

// includes from behavior/
#include "call_graph.hpp"
#include "call_graph_manager.hpp"
#include "op_graph.hpp"

// include from HLS/
#include "hls_manager.hpp"

// includes from HLS/vcd/
#include "CallSitesCollectorVisitor.hpp"
#include "Discrepancy.hpp"
#include "UnfoldedCallGraph.hpp"
#include "UnfoldedCallInfo.hpp"
#include "UnfoldedFunctionInfo.hpp"
#include "dbgPrintHelper.hpp"
#include "string_manipulation.hpp" // for STR

static void RecursivelyUnfold(const UnfoldedVertexDescriptor caller_v, UnfoldedCallGraph& ucg, const CallGraphConstRef& cg, const CallSitesInfoRef& call_sites_info)
{
   const unsigned int caller_id = get_node_info<UnfoldedFunctionInfo>(caller_v, ucg)->f_id;
   // if this function does not call other functions we're done
   const auto caller = call_sites_info->fu_id_to_call_ids.find(caller_id);
   if(caller == call_sites_info->fu_id_to_call_ids.cend())
      return;

   for(const unsigned int call_id : caller->second) // loop on the calls performed by function caller_id
   {
      if(call_id == 0) // this should happen only for artificial calls
         continue;
      bool is_direct = call_sites_info->indirect_calls.find(call_id) == call_sites_info->indirect_calls.end() and call_sites_info->taken_addresses.find(call_id) == call_sites_info->taken_addresses.end();
      for(auto called_id : call_sites_info->call_id_to_called_id.at(call_id)) // loop on the function called by call_id
      {
         // compute the behavior of the new vertex
         const std::map<unsigned int, FunctionBehaviorRef>& behaviors = cg->CGetCallGraphInfo()->behaviors;
         const auto b = behaviors.find(called_id);
         // add a new copy of the vertex representing the called function
         UnfoldedVertexDescriptor called_v = ucg.AddVertex(NodeInfoRef(new UnfoldedFunctionInfo(called_id, (b != behaviors.end()) ? b->second : FunctionBehaviorConstRef())));
         // add an edge between the caller and the called
         ucg.AddEdge(caller_v, called_v, EdgeInfoRef(new UnfoldedCallInfo(call_id, is_direct)));
         RecursivelyUnfold(called_v, ucg, cg, call_sites_info);
      }
   }
}

static void Unfold(const HLS_managerRef& HLSMgr)
{
   // check that there is only one root function
   const CallGraphManagerConstRef cgman = HLSMgr->CGetCallGraphManager();
   const auto root_functions = cgman->GetRootFunctions();
   THROW_ASSERT(root_functions.size() == 1, STR(root_functions.size()));
   const CallGraphConstRef cg = cgman->CGetCallGraph();
   {
      /*
       * Use a visitor to analyze the call graph in HLSMgr and to initialize the
       * CallSitesInfo in the Discrepancy data.
       */
      std::vector<boost::default_color_type> csc_color(boost::num_vertices(*cg));
      const auto root_function = *(root_functions.begin());
      boost::depth_first_visit(*cg, cgman->GetVertex(root_function), CallSitesCollectorVisitor(HLSMgr), boost::make_iterator_property_map(csc_color.begin(), boost::get(boost::vertex_index_t(), *cg), boost::white_color));
   }
   /*
    * After the collection of the data on the call sites we can actually start
    * to unfold the call graph
    */
   // get the id of the root function
   const unsigned int root_fun_id = *(root_functions.begin());
   const auto b = cg->CGetCallGraphInfo()->behaviors.find(root_fun_id);
   THROW_ASSERT(b != cg->CGetCallGraphInfo()->behaviors.end(), "no behavior for root function " + STR(root_fun_id));
   for(const auto fun_id : cgman->GetReachedBodyFunctionsFrom(root_fun_id))
   {
      const OpGraphConstRef op_graph = HLSMgr->CGetFunctionBehavior(fun_id)->CGetOpGraph(FunctionBehavior::FCFG);
      THROW_ASSERT(boost::num_vertices(*op_graph) >= 2, "at least ENTRY and EXIT node must exist for op graph of function " + STR(fun_id));
      HLSMgr->RDiscr->n_total_operations += boost::num_vertices(*op_graph) - 2;
   }
   // insert in the unfolded call graph the root function node
   HLSMgr->RDiscr->unfolded_root_v = HLSMgr->RDiscr->DiscrepancyCallGraph.AddVertex(NodeInfoRef(new UnfoldedFunctionInfo(root_fun_id, b->second)));
   RecursivelyUnfold(HLSMgr->RDiscr->unfolded_root_v, HLSMgr->RDiscr->DiscrepancyCallGraph, cg, HLSMgr->RDiscr->call_sites_info);
}

const CustomUnorderedSet<std::tuple<HLSFlowStep_Type, HLSFlowStepSpecializationConstRef, HLSFlowStep_Relationship>> CallGraphUnfolding::ComputeHLSRelationships(const DesignFlowStep::RelationshipType relationship_type) const
{
   CustomUnorderedSet<std::tuple<HLSFlowStep_Type, HLSFlowStepSpecializationConstRef, HLSFlowStep_Relationship>> ret;

   switch(relationship_type)
   {
      case DEPENDENCE_RELATIONSHIP:
      {
         ret.insert(std::make_tuple(HLSFlowStep_Type::HLS_SYNTHESIS_FLOW, HLSFlowStepSpecializationConstRef(), HLSFlowStep_Relationship::TOP_FUNCTION));
         break;
      }
      case INVALIDATION_RELATIONSHIP:
      case PRECEDENCE_RELATIONSHIP:
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

CallGraphUnfolding::CallGraphUnfolding(const ParameterConstRef _Param, const HLS_managerRef _HLSMgr, const DesignFlowManagerConstRef _design_flow_manager) : HLS_step(_Param, _HLSMgr, _design_flow_manager, HLSFlowStep_Type::CALL_GRAPH_UNFOLDING)
{
}

CallGraphUnfolding::~CallGraphUnfolding()
{
}

bool CallGraphUnfolding::HasToBeExecuted() const
{
   return true;
}
