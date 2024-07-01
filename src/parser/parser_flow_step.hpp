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
 * @file parser_flow_step.hpp
 * @brief This class contains the base representation for a generic parser step
 *
 * @author Marco Lattuada <marco.lattuada@polimi.it>
 *
 */
#ifndef PARSER_STEP_HPP
#define PARSER_STEP_HPP
#include "design_flow_step.hpp"

#include "config_HAVE_FROM_AADL_ASN_BUILT.hpp"

using ParserFlowStep_Type = enum ParserFlowStep_Type {
#if HAVE_FROM_AADL_ASN_BUILT
   AADL,
   ASN,
#endif
};

class ParserFlowStep : public DesignFlowStep
{
 protected:
   /// The type of the parse
   const ParserFlowStep_Type parser_step_type;

   /// The name of the file to be parsed
   const std::string file_name;

   /**
    * Return the name of the type of this frontend flow step
    */
   virtual std::string GetKindText() const final;

   void ComputeRelationships(DesignFlowStepSet& relationship,
                             const DesignFlowStep::RelationshipType relationship_type) override;

 public:
   /**
    * Constructor
    * @param design_flow_manager is the design flow manager
    * @param parser_step_type is the type of the parser
    * @param file_name is the name of the file
    * @param parameters is the set of input parameters
    */
   ParserFlowStep(const DesignFlowManagerConstRef design_flow_manager, const ParserFlowStep_Type parser_step_type,
                  const std::string& file_name, const ParameterConstRef parameters);

   ~ParserFlowStep() override;

   std::string GetName() const override;

   bool HasToBeExecuted() const override;

   DesignFlowStepFactoryConstRef CGetDesignFlowStepFactory() const override;

   /**
    * Compute the signature of a parser flow step
    * @param parser_step_type is the type of parser
    * @param file_name is the file name
    * @return the corresponding signature
    */
   static signature_t ComputeSignature(const ParserFlowStep_Type parser_step_type, const std::string& file_name);
};
#endif
