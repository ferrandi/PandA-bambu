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
 * @file c_backend.hpp
 * @brief Simple class used to drive the backend in order to be able to print c source code
 *
 * @author Luca Fossati <fossati@elet.polimi.it>
 * @author Marco Lattuada <lattuada@elet.polimi.it>
 *
 */

#ifndef C_BACKEND_HPP
#define C_BACKEND_HPP

#include "custom_set.hpp"
#include "design_flow_step.hpp"
#include "graph.hpp"
#include "refcount.hpp"

#include <fstream>
#include <iosfwd>
#include <list>
#include <string>

CONSTREF_FORWARD_DECL(BehavioralHelper);
CONSTREF_FORWARD_DECL(CBackendInformation);
REF_FORWARD_DECL(CWriter);
REF_FORWARD_DECL(IndentedOutputStream);
CONSTREF_FORWARD_DECL(OpGraph);
class OpVertexSet;
CONSTREF_FORWARD_DECL(Parameter);
CONSTREF_FORWARD_DECL(application_manager);
CONSTREF_FORWARD_DECL(ir_manager);
CONSTREF_FORWARD_DECL(ir_node);

/**
 * Class simply used to drive the backend in order to print C code
 */
class CBackend : public DesignFlowStep
{
 private:
   // CBackendStepFactory is the only class allowed to construct CBackend
   friend class CBackendStepFactory;

   /**
    * Constructor
    * @param c_backend_information is the information about the backend to be created
    * @param design_flow_manager is the design flow graph manager
    * @param AppM is the manager of the application
    * @param _parameters is the set of input parameters
    */
   CBackend(const CBackendInformationConstRef c_backend_information, const DesignFlowManager& design_flow_manager,
            const application_managerConstRef AppM, const ParameterConstRef _parameters);

 protected:
   const CWriterRef writer;

   const application_managerConstRef AppM;

   void ComputeRelationships(DesignFlowStepSet& relationship,
                             const DesignFlowStep::RelationshipType relationship_type) override;

 public:
   const CBackendInformationConstRef c_backend_info;

   bool HasToBeExecuted() const override;

   void Initialize() override;

   DesignFlowStep_Status Exec() override;

   DesignFlowStepFactoryConstRef CGetDesignFlowStepFactory() const override;

   std::string GetName() const override;

   /**
    * Compute the signature for a c backend step
    */
   static signature_t ComputeSignature(const CBackendInformationConstRef type);
};
using CBackendRef = refcount<CBackend>;
using CBackendConstRef = refcount<const CBackend>;
#endif
