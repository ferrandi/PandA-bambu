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

/// @file
/// @brief This file contains the declaration of the
/// CallGraphBuiltinCall pass that will add function called through a
/// built in to the call graph.

#ifndef CALL_GRAPH_BUILTIN_CALL_HPP
#define CALL_GRAPH_BUILTIN_CALL_HPP
#include "function_frontend_flow_step.hpp"

#include "custom_set.hpp"
#include "refcount.hpp"

CONSTREF_FORWARD_DECL(ir_node);
REF_FORWARD_DECL(ir_node);

class CallGraphBuiltinCall : public FunctionFrontendFlowStep
{
   using TypeDeclarationMap = std::map<std::string, CustomOrderedSet<unsigned int>>;

   bool modified;

   /// Already visited IR node (used to avoid infinite recursion)
   CustomUnorderedSet<unsigned int> already_visited;

   /**
    * Map function types to matching declarations.
    */
   TypeDeclarationMap typeToDeclaration;

   void lookForBuiltinCall(const ir_nodeRef& tn);

   void ExtendCallGraph(unsigned int callerIdx, const ir_nodeConstRef& funType, unsigned int stmtIdx);

   CustomUnorderedSet<std::pair<FrontendFlowStepType, FunctionRelationship>>
   ComputeFrontendRelationships(DesignFlowStep::RelationshipType RT) const override;

 public:
   CallGraphBuiltinCall(const application_managerRef AM, unsigned int functionId, const DesignFlowManager& DFM,
                        const ParameterConstRef P);

   void Initialize() override;

   DesignFlowStep_Status InternalExec() override;
};

#endif /* CALL_GRAPH_BUILTIN_CALL_H */
