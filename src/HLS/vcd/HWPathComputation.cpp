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
 * @author Pietro Fezzardi <pietrofezzardi@gmail.com>
 */

// class header
#include "HWPathComputation.hpp"

#include <utility>

// headers from behavior/
#include "behavioral_helper.hpp"
#include "call_graph_manager.hpp"
#include "function_behavior.hpp"
#include "op_graph.hpp"

// headers from circuit/
#include "structural_objects.hpp"

// headers from HLS/
#include "hls.hpp"
#include "hls_manager.hpp"
#include "hls_step.hpp"

// headers from HLS/binding/module/
#include "fu_binding.hpp"

// headers from HLS/function_allocation/
#include "functions.hpp"

// headers from HLS/vcd
#include "Discrepancy.hpp"
#include "UnfoldedCallInfo.hpp"
#include "UnfoldedFunctionInfo.hpp"

#include "dbgPrintHelper.hpp"

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

   ~HWCallPathCalculator();

   void start_vertex(const UnfoldedVertexDescriptor& v, const UnfoldedCallGraph& ucg);
   void discover_vertex(const UnfoldedVertexDescriptor& v, const UnfoldedCallGraph& ucg);
   void finish_vertex(const UnfoldedVertexDescriptor& v, const UnfoldedCallGraph&);
   void examine_edge(const EdgeDescriptor& e, const UnfoldedCallGraph& cg);
};

HWCallPathCalculator::HWCallPathCalculator(const HLS_managerRef _HLSMgr) : HLSMgr(_HLSMgr)
{
   THROW_ASSERT(HLSMgr->RDiscr, "Discr data structure is not correctly initialized");
}

HWCallPathCalculator::~HWCallPathCalculator() = default;

void HWCallPathCalculator::start_vertex(const UnfoldedVertexDescriptor& v, const UnfoldedCallGraph& ufcg)
{
   // cleanup internal data structures
   scope = std::stack<std::string>();
   shared_fun_scope.clear();
   top_fun_scope.clear();
   // start doing the real things
   const ParameterConstRef parameters = HLSMgr->get_parameter();
   std::string interface_scope;
   std::string top_interface_name;
   std::string simulator_scope;

   const std::string top_fu_name = Cget_node_info<UnfoldedFunctionInfo>(v, ufcg)->behavior->CGetBehavioralHelper()->get_function_name();
   // top module scope
   std::string top_name = "_" + top_fu_name + "_i0" + STR(HIERARCHY_SEPARATOR);
   if(HLSMgr->CGetCallGraphManager()->ExistsAddressedFunction() or (parameters->isOption(OPT_interface_type) and parameters->getOption<HLSFlowStep_Type>(OPT_interface_type) == HLSFlowStep_Type::WB4_INTERFACE_GENERATION))
   {
      top_name += "_" + top_fu_name + "_int_i0" + STR(HIERARCHY_SEPARATOR);
   }

   // top interface scope (depending on the interface)
   if(parameters->isOption(OPT_interface_type) and
      (parameters->getOption<HLSFlowStep_Type>(OPT_interface_type) == HLSFlowStep_Type::MINIMAL_INTERFACE_GENERATION || parameters->getOption<HLSFlowStep_Type>(OPT_interface_type) == HLSFlowStep_Type::INFERRED_INTERFACE_GENERATION))
   {
      top_interface_name = top_fu_name;
      interface_scope = top_interface_name + STR(HIERARCHY_SEPARATOR);
   }
   else if(parameters->isOption(OPT_interface_type) and parameters->getOption<HLSFlowStep_Type>(OPT_interface_type) == HLSFlowStep_Type::WB4_INTERFACE_GENERATION)
   {
      top_interface_name = top_fu_name + "_minimal_interface_wb4_interface";
      interface_scope = top_interface_name + STR(HIERARCHY_SEPARATOR) + top_fu_name + "_i0" + STR(HIERARCHY_SEPARATOR);
   }
   else
   {
      THROW_ERROR("signal selection for this interface type is not supported in discrepancy");
   }
   // simulation top scope, depends on simulator and interface
   if(parameters->isOption(OPT_simulator))
   {
      if(parameters->getOption<std::string>(OPT_simulator) == "MODELSIM" || parameters->getOption<std::string>(OPT_simulator) == "ICARUS" || parameters->getOption<std::string>(OPT_simulator) == "XSIM")
      {
         simulator_scope = top_interface_name + "_tb_top" + STR(HIERARCHY_SEPARATOR) + "DUT" + STR(HIERARCHY_SEPARATOR);
      }
      else if(parameters->getOption<std::string>(OPT_simulator) == "VERILATOR")
      {
         simulator_scope = "TOP" + STR(HIERARCHY_SEPARATOR) + "v" + STR(HIERARCHY_SEPARATOR);
      }
   }
   else
   {
      THROW_ERROR("signal selection for discrepancy analysis is supported only for simulators: ICARUS, XSIM, VERILATOR and MODELSIM");
   }
   top_fun_scope = simulator_scope + interface_scope + top_name;
   HLSMgr->RDiscr->unfolded_v_to_scope[v] = top_fun_scope;
   const unsigned int f_id = Cget_node_info<UnfoldedFunctionInfo>(v, ufcg)->f_id;
   HLSMgr->RDiscr->f_id_to_scope[f_id].insert(top_fun_scope);
}

void HWCallPathCalculator::discover_vertex(const UnfoldedVertexDescriptor& v, const UnfoldedCallGraph& ufcg)
{
   // get the function id
   const BehavioralHelperConstRef BH = Cget_node_info<UnfoldedFunctionInfo>(v, ufcg)->behavior->CGetBehavioralHelper();
   const unsigned int f_id = Cget_node_info<UnfoldedFunctionInfo>(v, ufcg)->f_id;
   if(not BH->has_implementation() or not BH->function_has_to_be_printed(f_id))
   {
      scope.push("");
      return;
   }
   THROW_ASSERT(HLSMgr->RDiscr->unfolded_v_to_scope.find(v) != HLSMgr->RDiscr->unfolded_v_to_scope.end(), "can't find scope for function " + STR(f_id));
   scope.push(HLSMgr->RDiscr->unfolded_v_to_scope.at(v));
   THROW_ASSERT(not scope.top().empty(), "Empty HW scope for function " + STR(f_id));
   /*
    * if there are shared functions allocated in this vertex, store their scope
    * in the map, so it can be pushed on the scope stack later, when the
    * exploration of the call graph reaches the vertex corresponding to those
    * functions
    */
   if(HLSMgr->Rfuns->has_shared_functions(f_id))
   {
      for(const std::string& shared_fu_name : HLSMgr->Rfuns->get_shared_functions(f_id))
      {
         shared_fun_scope[shared_fu_name] = scope.top() + "Datapath_i" + STR(HIERARCHY_SEPARATOR) + shared_fu_name + "_instance" + STR(HIERARCHY_SEPARATOR) + shared_fu_name + "_i" + STR(HIERARCHY_SEPARATOR);
      }
   }
}

void HWCallPathCalculator::finish_vertex(const UnfoldedVertexDescriptor&, const UnfoldedCallGraph&)
{
   scope.pop();
}

void HWCallPathCalculator::examine_edge(const EdgeDescriptor& e, const UnfoldedCallGraph& ufcg)
{
   const UnfoldedVertexDescriptor tgt = boost::target(e, ufcg);
   const unsigned int called_f_id = Cget_node_info<UnfoldedFunctionInfo>(tgt, ufcg)->f_id;
   const BehavioralHelperConstRef BH = Cget_node_info<UnfoldedFunctionInfo>(tgt, ufcg)->behavior->CGetBehavioralHelper();
   if(not BH->has_implementation() or not BH->function_has_to_be_printed(called_f_id))
   {
      return;
   }

   const std::string called_fu_name = functions::get_function_name_cleaned(BH->get_function_name());
   std::string called_scope;
   if(HLSMgr->Rfuns->is_a_proxied_function(called_fu_name))
   {
      THROW_ASSERT(shared_fun_scope.find(called_fu_name) != shared_fun_scope.end(), STR(called_fu_name) + " was not allocated in a dominator");
      called_scope = shared_fun_scope.at(called_fu_name);
   }
   else
   {
      if(Cget_raw_edge_info<UnfoldedCallInfo>(e, ufcg)->is_direct)
      {
         const unsigned int call_id = Cget_raw_edge_info<UnfoldedCallInfo>(e, ufcg)->call_id;
         THROW_ASSERT(call_id != 0, "No artificial calls allowed in UnfoldedCallGraph");
         const UnfoldedVertexDescriptor src = boost::source(e, ufcg);
         const unsigned int caller_f_id = Cget_node_info<UnfoldedFunctionInfo>(src, ufcg)->f_id;
         const auto caller_behavior = Cget_node_info<UnfoldedFunctionInfo>(src, ufcg)->behavior;
         const OpGraphConstRef op_graph = caller_behavior->CGetOpGraph(FunctionBehavior::CFG);
         const vertex call_op_v = op_graph->CGetOpGraphInfo()->tree_node_to_operation.at(call_id);
         const fu_bindingConstRef fu_bind = HLSMgr->get_HLS(caller_f_id)->Rfu;
         unsigned int fu_type_id = fu_bind->get_assign(call_op_v);
         unsigned int fu_instance_id = fu_bind->get_index(call_op_v);
         std::string extra_path;
         if(HLSMgr->hasToBeInterfaced(called_f_id))
         {
            extra_path += called_fu_name + "_int_i0" + STR(HIERARCHY_SEPARATOR);
         }

         if(fu_bind->get_operations(fu_type_id, fu_instance_id).size() == 1)
         {
            called_scope = scope.top() + "Datapath_i" + STR(HIERARCHY_SEPARATOR) + "fu_" + GET_NAME(op_graph, call_op_v) + STR(HIERARCHY_SEPARATOR) + extra_path;
         }
         else
         {
            called_scope = scope.top() + "Datapath_i" + STR(HIERARCHY_SEPARATOR) + fu_bind->get_fu_name(call_op_v) + "_i" + STR(fu_bind->get_index(call_op_v)) + STR(HIERARCHY_SEPARATOR) + extra_path;
         }
      }
      else
      {
         called_scope = top_fun_scope + "Datapath_i" + STR(HIERARCHY_SEPARATOR) + called_fu_name + "_i0" + STR(HIERARCHY_SEPARATOR) + called_fu_name + "_int_i0" + STR(HIERARCHY_SEPARATOR);
      }
   }
   HLSMgr->RDiscr->f_id_to_scope[called_f_id].insert(called_scope);
   HLSMgr->RDiscr->unfolded_v_to_scope[tgt] = called_scope;
}

const CustomUnorderedSet<std::tuple<HLSFlowStep_Type, HLSFlowStepSpecializationConstRef, HLSFlowStep_Relationship>> HWPathComputation::ComputeHLSRelationships(const DesignFlowStep::RelationshipType relationship_type) const
{
   CustomUnorderedSet<std::tuple<HLSFlowStep_Type, HLSFlowStepSpecializationConstRef, HLSFlowStep_Relationship>> ret;

   switch(relationship_type)
   {
      case DEPENDENCE_RELATIONSHIP:
      {
         ret.insert(std::make_tuple(HLSFlowStep_Type::HLS_SYNTHESIS_FLOW, HLSFlowStepSpecializationConstRef(), HLSFlowStep_Relationship::TOP_FUNCTION));
         ret.insert(std::make_tuple(HLSFlowStep_Type::CALL_GRAPH_UNFOLDING, HLSFlowStepSpecializationConstRef(), HLSFlowStep_Relationship::TOP_FUNCTION));
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

DesignFlowStep_Status HWPathComputation::Exec()
{
   // Calculate the HW paths and store them in Discrepancy
   INDENT_OUT_MEX(OUTPUT_LEVEL_VERBOSE, output_level, "-->Unfolding call graph");
   HWCallPathCalculator sig_sel_v(HLSMgr);
   std::vector<boost::default_color_type> sig_sel_color(boost::num_vertices(HLSMgr->RDiscr->DiscrepancyCallGraph), boost::white_color);
   boost::depth_first_visit(HLSMgr->RDiscr->DiscrepancyCallGraph, HLSMgr->RDiscr->unfolded_root_v, sig_sel_v,
                            boost::make_iterator_property_map(sig_sel_color.begin(), boost::get(boost::vertex_index_t(), HLSMgr->RDiscr->DiscrepancyCallGraph), boost::white_color));
   INDENT_OUT_MEX(OUTPUT_LEVEL_VERBOSE, output_level, "<--Unfolded call graph");
   return DesignFlowStep_Status::SUCCESS;
}

HWPathComputation::HWPathComputation(const ParameterConstRef _Param, const HLS_managerRef _HLSMgr, const DesignFlowManagerConstRef _design_flow_manager) : HLS_step(_Param, _HLSMgr, _design_flow_manager, HLSFlowStep_Type::HW_PATH_COMPUTATION)
{
}

HWPathComputation::~HWPathComputation()
{
}

bool HWPathComputation::HasToBeExecuted() const
{
   return true;
}
