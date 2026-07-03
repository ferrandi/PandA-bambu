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
 * @file reg_binding_creator.hpp
 * @brief Base class for all the register allocation algorithms.
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 * @author Christian Pilato <pilato@elet.polimi.it>
 *
 */
#ifndef _REG_BINDING_CREATOR_HPP_
#define _REG_BINDING_CREATOR_HPP_
#include "hls_function_step.hpp"

#include "HLS/fsm/FSMInfo.hpp"
#include "graph.hpp"

/**
 * Generic class managing the different register allocation algorithms.
 */
class reg_binding_creator : public HLSFunctionStep
{
 protected:
   /// lower bound
   unsigned int register_lower_bound;

   bool cs_modified;

   virtual DesignFlowStep_Status RegisterBinding() = 0;

   CustomMap<FSMInfo::state_descriptor, bool> ComputeContextSwitchStates();

   HLSRelationships ComputeHLSRelationships(const DesignFlowStep::RelationshipType relationship_type) const override;

 public:
   /**
    * Constructor
    * @param Param is the set of input parameters
    * @param HLSMgr is the HLS manager
    * @param funId is the identifier of the function being processed
    * @param design_flow_manager is the design flow manager
    * @param hls_flow_step_type is the register binding algorithm to be used
    * @param hls_flow_step_specialization is the specialization of the binding step
    */
   reg_binding_creator(
       const ParameterConstRef Param, const HLS_managerRef HLSMgr, unsigned int funId,
       const DesignFlowManager& design_flow_manager, const HLSFlowStep_Type hls_flow_step_type,
       const HLSFlowStepSpecializationConstRef hls_flow_step_specialization = HLSFlowStepSpecializationConstRef());

   void Initialize() override;

   DesignFlowStep_Status InternalExec() final;
};

#endif
