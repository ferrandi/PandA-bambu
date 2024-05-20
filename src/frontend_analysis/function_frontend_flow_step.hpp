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
 * @file function_frontend_flow_step.hpp
 * @brief This class contains the base representation for a generic frontend flow step which works on a single function
 *
 *
 * @author Marco Lattuada <lattuada@elet.polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
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
   void WriteBBGraphDot(const std::string& filename) const;

   void ComputeRelationships(DesignFlowStepSet& relationship,
                             const DesignFlowStep::RelationshipType relationship_type) override;

 public:
   /**
    * Constructor
    * @param _Param is the set of the parameters
    */
   FunctionFrontendFlowStep(const application_managerRef AppM, const unsigned int function_id,
                            const FrontendFlowStepType frontend_flow_step_type,
                            const DesignFlowManagerConstRef design_flow_manager, const ParameterConstRef parameters);

   /**
    * Destructor
    */
   virtual ~FunctionFrontendFlowStep() override;

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
