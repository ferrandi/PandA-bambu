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
