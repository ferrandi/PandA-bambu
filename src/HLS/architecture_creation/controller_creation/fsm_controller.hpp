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

REF_FORWARD_DECL(tree_manager);
CONSTREF_FORWARD_DECL(OpGraph);

class fsm_controller : public ControllerCreatorBaseStep
{
   /**
    * Generates the string representation of the FSM
    */
   void create_state_machine(std::string& parse);

   /**
    * Returns the value of the guard value of a case_label_expr
    * default is not managed
    */
   std::string get_guard_value(const tree_managerRef TM, const unsigned int index, vertex op, const OpGraphConstRef data);

   /**
    * Execute the step
    * @return the exit status of this step
    */
   DesignFlowStep_Status InternalExec() override;

 protected:
   /**
    * Set the correct NP functionality
    * @param state_representation is the state representation of the FSM
    */
   virtual void add_correct_transition_memory(std::string state_representation, structural_managerRef SM);

 public:
   /**
    * Constructor.
    * @param design_flow_manager is the design flow manager
    */
   fsm_controller(const ParameterConstRef Param, const HLS_managerRef HLSMgr, unsigned int funId, const DesignFlowManagerConstRef design_flow_manager, const HLSFlowStep_Type hls_flow_step_type = HLSFlowStep_Type::FSM_CONTROLLER_CREATOR);

   /**
    * Destructor.
    */
   ~fsm_controller() override;
};
#endif
