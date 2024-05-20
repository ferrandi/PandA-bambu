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
 * @file hls_c_writer.cpp
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 * @author Marco Minutoli <mminutoli@gmail.com>
 * @author Pietro Fezzardi <pietro.fezzardi@polimi.it>
 * @author Marco Lattuada <marco.lattuada@polimi.it>
 * @author Michele Fiorito <michele.fiorito@polimi.it>
 *
 */
#include "hls_c_writer.hpp"

#include "Parameter.hpp"
#include "SimulationInformation.hpp"
#include "behavioral_helper.hpp"
#include "c_initialization_parser.hpp"
#include "call_graph_manager.hpp"
#include "custom_map.hpp"
#include "custom_set.hpp"
#include "dbgPrintHelper.hpp" // for DEBUG_LEVEL_NONE
#include "function_behavior.hpp"
#include "hls_manager.hpp"
#include "indented_output_stream.hpp"
#include "instruction_writer.hpp"
#include "math_function.hpp"
#include "memory.hpp"
#include "memory_initialization_c_writer.hpp"
#include "string_manipulation.hpp" // for GET_CLASS
#include "structural_objects.hpp"
#include "technology_node.hpp"
#include "testbench_generation.hpp"
#include "testbench_generation_constants.hpp"
#include "tree_helper.hpp"
#include "tree_manager.hpp"
#include "tree_node.hpp"
#include "var_pp_functor.hpp"

#include <boost/algorithm/string/trim_all.hpp>
#include <regex>

#include <list>
#include <string>
#include <vector>

REF_FORWARD_DECL(memory_symbol);

static const std::regex wrapper_def("(ac_channel|stream|hls::stream)<(.*)>");
#define WRAPPER_GROUP_WTYPE 2

HLSCWriter::HLSCWriter(const CBackendInformationConstRef _c_backend_info, const HLS_managerConstRef _HLSMgr,
                       const InstructionWriterRef _instruction_writer,
                       const IndentedOutputStreamRef _indented_output_stream)
    : CWriter(_HLSMgr, _instruction_writer, _indented_output_stream), c_backend_info(_c_backend_info)
{
   debug_level = Param->get_class_debug_level(GET_CLASS(*this));
}

HLSCWriter::~HLSCWriter() = default;

void HLSCWriter::InternalInitialize()
{
}

void HLSCWriter::InternalWriteHeader()
{
   indented_output_stream->Append(R"(
#define _FILE_OFFSET_BITS 64

#define __Inf (1.0 / 0.0)
#define __Nan (0.0 / 0.0)

#ifdef __cplusplus
#undef printf

#include <cstdio>
#include <cstdlib>

typedef bool _Bool;
#else
#include <stdio.h>
#include <stdlib.h>

extern void exit(int status);
#endif

#include <sys/types.h>

#ifdef __AC_NAMESPACE
using namespace __AC_NAMESPACE;
#endif

)");

   // get the root function to be tested by the testbench
   const auto top_symbols = Param->getOption<std::vector<std::string>>(OPT_top_functions_names);
   THROW_ASSERT(top_symbols.size() == 1, "Expected single top function name");
   const auto top_fnode = TM->GetFunction(top_symbols.front());
   const auto fd = GetPointerS<const function_decl>(top_fnode);
   const auto top_fname = tree_helper::GetMangledFunctionName(fd);
   const auto& parms = HLSMgr->module_arch->GetArchitecture(top_fname)->parms;

   CustomOrderedSet<std::filesystem::path> includes;
   for(const auto& [parm, attrs] : parms)
   {
      const auto attr_it = attrs.find(FunctionArchitecture::parm_includes);
      if(attr_it != attrs.end())
      {
         string_to_container(std::inserter(includes, includes.end()), attr_it->second, ";");
      }
   }
   if(includes.size())
   {
      indented_output_stream->Append("#define " + top_fname + " __keep_your_declaration_out_of_my_code\n");
      indented_output_stream->Append("#define main __keep_your_main_out_of_my_code\n");

      const auto output_directory = Param->getOption<std::filesystem::path>(OPT_output_directory) / "simulation";
      for(const auto& inc : includes)
      {
         if(inc.is_absolute())
         {
            indented_output_stream->Append("#include \"" + inc.string() + "\"\n");
         }
         else
         {
            indented_output_stream->Append("#include \"" + inc.lexically_proximate(output_directory).string() + "\"\n");
         }
      }
      indented_output_stream->Append("#undef " + top_fname + "\n");
      indented_output_stream->Append("#undef main\n");
   }
   indented_output_stream->Append(R"(

#ifndef CDECL
#ifdef __cplusplus
#define CDECL extern "C"
#else
#define CDECL
#endif
#endif

#ifndef EXTERN_CDECL
#ifdef __cplusplus
#define EXTERN_CDECL extern "C"
#else
#define EXTERN_CDECL extern
#endif
#endif

)");
}

void HLSCWriter::InternalWriteGlobalDeclarations()
{
   instrWriter->write_declarations();
}

void HLSCWriter::WriteParamDecl(const BehavioralHelperConstRef BH)
{
   indented_output_stream->Append("// parameters declaration\n");

   for(const auto& par : BH->GetParameters())
   {
      const auto parm_type = tree_helper::CGetType(par);
      const auto type = tree_helper::IsPointerType(par) ? "void*" : tree_helper::PrintType(TM, parm_type);
      const auto param = BH->PrintVariable(par->index);

      if(tree_helper::IsVectorType(parm_type))
      {
         THROW_ERROR("parameter " + param + " of function under test " + BH->get_function_name() + " has type " + type +
                     "\nco-simulation does not support vectorized parameters at top level");
      }
      indented_output_stream->Append(type + " " + param + ";\n");
   }
}

void HLSCWriter::WriteParamInitialization(const BehavioralHelperConstRef BH,
                                          const std::map<std::string, std::string>& curr_test_vector)
{
   const auto fname = BH->GetMangledFunctionName();
   const auto& parm_attrs = HLSMgr->module_arch->GetArchitecture(fname)->parms;
   const auto params = BH->GetParameters();
   for(auto par_idx = 0U; par_idx < params.size(); ++par_idx)
   {
      const auto& par = params.at(par_idx);
      const auto parm_type = tree_helper::CGetType(par);
      const auto param = BH->PrintVariable(par->index);
      const auto has_file_init = [&]() {
         if(parm_attrs.size()) // FIX: this is probably always true
         {
            const auto& type_name = parm_attrs.at(param).at(FunctionArchitecture::parm_typename);
            return std::regex_search(type_name.c_str(), wrapper_def);
         }
         return false;
      }();
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Writing initialization of " + param);
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                     "---Type: " + parm_type->get_kind_text() + " - " + STR(parm_type));
      const auto init_it = curr_test_vector.find(param);
      if(init_it == curr_test_vector.end())
      {
         THROW_ERROR("Value of " + param + " is missing in test vector");
      }
      const auto test_v =
          (has_file_init && ends_with(init_it->second, ".dat")) ? ("\"" + init_it->second + "\"") : init_it->second;
      if(tree_helper::IsPointerType(parm_type))
      {
         const auto is_binary_init = ends_with(test_v, ".dat");

         std::string var_ptdtype;
         std::string temp_var_decl;
         bool is_a_true_pointer = true;
         if(!is_binary_init && parm_attrs.size())
         {
            var_ptdtype = parm_attrs.at(param).at(FunctionArchitecture::parm_original_typename);
            static const std::regex voidP = std::regex(R"(\bvoid\b)");
            while(std::regex_search(var_ptdtype, voidP))
            {
               var_ptdtype = std::regex_replace(var_ptdtype, voidP, "char");
            }
            is_a_true_pointer = var_ptdtype.back() == '*';
            if(is_a_true_pointer || var_ptdtype.back() == '&')
            {
               var_ptdtype.pop_back();
            }
            if(var_ptdtype.find("(*)") != std::string::npos)
            {
               temp_var_decl = var_ptdtype;
               temp_var_decl.replace(var_ptdtype.find("(*)"), 3, param + "_temp[]");
            }
            else
            {
               temp_var_decl = var_ptdtype + " " + param + "_temp" + (is_a_true_pointer ? "[]" : "");
            }
         }
         if(temp_var_decl == "")
         {
            const auto var_functor = var_pp_functorRef(new std_var_pp_functor(BH));
            const auto ptd = tree_helper::CGetPointedType(parm_type);
            temp_var_decl = tree_helper::PrintType(TM, ptd, false, false, false, par, var_functor);
            var_ptdtype = temp_var_decl.substr(0, temp_var_decl.find_last_of((*var_functor)(par->index)));
            if(tree_helper::IsVoidType(ptd))
            {
               boost::replace_all(temp_var_decl, "void ", "char ");
               boost::replace_all(var_ptdtype, "void ", "char ");
            }
            const auto first_square = temp_var_decl.find('[');
            if(first_square == std::string::npos)
            {
               temp_var_decl = temp_var_decl + "_temp[]";
            }
            else
            {
               temp_var_decl.insert(first_square, "_temp[]");
            }
         }
         THROW_ASSERT(temp_var_decl.size() && var_ptdtype.size(),
                      "var_decl: " + temp_var_decl + ", ptd_type: " + var_ptdtype);
         const auto arg_channel = std::regex_search(var_ptdtype, std::regex("(ac_channel|stream|hls::stream)<(.*)>"));
         const auto ptd_type = tree_helper::GetRealType(tree_helper::CGetPointedType(parm_type));
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                        "---Pointed type: " + ptd_type->get_kind_text() + " - " + STR(ptd_type));

         std::string param_size;
         if(is_binary_init)
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Initialized from binary file: " + test_v);
            const auto fp = param + "_fp";
            indented_output_stream->Append("FILE* " + fp + " = fopen(\"" + test_v + "\", \"rb\");\n");
            indented_output_stream->Append("fseek(" + fp + ", 0, SEEK_END);\n");
            indented_output_stream->Append("size_t " + param + "_size = ftell(" + fp + ");\n");
            indented_output_stream->Append("fseek(" + fp + ", 0, SEEK_SET);\n");
            indented_output_stream->Append("void* " + param + "_buf = malloc(" + param + "_size);\n");
            indented_output_stream->Append("if(fread((unsigned char*)" + param + "_buf, 1, " + param + "_size, " + fp +
                                           ") != " + param + "_size)\n");
            indented_output_stream->Append("{\n");
            indented_output_stream->Append("fclose(" + fp + ");\n");
            indented_output_stream->Append("printf(\"Unable to read " + test_v + " to initialize parameter " + param +
                                           "\");\n");
            indented_output_stream->Append("exit(-1);\n");
            indented_output_stream->Append("}\n");
            indented_output_stream->Append("fclose(" + fp + ");\n");
            indented_output_stream->Append(param + " = " + param + "_buf;\n");
            indented_output_stream->Append("m_param_alloc(" + STR(par_idx) + ", " + param + "_size);\n");
         }
         else
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Inline pointer initialization");
            const auto temp_initialization =
                temp_var_decl + " = " +
                ((test_v.front() != '{' && test_v.back() != '}' && is_a_true_pointer) ? "{" + test_v + "}" : test_v) +
                ";\n";
            indented_output_stream->Append(temp_initialization);
            indented_output_stream->Append(param + " = (void*)" + (is_a_true_pointer ? "" : "&") + param + "_temp;\n");
            if(!arg_channel)
            {
               indented_output_stream->Append("m_param_alloc(" + STR(par_idx) + ", sizeof(" + param + "_temp));\n");
            }
         }
      }
      else
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Inline initialization");
         if(tree_helper::IsRealType(parm_type) && test_v == "-0")
         {
            indented_output_stream->Append(param + " = -0.0;\n");
         }
         else
         {
            indented_output_stream->Append(param + " = " + test_v + ";\n");
         }
      }
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Written initialization of " + param);
   }
}

void HLSCWriter::WriteTestbenchFunctionCall(const BehavioralHelperConstRef BH)
{
   const auto function_index = BH->get_function_index();
   const auto return_type_index = BH->GetFunctionReturnType(function_index);

   const auto top_fname_mngl = BH->GetMangledFunctionName();
   const auto function_name = [&]() -> std::string {
      const auto is_discrepancy = (Param->isOption(OPT_discrepancy) && Param->getOption<bool>(OPT_discrepancy)) ||
                                  (Param->isOption(OPT_discrepancy_hw) && Param->getOption<bool>(OPT_discrepancy_hw));
      if(is_discrepancy)
      {
         // avoid collision with the main
         if(top_fname_mngl == "main")
         {
            return "_main";
         }
         return BH->get_function_name();
      }
      return top_fname_mngl;
   }();
   if(return_type_index)
   {
      indented_output_stream->Append(RETURN_PORT_NAME " = ");
   }

   indented_output_stream->Append(function_name + "(");
   bool is_first_argument = true;
   const auto& parms = HLSMgr->module_arch->GetArchitecture(top_fname_mngl)->parms;
   for(const auto& par : BH->GetParameters())
   {
      const auto param = BH->PrintVariable(par->index);
      if(!is_first_argument)
      {
         indented_output_stream->Append(", ");
      }
      else
      {
         is_first_argument = false;
      }
      if(tree_helper::IsPointerType(par))
      {
         auto arg_typename = parms.at(param).at(FunctionArchitecture::parm_original_typename);
         if(arg_typename.find("(*)") != std::string::npos)
         {
            arg_typename = arg_typename.substr(0, arg_typename.find("(*)")) + "*";
         }
         if(arg_typename.back() != '*')
         {
            indented_output_stream->Append("*(");
            indented_output_stream->Append(arg_typename.substr(0, arg_typename.size() - (arg_typename.back() == '&')) +
                                           "*");
            indented_output_stream->Append(") ");
         }
         else
         {
            indented_output_stream->Append("(");
            indented_output_stream->Append(arg_typename);
            indented_output_stream->Append(") ");
         }
      }
      indented_output_stream->Append(param);
   }
   indented_output_stream->Append(");\n");
}

void HLSCWriter::WriteExtraInitCode()
{
}

void HLSCWriter::WriteExtraCodeBeforeEveryMainCall()
{
}

void HLSCWriter::InternalWriteFile()
{
   const auto top_symbols = Param->getOption<std::vector<std::string>>(OPT_top_functions_names);
   THROW_ASSERT(top_symbols.size() == 1, "Expected single top function name");
   const auto top_fnode = TM->GetFunction(top_symbols.front());
   const auto top_fb = HLSMgr->CGetFunctionBehavior(top_fnode->index);
   const auto top_bh = top_fb->CGetBehavioralHelper();
   const auto top_fname = top_bh->get_function_name();
   const auto top_fname_mngl = top_bh->GetMangledFunctionName();
   const auto& func_arch = HLSMgr->module_arch->GetArchitecture(top_fname_mngl);
   THROW_ASSERT(func_arch, "Expected interface architecture for function " + top_fname_mngl);
   INDENT_OUT_MEX(OUTPUT_LEVEL_VERBOSE, output_level, "-->C-based testbench generation for function " + top_fname);

   const auto return_type = tree_helper::GetFunctionReturnType(top_fnode);
   const auto top_params = top_bh->GetParameters();

   std::string top_decl = return_type ? tree_helper::PrintType(TM, return_type) : "void";
   top_decl += " " + top_fname_mngl + "(";
   if(top_params.size())
   {
      for(const auto& arg : top_params)
      {
         const auto parm_name = top_bh->PrintVariable(arg->index);
         THROW_ASSERT(func_arch->parms.find(parm_name) != func_arch->parms.end(),
                      "Attributes missing for parameter " + parm_name + " in function " + top_fname);
         const auto& parm_attrs = func_arch->parms.at(parm_name);
         auto arg_typename = parm_attrs.at(FunctionArchitecture::parm_original_typename);
         if(arg_typename.find("(*)") != std::string::npos)
         {
            arg_typename = arg_typename.substr(0, arg_typename.find("(*)")) + "*";
         }
         top_decl += arg_typename + ", ";
      }
      top_decl.erase(top_decl.size() - 2);
   }
   top_decl += ")\n";

   indented_output_stream->AppendIndented("#include <mdpi/mdpi_user.h>\n\n");
   indented_output_stream->AppendIndented("CDECL " + top_decl.substr(0, top_decl.size() - 1) + ";\n\n");

   const auto& test_vectors = HLSMgr->RSim->test_vectors;
   if(top_fname != "main" && test_vectors.size())
   {
      indented_output_stream->Append("int main()\n{\n");
      // write additional initialization code needed by subclasses
      WriteExtraInitCode();
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Written extra init code");

      // parameters declaration
      WriteParamDecl(top_bh);

      // ---- WRITE VARIABLES DECLARATIONS ----
      // declaration of the return variable of the top function, if not void
      if(return_type)
      {
         const auto ret_type = tree_helper::PrintType(TM, return_type);
         if(tree_helper::IsVectorType(return_type))
         {
            THROW_ERROR("return type of function under test " + top_fname + " is " + STR(ret_type) +
                        "\nco-simulation does not support vectorized return types at top level");
         }
         indented_output_stream->Append("// return variable initialization\n");
         indented_output_stream->Append("volatile " + ret_type + " " RETURN_PORT_NAME ";\n");
      }
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Written parameters declaration");
      // ---- WRITE PARAMETERS INITIALIZATION AND FUNCTION CALLS ----
      for(unsigned int v_idx = 0; v_idx < test_vectors.size(); v_idx++)
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                        "-->Writing initialization for test vector " + STR(v_idx));
         indented_output_stream->Append("{\n");
         const auto& curr_test_vector = test_vectors.at(v_idx);
         // write parameter initialization
         indented_output_stream->Append("// parameter initialization\n");
         WriteParamInitialization(top_bh, curr_test_vector);
         WriteExtraCodeBeforeEveryMainCall();
         // write the call to the top function to be tested
         indented_output_stream->Append("// function call\n");
         WriteTestbenchFunctionCall(top_bh);

         indented_output_stream->Append("}\n");
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                        "<--Written initialization for test vector " + STR(v_idx));
      }
      indented_output_stream->Append("return 0;\n");
      indented_output_stream->Append("}\n");
   }
   INDENT_OUT_MEX(OUTPUT_LEVEL_VERBOSE, output_level, "<--Prepared testbench");
}

void HLSCWriter::WriteFunctionImplementation(unsigned int)
{
   /// Do nothing
}

void HLSCWriter::WriteBuiltinWaitCall()
{
   /// Do nothing
}
