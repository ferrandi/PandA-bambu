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
 * @file CSE.hpp
 * @brief CSE analysis
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 *
 */
#ifndef CSE_HPP
#define CSE_HPP
#include "function_frontend_flow_step.hpp"

#include "custom_map.hpp"
#include "graph.hpp"
#include "ir_common.hpp"
#include "refcount.hpp"

class assign_stmt;
class statement_list_node;
REF_FORWARD_DECL(Schedule);
REF_FORWARD_DECL(ir_manager);
REF_FORWARD_DECL(ir_node);

#if NO_ABSEIL_HASH

/**
 * Definition of hash function for InstrumentInstructionWriter::InstrumentationType
 */
namespace std
{
   template <>
   struct hash<enum kind>
   {
      size_t operator()(enum kind t) const
      {
         hash<int> hasher;
         return hasher(static_cast<int>(t));
      }
   };
} // namespace std
#endif

/**
 * @brief CSE analysis
 */
class CSE : public FunctionFrontendFlowStep
{
   /// define the type of the unique table key
   using CSE_tuple_key_type = std::pair<enum kind, std::vector<unsigned int>>;

   /// The scheduling solution
   ScheduleRef schedule;

   /// IR manager
   const ir_managerRef TM;

   /// when true PHI_OPT step has to restart
   bool restart_phi_opt;

   /// when true logical-condition analyses have to be recomputed
   bool restart_lut_opt;

   /// check if the statement has an equivalent in the unique table
   ir_nodeRef hash_check(
       const ir_nodeRef& tn, gc_vertex_descriptor bb, const statement_list_node* sl,
       std::map<gc_vertex_descriptor, CustomUnorderedMapStable<CSE_tuple_key_type, ir_nodeRef>>& unique_table) const;

   /// check if the assign_stmt is a load, store or a memcpy/memset
   bool has_memory_access(const assign_stmt* ga) const;

   CustomUnorderedSet<std::pair<FrontendFlowStepType, FunctionRelationship>>
   ComputeFrontendRelationships(const DesignFlowStep::RelationshipType relationship_type) const override;

 public:
   CSE(const ParameterConstRef _parameters, const application_managerRef _AppM, unsigned int function_id,
       const DesignFlowManager& design_flow_manager);

   void Initialize() override;

   DesignFlowStep_Status InternalExec() override;
};

#endif /* CSE_HPP */
