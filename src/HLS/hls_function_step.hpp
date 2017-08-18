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
 *              Copyright (c) 2004-2017 Politecnico di Milano
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
 *
*/

#ifndef HLS_FUNCION_STEP_HPP
#define HLS_FUNCION_STEP_HPP

///Autoheader include
#include "config_HAVE_STDCXX_11.hpp"

///Superclass include
#include "hls_step.hpp"

class HLSFunctionStep : public HLS_step
{
   protected:
      ///identifier of the function to be processed (0 means that it is a global step)
      const unsigned int funId;

      ///HLS datastructure of the function to be analyzed
      hlsRef HLS;

      ///The version of bb intermediate representation on which this step was applied
      unsigned int bb_version;

      /**
       * Execute the step
       * @return the exit status of this step
       */
      virtual DesignFlowStep_Status InternalExec() = 0;

   public:
      /**
       * Constructor
       * @param Param class containing all the parameters
       * @param HLS class containing all the HLS datastructures
       * @param design_flow_manager is the design flow manager
       * @param hls_flow_step_type is the type of this hls flow step
       */
      HLSFunctionStep(const ParameterConstRef Param, const HLS_managerRef HLSMgr, unsigned int funId, const DesignFlowManagerConstRef design_flow_manager, const HLSFlowStep_Type hls_flow_step_type, const HLSFlowStepSpecializationConstRef hls_flow_step_specialization = HLSFlowStepSpecializationConstRef());

      /**
       * Destructor
       */
      ~HLSFunctionStep();

      /**
       * Check if this step has actually to be executed
       * @return true if the step has to be executed
       */
      virtual bool HasToBeExecuted() const;

      /**
       * Initialize the step (i.e., like a constructor, but executed just before exec
       */
      virtual void Initialize();

      /**
       * Return a unified identifier of this design step
       * @return the signature of the design step
       */
      const std::string GetSignature() const;

      /**
       * Compute the signature of a hls flow step
       * @param hls_flow_step_type is the type of the step
       * @param hls_flow_step_specialization is how the step has to be specialized
       * @param function_id is the index of the function
       * @return the corresponding signature
       */
      static
      const std::string ComputeSignature(const HLSFlowStep_Type hls_flow_step_type, const HLSFlowStepSpecializationConstRef hls_flow_step_specialization, const unsigned int function_id);

      /**
       * Return the name of this design step
       * @return the name of the pass (for debug purpose)
       */
      virtual const std::string GetName() const;

      /**
       * Compute the relationships of a step with other steps
       * @param dependencies is where relationships will be stored
       * @param relationship_type is the type of relationship to be computed
       */
      virtual void ComputeRelationships(DesignFlowStepSet & relationship, const DesignFlowStep::RelationshipType relationship_type);

      /**
       * Execute the step
       * @return the exit status of this step
       */
      virtual DesignFlowStep_Status Exec()
#if HAVE_STDCXX_11
         final
#endif
         ;
};
#endif
