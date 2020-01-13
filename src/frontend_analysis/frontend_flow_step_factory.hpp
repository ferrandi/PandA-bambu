/*
 *
 *                   _/_/_/    _/_/   _/    _/ _/_/_/    _/_/
 *                  _/   _/ _/    _/ _/_/  _/ _/   _/ _/    _/
 *                 _/_/_/  _/_/_/_/ _/  _/_/ _/   _/ _/_/_/_/
 *                _/      _/    _/ _/    _/ _/   _/ _/    _/
 *               _/      _/    _/ _/    _/ _/_/_/  _/    _/
 *
 *             ***********************************************
 *                              PandA Project
 *                     URL: http://panda.dei.polimi.it
 *                       Politecnico di Milano - DEIB
 *                        System Architectures Group
 *             ***********************************************
 *              Copyright (C) 2004-2020 Politecnico di Milano
 *
 *   This file is part of the PandA framework.
 *
 *   The PandA framework is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */
/**
 * @file frontend_flow_step_factor.hpp
 * @brief This class contains the methods to create a frontend flow step
 *
 * @author Marco Lattuada <lattuada@elet.polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *
 */

#ifndef FRONTEND_FLOW_STEP_FACTORY_HPP
#define FRONTEND_FLOW_STEP_FACTORY_HPP

#include "config_HAVE_ARCH_BUILT.hpp" // for HAVE_ARCH_BUILT

#include "custom_set.hpp"               // for unordered_set
#include "design_flow_step.hpp"         // for DesignFlowStep (...
#include "design_flow_step_factory.hpp" // for DesignFlowStepFa...
#include "frontend_flow_step.hpp"       // for FrontendFlowStep...
#include "refcount.hpp"                 // for REF_FORWARD_DECL
#include <string>                       // for string

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

#if HAVE_ARCH_BUILT
   /// The architecture manager
   const ArchManagerRef arch_manager;
#endif

 public:
#if HAVE_ARCH_BUILT
   /**
    * Constructor
    * @param AppM is the application manager
    * @param arch_manager is the architecture manager
    * @param design_flow_manager is the design flow manager
    * @param _Param is the set of the parameters
    */
   FrontendFlowStepFactory(const application_managerRef AppM, const ArchManagerRef arch_manager, const DesignFlowManagerConstRef design_flow_manager, const ParameterConstRef parameters);
#endif

   /**
    * Constructor
    * @param AppM is the application manager
    * @param design_flow_manager is the design flow manager
    * @param _Param is the set of the parameters
    */
   FrontendFlowStepFactory(const application_managerRef AppM, const DesignFlowManagerConstRef design_flow_manager, const ParameterConstRef parameters);

   /**
    * Destructor
    */
   ~FrontendFlowStepFactory() override;

   /**
    * Create the frontend design flow steps
    * @param frontend_flow_step_types is the set of frontend flow transformation to be considered
    */
   const DesignFlowStepSet GenerateFrontendSteps(const CustomUnorderedSet<FrontendFlowStepType>& frontend_flow_step_types) const;

   /**
    * Create the frontend design flow step
    * @param frontend_flow_step_type is the frontend flow to be considered
    */
   const DesignFlowStepRef GenerateFrontendStep(FrontendFlowStepType frontend_flow_step_type) const;

   /**
    * Create an application frontend flow step
    * @param design_flow manager is the design flow manager
    * @param design_flow_step is the type of step to be created
    * @param parameters is the set of input parameters
    */
   const DesignFlowStepRef CreateApplicationFrontendFlowStep(const FrontendFlowStepType design_flow_step_type) const;

   /**
    * Create a function frontend flow step
    * @param design_flow_step is the type of step to be created
    * @param function_id is the index of the function
    */
   const DesignFlowStepRef CreateFunctionFrontendFlowStep(const FrontendFlowStepType design_flow_step_type, const unsigned int function_id) const;

   /**
    * Return the prefix of the steps created by the factory
    */
   const std::string GetPrefix() const override;
};
#endif
