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
 *              Copyright (C) 2017-2020 Politecnico di Milano
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
 * @file design_flow_factory.hpp
 * @brief Factory for creating design flows
 *
 * @author Marco Lattuada <lattuada@elet.polimi.it>
 *
 */

#ifndef DESIGN_FLOW_FACTORY_HPP
#define DESIGN_FLOW_FACTORY_HPP

#include "design_flow_step_factory.hpp" // for DesignFlowStepRef, DesignFlo...
#include <string>                       // for string

enum class DesignFlow_Type;

class DesignFlowFactory : public DesignFlowStepFactory
{
 public:
   /**
    * Constructor
    * @param design_flow_manager is the design flow manager
    * @param parameters is the set of input parameters
    */
   DesignFlowFactory(const DesignFlowManagerConstRef design_flow_manager, const ParameterConstRef parameters);

   /**
    * Destructor
    */
   ~DesignFlowFactory() override;

   /**
    * Return the prefix of the steps created by the factory
    */
   const std::string GetPrefix() const override;

   /**
    * Return a step given the signature
    * @param signature is the signature of the step to be created
    * @return the created step
    */
   DesignFlowStepRef CreateFlowStep(const std::string& signature) const override;

   /**
    * Create a design flow
    * @param design_flow_type is the type of design flow to be created
    * @return the step corresponding to the design flow
    */
   const DesignFlowStepRef CreateDesignFlow(const DesignFlow_Type design_flow_type) const;
};
#endif
