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
 *              Copyright (C) 2015-2024 Politecnico di Milano
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
#include "functional_unit_step.hpp"
#include "refcount.hpp"
#include "to_data_file_step.hpp"

REF_FORWARD_DECL(generic_device);

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

   void ComputeRelationships(DesignFlowStepSet& relationship,
                             const DesignFlowStep::RelationshipType relationship_type) override;

 public:
   /**
    * The constructor
    * @param _device is the device
    * @param design_flow_manager is the design flow manager
    * @param parameters is the set of input parameters
    */
   GenerateFuList(const generic_deviceRef _device, const DesignFlowManagerConstRef design_flow_manager,
                  const ParameterConstRef parameters);

   DesignFlowStep_Status Exec() override;

   bool HasToBeExecuted() const override;

   DesignFlowStepFactoryConstRef CGetDesignFlowStepFactory() const override;

   void AnalyzeCell(functional_unit* fu, const unsigned int prec, const std::vector<std::string>& portsize_parameters,
                    const size_t portsize_index, const std::vector<std::string>& pipe_parameters,
                    const size_t stage_index, const unsigned int constPort, const bool is_commutative,
                    size_t max_lut_size) override;
};
#endif
