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
 *              Copyright (C) 2016-2020 Politecnico di Milano
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
 * @file find_max_cfg_transformations.cpp
 * @brief Analysis step to find transformation which breaks synthesis flow by launching bambu with different values of --cfg-max-transformations
 *
 * @author Marco Lattuada <marco.lattuada@polimi.it>
 *
 */
#include "find_max_cfg_transformations.hpp"
#include "Parameter.hpp"                   // for Parameter, OPT_output_tem...
#include "dbgPrintHelper.hpp"              // for INDENT_OUT_MEX, OUTPUT_LE...
#include "exceptions.hpp"                  // for IsError, THROW_ASSERT
#include "fileIO.hpp"                      // for PandaSystem
#include "hash_helper.hpp"                 // for hash
#include "string_manipulation.hpp"         // for STR GET_CLASS
#include <boost/filesystem/operations.hpp> // for create_directory, exists

FindMaxCFGTransformations::FindMaxCFGTransformations(const application_managerRef _AppM, const DesignFlowManagerConstRef _design_flow_manager, const ParameterConstRef _parameters)
    : ApplicationFrontendFlowStep(_AppM, FrontendFlowStepType::FIND_MAX_CFG_TRANSFORMATIONS, _design_flow_manager, _parameters)
{
   debug_level = parameters->get_class_debug_level(GET_CLASS(*this));
}

FindMaxCFGTransformations::~FindMaxCFGTransformations() = default;

const CustomUnorderedSet<std::pair<FrontendFlowStepType, FrontendFlowStep::FunctionRelationship>> FindMaxCFGTransformations::ComputeFrontendRelationships(const DesignFlowStep::RelationshipType) const
{
   return CustomUnorderedSet<std::pair<FrontendFlowStepType, FrontendFlowStep::FunctionRelationship>>();
}

const std::string FindMaxCFGTransformations::ComputeArgString(const size_t cfg_max_transformations) const
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
         if(arg.find("--cfg-max-transformation") == std::string::npos and arg.find("--find-max-cfg-transformations") == std::string::npos)
         {
            arg_string += arg;
         }
      }
   }
   arg_string += " --cfg-max-transformation=" + STR(cfg_max_transformations);
   return arg_string;
}

bool FindMaxCFGTransformations::ExecuteBambu(const size_t cfg_max_transformations) const
{
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Executing with max transformations " + STR(cfg_max_transformations));
   const auto arg_string = ComputeArgString(cfg_max_transformations);
   const auto temp_directory = parameters->getOption<std::string>(OPT_output_temporary_directory);
   const auto new_directory = temp_directory + "/" + STR(cfg_max_transformations);
   if(boost::filesystem::exists(new_directory))
      boost::filesystem::remove_all(new_directory);
   boost::filesystem::create_directory(new_directory);
   const auto ret = PandaSystem(parameters, "cd " + new_directory + "; " + arg_string, new_directory + "/bambu_execution_output");
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

DesignFlowStep_Status FindMaxCFGTransformations::Exec()
{
   size_t correct_cmt = 0;
   size_t wrong_cmt = 0;
   /// First check
   const auto zero_execution = ExecuteBambu(0);
   if(not zero_execution)
   {
      INDENT_OUT_MEX(0, 0, "Bambu fails with --cfg-max-transformations=0");
      return DesignFlowStep_Status::ABORTED;
   }
   INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, output_level, "-->Looking for upper bound of cfg max transformations");
   correct_cmt = 1;
   while(ExecuteBambu(correct_cmt))
   {
      correct_cmt *= 2;
   }
   INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, output_level, "<--Upper bound is " + STR(correct_cmt));
   wrong_cmt = correct_cmt;
   correct_cmt = wrong_cmt / 2;
   while(wrong_cmt - correct_cmt > 1)
   {
      INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, output_level, "---Current range is [" + STR(correct_cmt) + ":" + STR(wrong_cmt) + "]");
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
