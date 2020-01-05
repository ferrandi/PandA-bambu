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

/// Autoheader include
#include "config_HAVE_BEAGLE.hpp"
#include "config_HAVE_EXPERIMENTAL.hpp"
#include "config_HAVE_FROM_PRAGMA_BUILT.hpp"
#include "config_HAVE_ILP_BUILT.hpp"
#include "config_HAVE_LIBRARY_CHARACTERIZATION_BUILT.hpp"
#include "config_HAVE_SIMULATION_WRAPPER_BUILT.hpp"
#include "config_HAVE_TASTE.hpp"
#include "config_HAVE_VCD_BUILT.hpp"

/// Superclass include
#include "design_flow_step.hpp"

/// STD include
#include <string>

/// STL include
#include "custom_map.hpp"

/// utility include
#include "refcount.hpp"

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
   /**
    * Destructor
    */
   virtual ~HLSFlowStepSpecialization();

   /**
    * Return the string representation of this
    */
   virtual const std::string GetKindText() const = 0;

   /**
    * Return the contribution to the signature of a step given by the specialization
    */
   virtual const std::string GetSignature() const = 0;
};
/// const refcount definition of the class
typedef refcount<const HLSFlowStepSpecialization> HLSFlowStepSpecializationConstRef;

enum class HLSFlowStep_Type
{
   UNKNOWN = 0,
   ADD_LIBRARY,
   ALLOCATION,
#if HAVE_EXPERIMENTAL
   AREA_ESTIMATION,
   AXI4LITE_INTERFACE_GENERATION,
#endif
   BB_STG_CREATOR,
   CALL_GRAPH_UNFOLDING,
   CDFC_MODULE_BINDING,
#if HAVE_EXPERIMENTAL
   CHAINING_BASED_LIVENESS,
#endif
   CHORDAL_COLORING_REGISTER_BINDING,
   CLASSIC_DATAPATH_CREATOR,
   DATAPATH_CS_CREATOR,
   DATAPATH_CS_PARALLEL_CREATOR,
#if HAVE_EXPERIMENTAL
   CLOCK_SLACK_ESTIMATION,
#endif
   CLASSICAL_HLS_SYNTHESIS_FLOW,
   COLORING_REGISTER_BINDING,
   CONTROL_FLOW_CHECKER,
   C_TESTBENCH_EXECUTION,
   DOMINATOR_FUNCTION_ALLOCATION,
   DOMINATOR_MEMORY_ALLOCATION,
   DOMINATOR_MEMORY_ALLOCATION_CS,
   DRY_RUN_EVALUATION,
#if HAVE_BEAGLE
   DSE_DESIGN_FLOW,
#endif
#if HAVE_EXPERIMENTAL
   DUMP_DESIGN_FLOW,
#endif
   EASY_MODULE_BINDING,
#if HAVE_EXPERIMENTAL
   EDGES_REDUCTION_EVALUATION,
   EPDG_SCHED_CHAINING,
#endif
   EVALUATION,
#if HAVE_EXPERIMENTAL
   EXPLORE_MUX_DESIGN_FLOW,
   EXPORT_PCORE,
#endif
   FIXED_SCHEDULING,
#if HAVE_EXPERIMENTAL
   FSL_INTERFACE_GENERATION,
#endif
   FSM_CONTROLLER_CREATOR,
   FSM_CS_CONTROLLER_CREATOR,
   FSM_NI_SSA_LIVENESS,
#if HAVE_EXPERIMENTAL
   FU_REG_BINDING_DESIGN_FLOW,
#endif
   GENERATE_HDL,
#if HAVE_EXPERIMENTAL
   GENERATE_RESP,
#endif
   GENERATE_SIMULATION_SCRIPT,
   GENERATE_SYNTHESIS_SCRIPT,
#if HAVE_TASTE
   GENERATE_TASTE_HDL_ARCHITECTURE,
   GENERATE_TASTE_SYNTHESIS_SCRIPT,
#endif
   HLS_BIT_VALUE,
   HLS_FUNCTION_BIT_VALUE,
   HLS_SYNTHESIS_FLOW,
   HW_PATH_COMPUTATION,
   HW_DISCREPANCY_ANALYSIS,
#if HAVE_ILP_BUILT && HAVE_EXPERIMENTAL
   ILP_NEW_FORM_SCHEDULING,
   ILP_SCHEDULING,
#endif
   INFERRED_INTERFACE_GENERATION,
   INITIALIZE_HLS,
   INTERFACE_CS_GENERATION,
#if HAVE_EXPERIMENTAL
   K_COFAMILY_REGISTER_BINDING,
   LEFT_EDGE_REGISTER_BINDING,
#endif
   LIST_BASED_SCHEDULING,
#if HAVE_EXPERIMENTAL
   MEMORY_CONFLICT_GRAPH,
#endif
   MINIMAL_INTERFACE_GENERATION,
   MINIMAL_TESTBENCH_GENERATION,
   MUX_INTERCONNECTION_BINDING,
#if HAVE_EXPERIMENTAL
   NPI_INTERFACE_GENERATION,
   NUM_AF_EDGES_EVALUATION,
#endif
#if HAVE_FROM_PRAGMA_BUILT
   OMP_ALLOCATION,
#endif
#if HAVE_FROM_PRAGMA_BUILT
#endif
   OMP_BODY_LOOP_SYNTHESIS_FLOW,
#if HAVE_EXPERIMENTAL && HAVE_FROM_PRAGMA_BUILT
   OMP_FOR_WRAPPER_SYNTHESIS_FLOW,
#endif
#if HAVE_FROM_PRAGMA_BUILT
   OMP_FOR_WRAPPER_CS_SYNTHESIS_FLOW,
#endif
#if HAVE_FROM_PRAGMA_BUILT
   OMP_FUNCTION_ALLOCATION,
#endif
#if HAVE_FROM_PRAGMA_BUILT
   OMP_FUNCTION_ALLOCATION_CS,
#endif
#if HAVE_EXPERIMENTAL
   PARALLEL_CONTROLLER_CREATOR,
#endif
   PORT_SWAPPING,
   SCHED_CHAINING,
#if HAVE_ILP_BUILT
   SDC_SCHEDULING,
#endif
#if HAVE_ILP_BUILT && HAVE_EXPERIMENTAL
   SILP_SCHEDULING,
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
   TESTBENCH_MEMORY_ALLOCATION,
   TESTBENCH_VALUES_C_GENERATION,
   TESTBENCH_VALUES_XML_GENERATION,
   TEST_VECTOR_PARSER,
#if HAVE_EXPERIMENTAL
   TIME_ESTIMATION,
#endif
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
   WB4_TESTBENCH_GENERATION,
   WEIGHTED_CLIQUE_REGISTER_BINDING,
   WRITE_HLS_SUMMARY,
#if HAVE_EXPERIMENTAL
   XML_HLS_SYNTHESIS_FLOW,
#endif
   XML_MEMORY_ALLOCATOR,
};

enum class HLSFlowStep_Relationship
{
   ALL_FUNCTIONS,
   CALLED_FUNCTIONS,
   SAME_FUNCTION,
   TOP_FUNCTION,
   WHOLE_APPLICATION
};

class HLS_step : public DesignFlowStep
{
 protected:
   /// Map hls step name to enum
   static CustomUnorderedMap<std::string, HLSFlowStep_Type> command_line_name_to_enum;

   /// information about all the HLS synthesis
   const HLS_managerRef HLSMgr;

   /// The type of this step
   const HLSFlowStep_Type hls_flow_step_type;

   /// The information about specialization
   const HLSFlowStepSpecializationConstRef hls_flow_step_specialization;

   /**
    * Return the set of analyses in relationship with this design step
    * @param relationship_type is the type of relationship to be considered
    */
   virtual const CustomUnorderedSet<std::tuple<HLSFlowStep_Type, HLSFlowStepSpecializationConstRef, HLSFlowStep_Relationship>> ComputeHLSRelationships(const DesignFlowStep::RelationshipType relationship_type) const;

 public:
   /**
    * Constructor
    * @param Param class containing all the parameters
    * @param HLS class containing all the HLS datastructures
    * @param design_flow_manager is the design flow manager
    * @param hls_flow_step_type is the type of this hls flow step
    */
   HLS_step(const ParameterConstRef Param, const HLS_managerRef HLSMgr, const DesignFlowManagerConstRef design_flow_manager, const HLSFlowStep_Type hls_flow_step_type,
            const HLSFlowStepSpecializationConstRef hls_flow_step_specialization = HLSFlowStepSpecializationConstRef());

   /**
    * Destructor
    */
   ~HLS_step() override;

   /**
    * Return a unified identifier of this design step
    * @return the signature of the design step
    */
   const std::string GetSignature() const override;

   /**
    * Compute the signature of a hls flow step
    * @param hls_flow_step_type is the type of the step
    * @param hls_flow_step_specialization is how the step has to be specialized
    * @return the corresponding signature
    */
   static const std::string ComputeSignature(const HLSFlowStep_Type hls_flow_step_type, const HLSFlowStepSpecializationConstRef hls_flow_step_specialization);

   /**
    * Return the name of this design step
    * @return the name of the pass (for debug purpose)
    */
   const std::string GetName() const override;

   /**
    * Return the name of the type of this frontend flow step
    */
   virtual const std::string GetKindText() const;

   /**
    * Given a HLS flow step type, return the name of the type
    * @param hls_flow_step_type is the type to be considered
    * @return the name of the type
    */
   static const std::string EnumToName(const HLSFlowStep_Type hls_flow_step_type);

   /**
    * Return the factory to create this type of steps
    */
   const DesignFlowStepFactoryConstRef CGetDesignFlowStepFactory() const override;

   /**
    * Compute the relationships of a step with other steps
    * @param dependencies is where relationships will be stored
    * @param relationship_type is the type of relationship to be computed
    */
   void ComputeRelationships(DesignFlowStepSet& relationship, const DesignFlowStep::RelationshipType relationship_type) override;
};
/// refcount definition of the class
typedef refcount<HLS_step> HLS_stepRef;

/**
 * Definition of hash function for HLSFlowStep_Type
 */
namespace std
{
   template <>
   struct hash<HLSFlowStep_Type> : public unary_function<HLSFlowStep_Type, size_t>
   {
      size_t operator()(HLSFlowStep_Type step) const
      {
         hash<int> hasher;
         return hasher(static_cast<int>(step));
      }
   };
} // namespace std

/**
 * Definition of hash function for std::tuple<HLSFlowStep_Type, HLSFlowStepSpecializationConstRef, HLSFlowStep_Relationship>
 */
namespace std
{
   template <>
   struct hash<std::tuple<HLSFlowStep_Type, HLSFlowStepSpecializationConstRef, HLSFlowStep_Relationship>> : public unary_function<std::tuple<HLSFlowStep_Type, HLSFlowStepSpecializationConstRef, HLSFlowStep_Relationship>, size_t>
   {
      size_t operator()(std::tuple<HLSFlowStep_Type, HLSFlowStepSpecializationConstRef, HLSFlowStep_Relationship> step) const
      {
         std::size_t ret = 0;
         hash<int> hasher;
         boost::hash_combine(ret, hasher(static_cast<int>(std::get<0>(step))));
         boost::hash_combine(ret, std::get<1>(step));
         boost::hash_combine(ret, hasher(static_cast<int>(std::get<2>(step))));
         return ret;
      }
   };
} // namespace std

/**
 * Definition of hash function for std::pair<HLSFlowStep_Type, HLSFlowStepSpecializationConstRef>
 */
namespace std
{
   template <>
   struct hash<std::pair<HLSFlowStep_Type, HLSFlowStepSpecializationConstRef>> : public unary_function<std::tuple<HLSFlowStep_Type, HLSFlowStepSpecializationConstRef>, size_t>
   {
      size_t operator()(std::pair<HLSFlowStep_Type, HLSFlowStepSpecializationConstRef> step) const
      {
         std::size_t ret = 0;
         hash<int> hasher;
         boost::hash_combine(ret, hasher(static_cast<int>(step.first)));
         boost::hash_combine(ret, step.second);
         return ret;
      }
   };
} // namespace std

#endif
