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
