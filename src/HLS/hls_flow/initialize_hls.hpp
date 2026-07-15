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
 * @file initialize_hls.hpp
 * @brief Step which intializes hls data structur
 *
 * @author Marco Lattuada <marco.lattuada@polimi.it>
 *
 */
#ifndef INITIALIZE_HLS_HPP
#define INITIALIZE_HLS_HPP

#include "hls_function_step.hpp"

class InitializeHLS : public HLSFunctionStep
{
 private:
   void ComputeRelationships(DesignFlowStepSet& relationship,
                             const DesignFlowStep::RelationshipType relationship_type) final;

 public:
   InitializeHLS(const ParameterConstRef _parameters, const HLS_managerRef _HLS_mgr, unsigned int _function_id,
                 const DesignFlowManager& design_flow_manager);

   void Initialize() override;

   DesignFlowStep_Status InternalExec() override;
};
#endif
