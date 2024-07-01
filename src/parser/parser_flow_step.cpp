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
 *              Copyright (C) 2015-2024 Politecnico di Milano
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
 * @file parser_flow_step.cpp
 * @brief This class contains the base representation for a generic parser step
 *
 * @author Marco Lattuada <marco.lattuada@polimi.it>
 *
 */
#include "parser_flow_step.hpp"

#include "design_flow_manager.hpp"
#include "string_manipulation.hpp"

#include <string>

ParserFlowStep::ParserFlowStep(const DesignFlowManagerConstRef _design_flow_manager,
                               const ParserFlowStep_Type _parser_step_type, const std::string& _file_name,
                               const ParameterConstRef _parameters)
    : DesignFlowStep(ComputeSignature(_parser_step_type, _file_name), _design_flow_manager, _parameters),
      parser_step_type(_parser_step_type),
      file_name(_file_name)
{
}

ParserFlowStep::~ParserFlowStep() = default;

std::string ParserFlowStep::GetName() const
{
   return "Parser::" + GetKindText() + "::" + file_name;
}

DesignFlowStep::signature_t ParserFlowStep::ComputeSignature(const ParserFlowStep_Type parser_step_type,
                                                             const std::string& file_name)
{
   return DesignFlowStep::ComputeSignature(PARSER, static_cast<unsigned short>(parser_step_type),
                                           std::hash<std::string>()(file_name) & 0xFFFFFFFFFFULL);
}

static std::string EnumToKindText(const ParserFlowStep_Type parser_step_type)
{
   switch(parser_step_type)
   {
#if HAVE_FROM_AADL_ASN_BUILT
      case AADL:
         return "Aadl";
      case ASN:
         return "Asn";
#endif
      default:
         THROW_UNREACHABLE("");
   }
   THROW_UNREACHABLE("");
   return "";
}

std::string ParserFlowStep::GetKindText() const
{
   return EnumToKindText(parser_step_type);
}

bool ParserFlowStep::HasToBeExecuted() const
{
   return true;
}

void ParserFlowStep::ComputeRelationships(DesignFlowStepSet&, const DesignFlowStep::RelationshipType)
{
}

DesignFlowStepFactoryConstRef ParserFlowStep::CGetDesignFlowStepFactory() const
{
   return design_flow_manager.lock()->CGetDesignFlowStepFactory(DesignFlowStep::PARSER);
}
