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
 *              Copyright (C) 2004-2026 Politecnico di Milano
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 *
 *   This file is part of the PandA framework.
 *
 *   Licensed under the Apache License, Version 2.0, with BAMBU exceptions (the "License");
 *   you may not use this file except in compliance with the License.
 *   You may obtain a copy of the License at
 *
 *       http://www.apache.org/licenses/LICENSE-2.0
 *
 *   Unless required by applicable law or agreed to in writing, software
 *   distributed under the License is distributed on an "AS IS" BASIS,
 *   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *   See the License for the specific language governing permissions and
 *   limitations under the License.
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
    * @param input_string is the user-defined string containing the test vectors
    */
   std::vector<std::map<std::string, std::string>> ParseUserString(const std::string& input_string) const;

   /**
    * Parse a user defined xml file extracting the test vectors
    */
   std::vector<std::map<std::string, std::string>> ParseXMLFile(const std::filesystem::path& input_xml) const;

   HLSRelationships ComputeHLSRelationships(const DesignFlowStep::RelationshipType relationship_type) const override;

 public:
   TestVectorParser(const ParameterConstRef _parameters, const HLS_managerRef _HLSMgr,
                    const DesignFlowManager& _design_flow_manager);

   DesignFlowStep_Status Exec() override;

   bool HasToBeExecuted() const override;
};
#endif
