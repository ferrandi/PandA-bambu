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
 * @file controller_creator_base_step.cpp
 * @brief Base class for all controller creation algorithms.
 *
 * @author Christian Pilato <pilato@elet.polimi.it>
 *
 */
#include "controller_creator_base_step.hpp"

/// implemented controllers
#include "fsm_controller.hpp"

#include "hls.hpp"
#include "hls_manager.hpp"

#include "BambuParameter.hpp"

#include "dbgPrintHelper.hpp"
#include "utility.hpp"

#include "behavioral_helper.hpp"
#include "function_behavior.hpp"

#include "commandport_obj.hpp"
#include "multi_unbounded_obj.hpp"

#include "exceptions.hpp"

#include "structural_manager.hpp"
#include "structural_objects.hpp"

#include "technology_manager.hpp"

#include "polixml.hpp"
#include "xml_helper.hpp"

ControllerCreatorBaseStep::ControllerCreatorBaseStep(const ParameterConstRef _Param, const HLS_managerRef _HLSMgr, unsigned int _funId, const DesignFlowManagerConstRef _design_flow_manager, const HLSFlowStep_Type _hls_flow_step_type)
    : HLSFunctionStep(_Param, _HLSMgr, _funId, _design_flow_manager, _hls_flow_step_type), out_num(0), in_num(0)
{
}

ControllerCreatorBaseStep::~ControllerCreatorBaseStep() = default;

const CustomUnorderedSet<std::tuple<HLSFlowStep_Type, HLSFlowStepSpecializationConstRef, HLSFlowStep_Relationship>> ControllerCreatorBaseStep::ComputeHLSRelationships(const DesignFlowStep::RelationshipType relationship_type) const
{
   CustomUnorderedSet<std::tuple<HLSFlowStep_Type, HLSFlowStepSpecializationConstRef, HLSFlowStep_Relationship>> ret;
   switch(relationship_type)
   {
      case DEPENDENCE_RELATIONSHIP:
      {
         ret.insert(std::make_tuple(parameters->getOption<HLSFlowStep_Type>(OPT_datapath_architecture), HLSFlowStepSpecializationConstRef(), HLSFlowStep_Relationship::SAME_FUNCTION));
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

void ControllerCreatorBaseStep::Initialize()
{
   HLSFunctionStep::Initialize();
   /// Test on previuos steps. They check if schedule, register and connection binding have been performed.
   /// Otherwise, the circuit cannot be created.
   THROW_ASSERT(this->HLS->Rsch, "Scheduling not performed");
   THROW_ASSERT(this->HLS->Rreg, "Register allocation not performed");
   THROW_ASSERT(this->HLS->Rconn, "Interconnection allocation not performed");

   // reference to the HLS controller circuit
   HLS->controller = structural_managerRef(new structural_manager(HLS->Param));
   this->SM = this->HLS->controller;
   out_ports.clear();
   in_ports.clear();
   cond_ports.clear();
   cond_obj.clear();
   out_num = 0;
   in_num = 0;
}

void ControllerCreatorBaseStep::add_common_ports(structural_objectRef circuit)
{
   PRINT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "Adding the done port...");
   this->add_done_port(circuit);

   this->add_command_ports(circuit);

   PRINT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "Adding clock and reset ports...");
   this->add_clock_reset(circuit);

   PRINT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "Adding the start port...");
   this->add_start_port(circuit);
}

void ControllerCreatorBaseStep::add_clock_reset(structural_objectRef circuit)
{
   /// define boolean type for the clock and reset signals
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

void ControllerCreatorBaseStep::add_done_port(structural_objectRef circuit)
{
   /// define boolean type for the done port
   structural_type_descriptorRef port_type = structural_type_descriptorRef(new structural_type_descriptor("bool", 0));
   PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "  * Start adding Done signal...");
   /// add done port
   SM->add_port(DONE_PORT_NAME, port_o::OUT, circuit, port_type);
   PRINT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "  - Done signal added!");
}

void ControllerCreatorBaseStep::add_start_port(structural_objectRef circuit)
{
   /// define boolean type for the start port
   structural_type_descriptorRef port_type = structural_type_descriptorRef(new structural_type_descriptor("bool", 0));
   PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "  * Start adding start signal...");
   /// add the start port
   SM->add_port(START_PORT_NAME, port_o::IN, circuit, port_type);
   PRINT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "  - Start signal added!");
}

void ControllerCreatorBaseStep::add_command_ports(structural_objectRef circuit)
{
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Adding command ports");

   /// define boolean type for command signals
   structural_type_descriptorRef bool_type = structural_type_descriptorRef(new structural_type_descriptor("bool", 0));
   const FunctionBehaviorConstRef FB = HLSMgr->CGetFunctionBehavior(funId);

   out_num = 0;
   in_num = 0;
   const auto& selectors = HLS->Rconn->GetSelectors();
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Adding " + std::to_string(selectors.size()) + " selectors");
   for(const auto& selector : selectors)
   {
      for(const auto& j : selector.second)
      {
         /// connections from controller to datapath
         if(selector.first == conn_binding::IN)
         {
            /// operations signals have not to be added at this point
            if(GetPointer<commandport_obj>(j.second)->get_command_type() == commandport_obj::OPERATION)
               continue;
            /// they represent commands to multiplexers or write enables to registers
            structural_objectRef sel_obj = SM->add_port(j.second->get_structural_obj()->get_id(), port_o::OUT, circuit, bool_type);
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
               sel_obj = SM->add_port(GetPointer<commandport_obj>(j.second)->get_string(), port_o::IN, circuit, bool_type);
               auto mu_obj = GetPointer<commandport_obj>(j.second)->get_elem();
               THROW_ASSERT(GetPointer<multi_unbounded_obj>(mu_obj), "unexpected condition");
               mu_ports[GetPointer<multi_unbounded_obj>(mu_obj)->get_fsm_state()] = in_num;
            }
            else
            {
               /// operation modifying the control flow (e.g., if, switch, ...)
               vertex cond_v = GetPointer<commandport_obj>(j.second)->get_vertex();
               if(GetPointer<commandport_obj>(j.second)->get_command_type() == commandport_obj::SWITCH)
               {
                  /// multi bit selector representing the evaluation of a switch
                  unsigned int var_written = HLSMgr->get_produced_value(HLS->functionId, cond_v);
                  structural_type_descriptorRef switch_port_type = structural_type_descriptorRef(new structural_type_descriptor(var_written, FB->CGetBehavioralHelper()));
                  sel_obj = SM->add_port(GetPointer<commandport_obj>(j.second)->get_string(), port_o::IN, circuit, switch_port_type);
               }
               else if(GetPointer<commandport_obj>(j.second)->get_command_type() == commandport_obj::MULTIIF)
               {
                  std::vector<HLS_manager::io_binding_type> var_read = HLSMgr->get_required_values(HLS->functionId, cond_v);
                  auto vect_size = static_cast<unsigned int>(var_read.size());
                  structural_type_descriptorRef multiif_port_type = structural_type_descriptorRef(new structural_type_descriptor("bool", vect_size));
                  sel_obj = SM->add_port(GetPointer<commandport_obj>(j.second)->get_string(), port_o::IN, circuit, multiif_port_type);
               }
               else
               {
                  /// single bit selector representing the evaluation of a condition
                  sel_obj = SM->add_port(GetPointer<commandport_obj>(j.second)->get_string(), port_o::IN, circuit, bool_type);
               }
               cond_ports[cond_v] = in_num;
               cond_obj[cond_v] = j.second;
            }
            GetPointer<commandport_obj>(j.second)->set_controller_obj(sel_obj);
            in_ports[j.second] = in_num;
            in_num++;
         }
      }
   }

   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Command ports added!");
}
