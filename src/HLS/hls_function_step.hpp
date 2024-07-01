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
 * @file HLS_function_step.hpp
 * @brief Base class for all HLS algorithms
 *
 * @author Marco Lattuada <marco.lattuada@polimi.it>
 * @author Michele Fiorito <michele.fiorito@polimi.it>
 *
 */
#ifndef HLS_FUNCION_STEP_HPP
#define HLS_FUNCION_STEP_HPP
#include "hls_step.hpp"

class HLSFunctionStep : public HLS_step
{
 private:
   /* Current bb+bitvalue version for a given function id */
   static CustomMap<unsigned int, unsigned int> curr_ver;

   /* Sum of called functions' bb+bitvalue versions after last Exec call */
   unsigned int last_ver_sum;

 protected:
   /// identifier of the function to be processed (0 means that it is a global step)
   const unsigned int funId;

   /// HLS data structure of the function to be analyzed
   hlsRef HLS;

   /// The version of bb intermediate representation on which this step was applied
   unsigned int bb_version;

   /// The version of bitvalue on which this step was applied
   unsigned int bitvalue_version;

   /// The version of memory representation on which this step was applied
   unsigned int memory_version;

   void ComputeRelationships(DesignFlowStepSet& design_flow_step_set,
                             const DesignFlowStep::RelationshipType relationship_type) override;

   /**
    * Execute the step
    * @return the exit status of this step
    */
   virtual DesignFlowStep_Status InternalExec() = 0;

 public:
   /**
    * Constructor
    * @param Param class containing all the parameters
    * @param HLS class containing all the HLS data structures
    * @param design_flow_manager is the design flow manager
    * @param hls_flow_step_type is the type of this hls flow step
    */
   HLSFunctionStep(
       const ParameterConstRef Param, const HLS_managerRef HLSMgr, unsigned int funId,
       const DesignFlowManagerConstRef design_flow_manager, const HLSFlowStep_Type hls_flow_step_type,
       const HLSFlowStepSpecializationConstRef hls_flow_step_specialization = HLSFlowStepSpecializationConstRef());

   ~HLSFunctionStep() override;

   virtual bool HasToBeExecuted() const override;

   virtual void Initialize() override;

   std::string GetName() const final;

   DesignFlowStep_Status Exec() final;

   /**
    * Compute the signature of a hls flow step
    * @param hls_flow_step_type is the type of the step
    * @param hls_flow_step_specialization is how the step has to be specialized
    * @param function_id is the index of the function
    * @return the corresponding signature
    */
   static signature_t ComputeSignature(const HLSFlowStep_Type hls_flow_step_type,
                                       const HLSFlowStepSpecializationConstRef hls_flow_step_specialization,
                                       const unsigned int function_id);
};
#endif
