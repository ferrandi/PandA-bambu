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
 *              Copyright (C) 2004-2026 Politecnico di Milano
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 *
 *   This file is part of the PandA framework.
 *
 *   Licensed under the Apache License, Version 2.0, with BAMBU exceptions (the "License");
 *   you may not use this file except in compliance with the License.
 *   You may obtain a copy of the License at
 *
 *       http://www.apache.org/licenses/LICENSE-2.0
 *
 *   Unless required by applicable law or agreed to in writing, software
 *   distributed under the License is distributed on an "AS IS" BASIS,
 *   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *   See the License for the specific language governing permissions and
 *   limitations under the License.
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
 *
 */
#ifndef _WB4_INTERFACE_HPP_
#define _WB4_INTERFACE_HPP_
#include "minimal_interface.hpp"

#include "refcount.hpp"

CONSTREF_FORWARD_DECL(BehavioralHelper);
REF_FORWARD_DECL(structural_type_descriptor);
REF_FORWARD_DECL(structural_object);

class WB4_interface : public minimal_interface
{
 protected:
   unsigned long long get_data_bus_bitsize();

   unsigned int get_addr_bus_bitsize();

   void build_WB4_bus_interface(structural_managerRef SM);

   void connect_with_signal_name(structural_managerRef SM, structural_objectRef portA, structural_objectRef portB,
                                 std::string signalName);

   void connect_with_signal_name(structural_managerRef SM, structural_objectRef APort, structural_objectRef B,
                                 std::string Bsignal, const std::string& signalName);

   void connect_with_signal_name(structural_managerRef SM, structural_objectRef A, std::string Asignal,
                                 structural_objectRef B, std::string Bsignal, const std::string& signalName);

   void connect_with_signal(structural_managerRef SM, structural_objectRef portA, structural_objectRef portB);

   void connect_with_signal(structural_managerRef SM, structural_objectRef A, std::string Asignal,
                            structural_objectRef B, std::string Bsignal);

   void connect_with_signal(structural_managerRef SM, structural_objectRef APort, structural_objectRef B,
                            std::string Bsignal);

   void connect_with_signal(structural_managerRef SM, structural_objectRef A, std::string Asignal,
                            structural_objectRef portB);

   void build_WB4_complete_logic(structural_managerRef SM, structural_objectRef wrappedObj,
                                 structural_objectRef interfaceObj);

 public:
   WB4_interface(const ParameterConstRef _parameters, const HLS_managerRef HLSMgr, unsigned int funId,
                 const DesignFlowManager& design_flow_manager,
                 const HLSFlowStep_Type hls_flow_step_type = HLSFlowStep_Type::WB4_INTERFACE_GENERATION);

   DesignFlowStep_Status InternalExec() override;
};
#endif
