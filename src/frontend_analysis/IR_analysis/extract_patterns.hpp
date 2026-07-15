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
 * @file extract_patterns.hpp
 * @brief Class extracting patterns extending the CLANG IR. An example is the ternary_add_node.
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 *
 */

#ifndef EXTRACT_PATTERNS_HPP
#define EXTRACT_PATTERNS_HPP

#include "function_frontend_flow_step.hpp"

#include "refcount.hpp"

REF_FORWARD_DECL(extract_patterns);
REF_FORWARD_DECL(ir_manager);
class statement_list_node;

/**
 * @brief Class extracting patterns extending the CLANG IR.
 */
class extract_patterns : public FunctionFrontendFlowStep
{
 private:
   CustomUnorderedSet<std::pair<FrontendFlowStepType, FunctionRelationship>>
   ComputeFrontendRelationships(const DesignFlowStep::RelationshipType relationship_type) const override;

 public:
   extract_patterns(const ParameterConstRef _Param, const application_managerRef _AppM, unsigned int function_id,
                    const DesignFlowManager& design_flow_manager);

   DesignFlowStep_Status InternalExec() override;
};

#endif /* EXTRACT_PATTERNS_HPP */
