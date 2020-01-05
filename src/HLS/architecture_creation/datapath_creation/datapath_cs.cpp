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
 * @file classic_datapath.cpp
 * @brief Datapath for context switch
 *
 * @author Nicola Saporetti <nicola.saporetti@gmail.com>
 *
 */

#include "datapath_cs.hpp"
#include "Parameter.hpp"
#include "hls.hpp"
#include "hls_manager.hpp"
#include "omp_functions.hpp"
#include "structural_manager.hpp"
#include "structural_objects.hpp"

/// STD include
#include <cmath>

/// utility include
#include "dbgPrintHelper.hpp"
#include "math_function.hpp"
#include "utility.hpp"

datapath_cs::datapath_cs(const ParameterConstRef _parameters, const HLS_managerRef _HLSMgr, unsigned int _funId, const DesignFlowManagerConstRef _design_flow_manager, const HLSFlowStep_Type _hls_flow_step_type)
    : classic_datapath(_parameters, _HLSMgr, _funId, _design_flow_manager, _hls_flow_step_type)
{
   debug_level = _parameters->get_class_debug_level(GET_CLASS(*this));
}

datapath_cs::~datapath_cs()
{
}

void datapath_cs::add_ports()
{
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Starting " + GET_CLASS(*this) + "::" + __func__);
   classic_datapath::add_ports(); // add standard port
   auto omp_functions = GetPointer<OmpFunctions>(HLSMgr->Rfuns);
   const structural_managerRef& SM = this->HLS->datapath;
   const structural_objectRef circuit = SM->get_circ();
   bool found = false;
   if(omp_functions->parallelized_functions.find(funId) != omp_functions->parallelized_functions.end())
      found = true;
   if(omp_functions->atomic_functions.find(funId) != omp_functions->atomic_functions.end())
      found = true;
   if(found) // function with selector
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERBOSE, debug_level, "---Adding ports selector and suspension");
      int num_slots = ceil_log2(parameters->getOption<unsigned long long int>(OPT_context_switch));
      if(!num_slots)
         num_slots = 1;
      structural_type_descriptorRef port_type = structural_type_descriptorRef(new structural_type_descriptor("bool", static_cast<unsigned>(num_slots)));
      SM->add_port(STR(SELECTOR_REGISTER_FILE), port_o::IN, circuit, port_type);
      structural_type_descriptorRef bool_type = structural_type_descriptorRef(new structural_type_descriptor("bool", 0));
      SM->add_port(STR(SUSPENSION), port_o::OUT, circuit, bool_type);
   }
   if(omp_functions->kernel_functions.find(funId) != omp_functions->kernel_functions.end())
   {
      THROW_ASSERT(!found, "unexpected condition");
      INDENT_DBG_MEX(DEBUG_LEVEL_VERBOSE, debug_level, "---Adding ports for kernel module");
      int num_slots = ceil_log2(parameters->getOption<unsigned long long int>(OPT_context_switch));
      if(!num_slots)
         num_slots = 1;
      structural_type_descriptorRef port_type = structural_type_descriptorRef(new structural_type_descriptor("bool", static_cast<unsigned>(num_slots)));
      structural_type_descriptorRef bool_type = structural_type_descriptorRef(new structural_type_descriptor("bool", 0));
      SM->add_port(STR(SELECTOR_REGISTER_FILE), port_o::OUT, circuit, port_type);
      SM->add_port(STR(TASKS_POOL_END), port_o::IN, circuit, bool_type);
      SM->add_port(STR(DONE_SCHEDULER), port_o::IN, circuit, bool_type);
      SM->add_port(STR(START_PORT_NAME) + "_task", port_o::IN, circuit, bool_type);
      SM->add_port(STR(DONE_REQUEST), port_o::OUT, circuit, bool_type);
      circuit->AddParameter("KERN_NUM", "0");
   }
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Ended " + GET_CLASS(*this) + "::" + __func__);
}
