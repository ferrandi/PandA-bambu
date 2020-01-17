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
 *              Copyright (C) 2004-2020 Politecnico di Milano
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
 * @file hls_flow_step_factory.hpp
 * @brief Factory for hls flow step
 *
 * @author Marco Lattuada <lattuada@elet.polimi.it>
 *
 */

#ifndef HLS_FLOW_STEP_FACTORY_HPP
#define HLS_FLOW_STEP_FACTORY_HPP

/// Superclass include
#include "design_flow_step_factory.hpp"

/// STL include
#include <vector>

REF_FORWARD_DECL(DesignFlowStep);
REF_FORWARD_DECL(HLS_manager);
CONSTREF_FORWARD_DECL(HLSFlowStepSpecialization);
class xml_element;
class DesignFlowStepSet;
enum class HLSFlowStep_Type;

class HLSFlowStepFactory : public DesignFlowStepFactory
{
 protected:
   /// The HLS manager
   const HLS_managerRef HLS_mgr;

   /**
    * Verifies if the current node has to be added to the list of steps
    */
   bool checkNode(const xml_element* node, unsigned int funId, const std::string& ref_step) const;

 public:
   /**
    * Constructor
    * @param design_flow_manager is the design flow manager
    * @param HLS_manager is the HLS manager
    * @param parameters is the set of input parameters
    */
   HLSFlowStepFactory(const DesignFlowManagerConstRef design_flow_manager, const HLS_managerRef _HLS_manager, const ParameterConstRef parameters);

   /**
    * Destructor
    */
   ~HLSFlowStepFactory() override;

   /**
    * Return the prefix of the steps created by the factory
    */
   const std::string GetPrefix() const override;

   /**
    * Create a scheduling design flow step
    * @param hls_flow_step_type is the type of scheduling step to be created
    * @param funId is the index of the function to be scheduled
    * @param hls_flow_step_specialization contains information about how specialize the single step
    */
   DesignFlowStepRef CreateHLSFlowStep(const HLSFlowStep_Type hls_flow_step_type, const unsigned int funId, const HLSFlowStepSpecializationConstRef hls_flow_step_specialization = HLSFlowStepSpecializationConstRef()) const;

   /**
    * Create the frontend design flow steps
    * @param hls_flow_steps is the set of steps to be created
    */
   const DesignFlowStepSet CreateHLSFlowSteps(const CustomUnorderedSet<std::pair<HLSFlowStep_Type, HLSFlowStepSpecializationConstRef>>& hls_flow_steps) const;

   /**
    * The same as CreateHLSFlowSteps, but just for one step
    */
   const DesignFlowStepSet CreateHLSFlowSteps(const std::pair<HLSFlowStep_Type, HLSFlowStepSpecializationConstRef>& hls_flow_step) const;
   /**
    * The same as CreateHLSFlowSteps, but just for one step, with an even
    * simpler syntax
    */
   const DesignFlowStepSet CreateHLSFlowSteps(const HLSFlowStep_Type type, const HLSFlowStepSpecializationConstRef hls_flow_step_specialization) const;
   const DesignFlowStepRef CreateHLSFlowStep(const std::pair<HLSFlowStep_Type, HLSFlowStepSpecializationConstRef>& hls_flow_step) const;

   const DesignFlowStepRef CreateHLSFlowStep(const HLSFlowStep_Type type, const HLSFlowStepSpecializationConstRef hls_flow_step_specialization) const;
};
#endif
