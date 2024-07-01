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
 *              Copyright (C) 2004-2024 Politecnico di Milano
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
 * @file test_vector_parser.hpp
 * @brief
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 * @author Michele Fiorito <michele.fiorito@polimi.it>
 */

#ifndef TEST_VECTOR_PARSER_HPP
#define TEST_VECTOR_PARSER_HPP

#include "hls_step.hpp"

#include <filesystem>
#include <map>
#include <string>
#include <vector>

class TestVectorParser : public HLS_step
{
   /**
    * Parse a user defined string extracting the test vectors
    * @param [out] test_vectors Where the parsed test vectors are stored
    */
   std::vector<std::map<std::string, std::string>> ParseUserString(const std::string& input_string) const;

   /**
    * Parse a user defined xml file extracting the test vectors
    */
   std::vector<std::map<std::string, std::string>> ParseXMLFile(const std::filesystem::path& input_xml) const;

   HLSRelationships ComputeHLSRelationships(const DesignFlowStep::RelationshipType relationship_type) const override;

 public:
   TestVectorParser(const ParameterConstRef _parameters, const HLS_managerRef _HLSMgr,
                    const DesignFlowManagerConstRef _design_flow_manager);

   DesignFlowStep_Status Exec() override;

   bool HasToBeExecuted() const override;
};
#endif
