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
 *   Copyright (C) 2015-2026 Politecnico di Milano
 *
 * Part of the PandA Project, under the Apache License v2.0 with LLVM Exceptions.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 *
 */
/**
 *
 * @author Pietro Fezzardi <pietrofezzardi@gmail.com>
 *
 */
#ifndef CALL_GRAPH_UNFOLDING_HPP
#define CALL_GRAPH_UNFOLDING_HPP

// include superclass header
#include "hls_step.hpp"

class CallGraphUnfolding : public HLS_step
{
 public:
   DesignFlowStep_Status Exec() override;

   HLSRelationships ComputeHLSRelationships(const DesignFlowStep::RelationshipType relationship_type) const override;

   CallGraphUnfolding(const ParameterConstRef Param, const HLS_managerRef HLSMgr,
                      const DesignFlowManager& design_flow_manager);

   bool HasToBeExecuted() const override;
};
#endif
