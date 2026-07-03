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
 * @file mem_dominator_allocation.hpp
 * @brief Class to allocate memories in HLS based on dominators
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 *
 */
#ifndef MEMORY_DOMINATOR_ALLOCATION_HPP
#define MEMORY_DOMINATOR_ALLOCATION_HPP
#include "memory_allocation.hpp"

#include "custom_set.hpp"

#include <map>
#include <set>
#include <string>

REF_FORWARD_DECL(ir_manager);
CONSTREF_FORWARD_DECL(CallGraphManager);

class mem_dominator_allocation : public memory_allocation
{
 protected:
   /// user defined base address
   unsigned long long int user_defined_base_address;

   std::map<std::string, std::set<std::string>> user_internal_objects;

   std::map<std::string, std::set<std::string>> user_external_objects;

   /// function checking if the current variable has to allocated inside the accelerator or outside
   virtual bool is_internal_obj(unsigned int var_id, unsigned int fun_id, bool multiple_top_call_graph);

 public:
   mem_dominator_allocation(const ParameterConstRef _parameters, const HLS_managerRef HLSMgr,
                            const DesignFlowManager& design_flow_manager,
                            const HLSFlowStepSpecializationConstRef hls_flow_step_specialization,
                            const HLSFlowStep_Type hls_flow_step_type = HLSFlowStep_Type::DOMINATOR_MEMORY_ALLOCATION);

   virtual ~mem_dominator_allocation() override = default;

   virtual DesignFlowStep_Status InternalExec() override;

   void Initialize() override;
};

#endif
