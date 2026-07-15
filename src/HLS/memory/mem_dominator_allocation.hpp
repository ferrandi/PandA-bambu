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
