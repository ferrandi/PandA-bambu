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
 * @file module_interface.hpp
 * @brief Base class to model interfaces for high-level synthesis
 *
 * This class is a pure virtual one, that has to be specialized in order to model a particular interface chosen
 * for connecting high-level synthesis results to microprocessors or buses
 *
 * @author Christian Pilato <pilato@elet.polimi.it>
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
   void add_sign(const structural_managerRef SM, const structural_objectRef sig1, const structural_objectRef sig2,
                 const std::string& sig_name);

   /**
    * Connects two ports by adding a signal
    * @param SM is the circuit
    * @param component1 is the first component
    * @param port1 is the name of the first port
    * @param component2 is the second component
    * @param port2 is the name of the second port
    * @param signal_name is the name of the signal to be added
    */
   void AddSignal(const structural_managerRef SM, const structural_objectRef component1, const std::string& port1,
                  const structural_objectRef component2, const std::string& port2, const std::string& signal_name);

   /**
    * Connects two ports by adding a signal
    * @param SM is the circuit
    * @param component1 is the first component
    * @param port1 is the name of the first port
    * @param component2 is the second component
    * @param port2 is the name of the second port
    */
   void AddConnection(const structural_managerRef SM, const structural_objectRef component1, const std::string& port1,
                      const structural_objectRef component2, const std::string& port2);

   /**
    * Connects a constant to a port
    * @param SM is the circuit
    * @param component is the component to which the port belongs
    * @param port is the name of the port
    * @param constant is the value of the constant
    * @param size is the size of the port
    */
   void AddConstant(const structural_managerRef SM, const structural_objectRef component, const std::string& port,
                    const std::string& constant, const unsigned long long size);

   /**
    * Connects two ports by adding a vector signal (i.e., wire)
    */
   void add_sign_vector(const structural_managerRef SM, const structural_objectRef sig1,
                        const structural_objectRef sig2, const std::string& sig_name);

   /**
    * Return the set of analyses in relationship with this design step
    * @param relationship_type is the type of relationship to be considered
    */
   HLSRelationships ComputeHLSRelationships(const DesignFlowStep::RelationshipType relationship_type) const override;

 public:
   module_interface(const ParameterConstRef _parameters, const HLS_managerRef HLSMgr, unsigned int funId,
                    const DesignFlowManager& design_flow_manager, const HLSFlowStep_Type hls_flow_step_type);
};

#endif
