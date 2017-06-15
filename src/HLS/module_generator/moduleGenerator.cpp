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
 *              Copyright (c) 2004-2017 Politecnico di Milano
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
 * @file moduleGenerator.cpp
 * @brief
 *
 *
 *
 * @author Alessandro Nacci <alenacci@gmail.com>
 * @author Gianluca Durelli <durellinux@gmail.com>
 * @author Marco Lattuada <marco.lattuada@polimi.it>
 *
*/
///header include
#include "moduleGenerator.hpp"

///Autoheader include
#include "config_BOOST_INCLUDE_DIR.hpp"

///. include
#include "Parameter.hpp"

///behavior include
#include "function_behavior.hpp"
#include "op_graph.hpp"

///circuit includes
#include "structural_manager.hpp"
#include "structural_objects.hpp"

///design_flows/backend/ToHDL
#include "language_writer.hpp"

///HLS include
#include "hls_manager.hpp"

///HLS/memory include
#include "memory.hpp"
#include "memory_cs.hpp"

///STD includes
#include <iosfwd>
#include <fstream>

///technology include
#include "technology_manager.hpp"

///technology/physical_library includes
#include "library_manager.hpp"
#include "technology_node.hpp"

///technology/physical_library/models include
#include "area_model.hpp"

///tree include
#include "behavioral_helper.hpp"
#include "tree_helper.hpp"
#include "tree_manager.hpp"
#include "tree_node.hpp"
#include "tree_reindex.hpp"

///utility include
#include "fileIO.hpp"
#include "math_function.hpp"
#include "utility.hpp"

moduleGenerator::moduleGenerator(const HLS_managerConstRef _HLSMgr, const ParameterConstRef _parameters) :
   HLSMgr(_HLSMgr),
   parameters(_parameters),
   debug_level(_parameters->get_class_debug_level(GET_CLASS(*this)))
{

}

moduleGenerator::~moduleGenerator()
{

}

#define NAMESEPARATOR "_"

structural_type_descriptorRef moduleGenerator::getDataType(unsigned int variable, const FunctionBehaviorConstRef function_behavior) const
{
   return structural_type_descriptorRef(new structural_type_descriptor(variable, function_behavior->CGetBehavioralHelper()));
}

static
unsigned int resize_to_8_or_greater(unsigned int value)
{
   if(value < 8)
      return 8;
   else
      return resize_to_1_8_16_32_64_128_256_512(value);
}

std::string moduleGenerator::get_specialized_name(std::vector<std::tuple<unsigned int,unsigned int> >& required_variables, const FunctionBehaviorConstRef FB) const
{
   std::string fuName="";
   for(std::vector<std::tuple<unsigned int,unsigned int> >::iterator l = required_variables.begin(); l != required_variables.end(); ++l)
   {
      unsigned int dataSize=getDataType(std::get<0>((*l)), FB)->vector_size!=0?getDataType(std::get<0>((*l)), FB)->vector_size:getDataType(std::get<0>((*l)), FB)->size;
      structural_type_descriptorRef typeRef=getDataType(std::get<0>((*l)), FB);
      fuName=fuName+NAMESEPARATOR+typeRef->get_name()+STR(resize_to_8_or_greater(dataSize));
   }
   return fuName;
}

std::string moduleGenerator::GenerateHDL(const std::string& hdl_template, std::vector<std::tuple<unsigned int,unsigned int> >& required_variables, const std::string& specializing_string, const FunctionBehaviorConstRef FB, const std::string& path_dynamic_generators, const HDLWriter_Language language)
{
   PRINT_DBG_MEX(DEBUG_LEVEL_VERBOSE, debug_level, "dynamic_generators @ Reading cpp-template input file '" << (path_dynamic_generators+"/"+hdl_template).c_str() << "'...");

   std::string cpp_input_file_path_string=path_dynamic_generators+"/"+hdl_template;

   const char *cpp_input_file_path = cpp_input_file_path_string.c_str();
   std::string cpp_input = "";
   std::string line;
   std::ifstream cpp_infile;
   cpp_infile.open(cpp_input_file_path,std::ifstream::in);
   if (cpp_infile.is_open()){
      while ( cpp_infile.good() ){
         getline (cpp_infile,line);
         cpp_input += line + "\n";
      }
      cpp_infile.close();
   }
   else
      THROW_ERROR("Unable to open file " + hdl_template);

   PRINT_DBG_MEX(DEBUG_LEVEL_VERBOSE, debug_level, "dynamic_generators @ Starting dynamic hdl generation...");

   std::string cpp_code_header = "";
   std::string cpp_code_body = "";
   std::string cpp_code_footer = "";

   PRINT_DBG_MEX(DEBUG_LEVEL_VERBOSE, debug_level, "dynamic_generators @ Initializing temporary c++ file for Verilog generation...");

   cpp_code_header += "#include <iostream>\n";
   cpp_code_header += "#include <string>\n";
   cpp_code_header += "#include <fstream>\n";
   cpp_code_header += "#include <sstream>\n";
   cpp_code_header += "#include <fcntl.h>\n";
   cpp_code_header += "#include <boost/lexical_cast.hpp>\n";
   cpp_code_header += "#include <boost/algorithm/string/replace.hpp>\n";

   cpp_code_header += "#define STR(x) boost::lexical_cast<std::string>(x)\n\n";
   cpp_code_header += "int main(int argc, char **argv)\n";
   cpp_code_header += "{\n";

   cpp_code_header += "   struct parameter\n";
   cpp_code_header += "   {\n";
   cpp_code_header += "         std::string name;\n";
   cpp_code_header += "         std::string type;\n";
   cpp_code_header += "         unsigned int type_size;\n";
   cpp_code_header += "         unsigned int address;\n";
   cpp_code_header += "   };\n";

   cpp_code_footer += "\n\n\n";
   cpp_code_footer += "   return 0;\n}\n";

   PRINT_DBG_MEX(DEBUG_LEVEL_VERBOSE, debug_level, "dynamic_generators @ Importing XML description...");

   cpp_code_body += "";

   PRINT_DBG_MEX(DEBUG_LEVEL_VERBOSE, debug_level, "dynamic_generators @ Setting up parameters structure...");

   unsigned int parNum = static_cast<unsigned int>(required_variables.size());

   cpp_code_body += "   int _np = "+STR(parNum)+";\n";
   cpp_code_body += "   parameter _p["+ STR(parNum) +"];\n";

   int portNum=0;

   for(std::vector<std::tuple<unsigned int,unsigned int> >::iterator l = required_variables.begin(); l != required_variables.end(); ++l)
   {
      structural_type_descriptorRef typeRef=getDataType(std::get<0>((*l)), FB);
      cpp_code_body += "   _p["+STR(portNum)+"].name = \"in"+STR(portNum+1)+"\";\n";
      cpp_code_body += "   _p["+STR(portNum)+"].type = \""+typeRef->get_name()+"\";\n";
      unsigned int dataSize = typeRef->vector_size !=0 ? typeRef->vector_size : typeRef->size;
      cpp_code_body += "   _p["+STR(portNum)+"].type_size = "+ STR(resize_to_8_or_greater(dataSize)) +";\n";
      portNum++;
   }
   cpp_code_body += "std::string data_bus_bitsize = \"" + STR(HLSMgr->Rmem->get_bus_data_bitsize()) + "\";\n";
   cpp_code_body += "std::string addr_bus_bitsize = \"" + STR(HLSMgr->Rmem->get_bus_addr_bitsize()) + "\";\n";
   cpp_code_body += "std::string size_bus_bitsize = \"" + STR(HLSMgr->Rmem->get_bus_size_bitsize()) + "\";\n";
   cpp_code_body += "std::string _specializing_string = \"" + specializing_string + "\";\n";
   if(parameters->isOption(OPT_context_switch))
   {
      cpp_code_body += "std::string tag_bus_bitsize = \"" + STR(GetPointer<memory_cs>(HLSMgr->Rmem)->get_bus_tag_bitsize()) + "\";\n";
   }
   if(parameters->isOption(OPT_channels_number) && parameters->getOption<unsigned int>(OPT_channels_number) > 1)
      cpp_code_body += "unsigned int _number_of_channes = " + STR(parameters->getOption<unsigned int>(OPT_channels_number)) + ";\n";

   cpp_code_body += "\n\n\n";

   cpp_code_body += cpp_input;

   std::string cpp_code = cpp_code_header + cpp_code_body + cpp_code_footer;

   PRINT_DBG_MEX(DEBUG_LEVEL_VERBOSE, debug_level, "dynamic_generators @ Creating temp c++ file...");

   std::fstream File;
   const std::string temp_generator_filename = language == HDLWriter_Language::VERILOG ? "temp_verilog_generator.cpp" : "temp_vhdl_generator.cpp";
   const std::string temp_generator_exec = language == HDLWriter_Language::VERILOG ? "temp_verilog_generator" : "temp_vhdl_generator";
   const std::string temp_generated_filename = language == HDLWriter_Language::VERILOG ? "temp_verilog_file.v" : "temp_vhdl_file.vhd";

   File.open(temp_generator_filename, std::ios::out);
   if (File.is_open ())
      File << cpp_code;
   File.close();

   int err;
   PRINT_DBG_MEX(DEBUG_LEVEL_VERBOSE, debug_level, "dynamic_generators @ Compiling temp c++ file...");
   err = PandaSystem(parameters, "g++ " + temp_generator_filename + " -o" + temp_generator_exec + " -I" + BOOST_INCLUDE_DIR);
   if(IsError(err))
   {
      THROW_ERROR("Error in generating " + temp_generator_exec);
   }
   PRINT_DBG_MEX(DEBUG_LEVEL_VERBOSE, debug_level, "dynamic_generators @ Executing temp c++ file...");
   err = PandaSystem(parameters, "./" + temp_generator_exec, temp_generated_filename);
   if(IsError(err))
   {
      THROW_ERROR("Error in generating " + temp_generated_filename);
   }

   PRINT_DBG_MEX(DEBUG_LEVEL_VERBOSE, debug_level, "dynamic_generators @ Verilog file generated successfully!...");
   PRINT_DBG_MEX(DEBUG_LEVEL_VERBOSE, debug_level, "dynamic_generators @ Importing hdl...");

   std::string HDLOutput = "";
   line = "";
   std::ifstream HDLFile (temp_generated_filename);
   if (HDLFile.is_open()){
      while ( HDLFile.good() ){
         getline (HDLFile,line);
         HDLOutput += line + "\n";
      }
      HDLFile.close();
   }
   else
      THROW_ERROR("dynamic_generators @ Unable to open file " + temp_generated_filename);
   if(!parameters->isOption(OPT_no_clean) || !parameters->getOption<bool>(OPT_no_clean))
   {
      PRINT_DBG_MEX(DEBUG_LEVEL_VERBOSE, debug_level, "dynamic_generators @ Deleting all temp files...");
      boost::filesystem::remove_all(temp_generator_filename);
      boost::filesystem::remove_all(temp_generator_exec);
      boost::filesystem::remove_all(temp_generated_filename);
   }
   PRINT_DBG_MEX(DEBUG_LEVEL_VERBOSE, debug_level, "dynamic_generators @ DONE");

   PRINT_DBG_MEX(DEBUG_LEVEL_VERBOSE, debug_level, "dynamic_generators @ The generated Dynamic-HDL is:");
   PRINT_DBG_MEX(DEBUG_LEVEL_VERBOSE, debug_level, HDLOutput);

   return HDLOutput;
}

void moduleGenerator::add_port_parameters(structural_objectRef generated_port,structural_objectRef original_port)
{
   original_port->copy(generated_port);
   generated_port->get_typeRef()->size=original_port->get_typeRef()->size;
   generated_port->get_typeRef()->vector_size=original_port->get_typeRef()->vector_size;
}

void moduleGenerator::specialize_fu(std::string fuName, vertex ve, std::string libraryId, const technology_managerRef TM, const FunctionBehaviorConstRef FB, std::string new_fu_name, std::map<std::string,technology_nodeRef> & new_fu, TargetDevice_Type dv_type)
{
   PRINT_DBG_MEX(DEBUG_LEVEL_VERBOSE, debug_level, "Found variable component: "+fuName);
   std::vector<std::tuple<unsigned int,unsigned int> > required_variables = HLSMgr->get_required_values(FB->CGetBehavioralHelper()->get_function_index(), ve);
   std::string specializing_string;
   const OpGraphConstRef cfg = FB->CGetOpGraph(FunctionBehavior::CFG);
   if(cfg->CGetOpNodeInfo(ve)->GetOperation() == GIMPLE_ASM)
      specializing_string = FB->CGetBehavioralHelper()->get_asm_string(cfg->CGetOpNodeInfo(ve)->GetNodeId());
   if(cfg->CGetOpNodeInfo(ve)->GetOperation() == BUILTIN_WAIT_CALL)
   {
      tree_managerRef TreeM = HLSMgr->get_tree_manager();
      const tree_nodeRef call = TreeM->GetTreeNode(cfg->CGetOpNodeInfo(ve)->GetNodeId());
      tree_nodeRef calledFunction = GetPointer<gimple_call>(call)->args[0];
      tree_nodeRef hasreturn_node = GetPointer<gimple_call>(call)->args[1];
      long long int hasreturn_value = tree_helper::get_integer_cst_value(GetPointer<integer_cst>(GET_NODE(hasreturn_node)));
      tree_nodeRef addrExpr = GET_NODE(calledFunction);
      unsigned int type_index;
      tree_nodeRef Type = tree_helper::get_type_node(addrExpr, type_index );
      tree_nodeRef functionType = GET_NODE(GetPointer<pointer_type>(Type)->ptd);
      tree_nodeRef return_type = GetPointer<function_type>(functionType)->retn;
      if(return_type && GET_NODE(return_type)->get_kind() != void_type_K && hasreturn_value)
         specializing_string = STR(tree_helper::size(TreeM, GET_INDEX_NODE(return_type)));
   }

   const library_managerRef libraryManager = TM->get_library_manager(libraryId);

   technology_nodeRef techNode_obj=libraryManager->get_fu(fuName);
   structural_managerRef structManager_obj=GetPointer<functional_unit>(techNode_obj)->CM;
   structural_objectRef fu_obj=structManager_obj->get_circ();
   module *fu_module=GetPointer<module>(fu_obj);

   PRINT_DBG_MEX(DEBUG_LEVEL_VERBOSE, debug_level, "Specializing: "+fuName+" as "+new_fu_name);

   if(new_fu.find(new_fu_name)!=new_fu.end())
   {
      PRINT_DBG_MEX(DEBUG_LEVEL_VERBOSE, debug_level, new_fu_name+" already in the library");
      PRINT_DBG_MEX(DEBUG_LEVEL_VERBOSE, debug_level, "Specialization completed");
   }
   else
   {
      structural_objectRef top;
      structural_managerRef CM;
      unsigned int n_ports = parameters->isOption(OPT_channels_number) ? parameters->getOption<unsigned int>(OPT_channels_number) : 0;

      std::string NP_parameters;

      //std::cout<<"Start creation"<<std::endl;

      CM = structural_managerRef(new structural_manager(parameters));
      structural_type_descriptorRef module_type = structural_type_descriptorRef(new structural_type_descriptor(new_fu_name));
      CM->set_top_info(new_fu_name, module_type);
      top = CM->get_circ();
      GetPointer<module>(top)->set_generated();
      ///add description and license
      GetPointer<module>(top)->set_description(fu_module->get_description());
      GetPointer<module>(top)->set_copyright(fu_module->get_copyright());
      GetPointer<module>(top)->set_authors(fu_module->get_authors());
      GetPointer<module>(top)->set_license(fu_module->get_license());
      for(const auto module_parameter : fu_module->GetParameters())
      {
         GetPointer<module>(top)->AddParameter(module_parameter.first, fu_module->GetDefaultParameter(module_parameter.first));
         GetPointer<module>(top)->SetParameter(module_parameter.first, module_parameter.second);
      }

      //std::cout<<"Module created, adding ports"<<std::endl;

      int portNum=1;
      std::string param_list= fu_module->get_NP_functionality()->get_NP_functionality(NP_functionality::LIBRARY);

      /*Adding ports*/
      unsigned int inPortSize=static_cast<unsigned int>(fu_module->get_in_port_size());
      unsigned int outPortSize=static_cast<unsigned int>(fu_module->get_out_port_size());


      structural_objectRef generated_port;
      std::string port_name="";
      unsigned int currentPort=0;
      for(currentPort=0;currentPort<inPortSize;currentPort++){
         structural_objectRef curr_port = fu_module->get_in_port(currentPort);
         if(GetPointer<port_o>(curr_port)->get_is_var_args()){
            for(std::vector<std::tuple<unsigned int,unsigned int> >::iterator l = required_variables.begin(); l != required_variables.end(); ++l)
            {
               unsigned int var = std::get<0>(*l);
               structural_type_descriptorRef dt = getDataType(var,FB);
               /// normalize type
               if(dt->vector_size == 0)
                  dt->size = resize_to_8_or_greater(dt->size);
               else
                  dt->vector_size = resize_to_8_or_greater(dt->vector_size);

               port_name="in"+STR(portNum);
               generated_port=CM->add_port(port_name, port_o::IN, top, dt);
               generated_port->get_typeRef()->size=dt->size;
               generated_port->get_typeRef()->vector_size=dt->vector_size;
               param_list=param_list+" "+port_name;
               portNum++;
               //std::cout<<"Added port NAME: "<<generated_port->get_id()<<" TYPE: "<<generated_port->get_typeRef()->get_name()<<" CLOCK: "<<GetPointer<port_o>(generated_port)->get_is_clock()<<" DATA_SIZE:"<<STR(generated_port->get_typeRef()->size)<<" VECTOR_SIZE:"<<STR(generated_port->get_typeRef()->vector_size)<<std::endl;
            }
         }
         else{
            port_name=curr_port->get_id();
            if(curr_port->get_kind() == port_vector_o_K)
            {
               generated_port=CM->add_port_vector(port_name, port_o::IN, n_ports, top, curr_port->get_typeRef());
            }
            else
               generated_port=CM->add_port(port_name, port_o::IN, top, curr_port->get_typeRef());
            add_port_parameters(generated_port, curr_port);
            //std::cout<<"Added port NAME: "<<generated_port->get_id()<<" TYPE: "<<generated_port->get_typeRef()->get_name()<<" CLOCK: "<<GetPointer<port_o>(generated_port)->get_is_clock()<<" DATA_SIZE:"<<STR(generated_port->get_typeRef()->size)<<" VECTOR_SIZE:"<<STR(generated_port->get_typeRef()->vector_size)<<std::endl;
         }
      }

      PRINT_DBG_MEX(DEBUG_LEVEL_VERBOSE, debug_level, "Adding output ports");

      for(currentPort=0;currentPort<outPortSize;currentPort++)
      {
         structural_objectRef curr_port = fu_module->get_out_port(currentPort);
         if(curr_port->get_kind() == port_vector_o_K)
         {
            generated_port=CM->add_port_vector(curr_port->get_id(), port_o::OUT, n_ports, top, curr_port->get_typeRef());
         }
         else
            generated_port=CM->add_port(curr_port->get_id(), port_o::OUT, top, curr_port->get_typeRef());
         add_port_parameters(generated_port, curr_port);
      }

      NP_parameters = new_fu_name + std::string(" ") + param_list;
      CM->add_NP_functionality(top, NP_functionality::LIBRARY, NP_parameters);

      const auto np = fu_module->get_NP_functionality();
      const auto writer = [&] () -> HDLWriter_Language
      {
         ///default language
         const auto required_language = static_cast<HDLWriter_Language>(parameters->getOption<unsigned int>(OPT_writer_language));
         if(required_language == HDLWriter_Language::VERILOG and np->exist_NP_functionality(NP_functionality::VERILOG_GENERATOR))
         {
            return HDLWriter_Language::VERILOG;
         }
         if(required_language == HDLWriter_Language::VHDL and np->exist_NP_functionality(NP_functionality::VHDL_GENERATOR))
         {
            return HDLWriter_Language::VHDL;
         }
         if(parameters->isOption(OPT_mixed_design) && not parameters->getOption<bool>(OPT_mixed_design))
         {
            THROW_ERROR("Missing VHDL GENERATOR for " + fuName);
         }
         if(not np->exist_NP_functionality(NP_functionality::VERILOG_GENERATOR) and not np->exist_NP_functionality(NP_functionality::VHDL_GENERATOR))
         {
            THROW_ERROR("Missing GENERATOR for " + fuName);
         }
         if(np->exist_NP_functionality(NP_functionality::VERILOG_GENERATOR))
         {
            return HDLWriter_Language::VERILOG;
         }
         else
         {
            return HDLWriter_Language::VHDL;
         }
      }();

      std::string hdl_template = fu_module->get_NP_functionality()->get_NP_functionality(writer == HDLWriter_Language::VERILOG ? NP_functionality::VERILOG_GENERATOR : NP_functionality::VHDL_GENERATOR);
      PRINT_DBG_MEX(DEBUG_LEVEL_VERBOSE, debug_level, new_fu_name+": Generating dynamic hdl code");
      std::string hdl_code = GenerateHDL(hdl_template, required_variables, specializing_string, FB, parameters->getOption<std::string>("dynamic_generators_dir"), writer);

      CM->add_NP_functionality(top, writer == HDLWriter_Language::VERILOG ? NP_functionality::VERILOG_PROVIDED : NP_functionality::VHDL_PROVIDED, hdl_code);

      technology_nodeRef new_techNode_obj = technology_nodeRef(new functional_unit);
      if (GetPointer<functional_unit>(techNode_obj)->area_m)
      {
         GetPointer<functional_unit>(new_techNode_obj)->area_m = area_model::create_model(dv_type, parameters);
      }
      GetPointer<functional_unit>(new_techNode_obj)->functional_unit_name = new_fu_name;
      GetPointer<functional_unit>(new_techNode_obj)->CM = CM;

      new_fu.insert(std::make_pair(new_fu_name,new_techNode_obj));

      PRINT_DBG_MEX(DEBUG_LEVEL_VERBOSE, debug_level, new_fu_name+" created successfully");

      std::vector<technology_nodeRef> op_vec=GetPointer<functional_unit>(techNode_obj)->get_operations();
      for(std::vector<technology_nodeRef>::iterator techIter=op_vec.begin();techIter!=op_vec.end();++techIter)
      {
         technology_nodeRef techNode_fu=*techIter;
         GetPointer<functional_unit>(new_techNode_obj)->add(techNode_fu);
      }

      PRINT_DBG_MEX(DEBUG_LEVEL_VERBOSE, debug_level, "Specialization completed");
   }
}
