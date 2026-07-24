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
 * @file basic_blocks_cfg_computation.hpp
 * @brief Build basic block control flow graph data structure starting from the ir_manager.
 *
 * @author Marco Lattuada <lattuada@elet.polimi.it>
 *
 */
#ifndef BASIC_BLOCK_CFG_COMPUTATION_HPP
#define BASIC_BLOCK_CFG_COMPUTATION_HPP

#include "function_frontend_flow_step.hpp"

/**
 * Build call graph structures starting from the ir_manager.
 */
class BasicBlocksCfgComputation : public FunctionFrontendFlowStep
{
 private:
   CustomUnorderedSet<std::pair<FrontendFlowStepType, FunctionRelationship>>
   ComputeFrontendRelationships(const DesignFlowStep::RelationshipType relationship_type) const override;

 public:
   BasicBlocksCfgComputation(const ParameterConstRef _parameters, const application_managerRef AppM,
                             unsigned int _function_id, const DesignFlowManager& design_flow_manager);

   DesignFlowStep_Status InternalExec() override;

   void Initialize() override;
};
#endif
