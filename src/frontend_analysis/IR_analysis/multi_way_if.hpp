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
 *              Copyright (C) 2004-2026 Politecnico di Milano
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
 * @file multi_way_if.hpp
 * @brief Analysis step rebuilding multi-way if.
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 *
 */
#ifndef MULTI_WAY_IF_HPP
#define MULTI_WAY_IF_HPP

/// Superclass include
#include "function_frontend_flow_step.hpp"

#include "refcount.hpp"

REF_FORWARD_DECL(bloc);
class statement_list_node;
REF_FORWARD_DECL(ir_manager);
REF_FORWARD_DECL(ir_manipulation);
REF_FORWARD_DECL(ir_node);

/**
 * Structure the original short circuit
 */
class multi_way_if : public FunctionFrontendFlowStep
{
 private:
   /// The statement list of the analyzed function
   statement_list_node* sl;

   /// The IR manager
   ir_managerRef TM;

   /// The IR manipulation
   ir_manipulationRef ir_man;

   /// Modified file
   bool bb_modified;

   /**
    * Merge two multi_way_if_stmt in a single one
    * @param pred_bb is the basic block containing the first multi_way_if_stmt
    * @param curr_bb is the basic block containing the second multi_way_if_stmt
    */
   void MergeMultiMulti(const blocRef& pred_bb, const blocRef& curr_bb);

   /**
    * Update the basic block control flow graph data structure
    * @param pred_bb is the predecessor basic block
    * @param curr_bb is the current basic block
    */
   void UpdateCfg(const blocRef& pred_bb, const blocRef& curr_bb);

   /**
    * Insert a basic block on an edge
    * @param pred_bb is the index of the first basic block
    * @param succ_bb is the index of the second basic block
    */
   void FixCfg(const blocRef& pred_bb, const blocRef& succ_bb);

   CustomUnorderedSet<std::pair<FrontendFlowStepType, FunctionRelationship>>
   ComputeFrontendRelationships(const DesignFlowStep::RelationshipType relationship_type) const override;

 public:
   multi_way_if(const ParameterConstRef _Param, const application_managerRef _AppM, unsigned int function_id,
                const DesignFlowManager& design_flow_manager);

   DesignFlowStep_Status InternalExec() override;

   void Initialize() override;

   bool HasToBeExecuted() const override;
};
#endif
