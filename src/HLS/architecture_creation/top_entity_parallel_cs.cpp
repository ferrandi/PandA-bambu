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
 * @file classic_datapath.hpp
 * @brief Base class for top entity for context_switch.
 *
 * @author Nicola Saporetti <nicola.saporetti@gmail.com>
 *
 */
#include "top_entity_parallel_cs.hpp"
#include "BambuParameter.hpp"
#include "behavioral_helper.hpp"
#include "copyrights_strings.hpp"
#include "hls.hpp"
#include "hls_manager.hpp"
#include "hls_target.hpp"
#include "loop.hpp"
#include "loops.hpp"
#include "memory.hpp"
#include "structural_manager.hpp"
#include "structural_objects.hpp"
#include "technology_manager.hpp"
#include "technology_node.hpp"
#include "tree_helper.hpp"
#include "tree_node.hpp"

/// behavior include
#include "call_graph_manager.hpp"

/// STD includes
#include <cmath>
#include <string>

/// STL includes
#include "custom_set.hpp"
#include <tuple>

/// utility includes
#include "dbgPrintHelper.hpp"
#include "math_function.hpp"
#include "utility.hpp"

top_entity_parallel_cs::top_entity_parallel_cs(const ParameterConstRef _parameters, const HLS_managerRef _HLSMgr, unsigned int _funId, const DesignFlowManagerConstRef _design_flow_manager, const HLSFlowStep_Type _hls_flow_step_type)
    : top_entity(_parameters, _HLSMgr, _funId, _design_flow_manager, _hls_flow_step_type)
{
   debug_level = parameters->get_class_debug_level(GET_CLASS(*this));
}

top_entity_parallel_cs::~top_entity_parallel_cs()
{
}

const CustomUnorderedSet<std::tuple<HLSFlowStep_Type, HLSFlowStepSpecializationConstRef, HLSFlowStep_Relationship>> top_entity_parallel_cs::ComputeHLSRelationships(const DesignFlowStep::RelationshipType relationship_type) const
{
   CustomUnorderedSet<std::tuple<HLSFlowStep_Type, HLSFlowStepSpecializationConstRef, HLSFlowStep_Relationship>> ret;
   switch(relationship_type)
   {
      case DEPENDENCE_RELATIONSHIP:
      {
         ret.insert(std::make_tuple(HLSFlowStep_Type::DATAPATH_CS_PARALLEL_CREATOR, HLSFlowStepSpecializationConstRef(), HLSFlowStep_Relationship::SAME_FUNCTION));
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

DesignFlowStep_Status top_entity_parallel_cs::InternalExec()
{
   /// function name to be synthesized
   const FunctionBehaviorConstRef FB = HLSMgr->CGetFunctionBehavior(funId);
   const std::string function_name = FB->CGetBehavioralHelper()->get_function_name();
   std::string module_name = function_name;
   const auto top_functions = HLSMgr->CGetCallGraphManager()->GetRootFunctions();
   bool is_top = top_functions.find(funId) != top_functions.end();
   if(is_top)
   {
      module_name = "_" + function_name;
   }

   /// Test on previuos steps. They checks if datapath and controller have been created. If they didn't,
   /// top circuit cannot be created.
   THROW_ASSERT(HLS->datapath, "Datapath not created");

   // reference to hls top circuit
   HLS->top = structural_managerRef(new structural_manager(parameters));
   SM = HLS->top;
   structural_managerRef Datapath = HLS->datapath;

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

   PRINT_DBG_MEX(DEBUG_LEVEL_VERBOSE, debug_level, "Creating datapath object");
   std::string parallel_controller_model = "__controller_parallel";
   std::string parallel_controller_name = "__controller_parallel";
   std::string par_ctrl_library = HLS->HLS_T->get_technology_manager()->get_library(parallel_controller_model);
   structural_objectRef controller_circuit = SM->add_module_from_technology_library(parallel_controller_name, parallel_controller_model, par_ctrl_library, circuit, HLS->HLS_T->get_technology_manager());
   controller_circuit->set_owner(circuit);
   auto loopBW = BW_loop_iter(circuit);
   resize_controller_parallel(controller_circuit, loopBW);
   THROW_ASSERT(controller_circuit, "Missing controller circuit");

   PRINT_DBG_MEX(DEBUG_LEVEL_VERBOSE, debug_level, "Creating datapath object");
   /// creating structural_manager
   PRINT_DBG_MEX(DEBUG_LEVEL_VERBOSE, debug_level, "Adding datapath");
   datapath_circuit->set_owner(circuit);
   GetPointer<module>(circuit)->add_internal_object(datapath_circuit);

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
   SM->add_connection(start_obj, controller_start);
   PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "\tStart signal added!");

   PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "\tStart adding Done signal...");
   /// add done port
   structural_objectRef done_obj = SM->add_port(DONE_PORT_NAME, port_o::OUT, circuit, bool_type);
   THROW_ASSERT(done_obj, "Done port not added in the top component");
   structural_objectRef controller_done = controller_circuit->find_member(DONE_PORT_NAME, port_o_K, controller_circuit);
   THROW_ASSERT(controller_done, "Done signal not found in the controller");
   if(HLS->registered_done_port &&
      (parameters->getOption<HLSFlowStep_Type>(OPT_controller_architecture) == HLSFlowStep_Type::FSM_CONTROLLER_CREATOR || parameters->getOption<HLSFlowStep_Type>(OPT_controller_architecture) == HLSFlowStep_Type::FSM_CS_CONTROLLER_CREATOR))
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
      structural_objectRef done_signal_out = SM->add_sign("done_delayed_REG_signal_out", circuit, GetPointer<module>(delay_gate)->get_out_port(0)->get_typeRef());
      SM->add_connection(GetPointer<module>(delay_gate)->get_out_port(0), done_signal_out);
      SM->add_connection(done_obj, done_signal_out);
   }
   else
      SM->add_connection(controller_done, done_obj);
   PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "\tDone signal added!");

   /// add entry in in_port_map between port id and port index

   PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "\tAdding input/output ports...");
   this->add_ports(circuit, clock_obj, reset_obj);
   PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "\tInput/output ports added!");

   connect_port_parallel(circuit, loopBW);

   memory::propagate_memory_parameters(HLS->datapath->get_circ(), HLS->top);

   PRINT_DBG_MEX(DEBUG_LEVEL_VERBOSE, debug_level, "Circuit created without errors!");

   return DesignFlowStep_Status::SUCCESS;
}

unsigned top_entity_parallel_cs::BW_loop_iter(const structural_objectRef circuit)
{
   const FunctionBehaviorConstRef FB = HLSMgr->CGetFunctionBehavior(funId);
   structural_objectRef controller_circuit = circuit->find_member("__controller_parallel", component_o_K, circuit);

   long long int n = 0;
   const auto listLoops = FB->CGetLoops()->GetList();
   for(auto loop : listLoops)
   {
      if(loop->GetId() != 0)
      {
         n = loop->upper_bound;
      }
   }
   if(n != 0)
   {
      auto loopBW = 1 + static_cast<unsigned>(ceil_log2(static_cast<unsigned long long int>(n)));
      return loopBW;
   }
   else
   {
      for(auto loop : listLoops)
      {
         if(loop->GetId() != 0)
         {
            THROW_ASSERT(loop->upper_bound_tn, "unexpected condition");
            return tree_helper::Size(loop->upper_bound_tn);
         }
      }
   }
   THROW_ERROR("unexpected condition");
   return 0;
}

void top_entity_parallel_cs::connect_loop_iter(const structural_objectRef circuit, unsigned loopBW)
{
   const FunctionBehaviorConstRef FB = HLSMgr->CGetFunctionBehavior(funId);
   const BehavioralHelperConstRef BH = FB->CGetBehavioralHelper();
   structural_objectRef controller_circuit = circuit->find_member("__controller_parallel", component_o_K, circuit);

   long long int n = 0;
   const auto listLoops = FB->CGetLoops()->GetList();
   for(auto loop : listLoops)
   {
      if(loop->GetId() != 0)
      {
         n = loop->upper_bound;
      }
   }
   if(n != 0)
   {
      structural_objectRef constant(new constant_o(parameters->getOption<int>(OPT_debug_level), circuit, STR(n)));
      structural_type_descriptorRef constant_type = structural_type_descriptorRef(new structural_type_descriptor("bool", loopBW));
      constant->set_type(constant_type);
      GetPointer<module>(SM->get_circ())->add_internal_object(constant);
      structural_objectRef controller_Loop_Iter = controller_circuit->find_member("LoopIteration", port_o_K, controller_circuit);
      THROW_ASSERT(controller_Loop_Iter, "unexpected condition");
      SM->add_connection(constant, controller_Loop_Iter);
      return;
   }
   else
   {
      for(auto loop : listLoops)
      {
         if(loop->GetId() != 0)
         {
            std::string name_Loop_Upper_Bound = BH->PrintVariable(loop->upper_bound_tn->index);
            structural_objectRef port_Loop_Iter = circuit->find_member(name_Loop_Upper_Bound, port_o_K, circuit);
            structural_objectRef controller_Loop_Iter = controller_circuit->find_member("LoopIteration", port_o_K, controller_circuit);
            THROW_ASSERT(controller_Loop_Iter, "unexpected condition");
            SM->add_connection(port_Loop_Iter, controller_Loop_Iter);
            return;
         }
      }
   }
   THROW_ERROR("unexpected condition");
}

void top_entity_parallel_cs::resize_controller_parallel(structural_objectRef controller_circuit, unsigned loopBW)
{
   unsigned int num_kernel = parameters->getOption<unsigned int>(OPT_num_accelerators);
   structural_objectRef controller_done_request = controller_circuit->find_member(STR(DONE_REQUEST) + "_accelerator", port_vector_o_K, controller_circuit);
   GetPointer<port_o>(controller_done_request)->add_n_ports(num_kernel, controller_done_request);
   structural_objectRef controller_done_port = controller_circuit->find_member(STR(DONE_PORT_NAME) + "_accelerator", port_vector_o_K, controller_circuit);
   GetPointer<port_o>(controller_done_port)->add_n_ports(num_kernel, controller_done_port);
   structural_objectRef controller_start_port = controller_circuit->find_member(STR(START_PORT_NAME) + "_accelerator", port_vector_o_K, controller_circuit);
   GetPointer<port_o>(controller_start_port)->add_n_ports(num_kernel, controller_start_port);

   structural_objectRef controller_request = controller_circuit->find_member("request", port_o_K, controller_circuit);
   GetPointer<port_o>(controller_request)->type_resize(loopBW);
   structural_objectRef controller_LoopIteration = controller_circuit->find_member("LoopIteration", port_o_K, controller_circuit);
   GetPointer<port_o>(controller_LoopIteration)->type_resize(loopBW);
}

void top_entity_parallel_cs::connect_port_parallel(const structural_objectRef circuit, unsigned loopBW)
{
   structural_managerRef Datapath = HLS->datapath;
   structural_objectRef datapath_circuit = Datapath->get_circ();
   structural_objectRef controller_circuit = circuit->find_member("__controller_parallel", component_o_K, circuit);
   structural_type_descriptorRef bool_type = structural_type_descriptorRef(new structural_type_descriptor("bool", 0));
   unsigned int num_slots = parameters->getOption<unsigned int>(OPT_num_accelerators);
   structural_type_descriptorRef data_type = structural_type_descriptorRef(new structural_type_descriptor("bool", loopBW));

   structural_objectRef controller_task_pool_end = controller_circuit->find_member(STR(TASKS_POOL_END), port_o_K, controller_circuit);
   structural_objectRef datapath_task_pool_end = datapath_circuit->find_member(STR(TASKS_POOL_END), port_o_K, datapath_circuit);
   structural_objectRef task_pool_end_sign = SM->add_sign(STR(TASKS_POOL_END) + "_signal", circuit, bool_type);
   SM->add_connection(datapath_task_pool_end, task_pool_end_sign);
   SM->add_connection(task_pool_end_sign, controller_task_pool_end);

   structural_objectRef datapath_done_request = datapath_circuit->find_member(STR(DONE_REQUEST) + "_accelerator", port_vector_o_K, datapath_circuit);
   structural_objectRef controller_done_request = controller_circuit->find_member(STR(DONE_REQUEST) + "_accelerator", port_vector_o_K, controller_circuit);
   structural_objectRef done_request_sign = SM->add_sign_vector(STR(DONE_REQUEST) + "_accelerator" + "_signal", num_slots, circuit, bool_type);
   SM->add_connection(datapath_done_request, done_request_sign);
   SM->add_connection(done_request_sign, controller_done_request);

   structural_objectRef datapath_done_port = datapath_circuit->find_member(STR(DONE_PORT_NAME) + "_accelerator", port_vector_o_K, datapath_circuit);
   structural_objectRef controller_done_port = controller_circuit->find_member(STR(DONE_PORT_NAME) + "_accelerator", port_vector_o_K, controller_circuit);
   structural_objectRef done_port_sign = SM->add_sign_vector(STR(DONE_PORT_NAME) + "_accelerator" + "_signal", num_slots, circuit, bool_type);
   SM->add_connection(datapath_done_port, done_port_sign);
   SM->add_connection(done_port_sign, controller_done_port);

   structural_objectRef datapath_start_port = datapath_circuit->find_member(STR(START_PORT_NAME) + "_accelerator", port_vector_o_K, datapath_circuit);
   structural_objectRef controller_start_port = controller_circuit->find_member(STR(START_PORT_NAME) + "_accelerator", port_vector_o_K, controller_circuit);
   structural_objectRef done_start_sign = SM->add_sign_vector(STR(START_PORT_NAME) + "_accelerator" + "_signal", num_slots, circuit, bool_type);
   SM->add_connection(controller_start_port, done_start_sign);
   SM->add_connection(done_start_sign, datapath_start_port);

   structural_objectRef datapath_request = datapath_circuit->find_member("request", port_o_K, datapath_circuit);
   structural_objectRef controller_request = controller_circuit->find_member("request", port_o_K, controller_circuit);
   structural_objectRef request_sign = SM->add_sign("request_signal", circuit, data_type);
   SM->add_connection(controller_request, request_sign);
   SM->add_connection(request_sign, datapath_request);

   connect_loop_iter(circuit, loopBW);
}
