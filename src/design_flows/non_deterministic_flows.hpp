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
 * @file non_deterministic_flows.hpp
 * @brief Design flow to check different non deterministic flows
 *
 * @author Marco Lattuada <marco.lattuada@polimi.it>
 *
 */
#ifndef NON_DETERMINISTIC_FLOWS_HPP
#define NON_DETERMINISTIC_FLOWS_HPP

#include "design_flow.hpp"      // for DesignFlow
#include "design_flow_step.hpp" // for DesignFlowManagerConstRef, DesignFlo...
#include <cstddef>              // for size_t
#include <string>               // for string

/**
 * Class to test non deterministic flows
 */
class NonDeterministicFlows : public DesignFlow
{
 private:
   /**
    * Compute the arg list string of the tool
    * @param seed is the seed to be passed
    * @return the argument string
    */
   const std::string ComputeArgString(const size_t seed) const;

   /**
    * Execute tool with non deterministic flow
    * @param seed is the seed to be passed
    * @return true if the execution was successful, false otherwise
    */
   bool ExecuteTool(const size_t seed) const;

 public:
   /**
    * Constructor.
    * @param design_flow_manager is the design flow manager
    * @param parameters is the set of the parameters
    */
   NonDeterministicFlows(const DesignFlowManagerConstRef design_flow_manager, const ParameterConstRef parameters);

   /**
    *  Destructor
    */
   ~NonDeterministicFlows() override;

   /**
    * Execute the flow
    * @return the exit status of this step
    */
   DesignFlowStep_Status Exec() override;
};
#endif
