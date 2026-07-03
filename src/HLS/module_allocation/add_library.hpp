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
