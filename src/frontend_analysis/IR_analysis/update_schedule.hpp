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
 *   Copyright (C) 2016-2026 Politecnico di Milano
 *
 * Part of the PandA Project, under the Apache License v2.0 with LLVM Exceptions.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 *
 */
/**
 * @file update_schedule.hpp
 * @brief Analysis step which updates the schedule of all the instructions
 *
 * @author Marco Lattuada <marco.lattuada@polimi.it>
 *
 */
#ifndef UPDATE_SCHEDULE_HPP
#define UPDATE_SCHEDULE_HPP

/// Superclass include
#include "function_frontend_flow_step.hpp"

#include "refcount.hpp"

REF_FORWARD_DECL(Schedule);

/**
 * Update schedule of all the instructions
 */
class UpdateSchedule : public FunctionFrontendFlowStep
{
 protected:
   /// The scheduling solution
   ScheduleRef schedule;

   CustomUnorderedSet<std::pair<FrontendFlowStepType, FunctionRelationship>>
   ComputeFrontendRelationships(const DesignFlowStep::RelationshipType relationship_type) const override;

 public:
   UpdateSchedule(const application_managerRef _AppM, unsigned int function_id,
                  const DesignFlowManager& design_flow_manager, const ParameterConstRef parameters);

   DesignFlowStep_Status InternalExec() override;

   void Initialize() override;

   bool HasToBeExecuted() const override;
};
#endif
