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
 * @file storage_value_insertion.hpp
 * @brief Storage value insertion step suitable for variables with stages
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 *
 */
#ifndef STORAGE_VALUE_INSERTION_HPP
#define STORAGE_VALUE_INSERTION_HPP
#include "hls_function_step.hpp"

class storage_value_insertion : public HLSFunctionStep
{
 protected:
   /**
    * Compute the relationship of this step
    * @param relationship_type is the type of relationship to be considered
    * @return the steps in relationship with this
    */
   HLSRelationships ComputeHLSRelationships(const DesignFlowStep::RelationshipType relationship_type) const override;

 public:
   /**
    * Constructor
    * @param Param is the parameter set
    * @param HLSMgr is the HLS manager
    * @param funId is the function identifier
    * @param design_flow_manager is the design flow manager
    */
   storage_value_insertion(const ParameterConstRef Param, const HLS_managerRef HLSMgr, unsigned int funId,
                           const DesignFlowManager& design_flow_manager);

   /**
    * Initialize the step (i.e., like a constructor, but executed just before exec
    */
   void Initialize() override;

   DesignFlowStep_Status InternalExec() override;
};
#endif
