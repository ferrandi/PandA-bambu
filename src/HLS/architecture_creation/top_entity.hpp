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
 * @file top_entity.hpp
 * @brief Base class for the top entity creation.
 *
 * @author Christian Pilato <pilato@elet.polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *
 */
#ifndef TOP_ENTITY_HPP
#define TOP_ENTITY_HPP

#include "hls_function_step.hpp"
REF_FORWARD_DECL(structural_manager);
REF_FORWARD_DECL(structural_object);

class top_entity : public HLSFunctionStep
{
 protected:
   /// reference to the resulting circuit
   structural_managerRef SM;

   /**
    * Adds the input/output ports to the circuit
    * @param circuit is the reference to the datastructure representing the circuit
    */
   void add_ports(structural_objectRef circuit, structural_objectRef clock_port, structural_objectRef reset_port);

   /**
    * Adds the command signals to the circuit
    * @param circuit is the reference to the datastructure representing the circuit
    */
   void add_command_signals(structural_objectRef circuit);

   /**
    * Return the set of analyses in relationship with this design step
    * @param relationship_type is the type of relationship to be considered
    */
   virtual const CustomUnorderedSet<std::tuple<HLSFlowStep_Type, HLSFlowStepSpecializationConstRef, HLSFlowStep_Relationship>> ComputeHLSRelationships(const DesignFlowStep::RelationshipType relationship_type) const;

   /**
    * Add the register to store input parameters
    * @param port_in is the input parameter port of the datapath
    * @param port_prefix is the prefix of the port name
    * @param circuit is the circuit of the top entity
    * @param clock_port is the port of the clock signal
    * @param reset_port is the port of the reset signal
    * @param e_port is the input parameter port of the top entity
    */
   virtual void add_input_register(structural_objectRef port_in, const std::string& port_prefix, structural_objectRef circuit, structural_objectRef clock_port, structural_objectRef reset_port, structural_objectRef e_port);

 public:
   /**
    * Constructor
    * @param design_flow_manager is the design flow manager
    * @param top_entity_type is the type of top entity to be created
    */
   top_entity(const ParameterConstRef Param, const HLS_managerRef HLSMgr, unsigned int funId, const DesignFlowManagerConstRef design_flow_manager, const HLSFlowStep_Type = HLSFlowStep_Type::TOP_ENTITY_CREATION);

   /**
    * Destructor
    */
   virtual ~top_entity();

   /**
    * Execute the step
    * @return the exit status of this step
    */
   virtual DesignFlowStep_Status InternalExec();
};
#endif
