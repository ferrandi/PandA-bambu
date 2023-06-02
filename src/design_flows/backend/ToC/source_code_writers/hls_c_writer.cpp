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
 *              Copyright (C) 2004-2023 Politecnico di Milano
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
#include "hls_c_backend_information.hpp"
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
#include "testbench_generation_base_step.hpp"
#include "testbench_generation_constants.hpp"
#include "tree_helper.hpp"
#include "tree_manager.hpp"
#include "tree_node.hpp"
#include "tree_reindex.hpp"
#include "var_pp_functor.hpp"

#include <boost/algorithm/string/trim_all.hpp>
#include <boost/filesystem/operations.hpp>
#include <boost/regex.hpp>

#include <list>
#include <string>
#include <vector>

REF_FORWARD_DECL(memory_symbol);

static const boost::regex wrapper_def("(ac_channel|stream|hls::stream)<(.*)>");
#define WRAPPER_GROUP_WTYPE 2

HLSCWriter::HLSCWriter(const HLSCBackendInformationConstRef _hls_c_backend_information,
                       const application_managerConstRef _AppM, const InstructionWriterRef _instruction_writer,
                       const IndentedOutputStreamRef _indented_output_stream, const ParameterConstRef _parameters,
                       bool _verbose)
    : CWriter(_AppM, _instruction_writer, _indented_output_stream, _parameters, _verbose),
      hls_c_backend_information(_hls_c_backend_information)
{
   /// include from cpp
   flag_cpp = TM->is_CPP() && !Param->isOption(OPT_pretty_print) &&
              (!Param->isOption(OPT_discrepancy) || !Param->getOption<bool>(OPT_discrepancy) ||
               !Param->isOption(OPT_discrepancy_hw) || !Param->getOption<bool>(OPT_discrepancy_hw));
   debug_level = _parameters->get_class_debug_level(GET_CLASS(*this));
}

void HLSCWriter::WriteHeader()
{
   indented_output_stream->Append(R"(
#define __Inf (1.0 / 0.0)
#define __Nan (0.0 / 0.0)

#ifdef __cplusplus
#undef printf

#include <cstdio>
#include <cstdlib>
#ifndef CUSTOM_VERIFICATION
#include <cstring>
#endif

typedef bool _Bool;
#else
#include <stdio.h>
#include <stdlib.h>
#ifndef CUSTOM_VERIFICATION
#include <string.h>
#endif

extern void exit(int status);
#endif

#include <sys/types.h>

#ifdef __AC_NAMESPACE
using namespace __AC_NAMESPACE;
#endif

#include <mdpi/mdpi_debug.h>
#include <mdpi/mdpi_wrapper.h>

)");

   // get the root function to be tested by the testbench
   const auto top_function_ids = AppM->CGetCallGraphManager()->GetRootFunctions();
   THROW_ASSERT(top_function_ids.size() == 1, "Multiple top function");
   const auto function_id = *(top_function_ids.begin());
   const auto fnode = TM->CGetTreeNode(function_id);
   const auto fd = GetPointerS<const function_decl>(fnode);
   const auto fname = tree_helper::GetMangledFunctionName(fd);
   auto& DesignInterfaceInclude = hls_c_backend_information->HLSMgr->design_interface_typenameinclude;
   if(DesignInterfaceInclude.find(fname) != DesignInterfaceInclude.end())
   {
      CustomOrderedSet<std::string> includes;
      const auto& DesignInterfaceArgsInclude = DesignInterfaceInclude.find(fname)->second;
      for(const auto& argInclude : DesignInterfaceArgsInclude)
      {
         const auto incls = convert_string_to_vector<std::string>(argInclude.second, ";");
         includes.insert(incls.begin(), incls.end());
      }
      for(const auto& inc : includes)
      {
         indented_output_stream->Append("#include \"" + inc + "\"\n");
      }
   }
}

void HLSCWriter::WriteGlobalDeclarations()
{
   instrWriter->write_declarations();
   WriteTestbenchGlobalVars();
   // WriteTestbenchHelperFunctions();
}

void HLSCWriter::WriteTestbenchGlobalVars()
{
}

void HLSCWriter::WriteTestbenchHelperFunctions()
{
   // exit function
   indented_output_stream->Append("//variable used to detect a standard end of the main (exit has not been called)\n");
   indented_output_stream->Append("unsigned int __standard_exit;\n");
   indented_output_stream->Append("//definition of __bambu_testbench_exit function\n");
   indented_output_stream->Append("void __bambu_testbench_exit(void) __attribute__ ((destructor));\n");
   indented_output_stream->Append("void __bambu_testbench_exit(void)\n");
   indented_output_stream->Append("{\n");
   indented_output_stream->Append("if (!__standard_exit)\n");
   indented_output_stream->Append("{\n");
   indented_output_stream->Append("fprintf(__bambu_testbench_fp, \"//expected value for return value\\n\");\n");
   indented_output_stream->Append("fprintf(__bambu_testbench_fp, \"o00000000000000000000000000000000\\n\");\n");
   indented_output_stream->Append("fprintf(__bambu_testbench_fp, \"e\\n\");\n");
   indented_output_stream->Append("}\n");
   indented_output_stream->Append("}\n\n");
   // decimal to binary conversion function
   indented_output_stream->Append(
       "void _Dec2Bin_(FILE * __bambu_testbench_fp, long long int num, unsigned int precision)\n");
   indented_output_stream->Append("{\n");
   indented_output_stream->Append("unsigned int i;\n");
   indented_output_stream->Append("unsigned long long int ull_value = (unsigned long long int) num;\n");
   indented_output_stream->Append("for (i = 0; i < precision; ++i)\n");
   indented_output_stream->Append(
       "fprintf(__bambu_testbench_fp, \"%c\", (((1LLU << (precision - i -1)) & ull_value) ? '1' : '0'));\n");
   indented_output_stream->Append("}\n\n");
   // pointer to binary conversion function
   indented_output_stream->Append(
       "void _Ptd2Bin_(FILE * __bambu_testbench_fp, unsigned char * num, unsigned int precision)\n");
   indented_output_stream->Append("{\n");
   indented_output_stream->Append("unsigned int i, j;\n");
   indented_output_stream->Append("char value;\n");
   indented_output_stream->Append("if (precision%8)\n");
   indented_output_stream->Append("{\n");
   indented_output_stream->Append("value = *(num+precision/8);\n");
   indented_output_stream->Append("for (j = 8-precision%8; j < 8; ++j)\n");
   indented_output_stream->Append(
       "fprintf(__bambu_testbench_fp, \"%c\", (((1LLU << (8 - j - 1)) & value) ? '1' : '0'));\n");
   indented_output_stream->Append("}\n");
   indented_output_stream->Append("for (i = 0; i < 8*(precision/8); i = i + 8)\n");
   indented_output_stream->Append("{\n");
   indented_output_stream->Append("value = *(num + (precision / 8) - (i / 8) - 1);\n");
   indented_output_stream->Append("for (j = 0; j < 8; ++j)\n");
   indented_output_stream->Append(
       "fprintf(__bambu_testbench_fp, \"%c\", (((1LLU << (8 - j - 1)) & value) ? '1' : '0'));\n");
   indented_output_stream->Append("}\n");
   indented_output_stream->Append("}\n\n");
   if(Param->isOption(OPT_discrepancy) and Param->getOption<bool>(OPT_discrepancy))
   {
      // Builtin floating point checkers
      indented_output_stream->Append("typedef union view_convert32 {\n");
      indented_output_stream->Append("unsigned int bits;\n");
      indented_output_stream->Append("float fp32;\n");
      indented_output_stream->Append("};\n\n");
      indented_output_stream->Append("typedef union view_convert64 {\n");
      indented_output_stream->Append("unsigned long long bits;\n");
      indented_output_stream->Append("double fp64;\n");
      indented_output_stream->Append("};\n\n");
      indented_output_stream->Append("float _Int32_ViewConvert(unsigned int i)\n");
      indented_output_stream->Append("{\n");
      indented_output_stream->Append("union view_convert32 vc;\n");
      indented_output_stream->Append("vc.bits = i;\n");
      indented_output_stream->Append("return vc.fp32;\n");
      indented_output_stream->Append("}\n\n");
      indented_output_stream->Append("double _Int64_ViewConvert(unsigned long long i)\n");
      indented_output_stream->Append("{\n");
      indented_output_stream->Append("union view_convert64 vc;\n");
      indented_output_stream->Append("vc.bits = i;\n");
      indented_output_stream->Append("return vc.fp64;\n");
      indented_output_stream->Append("}\n");
      indented_output_stream->Append("\n\n");
      indented_output_stream->Append("unsigned char _FPs32Mismatch_(float c, float e, float max_ulp)\n");
      indented_output_stream->Append("{\n");
      indented_output_stream->Append("unsigned int binary_c = *((unsigned int*)&c);\n");
      indented_output_stream->Append("unsigned int binary_e = *((unsigned int*)&e);\n");
      indented_output_stream->Append("unsigned int binary_abs_c = binary_c&(~(1U<<31));\n");
      indented_output_stream->Append("unsigned int binary_abs_e = binary_e&(~(1U<<31));\n");
      indented_output_stream->Append("unsigned int denom_0 = 0x34000000;\n");
      indented_output_stream->Append("unsigned int denom_e = ((binary_abs_e>> 23)-23)<<23;\n");
      indented_output_stream->Append("float cme=c - e;\n");
      indented_output_stream->Append("unsigned int binary_cme = *((unsigned int*)&cme);\n");
      indented_output_stream->Append("unsigned int binary_abs_cme = binary_cme&(~(1U<<31));\n");
      indented_output_stream->Append("float abs_cme=*((float*)&binary_abs_cme);\n");
      indented_output_stream->Append("float ulp = 0.0;\n");
      indented_output_stream->Append("if (binary_abs_c>0X7F800000 && binary_abs_c>0X7F800000) return 0;\n");
      indented_output_stream->Append("else if (binary_abs_c==0X7F800000 && binary_abs_e==0X7F800000)\n");
      indented_output_stream->Append("{\n");
      indented_output_stream->Append("if ((binary_c>>31) != (binary_e>>31))\n");
      indented_output_stream->Append("return 1;\n");
      indented_output_stream->Append("else\n");
      indented_output_stream->Append("return 0;\n");
      indented_output_stream->Append("}\n");
      indented_output_stream->Append("else if (binary_abs_c==0X7F800000 || binary_abs_e==0X7F800000 || "
                                     "binary_abs_c>0X7F800000 || binary_abs_e==0X7F800000) return 0;\n");
      indented_output_stream->Append("else\n");
      indented_output_stream->Append("{\n");
      indented_output_stream->Append("if (binary_abs_e == 0) ulp = abs_cme / (*((float *)&denom_0));\n");
      indented_output_stream->Append("else ulp = abs_cme / (*((float *)&denom_e));\n");
      indented_output_stream->Append("return ulp > max_ulp;\n");
      indented_output_stream->Append("}\n");
      indented_output_stream->Append("}\n\n");
      indented_output_stream->Append("unsigned char _FPs64Mismatch_(double c, double e, double max_ulp)\n");
      indented_output_stream->Append("{\n");
      indented_output_stream->Append("unsigned long long int binary_c = *((unsigned long long int*)&c);\n");
      indented_output_stream->Append("unsigned long long int binary_e = *((unsigned long long int*)&e);\n");
      indented_output_stream->Append("unsigned long long int binary_abs_c = binary_c&(~(1ULL<<63));\n");
      indented_output_stream->Append("unsigned long long int binary_abs_e = binary_e&(~(1ULL<<63));\n");
      indented_output_stream->Append("unsigned long long int denom_0 = 0x3CB0000000000000;\n");
      indented_output_stream->Append("unsigned long long int denom_e = ((binary_abs_e>> 52)-52)<<52;\n");
      indented_output_stream->Append("double cme=c - e;\n");
      indented_output_stream->Append("unsigned long long int binary_cme = *((unsigned int*)&cme);\n");
      indented_output_stream->Append("unsigned long long int binary_abs_cme = binary_cme&(~(1U<<31));\n");
      indented_output_stream->Append("double abs_cme=*((double*)&binary_abs_cme);\n");
      indented_output_stream->Append("double ulp = 0.0;\n");
      indented_output_stream->Append(
          "if (binary_abs_c>0X7FF0000000000000 && binary_abs_c>0X7FF0000000000000) return 0;\n");
      indented_output_stream->Append(
          "else if (binary_abs_c==0X7FF0000000000000 && binary_abs_e==0X7FF0000000000000)\n");
      indented_output_stream->Append("{\n");
      indented_output_stream->Append("if ((binary_c>>63) != (binary_e>>63))\n");
      indented_output_stream->Append("return 1;\n");
      indented_output_stream->Append("else\n");
      indented_output_stream->Append("return 0;\n");
      indented_output_stream->Append("}\n");
      indented_output_stream->Append(
          "else if (binary_abs_c==0X7FF0000000000000 || binary_abs_e==0X7FF0000000000000 || "
          "binary_abs_c>0X7FF0000000000000 || binary_abs_e==0X7FF0000000000000) return 0;\n");
      indented_output_stream->Append("else\n");
      indented_output_stream->Append("{\n");
      indented_output_stream->Append("if (binary_abs_e == 0) ulp = abs_cme / (*((double *)&denom_0));\n");
      indented_output_stream->Append("else ulp = abs_cme / (*((double *)&denom_e));\n");
      indented_output_stream->Append("return ulp > max_ulp;\n");
      indented_output_stream->Append("}\n");
      indented_output_stream->Append("}\n\n");
      indented_output_stream->Append("void _CheckBuiltinFPs32_(char * chk_str, unsigned char neq, float par_expected, "
                                     "float par_res, float par_a, float par_b)\n");
      indented_output_stream->Append("{\n");
      indented_output_stream->Append("if(neq)\n");
      indented_output_stream->Append("{\n");
      indented_output_stream->Append(
          "printf(\"\\n\\n***********************************************************\\nERROR ON A BASIC FLOATING "
          "POINT OPERATION : %s : expected=%a (%.20e) res=%a (%.20e) a=%a (%.20e) "
          "b=%a (%.20e)\\n***********************************************************\\n\\n\", chk_str, "
          "par_expected, par_expected, par_res, par_res, par_a, par_a, par_b, par_b);\n");
      indented_output_stream->Append("exit(1);\n");
      indented_output_stream->Append("}\n");
      indented_output_stream->Append("}\n\n");
      indented_output_stream->Append("void _CheckBuiltinFPs64_(char * chk_str, unsigned char neq, double par_expected, "
                                     "double par_res, double par_a, double par_b)\n");
      indented_output_stream->Append("{\n");
      indented_output_stream->Append("if(neq)\n");
      indented_output_stream->Append("{\n");
      indented_output_stream->Append(
          "printf(\"\\n\\n***********************************************************\\nERROR ON A BASIC FLOATING "
          "POINT OPERATION : %s : expected=%a (%.35e) res=%a (%.35e) a=%a (%.35e) "
          "b=%a (%.35e)\\n***********************************************************\\n\\n\", chk_str, "
          "par_expected, par_expected, par_res, par_res, par_a, par_a, par_b, par_b);\n");
      indented_output_stream->Append("exit(1);\n");
      indented_output_stream->Append("}\n");
      indented_output_stream->Append("}\n\n");
   }
}

void HLSCWriter::WriteParamDecl(const BehavioralHelperConstRef BH)
{
   indented_output_stream->Append("// parameters declaration\n");
   hls_c_backend_information->HLSMgr->RSim->simulationArgSignature.clear();

   for(const auto& par : BH->GetParameters())
   {
      const auto parm_type = tree_helper::CGetType(par);
      const auto type = tree_helper::IsPointerType(par) ? "void*" : tree_helper::PrintType(TM, parm_type);
      const auto param = BH->PrintVariable(par->index);
      hls_c_backend_information->HLSMgr->RSim->simulationArgSignature.push_back(param);

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
   const auto fnode = TM->CGetTreeReindex(BH->get_function_index());
   const auto fname = tree_helper::GetMangledFunctionName(GetPointerS<const function_decl>(GET_CONST_NODE(fnode)));
   const auto& DesignAttributes = hls_c_backend_information->HLSMgr->design_attributes;
   const auto function_if = [&]() -> const std::map<std::string, std::map<interface_attributes, std::string>>* {
      const auto it = DesignAttributes.find(fname);
      if(it != DesignAttributes.end())
      {
         return &it->second;
      }
      return nullptr;
   }();
   const auto interface_type = Param->getOption<HLSFlowStep_Type>(OPT_interface_type);
   const auto is_interface_inferred = interface_type == HLSFlowStep_Type::INFERRED_INTERFACE_GENERATION;
   const auto arg_signature_typename =
       hls_c_backend_information->HLSMgr->design_interface_typename_orig_signature.find(fname);
   THROW_ASSERT(!is_interface_inferred ||
                    arg_signature_typename !=
                        hls_c_backend_information->HLSMgr->design_interface_typename_orig_signature.end(),
                "");
   const auto params = BH->GetParameters();
   for(auto par_idx = 0U; par_idx < params.size(); ++par_idx)
   {
      const auto& par = params.at(par_idx);
      const auto parm_type = tree_helper::CGetType(par);
      const auto param = BH->PrintVariable(GET_INDEX_CONST_NODE(par));
      const auto has_file_init = [&]() {
         if(function_if)
         {
            const auto& type_name = function_if->at(param).at(attr_typename);
            return boost::regex_search(type_name.c_str(), wrapper_def);
         }
         return false;
      }();
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Writing initialization of " + param);
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                     "---Type: " + GET_CONST_NODE(parm_type)->get_kind_text() + " - " + STR(parm_type));
      const auto init_it = curr_test_vector.find(param);
      if(init_it == curr_test_vector.end())
      {
         THROW_ERROR("Value of " + param + " is missing in test vector");
      }
      const auto test_v = (has_file_init && boost::ends_with(init_it->second, ".dat")) ?
                              ("\"" + init_it->second + "\"") :
                              init_it->second;
      if(tree_helper::IsPointerType(parm_type))
      {
         const auto is_binary_init = boost::ends_with(test_v, ".dat");

         std::string var_ptdtype;
         std::string temp_var_decl;
         bool is_a_true_pointer = true;
         if(flag_cpp && !is_binary_init && is_interface_inferred)
         {
            var_ptdtype = arg_signature_typename->second.at(par_idx);
            is_a_true_pointer = var_ptdtype.back() == '*';
            if(is_a_true_pointer || var_ptdtype.back() == '&')
            {
               var_ptdtype.pop_back();
            }
            temp_var_decl = var_ptdtype + " " + param + "_temp" + (is_a_true_pointer ? "[]" : "");
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
         const auto arg_channel =
             boost::regex_search(var_ptdtype, boost::regex("(ac_channel|stream|hls::stream)<(.*)>"));
         const auto ptd_type = tree_helper::GetRealType(tree_helper::CGetPointedType(parm_type));
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                        "---Pointed type: " + GET_CONST_NODE(ptd_type)->get_kind_text() + " - " + STR(ptd_type));

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
            indented_output_stream->Append("m_alloc_param(" + STR(par_idx) + ", " + param + "_size);\n");
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
               indented_output_stream->Append("m_alloc_param(" + STR(par_idx) + ", sizeof(" + param + "_temp));\n");
            }
         }
      }
      else
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Inline initialization");
         const auto parm_type_bitsize = tree_helper::Size(parm_type);
         if(tree_helper::IsRealType(parm_type) && test_v == "-0")
         {
            if(parm_type_bitsize == 32)
            {
               indented_output_stream->Append(param + " = copysignf(0.0, -1.0);\n");
            }
            else
            {
               indented_output_stream->Append(param + " = copysign(0.0, -1.0);\n");
            }
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

   auto function_name = BH->get_function_name();
   // avoid collision with the main
   if(function_name == "main")
   {
      const auto is_discrepancy = (Param->isOption(OPT_discrepancy) && Param->getOption<bool>(OPT_discrepancy)) ||
                                  (Param->isOption(OPT_discrepancy_hw) && Param->getOption<bool>(OPT_discrepancy_hw));
      if(!is_discrepancy)
      {
         function_name = "system";
      }
      else
      {
         function_name = "_main";
      }
   }

   bool is_system;
   const auto decl = std::get<0>(BH->get_definition(function_index, is_system));
   if((is_system || decl == "<built-in>") && return_type_index && BH->is_real(return_type_index))
   {
      indented_output_stream->Append("extern " + BH->print_type(return_type_index) + " " + function_name + "(");
      bool is_first_parameter = true;
      for(const auto& p : BH->GetParameters())
      {
         if(is_first_parameter)
         {
            is_first_parameter = false;
         }
         else
         {
            indented_output_stream->Append(", ");
         }

         const auto parm_type = tree_helper::CGetType(p);
         const auto type_name = tree_helper::PrintType(TM, parm_type);
         const auto param = BH->PrintVariable(GET_INDEX_CONST_NODE(p));

         if(tree_helper::IsPointerType(p))
         {
            const auto var_functor = var_pp_functorRef(new std_var_pp_functor(BH));
            indented_output_stream->Append(tree_helper::PrintType(TM, parm_type, false, false, false, p, var_functor));
         }
         else
         {
            indented_output_stream->Append(type_name + " " + param + "");
         }
      }
      indented_output_stream->Append(");\n");
   }

   if(return_type_index)
   {
      indented_output_stream->Append(RETURN_PORT_NAME " = ");
   }

   indented_output_stream->Append(function_name + "(");
   // function arguments
   if(function_name != "system")
   {
      bool is_first_argument = true;
      unsigned par_index = 0;
      const auto top_fname_mngl = BH->GetMangledFunctionName();
      const auto& DesignInterfaceTypenameOrig =
          hls_c_backend_information->HLSMgr->design_interface_typename_orig_signature;
      for(const auto& par : BH->GetParameters())
      {
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
            if(DesignInterfaceTypenameOrig.find(top_fname_mngl) != DesignInterfaceTypenameOrig.end())
            {
               auto arg_typename = DesignInterfaceTypenameOrig.find(top_fname_mngl)->second.at(par_index);
               if(arg_typename.find("(*)") != std::string::npos)
               {
                  arg_typename = arg_typename.substr(0, arg_typename.find("(*)")) + "*";
               }
               if(arg_typename.back() != '*')
               {
                  indented_output_stream->Append("*(");
                  indented_output_stream->Append(
                      arg_typename.substr(0, arg_typename.size() - (arg_typename.back() == '&')) + "*");
                  indented_output_stream->Append(") ");
               }
               else
               {
                  indented_output_stream->Append("(");
                  indented_output_stream->Append(arg_typename);
                  indented_output_stream->Append(") ");
               }
            }
            else
            {
               indented_output_stream->Append("(" + tree_helper::PrintType(TM, par) + ")");
            }
         }
         const auto param = BH->PrintVariable(GET_INDEX_CONST_NODE(par));
         indented_output_stream->Append(param);
         ++par_index;
      }
   }
   else
   {
      indented_output_stream->Append("\"" + Param->getOption<std::string>(OPT_output_directory) +
                                     "/simulation/main_exec\"");
   }
   indented_output_stream->Append(");\n");

   if(function_name == "system" && return_type_index)
   {
      if(!Param->getOption<bool>(OPT_no_return_zero))
      {
         indented_output_stream->Append("if(" RETURN_PORT_NAME " != 0) exit(1);\n");
      }
      else
      {
         indented_output_stream->Append("if(!WIFEXITED(" RETURN_PORT_NAME ")) exit(1);\n");
         indented_output_stream->Append(RETURN_PORT_NAME " = WEXITSTATUS(" RETURN_PORT_NAME ");\n");
      }
   }
}

void HLSCWriter::WriteSimulatorInitMemory(const unsigned int function_id)
{
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Writing simulator init memory");
   const auto& HLSMgr = hls_c_backend_information->HLSMgr;
   const auto mem_vars = HLSMgr->Rmem->get_ext_memory_variables();
   const auto BH = HLSMgr->CGetFunctionBehavior(function_id)->CGetBehavioralHelper();
   const auto parameters = BH->get_parameters();
   indented_output_stream->Append(R"(
typedef struct
{
const char* filename;
const size_t size;
const ptr_t addrmap;
void* addr;
} __m_memmap_t;

static int cmpptr(const ptr_t a, const ptr_t b) { return a < b ? -1 : (a > b); }
static int cmpaddr(const void* a, const void* b) { return cmpptr(*(ptr_t*)a, *(ptr_t*)b); }

static void __m_memsetup(void* args[], size_t args_size)
{
size_t m_extmem_size, i;
void **m_extmem;
ptr_t prev, curr_base;
)");
   auto base_addr = HLSMgr->base_address;
   if(mem_vars.size())
   {
      indented_output_stream->Append("static __m_memmap_t memmap_init[] = {\n");
      const auto output_directory = Param->getOption<std::string>(OPT_output_directory) + "/simulation/";
      for(const auto& mem_var : mem_vars)
      {
         const auto var_id = mem_var.first;
         const auto is_top_param = std::find(parameters.begin(), parameters.end(), var_id) != parameters.end();
         if(!is_top_param)
         {
            const auto var_name = BH->PrintVariable(var_id);
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Writing initialization for " + var_name);
            const auto var_addr = HLSMgr->Rmem->get_external_base_address(var_id);
            const auto var_init_dat = output_directory + "init." + var_name + ".dat";
            const auto byte_count = TestbenchGenerationBaseStep::generate_init_file(
                var_init_dat, TM, var_id, hls_c_backend_information->HLSMgr->Rmem);
            indented_output_stream->Append("  {\"" + var_init_dat + "\", " + STR(byte_count) + ", " + STR(var_addr) +
                                           ", NULL},\n");
            base_addr = std::max(base_addr, var_addr + byte_count);
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Init file   : '" + var_init_dat + "'");
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Memory usage: " + STR(byte_count) + " bytes");
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Base address: " + STR(var_addr));
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--");
         }
      }
      indented_output_stream->Append("};\n");
   }
   indented_output_stream->Append("const ptr_t base_addr = " + STR(base_addr) + ";\n\n");

   indented_output_stream->Append("m_extmem_size = args_size;\n\n");

   indented_output_stream->Append("__m_memmap_init();\n");
   if(mem_vars.size())
   {
      indented_output_stream->Append(R"(
// Memory-mapped internal variables initialization
for(i = 0; i < sizeof(memmap_init) / sizeof(*memmap_init); ++i)
{
FILE* fp = fopen(memmap_init[i].filename, "rb");
if(!fp)
{
error("Unable to open file: %s", memmap_init[i].filename);
perror("");
exit(EXIT_FAILURE);
}
if(memmap_init[i].addr == NULL)
{
memmap_init[i].addr = malloc(memmap_init[i].size);
}
if(fread(memmap_init[i].addr, 1, memmap_init[i].size, fp) != memmap_init[i].size)
{
error("Unable to read %u bytes from file: %s", memmap_init[i].size, memmap_init[i].filename);
perror("");
exit(EXIT_FAILURE);
}
fclose(fp);
__m_memmap(memmap_init[i].addrmap, memmap_init[i].addr);
}
)");
   }

   indented_output_stream->Append(R"(
m_extmem = (void**)malloc(sizeof(void*) * m_extmem_size);
for(i = 0; i < args_size; ++i)
{
m_extmem[i] = args[i];
}

qsort(m_extmem, m_extmem_size, sizeof(void*), cmpaddr);
prev = (ptr_t)m_extmem[0];
curr_base = base_addr;
__m_memmap(curr_base, m_extmem[0]);
for(i = 1; i < m_extmem_size; ++i)
{
const ptr_t curr = (ptr_t)m_extmem[i];
curr_base += curr - prev;
__m_memmap(curr_base, m_extmem[i]);
prev = curr;
}
free(m_extmem);
}

)");
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Written simulator init memory");
}

void HLSCWriter::WriteExtraInitCode()
{
}

void HLSCWriter::WriteExtraCodeBeforeEveryMainCall()
{
}

void HLSCWriter::WriteMainTestbench()
{
   THROW_ASSERT(AppM->CGetCallGraphManager()->GetRootFunctions().size() == 1, "Multiple top functions not supported");
   const auto top_id = *AppM->CGetCallGraphManager()->GetRootFunctions().begin();
   const auto top_fb = AppM->CGetFunctionBehavior(top_id);
   const auto top_bh = top_fb->CGetBehavioralHelper();
   const auto top_fnode = TM->CGetTreeReindex(top_id);
   const auto top_fname = top_bh->get_function_name();
   const auto top_fname_mngl = top_bh->GetMangledFunctionName();
   const auto interface_type = Param->getOption<HLSFlowStep_Type>(OPT_interface_type);
   const auto is_interface_inferred = interface_type == HLSFlowStep_Type::INFERRED_INTERFACE_GENERATION;
   const auto arg_signature_typename =
       hls_c_backend_information->HLSMgr->design_interface_typename_orig_signature.find(top_fname_mngl);
   const auto arg_attributes = hls_c_backend_information->HLSMgr->design_attributes.find(top_fname_mngl);
   THROW_ASSERT(!is_interface_inferred ||
                    arg_signature_typename !=
                        hls_c_backend_information->HLSMgr->design_interface_typename_orig_signature.end(),
                "Original signature not found for function: " + top_fname_mngl + " (" + top_fname + ")");
   THROW_ASSERT(!is_interface_inferred || arg_attributes != hls_c_backend_information->HLSMgr->design_attributes.end(),
                "Design attributes not found for function: " + top_fname_mngl + " (" + top_fname + ")");

   const auto return_type = tree_helper::GetFunctionReturnType(top_fnode);
   const auto top_params = top_bh->GetParameters();
   const auto args_decl_size = top_params.size() + (return_type != nullptr);
   const auto has_subnormals = Param->isOption(OPT_fp_subnormal) && Param->getOption<bool>(OPT_fp_subnormal);
   const auto cmp_type = [&](tree_nodeConstRef t, const std::string& tname) -> std::string {
      if(boost::starts_with(tname, "struct") || boost::starts_with(tname, "union"))
      {
         return "mem";
      }
      else if(t)
      {
         if(tree_helper::IsPointerType(t))
         {
            t = tree_helper::CGetPointedType(t);
         }
         while(tree_helper::IsArrayType(t))
         {
            t = tree_helper::CGetElements(t);
         }
         if(tree_helper::IsRealType(t))
         {
            return has_subnormals ? "flts" : "flt";
         }
      }
      return "val";
   };
   const auto extern_decl = top_fname == top_fname_mngl ? "EXTERN_C " : "";

   std::string top_decl = extern_decl;
   std::string gold_decl = extern_decl;
   std::string gold_call;
   std::string args_init;
   std::string args_decl = "void* args[] = {";
   std::string args_set;
   std::string gold_cmp;
   size_t args_decl_idx = 0;
   if(return_type)
   {
      const auto return_type_str = tree_helper::PrintType(TM, return_type);
      top_decl += return_type_str;
      gold_decl += return_type_str;
      gold_call += "retval_gold = ";
      args_decl = return_type_str + " retval, retval_gold;\n" + args_decl + "(void*)&retval, ";
      args_set += "__m_setarg(0, args[0], " + STR(tree_helper::Size(return_type)) + ");\n";
      ++args_decl_idx;
   }
   else
   {
      top_decl += "void";
      gold_decl += "void";
   }
   top_decl += " " + top_fname + "(";
   gold_decl += " __m_" + top_fname + "(";
   gold_call += "__m_" + top_fname + "(";
   if(top_params.size())
   {
      for(const auto& arg : top_params)
      {
         std::string arg_typename, arg_interface, arg_size;
         const auto param_idx = args_decl_idx - (return_type != nullptr);
         const auto arg_type = tree_helper::CGetType(arg);
         if(arg_signature_typename != hls_c_backend_information->HLSMgr->design_interface_typename_orig_signature.end())
         {
            THROW_ASSERT(arg_signature_typename->second.size() > param_idx,
                         "Original signature missing for parameter " + STR(param_idx));
            arg_typename = arg_signature_typename->second.at(param_idx);
         }
         else
         {
            arg_typename = tree_helper::PrintType(TM, arg_type, false, true);
         }
         if(is_interface_inferred)
         {
            const auto param_name = top_bh->PrintVariable(GET_INDEX_CONST_NODE(arg));
            THROW_ASSERT(arg_attributes->second.count(param_name),
                         "Attributes missing for parameter " + param_name + " in function " + top_fname);
            arg_interface = arg_attributes->second.at(param_name).at(attr_interface_type);
         }
         else
         {
            arg_interface = "default";
         }
         if(arg_typename.find("(*)") != std::string::npos)
         {
            arg_typename = arg_typename.substr(0, arg_typename.find("(*)")) + "*";
         }
         arg_size = STR(tree_helper::Size(arg_type));
         const auto arg_name = "P" + STR(args_decl_idx);
         const auto is_pointer_type = arg_typename.back() == '*';
         const auto is_reference_type = arg_typename.back() == '&';
         top_decl += arg_typename + " " + arg_name + ", ";
         gold_decl += arg_typename + ", ";
         boost::cmatch what;
         if(boost::regex_search(arg_typename.data(), what, boost::regex("(ac_channel|stream|hls::stream)<(.*)>")))
         {
            THROW_ASSERT(is_pointer_type || is_reference_type, "Channel parameters must be pointers or references.");
            const std::string channel_type(what[1].first, what[1].second);
            arg_typename.pop_back();
            gold_call += arg_name + ", ";
            gold_cmp += "m_channelcmp(" + STR(args_decl_idx) + ", " + cmp_type(arg_type, channel_type) + ");\n";
            args_init += "m_channel_init(" + STR(args_decl_idx) + ");\n";
            args_decl += arg_name + "_sim, ";
            args_set += "__m_setargptr";
         }
         else if(is_pointer_type)
         {
            gold_call += "(" + arg_typename + ")" + arg_name + "_gold, ";
            gold_cmp += "m_argcmp(" + STR(args_decl_idx) + ", " + cmp_type(arg_type, arg_typename) + ");\n";
            args_decl += "(void*)" + arg_name + ", ";
            args_set += "m_setargptr";
         }
         else if(is_reference_type)
         {
            arg_typename.pop_back();
            gold_call += "*(" + arg_typename + "*)" + arg_name + "_gold, ";
            gold_cmp += "m_argcmp(" + STR(args_decl_idx) + ", " + cmp_type(arg_type, arg_typename) + ");\n";
            args_init += "m_alloc_param(" + STR(param_idx) + ", sizeof(" + arg_typename + "));\n";
            args_decl += "(void*)&" + arg_name + ", ";
            args_set += "m_setargptr";
         }
         else
         {
            gold_call += arg_name + ", ";
            args_decl += "(void*)&" + arg_name + ", ";
            args_set += arg_interface == "default" ? "__m_setarg" : "m_setargptr";
         }
         args_set += "(" + STR(args_decl_idx) + ", args[" + STR(args_decl_idx) + "], " + arg_size + ");\n";
         ++args_decl_idx;
      }
      top_decl.erase(top_decl.size() - 2);
      gold_decl.erase(gold_decl.size() - 2);
      gold_call.erase(gold_call.size() - 2);
      args_decl.erase(args_decl.size() - 2);
   }
   top_decl += ")\n";
   gold_decl += ");\n";
   gold_call += ");\n";
   args_decl += "};\n";

   indented_output_stream->AppendIndented(
       std::string() + R"(
#ifndef CUSTOM_VERIFICATION
#define m_setargptr(idx, ptr, ptrsize)                   \
   __m_setargptr(idx, ptr, ptrsize);                     \
   const size_t P##idx##_size = __m_param_size(idx)" +
       (return_type ? " - 1" : "") +
       R"(); \
   void* P##idx##_gold = malloc(P##idx##_size);          \
   memcpy(P##idx##_gold, ptr, P##idx##_size)

#define typeof __typeof__
#ifdef __cplusplus
template <typename T> struct __m_type { typedef T type; };
template <typename T> struct __m_type<T*> { typedef typename __m_type<T>::type type; };
#define m_getptrt(val) __m_type<typeof(val)>::type*
#define m_getvalt(val) __m_type<typeof(val)>::type
template <typename T> T* m_getptr(T& obj) { return &obj; }
template <typename T> T* m_getptr(T* obj) { return obj; }
#define __m_float_distance(a, b) m_float_distance(a, b)
#else
#define m_getptrt(val) typeof(val)
#define m_getvalt(val) typeof(*val)
#define m_getptr(ptr) (ptr)
#define __m_float_distance(a, b) \
   ((typeof(a)(*)(typeof(a), typeof(a)))((sizeof(a) == sizeof(float)) ? m_float_distancef : m_float_distance))(a, b)
#define __m_floats_distance(a, b) \
   ((typeof(a)(*)(typeof(a), typeof(a)))((sizeof(a) == sizeof(float)) ? m_floats_distancef : m_floats_distance))(a, b)
#endif

#define m_cmpval(ptra, ptrb) *(ptra) != *(ptrb)
#define m_cmpmem(ptra, ptrb) memcmp(ptra, ptrb, sizeof(*ptrb))
#define m_cmpflt(ptra, ptrb) __m_float_distance(*(ptra), *(ptrb)) > max_ulp
#define m_cmpflts(ptra, ptrb) __m_floats_distance(*(ptra), *(ptrb)) > max_ulp

#define m_argcmp(idx, cmp)                                                                             \
   const size_t P##idx##_count = P##idx##_size / sizeof(m_getvalt(P##idx));                            \
   for(i = 0; i < P##idx##_count; ++i)                                                                 \
   {                                                                                                   \
      if(m_cmp##cmp((m_getptrt(P##idx))P##idx##_gold + i, &m_getptr(P##idx)[i]))                       \
      {                                                                                                \
         error("Memory parameter %u (%u/%u) mismatch with respect to golden reference.\n", idx)" +
       (return_type ? " - 1" : "") +
       R"(, i + 1, \
               P##idx##_count);                                                                        \
         ++mismatch_count;                                                                             \
      }                                                                                                \
   }                                                                                                   \
   free(P##idx##_gold);

#define m_channelcmp(idx, cmp)                                                                              \
   P##idx##_count = m_getptr(P##idx)->size();                                                               \
   for(i = 0; i < P##idx##_count; ++i)                                                                      \
   {                                                                                                        \
      if(m_cmp##cmp((m_getvalt(m_getptr(P##idx))::element_type*)P##idx##_sim + i, &(*m_getptr(P##idx))[i])) \
      {                                                                                                     \
         error("Channel parameter %u (%u/%u) mismatch with respect to golden reference.\n", idx)" +
       (return_type ? " - 1" : "") +
       R"(, i + 1,     \
               P##idx##_count);                                                                             \
         ++mismatch_count;                                                                                  \
         break;                                                                                             \
      }                                                                                                     \
   }                                                                                                        \
   free(P##idx##_sim)

)" + gold_decl +
       R"(#else
#define m_setargptr __m_setptrarg

#define m_argcmp(...)

#define m_channelcmp(idx, cmp)                                                                                      \
   for(i = 0; i < P##idx##_count; ++i)                                                                              \
   {                                                                                                                \
      memcpy(&(*m_getptr(P##idx))[i], (m_getvalt(m_getptr(P##idx))::element_type*)P##idx##_sim + i, P##idx##_item); \
   }                                                                                                                \
   free(P##idx##_sim)
#endif

#define m_channel_init(idx)                                                                                         \
   const size_t P##idx##_item = sizeof(m_getvalt(m_getptr(P##idx))::element_type);                                  \
   size_t P##idx##_count = m_getptr(P##idx)->size();                                                                \
   m_alloc_param(idx)" +
       (return_type ? " - 1" : "") +
       R"(, P##idx##_count * P##idx##_item);                                                          \
   void* P##idx##_sim = malloc(P##idx##_count * P##idx##_item);                                                     \
   for(i = 0; i < P##idx##_count; ++i)                                                                              \
   {                                                                                                                \
      memcpy((m_getvalt(m_getptr(P##idx))::element_type*)P##idx##_sim + i, &(*m_getptr(P##idx))[i], P##idx##_item); \
   }
)");

   // write C code used to print initialization values for the HDL simulator's memory
   WriteSimulatorInitMemory(top_id);

   indented_output_stream->Append(top_decl);
   indented_output_stream->Append("{\n");
   const auto max_ulp = [&]() -> std::string {
      const auto par = Param->getOption<std::string>(OPT_max_ulp);
      if(par.find(".") != std::string::npos)
      {
         return par + ".0L";
      }
      return par;
   }();
   indented_output_stream->Append("const long double max_ulp = " + max_ulp + ";\n");
   indented_output_stream->Append("size_t i;\n");
   indented_output_stream->Append("enum mdpi_state state;\n");
   indented_output_stream->Append(args_init);
   indented_output_stream->Append(args_decl);
   indented_output_stream->Append("__m_memsetup(args, " + STR(args_decl_size) + ");\n\n");

   indented_output_stream->Append("__m_arg_init(" + STR(args_decl_size) + ");\n");
   indented_output_stream->Append(args_set);

   indented_output_stream->Append("\n__m_signal_to(MDPI_ENTITY_SIM, MDPI_SIM_SETUP);\n\n");
   indented_output_stream->Append("#ifndef CUSTOM_VERIFICATION\n");
   indented_output_stream->Append(gold_call);
   indented_output_stream->Append("#endif\n\n");
   indented_output_stream->Append("state = __m_wait_for(MDPI_ENTITY_COSIM);\n");
   indented_output_stream->Append("__m_arg_fini();\n\n");

   indented_output_stream->Append("if(state != MDPI_COSIM_INIT)\n");
   indented_output_stream->Append("{\n");
   indented_output_stream->Append("error(\"Unexpected simulator state : %s\\n\", mdpi_state_str(state));\n");
   indented_output_stream->Append("__m_signal_to(MDPI_ENTITY_SIM, MDPI_COSIM_END);\n");
   indented_output_stream->Append("pthread_exit((void*)((ptr_t)(MDPI_COSIM_ABORT)));\n");
   indented_output_stream->Append("}\n");

   if(gold_cmp.size() || return_type)
   {
      indented_output_stream->Append(R"(
#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wpointer-type-mismatch"
#else
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpointer-type-mismatch"
#endif

)");
      indented_output_stream->Append("size_t mismatch_count = 0;\n");
      indented_output_stream->Append(gold_cmp + "\n");
      indented_output_stream->Append("#ifndef CUSTOM_VERIFICATION\n");
      if(return_type)
      {
         indented_output_stream->Append("// Compare return value\n");
         indented_output_stream->Append("if(m_cmp" + cmp_type(return_type, "") + "(&retval, &retval_gold))\n");
         indented_output_stream->Append("{\n");
         indented_output_stream->Append("error(\"Return value mismatch with respect to golden reference.\\n\");\n");
         indented_output_stream->Append("++mismatch_count;\n");
         indented_output_stream->Append("}\n\n");
      }
      indented_output_stream->Append(R"(
if(mismatch_count)
{
error("Memory parameter mismatch for %u parameters.\n", mismatch_count);
__m_signal_to(MDPI_ENTITY_SIM, MDPI_COSIM_END);
pthread_exit((void*)((ptr_t)(MDPI_COSIM_ABORT)));
}
else
{
debug("Simulation matches golden reference.\n");
}
#endif
#ifdef __clang__
#pragma clang diagnostic pop
#else
#pragma GCC diagnostic pop
#endif
)");
   }

   if(return_type)
   {
      indented_output_stream->Append("return retval;\n");
   }
   indented_output_stream->Append("}\n\n");

   const auto& test_vectors = hls_c_backend_information->HLSMgr->RSim->test_vectors;
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
}

void HLSCWriter::WriteFile(const std::string& file_name)
{
   const auto top_function_ids = AppM->CGetCallGraphManager()->GetRootFunctions();
   THROW_ASSERT(top_function_ids.size() == 1, "Multiple top function");
   const auto function_id = *(top_function_ids.begin());
   const auto BH = AppM->CGetFunctionBehavior(function_id)->CGetBehavioralHelper();
   INDENT_OUT_MEX(OUTPUT_LEVEL_VERBOSE, output_level,
                  "-->C-based testbench generation for function " + BH->get_function_name() + ": " + file_name);

   WriteMainTestbench();
   INDENT_OUT_MEX(OUTPUT_LEVEL_VERBOSE, output_level, "<--Prepared testbench");

   indented_output_stream->WriteFile(file_name);
}

void HLSCWriter::WriteFunctionImplementation(unsigned int)
{
   /// Do nothing
}

void HLSCWriter::WriteBuiltinWaitCall()
{
   /// Do nothing
}
