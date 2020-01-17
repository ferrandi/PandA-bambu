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
 * @file parser_flow_step.hpp
 * @brief This class contains the base representation for a generic parser step
 *
 * @author Marco Lattuada <marco.lattuada@polimi.it>
 *
 */

#ifndef PARSER_STEP_HPP
#define PARSER_STEP_HPP

/// Autoheader include
#include "config_HAVE_FROM_AADL_ASN_BUILT.hpp"
#include "config_HAVE_STDCXX_11.hpp"

/// Superclass include
#include "design_flow_step.hpp"

typedef enum
{
#if HAVE_FROM_AADL_ASN_BUILT
   AADL,
   ASN,
#endif
} ParserFlowStep_Type;

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
   virtual const std::string GetKindText() const
#if HAVE_STDCXX_11
       final
#endif
       ;

   /**
    * Given a parser step type, return the name of the type
    * @param type is the type to be considered
    * @return the name of the type
    */
   static const std::string EnumToKindText(const ParserFlowStep_Type parser_step_type);

 public:
   /**
    * Constructor
    * @param design_flow_manager is the design flow manager
    * @param parser_step_type is the type of the parser
    * @param file_name is the name of the file
    * @param parameters is the set of input parameters
    */
   ParserFlowStep(const DesignFlowManagerConstRef design_flow_manager, const ParserFlowStep_Type parser_step_type, const std::string& file_name, const ParameterConstRef parameters);

   /**
    * Destructor
    */
   ~ParserFlowStep() override;

   /**
    * Return the signature of this step
    */
   const std::string GetSignature() const override;

   /**
    * Return the name of this design step
    * @return the name of the pass (for debug purpose)
    */
   const std::string GetName() const override;

   /**
    * Compute the signature of a parser flow step
    * @param parser_step_type is the type of parser
    * @param file_name is the file name
    * @return the corresponding signature
    */
   static const std::string ComputeSignature(const ParserFlowStep_Type parser_step_type, const std::string& file_name);

   /**
    * Check if this step has actually to be executed
    * @return true if the step has to be executed
    */
   bool HasToBeExecuted() const override;

   /**
    * Compute the relationships of a step with other steps
    * @param dependencies is where relationships will be stored
    * @param relationship_type is the type of relationship to be computed
    */
   void ComputeRelationships(DesignFlowStepSet& relationship, const DesignFlowStep::RelationshipType relationship_type) override;

   /**
    * Return the factory to create this type of steps
    */
   const DesignFlowStepFactoryConstRef CGetDesignFlowStepFactory() const override;
};
#endif
