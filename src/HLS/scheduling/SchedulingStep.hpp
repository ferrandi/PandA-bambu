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
 * @file SchedulingStep.hpp
 * @brief scheduling base class
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 *
 */
#ifndef SCHEDULING_BASE_STEP_HPP
#define SCHEDULING_BASE_STEP_HPP
#include "hls_function_step.hpp"

#include "custom_map.hpp"
#include "graph.hpp"

class OpGraph;

class SchedulingStep : public HLSFunctionStep
{
 protected:
   virtual HLSRelationships
   ComputeHLSRelationships(const DesignFlowStep::RelationshipType relationship_type) const override;

 public:
   SchedulingStep(
       const ParameterConstRef Param, const HLS_managerRef HLSMgr, unsigned int funId,
       const DesignFlowManager& design_flow_manager, const HLSFlowStep_Type hls_flow_step_type,
       const HLSFlowStepSpecializationConstRef hls_flow_step_specialization = HLSFlowStepSpecializationConstRef());

   virtual ~SchedulingStep() override = default;

   void Initialize() override;

   static void compute_RW_stmts(CustomUnorderedSet<gc_vertex_descriptor>& RW_stmts, const OpGraph& flow_graph,
                                const HLS_managerRef HLSMgr, unsigned function_id);
};
#endif
