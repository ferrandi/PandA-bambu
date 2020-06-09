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
 *              Copyright (c) 2016-2020 Politecnico di Milano
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
 * @file top_entity_cs.cpp
 * @brief Base class for top entity for context_switch.
 *
 * @author Nicola Saporetti <nicola.saporetti@gmail.com>
 *
 */
/// Header include
#include "top_entity_cs.hpp"

///.include
#include "BambuParameter.hpp"

/// circuit include
#include "structural_manager.hpp"
#include "structural_objects.hpp"

/// HLS includes
#include "hls.hpp"
#include "hls_manager.hpp"
#include "hls_target.hpp"

/// HLS/functions_allocation
#include "omp_functions.hpp"

/// STD include
#include <string>

/// technology include
#include "technology_manager.hpp"

/// utility includes
#include "dbgPrintHelper.hpp"
#include "math_function.hpp"
#include "utility.hpp"

top_entity_cs::top_entity_cs(const ParameterConstRef _parameters, const HLS_managerRef _HLSMgr, unsigned int _funId, const DesignFlowManagerConstRef _design_flow_manager, const HLSFlowStep_Type _hls_flow_step_type)
    : top_entity(_parameters, _HLSMgr, _funId, _design_flow_manager, _hls_flow_step_type)
{
   debug_level = parameters->get_class_debug_level(GET_CLASS(*this));
}

top_entity_cs::~top_entity_cs()
{
}

DesignFlowStep_Status top_entity_cs::InternalExec()
{
   auto omp_functions = GetPointer<OmpFunctions>(HLSMgr->Rfuns);
   /// Register input because of context switch
   if(omp_functions->kernel_functions.find(funId) != omp_functions->kernel_functions.end())
   {
      HLS->registered_inputs = true;
   }
   top_entity::InternalExec();

   if(omp_functions->kernel_functions.find(funId) != omp_functions->kernel_functions.end())
   {
      add_context_switch_port_kernel();
   }
   else
   {
      bool found = false;
      if(omp_functions->parallelized_functions.find(funId) != omp_functions->parallelized_functions.end())
         found = true;
      if(omp_functions->atomic_functions.find(funId) != omp_functions->atomic_functions.end())
         found = true;
      if(found) // function with selector
      {
         add_context_switch_port();
      }
   }
   return DesignFlowStep_Status::SUCCESS;
}

void top_entity_cs::add_context_switch_port()
{
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Adding ports");
   structural_objectRef circuit = SM->get_circ();
   structural_managerRef Datapath = HLS->datapath;
   structural_managerRef Controller = HLS->controller;
   structural_objectRef datapath_circuit = Datapath->get_circ();
   structural_objectRef controller_circuit = Controller->get_circ();
   structural_type_descriptorRef bool_type = structural_type_descriptorRef(new structural_type_descriptor("bool", 0));
   structural_objectRef suspension_obj = SM->add_port(STR(SUSPENSION), port_o::OUT, circuit, bool_type);
   structural_objectRef datapath_suspension = datapath_circuit->find_member(STR(SUSPENSION), port_o_K, datapath_circuit);
   SM->add_connection(datapath_suspension, suspension_obj);

   int num_slots = ceil_log2(parameters->getOption<unsigned long long int>(OPT_context_switch));
   if(!num_slots)
      num_slots = 1;
   structural_type_descriptorRef port_type = structural_type_descriptorRef(new structural_type_descriptor("bool", static_cast<unsigned>(num_slots)));
   structural_objectRef selector_obj = SM->add_port(STR(SELECTOR_REGISTER_FILE), port_o::IN, circuit, port_type);
   structural_objectRef datapath_selector = datapath_circuit->find_member(STR(SELECTOR_REGISTER_FILE), port_o_K, datapath_circuit);
   SM->add_connection(datapath_selector, selector_obj);
   structural_objectRef controller_selector = controller_circuit->find_member(STR(SELECTOR_REGISTER_FILE), port_o_K, controller_circuit);
   SM->add_connection(controller_selector, selector_obj);
   auto selector_regFile_sign = circuit->find_member(STR(SELECTOR_REGISTER_FILE) + "_signal", signal_o_K, circuit);
   if(selector_regFile_sign)
   {
      SM->add_connection(selector_obj, selector_regFile_sign);
   }
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Added ports");
}

void top_entity_cs::add_context_switch_port_kernel()
{
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Adding ports to kernel module");
   structural_managerRef Datapath = HLS->datapath;
   structural_managerRef Controller = HLS->controller;
   structural_objectRef datapath_circuit = Datapath->get_circ();
   structural_objectRef controller_circuit = Controller->get_circ();
   structural_objectRef circuit = SM->get_circ();
   structural_type_descriptorRef bool_type = structural_type_descriptorRef(new structural_type_descriptor("bool", 0));
   int num_slots = ceil_log2(parameters->getOption<unsigned long long int>(OPT_context_switch));
   if(!num_slots)
      num_slots = 1;

   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Adding selector register file connection");
   structural_type_descriptorRef port_type = structural_type_descriptorRef(new structural_type_descriptor("bool", static_cast<unsigned>(num_slots)));
   structural_objectRef datapath_selector = datapath_circuit->find_member(STR(SELECTOR_REGISTER_FILE), port_o_K, datapath_circuit);
   auto selector_regFile_sign = circuit->find_member(STR(SELECTOR_REGISTER_FILE) + "_signal", signal_o_K, circuit);
   if(not selector_regFile_sign)
   {
      selector_regFile_sign = SM->add_sign(STR(SELECTOR_REGISTER_FILE) + "_signal", circuit, port_type);
   }
   SM->add_connection(datapath_selector, selector_regFile_sign);
   structural_objectRef controller_selector = controller_circuit->find_member(STR(SELECTOR_REGISTER_FILE), port_o_K, controller_circuit);
   SM->add_connection(selector_regFile_sign, controller_selector);

   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Adding " + STR(TASKS_POOL_END));
   structural_objectRef task_pool_end_obj = SM->add_port(STR(TASKS_POOL_END), port_o::IN, circuit, bool_type);
   structural_objectRef datapath_task_pool_end = datapath_circuit->find_member(STR(TASKS_POOL_END), port_o_K, datapath_circuit);
   SM->add_connection(datapath_task_pool_end, task_pool_end_obj);

   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Adding " + STR(DONE_REQUEST));
   structural_objectRef done_request_obj = SM->add_port(STR(DONE_REQUEST), port_o::OUT, circuit, bool_type);
   structural_objectRef datapath_done_request = datapath_circuit->find_member(STR(DONE_REQUEST), port_o_K, datapath_circuit);
   SM->add_connection(datapath_done_request, done_request_obj);

   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Adding " + STR(DONE_SCHEDULER));
   structural_objectRef datapath_done_port = datapath_circuit->find_member(STR(DONE_SCHEDULER), port_o_K, datapath_circuit);
   structural_objectRef done_signal_in = circuit->find_member("done_delayed_REG_signal_in", signal_o_K, circuit);
   SM->add_connection(done_signal_in, datapath_done_port); // connect signal out controller to datapath START_PORT_NAME

   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Adding " + STR(START_PORT_NAME));
   structural_objectRef datapath_start_port = datapath_circuit->find_member(STR(START_PORT_NAME) + "_task", port_o_K, datapath_circuit);
   structural_objectRef start_signal_in = circuit->find_member(STR(START_PORT_NAME), port_o_K, circuit);
   SM->add_connection(start_signal_in, datapath_start_port); // connect start to datapath
   circuit->AddParameter("KERN_NUM", "0");

   GetPointer<module>(datapath_circuit)->SetParameter("KERN_NUM", "KERN_NUM");
   SM->add_NP_functionality(circuit, NP_functionality::LIBRARY, "KERN_NUM");
   GetPointer<module>(circuit)->AddParameter("KERN_NUM", "0"); // taken from kernel instantiation
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Added ports to kernel module");
}

void top_entity_cs::add_input_register(structural_objectRef port_in, const std::string& port_prefix, structural_objectRef circuit, structural_objectRef clock_port, structural_objectRef, structural_objectRef e_port)
{
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Adding input registers");
   auto TM = HLS->HLS_T->get_technology_manager();
   auto register_library = TM->get_library("register_file");
   auto register_file_module = SM->add_module_from_technology_library(port_prefix + "_REG", "register_file", register_library, circuit, TM);
   unsigned int cs_number = HLS->Param->getOption<unsigned int>(OPT_context_switch);
   GetPointer<module>(register_file_module)->SetParameter("n_elements", STR(cs_number));
   /// Resizing input port
   GetPointer<module>(register_file_module)->get_in_port(1)->type_resize(GET_TYPE_SIZE(port_in));

   /// Resizing output port
   GetPointer<module>(register_file_module)->get_out_port(0)->type_resize(GET_TYPE_SIZE(port_in));

   /// Add clock
   auto rf_clock_port = register_file_module->find_member(CLOCK_PORT_NAME, port_o_K, register_file_module);
   SM->add_connection(clock_port, rf_clock_port);

   /// Connect write port to start port
   auto start_port = circuit->find_member(START_PORT_NAME, port_o_K, circuit);
   auto rf_we_port = GetPointer<module>(register_file_module)->get_in_port(2);
   SM->add_connection(start_port, rf_we_port);

   /// Connect selector of register file
   auto controller_circuit = HLS->controller->get_circ();
   auto register_file_selector_port = controller_circuit->find_member(SELECTOR_REGISTER_FILE, port_o_K, controller_circuit);
   auto rf_register_file_selector_port = GetPointer<module>(register_file_module)->get_in_port(3);
   int sel_bits = ceil_log2(cs_number);
   rf_register_file_selector_port->type_resize(static_cast<unsigned>(sel_bits));

   auto register_file_selector_signal = circuit->find_member(SELECTOR_REGISTER_FILE "_signal", signal_o_K, circuit);
   if(not register_file_selector_signal)
   {
      register_file_selector_signal = SM->add_sign(STR(SELECTOR_REGISTER_FILE) + "_signal", circuit, register_file_selector_port->get_typeRef());
   }
   SM->add_connection(register_file_selector_signal, rf_register_file_selector_port);

   /// Add signal from external port
   auto external_port_to_register = SM->add_sign(port_prefix + "_to_reg", circuit, port_in->get_typeRef());
   SM->add_connection(e_port, external_port_to_register);
   SM->add_connection(GetPointer<module>(register_file_module)->get_in_port(1), external_port_to_register);

   /// Add signal to datapath
   auto register_to_internal_port = SM->add_sign(port_prefix + "_from_reg", circuit, GetPointer<module>(register_file_module)->get_out_port(0)->get_typeRef());
   SM->add_connection(GetPointer<module>(register_file_module)->get_out_port(0), register_to_internal_port);
   SM->add_connection(port_in, register_to_internal_port);
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Added input registers");
}
