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

HLSCWriter::HLSCWriter(const HLSCBackendInformationConstRef _hls_c_backend_information, const application_managerConstRef _AppM, const InstructionWriterRef _instruction_writer, const IndentedOutputStreamRef _indented_output_stream,
                       const ParameterConstRef _parameters, bool _verbose)
    : CWriter(_AppM, _instruction_writer, _indented_output_stream, _parameters, _verbose), hls_c_backend_information(_hls_c_backend_information)
{
   /// include from cpp
   flag_cpp = TM->is_CPP() && !Param->isOption(OPT_pretty_print) && (!Param->isOption(OPT_discrepancy) || !Param->getOption<bool>(OPT_discrepancy) || !Param->isOption(OPT_discrepancy_hw) || !Param->getOption<bool>(OPT_discrepancy_hw));
   debug_level = _parameters->get_class_debug_level(GET_CLASS(*this));
}

HLSCWriter::~HLSCWriter() = default;

void HLSCWriter::WriteHeader()
{
   bool is_discrepancy = (Param->isOption(OPT_discrepancy) and Param->getOption<bool>(OPT_discrepancy)) or (Param->isOption(OPT_discrepancy_hw) and Param->getOption<bool>(OPT_discrepancy_hw));
   indented_output_stream->Append("#define _FILE_OFFSET_BITS 64\n\n");
   indented_output_stream->Append("#define __Inf (1.0/0.0)\n");
   indented_output_stream->Append("#define __Nan (0.0/0.0)\n\n");
   indented_output_stream->Append("#ifdef __cplusplus\n");
   indented_output_stream->Append("#include <cstdio>\n\n");
   indented_output_stream->Append("#include <cstdlib>\n\n");
   indented_output_stream->Append("typedef bool _Bool;\n\n");
   indented_output_stream->Append("#else\n");
   indented_output_stream->Append("#include <stdio.h>\n\n");
   if(not is_discrepancy)
      indented_output_stream->Append("#include <stdlib.h>\n\n");
   indented_output_stream->Append("extern void exit(int status);\n");
   indented_output_stream->Append("#endif\n\n");
   indented_output_stream->Append("#include <sys/types.h>\n");

   if(flag_cpp)
   {
      // get the root function to be tested by the testbench
      const auto top_function_ids = AppM->CGetCallGraphManager()->GetRootFunctions();
      THROW_ASSERT(top_function_ids.size() == 1, "Multiple top function");
      const auto function_id = *(top_function_ids.begin());
      auto fnode = TM->get_tree_node_const(function_id);
      auto fd = GetPointer<function_decl>(fnode);
      std::string fname;
      tree_helper::get_mangled_fname(fd, fname);
      auto& DesignInterfaceInclude = hls_c_backend_information->HLSMgr->design_interface_typenameinclude;
      if(DesignInterfaceInclude.find(fname) != DesignInterfaceInclude.end())
      {
         CustomOrderedSet<std::string> includes;
         const auto& DesignInterfaceArgsInclude = DesignInterfaceInclude.find(fname)->second;
         for(auto argInclude : DesignInterfaceArgsInclude)
            includes.insert(argInclude.second);
         for(auto inc : includes)
            if(inc != "")
               indented_output_stream->Append("#include \"" + inc + "\"\n");
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
   indented_output_stream->Append("void _Dec2Bin_(FILE * __bambu_testbench_fp, long long int num, unsigned int precision)\n");
   indented_output_stream->Append("{\n");
   indented_output_stream->Append("unsigned int i;\n");
   indented_output_stream->Append("unsigned long long int ull_value = (unsigned long long int) num;\n");
   indented_output_stream->Append("for (i = 0; i < precision; ++i)\n");
   indented_output_stream->Append("fprintf(__bambu_testbench_fp, \"%c\", (((1LLU << (precision - i -1)) & ull_value) ? '1' : '0'));\n");
   indented_output_stream->Append("}\n\n");
   // pointer to binary conversion function
   indented_output_stream->Append("void _Ptd2Bin_(FILE * __bambu_testbench_fp, unsigned char * num, unsigned int precision)\n");
   indented_output_stream->Append("{\n");
   indented_output_stream->Append("unsigned int i, j;\n");
   indented_output_stream->Append("char value;\n");
   indented_output_stream->Append("if (precision%8)\n");
   indented_output_stream->Append("{\n");
   indented_output_stream->Append("value = *(num+precision/8);\n");
   indented_output_stream->Append("for (j = 8-precision%8; j < 8; ++j)\n");
   indented_output_stream->Append("fprintf(__bambu_testbench_fp, \"%c\", (((1LLU << (8 - j - 1)) & value) ? '1' : '0'));\n");
   indented_output_stream->Append("}\n");
   indented_output_stream->Append("for (i = 0; i < 8*(precision/8); i = i + 8)\n");
   indented_output_stream->Append("{\n");
   indented_output_stream->Append("value = *(num + (precision / 8) - (i / 8) - 1);\n");
   indented_output_stream->Append("for (j = 0; j < 8; ++j)\n");
   indented_output_stream->Append("fprintf(__bambu_testbench_fp, \"%c\", (((1LLU << (8 - j - 1)) & value) ? '1' : '0'));\n");
   indented_output_stream->Append("}\n");
   indented_output_stream->Append("}\n\n");
   if(Param->isOption(OPT_discrepancy) and Param->getOption<bool>(OPT_discrepancy))
   {
      // Builtin floating point checkers
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
      indented_output_stream->Append("else if (binary_abs_c==0X7F800000 || binary_abs_e==0X7F800000 || binary_abs_c>0X7F800000 || binary_abs_e==0X7F800000) return 0;\n");
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
      indented_output_stream->Append("if (binary_abs_c>0X7FF0000000000000 && binary_abs_c>0X7FF0000000000000) return 0;\n");
      indented_output_stream->Append("else if (binary_abs_c==0X7FF0000000000000 && binary_abs_e==0X7FF0000000000000)\n");
      indented_output_stream->Append("{\n");
      indented_output_stream->Append("if ((binary_c>>63) != (binary_e>>63))\n");
      indented_output_stream->Append("return 1;\n");
      indented_output_stream->Append("else\n");
      indented_output_stream->Append("return 0;\n");
      indented_output_stream->Append("}\n");
      indented_output_stream->Append("else if (binary_abs_c==0X7FF0000000000000 || binary_abs_e==0X7FF0000000000000 || binary_abs_c>0X7FF0000000000000 || binary_abs_e==0X7FF0000000000000) return 0;\n");
      indented_output_stream->Append("else\n");
      indented_output_stream->Append("{\n");
      indented_output_stream->Append("if (binary_abs_e == 0) ulp = abs_cme / (*((double *)&denom_0));\n");
      indented_output_stream->Append("else ulp = abs_cme / (*((double *)&denom_e));\n");
      indented_output_stream->Append("return ulp > max_ulp;\n");
      indented_output_stream->Append("}\n");
      indented_output_stream->Append("}\n\n");
      indented_output_stream->Append("void _CheckBuiltinFPs32_(char * chk_str, unsigned char neq, float par_expected, float par_res, float par_a, float par_b)\n");
      indented_output_stream->Append("{\n");
      indented_output_stream->Append("if(neq)\n");
      indented_output_stream->Append("{\n");
      indented_output_stream->Append(
          "printf(\"\\n\\n***********************************************************\\nERROR ON A BASIC FLOATING POINT OPERATION : %s : expected=%a res=%a a=%a b=%a\\n***********************************************************\\n\\n\", chk_str, "
          "par_expected, par_res, par_a, par_b);\n");
      indented_output_stream->Append("exit(1);\n");
      indented_output_stream->Append("}\n");
      indented_output_stream->Append("}\n\n");
      indented_output_stream->Append("void _CheckBuiltinFPs64_(char * chk_str, unsigned char neq, double par_expected, double par_res, double par_a, double par_b)\n");
      indented_output_stream->Append("{\n");
      indented_output_stream->Append("if(neq)\n");
      indented_output_stream->Append("{\n");
      indented_output_stream->Append(
          "printf(\"\\n\\n***********************************************************\\nERROR ON A BASIC FLOATING POINT OPERATION : %s : expected=%a res=%a a=%a b=%a\\n***********************************************************\\n\\n\", chk_str, "
          "par_expected, par_res, par_a, par_b);\n");
      indented_output_stream->Append("exit(1);\n");
      indented_output_stream->Append("}\n");
      indented_output_stream->Append("}\n\n");
   }
}

void HLSCWriter::WriteParamDecl(const BehavioralHelperConstRef behavioral_helper)
{
   std::string type;
   std::string param;

   bool doStandardWay = true;
   hls_c_backend_information->HLSMgr->RSim->simulationArgSignature.clear();
   if(flag_cpp)
   {
      auto fnode = TM->get_tree_node_const(behavioral_helper->get_function_index());
      auto fd = GetPointer<function_decl>(fnode);
      std::string fname;
      tree_helper::get_mangled_fname(fd, fname);
      auto& DesignInterfaceTypename = hls_c_backend_information->HLSMgr->design_interface_typename;
      if(DesignInterfaceTypename.find(fname) != DesignInterfaceTypename.end())
      {
         const auto& DesignInterfaceArgsTypename = DesignInterfaceTypename.find(fname)->second;
         unsigned int pIndex = 0;
         for(const auto& p : behavioral_helper->get_parameters())
         {
            unsigned int type_id = behavioral_helper->get_type(p);
            type = behavioral_helper->print_type(type_id);
            param = behavioral_helper->PrintVariable(p);
            hls_c_backend_information->HLSMgr->RSim->simulationArgSignature.push_back(param);

            if(tree_helper::is_a_vector(TM, type_id))
            {
               THROW_ERROR("parameter " + param + " of function under test " + behavioral_helper->get_function_name() + " has type " + type +
                           "\n"
                           "co-simulation does not support vectorized parameters at top level");
            }
            THROW_ASSERT(DesignInterfaceArgsTypename.find(param) != DesignInterfaceArgsTypename.end(), "unexpected condition");
            auto argTypename = DesignInterfaceArgsTypename.find(param)->second;
            if(argTypename.find("const ") == 0)
               argTypename = argTypename.substr(std::string("const ").size());
            if(argTypename.at(argTypename.size() - 1) == '&')
               argTypename = argTypename.substr(0, argTypename.size() - 1);
            indented_output_stream->Append(argTypename + " " + param + ";\n");
            ++pIndex;
         }
         doStandardWay = false;
      }
   }
   if(doStandardWay)
   {
      indented_output_stream->Append("// parameters declaration\n");
      for(const auto& p : behavioral_helper->get_parameters())
      {
         unsigned int type_id = behavioral_helper->get_type(p);
         type = behavioral_helper->print_type(type_id);
         param = behavioral_helper->PrintVariable(p);
         hls_c_backend_information->HLSMgr->RSim->simulationArgSignature.push_back(param);

         if(tree_helper::is_a_vector(TM, type_id))
         {
            THROW_ERROR("parameter " + param + " of function under test " + behavioral_helper->get_function_name() + " has type " + type +
                        "\n"
                        "co-simulation does not support vectorized parameters at top level");
         }
         if(behavioral_helper->is_a_pointer(p))
         {
            var_pp_functorRef var_functor = var_pp_functorRef(new std_var_pp_functor(behavioral_helper));
            std::string type_declaration = tree_helper::print_type(TM, type_id, false, false, false, p, var_functor);
            if(flag_cpp)
            {
               bool reference_type_p = false;
               tree_nodeRef pt_node = TM->get_tree_node_const(type_id);
               if(pt_node->get_kind() == pointer_type_K)
               {
                  reference_type_p = false;
               }
               else if(pt_node->get_kind() == reference_type_K)
               {
                  reference_type_p = true;
               }
               else
                  THROW_ERROR("A pointer type is expected");
               if(reference_type_p)
                  boost::replace_all(type_declaration, "/*&*/*", "");
            }
            indented_output_stream->Append(type_declaration + ";\n");
         }
         else
         {
            indented_output_stream->Append(type + " " + param + ";\n");
         }
      }
   }
}

void HLSCWriter::WriteParamInitialization(const BehavioralHelperConstRef behavioral_helper, const std::map<std::string, std::string>& curr_test_vector, const unsigned int v_idx)
{
   for(const auto& p : behavioral_helper->get_parameters())
   {
      unsigned int type_id = behavioral_helper->get_type(p);
      std::string type = behavioral_helper->print_type(type_id);
      const std::string param = behavioral_helper->PrintVariable(p);
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Writing initialization of " + param);
      if(behavioral_helper->is_a_pointer(p))
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Pointer");

         std::string test_v = "{0}";
         if(curr_test_vector.find(param) != curr_test_vector.end())
            test_v = curr_test_vector.find(param)->second;

         std::string temp_variable;
         bool is_a_true_pointer = true;
         if(flag_cpp)
         {
            auto fnode = TM->get_tree_node_const(behavioral_helper->get_function_index());
            auto fd = GetPointer<function_decl>(fnode);
            std::string fname;
            tree_helper::get_mangled_fname(fd, fname);
            auto& DesignInterfaceTypename = hls_c_backend_information->HLSMgr->design_interface_typename;
            if(DesignInterfaceTypename.find(fname) != DesignInterfaceTypename.end())
            {
               const auto& DesignInterfaceArgsTypename = DesignInterfaceTypename.find(fname)->second;
               auto argTypename = DesignInterfaceArgsTypename.find(param)->second;
               if((*argTypename.rbegin()) == '*')
               {
                  temp_variable = argTypename.substr(0, argTypename.size() - 1) + " " + param + "_temp[]";
               }
               else
               {
                  is_a_true_pointer = false;
                  if((*argTypename.rbegin()) == '&')
                     temp_variable = argTypename.substr(0, argTypename.size() - 1) + " " + param + "_temp";
                  else
                     temp_variable = argTypename + " " + param + "_temp";
               }
            }
         }
         if(temp_variable == "")
         {
            var_pp_functorRef var_functor = var_pp_functorRef(new std_var_pp_functor(behavioral_helper));
            temp_variable = tree_helper::print_type(TM, tree_helper::get_pointed_type(TM, tree_helper::get_type_index(TM, p)), false, false, false, p, var_functor);
            const auto first_square = temp_variable.find("[");
            if(first_square == std::string::npos)
               temp_variable = temp_variable + "_temp[]";
            else
               temp_variable.insert(first_square, "_temp[]");
         }

         if(test_v.size() > 4 && test_v.substr(test_v.size() - 4) == ".dat")
         {
            var_pp_functorRef var_functor = var_pp_functorRef(new std_var_pp_functor(behavioral_helper));
            std::string type_declaration = tree_helper::print_type(TM, type_id, false, false, false, p, var_functor);
            type_declaration = type_declaration.substr(0, type_declaration.find('*') + 1);
            const auto fp = param + "_fp";
            indented_output_stream->Append("FILE* " + fp + " = fopen(\"" + test_v + "\", \"rb\");\n");
            indented_output_stream->Append("fseek(" + fp + ", 0, SEEK_END);\n");
            indented_output_stream->Append("size_t " + param + "_size = ftell(" + fp + ");\n");
            indented_output_stream->Append("fseek(" + fp + ", 0, SEEK_SET);\n");
            indented_output_stream->Append("unsigned char* " + param + "_buf = (unsigned char*)malloc(" + param + "_size);\n");
            indented_output_stream->Append("if(fread(" + param + "_buf, sizeof *" + param + "_buf, " + param + "_size, " + fp + ") != " + param + "_size)\n");
            indented_output_stream->Append("{\n");
            indented_output_stream->Append("fclose(" + fp + ");\n");
            indented_output_stream->Append("printf(\"Unable to read " + test_v + " to initialise parameter " + param + "\");\n");
            indented_output_stream->Append("exit(-1);\n");
            indented_output_stream->Append("}\n");
            indented_output_stream->Append("fclose(" + fp + ");\n");
            indented_output_stream->Append(param + " = (" + type_declaration + ")" + param + "_buf;\n");
         }
         else
         {
            auto temp_initialization = temp_variable + " = " + ((test_v.front() != '{' && test_v.back() != '}' && is_a_true_pointer) ? "{" + test_v + "}" : test_v) + ";\n";
            indented_output_stream->Append(temp_initialization);
            indented_output_stream->Append(param + " = " + param + "_temp;\n");
         }

         std::string memory_addr;
         THROW_ASSERT(hls_c_backend_information->HLSMgr->RSim->param_address.find(v_idx)->second.find(p) != hls_c_backend_information->HLSMgr->RSim->param_address.find(v_idx)->second.end(), "parameter does not have an address");
         memory_addr = STR(hls_c_backend_information->HLSMgr->RSim->param_address.find(v_idx)->second.find(p)->second);

         indented_output_stream->Append("fprintf(__bambu_testbench_fp, \"//parameter: " + param + " value: " + memory_addr + "\\n\");\n");

         indented_output_stream->Append("fprintf(__bambu_testbench_fp, \"p" + ConvertInBinary(memory_addr, 32, false, false) + "\\n\");\n");

         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--");
      }
      else
      {
         if(curr_test_vector.find(param) == curr_test_vector.end())
            THROW_ERROR("Value of " + param + " is missing in test vector");

         if(type_id && behavioral_helper->is_real(type_id) && curr_test_vector.find(param)->second == "-0")
         {
            if(tree_helper::size(TM, type_id) == 32)
               indented_output_stream->Append(param + " = copysignf(0.0, -1.0);\n");
            else
               indented_output_stream->Append(param + " = copysign(0.0, -1.0);\n");
         }
         else
            indented_output_stream->Append(param + " = " + curr_test_vector.find(param)->second + ";\n");

         indented_output_stream->Append("fprintf(__bambu_testbench_fp, \"//parameter: " + param + " value: " + curr_test_vector.find(param)->second + "\\n\");\n");

         indented_output_stream->Append("fprintf(__bambu_testbench_fp, \"p" + ConvertInBinary(curr_test_vector.find(param)->second, tree_helper::size(TM, type_id), behavioral_helper->is_real(type_id), behavioral_helper->is_unsigned(type_id)) +
                                        "\\n\");\n");
      }
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Written initialization of " + param);
   }
}

void HLSCWriter::WriteTestbenchFunctionCall(const BehavioralHelperConstRef behavioral_helper)
{
   const unsigned int function_index = behavioral_helper->get_function_index();
   const unsigned int return_type_index = behavioral_helper->GetFunctionReturnType(function_index);

   std::string function_name;

   if(flag_cpp)
   {
      tree_nodeRef fd_node = TM->get_tree_node_const(function_index);
      auto* fd = GetPointer<function_decl>(fd_node);
      std::string simple_name;
      tree_nodeRef id_name = GET_NODE(fd->name);
      if(id_name->get_kind() == identifier_node_K)
      {
         auto* in = GetPointer<identifier_node>(id_name);
         if(!in->operator_flag)
            simple_name = in->strg;
      }
      if(simple_name != "")
         function_name = simple_name;
      else
         function_name = behavioral_helper->get_function_name();
   }
   else
      function_name = behavioral_helper->get_function_name();
   // avoid collision with the main
   if(function_name == "main")
   {
      bool is_discrepancy = (Param->isOption(OPT_discrepancy) and Param->getOption<bool>(OPT_discrepancy)) or (Param->isOption(OPT_discrepancy_hw) and Param->getOption<bool>(OPT_discrepancy_hw));
      if(not is_discrepancy)
      {
         function_name = "system";
      }
      else
      {
         function_name = "_main";
      }
   }

   bool is_system;
   std::string decl = std::get<0>(behavioral_helper->get_definition(behavioral_helper->get_function_index(), is_system));
   if((is_system || decl == "<built-in>") && return_type_index && behavioral_helper->is_real(return_type_index))
   {
      indented_output_stream->Append("extern " + behavioral_helper->print_type(return_type_index) + " " + function_name + "(");
      bool is_first_parameter = true;
      for(const auto& p : behavioral_helper->get_parameters())
      {
         if(is_first_parameter)
            is_first_parameter = false;
         else
            indented_output_stream->Append(", ");

         unsigned int type_id = behavioral_helper->get_type(p);
         std::string type = behavioral_helper->print_type(type_id);
         std::string param = behavioral_helper->PrintVariable(p);

         if(behavioral_helper->is_a_pointer(p))
         {
            var_pp_functorRef var_functor = var_pp_functorRef(new std_var_pp_functor(behavioral_helper));
            indented_output_stream->Append(tree_helper::print_type(TM, type_id, false, false, false, p, var_functor));
         }
         else
         {
            indented_output_stream->Append(type + " " + param + "");
         }
      }
      indented_output_stream->Append(");\n");
   }

   if(return_type_index)
      indented_output_stream->Append(std::string(RETURN_PORT_NAME) + " = ");

   indented_output_stream->Append(function_name + "(");
   // function arguments
   if(function_name != "system")
   {
      bool is_first_argument = true;
      for(const auto& p : behavioral_helper->get_parameters())
      {
         if(!is_first_argument)
            indented_output_stream->Append(", ");
         else
            is_first_argument = false;
         std::string param = behavioral_helper->PrintVariable(p);
         indented_output_stream->Append(param);
      }
   }
   else
   {
      indented_output_stream->Append("\"" + Param->getOption<std::string>(OPT_output_directory) + "/simulation/main_exec\"");
   }
   indented_output_stream->Append(");\n");

   if(function_name == "system" and return_type_index)
   {
      if(not Param->getOption<bool>(OPT_no_return_zero))
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

void HLSCWriter::WriteExpectedResults(const BehavioralHelperConstRef behavioral_helper, const std::map<std::string, std::string>& curr_test_vector, const unsigned v_idx)
{
   const HLSFlowStep_Type interface_type = Param->getOption<HLSFlowStep_Type>(OPT_interface_type);
   CInitializationParserRef c_initialization_parser = CInitializationParserRef(new CInitializationParser(Param));
   auto fnode = TM->get_tree_node_const(behavioral_helper->get_function_index());
   auto fd = GetPointer<function_decl>(fnode);
   std::string fname;
   tree_helper::get_mangled_fname(fd, fname);
   auto& DesignInterfaceTypename = hls_c_backend_information->HLSMgr->design_interface_typename;
   bool hasInterface = flag_cpp && DesignInterfaceTypename.find(fname) != DesignInterfaceTypename.end();

   const unsigned int return_type_index = behavioral_helper->GetFunctionReturnType(behavioral_helper->get_function_index());

   for(const auto& p : behavioral_helper->get_parameters())
   {
      std::string param = behavioral_helper->PrintVariable(p);
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Generating code for expected results of " + param);
      if(behavioral_helper->is_a_pointer(p))
      {
         std::string test_v = "{0}";
         if(curr_test_vector.find(param) != curr_test_vector.end())
            test_v = curr_test_vector.find(param)->second;

         /// FIXME: for c++ code the old code is still used
         if(flag_cpp or interface_type == HLSFlowStep_Type::INFERRED_INTERFACE_GENERATION)
         {
            bool reference_type_p = false;
            std::vector<std::string> splitted = SplitString(test_v, ",");

            unsigned int base_type = tree_helper::get_type_index(TM, p);
            tree_nodeRef pt_node = TM->get_tree_node_const(base_type);
            if(pt_node->get_kind() == pointer_type_K)
            {
               reference_type_p = false;
               base_type = GET_INDEX_NODE(GetPointer<pointer_type>(pt_node)->ptd);
            }
            else if(pt_node->get_kind() == reference_type_K)
            {
               reference_type_p = true;
               base_type = GET_INDEX_NODE(GetPointer<reference_type>(pt_node)->refd);
            }
            else
               THROW_ERROR("A pointer type is expected");
            unsigned int base_type_bitsize = tree_helper::size(TM, base_type);
            if(hasInterface)
            {
               auto argTypename = DesignInterfaceTypename.find(fname)->second.find(param)->second;
               if((*argTypename.rbegin()) != '*')
                  reference_type_p = true;
               bool is_signed, is_fixed;
               auto acTypeBw = ac_type_bitwidth(argTypename, is_signed, is_fixed);
               if(acTypeBw)
                  base_type_bitsize = acTypeBw;
            }

            if((behavioral_helper->is_real(base_type) || behavioral_helper->is_a_struct(base_type) || behavioral_helper->is_an_union(base_type)))
            {
               if(splitted.size() == 1 && flag_cpp && reference_type_p)
               {
                  indented_output_stream->Append("fprintf(__bambu_testbench_fp, \"//expected value for output " + param + ": %" + (behavioral_helper->is_real(base_type) ? std::string("g") : std::string("d")) + "\\n\", " +
                                                 (behavioral_helper->is_real(base_type) ? std::string("") : std::string("(int)")) + param + ");\n");
                  indented_output_stream->Append("fprintf(__bambu_testbench_fp, \"o\");\n");
                  indented_output_stream->Append("_Ptd2Bin_(__bambu_testbench_fp, (unsigned char*)&" + param + ", " + STR(base_type_bitsize) + ");\n");
                  indented_output_stream->Append("fprintf(__bambu_testbench_fp, \"\\n\");\n");
               }
               else
               {
                  indented_output_stream->Append("for (__testbench_index2 = 0; __testbench_index2 < " + STR(splitted.size()) + "; ++__testbench_index2)\n{\n");
                  if(output_level > OUTPUT_LEVEL_MINIMUM)
                  {
                     indented_output_stream->Append("fprintf(__bambu_testbench_fp, \"//expected value for output " + param + "[__testbench_index2]: %" + (behavioral_helper->is_real(base_type) ? std::string("g") : std::string("d")) + "\\n\", " +
                                                    (behavioral_helper->is_real(base_type) ? std::string("") : std::string("(int)")) + param + "[__testbench_index2]);\n");
                  }
                  indented_output_stream->Append("fprintf(__bambu_testbench_fp, \"o\");\n");
                  indented_output_stream->Append("_Ptd2Bin_(__bambu_testbench_fp, (unsigned char*)&" + param + "[__testbench_index2], " + STR(base_type_bitsize) + ");\n");
                  indented_output_stream->Append("fprintf(__bambu_testbench_fp, \"\\n\");\n");
                  indented_output_stream->Append("}\n");
               }
            }
            else if(!reference_type_p && behavioral_helper->is_an_array(base_type))
            {
               indented_output_stream->Append("for (__testbench_index2 = 0; __testbench_index2 < " + STR(splitted.size()) + "; ++__testbench_index2)\n{\n");
               unsigned int data_bitsize = tree_helper::get_array_data_bitsize(TM, base_type);
               unsigned int num_elements = tree_helper::get_array_num_elements(TM, base_type);
               unsigned int elmts_type = behavioral_helper->GetElements(base_type);
               while(behavioral_helper->is_an_array(elmts_type))
                  elmts_type = behavioral_helper->GetElements(elmts_type);
               if(output_level > OUTPUT_LEVEL_MINIMUM)
                  indented_output_stream->Append("fprintf(__bambu_testbench_fp, \"//expected value for output (*(((" + behavioral_helper->print_type(elmts_type) + "*)" + param + ")+ __testbench_index2)): %" +
                                                 (behavioral_helper->is_real(elmts_type) ? std::string("g") : std::string("d")) + "\\n\", (*(((" + behavioral_helper->print_type(elmts_type) + "*)" + param + ")+ __testbench_index2)));\n");
               indented_output_stream->Append("fprintf(__bambu_testbench_fp, \"o\");\n");
               if(splitted.size() == 1)
               {
                  for(unsigned int l = 0; l < num_elements; l++)
                  {
                     if(behavioral_helper->is_real(elmts_type))
                     {
                        indented_output_stream->Append("_Ptd2Bin_(__bambu_testbench_fp, "
                                                       "(unsigned char*)&(*" +
                                                       param + ")[" + STR(l) + "], " + STR(data_bitsize) + ");\n");
                     }
                     else
                     {
                        indented_output_stream->Append("_Dec2Bin_(__bambu_testbench_fp, "
                                                       "(*" +
                                                       param + ")[" + STR(l) + "], " + STR(data_bitsize) + ");\n");
                     }
                  }
               }
               else
               {
                  if(behavioral_helper->is_real(elmts_type))
                  {
                     indented_output_stream->Append("_Ptd2Bin_(__bambu_testbench_fp, "
                                                    "(unsigned char*)&(*(((" +
                                                    behavioral_helper->print_type(elmts_type) + "*)" + param + ")+ __testbench_index2)), " + STR(data_bitsize) + ");\n");
                  }
                  else
                  {
                     indented_output_stream->Append("_Dec2Bin_(__bambu_testbench_fp, "
                                                    "(*(((" +
                                                    behavioral_helper->print_type(elmts_type) + "*)" + param + ") + __testbench_index2)), " + STR(data_bitsize) + ");\n");
                  }
               }
               indented_output_stream->Append("fprintf(__bambu_testbench_fp, \"\\n\");\n");
               indented_output_stream->Append("}\n");
            }
            else
            {
               if(splitted.size() == 1 && flag_cpp && reference_type_p)
               {
                  if(output_level > OUTPUT_LEVEL_MINIMUM)
                     indented_output_stream->Append("fprintf(__bambu_testbench_fp, \"//expected value for output " + param + ": %" + (behavioral_helper->is_real(base_type) ? std::string("g") : std::string("d")) + "\\n\", " +
                                                    (behavioral_helper->is_real(base_type) ? std::string("") : std::string("(int)")) + param + ");\n");
                  indented_output_stream->Append("fprintf(__bambu_testbench_fp, \"o\");\n");
                  indented_output_stream->Append("_Dec2Bin_(__bambu_testbench_fp, " + param + ", " + STR(base_type_bitsize) + ");\n");
                  indented_output_stream->Append("fprintf(__bambu_testbench_fp, \"\\n\");\n");
               }
               else
               {
                  indented_output_stream->Append("for (__testbench_index2 = 0; __testbench_index2 < " + STR(splitted.size()) + "; ++__testbench_index2)\n{\n");
                  if(output_level > OUTPUT_LEVEL_MINIMUM)
                     indented_output_stream->Append("fprintf(__bambu_testbench_fp, \"//expected value for output " + param + "[%d]: %" + (behavioral_helper->is_real(base_type) ? std::string("g") : std::string("d")) + "\\n\", __testbench_index2, " + param +
                                                    "[__testbench_index2]);\n");
                  indented_output_stream->Append("fprintf(__bambu_testbench_fp, \"o\");\n");
                  indented_output_stream->Append("_Dec2Bin_(__bambu_testbench_fp, " + param + "[__testbench_index2], " + STR(base_type_bitsize) + ");\n");
                  indented_output_stream->Append("fprintf(__bambu_testbench_fp, \"\\n\");\n");
                  indented_output_stream->Append("}\n");
               }
            }
            indented_output_stream->Append("fprintf(__bambu_testbench_fp, \"e\\n\");\n");
         }
         else
         {
            /// Retrieve the space to be reserved in memory
            const unsigned int base_type = tree_helper::get_type_index(TM, p);
            const auto pt_node = tree_helper::CGetPointedType(TM->CGetTreeNode(base_type));
            const auto reserved_mem_bytes = hls_c_backend_information->HLSMgr->RSim->param_mem_size.at(v_idx).at(p);
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Reserved memory " + STR(reserved_mem_bytes) + " bytes");
            const auto element_size = tree_helper::Size(pt_node) / 8;
            THROW_ASSERT(reserved_mem_bytes % element_size == 0, STR(reserved_mem_bytes) + "/" + STR(element_size));
            const auto num_elements = reserved_mem_bytes / element_size;
            THROW_ASSERT(num_elements, STR(reserved_mem_bytes) + "/" + STR(element_size));
            indented_output_stream->Append("{\n");
            indented_output_stream->Append("int i0;\n");
            indented_output_stream->Append("for(i0 = 0; i0 < " + STR(num_elements) + "; i0++)\n");
            indented_output_stream->Append("{\n");
            WriteParamInMemory(behavioral_helper, param + "[i0]", pt_node->index, 1, false);
            indented_output_stream->Append("}\n");
            indented_output_stream->Append("fprintf(__bambu_testbench_fp, \"e\\n\");\n");
            indented_output_stream->Append("}\n");
         }
      }
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Generated code for expected results of " + param);
   }

   if(return_type_index)
   {
      if(output_level > OUTPUT_LEVEL_MINIMUM)
         indented_output_stream->Append("fprintf(__bambu_testbench_fp, \"//expected value for return value\\n\");\n");
      indented_output_stream->Append("fprintf(__bambu_testbench_fp, \"o\");\n");
      unsigned int base_type_bitsize = tree_helper::size(TM, return_type_index);
      if(behavioral_helper->is_real(return_type_index) || behavioral_helper->is_a_struct(return_type_index) || behavioral_helper->is_an_union(return_type_index))
      {
         indented_output_stream->Append("_Ptd2Bin_(__bambu_testbench_fp, (unsigned char*)&" + std::string(RETURN_PORT_NAME) + ", " + STR(base_type_bitsize) + ");\n");
      }
      else
      {
         indented_output_stream->Append("_Dec2Bin_(__bambu_testbench_fp, " + std::string(RETURN_PORT_NAME) + ", " + STR(base_type_bitsize) + ");\n");
      }
      indented_output_stream->Append("fprintf(__bambu_testbench_fp, \"\\n\");\n");
      indented_output_stream->Append("fprintf(__bambu_testbench_fp, \"e\\n\");\n");
   }
}

void HLSCWriter::WriteSimulatorInitMemory(const unsigned int function_id)
{
   CInitializationParserRef c_initialization_parser = CInitializationParserRef(new CInitializationParser(Param));
   const BehavioralHelperConstRef behavioral_helper = AppM->CGetFunctionBehavior(function_id)->CGetBehavioralHelper();
   // print base address
   unsigned long long int base_address = hls_c_backend_information->HLSMgr->base_address;
   indented_output_stream->Append("fprintf(__bambu_testbench_fp, \"//base address " + STR(base_address) + "\\n\");\n");
   std::string trimmed_value;
   for(unsigned int ind = 0; ind < 32; ind++)
      trimmed_value = trimmed_value + (((1LLU << (31 - ind)) & base_address) ? '1' : '0');
   indented_output_stream->Append("fprintf(__bambu_testbench_fp, \"b" + trimmed_value + "\\n\");\n");

   const std::map<unsigned int, memory_symbolRef>& mem_vars = hls_c_backend_information->HLSMgr->Rmem->get_ext_memory_variables();
   // get the mapping between variables in external memory and their external
   // base address
   std::map<unsigned long long int, unsigned int> address;
   for(const auto& m : mem_vars)
      address[hls_c_backend_information->HLSMgr->Rmem->get_external_base_address(m.first)] = m.first;

   std::list<unsigned int> mem;
   for(const auto& ma : address)
      mem.push_back(ma.second);

   const std::list<unsigned int>& parameters = behavioral_helper->get_parameters();
   for(const auto& p : parameters)
   {
      // if the function has some pointer parameters some memory needs to be
      // reserved for the place where they point to
      if(behavioral_helper->is_a_pointer(p) && mem_vars.find(p) == mem_vars.end())
         mem.push_back(p);
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
         std::string param = behavioral_helper->PrintVariable(l);
         INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "-->Considering memory variable " + param);
         if(param[0] == '"')
            param = "@" + STR(l);

         bool is_memory = false;
         std::string test_v;
         if(mem_vars.find(l) != mem_vars.end() && std::find(parameters.begin(), parameters.end(), l) == parameters.end())
         {
            is_memory = true;
            test_v = TestbenchGenerationBaseStep::print_var_init(TM, l, hls_c_backend_information->HLSMgr->Rmem);
            if(output_level > OUTPUT_LEVEL_MINIMUM)
               indented_output_stream->Append("fprintf(__bambu_testbench_fp, \"//memory initialization for variable " + param + "\\n\");\n");
         }
         else if(curr_test_vector.find(param) != curr_test_vector.end())
         {
            test_v = curr_test_vector.find(param)->second;
            if(flag_cpp)
            {
               /// Remove leading spaces
               test_v.erase(0, test_v.find_first_not_of(" \t"));
               /// Remove trailing spaces
               auto last_character = test_v.find_last_not_of(" \t");
               if(std::string::npos != last_character)
                  test_v.erase(last_character + 1);
               /// Remove first {
               if(test_v.front() == '{')
                  test_v.erase(0, 1);
               /// Remove last }
               if(test_v.back() == '}')
                  test_v.pop_back();
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
         const auto reserved_mem_bytes = [&]() -> size_t {
            if(is_memory)
            {
               const auto ret_value = tree_helper::size(TM, l) / 8;
               return ret_value ? ret_value : 1;
            }
            else
            {
               THROW_ASSERT(tree_helper::is_a_pointer(TM, l), "");
               unsigned int base_type = tree_helper::get_type_index(TM, l);
               tree_nodeRef pt_node = TM->get_tree_node_const(base_type);
               return hls_c_backend_information->HLSMgr->RSim->param_mem_size.find(v_idx)->second.find(l)->second;
            }
         }();
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Symbol: " + param + " Reserved memory " + STR(reserved_mem_bytes) + " - Test vector is " + test_v);

         /// FIXME: for c++ code the old code is still used
         if(flag_cpp or is_memory)
         {
            size_t printed_bytes = 0;
            std::string bits_offset = "";
            std::vector<std::string> splitted = SplitString(test_v, ",");
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Processing c++ init " + test_v);
            const auto isAllZero = [&]() -> bool {
               if(splitted.size() == 0)
                  return false;
               const auto first_string = splitted.at(0);
               const auto size_vec = first_string.size();
               if(size_vec % 8)
                  return false;
               for(auto strIndex = 0u; strIndex < size_vec; ++strIndex)
               {
                  if(first_string.at(strIndex) != '0')
                     return false;
               }
               for(unsigned int i = 1; i < splitted.size(); i++)
               {
                  if(first_string != splitted.at(i))
                     return false;
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
                  auto output_parameter_initialization_filename = Param->getOption<std::string>(OPT_output_directory) + "/simulation/";
                  unsigned int progressive = 0;
                  std::string candidate_out_file_name;
                  do
                  {
                     candidate_out_file_name = output_parameter_initialization_filename + param_name + "_" + std::to_string(progressive++) + ".data";
                  } while(boost::filesystem::exists(candidate_out_file_name));
                  output_parameter_initialization_filename = candidate_out_file_name;
                  std::ofstream parameter_init_file(output_parameter_initialization_filename.c_str());
                  for(unsigned int i = 0; i < splitted.size(); i++)
                  {
                     THROW_ASSERT(splitted[i] != "", "Not well formed test vector: " + test_v);
                     std::string initial_string = splitted[i];
                     printed_bytes += WriteBinaryMemoryInitToFile(parameter_init_file, initial_string, static_cast<unsigned int>(initial_string.size()), bits_offset);
                  }
                  indented_output_stream->Append("{\n");
                  indented_output_stream->Append("FILE * __bambu_testbench_fp_local_copy;\n");
                  indented_output_stream->Append("char * line = NULL;\n");
                  indented_output_stream->Append("size_t len = 0;\n");
                  indented_output_stream->Append("ssize_t read;\n");
                  indented_output_stream->Append("__bambu_testbench_fp_local_copy = fopen(\"" + output_parameter_initialization_filename + "\", \"r\");\n");
                  indented_output_stream->Append("if (__bambu_testbench_fp_local_copy == NULL)\n");
                  indented_output_stream->Append("   exit(1);\n");
                  indented_output_stream->Append("while ((read = getline(&line, &len, __bambu_testbench_fp_local_copy)) != -1) {\n");
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
                        printed_bytes += WriteBinaryMemoryInit(initial_string, static_cast<unsigned int>(initial_string.size()), bits_offset);
                     }
                     else
                     {
                        unsigned int base_type = tree_helper::get_type_index(TM, l);
                        std::string binary_string;
                        std::string init_value_copy = initial_string;
                        boost::replace_all(init_value_copy, "\\", "\\\\");
                        boost::replace_all(init_value_copy, "\n", "");
                        boost::replace_all(init_value_copy, "\"", "");
                        if(output_level > OUTPUT_LEVEL_MINIMUM)
                           indented_output_stream->Append("fprintf(__bambu_testbench_fp, \"//memory initialization for variable " + param /*+ " value: " + init_value_copy */ + "\\n\");\n");
                        tree_nodeRef pt_node = TM->get_tree_node_const(base_type);
                        unsigned int ptd_base_type = 0;
                        if(pt_node->get_kind() == pointer_type_K)
                           ptd_base_type = GET_INDEX_NODE(GetPointer<pointer_type>(pt_node)->ptd);
                        else if(pt_node->get_kind() == reference_type_K)
                           ptd_base_type = GET_INDEX_NODE(GetPointer<reference_type>(pt_node)->refd);
                        else
                           THROW_ERROR("A pointer type is expected");
                        tree_nodeRef ptd_base_type_node;
                        if(pt_node->get_kind() == pointer_type_K)
                           ptd_base_type_node = GET_NODE(GetPointer<pointer_type>(pt_node)->ptd);
                        else if(pt_node->get_kind() == reference_type_K)
                           ptd_base_type_node = GET_NODE(GetPointer<reference_type>(pt_node)->refd);
                        else
                           THROW_ERROR("A pointer type is expected");
                        INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Pointed base type is " + ptd_base_type_node->get_kind_text());
                        unsigned int data_bitsize = tree_helper::size(TM, ptd_base_type);
                        if(behavioral_helper->is_a_struct(ptd_base_type))
                        {
                           std::vector<std::string> splitted_fields = SplitString(initial_string, "|");
                           const auto fields = tree_helper::CGetFieldTypes(TM->CGetTreeNode(ptd_base_type));
                           size_t n_values = splitted_fields.size();
                           unsigned int index = 0;
                           for(auto it = fields.begin(); it != fields.end(); ++it, ++index)
                           {
                              const tree_nodeConstRef field_type = *it;
                              unsigned int field_size = tree_helper::Size(field_type);
                              if(index < n_values)
                              {
                                 binary_string = ConvertInBinary(splitted_fields[index], field_size, behavioral_helper->is_real(field_type->index), behavioral_helper->is_unsigned(field_type->index));
                              }
                              else
                              {
                                 binary_string = ConvertInBinary("0", field_size, behavioral_helper->is_real(field_type->index), behavioral_helper->is_unsigned(field_type->index));
                              }

                              printed_bytes += WriteBinaryMemoryInit(binary_string, field_size, bits_offset);
                           }
                        }
                        else if(behavioral_helper->is_an_union(ptd_base_type))
                        {
                           unsigned int max_bitsize_field = 0;
                           tree_helper::accessed_greatest_bitsize(TM, ptd_base_type_node, ptd_base_type, max_bitsize_field);
                           binary_string = ConvertInBinary("0", max_bitsize_field, false, false);
                           printed_bytes += WriteBinaryMemoryInit(binary_string, max_bitsize_field, bits_offset);
                        }
                        else if(behavioral_helper->is_an_array(ptd_base_type))
                        {
                           unsigned int elmts_type = behavioral_helper->GetElements(ptd_base_type);

                           while(behavioral_helper->is_an_array(elmts_type))
                              elmts_type = behavioral_helper->GetElements(elmts_type);

                           data_bitsize = tree_helper::get_array_data_bitsize(TM, ptd_base_type);

                           unsigned int num_elements = 1;
                           if(splitted.size() == 1)
                              num_elements = tree_helper::get_array_num_elements(TM, ptd_base_type);

                           indented_output_stream->Append("for (__testbench_index0 = 0; __testbench_index0 < " + STR(num_elements) + "; ++__testbench_index0)\n{\n");

                           binary_string = ConvertInBinary(initial_string, data_bitsize, behavioral_helper->is_real(elmts_type), behavioral_helper->is_unsigned(elmts_type));
                           printed_bytes += WriteBinaryMemoryInit(binary_string, data_bitsize, bits_offset);
                           indented_output_stream->Append("}\n");
                        }
                        else
                        {
                           binary_string = ConvertInBinary(initial_string, data_bitsize, behavioral_helper->is_real(ptd_base_type), behavioral_helper->is_unsigned(ptd_base_type));

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
                  tail_padding += "0";
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
            const auto type = TM->CGetTreeNode(tree_helper::get_type_index(TM, l));
            const CInitializationParserFunctorRef c_initialization_parser_functor =
                CInitializationParserFunctorRef(new MemoryInitializationCWriter(indented_output_stream, TM, behavioral_helper, reserved_mem_bytes, type, TestbenchGeneration_MemoryType::MEMORY_INITIALIZATION, Param));
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Parsing initialization of " + param + "(" + type->get_kind_text() + " - " + STR(type->index) + "): " + test_v);
            c_initialization_parser->Parse(c_initialization_parser_functor, test_v);
         }

         size_t next_object_offset = hls_c_backend_information->HLSMgr->RSim->param_next_off.find(v_idx)->second.find(l)->second;

         if(next_object_offset > reserved_mem_bytes)
         {
            indented_output_stream->Append("// next_object_offset > reserved_mem_bytes\n");
            WriteZeroedBytes(next_object_offset - reserved_mem_bytes);
         }
         INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "<--Considered memory variable " + param);
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
   const BehavioralHelperConstRef behavioral_helper = AppM->CGetFunctionBehavior(function_id)->CGetBehavioralHelper();
   const unsigned int return_type_index = behavioral_helper->GetFunctionReturnType(function_id);

   indented_output_stream->Append("#undef main\n");
   indented_output_stream->Append("int main()\n{\n");
   indented_output_stream->Append("unsigned int __testbench_index, __testbench_index0, __testbench_index1, __testbench_index2;\n");
   indented_output_stream->Append("__standard_exit = 0;\n");
   indented_output_stream->Append("__bambu_testbench_fp = fopen(\"" + hls_c_backend_information->results_filename + "\", \"w\");\n");
   indented_output_stream->Append("if (!__bambu_testbench_fp) {\n");
   indented_output_stream->Append("perror(\"can't open file: " + hls_c_backend_information->results_filename + "\");\n");
   indented_output_stream->Append("exit(1);\n");
   indented_output_stream->Append("}\n\n");
   // write additional initialization code needed by subclasses
   WriteExtraInitCode();
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Written extra init code");

   // parameters declaration
   WriteParamDecl(behavioral_helper);

   // write C code used to print initialization values for the HDL simulator's memory
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Writing simulator init memory");
   WriteSimulatorInitMemory(function_id);

   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Written simulator init memory");
   // ---- WRITE VARIABLES DECLARATIONS ----
   // declaration of the return variable of the top function, if not void
   if(return_type_index)
   {
      std::string ret_type = behavioral_helper->print_type(return_type_index);
      if(tree_helper::is_a_vector(TM, return_type_index))
      {
         THROW_ERROR("return type of function under test " + behavioral_helper->get_function_name() + " is " + ret_type +
                     "\n"
                     "co-simulation does not support vectorized return types at top level");
      }

      indented_output_stream->Append("// return variable initialization\n");
      indented_output_stream->Append(ret_type + " " + std::string(RETURN_PORT_NAME) + ";\n");
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
      WriteParamInitialization(behavioral_helper, curr_test_vector, v_idx);
      WriteExtraCodeBeforeEveryMainCall();
      // write the call to the top function to be tested
      indented_output_stream->Append("// function call\n");
      WriteTestbenchFunctionCall(behavioral_helper);
      // write the expected results
      indented_output_stream->Append("// print expected results\n");
      WriteExpectedResults(behavioral_helper, curr_test_vector, v_idx);
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
   const BehavioralHelperConstRef behavioral_helper = AppM->CGetFunctionBehavior(function_id)->CGetBehavioralHelper();
   INDENT_OUT_MEX(OUTPUT_LEVEL_VERBOSE, output_level, "-->C-based testbench generation for function " + behavioral_helper->get_function_name() + ": " + file_name);

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

size_t HLSCWriter::WriteBinaryMemoryInit(const std::string& binary_string, const size_t data_bitsize, std::string& bits_offset)
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
            indented_output_stream->Append("fprintf(__bambu_testbench_fp, \"m" + binary_string.substr(data_bitsize - (8 - bits_offset.size()), 8 - bits_offset.size()) + bits_offset + "\\n\");\n");
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
            indented_output_stream->Append("fprintf(__bambu_testbench_fp, \"m" + local_binary_string.substr(local_data_bitsize - 8 - base_index, 8) + "\\n\");\n");
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

size_t HLSCWriter::WriteBinaryMemoryInitToFile(std::ofstream& parameter_init_file, const std::string& binary_string, const size_t data_bitsize, std::string& bits_offset)
{
   size_t printed_bytes = 0;
   std::string local_binary_string;
   size_t local_data_bitsize;
   if(bits_offset.size())
   {
      if(static_cast<int>(data_bitsize) - 8 + static_cast<int>(bits_offset.size()) >= 0)
      {
         local_data_bitsize = data_bitsize - (8 - bits_offset.size());
         parameter_init_file << "m" + binary_string.substr(data_bitsize - (8 - bits_offset.size()), 8 - bits_offset.size()) + bits_offset + "\n";
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
      return false;
   for(size_t i = 0; i < size; ++i)
      if(str.at(i) != '0')
         return false;
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

void HLSCWriter::WriteParamInMemory(const BehavioralHelperConstRef behavioral_helper, const std::string& param, const unsigned int type_index, const unsigned int nesting_level, bool input)
{
   const auto type = TM->CGetTreeNode(type_index);
   switch(type->get_kind())
   {
      /// FIXME: real numbers at the moment have to be considered differently because of computation of ulp
      case real_type_K:
      {
         indented_output_stream->Append("fprintf(__bambu_testbench_fp, \"//expected value for output: " + param + "\\n\");\n");
         const auto size = tree_helper::Size(type);
         if(input)
         {
            const auto byte_size = tree_helper::Size(type) / 8;
            for(size_t byte = 0; byte < byte_size; byte++)
            {
               indented_output_stream->Append("fprintf(__bambu_testbench_fp, \"m\");\n");
               indented_output_stream->Append("_Ptd2Bin_(__bambu_testbench_fp, ((unsigned char *)&(" + param + ")) + " + STR(byte) + ", 8);\n");
               indented_output_stream->Append("fprintf(__bambu_testbench_fp, \"\\n\");\n");
            }
         }
         else
         {
            indented_output_stream->Append("fprintf(__bambu_testbench_fp, \"o\");\n");
            indented_output_stream->Append("_Ptd2Bin_(__bambu_testbench_fp, ((unsigned char *)&(" + param + ")), " + STR(size) + ");\n");
            indented_output_stream->Append("fprintf(__bambu_testbench_fp, \"\\n\");\n");
         }
         break;
      }
      case integer_type_K:
      {
         indented_output_stream->Append("fprintf(__bambu_testbench_fp, \"//expected value for output: " + param + "\\n\");\n");
         const auto byte_size = tree_helper::Size(type) / 8;
         for(size_t byte = 0; byte < byte_size; byte++)
         {
            indented_output_stream->Append("fprintf(__bambu_testbench_fp, \"" + std::string(input ? "m" : "o") + "\");\n");
            indented_output_stream->Append("_Ptd2Bin_(__bambu_testbench_fp, ((unsigned char *)&(" + param + ")) + " + STR(byte) + ", 8);\n");
            indented_output_stream->Append("fprintf(__bambu_testbench_fp, \"\\n\");\n");
         }
         break;
      }
      case record_type_K:
      {
         const auto rt = GetPointer<const record_type>(type);
         for(const auto field : rt->list_of_flds)
         {
            const auto field_param = param + "." + behavioral_helper->PrintVariable(field->index);
            WriteParamInMemory(behavioral_helper, field_param, tree_helper::get_type_index(TM, field->index), nesting_level + 1, input);
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
         const auto num_elements = tree_helper::get_integer_cst_value(arr_ic) / tree_helper::get_integer_cst_value(eln_ic);
         indented_output_stream->Append("{\n");
         const std::string variable_name = "i" + STR(nesting_level);
         indented_output_stream->Append("int " + variable_name + ";\n");
         indented_output_stream->Append("for(" + variable_name + " = 0; " + variable_name + " < " + STR(num_elements) + "; " + variable_name + "++)\n");
         indented_output_stream->Append("{\n");
         WriteParamInMemory(behavioral_helper, param + "[" + variable_name + "]", array_t->index, nesting_level + 1, input);
         indented_output_stream->Append("}\n");
         indented_output_stream->Append("}\n");
         break;
      }
      case pointer_type_K:
      case boolean_type_K:
      case CharType_K:
      case enumeral_type_K:
      case complex_type_K:
      case union_type_K:
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
      case void_type_K:
         THROW_ERROR("Unexpected type in initializing parameter/variable: " + param + " (type " + type->get_kind_text() + ")");
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
