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
 *              Copyright (C) 2015-2026 Politecnico di Milano
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
 * @file basic_blocks_profiling.hpp
 * @brief Analysis step performing a dynamic profiling of basic blocks execution and cycle count
 *
 * @author Marco Lattuada <marco.lattuada@polimi.it>
 * @author Michele Fiorito <michele.fiorito@polimi.it>
 *
 */
#ifndef BASIC_BLOCKS_PROFILING_HPP
#define BASIC_BLOCKS_PROFILING_HPP

#include "application_frontend_flow_step.hpp"

class BasicBlocksProfiling : public ApplicationFrontendFlowStep
{
   CustomUnorderedSet<std::pair<FrontendFlowStepType, FunctionRelationship>>
   ComputeFrontendRelationships(const DesignFlowStep::RelationshipType relationship_type) const override;

   void ComputeRelationships(DesignFlowStepSet& relationship,
                             const DesignFlowStep::RelationshipType relationship_type) final;

 public:
   BasicBlocksProfiling(const application_managerRef AppM, const DesignFlowManager& design_flow_manager,
                        const ParameterConstRef parameters);

   void Initialize() final;

   DesignFlowStep_Status Exec() final;
};
#endif
