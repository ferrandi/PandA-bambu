/*
 *
 *        _/_/_/    _/_/   _/    _/ _/_/_/    _/_/
 *       _/   _/ _/    _/ _/_/  _/ _/   _/ _/    _/
 *      _/_/_/  _/_/_/_/ _/  _/_/ _/   _/ _/_/_/_/
 *     _/      _/    _/ _/    _/ _/   _/ _/    _/
 *    _/      _/    _/ _/    _/ _/_/_/  _/    _/
 *
 *  ***********************************************
 *                   PandA Project
 *   URL: https://github.com/ferrandi/PandA-bambu
 *            Politecnico di Milano - DEIB
 *             System Architectures Group
 *  ***********************************************
 *   Copyright (C) 2004-2026 Politecnico di Milano
 *
 * Part of the PandA Project, under the Apache License v2.0 with LLVM Exceptions.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 *
 */
/**
 * @file controller_creator_base_step.cpp
 * @brief Base class for all controller creation algorithms.
 *
 * @author Christian Pilato <pilato@elet.polimi.it>
 *
 */
#include "controller_creator_base_step.hpp"

#include "BambuParameter.hpp"
#include "behavioral_helper.hpp"
#include "call_graph_manager.hpp"
#include "commandport_obj.hpp"
#include "dbgPrintHelper.hpp"
#include "exceptions.hpp"
#include "fsm_controller.hpp"
#include "function_behavior.hpp"
#include "hls.hpp"
#include "hls_manager.hpp"
#include "math_function.hpp"
#include "multi_unbounded_obj.hpp"
#include "structural_manager.hpp"
#include "structural_objects.hpp"
#include "technology_manager.hpp"
#include "utility.hpp"

ControllerCreatorBaseStep::ControllerCreatorBaseStep(const ParameterConstRef _Param, const HLS_managerRef _HLSMgr,
                                                     unsigned int _funId, const DesignFlowManager& _design_flow_manager,
                                                     const HLSFlowStep_Type _hls_flow_step_type)
    : HLSFunctionStep(_Param, _HLSMgr, _funId, _design_flow_manager, _hls_flow_step_type), out_num(0), in_num(0)
{
}

HLS_step::HLSRelationships
ControllerCreatorBaseStep::ComputeHLSRelationships(const DesignFlowStep::RelationshipType relationship_type) const
{
   HLSRelationships ret;
   switch(relationship_type)
   {
      case(DEPENDENCE_RELATIONSHIP):
      {
         ret.insert(std::make_tuple(parameters->getOption<HLSFlowStep_Type>(OPT_datapath_architecture),
                                    HLSFlowStepSpecializationConstRef(), HLSFlowStep_Relationship::SAME_FUNCTION));
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

void ControllerCreatorBaseStep::Initialize()
{
   HLSFunctionStep::Initialize();
   /// Test on previous steps. They check if schedule, register and connection binding have been performed.
   /// Otherwise, the circuit cannot be created.
   THROW_ASSERT(this->HLS->Rsch, "Scheduling not performed");
   THROW_ASSERT(this->HLS->Rreg, "Register allocation not performed");
   THROW_ASSERT(this->HLS->Rconn, "Interconnection allocation not performed");

   // reference to the HLS controller circuit
   HLS->controller = structural_managerRef(new structural_manager(HLS->Param));
   out_ports.clear();
   cond_ports.clear();
   out_num = 0;
   in_num = 0;
}

void ControllerCreatorBaseStep::add_common_ports(structural_objectRef circuit, structural_managerRef SM)
{
   PRINT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "Adding the done port...");
   this->add_done_port(circuit, SM);

   this->add_command_ports(circuit, SM);

   PRINT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "Adding clock and reset ports...");
   this->add_clock_reset(circuit, SM);

   PRINT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "Adding the start port...");
   this->add_start_port(circuit, SM);

   /// add idle_port only for top functions
   const auto& CGM = HLSMgr->CGetCallGraphManager();
   const bool is_top = CGM.GetRootFunctions().count(funId) != 0;
   if(is_top)
   {
      PRINT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "Adding the idle port (top function)...");
      this->add_idle_port(circuit, SM);
   }

   const auto omp_info = HLSMgr->CGetFunctionBehavior(funId)->GetOMPInfo();
   if(omp_info && omp_info->context_count > 1U)
   {
      PRINT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "Adding the context selection port...");
      const structural_type_descriptorRef selector_port_type(
          new structural_type_descriptor("bool", ceil_log2(omp_info->context_count)));
      SM->add_port(SELECTOR_REGISTER_FILE, port_o::IN, circuit, selector_port_type);
   }
}

void ControllerCreatorBaseStep::add_clock_reset(structural_objectRef circuit, structural_managerRef SM)
{
   /// define Boolean type for the clock and reset signals
   structural_type_descriptorRef port_type = structural_type_descriptorRef(new structural_type_descriptor("bool", 0));

   PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "  * Start adding clock signal...");
   /// add clock port
   structural_objectRef clock_obj = SM->add_port(CLOCK_PORT_NAME, port_o::IN, circuit, port_type);
   GetPointer<port_o>(clock_obj)->set_is_clock(true);
   /// add entry in in_port_map between port id and port index
   PRINT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "  - Clock signal added!");

   PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "  * Start adding reset signal...");
   /// add reset port
   SM->add_port(RESET_PORT_NAME, port_o::IN, circuit, port_type);
   PRINT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "  - Reset signal added!");
}

void ControllerCreatorBaseStep::add_done_port(structural_objectRef circuit, structural_managerRef SM)
{
   /// define Boolean type for the done port
   structural_type_descriptorRef port_type = structural_type_descriptorRef(new structural_type_descriptor("bool", 0));
   PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "  * Start adding Done signal...");
   /// add done port
   SM->add_port(DONE_PORT_NAME, port_o::OUT, circuit, port_type);
   PRINT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "  - Done signal added!");
}

void ControllerCreatorBaseStep::add_start_port(structural_objectRef circuit, structural_managerRef SM)
{
   /// define Boolean type for the start port
   structural_type_descriptorRef port_type = structural_type_descriptorRef(new structural_type_descriptor("bool", 0));
   PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "  * Start adding start signal...");
   /// add the start port
   SM->add_port(START_PORT_NAME, port_o::IN, circuit, port_type);
   PRINT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "  - Start signal added!");
}

void ControllerCreatorBaseStep::add_idle_port(structural_objectRef circuit, structural_managerRef SM)
{
   /// define Boolean type for the idle port
   structural_type_descriptorRef port_type = structural_type_descriptorRef(new structural_type_descriptor("bool", 0));
   PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "  * Start adding Idle signal...");
   /// add idle port as output (high when the top is not operating)
   SM->add_port(IDLE_PORT_NAME, port_o::OUT, circuit, port_type);
   PRINT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "  - Idle signal added!");
}

void ControllerCreatorBaseStep::add_command_ports(structural_objectRef circuit, structural_managerRef SM)
{
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Adding command ports");

   /// define Boolean type for command signals
   structural_type_descriptorRef bool_type = structural_type_descriptorRef(new structural_type_descriptor("bool", 0));
   const FunctionBehaviorConstRef FB = HLSMgr->CGetFunctionBehavior(funId);

   out_num = 0;
   in_num = 0;
   const auto& selectors = HLS->Rconn->GetSelectors();
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                  "---Adding " + std::to_string(selectors.size()) + " selectors");
   for(const auto& selector : selectors)
   {
      for(const auto& j : selector.second)
      {
         /// connections from controller to datapath
         if(selector.first == conn_binding::IN)
         {
            /// operations signals have not to be added at this point
            if(GetPointer<commandport_obj>(j.second)->get_command_type() == commandport_obj::OPERATION)
            {
               continue;
            }
            /// they represent commands to multiplexers or write enables to registers
            structural_objectRef sel_obj =
                SM->add_port(j.second->get_structural_obj()->get_id(), port_o::OUT, circuit, bool_type);
            GetPointer<commandport_obj>(j.second)->set_controller_obj(sel_obj);
            out_ports[j.second] = out_num++;
         }
         /// connections from datapath to controller
         if(selector.first == conn_binding::OUT)
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Connection from datapath to controller");
            structural_objectRef sel_obj;
            if(GetPointer<commandport_obj>(j.second)->get_command_type() == commandport_obj::MULTI_UNBOUNDED)
            {
               sel_obj =
                   SM->add_port(GetPointer<commandport_obj>(j.second)->get_string(), port_o::IN, circuit, bool_type);
               auto mu_obj = GetPointer<commandport_obj>(j.second)->get_elem();
               THROW_ASSERT(GetPointer<multi_unbounded_obj>(mu_obj), "unexpected condition");
               mu_ports[GetPointer<multi_unbounded_obj>(mu_obj)->get_fsm_state()] = in_num;
            }
            else
            {
               /// operation modifying the control flow (e.g., MULTIIF, ...)
               auto cond_v = GetPointer<commandport_obj>(j.second)->get_vertex();
               if(GetPointer<commandport_obj>(j.second)->get_command_type() == commandport_obj::MULTIIF)
               {
                  std::vector<HLS_manager::io_binding_type> var_read =
                      HLSMgr->get_required_values(HLS->functionId, cond_v);
                  auto vect_size = static_cast<unsigned int>(var_read.size());
                  structural_type_descriptorRef multiif_port_type =
                      structural_type_descriptorRef(new structural_type_descriptor("bool", vect_size));
                  sel_obj = SM->add_port(GetPointer<commandport_obj>(j.second)->get_string(), port_o::IN, circuit,
                                         multiif_port_type);
               }
               else
               {
                  /// single bit selector representing the evaluation of a condition
                  sel_obj =
                      SM->add_port(GetPointer<commandport_obj>(j.second)->get_string(), port_o::IN, circuit, bool_type);
               }
               cond_ports[cond_v] = in_num;
            }
            GetPointer<commandport_obj>(j.second)->set_controller_obj(sel_obj);
            in_num++;
         }
      }
   }

   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Command ports added!");
}
