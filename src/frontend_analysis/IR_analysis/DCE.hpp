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
 * @file DCE.hpp
 * @brief This file implements dead code elimination.
 *
 * @author Marco Lattuada <lattuada@elet.polimi.it>
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 *
 */
#ifndef DCE_HPP
#define DCE_HPP
#include "function_frontend_flow_step.hpp"

#include "custom_set.hpp"
#include <map>

REF_FORWARD_DECL(ir_manager);
REF_FORWARD_DECL(ir_node);
REF_FORWARD_DECL(bloc);
CONSTREF_FORWARD_DECL(Parameter);
class statement_list_node;
class node_stmt;

class DCE : public FunctionFrontendFlowStep
{
 private:
   std::map<unsigned int, bool> last_writing_memory;

   std::map<unsigned int, bool> last_reading_memory;

   bool restart_if_opt;

   bool restart_mwi_opt;

   bool restart_mem;

   CustomUnorderedSet<std::pair<FrontendFlowStepType, FunctionRelationship>>
   ComputeFrontendRelationships(const DesignFlowStep::RelationshipType relationship_type) const override;

   void kill_uses(const ir_managerRef& TM, const ir_nodeRef& op0) const;

   ir_nodeRef add_nop_stmt(const ir_managerRef& TM, const ir_nodeRef& cur_stmt, const blocRef& bb);

   void fix_sdc_motion(ir_nodeRef removedStmt) const;

   blocRef move2emptyBB(const ir_managerRef& TM, const unsigned int new_bbi, const statement_list_node* sl,
                        const blocRef& bb_pred, const unsigned int cand_bb_dest, const unsigned int bb_dest) const;

 public:
   DCE(const ParameterConstRef _parameters, const application_managerRef AppM, unsigned int function_id,
       const DesignFlowManager& design_flow_manager);

   DesignFlowStep_Status InternalExec() override;

   bool HasToBeExecuted() const override;

   /**
    * Replace virtual ssa definition with nop statements
    * @param TM IR manager instance
    * @param vdef virtual ssa
    * @return ir_nodeRef generated nop statement
    */
   static ir_nodeRef kill_vdef(const ir_managerRef& TM, const ir_nodeRef& vdef);

   static void fix_sdc_motion(const DesignFlowManager& design_flow_manager, unsigned int function_id,
                              ir_nodeRef removedStmt);
};

#endif
