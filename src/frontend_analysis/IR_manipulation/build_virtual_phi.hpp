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
 * @file build_virtual_phi.hpp
 * @brief Analysis step building phi of vops
 *
 * @author Marco Lattuada <marco.lattuada@polimi.it>
 *
 */
#ifndef BUILD_VIRTUAL_PHI_HPP
#define BUILD_VIRTUAL_PHI_HPP
#include "function_frontend_flow_step.hpp"

#include "refcount.hpp"

REF_FORWARD_DECL(ir_manager);

class BuildVirtualPhi : public FunctionFrontendFlowStep
{
   const ir_managerRef TM;

   CustomUnorderedSet<std::pair<FrontendFlowStepType, FunctionRelationship>>
   ComputeFrontendRelationships(const DesignFlowStep::RelationshipType relationship_type) const override;

 public:
   BuildVirtualPhi(const application_managerRef AppM, unsigned int function_id,
                   const DesignFlowManager& design_flow_manager, const ParameterConstRef parameters);

   DesignFlowStep_Status InternalExec() override;
};
#endif
