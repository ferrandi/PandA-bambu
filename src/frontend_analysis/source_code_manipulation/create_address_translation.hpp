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
 * @file  create_address_translation.hpp
 * @brief Writes source code of hdl module to translate addresses from pci address space to bambu address space
 *
 * @author Marco Lattuada <marco.lattuada@polimi.it>
 *
 */
#ifndef CREATE_ADDRESS_TRANSLATION_HPP
#define CREATE_ADDRESS_TRANSLATION_HPP

/// Superclass include
#include "application_frontend_flow_step.hpp"

/// utility include
#include "utility.hpp"

REF_FORWARD_DECL(AadlInformation);
REF_FORWARD_DECL(AsnType);
REF_FORWARD_DECL(IndentedOutputStream);
CONSTREF_FORWARD_DECL(tree_manager);

/**
 * Class which creates source code of hdl module to translate addresses from pci address space to bambu address space
 */
class CreateAddressTranslation : public ApplicationFrontendFlowStep
{
 protected:
   /// The address translation stream
   IndentedOutputStreamRef address_translation;

   /// The memory enabling stream
   IndentedOutputStreamRef memory_enabling;

   /// The data size stream
   IndentedOutputStreamRef data_size;

   /// The endianess check stream
   IndentedOutputStreamRef endianess_check;

   /// The tree manager
   tree_managerConstRef TreeM;

   /// The asn information
   const AadlInformationRef aadl_information;

   /// True if it was already executed
   bool already_executed{false};

   /**
    * Compute the addresses and add them to the writers
    * @param asn_type is the type of the asn parameter
    * @param tree_parameter_type is the index of the type
    * @param bambu_address is the current starting address for bambu interface
    * @param taste_address is the current starting address for taste interface
    * @param registers is the number of registers to be allocated for this parameter
    * @param first_level is true if we are analyzing a parameter and not something inside a parameter
    * @param little_endianess tells if the parameter has little endianess
    */
   void ComputeAddress(const AsnTypeRef asn_type, const unsigned int tree_parameter_type, unsigned long long& bambu_address, unsigned long long int& taste_address, unsigned int& registers, const bool first_level, const bool little_endianess);

   /**
    * Return the set of analyses in relationship with this design step
    * @param relationship_type is the type of relationship to be considered
    */
   const CustomUnorderedSet<std::pair<FrontendFlowStepType, FunctionRelationship>> ComputeFrontendRelationships(const DesignFlowStep::RelationshipType relationship_type) const override;

   /**
    * Initialize the step (i.e., like a constructor, but executed just before exec
    */
   void Initialize() override;

   /**
    * Check if this step has actually to be executed
    * @return true if the step has to be executed
    */
   bool HasToBeExecuted() const override;

 public:
   /**
    * Constructor
    * @param AppM is the application manager
    * @param design_flow_manager is the design flow manager
    * @param parameters is the set of input parameters
    */
   CreateAddressTranslation(const application_managerRef AppM, const DesignFlowManagerConstRef design_flow_manager, const ParameterConstRef parameters);

   /**
    * Destructor
    */
   ~CreateAddressTranslation() override;

   /**
    * Execute this step
    * @return the exit status of this step
    */
   DesignFlowStep_Status Exec() override;
};
#endif
