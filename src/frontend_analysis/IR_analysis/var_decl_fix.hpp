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
 * @file var_decl_fix.hpp
 * @brief Pre-analysis step fixing variable_val_node duplication.
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 *
 */
#ifndef VAR_DECL_FIX_HPP
#define VAR_DECL_FIX_HPP
#include "function_frontend_flow_step.hpp"

#include "custom_set.hpp"
#include "refcount.hpp"

REF_FORWARD_DECL(ir_node);

/**
 * Pre-analysis step. It transforms the raw intermediate representation removing
 * variable_val_node duplication: two var_decls with the same variable name in the same function.
 */
class VarDeclFix : public FunctionFrontendFlowStep
{
   bool recursive_examinate(const ir_nodeRef& tn, CustomUnorderedSet<unsigned int>& already_examinated_decls,
                            CustomUnorderedSet<std::string>& already_examinated_names,
                            CustomUnorderedSet<std::string>& already_examinated_type_names,
                            CustomUnorderedSet<unsigned int>& already_visited_ae);

   CustomUnorderedSet<std::pair<FrontendFlowStepType, FunctionRelationship>>
   ComputeFrontendRelationships(const DesignFlowStep::RelationshipType relationship_type) const override;

 public:
   VarDeclFix(const application_managerRef AppM, unsigned int _function_id,
              const DesignFlowManager& design_flow_manager, const ParameterConstRef parameters);

   bool HasToBeExecuted() const override;

   DesignFlowStep_Status InternalExec() override;
};
#endif
