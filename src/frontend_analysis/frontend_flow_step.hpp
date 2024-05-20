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
 * @file frontend_flow_step.hpp
 * @brief This class contains the base representation for a generic frontend flow step
 *
 *
 * @author Marco Lattuada <lattuada@elet.polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
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

#include "config_HAVE_FROM_PRAGMA_BUILT.hpp"
#include "config_HAVE_HOST_PROFILING_BUILT.hpp"
#include "config_HAVE_ILP_BUILT.hpp"
#include "config_HAVE_PRAGMA_BUILT.hpp"
#include "config_HAVE_TASTE.hpp"

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
   CREATE_TREE_MANAGER,
   FIND_MAX_TRANSFORMATIONS,
   FUNCTION_ANALYSIS, //! Creation of the call graph
#if HAVE_HOST_PROFILING_BUILT
   HOST_PROFILING,
#endif
#if HAVE_FROM_PRAGMA_BUILT
   PRAGMA_SUBSTITUTION,
#endif
   SYMBOLIC_APPLICATION_FRONTEND_FLOW_STEP,
   /// Function frontend flow steps
   ADD_BB_ECFG_EDGES,
   ADD_ARTIFICIAL_CALL_FLOW_EDGES,
   ADD_OP_EXIT_FLOW_EDGES,
   ADD_OP_LOOP_FLOW_EDGES,
   ADD_OP_PHI_FLOW_EDGES,
   BAMBU_FRONTEND_FLOW,
   BASIC_BLOCKS_CFG_COMPUTATION,
   BB_CONTROL_DEPENDENCE_COMPUTATION,
   BB_FEEDBACK_EDGES_IDENTIFICATION,
   BB_ORDER_COMPUTATION,
   BB_REACHABILITY_COMPUTATION,
   BIT_VALUE,
   BIT_VALUE_OPT,
   BITVALUE_RANGE,
   BIT_VALUE_IPA,
   BLOCK_FIX,
   BUILD_VIRTUAL_PHI,
   CALL_EXPR_FIX,
   CALL_GRAPH_BUILTIN_CALL,
   CHECK_SYSTEM_TYPE, //! Set the system flag to variables and types
   COMPLETE_BB_GRAPH,
   COMPLETE_CALL_GRAPH,
   COMPUTE_IMPLICIT_CALLS,
   COMMUTATIVE_EXPR_RESTRUCTURING,
   COND_EXPR_RESTRUCTURING,
#if HAVE_TASTE
   CREATE_ADDRESS_TRANSLATION,
#endif
   CSE_STEP,
   DATAFLOW_CG_EXT,
   DEAD_CODE_ELIMINATION,
   DEAD_CODE_ELIMINATION_IPA,
   DETERMINE_MEMORY_ACCESSES,
   DOM_POST_DOM_COMPUTATION,
   EXTRACT_GIMPLE_COND_OP,
#if HAVE_FROM_PRAGMA_BUILT
   EXTRACT_OMP_ATOMIC,
   EXTRACT_OMP_FOR,
#endif
   EXTRACT_PATTERNS,
   FIX_STRUCTS_PASSED_BY_VALUE,
   FUNCTION_CALL_TYPE_CLEANUP,
   FUNCTION_CALL_OPT,
   FANOUT_OPT,
   FIX_VDEF,
   HDL_FUNCTION_DECL_FIX,
   HDL_VAR_DECL_FIX,
   SOFT_INT_CG_EXT,
   MULT_EXPR_FRACTURING,
   HWCALL_INJECTION,
   INTERFACE_INFER,
   IR_LOWERING,
   LOOP_COMPUTATION,
   LOOPS_ANALYSIS_BAMBU,
   LOOPS_COMPUTATION,
   LUT_TRANSFORMATION,
   MULTI_WAY_IF,
   MULTIPLE_ENTRY_IF_REDUCTION,
   NI_SSA_LIVENESS,
   OP_CONTROL_DEPENDENCE_COMPUTATION,
   OP_FEEDBACK_EDGES_IDENTIFICATION,
   OP_ORDER_COMPUTATION,
   OP_REACHABILITY_COMPUTATION,
   OPERATIONS_CFG_COMPUTATION,
   PARM2SSA,
   PARM_DECL_TAKEN_ADDRESS,
   PHI_OPT,
#if HAVE_PRAGMA_BUILT
   PRAGMA_ANALYSIS,
#endif
   PREDICATE_STATEMENTS,
   ESSA,
   RANGE_ANALYSIS,
   REBUILD_INITIALIZATION,
   REBUILD_INITIALIZATION2,
   REMOVE_CLOBBER_GA,
   REMOVE_ENDING_IF,
   SCALAR_SSA_DATA_FLOW_ANALYSIS,
#if HAVE_ILP_BUILT
   SDC_CODE_MOTION,
#endif
   SERIALIZE_MUTUAL_EXCLUSIONS,
   SPLIT_RETURN,
   SHORT_CIRCUIT_TAF,
   SIMPLE_CODE_MOTION,
   SOFT_FLOAT_CG_EXT,
   STRING_CST_FIX,
   SWITCH_FIX,
   TREE2FUN,
   UN_COMPARISON_LOWERING,
   UNROLLING_DEGREE,
#if HAVE_ILP_BUILT
   UPDATE_SCHEDULE,
#endif
   USE_COUNTING,
   VAR_ANALYSIS,
   VAR_DECL_FIX,
   VECTORIZE,
   VERIFICATION_OPERATION,
   VIRTUAL_AGGREGATE_DATA_FLOW_ANALYSIS,
   VIRTUAL_PHI_NODES_SPLIT
};

#if NO_ABSEIL_HASH

/**
 * Definition of hash function for FunctionFrontendAnalysisType
 */
namespace std
{
   template <>
   struct hash<FrontendFlowStepType> : public unary_function<FrontendFlowStepType, size_t>
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
    * @param AppM is the application manager
    * @param design_flow_manager is the design flow manager
    * @param frontend_flow_step_type is the type of the analysis
    * @param _Param is the set of the parameters
    */
   FrontendFlowStep(DesignFlowStep::signature_t signature, const application_managerRef AppM,
                    const FrontendFlowStepType frontend_flow_step_type,
                    const DesignFlowManagerConstRef design_flow_manager, const ParameterConstRef parameters);

   /**
    * Destructor
    */
   ~FrontendFlowStep() override;

   /**
    * Compute the relationships of a step with other steps
    * @param dependencies is where relationships will be stored
    * @param relationship_type is the type of relationship to be computed
    */
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
   CreateSteps(const DesignFlowManagerConstRef design_flow_manager,
               const CustomUnorderedSet<std::pair<FrontendFlowStepType, FunctionRelationship>>& frontend_relationships,
               const application_managerConstRef application_manager, DesignFlowStepSet& relationships);

   /**
    * Return the name of the type of this frontend flow step
    */
   virtual std::string GetKindText() const;

   /**
    * Given a frontend flow step type, return the name of the type
    * @param type is the type to be considered
    * @return the name of the type
    */
   static const std::string EnumToKindText(const FrontendFlowStepType frontend_flow_step_type);

   /**
    * Return the factory to create this type of steps
    * @return the factory to create frontend flow step
    */
   DesignFlowStepFactoryConstRef CGetDesignFlowStepFactory() const override;

   /**
    * Dump the tree manager
    * @param before specifies if printing is performed before execution of this step"
    */
   void PrintTreeManager(const bool before) const;
   /**
    * Dump the initial intermediate representation
    */
   void PrintInitialIR() const override;

   /**
    * Dump the final intermediate representation
    */
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
       : public unary_function<FrontendFlowStep::FunctionRelationship, size_t>
   {
      size_t operator()(FrontendFlowStep::FunctionRelationship relationship) const
      {
         return static_cast<size_t>(relationship);
      }
   };
} // namespace std
#endif
#endif
