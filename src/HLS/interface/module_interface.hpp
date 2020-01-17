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
 * @file module_interface.hpp
 * @brief Base class to model interfaces for high-level synthesis
 *
 * This class is a pure virtual one, that has to be specialized in order to model a particular interface chosen
 * for connecting high-level synthesis results to microprocessors or buses
 *
 * @author Christian Pilato <pilato@elet.polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 */
#ifndef _MODULE_INTERFACE_HPP_
#define _MODULE_INTERFACE_HPP_

#include "hls_function_step.hpp"
REF_FORWARD_DECL(module_interface);
REF_FORWARD_DECL(structural_manager);
REF_FORWARD_DECL(structural_object);

class module_interface : public HLSFunctionStep
{
 protected:
   /**
    * Connects two ports by adding a signal (i.e., wire)
    */
   void add_sign(const structural_managerRef SM, const structural_objectRef sig1, const structural_objectRef sig2, const std::string& sig_name);

   /**
    * Connects two ports by adding a signal
    * @param SM is the circuit
    * @param component1 is the first component
    * @param port1 is the name of the first port
    * @param component2 is the second component
    * @param port2 is the name of the second port
    * @param sig_name is the name of the signal to be added
    */
   void AddSignal(const structural_managerRef SM, const structural_objectRef component1, const std::string& port1, const structural_objectRef component2, const std::string& port2, const std::string& signal_name);

   /**
    * Connects two ports by adding a signal
    * @param SM is the circuit
    * @param component1 is the first component
    * @param port1 is the name of the first port
    * @param component2 is the second component
    * @param port2 is the name of the second port
    */
   void AddConnection(const structural_managerRef SM, const structural_objectRef component1, const std::string& port1, const structural_objectRef component2, const std::string& port2);

   /**
    * Connects a constant to a port
    * @param SM is the circuit
    * @param component is the component to which the port belongs
    * @param port_name is the name of the port
    * @param constant is the value of the constant
    * @param size is the size of the port
    */
   void AddConstant(const structural_managerRef SM, const structural_objectRef component, const std::string& port, const std::string& constant, const unsigned int size);

   /**
    * Connects two ports by adding a vector signal (i.e., wire)
    */
   void add_sign_vector(const structural_managerRef SM, const structural_objectRef sig1, const structural_objectRef sig2, const std::string& sig_name);

   /**
    * Return the set of analyses in relationship with this design step
    * @param relationship_type is the type of relationship to be considered
    */
   const CustomUnorderedSet<std::tuple<HLSFlowStep_Type, HLSFlowStepSpecializationConstRef, HLSFlowStep_Relationship>> ComputeHLSRelationships(const DesignFlowStep::RelationshipType relationship_type) const override;

 public:
   /**
    * Constructor
    */
   module_interface(const ParameterConstRef Param, const HLS_managerRef HLSMgr, unsigned int funId, const DesignFlowManagerConstRef design_flow_manager, const HLSFlowStep_Type hls_flow_step_type);

   /**
    * Destructor
    */
   ~module_interface() override;
};

#endif
