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
 * @file extract_cond_op.hpp
 * @brief Analysis step that extract condition from multi_way_if_stmt
 *
 * @author Marco Lattuada <marco.lattuada@polimi.it>
 *
 */
#ifndef EXTRACT_COND_OP_HPP
#define EXTRACT_COND_OP_HPP
#include "function_frontend_flow_step.hpp"

#include "refcount.hpp"

/**
 * Extract cond from multi_way_if_stmt
 */
class ExtractCondOp : public FunctionFrontendFlowStep
{
 private:
   /// flag used to restart code motion step
   bool bb_modified;

   CustomUnorderedSet<std::pair<FrontendFlowStepType, FunctionRelationship>>
   ComputeFrontendRelationships(const DesignFlowStep::RelationshipType relationship_type) const override;

 public:
   ExtractCondOp(const application_managerRef AppM, const DesignFlowManager& design_flow_manager,
                 const unsigned int function_id, const ParameterConstRef parameters);

   DesignFlowStep_Status InternalExec() override;

   void Initialize() override;
};
#endif
