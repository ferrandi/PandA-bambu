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
 * @file add_op_phi_flow_edges.hpp
 * @brief Analysis step which adds flow edges from the computation of the condition(s) of gimple_cond and gimple_multi_way_if to phi
 *
 * @author Marco Lattuada <lattuada@elet.polimi.it>
 *
 */

#ifndef ADD_OP_PHI_FLOW_EDGES_HPP
#define ADD_OP_PHI_FLOW_EDGES_HPP

/// Superclass include
#include "function_frontend_flow_step.hpp"

/// Utility include
#include "refcount.hpp"

/**
 * Class to perform adding of flow edges targeting phi to operation graph
 */
class AddOpPhiFlowEdges : public FunctionFrontendFlowStep
{
 private:
   /**
    * Return the set of analyses in relationship with this design step
    * @param relationship_type is the type of relationship to be considered
    */
   const CustomUnorderedSet<std::pair<FrontendFlowStepType, FunctionRelationship>> ComputeFrontendRelationships(const DesignFlowStep::RelationshipType relationship_type) const override;

 public:
   /**
    * Constructor
    * @param AppM is the application manager
    * @param function_id is the function index
    * @param design_flow_manager is the design flow manager
    * @param parameters is the set of input parameters
    */
   AddOpPhiFlowEdges(const application_managerRef AppM, const unsigned int function_id, const DesignFlowManagerConstRef design_flow_manager, const ParameterConstRef parameters);

   /**
    * Destructor
    */
   ~AddOpPhiFlowEdges() override;

   /**
    * Initialize the step (i.e., like a constructor, but executed just before exec
    */
   void Initialize() override;

   /**
    * Performs the adding of flow edges to operation graphs
    * @return the exit status of this step
    */
   DesignFlowStep_Status InternalExec() override;
};
#endif
