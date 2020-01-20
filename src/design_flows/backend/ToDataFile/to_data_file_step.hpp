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
 * @file to_data_file_step.hpp
 * @brief Base class for data backend
 *
 * @author Marco Lattuada <marco.lattuada@polimi.it>
 *
 */
#ifndef TO_DATA_FILE_STEP_HPP
#define TO_DATA_FILE_STEP_HPP

/// Autoheader include
#include "config_HAVE_CIRCUIT_BUILT.hpp"

/// Superclass include
#include "design_flow_step.hpp"

/// STD include
#include <string>

/// utility include
#include "refcount.hpp"

enum class ToDataFileStep_Type
{
   UNKNOWN = 0,
#if HAVE_CIRCUIT_BUILT
   GENERATE_FU_LIST,
#endif
};

class ToDataFileStep : public virtual DesignFlowStep
{
 protected:
   /// The type of step
   ToDataFileStep_Type to_data_file_step_type;

 public:
   /**
    * Constructor
    * @param design_flow_manager is the design flow manager
    * @param to_data_file_step is the type of this step
    * @param parameters is the set of input parameters
    */
   ToDataFileStep(const DesignFlowManagerConstRef design_flow_manager, const ToDataFileStep_Type to_data_file_step, const ParameterConstRef parameters);

   /**
    * Return a unified identifier of this design step
    * @return the signature of the design step
    */
   const std::string GetSignature() const override;

   /**
    * Given a to data file step type, return the name of the type
    * @param to_data_file_step is the type to be considered
    * @return the name of the type
    */
   static const std::string EnumToName(const ToDataFileStep_Type to_data_file_step);

   /**
    * Given the name of data file step type, return the enum
    * @param to_data_file_step_type is the type to be considered
    * @return the name of the type
    */
   static ToDataFileStep_Type NameToEnum(const std::string& to_data_file_step);

   /**
    * Compute the signature of a to data file step
    * @param to_data_file_step_type is the type of the step
    * @return the corresponding signature
    */
   static const std::string ComputeSignature(const ToDataFileStep_Type to_data_file_step_type);

   /**
    * Return the name of this design step
    * @return the name of the pass (for debug purpose)
    */
   const std::string GetName() const override;

   /**
    * Compute the relationships of a step with other steps
    * @param dependencies is where relationships will be stored
    * @param relationship_type is the type of relationship to be computed
    */
   void ComputeRelationships(DesignFlowStepSet& relationship, const DesignFlowStep::RelationshipType relationship_type) override = 0;

   /**
    * Return the factory to create this type of steps
    */
   const DesignFlowStepFactoryConstRef CGetDesignFlowStepFactory() const override;

   /**
    * Check if this step has actually to be executed
    * @return true if the step has to be executed
    */
   bool HasToBeExecuted() const override;
};
#endif
