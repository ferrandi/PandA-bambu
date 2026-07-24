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
 * @file use_counting.hpp
 * @brief Analysis step counting how many time a ssa_node is used
 *
 * @author Marco Lattuada <lattuada@elet.polimi.it>
 *
 */
#ifndef USE_COUNTING_HPP
#define USE_COUNTING_HPP
#include "function_frontend_flow_step.hpp"

class use_counting : public FunctionFrontendFlowStep
{
   CustomUnorderedSet<std::pair<FrontendFlowStepType, FunctionRelationship>>
   ComputeFrontendRelationships(const DesignFlowStep::RelationshipType relationship_type) const override;

 public:
   use_counting(const ParameterConstRef _parameters, const application_managerRef AppM, unsigned int function_id,
                const DesignFlowManager& design_flow_manager);

   bool HasToBeExecuted() const override;

   DesignFlowStep_Status InternalExec() override;
};

#endif
