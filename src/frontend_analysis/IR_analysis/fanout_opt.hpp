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
 *   Copyright (C) 2017-2026 Politecnico di Milano
 *
 * Part of the PandA Project, under the Apache License v2.0 with LLVM Exceptions.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 *
 */
/**
 * @file fanout_opt.hpp
 * @brief Fanout optimization step.
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 *
 */

#ifndef FANOUT_OPT_HPP
#define FANOUT_OPT_HPP

#include "function_frontend_flow_step.hpp"

REF_FORWARD_DECL(fanout_opt);
REF_FORWARD_DECL(Schedule);
REF_FORWARD_DECL(ir_manager);
REF_FORWARD_DECL(ir_node);

/**
 * @brief fanout_opt analysis
 */
class fanout_opt : public FunctionFrontendFlowStep
{
 private:
   /// The scheduling solution
   ScheduleRef schedule;

   /// IR manager
   const ir_managerRef TM;

   CustomUnorderedSet<std::pair<FrontendFlowStepType, FunctionRelationship>>
   ComputeFrontendRelationships(const DesignFlowStep::RelationshipType relationship_type) const override;

   /// return true in case the use is relevant for the fanout optimization
   bool is_dest_relevant(ir_nodeRef t, bool is_phi);

 public:
   fanout_opt(const ParameterConstRef _parameters, const application_managerRef _AppM, unsigned int function_id,
              const DesignFlowManager& design_flow_manager);

   DesignFlowStep_Status InternalExec() override;

   void Initialize() override;
};

#endif /* FANOUT_OPT_HPP */
