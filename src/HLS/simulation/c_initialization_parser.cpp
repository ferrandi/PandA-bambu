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
 *              Copyright (c) 2018-2020 Politecnico di Milano
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
 * @file c_initialization_parser.cpp
 * @brief Interface to parse the initialization of c variable.
 *
 * @author Marco Lattuada <marco.lattuada@polimi.it>
 *
 */

/// Header include
#include "c_initialization_parser.hpp"

///. include
#include "Parameter.hpp"

/// HLS/simulation include
#include "c_initialization_parser_node.hpp"
#define YYSTYPE CInitializationParserNode
#include "c_initialization_flex_lexer.hpp"

#include "c_initialization_parser_functor.hpp"
#include "testbench_generation_constants.hpp"

/// utility include
#include "fileIO.hpp"
#include "utility.hpp"

CInitializationParser::CInitializationParser(const ParameterConstRef _parameters) : parameters(_parameters)
{
   debug_level = parameters->get_class_debug_level(GET_CLASS(*this));
}

void CInitializationParser::Parse(CInitializationParserFunctorRef c_initialization_parser_functor, const std::string& initialization_string) const
{
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Analyzing initialization string " + initialization_string);
   if(initialization_string.size() > DATA_SIZE_THRESHOLD)
   {
      std::string seed_name = "_cip";
      auto output_parameter_initialization_filename = parameters->getOption<std::string>(OPT_output_directory) + "/simulation/";
      unsigned int progressive = 0;
      std::string candidate_out_file_name;
      do
      {
         candidate_out_file_name = output_parameter_initialization_filename + seed_name + "_" + std::to_string(progressive++) + ".data";
      } while(boost::filesystem::exists(candidate_out_file_name));
      c_initialization_parser_functor->ActivateFileInit(candidate_out_file_name);
   }
   const auto parsed_stream = fileIO_istream_open_from_string(initialization_string);
   const CInitializationFlexLexerRef lexer(new CInitializationFlexLexer(parameters, parsed_stream.get(), nullptr));
   YYParse(c_initialization_parser_functor, lexer);
   if(initialization_string.size() > DATA_SIZE_THRESHOLD)
   {
      c_initialization_parser_functor->FinalizeFileInit();
   }
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Analyzed initialization string " + initialization_string);
}
