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
 *              Copyright (C) 2015-2020 Politecnico di Milano
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
 * @file technology_flow_step_factory.hpp
 * @brief Factory for technology flow step
 *
 * @author Marco Lattuada <marco.lattuada@polimi.it>
 *
 */

#ifndef TECHNOLOGY_FLOW_STEP_FACTORY_HPP
#define TECHNOLOGY_FLOW_STEP_FACTORY_HPP

/// Superclass include
#include "design_flow_step_factory.hpp"

/// utility include
#include "refcount.hpp"

enum class TechnologyFlowStep_Type;

REF_FORWARD_DECL(DesignFlowStep);
REF_FORWARD_DECL(target_device);
REF_FORWARD_DECL(technology_manager);

class TechnologyFlowStepFactory : public DesignFlowStepFactory
{
 protected:
   /// The technology manager
   const technology_managerRef TM;

   /// The target device
   const target_deviceRef target;

 public:
   /**
    * Constructor
    * @param TM is the technology manager
    * @param target is the target device
    * @param design_flow_manager is the design flow manager
    * @param parameters is the set of input parameters
    */
   TechnologyFlowStepFactory(const technology_managerRef TM, const target_deviceRef target, const DesignFlowManagerConstRef design_flow_manager, const ParameterConstRef parameters);

   /**
    * Destructor
    */
   ~TechnologyFlowStepFactory() override;

   /**
    * Return the prefix of the steps created by the factory
    */
   const std::string GetPrefix() const override;

   /**
    * Create a scheduling design flow step
    * @param technology_design_flow_step_type is the type of step to be created
    */
   DesignFlowStepRef CreateTechnologyFlowStep(const TechnologyFlowStep_Type hls_flow_step_type) const;
};
#endif
