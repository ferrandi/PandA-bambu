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
 * @file bambu_frontend_flow.cpp
 * @brief The step representing the frontend flow for bambu
 *
 * @author Marco Lattuada <marco.lattuada@polimi.it>
 */
#include "bambu_frontend_flow.hpp"

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

BambuFrontendFlow::BambuFrontendFlow(const application_managerRef _AppM,
                                     const DesignFlowManagerConstRef _design_flow_manager,
                                     const ParameterConstRef _parameters)
    : ApplicationFrontendFlowStep(_AppM, BAMBU_FRONTEND_FLOW, _design_flow_manager, _parameters)
{
   debug_level = parameters->get_class_debug_level(GET_CLASS(*this));
}

BambuFrontendFlow::~BambuFrontendFlow() = default;

CustomUnorderedSet<std::pair<FrontendFlowStepType, FrontendFlowStep::FunctionRelationship>>
BambuFrontendFlow::ComputeFrontendRelationships(const DesignFlowStep::RelationshipType relationship_type) const
{
   CustomUnorderedSet<std::pair<FrontendFlowStepType, FunctionRelationship>> relationships;
   switch(relationship_type)
   {
      case(DEPENDENCE_RELATIONSHIP):
      {
         relationships.insert(std::make_pair(ADD_ARTIFICIAL_CALL_FLOW_EDGES, WHOLE_APPLICATION));
         relationships.insert(std::make_pair(ADD_OP_EXIT_FLOW_EDGES, WHOLE_APPLICATION));
         // relationships.insert(std::make_pair(ADD_OP_LOOP_FLOW_EDGES, WHOLE_APPLICATION));
         relationships.insert(std::make_pair(VIRTUAL_AGGREGATE_DATA_FLOW_ANALYSIS, WHOLE_APPLICATION));
         relationships.insert(std::make_pair(OP_CONTROL_DEPENDENCE_COMPUTATION, WHOLE_APPLICATION));
         relationships.insert(std::make_pair(SCALAR_SSA_DATA_FLOW_ANALYSIS, WHOLE_APPLICATION));

         if(!parameters->getOption<int>(OPT_gcc_openmp_simd))
         {
            relationships.insert(std::make_pair(BITVALUE_RANGE, ALL_FUNCTIONS));
         }
         relationships.insert(std::make_pair(BLOCK_FIX, WHOLE_APPLICATION));
         relationships.insert(std::make_pair(CALL_EXPR_FIX, WHOLE_APPLICATION));
         relationships.insert(std::make_pair(COMPLETE_CALL_GRAPH, WHOLE_APPLICATION));
         relationships.insert(std::make_pair(CSE_STEP, WHOLE_APPLICATION));
         relationships.insert(std::make_pair(CHECK_SYSTEM_TYPE, WHOLE_APPLICATION));
         relationships.insert(std::make_pair(DEAD_CODE_ELIMINATION, WHOLE_APPLICATION));
         relationships.insert(std::make_pair(DEAD_CODE_ELIMINATION_IPA, WHOLE_APPLICATION));
         relationships.insert(std::make_pair(DETERMINE_MEMORY_ACCESSES, WHOLE_APPLICATION));
         relationships.insert(std::make_pair(EXTRACT_PATTERNS, WHOLE_APPLICATION));
         relationships.insert(std::make_pair(FANOUT_OPT, WHOLE_APPLICATION));
         relationships.insert(std::make_pair(FIX_STRUCTS_PASSED_BY_VALUE, WHOLE_APPLICATION));
         relationships.insert(std::make_pair(FIX_VDEF, WHOLE_APPLICATION));
         if(!parameters->IsParameter("function-opt") || parameters->GetParameter<bool>("function-opt"))
         {
            relationships.insert(std::make_pair(FUNCTION_CALL_OPT, WHOLE_APPLICATION));
         }
         relationships.insert(std::make_pair(FUNCTION_CALL_TYPE_CLEANUP, WHOLE_APPLICATION));
         if(parameters->getOption<HDLWriter_Language>(OPT_writer_language) == HDLWriter_Language::VHDL)
         {
            relationships.insert(std::make_pair(HDL_FUNCTION_DECL_FIX, WHOLE_APPLICATION));
         }
         relationships.insert(std::make_pair(SOFT_INT_CG_EXT, WHOLE_APPLICATION));
         relationships.insert(std::make_pair(MULT_EXPR_FRACTURING, WHOLE_APPLICATION));
         relationships.insert(std::make_pair(INTERFACE_INFER, WHOLE_APPLICATION));
         relationships.insert(std::make_pair(IR_LOWERING, ALL_FUNCTIONS));
         relationships.insert(std::make_pair(LUT_TRANSFORMATION, WHOLE_APPLICATION));
         relationships.insert(std::make_pair(MULTIPLE_ENTRY_IF_REDUCTION, WHOLE_APPLICATION));
         relationships.insert(std::make_pair(NI_SSA_LIVENESS, WHOLE_APPLICATION));
         relationships.insert(std::make_pair(PHI_OPT, WHOLE_APPLICATION));
         relationships.insert(std::make_pair(PARM2SSA, WHOLE_APPLICATION));
         relationships.insert(std::make_pair(PREDICATE_STATEMENTS, WHOLE_APPLICATION));
         relationships.insert(std::make_pair(REBUILD_INITIALIZATION, WHOLE_APPLICATION));
         relationships.insert(std::make_pair(REBUILD_INITIALIZATION2, WHOLE_APPLICATION));
         relationships.insert(std::make_pair(REMOVE_CLOBBER_GA, WHOLE_APPLICATION));
         relationships.insert(std::make_pair(SHORT_CIRCUIT_TAF, WHOLE_APPLICATION));
         relationships.insert(std::make_pair(SIMPLE_CODE_MOTION, WHOLE_APPLICATION));
         if(parameters->isOption(OPT_soft_float) && parameters->getOption<bool>(OPT_soft_float))
         {
            relationships.insert(std::make_pair(SOFT_FLOAT_CG_EXT, WHOLE_APPLICATION));
         }
         relationships.insert(std::make_pair(SPLIT_RETURN, WHOLE_APPLICATION));
         relationships.insert(std::make_pair(STRING_CST_FIX, WHOLE_APPLICATION));
         relationships.insert(std::make_pair(SWITCH_FIX, WHOLE_APPLICATION));
         relationships.insert(std::make_pair(TREE2FUN, WHOLE_APPLICATION));
         relationships.insert(std::make_pair(LOOPS_ANALYSIS_BAMBU, WHOLE_APPLICATION));

         relationships.insert(std::make_pair(MULTI_WAY_IF, WHOLE_APPLICATION));
#if HAVE_ILP_BUILT
         if(parameters->getOption<HLSFlowStep_Type>(OPT_scheduling_algorithm) == HLSFlowStep_Type::SDC_SCHEDULING)
         {
            relationships.insert(std::make_pair(ADD_OP_PHI_FLOW_EDGES, WHOLE_APPLICATION));
            relationships.insert(std::make_pair(COMMUTATIVE_EXPR_RESTRUCTURING, WHOLE_APPLICATION));
            relationships.insert(std::make_pair(COND_EXPR_RESTRUCTURING, WHOLE_APPLICATION));
            relationships.insert(std::make_pair(REMOVE_ENDING_IF, WHOLE_APPLICATION));
            relationships.insert(std::make_pair(SDC_CODE_MOTION, WHOLE_APPLICATION));
         }
#endif
#if HAVE_PRAGMA_BUILT
         if((parameters->isOption(OPT_parse_pragma) && parameters->getOption<bool>(OPT_parse_pragma)) ||
            parameters->getOption<int>(OPT_gcc_openmp_simd))
         {
            relationships.insert(std::make_pair(PRAGMA_ANALYSIS, WHOLE_APPLICATION));
            relationships.insert(std::make_pair(PRAGMA_SUBSTITUTION, WHOLE_APPLICATION));
         }
         if(parameters->getOption<int>(OPT_parse_pragma))
         {
            relationships.insert(std::make_pair(EXTRACT_OMP_ATOMIC, WHOLE_APPLICATION));
            relationships.insert(std::make_pair(EXTRACT_OMP_FOR, WHOLE_APPLICATION));
         }
         if(parameters->getOption<int>(OPT_gcc_openmp_simd))
         {
            relationships.insert(std::make_pair(VECTORIZE, WHOLE_APPLICATION));
         }
#endif
#if HAVE_HOST_PROFILING_BUILT
         if(parameters->getOption<HostProfiling_Method>(OPT_profiling_method) != HostProfiling_Method::PM_NONE)
         {
            relationships.insert(std::make_pair(HOST_PROFILING, WHOLE_APPLICATION));
         }
#endif
#if HAVE_TASTE
         if(parameters->getOption<bool>(OPT_generate_taste_architecture))
         {
            relationships.insert(std::make_pair(CREATE_ADDRESS_TRANSLATION, WHOLE_APPLICATION));
         }
#endif
         break;
      }
      case(PRECEDENCE_RELATIONSHIP):
      {
         break;
      }
      case(INVALIDATION_RELATIONSHIP):
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

bool BambuFrontendFlow::HasToBeExecuted() const
{
   return true;
}

DesignFlowStep_Status BambuFrontendFlow::Exec()
{
   if(parameters->getOption<bool>(OPT_print_dot) || debug_level >= DEBUG_LEVEL_PEDANTIC)
   {
      AppM->CGetCallGraphManager()->CGetCallGraph()->WriteDot("call_graph_final.dot");
   }
   return DesignFlowStep_Status::EMPTY;
}
