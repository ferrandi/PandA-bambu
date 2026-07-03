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
 *              Copyright (C) 2015-2026 Politecnico di Milano
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
 * @file select_tree_balancing.hpp
 * @brief Analysis step balancing select_node trees to reduce critical path delay
 *
 * @author Marco Lattuada <marco.lattuada@polimi.it>
 *
 */
#ifndef SELECT_TREE_BALANCING_HPP
#define SELECT_TREE_BALANCING_HPP
#include "function_frontend_flow_step.hpp"

#include "refcount.hpp"

REF_FORWARD_DECL(AllocationInformation);
REF_FORWARD_DECL(Schedule);
REF_FORWARD_DECL(ir_manager);
CONSTREF_FORWARD_DECL(ir_node);
REF_FORWARD_DECL(ir_node);

class SelectTreeBalancing : public FunctionFrontendFlowStep
{
 private:
   /// The schedule
   ScheduleRef schedule;

   /// The allocation information
   AllocationInformationRef allocation_information;

   /// The IR manager
   ir_managerRef TM;

   /**
    * Return true if IR node is an assign_stmt with a select_node on the right-hand side.
    * @param tn is the IR reindex to be considered
    */
   bool IsSelectStmt(const ir_nodeConstRef tn) const;

   /**
    * Given an assign_stmt with a select_node on the right-hand side and one of its operands, check whether:
    * - if operand is a ssa_node
    * - if operand is defined in the same basic block
    * - if operand is defined in an assign_stmt whose right-hand side is another select_node
    * - if operand is on the relative critical path (i.e., it delays execution of tn
    * @param tn is the starting assign_stmt
    * @param first is true if the first operand has to be considered, false if the second operand has to be considered
    * @param is_third_node tracks whether the explored node is the third node in the chain
    * @return the chained assign_stmt if all conditions hold
    */
   ir_nodeRef FindSelectChain(const ir_nodeConstRef tn, const bool first, bool is_third_node) const;

   /**
    * Return the set of analyses in relationship with this design step
    * @param relationship_type is the type of relationship to be considered
    */
   CustomUnorderedSet<std::pair<FrontendFlowStepType, FunctionRelationship>>
   ComputeFrontendRelationships(const DesignFlowStep::RelationshipType relationship_type) const override;

 public:
   SelectTreeBalancing(const application_managerRef AppM, unsigned int function_id,
                       const DesignFlowManager& design_flow_manager, const ParameterConstRef parameters);

   DesignFlowStep_Status InternalExec() override;

   void Initialize() override;

   bool HasToBeExecuted() const override;
};
#endif
