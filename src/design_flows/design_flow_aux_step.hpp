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
 *              Copyright (c) 2004-2017 Politecnico di Milano
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
 * @file design_flow_aux_step.hpp
 * @brief Class for describing auxiliary steps in design flow
 *
 * @author Marco Lattuada <lattuada@elet.polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *
*/

#ifndef DESIGN_FLOW_AUX_STEP_HPP
#define DESIGN_FLOW_AUX_STEP_HPP

///Superclass include
#include "design_flow_step.hpp"

///Identifier of the auxiliary design flow steps
typedef enum
{
   DESIGN_FLOW_ENTRY, //!Entry point for the design flow
   DESIGN_FLOW_EXIT   //!Exit point for the design flow
} AuxDesignFlowStepType;

/**
 * Class describing auxiliary steps in design flow
 */
class AuxDesignFlowStep : public DesignFlowStep
{
   private:
      ///The type of this auxiliary design flow step
      const AuxDesignFlowStepType type;

      ///The name of this auxiliary design flow step
      const std::string name;

   public:
      /**
       * Constructor
       * @param name is the name of the step
       * @param type is the type of the step
       * @param design_flow_manager is the design flow manager
       * @param paramters is the set of input parameters
       */
      AuxDesignFlowStep(const std::string name, const AuxDesignFlowStepType type, const DesignFlowManagerConstRef design_flow_manager, const ParameterConstRef parameters);

      /**
       * Destructor
       */
      ~AuxDesignFlowStep();

      /**
       * Execute the step
       * @return the exit status of this step
       */
      virtual DesignFlowStep_Status Exec();

      /**
       * Check if this step has actually to be executed
       * @return true if the step has to be executed
       */
      virtual bool HasToBeExecuted() const;

      /**
       * Return a unified identifier of this design step
       * @return the signature of the design step
       */
      virtual const std::string GetSignature() const;

      /**
       * Return the name of this design step
       * @return the name of the pass (for debug purpose)
       */
      virtual const std::string GetName() const;

      /**
       * Compute the relationships of a step with other steps
       * @param design_flow is the design flow graph
       * @param dependencies is where relationships will be stored
       * @param relationship_type is the type of relationship to be computed
       */
      virtual void ComputeRelationships(DesignFlowStepSet & relationship, const DesignFlowStep::RelationshipType relationship_type);

      /**
       * Compute the signature of a sdf design flow step
       * @param name is the name of the step
       * @param type is the type of auxiliary step
       * @return the signature corresponding to the analysis/transformation
       */
      static
      const std::string ComputeSignature(const std::string name, const AuxDesignFlowStepType type);

      /**
       * Write the label for a dot graph
       * @param out is the stream where label has to be printed
       */
      virtual void WriteDot(std::ostream & out) const;

      /**
       * Return the factory to create this type of steps
       */
      const DesignFlowStepFactoryConstRef CGetDesignFlowStepFactory() const;
};
#endif
