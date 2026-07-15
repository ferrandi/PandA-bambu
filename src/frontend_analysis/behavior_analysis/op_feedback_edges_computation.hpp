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
 * @file op_feedback_edges_computation.hpp
 * @brief Analysis step computing feedback edges for operation control flow graph
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 * @author Marco Lattuada <lattuada@elet.polimi.it>
 *
 */
#ifndef OP_FEEDBACK_EDGES_COMPUTATION_HPP
#define OP_FEEDBACK_EDGES_COMPUTATION_HPP
#include "function_frontend_flow_step.hpp"

#include "refcount.hpp"

REF_FORWARD_DECL(ir_manager);

/**
 * Compute the feedback edges of the operations graphs.
 */
class op_feedback_edges_computation : public FunctionFrontendFlowStep
{
 private:
   CustomUnorderedSet<std::pair<FrontendFlowStepType, FunctionRelationship>>
   ComputeFrontendRelationships(const DesignFlowStep::RelationshipType relationship_type) const override;

 public:
   op_feedback_edges_computation(const ParameterConstRef _parameters, const application_managerRef AppM,
                                 unsigned int function_id, const DesignFlowManager& design_flow_manager);

   DesignFlowStep_Status InternalExec() override;
};
#endif
