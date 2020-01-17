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
 * @file parser_flow_step_factory.cpp
 * @brief Factory for parser flow step
 *
 * @author Marco Lattuada <marco.lattuada@polimi.it>
 *
 */
/// Header include
#include "parser_flow_step_factory.hpp"

/// Autoheader includes
#include "config_HAVE_FROM_AADL_ASN_BUILT.hpp"

/// parser include
#include "parser_flow_step.hpp"

#if HAVE_FROM_AADL_ASN_BUILT
/// parser/aadl include
#include "aadl_parser.hpp"

/// parser/asn include
#include "asn_parser.hpp"
#endif

ParserFlowStepFactory::ParserFlowStepFactory(const DesignFlowManagerConstRef _design_flow_manager, const application_managerRef _AppM, const ParameterConstRef _parameters) : DesignFlowStepFactory(_design_flow_manager, _parameters), AppM(_AppM)
{
}

ParserFlowStepFactory::~ParserFlowStepFactory() = default;

const std::string ParserFlowStepFactory::GetPrefix() const
{
   return "Parser";
}

DesignFlowStepRef ParserFlowStepFactory::CreateFlowStep(const std::string& signature) const
{
   THROW_ASSERT(signature.find(GetPrefix() + "::") == 0, signature);
   const auto step_to_be_created = signature.substr(GetPrefix().size() + 2);
   const auto parser_flow_step_type = static_cast<ParserFlowStep_Type>(boost::lexical_cast<int>(step_to_be_created.substr(0, step_to_be_created.find("::"))));
   const auto file_name = step_to_be_created.substr(step_to_be_created.find("::") + 2);
   switch(parser_flow_step_type)
   {
#if HAVE_FROM_AADL_ASN_BUILT
      case ParserFlowStep_Type::AADL:
      {
         return DesignFlowStepRef(new AadlParser(design_flow_manager.lock(), file_name, AppM, parameters));
      }
      case ParserFlowStep_Type::ASN:
      {
         return DesignFlowStepRef(new AsnParser(design_flow_manager.lock(), file_name, AppM, parameters));
      }
#endif
      default:
         THROW_UNREACHABLE("");
   }
   THROW_UNREACHABLE("");
   return DesignFlowStepRef();
}
