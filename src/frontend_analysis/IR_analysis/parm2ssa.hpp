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
 *   Copyright (C) 2019-2026 Politecnico di Milano
 *
 * Part of the PandA Project, under the Apache License v2.0 with LLVM Exceptions.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 *
 */
/**
 * @file parm2ssa.hpp
 * @brief Pre-analysis step computing the relation between argument_val_node and the associated ssa_node.
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 *
 */
#ifndef PARM2SSA_HPP
#define PARM2SSA_HPP
#include "function_frontend_flow_step.hpp"

#include "custom_map.hpp"
#include "custom_set.hpp"
#include "refcount.hpp"

REF_FORWARD_DECL(ir_manager);
REF_FORWARD_DECL(ir_node);

/**
 * Pre-analysis step. computing the relation between argument_val_node and the associated ssa_node.
 */
class parm2ssa : public FunctionFrontendFlowStep
{
 protected:
   /**
    * Recursive IR node analysis
    */
   void recursive_analysis(const ir_nodeRef& tn, const std::string& loc_info,
                           CustomUnorderedSet<unsigned int>& already_visited_ae);

   CustomUnorderedSet<std::pair<FrontendFlowStepType, FunctionRelationship>>
   ComputeFrontendRelationships(const DesignFlowStep::RelationshipType relationship_type) const override;

 public:
   parm2ssa(const ParameterConstRef _parameters, const application_managerRef AppM, unsigned int _function_id,
            const DesignFlowManager& design_flow_manager);

   DesignFlowStep_Status InternalExec() override;
};
#endif
