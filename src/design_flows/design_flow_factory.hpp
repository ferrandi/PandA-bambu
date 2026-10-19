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
 *   Copyright (C) 2017-2026 Politecnico di Milano
 *
 * Part of the PandA Project, under the Apache License v2.0 with LLVM Exceptions.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 *
 */
/**
 * @file design_flow_factory.hpp
 * @brief Factory for creating design flows
 *
 * @author Marco Lattuada <lattuada@elet.polimi.it>
 *
 */
#ifndef DESIGN_FLOW_FACTORY_HPP
#define DESIGN_FLOW_FACTORY_HPP
#include "design_flow_step_factory.hpp"

#include <string>

enum class DesignFlow_Type;

class DesignFlowFactory : public DesignFlowStepFactory
{
 public:
   /**
    * Constructor
    * @param design_flow_manager is the design flow manager
    * @param parameters is the set of input parameters
    */
   DesignFlowFactory(const DesignFlowManager& design_flow_manager, const ParameterConstRef parameters);

   DesignFlowStepRef CreateFlowStep(DesignFlowStep::signature_t signature) const override;

   /**
    * Create a design flow
    * @param design_flow_type is the type of design flow to be created
    * @return the step corresponding to the design flow
    */
   DesignFlowStepRef CreateDesignFlow(const DesignFlow_Type design_flow_type) const;
};
#endif
