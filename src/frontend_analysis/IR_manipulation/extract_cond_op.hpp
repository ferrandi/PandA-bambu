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
