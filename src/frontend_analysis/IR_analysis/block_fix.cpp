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
 * @file block_fix.cpp
 * @brief Analysis step which modifies the control flow graph of the IR to make it more compliant and simple
 *
 * @author Marco Lattuada <lattuada@elet.polimi.it>
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 *
 */
#include "block_fix.hpp"

#include "Parameter.hpp"
#include "application_manager.hpp"
#include "dbgPrintHelper.hpp"
#include "function_behavior.hpp"
#include "ir_basic_block.hpp"
#include "ir_helper.hpp"
#include "ir_manager.hpp"
#include "ir_node.hpp"
#include "string_manipulation.hpp"

#include <fstream>

BlockFix::BlockFix(const application_managerRef _AppM, unsigned int _function_id,
                   const DesignFlowManager& _design_flow_manager, const ParameterConstRef _parameters)
    : FunctionFrontendFlowStep(_AppM, _function_id, BLOCK_FIX, _design_flow_manager, _parameters)
{
   debug_level = _parameters->get_class_debug_level(GET_CLASS(*this), DEBUG_LEVEL_NONE);
}

CustomUnorderedSet<std::pair<FrontendFlowStepType, FunctionFrontendFlowStep::FunctionRelationship>>
BlockFix::ComputeFrontendRelationships(const DesignFlowStep::RelationshipType relationship_type) const
{
   CustomUnorderedSet<std::pair<FrontendFlowStepType, FunctionRelationship>> relationships;
   switch(relationship_type)
   {
      case(DEPENDENCE_RELATIONSHIP):
      {
         relationships.insert(std::make_pair(CALL_NODE_FIX, SAME_FUNCTION));
         break;
      }
      case(PRECEDENCE_RELATIONSHIP):
      {
         break;
      }
      case(INVALIDATION_RELATIONSHIP):
      {
         break;
      }
      default:
      {
         THROW_UNREACHABLE("");
      }
   }
   return relationships;
}

DesignFlowStep_Status BlockFix::InternalExec()
{
   const ir_managerRef TM = AppM->get_ir_manager();
   ir_nodeRef temp = TM->GetIRNode(function_id);
   auto* fd = GetPointer<function_val_node>(temp);
   auto* sl = GetPointer<statement_list_node>(fd->body);

   std::map<unsigned int, blocRef>& list_of_bloc = sl->list_of_bloc;
   std::map<unsigned int, blocRef>::iterator it3, it3_end = list_of_bloc.end();

   // Adding entry block
   blocRef entry_bloc = blocRef(new bloc(BB_ENTRY));
   // Set of successor of entry
   std::vector<unsigned int>& succ_entry = entry_bloc->list_of_succ;
   for(it3 = list_of_bloc.begin(); it3 != it3_end; ++it3)
   {
      std::vector<unsigned int>::iterator it2, it2_end;
      if(it3->second)
      {
         it2_end = it3->second->list_of_pred.end();
         for(it2 = it3->second->list_of_pred.begin(); it2 != it2_end; ++it2)
         {
            if(*it2 == BB_ENTRY)
            {
               succ_entry.push_back(it3->second->number);
            }
         }
      }
   }
   /// set of predecessor of exit is missing! TO BE CHECK
   // Adding exit
   blocRef exit_bloc = blocRef(new bloc(BB_EXIT));
   sl->list_of_bloc[BB_ENTRY] = entry_bloc;
   sl->list_of_bloc[BB_EXIT] = exit_bloc;

   function_behavior->UpdateBBVersion();
   return DesignFlowStep_Status::SUCCESS;
}

bool BlockFix::HasToBeExecuted() const
{
   return bb_version == 0 && FunctionFrontendFlowStep::HasToBeExecuted();
}
