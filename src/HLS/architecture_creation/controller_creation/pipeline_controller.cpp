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
 *              Copyright (C) 2004-2019 Politecnico di Milano
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
 * @file pipeline_controller.cpp
 * @brief Starting from the FSM graph, it generates a shift register for control
 *
 * @author Luca Ezio Pozzoni <lucaezio.pozzoni@mail.polimi.it>
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 *
 */

#include "pipeline_controller.hpp"

#include "behavioral_helper.hpp"
#include "function_behavior.hpp"
#include "hls.hpp"
#include "hls_manager.hpp"
#include "hls_target.hpp"

#include "BambuParameter.hpp"

#include "commandport_obj.hpp"
#include "conn_binding.hpp"
#include "connection_obj.hpp"
#include "fu_binding.hpp"
#include "funit_obj.hpp"
#include "mux_obj.hpp"
#include "reg_binding.hpp"
#include "register_obj.hpp"
#include "schedule.hpp"

#include "state_transition_graph.hpp"
#include "state_transition_graph_manager.hpp"

#include "exceptions.hpp"
#include <iosfwd>

#include "tree_helper.hpp"
#include "tree_manager.hpp"
#include "tree_node.hpp"
#include "tree_reindex.hpp"

#include "structural_manager.hpp"
#include "structural_objects.hpp"

#include "dbgPrintHelper.hpp"
#include "op_graph.hpp"
#include "technology_manager.hpp"
#include "time_model.hpp"

/// HLS/binding/storage_value_insertion
#include "storage_value_information.hpp"

/// HLS/liveness include
#include "liveness.hpp"

/// HLS/module_allocation
#include "allocation_information.hpp"

/// STL includes
#include "custom_map.hpp"
#include "custom_set.hpp"
#include <deque>
#include <list>
#include <utility>
#include <vector>

/// technology/physical_library include
#include "technology_node.hpp"

#include "copyrights_strings.hpp"
#include "string_manipulation.hpp"

pipeline_controller::pipeline_controller(const ParameterConstRef _Param, const HLS_managerRef _HLSMgr, unsigned int _funId, const DesignFlowManagerConstRef _design_flow_manager, const HLSFlowStep_Type _hls_flow_step_type)
    : ControllerCreatorBaseStep(_Param, _HLSMgr, _funId, _design_flow_manager, _hls_flow_step_type)
{
   debug_level = parameters->get_class_debug_level(GET_CLASS(*this), DEBUG_LEVEL_NONE);
}

pipeline_controller::~pipeline_controller() = default;

DesignFlowStep_Status pipeline_controller::InternalExec()
{
   THROW_ASSERT(HLS->STG, "State transition graph not created");

   /// top circuit creation
   PRINT_DBG_MEX(DEBUG_LEVEL_VERBOSE, debug_level, "Pipeline shift-register controller creation");
   const FunctionBehaviorConstRef FB = HLSMgr->CGetFunctionBehavior(funId);

   const std::string function_name = FB->CGetBehavioralHelper()->get_function_name();
   /// main circuit type
   structural_type_descriptorRef module_type = structural_type_descriptorRef(new structural_type_descriptor("controller_" + function_name));
   structural_managerRef SM = this->HLS->controller;
   SM->set_top_info("Controller_i", module_type);
   structural_objectRef circuit = SM->get_circ();
   circuit->set_black_box(false);
   this->add_common_ports(circuit, SM);
   structural_objectRef clock_port = circuit->find_member(CLOCK_PORT_NAME, port_o_K, circuit);
   structural_objectRef reset_port = circuit->find_member(RESET_PORT_NAME, port_o_K, circuit);
   structural_objectRef done_port = circuit->find_member(DONE_PORT_NAME, port_o_K, circuit);
   structural_objectRef start_port = circuit->find_member(START_PORT_NAME, port_o_K, circuit);

   PRINT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "Determining size of the controller...");
   unsigned int num_states = HLS->STG->get_number_of_states() - 1;

   std::string name = "controller_" + function_name;
   std::string library = HLS->HLS_T->get_technology_manager()->get_library(register_SHIFT);
   structural_objectRef controller = SM->add_module_from_technology_library(name, register_SHIFT, library, circuit, HLS->HLS_T->get_technology_manager());
   controller->SetParameter("CONTROLLER_LENGTH", std::to_string(num_states));
   const StateTransitionGraphConstRef astg = HLS->STG->CGetAstg();
   const OpGraphConstRef data = FB->CGetOpGraph(FunctionBehavior::CFG);
   std::list<vertex> working_list;
   astg->TopologicalSort(working_list);
   vertex entry = HLS->STG->get_entry_state();
   THROW_ASSERT(boost::out_degree(entry, *astg) == 1, "Non deterministic initial state");
   OutEdgeIterator oe, oend;
   boost::tie(oe, oend) = boost::out_edges(entry, *astg);
   /// Getting first state (initial one). It will be also first state for resetting
   vertex first_state = boost::target(*oe, *astg);
   THROW_ASSERT(std::find(working_list.begin(), working_list.end(), first_state) != working_list.end(), "unexpected case");
   working_list.erase(std::find(working_list.begin(), working_list.end(), first_state));
   working_list.push_front(first_state); /// ensure that first_state is the really first one...
   std::map<unsigned int, structural_objectRef> null_values;
   for(const auto& v : working_list)
   {
      const auto& operations = astg->CGetStateInfo(v)->executing_operations;
      for(const auto& op : operations)
      {
         technology_nodeRef tn = HLS->allocation_information->get_fu(HLS->Rfu->get_assign(op));
         technology_nodeRef op_tn = GetPointer<functional_unit>(tn)->get_operation(tree_helper::normalized_ID(data->CGetOpNodeInfo(op)->GetOperation()));
         THROW_ASSERT(GetPointer<operation>(op_tn)->time_m, "Time model not available for operation: " + GET_NAME(data, op));
         structural_managerRef CM = GetPointer<functional_unit>(tn)->CM;
         if(!CM)
            continue;
         structural_objectRef top = CM->get_circ();
         THROW_ASSERT(top, "expected");
         auto* fu_module = GetPointer<module>(top);
         THROW_ASSERT(fu_module, "expected");
         structural_objectRef start_port_i = fu_module->find_member(START_PORT_NAME, port_o_K, top);
         if((GET_TYPE(data, op) & TYPE_EXTERNAL) && start_port_i)
         {
            auto genObj = HLS->Rconn->bind_selector_port(conn_binding::IN, commandport_obj::UNBOUNDED, op, data);
            THROW_ASSERT(genObj, "null object");
            auto portObj = GetPointer<commandport_obj>(genObj)->get_controller_obj();
            THROW_ASSERT(portObj, "null object");
            if(null_values.find(GET_TYPE_SIZE(portObj)) == null_values.end())
            {
               PRINT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "Adding null value for bitsize " + STR(GET_TYPE_SIZE(portObj)));
               structural_objectRef const_obj = SM->add_constant("null_value_" + STR(GET_TYPE_SIZE(portObj)), circuit, portObj->get_typeRef(), STR(0));
               null_values[GET_TYPE_SIZE(portObj)] = const_obj;
            }
            SM->add_connection(portObj, null_values[GET_TYPE_SIZE(portObj)]);
         }
      }
   }
   structural_objectRef port_ck = controller->find_member(CLOCK_PORT_NAME, port_o_K, controller);
   SM->add_connection(clock_port, port_ck);
   structural_objectRef port_rst = controller->find_member(RESET_PORT_NAME, port_o_K, controller);
   SM->add_connection(reset_port, port_rst);
   structural_objectRef port_dn = controller->find_member("out1", port_o_K, controller);
   SM->add_connection(done_port, port_dn);
   structural_objectRef port_strt = controller->find_member("in1", port_o_K, controller);
   SM->add_connection(start_port, port_strt);

   PRINT_DBG_MEX(DEBUG_LEVEL_VERBOSE, debug_level, "Created a shift register with " + std::to_string(num_states) + " bits as pipeline controller");

   out_ports.clear();
   mu_ports.clear();
   cond_ports.clear();
   return DesignFlowStep_Status::SUCCESS;
}
