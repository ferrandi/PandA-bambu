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
 * @file fun_dominator_allocation.hpp
 * @brief Class to allocate function in HLS based on dominators
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 *
 */

#ifndef FUN_DOMINATOR_ALLOCATION_HPP
#define FUN_DOMINATOR_ALLOCATION_HPP

#include "function_allocation.hpp"

class fun_dominator_allocation : public function_allocation
{
 protected:
   /// True if this step has yet been executed
   bool already_executed;

   /// list of trivial function that does not require to be proxied
   static const std::set<std::string> simple_functions;

 public:
   fun_dominator_allocation(
       const ParameterConstRef _parameters, const HLS_managerRef HLSMgr, const DesignFlowManager& design_flow_manager,
       const HLSFlowStep_Type hls_flow_step_type = HLSFlowStep_Type::DOMINATOR_FUNCTION_ALLOCATION);

   DesignFlowStep_Status Exec() override;

   void ComputeRelationships(DesignFlowStepSet& relationship,
                             const DesignFlowStep::RelationshipType relationship_type) override;

   bool HasToBeExecuted() const override;
};
#endif
