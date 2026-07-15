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
 * @file BitValueRange.hpp
 * @brief Class performing some optimizations on the IR exploiting Bit Value analysis but executed after Range Analysis.
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 *
 */

#ifndef BITVALUERANGE_HPP
#define BITVALUERANGE_HPP

#include "function_frontend_flow_step.hpp"

#include "refcount.hpp"

REF_FORWARD_DECL(ir_manager);
struct ssa_node;

class BitValueRange : public FunctionFrontendFlowStep
{
 private:
   CustomUnorderedSet<std::pair<FrontendFlowStepType, FunctionRelationship>>
   ComputeFrontendRelationships(const DesignFlowStep::RelationshipType relationship_type) const override;

 public:
   BitValueRange(const ParameterConstRef _Param, const application_managerRef _AppM, unsigned int function_id,
                 const DesignFlowManager& design_flow_manager);

   DesignFlowStep_Status InternalExec() override;

   bool HasToBeExecuted() const override;
};
#endif /* BITVALUERANGE_HPP */