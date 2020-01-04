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
 * @file c_writer.cpp
 * @brief This file contains the routines necessary to create a C executable program starting from an abstract decription of the threads composing the application.
 *
 * This file contains the routines necessary to create a C executable program
 * starting from an abstract decription of the threads composing the application.
 *
 * @author Luca Fossati <fossati@elet.polimi.it>
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 * @author Marco Lattuada <lattuada@elet.polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *
 */

/// Autoheader include
#include "config_HAVE_ARM_COMPILER.hpp"
#include "config_HAVE_GRAPH_PARTITIONING_BUILT.hpp"
#include "config_HAVE_HOST_PROFILING_BUILT.hpp"
#include "config_HAVE_SPARC_COMPILER.hpp"
#include "config_HAVE_TARGET_PROFILING.hpp"
#include "config_HAVE_TASK_GRAPHS_BUILT.hpp"
#include "config_HAVE_ZEBU_BUILT.hpp"
#include "config_PACKAGE_NAME.hpp"
#include "config_PACKAGE_VERSION.hpp"
#include "config_RELEASE.hpp"

/// Header include
#include "c_writer.hpp"

///. includes
#include "Parameter.hpp"

/// algorithms/dominance
#include "Dominance.hpp"

/// algorithms/loops_detection
#include "loop.hpp"
#include "loops.hpp"

/// behavior includes
#include "application_manager.hpp"
#include "basic_block.hpp"
#include "call_graph_manager.hpp"
#include "function_behavior.hpp"
#include "op_graph.hpp"

/// design_flows/backend/ToC includes
#include "c_backend_step_factory.hpp"
#if HAVE_TARGET_PROFILING
#include "escape_c_backend_information.hpp"
#include "instrument_c_backend_information.hpp"
#endif

/// design_flows/backend/ToC includes
#include "actor_graph_backend.hpp"

/// design_flows/backend/ToC/source_code_writers includes
#if HAVE_HOST_PROFILING_BUILT
#include "basic_blocks_profiling_c_writer.hpp"
#if HAVE_EXPERIMENTAL
#include "data_memory_profiling_c_writer.hpp"
#include "data_memory_profiling_instruction_writer.hpp"
#include "efficient_path_profiling_c_writer.hpp"
#endif
#endif
#if HAVE_TARGET_PROFILING
#include "escape_instruction_writer.hpp"
#endif
#if HAVE_EXPERIMENTAL && HAVE_HOST_PROFILING_BUILT
#include "hierarchical_path_profiling_c_writer.hpp"
#endif
#if HAVE_BAMBU_BUILT
#include "discrepancy_analysis_c_writer.hpp"
#include "discrepancy_instruction_writer.hpp"
#include "hls_c_backend_information.hpp"
#include "hls_c_writer.hpp"
#include "hls_instruction_writer.hpp"
#endif
#include "instruction_writer.hpp"
#if HAVE_TARGET_PROFILING
#include "instrument_c_writer.hpp"
#endif
#if HAVE_GRAPH_PARTITIONING_BUILT && HAVE_TARGET_PROFILING
#include "instrument_parallel_c_writer.hpp"
#endif
#if HAVE_TARGET_PROFILING
#include "instrument_writer.hpp"
#endif
#if HAVE_EXPERIMENTAL && HAVE_HOST_PROFILING_BUILT
#include "loops_profiling_c_writer.hpp"
#endif
#if HAVE_TARGET_PROFILING
#include "loops_instrument_c_writer.hpp"
#endif
#if HAVE_ZEBU_BUILT
#include "memory_profiling_c_writer.hpp"
#include "memory_profiling_instruction_writer.hpp"
#endif
#if HAVE_GRAPH_PARTITIONING_BUILT
#include "parallel_c_writer.hpp"
#endif
#if HAVE_EXPERIMENTAL && HAVE_HOST_PROFILING_BUILT
#include "tree_path_profiling_c_writer.hpp"
#endif

/// design_flows/codesign/partitioning/graph_partitioning include
#if HAVE_GRAPH_PARTITIONING_BUILT
#include "partitioning_manager.hpp"
#endif

/// tree includes
#include "behavioral_helper.hpp"
#include "ext_tree_node.hpp"
#include "tree_basic_block.hpp"
#include "tree_helper.hpp"
#include "tree_manager.hpp"
#include "tree_reindex.hpp"
#include "var_pp_functor.hpp"

/// utility include
#include "indented_output_stream.hpp"

/// wrapper/treegcc include
#include "gcc_wrapper.hpp"

/// Boost include
#include <boost/range/adaptor/reversed.hpp>

#if HAVE_UNORDERED
#include <boost/functional/hash/hash.hpp>

/// Hash function for std::pair<tree_nodeRef, tree_nodeRef>
namespace std
{
   template <>
   struct hash<std::pair<tree_nodeRef, tree_nodeRef>> : public std::unary_function<std::pair<tree_nodeRef, tree_nodeRef>, std::size_t>
   {
      std::size_t operator()(const std::pair<tree_nodeRef, tree_nodeRef>& val) const
      {
         std::size_t hash_value = 0;
         boost::hash_combine(hash_value, val.first);
         boost::hash_combine(hash_value, val.second);
         return hash_value;
      }
   };
} // namespace std
class TreeNodesPairSet : public CustomUnorderedSet<std::pair<tree_nodeRef, tree_nodeRef>>
{
};
#else
class TreeNodesPairSorter : public std::binary_function<std::pair<tree_nodeRef, tree_nodeRef>, std::pair<tree_nodeRef, tree_nodeRef>, bool>
{
 public:
   /**
    * Compare position of two const tree nodes
    * @param x is the first pair
    * @param y is the second pair
    * @return true if x is less than y
    */
   bool operator()(const std::pair<tree_nodeRef, tree_nodeRef>& x, const std::pair<tree_nodeRef, tree_nodeRef>& y) const
   {
      if(x.first->index == y.first->index)
      {
         return x.second->index < y.second->index;
      }
      else
      {
         return x.first->index < y.first->index;
      }
   }
};
class TreeNodesPairSet : public OrderedSetStd<std::pair<tree_nodeRef, tree_nodeRef>, TreeNodesPairSorter>
{
};
#endif
CWriter::CWriter(const application_managerConstRef _AppM, const InstructionWriterRef _instruction_writer, const IndentedOutputStreamRef _indented_output_stream, const ParameterConstRef _Param, bool _verbose)
    : AppM(_AppM),
      TM(_AppM->get_tree_manager()),
      indented_output_stream(_indented_output_stream),
      instrWriter(_instruction_writer),
      bb_label_counter(0),
      verbose(_verbose),
      Param(_Param),
      debug_level(_Param->get_class_debug_level("CWriter")),
      output_level(_Param->getOption<int>(OPT_output_level)),
      fake_max_tree_node_id(0),
      dominators(nullptr),
      post_dominators(nullptr)
{
}

CWriter::~CWriter() = default;

CWriterRef CWriter::CreateCWriter(const CBackend::Type type,
                                  const CBackendInformationConstRef
#if HAVE_TARGET_PROFILING || HAVE_BAMBU_BUILT
                                      c_backend_information
#endif
                                  ,
                                  const application_managerConstRef app_man, const IndentedOutputStreamRef indented_output_stream, const ParameterConstRef parameters, const bool verbose)
{
   switch(type)
   {
#if HAVE_HOST_PROFILING_BUILT
      case(CBackend::CB_BBP):
      {
         const InstructionWriterRef instruction_writer = InstructionWriter::CreateInstructionWriter(ActorGraphBackend_Type::BA_NONE, app_man, indented_output_stream, parameters);
         return CWriterRef(new BasicBlocksProfilingCWriter(app_man, instruction_writer, indented_output_stream, parameters, verbose));
      }
#endif
#if HAVE_HLS_BUILT
      case(CBackend::CB_DISCREPANCY_ANALYSIS):
      {
         const InstructionWriterRef instruction_writer(new discrepancy_instruction_writer(app_man, indented_output_stream, parameters));

         return CWriterRef(new DiscrepancyAnalysisCWriter(RefcountCast<const HLSCBackendInformation>(c_backend_information), app_man, instruction_writer, indented_output_stream, parameters, verbose));
      }
#endif
#if HAVE_TARGET_PROFILING
      case(CBackend::CB_ESCAPED_SEQUENTIAL):
      {
         const EscapeCBackendInformation* escape_c_backend_information = GetPointer<const EscapeCBackendInformation>(c_backend_information);
         const InstrumentWriterRef instrument_writer = InstrumentWriter::CreateInstrumentWriter(indented_output_stream, escape_c_backend_information->profiling_architecture, parameters);
         const InstructionWriterRef instruction_writer(
             new EscapeInstructionWriter(escape_c_backend_information->profiling_architecture, app_man, instrument_writer, indented_output_stream, escape_c_backend_information->exitings_after, escape_c_backend_information->exitings_before, parameters));
         return CWriterRef(new CWriter(app_man, instruction_writer, indented_output_stream, parameters, verbose));
      }
#endif
#if HAVE_BAMBU_BUILT
      case(CBackend::CB_HLS):
      {
         const InstructionWriterRef instruction_writer(new HLSInstructionWriter(app_man, indented_output_stream, parameters));
         return CWriterRef(new HLSCWriter(RefcountCast<const HLSCBackendInformation>(c_backend_information), app_man, instruction_writer, indented_output_stream, parameters, verbose));
      }
#endif
#if HAVE_GRAPH_PARTITIONING_BUILT && HAVE_TARGET_PROFILING
      case(CBackend::CB_INSTRUMENTED_PARALLEL):
      {
         const InstrumentCBackendInformation* instrument_c_backend_information = GetPointer<const InstrumentCBackendInformation>(c_backend_information);
         const InstrumentWriterRef instrument_writer = InstrumentWriter::CreateInstrumentWriter(indented_output_stream, instrument_c_backend_information->profiling_architecture, parameters);
         const InstructionWriterRef instruction_writer(new InstrumentInstructionWriter(instrument_c_backend_information->profiling_architecture, app_man, instrument_writer, indented_output_stream, parameters));
         return CWriterRef(new InstrumentParallelCWriter(RefcountCast<const PartitioningManager>(app_man), instrument_writer, instruction_writer, indented_output_stream, parameters, verbose));
      }
#endif
#if HAVE_TARGET_PROFILING
      case(CBackend::CB_INSTRUMENTED_SEQUENTIAL):
      {
         const InstrumentCBackendInformation* instrument_c_backend_information = GetPointer<const InstrumentCBackendInformation>(c_backend_information);
         const InstrumentWriterRef instrument_writer = InstrumentWriter::CreateInstrumentWriter(indented_output_stream, instrument_c_backend_information->profiling_architecture, parameters);
         const InstructionWriterRef instruction_writer(new InstrumentInstructionWriter(instrument_c_backend_information->profiling_architecture, app_man, instrument_writer, indented_output_stream, parameters));
         if(parameters->getOption<int>(OPT_analysis_level) >= static_cast<int>(InstrumentWriter_Level::AL_LOOP))
         {
            return CWriterRef(new LoopsInstrumentCWriter(app_man, instrument_writer, instruction_writer, indented_output_stream, parameters, verbose));
         }
         else
         {
            return CWriterRef(new InstrumentCWriter(app_man, instrument_writer, instruction_writer, indented_output_stream, parameters, verbose));
         }
      }
#endif
#if HAVE_ZEBU_BUILT
      case(CBackend::CB_POINTED_DATA_EVALUATION):
      {
         const InstructionWriterRef instruction_writer(new MemoryProfilingInstructionWriter(app_man, indented_output_stream, parameters));
         return CWriterRef(new MemoryProfilingCWriter(app_man, instruction_writer, indented_output_stream, parameters));
      }
#endif
#if HAVE_GRAPH_PARTITIONING_BUILT
      case(CBackend::CB_PARALLEL):
      {
         const InstructionWriterRef instruction_writer = InstructionWriter::CreateInstructionWriter(parameters->getOption<ActorGraphBackend_Type>(OPT_fork_join_backend), app_man, indented_output_stream, parameters);
         return CWriterRef(new ParallelCWriter(RefcountCast<const PartitioningManager>(app_man), instruction_writer, indented_output_stream, parameters, verbose));
      }
#endif
      case(CBackend::CB_SEQUENTIAL):
      {
         const InstructionWriterRef instruction_writer = InstructionWriter::CreateInstructionWriter(ActorGraphBackend_Type::BA_NONE, app_man, indented_output_stream, parameters);
         return CWriterRef(new CWriter(app_man, instruction_writer, indented_output_stream, parameters, verbose));
      }
      default:
      {
         THROW_UNREACHABLE("");
      }
   }
   return CWriterRef();
}

void CWriter::declare_cast_types(unsigned int funId, CustomSet<std::string>& locally_declared_types)
{
   const FunctionBehaviorConstRef function_behavior = AppM->CGetFunctionBehavior(funId);
   const BehavioralHelperConstRef behavioral_helper = function_behavior->CGetBehavioralHelper();
   const OpGraphConstRef inGraph = function_behavior->CGetOpGraph(FunctionBehavior::DFG);
   // I simply have to go over all the vertices and look for types used for type casting;
   VertexIterator v, vEnd;
   for(boost::tie(v, vEnd) = boost::vertices(*inGraph); v != vEnd; v++)
   {
      const unsigned int index = inGraph->CGetOpNodeInfo(*v)->GetNodeId();
      if(index != ENTRY_ID and index != EXIT_ID)
      {
         CustomUnorderedSet<unsigned int> types;
         behavioral_helper->get_typecast(index, types);
         CustomUnorderedSet<unsigned int>::const_iterator t_it_end = types.end();
         for(CustomUnorderedSet<unsigned int>::const_iterator t_it = types.begin(); t_it != t_it_end; ++t_it)
         {
            DeclareType(*t_it, behavioral_helper, locally_declared_types);
         }
      }
   }
}

void CWriter::Initialize()
{
   fake_max_tree_node_id = TM->get_next_available_tree_node_id();
   instrWriter->Initialize();
   globally_declared_types.clear();
   globallyDeclVars.clear();
   additionalIncludes.clear();
   writtenIncludes.clear();
}

void CWriter::WriteBodyLoop(const unsigned int function_index, const unsigned int, vertex current_vertex, bool bracket)
{
   writeRoutineInstructions_rec(current_vertex, bracket, function_index);
}

void CWriter::WriteFunctionBody(unsigned int function_id)
{
   const FunctionBehaviorConstRef function_behavior = AppM->CGetFunctionBehavior(function_id);
   const OpGraphConstRef op_graph = function_behavior->CGetOpGraph(FunctionBehavior::CFG);
   const BehavioralHelperConstRef behavioral_helper = function_behavior->CGetBehavioralHelper();
   var_pp_functorRef variableFunctor(new std_var_pp_functor(behavioral_helper));
   OpVertexSet vertices(op_graph);
   VertexIterator statement, statement_end;
   boost::tie(statement, statement_end);
   boost::tie(statement, statement_end) = boost::vertices(*op_graph);
   vertices.insert(statement, statement_end);
   THROW_ASSERT(vertices.size() > 0, "Graph for function " + behavioral_helper->get_function_name() + " is empty");
   writeRoutineInstructions(function_id, vertices, variableFunctor);
}

void CWriter::WriteFunctionImplementation(unsigned int function_id)
{
   StartFunctionBody(function_id);
   WriteFunctionBody(function_id);
   EndFunctionBody(function_id);
}

void CWriter::WriteHeader()
{
   bool is_readc_needed = false;
   bool is_builtin_cond_expr32 = false;
   for(auto extfun : AppM->get_functions_without_body())
   {
      const BehavioralHelperConstRef BH = this->AppM->CGetFunctionBehavior(extfun)->CGetBehavioralHelper();
      if(BH->get_function_name() == "__bambu_readc" || BH->get_function_name() == "__bambu_read4c")
         is_readc_needed = true;
      if(BH->get_function_name() == "__builtin_cond_expr32")
         is_builtin_cond_expr32 = true;
   }
   if(is_readc_needed)
   {
      indented_output_stream->Append("#include <unistd.h>\n");
      indented_output_stream->Append("short int __bambu_readc(int fd){char buf[1];int res = read(fd,buf,1);return res > 0 ? buf[0] : -1;}\n");
      indented_output_stream->Append(
          "long long int __bambu_read4c(int fd){long long int buf;int res = read(fd,&buf,4);return res == 4 ? buf : (res==3 ? (buf|(1ULL<<35)) : (res==2 ? (buf|(1ULL<<35)|(1ULL<<34)) : (res == 1 ? (buf|(1ULL<<35)|(1ULL<<34)|(1ULL<<33)) : "
          "(buf|(1ULL<<35)|(1ULL<<34)|(1ULL<<33)|(1ULL<<32))) ));}\n");
   }
   if(is_builtin_cond_expr32)
      indented_output_stream->Append("#define __builtin_cond_expr32(cond, value1, value2) cond ? value1 : value2\n\n");
}

void CWriter::WriteGlobalDeclarations()
{
   /// Writing auxiliary variables used by instruction writer
   instrWriter->write_declarations();

   /// Writing declarations of global variables
   CustomOrderedSet<unsigned int> functions = AppM->get_functions_with_body();
   THROW_ASSERT(functions.size() > 0, "at least one function is expected");
   unsigned int first_fun = *functions.begin();
   const BehavioralHelperConstRef behavioral_helper = AppM->CGetFunctionBehavior(first_fun)->CGetBehavioralHelper();

   const CustomSet<unsigned int>& gblVariables = AppM->get_global_variables();
   // Write the declarations for the global variables
   var_pp_functorRef variableFunctor(new std_var_pp_functor(behavioral_helper));
   CustomSet<unsigned int>::const_iterator gblVars, gblVarsEnd;
   for(gblVars = gblVariables.begin(), gblVarsEnd = gblVariables.end(); gblVars != gblVarsEnd; ++gblVars)
   {
      DeclareVariable(*gblVars, globallyDeclVars, globally_declared_types, behavioral_helper, variableFunctor);
   }
   indented_output_stream->Append("\n");
   if(AppM->CGetCallGraphManager()->ExistsAddressedFunction())
   {
      indented_output_stream->Append("#include <stdarg.h>\n\n");
      indented_output_stream->Append("void " + STR(BUILTIN_WAIT_CALL) + "(void * ptr, ...);\n");
   }
}

void CWriter::DeclareFunctionTypes(const unsigned int funId)
{
   const FunctionBehaviorConstRef FB = AppM->CGetFunctionBehavior(funId);

   const BehavioralHelperConstRef behavioral_helper = FB->CGetBehavioralHelper();
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Declaring function types for " + behavioral_helper->get_function_name());

   // In case the function parameters are of a non built_in type I have
   // to declare their type

   const CustomUnorderedSet<unsigned int> parameter_types = behavioral_helper->GetParameterTypes();
   CustomUnorderedSet<unsigned int>::const_iterator parameter_type, parameter_type_end = parameter_types.end();
   for(parameter_type = parameter_types.begin(); parameter_type != parameter_type_end; ++parameter_type)
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Parameter type " + STR(*parameter_type));
      DeclareType(*parameter_type, behavioral_helper, globally_declared_types);
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Parameter type " + STR(*parameter_type));
   }
   const unsigned int return_type_index = behavioral_helper->GetFunctionReturnType(funId);
   if(return_type_index)
   {
      DeclareType(return_type_index, behavioral_helper, globally_declared_types);
   }
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Declared function types for " + behavioral_helper->get_function_name());
}

const CustomSet<unsigned int> CWriter::GetLocalVariables(const unsigned int function_id) const
{
   const OpGraphConstRef inGraph = this->AppM->CGetFunctionBehavior(function_id)->CGetOpGraph(FunctionBehavior::DFG);
   const BehavioralHelperConstRef BH = AppM->CGetFunctionBehavior(function_id)->CGetBehavioralHelper();
   CustomSet<unsigned int> vars;
   // I simply have to go over all the vertices and get the used variables;
   // the variables which have to be declared are all those variables but
   // the globals ones
   VertexIterator v, vEnd;
   for(boost::tie(v, vEnd) = boost::vertices(*inGraph); v != vEnd; v++)
   {
      const CustomSet<unsigned int>& vars_temp = inGraph->CGetOpNodeInfo(*v)->cited_variables;
      vars.insert(vars_temp.begin(), vars_temp.end());
   }
   return vars;
}

void CWriter::WriteFunctionDeclaration(const unsigned int funId)
{
   const FunctionBehaviorConstRef FB = AppM->CGetFunctionBehavior(funId);
   const BehavioralHelperConstRef behavioral_helper = FB->CGetBehavioralHelper();
   const std::string& funName = behavioral_helper->get_function_name();
#if HAVE_ARM_COMPILER
   if(Param->getOption<GccWrapper_CompilerTarget>(OPT_default_compiler) == GccWrapper_CompilerTarget::CT_ARM_GCC and behavioral_helper->is_var_args())
   {
      THROW_ERROR_CODE(VARARGS_EC, "Source code containing vargs function (" + behavioral_helper->get_function_name() + ")produced using arm compiler can not be compiled by x86 compilers");
   }
#endif
#if HAVE_SPARC_COMPILER
   if(Param->getOption<GccWrapper_CompilerTarget>(OPT_default_compiler) == GccWrapper_CompilerTarget::CT_SPARC_GCC and behavioral_helper->is_var_args())
   {
      THROW_ERROR_CODE(VARARGS_EC, "Source code containing vargs function (" + behavioral_helper->get_function_name() + ") produced using sparc compiler can not be compiled by x86 compilers");
   }
#endif
   if(funName != "main")
   {
      this->instrWriter->declareFunction(funId);
      indented_output_stream->Append(";\n\n");
   }
}

void CWriter::StartFunctionBody(const unsigned int function_id)
{
   instrWriter->declareFunction(function_id);
   indented_output_stream->Append("\n{\n");

   const BehavioralHelperConstRef behavioral_helper = AppM->CGetFunctionBehavior(function_id)->CGetBehavioralHelper();
   CustomSet<unsigned int> vars = GetLocalVariables(function_id);

   const std::list<unsigned int>& funParams = behavioral_helper->get_parameters();
   for(unsigned int funParam : funParams)
   {
      if(vars.find(funParam) != vars.end())
         vars.erase(funParam);
   }

   const CustomSet<unsigned int>& gblVariables = AppM->get_global_variables();
   for(unsigned int gblVariable : gblVariables)
   {
      if(vars.find(gblVariable) != vars.end())
         vars.erase(gblVariable);
   }

   var_pp_functorRef variableFunctor(new std_var_pp_functor(behavioral_helper));
   CustomSet<unsigned int> already_declared_variables;
   CustomSet<std::string> locally_declared_types;
   declare_cast_types(function_id, locally_declared_types);
   DeclareLocalVariables(vars, already_declared_variables, locally_declared_types, behavioral_helper, variableFunctor);
}

void CWriter::EndFunctionBody(unsigned int funId)
{
   indented_output_stream->Append("}\n");
   if(this->verbose)
      indented_output_stream->Append("//end of function; id: " + STR(funId) + "\n");
   indented_output_stream->Append("\n");
   basic_block_prefix.clear();
   basic_block_tail.clear();
   renaming_table.clear();
}

void CWriter::writePreInstructionInfo(const FunctionBehaviorConstRef, const vertex)
{
}

void CWriter::writePostInstructionInfo(const FunctionBehaviorConstRef, const vertex)
{
}

void CWriter::writeRoutineInstructions_rec(vertex current_vertex, bool bracket, const unsigned int function_index)
{
   const BBGraphInfoConstRef& bb_graph_info = local_rec_bb_fcfgGraph->CGetBBGraphInfo();
   const BBNodeInfoConstRef& bb_node_info = local_rec_bb_fcfgGraph->CGetBBNodeInfo(current_vertex);
   const unsigned int bb_number = bb_node_info->block->number;

   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Starting writing BB" + STR(bb_number));
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->");
   THROW_ASSERT(bb_frontier.find(current_vertex) == bb_frontier.end(), "current_vertex cannot be part of the basic block frontier");
   // if this basic block has already been analyzed do nothing
   if(bb_analyzed.find(current_vertex) != bb_analyzed.end())
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--");
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--BB" + boost::lexical_cast<std::string>(bb_number) + " already written");
      return;
   }
   // mark this basic block as analyzed
   bb_analyzed.insert(current_vertex);
   // print a comment with info on the basicblock
   if(this->verbose)
   {
      indented_output_stream->Append("//Basic block " + STR(bb_number) + " - loop " + STR(bb_node_info->loop_id) + "\n");
   }
   // check if some extra strings must be printed before or after the basic
   // block. this is used for splitting the phi nodes
   bool add_phi_nodes_assignment_prefix = basic_block_prefix.find(bb_number) != basic_block_prefix.end();
   bool add_phi_nodes_assignment = basic_block_tail.find(bb_number) != basic_block_tail.end();
   // get immediate post-dominator and check if it has to be examined
   vertex bb_PD = post_dominators->get_immediate_dominator(current_vertex);
#ifndef NDEBUG
   {
      const BBNodeInfoConstRef& bb_node_info_pd = local_rec_bb_fcfgGraph->CGetBBNodeInfo(bb_PD);
      const unsigned int& bb_number_PD = bb_node_info_pd->block->number;

      std::string frontier_string;
      for(const auto bb : bb_frontier)
         frontier_string += "BB" + STR(local_rec_bb_fcfgGraph->CGetBBNodeInfo(bb)->block->number) + " ";
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Frontier at the moment is: " + frontier_string);
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Its post-dominator is BB" + STR(bb_number_PD));
   }
#endif
   bool analyze_bb_PD = bb_frontier.find(bb_PD) == bb_frontier.end() && bb_analyzed.find(bb_PD) == bb_analyzed.end();
   if(analyze_bb_PD)
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Post dominator will be examinated");
      bb_frontier.insert(bb_PD);
   }
   /// compute the last statement
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Looking for last statement");
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->");
   vertex last_stmt = NULL_VERTEX;
   bool is_there = false;
   const auto& stmts_list = bb_node_info->statements_list;
   for(const auto st : boost::adaptors::reverse(stmts_list))
   {
      if(local_rec_instructions.find(st) == local_rec_instructions.end())
         continue;
      if(GET_TYPE(local_rec_cfgGraph, st) & TYPE_VPHI)
         continue;
      if((GET_TYPE(local_rec_cfgGraph, st) & TYPE_INIT) != 0)
         continue;
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Considering operation " + GET_NAME(local_rec_cfgGraph, st));
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "This is basic block is not empty in this task. Last operation to be printed id " + GET_NAME(local_rec_cfgGraph, st));
      last_stmt = st;
      is_there = true;
      break;
   }
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--");
   /// check the feasibility
   bool last_statement_is_a_cond_or_goto = is_there and local_rec_behavioral_helper->end_with_a_cond_or_goto(bb_node_info->block) != 0 && last_stmt == stmts_list.back();

   THROW_ASSERT(!last_statement_is_a_cond_or_goto || !is_there || (last_statement_is_a_cond_or_goto && last_stmt == stmts_list.back()), "inconsistent recursion");
   // check if the basic block starts with a label
   bool start_with_a_label = local_rec_behavioral_helper->start_with_a_label(bb_node_info->block);
   if(start_with_a_label)
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Basic block starts with a label");
   // check if the label is already in the goto list
   bool add_bb_label = goto_list.find(current_vertex) != goto_list.end();
   if(add_bb_label)
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Basic block should start with a label");

   if(!add_bb_label and !start_with_a_label and boost::in_degree(current_vertex, *local_rec_bb_fcfgGraph) > 1)
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Basic block has an indegree > 1 and not associated label");
      InEdgeIterator inE, inEEnd;
      for(boost::tie(inE, inEEnd) = boost::in_edges(current_vertex, *local_rec_bb_fcfgGraph); inE != inEEnd; inE++)
      {
         vertex source = boost::source(*inE, *local_rec_bb_fcfgGraph);
         const BBNodeInfoConstRef pred_bb_node_info = local_rec_bb_fcfgGraph->CGetBBNodeInfo(source);
         // This condition match first basic block of case preceded by a case without break
         if(pred_bb_node_info->statements_list.size() and (GET_TYPE(local_rec_cfgGraph, *(pred_bb_node_info->statements_list.rbegin())) & TYPE_SWITCH) and post_dominators->get_immediate_dominator(source) != current_vertex)
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Basic block is the first case of a case preceded by a case without break");
            add_bb_label = true;
            break;
         }
         // Basic block start the body of a short circuit
         else if(bb_analyzed.find(source) == bb_analyzed.end() and !((FB_CFG_SELECTOR & local_rec_bb_fcfgGraph->GetSelector(*inE))))
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Basic block should start with a label since is the body of a short-circuit");
            add_bb_label = true;
            break;
         }
         // Basic block is a header loop, but it does not end with while or for
         else if((bb_analyzed.find(source) == bb_analyzed.end() or current_vertex == source) and (stmts_list.empty() or ((GET_TYPE(local_rec_cfgGraph, *(stmts_list.rbegin())) & (TYPE_WHILE | TYPE_FOR)) == 0)))
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Basic block is the header of a loop and it does not end with while or for");
            add_bb_label = true;
            break;
         }
      }
   }
   add_bb_label = add_bb_label && !start_with_a_label;
   bool add_semicolon = false;
   /// print each instruction
   if(bracket)
   {
      if(analyze_bb_PD || is_there || add_bb_label || add_phi_nodes_assignment || add_phi_nodes_assignment_prefix)
      {
         indented_output_stream->Append("{\n");
      }
      else
         add_semicolon = true;
   }
   if(add_bb_label)
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "A label should be added at the beginning");
      THROW_ASSERT(basic_blocks_labels.find(bb_number) != basic_blocks_labels.end(), "I do not know the destination: " + STR(bb_number));
      indented_output_stream->Append(basic_blocks_labels.find(bb_number)->second + ":;\n");
      add_semicolon = true;
   }
   else if(start_with_a_label)
   {
      add_semicolon = true;
   }
   WriteBBHeader(bb_number, function_index);

   std::list<vertex>::const_iterator vIter, vIterBegin;
   vIter = stmts_list.begin();
   vIterBegin = vIter;
   if(is_there)
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "There are instructions to be printed for this pair task - basic block");

      // fill the renaming table in case it is needed
      if(renaming_table.find(current_vertex) != renaming_table.end())
         for(const auto& rvt : renaming_table.find(current_vertex)->second)
            BehavioralHelper::rename_a_variable(rvt.first, rvt.second);
      bool label_has_to_be_printed = start_with_a_label;
      bool prefix_has_to_be_printed = basic_block_prefix.find(bb_number) != basic_block_prefix.end();
      do
      {
         // We can print results of split of phi nodes if they have not yet been printed and if label has already been printed (or there was not any label to be printed)
         if(prefix_has_to_be_printed and not label_has_to_be_printed)
         {
            prefix_has_to_be_printed = false;
            indented_output_stream->Append(basic_block_prefix.find(bb_number)->second);
         }
         if(local_rec_instructions.find(*vIter) == local_rec_instructions.end())
            continue;
         // If there is not any label to be printed, label_has_to_be_printed is already false, otherwise the label will be printed during this loop iteration
         label_has_to_be_printed = false;
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Preparing printing of operation " + GET_NAME(local_rec_cfgGraph, *vIter));
         // Write in the C file extra information before the instruction itself
         if(this->verbose)
            indented_output_stream->Append("//Instruction: " + GET_NAME(local_rec_cfgGraph, *vIter) + "\n");
         writePreInstructionInfo(local_rec_function_behavior, *vIter);
         // Now I print the instruction
         if(start_with_a_label && vIter == vIterBegin)
         {
            InEdgeIterator inE, inEEnd;
            for(boost::tie(inE, inEEnd) = boost::in_edges(current_vertex, *local_rec_bb_fcfgGraph); inE != inEEnd; inE++)
            {
               if(FB_CFG_SELECTOR & local_rec_bb_fcfgGraph->GetSelector(*inE))
                  indented_output_stream->Append("//start of a loop\n");
            }
         }
         bool isLastIntruction = last_stmt == *vIter;
         /// in case we have phi nodes we check if some assignments should be printed
         bool print_phi_now = ((GET_TYPE(local_rec_cfgGraph, *vIter) & (TYPE_IF | TYPE_WHILE | TYPE_FOR | TYPE_SWITCH | TYPE_MULTIIF))) || local_rec_behavioral_helper->end_with_a_cond_or_goto(bb_node_info->block);
         if(add_phi_nodes_assignment && isLastIntruction && print_phi_now)
            indented_output_stream->Append(basic_block_tail.find(bb_number)->second);
         if((GET_TYPE(local_rec_cfgGraph, *vIter) & (TYPE_VPHI)) == 0)
         {
            if(GET_TYPE(local_rec_cfgGraph, *vIter) & (TYPE_WHILE | TYPE_FOR) and this->verbose and local_rec_function_behavior->CGetLoops()->CGetLoop(bb_node_info->loop_id)->loop_type & DOALL_LOOP)
               indented_output_stream->Append("//#pragma omp parallel for\n");
            instrWriter->write(local_rec_function_behavior, *vIter, local_rec_variableFunctor);
            if((GET_TYPE(local_rec_cfgGraph, *vIter) & TYPE_LABEL) == 0)
               add_semicolon = false;
         }
         else if(this->verbose)
         {
            indented_output_stream->Append("//(removed virtual phi instruction)\n");
         }
         // Write in the C file extra information after the instruction statement
         writePostInstructionInfo(local_rec_function_behavior, *vIter);
         if(!isLastIntruction)
            continue;
         BehavioralHelper::clear_renaming_table();
         if(add_phi_nodes_assignment && !print_phi_now)
            indented_output_stream->Append(basic_block_tail.find(bb_number)->second);
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---This is not the last statement");
         // Now I check if this is a control statement and I consequently print
         // the instructions contained in its branches
         if(GET_TYPE(local_rec_cfgGraph, *vIter) & TYPE_IF)
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Operation is an if");
            const unsigned int& bb_true_number = bb_node_info->block->true_edge;
            THROW_ASSERT(bb_graph_info->bb_index_map.find(bb_true_number) != bb_graph_info->bb_index_map.end(), "BB" + STR(bb_true_number) + " does not exist");
            const vertex true_vertex = bb_graph_info->bb_index_map.find(bb_true_number)->second;
            THROW_ASSERT(bb_graph_info->bb_index_map.find(bb_node_info->block->false_edge) != bb_graph_info->bb_index_map.end(), "BB" + STR(bb_node_info->block->false_edge) + " does not exist");
            vertex false_vertex = bb_graph_info->bb_index_map.find(bb_node_info->block->false_edge)->second;
            bool add_false_to_goto = false;
            if(bb_frontier.find(true_vertex) == bb_frontier.end())
            {
               if(bb_frontier.find(false_vertex) == bb_frontier.end() && goto_list.find(false_vertex) == goto_list.end())
               {
                  goto_list.insert(false_vertex);
                  add_false_to_goto = true;
               }
               if(bb_analyzed.find(true_vertex) == bb_analyzed.end())
                  writeRoutineInstructions_rec(true_vertex, true, function_index);
               else
               {
                  THROW_ASSERT(basic_blocks_labels.find(bb_true_number) != basic_blocks_labels.end(), "I do not know the destination");
                  indented_output_stream->Append("   goto " + basic_blocks_labels.find(bb_true_number)->second + ";\n");
                  goto_list.insert(true_vertex);
               }
            }
            else
               indented_output_stream->Append("{}\n");
            if(add_false_to_goto)
               goto_list.erase(false_vertex);
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Printed then");
            if(bb_frontier.find(false_vertex) == bb_frontier.end())
            {
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "False BB is not on the frontier");
               false_vertex = bb_graph_info->bb_index_map.find(bb_node_info->block->false_edge)->second;
               if(not(FB_CFG_SELECTOR & local_rec_bb_fcfgGraph->GetSelector(boost::edge(current_vertex, false_vertex, *local_rec_bb_fcfgGraph).first)))
               {
                  indented_output_stream->Append("else\n");
                  if(bb_analyzed.find(false_vertex) == bb_analyzed.end())
                     writeRoutineInstructions_rec(false_vertex, true, function_index);
                  else
                  {
                     THROW_ASSERT(basic_blocks_labels.find(bb_node_info->block->false_edge) != basic_blocks_labels.end(), "I do not know the destination");
                     indented_output_stream->Append("   goto " + basic_blocks_labels.find(bb_node_info->block->false_edge)->second + ";\n");
                  }
               }
               /// Feedback edge on the false path of an if
               else
               {
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Target is a header");
                  const vertex target = boost::target(boost::edge(current_vertex, false_vertex, *local_rec_bb_fcfgGraph).first, *local_rec_bb_fcfgGraph);
                  const BBNodeInfoConstRef target_bb_node_info = local_rec_bb_fcfgGraph->CGetBBNodeInfo(target);
                  /// Target is not a while or a for
                  if(target_bb_node_info->statements_list.empty() or ((GET_TYPE(local_rec_cfgGraph, *(target_bb_node_info->statements_list.rbegin())) & (TYPE_WHILE | TYPE_FOR)) == 0))
                  {
                     indented_output_stream->Append("else\n");
                     THROW_ASSERT(basic_blocks_labels.find(bb_node_info->block->false_edge) != basic_blocks_labels.end(), "I do not know the destination");
                     indented_output_stream->Append("   goto " + basic_blocks_labels.find(bb_node_info->block->false_edge)->second + ";\n");
                  }
               }
            }
         }
         else if(GET_TYPE(local_rec_cfgGraph, *vIter) & (TYPE_WHILE | TYPE_FOR))
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Operation is a while or a for");
            const unsigned int& bb_true_number = bb_node_info->block->true_edge;
            const vertex true_vertex = bb_graph_info->bb_index_map.find(bb_true_number)->second;
            if(bb_frontier.find(true_vertex) == bb_frontier.end())
            {
               if(bb_analyzed.find(true_vertex) == bb_analyzed.end())
               {
                  WriteBodyLoop(local_rec_function_behavior->CGetBehavioralHelper()->get_function_index(), bb_node_info->block->number, true_vertex, true);
               }
               else
               {
                  THROW_ERROR("Body of a loop has yet been printed before the while statement");
               }
            }
            else
            {
               return;
            }

            const unsigned int bb_false_number = bb_node_info->block->false_edge;
            const vertex false_vertex = bb_graph_info->bb_index_map.find(bb_false_number)->second;
            if(bb_frontier.find(false_vertex) == bb_frontier.end())
            {
               if(bb_analyzed.find(false_vertex) == bb_analyzed.end())
               {
                  writeRoutineInstructions_rec(false_vertex, false, function_index);
               }
               else
               {
                  indented_output_stream->Append("goto " + basic_blocks_labels.find(bb_false_number)->second + ";\n");
                  goto_list.insert(false_vertex);
               }
            }
         }
         else if(GET_TYPE(local_rec_cfgGraph, *vIter) & TYPE_MULTIIF)
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Operation is a multiif");
            unsigned int node_id = local_rec_cfgGraph->CGetOpNodeInfo(last_stmt)->GetNodeId();
            const tree_nodeRef node = TM->get_tree_node_const(node_id);
            THROW_ASSERT(node->get_kind() == gimple_multi_way_if_K, "unexpected node");
            auto* gmwi = GetPointer<gimple_multi_way_if>(node);
            std::map<unsigned int, bool> add_elseif_to_goto;
            for(const auto& cond : gmwi->list_of_cond)
            {
               unsigned int bb_index_num = cond.second;
               const vertex bb_vertex = bb_graph_info->bb_index_map.find(bb_index_num)->second;
               if(cond != gmwi->list_of_cond.front())
               {
                  bool to_be_added = bb_frontier.find(bb_vertex) == bb_frontier.end() && goto_list.find(bb_vertex) == goto_list.end();
                  add_elseif_to_goto[bb_index_num] = to_be_added;
                  if(to_be_added)
                     goto_list.insert(bb_vertex);
               }
               else
                  add_elseif_to_goto[bb_index_num] = false;
            }
            for(const auto& cond : gmwi->list_of_cond)
            {
               unsigned int bb_index_num = cond.second;
               const vertex bb_vertex = bb_graph_info->bb_index_map.find(bb_index_num)->second;
               if(cond != gmwi->list_of_cond.front())
               {
                  if(cond.first)
                  {
                     indented_output_stream->Append("else if(");
                     indented_output_stream->Append(local_rec_behavioral_helper->PrintVariable(GET_INDEX_NODE(cond.first)));
                     indented_output_stream->Append(")\n");
                  }
                  else
                     indented_output_stream->Append("else\n");
               }
               if(add_elseif_to_goto.find(bb_index_num) != add_elseif_to_goto.end() && add_elseif_to_goto.find(bb_index_num)->second)
                  goto_list.erase(bb_vertex);
               if(bb_frontier.find(bb_vertex) == bb_frontier.end())
               {
                  if(bb_analyzed.find(bb_vertex) == bb_analyzed.end())
                  {
                     writeRoutineInstructions_rec(bb_vertex, true, function_index);
                  }
                  else
                  {
                     INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Successor has already been examined");

                     THROW_ASSERT(basic_blocks_labels.find(bb_index_num) != basic_blocks_labels.end(), "I do not know the destination " + STR(bb_index_num));
                     indented_output_stream->Append("   goto " + basic_blocks_labels.find(bb_index_num)->second + ";\n");
                     goto_list.insert(bb_vertex);
                  }
               }
               else
                  indented_output_stream->Append("{}\n");
            }
         }
         else if(GET_TYPE(local_rec_cfgGraph, *vIter) & TYPE_SWITCH)
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Operation is a switch");
            /// now we can analyze the following basic blocks
            indented_output_stream->Append("{\n");
            OutEdgeIterator oE, oEEnd;
            for(boost::tie(oE, oEEnd) = boost::out_edges(current_vertex, *local_rec_bb_fcfgGraph); oE != oEEnd; oE++)
            {
               bool empty_block = false;
               vertex next_bb = boost::target(*oE, *local_rec_bb_fcfgGraph);
               const BBNodeInfoConstRef next_bb_node_info = local_rec_bb_fcfgGraph->CGetBBNodeInfo(next_bb);
#ifndef NDEBUG
               const unsigned int bb_number_next_bb = next_bb_node_info->block->number;
#endif
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Examining successor " + STR(bb_number_next_bb));
               CustomOrderedSet<unsigned int>::const_iterator eIdBeg, eIdEnd;
               CustomOrderedSet<unsigned int> Set = local_rec_bb_fcfgGraph->CGetBBEdgeInfo(*oE)->get_labels(CFG_SELECTOR);
               for(eIdBeg = Set.begin(), eIdEnd = Set.end(); eIdBeg != eIdEnd; ++eIdBeg)
               {
                  if(*eIdBeg == default_COND)
                  {
                     INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Successor is default case");
                     if(next_bb == post_dominators->get_immediate_dominator(current_vertex))
                     {
                        INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Default is empty");
                        empty_block = true;
                        indented_output_stream->Append("default:\n");
                        if(current_vertex == dominators->get_immediate_dominator(next_bb))
                           analyze_bb_PD = true;
                        break;
                     }
                     indented_output_stream->Append("default");
                  }
                  else
                  {
                     if(next_bb == bb_PD) /// then adjust post dominator
                     {
                        empty_block = true;
                        INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Removed current basic block");
                     }

                     indented_output_stream->Append("case " + local_rec_behavioral_helper->print_constant(*eIdBeg));
                  }
                  indented_output_stream->Append(":\n");
               }
               if(empty_block)
               {
                  indented_output_stream->Append("break;\n");
                  continue;
               }
               if(bb_analyzed.find(next_bb) == bb_analyzed.end())
               {
                  writeRoutineInstructions_rec(next_bb, true, function_index);
                  indented_output_stream->Append("break;\n");
               }
               else
               {
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Successor has already been examined");
                  const unsigned int next_bb_number = next_bb_node_info->block->number;

                  THROW_ASSERT(basic_blocks_labels.find(next_bb_number) != basic_blocks_labels.end(), "I do not know the destination " + STR(next_bb_number));
                  indented_output_stream->Append("   goto " + basic_blocks_labels.find(next_bb_number)->second + ";\n");
               }
            }
            indented_output_stream->Append("}\n");
         }
         else if(local_rec_behavioral_helper->end_with_a_cond_or_goto(bb_node_info->block))
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Basic block ends with a cond or a goto");
            if(last_statement_is_a_cond_or_goto)
            {
               /// now we can analyze the following basic blocks
               OutEdgeIterator oE, oEEnd;
               for(boost::tie(oE, oEEnd) = boost::out_edges(current_vertex, *local_rec_bb_fcfgGraph); oE != oEEnd; oE++)
               {
                  vertex next_bb = boost::target(*oE, *local_rec_bb_fcfgGraph);
                  if(bb_frontier.find(next_bb) != bb_frontier.end())
                     continue;
                  goto_list.insert(next_bb);
               }
            }
         }
         else
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---It is not a special operation");
            const vertex bbentry = local_rec_bb_fcfgGraph->CGetBBGraphInfo()->entry_vertex;
            if(current_vertex == bbentry)
            {
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--");
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Ended writing basic block " + STR(bb_number));
               return;
            }
            THROW_ASSERT(boost::out_degree(current_vertex, *local_rec_bb_fcfgGraph) <= 1, "only one edge expected");
            OutEdgeIterator oE, oEEnd;
            for(boost::tie(oE, oEEnd) = boost::out_edges(current_vertex, *local_rec_bb_fcfgGraph); oE != oEEnd; oE++)
            {
               vertex next_bb = boost::target(*oE, *local_rec_bb_fcfgGraph);
               if(bb_frontier.find(next_bb) != bb_frontier.end())
               {
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Not adding goto since target is in the frontier");
                  continue;
               }
               if(FB_CFG_SELECTOR & local_rec_bb_fcfgGraph->GetSelector(*oE))
               {
                  const vertex target = boost::target(*oE, *local_rec_bb_fcfgGraph);
                  const BBNodeInfoConstRef target_bb_node_info = local_rec_bb_fcfgGraph->CGetBBNodeInfo(target);
                  if(target_bb_node_info->statements_list.size() and (GET_TYPE(local_rec_cfgGraph, *(target_bb_node_info->statements_list.rbegin())) & (TYPE_WHILE | TYPE_FOR)))
                  {
                     INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Not adding a goto since target is a while/for");
                     continue;
                  }
               }
               if(boost::in_degree(next_bb, *local_rec_bb_fcfgGraph) == 1)
               {
                  writeRoutineInstructions_rec(next_bb, false, function_index);
               }
               else
               {
                  const BBNodeInfoConstRef next_bb_node_info = local_rec_bb_fcfgGraph->CGetBBNodeInfo(next_bb);
                  const unsigned int next_bb_number = next_bb_node_info->block->number;
                  THROW_ASSERT(basic_blocks_labels.find(next_bb_number) != basic_blocks_labels.end(), "I do not know the destination");
                  indented_output_stream->Append("   goto " + basic_blocks_labels.find(next_bb_number)->second + ";\n");
                  goto_list.insert(next_bb);
               }
            }
         }
      } while(*vIter++ != last_stmt);
   }
   else
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "is_there is false");
      /// in case we have phi nodes we check if some assignments should be printed
      if(add_phi_nodes_assignment)
      {
         indented_output_stream->Append(basic_block_tail.find(bb_number)->second);
         add_semicolon = false;
      }
      if(!local_rec_behavioral_helper->end_with_a_cond_or_goto(bb_node_info->block) && ((stmts_list.empty()) || ((GET_TYPE(local_rec_cfgGraph, *stmts_list.rbegin()) & (TYPE_SWITCH | TYPE_WHILE | TYPE_FOR)) == 0)))
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Not end with a cond or goto nor switch");
         const vertex bbentry = local_rec_bb_fcfgGraph->CGetBBGraphInfo()->entry_vertex;
         if(current_vertex == bbentry)
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--");
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Ended writing basic block " + STR(bb_number));
            return;
         }
         THROW_ASSERT(boost::out_degree(current_vertex, *local_rec_bb_fcfgGraph) <= 1, "only one edge expected BB(" + STR(bb_number) + ") Fun(" + STR(local_rec_behavioral_helper->get_function_index()) + ")");
         OutEdgeIterator oE, oEEnd;
         for(boost::tie(oE, oEEnd) = boost::out_edges(current_vertex, *local_rec_bb_fcfgGraph); oE != oEEnd; oE++)
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Examining the only? successor");
            vertex next_bb = boost::target(*oE, *local_rec_bb_fcfgGraph);
            if(bb_frontier.find(next_bb) != bb_frontier.end() or boost::in_degree(next_bb, *local_rec_bb_fcfgGraph) == 1)
               continue;
            /// Last basic block of a while/for loop
            if(FB_CFG_SELECTOR & local_rec_bb_fcfgGraph->GetSelector(*oE))
            {
               const vertex target = boost::target(*oE, *local_rec_bb_fcfgGraph);
               const BBNodeInfoConstRef target_bb_node_info = local_rec_bb_fcfgGraph->CGetBBNodeInfo(target);
               if(target_bb_node_info->statements_list.size() and (GET_TYPE(local_rec_cfgGraph, *(target_bb_node_info->statements_list.rbegin())) & (TYPE_WHILE | TYPE_FOR)))
                  continue;
            }

            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Successor does not belong to frontier");
            const BBNodeInfoConstRef next_bb_node_info = local_rec_bb_fcfgGraph->CGetBBNodeInfo(next_bb);
            const unsigned int next_bb_number = next_bb_node_info->block->number;
            THROW_ASSERT(basic_blocks_labels.find(next_bb_number) != basic_blocks_labels.end(), "I do not know the destination");
            indented_output_stream->Append("   goto " + basic_blocks_labels.find(next_bb_number)->second + ";\n");
            goto_list.insert(next_bb);
            add_semicolon = false;
         }
      }
   }
   if(add_semicolon)
      indented_output_stream->Append(";\n"); /// added a fake indent

   if(analyze_bb_PD)
   {
      // recurse on the post dominator
      bb_frontier.erase(bb_PD);
      THROW_ASSERT(bb_analyzed.find(bb_PD) == bb_analyzed.end(), "something of wrong happen " + STR(local_rec_bb_fcfgGraph->CGetBBNodeInfo(bb_PD)->block->number) + " Fun(" + STR(local_rec_behavioral_helper->get_function_index()) + ")");
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Printing the post dominator");
      writeRoutineInstructions_rec(bb_PD, false, function_index);
   }
   if((analyze_bb_PD || is_there || add_bb_label || add_phi_nodes_assignment || add_phi_nodes_assignment_prefix) && bracket)
      indented_output_stream->Append("}\n");
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--");
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Ended writing basic block " + STR(bb_number));
}

void CWriter::compute_phi_nodes(const FunctionBehaviorConstRef function_behavior, const OpVertexSet& instructions, var_pp_functorConstRef variableFunctor)
{
   /// compute the assignment introduced by the phi nodes destruction
   const BBGraphConstRef bb_domGraph = function_behavior->CGetBBGraph(FunctionBehavior::DOM_TREE);
   const BBGraphConstRef bb_fcfgGraph = function_behavior->CGetBBGraph(FunctionBehavior::FBB);
   const OpGraphConstRef cfgGraph = function_behavior->CGetOpGraph(FunctionBehavior::FCFG);
   CustomSet<unsigned int> phi_instructions;
   for(const auto instruction : instructions)
   {
      if(GET_TYPE(cfgGraph, instruction) & TYPE_PHI)
      {
         phi_instructions.insert(cfgGraph->CGetOpNodeInfo(instruction)->GetNodeId());
      }
   }
   if(!phi_instructions.empty())
   {
      std::map<unsigned int, unsigned int> created_variables;
      std::map<unsigned int, std::string> symbol_table;
      std::map<unsigned int, std::deque<std::string>> array_of_stacks;
      insert_copies(bb_domGraph->CGetBBGraphInfo()->entry_vertex, bb_domGraph, bb_fcfgGraph, variableFunctor, phi_instructions, created_variables, symbol_table, array_of_stacks);
      /// in case we declare the variables introduced during the phi nodes destruction
      if(!created_variables.empty())
      {
         std::map<unsigned int, unsigned int>::const_iterator cv_it_end = created_variables.end();
         for(std::map<unsigned int, unsigned int>::const_iterator cv_it = created_variables.begin(); cv_it != cv_it_end; ++cv_it)
         {
            THROW_ASSERT(symbol_table.find(cv_it->first) != symbol_table.end(), "variable not found in symbol_table");
            unsigned real_var = cv_it->second;
            var_pp_functorRef phi_functor = var_pp_functorRef(new isolated_var_pp_functor(function_behavior->CGetBehavioralHelper(), real_var, symbol_table[cv_it->first]));
            indented_output_stream->Append(tree_helper::print_type(TM, tree_helper::get_type_index(TM, real_var), false, false, false, real_var, phi_functor));
            indented_output_stream->Append(";\n");
         }
      }
   }
}

void CWriter::writeRoutineInstructions(const unsigned int function_index, const OpVertexSet& instructions, var_pp_functorConstRef variableFunctor, vertex bb_start, CustomOrderedSet<vertex> bb_end)
{
   bb_label_counter++;
   const FunctionBehaviorConstRef function_behavior = AppM->CGetFunctionBehavior(function_index);
   const BehavioralHelperConstRef behavioral_helper = function_behavior->CGetBehavioralHelper();
   const OpGraphConstRef cfgGraph = function_behavior->CGetOpGraph(FunctionBehavior::FCFG);
   INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "-->CWriter::writeRoutineInstructions - Start");
   if(instructions.empty())
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "<--CWriter::writeRoutineInstructions - instructions is an empty set");
      return;
   }
   else if(instructions.size() == 1)
   {
      if(GET_TYPE(cfgGraph, (*instructions.begin())) & TYPE_ENTRY)
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "<--CWriter::writeRoutineInstructions - instructions is a set with only entry");
         return;
      }
   }
   const BBGraphConstRef bb_fcfgGraph = function_behavior->CGetBBGraph(FunctionBehavior::FBB);
   /// Then I compute all the labels associated with a basic block with more than one entering edge.
   basic_blocks_labels.clear();
   VertexIterator vi, vi_end;
   vertex bbentry;
   CustomOrderedSet<vertex> bb_exit;

   if(!bb_start)
      bbentry = bb_fcfgGraph->CGetBBGraphInfo()->entry_vertex;
   else
      bbentry = bb_start;

   if(bb_end.empty())
   {
      bb_exit.insert(bb_fcfgGraph->CGetBBGraphInfo()->exit_vertex);
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "There are " + STR(bb_exit.size()) + " exit basic blocks");
   }
   else
   {
      bb_exit = bb_end;
   }

   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Computing labels");
   for(boost::tie(vi, vi_end) = boost::vertices(*bb_fcfgGraph); vi != vi_end; vi++)
   {
      size_t delta = bb_exit.find(*vi) != bb_exit.end() ? 1u : 0u;
      if(boost::in_degree(*vi, *bb_fcfgGraph) <= (1 + delta))
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Skipped BB" + STR(bb_fcfgGraph->CGetBBNodeInfo(*vi)->block->number));
         continue;
      }
      const BBNodeInfoConstRef bb_node_info = bb_fcfgGraph->CGetBBNodeInfo(*vi);
      const unsigned int le = behavioral_helper->start_with_a_label(bb_node_info->block);
      basic_blocks_labels[bb_node_info->block->number] = (le ? behavioral_helper->get_label_name(le) : ("BB_LABEL_" + STR(bb_node_info->block->number)) + (bb_label_counter == 1 ? "" : "_" + STR(bb_label_counter)));
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Label of BB" + STR(bb_fcfgGraph->CGetBBNodeInfo(*vi)->block->number) + " is " + basic_blocks_labels[bb_node_info->block->number]);
   }
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Computed labels");
   /// set of basic block already analyzed
   bb_analyzed.clear();
   bb_analyzed.insert(bb_exit.begin(), bb_exit.end());
   /// store for which basic block the goto has been used
   goto_list.clear();
   /// basic block frontier over which writeRoutineInstructions_rec cannot go.
   bb_frontier.clear();
   bb_frontier.insert(bb_exit.begin(), bb_exit.end());
   local_rec_variableFunctor = variableFunctor;
   local_rec_function_behavior = function_behavior;
   local_rec_instructions.clear();
   local_rec_instructions.insert(instructions.begin(), instructions.end());
   dominators = local_rec_function_behavior->dominators;
   post_dominators = local_rec_function_behavior->post_dominators;
   local_rec_bb_fcfgGraph = local_rec_function_behavior->CGetBBGraph(FunctionBehavior::FBB);
   local_rec_cfgGraph = local_rec_function_behavior->CGetOpGraph(FunctionBehavior::FCFG);
   local_rec_behavioral_helper = local_rec_function_behavior->CGetBehavioralHelper();

   /// some statements can be in entry
   writeRoutineInstructions_rec(bbentry, false, function_index);
   if(!bb_start && bb_end.size() == 0)
   {
      OutEdgeIterator oE, oEEnd;
      for(boost::tie(oE, oEEnd) = boost::out_edges(bbentry, *bb_fcfgGraph); oE != oEEnd; oE++)
      {
         if(bb_exit.find(boost::target(*oE, *bb_fcfgGraph)) != bb_exit.end())
            continue;
         else
         {
            writeRoutineInstructions_rec(boost::target(*oE, *bb_fcfgGraph), false, function_index);
         }
      }
   }
   CustomOrderedSet<vertex> not_yet_considered;
   std::set_difference(goto_list.begin(), goto_list.end(),     /*first set*/
                       bb_analyzed.begin(), bb_analyzed.end(), /*second set*/
                       std::inserter(not_yet_considered, not_yet_considered.begin()) /*result*/);
   while(!not_yet_considered.empty())
   {
      vertex next_bb = *not_yet_considered.begin();
      not_yet_considered.erase(next_bb);
      writeRoutineInstructions_rec(next_bb, false, function_index);
      not_yet_considered.clear();
      std::set_difference(goto_list.begin(), goto_list.end(),     /*first set*/
                          bb_analyzed.begin(), bb_analyzed.end(), /*second set*/
                          std::inserter(not_yet_considered, not_yet_considered.begin()) /*result*/);
   }
   const vertex exit = bb_fcfgGraph->CGetBBGraphInfo()->exit_vertex;
   if(goto_list.find(exit) != goto_list.end() && basic_blocks_labels.find(bloc::EXIT_BLOCK_ID) != basic_blocks_labels.end())
   {
      indented_output_stream->Append(basic_blocks_labels.find(bloc::EXIT_BLOCK_ID)->second + ":\n");
   }
   INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "<--CWriter::writeRoutineInstructions - End");
}

void CWriter::DeclareType(unsigned int varType, const BehavioralHelperConstRef behavioral_helper, CustomSet<std::string>& locally_declared_types)
{
#ifndef NDEBUG
   const std::string& routine_name = behavioral_helper->get_function_name();
#endif

   const auto without_transformation = Param->getOption<bool>(OPT_without_transformation);
   const unsigned int real_var_type = tree_helper::GetRealType(TM, varType);
   const std::string type_name = tree_helper::name_type(TM, real_var_type);

   // Check that the variable really needs the declaration of a new type
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Declaration of type " + type_name + " " + STR(varType) + "(" + STR(real_var_type) + ") in function " + routine_name);

   if(globally_declared_types.find(type_name) == globally_declared_types.end() and locally_declared_types.find(type_name) == locally_declared_types.end())
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---This type has not been declared in this function");
      locally_declared_types.insert(type_name);
      bool is_system;
      const std::string decl = std::get<0>(behavioral_helper->get_definition(varType, is_system));
      if(not decl.empty() and decl != "<built-in>" and is_system
#if HAVE_BAMBU_BUILT
         and not tree_helper::IsInLibbambu(TM, varType)
#endif
      )
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Type has not to be declared since it is declared in included " + decl);
         return;
      }
      const CustomUnorderedSet<unsigned int> types_to_be_declared_before = tree_helper::GetTypesToBeDeclaredBefore(TM, real_var_type, without_transformation);
      for(const auto type_to_be_declared : types_to_be_declared_before)
      {
         DeclareType(type_to_be_declared, behavioral_helper, locally_declared_types);
      }
      if(tree_helper::HasToBeDeclared(TM, real_var_type))
      {
         if(this->verbose)
         {
            indented_output_stream->Append("//declaration of type " + STR(varType) + "(" + STR(real_var_type) + ")\n");
         }
         indented_output_stream->Append(behavioral_helper->print_type_declaration(real_var_type) + ";\n");
      }
      const CustomUnorderedSet<unsigned int> types_to_be_declared_after = tree_helper::GetTypesToBeDeclaredAfter(TM, real_var_type, without_transformation);
      for(const auto type_to_be_declared : types_to_be_declared_after)
      {
         DeclareType(type_to_be_declared, behavioral_helper, locally_declared_types);
      }
   }
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Declared type " + STR(varType));
}

void CWriter::DeclareVariable(unsigned int curVar, CustomSet<unsigned int>& already_declared_variables, CustomSet<std::string>& locally_declared_types, const BehavioralHelperConstRef behavioral_helper, const var_pp_functorConstRef varFunc)
{
   if(already_declared_variables.find(curVar) != already_declared_variables.end())
      return;
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Declaring variable (" + STR(curVar) + ") " + behavioral_helper->PrintVariable(curVar));
   already_declared_variables.insert(curVar);

   CustomUnorderedSet<unsigned int> initVars;
   CustomUnorderedSet<unsigned int>::const_iterator initVarsIter, initVarsIterEnd;
   if(behavioral_helper->GetInit(curVar, initVars))
   {
      for(initVarsIter = initVars.begin(), initVarsIterEnd = initVars.end(); initVarsIter != initVarsIterEnd; ++initVarsIter)
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "For variable " + STR(curVar) + " recursing on " + STR(*initVarsIter));
         if(already_declared_variables.find(*initVarsIter) == already_declared_variables.end() && globallyDeclVars.find(*initVarsIter) == globallyDeclVars.end())
         {
            DeclareVariable(*initVarsIter, already_declared_variables, locally_declared_types, behavioral_helper, varFunc);
         }
      }
   }
   const unsigned int variable_type = tree_helper::get_type_index(TM, curVar);
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Type is " + STR(variable_type));
   DeclareType(variable_type, behavioral_helper, locally_declared_types);
   if(!tree_helper::is_system(TM, curVar)
#if HAVE_BAMBU_BUILT
      or tree_helper::IsInLibbambu(TM, curVar)
#endif
   )
   {
      if(verbose)
         indented_output_stream->Append("//declaring variable " + STR(curVar) + " - type: " + STR(behavioral_helper->get_type(curVar)) + "\n");
      const tree_nodeRef& curr_tn = TM->get_tree_node_const(curVar);
      if(GetPointer<function_decl>(curr_tn))
      {
         instrWriter->declareFunction(curVar);
         indented_output_stream->Append(";\n");
      }
      else
      {
         indented_output_stream->Append(behavioral_helper->PrintVarDeclaration(curVar, varFunc, true) + ";\n");
      }
   }
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Declared variable " + behavioral_helper->PrintVariable(curVar));
}

const InstructionWriterRef CWriter::getInstructionWriter() const
{
   return instrWriter;
}

void CWriter::writeInclude(const std::string& file_name)
{
   if(file_name.find(".h") == std::string::npos || writtenIncludes.find(file_name) != writtenIncludes.end())
      return;
   writtenIncludes.insert(file_name);
   indented_output_stream->Append("#include \"" + file_name + "\"\n");
}

void CWriter::DeclareLocalVariables(const CustomSet<unsigned int>& to_be_declared, CustomSet<unsigned int>& already_declared_variables, CustomSet<std::string>& already_declared_types, const BehavioralHelperConstRef behavioral_helper,
                                    const var_pp_functorConstRef varFunc)
{
   const auto p = behavioral_helper->get_parameters();
   const auto TreeMan = TM;
   const auto is_to_declare = [&p, &TreeMan](unsigned int obj) -> bool {
      if(std::find(p.cbegin(), p.cend(), obj) != p.cend())
         return false;
      const tree_nodeRef node = TreeMan->get_tree_node_const(obj);
      if(node->get_kind() == parm_decl_K)
         return false;
      auto* sa = GetPointer<ssa_name>(node);
      if(sa and (sa->volatile_flag || GET_NODE(sa->CGetDefStmt())->get_kind() == gimple_nop_K) and sa->var and GET_NODE(sa->var)->get_kind() == parm_decl_K)
         return false;
      return true;
   };

   unsigned int funId = behavioral_helper->get_function_index();
   INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "-->Declaring " + STR(to_be_declared.size()) + " local variables");
   for(const auto var : to_be_declared)
      if(is_to_declare(var))
         DeclareVariable(var, already_declared_variables, already_declared_types, behavioral_helper, varFunc);
   var_pp_functorRef variableFunctor(new std_var_pp_functor(behavioral_helper));
   const FunctionBehaviorConstRef function_behavior = AppM->CGetFunctionBehavior(funId);
   const OpGraphConstRef data = function_behavior->CGetOpGraph(FunctionBehavior::DFG);
   OpVertexSet vertices = OpVertexSet(data);
   VertexIterator v, vEnd;
   for(boost::tie(v, vEnd) = boost::vertices(*data); v != vEnd; v++)
      vertices.insert(*v);
   THROW_ASSERT(vertices.size() > 0, "Graph for function " + behavioral_helper->get_function_name() + " is empty");
   compute_phi_nodes(function_behavior, vertices, variableFunctor);
   INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "<--Declaring local variables");
}

void CWriter::insert_copies(vertex b, const BBGraphConstRef bb_domGraph, const BBGraphConstRef bb_fcfgGraph, var_pp_functorConstRef variableFunctor, const CustomSet<unsigned int>& phi_instructions, std::map<unsigned int, unsigned int>& created_variables,
                            std::map<unsigned int, std::string>& symbol_table, std::map<unsigned int, std::deque<std::string>>& array_of_stacks)
{
   std::list<unsigned int> pushed;
   /// fill the renaming table for basic block b
   std::map<unsigned int, std::deque<std::string>>::const_iterator aos_it_end = array_of_stacks.end();
   for(std::map<unsigned int, std::deque<std::string>>::const_iterator aos_it = array_of_stacks.begin(); aos_it != aos_it_end; ++aos_it)
   {
      if(!aos_it->second.empty())
      {
         renaming_table[b][aos_it->first] = aos_it->second.back();
         symbol_table[aos_it->first] = aos_it->second.back();
      }
   }
   schedule_copies(b, bb_domGraph, bb_fcfgGraph, variableFunctor, phi_instructions, created_variables, symbol_table, pushed, array_of_stacks);
   OutEdgeIterator oi, oend;
   for(boost::tie(oi, oend) = boost::out_edges(b, *bb_domGraph); oi != oend; ++oi)
   {
      vertex c = boost::target(*oi, *bb_domGraph);
      insert_copies(c, bb_domGraph, bb_fcfgGraph, variableFunctor, phi_instructions, created_variables, symbol_table, array_of_stacks);
   }
   pop_stack(pushed, array_of_stacks);
}

void CWriter::schedule_copies(vertex b, const BBGraphConstRef bb_domGraph, const BBGraphConstRef bb_fcfgGraph, var_pp_functorConstRef variableFunctor, const CustomSet<unsigned int>& phi_instructions, std::map<unsigned int, unsigned int>& created_variables,
                              std::map<unsigned int, std::string>& symbol_table, std::list<unsigned int>& pushed, std::map<unsigned int, std::deque<std::string>>& array_of_stacks)
{
   /// Pass One: initialize the data structures
   const BBNodeInfoConstRef bb_node_info = bb_fcfgGraph->CGetBBNodeInfo(b);
   unsigned int bi_id = bb_node_info->block->number;

   TreeNodesPairSet copy_set, worklist;
   std::map<unsigned int, unsigned int> map;
   CustomOrderedSet<unsigned int> used_by_another;
   std::map<unsigned int, unsigned int> bb_dest_definition;
   OutEdgeIterator oi, oend;
   for(boost::tie(oi, oend) = boost::out_edges(b, *bb_fcfgGraph); oi != oend; ++oi)
   {
      vertex s = boost::target(*oi, *bb_fcfgGraph);
      const BBNodeInfoConstRef si = bb_fcfgGraph->CGetBBNodeInfo(s);
      for(const auto& phi_op : si->block->CGetPhiList())
      {
         if(phi_instructions.find(GET_INDEX_NODE(phi_op)) == phi_instructions.end())
            continue;
         auto* pn = GetPointer<gimple_phi>(GET_NODE(phi_op));
         tree_nodeRef dest = pn->res;
         unsigned int dest_i = GET_INDEX_NODE(pn->res);
         bool is_virtual = pn->virtual_flag;
         if(!is_virtual)
         {
            bb_dest_definition[dest_i] = si->block->number;
            for(const auto& def_edge : pn->CGetDefEdgesList())
            {
               if(def_edge.second == bi_id)
               {
                  tree_nodeRef src = def_edge.first;
                  unsigned int src_i = GET_INDEX_NODE(def_edge.first);
                  copy_set.insert(std::pair<tree_nodeRef, tree_nodeRef>(src, dest));
                  map[src_i] = src_i;
                  map[dest_i] = dest_i;
                  used_by_another.insert(src_i);
                  break;
               }
            }
         }
      }
   }

   /// Pass two: Set up the worklist of initial copies
   for(auto cs_it = copy_set.begin(); cs_it != copy_set.end();)
   {
      auto current_it = cs_it;
      ++cs_it;
      if(used_by_another.find(GET_INDEX_NODE(current_it->second)) == used_by_another.end())
      {
         worklist.insert(*current_it);
         copy_set.erase(current_it);
      }
   }

   /// Pass Three: Iterate over the worklist, inserting copies
   while(!worklist.empty() || !copy_set.empty())
   {
      TreeNodesPairSet worklist_restart;
      do
      {
         for(auto& wl : worklist)
         {
            tree_nodeRef src = GET_NODE(wl.first);
            unsigned int src_i = GET_INDEX_NODE(wl.first);
            unsigned int dest_i = GET_INDEX_NODE(wl.second);
            /// if dest \belongs live\_out(b)
            /// wrt the original algorithm an optimization has been added: in case b does not dominate any other node we can skip the creation of t
            //            if(boost::out_degree(b, *bb_domGraph) > 0 &&
            //                  bb_node_info->block->live_out.find(dest_i) != bb_node_info->block->live_out.end()
            //                  )
            bool add_copy = false;
            OutEdgeIterator o_it, o_it_end;
            for(boost::tie(o_it, o_it_end) = boost::out_edges(b, *bb_domGraph); o_it != o_it_end; ++o_it)
            {
               vertex tgt_bb = boost::target(*o_it, *bb_domGraph);
               if(tgt_bb == b)
                  continue;
               const BBNodeInfoConstRef tgt_bi = bb_domGraph->CGetBBNodeInfo(tgt_bb);
               if(tgt_bi && tgt_bi->block->live_in.find(dest_i) != tgt_bi->block->live_in.end())
               {
                  add_copy = true;
               }
            }
            if(add_copy)
            {
               // THROW_ERROR("check the source code @" + STR(dest_i));
               ///   insert a copy from dest to a new temp t at phi-node defining dest
               unsigned int t_i = create_new_identifier(symbol_table);
               basic_block_prefix[bb_dest_definition[dest_i]] += symbol_table.find(t_i)->second + " = " + (*variableFunctor)(dest_i) + ";\n";
               created_variables[t_i] = dest_i;
               map[t_i] = t_i;
               ///   push(t, Stack[dest])
               push_stack(symbol_table.find(t_i)->second, dest_i, pushed, array_of_stacks);
               renaming_table[b][dest_i] = symbol_table.find(t_i)->second;
            }
            /// insert a copy operation from map[src] to dest at the end of b
            std::string copy_statement;
            if(symbol_table.find(map.find(src_i)->second) != symbol_table.end())
               copy_statement += (*variableFunctor)(dest_i) + " = " + symbol_table.find(map.find(src_i)->second)->second + ";\n";
            else if(dest_i != map.find(src_i)->second)
               copy_statement += (*variableFunctor)(dest_i) + " = " + (*variableFunctor)(map.find(src_i)->second) + ";\n";

            basic_block_tail[bi_id] += copy_statement;
            // map[src_i] = dest_i;

            for(auto cs1_it = copy_set.begin(); cs1_it != copy_set.end();)
            {
               auto current1_it = cs1_it;
               ++cs1_it;
               if(src == current1_it->second)
               {
                  worklist_restart.insert(*current1_it);
                  copy_set.erase(current1_it);
               }
            }
         }
         worklist = worklist_restart;
         worklist_restart.clear();
      } while(!worklist.empty());

      auto cs2_it_end = copy_set.end();
      auto cs2_it = copy_set.begin();
      if(cs2_it != cs2_it_end)
      {
         auto current2_it = cs2_it;
         unsigned int dest_i = GET_INDEX_NODE(current2_it->second);
         /// check if dest_i is source of any other pair in copy_set
         /// this optimization is not described in the original algorithm
         ++cs2_it;
         bool add_temporary = false;
         for(; cs2_it != cs2_it_end; ++cs2_it)
         {
            if(GET_INDEX_NODE(cs2_it->first) == dest_i)
            {
               add_temporary = true;
               break;
            }
         }
         if(add_temporary)
         {
            /// create a new symbol
            unsigned int t_i = create_new_identifier(symbol_table);
            /// insert a copy from dest to a new temp t at the end of b
            basic_block_tail[bi_id] += symbol_table.find(t_i)->second + " = " + (*variableFunctor)(dest_i) + ";\n";
            created_variables[t_i] = dest_i;
            map[t_i] = t_i;
            map[dest_i] = t_i;
         }
         worklist.insert(*current2_it);
         copy_set.erase(current2_it);
      }
   }
}

unsigned int CWriter::create_new_identifier(std::map<unsigned int, std::string>& symbol_table)
{
   unsigned int counter = 0;
   unsigned int node_id_this = 0;
   std::string new_name;
   do
   {
      new_name = "__t__" + STR(fake_max_tree_node_id) + "_" + STR(counter);
      node_id_this = TM->find_identifier_nodeID(new_name);
      counter++;
   } while(node_id_this > 0);
   symbol_table[fake_max_tree_node_id] = new_name;
   node_id_this = fake_max_tree_node_id;
   ++fake_max_tree_node_id;
   return node_id_this;
}

void CWriter::push_stack(std::string symbol_name, unsigned int dest_i, std::list<unsigned int>& pushed, std::map<unsigned int, std::deque<std::string>>& array_of_stacks)
{
   THROW_ASSERT(std::find(pushed.begin(), pushed.end(), dest_i) == pushed.end(), "multiple push is not allowed");
   array_of_stacks[dest_i].push_back(symbol_name);
   pushed.push_back(dest_i);
}

void CWriter::pop_stack(std::list<unsigned int>& pushed, std::map<unsigned int, std::deque<std::string>>& array_of_stacks)
{
   for(auto var_id : pushed)
   {
      THROW_ASSERT(array_of_stacks.find(var_id) != array_of_stacks.end(), "the array of stacks is inconsistent");
      THROW_ASSERT(!array_of_stacks.find(var_id)->second.empty(), "the variable is not mapped");
      array_of_stacks.find(var_id)->second.pop_back();
   }
   pushed.clear();
}

void CWriter::WriteHashTableImplementation()
{
   indented_output_stream->Append("#define NIL(type)    ((type *) 0)\n");
   indented_output_stream->Append("typedef struct st_table_entry st_table_entry;\n");
   indented_output_stream->Append("struct st_table_entry\n");
   indented_output_stream->Append("{\n");
   indented_output_stream->Append("unsigned long long key;\n");
   indented_output_stream->Append("unsigned long long record;\n");
   indented_output_stream->Append("st_table_entry *next;\n");
   indented_output_stream->Append("};\n");
   indented_output_stream->Append("\n");
   indented_output_stream->Append("typedef struct st_table st_table;\n");
   indented_output_stream->Append("struct st_table\n");
   indented_output_stream->Append("{\n");
   indented_output_stream->Append("int num_bins;\n");
   indented_output_stream->Append("int num_entries;\n");
   indented_output_stream->Append("int max_density;\n");
   indented_output_stream->Append("int reorder_flag;\n");
   indented_output_stream->Append("double grow_factor;\n");
   indented_output_stream->Append("st_table_entry **bins;\n");
   indented_output_stream->Append("};\n");
   indented_output_stream->Append("typedef struct st_generator st_generator;\n");
   indented_output_stream->Append("struct st_generator\n");
   indented_output_stream->Append("{\n");
   indented_output_stream->Append("st_table *table;\n");
   indented_output_stream->Append("st_table_entry *entry;\n");
   indented_output_stream->Append("int index;\n");
   indented_output_stream->Append("};\n");
   indented_output_stream->Append("#define ST_DEFAULT_MAX_DENSITY 5\n");
   indented_output_stream->Append("#define ST_DEFAULT_INIT_TABLE_SIZE 11\n");
   indented_output_stream->Append("#define ST_DEFAULT_GROW_FACTOR 2.0\n");
   indented_output_stream->Append("#define ST_DEFAULT_REORDER_FLAG 0\n");
   indented_output_stream->Append("#define ST_OUT_OF_MEM -10000\n");
   indented_output_stream->Append("#define FREE(obj) ((obj) ? (free((char *) (obj)), (obj) = 0) : 0)\n");
   indented_output_stream->Append("#define ALLOC(type, num) ((type *) malloc(sizeof(type) * (num)))\n");
   indented_output_stream->Append("#if SIZEOF_VOID_P == 8\n");
   indented_output_stream->Append("#define st_shift 3\n");
   indented_output_stream->Append("#else\n");
   indented_output_stream->Append("#define st_shift 2\n");
   indented_output_stream->Append("#endif\n");
   indented_output_stream->Append("#define ST_PTRHASH(x,size) ((unsigned int)((unsigned long)(x)>>st_shift)%size)\n");
   indented_output_stream->Append("\n");
   indented_output_stream->Append("static int rehash(st_table *table)\n");
   indented_output_stream->Append("{\n");
   indented_output_stream->Append("st_table_entry *ptr, *next, **old_bins;\n");
   indented_output_stream->Append("int i, old_num_bins, hash_val, old_num_entries;\n");
   indented_output_stream->Append("/* save old values */\n");
   indented_output_stream->Append("old_bins = table->bins;\n");
   indented_output_stream->Append("old_num_bins = table->num_bins;\n");
   indented_output_stream->Append("old_num_entries = table->num_entries;\n");
   indented_output_stream->Append("/* rehash */\n");
   indented_output_stream->Append("table->num_bins = (int) (table->grow_factor * old_num_bins);\n");
   indented_output_stream->Append("if (table->num_bins % 2 == 0)\n");
   indented_output_stream->Append("{\n");
   indented_output_stream->Append("table->num_bins += 1;\n");
   indented_output_stream->Append("}\n");
   indented_output_stream->Append("table->num_entries = 0;\n");
   indented_output_stream->Append("table->bins = ALLOC(st_table_entry *, table->num_bins);\n");
   indented_output_stream->Append("if (table->bins == NIL(st_table_entry *))\n");
   indented_output_stream->Append("{\n");
   indented_output_stream->Append("table->bins = old_bins;\n");
   indented_output_stream->Append("table->num_bins = old_num_bins;\n");
   indented_output_stream->Append("table->num_entries = old_num_entries;\n");
   indented_output_stream->Append("return ST_OUT_OF_MEM;\n");
   indented_output_stream->Append("}\n");
   indented_output_stream->Append("/* initialize */\n");
   indented_output_stream->Append("for (i = 0; i < table->num_bins; i++)\n");
   indented_output_stream->Append("{\n");
   indented_output_stream->Append("table->bins[i] = 0;\n");
   indented_output_stream->Append("}\n");
   indented_output_stream->Append("/* copy data over */\n");
   indented_output_stream->Append("for (i = 0; i < old_num_bins; i++)\n");
   indented_output_stream->Append("{\n");
   indented_output_stream->Append("ptr = old_bins[i];\n");
   indented_output_stream->Append("while (ptr != NIL(st_table_entry))\n");
   indented_output_stream->Append("{\n");
   indented_output_stream->Append("next = ptr->next;\n");
   indented_output_stream->Append("hash_val = ptr->key%table->num_bins;\n");
   indented_output_stream->Append("ptr->next = table->bins[hash_val];\n");
   indented_output_stream->Append("table->bins[hash_val] = ptr;\n");
   indented_output_stream->Append("table->num_entries++;\n");
   indented_output_stream->Append("ptr = next;\n");
   indented_output_stream->Append("}\n");
   indented_output_stream->Append("}\n");
   indented_output_stream->Append("FREE(old_bins);\n");
   indented_output_stream->Append("return 1;\n");
   indented_output_stream->Append("} /* rehash */\n");
   indented_output_stream->Append("\n");
   indented_output_stream->Append("st_table * st_init_table_with_params(\n");
   indented_output_stream->Append("int size,\n");
   indented_output_stream->Append("int density,\n");
   indented_output_stream->Append("double grow_factor,\n");
   indented_output_stream->Append("int reorder_flag)\n");
   indented_output_stream->Append("{\n");
   indented_output_stream->Append("int i;\n");
   indented_output_stream->Append("st_table *newt;\n");
   indented_output_stream->Append("newt = ALLOC(st_table, 1);\n");
   indented_output_stream->Append("if (newt == NIL(st_table)) \n");
   indented_output_stream->Append("{\n");
   indented_output_stream->Append("return NIL(st_table);\n");
   indented_output_stream->Append("}\n");
   indented_output_stream->Append("newt->num_entries = 0;\n");
   indented_output_stream->Append("newt->max_density = density;\n");
   indented_output_stream->Append("newt->grow_factor = grow_factor;\n");
   indented_output_stream->Append("newt->reorder_flag = reorder_flag;\n");
   indented_output_stream->Append("if (size <= 0)\n");
   indented_output_stream->Append("{\n");
   indented_output_stream->Append("size = 1;\n");
   indented_output_stream->Append("}\n");
   indented_output_stream->Append("newt->num_bins = size;\n");
   indented_output_stream->Append("newt->bins = ALLOC(st_table_entry *, size);\n");
   indented_output_stream->Append("if (newt->bins == NIL(st_table_entry *))\n");
   indented_output_stream->Append("{\n");
   indented_output_stream->Append("FREE(newt);\n");
   indented_output_stream->Append("return NIL(st_table);\n");
   indented_output_stream->Append("}\n");
   indented_output_stream->Append("for(i = 0; i < size; i++)\n");
   indented_output_stream->Append("{\n");
   indented_output_stream->Append("newt->bins[i] = 0;\n");
   indented_output_stream->Append("}\n");
   indented_output_stream->Append("return newt;\n");
   indented_output_stream->Append("} /* st_init_table_with_params */\n");
   indented_output_stream->Append("\n");
   indented_output_stream->Append("st_table * st_init_table()\n");
   indented_output_stream->Append("{\n");
   indented_output_stream->Append("return st_init_table_with_params(ST_DEFAULT_INIT_TABLE_SIZE,\n");
   indented_output_stream->Append("ST_DEFAULT_MAX_DENSITY,\n");
   indented_output_stream->Append("ST_DEFAULT_GROW_FACTOR,\n");
   indented_output_stream->Append("ST_DEFAULT_REORDER_FLAG);\n");
   indented_output_stream->Append("} /* st_init_table */\n");
   indented_output_stream->Append("\n");
   indented_output_stream->Append("st_generator *\n");
   indented_output_stream->Append("st_init_gen(st_table *table)\n");
   indented_output_stream->Append("{\n");
   indented_output_stream->Append("st_generator *gen;\n");
   indented_output_stream->Append("gen = ALLOC(st_generator, 1);\n");
   indented_output_stream->Append("if (gen == NIL(st_generator))\n");
   indented_output_stream->Append("{\n");
   indented_output_stream->Append("return NIL(st_generator);\n");
   indented_output_stream->Append("}\n");
   indented_output_stream->Append("gen->table = table;\n");
   indented_output_stream->Append("gen->entry = NIL(st_table_entry);\n");
   indented_output_stream->Append("gen->index = 0;\n");
   indented_output_stream->Append("return gen;\n");
   indented_output_stream->Append("} /* st_init_gen */\n");
   indented_output_stream->Append("\n");
   indented_output_stream->Append("int st_gen_int(st_generator *gen, unsigned long long * key_p, unsigned long long * value_p)\n");
   indented_output_stream->Append("{\n");
   indented_output_stream->Append("int i;\n");
   indented_output_stream->Append("if (gen->entry == NIL(st_table_entry))\n");
   indented_output_stream->Append("{\n");
   indented_output_stream->Append("/* try to find next entry */\n");
   indented_output_stream->Append("for(i = gen->index; i < gen->table->num_bins; i++)\n");
   indented_output_stream->Append("{\n");
   indented_output_stream->Append("if (gen->table->bins[i] != NIL(st_table_entry))\n");
   indented_output_stream->Append("{\n");
   indented_output_stream->Append("gen->index = i+1;\n");
   indented_output_stream->Append("gen->entry = gen->table->bins[i];\n");
   indented_output_stream->Append("break;\n");
   indented_output_stream->Append("}\n");
   indented_output_stream->Append("}\n");
   indented_output_stream->Append("if (gen->entry == NIL(st_table_entry))\n");
   indented_output_stream->Append("{\n");
   indented_output_stream->Append("return 0;     /* that's all folks ! */\n");
   indented_output_stream->Append("}\n");
   indented_output_stream->Append("}\n");
   indented_output_stream->Append("*key_p = gen->entry->key;\n");
   indented_output_stream->Append("if (value_p != NIL(unsigned long long))\n");
   indented_output_stream->Append("{\n");
   indented_output_stream->Append("*value_p = gen->entry->record;\n");
   indented_output_stream->Append("}\n");
   indented_output_stream->Append("gen->entry = gen->entry->next;\n");
   indented_output_stream->Append("return 1;\n");
   indented_output_stream->Append("} /* st_gen_int */\n");
   indented_output_stream->Append("\n");
   indented_output_stream->Append("void st_free_gen(st_generator *gen)\n");
   indented_output_stream->Append("{\n");
   indented_output_stream->Append("FREE(gen);\n");
   indented_output_stream->Append("} /* st_free_gen */\n");
   indented_output_stream->Append("\n");
   indented_output_stream->Append("int st_find_or_add(st_table *table, unsigned long long key, unsigned long long ** slot)\n");
   indented_output_stream->Append("{\n");
   indented_output_stream->Append("int hash_val;\n");
   indented_output_stream->Append("st_table_entry *newt, *ptr, **last;\n");
   indented_output_stream->Append("hash_val = key%table->num_bins;\n");
   indented_output_stream->Append("last = &(table)->bins[hash_val];\n");
   indented_output_stream->Append("ptr = *(last);\n");
   indented_output_stream->Append("while (ptr != NIL(st_table_entry) && ptr->key != key)\n");
   indented_output_stream->Append("{\n");
   indented_output_stream->Append("last = &(ptr)->next;\n");
   indented_output_stream->Append("ptr = *(last);\n");
   indented_output_stream->Append("}\n");
   indented_output_stream->Append("if (ptr != NIL(st_table_entry) && (table)->reorder_flag)\n");
   indented_output_stream->Append("{\n");
   indented_output_stream->Append("*(last) = (ptr)->next;\n");
   indented_output_stream->Append("(ptr)->next = (table)->bins[hash_val];\n");
   indented_output_stream->Append("(table)->bins[hash_val] = (ptr);\n");
   indented_output_stream->Append("}\n");
   indented_output_stream->Append("if (ptr == NIL(st_table_entry)) \n");
   indented_output_stream->Append("{\n");
   indented_output_stream->Append("if (table->num_entries / table->num_bins >= table->max_density) \n");
   indented_output_stream->Append("{\n");
   indented_output_stream->Append("if (rehash(table) == ST_OUT_OF_MEM) \n");
   indented_output_stream->Append("{\n");
   indented_output_stream->Append("return ST_OUT_OF_MEM;\n");
   indented_output_stream->Append("}\n");
   indented_output_stream->Append("hash_val = key%table->num_bins;\n");
   indented_output_stream->Append("}\n");
   indented_output_stream->Append("newt = ALLOC(st_table_entry, 1);\n");
   indented_output_stream->Append("if (newt == NIL(st_table_entry)) \n");
   indented_output_stream->Append("{\n");
   indented_output_stream->Append("return ST_OUT_OF_MEM;\n");
   indented_output_stream->Append("}\n");
   indented_output_stream->Append("newt->key = key;\n");
   indented_output_stream->Append("newt->record = 0;\n");
   indented_output_stream->Append("newt->next = table->bins[hash_val];\n");
   indented_output_stream->Append("table->bins[hash_val] = newt;\n");
   indented_output_stream->Append("table->num_entries++;\n");
   indented_output_stream->Append("if (slot != NIL(void)) \n");
   indented_output_stream->Append("*slot = &newt->record;\n");
   indented_output_stream->Append("return 0;\n");
   indented_output_stream->Append("}\n");
   indented_output_stream->Append("else\n");
   indented_output_stream->Append("{\n");
   indented_output_stream->Append("if (slot != NIL(void)) \n");
   indented_output_stream->Append("*slot = &ptr->record;\n");
   indented_output_stream->Append("return 1;\n");
   indented_output_stream->Append("}\n");
   indented_output_stream->Append("} /* st_find_or_add */\n");
   indented_output_stream->Append("#define st_foreach_item_int(table, gen, key, value) for(gen=st_init_gen(table); st_gen_int(gen,key,value) || (st_free_gen(gen),0);)\n");
}

void CWriter::WriteFile(const std::string& file_name)
{
   indented_output_stream->WriteFile(file_name);
}

void CWriter::WriteBBHeader(const unsigned int, const unsigned int)
{
}

void CWriter::WriteBuiltinWaitCall()
{
   indented_output_stream->Append("void " + STR(BUILTIN_WAIT_CALL) + "(void * ptr, ...)\n");
   indented_output_stream->Append("{\n");
   indented_output_stream->Append("va_list ap;\n");
   indented_output_stream->Append("va_start(ap, ptr);\n");
   indented_output_stream->Append("int boolReturn = va_arg(ap, int);\n");
   for(const unsigned int id : AppM->CGetCallGraphManager()->GetAddressedFunctions())
   {
      const BehavioralHelperConstRef BH = AppM->CGetFunctionBehavior(id)->CGetBehavioralHelper();
      indented_output_stream->Append("if (ptr == " + BH->get_function_name() + ")\n");
      indented_output_stream->Append("{\n");
      std::vector<std::pair<std::string, std::string>> typeAndName;
      for(const auto& I : BH->get_parameters())
      {
         unsigned int type_index = BH->get_type(I);
         std::string type = BH->print_type(type_index);
         if(BH->is_int(type_index))
         {
            if(BH->get_size(type_index) < 32)
               type = "int";
         }
         else if(BH->is_unsigned(type_index))
         {
            if(BH->get_size(type_index) < 32)
               type = "unsigned int";
         }
         else if(BH->is_real(type_index))
         {
            if(BH->get_size(type_index) < 64)
               type = "double";
         }
         std::string name = BH->PrintVariable(I);
         typeAndName.push_back(std::make_pair(type, name));
      }
      for(const auto& I : typeAndName)
      {
         indented_output_stream->Append(I.first + " " + I.second + " = va_arg(ap, " + I.first + ");\n");
      }
      unsigned int returnTypeIdx = BH->GetFunctionReturnType(id);
      std::string returnType;
      if(returnTypeIdx)
      {
         returnType = BH->print_type(returnTypeIdx);
         if(BH->is_int(returnTypeIdx))
         {
            if(BH->get_size(returnTypeIdx) < 32)
               returnType = "int";
         }
         else if(BH->is_unsigned(returnTypeIdx))
         {
            if(BH->get_size(returnTypeIdx) < 32)
               returnType = "unsigned int";
         }
         else if(BH->is_real(returnTypeIdx))
         {
            if(BH->get_size(returnTypeIdx) < 64)
               returnType = "double";
         }
      }
      else
         returnType = "void";
      if(returnType != "void")
      {
         indented_output_stream->Append(returnType + " res = ");
      }
      indented_output_stream->Append(BH->get_function_name() + "(");
      size_t typeAndNameSize = typeAndName.size();
      if(typeAndNameSize)
      {
         for(size_t i = 0; i < typeAndNameSize - 1; ++i)
         {
            indented_output_stream->Append(typeAndName[i].second + ",");
         }
         indented_output_stream->Append(typeAndName[typeAndNameSize - 1].second);
      }
      indented_output_stream->Append(");\n");
      if(returnType != "void")
      {
         indented_output_stream->Append("if (boolReturn) {\n");
         indented_output_stream->Append(returnType + "* " + "resultAddress = va_arg(ap, " + returnType + " *);\n");
         indented_output_stream->Append("*resultAddress = res;\n");
         indented_output_stream->Append("}\n");
      }
      indented_output_stream->Append("}\n");
   }
   indented_output_stream->Append("va_end(ap);\n");
   indented_output_stream->Append("}\n\n");
}
