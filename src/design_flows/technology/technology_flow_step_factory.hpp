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
 * @file technology_flow_step_factory.hpp
 * @brief Factory for technology flow step
 *
 * @author Marco Lattuada <marco.lattuada@polimi.it>
 *
 */

#ifndef TECHNOLOGY_FLOW_STEP_FACTORY_HPP
#define TECHNOLOGY_FLOW_STEP_FACTORY_HPP

#include "design_flow_step_factory.hpp"

#include "refcount.hpp"

enum class TechnologyFlowStep_Type;

REF_FORWARD_DECL(DesignFlowStep);
REF_FORWARD_DECL(generic_device);
REF_FORWARD_DECL(technology_manager);

class TechnologyFlowStepFactory : public DesignFlowStepFactory
{
 protected:
   /// The technology manager
   const technology_managerRef TM;

   /// The target device
   const generic_deviceRef target;

 public:
   /**
    * Constructor
    * @param TM is the technology manager
    * @param target is the target device
    * @param design_flow_manager is the design flow manager
    * @param parameters is the set of input parameters
    */
   TechnologyFlowStepFactory(const technology_managerRef TM, const generic_deviceRef target,
                             const DesignFlowManager& design_flow_manager, const ParameterConstRef parameters);

   /**
    * Create a scheduling design flow step
    * @param technology_flow_step_type is the type of step to be created
    */
   DesignFlowStepRef CreateTechnologyFlowStep(const TechnologyFlowStep_Type technology_flow_step_type) const;
};
#endif
