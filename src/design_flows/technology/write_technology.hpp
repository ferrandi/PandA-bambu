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
 * @file write_technology.hpp
 * @brief Step to writes technology as xml file
 *
 * @author Marco Lattuada <marco.lattuada@polimi.it>
 *
 */

#ifndef WRITE_TECHNOLOGY_HPP
#define WRITE_TECHNOLOGY_HPP

#include "technology_flow_step.hpp"

#include "refcount.hpp"

REF_FORWARD_DECL(generic_device);
REF_FORWARD_DECL(technology_manager);

/**
 * Step which write technology information
 */
class WriteTechnology : public TechnologyFlowStep
{
 protected:
   /**
    * Return the set of analyses in relationship with this design step
    * @param relationship_type is the type of relationship to be considered
    */
   CustomUnorderedSet<TechnologyFlowStep_Type>
   ComputeTechnologyRelationships(const DesignFlowStep::RelationshipType relationship_type) const override;

 public:
   /**
    * Constructor.
    * @param TM is the technology manager
    * @param target is the target device
    * @param design_flow_manager is the design flow manager
    * @param parameters is the set of input parameters
    */
   WriteTechnology(const technology_managerRef TM, const generic_deviceRef target,
                   const DesignFlowManager& design_flow_manager, const ParameterConstRef parameters);

   /**
    * Execute the step
    * @return the exit status of this step
    */
   DesignFlowStep_Status Exec() override;
};
#endif
