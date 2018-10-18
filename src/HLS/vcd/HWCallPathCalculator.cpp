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
 *              Copyright (c) 2004-2018 Politecnico di Milano
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
#include "HWCallPathCalculator.hpp"

#include <utility>

// headers from ./
#include "Parameter.hpp"

// headers from behavior/
#include "behavioral_helper.hpp"
#include "call_graph_manager.hpp"
#include "function_behavior.hpp"

// headers from circuit/
#include "structural_objects.hpp"

// headers from HLS/
#include "hls_manager.hpp"
#include "hls_step.hpp"
#include "hls.hpp"

// headers from HLS/binding/module/
#include "fu_binding.hpp"

// headers from HLS/function_allocation/
#include "functions.hpp"

// headers from HLS/vcd
#include "Discrepancy.hpp"

HWCallPathCalculator::HWCallPathCalculator(
      HLS_managerRef _HLSMgr,
      ParameterConstRef _parameters,
      std::map<unsigned int, vertex> & _call_id_to_OpVertex)
   : HLSMgr(std::move(_HLSMgr))
   , parameters(std::move(_parameters))
   , call_id_to_OpVertex(_call_id_to_OpVertex)
{
   THROW_ASSERT(parameters->isOption(OPT_discrepancy) and
         parameters->getOption<bool>(OPT_discrepancy),
         "Step " + STR(__PRETTY_FUNCTION__) + " should not be added without discrepancy");
   THROW_ASSERT(HLSMgr->RDiscr, "Discr data structure is not correctly initialized");
}

HWCallPathCalculator::~HWCallPathCalculator()
= default;

void HWCallPathCalculator::start_vertex(
      const UnfoldedVertexDescriptor & v,
      const UnfoldedCallGraph & ufcg)
{
   std::string interface_scope;
   std::string top_interface_name;
   std::string simulator_scope;

   const std::string top_fu_name = Cget_node_info<UnfoldedFunctionInfo>(v, ufcg)->behavior->CGetBehavioralHelper()->get_function_name();
   // top module scope
   std::string top_name = top_fu_name + "_i0" + STR(HIERARCHY_SEPARATOR);
   if (HLSMgr->CGetCallGraphManager()->ExistsAddressedFunction() or
         (parameters->isOption(OPT_interface_type) and
          parameters->getOption<HLSFlowStep_Type>(OPT_interface_type) ==
          HLSFlowStep_Type::WB4_INTERFACE_GENERATION))
   {
      top_name += top_fu_name + "_int_i0" + STR(HIERARCHY_SEPARATOR);
   }

   // top interface scope (depending on the interface)
   if (parameters->isOption(OPT_interface_type) and
         (parameters->getOption<HLSFlowStep_Type>(OPT_interface_type) ==
         HLSFlowStep_Type::MINIMAL_INTERFACE_GENERATION ||
          parameters->getOption<HLSFlowStep_Type>(OPT_interface_type) ==
                   HLSFlowStep_Type::INFER_INTERFACE_GENERATION))
   {
      top_interface_name = top_fu_name + "_minimal_interface";
      interface_scope = top_interface_name + STR(HIERARCHY_SEPARATOR);
   }
   else if (parameters->isOption(OPT_interface_type) and
         parameters->getOption<HLSFlowStep_Type>(OPT_interface_type) ==
         HLSFlowStep_Type::WB4_INTERFACE_GENERATION)
   {
      top_interface_name = top_fu_name + "_minimal_interface_wb4_interface";
      interface_scope = top_interface_name + STR(HIERARCHY_SEPARATOR) +
         top_fu_name + "_minimal_interface_i0" + STR(HIERARCHY_SEPARATOR);
   }
   else
   {
      THROW_ERROR("signal selection for this interface type is not supported in discrepancy");
   }
   // simulation top scope, depends on simulator and interface
   if (parameters->isOption(OPT_simulator))
   {
      if (parameters->getOption<std::string>(OPT_simulator) == "MODELSIM" ||
         parameters->getOption<std::string>(OPT_simulator) == "ICARUS")
      {
         simulator_scope =
            top_interface_name + "_tb_top" + STR(HIERARCHY_SEPARATOR) +
            "DUT" + STR(HIERARCHY_SEPARATOR);
      }
      else if (parameters->getOption<std::string>(OPT_simulator) == "VERILATOR")
      {
         simulator_scope =
               "TOP" + STR(HIERARCHY_SEPARATOR) +
               "v" + STR(HIERARCHY_SEPARATOR);
      }
   }
   else
   {
      THROW_ERROR("signal selection for discrepancy analysis is supported only for simulators: ICARUS, VERILATOR and MODELSIM");
   }
   top_fun_scope = simulator_scope + interface_scope + top_name;
   HLSMgr->RDiscr->unfolded_v_to_scope[v] = top_fun_scope;
   const unsigned int f_id = Cget_node_info<UnfoldedFunctionInfo>(v, ufcg)->f_id;
   HLSMgr->RDiscr->f_id_to_scope[f_id].insert(top_fun_scope);
}

void HWCallPathCalculator::discover_vertex(
      const UnfoldedVertexDescriptor & v,
      const UnfoldedCallGraph & ufcg)
{
   // get the function id
   const BehavioralHelperConstRef BH =
      Cget_node_info<UnfoldedFunctionInfo>(v, ufcg)->behavior->CGetBehavioralHelper();
   const unsigned int f_id = Cget_node_info<UnfoldedFunctionInfo>(v, ufcg)->f_id;
   if (not BH->has_implementation() or not BH->function_has_to_be_printed(f_id))
   {
      scope.push("");
      return;
   }
   THROW_ASSERT(HLSMgr->RDiscr->unfolded_v_to_scope.find(v) !=
         HLSMgr->RDiscr->unfolded_v_to_scope.end(),
         "can't find scope for function " + STR(f_id));
   scope.push(HLSMgr->RDiscr->unfolded_v_to_scope.at(v));
   THROW_ASSERT(not scope.top().empty(), "Empty HW scope for function " + STR(f_id));
   /*
    * if there are shared functions allocated in this vertex, store their scope
    * in the map, so it can be pushed on the scope stack later, when the
    * exploration of the call graph reaches the vertex corresponding to those
    * functions
    */
   if (HLSMgr->Rfuns->has_shared_functions(f_id))
   {
      for (const std::string& shared_fu_name : HLSMgr->Rfuns->get_shared_functions(f_id))
      {
         shared_fun_scope[shared_fu_name] =
            scope.top() + "Datapath_i" + STR(HIERARCHY_SEPARATOR) +
            shared_fu_name + "_instance" + STR(HIERARCHY_SEPARATOR) +
            shared_fu_name + "_i" + STR(HIERARCHY_SEPARATOR);
      }
   }
}

void HWCallPathCalculator::finish_vertex(
      const UnfoldedVertexDescriptor &,
      const UnfoldedCallGraph &)
{
   scope.pop();
}

void HWCallPathCalculator::examine_edge(
      const EdgeDescriptor & e,
      const UnfoldedCallGraph & ufcg)
{
   const UnfoldedVertexDescriptor tgt = boost::target(e, ufcg);
   const unsigned int called_f_id =
      Cget_node_info<UnfoldedFunctionInfo>(tgt, ufcg)->f_id;
   const BehavioralHelperConstRef BH =
      Cget_node_info<UnfoldedFunctionInfo>(tgt, ufcg)->behavior->CGetBehavioralHelper();
   if (not BH->has_implementation() or not BH->function_has_to_be_printed(called_f_id))
   {
      return;
   }

   const std::string called_fu_name = BH->get_function_name();
   std::string called_scope;
   if (HLSMgr->Rfuns->is_a_proxied_function(called_fu_name))
   {
      THROW_ASSERT(shared_fun_scope.find(called_fu_name) != shared_fun_scope.end(),
            STR(called_fu_name) + " was not allocated in a dominator");
      called_scope = shared_fun_scope.at(called_fu_name);
   }
   else
   {
      if (Cget_raw_edge_info<UnfoldedCallInfo>(e, ufcg)->is_direct)
      {
         const unsigned int call_id = Cget_raw_edge_info<UnfoldedCallInfo>(e, ufcg)->call_id;
         const UnfoldedVertexDescriptor src = boost::source(e, ufcg);
         const unsigned int caller_f_id = Cget_node_info<UnfoldedFunctionInfo>(src, ufcg)->f_id;
         THROW_ASSERT(call_id != 0, "No artificial calls allowed in UnfoldedCallGraph");
         THROW_ASSERT(call_id_to_OpVertex.find(call_id) != call_id_to_OpVertex.end(),
               "cannot find op vertex for call id:\n\t"
               "caller function = " + STR(Cget_node_info<UnfoldedFunctionInfo>(src, ufcg)->behavior->CGetBehavioralHelper()->get_function_name()) + "\n\t"
               "caller id = " + STR(caller_f_id) + "\n\t"
               "call id = " + STR(call_id) + "\n\t"
               "called function = " + STR(called_fu_name) + "\n\t"
               "called id = " + STR(called_f_id) + "\n\t"
               "the call code was probably dead, but the call edge was not removed from the call graph\n");
         const fu_bindingConstRef fu_bind = HLSMgr->get_HLS(caller_f_id)->Rfu;
         const OpGraphConstRef op_graph = HLSMgr->CGetFunctionBehavior(caller_f_id)->CGetOpGraph(FunctionBehavior::FCFG);
         const vertex call_op_v = call_id_to_OpVertex.at(call_id);
         unsigned int fu_type_id = fu_bind->get_assign(call_op_v);
         unsigned int fu_instance_id = fu_bind->get_index(call_op_v);
         std::string extra_path;
         if (HLSMgr->hasToBeInterfaced(called_f_id))
         {
            extra_path  += called_fu_name + "_int_i0" + STR(HIERARCHY_SEPARATOR);
         }

         if (fu_bind->get_operations(fu_type_id, fu_instance_id).size() == 1)
         {
            called_scope = scope.top() + "Datapath_i" + STR(HIERARCHY_SEPARATOR) +
               "fu_" + GET_NAME(op_graph, call_op_v) + STR(HIERARCHY_SEPARATOR) +
               extra_path;
         }
         else
         {
            called_scope = scope.top() + "Datapath_i" + STR(HIERARCHY_SEPARATOR) +
               fu_bind->get_fu_name(call_op_v) + "_i" +
               STR(fu_bind->get_index(call_op_v)) + STR(HIERARCHY_SEPARATOR) +
               extra_path;
         }
      }
      else
      {
         called_scope = top_fun_scope + "Datapath_i" + STR(HIERARCHY_SEPARATOR) +
            called_fu_name + "_i0" + STR(HIERARCHY_SEPARATOR) +
            called_fu_name + "_int_i0" + STR(HIERARCHY_SEPARATOR);
      }
   }
   HLSMgr->RDiscr->f_id_to_scope[called_f_id].insert(called_scope);
   HLSMgr->RDiscr->unfolded_v_to_scope[tgt] = called_scope;
}

