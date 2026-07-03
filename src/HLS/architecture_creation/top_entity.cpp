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
 * @file top_entity.cpp
 * @brief Implementation of the class creating the top entity.
 *
 * @author Christian Pilato <pilato@elet.polimi.it>
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 *
 */
#include "top_entity.hpp"

#include "BambuParameter.hpp"
#include "behavioral_helper.hpp"
#include "call_graph_manager.hpp"
#include "commandport_obj.hpp"
#include "conn_binding.hpp"
#include "copyrights_strings.hpp"
#include "custom_map.hpp"
#include "custom_set.hpp"
#include "dbgPrintHelper.hpp"
#include "exceptions.hpp"
#include "fu_binding.hpp"
#include "function_behavior.hpp"
#include "functions.hpp"
#include "generic_obj.hpp"
#include "hls.hpp"
#include "hls_device.hpp"
#include "hls_manager.hpp"
#include "memory.hpp"
#include "schedule.hpp"
#include "string_manipulation.hpp"
#include "structural_manager.hpp"
#include "structural_objects.hpp"
#include "technology_manager.hpp"
#include "technology_node.hpp"

#include <list>
#include <string>
#include <tuple>

top_entity::top_entity(const ParameterConstRef _parameters, const HLS_managerRef _HLSMgr, unsigned int _funId,
                       const DesignFlowManager& _design_flow_manager, const HLSFlowStep_Type _hls_flow_step_type)
    : HLSFunctionStep(_parameters, _HLSMgr, _funId, _design_flow_manager, _hls_flow_step_type)
{
   debug_level = parameters->get_class_debug_level(GET_CLASS(*this));
}

HLS_step::HLSRelationships
top_entity::ComputeHLSRelationships(const DesignFlowStep::RelationshipType relationship_type) const
{
   HLSRelationships ret;
   switch(relationship_type)
   {
      case(DEPENDENCE_RELATIONSHIP):
      {
         ret.insert(std::make_tuple(parameters->getOption<HLSFlowStep_Type>(OPT_datapath_architecture),
                                    HLSFlowStepSpecializationConstRef(), HLSFlowStep_Relationship::SAME_FUNCTION));
         if(HLSMgr->get_HLS(funId))
         {
            ret.insert(std::make_tuple(HLSFlowStep_Type::FSM_CONTROLLER_CREATOR, HLSFlowStepSpecializationConstRef(),
                                       HLSFlowStep_Relationship::SAME_FUNCTION));
         }
         break;
      }
      case(INVALIDATION_RELATIONSHIP):
      {
         break;
      }
      case(PRECEDENCE_RELATIONSHIP):
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
   const auto FB = HLSMgr->CGetFunctionBehavior(funId);
   const auto BH = FB->CGetBehavioralHelper();
   const auto function_name = BH->GetFunctionName();
   const bool is_top = HLSMgr->CGetCallGraphManager().GetRootFunctions().count(BH->get_function_index());
   const auto module_name = is_top ? TOP_FUNCTION_WRAPPER_PREFIX + function_name : function_name;

   /// Test on previous steps. They checks if datapath and controller have been created. If they didn't,
   /// top circuit cannot be created.
   THROW_ASSERT(HLS->datapath, "Datapath not created");
   THROW_ASSERT(HLS->controller, "Controller not created");

   // reference to hls top circuit
   HLS->top = structural_managerRef(new structural_manager(parameters));
   SM = HLS->top;
   const auto& Datapath = HLS->datapath;
   const auto& Controller = HLS->controller;

   /// top circuit creation
   PRINT_DBG_MEX(DEBUG_LEVEL_VERBOSE, debug_level, "Top circuit creation");

   /// main circuit type
   const structural_type_descriptorRef module_type(new structural_type_descriptor(module_name));
   /// setting top circuit component
   SM->set_top_info(module_name, module_type);
   const auto circuit = SM->get_circ();
   THROW_ASSERT(circuit, "Top circuit is missing");
   // Now the top circuit is created, just as an empty box. <circuit> is a reference to the structural object that
   // will contain all the circuit components

   circuit->set_black_box(false);

   /// Set some descriptions and legal stuff
   GetPointerS<module_o>(circuit)->set_description("Top component for " + function_name);
   GetPointerS<module_o>(circuit)->set_copyright(GENERATED_COPYRIGHT);
   GetPointerS<module_o>(circuit)->set_authors("Component automatically generated by bambu");
   GetPointerS<module_o>(circuit)->set_license(GENERATED_LICENSE);

   const auto datapath_circuit = Datapath->get_circ();
   THROW_ASSERT(datapath_circuit, "Missing datapath circuit");
   const auto controller_circuit = Controller->get_circ();
   THROW_ASSERT(controller_circuit, "Missing controller circuit");

   PRINT_DBG_MEX(DEBUG_LEVEL_VERBOSE, debug_level, "Creating datapath object");
   /// creating structural_manager
   PRINT_DBG_MEX(DEBUG_LEVEL_VERBOSE, debug_level, "Adding datapath");
   datapath_circuit->set_owner(circuit);
   GetPointerS<module_o>(circuit)->add_internal_object(datapath_circuit);

   PRINT_DBG_MEX(DEBUG_LEVEL_VERBOSE, debug_level, "Creating controller object");
   /// creating structural_manager
   PRINT_DBG_MEX(DEBUG_LEVEL_VERBOSE, debug_level, "Adding controller");
   controller_circuit->set_owner(circuit);
   GetPointerS<module_o>(circuit)->add_internal_object(controller_circuit);

   /// command signal type descriptor
   const structural_type_descriptorRef bool_type(new structural_type_descriptor("bool", 0));

   PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "\tStart adding clock signal...");
   /// add clock port
   const auto clock_obj = SM->add_port(CLOCK_PORT_NAME, port_o::IN, circuit, bool_type);
   GetPointerS<port_o>(clock_obj)->set_is_clock(true);
   /// connect to datapath and controller clock
   const auto datapath_clock = datapath_circuit->find_member(CLOCK_PORT_NAME, port_o_K, datapath_circuit);
   SM->add_connection(datapath_clock, clock_obj);
   const auto controller_clock = controller_circuit->find_member(CLOCK_PORT_NAME, port_o_K, controller_circuit);
   SM->add_connection(controller_clock, clock_obj);
   PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "\tClock signal added!");

   PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "\tAdding reset signal...");
   /// add reset port
   const auto reset_obj = SM->add_port(RESET_PORT_NAME, port_o::IN, circuit, bool_type);
   /// connecting global reset port to the datapath one
   const auto datapath_reset = datapath_circuit->find_member(RESET_PORT_NAME, port_o_K, datapath_circuit);
   SM->add_connection(datapath_reset, reset_obj);
   /// connecting global reset port to the controller one
   const auto controller_reset = controller_circuit->find_member(RESET_PORT_NAME, port_o_K, controller_circuit);
   SM->add_connection(controller_reset, reset_obj);
   PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "\tReset signal added!");

   PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "\tAdding start signal...");
   /// start port
   const auto start_obj = SM->add_port(START_PORT_NAME, port_o::IN, circuit, bool_type);
   const auto controller_start = controller_circuit->find_member(START_PORT_NAME, port_o_K, controller_circuit);
   /// check if datapath has a start signal
   const auto datapath_start = datapath_circuit->find_member(START_PORT_NAME, port_o_K, datapath_circuit);
   structural_objectRef sync_datapath_controller;
   if(datapath_start)
   {
      SM->add_connection(start_obj, datapath_start);
      sync_datapath_controller = SM->add_sign("done2start", circuit, bool_type);
      structural_objectRef datapath_done = datapath_circuit->find_member(DONE_PORT_NAME, port_o_K, datapath_circuit);
      SM->add_connection(datapath_done, sync_datapath_controller);
   }
   else
   {
      SM->add_connection(start_obj, controller_start);
   }
   PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "\tStart signal added!");

   PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "\tStart adding Done signal...");
   /// add done port
   const auto done_obj = SM->add_port(DONE_PORT_NAME, port_o::OUT, circuit, bool_type);
   THROW_ASSERT(done_obj, "Done port not added in the top component");
   const auto controller_done = controller_circuit->find_member(DONE_PORT_NAME, port_o_K, controller_circuit);
   THROW_ASSERT(controller_done, "Done signal not found in the controller");
   if(datapath_start)
   {
      SM->add_connection(sync_datapath_controller, controller_start);
   }
   structural_objectRef done_signal_out;
   if(HLS->registered_done_port)
   {
      const auto TM = HLS->HLS_D->get_technology_manager();
      const auto reset_type = parameters->getOption<std::string>(OPT_reset_type);
      const auto delay_unit = reset_type == "sync" ? register_SR : register_AR;
      const auto delay_gate =
          SM->add_module_from_technology_library("done_delayed_REG", delay_unit, LIBRARY_STD, circuit, TM);
      const auto port_ck = delay_gate->find_member(CLOCK_PORT_NAME, port_o_K, delay_gate);
      if(port_ck)
      {
         SM->add_connection(clock_obj, port_ck);
      }
      const auto port_rst = delay_gate->find_member(RESET_PORT_NAME, port_o_K, delay_gate);
      if(port_rst)
      {
         SM->add_connection(reset_obj, port_rst);
      }

      const auto delay_gate_m = GetPointerS<module_o>(delay_gate);
      const auto in1 = delay_gate_m->get_in_port(2);
      const auto out1 = delay_gate_m->get_out_port(0);
      const auto done_signal_in = SM->add_sign("done_delayed_REG_signal_in", circuit, controller_done->get_typeRef());
      SM->add_connection(in1, done_signal_in);
      SM->add_connection(controller_done, done_signal_in);
      done_signal_out = SM->add_sign("done_delayed_REG_signal_out", circuit, controller_done->get_typeRef());
      SM->add_connection(out1, done_signal_out);
      SM->add_connection(done_obj, done_signal_out);
   }
   else
   {
      SM->add_connection(controller_done, done_obj);
   }
   PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "\tDone signal added!");

   /// propagate idle_port only for top functions
   if(is_top)
   {
      PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "\tStart adding Idle signal...");
      const auto idle_obj = SM->add_port(IDLE_PORT_NAME, port_o::OUT, circuit, bool_type);
      THROW_ASSERT(idle_obj, "Idle port not added in the top component");
      const auto controller_idle = controller_circuit->find_member(IDLE_PORT_NAME, port_o_K, controller_circuit);
      THROW_ASSERT(controller_idle, "Idle signal not found in the controller");
      SM->add_connection(controller_idle, idle_obj);
      PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "\tIdle signal added!");
   }

   /// add entry in in_port_map between port id and port index

   PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "\tAdding input/output ports...");
   add_ports(circuit, clock_obj, reset_obj);
   PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "\tInput/output ports added!");

   PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "\tAdding command ports...");
   add_command_signals(circuit);
   PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "\tCommand ports added!");

   if(!is_top || (parameters->isOption(OPT_expose_globals) && parameters->getOption<bool>(OPT_expose_globals)) ||
      (parameters->isOption(OPT_memory_mapped_top) && parameters->getOption<bool>(OPT_memory_mapped_top)))
   {
      memory::propagate_memory_parameters(HLS->datapath->get_circ(), HLS->top);
   }

   for(const auto& p : datapath_circuit->GetParameters())
   {
      const auto name = p.first;
      const auto value = p.second;
      if(name == "CONTEXT_BIT_START" || name == "CONTEXT_BIT_END")
      {
         circuit->AddParameter(name, "0");
         circuit->SetParameter(name, value);
         datapath_circuit->SetParameter(name, name);
      }
   }

   PRINT_DBG_MEX(DEBUG_LEVEL_VERBOSE, debug_level, "Circuit created without errors!");
   return DesignFlowStep_Status::SUCCESS;
}

void top_entity::add_input_register(structural_objectRef port_in, const std::string& port_prefix,
                                    structural_objectRef circuit, structural_objectRef clock_port,
                                    structural_objectRef reset_port, structural_objectRef e_port)
{
   const auto TM = HLS->HLS_D->get_technology_manager();
   const auto register_library = TM->get_library(register_STD);
   structural_objectRef r_signal;
   const auto reg_mod =
       SM->add_module_from_technology_library(port_prefix + "_REG", register_STD, register_library, circuit, TM);
   GetPointerS<module_o>(reg_mod)->get_in_port(2)->type_resize(GET_TYPE_SIZE(port_in));
   GetPointerS<module_o>(reg_mod)->get_out_port(0)->type_resize(GET_TYPE_SIZE(port_in));

   const auto port_ck = reg_mod->find_member(CLOCK_PORT_NAME, port_o_K, reg_mod);
   SM->add_connection(clock_port, port_ck);

   const auto port_rs = reg_mod->find_member(RESET_PORT_NAME, port_o_K, reg_mod);
   SM->add_connection(reset_port, port_rs);

   r_signal = SM->add_sign(port_prefix + "_SIGI1", circuit, port_in->get_typeRef());
   SM->add_connection(e_port, r_signal);
   SM->add_connection(GetPointerS<module_o>(reg_mod)->get_in_port(2), r_signal);

   r_signal =
       SM->add_sign(port_prefix + "_SIGI2", circuit, GetPointerS<module_o>(reg_mod)->get_out_port(0)->get_typeRef());
   SM->add_connection(GetPointerS<module_o>(reg_mod)->get_out_port(0), r_signal);
   SM->add_connection(port_in, r_signal);
}

void top_entity::add_ports(structural_objectRef circuit, structural_objectRef clock_port,
                           structural_objectRef reset_port)
{
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Adding ports");
   const auto FB = HLSMgr->CGetFunctionBehavior(funId);
   const auto BH = FB->CGetBehavioralHelper();
   bool has_registered_inputs = HLS->registered_inputs;

   const auto Datapath = HLS->datapath->get_circ();
   const auto function_parameters = BH->get_parameters();
   const auto conn = HLS->Rconn;
   const auto curr_address_bitsize = HLSMgr->get_address_bitsize();
   for(const auto& function_parameter : function_parameters)
   {
      const auto prefix = "in_port_";
      const auto in_obj = Datapath->find_member(prefix + BH->PrintVariable(function_parameter), port_o_K,
                                                Datapath); // port get by name in order to do not use conn_binding
      THROW_ASSERT(in_obj, "in_obj is not a port");
      structural_type_descriptorRef port_type;
      if(HLSMgr->Rmem->has_base_address(function_parameter) &&
         !HLSMgr->Rmem->has_parameter_base_address(function_parameter, HLS->functionId) &&
         !HLSMgr->Rmem->is_parm_decl_stored(function_parameter))
      {
         port_type = structural_type_descriptorRef(new structural_type_descriptor("bool", curr_address_bitsize));
      }
      else
      {
         port_type = structural_type_descriptorRef(new structural_type_descriptor(function_parameter, BH));
      }
      structural_objectRef top_obj;
      if(in_obj->get_kind() == port_vector_o_K)
      {
         THROW_ERROR("Should never be reached, in_obj is not a port vector");
         top_obj = SM->add_port_vector(FB->CGetBehavioralHelper()->PrintVariable(function_parameter), port_o::IN,
                                       GetPointerS<port_o>(in_obj)->get_ports_size(), circuit, port_type);
      }
      else
      {
         top_obj = SM->add_port(FB->CGetBehavioralHelper()->PrintVariable(function_parameter), port_o::IN, circuit,
                                port_type);
      }
      if(has_registered_inputs)
      {
         const auto port_prefix = GetPointerS<port_o>(in_obj)->get_id();
         if(in_obj->get_kind() == port_vector_o_K)
         {
            for(auto p = 0U; p < GetPointerS<port_o>(in_obj)->get_ports_size(); ++p)
            {
               add_input_register(GetPointerS<port_o>(in_obj)->get_port(p),
                                  port_prefix + GetPointerS<port_o>(in_obj)->get_port(p)->get_id(), circuit, clock_port,
                                  reset_port, GetPointerS<port_o>(top_obj)->get_port(p));
            }
         }
         else
         {
            add_input_register(in_obj, port_prefix, circuit, clock_port, reset_port, top_obj);
         }
      }
      else
      {
         SM->add_connection(in_obj, top_obj);
      }
   }
   const auto return_type_index = BH->GetFunctionReturnType();
   if(return_type_index)
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Return type index is " + STR(return_type_index));
      structural_type_descriptorRef port_type;
      structural_objectRef ret_obj;
      if(conn)
      {
         const auto return_port = conn->get_port(return_type_index, conn_binding::OUT);
         ret_obj = return_port->get_structural_obj();
         port_type = ret_obj->get_typeRef();
      }
      else
      {
         ret_obj = Datapath->find_member(RETURN_PORT_NAME, port_o_K,
                                         Datapath); // port get by name in order to do not use conn_binding
         THROW_ASSERT(ret_obj, "in_obj is not a port");
         if(HLSMgr->Rmem->has_base_address(return_type_index) &&
            !HLSMgr->Rmem->has_parameter_base_address(return_type_index, HLS->functionId) &&
            !HLSMgr->Rmem->is_parm_decl_stored(return_type_index))
         {
            port_type = structural_type_descriptorRef(new structural_type_descriptor("bool", curr_address_bitsize));
         }
         else
         {
            port_type = structural_type_descriptorRef(new structural_type_descriptor(return_type_index, BH));
         }
      }
      structural_objectRef top_obj;
      if(ret_obj->get_kind() == port_vector_o_K)
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Adding vector return port");
         top_obj = SM->add_port_vector(RETURN_PORT_NAME, port_o::OUT, GetPointer<port_o>(ret_obj)->get_ports_size(),
                                       circuit, port_type);
      }
      else
      {
         THROW_ASSERT(ret_obj->get_kind() == port_o_K, ret_obj->get_kind_text());
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Adding return port");
         top_obj = SM->add_port(RETURN_PORT_NAME, port_o::OUT, circuit, port_type);
      }
      SM->add_connection(ret_obj, top_obj);
   }

   const bool is_top = HLSMgr->CGetCallGraphManager().GetRootFunctions().count(BH->get_function_index());
   bool master_port = true; // Datapath->find_member("M_DataRdy", port_o_K, Datapath);
   ////////////////////////////////////////////////////////////////////////////////////////
   ////////////////////////////////////////////////////////////////////////////////////////
   ////////////////////////////////////////////////////////////////////////////////////////
   std::map<unsigned long long, structural_objectRef> null_values;
   /// creating extern IN port on top starting from extern ports on datapath and add connection
   for(unsigned int j = 0; j < GetPointerS<module_o>(Datapath)->get_in_port_size(); j++)
   {
      structural_objectRef port_in = GetPointerS<module_o>(Datapath)->get_in_port(j);
      if(GetPointerS<port_o>(port_in)->get_is_extern())
      {
         if(!GetPointer<port_o>(port_in)->get_is_memory() || !is_top || master_port)
         {
            structural_objectRef ext_port;
            if(port_in->get_kind() == port_vector_o_K)
            {
               ext_port =
                   SM->add_port_vector(GetPointerS<port_o>(port_in)->get_id(), port_o::IN,
                                       GetPointerS<port_o>(port_in)->get_ports_size(), circuit, port_in->get_typeRef());
            }
            else
            {
               ext_port =
                   SM->add_port(GetPointerS<port_o>(port_in)->get_id(), port_o::IN, circuit, port_in->get_typeRef());
            }
            port_o::fix_port_properties(port_in, ext_port);
            // adding connection between datapath extern port and top extern port
            SM->add_connection(port_in, ext_port);
         }
         else
         {
            if(null_values.find(GET_TYPE_SIZE(port_in)) == null_values.end())
            {
               const auto const_obj = SM->add_constant("null_value_" + STR(GET_TYPE_SIZE(port_in)), circuit,
                                                       port_in->get_typeRef(), STR(0));
               null_values[GET_TYPE_SIZE(port_in)] = const_obj;
            }
            SM->add_connection(port_in, null_values[GET_TYPE_SIZE(port_in)]);
         }
      }
      else if(GetPointerS<port_o>(port_in)->get_is_memory())
      {
         if(!is_top || master_port)
         {
            structural_objectRef ext_port;
            if(port_in->get_kind() == port_vector_o_K)
            {
               ext_port =
                   SM->add_port_vector(GetPointerS<port_o>(port_in)->get_id(), port_o::IN,
                                       GetPointerS<port_o>(port_in)->get_ports_size(), circuit, port_in->get_typeRef());
            }
            else
            {
               ext_port =
                   SM->add_port(GetPointerS<port_o>(port_in)->get_id(), port_o::IN, circuit, port_in->get_typeRef());
            }
            port_o::fix_port_properties(port_in, ext_port);
            // adding connection between datapath extern port and top extern port
            SM->add_connection(port_in, ext_port);
         }
         else
         {
            if(null_values.find(GET_TYPE_SIZE(port_in)) == null_values.end())
            {
               structural_objectRef const_obj = SM->add_constant("null_value_" + STR(GET_TYPE_SIZE(port_in)), circuit,
                                                                 port_in->get_typeRef(), STR(0));
               null_values[GET_TYPE_SIZE(port_in)] = const_obj;
            }
            SM->add_connection(port_in, null_values[GET_TYPE_SIZE(port_in)]);
         }
      }
      else if(GetPointerS<port_o>(port_in)->get_port_interface() != port_o::port_interface::PI_DEFAULT)
      {
         structural_objectRef ext_port;
         if(port_in->get_kind() == port_vector_o_K)
         {
            ext_port =
                SM->add_port_vector(GetPointerS<port_o>(port_in)->get_id(), port_o::IN,
                                    GetPointerS<port_o>(port_in)->get_ports_size(), circuit, port_in->get_typeRef());
         }
         else
         {
            ext_port =
                SM->add_port(GetPointerS<port_o>(port_in)->get_id(), port_o::IN, circuit, port_in->get_typeRef());
         }
         port_o::fix_port_properties(port_in, ext_port);
         // adding connection between datapath interface port and top interface port
         SM->add_connection(port_in, ext_port);
      }
   }
   /// creating extern OUT port on top starting from extern ports on datapath and add connection
   for(unsigned int j = 0; j < GetPointer<module_o>(Datapath)->get_out_port_size(); j++)
   {
      structural_objectRef port_out = GetPointer<module_o>(Datapath)->get_out_port(j);
      if(GetPointer<port_o>(port_out)->get_is_memory() && is_top && !master_port)
      {
         continue;
      }
      if(GetPointer<port_o>(port_out)->get_is_extern())
      {
         structural_objectRef ext_port;
         if(port_out->get_kind() == port_vector_o_K)
         {
            ext_port =
                SM->add_port_vector(GetPointer<port_o>(port_out)->get_id(), port_o::OUT,
                                    GetPointer<port_o>(port_out)->get_ports_size(), circuit, port_out->get_typeRef());
         }
         else
         {
            ext_port =
                SM->add_port(GetPointer<port_o>(port_out)->get_id(), port_o::OUT, circuit, port_out->get_typeRef());
         }
         port_o::fix_port_properties(port_out, ext_port);
         // adding connection between datapath extern port and top extern port
         SM->add_connection(port_out, ext_port);
      }
      else if(GetPointer<port_o>(port_out)->get_is_memory())
      {
         structural_objectRef ext_port;
         if(port_out->get_kind() == port_vector_o_K)
         {
            ext_port =
                SM->add_port_vector(GetPointer<port_o>(port_out)->get_id(), port_o::OUT,
                                    GetPointer<port_o>(port_out)->get_ports_size(), circuit, port_out->get_typeRef());
         }
         else
         {
            ext_port =
                SM->add_port(GetPointer<port_o>(port_out)->get_id(), port_o::OUT, circuit, port_out->get_typeRef());
         }
         port_o::fix_port_properties(port_out, ext_port);
         // adding connection between datapath extern port and top extern port
         SM->add_connection(port_out, ext_port);
      }
      else if(GetPointer<port_o>(port_out)->get_port_interface() != port_o::port_interface::PI_DEFAULT)
      {
         structural_objectRef ext_port;
         if(port_out->get_kind() == port_vector_o_K)
         {
            ext_port =
                SM->add_port_vector(GetPointer<port_o>(port_out)->get_id(), port_o::OUT,
                                    GetPointer<port_o>(port_out)->get_ports_size(), circuit, port_out->get_typeRef());
         }
         else
         {
            ext_port =
                SM->add_port(GetPointer<port_o>(port_out)->get_id(), port_o::OUT, circuit, port_out->get_typeRef());
         }
         port_o::fix_port_properties(port_out, ext_port);
         // adding connection between datapath interface port and top interface port
         SM->add_connection(port_out, ext_port);
      }
   }

   /// creating extern IO port on top starting from extern ports on datapath and add connection
   for(unsigned int j = 0; j < GetPointer<module_o>(Datapath)->get_in_out_port_size(); j++)
   {
      structural_objectRef port_in_out = GetPointer<module_o>(Datapath)->get_in_out_port(j);
      if(GetPointer<port_o>(port_in_out)->get_is_extern())
      {
         structural_objectRef ext_port;
         if(port_in_out->get_kind() == port_vector_o_K)
         {
            ext_port = SM->add_port_vector(GetPointer<port_o>(port_in_out)->get_id(), port_o::IO,
                                           GetPointer<port_o>(port_in_out)->get_ports_size(), circuit,
                                           port_in_out->get_typeRef());
         }
         else
         {
            ext_port = SM->add_port(GetPointer<port_o>(port_in_out)->get_id(), port_o::IO, circuit,
                                    port_in_out->get_typeRef());
         }
         port_o::fix_port_properties(port_in_out, ext_port);
         // adding connection between datapath extern port and top extern port
         SM->add_connection(port_in_out, ext_port);
      }
   }
   ////////////////////////////////////////////////////////////////////////////////////////
   ////////////////////////////////////////////////////////////////////////////////////////
   ////////////////////////////////////////////////////////////////////////////////////////
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Added ports");
}

void top_entity::add_command_signals(structural_objectRef circuit)
{
   const auto Datapath = HLS->datapath->get_circ();
   const auto Controller = HLS->controller->get_circ();

   for(const auto& selector : HLS->Rconn->GetSelectors())
   {
      for(const auto& l : selector.second)
      {
         structural_objectRef datapath_obj = l.second->get_structural_obj();
         THROW_ASSERT(datapath_obj, "missing structural object associated with the selector " + l.second->get_string());
         std::string datapath_name = datapath_obj->get_id();
         structural_objectRef controller_obj = GetPointer<commandport_obj>(l.second)->get_controller_obj();
         /// it means that the operation has not to be executed
         if(!controller_obj)
         {
            continue;
         }
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

   const auto omp_info = HLSMgr->CGetFunctionBehavior(funId)->GetOMPInfo();
   if(omp_info && omp_info->context_count > 1U)
   {
      const auto datapath_selector = Datapath->find_member(SELECTOR_REGISTER_FILE, port_o_K, Datapath);
      const auto controller_selector = Controller->find_member(SELECTOR_REGISTER_FILE, port_o_K, Controller);
      THROW_ASSERT(datapath_selector, "Datapath selector signal not found.");
      THROW_ASSERT(controller_selector, "Controller selector signal not found.");
      const structural_type_descriptorRef selector_type(new structural_type_descriptor);
      datapath_selector->get_typeRef()->copy(selector_type);
      const auto selector_port = SM->add_port(SELECTOR_REGISTER_FILE, port_o::IN, circuit, selector_type);
      SM->add_connection(selector_port, datapath_selector);
      SM->add_connection(selector_port, controller_selector);
   }
}
