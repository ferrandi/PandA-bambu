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
 * @file aadl_parser.hpp
 * @brief Specification of the aadl parsing interface function.
 *
 * @author Marco Lattuada <marco.lattuada@polimi.it>
 *
 */
#ifndef AADL_PARSER_HPP
#define AADL_PARSER_HPP

/// Super class include
#include "parser_flow_step.hpp"

/// utility include
#include "custom_map.hpp"

/// utility include
REF_FORWARD_DECL(AadlFlexLexer);
REF_FORWARD_DECL(application_manager);

struct AadlParserData
{
   /// For each subprogram the associated features
   CustomMap<std::string, std::list<std::pair<std::string, CustomMap<std::string, std::string>>>> subprogram_features;

   /// For each system the associated features
   CustomMap<std::string, std::list<std::pair<std::string, CustomMap<std::string, std::string>>>> system_features;

   /// For each system the associated properties
   CustomMap<std::string, CustomMap<std::string, std::string>> system_properties;

   /// For each package the associated properties
   CustomMap<std::string, CustomMap<std::string, std::string>> package_properties;

   /// For each data the associated properties
   CustomMap<std::string, CustomMap<std::string, std::string>> data_properties;

   /// The set of input parameters
   const ParameterConstRef parameters;

   /// The debug level
   const int debug_level;

   /**
    * Constructor
    */
   explicit AadlParserData(const ParameterConstRef parameters);
};
typedef refcount<AadlParserData> AadlParserDataRef;

class AadlParser : public ParserFlowStep
{
 protected:
   /// The application manager
   const application_managerRef AppM;

   /**
    * Wrapper to yyparse
    * @param data is the data to be passed to the parser
    * @param lexer is the lexer
    */
   void YYParse(const AadlParserDataRef data, const AadlFlexLexerRef lexer) const;

 public:
   /**
    * Constructor
    * @param design_flow_manager is the design flow manager
    * @param file_name is the file to be parsed
    * @param AppM is the application manager
    * @param parameters is the set of input parameters
    */
   AadlParser(const DesignFlowManagerConstRef design_flow_manager, const std::string& file_name, const application_managerRef AppM, const ParameterConstRef parameters);

   /**
    * Destuctor
    */
   ~AadlParser() override;

   /**
    * Execute the step
    * @return the exit status of this step
    */
   DesignFlowStep_Status Exec() override;
};
#endif
