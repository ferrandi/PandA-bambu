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
 * @file bambu_frontend_flow.cpp
 * @brief The step representing the frontend flow for bambu
 *
 * @author Marco Lattuada <marco.lattuada@polimi.it>
 */
#include "bambu_frontend_flow.hpp"

#include "config_HAVE_EXPERIMENTAL.hpp"         // for HAVE_EXPERIMENTAL
#include "config_HAVE_HOST_PROFILING_BUILT.hpp" // for HAVE_HOST_PROFILING_...
#include "config_HAVE_ILP_BUILT.hpp"            // for HAVE_ILP_BUILT
#include "config_HAVE_PRAGMA_BUILT.hpp"         // for HAVE_PRAGMA_BUILT
#include "config_HAVE_TASTE.hpp"                // for HAVE_TASTE

#include "Parameter.hpp"                   // for Parameter, OPT_parse...
#include "application_manager.hpp"         // for application_manager
#include "call_graph.hpp"                  // for CallGraph
#include "call_graph_manager.hpp"          // for CallGraphConstRef
#include "dbgPrintHelper.hpp"              // for DEBUG_LEVEL_PEDANTIC
#include "exceptions.hpp"                  // for THROW_UNREACHABLE
#include "frontend_flow_step_factory.hpp"  // for application_managerRef
#include "function_frontend_flow_step.hpp" // for DesignFlowManagerCon...
#include "hash_helper.hpp"                 // for hash
#include "hls_step.hpp"                    // for HLSFlowStep_Type
#include <iosfwd>                          // for ofstream
#include <string>                          // for string, operator+
#if HAVE_HOST_PROFILING_BUILT
#include "host_profiling.hpp" // for HostProfiling_Method
#endif
#include "language_writer.hpp"     // for HDLWriter_Language
#include "string_manipulation.hpp" // for GET_CLASS
#include "tree_manager.hpp"        // for tree_managerConstRef

/// STL include
#include "custom_set.hpp"

BambuFrontendFlow::BambuFrontendFlow(const application_managerRef _AppM, const DesignFlowManagerConstRef _design_flow_manager, const ParameterConstRef _parameters) : ApplicationFrontendFlowStep(_AppM, BAMBU_FRONTEND_FLOW, _design_flow_manager, _parameters)
{
   debug_level = parameters->get_class_debug_level(GET_CLASS(*this));
}

BambuFrontendFlow::~BambuFrontendFlow() = default;

const CustomUnorderedSet<std::pair<FrontendFlowStepType, FrontendFlowStep::FunctionRelationship>> BambuFrontendFlow::ComputeFrontendRelationships(const DesignFlowStep::RelationshipType relationship_type) const
{
   CustomUnorderedSet<std::pair<FrontendFlowStepType, FunctionRelationship>> relationships;
   switch(relationship_type)
   {
      case(DEPENDENCE_RELATIONSHIP):
      {
         relationships.insert(std::pair<FrontendFlowStepType, FunctionRelationship>(ADD_ARTIFICIAL_CALL_FLOW_EDGES, WHOLE_APPLICATION));
         relationships.insert(std::pair<FrontendFlowStepType, FunctionRelationship>(ADD_OP_EXIT_FLOW_EDGES, WHOLE_APPLICATION));
         //            relationships.insert(std::pair<FrontendFlowStepType, FunctionRelationship>(ADD_OP_LOOP_FLOW_EDGES, WHOLE_APPLICATION));
         relationships.insert(std::pair<FrontendFlowStepType, FunctionRelationship>(BLOCK_FIX, WHOLE_APPLICATION));
         relationships.insert(std::pair<FrontendFlowStepType, FunctionRelationship>(CALL_EXPR_FIX, WHOLE_APPLICATION));
         relationships.insert(std::pair<FrontendFlowStepType, FunctionRelationship>(CHECK_SYSTEM_TYPE, WHOLE_APPLICATION));
         relationships.insert(std::pair<FrontendFlowStepType, FunctionRelationship>(DETERMINE_MEMORY_ACCESSES, WHOLE_APPLICATION));
         relationships.insert(std::pair<FrontendFlowStepType, FunctionRelationship>(OP_CONTROL_DEPENDENCE_COMPUTATION, WHOLE_APPLICATION));
         relationships.insert(std::pair<FrontendFlowStepType, FunctionRelationship>(IR_LOWERING, WHOLE_APPLICATION));
         relationships.insert(std::pair<FrontendFlowStepType, FunctionRelationship>(LUT_TRANSFORMATION, WHOLE_APPLICATION));
         relationships.insert(std::pair<FrontendFlowStepType, FunctionRelationship>(SCALAR_SSA_DATA_FLOW_ANALYSIS, WHOLE_APPLICATION));
         relationships.insert(std::pair<FrontendFlowStepType, FunctionRelationship>(AGGREGATE_DATA_FLOW_ANALYSIS, WHOLE_APPLICATION));
         relationships.insert(std::pair<FrontendFlowStepType, FunctionRelationship>(SWITCH_FIX, WHOLE_APPLICATION));
         relationships.insert(std::pair<FrontendFlowStepType, FunctionRelationship>(PREDICATE_STATEMENTS, WHOLE_APPLICATION));
#if HAVE_EXPERIMENTAL
         if(parameters->getOption<bool>(OPT_speculative))
            relationships.insert(std::pair<FrontendFlowStepType, FunctionRelationship>(SPECULATION_EDGES_COMPUTATION, WHOLE_APPLICATION));
         if(parameters->getOption<HLSFlowStep_Type>(OPT_controller_architecture) == HLSFlowStep_Type::PARALLEL_CONTROLLER_CREATOR)
         {
            // Silvia: this is the transformation to disable for testing
            // relationships.insert(std::pair<FrontendFlowStepType, FunctionRelationship>(SPLIT_PHINODES, WHOLE_APPLICATION));
            // relationships.insert(std::pair<FrontendFlowStepType, FunctionRelationship>(VIRTUAL_PHI_NODES_SPLIT, WHOLE_APPLICATION));
            if(parameters->isOption(OPT_chaining) and parameters->getOption<bool>(OPT_chaining))
               relationships.insert(std::pair<FrontendFlowStepType, FunctionRelationship>(PARALLEL_REGIONS_GRAPH_COMPUTATION, WHOLE_APPLICATION));
            relationships.insert(std::pair<FrontendFlowStepType, FunctionRelationship>(EXTENDED_PDG_COMPUTATION, WHOLE_APPLICATION));
            if(parameters->isOption("pdg-reduction") and parameters->getOption<bool>("pdg-reduction"))
               relationships.insert(std::pair<FrontendFlowStepType, FunctionRelationship>(REDUCED_PDG_COMPUTATION, WHOLE_APPLICATION));
         }
         else
         {
            if(not parameters->getOption<int>(OPT_gcc_openmp_simd))
               relationships.insert(std::pair<FrontendFlowStepType, FunctionRelationship>(MULTI_WAY_IF, WHOLE_APPLICATION));
         }
#else
         relationships.insert(std::pair<FrontendFlowStepType, FunctionRelationship>(MULTI_WAY_IF, WHOLE_APPLICATION));
#endif
         relationships.insert(std::pair<FrontendFlowStepType, FunctionRelationship>(SIMPLE_CODE_MOTION, WHOLE_APPLICATION));
#if HAVE_ILP_BUILT
         if((parameters->getOption<HLSFlowStep_Type>(OPT_scheduling_algorithm) == HLSFlowStep_Type::SDC_SCHEDULING))
         {
            relationships.insert(std::pair<FrontendFlowStepType, FunctionRelationship>(ADD_OP_PHI_FLOW_EDGES, WHOLE_APPLICATION));
            relationships.insert(std::pair<FrontendFlowStepType, FunctionRelationship>(SDC_CODE_MOTION, WHOLE_APPLICATION));
            relationships.insert(std::pair<FrontendFlowStepType, FunctionRelationship>(COMMUTATIVE_EXPR_RESTRUCTURING, WHOLE_APPLICATION));
            relationships.insert(std::pair<FrontendFlowStepType, FunctionRelationship>(COND_EXPR_RESTRUCTURING, WHOLE_APPLICATION));
            relationships.insert(std::pair<FrontendFlowStepType, FunctionRelationship>(REMOVE_ENDING_IF, WHOLE_APPLICATION));
         }
#endif
         relationships.insert(std::pair<FrontendFlowStepType, FunctionRelationship>(REMOVE_CLOBBER_GA, WHOLE_APPLICATION));
         relationships.insert(std::pair<FrontendFlowStepType, FunctionRelationship>(SPLIT_RETURN, WHOLE_APPLICATION));
         relationships.insert(std::pair<FrontendFlowStepType, FunctionRelationship>(SHORT_CIRCUIT_TAF, WHOLE_APPLICATION));
         relationships.insert(std::pair<FrontendFlowStepType, FunctionRelationship>(PHI_OPT, WHOLE_APPLICATION));
         if(not parameters->getOption<int>(OPT_gcc_openmp_simd))
         {
            relationships.insert(std::pair<FrontendFlowStepType, FunctionRelationship>(BIT_VALUE, WHOLE_APPLICATION));
            relationships.insert(std::pair<FrontendFlowStepType, FunctionRelationship>(BIT_VALUE_OPT, WHOLE_APPLICATION));
         }
         relationships.insert(std::pair<FrontendFlowStepType, FunctionRelationship>(MULTIPLE_ENTRY_IF_REDUCTION, WHOLE_APPLICATION));
         relationships.insert(std::pair<FrontendFlowStepType, FunctionRelationship>(DEAD_CODE_ELIMINATION, WHOLE_APPLICATION));
         relationships.insert(std::pair<FrontendFlowStepType, FunctionRelationship>(EXTRACT_PATTERNS, WHOLE_APPLICATION));
         relationships.insert(std::pair<FrontendFlowStepType, FunctionRelationship>(REBUILD_INITIALIZATION, WHOLE_APPLICATION));
         relationships.insert(std::pair<FrontendFlowStepType, FunctionRelationship>(REBUILD_INITIALIZATION2, WHOLE_APPLICATION));
         //         relationships.insert(std::pair<FrontendFlowStepType, FunctionRelationship>(IPA_POINT_TO_ANALYSIS, WHOLE_APPLICATION));
         relationships.insert(std::pair<FrontendFlowStepType, FunctionRelationship>(PARM2SSA, WHOLE_APPLICATION));
         relationships.insert(std::pair<FrontendFlowStepType, FunctionRelationship>(STRING_CST_FIX, WHOLE_APPLICATION));
         relationships.insert(std::pair<FrontendFlowStepType, FunctionRelationship>(NI_SSA_LIVENESS, WHOLE_APPLICATION));
         relationships.insert(std::pair<FrontendFlowStepType, FunctionRelationship>(COMPLETE_CALL_GRAPH, WHOLE_APPLICATION));
         relationships.insert(std::pair<FrontendFlowStepType, FunctionRelationship>(CSE_STEP, WHOLE_APPLICATION));
         relationships.insert(std::pair<FrontendFlowStepType, FunctionRelationship>(FANOUT_OPT, WHOLE_APPLICATION));
#if HAVE_PRAGMA_BUILT
         if((parameters->isOption(OPT_parse_pragma) and parameters->getOption<bool>(OPT_parse_pragma)) or parameters->getOption<int>(OPT_gcc_openmp_simd))
         {
            relationships.insert(std::pair<FrontendFlowStepType, FunctionRelationship>(PRAGMA_SUBSTITUTION, WHOLE_APPLICATION));
            relationships.insert(std::pair<FrontendFlowStepType, FunctionRelationship>(PRAGMA_ANALYSIS, WHOLE_APPLICATION));
         }
         if(parameters->getOption<int>(OPT_parse_pragma))
         {
            relationships.insert(std::pair<FrontendFlowStepType, FunctionRelationship>(LOOPS_ANALYSIS_BAMBU, WHOLE_APPLICATION));
            relationships.insert(std::pair<FrontendFlowStepType, FunctionRelationship>(EXTRACT_OMP_ATOMIC, WHOLE_APPLICATION));
            relationships.insert(std::pair<FrontendFlowStepType, FunctionRelationship>(EXTRACT_OMP_FOR, WHOLE_APPLICATION));
#if HAVE_EXPERIMENTAL
            if(not parameters->isOption(OPT_context_switch))
            {
               relationships.insert(std::pair<FrontendFlowStepType, FunctionRelationship>(UNROLL_LOOPS, WHOLE_APPLICATION));
            }
#endif
         }
         if(parameters->getOption<int>(OPT_gcc_openmp_simd))
         {
            relationships.insert(std::pair<FrontendFlowStepType, FunctionRelationship>(VECTORIZE, WHOLE_APPLICATION));
         }
#endif
#if HAVE_HOST_PROFILING_BUILT
         if(parameters->getOption<HostProfiling_Method>(OPT_profiling_method) != HostProfiling_Method::PM_NONE)
         {
            relationships.insert(std::pair<FrontendFlowStepType, FunctionRelationship>(HOST_PROFILING, WHOLE_APPLICATION));
         }
#endif
         const auto hdl_writer_type = static_cast<HDLWriter_Language>(parameters->getOption<unsigned int>(OPT_writer_language));
         if(hdl_writer_type == HDLWriter_Language::VHDL)
         {
            relationships.insert(std::pair<FrontendFlowStepType, FunctionRelationship>(HDL_FUNCTION_DECL_FIX, WHOLE_APPLICATION));
         }
#if HAVE_TASTE
         if(parameters->getOption<bool>(OPT_generate_taste_architecture))
         {
            relationships.insert(std::pair<FrontendFlowStepType, FunctionRelationship>(CREATE_ADDRESS_TRANSLATION, WHOLE_APPLICATION));
         }
#endif
         if(parameters->getOption<HLSFlowStep_Type>(OPT_interface_type) == HLSFlowStep_Type::INFERRED_INTERFACE_GENERATION)
         {
            relationships.insert(std::pair<FrontendFlowStepType, FunctionRelationship>(INTERFACE_INFER, WHOLE_APPLICATION));
         }
         break;
      }
      case(INVALIDATION_RELATIONSHIP):
      {
         break;
      }
      case(PRECEDENCE_RELATIONSHIP):
      {
         break;
      }
      default:
      {
         THROW_UNREACHABLE("");
      }
   }
   return relationships;
}

DesignFlowStep_Status BambuFrontendFlow::Exec()
{
#ifndef NDEBUG
   if(parameters->getOption<bool>(OPT_print_dot) or debug_level >= DEBUG_LEVEL_PEDANTIC)
   {
      AppM->CGetCallGraphManager()->CGetCallGraph()->WriteDot("call_graph_final.dot");
   }
#endif
   return DesignFlowStep_Status::EMPTY;
}

bool BambuFrontendFlow::HasToBeExecuted() const
{
   return true;
}

void BambuFrontendFlow::ComputeRelationships(DesignFlowStepSet& relationship, const DesignFlowStep::RelationshipType relationship_type)
{
   if(parameters->getOption<bool>(OPT_parse_pragma) and relationship_type == DesignFlowStep::DEPENDENCE_RELATIONSHIP)
   {
#if HAVE_EXPERIMENTAL
      const auto TM = AppM->get_tree_manager();
      const FrontendFlowStepFactory* frontend_flow_step_factory = GetPointer<const FrontendFlowStepFactory>(design_flow_manager.lock()->CGetDesignFlowStepFactory("Frontend"));
      CustomOrderedSet<FrontendFlowStepType> step_types;
      if(parameters->isOption(OPT_chaining) and parameters->getOption<bool>(OPT_chaining))
         step_types.insert(PARALLEL_REGIONS_GRAPH_COMPUTATION);
      step_types.insert(EXTENDED_PDG_COMPUTATION);
      for(const auto body_function : AppM->CGetCallGraphManager()->GetReachedBodyFunctions())
      {
         if(GetPointer<const function_decl>(TM->CGetTreeNode(body_function))->omp_for_wrapper)
         {
            for(const auto step_type : step_types)
            {
               const auto step = design_flow_manager.lock()->GetDesignFlowStep(FunctionFrontendFlowStep::ComputeSignature(step_type, body_function));
               const auto design_flow_step = step ? design_flow_manager.lock()->CGetDesignFlowGraph()->CGetDesignFlowStepInfo(step)->design_flow_step : frontend_flow_step_factory->CreateFunctionFrontendFlowStep(step_type, body_function);
               relationship.insert(design_flow_step);
            }
         }
      }
#endif
   }
   ApplicationFrontendFlowStep::ComputeRelationships(relationship, relationship_type);
}
