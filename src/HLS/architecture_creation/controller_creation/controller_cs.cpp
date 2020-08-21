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
 * @file controller_creator_base_step.hpp
 * @brief Base class for all the controller creation algorithms.
 *
 * This class is a pure virtual one, that has to be specilized in order to implement a particular algorithm to create the
 * controller.
 *
 * @author Nicola Saporetti <nicola.saporetti@gmail.com>
 *
 */

#include "controller_cs.hpp"
#include "BambuParameter.hpp"
#include "hls.hpp"
#include "hls_manager.hpp"
#include "math.h"
#include "omp_functions.hpp"
#include "structural_manager.hpp"
#include "structural_objects.hpp"

/// utility include
#include "dbgPrintHelper.hpp"
#include "math_function.hpp"

controller_cs::controller_cs(const ParameterConstRef _Param, const HLS_managerRef _HLSMgr, unsigned int _funId, const DesignFlowManagerConstRef _design_flow_manager, const HLSFlowStep_Type _hls_flow_step_type)
    : fsm_controller(_Param, _HLSMgr, _funId, _design_flow_manager, _hls_flow_step_type)
{
}

controller_cs::~controller_cs()
{
}

void controller_cs::add_common_ports(structural_objectRef circuit, structural_managerRef SM)
{
   fsm_controller::add_common_ports(circuit, SM);
   auto omp_functions = GetPointer<OmpFunctions>(HLSMgr->Rfuns);
   bool found = false;
   if(omp_functions->kernel_functions.find(funId) != omp_functions->kernel_functions.end())
      found = true;
   if(omp_functions->parallelized_functions.find(funId) != omp_functions->parallelized_functions.end())
      found = true;
   if(omp_functions->atomic_functions.find(funId) != omp_functions->atomic_functions.end())
      found = true;
   if(found) // function with selector
   {
      PRINT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "Adding the selector port...");
      this->add_selector_register_file_port(circuit, SM);
   }
}

void controller_cs::add_selector_register_file_port(structural_objectRef circuit, structural_managerRef SM)
{
   int num_slots = ceil_log2(parameters->getOption<unsigned long long int>(OPT_context_switch));
   if(!num_slots)
      num_slots = 1;
   structural_type_descriptorRef port_type = structural_type_descriptorRef(new structural_type_descriptor("bool", static_cast<unsigned>(num_slots)));
   PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "  * Start adding Selector signal...");
   /// add selector port
   SM->add_port(STR(SELECTOR_REGISTER_FILE), port_o::IN, circuit, port_type);
   PRINT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "  - Selector signal added!");
}

void controller_cs::add_correct_transition_memory(std::string state_representation, structural_managerRef SM)
{
   structural_objectRef circuit = SM->get_circ();
   auto omp_functions = GetPointer<OmpFunctions>(HLSMgr->Rfuns);
   bool found = false;
   if(omp_functions->kernel_functions.find(funId) != omp_functions->kernel_functions.end())
      found = true;
   if(omp_functions->parallelized_functions.find(funId) != omp_functions->parallelized_functions.end())
      found = true;
   if(omp_functions->atomic_functions.find(funId) != omp_functions->atomic_functions.end())
      found = true;
   if(found) // function with selector
      SM->add_NP_functionality(circuit, NP_functionality::FSM_CS, state_representation);
   else
      SM->add_NP_functionality(circuit, NP_functionality::FSM, state_representation);
}
