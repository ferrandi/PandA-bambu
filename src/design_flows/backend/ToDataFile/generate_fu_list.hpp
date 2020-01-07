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
 * @file generate_fu_list.hpp
 * @brief Class for generating the list of functional untis to be characterized
 *
 * @author Marco Lattuada <marco.lattuada@polimi.it>
 *
 */
#ifndef GENERATE_FU_LIST_HPP
#define GENERATE_FU_LIST_HPP

/// Superclass include
#include "functional_unit_step.hpp"
#include "to_data_file_step.hpp"

/// utility include
#include "refcount.hpp"

REF_FORWARD_DECL(target_manager);

class GenerateFuList : public ToDataFileStep, public FunctionalUnitStep
{
 private:
   /// The set of list of cells
   CustomOrderedSet<std::string> cells;

   /// The list of components to be added to the list; if the list is empty, all the components will be added
   CustomOrderedSet<std::string> components_to_be_characterized;

   /// The current functional unit
   std::string component;

   /// The current entry for list of functional units
   std::string current_list;

 public:
   /**
    * The constructor
    * @param target is the target manager
    * @param design_flow_manager is the design flow manager
    * @param parameters is the set of input parameters
    */
   GenerateFuList(const target_managerRef target, const DesignFlowManagerConstRef design_flow_manager, const ParameterConstRef parameters);

   /**
    * Execute the step
    * @return the exit status of this step
    */
   DesignFlowStep_Status Exec() override;

   /**
    * Compute the relationships of a step with other steps
    * @param dependencies is where relationships will be stored
    * @param relationship_type is the type of relationship to be computed
    */
   void ComputeRelationships(DesignFlowStepSet& relationship, const DesignFlowStep::RelationshipType relationship_type) override;

   /**
    * Check if this step has actually to be executed
    * @return true if the step has to be executed
    */
   bool HasToBeExecuted() const override;

   /**
    * Return a unified identifier of this design step
    * @return the signature of the design step
    */
   const std::string GetSignature() const override;

   /**
    * Return the name of this design step
    * @return the name of the pass (for debug purpose)
    */
   const std::string GetName() const override;

   /**
    * Return the factory to create this type of steps
    */
   const DesignFlowStepFactoryConstRef CGetDesignFlowStepFactory() const override;

   /**
    * Analyze the single cell
    * @param fu is the cell
    * @param prec is the precision
    * @param portsize_parameters is the size of parameters
    * @param portsize_index
    * @param pipe_parameters
    * @param stage_index
    * @param constPort is the index of the constant port
    */
   void AnalyzeCell(functional_unit* fu, const unsigned int prec, const std::vector<std::string>& portsize_parameters, const size_t portsize_index, const std::vector<std::string>& pipe_parameters, const size_t stage_index, const unsigned int constPort,
                    const bool is_commutative, size_t max_lut_size) override;
};
#endif
