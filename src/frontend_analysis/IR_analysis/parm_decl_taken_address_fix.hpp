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
 * @file parm_decl_taken_address_fix.cpp
 *
 * @brief Replace direct loads of argument_val_node with reads from local variable_val_node
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 *
 */

#ifndef PARM_DECL_TAKEN_ADDRES_HPP
#define PARM_DECL_TAKEN_ADDRES_HPP

#include "function_frontend_flow_step.hpp"

class parm_decl_taken_address_fix : public FunctionFrontendFlowStep
{
   CustomUnorderedSet<std::pair<FrontendFlowStepType, FunctionRelationship>>
   ComputeFrontendRelationships(const DesignFlowStep::RelationshipType relationship_type) const override;

 public:
   parm_decl_taken_address_fix(const ParameterConstRef params, const application_managerRef AM, unsigned int fun_id,
                               const DesignFlowManager& dfm);

   bool HasToBeExecuted() const override;

   DesignFlowStep_Status InternalExec() override;
};

#endif
