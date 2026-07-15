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
 * @file minimal_interface.hpp
 * @brief Class to generate minimal interfaces for high-level synthesis
 *
 * This class generates minimal interfaces for connecting modules to
 * microprocessors or buses
 *
 * @author Marco Minutoli <mminutoli@gmail.com>
 *
 */
#ifndef _MINIMAL_INTERFACE_HPP_
#define _MINIMAL_INTERFACE_HPP_

#include "module_interface.hpp"

CONSTREF_FORWARD_DECL(BehavioralHelper);

/**
 * Class generating minimal interfaces
 */
class minimal_interface : public module_interface
{
   void build_wrapper(structural_objectRef wrappedObj, structural_objectRef interfaceObj,
                      structural_managerRef SM_minimal_interface);

 public:
   minimal_interface(const ParameterConstRef Param, const HLS_managerRef HLSMgr, unsigned int funId,
                     const DesignFlowManager& design_flow_manager,
                     const HLSFlowStep_Type hls_flow_step_type = HLSFlowStep_Type::MINIMAL_INTERFACE_GENERATION);

   DesignFlowStep_Status InternalExec() override;
};

#endif
