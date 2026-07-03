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
 * @file ir2fun.hpp
 * @brief Step that replace some IR node expression with function call
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 *
 */
#ifndef IR2FUN_HPP
#define IR2FUN_HPP
#include "custom_set.hpp"
#include "function_frontend_flow_step.hpp"
#include "refcount.hpp"

REF_FORWARD_DECL(ir2fun);
REF_FORWARD_DECL(ir_manager);
REF_FORWARD_DECL(ir_manipulation);
REF_FORWARD_DECL(ir_node);

/**
 * Add to the call graph the function calls associated with the integer division and modulus operations.
 */
class ir2fun : public FunctionFrontendFlowStep
{
 private:
   /// Already visited IR node (used to avoid infinite recursion)
   CustomUnorderedSet<unsigned int> already_visited;

   const ir_managerRef IRM;

   /**
    * Recursively examine IR node
    */
   bool recursive_transform(const ir_nodeRef& current_ir_node, const ir_nodeRef& current_statement,
                            const ir_manipulationRef ir_man);

   CustomUnorderedSet<std::pair<FrontendFlowStepType, FunctionRelationship>>
   ComputeFrontendRelationships(const DesignFlowStep::RelationshipType relationship_type) const override;

 public:
   ir2fun(const ParameterConstRef _parameters, const application_managerRef AppM, unsigned int _function_id,
          const DesignFlowManager& design_flow_manager);

   DesignFlowStep_Status InternalExec() override;
};
#endif
