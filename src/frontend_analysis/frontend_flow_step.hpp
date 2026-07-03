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
 * @file frontend_flow_step.hpp
 * @brief This class contains the base representation for a generic frontend flow step
 *
 *
 * @author Marco Lattuada <lattuada@elet.polimi.it>
 *
 */

#ifndef FRONTEND_FLOW_STEP_HPP
#define FRONTEND_FLOW_STEP_HPP

#include "custom_set.hpp"
#include "design_flow_step.hpp"
#include "refcount.hpp"

#include <cstddef>
#include <functional>
#include <string>
#include <typeindex>
#include <utility>

#include "config_HAVE_HOST_PROFILING_BUILT.hpp"

/// Forward declaration
CONSTREF_FORWARD_DECL(application_manager);
REF_FORWARD_DECL(application_manager);
REF_FORWARD_DECL(ArchManager);
REF_FORWARD_DECL(DesignFlowManager);

using FrontendFlowStepType = enum FrontendFlowStepType {
/// Application frontend flow steps
#if HAVE_HOST_PROFILING_BUILT
   BASIC_BLOCKS_PROFILING,
#endif
   CREATE_IR_MANAGER,
   FIND_MAX_TRANSFORMATIONS,
   FUNCTION_ANALYSIS, //! Creation of the call graph
#if HAVE_HOST_PROFILING_BUILT
   HOST_PROFILING,
#endif
   SYMBOLIC_APPLICATION_FRONTEND_FLOW_STEP,
   /// Function frontend flow steps
   ADD_ARTIFICIAL_CALL_FLOW_EDGES,
   ADD_OP_EXIT_FLOW_EDGES,
   BAMBU_FRONTEND_FLOW,
   BASIC_BLOCKS_CFG_COMPUTATION,
   BB_CONTROL_DEPENDENCE_COMPUTATION,
   BB_FEEDBACK_EDGES_IDENTIFICATION,
   BB_ORDER_COMPUTATION,
   BIT_VALUE,
   BIT_VALUE_OPT,
   BITVALUE_RANGE,
   BIT_VALUE_IPA,
   BLOCK_FIX,
   BUILD_VIRTUAL_PHI,
   CALL_NODE_FIX,
   CALL_GRAPH_BUILTIN_CALL,
   CHECK_SYSTEM_TYPE, //! Set the system flag to variables and types
   COMPLETE_BB_GRAPH,
   COMPLETE_CALL_GRAPH,
   COMMUTATIVE_EXPR_RESTRUCTURING,
   SELECT_TREE_BALANCING,
   CSE_STEP,
   DATAFLOW_CG_EXT,
   DCE_PASS,
   DEAD_CODE_ELIMINATION_IPA,
   DETERMINE_MEMORY_ACCESSES,
   DOM_POST_DOM_COMPUTATION,
   EXTRACT_COND_OP,
   EXTRACT_PATTERNS,
   FIX_STRUCTS_PASSED_BY_VALUE,
   FUNCTION_CALL_TYPE_CLEANUP,
   FUNCTION_CALL_OPT,
   FANOUT_OPT,
   FIX_VDEF,
   SOFT_INT_CG_EXT,
   MUL_DECOMPOSITION,
   HWCALL_INJECTION,
   INTERFACE_INFER,
   IR_LOWERING,
   LOOP_COMPUTATION,
   LOOPS_COMPUTATION,
   MULTI_WAY_IF,
   NI_SSA_LIVENESS,
   OMP_CG_EXT,
   OMP_LOWERING,
   OP_CONTROL_DEPENDENCE_COMPUTATION,
   OP_FEEDBACK_EDGES_IDENTIFICATION,
   OP_ORDER_COMPUTATION,
   OPERATIONS_CFG_COMPUTATION,
   PARM2SSA,
   PARM_DECL_TAKEN_ADDRESS,
   PHI_OPT,
   PREDICATE_STATEMENTS,
   RANGE_ANALYSIS,
   REBUILD_INITIALIZATION2,
   SCALAR_SSA_DATA_FLOW_ANALYSIS,
   SDC_CODE_MOTION,
   SIMPLE_CODE_MOTION,
   SOFT_FLOAT_CG_EXT,
   IR2FUN,
   UNROLLING_DEGREE,
   UPDATE_SCHEDULE,
   USE_COUNTING,
   VAR_ANALYSIS,
   VAR_DECL_FIX,
   VERIFICATION_OPERATION,
   VIRTUAL_AGGREGATE_DATA_FLOW_ANALYSIS
};

#if NO_ABSEIL_HASH

/**
 * Definition of hash function for FunctionFrontendAnalysisType
 */
namespace std
{
   template <>
   struct hash<FrontendFlowStepType>
   {
      size_t operator()(FrontendFlowStepType algorithm) const
      {
         return static_cast<size_t>(algorithm);
      }
   };
} // namespace std
#endif

class FrontendFlowStep : public DesignFlowStep
{
 public:
   /// The different relationship type between function analysis
   using FunctionRelationship = enum {
      ALL_FUNCTIONS,     /**! All the functions composing the application */
      CALLED_FUNCTIONS,  /**! All the functions called by the current one */
      CALLING_FUNCTIONS, /**! All the functions which call the current one */
      SAME_FUNCTION,     /**! Same function */
      WHOLE_APPLICATION  /**! The whole application */
   };

 protected:
   /// The application manager
   const application_managerRef AppM;

   /// The type of this step
   const FrontendFlowStepType frontend_flow_step_type;

   /// Print counter
   unsigned int print_counter;

   /**
    * Return the set of analyses in relationship with this design step
    * @param relationship_type is the type of relationship to be considered
    */
   virtual CustomUnorderedSet<std::pair<FrontendFlowStepType, FunctionRelationship>>
   ComputeFrontendRelationships(const DesignFlowStep::RelationshipType relationship_type) const = 0;

 public:
   /**
    * Constructor
    * @param signature is the signature of the step
    * @param AppM is the application manager
    * @param design_flow_manager is the design flow manager
    * @param frontend_flow_step_type is the type of the analysis
    * @param parameters is the set of the parameters
    */
   FrontendFlowStep(DesignFlowStep::signature_t signature, const application_managerRef AppM,
                    const FrontendFlowStepType frontend_flow_step_type, const DesignFlowManager& design_flow_manager,
                    const ParameterConstRef parameters);

   virtual ~FrontendFlowStep() override = default;

   void ComputeRelationships(DesignFlowStepSet& relationship,
                             const DesignFlowStep::RelationshipType relationship_type) override;

   /**
    * Create the relationship steps of a step with other steps starting from already specified dependencies between
    * frontend flow steps
    * @param design_flow_manager is the design flow manager
    * @param frontend_relationships describes the set of relationships to be created
    * @param application_manager is the application manager
    * @param relationships is the output of the function
    */
   static void
   CreateSteps(const DesignFlowManager& design_flow_manager,
               const CustomUnorderedSet<std::pair<FrontendFlowStepType, FunctionRelationship>>& frontend_relationships,
               const application_managerConstRef application_manager, DesignFlowStepSet& relationships);

   /**
    * Return the name of the type of this frontend flow step
    */
   virtual std::string GetKindText() const;

   /**
    * Given a frontend flow step type, return the name of the type
    * @param frontend_flow_step_type is the type to be considered
    * @return the name of the type
    */
   static const std::string EnumToKindText(const FrontendFlowStepType frontend_flow_step_type);

   DesignFlowStepFactoryConstRef CGetDesignFlowStepFactory() const override;

   /**
    * Dump the IR manager
    * @param before specifies if printing is performed before execution of this step"
    */
   void PrintIRManager(const bool before) const;

   void PrintInitialIR() const override;

   void PrintFinalIR() const override;
};

#if NO_ABSEIL_HASH
/**
 * Definition of hash function for FrontendFlowStep::FunctionRelationship
 */
namespace std
{
   template <>
   struct hash<FrontendFlowStep::FunctionRelationship>
   {
      size_t operator()(FrontendFlowStep::FunctionRelationship relationship) const
      {
         return static_cast<size_t>(relationship);
      }
   };
} // namespace std
#endif
#endif
