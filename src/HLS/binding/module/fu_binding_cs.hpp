/*
 *
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
 * @file fu_binding_cs.hpp
 * @brief Derived class to add module scheduler, mem_ctrl_parallel and bind correctly the channels
 *
 * @author Nicola Saporetti <nicola.saporetti@gmail.com>
 */
#ifndef FU_BINDING_CS_H
#define FU_BINDING_CS_H

#include "fu_binding.hpp"

class fu_binding_cs : public fu_binding
{
 public:
   fu_binding_cs(const HLS_managerConstRef HLS_mgr, const unsigned int function_id, const ParameterConstRef parameters);

   /**
    * Destructor
    */
   virtual ~fu_binding_cs();

   /**
    * Call different method that instantiate the new component for each function_type
    */
   virtual void add_to_SM(const HLS_managerRef HLSMgr, const hlsRef HLS, structural_objectRef clock_port, structural_objectRef reset_port);

   /**
    * @brief decide based on function what function to call in order to connect appropriately the datapath memory_signal with the component
    * @param HLSMgr
    * @param SM
    * @param memory_modules
    * @param circuit
    * @param HLS
    * @param _unique_id
    */
   void manage_memory_ports_parallel_chained(const HLS_managerRef HLSMgr, const structural_managerRef SM, const std::list<structural_objectRef>& memory_modules, const structural_objectRef circuit, const hlsRef HLS, unsigned int& _unique_id);

   /**
    * @brief manage_extern_global_port based on function attach the input of memory modules
    * @param HLSMgr
    * @param HLS
    * @param SM
    * @param port_in
    * @param dir
    * @param circuit
    * @param num
    */
   void manage_extern_global_port(const HLS_managerRef HLSMgr, const hlsRef HLS, const structural_managerRef SM, structural_objectRef port_in, unsigned int dir, structural_objectRef circuit, unsigned int num);

 protected:
   /**
    * @brief instantiate_component_kernel
    * @param HLSMgr
    * @param HLS
    * @param clock_port
    * @param reset_port
    */
   void instantiate_component_kernel(const HLS_managerRef HLSMgr, const hlsRef HLS, structural_objectRef clock_port, structural_objectRef reset_port);

   /**
    * @brief connectOutOr connect or with datapath or scheduler depending on function
    * @param HLSMgr
    * @param HLS
    * @param suspensionOr
    */
   void connectOutOr(const HLS_managerRef HLSMgr, const hlsRef HLS, structural_objectRef port_out_or);

   /**
    * @brief connect scheduler with datapath and other memory modules
    * @param SM
    * @param memory_modules
    * @param circuit
    * @param HLS
    * @param _unique_id
    */
   void connect_scheduler_Datapath(const structural_managerRef SM, const std::list<structural_objectRef>& memory_modules, const structural_objectRef circuit, const hlsRef HLS, unsigned int& _unique_id);

   /**
    * @brief for each port decide its vector size
    * @param HLSMgr
    * @param scheduler_mod
    */
   void resize_scheduler_ports(const HLS_managerRef HLSMgr, structural_objectRef scheduler_mod);

   /**
    * @brief for each port resize it using vector size
    * @param HLSMgr
    * @param port
    */
   void resize_dimension_bus_port(const HLS_managerRef HLSMgr, structural_objectRef port);

   /**
    * @brief manage_memory_port_kernel connect correctly memory port when in kernel function
    * @param SM
    * @param memory_modules
    * @param circuit
    * @param HLS
    * @param _unique_id
    */
   void manage_memory_port_kernel(const structural_managerRef SM, const std::list<structural_objectRef>& memory_modules, const structural_objectRef circuit, const hlsRef HLS, unsigned int& _unique_id);

   /**
    * @brief manage_memory_port_hierarchical connect correctly memory port when in hierarchical function
    * @param SM
    * @param memory_modules
    * @param circuit
    * @param HLS
    * @param _unique_id
    */
   void manage_memory_port_hierarchical(const structural_managerRef SM, const std::list<structural_objectRef>& memory_modules, const structural_objectRef circuit, const hlsRef HLS, unsigned int& _unique_id);

   /**
    * @brief connect_selector in function not kernel connect selector with all the module that have the right port
    * @param HLS
    */
   void connect_selector(const hlsRef HLS);

   /**
    * @brief connect_selector_kernel, kernel take selector from scheduler
    * @param HLS
    */
   void connect_selector_kernel(const hlsRef HLS);

   /**
    * @brief set_atomic_memory_parameter need to set in order to have memory operation defined as atomic
    * @param HLS
    */
   void set_atomic_memory_parameter(const hlsRef HLS);
};
#endif // FU_BINDING_CS_H
