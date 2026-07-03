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
 * @file hls_step.hpp
 * @brief Base class for all HLS algorithms
 *
 * @author Christian Pilato <pilato@elet.polimi.it>
 *
 */
#ifndef HLS_STEP_HPP
#define HLS_STEP_HPP
#include "custom_map.hpp"
#include "design_flow_step.hpp"
#include "refcount.hpp"

#include <string>

#include "config_HAVE_LIBRARY_CHARACTERIZATION_BUILT.hpp"
#include "config_HAVE_SIMULATION_WRAPPER_BUILT.hpp"
#include "config_HAVE_VCD_BUILT.hpp"

CONSTREF_FORWARD_DECL(Parameter);
REF_FORWARD_DECL(hls);
REF_FORWARD_DECL(HLS_manager);
REF_FORWARD_DECL(HLS_step);
class xml_element;

/**
 * Abstract class containing information about specialization of the single steps
 */
class HLSFlowStepSpecialization
{
 public:
   enum SpecializationClass
   {
      C_BACKEND = 0,
      CDFC_MODULE_BINDING,
      WEIGHTED_CLIQUE_REGISTER,
      MEMORY_ALLOCATION,
      ADD_LIBRARY,
      PARAMETRIC_LIST_BASED
   };
   using context_t = unsigned short;

   HLSFlowStepSpecialization();

   virtual ~HLSFlowStepSpecialization() = default;
   /**
    * @brief Get the name of this specialization
    *
    * @return std::string Name of the specialization
    */
   virtual std::string GetName() const = 0;

   /**
    * @brief Get the signature context for this specialization
    *
    * @return context_t signature context
    */
   virtual context_t GetSignatureContext() const = 0;

   /**
    * @brief Compute signature context
    *
    * @param spec_class Specialization class
    * @param context Additional context
    */
   static context_t ComputeSignatureContext(SpecializationClass spec_class, unsigned char context);
};
/// const refcount definition of the class
using HLSFlowStepSpecializationConstRef = refcount<const HLSFlowStepSpecialization>;

enum class HLSFlowStep_Type
{
   UNKNOWN = 0,
   ADD_LIBRARY,
   ALLOCATION,
   BUILD_FSM,
   CALL_GRAPH_UNFOLDING,
   CDFC_MODULE_BINDING,
   CHORDAL_COLORING_REGISTER_BINDING,
   CLASSIC_DATAPATH_CREATOR,
   CLASSICAL_HLS_SYNTHESIS_FLOW,
   COLORING_REGISTER_BINDING,
   C_TESTBENCH_GENERATION,
   DOMINATOR_ALLOCATION,
   DOMINATOR_FUNCTION_ALLOCATION,
   DOMINATOR_MEMORY_ALLOCATION,
   EASY_MODULE_BINDING,
   EVALUATION,
   FSM_CONTROLLER_CREATOR,
   FSM_NI_SSA_LIVENESS,
   GENERATE_HDL,
   HDL_TESTBENCH_GENERATION,
   HLS_FUNCTION_BIT_VALUE,
   HLS_SYNTHESIS_FLOW,
   HW_PATH_COMPUTATION,
   INFERRED_INTERFACE_GENERATION,
   INITIALIZE_HLS,
   BUS_INTERFACE_GENERATION,
   LIST_BASED_SCHEDULING,
   MINIMAL_INTERFACE_GENERATION,
   MUX_INTERCONNECTION_BINDING,
   SCHED_CHAINING,
   SDC_SCHEDULING,
#if HAVE_SIMULATION_WRAPPER_BUILT
#endif
   STANDARD_HLS_FLOW,
   TESTBENCH_GENERATION,
   TEST_VECTOR_PARSER,
   TOP_ENTITY_OMP_CS_CREATION,
   TOP_ENTITY_CREATION,
   TOP_ENTITY_MEMORY_MAPPED_CREATION,
   UNIQUE_MODULE_BINDING,
   UNIQUE_REGISTER_BINDING,
   VALUES_SCHEME_STORAGE_VALUE_INSERTION,
#if HAVE_VCD_BUILT
   VCD_SIGNAL_SELECTION,
   VCD_UTILITY,
#endif
   VIRTUAL_DESIGN_FLOW,
   WB4_INTERCON_INTERFACE_GENERATION,
   WB4_INTERFACE_GENERATION,
   WEIGHTED_CLIQUE_REGISTER_BINDING,
   WRITE_HLS_SUMMARY
};

enum class HLSFlowStep_Relationship
{
   ALL_FUNCTIONS = 0,
   CALLED_FUNCTIONS,
   SAME_FUNCTION,
   TOP_FUNCTION,
   WHOLE_APPLICATION
};

class HLS_step : public DesignFlowStep
{
 public:
   using HLSRelationship = std::tuple<HLSFlowStep_Type, HLSFlowStepSpecializationConstRef, HLSFlowStep_Relationship>;

   struct HLSRelationshipEqual
   {
      inline bool operator()(const HLSRelationship& x, const HLSRelationship& y) const
      {
         if(std::get<0>(x) == std::get<0>(y) && std::get<2>(x) == std::get<2>(y))
         {
            if(std::get<1>(x) == std::get<1>(y))
            {
               return true;
            }
            else if(std::get<1>(x) && std::get<1>(y))
            {
               return std::get<1>(x)->GetSignatureContext() == std::get<1>(y)->GetSignatureContext();
            }
         }
         return false;
      }
   };

   struct HLSRelationshipHash
   {
      inline size_t operator()(const HLSRelationship& r) const
      {
         return static_cast<size_t>(std::get<0>(r)) << 24U |
                static_cast<size_t>(std::get<1>(r) ? std::get<1>(r)->GetSignatureContext() : 0U) << 8U |
                (static_cast<size_t>(std::get<2>(r)) & 0xFFU);
      }
   };

   using HLSRelationships = CustomUnorderedSet<HLSRelationship, HLSRelationshipHash, HLSRelationshipEqual>;

 protected:
   /// Map hls step name to enum
   static CustomUnorderedMap<std::string, HLSFlowStep_Type> command_line_name_to_enum;

   /// information about all the HLS synthesis
   const HLS_managerRef HLSMgr;

   /// The type of this step
   const HLSFlowStep_Type hls_flow_step_type;

   /// The information about specialization
   const HLSFlowStepSpecializationConstRef hls_flow_step_specialization;

   HLS_step(signature_t signature, const ParameterConstRef _parameters, const HLS_managerRef HLSMgr,
            const DesignFlowManager& design_flow_manager, const HLSFlowStep_Type hls_flow_step_type,
            const HLSFlowStepSpecializationConstRef hls_flow_step_specialization = HLSFlowStepSpecializationConstRef());

   /**
    * Return the set of analyses in relationship with this design step
    * @param relationship_type is the type of relationship to be considered
    */
   virtual HLSRelationships ComputeHLSRelationships(const DesignFlowStep::RelationshipType relationship_type) const;

   void ComputeRelationships(DesignFlowStepSet& design_flow_step_set,
                             const DesignFlowStep::RelationshipType relationship_type) override;

 public:
   /**
    * Constructor
    * @param _parameters class containing all the parameters
    * @param HLSMgr class containing all the HLS data-structures
    * @param design_flow_manager is the design flow manager
    * @param hls_flow_step_type is the type of this hls flow step
    * @param hls_flow_step_specialization is the optional specialization associated with this flow step
    */
   HLS_step(const ParameterConstRef _parameters, const HLS_managerRef HLSMgr,
            const DesignFlowManager& design_flow_manager, const HLSFlowStep_Type hls_flow_step_type,
            const HLSFlowStepSpecializationConstRef hls_flow_step_specialization = HLSFlowStepSpecializationConstRef());

   virtual std::string GetName() const override;

   DesignFlowStepFactoryConstRef CGetDesignFlowStepFactory() const final;

   /**
    * Given a HLS flow step type, return the name of the type
    * @param hls_flow_step_type is the type to be considered
    * @return the name of the type
    */
   static std::string EnumToName(const HLSFlowStep_Type hls_flow_step_type);

   /**
    * Compute the signature of a hls flow step
    * @param hls_flow_step_type is the type of the step
    * @param hls_flow_step_specialization is how the step has to be specialized
    * @return the corresponding signature
    */
   static signature_t ComputeSignature(const HLSFlowStep_Type hls_flow_step_type,
                                       const HLSFlowStepSpecializationConstRef hls_flow_step_specialization);
};

using HLS_stepRef = refcount<HLS_step>;

namespace std
{
   /**
    * Definition of hash function for std::pair<HLSFlowStep_Type, HLSFlowStepSpecializationConstRef>
    */
   template <>
   struct hash<std::pair<HLSFlowStep_Type, HLSFlowStepSpecializationConstRef>>
   {
      size_t operator()(std::pair<HLSFlowStep_Type, HLSFlowStepSpecializationConstRef> step) const
      {
         return static_cast<size_t>(std::get<0>(step)) << 16U | std::get<1>(step)->GetSignatureContext();
      }
   };
} // namespace std

#endif
