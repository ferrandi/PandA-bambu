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
 * @file conn_binding_creator.hpp
 * @brief Base class for all interconnection binding algorithms.
 *
 * @defgroup conn_binding_creator_p Interconnection Binding
 * @ingroup binding_p
 *
 * @author Christian Pilato <pilato@elet.polimi.it>
 * $Locker:  $
 * $State: Exp $
 *
 */
#ifndef _CONN_BINDING_CREATOR_HPP_
#define _CONN_BINDING_CREATOR_HPP_

#include "hls_function_step.hpp"
REF_FORWARD_DECL(conn_binding_creator);
REF_FORWARD_DECL(generic_obj);

#include "custom_map.hpp"

/**
 * @defgroup interconnect Interconnection allocation and binding
 * @ingroup HLS
 *
 * Into this subproject, interconnections are computed and then allocated to connect elements into datapath.
 */

/**
 * @class conn_binding_creator
 * @ingroup conn_binding_creator_p
 * Generic class managing interconnection binding algorithms.
 */
class conn_binding_creator : public HLSFunctionStep
{
 protected:
   /// map between input port variable and generic object
   std::map<unsigned int, generic_objRef> input_ports;

   /// map between output port variable and generic object
   std::map<unsigned int, generic_objRef> output_ports;

   /**
    * Add ports representing parameter values
    */
   void add_parameter_ports();

   /**
    * Compute the relationship of this step
    * @param relationship_type is the type of relationship to be considered
    * @return the steps in relationship with this
    */
   HLSRelationships ComputeHLSRelationships(const DesignFlowStep::RelationshipType relationship_type) const override;

 public:
   /**
    * Constructor.
    * @param Param is the set of input parameters
    * @param HLSMgr is the HLS manager
    * @param funId is the identifier of the function being processed
    * @param design_flow_manager is the design flow manager
    * @param hls_flow_step_type is the type of connection binding algorithm
    */
   conn_binding_creator(const ParameterConstRef Param, const HLS_managerRef HLSMgr, unsigned int funId,
                        const DesignFlowManager& design_flow_manager, const HLSFlowStep_Type hls_flow_step_type);
};
#endif
