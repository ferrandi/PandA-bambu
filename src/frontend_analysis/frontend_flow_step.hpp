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

/// Autoheader include
#include "config_HAVE_BAMBU_BUILT.hpp"
#include "config_HAVE_EXPERIMENTAL.hpp"
#include "config_HAVE_FROM_PRAGMA_BUILT.hpp"
#include "config_HAVE_HOST_PROFILING_BUILT.hpp"
#include "config_HAVE_ILP_BUILT.hpp"
#include "config_HAVE_PRAGMA_BUILT.hpp"
#include "config_HAVE_RTL_BUILT.hpp"
#include "config_HAVE_TASTE.hpp"
#include "config_HAVE_TUCANO_BUILT.hpp"
#include "config_HAVE_ZEBU_BUILT.hpp"

#include "custom_set.hpp" // for unordered_set
#include <cstddef>        // for size_t
#include <string>         // for string
#include <typeindex>      // for hash
#include <utility>        // for pair

#include "design_flow_step.hpp" // for DesignFlowStep
#include "refcount.hpp"         // for REF_FORWARD_DECL

/// STD include
#include <functional>

/// Forward declaration
CONSTREF_FORWARD_DECL(application_manager);
REF_FORWARD_DECL(application_manager);
REF_FORWARD_DECL(ArchManager);
REF_FORWARD_DECL(DesignFlowManager);

typedef enum
{
/// Application frontend flow steps
#if HAVE_HOST_PROFILING_BUILT
   BASIC_BLOCKS_PROFILING,
#endif
   CREATE_TREE_MANAGER,
#if HAVE_BAMBU_BUILT
   FIND_MAX_CFG_TRANSFORMATIONS,
#endif
   FUNCTION_ANALYSIS, //! Creation of the call graph
#if HAVE_ZEBU_BUILT
   FUNCTION_POINTER_CALLGRAPH_COMPUTATION,
#endif
#if HAVE_HOST_PROFILING_BUILT
   HOST_PROFILING,
#endif
#if HAVE_BAMBU_BUILT
   IPA_POINT_TO_ANALYSIS,
#endif
#if HAVE_FROM_PRAGMA_BUILT
   PRAGMA_SUBSTITUTION,
#endif
#if HAVE_ZEBU_BUILT
   SIZEOF_SUBSTITUTION,
#endif
   SYMBOLIC_APPLICATION_FRONTEND_FLOW_STEP,
   /// Function frontend flow steps
   ADD_BB_ECFG_EDGES,
#if HAVE_ZEBU_BUILT
   ADD_OP_ECFG_EDGES,
#endif
#if HAVE_BAMBU_BUILT
   ADD_ARTIFICIAL_CALL_FLOW_EDGES,
#endif
   ADD_OP_EXIT_FLOW_EDGES,
   ADD_OP_LOOP_FLOW_EDGES,
#if HAVE_BAMBU_BUILT
   ADD_OP_PHI_FLOW_EDGES,
#endif
   AGGREGATE_DATA_FLOW_ANALYSIS,
#if HAVE_ZEBU_BUILT
   ARRAY_REF_FIX,
#endif
#if HAVE_BAMBU_BUILT
   BAMBU_FRONTEND_FLOW,
#endif
   BASIC_BLOCKS_CFG_COMPUTATION,
   BB_CONTROL_DEPENDENCE_COMPUTATION,
   BB_FEEDBACK_EDGES_IDENTIFICATION,
   BB_ORDER_COMPUTATION,
   BB_REACHABILITY_COMPUTATION,
#if HAVE_BAMBU_BUILT
   BIT_VALUE,
   BIT_VALUE_OPT,
   BIT_VALUE_IPA,
#endif
   BLOCK_FIX,
   BUILD_VIRTUAL_PHI,
#if HAVE_ZEBU_BUILT
   CALL_ARGS_STRUCTURING,
#endif
   CALL_EXPR_FIX,
#if HAVE_BAMBU_BUILT
   CALL_GRAPH_BUILTIN_CALL,
#endif
#if HAVE_ZEBU_BUILT
   CHECK_PIPELINABLE_LOOPS,
#endif
#if HAVE_FROM_PRAGMA_BUILT && HAVE_BAMBU_BUILT && HAVE_EXPERIMENTAL
   CHECK_CRITICAL_SESSION,
#endif
   CHECK_SYSTEM_TYPE, //! Set the system flag to variables and types
   CLEAN_VIRTUAL_PHI,
   COMPLETE_BB_GRAPH,
   COMPLETE_CALL_GRAPH,
#if HAVE_BAMBU_BUILT
   COMPUTE_IMPLICIT_CALLS,
   COMMUTATIVE_EXPR_RESTRUCTURING,
   COND_EXPR_RESTRUCTURING,
#endif
#if HAVE_TASTE
   CREATE_ADDRESS_TRANSLATION,
#endif
#if HAVE_BAMBU_BUILT
   CSE_STEP,
#endif
#if HAVE_ZEBU_BUILT || HAVE_BAMBU_BUILT
   DEAD_CODE_ELIMINATION,
#endif
#if HAVE_BAMBU_BUILT
   DETERMINE_MEMORY_ACCESSES,
#endif
   DOM_POST_DOM_COMPUTATION,
#if HAVE_EXPERIMENTAL && HAVE_ZEBU_BUILT
   DYNAMIC_AGGREGATE_DATA_FLOW_ANALYSIS,
   DYNAMIC_VAR_COMPUTATION,
#endif
#if HAVE_BAMBU_BUILT && HAVE_EXPERIMENTAL
   EXTENDED_PDG_COMPUTATION,
   REDUCED_PDG_COMPUTATION,
   PARALLEL_REGIONS_GRAPH_COMPUTATION,
#endif
#if HAVE_BAMBU_BUILT
   EXTRACT_GIMPLE_COND_OP,
#endif
#if HAVE_FROM_PRAGMA_BUILT && HAVE_BAMBU_BUILT
   EXTRACT_OMP_ATOMIC,
   EXTRACT_OMP_FOR,
#endif
#if HAVE_BAMBU_BUILT
   EXTRACT_PATTERNS,
   FIX_STRUCTS_PASSED_BY_VALUE,
   FUNCTION_PARM_MASK,
   FUNCTION_CALL_TYPE_CLEANUP,
   FANOUT_OPT,
#endif
#if HAVE_ZEBU_BUILT
   GLOBAL_VARIABLES_ANALYSIS,
#endif
#if HAVE_BAMBU_BUILT
   HDL_FUNCTION_DECL_FIX,
   HDL_VAR_DECL_FIX,
#endif
#if HAVE_ZEBU_BUILT
   HEADER_STRUCTURING,
#endif
#if HAVE_BAMBU_BUILT
   HLS_DIV_CG_EXT,
   HWCALL_INJECTION,
#endif
#if HAVE_ZEBU_BUILT
   INSTRUCTION_SEQUENCES_COMPUTATION,
#endif
#if HAVE_BAMBU_BUILT
   INTERFACE_INFER,
   IR_LOWERING,
#endif
   LOOP_COMPUTATION,
#if HAVE_ZEBU_BUILT
   LOOP_REGIONS_COMPUTATION,
   LOOP_REGIONS_FLOW_COMPUTATION,
#endif
#if HAVE_BAMBU_BUILT
   LOOPS_ANALYSIS_BAMBU,
#endif
#if HAVE_ZEBU_BUILT
   LOOPS_ANALYSIS_ZEBU,
#endif
   LOOPS_COMPUTATION,
#if HAVE_ZEBU_BUILT
   LOOPS_REBUILDING,
#endif
#if HAVE_BAMBU_BUILT
   LUT_TRANSFORMATION,
#endif
   MEMORY_DATA_FLOW_ANALYSIS,
   MEM_CG_EXT,
#if HAVE_BAMBU_BUILT
   MULTI_WAY_IF,
   MULTIPLE_ENTRY_IF_REDUCTION,
   NI_SSA_LIVENESS,
#endif
   OP_CONTROL_DEPENDENCE_COMPUTATION,
   OP_FEEDBACK_EDGES_IDENTIFICATION,
   OP_ORDER_COMPUTATION,
   OP_REACHABILITY_COMPUTATION,
   OPERATIONS_CFG_COMPUTATION,
#if HAVE_ZEBU_BUILT && HAVE_EXPERIMENTAL
   PARALLEL_LOOP_SWAP,
   PARALLEL_LOOPS_ANALYSIS,
#endif
   PARM2SSA,
#if HAVE_BAMBU_BUILT
   PARM_DECL_TAKEN_ADDRESS,
   PHI_OPT,
#endif
#if HAVE_ZEBU_BUILT
   POINTED_DATA_COMPUTATION,
   POINTED_DATA_EVALUATION,
#endif
#if HAVE_PRAGMA_BUILT
   PRAGMA_ANALYSIS,
#endif
#if HAVE_BAMBU_BUILT
   PREDICATE_STATEMENTS,
#endif
#if HAVE_ZEBU_BUILT
   PREDICTABILITY_ANALYSIS,
#endif
#if HAVE_ZEBU_BUILT
   PROBABILITY_PATH,
#endif
#if HAVE_BAMBU_BUILT
   ESSA,
   RANGE_ANALYSIS,
#endif
#if HAVE_BAMBU_BUILT
   REBUILD_INITIALIZATION,
   REBUILD_INITIALIZATION2,
#endif
#if HAVE_ZEBU_BUILT
#if HAVE_EXPERIMENTAL
   REFINED_AGGREGATE_DATA_FLOW_ANALYSIS,
   REFINED_VAR_COMPUTATION,
#endif
#endif
#if HAVE_BAMBU_BUILT
   REMOVE_CLOBBER_GA,
#endif
#if HAVE_BAMBU_BUILT
   REMOVE_ENDING_IF,
#endif
#if HAVE_ZEBU_BUILT
   REVERSE_RESTRICT_COMPUTATION,
#endif
   SCALAR_SSA_DATA_FLOW_ANALYSIS,
#if HAVE_ILP_BUILT && HAVE_BAMBU_BUILT
   SDC_CODE_MOTION,
#endif
#if HAVE_BAMBU_BUILT
   SERIALIZE_MUTUAL_EXCLUSIONS,
#endif
#if HAVE_ZEBU_BUILT
   SHORT_CIRCUIT_STRUCTURING,
#endif
#if HAVE_BAMBU_BUILT
   SPLIT_RETURN,
   SHORT_CIRCUIT_TAF,
   SIMPLE_CODE_MOTION,
   SOFT_FLOAT_CG_EXT,
#endif
#if HAVE_ZEBU_BUILT
   SOURCE_CODE_STATISTICS,
#endif
#if HAVE_BAMBU_BUILT && HAVE_EXPERIMENTAL
   SPECULATION_EDGES_COMPUTATION,
#endif
#if HAVE_TUCANO_BUILT || HAVE_ZEBU_BUILT
   SPLIT_PHINODES,
#endif
   STRING_CST_FIX,
   SWITCH_FIX,
#if HAVE_BAMBU_BUILT
   UN_COMPARISON_LOWERING,
#endif
#if HAVE_EXPERIMENTAL && HAVE_FROM_PRAGMA_BUILT && HAVE_BAMBU_BUILT
   UNROLL_LOOPS,
#endif
#if HAVE_BAMBU_BUILT
   UNROLLING_DEGREE,
#endif
#if HAVE_RTL_BUILT && HAVE_ZEBU_BUILT
   UPDATE_RTL_WEIGHT,
#endif
#if HAVE_ILP_BUILT && HAVE_BAMBU_BUILT
   UPDATE_SCHEDULE,
#endif
#if HAVE_ZEBU_BUILT
   UPDATE_TREE_WEIGHT,
#endif
   USE_COUNTING,
   VAR_ANALYSIS,
   VAR_DECL_FIX,
#if HAVE_BAMBU_BUILT
   VECTORIZE,
#endif
#if HAVE_BAMBU_BUILT
   VERIFICATION_OPERATION,
#endif
   VIRTUAL_AGGREGATE_DATA_FLOW_ANALYSIS,
#if HAVE_BAMBU_BUILT
   VIRTUAL_PHI_NODES_SPLIT
#endif
} FrontendFlowStepType;

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
         hash<int> hasher;
         return hasher(static_cast<int>(algorithm));
      }
   };
} // namespace std
#endif
class FrontendFlowStep : public DesignFlowStep
{
 public:
   /// The different relationship type between function analysis
   typedef enum
   {
      ALL_FUNCTIONS,     /**! All the functions composing the application */
      CALLED_FUNCTIONS,  /**! All the functions called by the current one */
      CALLING_FUNCTIONS, /**! All the functions which call the current one */
      SAME_FUNCTION,     /**! Same function */
      WHOLE_APPLICATION  /**! The whole application */
   } FunctionRelationship;

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
   virtual const CustomUnorderedSet<std::pair<FrontendFlowStepType, FunctionRelationship>> ComputeFrontendRelationships(const DesignFlowStep::RelationshipType relationship_type) const = 0;

 public:
   /**
    * Constructor
    * @param AppM is the application manager
    * @param design_flow_manager is the design flow manager
    * @param frontend_flow_step_type is the type of the analysis
    * @param _Param is the set of the parameters
    */
   FrontendFlowStep(const application_managerRef AppM, const FrontendFlowStepType frontend_flow_step_type, const DesignFlowManagerConstRef design_flow_manager, const ParameterConstRef parameters);

   /**
    * Destructor
    */
   ~FrontendFlowStep() override;

   /**
    * Compute the relationships of a step with other steps
    * @param dependencies is where relationships will be stored
    * @param relationship_type is the type of relationship to be computed
    */
   void ComputeRelationships(DesignFlowStepSet& relationship, const DesignFlowStep::RelationshipType relationship_type) override;

   /**
    * Create the relationship steps of a step with other steps starting from already specified dependencies between frontend flow steps
    * @param design_flow_manager is the design flow manager
    * @param frontend_relationships describes the set of relationships to be created
    * @param application_manager is the application manager
    * @param relationships is the output of the function
    */
   static void CreateSteps(const DesignFlowManagerConstRef design_flow_manager, const CustomUnorderedSet<std::pair<FrontendFlowStepType, FunctionRelationship>>& frontend_relationships, const application_managerConstRef application_manager,
                           DesignFlowStepSet& relationships);

   /**
    * Return the name of the type of this frontend flow step
    */
   virtual const std::string GetKindText() const;

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
   const DesignFlowStepFactoryConstRef CGetDesignFlowStepFactory() const override;

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
   struct hash<FrontendFlowStep::FunctionRelationship> : public unary_function<FrontendFlowStep::FunctionRelationship, size_t>
   {
      size_t operator()(FrontendFlowStep::FunctionRelationship relationship) const
      {
         hash<int> hasher;
         return hasher(static_cast<int>(relationship));
      }
   };
} // namespace std
#endif
#endif
