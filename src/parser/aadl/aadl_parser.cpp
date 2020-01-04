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
 * @file aadl_parser.cpp
 * @brief Specification of the aadl parsing interface function.
 *
 * @author Marco Lattuada <marco.lattuada@polimi.it>
 *
 */

/// Header include
#include "aadl_parser.hpp"

///. include
#include "Parameter.hpp"

/// behavior include
#include "application_manager.hpp"

/// constants include
#include "aadl_constants.hpp"
#include "constants.hpp"

/// HLS include
#include "hls_manager.hpp"

/// intermediate_representation/aadl
#include "aadl_information.hpp"

/// parser/aadl include
#include "aadl_parser_node.hpp"
#define YYSTYPE AadlParserNode
#include "aadl_lexer.hpp"

/// utility include
#include "fileIO.hpp"
#include "string_manipulation.hpp" // for GET_CLASS

static std::string GetFile(const std::string& directory, const std::string& file)
{
   std::string ret = file;
   ret = ret.at(0) == '"' ? ret.substr(1, ret.size() - 2) : ret;
   if(ret.at(0) == '/')
   {
      if(not ExistFile(ret))
      {
         THROW_ERROR(ret + " does not exist");
      }
      return ret;
   }
   if(ExistFile(ret))
   {
      return ret;
   }
   if(ExistFile(directory + "/" + ret))
   {
      return directory + "/" + ret;
   }
   THROW_ERROR(ret + " not found");
   return "";
}

AadlParserData::AadlParserData(const ParameterConstRef _parameters) : parameters(_parameters), debug_level(_parameters->get_class_debug_level("AadlParser"))
{
}

AadlParser::AadlParser(const DesignFlowManagerConstRef _design_flow_manager, const std::string& _file_name, const application_managerRef _AppM, const ParameterConstRef _parameters)
    : ParserFlowStep(_design_flow_manager, ParserFlowStep_Type::AADL, _file_name, _parameters), AppM(_AppM)
{
   debug_level = _parameters->get_class_debug_level(GET_CLASS(*this));
}

AadlParser::~AadlParser() = default;

DesignFlowStep_Status AadlParser::Exec()
{
   fileIO_istreamRef sname = fileIO_istream_open(file_name);
   const auto directory = GetDirectory(file_name);
   if(sname->fail())
      THROW_ERROR(std::string("FILE does not exist: ") + file_name);
   const AadlFlexLexerRef lexer(new AadlFlexLexer(parameters, sname.get(), nullptr));
   const AadlParserDataRef data(new AadlParserData(parameters));
   YYParse(data, lexer);
   AppM->input_files.erase(file_name);
   const auto aadl_information = GetPointer<HLS_manager>(AppM)->aadl_information;
   for(const auto& system : data->system_properties)
   {
      if(system.second.find("Source_Language") != system.second.end() and system.second.find("Source_Language")->second == "VHDL")
      {
         if(system.second.find("Source_Text") != system.second.end())
         {
            const auto input_file = GetFile(directory, system.second.find("Source_Text")->second);
            AppM->input_files[input_file] = input_file;
         }
         if(data->system_features.find(system.first) != data->system_features.end())
         {
            const auto features = data->system_features.find(system.first)->second;
            for(const auto& feature : features)
            {
               if(feature.second.find("Taste::InterfaceName") != feature.second.end())
               {
                  const auto top_functions = parameters->isOption(OPT_top_functions_names) ? parameters->getOption<std::string>(OPT_top_functions_names) + STR_CST_string_separator : "";
                  const auto function_name = feature.second.find("Taste::InterfaceName")->second;
                  const auto without_quota_function_name = function_name.at(0) == '"' ? function_name.substr(1, function_name.size() - 2) : function_name;
                  aadl_information->top_functions_names.push_back(without_quota_function_name);
                  const_cast<Parameter*>(parameters.get())->setOption(OPT_top_functions_names, top_functions + without_quota_function_name);
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Adding top function " + without_quota_function_name);
               }
            }
         }
      }
   }
   for(const auto& property : data->package_properties)
   {
      if(property.second.find("Taste::dataViewPath") != property.second.end())
      {
         const auto data_view_file = GetFile(directory, property.second.find("Taste::dataViewPath")->second);
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Added " + data_view_file + " to input files");
         AppM->input_files[data_view_file] = data_view_file;
      }
   }
   for(const auto& property : data->data_properties)
   {
      if(property.second.find("Source_Text") != property.second.end())
      {
         const auto data_view_file = GetFile(directory, property.second.find("Source_Text")->second);
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Added " + data_view_file + " to input files");
         AppM->input_files[data_view_file] = data_view_file;
      }
   }
   for(const auto& subprogram : data->subprogram_features)
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Analyzing subprogram " + subprogram.first);
      for(const auto& feature : subprogram.second)
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Analyzing feature " + feature.first);
         if(feature.second.find(STR_CST_aadl_parameter_type) != feature.second.end())
         {
            AadlInformation::AadlParameter aadl_parameter;
            aadl_parameter.name = feature.first;
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Found parameter " + feature.first);
            aadl_parameter.asn_type = feature.second.find(STR_CST_aadl_parameter_type)->second;
            if(aadl_parameter.asn_type.find(STR_CST_aadl_data_view) == 0)
               aadl_parameter.asn_type = aadl_parameter.asn_type.substr(std::string(STR_CST_aadl_data_view).size());
            THROW_ASSERT(feature.second.find(STR_CST_aadl_parameter_direction) != feature.second.end(), "");
            THROW_ASSERT(feature.second.find(STR_CST_aadl_parameter_endianess) != feature.second.end(), "");
            aadl_parameter.direction = AadlInformation::AadlParameter::GetDirection(feature.second.find(STR_CST_aadl_parameter_direction)->second);
            aadl_parameter.endianess = AadlInformation::AadlParameter::Endianess(feature.second.find(STR_CST_aadl_parameter_endianess)->second);
            aadl_information->function_parameters[subprogram.first].push_back(aadl_parameter);
         }
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Analyzed feature " + feature.first);
      }
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Analyzed subprogram " + subprogram.first);
   }
   if(parameters->getOption<const std::list<std::string>>(OPT_top_functions_names).size() > 1)
   {
      const_cast<Parameter*>(parameters.get())->setOption(OPT_disable_function_proxy, true);
   }
   return DesignFlowStep_Status::SUCCESS;
}
