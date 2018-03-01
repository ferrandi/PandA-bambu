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
 *              Copyright (c) 2018 Politecnico di Milano
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

///Header include
#include "c_initialization_parser.hpp"

///. include
#include "Parameter.hpp"

///HLS/simulation include
#include "c_initialization_parser_node.hpp"
#define YYSTYPE CInitializationParserNode
#include "c_initialization_flex_lexer.hpp"
#include "c_initialization_parser_data.hpp"

///tree includes
#include "behavioral_helper.hpp"
#include "tree_helper.hpp"
#include "tree_manager.hpp"

///utility include
#include "fileIO.hpp"
#include "utility.hpp"

CInitializationParser::CInitializationParser(std::ofstream & _output_stream, const tree_managerConstRef _TM, const BehavioralHelperConstRef _behavioral_helper, const ParameterConstRef _parameters) :
   output_stream(_output_stream),
   TM(_TM),
   behavioral_helper(_behavioral_helper),
   parameters(_parameters)
{
   debug_level = parameters->get_class_debug_level(GET_CLASS(*this));
}

void CInitializationParser::Parse(const std::string & initialization_string, const unsigned long int reserved_mem_bytes, const tree_nodeConstRef function_parameter, const TestbenchGeneration_MemoryType testbench_generation_memory_type) const
{
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Analyzing initialization string " + initialization_string);
   const auto parsed_stream = fileIO_istream_open_from_string(initialization_string);
   const CInitializationFlexLexerRef lexer(new CInitializationFlexLexer(parameters, parsed_stream.get(), nullptr));
   const CInitializationParserDataRef data(new CInitializationParserData(output_stream, TM, behavioral_helper, reserved_mem_bytes, function_parameter, testbench_generation_memory_type, parameters));
   YYParse(data, lexer);
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Analyzed initialization string " + initialization_string);
}
