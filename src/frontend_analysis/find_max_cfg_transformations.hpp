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
 *              Copyright (C) 2016-2020 Politecnico di Milano
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
 * @file find_max_cfg_transformations.hpp
 * @brief Analysis step to find transformation which breaks synthesis flow by launching bambu with different values of --cfg-max-transformations
 *
 * @author Marco Lattuada <marco.lattuada@polimi.it>
 *
 */
#ifndef FIND_MAX_CFG_TRANSFORMATIONS_HPP
#define FIND_MAX_CFG_TRANSFORMATIONS_HPP

#include "application_frontend_flow_step.hpp" // for ApplicationFrontendFlo...
#include "custom_set.hpp"                     // for unordered_set
#include "design_flow_step.hpp"               // for DesignFlowManagerConstRef
#include "frontend_flow_step.hpp"             // for FrontendFlowStep::Func...
#include <cstddef>                            // for size_t
#include <string>                             // for string
#include <utility>                            // for pair

/**
 * Class to find the maximum admissible value of cfg-max-transformations
 */
class FindMaxCFGTransformations : public ApplicationFrontendFlowStep
{
 private:
   /**
    * Return the set of analyses in relationship with this design step
    * @param relationship_type is the type of relationship to be considered
    */
   const CustomUnorderedSet<std::pair<FrontendFlowStepType, FunctionRelationship>> ComputeFrontendRelationships(const DesignFlowStep::RelationshipType relationship_type) const override;

   /**
    * Compute the arg list string of bambu
    * @param cfg_max_transformations is the value to be used in the option
    * @return the argument string
    */
   const std::string ComputeArgString(const size_t cfg_max_transformations) const;

   /**
    * Execute bambu with cfg-max-transformations
    * @param cfg_max_transformations is the value to be used in the option
    * @return true if the execution was successful, false otherwise
    */
   bool ExecuteBambu(const size_t cfg_max_transformations) const;

 public:
   /**
    * Constructor.
    * @param AppM is the application manager
    * @param design_flow_manager is the design flow manager
    * @param parameters is the set of the parameters
    */
   FindMaxCFGTransformations(const application_managerRef AppM, const DesignFlowManagerConstRef design_flow_manager, const ParameterConstRef parameters);

   /**
    *  Destructor
    */
   ~FindMaxCFGTransformations() override;

   /**
    * Performs the profiling step
    * @return the exit status of this step
    */
   DesignFlowStep_Status Exec() override;
};
#endif
