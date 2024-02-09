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
 *              Copyright (C) 2004-2024 Politecnico di Milano
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
 * @file scheduling_base_step.hpp
 * @brief scheduling base class
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 *
 */
#ifndef SCHEDULING_BASE_STEP_HPP
#define SCHEDULING_BASE_STEP_HPP

#include "custom_map.hpp"

#include "graph.hpp"
#include "hls_function_step.hpp"
#include "strong_typedef.hpp"

CONSTREF_FORWARD_DECL(OpGraph);
REF_FORWARD_DECL(DesignFlowStep);
UINT_STRONG_TYPEDEF_FORWARD_DECL(ControlStep);

/**
 * Generic class managing scheduling algorithms.
 */
class schedulingBaseStep : public HLSFunctionStep
{
 protected:
   /**
    * Compute the relationship of this step
    * @param relationship_type is the type of relationship to be considered
    * @return the steps in relationship with this
    */
   const CustomUnorderedSet<std::tuple<HLSFlowStep_Type, HLSFlowStepSpecializationConstRef, HLSFlowStep_Relationship>>
   ComputeHLSRelationships(const DesignFlowStep::RelationshipType relationship_type) const override;

 public:
   /**
    * Constructor
    * @param design_flow_manager is the design flow manager
    * @param hls_flow_step_type is the type of scheduling
    */
   schedulingBaseStep(
       const ParameterConstRef Param, const HLS_managerRef HLSMgr, unsigned int funId,
       const DesignFlowManagerConstRef design_flow_manager, const HLSFlowStep_Type hls_flow_step_type,
       const HLSFlowStepSpecializationConstRef hls_flow_step_specialization = HLSFlowStepSpecializationConstRef());

   /**
    * Destructor.
    */
   ~schedulingBaseStep() override;

   /**
    * Initialize the step
    */
   void Initialize() override;

   static void compute_RW_stmts(CustomUnorderedSet<vertex>& RW_stmts, OpGraphConstRef flow_graph,
                                const HLS_managerRef HLSMgr, unsigned function_id);
};
#endif
