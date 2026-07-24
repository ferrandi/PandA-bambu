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
 * @file top_entity.hpp
 * @brief Base class for the top entity creation.
 *
 * @author Christian Pilato <pilato@elet.polimi.it>
 *
 */
#ifndef TOP_ENTITY_HPP
#define TOP_ENTITY_HPP

#include "hls_function_step.hpp"
REF_FORWARD_DECL(structural_manager);
REF_FORWARD_DECL(structural_object);

#define TOP_FUNCTION_WRAPPER_PREFIX "PBI_"

class top_entity : public HLSFunctionStep
{
 protected:
   /// reference to the resulting circuit
   structural_managerRef SM;

   /**
    * Adds the input/output ports to the circuit
    * @param circuit is the reference to the data-structure representing the circuit
    * @param clock_port is the clock port to be attached
    * @param reset_port is the reset port to be attached
    */
   void add_ports(structural_objectRef circuit, structural_objectRef clock_port, structural_objectRef reset_port);

   /**
    * Adds the command signals to the circuit
    * @param circuit is the reference to the data-structure representing the circuit
    */
   void add_command_signals(structural_objectRef circuit);

   /**
    * Return the set of analyses in relationship with this design step
    * @param relationship_type is the type of relationship to be considered
    */
   HLSRelationships ComputeHLSRelationships(const DesignFlowStep::RelationshipType relationship_type) const override;

   /**
    * Add the register to store input parameters
    * @param port_in is the input parameter port of the datapath
    * @param port_prefix is the prefix of the port name
    * @param circuit is the circuit of the top entity
    * @param clock_port is the port of the clock signal
    * @param reset_port is the port of the reset signal
    * @param e_port is the input parameter port of the top entity
    */
   virtual void add_input_register(structural_objectRef port_in, const std::string& port_prefix,
                                   structural_objectRef circuit, structural_objectRef clock_port,
                                   structural_objectRef reset_port, structural_objectRef e_port);

 public:
   top_entity(const ParameterConstRef _parameters, const HLS_managerRef HLSMgr, unsigned int funId,
              const DesignFlowManager& design_flow_manager,
              const HLSFlowStep_Type = HLSFlowStep_Type::TOP_ENTITY_CREATION);

   DesignFlowStep_Status InternalExec() override;
};
#endif
