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
 *              Copyright (C) 2016-2026 Politecnico di Milano
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
 * @file find_max_transformations.cpp
 * @brief Analysis step to find transformation which breaks synthesis flow by launching bambu with different values of
 * --max-transformations
 *
 * @author Marco Lattuada <marco.lattuada@polimi.it>
 *
 */
#include "find_max_transformations.hpp"

#include "Parameter.hpp"
#include "dbgPrintHelper.hpp"
#include "exceptions.hpp"
#include "fileIO.hpp"
#include "hash_helper.hpp"
#include "string_manipulation.hpp"

#include <filesystem>

FindMaxTransformations::FindMaxTransformations(const application_managerRef _AppM,
                                               const DesignFlowManager& _design_flow_manager,
                                               const ParameterConstRef _parameters)
    : ApplicationFrontendFlowStep(_AppM, FrontendFlowStepType::FIND_MAX_TRANSFORMATIONS, _design_flow_manager,
                                  _parameters)
{
   debug_level = parameters->get_class_debug_level(GET_CLASS(*this));
}

CustomUnorderedSet<std::pair<FrontendFlowStepType, FrontendFlowStep::FunctionRelationship>>
FindMaxTransformations::ComputeFrontendRelationships(const DesignFlowStep::RelationshipType) const
{
   return CustomUnorderedSet<std::pair<FrontendFlowStepType, FrontendFlowStep::FunctionRelationship>>();
}

const std::string FindMaxTransformations::ComputeArgString(const size_t max_transformations) const
{
   const auto argv = parameters->CGetArgv();
   std::string arg_string;
   bool first_argument = true;
   for(const auto& arg : argv)
   {
      /// Executable
      if(first_argument)
      {
         THROW_ASSERT(arg.size() and arg[0] == '/', "Relative path executable not supported " + arg);
         arg_string += shell_escape_argument(arg);
         first_argument = false;
      }
      else
      {
         if(arg.find("--max-transformations") == std::string::npos and
            arg.find("--find-max-transformations") == std::string::npos)
         {
            arg_string += " " + shell_escape_argument(arg);
         }
      }
   }
   arg_string += " " + shell_escape_argument("--max-transformations=" + STR(max_transformations));
   return arg_string;
}

bool FindMaxTransformations::ExecuteBambu(const size_t max_transformations) const
{
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                  "-->Executing with max transformations " + STR(max_transformations));
   const auto arg_string = ComputeArgString(max_transformations);
   const auto new_directory =
       parameters->getOption<std::filesystem::path>(OPT_output_temporary_directory) / STR(max_transformations);
   if(std::filesystem::exists(new_directory))
   {
      std::filesystem::remove_all(new_directory);
   }
   std::filesystem::create_directory(new_directory);
   if(IsError(PandaSystem(parameters, "cd " + shell_escape_argument(new_directory.string()) + "; " + arg_string, false,
                          new_directory / "bambu_execution_output")))
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Failure");
      return false;
   }
   std::filesystem::remove_all(new_directory);
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Success");
   return true;
}

DesignFlowStep_Status FindMaxTransformations::Exec()
{
   size_t correct_cmt = 1;
   size_t wrong_cmt = 0;
   if(parameters->IsParameter("wrong-transformation"))
   {
      wrong_cmt = parameters->GetParameter<unsigned int>("wrong-transformation");
   }
   if(parameters->IsParameter("correct-transformation"))
   {
      correct_cmt = parameters->GetParameter<unsigned int>("correct-transformation");
   }

   if(!wrong_cmt)
   {
      /// First check
      const auto zero_execution = ExecuteBambu(0);
      if(not zero_execution)
      {
         INDENT_OUT_MEX(0, 0, "Bambu fails with --max-transformations=0");
         return DesignFlowStep_Status::ABORTED;
      }
      INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, output_level, "-->Looking for upper bound of max transformations");

      while(ExecuteBambu(correct_cmt))
      {
         correct_cmt *= 2;
      }
      INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, output_level, "<--Upper bound is " + STR(correct_cmt));
      wrong_cmt = correct_cmt;
      correct_cmt = wrong_cmt / 2;
   }
   while(wrong_cmt - correct_cmt > 1)
   {
      INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, output_level,
                     "---Current range is [" + STR(correct_cmt) + ":" + STR(wrong_cmt) + "]");
      size_t middle_cmt = (wrong_cmt + correct_cmt) / 2;
      const auto middle_execution = ExecuteBambu(middle_cmt);
      if(middle_execution)
      {
         correct_cmt = middle_cmt;
      }
      else
      {
         wrong_cmt = middle_cmt;
      }
   }
   INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, output_level, "---Error occours at " + STR(wrong_cmt) + " transformation");
   return DesignFlowStep_Status::ABORTED;
}
