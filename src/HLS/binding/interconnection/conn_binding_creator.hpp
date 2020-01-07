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
 * @file conn_binding_creator.hpp
 * @brief Base class for all interconnection binding algorithms.
 *
 * @defgroup conn_binding_creator_p Interconnection Binding
 * @ingroup binding_p
 *
 * @author Christian Pilato <pilato@elet.polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
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
   const CustomUnorderedSet<std::tuple<HLSFlowStep_Type, HLSFlowStepSpecializationConstRef, HLSFlowStep_Relationship>> ComputeHLSRelationships(const DesignFlowStep::RelationshipType relationship_type) const override;

 public:
   /**
    * Constructor.
    * @param design_flow_manager is the design flow manager
    * @param hls_flow_step_type is the type of connection binding algorithm
    */
   conn_binding_creator(const ParameterConstRef Param, const HLS_managerRef HLSMgr, unsigned int funId, const DesignFlowManagerConstRef design_flow_manager, const HLSFlowStep_Type hls_flow_step_type);

   /**
    * Destructor.
    */
   ~conn_binding_creator() override;
};
#endif
