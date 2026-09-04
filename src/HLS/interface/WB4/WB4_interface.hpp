/*
 *
 *        _/_/_/    _/_/   _/    _/ _/_/_/    _/_/
 *       _/   _/ _/    _/ _/_/  _/ _/   _/ _/    _/
 *      _/_/_/  _/_/_/_/ _/  _/_/ _/   _/ _/_/_/_/
 *     _/      _/    _/ _/    _/ _/   _/ _/    _/
 *    _/      _/    _/ _/    _/ _/_/_/  _/    _/
 *
 *  ***********************************************
 *                   PandA Project
 *   URL: https://github.com/ferrandi/PandA-bambu
 *            Politecnico di Milano - DEIB
 *             System Architectures Group
 *  ***********************************************
 *   Copyright (C) 2004-2026 Politecnico di Milano
 *
 * Part of the PandA Project, under the Apache License v2.0 with LLVM Exceptions.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
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
