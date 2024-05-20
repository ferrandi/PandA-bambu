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
 *              Copyright (C) 2017-2024 Politecnico di Milano
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
 * @file design_flow.hpp
 * @brief This class contains the base representation for design flow
 *
 *
 * @author Marco Lattuada <lattuada@elet.polimi.it>
 *
 */
#ifndef DESIGN_FLOW_HPP
#define DESIGN_FLOW_HPP
#include "design_flow_step.hpp"

#include <string>

enum class DesignFlow_Type
{
   NON_DETERMINISTIC_FLOWS = 0
};

class DesignFlow : public DesignFlowStep
{
 protected:
   /// The type of this design flow
   const DesignFlow_Type design_flow_type;

   void ComputeRelationships(DesignFlowStepSet& relationship,
                             const DesignFlowStep::RelationshipType relationship_type) override;

 public:
   /**
    * Constructor
    * @param design_flow_manager is the design flow manager
    * @param design_flow_type is the type of the flow
    * @param parameters is the set of the parameters
    */
   DesignFlow(const DesignFlowManagerConstRef design_flow_manager, DesignFlow_Type design_flow_type,
              const ParameterConstRef parameters);

   ~DesignFlow() override;

   std::string GetName() const override;

   DesignFlowStepFactoryConstRef CGetDesignFlowStepFactory() const override;

   bool HasToBeExecuted() const override;

   /**
    * Compute the signature of a step
    * @param design_flow_type is the type of design flow
    * @return the signature corresponding to the design flow
    */
   static signature_t ComputeSignature(DesignFlow_Type design_flow_type);

   /**
    * Return the name of the type
    * @param design_flow_type is the type of the design flow
    * @return the name
    */
   static std::string EnumToKindText(const DesignFlow_Type design_flow_type);

   /**
    * Given the name of design flow, return the enum
    * @param name is the name of the design flow
    * @return the corresponding enum
    */
   static DesignFlow_Type KindTextToEnum(const std::string& name);
};
#endif
