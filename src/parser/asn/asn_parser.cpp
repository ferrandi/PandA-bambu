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
 * @file asn_parser.cpp
 * @brief Specification of the asn parsing interface function.
 *
 * @author Marco Lattuada <marco.lattuada@polimi.it>
 *
 */

/// Header include
#include "asn_parser.hpp"

///. include
#include "Parameter.hpp"

/// behavior include
#include "application_manager.hpp"

/// HLS include
#include "hls_manager.hpp"

/// intermediate_representation/aadl_asn include
#include "aadl_information.hpp"

/// parser/asn include
#include "asn_parser_node.hpp"
#define YYSTYPE AsnParserNode
#include "asn_lexer.hpp"

/// utility include
#include "fileIO.hpp"
#include "string_manipulation.hpp" // for GET_CLASS

AsnParserData::AsnParserData(const AadlInformationRef _aadl_information, const ParameterConstRef _parameters) : aadl_information(_aadl_information), parameters(_parameters), debug_level(_parameters->get_class_debug_level("AsnParser"))
{
}

AsnParser::AsnParser(const DesignFlowManagerConstRef _design_flow_manager, const std::string& _file_name, const application_managerRef _AppM, const ParameterConstRef _parameters)
    : ParserFlowStep(_design_flow_manager, ParserFlowStep_Type::ASN, _file_name, _parameters), AppM(_AppM)
{
   debug_level = _parameters->get_class_debug_level(GET_CLASS(*this));
}

AsnParser::~AsnParser() = default;

DesignFlowStep_Status AsnParser::Exec()
{
   fileIO_istreamRef sname = fileIO_istream_open(file_name);
   if(sname->fail())
      THROW_ERROR(std::string("FILE does not exist: ") + file_name);
   const AsnFlexLexerRef lexer(new AsnFlexLexer(parameters, sname.get(), nullptr));
   const AsnParserDataRef data(new AsnParserData(GetPointer<HLS_manager>(AppM)->aadl_information, parameters));
   YYParse(data, lexer);
   AppM->input_files.erase(file_name);
   return DesignFlowStep_Status::SUCCESS;
}
