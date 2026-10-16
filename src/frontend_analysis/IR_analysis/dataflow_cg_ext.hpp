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
 *   Copyright (C) 2024-2026 Politecnico di Milano
 *
 * Part of the PandA Project, under the Apache License v2.0 with LLVM Exceptions.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 *
 */
/**
 * @file dataflow_cg_ext.hpp
 * @brief Dataflow call graph extension
 *
 * @author Michele Fiorito <michele.fiorito@polimi.it>
 *
 */
#ifndef DATAFLOW_CG_EXT_HPP
#define DATAFLOW_CG_EXT_HPP

#include "function_frontend_flow_step.hpp"

class dataflow_cg_ext : public FunctionFrontendFlowStep
{
   CustomUnorderedSet<std::pair<FrontendFlowStepType, FunctionRelationship>>
   ComputeFrontendRelationships(const DesignFlowStep::RelationshipType relationship_type) const override;

 public:
   dataflow_cg_ext(const ParameterConstRef _parameters, const application_managerRef AppM, unsigned int function_id,
                   const DesignFlowManager& design_flow_manager);

   DesignFlowStep_Status InternalExec() override;

   bool HasToBeExecuted() const override;
};

#endif // DATAFLOW_CG_EXT_HPP