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
 *   Copyright (C) 2015-2026 Politecnico di Milano
 *
 * Part of the PandA Project, under the Apache License v2.0 with LLVM Exceptions.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 *
 */
/**
 * @file technology_flow_step.hpp
 * @brief Base class for technology flow steps
 *
 * @author Marco Lattuada <marco.lattuada@polimi.it>
 *
 */
#ifndef TECHNOLOGY_FLOW_STEP_HPP
#define TECHNOLOGY_FLOW_STEP_HPP
#include "design_flow_step.hpp"
#include "refcount.hpp"

#include <string>

#include "config_HAVE_CIRCUIT_BUILT.hpp"

REF_FORWARD_DECL(generic_device);
REF_FORWARD_DECL(technology_manager);

enum class TechnologyFlowStep_Type
{
#if HAVE_CIRCUIT_BUILT
   LOAD_BUILTIN_TECHNOLOGY,
#endif
   LOAD_DEFAULT_TECHNOLOGY,
   LOAD_DEVICE_TECHNOLOGY,
   LOAD_FILE_TECHNOLOGY,
   LOAD_TECHNOLOGY,
   WRITE_TECHNOLOGY
};

#if NO_ABSEIL_HASH
/**
 * Definition of hash function for TechnologyFlowStep_Type
 */
namespace std
{
   template <>
   struct hash<TechnologyFlowStep_Type>
   {
      size_t operator()(TechnologyFlowStep_Type design_flow_step) const
      {
         hash<int> hasher;
         return hasher(static_cast<int>(design_flow_step));
      }
   };
} // namespace std
#endif

class TechnologyFlowStep : public DesignFlowStep
{
 protected:
   /// The type of step
   TechnologyFlowStep_Type technology_flow_step_type;

   /// The technology manager
   const technology_managerRef TM;

   /// The target device
   const generic_deviceRef target;

   /**
    * Return the set of analyses in relationship with this design step
    * @param relationship_type is the type of relationship to be considered
    */
   virtual CustomUnorderedSet<TechnologyFlowStep_Type>
   ComputeTechnologyRelationships(const DesignFlowStep::RelationshipType relationship_type) const = 0;

   void ComputeRelationships(DesignFlowStepSet& steps,
                             const DesignFlowStep::RelationshipType relationship_type) override;

 public:
   /**
    * Constructor
    * @param _TM is the technology manager
    * @param target is the target device
    * @param design_flow_manager is the design flow manager
    * @param technology_flow_step_type is the type of this step
    * @param parameters is the set of input parameters
    */
   TechnologyFlowStep(const technology_managerRef _TM, const generic_deviceRef target,
                      const DesignFlowManager& design_flow_manager,
                      const TechnologyFlowStep_Type technology_flow_step_type, const ParameterConstRef parameters);

   std::string GetName() const override;

   DesignFlowStepFactoryConstRef CGetDesignFlowStepFactory() const override;

   bool HasToBeExecuted() const override;

   /**
    * Compute the signature of a technology flow step
    * @param technology_flow_step_type is the type of the step
    * @return the corresponding signature
    */
   static signature_t ComputeSignature(const TechnologyFlowStep_Type technology_flow_step_type);
};
#endif
