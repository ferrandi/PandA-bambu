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
 *   Copyright (C) 2023-2026 Politecnico di Milano
 *
 * Part of the PandA Project, under the Apache License v2.0 with LLVM Exceptions.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 *
 */
/**
 * @file dominator_allocation.hpp
 * @brief Composed pass to wrap function and memory dominator allocation
 *
 * @author Michele Fiorito <michele.fiorito@polimi.it>
 *
 */

#ifndef DOMINATOR_ALLOCATION_HPP
#define DOMINATOR_ALLOCATION_HPP

#include "hls_step.hpp"

class dominator_allocation : public HLS_step
{
 protected:
   HLSRelationships ComputeHLSRelationships(const DesignFlowStep::RelationshipType relationship_type) const override;

 public:
   dominator_allocation(const ParameterConstRef _parameters, const HLS_managerRef HLSMgr,
                        const DesignFlowManager& design_flow_manager);

   bool HasToBeExecuted() const override;

   DesignFlowStep_Status Exec() override;
};

#endif