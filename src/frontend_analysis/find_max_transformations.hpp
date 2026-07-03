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
 *              Copyright (C) 2016-2026 Politecnico di Milano
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
 * @file find_max_transformations.hpp
 * @brief Analysis step to find transformation which breaks synthesis flow by launching bambu with different values of
 * --max-transformations
 *
 * @author Marco Lattuada <marco.lattuada@polimi.it>
 *
 */
#ifndef FIND_MAX_TRANSFORMATIONS_HPP
#define FIND_MAX_TRANSFORMATIONS_HPP
#include "application_frontend_flow_step.hpp"

#include "custom_set.hpp"
#include "design_flow_step.hpp"
#include "frontend_flow_step.hpp"

#include <cstddef>
#include <string>
#include <utility>

/**
 * Class to find the maximum admissible value of max-transformations
 */
class FindMaxTransformations : public ApplicationFrontendFlowStep
{
 private:
   /**
    * Return the set of analyses in relationship with this design step
    * @param relationship_type is the type of relationship to be considered
    */
   CustomUnorderedSet<std::pair<FrontendFlowStepType, FunctionRelationship>>
   ComputeFrontendRelationships(const DesignFlowStep::RelationshipType relationship_type) const override;

   /**
    * Compute the arg list string of bambu
    * @param max_transformations is the value to be used in the option
    * @return the argument string
    */
   const std::string ComputeArgString(const size_t max_transformations) const;

   /**
    * Execute bambu with max-transformations
    * @param max_transformations is the value to be used in the option
    * @return true if the execution was successful, false otherwise
    */
   bool ExecuteBambu(const size_t max_transformations) const;

 public:
   /**
    * Constructor.
    * @param AppM is the application manager
    * @param design_flow_manager is the design flow manager
    * @param parameters is the set of the parameters
    */
   FindMaxTransformations(const application_managerRef AppM, const DesignFlowManager& design_flow_manager,
                          const ParameterConstRef parameters);

   /**
    * Performs the profiling step
    * @return the exit status of this step
    */
   DesignFlowStep_Status Exec() override;
};
#endif
