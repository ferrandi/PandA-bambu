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
 * @author Pietro Fezzardi <pietrofezzardi@gmail.com>
 */
#include "HWPathComputation.hpp"

#include "Discrepancy.hpp"
#include "HDL_manager.hpp"
#include "UnfoldedCallGraph.hpp"
#include "behavioral_helper.hpp"
#include "call_graph_manager.hpp"
#include "dbgPrintHelper.hpp"
#include "fu_binding.hpp"
#include "function_behavior.hpp"
#include "functions.hpp"
#include "hls.hpp"
#include "hls_manager.hpp"
#include "hls_step.hpp"
#include "op_graph.hpp"
#include "structural_objects.hpp"
#include "top_entity.hpp"
#include "utility.hpp"

#include <utility>

class HWCallPathCalculator : public boost::default_dfs_visitor
{
 protected:
   /// a refcount to the HLS_manager
   const HLS_managerRef HLSMgr;

   /// a stack of scopes used during the traversal of the UnfoldedCallGraph
   std::stack<std::string> scope;

   /// The key is the name of a shared function, the mapped value is the HW
   //  scope of that shared function
   std::map<std::string, std::string> shared_fun_scope;

   /// The scope of the top function. It depends on different parameters and
   //  it is computed by start_vertex
   std::string top_fun_scope;

 public:
   HWCallPathCalculator(const HLS_managerRef _HLSMgr);

   void start_vertex(UnfoldedCallGraph::vertex_descriptor v, const UnfoldedCallGraph& ufcg);
   void discover_vertex(UnfoldedCallGraph::vertex_descriptor v, const UnfoldedCallGraph& ufcg);
   void finish_vertex(UnfoldedCallGraph::vertex_descriptor v, const UnfoldedCallGraph&);
   void examine_edge(const UnfoldedCallGraph::edge_descriptor& e, const UnfoldedCallGraph& cg);
};

HWCallPathCalculator::HWCallPathCalculator(const HLS_managerRef _HLSMgr) : HLSMgr(_HLSMgr)
{
   THROW_ASSERT(HLSMgr->RDiscr, "Discr data structure is not correctly initialized");
}

void HWCallPathCalculator::start_vertex(UnfoldedCallGraph::vertex_descriptor v, const UnfoldedCallGraph& ufcg)
{
   scope = std::stack<std::string>();
   shared_fun_scope.clear();
   const auto top_fu_name = ufcg.CGetNodeInfo(v).behavior->CGetBehavioralHelper()->GetFunctionName();

   top_fun_scope = "clocked_bambu_testbench" HIERARCHY_SEPARATOR "bambu_testbench" HIERARCHY_SEPARATOR
                   "system" HIERARCHY_SEPARATOR "DUT" HIERARCHY_SEPARATOR "top" HIERARCHY_SEPARATOR +
                   HDL_manager::convert_to_identifier(TOP_FUNCTION_WRAPPER_PREFIX + top_fu_name + "_i0") +
                   HIERARCHY_SEPARATOR;

   HLSMgr->RDiscr->unfolded_v_to_scope[v] = top_fun_scope;
   const auto f_id = ufcg.CGetNodeInfo(v).f_id;
   HLSMgr->RDiscr->f_id_to_scope[f_id].insert(top_fun_scope);
}

void HWCallPathCalculator::discover_vertex(UnfoldedCallGraph::vertex_descriptor v, const UnfoldedCallGraph& ufcg)
{
   // get the function id
   const auto BH = ufcg.CGetNodeInfo(v).behavior->CGetBehavioralHelper();
   const auto f_id = ufcg.CGetNodeInfo(v).f_id;
   if(!BH->has_implementation() || !BH->function_has_to_be_printed(f_id))
   {
      scope.push("");
      return;
   }
   THROW_ASSERT(HLSMgr->RDiscr->unfolded_v_to_scope.count(v), "can't find scope for function " + STR(f_id));
   scope.push(HLSMgr->RDiscr->unfolded_v_to_scope.at(v));
   THROW_ASSERT(!scope.top().empty(), "Empty HW scope for function " + STR(f_id));
   /*
    * if there are shared functions allocated in this vertex, store their scope
    * in the map, so it can be pushed on the scope stack later, when the
    * exploration of the call graph reaches the vertex corresponding to those
    * functions
    */
   if(HLSMgr->Rfuns->has_shared_functions(f_id))
   {
      for(const auto& shared_fu_name : HLSMgr->Rfuns->get_shared_functions(f_id))
      {
         const auto fu_scope = HDL_manager::convert_to_identifier(scope.top() + "Datapath_i") + HIERARCHY_SEPARATOR +
                               HDL_manager::convert_to_identifier(shared_fu_name + "_instance") + HIERARCHY_SEPARATOR +
                               HDL_manager::convert_to_identifier(shared_fu_name + "_i") + HIERARCHY_SEPARATOR;
         shared_fun_scope[shared_fu_name] = fu_scope;
      }
   }
}

void HWCallPathCalculator::finish_vertex(UnfoldedCallGraph::vertex_descriptor, const UnfoldedCallGraph&)
{
   scope.pop();
}

void HWCallPathCalculator::examine_edge(const UnfoldedCallGraph::edge_descriptor& e, const UnfoldedCallGraph& ufcg)
{
   const auto tgt = ufcg.target(e);
   const auto called_f_id = ufcg.CGetNodeInfo(tgt).f_id;
   const auto BH = ufcg.CGetNodeInfo(tgt).behavior->CGetBehavioralHelper();
   if(!BH->has_implementation() || !BH->function_has_to_be_printed(called_f_id))
   {
      return;
   }

   const auto called_fu_name = [&]() {
      const auto fu_name = functions::GetFUName(BH->GetFunctionName(), HLSMgr);
      return starts_with(fu_name, WRAPPED_PROXY_PREFIX) ? fu_name.substr(sizeof(WRAPPED_PROXY_PREFIX) - 1) : fu_name;
   }();
   std::string called_scope;
   if(HLSMgr->Rfuns->is_a_proxied_function(called_fu_name))
   {
      THROW_ASSERT(shared_fun_scope.count(called_fu_name), called_fu_name + " was not allocated in a dominator");
      called_scope = shared_fun_scope.at(called_fu_name);
   }
   else
   {
      if(ufcg.CGetEdgeInfo(e).is_direct)
      {
         const auto call_id = ufcg.CGetEdgeInfo(e).call_id;
         THROW_ASSERT(call_id != 0U, "No artificial calls allowed in UnfoldedCallGraph");
         const auto src = ufcg.source(e);
         const auto& caller_f_id = ufcg.CGetNodeInfo(src).f_id;
         const auto& caller_behavior = ufcg.CGetNodeInfo(src).behavior;
         const auto op_graph = caller_behavior->GetOpGraph(FunctionBehavior::CFG);
         const auto call_op_v = op_graph.CGetGraphInfo().ir_node_to_operation.at(call_id);
         const auto& fu_bind = HLSMgr->get_HLS(caller_f_id)->Rfu;
         const auto fu_type_id = fu_bind->get_assign(call_op_v);
         const auto fu_instance_id = fu_bind->get_index(call_op_v);
         std::string extra_path;
         if(HLSMgr->hasToBeInterfaced(called_f_id))
         {
            extra_path += HDL_manager::convert_to_identifier(called_fu_name + "_int_i0") + HIERARCHY_SEPARATOR;
         }

         if(fu_bind->get_operations(fu_type_id, fu_instance_id).size() == 1U)
         {
            called_scope = HDL_manager::convert_to_identifier(scope.top() + "Datapath_i") + HIERARCHY_SEPARATOR +
                           HDL_manager::convert_to_identifier("fu_" + op_graph.CGetNodeInfo(call_op_v).vertex_name) +
                           HIERARCHY_SEPARATOR + extra_path;
         }
         else
         {
            called_scope = HDL_manager::convert_to_identifier(scope.top() + "Datapath_i") + HIERARCHY_SEPARATOR +
                           HDL_manager::convert_to_identifier(fu_bind->get_fu_name(call_op_v) + "_i" +
                                                              STR(fu_bind->get_index(call_op_v))) +
                           HIERARCHY_SEPARATOR + extra_path;
         }
      }
      else
      {
         called_scope = HDL_manager::convert_to_identifier(top_fun_scope + "Datapath_i") + HIERARCHY_SEPARATOR +
                        HDL_manager::convert_to_identifier(called_fu_name + "_i0") + HIERARCHY_SEPARATOR +
                        HDL_manager::convert_to_identifier(called_fu_name + "_int_i0") + HIERARCHY_SEPARATOR;
      }
   }
   HLSMgr->RDiscr->f_id_to_scope[called_f_id].insert(called_scope);
   HLSMgr->RDiscr->unfolded_v_to_scope[tgt] = called_scope;
}

HWPathComputation::HWPathComputation(const ParameterConstRef _Param, const HLS_managerRef _HLSMgr,
                                     const DesignFlowManager& _design_flow_manager)
    : HLS_step(_Param, _HLSMgr, _design_flow_manager, HLSFlowStep_Type::HW_PATH_COMPUTATION)
{
}

HLS_step::HLSRelationships
HWPathComputation::ComputeHLSRelationships(const DesignFlowStep::RelationshipType relationship_type) const
{
   HLSRelationships ret;

   switch(relationship_type)
   {
      case(DEPENDENCE_RELATIONSHIP):
      {
         ret.insert(std::make_tuple(HLSFlowStep_Type::HLS_SYNTHESIS_FLOW, HLSFlowStepSpecializationConstRef(),
                                    HLSFlowStep_Relationship::TOP_FUNCTION));
         ret.insert(std::make_tuple(HLSFlowStep_Type::CALL_GRAPH_UNFOLDING, HLSFlowStepSpecializationConstRef(),
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

bool HWPathComputation::HasToBeExecuted() const
{
   return true;
}

DesignFlowStep_Status HWPathComputation::Exec()
{
   // Calculate the HW paths and store them in Discrepancy
   INDENT_OUT_MEX(OUTPUT_LEVEL_VERBOSE, output_level, "-->Unfolding call graph");
   HWCallPathCalculator sig_sel_v(HLSMgr);
   std::vector<boost::default_color_type> sig_sel_color(HLSMgr->RDiscr->DiscrepancyCallGraph.num_vertices(),
                                                        boost::white_color);
   boost::depth_first_visit(
       HLSMgr->RDiscr->DiscrepancyCallGraph, HLSMgr->RDiscr->unfolded_root_v, sig_sel_v,
       boost::make_iterator_property_map(sig_sel_color.begin(),
                                         boost::get(boost::vertex_index_t(), HLSMgr->RDiscr->DiscrepancyCallGraph),
                                         boost::white_color));
   INDENT_OUT_MEX(OUTPUT_LEVEL_VERBOSE, output_level, "<--Unfolded call graph");
   return DesignFlowStep_Status::SUCCESS;
}
