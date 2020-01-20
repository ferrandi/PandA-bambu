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
 *              Copyright (C) 2017-2020 Politecnico di Milano
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
 * @author Pietro Fezzardi <pietrofezzardi@gmail.com>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 */

#ifndef CONTROL_FLOW_CHECKER_HPP
#define CONTROL_FLOW_CHECKER_HPP

#include "hls_function_step.hpp"

/**
 * @name forward declarations
 */
//@{
REF_FORWARD_DECL(structural_object);
class module;
//@}

class ControlFlowChecker : public HLSFunctionStep
{
 protected:
   /**
    * Return the set of analyses in relationship with this design step
    * @param relationship_type is the type of relationship to be considered
    */
   virtual const CustomUnorderedSet<std::tuple<HLSFlowStep_Type, HLSFlowStepSpecializationConstRef, HLSFlowStep_Relationship>> ComputeHLSRelationships(const DesignFlowStep::RelationshipType relationship_type) const;

   void add_clock_reset(structural_objectRef circuit);

   void add_done_port(structural_objectRef circuit);

   void add_start_port(structural_objectRef circuit);

   void add_present_state(structural_objectRef circuit, unsigned int state_bitsize);

   void add_notifiers(structural_objectRef circuit);

   void add_common_ports(structural_objectRef circuit, unsigned int state_bitsize);

 public:
   /**
    * Constructor
    */
   ControlFlowChecker(const ParameterConstRef Param, const HLS_managerRef HLSMgr, unsigned int funId, const DesignFlowManagerConstRef design_flow_manager);

   /**
    * Destructor.
    */
   virtual ~ControlFlowChecker();

   /**
    * Execute the step
    * @return the exit status of this step
    */
   virtual DesignFlowStep_Status InternalExec();
};

#endif
