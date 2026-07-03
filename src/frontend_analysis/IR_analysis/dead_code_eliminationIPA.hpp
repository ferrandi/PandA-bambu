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
 * @file dead_code_eliminationIPA.hpp
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 * @author Michele Fiorito <michele.fiorito@polimi.it>
 *
 */
#ifndef DEAD_CODE_ELIMINATION_IPA_HPP
#define DEAD_CODE_ELIMINATION_IPA_HPP
#include "application_frontend_flow_step.hpp"

REF_FORWARD_DECL(ir_manager);
struct function_val_node;

/**
 * @brief Inter-procedural dead code elimination analysis
 */
class dead_code_eliminationIPA : public ApplicationFrontendFlowStep
{
 protected:
   /**
    * stores the function ids of the functions whose Bit_Value intra procedural steps have to be invalidated by this
    * step
    */
   CustomOrderedSet<unsigned int> fun_id_to_restart;
   /**
    * stores the function ids of the functions whose Parm2SSA intra procedural steps have to be invalidated by this step
    */
   CustomOrderedSet<unsigned int> fun_id_to_restartParm;

   CustomUnorderedSet<std::pair<FrontendFlowStepType, FunctionRelationship>>
   ComputeFrontendRelationships(const DesignFlowStep::RelationshipType relationship_type) const override;

   void ComputeRelationships(DesignFlowStepSet& relationships,
                             const DesignFlowStep::RelationshipType relationship_type) override;

   bool signature_opt(const ir_managerRef& TM, function_val_node* fd, unsigned int function_id,
                      const CustomOrderedSet<unsigned int>& rFunctions);

 public:
   dead_code_eliminationIPA(const application_managerRef AM, const DesignFlowManager& dfm,
                            const ParameterConstRef parameters);

   /**
    * Execute the step
    * @return the exit status of this step
    */
   DesignFlowStep_Status Exec() override;

   /**
    * Check if this step has actually to be executed
    * @return true if the step has to be executed
    */
   bool HasToBeExecuted() const override;
};

#endif
