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
/**
 * @file NI_SSA_liveness.hpp
 * @brief Non-Iterative liveness analysis for SSA based descriptions.
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 *
 */
#ifndef NI_SSA_LIVENESS_HPP
#define NI_SSA_LIVENESS_HPP
#include "function_frontend_flow_step.hpp"

#include "refcount.hpp"

REF_FORWARD_DECL(NI_SSA_liveness);
REF_FORWARD_DECL(application_manager);
CONSTREF_FORWARD_DECL(Parameter);
class statement_list_node;
REF_FORWARD_DECL(bloc);
REF_FORWARD_DECL(ir_node);

/**
 *
 */
class NI_SSA_liveness : public FunctionFrontendFlowStep
{
 private:
   /// Algorithm 5: Explore all paths from a variable’s use to its definition.
   void Up_and_Mark(blocRef B, ir_nodeRef v, statement_list_node* sl);

   CustomUnorderedSet<std::pair<FrontendFlowStepType, FunctionRelationship>>
   ComputeFrontendRelationships(const DesignFlowStep::RelationshipType relationship_type) const override;

 public:
   NI_SSA_liveness(const ParameterConstRef _Param, const application_managerRef _AppM, unsigned int function_id,
                   const DesignFlowManager& design_flow_manager);

   DesignFlowStep_Status InternalExec() override;

   void Initialize() override;
};
#endif
