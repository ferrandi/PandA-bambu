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
 * @file non_deterministic_flows.cpp
 * @brief Design flow to check different non deterministic flows
 *
 * @author Marco Lattuada <marco.lattuada@polimi.it>
 *
 */

#include "non_deterministic_flows.hpp"
#include "Parameter.hpp"                   // for Parameter, OPT_output_tem...
#include "dbgPrintHelper.hpp"              // for DEBUG_LEVEL_VERY_PEDANTIC
#include "exceptions.hpp"                  // for IsError, THROW_ASSERT
#include "fileIO.hpp"                      // for PandaSystem
#include "string_manipulation.hpp"         // for STR
#include <boost/filesystem/operations.hpp> // for create_directory, exists

const std::string NonDeterministicFlows::ComputeArgString(const size_t seed) const
{
   const auto argv = parameters->CGetArgv();
   std::string arg_string;
   for(const auto& arg : argv)
   {
      /// Executable
      if(arg_string == "")
      {
         THROW_ASSERT(arg.size() and arg[0] == '/', "Relative path executable not supported " + arg);
         arg_string += arg;
      }
      else
      {
         arg_string += " ";
         if(arg.find("--test-single-non-deterministic-flow") == std::string::npos and arg.find("--test-multiple-non-deterministic-flows") == std::string::npos)
         {
            arg_string += arg;
         }
      }
   }
   arg_string += " --test-single-non-deterministic-flow=" + STR(seed);
   return arg_string;
}

bool NonDeterministicFlows::ExecuteTool(const size_t seed) const
{
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Executing with seed " + STR(seed));
   const auto arg_string = ComputeArgString(seed);
   const auto temp_directory = parameters->getOption<std::string>(OPT_output_temporary_directory);
   const auto new_directory = temp_directory + "/" + STR(seed);
   if(boost::filesystem::exists(new_directory))
      boost::filesystem::remove_all(new_directory);
   boost::filesystem::create_directory(new_directory);
   const auto ret = PandaSystem(parameters, "cd " + new_directory + "; " + arg_string, new_directory + "/tool_execution_output");
   if(IsError(ret))
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Failure");
      return false;
   }
   else
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Success");
      return true;
   }
}

NonDeterministicFlows::NonDeterministicFlows(const DesignFlowManagerConstRef _design_flow_manager, const ParameterConstRef _parameters) : DesignFlow(_design_flow_manager, DesignFlow_Type::NON_DETERMINISTIC_FLOWS, _parameters)
{
}

NonDeterministicFlows::~NonDeterministicFlows() = default;

DesignFlowStep_Status NonDeterministicFlows::Exec()
{
   const auto initial_seed = parameters->getOption<size_t>(OPT_seed);
   const auto number_of_runs = parameters->getOption<size_t>(OPT_test_multiple_non_deterministic_flows);
   for(size_t run = 0; run < number_of_runs; run++)
   {
      if(not ExecuteTool(initial_seed + run))
      {
         return DesignFlowStep_Status::ABORTED;
      }
   }
   return DesignFlowStep_Status::SUCCESS;
}
