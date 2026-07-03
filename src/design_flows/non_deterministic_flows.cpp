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
 *              Copyright (C) 2017-2026 Politecnico di Milano
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 *
 *   This file is part of the PandA framework.
 *
 *   Licensed under the Apache License, Version 2.0, with BAMBU exceptions (the "License");
 *   you may not use this file except in compliance with the License.
 *   You may obtain a copy of the License at
 *
 *       http://www.apache.org/licenses/LICENSE-2.0
 *
 *   Unless required by applicable law or agreed to in writing, software
 *   distributed under the License is distributed on an "AS IS" BASIS,
 *   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *   See the License for the specific language governing permissions and
 *   limitations under the License.
 *
 */
/**
 * @file non_deterministic_flows.cpp
 * @brief Design flow to check different non deterministic flows
 *
 * @author Marco Lattuada <marco.lattuada@polimi.it>
 *
 */
#include "non_deterministic_flows.hpp"

#include "Parameter.hpp"
#include "dbgPrintHelper.hpp"
#include "exceptions.hpp"
#include "fileIO.hpp"
#include "string_manipulation.hpp"

#include <filesystem>

std::string NonDeterministicFlows::ComputeArgString(uint_fast32_t seed) const
{
   const auto argv = parameters->CGetArgv();
   std::string arg_string;
   for(const auto& arg : argv)
   {
      /// Executable
      if(arg_string.empty())
      {
         THROW_ASSERT(!arg.empty() && arg[0] == '/', "Relative path executable not supported " + arg);
         arg_string += arg;
      }
      else
      {
         arg_string += " ";
         if(arg.find("--test-single-non-deterministic-flow") == std::string::npos &&
            arg.find("--test-multiple-non-deterministic-flows") == std::string::npos)
         {
            arg_string += arg;
         }
      }
   }
   arg_string += " --test-single-non-deterministic-flow=" + STR(seed);
   return arg_string;
}

bool NonDeterministicFlows::ExecuteTool(uint_fast32_t seed) const
{
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Executing with seed " + STR(seed));
   const auto arg_string = ComputeArgString(seed);
   const auto new_directory = parameters->getOption<std::filesystem::path>(OPT_output_temporary_directory) / STR(seed);
   if(std::filesystem::exists(new_directory))
   {
      std::filesystem::remove_all(new_directory);
   }
   std::filesystem::create_directory(new_directory);
   if(IsError(PandaSystem(parameters, "cd " + new_directory.string() + "; " + arg_string, false,
                          new_directory / "tool_execution_output")))
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Failure");
      return false;
   }
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Success");
   return true;
}

NonDeterministicFlows::NonDeterministicFlows(const DesignFlowManager& _design_flow_manager,
                                             const ParameterConstRef _parameters)
    : DesignFlow(_design_flow_manager, DesignFlow_Type::NON_DETERMINISTIC_FLOWS, _parameters)
{
}

DesignFlowStep_Status NonDeterministicFlows::Exec()
{
   const auto initial_seed = parameters->getOption<uint_fast32_t>(OPT_seed);
   const auto number_of_runs = parameters->getOption<uint_fast32_t>(OPT_test_multiple_non_deterministic_flows);
   for(uint_fast32_t run = initial_seed; run < number_of_runs; ++run)
   {
      if(!ExecuteTool(run))
      {
         return DesignFlowStep_Status::ABORTED;
      }
   }
   return DesignFlowStep_Status::SUCCESS;
}
