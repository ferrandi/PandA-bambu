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
 *              Copyright (c) 2018-2020 Politecnico di Milano
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
 * @file testbench_values_c_generation.hpp
 * @brief Class to compute testbench values exploiting C
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 * @author Marco Minutoli <mminutoli@gmail.com>
 * @author Manuel Beniani <manuel.beniani@gmail.com>
 * @author Christian Pilato <pilato@elet.polimi.it>
 * @author Pietro Fezzardi <pietrofezzardi@gmail.com>
 * @author Marco Lattuada<marco.lattuada@polimi.it>
 *
 */

#ifndef TESTBENCH_VALUES_C_GENERATION_HPP
#define TESTBENCH_VALUES_C_GENERATION_HPP

/// Superclass include
#include "hls_step.hpp"

/// utility include
#include "refcount.hpp"

CONSTREF_FORWARD_DECL(Parameter);
REF_FORWARD_DECL(HLS_manager);
CONSTREF_FORWARD_DECL(DesignFlowManager);

/**
 * TestbenchValuesCGeneration is the step which generates values.txt exploiting values.c
 */
class TestbenchValuesCGeneration : public HLS_step
{
 protected:
   /// output directory
   const std::string output_directory;

   /**
    * Initialize the step (i.e., like a constructor, but executed just before exec
    */
   virtual void Initialize();

   /**
    * Compute the relationship of this step
    * @param relationship_type is the type of relationship to be considered
    * @return the steps in relationship with this
    */
   virtual const CustomUnorderedSet<std::tuple<HLSFlowStep_Type, HLSFlowStepSpecializationConstRef, HLSFlowStep_Relationship>> ComputeHLSRelationships(const DesignFlowStep::RelationshipType relationship_type) const;

 public:
   /**
    * Constructor.
    * @param parameters is the set of the input parameters
    * @param hls_manager is the HLS manager
    * @param design_flow_manager is the design flow manager
    *
    */
   TestbenchValuesCGeneration(const ParameterConstRef parameters, const HLS_managerRef hls_manager, const DesignFlowManagerConstRef design_flow_manager);

   /**
    * Destructor.
    */
   virtual ~TestbenchValuesCGeneration();

   /**
    * Execute the step
    * @return the exit status of this step
    */
   DesignFlowStep_Status Exec();

   /**
    * Check if this step has actually to be executed
    * @return true if the step has to be executed
    */
   bool HasToBeExecuted() const;

   /**
    * Compute the relationships of this step with other steps
    * @param dependencies is where relationships will be stored
    * @param relationship_type is the type of relationship to be computed
    */
   void ComputeRelationships(DesignFlowStepSet& relationship, const DesignFlowStep::RelationshipType relationship_type);
};
#endif
