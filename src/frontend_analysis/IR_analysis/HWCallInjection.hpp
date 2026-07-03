/*
 *                 _/_/_/    _/_/   _/    _/ _/_/_/    _/_/
 *                _/   _/ _/    _/ _/_/  _/ _/   _/ _/    _/
 *               _/_/_/  _/_/_/_/ _/  _/_/ _/   _/ _/_/_/_/
 *              _/      _/    _/ _/    _/ _/   _/ _/    _/
 *             _/      _/    _/ _/    _/ _/_/_/  _/    _/
 *
 *           ***********************************************
 *                            PandA Project
 *                   URL: http://panda.dei.polimi.it
 *                     Politecnico di Milano - DEIB
 *                      System Architectures Group
 *           ***********************************************
 *            Copyright (C) 2004-2026 Politecnico di Milano
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 *
 * This file is part of the PandA framework.
 *
 * Licensed under the Apache License, Version 2.0, with BAMBU exceptions (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef HW_CALL_INJECTION_HPP
#define HW_CALL_INJECTION_HPP

#include "function_frontend_flow_step.hpp"

#include "refcount.hpp"

REF_FORWARD_DECL(bloc);
REF_FORWARD_DECL(ir_node);

class HWCallInjection : public FunctionFrontendFlowStep
{
 private:
   static ir_nodeRef builtinWaitCallDecl;

   CustomUnorderedSet<std::pair<FrontendFlowStepType, FunctionRelationship>>
   ComputeFrontendRelationships(const DesignFlowStep::RelationshipType RT) const override;

   void buildBuiltinCall(const blocRef& block, const ir_nodeRef& stmt);

 public:
   HWCallInjection(const ParameterConstRef Param, const application_managerRef AppM, unsigned int funId,
                   const DesignFlowManager& DFM);

   DesignFlowStep_Status InternalExec() override;

   bool HasToBeExecuted() const override;
};

#endif
