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
 * @file generate_simulation_scripts.hpp
 * @brief Wrapper used to generate simulation scripts
 *
 * @author Marco Lattuada <marco.lattuada@polimi.it>
 *
 */
#ifndef GENERATE_SIMULATION_SCRIPTS_HPP
#define GENERATE_SIMULATION_SCRIPTS_HPP
#include "hls_step.hpp"

#include <filesystem>

class GenerateSimulationScripts : public HLS_step
{
   const std::filesystem::path _c_testbench;

   const std::filesystem::path _c_driver_wrapper;

   HLSRelationships ComputeHLSRelationships(const DesignFlowStep::RelationshipType relationship_type) const override;

   void ComputeRelationships(DesignFlowStepSet& design_flow_step_set,
                             const DesignFlowStep::RelationshipType relationship_type) override;

 public:
   GenerateSimulationScripts(const ParameterConstRef _parameters, const HLS_managerRef HLSMgr,
                             const DesignFlowManagerConstRef design_flow_manager);

   DesignFlowStep_Status Exec() override;

   bool HasToBeExecuted() const override;
};
#endif
