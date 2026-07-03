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
 * @file c_writer.cpp
 * @brief This file contains the routines necessary to create a C executable program starting from an abstract
 * decription of the threads composing the application.
 *
 * This file contains the routines necessary to create a C executable program
 * starting from an abstract decription of the threads composing the application.
 *
 * @author Luca Fossati <fossati@elet.polimi.it>
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 * @author Marco Lattuada <lattuada@elet.polimi.it>
 *
 */
#include "c_writer.hpp"

#include "Parameter.hpp"
#include "SemiNCADominance.hpp"
#include "application_manager.hpp"
#include "basic_block.hpp"
#include "behavioral_helper.hpp"
#include "c_backend.hpp"
#include "c_backend_information.hpp"
#include "c_backend_step_factory.hpp"
#include "call_graph_manager.hpp"
#include "config_HAVE_HOST_PROFILING_BUILT.hpp"
#include "config_PACKAGE_NAME.hpp"
#include "constant_strings.hpp"
#include "discrepancy_analysis_c_writer.hpp"
#include "discrepancy_instruction_writer.hpp"
#include "function_behavior.hpp"
#include "hls_c_writer.hpp"
#include "hls_instruction_writer.hpp"
#include "hls_manager.hpp"
#include "indented_output_stream.hpp"
#include "instruction_writer.hpp"
#include "ir_basic_block.hpp"
#include "ir_helper.hpp"
#include "ir_manager.hpp"
#include "loop.hpp"
#include "loops.hpp"
#include "mdpi_wrapper_c_writer.hpp"
#include "op_graph.hpp"
#include "utility.hpp"
#include "var_pp_functor.hpp"
#include <boost/range/adaptor/reversed.hpp>
#if HAVE_HOST_PROFILING_BUILT
#include "basic_blocks_profiling_c_writer.hpp"
#endif
#if __WORDSIZE < 64
#include <boost/functional/hash/hash.hpp>
#endif

/// Hash function for std::pair<ir_nodeRef, ir_nodeRef>
struct IRNodesPairHash
{
   size_t operator()(const std::pair<ir_nodeRef, ir_nodeRef>& val) const
   {
      size_t hash_value = 0;
#if __WORDSIZE < 64
      boost::hash_combine(hash_value, val.first);
      boost::hash_combine(hash_value, val.second);
#else
      hash_value = (static_cast<size_t>(val.second->index) << 32) | val.first->index;
#endif
      return hash_value;
   }
};

struct IRNodesPairEqual
{
   bool operator()(const std::pair<ir_nodeRef, ir_nodeRef>& x, const std::pair<ir_nodeRef, ir_nodeRef>& y) const
   {
      return x.first->index == y.first->index && x.second->index == y.second->index;
   }
};

struct IRNodesPairSorter
{
   bool operator()(const std::pair<ir_nodeRef, ir_nodeRef>& x, const std::pair<ir_nodeRef, ir_nodeRef>& y) const
   {
      return x.first->index == y.first->index ? x.second->index < y.second->index : x.first->index < y.first->index;
   }
};

#if HAVE_UNORDERED
using IRNodesPairSet = CustomUnorderedSet<std::pair<ir_nodeRef, ir_nodeRef>, IRNodesPairHash, IRNodesPairEqual>;
#else
using IRNodesPairSet = CustomOrderedSet<std::pair<ir_nodeRef, ir_nodeRef>, IRNodesPairSorter>;
#endif

CWriter::CWriter(const HLS_managerConstRef _HLSMgr, const InstructionWriterRef _instruction_writer,
                 const IndentedOutputStreamRef _indented_output_stream)
    : HLSMgr(_HLSMgr),
      TM(_HLSMgr->get_ir_manager()),
      indented_output_stream(_indented_output_stream),
      instrWriter(_instruction_writer),
      declared_functions(),
      defined_functions(),
      bb_label_counter(0),
      verbose(_HLSMgr->get_parameter()->getOption<int>(OPT_debug_level) >= DEBUG_LEVEL_VERBOSE),
      Param(_HLSMgr->get_parameter()),
      debug_level(_HLSMgr->get_parameter()->get_class_debug_level("CWriter")),
      output_level(_HLSMgr->get_parameter()->getOption<int>(OPT_output_level)),
      fake_max_ir_node_id(0)
{
}

CWriterRef CWriter::CreateCWriter(const CBackendInformationConstRef c_backend_info, const HLS_managerConstRef hls_man,
                                  const IndentedOutputStreamRef indented_output_stream)
{
   const auto app_man = std::static_pointer_cast<const application_manager>(hls_man);
   switch(c_backend_info->type)
   {
      case(CBackendInformation::CB_BBP):
      {
#if HAVE_HOST_PROFILING_BUILT
         const InstructionWriterRef instruction_writer(
             new InstructionWriter(app_man, indented_output_stream, hls_man->get_parameter()));
         return CWriterRef(new BasicBlocksProfilingCWriter(hls_man, instruction_writer, indented_output_stream));
#else
         break;
#endif
      }
#if HAVE_HLS_BUILT
      case(CBackendInformation::CB_DISCREPANCY_ANALYSIS):
      {
         const InstructionWriterRef instruction_writer(
             new discrepancy_instruction_writer(app_man, indented_output_stream, hls_man->get_parameter()));

         return CWriterRef(
             new DiscrepancyAnalysisCWriter(c_backend_info, hls_man, instruction_writer, indented_output_stream));
      }
#endif
      case(CBackendInformation::CB_HLS):
      {
         const InstructionWriterRef instruction_writer(
             new HLSInstructionWriter(app_man, indented_output_stream, hls_man->get_parameter()));
         return CWriterRef(new HLSCWriter(c_backend_info, hls_man, instruction_writer, indented_output_stream));
      }
      case(CBackendInformation::CB_SEQUENTIAL):
      {
         const InstructionWriterRef instruction_writer(
             new InstructionWriter(app_man, indented_output_stream, hls_man->get_parameter()));
         return CWriterRef(new CWriter(hls_man, instruction_writer, indented_output_stream));
      }
      case(CBackendInformation::CB_MDPI_WRAPPER):
      {
         const InstructionWriterRef instruction_writer(
             new HLSInstructionWriter(app_man, indented_output_stream, hls_man->get_parameter()));
         return CWriterRef(new MdpiWrapperCWriter(hls_man, instruction_writer, indented_output_stream));
      }
      default:
      {
         break;
      }
   }
   THROW_UNREACHABLE("");
   return CWriterRef();
}

void CWriter::declare_cast_types(unsigned int funId, CustomSet<std::string>& locally_declared_types)
{
   const auto function_behavior = HLSMgr->CGetFunctionBehavior(funId);
   const auto BH = function_behavior->CGetBehavioralHelper();
   const auto inGraph = function_behavior->GetOpGraph(FunctionBehavior::DFG);
   // I simply have to go over all the vertices and look for types used for type casting;
   for(const auto& v : inGraph.vertices())
   {
      const auto& node = inGraph.CGetNodeInfo(v).node;
      if(node)
      {
         IRNodeConstSet types;
         BH->GetTypecast(node, types);
         for(const auto& t : types)
         {
            DeclareType(t, BH, locally_declared_types);
         }
      }
   }
}

void CWriter::InternalInitialize()
{
   declared_functions = HLSMgr->get_functions_without_body();
   defined_functions = HLSMgr->get_functions_with_body();
}

void CWriter::Initialize()
{
   fake_max_ir_node_id = TM->get_next_available_ir_node_id();
   instrWriter->Initialize();
   globally_declared_types.clear();
   globallyDeclVars.clear();
   additionalIncludes.clear();
   writtenIncludes.clear();
   InternalInitialize();
}

void CWriter::WriteBodyLoop(const unsigned int fid, const unsigned int, gc_vertex_descriptor current_vertex,
                            bool bracket, const std::unique_ptr<var_pp_functor>& variableFunctor)
{
   writeRoutineInstructions_rec(fid, current_vertex, bracket, variableFunctor);
}

void CWriter::WriteFunctionBody(unsigned int function_id)
{
   const auto function_behavior = HLSMgr->CGetFunctionBehavior(function_id);
   const auto op_graph = function_behavior->GetOpGraph(FunctionBehavior::CFG);
   const auto BH = function_behavior->CGetBehavioralHelper();

   OpVertexSet vertices(&function_behavior->GetOpGraphsCollection());
   const auto [statement, statement_end] = boost::vertices(op_graph);
   vertices.insert(statement, statement_end);
   THROW_ASSERT(vertices.size() > 0, "Graph for function " + BH->GetFunctionName() + " is empty");
   writeRoutineInstructions(function_id, vertices, std::make_unique<std_var_pp_functor>(BH));
}

void CWriter::WriteFunctionImplementation(unsigned int function_id)
{
   StartFunctionBody(function_id);
   WriteFunctionBody(function_id);
   EndFunctionBody(function_id);
}

void CWriter::AnalyzeInclude(const ir_nodeConstRef& tn, const BehavioralHelperConstRef& BH,
                             CustomOrderedSet<std::string>& includes_to_write, CustomSet<unsigned int>& already_visited)
{
   if(already_visited.count(tn->index))
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Skipped already analyzed " + STR(tn->index));
      return;
   }
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                  "-->Computing include for " + STR(tn->index) + " " +
                      (ir_helper::IsFunctionDeclaration(tn) ? "" : STR(tn)));
   already_visited.insert(tn->index);
   bool is_system;
   const auto decl = std::get<0>(ir_helper::GetSourcePath(tn, is_system));
   if(!decl.empty() && decl != "<built-in>" && is_system && !ir_helper::IsInLibbambu(tn))
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Adding " + decl + " to the list of includes");
      includes_to_write.insert(decl);
   }
   else
   {
      const auto type = ir_helper::CGetType(tn);
      const auto types_to_be_declared_before =
          ir_helper::GetTypesToBeDeclaredBefore(type, Param->getOption<bool>(OPT_without_transformation));
      for(const auto& type_to_be_declared : types_to_be_declared_before)
      {
         AnalyzeInclude(type_to_be_declared, BH, includes_to_write, already_visited);
      }
      const auto types_to_be_declared_after =
          ir_helper::GetTypesToBeDeclaredAfter(type, Param->getOption<bool>(OPT_without_transformation));
      for(const auto& type_to_be_declared : types_to_be_declared_after)
      {
         AnalyzeInclude(type_to_be_declared, BH, includes_to_write, already_visited);
      }
   }
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Computed");
}

void CWriter::InternalWriteHeader()
{
   indented_output_stream->Append(R"(
#include <mdpi/mdpi_pp.h>

#include <stdbool.h>
#include <sys/types.h>
)");

   bool is_builtin_select32 = false;
   for(const auto fid : declared_functions)
   {
      const auto BH = HLSMgr->CGetFunctionBehavior(fid)->CGetBehavioralHelper();
      if(BH->GetFunctionName() == "__builtin_select32")
      {
         is_builtin_select32 = true;
      }
   }
   if(is_builtin_select32)
   {
      indented_output_stream->Append("#define __builtin_select32(cond, value1, value2) cond ? value1 : value2\n\n");
   }
   // TODO: add bambu param manager implementation
}

void CWriter::WriteHeader()
{
   indented_output_stream->Append("/*\n");
   indented_output_stream->Append(" * Politecnico di Milano\n");
   indented_output_stream->Append(" * Code created using " PACKAGE_NAME " - " + Param->PrintVersion());
   indented_output_stream->Append(" - Date " + TimeStamp::GetCurrentTimeStamp());
   indented_output_stream->Append("\n");
   if(Param->isOption(OPT_cat_args))
   {
      indented_output_stream->Append(" * Bambu executed with: " + Param->getOption<std::string>(OPT_cat_args) + "\n");
   }
   indented_output_stream->Append(" */\n");

   InternalWriteHeader();

   CustomOrderedSet<std::string> includes_to_write;
   CustomSet<unsigned int> already_visited;
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Computing includes for external functions");
   for(const auto f_id : defined_functions)
   {
      const auto FB = HLSMgr->CGetFunctionBehavior(f_id);
      const auto BH = FB->CGetBehavioralHelper();
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Computing includes for " + BH->GetFunctionName());
      AnalyzeInclude(TM->GetIRNode(f_id), BH, includes_to_write, already_visited);

      IRNodeConstSet decl_nodes;
      const auto& tmp_vars = GetLocalVariables(f_id);
      for(const auto& tmp_var : tmp_vars)
      {
         decl_nodes.insert(TM->GetIRNode(tmp_var));
      }
      const auto funParams = BH->GetParameters();
      decl_nodes.insert(funParams.begin(), funParams.end());
      const auto& vars = HLSMgr->GetGlobalVariables();
      decl_nodes.insert(vars.begin(), vars.end());

      for(const auto& v : decl_nodes)
      {
         const auto variable_type = ir_helper::CGetType(v);
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                        "-->Analyzing includes for variable " + BH->PrintVariable(v->index) + " of type " +
                            STR(variable_type));
         AnalyzeInclude(variable_type, BH, includes_to_write, already_visited);
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                        "<--Analyzed includes for variable " + BH->PrintVariable(v->index) + " of type " +
                            STR(variable_type));
      }

      const auto op_graph = FB->GetOpGraph(FunctionBehavior::DFG);
      for(const auto& v : op_graph.vertices())
      {
         const auto& node = op_graph.CGetNodeInfo(v).node;
         if(node)
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Analyzing includes for operation " + STR(node));
            IRNodeConstSet types;
            BH->GetTypecast(node, types);
            for(const auto& type : types)
            {
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Analyzing includes for type " + STR(type));
               AnalyzeInclude(type, BH, includes_to_write, already_visited);
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Analyzed includes for type " + STR(type));
            }
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Analyzed includes for operation " + STR(node));
         }
      }
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Computed includes for " + BH->GetFunctionName());
   }
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Computed includes for external functions");

   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Writing includes");
   for(const auto& s : includes_to_write)
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Writing: " + s);
      writeInclude(s);
   }
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Written includes");
}

void CWriter::InternalWriteGlobalDeclarations()
{
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Writing global declarations");
   /// Writing auxiliary variables used by instruction writer
   instrWriter->write_declarations();

   /// Writing declarations of global variables
   THROW_ASSERT(defined_functions.size() > 0, "at least one function is expected");
   unsigned int first_fun = *defined_functions.begin();
   const auto BH = HLSMgr->CGetFunctionBehavior(first_fun)->CGetBehavioralHelper();

   const auto& gblVariables = HLSMgr->GetGlobalVariables();
   // Write the declarations for the global variables
   const std::unique_ptr<var_pp_functor> variableFunctor = std::make_unique<std_var_pp_functor>(BH);
   for(const auto& glbVar : gblVariables)
   {
      DeclareVariable(glbVar, globallyDeclVars, globally_declared_types, BH, variableFunctor);
   }
   indented_output_stream->Append("\n");
   if(HLSMgr->CGetCallGraphManager().ExistsAddressedFunction())
   {
      indented_output_stream->Append("#include <stdarg.h>\n\n");
      indented_output_stream->Append("void " BUILTIN_WAIT_CALL "(void * ptr, ...);\n");
   }
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Written global declarations");
}

void CWriter::WriteGlobalDeclarations()
{
   InternalWriteGlobalDeclarations();
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Writing function prototypes");
   for(const auto fid : HLSMgr->CGetCallGraphManager().GetRootFunctions())
   {
      const auto fnode = TM->GetIRNode(fid);
      const auto fd = GetPointerS<const function_val_node>(fnode);
      const auto is_void = ir_helper::GetFunctionReturnType(fnode) == nullptr;
      const auto mem_idx = std::to_string(fd->list_of_args.size() + (!is_void));
      indented_output_stream->Append("#define __mem_bambu_artificial_idx " + mem_idx + "\n");
      const auto fsymbol = ir_helper::GetFunctionName(fnode);
      const auto arch = HLSMgr->module_arch->GetArchitecture(fsymbol);
      for(const auto& [parm, attrs] : arch->parms)
      {
         const auto idx = attrs.at(FunctionArchitecture::parm_index);
         const auto iface_mode =
             arch->ifaces.at(attrs.at(FunctionArchitecture::parm_bundle)).at(FunctionArchitecture::iface_mode);
         if(iface_mode != "m_axi" && iface_mode != "default")
         {
            indented_output_stream->Append("#define " + parm + "_bambu_artificial_idx " + idx + "\n");
         }
      }
      for(const auto& [iface, attrs] : arch->ifaces)
      {
         THROW_ASSERT(attrs.find(FunctionArchitecture::iface_alignment) != attrs.end(),
                      "Interface " + iface + " has no alignment");
         auto align = attrs.at(FunctionArchitecture::iface_alignment);
         const auto mode = attrs.at(FunctionArchitecture::iface_mode);
         if(mode == "m_axi")
         {
            align = "1";
            indented_output_stream->Append("#define " + iface + "_bambu_artificial_idx __mem_bambu_artificial_idx\n");
         }
         indented_output_stream->Append("#define " + iface + "_bambu_artificial_align " + align + "\n");
      }
   }

   for(const auto fid : declared_functions)
   {
      const auto BH = HLSMgr->CGetFunctionBehavior(fid)->CGetBehavioralHelper();
      const auto fname = BH->GetFunctionName();
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Writing external function prototype: " + fname);
      const auto if_param_pos = fname.find(STR_CST_interface_parameter_keyword);
      if(if_param_pos != std::string::npos)
      {
         const auto mgr_name = fname.substr(0, if_param_pos);
         indented_output_stream->Append(
             "#define " + fname + "(rw, bitsize, data, addr) aligned_bambu_artificial_ParmMgr(" + mgr_name +
             "_bambu_artificial_idx, rw, bitsize, data, addr, " + mgr_name + "_bambu_artificial_align)\n\n");
      }
      else if(BH->function_has_to_be_printed(fid))
      {
         DeclareFunctionTypes(TM->GetIRNode(fid));
         WriteFunctionDeclaration(fid);
      }
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Written external function prototype: " + fname);
   }

   for(const auto fid : defined_functions)
   {
      const auto BH = HLSMgr->CGetFunctionBehavior(fid)->CGetBehavioralHelper();
      const auto fname = BH->GetFunctionName();
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Writing function prototype of " + fname);

      if(Param->isOption(OPT_pretty_print))
      {
         if(starts_with(fname, "__builtin_"))
         {
            indented_output_stream->Append("#define " + fname + " _bambu_" + fname + "\n");
         }
      }

      if(BH->function_has_to_be_printed(fid))
      {
         DeclareFunctionTypes(TM->GetIRNode(fid));
         WriteFunctionDeclaration(fid);
      }
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Written function prototype of " + fname);
   }
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Written function prototypes");
}

void CWriter::DeclareFunctionTypes(const ir_nodeConstRef& tn)
{
   const auto FB = HLSMgr->CGetFunctionBehavior(tn->index);
   const auto BH = FB->CGetBehavioralHelper();
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Declaring function types for " + BH->GetFunctionName());

   // In case the function parameters are of a non built_in type I have
   // to declare their type

   for(const auto& parameter_type : BH->GetParameterTypes())
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Parameter type " + STR(parameter_type));
      DeclareType(parameter_type, BH, globally_declared_types);
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Parameter type " + STR(parameter_type));
   }
   const auto return_type = ir_helper::GetFunctionReturnType(tn);
   if(return_type)
   {
      DeclareType(return_type, BH, globally_declared_types);
   }
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Declared function types for " + BH->GetFunctionName());
}

const CustomSet<unsigned int> CWriter::GetLocalVariables(const unsigned int function_id) const
{
   const auto inGraph = HLSMgr->CGetFunctionBehavior(function_id)->GetOpGraph(FunctionBehavior::DFG);
   CustomSet<unsigned int> vars;
   // I simply have to go over all the vertices and get the used variables;
   // the variables which have to be declared are all those variables but
   // the globals ones
   for(const auto& v : inGraph.vertices())
   {
      const auto& vars_temp = inGraph.CGetNodeInfo(v).cited_variables;
      vars.insert(vars_temp.begin(), vars_temp.end());
   }
   return vars;
}

void CWriter::WriteFunctionDeclaration(const unsigned int funId)
{
   const auto FB = HLSMgr->CGetFunctionBehavior(funId);
   const auto BH = FB->CGetBehavioralHelper();
   const auto funName = BH->GetFunctionName();
   if(funName != "main")
   {
      instrWriter->declareFunction(funId);
      indented_output_stream->Append(";\n\n");
   }
}

void CWriter::StartFunctionBody(const unsigned int function_id)
{
   instrWriter->declareFunction(function_id);
   indented_output_stream->Append("\n{\n");

   const auto BH = HLSMgr->CGetFunctionBehavior(function_id)->CGetBehavioralHelper();
   auto vars = GetLocalVariables(function_id);

   for(const auto& funParam : BH->GetParameters())
   {
      vars.erase(funParam->index);
   }

   for(const auto& gblVariable : HLSMgr->GetGlobalVariables())
   {
      vars.erase(gblVariable->index);
   }

   CustomSet<unsigned int> already_declared_variables;
   CustomSet<std::string> locally_declared_types;
   declare_cast_types(function_id, locally_declared_types);
   DeclareLocalVariables(vars, already_declared_variables, locally_declared_types, BH,
                         std::make_unique<std_var_pp_functor>(BH));
}

void CWriter::EndFunctionBody(unsigned int funId)
{
   indented_output_stream->Append("}\n");
   if(verbose)
   {
      indented_output_stream->Append("//end of function; id: " + STR(funId) + "\n");
   }
   indented_output_stream->Append("\n");
   basic_block_prefix.clear();
   basic_block_tail.clear();
   renaming_table.clear();
}

void CWriter::writePreInstructionInfo(const FunctionBehaviorConstRef, const gc_vertex_descriptor)
{
}

void CWriter::writePostInstructionInfo(const FunctionBehaviorConstRef, const gc_vertex_descriptor)
{
}

void CWriter::writeRoutineInstructions_rec(unsigned fid, gc_vertex_descriptor current_vertex, bool bracket,
                                           const std::unique_ptr<var_pp_functor>& variableFunctor)
{
   const auto function_behavior = HLSMgr->CGetFunctionBehavior(fid);
   const auto behavioral_helper = function_behavior->CGetBehavioralHelper();
   const auto cfgGraph = function_behavior->GetOpGraph(FunctionBehavior::FCFG);
   const auto bb_fcfgGraph = function_behavior->GetBBGraph(FunctionBehavior::FBB);
   const auto& bb_graph_info = bb_fcfgGraph.CGetGraphInfo();
   const auto& bb_node_info = bb_fcfgGraph.CGetNodeInfo(current_vertex);
   const unsigned int bb_number = bb_node_info.block->number;

   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Starting writing BB" + STR(bb_number));
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->");
   THROW_ASSERT(bb_frontier.find(current_vertex) == bb_frontier.end(),
                "current_vertex cannot be part of the basic block frontier");
   // if this basic block has already been analyzed do nothing
   if(bb_analyzed.find(current_vertex) != bb_analyzed.end())
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--");
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--BB" + STR(bb_number) + " already written");
      return;
   }
   // mark this basic block as analyzed
   bb_analyzed.insert(current_vertex);
   // print a comment with info on the basicblock
   if(verbose)
   {
      indented_output_stream->Append("//Basic block " + STR(bb_number) + " - loop " + STR(bb_node_info.loop_id) + "\n");
   }
   // check if some extra strings must be printed before or after the basic
   // block. this is used for splitting the phi nodes
   bool add_phi_nodes_assignment_prefix = basic_block_prefix.count(bb_number);
   bool add_phi_nodes_assignment = basic_block_tail.count(bb_number);
   // get immediate post-dominator and check if it has to be examined
   auto bb_PD = function_behavior->post_dominators->getImmediateDominator(current_vertex);
#ifndef NDEBUG
   {
      const auto bb_node_info_pd = bb_fcfgGraph.CGetNodeInfo(bb_PD);
      const auto& bb_number_PD = bb_node_info_pd.block->number;

      std::string frontier_string;
      for(const auto bb : bb_frontier)
      {
         frontier_string += "BB" + STR(bb_fcfgGraph.CGetNodeInfo(bb).block->number) + " ";
      }
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Frontier at the moment is: " + frontier_string);
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Its post-dominator is BB" + STR(bb_number_PD));
   }
#endif
   bool analyze_bb_PD = !bb_frontier.count(bb_PD) && !bb_analyzed.count(bb_PD);
   if(analyze_bb_PD)
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Post dominator will be examinated");
      bb_frontier.insert(bb_PD);
   }
   /// compute the last statement
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Looking for last statement");
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->");
   auto last_stmt = OpGraph::null_vertex();
   bool is_there = false;
   const auto& stmts_list = bb_node_info.statements_list;
   for(const auto st : boost::adaptors::reverse(stmts_list))
   {
      const auto& op_info = cfgGraph.CGetNodeInfo(st);
      if(!local_rec_instructions.count(st))
      {
         continue;
      }
      if(op_info.node_type & TYPE_VPHI)
      {
         continue;
      }
      if((op_info.node_type & TYPE_INIT) != 0)
      {
         continue;
      }
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Considering operation " + op_info.vertex_name);
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                     "This basic block is not empty in this task. Last operation to be printed id " +
                         op_info.vertex_name);
      last_stmt = st;
      is_there = true;
      break;
   }
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--");
   /// check the feasibility
   bool last_statement_is_a_cond_or_goto = is_there and
                                           behavioral_helper->end_with_a_cond_or_goto(bb_node_info.block) != 0 &&
                                           last_stmt == stmts_list.back();

   THROW_ASSERT(!last_statement_is_a_cond_or_goto || !is_there ||
                    (last_statement_is_a_cond_or_goto && last_stmt == stmts_list.back()),
                "inconsistent recursion");
   // check if the label is already in the goto list
   bool add_bb_label = goto_list.find(current_vertex) != goto_list.end();
   if(add_bb_label)
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Basic block should start with a label");
   }

   if(!add_bb_label && bb_fcfgGraph.in_degree(current_vertex) > 1)
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                     "Basic block has an indegree > 1 and not associated label");
      for(const auto& ie : bb_fcfgGraph.in_edges(current_vertex))
      {
         const auto source = bb_fcfgGraph.source(ie);
         // Basic block start the body of a short circuit
         if(!bb_analyzed.count(source) && !((FB_CFG_SELECTOR & bb_fcfgGraph.GetSelector(ie))))
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                           "Basic block should start with a label since is the body of a short-circuit");
            add_bb_label = true;
            break;
         }
         // Basic block is a header loop
         else if(!bb_analyzed.count(source) || current_vertex == source)
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                           "Basic block is the header of a loop and it does not end with while or for");
            add_bb_label = true;
            break;
         }
      }
   }
   add_bb_label = add_bb_label;
   bool add_semicolon = false;
   /// print each instruction
   if(bracket)
   {
      if(analyze_bb_PD || is_there || add_bb_label || add_phi_nodes_assignment || add_phi_nodes_assignment_prefix)
      {
         indented_output_stream->Append("{\n");
      }
      else
      {
         add_semicolon = true;
      }
   }
   if(add_bb_label)
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "A label should be added at the beginning");
      THROW_ASSERT(basic_blocks_labels.count(bb_number), "I do not know the destination: " + STR(bb_number));
      indented_output_stream->Append(basic_blocks_labels.at(bb_number) + ":;\n");
      add_semicolon = true;
   }
   WriteBBHeader(bb_number, fid);

   auto vIter = stmts_list.begin();
   if(is_there)
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                     "There are instructions to be printed for this pair task - basic block");

      // fill the renaming table in case it is needed
      if(renaming_table.find(current_vertex) != renaming_table.end())
      {
         for(const auto& rvt : renaming_table.find(current_vertex)->second)
         {
            BehavioralHelper::rename_a_variable(rvt.first, rvt.second);
         }
      }
      bool prefix_has_to_be_printed = basic_block_prefix.find(bb_number) != basic_block_prefix.end();
      do
      {
         // We can print results of split of phi nodes if they have not yet been printed and if label has already been
         // printed (or there was not any label to be printed)
         if(prefix_has_to_be_printed)
         {
            prefix_has_to_be_printed = false;
            indented_output_stream->Append(basic_block_prefix.find(bb_number)->second);
         }
         if(local_rec_instructions.find(*vIter) == local_rec_instructions.end())
         {
            continue;
         }
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                        "Preparing printing of operation " + cfgGraph.CGetNodeInfo(*vIter).vertex_name);
         // Write in the C file extra information before the instruction itself
         if(verbose)
         {
            indented_output_stream->Append("//Instruction: " + cfgGraph.CGetNodeInfo(*vIter).vertex_name + "\n");
         }
         writePreInstructionInfo(function_behavior, *vIter);

         bool isLastIntruction = last_stmt == *vIter;
         const auto v_type = cfgGraph.CGetNodeInfo(*vIter).node_type;
         /// in case we have phi nodes we check if some assignments should be printed
         bool print_phi_now =
             ((v_type & TYPE_MULTIIF)) || behavioral_helper->end_with_a_cond_or_goto(bb_node_info.block);
         if(add_phi_nodes_assignment && isLastIntruction && print_phi_now)
         {
            indented_output_stream->Append(basic_block_tail.find(bb_number)->second);
         }
         if((v_type & (TYPE_VPHI)) == 0)
         {
            instrWriter->write(function_behavior, *vIter, variableFunctor);
            if((v_type & TYPE_LABEL) == 0)
            {
               add_semicolon = false;
            }
         }
         else if(verbose)
         {
            indented_output_stream->Append("//(removed virtual phi instruction)\n");
         }
         // Write in the C file extra information after the instruction statement
         writePostInstructionInfo(function_behavior, *vIter);
         if(!isLastIntruction)
         {
            continue;
         }
         BehavioralHelper::clear_renaming_table();
         if(add_phi_nodes_assignment && !print_phi_now)
         {
            indented_output_stream->Append(basic_block_tail.find(bb_number)->second);
         }
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---This is not the last statement");
         // Now I check if this is a control statement and I consequently print
         // the instructions contained in its branches
         if(v_type & TYPE_MULTIIF)
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Operation is a multiif");
            unsigned int node_id = cfgGraph.CGetNodeInfo(last_stmt).GetNodeId();
            const ir_nodeRef node = TM->GetIRNode(node_id);
            THROW_ASSERT(node->get_kind() == multi_way_if_stmt_K, "unexpected node");
            auto* gmwi = GetPointer<multi_way_if_stmt>(node);
            std::map<unsigned int, bool> add_elseif_to_goto;
            for(const auto& cond : gmwi->list_of_cond)
            {
               unsigned int bb_index_num = cond.second;
               const auto bb_vertex = bb_graph_info.bb_index_map.find(bb_index_num)->second;
               if(cond != gmwi->list_of_cond.front())
               {
                  bool to_be_added =
                      bb_frontier.find(bb_vertex) == bb_frontier.end() && goto_list.find(bb_vertex) == goto_list.end();
                  add_elseif_to_goto[bb_index_num] = to_be_added;
                  if(to_be_added)
                  {
                     goto_list.insert(bb_vertex);
                  }
               }
               else
               {
                  add_elseif_to_goto[bb_index_num] = false;
               }
            }
            for(const auto& cond : gmwi->list_of_cond)
            {
               unsigned int bb_index_num = cond.second;
               const auto bb_vertex = bb_graph_info.bb_index_map.find(bb_index_num)->second;
               if(cond != gmwi->list_of_cond.front())
               {
                  if(cond.first)
                  {
                     const auto cond_expr = behavioral_helper->PrintVariable(cond.first->index);
                     indented_output_stream->Append("else if(");
                     indented_output_stream->Append(ir_helper::IsBooleanType(cond.first) ? "((" + cond_expr + ") & 1)" :
                                                                                           cond_expr);
                     indented_output_stream->Append(")\n");
                  }
                  else
                  {
                     indented_output_stream->Append("else\n");
                  }
               }
               if(add_elseif_to_goto.find(bb_index_num) != add_elseif_to_goto.end() &&
                  add_elseif_to_goto.find(bb_index_num)->second)
               {
                  goto_list.erase(bb_vertex);
               }
               if(bb_frontier.find(bb_vertex) == bb_frontier.end())
               {
                  if(bb_analyzed.find(bb_vertex) == bb_analyzed.end())
                  {
                     writeRoutineInstructions_rec(fid, bb_vertex, true, variableFunctor);
                  }
                  else
                  {
                     INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Successor has already been examined");

                     THROW_ASSERT(basic_blocks_labels.find(bb_index_num) != basic_blocks_labels.end(),
                                  "I do not know the destination " + STR(bb_index_num));
                     indented_output_stream->Append("   goto " + basic_blocks_labels.find(bb_index_num)->second +
                                                    ";\n");
                     goto_list.insert(bb_vertex);
                  }
               }
               else
               {
                  indented_output_stream->Append("{}\n");
               }
            }
         }
         else if(behavioral_helper->end_with_a_cond_or_goto(bb_node_info.block))
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Basic block ends with a cond or a goto");
            if(last_statement_is_a_cond_or_goto)
            {
               /// now we can analyze the following basic blocks
               for(const auto& oE : bb_fcfgGraph.out_edges(current_vertex))
               {
                  auto next_bb = bb_fcfgGraph.target(oE);
                  if(bb_frontier.find(next_bb) != bb_frontier.end())
                  {
                     continue;
                  }
                  goto_list.insert(next_bb);
               }
            }
         }
         else
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---It is not a special operation");
            const auto bbentry = bb_fcfgGraph.CGetGraphInfo().entry_vertex;
            if(current_vertex == bbentry)
            {
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--");
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Ended writing basic block " + STR(bb_number));
               return;
            }
            THROW_ASSERT(bb_fcfgGraph.out_degree(current_vertex) <= 1, "only one edge expected");
            for(const auto& oE : bb_fcfgGraph.out_edges(current_vertex))
            {
               auto next_bb = bb_fcfgGraph.target(oE);
               if(bb_frontier.find(next_bb) != bb_frontier.end())
               {
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                 "Not adding goto since target is in the frontier");
                  continue;
               }
               if(bb_fcfgGraph.in_degree(next_bb) == 1)
               {
                  writeRoutineInstructions_rec(fid, next_bb, false, variableFunctor);
               }
               else
               {
                  const auto& next_bb_node_info = bb_fcfgGraph.CGetNodeInfo(next_bb);
                  const unsigned int next_bb_number = next_bb_node_info.block->number;
                  THROW_ASSERT(basic_blocks_labels.find(next_bb_number) != basic_blocks_labels.end(),
                               "I do not know the destination");
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
      if(!behavioral_helper->end_with_a_cond_or_goto(bb_node_info.block))
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Not end with a cond or goto");
         const auto bbentry = bb_fcfgGraph.CGetGraphInfo().entry_vertex;
         if(current_vertex == bbentry)
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--");
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Ended writing basic block " + STR(bb_number));
            return;
         }
         THROW_ASSERT(bb_fcfgGraph.out_degree(current_vertex) <= 1,
                      "only one edge expected BB(" + STR(bb_number) + ") Fun(" + STR(fid) + ")");
         for(const auto& oE : bb_fcfgGraph.out_edges(current_vertex))
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Examining the only? successor");
            auto next_bb = bb_fcfgGraph.target(oE);
            if(bb_frontier.find(next_bb) != bb_frontier.end() or bb_fcfgGraph.in_degree(next_bb) == 1)
            {
               continue;
            }

            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Successor does not belong to frontier");
            const auto& next_bb_node_info = bb_fcfgGraph.CGetNodeInfo(next_bb);
            const auto next_bb_number = next_bb_node_info.block->number;
            THROW_ASSERT(basic_blocks_labels.find(next_bb_number) != basic_blocks_labels.end(),
                         "I do not know the destination");
            indented_output_stream->Append("   goto " + basic_blocks_labels.find(next_bb_number)->second + ";\n");
            goto_list.insert(next_bb);
            add_semicolon = false;
         }
      }
   }
   if(add_semicolon)
   {
      indented_output_stream->Append(";\n"); /// added a fake indent
   }

   if(analyze_bb_PD)
   {
      // recurse on the post dominator
      bb_frontier.erase(bb_PD);
      THROW_ASSERT(bb_analyzed.find(bb_PD) == bb_analyzed.end(),
                   "something wrong happened " + STR(bb_fcfgGraph.CGetNodeInfo(bb_PD).block->number) + " Fun(" +
                       STR(fid) + ")");
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Printing the post dominator");
      writeRoutineInstructions_rec(fid, bb_PD, false, variableFunctor);
   }
   if((analyze_bb_PD || is_there || add_bb_label || add_phi_nodes_assignment || add_phi_nodes_assignment_prefix) &&
      bracket)
   {
      indented_output_stream->Append("}\n");
   }
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--");
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Ended writing basic block " + STR(bb_number));
}

void CWriter::compute_phi_nodes(const FunctionBehaviorConstRef function_behavior, const OpVertexSet& instructions,
                                const std::unique_ptr<var_pp_functor>& variableFunctor)
{
   /// compute the assignment introduced by the phi nodes destruction
   const auto bb_domGraph = function_behavior->GetBBGraph(FunctionBehavior::DOM_TREE);
   const auto bb_fcfgGraph = function_behavior->GetBBGraph(FunctionBehavior::FBB);
   const auto cfgGraph = function_behavior->GetOpGraph(FunctionBehavior::FCFG);
   CustomSet<unsigned int> phi_instructions;
   for(const auto instruction : instructions)
   {
      if(cfgGraph.CGetNodeInfo(instruction).node_type & TYPE_PHI)
      {
         phi_instructions.insert(cfgGraph.CGetNodeInfo(instruction).GetNodeId());
      }
   }
   if(!phi_instructions.empty())
   {
      std::map<unsigned int, unsigned int> created_variables;
      std::map<unsigned int, std::string> symbol_table;
      std::map<unsigned int, std::deque<std::string>> array_of_stacks;
      insert_copies(bb_domGraph.CGetGraphInfo().entry_vertex, bb_domGraph, bb_fcfgGraph, variableFunctor,
                    phi_instructions, created_variables, symbol_table, array_of_stacks);
      /// in case we declare the variables introduced during the phi nodes destruction
      if(!created_variables.empty())
      {
         auto cv_it_end = created_variables.end();
         for(auto cv_it = created_variables.begin(); cv_it != cv_it_end; ++cv_it)
         {
            THROW_ASSERT(symbol_table.find(cv_it->first) != symbol_table.end(), "variable not found in symbol_table");
            unsigned real_var = cv_it->second;
            const std::unique_ptr<var_pp_functor> phi_functor = std::make_unique<isolated_var_pp_functor>(
                function_behavior->CGetBehavioralHelper(), real_var, symbol_table[cv_it->first]);
            auto real_var_node = TM->GetIRNode(real_var);
            indented_output_stream->Append(
                ir_helper::PrintType(ir_helper::CGetType(real_var_node), false, false, real_var_node, phi_functor));
            indented_output_stream->Append(";\n");
         }
      }
   }
}

void CWriter::writeRoutineInstructions(const unsigned int fid, const OpVertexSet& instructions,
                                       const std::unique_ptr<var_pp_functor>& variableFunctor,
                                       gc_vertex_descriptor bb_start, CustomOrderedSet<gc_vertex_descriptor> bb_end)
{
   bb_label_counter++;
   const auto function_behavior = HLSMgr->CGetFunctionBehavior(fid);
   const auto BH = function_behavior->CGetBehavioralHelper();
   const auto cfgGraph = function_behavior->GetOpGraph(FunctionBehavior::FCFG);
   INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "-->CWriter::writeRoutineInstructions - Start");
   if(instructions.empty())
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level,
                     "<--CWriter::writeRoutineInstructions - instructions is an empty set");
      return;
   }
   else if(instructions.size() == 1)
   {
      if(cfgGraph.CGetNodeInfo((*instructions.begin())).node_type & TYPE_ENTRY)
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level,
                        "<--CWriter::writeRoutineInstructions - instructions is a set with only entry");
         return;
      }
   }
   const auto bb_fcfgGraph = function_behavior->GetBBGraph(FunctionBehavior::FBB);
   /// Then I compute all the labels associated with a basic block with more than one entering edge.
   basic_blocks_labels.clear();
   BBGraph::vertex_descriptor bbentry;
   CustomOrderedSet<BBGraph::vertex_descriptor> bb_exit;

   if(!bb_start)
   {
      bbentry = bb_fcfgGraph.CGetGraphInfo().entry_vertex;
   }
   else
   {
      bbentry = bb_start;
   }

   if(bb_end.empty())
   {
      bb_exit.insert(bb_fcfgGraph.CGetGraphInfo().exit_vertex);
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "There are " + STR(bb_exit.size()) + " exit basic blocks");
   }
   else
   {
      bb_exit = bb_end;
   }

   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Computing labels");
   for(const auto& v : bb_fcfgGraph.vertices())
   {
      size_t delta = bb_exit.count(v) ? 1u : 0u;
      if(bb_fcfgGraph.in_degree(v) <= (1 + delta))
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                        "---Skipped BB" + STR(bb_fcfgGraph.CGetNodeInfo(v).block->number));
         continue;
      }
      const auto& bb_node_info = bb_fcfgGraph.CGetNodeInfo(v);
      basic_blocks_labels[bb_node_info.block->number] =

          ("BB_LABEL_" + STR(bb_node_info.block->number)) + (bb_label_counter == 1 ? "" : "_" + STR(bb_label_counter));
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                     "---Label of BB" + STR(bb_fcfgGraph.CGetNodeInfo(v).block->number) + " is " +
                         basic_blocks_labels[bb_node_info.block->number]);
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
   local_rec_instructions.clear();
   local_rec_instructions.insert(instructions.begin(), instructions.end());

   /// some statements can be in entry
   writeRoutineInstructions_rec(fid, bbentry, false, variableFunctor);
   if(!bb_start && bb_end.size() == 0)
   {
      for(const auto& oE : bb_fcfgGraph.out_edges(bbentry))
      {
         if(bb_exit.count(bb_fcfgGraph.target(oE)))
         {
            continue;
         }
         else
         {
            writeRoutineInstructions_rec(fid, bb_fcfgGraph.target(oE), false, variableFunctor);
         }
      }
   }
   CustomOrderedSet<gc_vertex_descriptor> not_yet_considered;
   std::set_difference(goto_list.begin(), goto_list.end(),     /*first set*/
                       bb_analyzed.begin(), bb_analyzed.end(), /*second set*/
                       std::inserter(not_yet_considered, not_yet_considered.begin()) /*result*/);
   while(!not_yet_considered.empty())
   {
      auto next_bb = *not_yet_considered.begin();
      not_yet_considered.erase(next_bb);
      writeRoutineInstructions_rec(fid, next_bb, false, variableFunctor);
      not_yet_considered.clear();
      std::set_difference(goto_list.begin(), goto_list.end(),     /*first set*/
                          bb_analyzed.begin(), bb_analyzed.end(), /*second set*/
                          std::inserter(not_yet_considered, not_yet_considered.begin()) /*result*/);
   }
   const auto& exit = bb_fcfgGraph.CGetGraphInfo().exit_vertex;
   if(goto_list.count(exit) && basic_blocks_labels.find(bloc::EXIT_BLOCK_ID) != basic_blocks_labels.end())
   {
      indented_output_stream->Append(basic_blocks_labels.find(bloc::EXIT_BLOCK_ID)->second + ":\n");
   }
   INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "<--CWriter::writeRoutineInstructions - End");
}

void CWriter::DeclareType(const ir_nodeConstRef& varType, const BehavioralHelperConstRef& BH,
                          CustomSet<std::string>& locally_declared_types)
{
#ifndef NDEBUG
   const auto routine_name = BH->GetFunctionName();
#endif

   const auto without_transformation = Param->getOption<bool>(OPT_without_transformation);
   const auto real_var_type = (varType);
   const auto type_name = ir_helper::PrintType(real_var_type);

   // Check that the variable really needs the declaration of a new type
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                  "-->Declaration of type " + type_name + " (" + varType->ToString() + " - " +
                      real_var_type->ToString() + ") in function " + routine_name);

   if(!globally_declared_types.count(type_name) && !locally_declared_types.count(type_name))
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---This type has not been declared in this function");
      locally_declared_types.insert(type_name);
      bool is_system;
      const auto decl = std::get<0>(ir_helper::GetSourcePath(varType, is_system));
      if(!decl.empty() && decl != "<built-in>" && is_system && !ir_helper::IsInLibbambu(varType))
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                        "<--Type has not to be declared since it is declared in included " + decl);
         return;
      }
      const auto types_to_be_declared_before =
          ir_helper::GetTypesToBeDeclaredBefore(real_var_type, without_transformation);
      for(const auto& type_to_be_declared : types_to_be_declared_before)
      {
         DeclareType(type_to_be_declared, BH, locally_declared_types);
      }
      if(ir_helper::HasToBeDeclared(real_var_type))
      {
         if(verbose)
         {
            indented_output_stream->Append("//declaration of type " + STR(varType) + "(" + STR(real_var_type) + ")\n");
         }
         indented_output_stream->Append(BH->print_type_declaration(real_var_type->index) + ";\n");
      }
      const auto types_to_be_declared_after =
          ir_helper::GetTypesToBeDeclaredAfter(real_var_type, without_transformation);
      for(const auto& type_to_be_declared : types_to_be_declared_after)
      {
         DeclareType(type_to_be_declared, BH, locally_declared_types);
      }
   }
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Declared type " + varType->ToString());
}

void CWriter::DeclareVariable(const ir_nodeConstRef& curVar, CustomSet<unsigned int>& already_declared_variables,
                              CustomSet<std::string>& locally_declared_types, const BehavioralHelperConstRef& BH,
                              const std::unique_ptr<var_pp_functor>& varFunc)
{
   if(already_declared_variables.count(curVar->index))
   {
      return;
   }
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Declaring variable " + STR(curVar));
   already_declared_variables.insert(curVar->index);

   CustomUnorderedSet<unsigned int> initVars;
   if(BH->GetInit(curVar->index, initVars))
   {
      for(const auto initVar : initVars)
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level,
                        "For variable " + STR(curVar) + " recursing on " + STR(initVar));
         if(!already_declared_variables.count(initVar) && !globallyDeclVars.count(initVar))
         {
            DeclareVariable(TM->GetIRNode(initVar), already_declared_variables, locally_declared_types, BH, varFunc);
         }
      }
   }
   const auto variable_type = ir_helper::CGetType(curVar);
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Type is " + STR(variable_type));
   DeclareType(variable_type, BH, locally_declared_types);
   if(!ir_helper::IsSystemType(curVar) || ir_helper::IsInLibbambu(curVar))
   {
      if(verbose)
      {
         indented_output_stream->Append("//declaring variable " + STR(curVar) + " - type: " + STR(variable_type) +
                                        "\n");
      }
      if(GetPointer<const function_val_node>(curVar))
      {
         instrWriter->declareFunction(curVar->index);
         indented_output_stream->Append(";\n");
      }
      else
      {
         indented_output_stream->Append(BH->PrintVarDeclaration(curVar->index, varFunc, true) + ";\n");
      }
   }
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Declared variable " + STR(curVar));
}

void CWriter::writeInclude(const std::string& file_name)
{
   if(file_name.find(".h") == std::string::npos || writtenIncludes.find(file_name) != writtenIncludes.end())
   {
      return;
   }
   writtenIncludes.insert(file_name);
   indented_output_stream->Append("#include \"" + file_name + "\"\n");
}

void CWriter::DeclareLocalVariables(const CustomSet<unsigned int>& to_be_declared,
                                    CustomSet<unsigned int>& already_declared_variables,
                                    CustomSet<std::string>& already_declared_types, const BehavioralHelperConstRef BH,
                                    const std::unique_ptr<var_pp_functor>& varFunc)
{
   const auto p = BH->get_parameters();
   const auto IRMan = TM;
   const auto is_to_declare = [&p, &IRMan](unsigned int obj) -> bool {
      if(std::find(p.cbegin(), p.cend(), obj) != p.cend())
      {
         return false;
      }
      const ir_nodeRef node = IRMan->GetIRNode(obj);
      if(node->get_kind() == argument_val_node_K)
      {
         return false;
      }
      auto* sa = GetPointer<ssa_node>(node);
      if(sa && ((sa->GetDefStmt()->get_kind() == nop_stmt_K)) && sa->var &&
         (sa->var->get_kind() == argument_val_node_K))
      {
         return false;
      }
      return true;
   };

   const auto fid = BH->get_function_index();
   INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "-->Declaring " + STR(to_be_declared.size()) + " local variables");
   for(const auto var : to_be_declared)
   {
      if(is_to_declare(var))
      {
         DeclareVariable(TM->GetIRNode(var), already_declared_variables, already_declared_types, BH, varFunc);
      }
   }

   const auto function_behavior = HLSMgr->CGetFunctionBehavior(fid);
   const auto data = function_behavior->GetOpGraph(FunctionBehavior::DFG);
   OpVertexSet vertices(&function_behavior->GetOpGraphsCollection());
   const auto [vit, vit_end] = boost::vertices(data);
   vertices.insert(vit, vit_end);
   THROW_ASSERT(vertices.size() > 0, "Graph for function " + BH->GetFunctionName() + " is empty");
   compute_phi_nodes(function_behavior, vertices, std::make_unique<std_var_pp_functor>(BH));
   INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "<--Declaring local variables");
}

void CWriter::insert_copies(gc_vertex_descriptor b, const BBGraph& bb_domGraph, const BBGraph& bb_fcfgGraph,
                            const std::unique_ptr<var_pp_functor>& variableFunctor,
                            const CustomSet<unsigned int>& phi_instructions,
                            std::map<unsigned int, unsigned int>& created_variables,
                            std::map<unsigned int, std::string>& symbol_table,
                            std::map<unsigned int, std::deque<std::string>>& array_of_stacks)
{
   std::list<unsigned int> pushed;
   /// fill the renaming table for basic block b
   for(const auto& aos : array_of_stacks)
   {
      if(!aos.second.empty())
      {
         renaming_table[b][aos.first] = aos.second.back();
         symbol_table[aos.first] = aos.second.back();
      }
   }
   schedule_copies(b, bb_domGraph, bb_fcfgGraph, variableFunctor, phi_instructions, created_variables, symbol_table,
                   pushed, array_of_stacks);
   for(const auto& oi : bb_domGraph.out_edges(b))
   {
      auto c = bb_domGraph.target(oi);
      insert_copies(c, bb_domGraph, bb_fcfgGraph, variableFunctor, phi_instructions, created_variables, symbol_table,
                    array_of_stacks);
   }
   pop_stack(pushed, array_of_stacks);
}

void CWriter::schedule_copies(gc_vertex_descriptor b, const BBGraph& bb_domGraph, const BBGraph& bb_fcfgGraph,
                              const std::unique_ptr<var_pp_functor>& variableFunctor,
                              const CustomSet<unsigned int>& phi_instructions,
                              std::map<unsigned int, unsigned int>& created_variables,
                              std::map<unsigned int, std::string>& symbol_table, std::list<unsigned int>& pushed,
                              std::map<unsigned int, std::deque<std::string>>& array_of_stacks)
{
   /// Pass One: initialize the data structures
   const auto& bb_node_info = bb_fcfgGraph.CGetNodeInfo(b);
   unsigned int bi_id = bb_node_info.block->number;

   IRNodesPairSet copy_set, worklist;
   std::map<unsigned int, unsigned int> map;
   CustomOrderedSet<unsigned int> used_by_another;
   std::map<unsigned int, unsigned int> bb_dest_definition;
   for(const auto& oi : bb_fcfgGraph.out_edges(b))
   {
      auto s = bb_fcfgGraph.target(oi);
      const auto& si = bb_fcfgGraph.CGetNodeInfo(s);
      for(const auto& phi_op : si.block->CGetPhiList())
      {
         if(phi_instructions.find(phi_op->index) == phi_instructions.end())
         {
            continue;
         }
         auto* pn = GetPointer<phi_stmt>(phi_op);
         ir_nodeRef dest = pn->res;
         unsigned int dest_i = pn->res->index;
         bool is_virtual = pn->virtual_flag;
         if(!is_virtual)
         {
            bb_dest_definition[dest_i] = si.block->number;
            for(const auto& [src, edge_id] : pn->CGetDefEdgesList())
            {
               if(edge_id == bi_id)
               {
                  unsigned int src_i = src->index;
                  copy_set.insert(std::pair<ir_nodeRef, ir_nodeRef>(src, dest));
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
      if(used_by_another.find(cs_it->second->index) == used_by_another.end())
      {
         worklist.insert(*cs_it);
         cs_it = copy_set.erase(cs_it);
      }
      else
      {
         ++cs_it;
      }
   }

   /// Pass Three: Iterate over the worklist, inserting copies
   while(!worklist.empty() || !copy_set.empty())
   {
      IRNodesPairSet worklist_restart;
      do
      {
         for(auto& [src, dest] : worklist)
         {
            unsigned int src_i = src->index;
            unsigned int dest_i = dest->index;
            /// if dest belongs to live_out(b)
            /// wrt the original algorithm an optimization has been added: in case b does not dominate any other node we
            /// can skip the creation of t
            //            if(bb_domGraph.out_degree(b) > 0 &&
            //                  bb_node_info.block->live_out.find(dest_i) != bb_node_info.block->live_out.end()
            //                  )
            bool add_copy = false;
            for(const auto& oe : bb_domGraph.out_edges(b))
            {
               auto tgt_bb = bb_domGraph.target(oe);
               if(tgt_bb == b)
               {
                  continue;
               }
               const auto& tgt_bi = bb_domGraph.CGetNodeInfo(tgt_bb);
               if(tgt_bi.block->live_in.find(dest_i) != tgt_bi.block->live_in.end())
               {
                  add_copy = true;
               }
            }
            if(add_copy)
            {
               // THROW_ERROR("check the source code @" + STR(dest_i));
               ///   insert a copy from dest to a new temp t at phi-node defining dest
               unsigned int t_i = create_new_identifier(symbol_table);
               basic_block_prefix[bb_dest_definition[dest_i]] +=
                   symbol_table.find(t_i)->second + " = " + (*variableFunctor)(dest_i) + ";\n";
               created_variables[t_i] = dest_i;
               map[t_i] = t_i;
               ///   push(t, Stack[dest])
               push_stack(symbol_table.find(t_i)->second, dest_i, pushed, array_of_stacks);
               renaming_table[b][dest_i] = symbol_table.find(t_i)->second;
            }
            /// insert a copy operation from map[src] to dest at the end of b
            std::string copy_statement;
            if(symbol_table.find(map.find(src_i)->second) != symbol_table.end())
            {
               copy_statement +=
                   (*variableFunctor)(dest_i) + " = " + symbol_table.find(map.find(src_i)->second)->second + ";\n";
            }
            else if(dest_i != map.find(src_i)->second)
            {
               copy_statement +=
                   (*variableFunctor)(dest_i) + " = " + (*variableFunctor)(map.find(src_i)->second) + ";\n";
            }

            basic_block_tail[bi_id] += copy_statement;
            // map[src_i] = dest_i;

            for(auto cs_it = copy_set.begin(); cs_it != copy_set.end();)
            {
               if(src == cs_it->second)
               {
                  worklist_restart.insert(*cs_it);
                  cs_it = copy_set.erase(cs_it);
               }
               else
               {
                  ++cs_it;
               }
            }
         }
         worklist = worklist_restart;
         worklist_restart.clear();
      } while(!worklist.empty());

      if(copy_set.size())
      {
         const auto begin_it = copy_set.begin();
         unsigned int dest_i = begin_it->second->index;
         /// check if dest_i is source of any other pair in copy_set
         /// this optimization is not described in the original algorithm
         for(auto cs_it = std::next(begin_it); cs_it != copy_set.end(); ++cs_it)
         {
            if(cs_it->first->index == dest_i)
            {
               /// create a new symbol
               unsigned int t_i = create_new_identifier(symbol_table);
               /// insert a copy from dest to a new temp t at the end of b
               basic_block_tail[bi_id] += symbol_table.at(t_i) + " = " + (*variableFunctor)(dest_i) + ";\n";
               created_variables[t_i] = dest_i;
               map[t_i] = t_i;
               map[dest_i] = t_i;
               break;
            }
         }
         worklist.insert(*begin_it);
         copy_set.erase(begin_it);
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
      new_name = "__t__" + STR(fake_max_ir_node_id) + "_" + STR(counter);
      node_id_this = TM->find_identifier_nodeID(new_name);
      counter++;
   } while(node_id_this > 0);
   symbol_table[fake_max_ir_node_id] = new_name;
   node_id_this = fake_max_ir_node_id;
   ++fake_max_ir_node_id;
   return node_id_this;
}

void CWriter::push_stack(std::string symbol_name, unsigned int dest_i, std::list<unsigned int>& pushed,
                         std::map<unsigned int, std::deque<std::string>>& array_of_stacks)
{
   THROW_ASSERT(std::find(pushed.begin(), pushed.end(), dest_i) == pushed.end(), "multiple push is not allowed");
   array_of_stacks[dest_i].push_back(symbol_name);
   pushed.push_back(dest_i);
}

void CWriter::pop_stack(std::list<unsigned int>& pushed,
                        std::map<unsigned int, std::deque<std::string>>& array_of_stacks)
{
   for(auto var_id : pushed)
   {
      THROW_ASSERT(array_of_stacks.find(var_id) != array_of_stacks.end(), "the array of stacks is inconsistent");
      THROW_ASSERT(!array_of_stacks.find(var_id)->second.empty(), "the variable is not mapped");
      array_of_stacks.find(var_id)->second.pop_back();
   }
   pushed.clear();
}

void CWriter::InternalWriteFile()
{
}

void CWriter::WriteFile(const std::string& file_name)
{
   Initialize();
   WriteHeader();
   WriteGlobalDeclarations();
   WriteImplementations();
   InternalWriteFile();
   indented_output_stream->WriteFile(file_name);
}

void CWriter::WriteBBHeader(const unsigned int, const unsigned int)
{
}

void CWriter::WriteImplementations()
{
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Writing implementations");
   // First of all I declare the functions and then the tasks
   for(const auto fid : defined_functions)
   {
      const auto BH = HLSMgr->CGetFunctionBehavior(fid)->CGetBehavioralHelper();
      if(BH->function_has_to_be_printed(fid))
      {
         WriteFunctionImplementation(fid);
      }
   }
   if(HLSMgr->CGetCallGraphManager().ExistsAddressedFunction())
   {
      WriteBuiltinWaitCall();
   }
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Written implementations");
}

void CWriter::WriteBuiltinWaitCall()
{
   indented_output_stream->Append("void " + STR(BUILTIN_WAIT_CALL) + "(void * ptr, ...)\n");
   indented_output_stream->Append("{\n");
   indented_output_stream->Append("va_list ap;\n");
   indented_output_stream->Append("va_start(ap, ptr);\n");
   indented_output_stream->Append("int boolReturn = va_arg(ap, int);\n");
   for(const unsigned int id : HLSMgr->CGetCallGraphManager().GetAddressedFunctions())
   {
      const auto BH = HLSMgr->CGetFunctionBehavior(id)->CGetBehavioralHelper();
      indented_output_stream->Append("if(ptr == " + BH->GetFunctionName() + ")\n");
      indented_output_stream->Append("{\n");
      std::vector<std::pair<std::string, std::string>> typeAndName;
      for(const auto& I : BH->GetParameters())
      {
         const auto type_node = ir_helper::CGetType(I);
         const auto type_size = ir_helper::SizeAlloc(type_node);
         auto type = ir_helper::PrintType(type_node);
         if(ir_helper::IsSignedIntegerType(type_node))
         {
            if(type_size < 32)
            {
               type = "int";
            }
         }
         else if(ir_helper::IsUnsignedIntegerType(type_node))
         {
            if(type_size < 32)
            {
               type = "unsigned int";
            }
         }
         else if(ir_helper::IsRealType(type_node))
         {
            if(type_size < 64)
            {
               type = "double";
            }
         }
         const auto name = BH->PrintVariable(I->index);
         typeAndName.push_back(std::make_pair(type, name));
      }
      for(const auto& I : typeAndName)
      {
         indented_output_stream->Append(I.first + " " + I.second + " = va_arg(ap, " + I.first + ");\n");
      }
      const auto returnType_node = ir_helper::GetFunctionReturnType(TM->GetIRNode(id));
      std::string returnType;
      if(returnType_node)
      {
         const auto returnType_size = ir_helper::SizeAlloc(returnType_node);
         returnType = ir_helper::PrintType(returnType_node);
         if(ir_helper::IsSignedIntegerType(returnType_node))
         {
            if(returnType_size < 32)
            {
               returnType = "int";
            }
         }
         else if(ir_helper::IsUnsignedIntegerType(returnType_node))
         {
            if(returnType_size < 32)
            {
               returnType = "unsigned int";
            }
         }
         else if(ir_helper::IsRealType(returnType_node))
         {
            if(returnType_size < 64)
            {
               returnType = "double";
            }
         }
      }
      else
      {
         returnType = "void";
      }
      if(returnType != "void")
      {
         indented_output_stream->Append(returnType + " res = ");
      }
      indented_output_stream->Append(BH->GetFunctionName() + "(");
      const auto typeAndNameSize = typeAndName.size();
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
         indented_output_stream->Append("if(boolReturn) {\n");
         indented_output_stream->Append(returnType + "* " + "resultAddress = va_arg(ap, " + returnType + " *);\n");
         indented_output_stream->Append("*resultAddress = res;\n");
         indented_output_stream->Append("}\n");
      }
      indented_output_stream->Append("}\n");
   }
   indented_output_stream->Append("va_end(ap);\n");
   indented_output_stream->Append("}\n\n");
}
