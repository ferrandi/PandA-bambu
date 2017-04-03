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
 *              Copyright (c) 2004-2016 Politecnico di Milano
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
#include "top_entity_cs.hpp"
#include "structural_objects.hpp"
#include "omp_functions.hpp"

top_entity_cs::top_entity_cs(const ParameterConstRef _parameters, const HLS_managerRef _HLSMgr, unsigned int _funId, const DesignFlowManagerConstRef _design_flow_manager, const HLSFlowStep_Type _hls_flow_step_type) :
   top_entity(_parameters, _HLSMgr, _funId, _design_flow_manager, _hls_flow_step_type)
{
    debug_level = parameters->get_class_debug_level(GET_CLASS(*this));
}

top_entity_cs::~top_entity_cs()
{

}

DesignFlowStep_Status top_entity_cs::InternalExec()
{
    auto omp_functions = GetPointer<OmpFunctions>(HLSMgr->Rfuns);
    bool found=false;
    if(omp_functions->kernel_functions.find(funId) != omp_functions->kernel_functions.end())
    {
        add_context_switch_port_kernel();
    }
    else
    {
       if(omp_functions->parallelized_functions.find(funId) != omp_functions->parallelized_functions.end()) found=true;
       if(omp_functions->atomic_functions.find(funId) != omp_functions->atomic_functions.end()) found=true;
       if(found)       //function with selector
       {
          add_context_switch_port();
          PRINT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "Adding the selector port...");
          this->add_selector_register_file_port(circuit);
       }
    }
    top_entity::InternalExec();
    return DesignFlowStep_Status::SUCCESS;
}

void top_entity_cs::add_context_switch_port()
{
    structural_objectRef circuit = SM->get_circ();
    structural_type_descriptorRef bool_type = structural_type_descriptorRef(new structural_type_descriptor("bool", 0));
    PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "\tStart adding suspension signal...");
    structural_objectRef suspension_obj = SM->add_port(SUSPENSION, port_o::OUT, circuit, bool_type);
    PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "\Suspension signal added!");

    unsigned int num_slots=static_cast<unsigned int>(log2(HLS->Param->getOption<unsigned int>(OPT_context_switch)));
    structural_type_descriptorRef port_type = structural_type_descriptorRef(new structural_type_descriptor("bool", num_slots));
    PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "\tStart selector signal...");
    structural_objectRef selector_obj = SM->add_port(SELECTOR_REGISTER_FILE, port_o::IN, circuit, port_type);
    structural_objectRef datapath_selector = datapath_circuit->find_member(SELECTOR_REGISTER_FILE, port_o_K, datapath_circuit);
    SM->add_connection(datapath_selector, selector_obj);
    structural_objectRef controller_selector = controller_circuit->find_member(SELECTOR_REGISTER_FILE, port_o_K, controller_circuit);
    SM->add_connection(controller_selector, selector_obj);
    PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "\Selector signal added!");
}

void top_entity_cs::add_context_switch_port_kernel()
{
    structural_objectRef circuit = SM->get_circ();
    unsigned int num_slots=static_cast<unsigned int>(log2(HLS->Param->getOption<unsigned int>(OPT_context_switch)));
    structural_type_descriptorRef port_type = structural_type_descriptorRef(new structural_type_descriptor("bool", num_slots));
    PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "\tStart selector signal...");
    //check if SM->add port add the port to the structural if so delete it
    structural_objectRef selector_obj = SM->add_port(SELECTOR_REGISTER_FILE, port_o::IN, circuit, port_type);
    structural_objectRef datapath_selector = datapath_circuit->find_member(SELECTOR_REGISTER_FILE, port_o_K, datapath_circuit);
    SM->add_connection(datapath_selector, selector_obj);
    structural_objectRef controller_selector = controller_circuit->find_member(SELECTOR_REGISTER_FILE, port_o_K, controller_circuit);
    SM->add_connection(controller_selector, selector_obj);
    PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "\Selector signal added!");
}
