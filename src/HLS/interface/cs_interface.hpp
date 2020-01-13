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
 * @file cs_interface.hpp
 * @brief Class to generate the interface for the context switch project
 *
 * @author Nicola Saporetti <nicola.saporetti@gmail.com>
 *
 */
#ifndef CS_INTERFACE_H
#define CS_INTERFACE_H

#include "module_interface.hpp"
REF_FORWARD_DECL(structural_manager);
REF_FORWARD_DECL(structural_object);

class cs_interface : public module_interface
{
 public:
   /**
    * Constructor
    */
   cs_interface(const ParameterConstRef Param, const HLS_managerRef HLSMgr, unsigned int funId, const DesignFlowManagerConstRef design_flow_manager, const HLSFlowStep_Type hls_flow_step_type = HLSFlowStep_Type::INTERFACE_CS_GENERATION);

   void build_wrapper(structural_objectRef wrappedObj, structural_objectRef interfaceObj, structural_managerRef SM_minimal_interface);
   /**
    * Destructor
    */
   virtual ~cs_interface();

   /**
    * Execute the step
    * @return the exit status of this step
    */
   virtual DesignFlowStep_Status InternalExec();

 protected:
   /**
    * @brief resize_memory_ctrl_ports
    * @param mem_ctrl_mod
    */
   void resize_memory_ctrl_ports(structural_objectRef mem_ctrl_mod);

   /**
    * @brief resize_dimension_bus_port
    * @param vector_size
    * @param port
    */
   void resize_dimension_bus_port(unsigned int vector_size, structural_objectRef port);

   /**
    * @brief manage_memory_ports_parallel_chained_parallel
    * @param SM
    * @param memory_module
    * @param circuit
    */
   void manage_extern_global_port_top(const structural_managerRef SM, const structural_objectRef memory_module, const structural_objectRef circuit);

   /**
    * @brief instantiate_component_parallel
    * @param SM
    * @param clock_port
    * @param reset_port
    */
   void instantiate_component_parallel(const structural_managerRef SM, structural_objectRef clock_port, structural_objectRef reset_port);

   /**
    * @brief add_parameter_port
    * @param SM
    * @param circuit
    * @param top_module
    */
   void add_parameter_port(const structural_managerRef SM, structural_objectRef circuit, structural_objectRef top_module);
};
#endif // CS_INTERFACE_H
