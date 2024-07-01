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
 * @file HLS_step.hpp
 * @brief Base class for all HLS algorithms
 *
 * @author Christian Pilato <pilato@elet.polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *
 */
#ifndef HLS_STEP_HPP
#define HLS_STEP_HPP
#include "custom_map.hpp"
#include "design_flow_step.hpp"
#include "refcount.hpp"

#include <string>

#include "config_HAVE_FROM_PRAGMA_BUILT.hpp"
#include "config_HAVE_ILP_BUILT.hpp"
#include "config_HAVE_LIBRARY_CHARACTERIZATION_BUILT.hpp"
#include "config_HAVE_SIMULATION_WRAPPER_BUILT.hpp"
#include "config_HAVE_TASTE.hpp"
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

   virtual ~HLSFlowStepSpecialization();

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
   BB_STG_CREATOR,
   CALL_GRAPH_UNFOLDING,
   CDFC_MODULE_BINDING,
   CHORDAL_COLORING_REGISTER_BINDING,
   CLASSIC_DATAPATH_CREATOR,
   DATAPATH_CS_CREATOR,
   DATAPATH_CS_PARALLEL_CREATOR,
   CLASSICAL_HLS_SYNTHESIS_FLOW,
   COLORING_REGISTER_BINDING,
   CONTROL_FLOW_CHECKER,
   C_TESTBENCH_EXECUTION,
   DOMINATOR_ALLOCATION,
   DOMINATOR_FUNCTION_ALLOCATION,
   DOMINATOR_MEMORY_ALLOCATION,
   DOMINATOR_MEMORY_ALLOCATION_CS,
   DRY_RUN_EVALUATION,
   EASY_MODULE_BINDING,
   EVALUATION,
   FSM_CONTROLLER_CREATOR,
   FSM_CS_CONTROLLER_CREATOR,
   FSM_NI_SSA_LIVENESS,
   GENERATE_HDL,
   GENERATE_SIMULATION_SCRIPT,
   GENERATE_SYNTHESIS_SCRIPT,
#if HAVE_TASTE
   GENERATE_TASTE_HDL_ARCHITECTURE,
   GENERATE_TASTE_SYNTHESIS_SCRIPT,
#endif
   HLS_FUNCTION_BIT_VALUE,
   HLS_SYNTHESIS_FLOW,
   HW_PATH_COMPUTATION,
   HW_DISCREPANCY_ANALYSIS,
   INFERRED_INTERFACE_GENERATION,
   INITIALIZE_HLS,
   INTERFACE_CS_GENERATION,
   LIST_BASED_SCHEDULING,
   MINIMAL_INTERFACE_GENERATION,
   MUX_INTERCONNECTION_BINDING,
#if HAVE_FROM_PRAGMA_BUILT
   OMP_ALLOCATION,
#endif
#if HAVE_FROM_PRAGMA_BUILT
#endif
   OMP_BODY_LOOP_SYNTHESIS_FLOW,
#if HAVE_FROM_PRAGMA_BUILT
   OMP_FOR_WRAPPER_CS_SYNTHESIS_FLOW,
#endif
#if HAVE_FROM_PRAGMA_BUILT
   OMP_FUNCTION_ALLOCATION,
#endif
#if HAVE_FROM_PRAGMA_BUILT
   OMP_FUNCTION_ALLOCATION_CS,
#endif
   PIPELINE_CONTROLLER_CREATOR,
   PORT_SWAPPING,
   SCHED_CHAINING,
#if HAVE_ILP_BUILT
   SDC_SCHEDULING,
#endif
#if HAVE_SIMULATION_WRAPPER_BUILT
   SIMULATION_EVALUATION,
#endif
   STANDARD_HLS_FLOW,
#if HAVE_LIBRARY_CHARACTERIZATION_BUILT
   SYNTHESIS_EVALUATION,
#endif
#if HAVE_TASTE
   TASTE_INTERFACE_GENERATION,
#endif
   TESTBENCH_GENERATION,
   TEST_VECTOR_PARSER,
   TOP_ENTITY_CS_CREATION,
   TOP_ENTITY_CS_PARALLEL_CREATION,
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
            const DesignFlowManagerConstRef design_flow_manager, const HLSFlowStep_Type hls_flow_step_type,
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
    * @param Param class containing all the parameters
    * @param HLS class containing all the HLS data-structures
    * @param design_flow_manager is the design flow manager
    * @param hls_flow_step_type is the type of this hls flow step
    */
   HLS_step(const ParameterConstRef _parameters, const HLS_managerRef HLSMgr,
            const DesignFlowManagerConstRef design_flow_manager, const HLSFlowStep_Type hls_flow_step_type,
            const HLSFlowStepSpecializationConstRef hls_flow_step_specialization = HLSFlowStepSpecializationConstRef());

   ~HLS_step() override;

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
       : public unary_function<std::tuple<HLSFlowStep_Type, HLSFlowStepSpecializationConstRef>, size_t>
   {
      size_t operator()(std::pair<HLSFlowStep_Type, HLSFlowStepSpecializationConstRef> step) const
      {
         return static_cast<size_t>(std::get<0>(step)) << 16U | std::get<1>(step)->GetSignatureContext();
      }
   };
} // namespace std

#endif
