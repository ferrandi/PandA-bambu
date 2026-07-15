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
 * @file frontend_flow_step_factory.hpp
 * @brief This class contains the methods to create a frontend flow step
 *
 * @author Marco Lattuada <lattuada@elet.polimi.it>
 *
 */

#ifndef FRONTEND_FLOW_STEP_FACTORY_HPP
#define FRONTEND_FLOW_STEP_FACTORY_HPP

#include "custom_set.hpp"
#include "design_flow_step.hpp"
#include "design_flow_step_factory.hpp"
#include "frontend_flow_step.hpp"
#include "refcount.hpp"

#include <string>

/// Forward declaration
REF_FORWARD_DECL(application_manager);
REF_FORWARD_DECL(ArchManager);
REF_FORWARD_DECL(DesignFlowManager);
REF_FORWARD_DECL(DesignFlowStep);

class FrontendFlowStepFactory : public DesignFlowStepFactory
{
 protected:
   /// The application manager
   const application_managerRef AppM;

 public:
   /**
    * Constructor
    * @param AppM is the application manager
    * @param design_flow_manager is the design flow manager
    * @param parameters is the set of input parameters
    */
   FrontendFlowStepFactory(const application_managerRef AppM, const DesignFlowManager& design_flow_manager,
                           const ParameterConstRef parameters);

   /**
    * Create the frontend design flow steps
    * @param frontend_flow_step_types is the set of frontend flow transformation to be considered
    */
   DesignFlowStepSet
   GenerateFrontendSteps(const CustomUnorderedSet<FrontendFlowStepType>& frontend_flow_step_types) const;

   /**
    * Create the frontend design flow step
    * @param frontend_flow_step_type is the frontend flow to be considered
    */
   DesignFlowStepRef GenerateFrontendStep(FrontendFlowStepType frontend_flow_step_type) const;

   /**
    * Create an application frontend flow step
    * @param design_flow_step_type is the type of step to be created
    */
   DesignFlowStepRef CreateApplicationFrontendFlowStep(const FrontendFlowStepType design_flow_step_type) const;

   /**
    * Create a function frontend flow step
    * @param design_flow_step_type is the type of step to be created
    * @param function_id is the index of the function
    */
   DesignFlowStepRef CreateFunctionFrontendFlowStep(const FrontendFlowStepType design_flow_step_type,
                                                    const unsigned int function_id) const;
};
#endif
