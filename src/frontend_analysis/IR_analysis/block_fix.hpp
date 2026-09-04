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
 * @file block_fix.hpp
 * @brief Analysis step that modifies the control flow graph of the IR to make it more compliant
 *
 * @author Marco Lattuada <lattuada@elet.polimi.it>
 *
 */
#ifndef BLOCK_FIX_HPP
#define BLOCK_FIX_HPP

#include "function_frontend_flow_step.hpp"

#include "refcount.hpp"

REF_FORWARD_DECL(ir_manager);
REF_FORWARD_DECL(ir_node);
REF_FORWARD_DECL(bloc);
class statement_list_node;
class phi_stmt;

/**
 * Restructure the IR control flow graph
 */
class BlockFix : public FunctionFrontendFlowStep
{
 private:
   /// Flag to check if the initial IR has been dumped.
   static bool ir_dumped;

   CustomUnorderedSet<std::pair<FrontendFlowStepType, FunctionRelationship>>
   ComputeFrontendRelationships(const DesignFlowStep::RelationshipType relationship_type) const override;

 public:
   BlockFix(const application_managerRef AppM, unsigned int function_id, const DesignFlowManager& design_flow_manager,
            const ParameterConstRef parameters);

   DesignFlowStep_Status InternalExec() override;

   bool HasToBeExecuted() const override;
};
#endif
