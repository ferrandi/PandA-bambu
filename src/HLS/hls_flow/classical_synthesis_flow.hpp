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
 * @file classical_synthesis_flow.hpp
 * @brief Classical synthesis flow
 *
 * It performs the memory allocation and, then, the separated synthesis of each of the functions, in
 * topological order
 *
 * @author Christian Pilato <pilato@elet.polimi.it>
 *
 */
#ifndef CLASSICAL_SYNTHESIS_FLOW_HPP
#define CLASSICAL_SYNTHESIS_FLOW_HPP
#include "hls_step.hpp"

#include "refcount.hpp"

class ClassicalHLSSynthesisFlow : public HLS_step
{
 protected:
   /**
    * Return the set of analyses in relationship with this design step
    * @param relationship_type is the type of relationship to be considered
    */
   HLSRelationships ComputeHLSRelationships(const DesignFlowStep::RelationshipType relationship_type) const override;

 public:
   /**
    * Constructor
    * @param _parameters is the set of input parameters
    * @param HLSMgr is the HLS manager
    * @param design_flow_manager is the design flow manager
    */
   ClassicalHLSSynthesisFlow(const ParameterConstRef _parameters, const HLS_managerRef HLSMgr,
                             const DesignFlowManager& design_flow_manager);

   /**
    * Execute the step
    * @return the exit status of this step
    */
   DesignFlowStep_Status Exec() override;

   /**
    * Check if this step has actually to be executed
    * @return true if the step has to be executed
    */
   bool HasToBeExecuted() const override;
};
#endif
