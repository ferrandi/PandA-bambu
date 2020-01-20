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
 *              Copyright (C) 2004-2020 Politecnico di Milano
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
 * @file pragma_substitution.cpp
 * @brief Analysis step that replaces the pragmas in the specification with calls
 *
 * @author Marco Lattuada <lattuada@elet.polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *
 */
/// Header include
#include "pragma_substitution.hpp"

/// Behavior include
#include "application_manager.hpp"

/// Parameter include
#include "Parameter.hpp"

/// Parser include
#include "PragmaParser.hpp"

#include "hash_helper.hpp"
#include "string_manipulation.hpp" // for GET_CLASS
#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>

PragmaSubstitution::PragmaSubstitution(const application_managerRef _AppM, const DesignFlowManagerConstRef _design_flow_manager, const ParameterConstRef _parameters)
    : ApplicationFrontendFlowStep(_AppM, PRAGMA_SUBSTITUTION, _design_flow_manager, _parameters)
{
   debug_level = parameters->get_class_debug_level(GET_CLASS(*this));
}

PragmaSubstitution::~PragmaSubstitution() = default;

const CustomUnorderedSet<std::pair<FrontendFlowStepType, FrontendFlowStep::FunctionRelationship>> PragmaSubstitution::ComputeFrontendRelationships(const DesignFlowStep::RelationshipType relationship_type) const
{
   CustomUnorderedSet<std::pair<FrontendFlowStepType, FunctionRelationship>> relationships;
   switch(relationship_type)
   {
      case(DEPENDENCE_RELATIONSHIP):
      {
         break;
      }
      case(INVALIDATION_RELATIONSHIP):
      {
         break;
      }
      case(PRECEDENCE_RELATIONSHIP):
      {
         break;
      }
      default:
      {
         THROW_UNREACHABLE("");
      }
   }
   return relationships;
}

DesignFlowStep_Status PragmaSubstitution::Exec()
{
   for(const auto& input_file : AppM->input_files)
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Patching file " + input_file.first + "(" + input_file.second + ")");
      if(not boost::filesystem::exists(boost::filesystem::path(input_file.second)))
      {
         THROW_ERROR("File " + input_file.second + " does not exist");
      }

      const PragmaParserRef parser = PragmaParserRef(new PragmaParser(AppM->get_pragma_manager(), parameters));
      const std::string new_file = parser->substitutePragmas(input_file.second);
      AppM->input_files[input_file.first] = new_file;
   }
   return DesignFlowStep_Status::SUCCESS;
}
