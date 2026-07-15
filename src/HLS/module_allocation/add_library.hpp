/*
 *
 *        _/_/_/    _/_/   _/    _/ _/_/_/    _/_/
 *       _/   _/ _/    _/ _/_/  _/ _/   _/ _/    _/
 *      _/_/_/  _/_/_/_/ _/  _/_/ _/   _/ _/_/_/_/
 *     _/      _/    _/ _/    _/ _/   _/ _/    _/
 *    _/      _/    _/ _/    _/ _/_/_/  _/    _/
 *
 *  ***********************************************            
 *                   PandA Project
 *   URL: https://github.com/ferrandi/PandA-bambu
 *            Politecnico di Milano - DEIB
 *             System Architectures Group
 *  ***********************************************
 *   Copyright (C) 2004-2026 Politecnico di Milano
 *
 * Part of the PandA Project, under the Apache License v2.0 with LLVM Exceptions.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 *
 */
/**
 * @file add_library.hpp
 * @brief This step adds the current module to the technology library
 *
 * @author Christian Pilato <pilato@elet.polimi.it>
 *
 */
#ifndef ADD_LIBRARY_HPP
#define ADD_LIBRARY_HPP

#include "design_flow_step.hpp"
#include "hls_function_step.hpp"
#include "hls_step.hpp"
#include "refcount.hpp"

#include <string>

REF_FORWARD_DECL(add_library);
CONSTREF_FORWARD_DECL(Parameter);

/**
 * Information about speciaization of add_library
 */
class AddLibrarySpecialization : public HLSFlowStepSpecialization
{
 public:
   /// True if we are adding module with interface
   const bool interfaced;

   /**
    * Constructor
    * @param interfaced is true if we are adding module with interface
    */
   explicit AddLibrarySpecialization(const bool interfaced);

   std::string GetName() const override;

   context_t GetSignatureContext() const override;
};

class add_library : public HLSFunctionStep
{
 private:
   HLSRelationships ComputeHLSRelationships(const DesignFlowStep::RelationshipType relationship_type) const override;

 public:
   add_library(const ParameterConstRef _parameters, const HLS_managerRef HLSMgr, unsigned int funId,
               const DesignFlowManager& design_flow_manager,
               const HLSFlowStepSpecializationConstRef hls_flow_step_specialization);

   DesignFlowStep_Status InternalExec() override;
};

#endif // ADD_LIBRARY_HPP
