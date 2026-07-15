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
 * @file standard_hls.hpp
 * @brief Definition of the class to create the structural description of the component
 *
 * @author Christian Pilato <pilato@elet.polimi.it>
 *
 */
#ifndef STANDARD_HLS_HPP
#define STANDARD_HLS_HPP

#include "hls_function_step.hpp"

class standard_hls : public HLSFunctionStep
{
 protected:
   HLSRelationships ComputeHLSRelationships(const DesignFlowStep::RelationshipType relationship_type) const override;

 public:
   standard_hls(const ParameterConstRef _parameters, const HLS_managerRef HLSMgr, unsigned int funId,
                const DesignFlowManager& design_flow_manager);

   DesignFlowStep_Status InternalExec() override;
};
#endif
