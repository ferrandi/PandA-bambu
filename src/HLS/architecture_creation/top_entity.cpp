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
 * @file top_entity.cpp
 * @brief Implementation of the class creating the top entity.
 *
 * @author Christian Pilato <pilato@elet.polimi.it>
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 *
 */
#include "top_entity.hpp"

#include "behavioral_helper.hpp"
#include "function_behavior.hpp"

#include "call_graph_manager.hpp"
#include "conn_binding.hpp"
#include "fu_binding.hpp"
#include "functions.hpp"
#include "hls.hpp"
#include "hls_manager.hpp"
#include "hls_target.hpp"
#include "memory.hpp"
#include "schedule.hpp"

#include "commandport_obj.hpp"
#include "generic_obj.hpp"
#include "structural_manager.hpp"
#include "structural_objects.hpp"

#include "technology_manager.hpp"

#include "BambuParameter.hpp"
#include "dbgPrintHelper.hpp"
#include "exceptions.hpp"

/// STD include
#include <string>

/// STL includes
#include "custom_map.hpp"
#include "custom_set.hpp"
#include <list>
#include <tuple>

/// technology/physical_library include
#include "copyrights_strings.hpp"
#include "string_manipulation.hpp" // for GET_CLASS
#include "technology_node.hpp"

top_entity::top_entity(const ParameterConstRef _parameters, const HLS_managerRef _HLSMgr, unsigned int _funId, const DesignFlowManagerConstRef _design_flow_manager, const HLSFlowStep_Type _hls_flow_step_type)
    : HLSFunctionStep(_parameters, _HLSMgr, _funId, _design_flow_manager, _hls_flow_step_type)
{
   debug_level = parameters->get_class_debug_level(GET_CLASS(*this));
}

top_entity::~top_entity() = default;

const CustomUnorderedSet<std::tuple<HLSFlowStep_Type, HLSFlowStepSpecializationConstRef, HLSFlowStep_Relationship>> top_entity::ComputeHLSRelationships(const DesignFlowStep::RelationshipType relationship_type) const
{
   CustomUnorderedSet<std::tuple<HLSFlowStep_Type, HLSFlowStepSpecializationConstRef, HLSFlowStep_Relationship>> ret;
   switch(relationship_type)
   {
      case DEPENDENCE_RELATIONSHIP:
      {
         ret.insert(std::make_tuple(parameters->getOption<HLSFlowStep_Type>(OPT_datapath_architecture), HLSFlowStepSpecializationConstRef(), HLSFlowStep_Relationship::SAME_FUNCTION));
         if(HLSMgr->get_HLS(funId))
         {
            ret.insert(std::make_tuple(HLSMgr->get_HLS(funId)->controller_type, HLSFlowStepSpecializationConstRef(), HLSFlowStep_Relationship::SAME_FUNCTION));
         }
         break;
      }
      case INVALIDATION_RELATIONSHIP:
      {
         break;
      }
      case PRECEDENCE_RELATIONSHIP:
      {
         break;
      }
      default:
         THROW_UNREACHABLE("");
   }
   return ret;
}

DesignFlowStep_Status top_entity::InternalExec()
{
   /// function name to be synthesized
   const FunctionBehaviorConstRef FB = HLSMgr->CGetFunctionBehavior(funId);
   const BehavioralHelperConstRef BH = FB->CGetBehavioralHelper();
   std::string function_name = BH->get_function_name();
   std::string module_name = function_name;
   const auto top_functions = HLSMgr->CGetCallGraphManager()->GetRootFunctions();
   bool is_top = top_functions.find(BH->get_function_index()) != top_functions.end();
   if(is_top)
      module_name = "_" + function_name;

   /// Test on previous steps. They checks if datapath and controller have been created. If they didn't,
   /// top circuit cannot be created.
   THROW_ASSERT(HLS->datapath, "Datapath not created");
   THROW_ASSERT(HLS->controller, "Controller not created");

   // reference to hls top circuit
   HLS->top = structural_managerRef(new structural_manager(parameters));
   SM = HLS->top;
   structural_managerRef Datapath = HLS->datapath;
   structural_managerRef Controller = HLS->controller;

   /// top circuit creation
   PRINT_DBG_MEX(DEBUG_LEVEL_VERBOSE, debug_level, "Top circuit creation");

   /// main circuit type
   structural_type_descriptorRef module_type = structural_type_descriptorRef(new structural_type_descriptor(module_name));
   /// setting top circuit component
   SM->set_top_info(module_name, module_type);
   structural_objectRef circuit = SM->get_circ();
   THROW_ASSERT(circuit, "Top circuit is missing");
   // Now the top circuit is created, just as an empty box. <circuit> is a reference to the structural object that
   // will contain all the circuit components

   circuit->set_black_box(false);

   /// Set some descriptions and legal stuff
   GetPointer<module>(circuit)->set_description("Top component for " + function_name);
   GetPointer<module>(circuit)->set_copyright(GENERATED_COPYRIGHT);
   GetPointer<module>(circuit)->set_authors("Component automatically generated by bambu");
   GetPointer<module>(circuit)->set_license(GENERATED_LICENSE);

   structural_objectRef datapath_circuit = Datapath->get_circ();
   THROW_ASSERT(datapath_circuit, "Missing datapath circuit");
   structural_objectRef controller_circuit = Controller->get_circ();
   THROW_ASSERT(controller_circuit, "Missing controller circuit");

   PRINT_DBG_MEX(DEBUG_LEVEL_VERBOSE, debug_level, "Creating datapath object");
   /// creating structural_manager
   PRINT_DBG_MEX(DEBUG_LEVEL_VERBOSE, debug_level, "Adding datapath");
   datapath_circuit->set_owner(circuit);
   GetPointer<module>(circuit)->add_internal_object(datapath_circuit);

   PRINT_DBG_MEX(DEBUG_LEVEL_VERBOSE, debug_level, "Creating controller object");
   /// creating structural_manager
   PRINT_DBG_MEX(DEBUG_LEVEL_VERBOSE, debug_level, "Adding controller");
   controller_circuit->set_owner(circuit);
   GetPointer<module>(circuit)->add_internal_object(controller_circuit);

   /// command signal type descriptor
   structural_type_descriptorRef bool_type = structural_type_descriptorRef(new structural_type_descriptor("bool", 0));

   PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "\tStart adding clock signal...");
   /// add clock port
   structural_objectRef clock_obj = SM->add_port(CLOCK_PORT_NAME, port_o::IN, circuit, bool_type);
   GetPointer<port_o>(clock_obj)->set_is_clock(true);
   /// connect to datapath and controller clock
   structural_objectRef datapath_clock = datapath_circuit->find_member(CLOCK_PORT_NAME, port_o_K, datapath_circuit);
   SM->add_connection(datapath_clock, clock_obj);
   structural_objectRef controller_clock = controller_circuit->find_member(CLOCK_PORT_NAME, port_o_K, controller_circuit);
   SM->add_connection(controller_clock, clock_obj);
   PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "\tClock signal added!");

   PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "\tAdding reset signal...");
   /// add reset port
   structural_objectRef reset_obj = SM->add_port(RESET_PORT_NAME, port_o::IN, circuit, bool_type);
   /// connecting global reset port to the datapath one
   structural_objectRef datapath_reset = datapath_circuit->find_member(RESET_PORT_NAME, port_o_K, datapath_circuit);
   SM->add_connection(datapath_reset, reset_obj);
   /// connecting global reset port to the controller one
   structural_objectRef controller_reset = controller_circuit->find_member(RESET_PORT_NAME, port_o_K, controller_circuit);
   SM->add_connection(controller_reset, reset_obj);
   PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "\tReset signal added!");

   PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "\tAdding start signal...");
   /// start port
   structural_objectRef start_obj = SM->add_port(START_PORT_NAME, port_o::IN, circuit, bool_type);
   structural_objectRef controller_start = controller_circuit->find_member(START_PORT_NAME, port_o_K, controller_circuit);
   /// check if datapath has a start signal
   structural_objectRef datapath_start = datapath_circuit->find_member(START_PORT_NAME, port_o_K, datapath_circuit);
   structural_objectRef sync_datapath_controller;
   if(datapath_start)
   {
      SM->add_connection(start_obj, datapath_start);
      sync_datapath_controller = SM->add_sign("done2start", circuit, bool_type);
      structural_objectRef datapath_done = datapath_circuit->find_member(DONE_PORT_NAME, port_o_K, datapath_circuit);
      SM->add_connection(datapath_done, sync_datapath_controller);
   }
   else
      SM->add_connection(start_obj, controller_start);
   PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "\tStart signal added!");

   PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "\tStart adding Done signal...");
   /// add done port
   structural_objectRef done_obj = SM->add_port(DONE_PORT_NAME, port_o::OUT, circuit, bool_type);
   THROW_ASSERT(done_obj, "Done port not added in the top component");
   structural_objectRef controller_done = controller_circuit->find_member(DONE_PORT_NAME, port_o_K, controller_circuit);
   THROW_ASSERT(controller_done, "Done signal not found in the controller");
   if(datapath_start)
      SM->add_connection(sync_datapath_controller, controller_start);
   structural_objectRef done_signal_out;
   if(HLS->registered_done_port and
      (parameters->getOption<HLSFlowStep_Type>(OPT_controller_architecture) == HLSFlowStep_Type::FSM_CONTROLLER_CREATOR or parameters->getOption<HLSFlowStep_Type>(OPT_controller_architecture) == HLSFlowStep_Type::FSM_CS_CONTROLLER_CREATOR))
   {
      const technology_managerRef TM = HLS->HLS_T->get_technology_manager();
      std::string delay_unit;
      std::string synch_reset = parameters->getOption<std::string>(OPT_sync_reset);
      if(synch_reset == "sync")
         delay_unit = flipflop_SR;
      else
         delay_unit = flipflop_AR;
      structural_objectRef delay_gate = SM->add_module_from_technology_library("done_delayed_REG", delay_unit, LIBRARY_STD, circuit, TM);
      structural_objectRef port_ck = delay_gate->find_member(CLOCK_PORT_NAME, port_o_K, delay_gate);
      if(port_ck)
         SM->add_connection(clock_obj, port_ck);
      structural_objectRef port_rst = delay_gate->find_member(RESET_PORT_NAME, port_o_K, delay_gate);
      if(port_rst)
         SM->add_connection(reset_obj, port_rst);

      structural_objectRef done_signal_in = SM->add_sign("done_delayed_REG_signal_in", circuit, GetPointer<module>(delay_gate)->get_in_port(2)->get_typeRef());
      SM->add_connection(GetPointer<module>(delay_gate)->get_in_port(2), done_signal_in);
      SM->add_connection(controller_done, done_signal_in);
      done_signal_out = SM->add_sign("done_delayed_REG_signal_out", circuit, GetPointer<module>(delay_gate)->get_out_port(0)->get_typeRef());
      SM->add_connection(GetPointer<module>(delay_gate)->get_out_port(0), done_signal_out);
      SM->add_connection(done_obj, done_signal_out);
   }
   else
   {
      if(HLS->control_flow_checker)
      {
         done_signal_out = SM->add_sign("done_signal_out", circuit, bool_type);
         SM->add_connection(controller_done, done_signal_out);
         SM->add_connection(done_signal_out, done_obj);
      }
      else
         SM->add_connection(controller_done, done_obj);
   }
   PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "\tDone signal added!");

   /// check if checker has to be added
   if(HLS->control_flow_checker)
   {
      structural_objectRef controller_flow_start = datapath_circuit->find_member(START_PORT_NAME_CFC, port_o_K, datapath_circuit);
      THROW_ASSERT(controller_flow_start, "controller flow start signal not found in the datapath");
      if(datapath_start)
         SM->add_connection(sync_datapath_controller, controller_flow_start);
      else
         SM->add_connection(start_obj, controller_flow_start);
      THROW_ASSERT(done_signal_out, "expected done signal");
      structural_objectRef controller_flow_done = datapath_circuit->find_member(DONE_PORT_NAME_CFC, port_o_K, datapath_circuit);
      THROW_ASSERT(controller_flow_done, "controller flow done signal not found in the datapath");
      SM->add_connection(done_signal_out, controller_flow_done);
      structural_objectRef controller_flow_present_state = datapath_circuit->find_member(PRESENT_STATE_PORT_NAME, port_o_K, datapath_circuit);
      THROW_ASSERT(controller_flow_present_state, "controller flow present state signal not found in the datapath");
      structural_objectRef controller_present_state = Controller->add_port(PRESENT_STATE_PORT_NAME, port_o::OUT, controller_circuit, controller_flow_present_state->get_typeRef());
      structural_objectRef p_signal = SM->add_sign(PRESENT_STATE_PORT_NAME "_sig1", circuit, controller_flow_present_state->get_typeRef());
      SM->add_connection(controller_present_state, p_signal);
      SM->add_connection(p_signal, controller_flow_present_state);
      structural_objectRef controller_flow_next_state = datapath_circuit->find_member(NEXT_STATE_PORT_NAME, port_o_K, datapath_circuit);
      THROW_ASSERT(controller_flow_next_state, "controller flow next state signal not found in the datapath");
      structural_objectRef controller_next_state = Controller->add_port(NEXT_STATE_PORT_NAME, port_o::OUT, controller_circuit, controller_flow_next_state->get_typeRef());
      structural_objectRef n_signal = SM->add_sign(NEXT_STATE_PORT_NAME "_sig1", circuit, controller_flow_next_state->get_typeRef());
      SM->add_connection(controller_next_state, n_signal);
      SM->add_connection(n_signal, controller_flow_next_state);
   }

   /// add entry in in_port_map between port id and port index

   PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "\tAdding input/output ports...");
   this->add_ports(circuit, clock_obj, reset_obj);
   PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "\tInput/output ports added!");

   PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "\tAdding command ports...");
   this->add_command_signals(circuit);
   PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "\tCommand ports added!");

   if(!is_top || !parameters->isOption(OPT_do_not_expose_globals) || !parameters->getOption<bool>(OPT_do_not_expose_globals))
      memory::propagate_memory_parameters(HLS->datapath->get_circ(), HLS->top);

   PRINT_DBG_MEX(DEBUG_LEVEL_VERBOSE, debug_level, "Circuit created without errors!");
   return DesignFlowStep_Status::SUCCESS;
}

void top_entity::add_input_register(structural_objectRef port_in, const std::string& port_prefix, structural_objectRef circuit, structural_objectRef clock_port, structural_objectRef reset_port, structural_objectRef e_port)
{
   const technology_managerRef TM = HLS->HLS_T->get_technology_manager();
   std::string register_library = TM->get_library(register_STD);
   structural_objectRef r_signal;
   structural_objectRef reg_mod = SM->add_module_from_technology_library(port_prefix + "_REG", register_STD, register_library, circuit, TM);
   GetPointer<module>(reg_mod)->get_in_port(2)->type_resize(GET_TYPE_SIZE(port_in));
   GetPointer<module>(reg_mod)->get_out_port(0)->type_resize(GET_TYPE_SIZE(port_in));

   structural_objectRef port_ck = reg_mod->find_member(CLOCK_PORT_NAME, port_o_K, reg_mod);
   SM->add_connection(clock_port, port_ck);

   structural_objectRef port_rs = reg_mod->find_member(RESET_PORT_NAME, port_o_K, reg_mod);
   SM->add_connection(reset_port, port_rs);

   r_signal = SM->add_sign(port_prefix + "_SIGI1", circuit, port_in->get_typeRef());
   SM->add_connection(e_port, r_signal);
   SM->add_connection(GetPointer<module>(reg_mod)->get_in_port(2), r_signal);

   r_signal = SM->add_sign(port_prefix + "_SIGI2", circuit, GetPointer<module>(reg_mod)->get_out_port(0)->get_typeRef());
   SM->add_connection(GetPointer<module>(reg_mod)->get_out_port(0), r_signal);
   SM->add_connection(port_in, r_signal);
}

void top_entity::add_ports(structural_objectRef circuit, structural_objectRef clock_port, structural_objectRef reset_port)
{
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Adding ports");
   const FunctionBehaviorConstRef FB = HLSMgr->CGetFunctionBehavior(funId);
   const BehavioralHelperConstRef BH = FB->CGetBehavioralHelper();
   bool has_registered_inputs = HLS->registered_inputs;

   const structural_objectRef& Datapath = HLS->datapath->get_circ();
   const std::list<unsigned int>& function_parameters = BH->get_parameters();
   const auto conn = HLS->Rconn;
   for(const auto function_parameter : function_parameters)
   {
      // generic_objRef input = conn.get_port(function_parameter, conn_binding::IN);
      // structural_objectRef in_obj = input->get_structural_obj();
      // structural_type_descriptorRef port_type = in_obj->get_typeRef();
      std::string prefix = "in_port_";
      structural_objectRef in_obj = Datapath->find_member(prefix + BH->PrintVariable(function_parameter), port_o_K, Datapath); // port get by name in order to do not use conn_binding
      THROW_ASSERT(in_obj, "in_obj is not a port");
      structural_type_descriptorRef port_type;
      if(HLSMgr->Rmem->has_base_address(function_parameter) && !HLSMgr->Rmem->has_parameter_base_address(function_parameter, HLS->functionId) && !HLSMgr->Rmem->is_parm_decl_stored(function_parameter))
      {
         port_type = structural_type_descriptorRef(new structural_type_descriptor("bool", 32));
      }
      else
         port_type = structural_type_descriptorRef(new structural_type_descriptor(function_parameter, BH));
      structural_objectRef top_obj;
      if(in_obj->get_kind() == port_vector_o_K)
      {
         THROW_ERROR("Should never be reached, in_obj is not a port vector");
         top_obj = SM->add_port_vector(FB->CGetBehavioralHelper()->PrintVariable(function_parameter), port_o::IN, GetPointer<port_o>(in_obj)->get_ports_size(), circuit, port_type);
      }
      else
         top_obj = SM->add_port(FB->CGetBehavioralHelper()->PrintVariable(function_parameter), port_o::IN, circuit, port_type);
      if(has_registered_inputs)
      {
         std::string port_prefix = GetPointer<port_o>(in_obj)->get_id();
         if(in_obj->get_kind() == port_vector_o_K)
         {
            for(unsigned int p = 0; p < GetPointer<port_o>(in_obj)->get_ports_size(); ++p)
            {
               add_input_register(GetPointer<port_o>(in_obj)->get_port(p), port_prefix + GetPointer<port_o>(in_obj)->get_port(p)->get_id(), circuit, clock_port, reset_port, GetPointer<port_o>(top_obj)->get_port(p));
            }
         }
         else
            add_input_register(in_obj, port_prefix, circuit, clock_port, reset_port, top_obj);
      }
      else
         SM->add_connection(in_obj, top_obj);
   }
   const unsigned int return_type_index = BH->GetFunctionReturnType(BH->get_function_index());
   if(return_type_index)
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Return type index is " + STR(return_type_index));
      structural_type_descriptorRef port_type;
      structural_objectRef ret_obj;
      if(conn)
      {
         generic_objRef return_port = conn->get_port(return_type_index, conn_binding::OUT);
         ret_obj = return_port->get_structural_obj();
         port_type = ret_obj->get_typeRef();
      }
      else
      {
         ret_obj = Datapath->find_member(RETURN_PORT_NAME, port_o_K, Datapath); // port get by name in order to do not use conn_binding
         THROW_ASSERT(ret_obj, "in_obj is not a port");
         if(HLSMgr->Rmem->has_base_address(return_type_index) && !HLSMgr->Rmem->has_parameter_base_address(return_type_index, HLS->functionId) && !HLSMgr->Rmem->is_parm_decl_stored(return_type_index))
         {
            port_type = structural_type_descriptorRef(new structural_type_descriptor("bool", 32));
         }
         else
            port_type = structural_type_descriptorRef(new structural_type_descriptor(return_type_index, BH));
      }
      structural_objectRef top_obj;
      if(ret_obj->get_kind() == port_vector_o_K)
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Adding vector return port");
         top_obj = SM->add_port_vector(RETURN_PORT_NAME, port_o::OUT, GetPointer<port_o>(ret_obj)->get_ports_size(), circuit, port_type);
      }
      else
      {
         THROW_ASSERT(ret_obj->get_kind() == port_o_K, ret_obj->get_kind_text());
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Adding return port");
         top_obj = SM->add_port(RETURN_PORT_NAME, port_o::OUT, circuit, port_type);
      }
      SM->add_connection(ret_obj, top_obj);
   }

   const auto top_functions = HLSMgr->CGetCallGraphManager()->GetRootFunctions();
   bool is_top = top_functions.find(BH->get_function_index()) != top_functions.end();
   bool master_port = true; // Datapath->find_member("M_DataRdy", port_o_K, Datapath);
   ////////////////////////////////////////////////////////////////////////////////////////
   ////////////////////////////////////////////////////////////////////////////////////////
   ////////////////////////////////////////////////////////////////////////////////////////
   std::map<unsigned int, structural_objectRef> null_values;
   /// creating extern IN port on top starting from extern ports on datapath and add connection
   for(unsigned int j = 0; j < GetPointer<module>(Datapath)->get_in_port_size(); j++)
   {
      structural_objectRef port_in = GetPointer<module>(Datapath)->get_in_port(j);
      if(GetPointer<port_o>(port_in)->get_is_extern())
      {
         if(!GetPointer<port_o>(port_in)->get_is_memory() || !is_top || master_port)
         {
            structural_objectRef ext_port;
            if(port_in->get_kind() == port_vector_o_K)
               ext_port = this->SM->add_port_vector(GetPointer<port_o>(port_in)->get_id(), port_o::IN, GetPointer<port_o>(port_in)->get_ports_size(), circuit, port_in->get_typeRef());
            else
               ext_port = this->SM->add_port(GetPointer<port_o>(port_in)->get_id(), port_o::IN, circuit, port_in->get_typeRef());
            port_o::fix_port_properties(port_in, ext_port);
            // adding connection between datapath extern port and top extern port
            this->SM->add_connection(port_in, ext_port);
         }
         else
         {
            if(null_values.find(GET_TYPE_SIZE(port_in)) == null_values.end())
            {
               structural_objectRef const_obj = SM->add_constant("null_value_" + STR(GET_TYPE_SIZE(port_in)), circuit, port_in->get_typeRef(), STR(0));
               null_values[GET_TYPE_SIZE(port_in)] = const_obj;
            }
            SM->add_connection(port_in, null_values[GET_TYPE_SIZE(port_in)]);
         }
      }
      else if(GetPointer<port_o>(port_in)->get_is_memory())
      {
         if(!is_top || master_port)
         {
            structural_objectRef ext_port;
            if(port_in->get_kind() == port_vector_o_K)
               ext_port = this->SM->add_port_vector(GetPointer<port_o>(port_in)->get_id(), port_o::IN, GetPointer<port_o>(port_in)->get_ports_size(), circuit, port_in->get_typeRef());
            else
               ext_port = this->SM->add_port(GetPointer<port_o>(port_in)->get_id(), port_o::IN, circuit, port_in->get_typeRef());
            port_o::fix_port_properties(port_in, ext_port);
            // adding connection between datapath extern port and top extern port
            this->SM->add_connection(port_in, ext_port);
         }
         else
         {
            if(null_values.find(GET_TYPE_SIZE(port_in)) == null_values.end())
            {
               structural_objectRef const_obj = SM->add_constant("null_value_" + STR(GET_TYPE_SIZE(port_in)), circuit, port_in->get_typeRef(), STR(0));
               null_values[GET_TYPE_SIZE(port_in)] = const_obj;
            }
            SM->add_connection(port_in, null_values[GET_TYPE_SIZE(port_in)]);
         }
      }
      else if(GetPointer<port_o>(port_in)->get_port_interface() != port_o::port_interface::PI_DEFAULT)
      {
         structural_objectRef ext_port;
         if(port_in->get_kind() == port_vector_o_K)
            ext_port = this->SM->add_port_vector(GetPointer<port_o>(port_in)->get_id(), port_o::IN, GetPointer<port_o>(port_in)->get_ports_size(), circuit, port_in->get_typeRef());
         else
            ext_port = this->SM->add_port(GetPointer<port_o>(port_in)->get_id(), port_o::IN, circuit, port_in->get_typeRef());
         port_o::fix_port_properties(port_in, ext_port);
         // adding connection between datapath interface port and top interface port
         this->SM->add_connection(port_in, ext_port);
      }
   }
   /// creating extern OUT port on top starting from extern ports on datapath and add connection
   for(unsigned int j = 0; j < GetPointer<module>(Datapath)->get_out_port_size(); j++)
   {
      structural_objectRef port_out = GetPointer<module>(Datapath)->get_out_port(j);
      if(GetPointer<port_o>(port_out)->get_is_memory() && is_top && !master_port)
         continue;
      if(GetPointer<port_o>(port_out)->get_is_extern())
      {
         structural_objectRef ext_port;
         if(port_out->get_kind() == port_vector_o_K)
            ext_port = this->SM->add_port_vector(GetPointer<port_o>(port_out)->get_id(), port_o::OUT, GetPointer<port_o>(port_out)->get_ports_size(), circuit, port_out->get_typeRef());
         else
            ext_port = this->SM->add_port(GetPointer<port_o>(port_out)->get_id(), port_o::OUT, circuit, port_out->get_typeRef());
         port_o::fix_port_properties(port_out, ext_port);
         // adding connection between datapath extern port and top extern port
         this->SM->add_connection(port_out, ext_port);
      }
      else if(GetPointer<port_o>(port_out)->get_is_memory())
      {
         structural_objectRef ext_port;
         if(port_out->get_kind() == port_vector_o_K)
            ext_port = this->SM->add_port_vector(GetPointer<port_o>(port_out)->get_id(), port_o::OUT, GetPointer<port_o>(port_out)->get_ports_size(), circuit, port_out->get_typeRef());
         else
            ext_port = this->SM->add_port(GetPointer<port_o>(port_out)->get_id(), port_o::OUT, circuit, port_out->get_typeRef());
         port_o::fix_port_properties(port_out, ext_port);
         // adding connection between datapath extern port and top extern port
         this->SM->add_connection(port_out, ext_port);
      }
      else if(GetPointer<port_o>(port_out)->get_port_interface() != port_o::port_interface::PI_DEFAULT)
      {
         structural_objectRef ext_port;
         if(port_out->get_kind() == port_vector_o_K)
            ext_port = this->SM->add_port_vector(GetPointer<port_o>(port_out)->get_id(), port_o::OUT, GetPointer<port_o>(port_out)->get_ports_size(), circuit, port_out->get_typeRef());
         else
            ext_port = this->SM->add_port(GetPointer<port_o>(port_out)->get_id(), port_o::OUT, circuit, port_out->get_typeRef());
         port_o::fix_port_properties(port_out, ext_port);
         // adding connection between datapath interface port and top interface port
         this->SM->add_connection(port_out, ext_port);
      }
   }

   /// creating extern IO port on top starting from extern ports on datapath and add connection
   for(unsigned int j = 0; j < GetPointer<module>(Datapath)->get_in_out_port_size(); j++)
   {
      structural_objectRef port_in_out = GetPointer<module>(Datapath)->get_in_out_port(j);
      if(GetPointer<port_o>(port_in_out)->get_is_extern())
      {
         structural_objectRef ext_port;
         if(port_in_out->get_kind() == port_vector_o_K)
            ext_port = this->SM->add_port_vector(GetPointer<port_o>(port_in_out)->get_id(), port_o::IO, GetPointer<port_o>(port_in_out)->get_ports_size(), circuit, port_in_out->get_typeRef());
         else
            ext_port = this->SM->add_port(GetPointer<port_o>(port_in_out)->get_id(), port_o::IO, circuit, port_in_out->get_typeRef());
         port_o::fix_port_properties(port_in_out, ext_port);
         // adding connection between datapath extern port and top extern port
         this->SM->add_connection(port_in_out, ext_port);
      }
   }
   ////////////////////////////////////////////////////////////////////////////////////////
   ////////////////////////////////////////////////////////////////////////////////////////
   ////////////////////////////////////////////////////////////////////////////////////////
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Added ports");
}

void top_entity::add_command_signals(structural_objectRef circuit)
{
   structural_objectRef Datapath = HLS->datapath->get_circ();
   structural_objectRef Controller = HLS->controller->get_circ();

   const auto& selectors = HLS->Rconn->GetSelectors();
   for(const auto& selector : selectors)
   {
      for(const auto& l : selector.second)
      {
         structural_objectRef datapath_obj = l.second->get_structural_obj();
         THROW_ASSERT(datapath_obj, "missing structural object associated with the selector " + l.second->get_string());
         std::string datapath_name = datapath_obj->get_id();
         structural_objectRef controller_obj = GetPointer<commandport_obj>(l.second)->get_controller_obj();
         /// it means that the operation has not to be executed
         if(!controller_obj)
            continue;
         std::string controller_name = controller_obj->get_id();
         structural_objectRef src = Controller->find_member(controller_name, port_o_K, Controller);
         THROW_ASSERT(src, "Missing select port in the controller");
         structural_objectRef tgt = Datapath->find_member(datapath_name, port_o_K, Datapath);
         THROW_ASSERT(tgt, "Missing select port in the datapath");
         structural_objectRef sign = SM->add_sign(datapath_name, circuit, src->get_typeRef());
         SM->add_connection(src, sign);
         SM->add_connection(sign, tgt);
      }
   }
}
