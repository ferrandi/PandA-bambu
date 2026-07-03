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
 * @file var_decl_fix.hpp
 * @brief Pre-analysis step fixing variable_val_node duplication.
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 *
 */
#ifndef VAR_DECL_FIX_HPP
#define VAR_DECL_FIX_HPP
#include "function_frontend_flow_step.hpp"

#include "custom_set.hpp"
#include "refcount.hpp"

REF_FORWARD_DECL(ir_node);

/**
 * Pre-analysis step. It transforms the raw intermediate representation removing
 * variable_val_node duplication: two var_decls with the same variable name in the same function.
 */
class VarDeclFix : public FunctionFrontendFlowStep
{
   bool recursive_examinate(const ir_nodeRef& tn, CustomUnorderedSet<unsigned int>& already_examinated_decls,
                            CustomUnorderedSet<std::string>& already_examinated_names,
                            CustomUnorderedSet<std::string>& already_examinated_type_names,
                            CustomUnorderedSet<unsigned int>& already_visited_ae);

   CustomUnorderedSet<std::pair<FrontendFlowStepType, FunctionRelationship>>
   ComputeFrontendRelationships(const DesignFlowStep::RelationshipType relationship_type) const override;

 public:
   VarDeclFix(const application_managerRef AppM, unsigned int _function_id,
              const DesignFlowManager& design_flow_manager, const ParameterConstRef parameters);

   bool HasToBeExecuted() const override;

   DesignFlowStep_Status InternalExec() override;
};
#endif
