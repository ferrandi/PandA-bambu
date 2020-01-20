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
 * @file test_vector_parser.cpp
 * @brief .
 *
 */

#ifndef TEST_VECTOR_PARSER_HPP
#define TEST_VECTOR_PARSER_HPP

#include "custom_map.hpp"
#include <string>
#include <vector>

#include "hls_step.hpp"

class TestVectorParser : public HLS_step
{
 private:
   std::string input_xml_filename;

   std::string user_input_string;

   /**
    * Parse a user defined string extracting the test vectors
    * @param [out] test_vectors Where the parsed test vectors are stored
    */
   void ParseUserString(std::vector<std::map<std::string, std::string>>& test_vectors) const;

   /**
    * Parse a user defined xml file extracting the test vectors
    */
   void ParseXMLFile(std::vector<std::map<std::string, std::string>>& test_vectors) const;

   /**
    * Parses the test vectors using one of ParseUserString() or
    * ParseXMLFile(), depending on internal parameters set from Initialize()
    * @param [out] test_vectors Where the parsed test vectors are stored
    */
   size_t ParseTestVectors(std::vector<std::map<std::string, std::string>>& test_vectors) const;

 public:
   /**
    * Constructor
    */
   TestVectorParser(const ParameterConstRef _parameters, const HLS_managerRef _HLSMgr, const DesignFlowManagerConstRef _design_flow_manager);

   /**
    * Destructor
    */
   ~TestVectorParser() override;

   /**
    * Initializes the data needed for execution. It's something similar to a
    * constructor, but it runs just before execution. The reason is that some
    * data needed for the execution may not be present when the step is
    * created and they need to be retreived just before the execution
    */
   void Initialize() override;

   /**
    * Executes the step
    */
   DesignFlowStep_Status Exec() override;

   /**
    * Compute the HLS relationships of this step
    */
   const CustomUnorderedSet<std::tuple<HLSFlowStep_Type, HLSFlowStepSpecializationConstRef, HLSFlowStep_Relationship>> ComputeHLSRelationships(const DesignFlowStep::RelationshipType relationship_type) const override;

   /**
    * Check if this step has actually to be executed
    * @return true if the step has to be executed
    */
   bool HasToBeExecuted() const override;
};
#endif
