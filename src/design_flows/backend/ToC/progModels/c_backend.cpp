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
 * @file c_backend.cpp
 * @brief Simple class used to drive the backend in order to be able to print c source code
 *
 * @author Luca Fossati <fossati@elet.polimi.it>
 * @author Marco Lattuada <lattuada@elet.polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *
 */

// include for autoheader
#include "config_PACKAGE_NAME.hpp"
#include "config_RELEASE.hpp"

/// Header include
#include "c_backend.hpp"

/// design_flows include
#include "design_flow_graph.hpp"
#include "design_flow_manager.hpp"

/// design_flows/backend/ToC
#include "c_backend_step_factory.hpp"

/// design_flows/backend/ToC/source_code_writers include
#include "c_writer.hpp"

/// frontend_analysis
#include "application_frontend_flow_step.hpp"
#include "frontend_flow_step.hpp"
#include "frontend_flow_step_factory.hpp"

/// Behavior include
#include "application_manager.hpp"
#include "behavioral_helper.hpp"
#include "call_graph.hpp"
#include "call_graph_manager.hpp"
#include "function_behavior.hpp"
#include "op_graph.hpp"
#include "prettyPrintVertex.hpp"

/// Graph include
#include "graph.hpp"

#if HAVE_BAMBU_BUILT
/// HLS include
#include "hls_flow_step_factory.hpp"
#include "hls_function_step.hpp"
#endif

/// Paramter include
#include "Parameter.hpp"

/// STD include
#include <fstream>
#include <iosfwd>
#include <ostream>
#include <sstream>
#include <string>

/// STL include
#include "custom_map.hpp"
#include "custom_set.hpp"
#include <deque>
#include <list>
#include <utility>
#include <vector>

/// tree includes
#include "tree_helper.hpp"
#include "tree_manager.hpp"
#include "var_pp_functor.hpp"

/// Utility include
#include "boost/filesystem/operations.hpp"
#include "boost/filesystem/path.hpp"
#include "dbgPrintHelper.hpp" // for DEBUG_LEVEL_NONE
#include "indented_output_stream.hpp"
#include "refcount.hpp"
#include "string_manipulation.hpp" // for GET_CLASS
#include "utility.hpp"
#include <boost/config.hpp>
#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/filtered_graph.hpp>
#include <boost/graph/graph_utility.hpp>
#include <boost/graph/topological_sort.hpp>
#include <boost/lexical_cast.hpp>

CBackend::CBackend(const Type _c_backend_type, const CBackendInformationConstRef c_backend_information, const DesignFlowManagerConstRef _design_flow_manager, const application_managerConstRef _AppM, std::string _file_name,
                   const ParameterConstRef _parameters)
    : DesignFlowStep(_design_flow_manager, _parameters),
      indented_output_stream(new IndentedOutputStream()),
      writer(CWriter::CreateCWriter(_c_backend_type, c_backend_information, _AppM, indented_output_stream, _parameters, _parameters->getOption<int>(OPT_debug_level) >= DEBUG_LEVEL_VERBOSE)),
      file_name(std::move(_file_name)),
      AppM(_AppM),
      TM(_AppM->get_tree_manager()),
      c_backend_type(_c_backend_type)
{
   debug_level = parameters->get_class_debug_level(GET_CLASS(*this));
}

CBackend::~CBackend() = default;

const CWriterRef CBackend::GetCWriter() const
{
   return writer;
}

DesignFlowStep_Status CBackend::Exec()
{
   INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "-->CBackend: writing " + file_name);
   // first write panda header
   INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "-->CBackend: writing panda header");
   indented_output_stream->Append("/*\n");
   indented_output_stream->Append(" * Politecnico di Milano\n");
   indented_output_stream->Append(std::string(" * Code created using ") + PACKAGE_NAME + " - " + parameters->PrintVersion());
   indented_output_stream->Append(" - Date " + TimeStamp::GetCurrentTimeStamp());
   indented_output_stream->Append("\n");
   if(parameters->isOption(OPT_cat_args))
      indented_output_stream->Append(" * " + parameters->getOption<std::string>(OPT_program_name) + " executed with: " + parameters->getOption<std::string>(OPT_cat_args) + "\n");
   indented_output_stream->Append(" */\n");
   INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "<--CBackend: written panda header");
   // write cwriter specific header
   INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "-->CBackend: writing header");
   writer->WriteHeader();
   INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "<--CBackend: written header");
   writeIncludes();
   WriteGlobalDeclarations();
   writeImplementations();
   writer->WriteFile(file_name);
   INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "<--Cbackend: written " + file_name);
   return DesignFlowStep_Status::SUCCESS;
}

bool CBackend::HasToBeExecuted() const
{
   return true;
}

void CBackend::Initialize()
{
   writer->Initialize();
   if(boost::filesystem::exists(file_name))
      boost::filesystem::remove_all(file_name);
   already_visited.clear();
   if(c_backend_type == CB_HLS)
   {
      functions_to_be_declared = AppM->CGetCallGraphManager()->GetRootFunctions();
   }
   else
   {
      functions_to_be_declared = AppM->get_functions_without_body();
      functions_to_be_defined = AppM->get_functions_with_body();
   }
}

void CBackend::WriteGlobalDeclarations()
{
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->CBackend: Writing global declarations");
   writer->WriteGlobalDeclarations();
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Writing function prototypes");
   for(const auto extBeg : functions_to_be_declared)
   {
      const BehavioralHelperConstRef BH = AppM->CGetFunctionBehavior(extBeg)->CGetBehavioralHelper();
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Writing external function prototype: " + BH->get_function_name());
      if(BH->function_has_to_be_printed(extBeg))
      {
         writer->DeclareFunctionTypes(extBeg);
         writer->WriteFunctionDeclaration(extBeg);
      }
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Written external function prototype: " + BH->get_function_name());
   }

   CustomOrderedSet<unsigned int> functions = functions_to_be_defined;
   for(const auto it : functions)
   {
      const BehavioralHelperConstRef BH = AppM->CGetFunctionBehavior(it)->CGetBehavioralHelper();
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Writing function prototype of " + BH->get_function_name());

#if HAVE_BAMBU_BUILT
      if(parameters->isOption(OPT_pretty_print))
      {
         std::string f_name = BH->get_function_name();
         if(boost::algorithm::starts_with(f_name, "__builtin_"))
         {
            indented_output_stream->Append("#define " + f_name + " _bambu_" + f_name + "\n");
         }
      }
#endif

      if(BH->function_has_to_be_printed(it))
      {
         writer->DeclareFunctionTypes(it);
         writer->WriteFunctionDeclaration(it);
      }
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Written function prototype of " + BH->get_function_name());
   }
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Written function prototypes");
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--CBackend: Written global declarations");
}

void CBackend::writeImplementations()
{
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Writing implementations");
   // First of all I declare the functions and then the tasks
   for(const auto it : functions_to_be_defined)
   {
      const BehavioralHelperConstRef BH = AppM->CGetFunctionBehavior(it)->CGetBehavioralHelper();
      if(BH->function_has_to_be_printed(it))
         writer->WriteFunctionImplementation(it);
   }
   if(AppM->CGetCallGraphManager()->ExistsAddressedFunction())
      writer->WriteBuiltinWaitCall();
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Written implementations");
}

void CBackend::writeIncludes()
{
   CustomOrderedSet<std::string> includes_to_write;
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Computing includes for external functions");
   for(const auto f_id : functions_to_be_defined)
   {
      const FunctionBehaviorConstRef FB = AppM->CGetFunctionBehavior(f_id);
      const BehavioralHelperConstRef BH = FB->CGetBehavioralHelper();
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Computing includes for " + BH->get_function_name());
      AnalyzeInclude(f_id, BH, includes_to_write);

      CustomUnorderedSet<unsigned int> decl_nodes;
      const CustomSet<unsigned int>& tmp_vars = writer->GetLocalVariables(f_id);
      decl_nodes.insert(tmp_vars.begin(), tmp_vars.end());
      const std::list<unsigned int>& funParams = BH->get_parameters();
      decl_nodes.insert(funParams.begin(), funParams.end());
      const CustomSet<unsigned int> vars = AppM->get_global_variables();
      decl_nodes.insert(vars.begin(), vars.end());

      for(const auto& v : decl_nodes)
      {
         const unsigned int variable_type = BH->get_type(v);
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Analyzing includes for variable " + BH->PrintVariable(v) + " of type " + STR(variable_type));
         AnalyzeInclude(variable_type, BH, includes_to_write);
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Analyzed includes for variable " + BH->PrintVariable(v) + " of type " + STR(variable_type));
      }

      const OpGraphConstRef op_graph = FB->CGetOpGraph(FunctionBehavior::DFG);
      VertexIterator v, vEnd;
      for(boost::tie(v, vEnd) = boost::vertices(*op_graph); v != vEnd; v++)
      {
         const unsigned int index = op_graph->CGetOpNodeInfo(*v)->GetNodeId();
         if(index != ENTRY_ID and index != EXIT_ID)
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Analyzing includes for operation" + STR(*v));
            CustomUnorderedSet<unsigned int> types;
            BH->get_typecast(index, types);
            for(const auto type_id : types)
            {
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Analyzing includes for type" + STR(type_id));
               AnalyzeInclude(type_id, BH, includes_to_write);
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Analyzed includes for type" + STR(type_id));
            }
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Analyzed includes for operation" + STR(*v));
         }
      }
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Computed includes for " + BH->get_function_name());
   }
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Computed includes for external functions");

   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Writing includes");
   for(const auto& s : includes_to_write)
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Writing: " + s);
      writer->writeInclude(s);
   }
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Written includes");
}

void CBackend::AnalyzeInclude(unsigned int index, const BehavioralHelperConstRef BH, CustomOrderedSet<std::string>& includes_to_write)
{
   if(already_visited.find(index) != already_visited.end())
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Skipped already analyzed " + STR(index));
      return;
   }
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Computing include for (" + STR(index) + ")" + STR(TM->CGetTreeNode(index)));
   already_visited.insert(index);
   bool is_system;
   const std::string decl = std::get<0>(BH->get_definition(index, is_system));
   if(not decl.empty() and decl != "<built-in>" and is_system
#if HAVE_BAMBU_BUILT
      and not tree_helper::IsInLibbambu(TM, index)
#endif
   )
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Adding " + decl + " to the list of includes");
      includes_to_write.insert(decl);
   }
   else
   {
      const unsigned int type = tree_helper::get_type_index(TM, index);
      const auto types_to_be_declared_before = tree_helper::GetTypesToBeDeclaredBefore(TM, type, parameters->getOption<bool>(OPT_without_transformation));
      for(const auto type_to_be_declared : types_to_be_declared_before)
      {
         AnalyzeInclude(type_to_be_declared, BH, includes_to_write);
      }
      const auto types_to_be_declared_after = tree_helper::GetTypesToBeDeclaredAfter(TM, type, parameters->getOption<bool>(OPT_without_transformation));
      for(const auto type_to_be_declared : types_to_be_declared_after)
      {
         AnalyzeInclude(type_to_be_declared, BH, includes_to_write);
      }
   }
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Computed");
}

void CBackend::compute_variables(const OpGraphConstRef inGraph, const CustomUnorderedSet<unsigned int>& gblVariables, std::list<unsigned int>& funParams, CustomUnorderedSet<unsigned int>& vars)
{
   // I simply have to go over all the vertices and get the used variables;
   // the variables which have to be declared are all those variables but
   // the globals ones
   VertexIterator v, vEnd;
   for(boost::tie(v, vEnd) = boost::vertices(*inGraph); v != vEnd; v++)
   {
      const CustomSet<unsigned int> vars_temp = inGraph->CGetOpNodeInfo(*v)->cited_variables;
      vars.insert(vars_temp.begin(), vars_temp.end());
   }

   // I have to take out the variables global to the whole program and the function parameters
   CustomUnorderedSet<unsigned int>::const_iterator it, it_end = gblVariables.end();
   for(it = gblVariables.begin(); it != it_end; ++it)
   {
      vars.erase(*it);
   }
   std::list<unsigned int>::const_iterator it2, it2_end = funParams.end();
   for(it2 = funParams.begin(); it2 != it2_end; ++it2)
   {
      vars.erase(*it2);
   }
}

const DesignFlowStepFactoryConstRef CBackend::CGetDesignFlowStepFactory() const
{
   return design_flow_manager.lock()->CGetDesignFlowStepFactory("CBackend");
}

const std::string CBackend::GetSignature() const
{
   return ComputeSignature(c_backend_type);
}

const std::string CBackend::ComputeSignature(const CBackend::Type type)
{
   switch(type)
   {
#if HAVE_HOST_PROFILING_BUILT
      case(CB_BBP):
         return "CBackend::BasicBlocksProfiling";
#endif
#if HAVE_HLS_BUILT
      case(CB_DISCREPANCY_ANALYSIS):
         return "CBackend::DiscrepancyAnalysis";
#endif
#if HAVE_TARGET_PROFILING
      case(CB_ESCAPED_SEQUENTIAL):
         return "CBackend::Escaped";
#endif
#if HAVE_BAMBU_BUILT
      case(CB_HLS):
         return "CBackend::HighLevelSynthesis";
#endif
#if HAVE_GRAPH_PARTITIONING_BUILT && HAVE_TARGET_PROFILING
      case(CB_INSTRUMENTED_PARALLEL):
         return "CBackend::InstrumentedParallel";
#endif
#if HAVE_TARGET_PROFILING
      case(CB_INSTRUMENTED_SEQUENTIAL):
         return "CBackend::InstrumentedSequential";
#endif
#if HAVE_ZEBU_BUILT
      case(CB_POINTED_DATA_EVALUATION):
         return "CBackend::PointedDataEvaluation";
#endif
#if HAVE_GRAPH_PARTITIONING_BUILT
      case(CB_PARALLEL):
         return "CBackend::Parallel";
#endif
      case(CB_SEQUENTIAL):
         return "CBackend::Sequential";
      default:
      {
         THROW_UNREACHABLE("");
      }
   }
   return "";
}

const std::string CBackend::GetName() const
{
   return GetSignature();
}

void CBackend::ComputeRelationships(DesignFlowStepSet& relationships, const DesignFlowStep::RelationshipType relationship_type)
{
   switch(relationship_type)
   {
      case(DEPENDENCE_RELATIONSHIP):
      {
         switch(c_backend_type)
         {
            case CB_SEQUENTIAL:
            {
               CustomUnorderedSet<std::pair<FrontendFlowStepType, FrontendFlowStep::FunctionRelationship>> frontend_relationships;
               frontend_relationships.insert(std::pair<FrontendFlowStepType, FrontendFlowStep::FunctionRelationship>(BAMBU_FRONTEND_FLOW, FrontendFlowStep::WHOLE_APPLICATION));
               FrontendFlowStep::CreateSteps(design_flow_manager.lock(), frontend_relationships, AppM, relationships);
               break;
            }
#if HAVE_HOST_PROFILING_BUILT
            case(CB_BBP):
            {
               CustomUnorderedSet<std::pair<FrontendFlowStepType, FrontendFlowStep::FunctionRelationship>> frontend_relationships;
               frontend_relationships.insert(std::pair<FrontendFlowStepType, FrontendFlowStep::FunctionRelationship>(BASIC_BLOCKS_CFG_COMPUTATION, FrontendFlowStep::ALL_FUNCTIONS));
               frontend_relationships.insert(std::pair<FrontendFlowStepType, FrontendFlowStep::FunctionRelationship>(DEAD_CODE_ELIMINATION, FrontendFlowStep::ALL_FUNCTIONS));
               frontend_relationships.insert(std::pair<FrontendFlowStepType, FrontendFlowStep::FunctionRelationship>(LOOP_COMPUTATION, FrontendFlowStep::ALL_FUNCTIONS));
               frontend_relationships.insert(std::pair<FrontendFlowStepType, FrontendFlowStep::FunctionRelationship>(NI_SSA_LIVENESS, FrontendFlowStep::ALL_FUNCTIONS));
               frontend_relationships.insert(std::pair<FrontendFlowStepType, FrontendFlowStep::FunctionRelationship>(OPERATIONS_CFG_COMPUTATION, FrontendFlowStep::ALL_FUNCTIONS));
               frontend_relationships.insert(std::pair<FrontendFlowStepType, FrontendFlowStep::FunctionRelationship>(VAR_ANALYSIS, FrontendFlowStep::ALL_FUNCTIONS));
               FrontendFlowStep::CreateSteps(design_flow_manager.lock(), frontend_relationships, AppM, relationships);
               break;
            }
#endif
#if HAVE_HLS_BUILT
            case(CB_DISCREPANCY_ANALYSIS):
            case(CB_HLS):
            {
               // The first time this step is added, we add the dependence
               // from the complete call graph computation. Ideally we would
               // need also the dependence from the HLS steps,
               // but at this point we don't know the top function yet.
               // The trick is that the dependencies will be recomputed again
               // before this is executed. At that time the top
               // function will be ready. The dependencies from HLS steps are
               // added after the check on the call graph for this reason.
               const auto* frontend_step_factory = GetPointer<const FrontendFlowStepFactory>(design_flow_manager.lock()->CGetDesignFlowStepFactory("Frontend"));

               const vertex call_graph_computation_step = design_flow_manager.lock()->GetDesignFlowStep(ApplicationFrontendFlowStep::ComputeSignature(FUNCTION_ANALYSIS));

               const DesignFlowGraphConstRef design_flow_graph = design_flow_manager.lock()->CGetDesignFlowGraph();

               const DesignFlowStepRef cg_design_flow_step =
                   call_graph_computation_step ? design_flow_graph->CGetDesignFlowStepInfo(call_graph_computation_step)->design_flow_step : frontend_step_factory->CreateApplicationFrontendFlowStep(FUNCTION_ANALYSIS);

               relationships.insert(cg_design_flow_step);

               // Root function cannot be computed at the beginning so if the
               // call graph is not ready yet we exit. The relationships will
               // be computed again after the call graph computation.
               const CallGraphManagerConstRef call_graph_manager = AppM->CGetCallGraphManager();
               if(boost::num_vertices(*(call_graph_manager->CGetCallGraph())) == 0)
                  break;

               const auto* hls_step_factory = GetPointer<const HLSFlowStepFactory>(design_flow_manager.lock()->CGetDesignFlowStepFactory("HLS"));

               relationships.insert(hls_step_factory->CreateHLSFlowStep(HLSFlowStep_Type::TESTBENCH_MEMORY_ALLOCATION, HLSFlowStepSpecializationConstRef()));

               relationships.insert(hls_step_factory->CreateHLSFlowStep(HLSFlowStep_Type::TEST_VECTOR_PARSER, HLSFlowStepSpecializationConstRef()));
               const bool is_hw_discrepancy = parameters->isOption(OPT_discrepancy_hw) and parameters->getOption<bool>(OPT_discrepancy_hw);
               if(c_backend_type == CB_DISCREPANCY_ANALYSIS and not is_hw_discrepancy)
               {
                  relationships.insert(hls_step_factory->CreateHLSFlowStep(HLSFlowStep_Type::VCD_SIGNAL_SELECTION, HLSFlowStepSpecializationConstRef()));
               }
               if(parameters->isOption(OPT_pretty_print))
               {
                  const auto* c_backend_step_factory = GetPointer<const CBackendStepFactory>(design_flow_manager.lock()->CGetDesignFlowStepFactory("CBackend"));
                  const std::string output_file_name = parameters->getOption<std::string>(OPT_pretty_print);
                  const vertex c_backend_vertex = design_flow_manager.lock()->GetDesignFlowStep(CBackend::ComputeSignature(CBackend::CB_SEQUENTIAL));
                  const DesignFlowStepRef c_backend_step =
                      c_backend_vertex ? design_flow_graph->CGetDesignFlowStepInfo(c_backend_vertex)->design_flow_step : c_backend_step_factory->CreateCBackendStep(CBackend::CB_SEQUENTIAL, output_file_name, CBackendInformationConstRef());
                  relationships.insert(c_backend_step);
               }

               break;
            }
#endif
#if HAVE_TARGET_PROFILING
            case(CB_ESCAPED_SEQUENTIAL):
#endif
#if HAVE_GRAPH_PARTITIONING_BUILT && HAVE_TARGET_PROFILING
            case(CB_INSTRUMENTED_PARALLEL):
#endif
#if HAVE_TARGET_PROFILING
            case(CB_INSTRUMENTED_SEQUENTIAL):
#endif
#if HAVE_ZEBU_BUILT
            case(CB_POINTED_DATA_EVALUATION):
#endif
#if HAVE_GRAPH_PARTITIONING_BUILT
            case(CB_PARALLEL):
#endif
            default:
            {
               THROW_UNREACHABLE("");
            }
         }
         break;
      }
      case(INVALIDATION_RELATIONSHIP):
      {
         break;
      }
      case(PRECEDENCE_RELATIONSHIP):
      {
         switch(c_backend_type)
         {
            case CB_SEQUENTIAL:
#if HAVE_HOST_PROFILING_BUILT
            case(CB_BBP):
#endif
            {
#if HAVE_BAMBU_BUILT
               CustomUnorderedSet<std::pair<FrontendFlowStepType, FrontendFlowStep::FunctionRelationship>> frontend_relationships;
               frontend_relationships.insert(std::pair<FrontendFlowStepType, FrontendFlowStep::FunctionRelationship>(FrontendFlowStepType::MULTIPLE_ENTRY_IF_REDUCTION, FrontendFlowStep::ALL_FUNCTIONS));
               FrontendFlowStep::CreateSteps(design_flow_manager.lock(), frontend_relationships, AppM, relationships);
               if(c_backend_type == CB_SEQUENTIAL)
               {
                  // The first time this step is added, we add the dependence
                  // from the complete call graph computation. Ideally we would
                  // need also the dependence from the HLS steps,
                  // but at this point we don't know the top function yet.
                  // The trick is that the dependencies will be recomputed again
                  // before this is executed. At that time the top
                  // function will be ready. The dependencies from HLS steps are
                  // added after the check on the call graph for this reason.
                  const FrontendFlowStepFactory* frontend_step_factory = GetPointer<const FrontendFlowStepFactory>(design_flow_manager.lock()->CGetDesignFlowStepFactory("Frontend"));
                  const vertex call_graph_computation_step = design_flow_manager.lock()->GetDesignFlowStep(ApplicationFrontendFlowStep::ComputeSignature(FUNCTION_ANALYSIS));
                  const DesignFlowGraphConstRef design_flow_graph = design_flow_manager.lock()->CGetDesignFlowGraph();
                  const DesignFlowStepRef cg_design_flow_step =
                      call_graph_computation_step ? design_flow_graph->CGetDesignFlowStepInfo(call_graph_computation_step)->design_flow_step : frontend_step_factory->CreateApplicationFrontendFlowStep(FUNCTION_ANALYSIS);
                  relationships.insert(cg_design_flow_step);

                  // Root function cannot be computed at the beginning so if the
                  // call graph is not ready yet we exit. The relationships will
                  // be computed again after the call graph computation.
                  const CallGraphManagerConstRef call_graph_manager = AppM->CGetCallGraphManager();
                  if(boost::num_vertices(*(call_graph_manager->CGetCallGraph())) == 0)
                     break;
                  const auto top_funs = call_graph_manager->GetRootFunctions();
                  THROW_ASSERT(top_funs.size() == 1, "");
                  const auto top_fu_id = *top_funs.begin();
                  const HLSFlowStepFactory* hls_step_factory = GetPointer<const HLSFlowStepFactory>(design_flow_manager.lock()->CGetDesignFlowStepFactory("HLS"));

                  const vertex hls_top_function = design_flow_manager.lock()->GetDesignFlowStep(HLSFunctionStep::ComputeSignature(HLSFlowStep_Type::HLS_SYNTHESIS_FLOW, HLSFlowStepSpecializationConstRef(), top_fu_id));
                  const DesignFlowStepRef hls_top_function_step = hls_top_function ? design_flow_graph->CGetDesignFlowStepInfo(hls_top_function)->design_flow_step : hls_step_factory->CreateHLSFlowStep(HLSFlowStep_Type::HLS_SYNTHESIS_FLOW, top_fu_id);
                  relationships.insert(hls_top_function_step);
               }
#endif
               break;
            }
#if HAVE_HLS_BUILT
            case(CB_DISCREPANCY_ANALYSIS):
            case(CB_HLS):
            {
               break;
            }
#endif
#if HAVE_TARGET_PROFILING
            case(CB_ESCAPED_SEQUENTIAL):
#endif
#if HAVE_GRAPH_PARTITIONING_BUILT && HAVE_TARGET_PROFILING
            case(CB_INSTRUMENTED_PARALLEL):
#endif
#if HAVE_TARGET_PROFILING
            case(CB_INSTRUMENTED_SEQUENTIAL):
#endif
#if HAVE_ZEBU_BUILT
            case(CB_POINTED_DATA_EVALUATION):
#endif
#if HAVE_GRAPH_PARTITIONING_BUILT
            case(CB_PARALLEL):
#endif
            default:
            {
               THROW_UNREACHABLE("");
            }
         }
         break;
      }
      default:
      {
         THROW_UNREACHABLE("");
      }
   }
}
