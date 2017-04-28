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
    * Call different method that instantiate the new component for each function_type
    */
   virtual void add_to_SM(const HLS_managerRef HLSMgr, const hlsRef HLS, structural_objectRef clock_port, structural_objectRef reset_port);

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
    * @brief instantiate_suspension_component
    * @param HLSMgr
    * @param HLS
    */
   void instantiate_suspension_component(const HLS_managerRef HLSMgr, const hlsRef HLS);

   /**
    * @brief connectOutOr connect or with datapath or scheduler depending on function
    * @param HLSMgr
    * @param HLS
    * @param suspensionOr
    */
   void connectOutOr(const HLS_managerRef HLSMgr, const hlsRef HLS, structural_objectRef port_out_or);

   /**
    * @brief decide based on function what function to call in order to connect appropriately the datapath memory_signal with the component
    * @param HLSMgr
    * @param SM
    * @param memory_modules
    * @param circuit
    * @param HLS
    * @param _unique_id
    */
   void manage_memory_ports_parallel_chained(const HLS_managerRef HLSMgr, const structural_managerRef SM, const std::set<structural_objectRef> &memory_modules, const structural_objectRef circuit, const hlsRef HLS, unsigned int & _unique_id);

   /**
    * @brief connect scheduler with datapath and other memory modules
    * @param SM
    * @param memory_modules
    * @param circuit
    * @param HLS
    * @param _unique_id
    */
   void manage_memory_ports_parallel_chained_kernel(const structural_managerRef SM, const std::set<structural_objectRef> &memory_modules, const structural_objectRef circuit, const hlsRef HLS, unsigned int & _unique_id);

   /**
    * @brief merging on exit for the 1° channel, division on input for the 2 channel
    * @param SM
    * @param memory_modules
    * @param circuit
    * @param HLS
    * @param _unique_id
    */
   void manage_memory_ports_parallel_chained_hierarchical(const structural_managerRef SM, const std::set<structural_objectRef> &memory_modules, const structural_objectRef circuit, const hlsRef HLS, unsigned int & _unique_id);

   /**
    * @brief for each port decide its vector size
    * @param HLSMgr
    * @param HLS
    * @param scheduler_mod
    */
   void resize_scheduler_ports(const HLS_managerRef HLSMgr, const hlsRef HLS, structural_objectRef scheduler_mod);

   /**
    * @brief for each port resize it using vector size
    * @param HLSMgr
    * @param vector_size
    * @param port
    */
   void resize_dimension_bus_port(const HLS_managerRef HLSMgr, unsigned int vector_size, structural_objectRef port);
};
#endif // FU_BINDING_CS_H
