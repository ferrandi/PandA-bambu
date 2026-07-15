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
