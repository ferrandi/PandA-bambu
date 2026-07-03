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
 * @file function_frontend_flow_step.hpp
 * @brief This class contains the base representation for a generic frontend flow step which works on a single function
 *
 *
 * @author Marco Lattuada <lattuada@elet.polimi.it>
 *
 */
#ifndef FUNCTION_FRONTEND_FLOW_STEP_HPP
#define FUNCTION_FRONTEND_FLOW_STEP_HPP
#include "design_flow_step.hpp"
#include "frontend_flow_step.hpp"
#include "refcount.hpp"

#include <string>

REF_FORWARD_DECL(ArchManager);
CONSTREF_FORWARD_DECL(DesignFlowManager);
REF_FORWARD_DECL(FunctionBehavior);

/**
 * Enum class used to specify if a statement can be moved
 */
enum class FunctionFrontendFlowStep_Movable
{
   UNMOVABLE, /**< Operation cannot be moved */
   TIMING,    /**< Operation cannot be moved because of timing */
   MOVABLE    /**< Operation can be moved */
};

class FunctionFrontendFlowStep : public FrontendFlowStep
{
 protected:
   /// The function behavior of the function to be analyzed
   const FunctionBehaviorRef function_behavior;

   /// The index of the function to be analyzed
   const unsigned int function_id;

   /// The version of the basic block intermediate representation on which this step has been applied
   unsigned int bb_version;

   /// The version of the bitvalue information on which this step has been applied
   unsigned int bitvalue_version;

   /**
    * Execute the step
    * @return the exit status of this step
    */
   virtual DesignFlowStep_Status InternalExec() = 0;

   /**
    * Write the current version of statement list in dot format
    * @param filename is the file name to be written
    */
   void WriteBBGraphDot(const std::filesystem::path& filename) const;

   void ComputeRelationships(DesignFlowStepSet& relationship,
                             const DesignFlowStep::RelationshipType relationship_type) override;

 public:
   /**
    * Constructor
    * @param AppM is the application manager
    * @param function_id is the identifier of the function being processed
    * @param frontend_flow_step_type is the type of the analysis
    * @param design_flow_manager is the design flow manager
    * @param parameters is the set of the parameters
    */
   FunctionFrontendFlowStep(const application_managerRef AppM, const unsigned int function_id,
                            const FrontendFlowStepType frontend_flow_step_type,
                            const DesignFlowManager& design_flow_manager, const ParameterConstRef parameters);

   virtual ~FunctionFrontendFlowStep() override = default;

   std::string GetName() const final;

   DesignFlowStep_Status Exec() final;

   bool HasToBeExecuted() const override;

   void PrintInitialIR() const override;

   void PrintFinalIR() const override;

   /**
    * @return on which bb version this step has been executed last time
    */
   unsigned int CGetBBVersion() const;

   /**
    * @return on which bit_value version this step has been executed last time
    */
   unsigned int GetBitValueVersion() const;

   /**
    * Compute the signature of a function frontend flow step
    * @param frontend_flow_step_type is the type of frontend flow
    * @param function_id is the index of the function
    * @return the corresponding signature
    */
   static signature_t ComputeSignature(const FrontendFlowStepType frontend_flow_step_type,
                                       const unsigned int function_id);
};
#endif
