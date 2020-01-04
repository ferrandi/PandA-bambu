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
#ifndef TOP_ENTITY_CS_H
#define TOP_ENTITY_CS_H
#include "top_entity.hpp"

class top_entity_cs : public top_entity
{
 protected:
   /**
    * Add the register file to store input parameters
    * @param port_in is the input parameter port of the datapath
    * @param port_prefix is the prefix of the port name
    * @param circuit is the circuit of the top entity
    * @param clock_port is the port of the clock signal
    * @param reset_port is the port of the reset signal
    * @param e_port is the input parameter port of the top entity
    */
   void add_input_register(structural_objectRef port_in, const std::string& port_prefix, structural_objectRef circuit, structural_objectRef clock_port, structural_objectRef reset_port, structural_objectRef e_port);

   void add_context_switch_port();

   void add_context_switch_port_kernel();

 public:
   top_entity_cs(const ParameterConstRef Param, const HLS_managerRef HLSMgr, unsigned int funId, const DesignFlowManagerConstRef design_flow_manager, const HLSFlowStep_Type _hls_flow_step_type = HLSFlowStep_Type::TOP_ENTITY_CS_CREATION);

   /**
    * Destructor
    */
   virtual ~top_entity_cs();

   /**
    * Add selector and suspension
    * @return the exit status of this step
    */
   virtual DesignFlowStep_Status InternalExec();
};

#endif // TOP_ENTITY_CS_H
