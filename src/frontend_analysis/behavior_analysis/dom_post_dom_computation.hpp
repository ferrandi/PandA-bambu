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
 * @file dom_post_dom_computation.hpp
 * @brief Analysis step performing dominators and post dominators computation.
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 * @author Marco Lattuada <lattuada@elet.polimi.it>
 *
 */
#ifndef DOM_POST_DOM_COMPUTATION_HPP
#define DOM_POST_DOM_COMPUTATION_HPP

#include "function_frontend_flow_step.hpp"

#include "refcount.hpp"

REF_FORWARD_DECL(dom_post_dom_computation);
CONSTREF_FORWARD_DECL(Parameter);
REF_FORWARD_DECL(ir_manager);

/**
 * loops computation analysis step
 */
class dom_post_dom_computation : public FunctionFrontendFlowStep
{
 private:
   CustomUnorderedSet<std::pair<FrontendFlowStepType, FunctionRelationship>>
   ComputeFrontendRelationships(const DesignFlowStep::RelationshipType relationship_type) const override;

 public:
   dom_post_dom_computation(const ParameterConstRef _parameters, const application_managerRef AppM,
                            unsigned int function_id, const DesignFlowManager& design_flow_manager);

   DesignFlowStep_Status InternalExec() override;

   void Initialize() override;
};
#endif
