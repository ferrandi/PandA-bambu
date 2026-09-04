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
 *   Copyright (C) 2016-2026 Politecnico di Milano
 *
 * Part of the PandA Project, under the Apache License v2.0 with LLVM Exceptions.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 *
 */
/**
 * @file predicate_statements.hpp
 * @brief This class contains the methods for setting predicates of instructions which cannot be speculated
 *
 * @author Marco Lattuada <marco.lattuada@polimi.it>
 *
 */

/// Superclass include
#include "function_frontend_flow_step.hpp"

class PredicateStatements : public FunctionFrontendFlowStep
{
 private:
   CustomUnorderedSet<std::pair<FrontendFlowStepType, FunctionRelationship>>
   ComputeFrontendRelationships(const DesignFlowStep::RelationshipType relationship_type) const override;

 public:
   PredicateStatements(const application_managerRef AppM, unsigned int function_id,
                       const DesignFlowManager& design_flow_manager, const ParameterConstRef parameters);

   DesignFlowStep_Status InternalExec() override;

   bool HasToBeExecuted() const override;
};
