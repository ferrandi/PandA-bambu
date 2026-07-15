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
 * @file sdc_code_motion.hpp
 * @brief Analysis step performing code motion speculation on the basis of sdc results.
 *
 * @author Marco Lattuada <marco.lattuada@polimi.it>
 *
 */
#ifndef SDC_CODE_MOTION_HPP
#define SDC_CODE_MOTION_HPP
#include "function_frontend_flow_step.hpp"

class SDCCodeMotion : public FunctionFrontendFlowStep
{
 private:
   bool restart_ifmwi_opt;

   CustomUnorderedSet<std::pair<FrontendFlowStepType, FunctionRelationship>>
   ComputeFrontendRelationships(const DesignFlowStep::RelationshipType relationship_type) const override;

 public:
   SDCCodeMotion(const application_managerRef AppM, unsigned int function_id,
                 const DesignFlowManager& design_flow_manager, const ParameterConstRef parameters);

   DesignFlowStep_Status InternalExec() override;

   bool HasToBeExecuted() const override;
};
#endif
