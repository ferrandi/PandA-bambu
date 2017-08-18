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
 *              Copyright (c) 2017 Politecnico di Milano
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

///Superclass include
#include "design_flow_step.hpp"

enum class DesignFlow_Type
{
   NON_DETERMINISTIC_FLOWS
};

class DesignFlow : public DesignFlowStep
{
   protected:
      ///The type of this design flow
      const DesignFlow_Type design_flow_type;

   public:
      /**
       * Constructor
       * @param design_flow_manager is the design flow manager
       * @param design_flow_type is the type of the flow
       * @param _Param is the set of the parameters
       */
      DesignFlow(const DesignFlowManagerConstRef design_flow_manager, const DesignFlow_Type design_flow_type, const ParameterConstRef parameters);

      /**
       * Destructor
       */
      virtual ~DesignFlow();

      /**
       * Execute the step
       * @return the exit status of this step
       */
      virtual DesignFlowStep_Status Exec() = 0;

      /**
       * Compute the relationships of a step with other steps
       * @param dependencies is where relationships will be stored
       * @param relationship_type is the type of relationship to be computed
       */
      virtual void ComputeRelationships(DesignFlowStepSet & relationship, const DesignFlowStep::RelationshipType relationship_type);

      /**
       * Compute the siganture of a step
       * @param design_flow_type is the type of design flow
       * @return the signature corresponding to the design flow
       */
      static
      std::string ComputeSignature(const DesignFlow_Type design_flow_type);

      /**
       * Return the signature of this step
       */
      virtual const std::string GetSignature() const;

      /**
       * Return the name of this design step
       * @return the name of the pass (for debug purpose)
       */
      virtual const std::string GetName() const;

      /**
       * Return the name of the type
       * @param design_flow_type is the type of the design flow
       * @return the name
       */
      static
      const std::string EnumToKindText(const DesignFlow_Type design_flow_type);

      /**
       * Return the factory to create this type of steps
       * @return the factory to create frontend flow step
       */
      const DesignFlowStepFactoryConstRef CGetDesignFlowStepFactory() const;

      /**
       * Given the name of design flow, return the enum
       * @param name is the name of the design flow
       * @return the corresponding enum
       */
      static
      DesignFlow_Type KindTextToEnum(const std::string name);

      /**
       * Check if this step has actually to be executed
       * @return true if the step has to be executed
       */
      virtual bool HasToBeExecuted() const;
};
#endif
