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
 * @file load_builtin_technology.hpp
 * @brief This class load builtin components in technology manager
 *
 * @author Marco Lattuada <marco.lattuada@polimi.it>
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 *
 */
#ifndef LOAD_BUILTIN_TECHNOLOGY_HPP
#define LOAD_BUILTIN_TECHNOLOGY_HPP

#include "technology_flow_step.hpp"

#include "refcount.hpp"

REF_FORWARD_DECL(technology_manager);

/**
 * Technology steps which loads builtin components
 */
class LoadBuiltinTechnology : public TechnologyFlowStep
{
 protected:
   /**
    * Return the set of analyses in relationship with this design step
    * @param relationship_type is the type of relationship to be considered
    */
   CustomUnorderedSet<TechnologyFlowStep_Type>
   ComputeTechnologyRelationships(const DesignFlowStep::RelationshipType relationship_type) const override;

 public:
   /**
    * Constructor.
    * @param TM is the technology manager
    * @param target is the target device
    * @param design_flow_manager is the design flow manager
    * @param parameters is the set of input parameters
    */
   LoadBuiltinTechnology(const technology_managerRef TM, const generic_deviceRef target,
                         const DesignFlowManager& design_flow_manager, const ParameterConstRef parameters);

   /**
    * Execute the step
    * @return the exit status of this step
    */
   DesignFlowStep_Status Exec() override;
};
#endif
