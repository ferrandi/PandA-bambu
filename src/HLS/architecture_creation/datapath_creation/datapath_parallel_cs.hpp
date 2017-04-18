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
 *              Copyright (c) 2004-2016 Politecnico di Milano
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
 * @file datapath_parallel.hpp
 * @brief Instantiate the kernel and connect port
 *
 * @author Nicola Saporetti <nicola.saporetti@gmail.com>
 *
*/

#ifndef DATAPATH_PARALLEL_CS_H
#define DATAPATH_PARALLEL_CS_H

#include "hls_function_step.hpp"

REF_FORWARD_DECL(structural_object);
REF_FORWARD_DECL(structural_manager);

class datapath_parallel_cs : public HLSFunctionStep
{
protected:
   /**
    * Return the set of analyses in relationship with this design step
    * @param relationship_type is the type of relationship to be considered
    */
   virtual const std::unordered_set<std::tuple<HLSFlowStep_Type, HLSFlowStepSpecializationConstRef, HLSFlowStep_Relationship> > ComputeHLSRelationships(const DesignFlowStep::RelationshipType relationship_type) const;

   /**
    * Adds the input/output ports of the module
    */
   void add_ports();

   /**
    * Adds the clock and reset ports to the structural description of the circuit
    * @param clock_sign is the object representing the clock signal
    * @param reset_sign is the object representing the reset signal
    */
   void add_clock_reset(structural_objectRef& clock_sign, structural_objectRef& reset_sign);

   /**
    * @brief instantiate_component_parallel
    * @param HLS
    * @param clock_port
    * @param reset_port
    */
   void instantiate_component_parallel(structural_objectRef clock_port, structural_objectRef reset_port);

   /**
    * @brief connect mem_parallel with datapath and kernels
    * @param SM
    * @param memory_modules
    * @param circuit
    */
   void manage_memory_ports_parallel_chained_parallel(const structural_managerRef SM, const std::set<structural_objectRef> &memory_modules, const structural_objectRef circuit);


public:
   /**
    * Constructor.
    * @param design_flow_manager is the design flow manager
    */
   datapath_parallel_cs(const ParameterConstRef Param, const HLS_managerRef HLSMgr, unsigned int funId, const DesignFlowManagerConstRef design_flow_manager, const HLSFlowStep_Type hls_flow_step_type);

   /**
    * Destructor.
    */
   virtual ~datapath_parallel_cs();

   /**
    * Execute the step
    * @return the exit status of this step
    */
   DesignFlowStep_Status InternalExec();
};

#endif // DATAPATH_PARALLEL_CS_H
