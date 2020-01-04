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
 * @file complete_bb_graph.hpp
 * @brief This class models the ending of execution of all steps which can modify control flow graph of basic blocks
 *
 * @author Marco Lattuada <marco.lattuada@polimi.it>
 *
 */

#ifndef COMPLETE_BB_GRAPH_HPP
#define COMPLETE_BB_GRAPH_HPP

#include "custom_set.hpp"                  // for unordered_set
#include "design_flow_step.hpp"            // for DesignFlowStep, DesignFlo...
#include "frontend_flow_step.hpp"          // for FrontendFlowStep::Functio...
#include "function_frontend_flow_step.hpp" // for DesignFlowManagerConstRef
#include <utility>                         // for pair

class CompleteBBGraph : public FunctionFrontendFlowStep
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
    * @param function_index is the index of the function
    * @param design_flow_manager is the design flow manager
    * @param _Param is the set of the parameters
    */
   CompleteBBGraph(const application_managerRef AppM, const unsigned int function_index, const DesignFlowManagerConstRef design_flow_manager, const ParameterConstRef parameters);

   /**
    * Destructor
    */
   ~CompleteBBGraph() override;

   /**
    * Execute this step
    * @return the exit status of this step
    */
   DesignFlowStep_Status InternalExec() override;
};
#endif
