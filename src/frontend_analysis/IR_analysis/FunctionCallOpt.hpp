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
 *              Copyright (C) 2021-2026 Politecnico di Milano
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
 * @file FunctionCallOpt.hpp
 *
 * @author Michele Fiorito <michele.fiorito@polimi.it>
 *
 */

#ifndef FUNCTION_CALL_OPT_HPP
#define FUNCTION_CALL_OPT_HPP

#include "function_frontend_flow_step.hpp"

#include <utility>

#include "custom_map.hpp"
#include "custom_set.hpp"

class statement_list_node;
REF_FORWARD_DECL(application_manager);
CONSTREF_FORWARD_DECL(DesignFlowManager);
CONSTREF_FORWARD_DECL(Parameter);
CONSTREF_FORWARD_DECL(ir_node);

enum FunctionOptType
{
   INLINE,
   VERSION
};

class FunctionCallOpt : public FunctionFrontendFlowStep
{
 private:
   static CustomMap<unsigned int, CustomSet<std::tuple<unsigned int, FunctionOptType>>> opt_call;

   static size_t inline_max_cost;

   CustomMap<unsigned int, unsigned int> caller_bb;

   CustomUnorderedSet<unsigned int> already_visited;

   CustomUnorderedSet<std::pair<FrontendFlowStepType, FunctionRelationship>>
   ComputeFrontendRelationships(const DesignFlowStep::RelationshipType relationship_type) const override;

   /**
    * Compute function body cost based on statements' types
    * @param body function body to be considered
    * @return size_t Cost value
    */
   size_t compute_cost(const statement_list_node* body);

   /**
    * Check if given function body has loops
    * @param body function body to be considered
    * @return true If body has loops between its basic blocks
    * @return false If no loops where detected in the cfg
    */
   size_t detect_loops(const statement_list_node* body) const;

 public:
   /// Set of always inlined functions
   static CustomSet<unsigned int> always_inline;

   /// Set of never inlined functions
   static CustomSet<unsigned int> never_inline;

   /**
    * @brief Request optimization for given call statement
    *
    * @param call_stmt the call statement optimize
    * @param caller_id id of the function where the call_stmt is present
    * @param opt type of optimization to apply
    */
   static void RequestCallOpt(const ir_nodeConstRef& call_stmt, unsigned int caller_id, FunctionOptType opt);

   FunctionCallOpt(const ParameterConstRef Param, const application_managerRef AppM, unsigned int function_id,
                   const DesignFlowManager& design_flow_manager);

   void Initialize() override;

   DesignFlowStep_Status InternalExec() override;

   bool HasToBeExecuted() const override;
};
#endif
