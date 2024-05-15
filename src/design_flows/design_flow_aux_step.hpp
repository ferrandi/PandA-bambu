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
 *              Copyright (C) 2004-2024 Politecnico di Milano
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
#include "design_flow_step.hpp"

#include <iosfwd>
#include <string>

/// Identifier of the auxiliary design flow steps
using AuxDesignFlowStepType = enum AuxDesignFlowStepType {
   DESIGN_FLOW_ENTRY, //! Entry point for the design flow
   DESIGN_FLOW_EXIT   //! Exit point for the design flow
};

/**
 * Class describing auxiliary steps in design flow
 */
class AuxDesignFlowStep : public DesignFlowStep
{
 private:
   /// The type of this auxiliary design flow step
   const AuxDesignFlowStepType type;

   void ComputeRelationships(DesignFlowStepSet& relationship,
                             const DesignFlowStep::RelationshipType relationship_type) override;

 public:
   /**
    * Constructor
    * @param type is the type of the step
    * @param design_flow_manager is the design flow manager
    * @param parameters is the set of input parameters
    */
   AuxDesignFlowStep(AuxDesignFlowStepType type, const DesignFlowManagerConstRef design_flow_manager,
                     const ParameterConstRef parameters);

   ~AuxDesignFlowStep() override;

   DesignFlowStep_Status Exec() override;

   bool HasToBeExecuted() const override;

   std::string GetName() const override;

   void WriteDot(std::ostream& out) const override;

   DesignFlowStepFactoryConstRef CGetDesignFlowStepFactory() const override;

   /**
    * Compute the signature of a sdf design flow step
    * @param type is the type of auxiliary step
    * @return the signature corresponding to the analysis/transformation
    */
   static signature_t ComputeSignature(AuxDesignFlowStepType type);
};
#endif
