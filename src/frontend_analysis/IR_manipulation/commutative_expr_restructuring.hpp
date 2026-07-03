/*
 *
 *                   _/_/_/    _/_/   _/    _/ _/_/_/    _/_/
 *                  _/   _/ _/    _/ _/_/  _/ _/   _/ _/    _/
 *                 _/_/_/  _/_/_/_/ _/  _/_/ _/   _/ _/_/_/_/
 *                _/      _/    _/ _/    _/ _/   _/ _/    _/
 *               _/      _/    _/ _/    _/ _/_/_/  _/    _/
 *
 *             ***********************************************
 *                              PandA Project
 *                     URL: http://panda.dei.polimi.it
 *                       Politecnico di Milano - DEIB
 *                        System Architectures Group
 *             ***********************************************
 *              Copyright (C) 2018-2026 Politecnico di Milano
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 *
 *   This file is part of the PandA framework.
 *
 *   Licensed under the Apache License, Version 2.0, with BAMBU exceptions (the "License");
 *   you may not use this file except in compliance with the License.
 *   You may obtain a copy of the License at
 *
 *       http://www.apache.org/licenses/LICENSE-2.0
 *
 *   Unless required by applicable law or agreed to in writing, software
 *   distributed under the License is distributed on an "AS IS" BASIS,
 *   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *   See the License for the specific language governing permissions and
 *   limitations under the License.
 *
 */
/**
 * @file commutative_expr_restructuring.hpp
 * @brief Analysis step restructing tree of commutative expressions to reduce the critical path delay.
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 *
 */
#ifndef COMMUTATIVE_EXPR_RESTRUCTURING_HPP
#define COMMUTATIVE_EXPR_RESTRUCTURING_HPP
#include "function_frontend_flow_step.hpp"

#include "refcount.hpp"

REF_FORWARD_DECL(AllocationInformation);
REF_FORWARD_DECL(Schedule);
REF_FORWARD_DECL(ir_manager);
CONSTREF_FORWARD_DECL(ir_node);
REF_FORWARD_DECL(ir_node);

class commutative_expr_restructuring : public FunctionFrontendFlowStep
{
 private:
   /// The schedule
   ScheduleRef schedule;

   /// The allocation information
   AllocationInformationRef allocation_information;

   /// The IR manager
   ir_managerRef TM;

   /**
    * Return true if IR node is a assign_stmt with a mul_node/add_node in the right part
    * @param tn is the IR reindex to be considered
    */
   bool IsCommExpr(const ir_nodeConstRef tn) const;

   /**
    * Given a assign_stmt with a commutative operation it checks:
    * - if operand is a ssa_node
    * - if operand is defined in the same basic block
    * - if operand is defined in a assign_stmt whose operand is another commutative operation
    * - if operand is on the relative critical path (i.e., it delays execution of tn
    * @param tn is the starting assign_stmt
    * @param first is true if first operand has to be considered, false if second operand has to be considered
    * @param is_third_node if has to consider or not the number of uses
    * @return the chained assign_stmtment if all conditions hold
    */
   ir_nodeRef IsCommExprChain(const ir_nodeConstRef tn, const bool first, bool is_third_node) const;

   CustomUnorderedSet<std::pair<FrontendFlowStepType, FunctionRelationship>>
   ComputeFrontendRelationships(const DesignFlowStep::RelationshipType relationship_type) const override;

 public:
   commutative_expr_restructuring(const application_managerRef AppM, unsigned int function_id,
                                  const DesignFlowManager& design_flow_manager, const ParameterConstRef parameters);

   DesignFlowStep_Status InternalExec() override;

   void Initialize() override;

   bool HasToBeExecuted() const override;
};
#endif
