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
 * @file design_flow_aux_step.hpp
 * @brief Class for describing auxiliary steps in design flow
 *
 * @author Marco Lattuada <lattuada@elet.polimi.it>
 *
 */
#ifndef DESIGN_FLOW_AUX_STEP_HPP
#define DESIGN_FLOW_AUX_STEP_HPP
#include "design_flow_step.hpp"

#include <iosfwd>
#include <string>

/// Identifier of the auxiliary design flow steps
using AuxDesignFlowStepType = enum AuxDesignFlowStepType {
   DESIGN_FLOW_ENTRY, //! Entry point for the design flow
   DESIGN_FLOW_EXIT   //! Exit point for the design flow
};

/**
 * Class describing auxiliary steps in design flow
 */
class AuxDesignFlowStep : public DesignFlowStep
{
 private:
   /// The type of this auxiliary design flow step
   const AuxDesignFlowStepType type;

   void ComputeRelationships(DesignFlowStepSet& relationship,
                             const DesignFlowStep::RelationshipType relationship_type) override;

 public:
   /**
    * Constructor
    * @param type is the type of the step
    * @param design_flow_manager is the design flow manager
    * @param parameters is the set of input parameters
    */
   AuxDesignFlowStep(AuxDesignFlowStepType type, const DesignFlowManager& design_flow_manager,
                     const ParameterConstRef parameters);

   DesignFlowStep_Status Exec() override;

   bool HasToBeExecuted() const override;

   std::string GetName() const override;

   void writeDot(std::ostream& out) const override;

   DesignFlowStepFactoryConstRef CGetDesignFlowStepFactory() const override;

   /**
    * Compute the signature of a sdf design flow step
    * @param type is the type of auxiliary step
    * @return the signature corresponding to the analysis/transformation
    */
   static signature_t ComputeSignature(AuxDesignFlowStepType type);
};
#endif
