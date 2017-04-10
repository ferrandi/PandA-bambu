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
#include "top_entity_parallel_cs.hpp"
#include "math.h"
#include "hls.hpp"
#include "structural_manager.hpp"
#include "structural_objects.hpp"
#include "hls_manager.hpp"
#include "BambuParameter.hpp"

top_entity_parallel_cs::top_entity_parallel_cs(const ParameterConstRef _parameters, const HLS_managerRef _HLSMgr, unsigned int _funId, const DesignFlowManagerConstRef _design_flow_manager, const HLSFlowStep_Type _hls_flow_step_type) :
   top_entity(_parameters, _HLSMgr, _funId, _design_flow_manager, _hls_flow_step_type)
{
    debug_level = parameters->get_class_debug_level(GET_CLASS(*this));
}

top_entity_parallel_cs::~top_entity_parallel_cs()
{

}

DesignFlowStep_Status top_entity_parallel_cs::InternalExec()
{
   connect_port_parallel();
   return top_entity::InternalExec();
}

void top_entity_parallel_cs::connect_port_parallel()
{
    structural_managerRef Datapath = HLS->datapath;
    structural_managerRef Controller = HLS->controller;
    structural_objectRef datapath_circuit = Datapath->get_circ();
    structural_objectRef controller_circuit = Controller->get_circ();
    structural_objectRef circuit = SM->get_circ();
    structural_type_descriptorRef bool_type = structural_type_descriptorRef(new structural_type_descriptor("bool", 0));
    unsigned int num_slots=static_cast<unsigned int>(log2(HLS->Param->getOption<unsigned int>(OPT_context_switch)));
    structural_type_descriptorRef port_type = structural_type_descriptorRef(new structural_type_descriptor("bool", num_slots));

    structural_objectRef controller_task_finished = controller_circuit->find_member(STR(TASK_FINISHED)+"port", port_o_K, controller_circuit);
    structural_objectRef datapath_task_finished = datapath_circuit->find_member(STR(TASK_FINISHED)+"port", port_o_K, datapath_circuit);
    structural_objectRef task_finished_sign=SM->add_sign(STR(TASK_FINISHED)+"signal", circuit, bool_type);
    SM->add_connection(datapath_task_finished, task_finished_sign);
    SM->add_connection(task_finished_sign, controller_task_finished);

    structural_objectRef datapath_done_request = datapath_circuit->find_member(STR(DONE_REQUEST)+"port"+"accelerator", port_vector_o_K, datapath_circuit);
    structural_objectRef controller_done_request = controller_circuit->find_member(STR(DONE_REQUEST)+"port"+"accelerator", port_vector_o_K, datapath_circuit);
    structural_objectRef done_request_sign=SM->add_sign_vector(STR(DONE_REQUEST)+"accelerator"+"signal", num_slots, circuit, bool_type);
    SM->add_connection(datapath_done_request, done_request_sign);
    SM->add_connection(done_request_sign, controller_done_request);

    structural_objectRef datapath_done_port = datapath_circuit->find_member(STR(DONE_PORT_NAME)+"accelerator"+"port", port_vector_o_K, datapath_circuit);
    structural_objectRef controller_done_port = controller_circuit->find_member(STR(DONE_PORT_NAME)+"accelerator"+"port", port_vector_o_K, controller_circuit);
    structural_objectRef done_port_sign=SM->add_sign_vector(STR(DONE_PORT_NAME)+"accelerator"+"signal", num_slots, circuit, bool_type);
    SM->add_connection(datapath_done_port, done_port_sign);
    SM->add_connection(done_port_sign, controller_done_port);

    structural_objectRef datapath_start_port = datapath_circuit->find_member(STR(START_PORT_NAME)+"accelerator"+"port", port_vector_o_K, datapath_circuit);
    structural_objectRef controller_start_port = controller_circuit->find_member(STR(START_PORT_NAME)+"accelerator"+"port", port_vector_o_K, controller_circuit);
    structural_objectRef done_start_sign=SM->add_sign_vector(STR(START_PORT_NAME)+"accelerator"+"signal", num_slots, circuit, bool_type);
    SM->add_connection(controller_start_port, done_start_sign);
    SM->add_connection(done_start_sign, datapath_start_port);

    structural_type_descriptorRef request_type = structural_type_descriptorRef(new structural_type_descriptor("bool", 32));
    structural_objectRef datapath_request = datapath_circuit->find_member("request_port", port_o_K, datapath_circuit);
    structural_objectRef controller_request = controller_circuit->find_member("request_port", port_o_K, controller_circuit);
    structural_objectRef request_sign=SM->add_sign("request_signal", circuit, request_type);
    SM->add_connection(controller_request, request_sign);
    SM->add_connection(request_sign, datapath_request);
}
