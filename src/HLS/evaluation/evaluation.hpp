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
 * @file evaluation.hpp
 * @brief Class to compute evaluations about high-level synthesis
 *
 *
 * @author Christian Pilato <pilato@elet.polimi.it>
 *
 */
#ifndef EVALUATION_HPP
#define EVALUATION_HPP
#include "hls_step.hpp"

class Evaluation final : public HLS_step
{
   HLSRelationships ComputeHLSRelationships(const DesignFlowStep::RelationshipType relationship_type) const override;

 public:
   Evaluation(const ParameterConstRef _parameters, const HLS_managerRef HLSMgr,
              const DesignFlowManager& design_flow_manager);

   bool HasToBeExecuted() const override;

   DesignFlowStep_Status Exec() override final;
};
#endif
