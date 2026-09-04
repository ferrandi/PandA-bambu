/*
 *
 *        _/_/_/    _/_/   _/    _/ _/_/_/    _/_/
 *       _/   _/ _/    _/ _/_/  _/ _/   _/ _/    _/
 *      _/_/_/  _/_/_/_/ _/  _/_/ _/   _/ _/_/_/_/
 *     _/      _/    _/ _/    _/ _/   _/ _/    _/
 *    _/      _/    _/ _/    _/ _/_/_/  _/    _/
 *
 *  ***********************************************
 *                   PandA Project
 *   URL: https://github.com/ferrandi/PandA-bambu
 *            Politecnico di Milano - DEIB
 *             System Architectures Group
 *  ***********************************************
 *   Copyright (C) 2004-2026 Politecnico di Milano
 *
 * Part of the PandA Project, under the Apache License v2.0 with LLVM Exceptions.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 *
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
