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
 * @file fsm_controller.hpp
 * @brief Header class for the creation of the classical FSM controller.
 *
 * @author Christian Pilato <pilato@elet.polimi.it>
 *
 */

#ifndef FSM_CONTROLLER_HPP
#define FSM_CONTROLLER_HPP

#include "controller_creator_base_step.hpp"

/// STD include
#include <string>

REF_FORWARD_DECL(ir_manager);
CONSTREF_FORWARD_DECL(OpGraph);

class fsm_controller : public ControllerCreatorBaseStep
{
   /**
    * Generates the string representation of the FSM
    */
   void create_state_machine(std::string& parse);

   /**
    * Returns the value of the condition of an if else if
    * default is not managed
    */
   std::string get_guard_value(const ir_managerRef TM, const unsigned int index, gc_vertex_descriptor op,
                               const OpGraph& data);

   DesignFlowStep_Status InternalExec() override;

 protected:
   /**
    * Set the FSM functionality
    * @param state_representation is the state representation of the FSM
    * @param SM is the structural manager receiving the FSM logic
    */
   virtual void add_FSM(const std::string& state_representation, structural_managerRef SM);

   /**
    * @brief add_FSM_stages serialize in a string all the stage information required for the FSM generation
    * @param SM is the structural manager at which the info are added to.
    */
   virtual void add_FSM_stages(structural_managerRef SM);

 public:
   fsm_controller(const ParameterConstRef Param, const HLS_managerRef HLSMgr, unsigned int funId,
                  const DesignFlowManager& design_flow_manager,
                  const HLSFlowStep_Type hls_flow_step_type = HLSFlowStep_Type::FSM_CONTROLLER_CREATOR);
};
#endif
