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
 *              Copyright (C) 2004-2022 Politecnico di Milano
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

/// Header include
#include "hls_c_writer.hpp"

///. include
#include "Parameter.hpp"

/// behavior include
#include "behavioral_helper.hpp"
#include "call_graph_manager.hpp"
#include "function_behavior.hpp"
#include "var_pp_functor.hpp"

/// circuit include
#include "structural_objects.hpp"

/// design_flows/backend/ToC includes
#include "hls_c_backend_information.hpp"

/// design_flows/backend/ToC/source_code_writer
#include "instruction_writer.hpp"

/// HLS include
#include "hls_manager.hpp"

/// HLS/memory include
#include "memory.hpp"

/// HLS/simulation include
#include "SimulationInformation.hpp"
#include "c_initialization_parser.hpp"
#include "memory_initialization_c_writer.hpp"
#include "testbench_generation.hpp"
#include "testbench_generation_base_step.hpp"
#include "testbench_generation_constants.hpp"

/// STD include
#include <string>

/// STL include
#include "custom_map.hpp"
#include "custom_set.hpp"
#include <boost/filesystem/operations.hpp>
#include <list>
#include <vector>

/// technology/physical_library include
#include "technology_node.hpp"

/// tree include
#include "tree_helper.hpp"
#include "tree_manager.hpp"
#include "tree_node.hpp"
#include "tree_reindex.hpp"

/// utility include
#include "dbgPrintHelper.hpp" // for DEBUG_LEVEL_NONE
#include "indented_output_stream.hpp"
#include "string_manipulation.hpp" // for GET_CLASS

REF_FORWARD_DECL(memory_symbol);

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

HLSCWriter::~HLSCWriter() = default;

void HLSCWriter::WriteHeader()
{
   bool is_discrepancy = (Param->isOption(OPT_discrepancy) && Param->getOption<bool>(OPT_discrepancy)) ||
                         (Param->isOption(OPT_discrepancy_hw) && Param->getOption<bool>(OPT_discrepancy_hw));
   indented_output_stream->Append("#define _FILE_OFFSET_BITS 64\n\n");
   indented_output_stream->Append("#define __Inf (1.0/0.0)\n");
   indented_output_stream->Append("#define __Nan (0.0/0.0)\n\n");
   indented_output_stream->Append("#ifdef __cplusplus\n");
   indented_output_stream->Append("#undef printf\n\n");
   indented_output_stream->Append("#include <cstdio>\n\n");
   indented_output_stream->Append("#include <cstdlib>\n\n");
   indented_output_stream->Append("typedef bool _Bool;\n\n");
   indented_output_stream->Append("#else\n");
   indented_output_stream->Append("#include <stdio.h>\n\n");
   if(!is_discrepancy)
   {
      indented_output_stream->Append("#include <stdlib.h>\n\n");
   }
   if(Param->getOption<bool>(OPT_no_return_zero))
   {
      indented_output_stream->Append("#include <sys/wait.h>\n\n");
   }
   indented_output_stream->Append("extern void exit(int status);\n");
   indented_output_stream->Append("#endif\n\n");
   indented_output_stream->Append("#include <sys/types.h>\n");

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
         if(inc != "")
         {
            indented_output_stream->Append("#include \"" + inc + "\"\n");
         }
      }
      indented_output_stream->Append("\n");
   }
}

void HLSCWriter::WriteGlobalDeclarations()
{
   instrWriter->write_declarations();
   WriteTestbenchGlobalVars();
   WriteTestbenchHelperFunctions();
}

void HLSCWriter::WriteTestbenchGlobalVars()
{
   // global variables for testbench
   indented_output_stream->Append("//global variable used to store the output file\n");
   indented_output_stream->Append("FILE * __bambu_testbench_fp;\n\n");
}

void HLSCWriter::WriteTestbenchHelperFunctions()
{
   indented_output_stream->Append("#ifdef __AC_NAMESPACE\n");
   indented_output_stream->Append("using namespace __AC_NAMESPACE;\n");
   indented_output_stream->Append("#endif\n");
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
      const auto type = tree_helper::PrintType(TM, parm_type);
      const auto param = BH->PrintVariable(par->index);
      hls_c_backend_information->HLSMgr->RSim->simulationArgSignature.push_back(param);

      if(tree_helper::IsVectorType(parm_type))
      {
         THROW_ERROR("parameter " + param + " of function under test " + BH->get_function_name() + " has type " + type +
                     "\nco-simulation does not support vectorized parameters at top level");
      }
      if(tree_helper::IsPointerType(par))
      {
         const var_pp_functorRef var_functor(new std_var_pp_functor(BH));
         const auto type_declaration = tree_helper::PrintType(TM, parm_type, false, false, false, par, var_functor);
         indented_output_stream->Append(type_declaration + ";\n");
      }
      else
      {
         indented_output_stream->Append(type + " " + param + ";\n");
      }
   }
}

void HLSCWriter::WriteParamInitialization(const BehavioralHelperConstRef BH,
                                          const std::map<std::string, std::string>& curr_test_vector,
                                          const unsigned int v_idx)
{
   const auto params = BH->GetParameters();
   for(auto par_idx = 0U; par_idx < params.size(); ++par_idx)
   {
      const auto& par = params.at(par_idx);
      const auto parm_type = tree_helper::CGetType(par);
      const auto param = BH->PrintVariable(GET_INDEX_CONST_NODE(par));
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Writing initialization of " + param);
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                     "---Type: " + GET_CONST_NODE(parm_type)->get_kind_text() + " - " + STR(parm_type));
      const auto init_it = curr_test_vector.find(param);
      if(init_it == curr_test_vector.end())
      {
         THROW_ERROR("Value of " + param + " is missing in test vector");
      }
      const auto& test_v = init_it->second;
      if(tree_helper::IsPointerType(parm_type))
      {
         const auto is_binary_init = test_v.size() > 4 && test_v.substr(test_v.size() - 4) == ".dat";

         std::string var_ptdtype;
         std::string temp_var_decl;
         bool is_a_true_pointer = true;
         if(flag_cpp && !is_binary_init)
         {
            const auto fnode = TM->CGetTreeNode(BH->get_function_index());
            const auto fd = GetPointerS<const function_decl>(fnode);
            const auto fname = tree_helper::GetMangledFunctionName(fd);
            auto& DesignInterfaceTypename = hls_c_backend_information->HLSMgr->design_interface_typename_orig_signature;
            if(DesignInterfaceTypename.find(fname) != DesignInterfaceTypename.end())
            {
               THROW_ASSERT(DesignInterfaceTypename.count(fname), "");
               const auto& DesignInterfaceArgsTypename = DesignInterfaceTypename.at(fname);
               THROW_ASSERT(DesignInterfaceArgsTypename.size() > par_idx, "");
               const auto& argTypename = DesignInterfaceArgsTypename.at(par_idx);
               if(argTypename.back() == '*')
               {
                  var_ptdtype = argTypename.substr(0, argTypename.size() - 1);
                  temp_var_decl = var_ptdtype + " " + param + "_temp[]";
               }
               else
               {
                  is_a_true_pointer = false;
                  var_ptdtype = argTypename.back() == '&' ? argTypename.substr(0, argTypename.size() - 1) : argTypename;
                  temp_var_decl = var_ptdtype + " " + param + "_temp";
               }
            }
         }
         if(temp_var_decl == "")
         {
            const auto var_functor = var_pp_functorRef(new std_var_pp_functor(BH));
            const auto ptd = tree_helper::CGetPointedType(parm_type);
            temp_var_decl = tree_helper::PrintType(TM, ptd, false, false, false, par, var_functor);
            var_ptdtype = temp_var_decl.substr(0, temp_var_decl.find((*var_functor)(par->index)));
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
         THROW_ASSERT(temp_var_decl.size() && var_ptdtype.size(), "");
         const auto ptd_type = tree_helper::GetRealType(tree_helper::CGetPointedType(parm_type));
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                        "---Pointed type: " + GET_CONST_NODE(ptd_type)->get_kind_text() + " - " + STR(ptd_type));
         const auto c_type_cast = [&]()
         {
            auto bare_ptd_type = ptd_type;
            while(tree_helper::IsArrayType(bare_ptd_type))
            {
               bare_ptd_type = tree_helper::CGetElements(bare_ptd_type);
            }
            auto type_name = tree_helper::PrintType(TM, bare_ptd_type);
            if(tree_helper::IsVoidType(bare_ptd_type))
            {
               boost::replace_all(type_name, "void ", "char ");
            }
            return "(" + type_name + "*)";
         }();

         if(is_binary_init)
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Initialized from binary file: " + test_v);
            const auto fp = param + "_fp";
            indented_output_stream->Append("FILE* " + fp + " = fopen(\"" + test_v + "\", \"rb\");\n");
            indented_output_stream->Append("fseek(" + fp + ", 0, SEEK_END);\n");
            indented_output_stream->Append("size_t " + param + "_size = ftell(" + fp + ");\n");
            indented_output_stream->Append("fseek(" + fp + ", 0, SEEK_SET);\n");
            indented_output_stream->Append("unsigned char* " + param + "_buf = (unsigned char*)malloc(" + param +
                                           "_size);\n");
            indented_output_stream->Append("if(fread(" + param + "_buf, 1, " + param + "_size, " + fp +
                                           ") != " + param + "_size)\n");
            indented_output_stream->Append("{\n");
            indented_output_stream->Append("fclose(" + fp + ");\n");
            indented_output_stream->Append("printf(\"Unable to read " + test_v + " to initialize parameter " + param +
                                           "\");\n");
            indented_output_stream->Append("exit(-1);\n");
            indented_output_stream->Append("}\n");
            indented_output_stream->Append("fclose(" + fp + ");\n");
            indented_output_stream->Append(param + " = " + c_type_cast + "" + param + "_buf;\n");
         }
         else
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Inline pointer initialization");
            const auto temp_initialization =
                temp_var_decl + " = " +
                ((test_v.front() != '{' && test_v.back() != '}' && is_a_true_pointer) ? "{" + test_v + "}" : test_v) +
                ";\n";
            indented_output_stream->Append(temp_initialization);
            indented_output_stream->Append(param + " = " + c_type_cast + (is_a_true_pointer ? "" : "&") + param +
                                           "_temp;\n");
         }

         THROW_ASSERT(hls_c_backend_information->HLSMgr->RSim->param_address.at(v_idx).find(GET_INDEX_CONST_NODE(
                          par)) != hls_c_backend_information->HLSMgr->RSim->param_address.at(v_idx).end(),
                      "parameter does not have an address");
         const auto memory_addr =
             STR(hls_c_backend_information->HLSMgr->RSim->param_address.at(v_idx).at(GET_INDEX_CONST_NODE(par)));

         indented_output_stream->Append("fprintf(__bambu_testbench_fp, \"//parameter: " + param +
                                        " value: " + memory_addr + "\\n\");\n");

         indented_output_stream->Append("fprintf(__bambu_testbench_fp, \"p" +
                                        ConvertInBinary(memory_addr, 32, false, false) + "\\n\");\n");
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

         indented_output_stream->Append("fprintf(__bambu_testbench_fp, \"//parameter: " + param + " value: " + test_v +
                                        "\\n\");\n");
         indented_output_stream->Append("fprintf(__bambu_testbench_fp, \"p" +
                                        ConvertInBinary(test_v, parm_type_bitsize, tree_helper::IsRealType(parm_type),
                                                        tree_helper::IsUnsignedIntegerType(parm_type)) +
                                        "\\n\");\n");
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
   const auto decl = std::get<0>(BH->get_definition(BH->get_function_index(), is_system));
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
      for(const auto& p : BH->GetParameters())
      {
         if(!is_first_argument)
         {
            indented_output_stream->Append(", ");
         }
         else
         {
            is_first_argument = false;
         }
         if(flag_cpp && tree_helper::IsPointerType(p))
         {
            const auto fnode = TM->CGetTreeNode(BH->get_function_index());
            const auto fd = GetPointerS<const function_decl>(fnode);
            const auto fname = tree_helper::GetMangledFunctionName(fd);
            const auto& DesignInterfaceTypenameOrig =
                hls_c_backend_information->HLSMgr->design_interface_typename_orig_signature;
            if(DesignInterfaceTypenameOrig.find(fname) != DesignInterfaceTypenameOrig.end())
            {
               const auto arg_typename = DesignInterfaceTypenameOrig.find(fname)->second.at(par_index);
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
         }
         const auto param = BH->PrintVariable(GET_INDEX_CONST_NODE(p));
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

void HLSCWriter::WriteExpectedResults(const BehavioralHelperConstRef BH,
                                      const std::map<std::string, std::string>& curr_test_vector, const unsigned v_idx)
{
   const auto interface_type = Param->getOption<HLSFlowStep_Type>(OPT_interface_type);
   const auto fnode = TM->CGetTreeReindex(BH->get_function_index());
   const auto fd = GetPointerS<const function_decl>(GET_CONST_NODE(fnode));
   const auto fname = tree_helper::GetMangledFunctionName(fd);
   const auto& DesignInterfaceTypename = hls_c_backend_information->HLSMgr->design_interface_typename_orig_signature;
   const auto& DesignInterfaceSpecifier = hls_c_backend_information->HLSMgr->design_interface;
   const auto hasInterface = DesignInterfaceTypename.find(fname) != DesignInterfaceTypename.end();
   const auto hasSpecifier = DesignInterfaceSpecifier.find(fname) != DesignInterfaceSpecifier.end();

   const auto params = BH->GetParameters();
   for(auto par_idx = 0U; par_idx < params.size(); ++par_idx)
   {
      const auto& par = params.at(par_idx);
      const auto param = BH->PrintVariable(GET_INDEX_CONST_NODE(par));
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Generating code for expected results of " + param);
      if(tree_helper::IsPointerType(par))
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Pointer parameter");
         const auto& test_v = curr_test_vector.at(param);
         bool reference_type_p = false;
         const auto base_type = tree_helper::CGetType(par);
         const auto ptd_type = tree_helper::CGetPointedType(base_type);
         auto is_ac_type = false;
         const auto ptd_type_bitsize = [&]()
         {
            if(hasInterface)
            {
               const auto arg_typename = DesignInterfaceTypename.at(fname).at(par_idx);
               bool is_signed, is_fixed;
               auto ac_bitwidth = ac_type_bitwidth(arg_typename, is_signed, is_fixed);
               if(ac_bitwidth)
               {
                  is_ac_type = true;
                  return ac_bitwidth;
               }
            }
            return tree_helper::Size(ptd_type);
         }();
         const auto arg_typename = [&]()
         {
            if(hasInterface)
            {
               auto type_name = DesignInterfaceTypename.at(fname).at(par_idx);
               if(type_name.back() == '&')
               {
                  return type_name.substr(0, type_name.size() - 1U) + "*";
               }
               else if(type_name.back() != '*')
               {
                  // TODO: Frontend replace struct/class passed by value with pointers, remove this when fixed
                  return type_name + "*";
               }
               return type_name;
            }
            return tree_helper::PrintType(TM, base_type);
         }();
         const auto param_cast = "((" + arg_typename + ")" + param + ")";

         if(interface_type == HLSFlowStep_Type::INFERRED_INTERFACE_GENERATION)
         {
            std::vector<std::string> splitted = SplitString(test_v, ",");

            if(tree_helper::IsRealType(ptd_type) || is_ac_type)
            {
               indented_output_stream->Append("for (__testbench_index2 = 0; __testbench_index2 < " +
                                              STR(splitted.size()) + "; ++__testbench_index2)\n{\n");
               if(output_level > OUTPUT_LEVEL_MINIMUM)
               {
                  indented_output_stream->Append(
                      "fprintf(__bambu_testbench_fp, \"//expected value for output " + param +
                      "[__testbench_index2]: " + (tree_helper::IsRealType(ptd_type) ? "%g" : "%d") + "\\n\", " +
                      (tree_helper::IsRealType(ptd_type) ? "" : "(int)") + param_cast + "[__testbench_index2]);\n");
               }
               indented_output_stream->Append("fprintf(__bambu_testbench_fp, \"o\");\n");
               indented_output_stream->Append("_Ptd2Bin_(__bambu_testbench_fp, (unsigned char*)&" + param +
                                              "[__testbench_index2], " + STR(ptd_type_bitsize) + ");\n");
               indented_output_stream->Append("fprintf(__bambu_testbench_fp, \"\\n\");\n");
               indented_output_stream->Append("}\n");
            }
            else if(tree_helper::IsArrayType(ptd_type) && !tree_helper::IsStructType(ptd_type) &&
                    !tree_helper::IsUnionType(ptd_type))
            {
               indented_output_stream->Append("for (__testbench_index2 = 0; __testbench_index2 < " +
                                              STR(splitted.size()) + "; ++__testbench_index2)\n{\n");
               const auto data_bitsize = tree_helper::GetArrayElementSize(ptd_type);
               const auto num_elements = tree_helper::GetArrayTotalSize(ptd_type);
               const auto elmts_type = [&]()
               {
                  auto t = ptd_type;
                  while(tree_helper::IsArrayType(t))
                  {
                     t = tree_helper::CGetElements(t);
                  }
                  return t;
               }();
               const auto elmts_typename = tree_helper::PrintType(TM, elmts_type);
               if(output_level > OUTPUT_LEVEL_MINIMUM)
               {
                  indented_output_stream->Append("fprintf(__bambu_testbench_fp, \"//expected value for output (*(((" +
                                                 elmts_typename + "*)" + param + ")+ __testbench_index2)): %" +
                                                 (tree_helper::IsRealType(elmts_type) ? "g" : "d") + "\\n\", (*(((" +
                                                 elmts_typename + "*)" + param + ")+ __testbench_index2)));\n");
               }
               indented_output_stream->Append("fprintf(__bambu_testbench_fp, \"o\");\n");
               if(splitted.size() == 1)
               {
                  for(unsigned int l = 0; l < num_elements; l++)
                  {
                     if(tree_helper::IsRealType(elmts_type))
                     {
                        indented_output_stream->Append("_Ptd2Bin_(__bambu_testbench_fp, (unsigned char*)&(*" + param +
                                                       ")[" + STR(l) + "], " + STR(data_bitsize) + ");\n");
                     }
                     else
                     {
                        indented_output_stream->Append("_Dec2Bin_(__bambu_testbench_fp, (*" + param + ")[" + STR(l) +
                                                       "], " + STR(data_bitsize) + ");\n");
                     }
                  }
               }
               else
               {
                  if(tree_helper::IsRealType(elmts_type))
                  {
                     indented_output_stream->Append("_Ptd2Bin_(__bambu_testbench_fp, (unsigned char*)&(*(((" +
                                                    elmts_typename + "*)" + param + ")+ __testbench_index2)), " +
                                                    STR(data_bitsize) + ");\n");
                  }
                  else
                  {
                     indented_output_stream->Append("_Dec2Bin_(__bambu_testbench_fp, (*(((" + elmts_typename + "*)" +
                                                    param + ") + __testbench_index2)), " + STR(data_bitsize) + ");\n");
                  }
               }
               indented_output_stream->Append("fprintf(__bambu_testbench_fp, \"\\n\");\n");
               indented_output_stream->Append("}\n");
            }
            else
            {
               const auto interfaceSpecifier = hasSpecifier ? DesignInterfaceSpecifier.at(fname).at(param) : "";
               /* m_axi interfaces require writing the full results in a row, not a single byte */
               if(interfaceSpecifier == "m_axi")
               {
                  /// Retrieve the space to be reserved in memory
                  const auto reserved_mem_bytes =
                      hls_c_backend_information->HLSMgr->RSim->param_mem_size.at(v_idx).at(GET_INDEX_CONST_NODE(par));
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                 "---Reserved memory " + STR(reserved_mem_bytes) + " bytes");
                  const auto element_size = ptd_type_bitsize / 8;
                  THROW_ASSERT(reserved_mem_bytes % element_size == 0,
                               STR(reserved_mem_bytes) + "/" + STR(element_size));
                  const auto num_elements = reserved_mem_bytes / element_size;
                  THROW_ASSERT(num_elements, STR(reserved_mem_bytes) + "/" + STR(element_size));
                  indented_output_stream->Append("{\n");
                  if(num_elements > 1 || !reference_type_p)
                  {
                     indented_output_stream->Append("for(unsigned int i0 = 0; i0 < " + STR(num_elements) + "; i0++)\n");
                     indented_output_stream->Append("{\n");
                  }
                  WriteParamInMemory(BH, param + (reference_type_p ? "" : "[i0]"), GET_INDEX_CONST_NODE(ptd_type), 1,
                                     false, false);
                  if(num_elements > 1 || !reference_type_p)
                  {
                     indented_output_stream->Append("}\n");
                  }
                  indented_output_stream->Append("}\n");
               }
               else
               {
                  indented_output_stream->Append("for (__testbench_index2 = 0; __testbench_index2 < " +
                                                 STR(splitted.size()) + "; ++__testbench_index2)\n{\n");
                  if(output_level > OUTPUT_LEVEL_MINIMUM)
                  {
                     indented_output_stream->Append("fprintf(__bambu_testbench_fp, \"//expected value for output " +
                                                    param + "[%d]: %" +
                                                    (tree_helper::IsRealType(ptd_type) ? "g" : "d") +
                                                    "\\n\", __testbench_index2, " + param + "[__testbench_index2]);\n");
                  }
                  indented_output_stream->Append("fprintf(__bambu_testbench_fp, \"o\");\n");
                  indented_output_stream->Append("_Dec2Bin_(__bambu_testbench_fp, " + param + "[__testbench_index2], " +
                                                 STR(ptd_type_bitsize) + ");\n");
                  indented_output_stream->Append("fprintf(__bambu_testbench_fp, \"\\n\");\n");
                  indented_output_stream->Append("}\n");
               }
            }
            indented_output_stream->Append("fprintf(__bambu_testbench_fp, \"e\\n\");\n");
         }
         else
         {
            /// Retrieve the space to be reserved in memory
            const auto reserved_mem_bytes =
                hls_c_backend_information->HLSMgr->RSim->param_mem_size.at(v_idx).at(GET_INDEX_CONST_NODE(par));
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                           "---Reserved memory " + STR(reserved_mem_bytes) + " bytes");
            const auto element_size = ptd_type_bitsize / 8;
            THROW_ASSERT(reserved_mem_bytes % element_size == 0, STR(reserved_mem_bytes) + "/" + STR(element_size));
            const auto num_elements = reserved_mem_bytes / element_size;
            THROW_ASSERT(num_elements, STR(reserved_mem_bytes) + "/" + STR(element_size));
            indented_output_stream->Append("{\n");
            indented_output_stream->Append("int i0=0;\n");
            if(num_elements > 1)
            {
               indented_output_stream->Append("for(i0 = 0; i0 < " + STR(num_elements) + "; ++i0)\n");
               indented_output_stream->Append("{\n");
            }
            WriteParamInMemory(BH, param + "[i0]", GET_INDEX_CONST_NODE(ptd_type), 1, false, false);
            if(num_elements > 1)
            {
               indented_output_stream->Append("}\n");
            }
            indented_output_stream->Append("fprintf(__bambu_testbench_fp, \"e\\n\");\n");
            indented_output_stream->Append("}\n");
         }
      }
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Generated code for expected results of " + param);
   }

   const auto ret_type = tree_helper::GetFunctionReturnType(fnode);
   if(ret_type)
   {
      if(output_level > OUTPUT_LEVEL_MINIMUM)
      {
         indented_output_stream->Append("fprintf(__bambu_testbench_fp, \"//expected value for return value\\n\");\n");
      }
      indented_output_stream->Append("fprintf(__bambu_testbench_fp, \"o\");\n");
      const auto ret_type_bitsize = tree_helper::Size(ret_type);
      if(tree_helper::IsRealType(ret_type) || tree_helper::IsStructType(ret_type) || tree_helper::IsUnionType(ret_type))
      {
         indented_output_stream->Append("_Ptd2Bin_(__bambu_testbench_fp, (unsigned char*)&" +
                                        std::string(RETURN_PORT_NAME) + ", " + STR(ret_type_bitsize) + ");\n");
      }
      else
      {
         indented_output_stream->Append("_Dec2Bin_(__bambu_testbench_fp, " + std::string(RETURN_PORT_NAME) + ", " +
                                        STR(ret_type_bitsize) + ");\n");
      }
      indented_output_stream->Append("fprintf(__bambu_testbench_fp, \"\\n\");\n");
      indented_output_stream->Append("fprintf(__bambu_testbench_fp, \"e\\n\");\n");
   }
}

void HLSCWriter::WriteSimulatorInitMemory(const unsigned int function_id)
{
   CInitializationParserRef c_initialization_parser(new CInitializationParser(Param));
   const auto BH = AppM->CGetFunctionBehavior(function_id)->CGetBehavioralHelper();
   // print base address
   unsigned long long int base_address = hls_c_backend_information->HLSMgr->base_address;
   indented_output_stream->Append("fprintf(__bambu_testbench_fp, \"//base address " + STR(base_address) + "\\n\");\n");
   std::string trimmed_value;
   for(unsigned int ind = 0; ind < 32; ind++)
   {
      trimmed_value = trimmed_value + (((1LLU << (31 - ind)) & base_address) ? '1' : '0');
   }
   indented_output_stream->Append("fprintf(__bambu_testbench_fp, \"b" + trimmed_value + "\\n\");\n");

   const std::map<unsigned int, memory_symbolRef>& mem_vars =
       hls_c_backend_information->HLSMgr->Rmem->get_ext_memory_variables();
   // get the mapping between variables in external memory and their external
   // base address
   std::map<unsigned long long int, unsigned int> address;
   for(const auto& m : mem_vars)
   {
      address[hls_c_backend_information->HLSMgr->Rmem->get_external_base_address(m.first)] = m.first;
   }

   std::list<unsigned int> mem;
   std::transform(address.begin(), address.end(), std::back_inserter(mem),
                  [](const std::pair<unsigned long long, unsigned int>& ma) { return ma.second; });

   const auto fname =
       tree_helper::GetMangledFunctionName(GetPointerS<const function_decl>(TM->CGetTreeNode(function_id)));
   const auto& DesignInterfaceTypename = hls_c_backend_information->HLSMgr->design_interface_typename;
   const auto DesignInterfaceArgsTypename_it = DesignInterfaceTypename.find(fname);

   std::vector<unsigned int> mem_interface;
   const auto& parameters = BH->get_parameters();
   for(const auto& p : parameters)
   {
      // if the function has some pointer parameters some memory needs to be
      // reserved for the place where they point to
      if(tree_helper::is_a_pointer(TM, p) && mem_vars.find(p) == mem_vars.end())
      {
         mem.push_back(p);
         mem_interface.push_back(p);
      }
   }

   unsigned int v_idx = 0;
   // loop on the test vectors
   for(const auto& curr_test_vector : hls_c_backend_information->HLSMgr->RSim->test_vectors)
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Considering test vector " + STR(v_idx));
      indented_output_stream->Append("{\n");
      // loop on the variables in memory
      for(const auto& l : mem)
      {
         std::string param = BH->PrintVariable(l);
         INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "-->Considering memory variable '" + param + "'");
         const auto is_interface = std::find(parameters.begin(), parameters.end(), l) != parameters.end();
         std::string argTypename = "";
         if(DesignInterfaceArgsTypename_it != DesignInterfaceTypename.end() && is_interface)
         {
            THROW_ASSERT(DesignInterfaceArgsTypename_it->second.count(param),
                         "Parameter should be present in design interface.");
            argTypename = DesignInterfaceArgsTypename_it->second.at(param) + " ";
            if(argTypename.find("fixed") == std::string::npos)
            {
               argTypename = "";
            }
         }
         if(param[0] == '"')
         {
            param = "@" + STR(l);
         }

         bool is_memory = false;
         std::string test_v;
         bool binary_test_v = false;
         if(mem_vars.find(l) != mem_vars.end() && !is_interface)
         {
            is_memory = true;
            test_v = TestbenchGenerationBaseStep::print_var_init(TM, l, hls_c_backend_information->HLSMgr->Rmem);
            if(output_level > OUTPUT_LEVEL_MINIMUM)
            {
               indented_output_stream->Append("fprintf(__bambu_testbench_fp, \"//memory initialization for variable " +
                                              param + "\\n\");\n");
            }
         }
         else if(curr_test_vector.find(param) != curr_test_vector.end())
         {
            test_v = curr_test_vector.find(param)->second;
            if(test_v.size() < 4 || test_v.substr(test_v.size() - 4) != ".dat")
            {
               if(flag_cpp)
               {
                  /// Remove leading spaces
                  test_v.erase(0, test_v.find_first_not_of(" \t"));
                  /// Remove trailing spaces
                  auto last_character = test_v.find_last_not_of(" \t");
                  if(std::string::npos != last_character)
                  {
                     test_v.erase(last_character + 1);
                  }
                  /// Remove first {
                  if(test_v.front() == '{')
                  {
                     test_v.erase(0, 1);
                  }
                  /// Remove last }
                  if(test_v.back() == '}')
                  {
                     test_v.pop_back();
                  }
               }
               else
               {
                  if(test_v.front() != '{' && test_v.back() != '}')
                  {
                     test_v = std::string("{") + test_v;
                     test_v = test_v + "}";
                  }
               }
            }
            else
            {
               binary_test_v = true;
            }
         }
         else if(flag_cpp)
         {
            test_v = "0";
         }
         else
         {
            test_v = "{0}";
         }

         if(v_idx > 0 && is_memory)
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "<--Skip memory variable " + param);
            continue; // memory has been already initialized
         }

         /// Retrieve the space to be reserved in memory
         const auto reserved_mem_bytes = [&]() -> size_t
         {
            if(is_memory)
            {
               const auto ret_value = tree_helper::Size(TM->CGetTreeReindex(l)) / 8;
               return ret_value ? ret_value : 1;
            }
            else
            {
               THROW_ASSERT(hls_c_backend_information->HLSMgr->RSim->param_mem_size.count(v_idx), "");
               THROW_ASSERT(hls_c_backend_information->HLSMgr->RSim->param_mem_size.at(v_idx).count(l), "");
               return hls_c_backend_information->HLSMgr->RSim->param_mem_size.at(v_idx).at(l);
            }
         }();
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                        "---Symbol: " + param + " Reserved memory " + STR(reserved_mem_bytes) + " - Test vector is " +
                            test_v);

         if(binary_test_v)
         {
            const auto fp = param + "_fp_local";
            indented_output_stream->Append("FILE* " + fp + " = fopen(\"" + test_v + "\", \"rb\");\n");
            indented_output_stream->Append("fseek(" + fp + ", 0, SEEK_END);\n");
            indented_output_stream->Append("size_t " + param + "_size = ftell(" + fp + ");\n");
            indented_output_stream->Append("fseek(" + fp + ", 0, SEEK_SET);\n");
            indented_output_stream->Append("unsigned char* " + param + "_buf_local = (unsigned char*)malloc(" + param +
                                           "_size);\n");
            indented_output_stream->Append("if(fread(" + param + "_buf_local, 1, " + param + "_size, " + fp +
                                           ") != " + param + "_size)\n");
            indented_output_stream->Append("{\n");
            indented_output_stream->Append("fclose(" + fp + ");\n");
            indented_output_stream->Append("printf(\"Unable to read " + test_v + " to initialize parameter " + param +
                                           "\");\n");
            indented_output_stream->Append("exit(-1);\n");
            indented_output_stream->Append("}\n");
            indented_output_stream->Append("fclose(" + fp + ");\n");
            indented_output_stream->Append("for (__testbench_index = 0; __testbench_index < " + param +
                                           "_size; ++__testbench_index){\n");
            indented_output_stream->Append("   fprintf(__bambu_testbench_fp, \"m\");\n");
            indented_output_stream->Append("   _Dec2Bin_(__bambu_testbench_fp," + param +
                                           "_buf_local[__testbench_index], 8);\n");
            indented_output_stream->Append("   fprintf(__bambu_testbench_fp, \"\\n\");\n");
            indented_output_stream->Append("}\n");
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                           "---Using a binary file for " + param + " - " + test_v);
         }
         else if(flag_cpp || is_memory) /// FIXME: for c++ code the old code is still used
         {
            size_t printed_bytes = 0;
            std::string bits_offset = "";
            std::vector<std::string> splitted = SplitString(test_v, ",");
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Processing c++ init " + test_v);
            const auto isAllZero = [&]() -> bool
            {
               if(splitted.size() == 0)
               {
                  return false;
               }
               const auto first_string = splitted.at(0);
               const auto size_vec = first_string.size();
               if(size_vec % 8)
               {
                  return false;
               }
               for(auto strIndex = 0u; strIndex < size_vec; ++strIndex)
               {
                  if(first_string.at(strIndex) != '0')
                  {
                     return false;
                  }
               }
               for(unsigned int i = 1; i < splitted.size(); i++)
               {
                  if(first_string != splitted.at(i))
                  {
                     return false;
                  }
               }
               return true;
            }();
            if(is_memory && isAllZero)
            {
               auto nZeroBytes = splitted.size() * (splitted.at(0).size() / 8);
               printed_bytes += nZeroBytes;
               indented_output_stream->Append("for (__testbench_index = 0; "
                                              "__testbench_index < " +
                                              STR(nZeroBytes) + "; " +
                                              "++__testbench_index)\n"
                                              "   fprintf(__bambu_testbench_fp, \"m00000000\\n\");\n");
            }
            else
            {
               if(is_memory && test_v.size() > DATA_SIZE_THRESHOLD)
               {
                  std::string param_name = param;
                  boost::replace_all(param_name, "@", "_");
                  auto output_parameter_initialization_filename =
                      Param->getOption<std::string>(OPT_output_directory) + "/simulation/";
                  unsigned int progressive = 0;
                  std::string candidate_out_file_name;
                  do
                  {
                     candidate_out_file_name = output_parameter_initialization_filename + param_name + "_" +
                                               std::to_string(progressive++) + ".data";
                  } while(boost::filesystem::exists(candidate_out_file_name));
                  output_parameter_initialization_filename = candidate_out_file_name;
                  std::ofstream parameter_init_file(output_parameter_initialization_filename.c_str());
                  for(const auto& initial_string : splitted)
                  {
                     THROW_ASSERT(initial_string != "", "Not well formed test vector: " + test_v);
                     printed_bytes +=
                         WriteBinaryMemoryInitToFile(parameter_init_file, initial_string,
                                                     static_cast<unsigned int>(initial_string.size()), bits_offset);
                  }
                  indented_output_stream->Append("{\n");
                  indented_output_stream->Append("FILE * __bambu_testbench_fp_local_copy;\n");
                  indented_output_stream->Append("char * line = NULL;\n");
                  indented_output_stream->Append("size_t len = 0;\n");
                  indented_output_stream->Append("ssize_t read;\n");
                  indented_output_stream->Append("__bambu_testbench_fp_local_copy = fopen(\"" +
                                                 output_parameter_initialization_filename + "\", \"r\");\n");
                  indented_output_stream->Append("if (__bambu_testbench_fp_local_copy == NULL)\n");
                  indented_output_stream->Append("   exit(1);\n");
                  indented_output_stream->Append(
                      "while ((read = getline(&line, &len, __bambu_testbench_fp_local_copy)) != -1) {\n");
                  indented_output_stream->Append("   fprintf(__bambu_testbench_fp, \"%s\", line);\n");
                  indented_output_stream->Append("}\n");
                  indented_output_stream->Append("fclose(__bambu_testbench_fp_local_copy);\n");
                  indented_output_stream->Append("if (line)\n");
                  indented_output_stream->Append("   free(line);\n");
                  indented_output_stream->Append("}\n");
               }
               else
               {
                  for(unsigned int i = 0; i < splitted.size(); i++)
                  {
                     THROW_ASSERT(splitted[i] != "", "Not well formed test vector: " + test_v);
                     std::string initial_string = splitted[i];

                     if(is_memory)
                     {
                        printed_bytes += WriteBinaryMemoryInit(
                            initial_string, static_cast<unsigned int>(initial_string.size()), bits_offset);
                     }
                     else
                     {
                        const auto base_type = tree_helper::CGetType(TM->CGetTreeReindex(l));
                        std::string binary_string;
                        std::string init_value_copy = initial_string;
                        boost::replace_all(init_value_copy, "\\", "\\\\");
                        boost::replace_all(init_value_copy, "\n", "");
                        boost::replace_all(init_value_copy, "\"", "");
                        if(output_level > OUTPUT_LEVEL_MINIMUM)
                        {
                           indented_output_stream->Append(
                               "fprintf(__bambu_testbench_fp, \"//memory initialization for variable " +
                               param /*+ " value: " + init_value_copy */ + "\\n\");\n");
                        }
                        tree_nodeConstRef ptd_base_type_node;
                        if(GET_CONST_NODE(base_type)->get_kind() == pointer_type_K)
                        {
                           ptd_base_type_node = GetPointerS<const pointer_type>(GET_CONST_NODE(base_type))->ptd;
                        }
                        else if(GET_CONST_NODE(base_type)->get_kind() == reference_type_K)
                        {
                           ptd_base_type_node = GetPointerS<const reference_type>(GET_CONST_NODE(base_type))->refd;
                        }
                        else
                        {
                           THROW_ERROR("A pointer type is expected");
                        }
                        INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                       "---Pointed base type is " +
                                           GET_CONST_NODE(ptd_base_type_node)->get_kind_text());
                        auto data_bitsize = tree_helper::Size(ptd_base_type_node);
                        if(tree_helper::IsStructType(ptd_base_type_node))
                        {
                           const auto splitted_fields = SplitString(initial_string, "|");
                           const auto fields = tree_helper::CGetFieldTypes(ptd_base_type_node);
                           const auto n_values = splitted_fields.size();
                           unsigned int index = 0;
                           for(auto it = fields.begin(); it != fields.end(); ++it, ++index)
                           {
                              const auto field_type = *it;
                              const auto field_size = tree_helper::Size(field_type);
                              if(index < n_values)
                              {
                                 binary_string = ConvertInBinary(argTypename + splitted_fields[index], field_size,
                                                                 tree_helper::IsRealType(field_type),
                                                                 tree_helper::IsUnsignedIntegerType(field_type));
                              }
                              else
                              {
                                 binary_string =
                                     ConvertInBinary(argTypename + "0", field_size, tree_helper::IsRealType(field_type),
                                                     tree_helper::IsUnsignedIntegerType(field_type));
                              }

                              printed_bytes += WriteBinaryMemoryInit(binary_string, field_size, bits_offset);
                           }
                        }
                        else if(tree_helper::IsUnionType(ptd_base_type_node))
                        {
                           const auto max_bitsize_field = tree_helper::AccessedMaximumBitsize(ptd_base_type_node, 0);
                           binary_string = ConvertInBinary(argTypename + "0", max_bitsize_field, false, false);
                           printed_bytes += WriteBinaryMemoryInit(binary_string, max_bitsize_field, bits_offset);
                        }
                        else if(tree_helper::IsArrayType(ptd_base_type_node))
                        {
                           auto elmts_type = tree_helper::CGetElements(ptd_base_type_node);
                           while(tree_helper::IsArrayType(elmts_type))
                           {
                              elmts_type = tree_helper::CGetElements(elmts_type);
                           }

                           data_bitsize = tree_helper::GetArrayElementSize(ptd_base_type_node);

                           auto num_elements = 1ull;
                           if(splitted.size() == 1)
                           {
                              num_elements = tree_helper::GetArrayTotalSize(ptd_base_type_node);
                           }

                           indented_output_stream->Append("for (__testbench_index0 = 0; __testbench_index0 < " +
                                                          STR(num_elements) + "; ++__testbench_index0)\n{\n");

                           binary_string = ConvertInBinary(argTypename + initial_string, data_bitsize,
                                                           tree_helper::IsRealType(elmts_type),
                                                           tree_helper::IsUnsignedIntegerType(elmts_type));
                           printed_bytes += WriteBinaryMemoryInit(binary_string, data_bitsize, bits_offset);
                           indented_output_stream->Append("}\n");
                        }
                        else
                        {
                           binary_string = ConvertInBinary(argTypename + initial_string, data_bitsize,
                                                           tree_helper::IsRealType(ptd_base_type_node),
                                                           tree_helper::IsUnsignedIntegerType(ptd_base_type_node));

                           if(data_bitsize == 1)
                           {
                              data_bitsize = 8;
                              binary_string = "0000000" + binary_string;
                           }
                           else
                           {
                              THROW_ASSERT(data_bitsize % 8 == 0, "unexpected case");
                           }

                           printed_bytes += WriteBinaryMemoryInit(binary_string, data_bitsize, bits_offset);
                        }
                     }
                  }
               }
            }

            if(bits_offset.size())
            {
               std::string tail_padding;
               for(auto tail_padding_ind = bits_offset.size(); tail_padding_ind < 8; ++tail_padding_ind)
               {
                  tail_padding += "0";
               }
               tail_padding = tail_padding + bits_offset;
               bits_offset = "";
               ++printed_bytes;
               indented_output_stream->Append("fprintf(__bambu_testbench_fp, \"m" + tail_padding + "\\n\");\n");
            }
            if(reserved_mem_bytes > printed_bytes)
            {
               indented_output_stream->Append("// reserved_mem_bytes > printed_bytes\n");
               WriteZeroedBytes(reserved_mem_bytes - printed_bytes);
            }
         }
         else
         {
            /// Call the parser to translate C initialization to values.txt initialization
            const auto type = tree_helper::CGetType(TM->CGetTreeReindex(l));
            const CInitializationParserFunctorRef c_initialization_parser_functor(
                new MemoryInitializationCWriter(indented_output_stream, TM, BH, reserved_mem_bytes, type,
                                                TestbenchGeneration_MemoryType::MEMORY_INITIALIZATION, Param));
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                           "---Parsing initialization of " + param + "(" + GET_CONST_NODE(type)->get_kind_text() +
                               " - " + STR(type->index) + "): " + test_v);
            c_initialization_parser->Parse(c_initialization_parser_functor, test_v);
         }

         THROW_ASSERT(hls_c_backend_information->HLSMgr->RSim->param_next_off.count(v_idx), "");
         THROW_ASSERT(hls_c_backend_information->HLSMgr->RSim->param_next_off.at(v_idx).count(l), "");
         const auto next_object_offset = hls_c_backend_information->HLSMgr->RSim->param_next_off.at(v_idx).at(l);
         if(next_object_offset > reserved_mem_bytes)
         {
            indented_output_stream->Append("// next_object_offset > reserved_mem_bytes\n");
            WriteZeroedBytes(next_object_offset - reserved_mem_bytes);
         }
         INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "<--Considered memory variable '" + param + "'");
      }
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Considered test vector " + STR(v_idx));
      ++v_idx;
      indented_output_stream->Append("}\n");
   }
}

void HLSCWriter::WriteExtraInitCode()
{
}

void HLSCWriter::WriteExtraCodeBeforeEveryMainCall()
{
}

void HLSCWriter::WriteMainTestbench()
{
   // get the root function to be tested by the testbench
   const auto top_function_ids = AppM->CGetCallGraphManager()->GetRootFunctions();
   THROW_ASSERT(top_function_ids.size() == 1, "Multiple top function");
   const auto function_id = *(top_function_ids.begin());
   const auto BH = AppM->CGetFunctionBehavior(function_id)->CGetBehavioralHelper();
   const auto return_type = tree_helper::GetFunctionReturnType(TM->CGetTreeReindex(function_id));

   indented_output_stream->Append("#undef main\n");
   indented_output_stream->Append("int main()\n{\n");
   indented_output_stream->Append(
       "unsigned int __testbench_index, __testbench_index0, __testbench_index1, __testbench_index2;\n");
   indented_output_stream->Append("__standard_exit = 0;\n");
   indented_output_stream->Append("__bambu_testbench_fp = fopen(\"" + hls_c_backend_information->results_filename +
                                  "\", \"w\");\n");
   indented_output_stream->Append("if (!__bambu_testbench_fp) {\n");
   indented_output_stream->Append("perror(\"can't open file: " + hls_c_backend_information->results_filename +
                                  "\");\n");
   indented_output_stream->Append("exit(1);\n");
   indented_output_stream->Append("}\n\n");
   // write additional initialization code needed by subclasses
   WriteExtraInitCode();
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Written extra init code");

   // parameters declaration
   WriteParamDecl(BH);

   // write C code used to print initialization values for the HDL simulator's memory
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Writing simulator init memory");
   WriteSimulatorInitMemory(function_id);

   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Written simulator init memory");
   // ---- WRITE VARIABLES DECLARATIONS ----
   // declaration of the return variable of the top function, if not void
   if(return_type)
   {
      const auto ret_type = BH->print_type(return_type->index);
      if(tree_helper::IsVectorType(return_type))
      {
         THROW_ERROR("return type of function under test " + BH->get_function_name() + " is " + STR(ret_type) +
                     "\nco-simulation does not support vectorized return types at top level");
      }

      indented_output_stream->Append("// return variable initialization\n");
      indented_output_stream->Append(ret_type + " " RETURN_PORT_NAME ";\n");
   }
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Written parameters declaration");
   // ---- WRITE PARAMETERS INITIALIZATION, FUNCTION CALLS AND CHECK RESULTS ----
   auto& test_vectors = hls_c_backend_information->HLSMgr->RSim->test_vectors;
   for(unsigned int v_idx = 0; v_idx < test_vectors.size(); v_idx++)
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Writing initialization for test vector " + STR(v_idx));
      indented_output_stream->Append("{\n");
      const auto& curr_test_vector = test_vectors[v_idx];
      // write parameter initialization
      indented_output_stream->Append("// parameter initialization\n");
      WriteParamInitialization(BH, curr_test_vector, v_idx);
      WriteExtraCodeBeforeEveryMainCall();
      // write the call to the top function to be tested
      indented_output_stream->Append("// function call\n");
      WriteTestbenchFunctionCall(BH);
      // write the expected results
      indented_output_stream->Append("// print expected results\n");
      WriteExpectedResults(BH, curr_test_vector, v_idx);
      indented_output_stream->Append("}\n");
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Written initialization for test vector " + STR(v_idx));
   }
   // print exit statements
   indented_output_stream->Append("__standard_exit = 1;\n");
   indented_output_stream->Append("exit(0);\n");
   indented_output_stream->Append("}\n");
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

inline void HLSCWriter::WriteZeroedBytes(const size_t n_bytes)
{
   indented_output_stream->Append("for (__testbench_index = 0; "
                                  "__testbench_index < " +
                                  STR(n_bytes) + "; " +
                                  "++__testbench_index)\n"
                                  "   fprintf(__bambu_testbench_fp, \"m00000000\\n\");\n");
}

size_t HLSCWriter::WriteBinaryMemoryInit(const std::string& binary_string, const size_t data_bitsize,
                                         std::string& bits_offset)
{
   size_t printed_bytes = 0;
   if(bits_offset.size() == 0 && is_all_8zeros(binary_string))
   {
      WriteZeroedBytes(binary_string.size() / 8);
      printed_bytes = binary_string.size() / 8;
   }
   else
   {
      std::string local_binary_string;
      size_t local_data_bitsize;
      if(bits_offset.size())
      {
         if(static_cast<int>(data_bitsize) - 8 + static_cast<int>(bits_offset.size()) >= 0)
         {
            local_data_bitsize = data_bitsize - (8 - bits_offset.size());
            indented_output_stream->Append(
                "fprintf(__bambu_testbench_fp, \"m" +
                binary_string.substr(data_bitsize - (8 - bits_offset.size()), 8 - bits_offset.size()) + bits_offset +
                "\\n\");\n");
            local_binary_string = binary_string.substr(0, local_data_bitsize);
            bits_offset = "";
            printed_bytes++;
         }
         else
         {
            local_data_bitsize = 0;
            bits_offset = binary_string + bits_offset;
         }
      }
      else
      {
         local_binary_string = binary_string;
         local_data_bitsize = data_bitsize;
      }
      for(unsigned int base_index = 0; base_index < local_data_bitsize; base_index = base_index + 8)
      {
         if((static_cast<int>(local_data_bitsize) - 8 - static_cast<int>(base_index)) >= 0)
         {
            indented_output_stream->Append("fprintf(__bambu_testbench_fp, \"m" +
                                           local_binary_string.substr(local_data_bitsize - 8 - base_index, 8) +
                                           "\\n\");\n");
            printed_bytes++;
         }
         else
         {
            bits_offset = local_binary_string.substr(0, local_data_bitsize - base_index);
         }
      }
   }
   return printed_bytes;
}

size_t HLSCWriter::WriteBinaryMemoryInitToFile(std::ofstream& parameter_init_file, const std::string& binary_string,
                                               const size_t data_bitsize, std::string& bits_offset)
{
   size_t printed_bytes = 0;
   std::string local_binary_string;
   size_t local_data_bitsize;
   if(bits_offset.size())
   {
      if(static_cast<int>(data_bitsize) - 8 + static_cast<int>(bits_offset.size()) >= 0)
      {
         local_data_bitsize = data_bitsize - (8 - bits_offset.size());
         parameter_init_file << "m" +
                                    binary_string.substr(data_bitsize - (8 - bits_offset.size()),
                                                         8 - bits_offset.size()) +
                                    bits_offset + "\n";
         local_binary_string = binary_string.substr(0, local_data_bitsize);
         bits_offset = "";
         printed_bytes++;
      }
      else
      {
         local_data_bitsize = 0;
         bits_offset = binary_string + bits_offset;
      }
   }
   else
   {
      local_binary_string = binary_string;
      local_data_bitsize = data_bitsize;
   }
   for(unsigned int base_index = 0; base_index < local_data_bitsize; base_index = base_index + 8)
   {
      if((static_cast<int>(local_data_bitsize) - 8 - static_cast<int>(base_index)) >= 0)
      {
         parameter_init_file << "m" + local_binary_string.substr(local_data_bitsize - 8 - base_index, 8) + "\n";
         printed_bytes++;
      }
      else
      {
         bits_offset = local_binary_string.substr(0, local_data_bitsize - base_index);
      }
   }
   return printed_bytes;
}

bool HLSCWriter::is_all_8zeros(const std::string& str)
{
   size_t size = str.size();
   if(size % 8 != 0 || size == 8)
   {
      return false;
   }
   for(size_t i = 0; i < size; ++i)
   {
      if(str.at(i) != '0')
      {
         return false;
      }
   }
   return true;
}

void HLSCWriter::WriteFunctionImplementation(unsigned int)
{
   /// Do nothing
}

void HLSCWriter::WriteBuiltinWaitCall()
{
   /// Do nothing
}

void HLSCWriter::WriteParamInMemory(const BehavioralHelperConstRef BH, const std::string& param,
                                    const unsigned int type_index, const unsigned int nesting_level, bool input,
                                    bool is_struct_or_union)
{
   const auto type = TM->CGetTreeNode(type_index);
   switch(type->get_kind())
   {
      /// FIXME: real numbers at the moment have to be considered differently because of computation of ulp
      case real_type_K:
      {
         indented_output_stream->Append("fprintf(__bambu_testbench_fp, \"//expected value for output: " + param +
                                        "\\n\");\n");
         const auto size = tree_helper::Size(type);
         if(input || is_struct_or_union) // Checking ULP on expected floating point fields is not possible
         {
            const auto byte_size = tree_helper::Size(type) / 8;
            for(size_t byte = 0; byte < byte_size; byte++)
            {
               indented_output_stream->Append("fprintf(__bambu_testbench_fp, \"" + std::string(input ? "m" : "o") +
                                              "\");\n");
               indented_output_stream->Append("_Ptd2Bin_(__bambu_testbench_fp, ((unsigned char *)&(" + param + ")) + " +
                                              STR(byte) + ", 8);\n");
               indented_output_stream->Append("fprintf(__bambu_testbench_fp, \"\\n\");\n");
            }
         }
         else
         {
            indented_output_stream->Append("fprintf(__bambu_testbench_fp, \"o\");\n");
            indented_output_stream->Append("_Ptd2Bin_(__bambu_testbench_fp, ((unsigned char *)&(" + param + ")), " +
                                           STR(size) + ");\n");
            indented_output_stream->Append("fprintf(__bambu_testbench_fp, \"\\n\");\n");
         }
         break;
      }
      case void_type_K:
      case integer_type_K:
      case pointer_type_K:
      {
         indented_output_stream->Append("fprintf(__bambu_testbench_fp, \"//expected value for output: " + param +
                                        "\\n\");\n");
         const auto byte_size = tree_helper::Size(type) / 8;
         for(size_t byte = 0; byte < byte_size; byte++)
         {
            indented_output_stream->Append("fprintf(__bambu_testbench_fp, \"" + std::string(input ? "m" : "o") +
                                           "\");\n");
            indented_output_stream->Append("_Ptd2Bin_(__bambu_testbench_fp, ((unsigned char *)&(" + param + ")) + " +
                                           STR(byte) + ", 8);\n");
            indented_output_stream->Append("fprintf(__bambu_testbench_fp, \"\\n\");\n");
         }
         break;
      }
      case record_type_K:
      {
         const auto rt = GetPointer<const record_type>(type);
         for(const auto& field : rt->list_of_flds)
         {
            const auto field_param = param + "." + BH->PrintVariable(field->index);
            WriteParamInMemory(BH, field_param, tree_helper::get_type_index(TM, field->index), nesting_level + 1, input,
                               true);
         }
         break;
      }
      case union_type_K:
      {
         const auto ut = GetPointer<const union_type>(type);
         for(const auto& field : ut->list_of_flds)
         {
            const auto field_param = param + "." + BH->PrintVariable(field->index);
            WriteParamInMemory(BH, field_param, tree_helper::get_type_index(TM, field->index), nesting_level + 1, input,
                               true);
            break; // only the first field will be considered
         }
         break;
      }
      case array_type_K:
      {
         const auto at = GetPointer<const array_type>(type);
         const auto array_size = GET_CONST_NODE(at->size);
         const auto array_t = GET_NODE(at->elts);
         THROW_ASSERT(array_size->get_kind() == integer_cst_K, "Size of array is " + array_size->get_kind_text());
         const auto arr_ic = GetPointer<integer_cst>(array_size);
         const auto tn = GetPointer<type_node>(array_t);
         const auto eln_ic = GetPointer<integer_cst>(GET_NODE(tn->size));
         const auto num_elements =
             tree_helper::get_integer_cst_value(arr_ic) / tree_helper::get_integer_cst_value(eln_ic);
         indented_output_stream->Append("{\n");
         const std::string variable_name = "i" + STR(nesting_level);
         indented_output_stream->Append("int " + variable_name + ";\n");
         indented_output_stream->Append("for(" + variable_name + " = 0; " + variable_name + " < " + STR(num_elements) +
                                        "; " + variable_name + "++)\n");
         indented_output_stream->Append("{\n");
         WriteParamInMemory(BH, param + "[" + variable_name + "]", array_t->index, nesting_level + 1, input,
                            is_struct_or_union);
         indented_output_stream->Append("}\n");
         indented_output_stream->Append("}\n");
         break;
      }
      case boolean_type_K:
      case CharType_K:
      case enumeral_type_K:
      case complex_type_K:
      case function_type_K:
      case lang_type_K:
      case method_type_K:
      case nullptr_type_K:
      case offset_type_K:
      case qual_union_type_K:
      case reference_type_K:
      case set_type_K:
      case template_type_parm_K:
      case typename_type_K:
      case type_argument_pack_K:
      case type_pack_expansion_K:
      case vector_type_K:
         THROW_ERROR("Unexpected type in initializing parameter/variable: " + param + " (type " +
                     type->get_kind_text() + ")");
         break;
      case aggr_init_expr_K:
      case binfo_K:
      case block_K:
      case call_expr_K:
      case case_label_expr_K:
      case constructor_K:
      case error_mark_K:
      case identifier_node_K:
      case ssa_name_K:
      case statement_list_K:
      case target_expr_K:
      case target_mem_ref_K:
      case target_mem_ref461_K:
      case tree_list_K:
      case tree_vec_K:
      case lut_expr_K:
      case CASE_CPP_NODES:
      case CASE_BINARY_EXPRESSION:
      case CASE_CST_NODES:
      case CASE_DECL_NODES:
      case CASE_FAKE_NODES:
      case CASE_GIMPLE_NODES:
      case CASE_PRAGMA_NODES:
      case CASE_QUATERNARY_EXPRESSION:
      case CASE_TERNARY_EXPRESSION:
      case CASE_UNARY_EXPRESSION:
      default:
         THROW_ERROR_CODE(NODE_NOT_YET_SUPPORTED_EC, "Not supported node: " + type->get_kind_text());
   }
}
