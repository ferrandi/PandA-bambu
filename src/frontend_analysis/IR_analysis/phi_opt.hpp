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
 * @file phi_opt.hpp
 * @brief Analysis step that optimize the phis starting from the IR
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 *
 */
#ifndef PHI_OPT_HPP
#define PHI_OPT_HPP
#include "function_frontend_flow_step.hpp"

#include "ir_node.hpp"
#include "refcount.hpp"

REF_FORWARD_DECL(bloc);
REF_FORWARD_DECL(Schedule);
REF_FORWARD_DECL(ir_manager);
CONSTREF_FORWARD_DECL(ir_manipulation);
class statement_list_node;
class phi_stmt;

/**
 * Identifier of patterns to be transformed by phi_opt
 */
enum class PhiOpt_PatternType
{
   UNKNOWN,       /**< Unknown pattern */
   DIFF_NOTHING,  /**< Empty basic block with multiple input */
   NO_STMT,       /**< Empty basic block dominated by assign can be removed without further changes */
   MULTI_MERGE,   /**< Phi dominated by multi_way_if_stmt to be merged */
   MULTI_NOTHING, /**< Basic block dominated by multi way if can be removed without further changes */
   MULTI_REMOVE,  /**< Phi dominated by multi_way_if_stmt to be removed */
   UNCHANGED      /**< Transformation blocked by timing */
};

/**
 * Restructure the IR control flow graph
 */
class PhiOpt : public FunctionFrontendFlowStep
{
 private:
   /// The IR manager
   ir_managerRef TM;

   /// The IR manipulation
   ir_manipulationConstRef ir_man;

   /// The basic block graph of the function
   statement_list_node* sl{nullptr};

   /// Flag to check if the initial IR has been dumped.
   static bool ir_dumped;

   /// flag used to restart code motion step
   bool bb_modified;

   /// The scheduling solution
   ScheduleRef schedule;

   /**
    * Identify to which pattern an empty basic block belongs
    * @param bb_index is the index of the empty basic block
    * @return the identified pattern
    */
   PhiOpt_PatternType IdentifyPattern(const unsigned int bb_index) const;

   /**
    * Remove an empty basic block with multiple input edges
    */
   void ApplyDiffNothing(const unsigned int bb_index);

   /**
    * Remove an empty basic block dominated by an assign statement
    * @param bb_index is the index of the empty basic block
    */
   void ApplyNoStmt(const unsigned int bb_index);

   /**
    * Transform the control flow graph by merging a phi_stmt dominated by a multi_way_if_stmt
    * @param bb_index is the index of the empty basic block
    */
   void ApplyMultiMerge(const unsigned int bb_index);

   /**
    * Transform the control flow graph by eliminating an empty basic block dominated by
    * multi_way_if_stmt without modifying phi
    * @param bb_index is the index of the empty basic block
    */
   void ApplyMultiNothing(const unsigned int bb_index);

   /**
    * Transform the control flow graph by removing a phi_stmt dominated by a multi_way_if_stmt
    * @param bb_index is the index of the empty basic block
    */
   void ApplyMultiRemove(const unsigned int bb_index);

   /**
    * Transform single input phi in assignment
    */
   void SinglePhiOptimization(const unsigned int bb_index);

   /**
    * Remove chains of basic blocks
    * @param bb_index is the starting basic block index
    */
   void ChainOptimization(const unsigned int bb_index);

   /**
    * Remove a basic block composed only of phis my merging with the successor
    * @param bb_index is the index of the basic block
    */
   void MergePhi(const unsigned int bb_index);

   /**
    * Remove a redundant select_node
    * @param statement is the statement containing
    */
   void RemoveSelectNode(const ir_nodeRef statement);

   void ReplaceVirtualUses(const ir_nodeRef& old_vssa, const OrderedIRNodeSet& new_ssa) const;

   CustomUnorderedSet<std::pair<FrontendFlowStepType, FunctionRelationship>>
   ComputeFrontendRelationships(const DesignFlowStep::RelationshipType relationship_type) const override;

 public:
   PhiOpt(const application_managerRef AppM, unsigned int function_id, const DesignFlowManager& design_flow_manager,
          const ParameterConstRef parameters);

   DesignFlowStep_Status InternalExec() override;

   void Initialize() override;
};
#endif
