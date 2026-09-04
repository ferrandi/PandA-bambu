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
 * @file function_allocation.hpp
 * @brief Base class to allocate functions in high-level synthesis
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 *
 */

#ifndef FUNCTION_ALLOCATION_HPP
#define FUNCTION_ALLOCATION_HPP

#include "hls_step.hpp"
REF_FORWARD_DECL(function_allocation);

/**
 * Allocation function class
 */
class function_allocation : public HLS_step
{
 protected:
   virtual HLSRelationships
   ComputeHLSRelationships(const DesignFlowStep::RelationshipType relationship_type) const override;

 public:
   function_allocation(const ParameterConstRef Param, const HLS_managerRef HLSMgr,
                       const DesignFlowManager& design_flow_manager, const HLSFlowStep_Type hls_flow_step_type);

   /**
    * Initialize the step (i.e., like a constructor, but executed just before exec
    */
   void Initialize() override;
};
#endif
