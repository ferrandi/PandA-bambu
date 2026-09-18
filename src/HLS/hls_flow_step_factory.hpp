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
 * @file hls_flow_step_factory.hpp
 * @brief Factory for hls flow step
 *
 * @author Marco Lattuada <lattuada@elet.polimi.it>
 *
 */
#ifndef HLS_FLOW_STEP_FACTORY_HPP
#define HLS_FLOW_STEP_FACTORY_HPP
#include "custom_set.hpp"
#include "design_flow_step.hpp"
#include "design_flow_step_factory.hpp"

REF_FORWARD_DECL(HLS_manager);
CONSTREF_FORWARD_DECL(HLSFlowStepSpecialization);
class xml_element;
enum class HLSFlowStep_Type;

class HLSFlowStepFactory : public DesignFlowStepFactory
{
 protected:
   /// The HLS manager
   const HLS_managerRef HLS_mgr;

   /**
    * Verifies if the current node has to be added to the list of steps
    */
   bool checkNode(const xml_element* node, unsigned int funId, const std::string& ref_step) const;

 public:
   /**
    * Constructor
    * @param design_flow_manager is the design flow manager
    * @param _HLS_mgr is the HLS manager
    * @param parameters is the set of input parameters
    */
   HLSFlowStepFactory(const DesignFlowManager& design_flow_manager, const HLS_managerRef _HLS_mgr,
                      const ParameterConstRef parameters);

   /**
    * Create a scheduling design flow step
    * @param hls_flow_step_type is the type of scheduling step to be created
    * @param funId is the index of the function to be scheduled
    * @param hls_flow_step_specialization contains information about how specialize the single step
    */
   DesignFlowStepRef
   CreateHLSFlowStep(const HLSFlowStep_Type hls_flow_step_type, const unsigned int funId,
                     const HLSFlowStepSpecializationConstRef hls_flow_step_specialization = nullptr) const;

   DesignFlowStepRef
   CreateHLSFlowStep(const HLSFlowStep_Type type,
                     const HLSFlowStepSpecializationConstRef hls_flow_step_specialization = nullptr) const;
};
#endif
