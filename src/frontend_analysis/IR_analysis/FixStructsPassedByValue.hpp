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
 * @file FixStructsPassedByValue.hpp
 *
 * @author Pietro Fezzardi <pietrofezzardi@gmail.com>
 *
 */
#ifndef FIX_STRUCTS_PASSED_BY_VALUE_HPP
#define FIX_STRUCTS_PASSED_BY_VALUE_HPP
#include "function_frontend_flow_step.hpp"

class FixStructsPassedByValue : public FunctionFrontendFlowStep
{
 protected:
   DesignFlowStep_Status InternalExec() override;

   CustomUnorderedSet<std::pair<FrontendFlowStepType, FunctionRelationship>>
   ComputeFrontendRelationships(const DesignFlowStep::RelationshipType relationship_type) const override;

 public:
   FixStructsPassedByValue(const ParameterConstRef params, const application_managerRef AM, unsigned int fun_id,
                           const DesignFlowManager& dfm);
};

#endif
