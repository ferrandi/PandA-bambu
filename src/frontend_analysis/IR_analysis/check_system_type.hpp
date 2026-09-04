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
 * @file check_system_type.hpp
 * @brief analyse loc_info of variables and types to detect system ones; the identified one are flagged
 *
 * @author Marco Lattuada <lattuada@elet.polimi.it>
 * @author Michele Fiorito <michele.fiorito@polimi.it>
 *
 */
#ifndef CHECK_SYSTEM_TYPE_HPP
#define CHECK_SYSTEM_TYPE_HPP

#include "custom_map.hpp"
#include "custom_set.hpp"
#include "function_frontend_flow_step.hpp"
#include "refcount.hpp"

#include <string>
#include <vector>

CONSTREF_FORWARD_DECL(BehavioralHelper);
REF_FORWARD_DECL(ir_manager);
REF_FORWARD_DECL(ir_node);

/**
 * Class which system_flag to ir_node of variables and types when necessary
 */
class CheckSystemType : public FunctionFrontendFlowStep
{
 private:
   /// The helper associated with the current function
   const BehavioralHelperConstRef behavioral_helper;

   /// The IR manager
   const ir_managerRef TM;

   /**
    * Examinate recursively the IR to detect system types and system variables
    * @param tn is the root of the IR subtree to be examinated; it must be a ir_reindex
    * @param already_visited stores the IR nodes already visited during the recursive walk
    */
   void recursive_examinate(const ir_nodeRef& tn, CustomUnorderedSet<unsigned int>& already_visited) const;

   CustomUnorderedSet<std::pair<FrontendFlowStepType, FunctionRelationship>>
   ComputeFrontendRelationships(const DesignFlowStep::RelationshipType relationship_type) const override;

 public:
   CheckSystemType(const ParameterConstRef _parameters, const application_managerRef AppM, unsigned int function_id,
                   const DesignFlowManager& design_flow_manager);

   DesignFlowStep_Status InternalExec() override;

   bool HasToBeExecuted() const override;
};
#endif
