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
 * @file WB4_interface.hpp
 * @brief Class to generate WB4 interfaces for high-level synthesis
 *
 * This class generates WB4 intefaces for connecting modules to
 * microprocessors or busses
 *
 * @author Marco Minutoli <mminutoli@gmail.com>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 * $Locker:  $
 * $State: Exp $
 *
 */

#ifndef _WB4_INTERFACE_HPP_
#define _WB4_INTERFACE_HPP_

#include "minimal_interface.hpp"

/// utility include
#include "refcount.hpp"
/**
 * Class generating WB4 interfaces
 */

CONSTREF_FORWARD_DECL(BehavioralHelper);
REF_FORWARD_DECL(memory_symbol);
REF_FORWARD_DECL(structural_type_descriptor);

class WB4_interface : public minimal_interface
{
 protected:
   unsigned int get_data_bus_bitsize();

   unsigned int get_addr_bus_bitsize();

   void build_WB4_bus_interface(structural_managerRef SM);

   void connect_with_signal_name(structural_managerRef SM, structural_objectRef portA, structural_objectRef portB, std::string signalName);

   void connect_with_signal_name(structural_managerRef SM, structural_objectRef APort, structural_objectRef B, std::string Bsignal, const std::string& signalName);

   void connect_with_signal_name(structural_managerRef SM, structural_objectRef A, std::string Asignal, structural_objectRef B, std::string Bsignal, const std::string& signalName);

   void connect_with_signal(structural_managerRef SM, structural_objectRef portA, structural_objectRef portB);

   void connect_with_signal(structural_managerRef SM, structural_objectRef A, std::string Asignal, structural_objectRef B, std::string Bsignal);

   void connect_with_signal(structural_managerRef SM, structural_objectRef APort, structural_objectRef B, std::string Bsignal);

   void connect_with_signal(structural_managerRef SM, structural_objectRef A, std::string Asignal, structural_objectRef portB);

   void build_WB4_complete_logic(structural_managerRef SM, structural_objectRef wrappedObj, structural_objectRef interfaceObj);

 public:
   /**
    * Constructor
    */
   WB4_interface(const ParameterConstRef Param, const HLS_managerRef HLSMgr, unsigned int funId, const DesignFlowManagerConstRef design_flow_manager, const HLSFlowStep_Type hls_flow_step_type = HLSFlowStep_Type::WB4_INTERFACE_GENERATION);

   /**
    * Destructor
    */
   ~WB4_interface() override;

   /**
    * Execute the step
    * @return the exit status of this step
    */
   DesignFlowStep_Status InternalExec() override;
};
#endif
