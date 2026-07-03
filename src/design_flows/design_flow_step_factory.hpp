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
 *              Copyright (C) 2004-2026 Politecnico di Milano
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 *
 *   This file is part of the PandA framework.
 *
 *   Licensed under the Apache License, Version 2.0, with BAMBU exceptions (the "License");
 *   you may not use this file except in compliance with the License.
 *   You may obtain a copy of the License at
 *
 *       http://www.apache.org/licenses/LICENSE-2.0
 *
 *   Unless required by applicable law or agreed to in writing, software
 *   distributed under the License is distributed on an "AS IS" BASIS,
 *   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *   See the License for the specific language governing permissions and
 *   limitations under the License.
 *
 */
/**
 * @file design_flow_step_factory.hpp
 * @brief Pure virtual base class for all the design flow step factory
 *
 * @author Marco Lattuada <lattuada@elet.polimi.it>
 *
 */

#ifndef DESIGN_FLOW_STEP_FACTORY_HPP
#define DESIGN_FLOW_STEP_FACTORY_HPP
#include "design_flow_step.hpp"
#include "refcount.hpp"

#include <string>

class DesignFlowManager;
CONSTREF_FORWARD_DECL(Parameter);

class DesignFlowStepFactory
{
 protected:
   /// The design flow manager
   const DesignFlowManager& design_flow_manager;

   /// The set of input parameters
   const ParameterConstRef parameters;

   /// The debug level
   int debug_level;

   const DesignFlowStep::StepClass step_class;

   /**
    * Constructor
    * @param step_class is the class of steps created by this factory
    * @param design_flow_manager is the design flow manager
    * @param parameters is the set of input parameters
    */
   DesignFlowStepFactory(DesignFlowStep::StepClass step_class, const DesignFlowManager& design_flow_manager,
                         const ParameterConstRef& parameters);

 public:
   virtual ~DesignFlowStepFactory() = default;

   /**
    * Return the class of the steps created by the factory
    */
   inline DesignFlowStep::StepClass GetClass() const
   {
      return step_class;
   }

   /**
    * Return a step given the signature
    * @param signature is the signature of the step to be created
    * @return the created step
    */
   virtual DesignFlowStepRef CreateFlowStep(DesignFlowStep::signature_t signature) const;
};
#endif
