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
 *              Copyright (C) 2022-2023 Politecnico di Milano
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
 * @file InterfaceInfer.cpp
 * @brief Load parsed protocol interface attributes
 *
 * @author Michele Fiorito <michele.fiorito@polimi.it>
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 * @author Claudio Barone <claudio.barone@polimi.it>
 */
#include "InterfaceInfer.hpp"

#include "config_PANDA_DATA_INSTALLDIR.hpp"

#include "Parameter.hpp"
#include "application_manager.hpp"
#include "area_info.hpp"
#include "behavioral_helper.hpp"
#include "call_graph.hpp"
#include "call_graph_manager.hpp"
#include "compiler_wrapper.hpp"
#include "constant_strings.hpp"
#include "copyrights_strings.hpp"
#include "dbgPrintHelper.hpp" // for DEBUG_LEVEL_
#include "design_flow_graph.hpp"
#include "design_flow_manager.hpp"
#include "function_behavior.hpp"
#include "hls_device.hpp"
#include "hls_manager.hpp"
#include "hls_step.hpp"
#include "language_writer.hpp"
#include "library_manager.hpp"
#include "math_function.hpp"
#include "polixml.hpp"
#include "string_manipulation.hpp" // for GET_CLASS
#include "structural_manager.hpp"
#include "structural_objects.hpp"
#include "technology_flow_step.hpp"
#include "technology_flow_step_factory.hpp"
#include "technology_manager.hpp"
#include "technology_node.hpp"
#include "time_info.hpp"
#include "token_interface.hpp"
#include "tree_basic_block.hpp"
#include "tree_helper.hpp"
#include "tree_manager.hpp"
#include "tree_manipulation.hpp"
#include "tree_node.hpp"
#include "tree_reindex.hpp"
#include "xml_dom_parser.hpp"
#include "xml_helper.hpp"

#include <regex>

#define EPSILON 0.000000001
#define ENCODE_FDNAME(arg_name, MODE, interface_type) \
   ((arg_name) + STR_CST_interface_parameter_keyword + (MODE) + (interface_type))

enum class InterfaceInfer::m_axi_type
{
   none,
   direct,
   axi_slave
};

enum class InterfaceInfer::datatype
{
   generic,
   ac_type
};

struct InterfaceInfer::interface_info
{
   bool _fixed_size;

 public:
   std::string name;
   const std::string interface_id;
   unsigned alignment;
   unsigned long long bitwidth;
   unsigned long long factor;
   datatype type;

   interface_info(const std::string& _interface_id, bool fixed_size)
       : _fixed_size(fixed_size),
         name(""),
         interface_id(_interface_id),
         alignment(1U),
         bitwidth(0ULL),
         factor(1ULL),
         type(datatype::generic)
   {
   }

   void update(const tree_nodeRef& tn, const std::string& _type_name, ParameterConstRef parameters)
   {
      if(type != datatype::ac_type)
      {
         bool is_signed, is_fixed;
         const auto type_name =
             std::regex_replace(_type_name, std::regex("(ac_channel|stream|hls::stream)<(.*)>"), "$2");
         const auto ac_bitwidth = ac_type_bitwidth(type_name, is_signed, is_fixed);
         auto _type = ac_bitwidth != 0ULL ? datatype::ac_type : datatype::generic;
         unsigned long long _bitwidth = 0;
         if(_type == datatype::ac_type)
         {
            _bitwidth = ac_bitwidth;
            type = datatype::ac_type;
            alignment = static_cast<unsigned>(get_aligned_ac_bitsize(_bitwidth) >> 3);
         }
         else if(_type_name.empty())
         {
            const auto ptd_type = tree_helper::CGetType(tn);
            if(tree_helper::IsArrayEquivType(ptd_type))
            {
               if(tree_helper::IsStructType(ptd_type))
               {
                  _bitwidth = tree_helper::Size(tree_helper::CGetArrayBaseType(ptd_type));
               }
               else
               {
                  const auto elt_type = tree_helper::CGetArrayBaseType(ptd_type);
                  if(tree_helper::IsStructType(elt_type))
                  {
                     if(!tree_helper::IsArrayEquivType(elt_type))
                     {
                        THROW_ERROR("Struct type not supported for interfacing: " + STR(elt_type));
                     }
                     _bitwidth = tree_helper::Size(tree_helper::CGetArrayBaseType(elt_type));
                  }
                  else
                  {
                     _bitwidth = tree_helper::Size(elt_type);
                  }
               }
            }
            else if(tree_helper::IsPointerType(ptd_type) || tree_helper::IsStructType(ptd_type))
            {
               _bitwidth = static_cast<unsigned long long>(CompilerWrapper::CGetPointerSize(parameters));
            }
            else
            {
               _bitwidth = tree_helper::Size(ptd_type);
            }
            const auto _alignment = static_cast<unsigned>(get_aligned_bitsize(_bitwidth) >> 3);
            alignment = std::max(alignment, _alignment);
         }

         if(_fixed_size && bitwidth && bitwidth != _bitwidth)
         {
            THROW_ERROR("Unaligned access not allowed for required interface!");
         }
         bitwidth = std::max(bitwidth, _bitwidth);
      }
   }
};

InterfaceInfer::InterfaceInfer(const application_managerRef _AppM, const DesignFlowManagerConstRef _design_flow_manager,
                               const ParameterConstRef _parameters)
    : ApplicationFrontendFlowStep(_AppM, INTERFACE_INFER, _design_flow_manager, _parameters), already_executed(false)
{
   debug_level = parameters->get_class_debug_level(GET_CLASS(*this));
}

InterfaceInfer::~InterfaceInfer() = default;

const CustomUnorderedSet<std::pair<FrontendFlowStepType, FrontendFlowStep::FunctionRelationship>>
InterfaceInfer::ComputeFrontendRelationships(const DesignFlowStep::RelationshipType relationship_type) const
{
   CustomUnorderedSet<std::pair<FrontendFlowStepType, FunctionRelationship>> relationships;
   switch(relationship_type)
   {
      case(DEPENDENCE_RELATIONSHIP):
      {
         relationships.insert(std::make_pair(IR_LOWERING, ALL_FUNCTIONS));
         relationships.insert(std::make_pair(USE_COUNTING, ALL_FUNCTIONS));
         relationships.insert(std::make_pair(PARM2SSA, ALL_FUNCTIONS));
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

void InterfaceInfer::ComputeRelationships(DesignFlowStepSet& relationship,
                                          const DesignFlowStep::RelationshipType relationship_type)
{
   switch(relationship_type)
   {
      case(PRECEDENCE_RELATIONSHIP):
      {
         break;
      }
      case DEPENDENCE_RELATIONSHIP:
      {
         const auto design_flow_graph = design_flow_manager.lock()->CGetDesignFlowGraph();
         const auto technology_flow_step_factory = GetPointer<const TechnologyFlowStepFactory>(
             design_flow_manager.lock()->CGetDesignFlowStepFactory("Technology"));
         const auto technology_flow_signature =
             TechnologyFlowStep::ComputeSignature(TechnologyFlowStep_Type::LOAD_TECHNOLOGY);
         const auto technology_flow_step = design_flow_manager.lock()->GetDesignFlowStep(technology_flow_signature);
         const auto technology_design_flow_step =
             technology_flow_step ?
                 design_flow_graph->CGetDesignFlowStepInfo(technology_flow_step)->design_flow_step :
                 technology_flow_step_factory->CreateTechnologyFlowStep(TechnologyFlowStep_Type::LOAD_TECHNOLOGY);
         relationship.insert(technology_design_flow_step);
         break;
      }
      case INVALIDATION_RELATIONSHIP:
      {
         break;
      }
      default:
         THROW_UNREACHABLE("");
   }
   ApplicationFrontendFlowStep::ComputeRelationships(relationship, relationship_type);
}

static const std::regex signature_param_typename("((?:\\w+\\s*)+(?:<[^>]*>)?\\s*[\\*&]?\\s*)");

bool InterfaceInfer::HasToBeExecuted() const
{
   return !already_executed;
}

void InterfaceInfer::Initialize()
{
   const auto HLSMgr = GetPointer<HLS_manager>(AppM);
   THROW_ASSERT(HLSMgr, "");
   const auto parseInterfaceXML = [&](const std::string& XMLfilename) {
      if(std::filesystem::exists(std::filesystem::path(XMLfilename)))
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->parsing " + XMLfilename);
         XMLDomParser parser(XMLfilename);
         parser.Exec();
         if(parser)
         {
            // Walk the tree:
            const auto node = parser.get_document()->get_root_node(); // deleted by DomParser.
            for(const auto& iter : node->get_children())
            {
               const auto Enode = GetPointer<const xml_element>(iter);
               if(!Enode)
               {
                  continue;
               }
               if(Enode->get_name() == "function")
               {
                  std::string fname;
                  for(const auto& attr : Enode->get_attributes())
                  {
                     const auto key = attr->get_name();
                     const auto value = attr->get_value();
                     if(key == "id")
                     {
                        fname = value;
                     }
                  }
                  if(fname == "")
                  {
                     THROW_ERROR("malformed interface file");
                  }
                  for(const auto& iterArg : Enode->get_children())
                  {
                     const auto EnodeArg = GetPointer<const xml_element>(iterArg);
                     if(!EnodeArg)
                     {
                        continue;
                     }
                     if(EnodeArg->get_name() == "arg")
                     {
                        std::string argName;
                        std::string interface_type;
                        std::string interfaceSize;
                        std::string interfaceSizeInBytes;
                        std::string offset;
                        std::string bundleName;
                        bool bundle_p = false;
                        std::string interface_typename;
                        std::string interface_typenameOrig;
                        std::string interface_typenameInclude;
                        std::string way_lines;
                        bool way_lines_p = false;
                        std::string line_size;
                        bool line_size_p = false;
                        std::string bus_size;
                        bool bus_size_p = false;
                        std::string ways;
                        bool ways_p = false;
                        std::string buf_size;
                        bool buf_size_p = false;
                        std::string rep_pol;
                        bool rep_pol_p = false;
                        std::string wr_pol;
                        bool wr_pol_p = false;
                        for(const auto& attrArg : EnodeArg->get_attributes())
                        {
                           const auto key = attrArg->get_name();
                           const auto value = attrArg->get_value();
                           if(key == "id")
                           {
                              argName = value;
                           }
                           if(key == "interface_type")
                           {
                              interface_type = value;
                           }
                           if(key == "size")
                           {
                              interfaceSize = value;
                           }
                           if(key == "SizeInBytes")
                           {
                              interfaceSizeInBytes = value;
                           }
                           if(key == "offset")
                           {
                              offset = value;
                           }
                           if(key == "bundle_name")
                           {
                              bundleName = value;
                              bundle_p = true;
                           }
                           if(key == "way_size")
                           {
                              way_lines = value;
                              way_lines_p = true;
                           }
                           if(key == "line_size")
                           {
                              line_size = value;
                              line_size_p = true;
                           }
                           if(key == "bus_size")
                           {
                              bus_size = value;
                              bus_size_p = true;
                           }
                           if(key == "n_ways")
                           {
                              ways = value;
                              ways_p = true;
                           }
                           if(key == "buffer_size")
                           {
                              buf_size = value;
                              buf_size_p = true;
                           }
                           if(key == "rep_pol")
                           {
                              rep_pol = value;
                              rep_pol_p = true;
                           }
                           if(key == "write_pol")
                           {
                              wr_pol = value;
                              wr_pol_p = true;
                           }
                           if(key == "interface_typename")
                           {
                              interface_typename = value;
                              xml_node::convert_escaped(interface_typename);
                           }
                           if(key == "interface_typename_orig")
                           {
                              interface_typenameOrig = value;
                              xml_node::convert_escaped(interface_typenameOrig);
                           }
                           if(key == "interface_typename_include")
                           {
                              interface_typenameInclude = value;
                           }
                        }
                        if(argName == "")
                        {
                           THROW_ERROR("malformed interface file");
                        }
                        if(parameters->getOption<HLSFlowStep_Type>(OPT_interface_type) ==
                           HLSFlowStep_Type::INFERRED_INTERFACE_GENERATION)
                        {
                           if(interface_type == "")
                           {
                              THROW_ERROR("malformed interface file");
                           }
                           INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level,
                                          "---|" + argName + "|" + interface_type + "|\n");
                           HLSMgr->design_attributes[fname][argName][attr_interface_type] = interface_type;
                           HLSMgr->design_attributes[fname][argName][attr_typename] = interface_typename;
                           if(interface_type == "array")
                           {
                              HLSMgr->design_attributes[fname][argName][attr_size] = interfaceSize;
                           }
                           if(interfaceSizeInBytes != "")
                           {
                              HLSMgr->design_attributes[fname][argName][attr_size_in_bytes] = interfaceSizeInBytes;
                           }
                           if(interface_type == "m_axi")
                           {
                              HLSMgr->design_attributes[fname][argName][attr_offset] = offset;
                           }
                           if((interface_type == "m_axi" || interface_type == "array") && bundle_p)
                           {
                              HLSMgr->design_attributes[fname][argName][attr_bundle_name] = bundleName;
                           }
                           if(interface_type == "m_axi")
                           {
                              if(way_lines_p)
                              {
                                 HLSMgr->design_attributes[fname][argName][attr_way_lines] = way_lines;
                              }
                              if(line_size_p)
                              {
                                 HLSMgr->design_attributes[fname][argName][attr_line_size] = line_size;
                              }
                              if(bus_size_p)
                              {
                                 HLSMgr->design_attributes[fname][argName][attr_bus_size] = bus_size;
                              }
                              if(ways_p)
                              {
                                 HLSMgr->design_attributes[fname][argName][attr_n_ways] = ways;
                              }
                              if(buf_size_p)
                              {
                                 HLSMgr->design_attributes[fname][argName][attr_buf_size] = buf_size;
                              }
                              if(rep_pol_p)
                              {
                                 HLSMgr->design_attributes[fname][argName][attr_rep_pol] = rep_pol;
                              }
                              if(wr_pol_p)
                              {
                                 HLSMgr->design_attributes[fname][argName][attr_wr_pol] = wr_pol;
                              }
                           }
                        }

                        HLSMgr->design_interface_typename_signature[fname].push_back(interface_typename);
                        HLSMgr->design_interface_typename_orig_signature[fname].push_back(interface_typenameOrig);
                        if((interface_typenameOrig.find("ap_int<") != std::string::npos ||
                            interface_typenameOrig.find("ap_uint<") != std::string::npos) &&
                           interface_typenameInclude.find("ac_int.h") != std::string::npos)
                        {
                           boost::replace_all(interface_typenameInclude, "ac_int.h", "ap_int.h");
                        }
                        if((interface_typenameOrig.find("ap_fixed<") != std::string::npos ||
                            interface_typenameOrig.find("ap_ufixed<") != std::string::npos) &&
                           interface_typenameInclude.find("ac_fixed.h") != std::string::npos)
                        {
                           boost::replace_all(interface_typenameInclude, "ac_fixed.h", "ap_fixed.h");
                        }
                        if((interface_typenameOrig.find("hls::stream<") != std::string::npos ||
                            interface_typenameOrig.find("stream<") != std::string::npos) &&
                           interface_typenameInclude.find("ac_channel.h") != std::string::npos)
                        {
                           boost::replace_all(interface_typenameInclude, "ac_channel.h", "hls_stream.h");
                        }
                        HLSMgr->design_interface_typenameinclude[fname][argName] = interface_typenameInclude;
                     }
                  }
               }
            }
         }
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--parsed file " + XMLfilename);
      }
   };
   if(parameters->isOption(OPT_interface_xml_filename))
   {
      parseInterfaceXML(parameters->getOption<std::string>(OPT_interface_xml_filename));
   }
   else
   {
      /// load xml interface specification file
      for(const auto& source_file : AppM->input_files)
      {
         const auto output_temporary_directory = parameters->getOption<std::string>(OPT_output_temporary_directory);
         const std::string leaf_name =
             source_file.second == "-" ? "stdin-" : std::filesystem::path(source_file.second).filename().string();
         const auto XMLfilename = output_temporary_directory + "/" + leaf_name + ".interface.xml";
         parseInterfaceXML(XMLfilename);
      }
   }
}

DesignFlowStep_Status InterfaceInfer::Exec()
{
   const auto top_functions = AppM->CGetCallGraphManager()->GetRootFunctions();
   const auto HLSMgr = GetPointer<HLS_manager>(AppM);
   THROW_ASSERT(HLSMgr, "");
   const auto TM = AppM->get_tree_manager();
   std::set<unsigned int> modified;
   const auto add_to_modified = [&](const tree_nodeRef& tn) {
      modified.insert(GET_INDEX_CONST_NODE(GetPointer<gimple_node>(GET_CONST_NODE(tn))->scpe));
   };
   for(const auto& top_id : top_functions)
   {
      const auto fnode = TM->CGetTreeNode(top_id);
      const auto fd = GetPointer<const function_decl>(fnode);
      const auto fname = tree_helper::GetMangledFunctionName(fd);
      /* Check if there is a typename corresponding to fname */
      bool typename_found = false;
      if(HLSMgr->design_attributes.find(fname) != HLSMgr->design_attributes.end())
      {
         for(auto& par : HLSMgr->design_attributes.at(fname))
         {
            if(par.second.find(attr_typename) != par.second.end())
            {
               typename_found = true;
            }
         }
      }
      if(!typename_found)
      {
         const auto dfname = string_demangle(fname);
         if(!dfname.empty())
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Extracting interface from signature " + fname);
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Demangled as " + dfname);
            std::sregex_token_iterator typename_it(dfname.begin(), dfname.end(), signature_param_typename, 0), end;
            ++typename_it; // First match is the function name
            auto& top_design_interface_typename_signature = HLSMgr->design_interface_typename_signature[fname];
            auto& top_design_interface_typename_orig_signature =
                HLSMgr->design_interface_typename_orig_signature[fname];
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Iterating arguments:");
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->");
            for(const auto& arg : fd->list_of_args)
            {
               THROW_ASSERT(typename_it != end, "");
               const auto pname = [&]() {
                  std::stringstream ss;
                  ss << arg;
                  return ss.str();
               }();
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Argument " + pname);
               const std::string tname(*typename_it);
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Typename " + tname);
               if(parameters->getOption<HLSFlowStep_Type>(OPT_interface_type) ==
                  HLSFlowStep_Type::INFERRED_INTERFACE_GENERATION)
               {
                  HLSMgr->design_attributes[fname][pname][attr_typename] = tname;
               }
               top_design_interface_typename_signature.push_back(tname);
               top_design_interface_typename_orig_signature.push_back(tname);
               if(tname.find("_fixed<") != std::string::npos)
               {
                  HLSMgr->design_interface_typenameinclude[fname][pname] +=
                      STR(PANDA_DATA_INSTALLDIR "/panda/ac_types/include/" + tname.substr(0, 2) + "_fixed.h");
               }
               if(tname.find("_int<") != std::string::npos)
               {
                  HLSMgr->design_interface_typenameinclude[fname][pname] +=
                      STR(PANDA_DATA_INSTALLDIR "/panda/ac_types/include/" + tname.substr(0, 2) + "_int.h");
               }
               if(tname.find("ac_channel<") != std::string::npos)
               {
                  HLSMgr->design_interface_typenameinclude[fname][pname] +=
                      STR(PANDA_DATA_INSTALLDIR "/panda/ac_types/include/ac_channel.h");
               }
               ++typename_it;
            }
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--");
         }
      }

      if(parameters->getOption<HLSFlowStep_Type>(OPT_interface_type) == HLSFlowStep_Type::INFERRED_INTERFACE_GENERATION)
      {
         /* Check if there is at least one interface type associated to fname */
#if HAVE_ASSERTS
         bool type_found = false;
         if(HLSMgr->design_attributes.find(fname) != HLSMgr->design_attributes.end())
         {
            for(auto& par : HLSMgr->design_attributes.at(fname))
            {
               if(par.second.find(attr_interface_type) != par.second.end())
               {
                  type_found = true;
               }
            }
         }
#endif
         THROW_ASSERT(type_found, "");
         const tree_manipulationRef tree_man(new tree_manipulation(TM, parameters, AppM));

         INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, output_level, "-->Analyzing function " + fname);
         auto& DesignAttributes = HLSMgr->design_attributes[fname];
         std::map<std::string, TreeNodeSet> bundle_vdefs;
         for(const auto& arg : fd->list_of_args)
         {
            const auto arg_pd = GetPointerS<const parm_decl>(GET_CONST_NODE(arg));
            const auto arg_id = GET_INDEX_NODE(arg);
            const auto& arg_type = arg_pd->type;
            THROW_ASSERT(GetPointer<const identifier_node>(GET_CONST_NODE(arg_pd->name)), "unexpected condition");
            const auto& arg_name = GetPointerS<const identifier_node>(GET_CONST_NODE(arg_pd->name))->strg;
            INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, output_level, "---Parameter @" + STR(arg_id) + " " + arg_name);
            THROW_ASSERT(DesignAttributes.count(arg_name) && DesignAttributes.at(arg_name).count(attr_interface_type),
                         "Not matched parameter name: " + arg_name);
            auto& arg_attributes = DesignAttributes.at(arg_name);
            arg_attributes[attr_interface_dir] = port_o::GetString(port_o::IN);
            arg_attributes[attr_interface_bitwidth] = STR(tree_helper::Size(arg_type));
            arg_attributes[attr_interface_alignment] = STR(get_aligned_bitsize(tree_helper::Size(arg_type)));
            auto& interface_type = arg_attributes.at(attr_interface_type);
            if(interface_type == "bus")
            {
               interface_type = "default";
            }
            else if(interface_type != "default")
            {
               const auto arg_ssa_id = AppM->getSSAFromParm(top_id, arg_id);
               const auto arg_ssa = TM->GetTreeReindex(arg_ssa_id);
               THROW_ASSERT(GET_CONST_NODE(arg_ssa)->get_kind() == ssa_name_K, "");
               if(GetPointerS<const ssa_name>(GET_CONST_NODE(arg_ssa))->CGetUseStmts().empty())
               {
                  THROW_WARNING("Parameter '" + arg_name + "' not used by any statement");
                  if(tree_helper::IsPointerType(arg_type))
                  {
                     // BEAWARE: none is used here in place of default to avoid memory allocation to consider this as
                     // an active pointer parameter
                     interface_type = "none";
                  }
                  else
                  {
                     THROW_ERROR("parameter not used: specified interface does not make sense - " + interface_type);
                  }
                  continue;
               }
               if(tree_helper::IsPointerType(arg_type))
               {
                  INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "---Is a pointer type");
                  const auto bundle_name = [&]() -> std::string {
                     if(interface_type == "m_axi" && arg_attributes.count(attr_way_lines) &&
                        std::stoull(arg_attributes.at(attr_way_lines)))
                     {
                        THROW_ASSERT(arg_attributes.count(attr_bundle_name), "");
                        return arg_attributes.at(attr_bundle_name);
                     }
                     return "";
                  }();
                  interface_info info(bundle_name.size() ? bundle_name : arg_name, interface_type == "array" ||
                                                                                       interface_type == "fifo" ||
                                                                                       interface_type == "axis");
                  info.update(arg_ssa, HLSMgr->design_attributes.at(fname).at(arg_name).at(attr_typename), parameters);

                  std::list<tree_nodeRef> writeStmt;
                  std::list<tree_nodeRef> readStmt;
                  ChasePointerInterface(arg_ssa, writeStmt, readStmt, info);
                  const auto isRead = !readStmt.empty();
                  const auto isWrite = !writeStmt.empty();

                  if(!isRead && !isWrite)
                  {
                     THROW_ERROR("Parameter '" + arg_name + "' cannot have interface type '" + interface_type +
                                 "' since no load/store is associated with it");
                  }

                  if(arg_attributes.find(attr_size_in_bytes) != arg_attributes.end())
                  {
                     auto temp_factor = info.type == datatype::generic ?
                                            (8 * std::stoull(arg_attributes.at(attr_size_in_bytes))) / info.bitwidth :
                                            1;
                     if(temp_factor > 1)
                     {
                        info.factor = temp_factor;
                     }
                  }
                  else
                  {
                     info.factor = 1;
                  }
                  info.name = [&]() -> std::string {
                     if(isRead && isWrite)
                     {
                        arg_attributes[attr_interface_dir] = port_o::GetString(port_o::IO);
                        INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "---I/O interface");
                        if(interface_type == "array")
                        {
                           if(info.factor > 1)
                           {
                              arg_attributes[attr_size] = STR(info.factor);
                           }
                           return "array";
                        }
                        else if(interface_type == "ptrdefault")
                        {
                           if(info.factor > 1)
                           {
                              arg_attributes[attr_size] = STR(info.factor);
                              return "array";
                           }
                           else if(parameters->IsParameter("none-ptrdefault") &&
                                   parameters->GetParameter<int>("none-ptrdefault") == 1)
                           {
                              return "none";
                           }
                           else if(parameters->IsParameter("none-registered-ptrdefault") &&
                                   parameters->GetParameter<int>("none-registered-ptrdefault") == 1)
                           {
                              return "none_registered";
                           }
                           return "ovalid";
                        }
                        else if(interface_type == "fifo" || interface_type == "axis")
                        {
                           THROW_ERROR("parameter " + arg_name + " cannot have interface " + interface_type +
                                       " because it cannot be read and written at the same time");
                        }
                     }
                     else if(isRead)
                     {
                        arg_attributes[attr_interface_dir] = port_o::GetString(port_o::IN);
                        INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "---Read-only interface");
                        if(interface_type == "array")
                        {
                           if(info.factor > 1)
                           {
                              arg_attributes[attr_size] = STR(info.factor);
                           }
                           return "array";
                        }
                        else if(interface_type == "ptrdefault")
                        {
                           if(info.factor > 1)
                           {
                              arg_attributes[attr_size] = STR(info.factor);
                              return "array";
                           }
                           return "none";
                        }
                        else if(interface_type == "ovalid")
                        {
                           THROW_ERROR("parameter " + arg_name + " cannot have interface " + interface_type +
                                       " because it is read only");
                        }
                     }
                     else if(isWrite)
                     {
                        arg_attributes[attr_interface_dir] = port_o::GetString(port_o::OUT);
                        INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "---Write-only interface");
                        if(interface_type == "array")
                        {
                           if(info.factor > 1)
                           {
                              arg_attributes[attr_size] = STR(info.factor);
                           }
                           return "array";
                        }
                        else if(interface_type == "ptrdefault")
                        {
                           if(info.factor > 1)
                           {
                              arg_attributes[attr_size] = STR(info.factor);
                              return "array";
                           }
                           else if(parameters->IsParameter("none-ptrdefault") &&
                                   parameters->GetParameter<int>("none-ptrdefault") == 1)
                           {
                              return "none";
                           }
                           else if(parameters->IsParameter("none-registered-ptrdefault") &&
                                   parameters->GetParameter<int>("none-registered-ptrdefault") == 1)
                           {
                              return "none_registered";
                           }
                           return "valid";
                        }
                     }
                     return interface_type;
                  }();
                  arg_attributes[attr_interface_bitwidth] = STR(info.bitwidth);
                  arg_attributes[attr_interface_alignment] = STR(info.alignment);
                  interface_type = info.name;

                  INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, output_level, "-->Interface specification:");
                  INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, output_level, "---Protocol  : " + interface_type);
                  if(bundle_name.size())
                  {
                     INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, output_level, "---Bundle    : " + bundle_name);
                  }
                  INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, output_level, "---Bitwidth  : " + STR(info.bitwidth));
                  if(arg_attributes.find(attr_size) != arg_attributes.end())
                  {
                     INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, output_level,
                                    "---Size      : " + STR(std::stoull(arg_attributes.at(attr_size))));
                  }
                  INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, output_level, "---Alignment : " + STR(info.alignment));
                  INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, output_level, "<--");
                  THROW_ASSERT(info.bitwidth, "");

                  std::set<std::string> operationsR, operationsW;
                  const auto interface_datatype = tree_man->GetCustomIntegerType(info.bitwidth, true);
                  const auto commonRWSignature = interface_type == "array" || interface_type == "m_axi";
                  const auto store_vdef = [&](const tree_nodeRef& stmt) {
                     if(bundle_name.size())
                     {
                        const auto& vdef = GetPointerS<const gimple_node>(GET_CONST_NODE(stmt))->vdef;
                        if(vdef)
                        {
                           bundle_vdefs[bundle_name].insert(vdef);
                        }
                     }
                  };
                  for(const auto& stmt : readStmt)
                  {
                     setReadInterface(stmt, arg_name, operationsR, commonRWSignature, interface_datatype, tree_man, TM);
                     add_to_modified(stmt);
                     store_vdef(stmt);
                  }
                  for(const auto& stmt : writeStmt)
                  {
                     setWriteInterface(stmt, arg_name, operationsW, commonRWSignature, interface_datatype, tree_man,
                                       TM);
                     add_to_modified(stmt);
                     store_vdef(stmt);
                  }
                  create_resource(operationsR, operationsW, arg_name, info, fname);
               }
               else if(interface_type == "none")
               {
                  THROW_ERROR("Interface type '" + interface_type + "' for parameter '" + arg_name + "' unexpected");
               }
               else
               {
                  THROW_ERROR("Interface type '" + interface_type + "' for parameter '" + arg_name +
                              "' is not supported");
               }
            }
         }
         /* Add cache flush operation */
         for(const auto& bundle_vdef : bundle_vdefs)
         {
            const auto& bundle_name = bundle_vdef.first;
            const auto& vdefs = bundle_vdef.second;
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Interface finalizer for bundle " + bundle_name);
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Virtuals count: " + STR(vdefs.size()));
            const auto generate_fini_call = [&]() {
               const auto fini_fname = ENCODE_FDNAME(bundle_name, "_Flush_", "m_axi");
               std::vector<tree_nodeConstRef> argsT;
               argsT.push_back(tree_man->GetBooleanType());
               argsT.push_back(tree_man->GetUnsignedIntegerType());
               const auto fini_fd = tree_man->create_function_decl(fini_fname, fd->scpe, argsT, tree_man->GetVoidType(),
                                                                   BUILTIN_SRCP, false);

               // Cache flush is indicated by a write of size 0.
               std::vector<tree_nodeRef> args;
               args.push_back(TM->CreateUniqueIntegerCst(1, argsT.at(0)));
               args.push_back(TM->CreateUniqueIntegerCst(0, argsT.at(1)));
               const auto fini_call =
                   tree_man->create_gimple_call(fini_fd, args, GET_INDEX_NODE(fd->scpe), BUILTIN_SRCP);
               const auto fini_node = GetPointerS<gimple_node>(GET_CONST_NODE(fini_call));
               for(const auto& vdef : vdefs)
               {
                  fini_node->AddVuse(vdef);
               }
               return fini_call;
            };

            const auto sl = GetPointerS<const statement_list>(GET_CONST_NODE(fd->body));
            for(const auto& bbi_bb : sl->list_of_bloc)
            {
               const auto& bb = bbi_bb.second;
               const auto is_last = std::count(bb->list_of_succ.cbegin(), bb->list_of_succ.cend(), BB_EXIT);
               if(bb->number != BB_ENTRY && is_last)
               {
                  const auto fini_call = generate_fini_call();
                  THROW_ASSERT(bb->CGetStmtList().size(), "BB" + STR(bb->number) + " should not be empty");
                  const auto last_stmt = bb->CGetStmtList().back();
                  if(GetPointer<const gimple_return>(GET_CONST_NODE(last_stmt)))
                  {
                     bb->PushBefore(fini_call, last_stmt, AppM);
                  }
                  else
                  {
                     bb->PushBack(fini_call, AppM);
                  }
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                 "---Added function call " + STR(fini_call) + " to BB" + STR(bb->number));
               }
            }
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--");
         }

         INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, output_level, "<--Analyzed function " + fname);
      }
   }
   already_executed = true;
   if(modified.size())
   {
      for(const auto& f_id : modified)
      {
         AppM->GetFunctionBehavior(f_id)->UpdateBBVersion();
      }
      return DesignFlowStep_Status::SUCCESS;
   }
   return DesignFlowStep_Status::UNCHANGED;
}

void InterfaceInfer::ChasePointerInterfaceRecurse(CustomOrderedSet<unsigned>& Visited, tree_nodeRef ssa_node,
                                                  std::list<tree_nodeRef>& writeStmt, std::list<tree_nodeRef>& readStmt,
                                                  interface_info& info)
{
   const auto TM = AppM->get_tree_manager();
   enum call_type
   {
      ct_forward,
      ct_read,
      ct_write
   };
   const auto propagate_arg_use = [&](tree_nodeRef arg_var, size_t use_count, tree_nodeRef fd_node,
                                      const std::vector<tree_nodeRef>& call_args, tree_nodeRef& ssa_var) -> call_type {
      THROW_ASSERT(arg_var && fd_node, "unexpected condition");
      ssa_var = arg_var;
      const auto call_fd = [&]() {
         const auto fd_kind = GET_CONST_NODE(fd_node)->get_kind();
         auto& fn = fd_node;
         if(fd_kind == addr_expr_K)
         {
            fn = GetPointerS<const addr_expr>(GET_CONST_NODE(fd_node))->op;
         }
         THROW_ASSERT(GET_CONST_NODE(fn)->get_kind() == function_decl_K,
                      "unexpected condition: " + GET_CONST_NODE(fn)->get_kind_text());
         return GetPointerS<const function_decl>(GET_CONST_NODE(fn));
      }();
      if(!call_fd->body)
      {
         const auto called_fname = [&]() {
            const auto fname = tree_helper::print_function_name(TM, call_fd);
            const auto demangled = string_demangle(fname);
            return demangled.size() ? demangled : fname;
         }();
         if(called_fname.find(STR_CST_interface_parameter_keyword) != std::string::npos)
         {
            if(!boost::starts_with(called_fname, info.interface_id + STR_CST_interface_parameter_keyword))
            {
               THROW_ERROR("Shared memory operation is not supported with required I/O interface setup.");
            }
            return call_type::ct_forward;
         }
         else if(called_fname.find("ac_channel") != std::string::npos)
         {
            if(called_fname.find("::_read") != std::string::npos)
            {
               if(call_fd->list_of_args.size() >= 1 && call_fd->list_of_args.size() <= 3)
               {
                  auto ret_type = GET_CONST_NODE(call_fd->type);
                  if(ret_type->get_kind() == function_type_K || ret_type->get_kind() == method_type_K)
                  {
                     const auto ft = GetPointerS<const function_type>(ret_type);
                     if(GET_CONST_NODE(ft->retn)->get_kind() != void_type_K)
                     {
                        ssa_var = ft->retn;
                     }
                     else
                     {
                        THROW_ASSERT(call_fd->list_of_args.size() >= 2, "unexpected condition");
                        THROW_ASSERT(call_args.size() >= 2, "unexpected condition");
                        auto first_arg = call_args.at(0);
                        THROW_ASSERT(tree_helper::IsPointerType(first_arg), "unexpected condition");
                        ssa_var = TM->CGetTreeReindex(tree_helper::GetBaseVariable(first_arg)->index);
                        THROW_ASSERT(ssa_var != first_arg, "unexpected condition");
                     }
                  }
                  else
                  {
                     THROW_ERROR("unexpected condition");
                  }
               }
               else
               {
                  THROW_ERROR("unexpected condition");
               }
               return call_type::ct_read;
            }
            else if(called_fname.find("::_write") != std::string::npos)
            {
               if(call_fd->list_of_args.size() == 2)
               {
                  auto ret_type = GET_CONST_NODE(call_fd->type);
                  if(ret_type->get_kind() == void_type_K)
                  {
                     THROW_ERROR("unexpected condition");
                  }
                  if(tree_helper::IsPointerType(call_fd->list_of_args.at(1)))
                  {
                     auto second_arg = call_args.at(1);
                     ssa_var = TM->CGetTreeReindex(tree_helper::GetBaseVariable(second_arg)->index);
                     THROW_ASSERT(ssa_var != second_arg, "unexpected condition");
                  }
                  else
                  {
                     ssa_var = call_fd->list_of_args.at(1);
                  }
               }
               else
               {
                  THROW_ERROR("unexpected condition");
               }
               return call_type::ct_write;
            }
            THROW_UNREACHABLE("AC channel method not supported: " + called_fname);
         }
         THROW_UNREACHABLE("Hardware function interfacing not supported.");
      }

      size_t par_index = 0U;
      for(auto use_idx = 0U; use_idx < use_count; ++use_idx, ++par_index)
      {
         // look for the actual vs formal parameter binding
         par_index = [&](size_t start_idx) {
            for(auto idx = start_idx; idx < call_args.size(); ++idx)
            {
               if(GET_INDEX_CONST_NODE(call_args[idx]) == GET_INDEX_CONST_NODE(arg_var))
               {
                  return idx;
               }
            }
            THROW_ERROR("Use of " + arg_var->ToString() + " not found.");
            return static_cast<size_t>(-1);
         }(par_index);
         THROW_ASSERT(call_fd->list_of_args.size() > par_index, "unexpected condition");
         const auto call_arg_id = GET_INDEX_CONST_NODE(call_fd->list_of_args[par_index]);

         const auto call_arg_ssa_id = AppM->getSSAFromParm(call_fd->index, call_arg_id);
         const auto call_arg_ssa = TM->CGetTreeReindex(call_arg_ssa_id);
         THROW_ASSERT(GET_CONST_NODE(call_arg_ssa)->get_kind() == ssa_name_K, "");
         if(GetPointerS<const ssa_name>(GET_CONST_NODE(call_arg_ssa))->CGetUseStmts().size())
         {
            /// propagate design interfaces
            INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level,
                           "-->Pointer forwarded as function argument " + STR(par_index));
            ChasePointerInterfaceRecurse(Visited, call_arg_ssa, writeStmt, readStmt, info);
            INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "<--Sub-function done");
         }
      }
      return call_type::ct_forward;
   };
   const auto push_stmt = [&](tree_nodeRef stmt, call_type ct, tree_nodeRef val) {
      if(ct == call_type::ct_forward)
      {
         return;
      }
      if(ct == call_type::ct_read)
      {
         readStmt.push_back(stmt);
         info.update(val, "", parameters);
         INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "---  LOAD OPERATION");
      }
      else if(ct == call_type::ct_write)
      {
         writeStmt.push_back(stmt);
         info.update(val, "", parameters);
         INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "---  STORE OPERATION");
      }
   };
   if(!Visited.insert(GET_INDEX_CONST_NODE(ssa_node)).second)
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level,
                     "---SKIPPED FUNCTION: already visited through argument " + ssa_node->ToString());
      return;
   }

   std::queue<tree_nodeRef> chase_ssa;
   chase_ssa.push(ssa_node);
   while(chase_ssa.size())
   {
      ssa_node = chase_ssa.front();
      const auto ssa_var = GetPointer<const ssa_name>(GET_CONST_NODE(ssa_node));
      chase_ssa.pop();
      INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level,
                     "-->SSA VARIABLE: " + ssa_var->ToString() + " with " + STR(ssa_var->CGetUseStmts().size()) +
                         " use statements");
      for(const auto& stmt_count : ssa_var->CGetUseStmts())
      {
         const auto use_stmt = GET_CONST_NODE(stmt_count.first);
         const auto& use_count = stmt_count.second;
         INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "---STMT: " + use_stmt->ToString());
         if(const auto ga = GetPointer<const gimple_assign>(use_stmt))
         {
            const auto op0_kind = GET_CONST_NODE(ga->op0)->get_kind();
            const auto op1_kind = GET_CONST_NODE(ga->op1)->get_kind();
            if(op0_kind == mem_ref_K)
            {
               if(op1_kind == mem_ref_K)
               {
                  THROW_ERROR("Pattern currently not supported: *x=*y; " + use_stmt->ToString());
               }
               else
               {
                  THROW_ASSERT(op1_kind == ssa_name_K || GetPointer<const cst_node>(GET_CONST_NODE(ga->op1)),
                               "unexpected condition");
                  if(GetPointer<const cst_node>(GET_CONST_NODE(ga->op1)) ||
                     GetPointer<const ssa_name>(GET_CONST_NODE(ga->op1)) != ssa_var)
                  {
                     push_stmt(stmt_count.first, call_type::ct_write, ga->op1);
                  }
               }
            }
            else if(op1_kind == mem_ref_K)
            {
               push_stmt(stmt_count.first, call_type::ct_read, ga->op0);
               if(tree_helper::IsPointerType(ga->op0))
               {
                  INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "---Pointer to pointer interface propagation");
                  chase_ssa.push(ga->op0);
               }
            }
            else if(op1_kind == call_expr_K)
            {
               const auto ce = GetPointerS<const call_expr>(GET_CONST_NODE(ga->op1));
               const auto return_type = tree_helper::CGetType(ga->op0);
               if(tree_helper::IsPointerType(return_type))
               {
                  THROW_ERROR("unexpected pattern");
               }
               tree_nodeRef ssa_val;
               const auto ct = propagate_arg_use(ssa_node, use_count, ce->fn, ce->args, ssa_val);
               push_stmt(stmt_count.first, ct, ssa_val);
            }
            else if(op1_kind == eq_expr_K || op1_kind == ne_expr_K || op1_kind == gt_expr_K || op1_kind == lt_expr_K ||
                    op1_kind == ge_expr_K || op1_kind == le_expr_K)
            {
               THROW_WARNING("Pattern potentially not supported: pointer parameter is used in a compare statement: " +
                             use_stmt->ToString() + ":" + GET_CONST_NODE(ga->op1)->get_kind_text());
            }
            else
            {
               if(!tree_helper::IsPointerType(ga->op0))
               {
                  THROW_WARNING("Pattern potentially not supported: parameter converted to non-pointer type: " +
                                use_stmt->ToString() + ":" + GET_CONST_NODE(ga->op1)->get_kind_text());
               }
               else if(op1_kind != nop_expr_K && op1_kind != view_convert_expr_K && op1_kind != ssa_name_K &&
                       op1_kind != pointer_plus_expr_K && op1_kind != cond_expr_K)
               {
                  THROW_WARNING("Pattern potentially not supported: parameter used in a non-supported statement: " +
                                use_stmt->ToString() + ":" + GET_CONST_NODE(ga->op1)->get_kind_text());
               }
               ChasePointerInterfaceRecurse(Visited, ga->op0, writeStmt, readStmt, info);
            }
         }
         else if(const auto gc = GetPointer<const gimple_call>(use_stmt))
         {
            THROW_ASSERT(gc->fn, "unexpected condition");
            const auto fn_node = GET_CONST_NODE(gc->fn);
            if(fn_node->get_kind() == addr_expr_K)
            {
               const auto ae = GetPointerS<const addr_expr>(fn_node);
               const auto ae_op = GET_CONST_NODE(ae->op);
               if(ae_op->get_kind() == function_decl_K)
               {
                  tree_nodeRef ssa_val;
                  const auto ct = propagate_arg_use(ssa_node, use_count, ae->op, gc->args, ssa_val);
                  push_stmt(stmt_count.first, ct, ssa_val);
               }
               else
               {
                  THROW_ERROR("unexpected pattern: " + ae_op->ToString());
               }
            }
            else if(fn_node)
            {
               THROW_ERROR("unexpected pattern: " + fn_node->ToString());
            }
            else
            {
               THROW_ERROR("unexpected pattern");
            }
         }
         else if(const auto gp = GetPointer<const gimple_phi>(use_stmt))
         {
            THROW_ASSERT(ssa_var, "unexpected condition");
            THROW_ASSERT(!ssa_var->virtual_flag, "unexpected condition");
            ChasePointerInterfaceRecurse(Visited, gp->res, writeStmt, readStmt, info);
         }
         else
         {
            THROW_ERROR("USE PATTERN unexpected: " + use_stmt->ToString());
         }
      }
      INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "<--");
   }
}

void InterfaceInfer::ChasePointerInterface(tree_nodeRef ssa_node, std::list<tree_nodeRef>& writeStmt,
                                           std::list<tree_nodeRef>& readStmt, interface_info& info)
{
   CustomOrderedSet<unsigned> Visited;
   INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "-->Parameter uses:");
   ChasePointerInterfaceRecurse(Visited, ssa_node, writeStmt, readStmt, info);
   INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "<--");
}

void InterfaceInfer::setReadInterface(tree_nodeRef stmt, const std::string& arg_name,
                                      std::set<std::string>& operationsR, bool commonRWSignature,
                                      tree_nodeConstRef interface_datatype, const tree_manipulationRef tree_man,
                                      const tree_managerRef TM)
{
   const auto gn = GetPointerS<gimple_node>(GET_NODE(stmt));
   THROW_ASSERT(gn->scpe && GET_CONST_NODE(gn->scpe)->get_kind() == function_decl_K, "expected a function_decl scope");
   const auto fd = GetPointerS<function_decl>(GET_CONST_NODE(gn->scpe));
   const auto fname = tree_helper::GetMangledFunctionName(fd);
   INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "-->LOAD from " + fname + ":");
   INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "---BEFORE: " + stmt->ToString());

   THROW_ASSERT(fd->body, "expected a body");
   const auto sl = GetPointerS<statement_list>(GET_NODE(fd->body));
   const auto curr_bb = sl->list_of_bloc.at(gn->bb_index);
   const auto ret_call = GET_NODE(stmt)->get_kind() == gimple_assign_K &&
                         GET_NODE(GetPointerS<gimple_assign>(GET_NODE(stmt))->op1)->get_kind() == call_expr_K;
   const auto ref_call = GET_NODE(stmt)->get_kind() == gimple_call_K;
   if(ret_call || ref_call)
   {
      bool valid_var;
      tree_nodeRef valid_ptr;
      tree_nodeConstRef data_type;
      tree_nodeRef stmt_op0;
      tree_nodeRef data_ptr;
      if(ret_call)
      {
         const auto ga = GetPointerS<const gimple_assign>(GET_CONST_NODE(stmt));
         stmt_op0 = ga->op0;
         const auto ce = GetPointerS<const call_expr>(GET_CONST_NODE(ga->op1));
         valid_var = ce->args.size() == 2;
         data_type = tree_helper::CGetType(ga->op0);
         if(valid_var)
         {
            valid_ptr = ce->args.at(1);
         }
         else
         {
            THROW_ASSERT(ce->args.size() == 1, "unexpected condition");
         }
      }
      else
      {
         const auto gc = GetPointerS<const gimple_call>(GET_CONST_NODE(stmt));
         valid_var = gc->args.size() == 3;
         auto first_arg = gc->args.at(0);
         THROW_ASSERT(tree_helper::IsPointerType(first_arg), "unexpected condition");
         data_ptr = first_arg;
         auto data_obj = TM->CGetTreeReindex(tree_helper::GetBaseVariable(first_arg)->index);
         THROW_ASSERT(data_obj != first_arg, "unexpected condition");
         data_type = tree_helper::CGetType(data_obj);
         if(valid_var)
         {
            valid_ptr = gc->args.at(2);
         }
         else
         {
            THROW_ASSERT(gc->args.size() == 2, "unexpected condition");
         }
      }
      THROW_ASSERT(!gn->memdef && !gn->memuse, "");
      THROW_ASSERT(gn->vdef, "");
      const auto vdef = gn->vdef;

      const auto data_size = tree_helper::Size(data_type);
      const auto sel_type = tree_man->GetBooleanType();
      const auto ret_type = tree_man->GetCustomIntegerType(data_size + 1, true);
      const auto out_type = tree_man->GetCustomIntegerType(data_size, true);
      const auto out_ptr_type = tree_man->GetPointerType(out_type);
      const auto fdecl_node = [&]() {
         const auto interface_fname = ENCODE_FDNAME(arg_name, valid_var ? "_ReadAsync" : "_Read", "Channel");
         operationsR.insert(interface_fname);
         std::vector<tree_nodeConstRef> argsT;
         argsT.push_back(sel_type);
         return tree_man->create_function_decl(interface_fname, fd->scpe, argsT, ret_type, BUILTIN_SRCP, false);
      }();

      std::vector<tree_nodeRef> args;
      args.push_back(TM->CreateUniqueIntegerCst(valid_var, sel_type));
      const auto new_ce = tree_man->CreateCallExpr(fdecl_node, args, BUILTIN_SRCP);
      const auto new_call = tree_man->CreateGimpleAssign(ret_type, nullptr, nullptr, new_ce, fd->index, BUILTIN_SRCP);
      curr_bb->PushBefore(new_call, stmt, AppM);
      INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "--- AFTER: " + new_call->ToString());

      // Mask and cast read data
      const auto retval = GetPointerS<const gimple_assign>(GET_CONST_NODE(new_call))->op0;
      const auto be_mask = tree_man->create_binary_operation(
          ret_type, retval, TM->CreateUniqueIntegerCst((APInt(1) << data_size) - 1, ret_type), BUILTIN_SRCP,
          bit_and_expr_K);
      const auto ga_mask = tree_man->CreateGimpleAssign(ret_type, nullptr, nullptr, be_mask, fd->index, BUILTIN_SRCP);
      curr_bb->PushBefore(ga_mask, stmt, AppM);
      INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "---  MASK: " + ga_mask->ToString());
      const auto data_mask = GetPointerS<const gimple_assign>(GET_CONST_NODE(ga_mask))->op0;

      tree_nodeRef last_stmt;
      if(ret_call)
      {
         const auto nop_node = tree_man->create_unary_operation(data_type, data_mask, BUILTIN_SRCP, nop_expr_K);
         const auto ga_data = tree_man->create_gimple_modify_stmt(stmt_op0, nop_node, fd->index, BUILTIN_SRCP);
         curr_bb->PushBefore(ga_data, stmt, AppM);
         INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "---  DATA: " + ga_data->ToString());
         last_stmt = ga_data;
      }
      else
      {
         const auto ga_nop = tree_man->CreateNopExpr(data_mask, out_type, nullptr, nullptr, fd->index);
         curr_bb->PushBefore(ga_nop, stmt, AppM);
         INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "---  NOP: " + ga_nop->ToString());
         const auto ga_nop_var_ptr = GetPointerS<const gimple_assign>(GET_CONST_NODE(ga_nop))->op0;

         const auto data_ptr_type = tree_helper::CGetType(data_ptr);
         const auto data_ref = tree_man->create_binary_operation(
             data_type, data_ptr, TM->CreateUniqueIntegerCst(0, data_ptr_type), BUILTIN_SRCP, mem_ref_K);
         const auto ga_store = tree_man->create_gimple_modify_stmt(data_ref, ga_nop_var_ptr, fd->index, BUILTIN_SRCP);
         if(valid_var)
         {
            auto newSSAVdef = tree_man->create_ssa_name(tree_nodeRef(), tree_helper::CGetType(vdef), tree_nodeRef(),
                                                        tree_nodeRef(), false, true);
            GetPointerS<gimple_assign>(GET_NODE(ga_store))->SetVdef(newSSAVdef);
            for(const auto& vuse : GetPointerS<gimple_assign>(GET_NODE(stmt))->vuses)
            {
               if(GetPointerS<gimple_node>(GET_NODE(ga_store))->AddVuse(vuse))
               {
                  GetPointerS<ssa_name>(GET_NODE(vuse))->AddUseStmt(ga_store);
               }
            }
            THROW_ASSERT(GET_NODE(vdef)->get_kind() == ssa_name_K, "unexpected condition");
            auto vdef_ssa = GetPointerS<ssa_name>(GET_NODE(vdef));
            for(const auto& usingStmt : vdef_ssa->CGetUseStmts())
            {
               if(GetPointerS<gimple_node>(GET_NODE(usingStmt.first))->AddVuse(newSSAVdef))
               {
                  GetPointerS<ssa_name>(GET_NODE(newSSAVdef))->AddUseStmt(usingStmt.first);
               }
            }
            curr_bb->PushBefore(ga_store, stmt, AppM);
         }
         else
         {
            curr_bb->Replace(stmt, ga_store, true, AppM);
         }
         INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "--- STORE: " + ga_store->ToString());

         last_stmt = ga_store;
      }

      if(valid_var)
      {
         // Mask and cast valid bit
         const auto be_vshift = tree_man->create_binary_operation(
             ret_type, retval, TM->CreateUniqueIntegerCst(data_size, ret_type), BUILTIN_SRCP, rshift_expr_K);
         const auto ga_vshift =
             tree_man->CreateGimpleAssign(ret_type, nullptr, nullptr, be_vshift, fd->index, BUILTIN_SRCP);
         curr_bb->PushBefore(ga_vshift, stmt, AppM);
         INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "---VSHIFT: " + ga_vshift->ToString());
         const auto v_shift = GetPointerS<const gimple_assign>(GET_CONST_NODE(ga_vshift))->op0;
         const auto be_vmask = tree_man->create_binary_operation(
             ret_type, v_shift, TM->CreateUniqueIntegerCst(1, ret_type), BUILTIN_SRCP, bit_and_expr_K);
         const auto ga_vmask =
             tree_man->CreateGimpleAssign(ret_type, nullptr, nullptr, be_vmask, fd->index, BUILTIN_SRCP);
         curr_bb->PushBefore(ga_vmask, stmt, AppM);
         INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "--- VMASK: " + ga_vmask->ToString());
         const auto v_mask = GetPointerS<const gimple_assign>(GET_CONST_NODE(ga_vmask))->op0;
         const auto valid_ptr_type = tree_man->GetPointerType(tree_man->GetBooleanType());
         THROW_ASSERT(tree_helper::IsPointerType(valid_ptr_type), "unexpected condition");
         const auto valid_type = tree_helper::CGetPointedType(valid_ptr_type);
         const auto ga_valid = tree_man->CreateNopExpr(v_mask, valid_type, nullptr, nullptr, fd->index);
         curr_bb->PushBefore(ga_valid, stmt, AppM);
         INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "--- VALID: " + ga_valid->ToString());
         const auto valid_ref = GetPointerS<const gimple_assign>(GET_CONST_NODE(ga_valid))->op0;
         const auto valid_memref = tree_man->create_binary_operation(
             valid_type, valid_ptr, TM->CreateUniqueIntegerCst(0, valid_ptr_type), BUILTIN_SRCP, mem_ref_K);
         const auto ga_valid_store =
             tree_man->create_gimple_modify_stmt(valid_memref, valid_ref, fd->index, BUILTIN_SRCP);
         curr_bb->Replace(stmt, ga_valid_store, true, AppM);
         if(!ret_call)
         {
            auto ssaDef = GetPointerS<gimple_assign>(GET_NODE(last_stmt))->vdef;
            if(GetPointerS<gimple_node>(GET_NODE(ga_valid_store))->AddVuse(ssaDef))
            {
               GetPointerS<ssa_name>(GET_NODE(ssaDef))->AddUseStmt(ga_valid_store);
            }
         }
         INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "--- VALID STORE: " + ga_valid_store->ToString());
      }
      else if(ret_call)
      {
         curr_bb->RemoveStmt(stmt, AppM);
      }
   }
   else
   {
      THROW_ASSERT(stmt && GET_NODE(stmt)->get_kind() == gimple_assign_K, "unexpected condition");
      const auto ga = GetPointerS<gimple_assign>(GET_NODE(stmt));
      THROW_ASSERT(GET_NODE(ga->op1)->get_kind() == mem_ref_K, "unexpected condition");

      /// create the function_decl
      const auto actual_type = tree_helper::CGetType(ga->op0);
      const auto bit_size_type = tree_man->GetUnsignedIntegerType();
      const auto boolean_type = tree_man->GetBooleanType();
      const auto fdecl_node = [&]() {
         const auto interface_fname = ENCODE_FDNAME(arg_name, "_Read", "");
         operationsR.insert(interface_fname);
         std::vector<tree_nodeConstRef> argsT;
         if(commonRWSignature)
         {
            argsT.push_back(boolean_type);
            argsT.push_back(bit_size_type);
            argsT.push_back(interface_datatype);
         }
         argsT.push_back(tree_helper::CGetType(ga->op1));
         return tree_man->create_function_decl(interface_fname, fd->scpe, argsT, interface_datatype, BUILTIN_SRCP,
                                               false);
      }();
      std::vector<tree_nodeRef> args;
      if(commonRWSignature)
      {
         const auto sel_value = TM->CreateUniqueIntegerCst(0, boolean_type);
         const auto size_value =
             TM->CreateUniqueIntegerCst(static_cast<long long>(tree_helper::Size(actual_type)), bit_size_type);

         const auto data_value = TM->CreateUniqueIntegerCst(0, interface_datatype);
         args.push_back(sel_value);
         args.push_back(size_value);
         args.push_back(data_value);
      }

      THROW_ASSERT(GET_CONST_NODE(ga->op1)->get_kind() == mem_ref_K, "unexpected condition");
      const auto mr = GetPointerS<const mem_ref>(GET_CONST_NODE(ga->op1));
      args.push_back(mr->op0);

      const auto ce = tree_man->CreateCallExpr(fdecl_node, args, BUILTIN_SRCP);
      if(tree_helper::IsSameType(interface_datatype, actual_type))
      {
         TM->ReplaceTreeNode(stmt, ga->op1, ce);
         CustomUnorderedSet<unsigned int> AV;
         CallGraphManager::addCallPointAndExpand(AV, AppM, GET_INDEX_CONST_NODE(ga->scpe),
                                                 GET_INDEX_CONST_NODE(fdecl_node), GET_INDEX_CONST_NODE(stmt),
                                                 FunctionEdgeInfo::CallType::direct_call, DEBUG_LEVEL_NONE);
         GetPointer<HLS_manager>(AppM)->design_interface_io[fname][ga->bb_index][arg_name].push_back(
             GET_INDEX_CONST_NODE(stmt));
         INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "--- AFTER: " + stmt->ToString());
      }
      else
      {
         const auto is_real = tree_helper::IsRealType(actual_type);
         const auto tmp_type =
             is_real ? tree_man->GetCustomIntegerType(tree_helper::Size(actual_type), true) : interface_datatype;
         const auto tmp_ssa = tree_man->create_ssa_name(nullptr, tmp_type, nullptr, nullptr);
         const auto gc = tree_man->create_gimple_modify_stmt(tmp_ssa, ce, fd->index, BUILTIN_SRCP);
         curr_bb->Replace(stmt, gc, true, AppM);
         const auto vc = tree_man->create_unary_operation(actual_type, tmp_ssa, BUILTIN_SRCP,
                                                          is_real ? view_convert_expr_K : nop_expr_K);
         const auto cast = tree_man->create_gimple_modify_stmt(ga->op0, vc, fd->index, BUILTIN_SRCP);
         curr_bb->PushAfter(cast, gc, AppM);
         GetPointer<HLS_manager>(AppM)->design_interface_io[fname][curr_bb->number][arg_name].push_back(
             GET_INDEX_CONST_NODE(gc));

         INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "--- AFTER: " + gc->ToString());
         INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "---   NOP: " + stmt->ToString());
      }
   }
   INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "<--");
}

void InterfaceInfer::setWriteInterface(tree_nodeRef stmt, const std::string& arg_name,
                                       std::set<std::string>& operationsW, bool commonRWSignature,
                                       tree_nodeConstRef interface_datatype, const tree_manipulationRef tree_man,
                                       const tree_managerRef TM)
{
   const auto gn = GetPointerS<gimple_node>(GET_NODE(stmt));
   THROW_ASSERT(gn->scpe && GET_CONST_NODE(gn->scpe)->get_kind() == function_decl_K, "expected a function_decl scope");
   const auto fd = GetPointerS<function_decl>(GET_CONST_NODE(gn->scpe));
   const auto fname = tree_helper::GetMangledFunctionName(fd);
   INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "-->STORE from " + fname + ":");
   INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "---BEFORE: " + stmt->ToString());

   THROW_ASSERT(fd->body, "expected a body");
   const auto sl = GetPointerS<statement_list>(GET_NODE(fd->body));
   const auto curr_bb = sl->list_of_bloc.at(gn->bb_index);
   const auto ret_call = GET_NODE(stmt)->get_kind() == gimple_assign_K &&
                         GET_NODE(GetPointerS<gimple_assign>(GET_NODE(stmt))->op1)->get_kind() == call_expr_K;
   const auto ref_call = GET_NODE(stmt)->get_kind() == gimple_call_K;
   if(ret_call || ref_call)
   {
      tree_nodeRef data_obj;
      tree_nodeRef data_ptr;
      tree_nodeConstRef ret_type;
      bool valid_var;
      bool isAStruct = false;
      if(ret_call)
      {
         const auto ga = GetPointerS<const gimple_assign>(GET_CONST_NODE(stmt));
         const auto ce = GetPointerS<const call_expr>(GET_CONST_NODE(ga->op1));
         THROW_ASSERT(ce->args.size() == 2, "unexpected condition");
         data_obj = ce->args.at(1);
         if(tree_helper::IsPointerType(data_obj))
         {
            data_ptr = data_obj;
            data_obj = TM->CGetTreeReindex(tree_helper::GetBaseVariable(data_obj)->index);
            THROW_ASSERT(data_obj != data_ptr, "unexpected condition");
            isAStruct = true;
         }
         valid_var = true;
         ret_type = tree_helper::CGetType(ga->op0);
      }
      else
      {
         const auto gc = GetPointerS<const gimple_call>(GET_CONST_NODE(stmt));
         THROW_ASSERT(gc->args.size() == 2, "unexpected condition");
         data_obj = gc->args.at(1);
         if(tree_helper::IsPointerType(data_obj))
         {
            data_ptr = data_obj;
            data_obj = TM->CGetTreeReindex(tree_helper::GetBaseVariable(data_obj)->index);
            THROW_ASSERT(data_obj != data_ptr, "unexpected condition");
            isAStruct = true;
         }
         valid_var = false;
         ret_type = tree_man->GetVoidType();
      }

      const auto data_type = tree_helper::CGetType(data_obj);
      const auto data_size = tree_helper::Size(data_type);
      const auto sel_type = tree_man->GetBooleanType();
      const auto out_type = tree_man->GetCustomIntegerType(data_size, true);
      const auto out_ptr_type = tree_man->GetPointerType(out_type);
      const auto fdecl_node = [&]() {
         const auto interface_fname = ENCODE_FDNAME(arg_name, valid_var ? "_WriteAsync" : "_Write", "Channel");
         operationsW.insert(interface_fname);
         std::vector<tree_nodeConstRef> argsT;
         argsT.push_back(sel_type);
         argsT.push_back(data_type);
         return tree_man->create_function_decl(interface_fname, fd->scpe, argsT, ret_type, BUILTIN_SRCP, false);
      }();

      tree_nodeRef lastStmt;
      if(isAStruct)
      {
         const auto ga_ptr = tree_man->CreateNopExpr(data_ptr, out_ptr_type, nullptr, nullptr, fd->index);
         curr_bb->PushBefore(ga_ptr, stmt, AppM);
         INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "--- PCAST: " + ga_ptr->ToString());
         const auto out_data_ptr = GetPointerS<const gimple_assign>(GET_CONST_NODE(ga_ptr))->op0;
         const auto data_ref = tree_man->create_binary_operation(
             out_type, out_data_ptr, TM->CreateUniqueIntegerCst(0, out_ptr_type), BUILTIN_SRCP, mem_ref_K);
         const auto ga_load =
             tree_man->CreateGimpleAssign(out_type, nullptr, nullptr, data_ref, fd->index, BUILTIN_SRCP);
         curr_bb->Replace(stmt, ga_load, true, AppM);
         INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "---  LOAD: " + ga_load->ToString());
         data_obj = GetPointerS<const gimple_assign>(GET_CONST_NODE(ga_load))->op0;
         lastStmt = ga_load;
      }

      std::vector<tree_nodeRef> args;
      args.push_back(TM->CreateUniqueIntegerCst(valid_var, sel_type));
      args.push_back(data_obj);
      if(valid_var)
      {
         const auto ga_stmt = GetPointerS<const gimple_assign>(GET_CONST_NODE(stmt));
         const auto ce = tree_man->CreateCallExpr(fdecl_node, args, BUILTIN_SRCP);
         const auto ga_call = tree_man->create_gimple_modify_stmt(ga_stmt->op0, ce, fd->index, BUILTIN_SRCP);
         if(isAStruct)
         {
            curr_bb->PushAfter(ga_call, lastStmt, AppM);
         }
         else
         {
            curr_bb->Replace(stmt, ga_call, true, AppM);
         }
         INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "--- AFTER: " + ga_call->ToString());
      }
      else
      {
         const auto gc = tree_man->create_gimple_call(fdecl_node, args, fd->index, BUILTIN_SRCP);
         if(isAStruct)
         {
            curr_bb->PushAfter(gc, lastStmt, AppM);
         }
         else
         {
            curr_bb->Replace(stmt, gc, true, AppM);
         }
         INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "--- AFTER: " + gc->ToString());
      }
   }
   else
   {
      THROW_ASSERT(stmt && GET_NODE(stmt)->get_kind() == gimple_assign_K, "unexpected condition");
      const auto ga = GetPointerS<gimple_assign>(GET_NODE(stmt));
      THROW_ASSERT(GET_NODE(ga->op0)->get_kind() == mem_ref_K, "unexpected condition");

      auto value_node = ga->op1;
      auto actual_type = tree_helper::CGetType(value_node);
      if(tree_helper::IsSameType(interface_datatype, actual_type))
      {
         tree_nodeRef nop;
         if(tree_helper::IsRealType(actual_type))
         {
            const auto int_type = tree_man->GetCustomIntegerType(tree_helper::Size(actual_type), true);
            const auto vc = tree_man->create_unary_operation(int_type, value_node, BUILTIN_SRCP, view_convert_expr_K);
            value_node = tree_man->create_ssa_name(nullptr, int_type, nullptr, nullptr);
            nop = tree_man->create_gimple_modify_stmt(value_node, vc, fd->index, BUILTIN_SRCP);
         }
         else
         {
            nop = tree_man->CreateNopExpr(value_node, interface_datatype, nullptr, nullptr,
                                          GET_INDEX_CONST_NODE(ga->scpe));
            value_node = GetPointerS<const gimple_assign>(GET_CONST_NODE(nop))->op0;
         }
         curr_bb->PushBefore(nop, stmt, AppM);
         INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "---   NOP: " + nop->ToString());
      }
      const auto boolean_type = tree_man->GetBooleanType();
      const auto bit_size_type = tree_man->GetUnsignedIntegerType();

      /// create the function_decl
      const auto fdecl_node = [&]() {
         const auto interface_fname = ENCODE_FDNAME(arg_name, "_Write", "");
         operationsW.insert(interface_fname);
         std::vector<tree_nodeConstRef> argsT;
         if(commonRWSignature)
         {
            argsT.push_back(boolean_type);
         }
         argsT.push_back(bit_size_type);
         argsT.push_back(interface_datatype);
         argsT.push_back(tree_helper::CGetType(ga->op0));

         return tree_man->create_function_decl(interface_fname, fd->scpe, argsT, tree_man->GetVoidType(), BUILTIN_SRCP,
                                               false);
      }();

      std::vector<tree_nodeRef> args;
      if(commonRWSignature)
      {
         args.push_back(TM->CreateUniqueIntegerCst(1, boolean_type));
      }
      args.push_back(TM->CreateUniqueIntegerCst(static_cast<long long>(tree_helper::Size(actual_type)), bit_size_type));
      args.push_back(value_node);
      const auto mr = GetPointerS<const mem_ref>(GET_CONST_NODE(ga->op0));
      args.push_back(mr->op0);

      const auto gc = tree_man->create_gimple_call(fdecl_node, args, GET_INDEX_NODE(ga->scpe), BUILTIN_SRCP);
      curr_bb->Replace(stmt, gc, true, AppM);
      GetPointer<HLS_manager>(AppM)->design_interface_io[fname][curr_bb->number][arg_name].push_back(
          GET_INDEX_CONST_NODE(gc));

      INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "--- AFTER: " + gc->ToString());
   }
   INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "<--");
}

void InterfaceInfer::create_resource_Read_simple(const std::set<std::string>& operations, const std::string& arg_name,
                                                 const interface_info& info, bool IO_port) const
{
   if(operations.empty())
   {
      return;
   }
   const std::string ResourceName = ENCODE_FDNAME(arg_name, "_Read_", info.name);
   const auto HLSMgr = GetPointer<HLS_manager>(AppM);
   const auto HLS_D = HLSMgr->get_HLS_device();
   const auto TechMan = HLS_D->get_technology_manager();
   if(!TechMan->is_library_manager(INTERFACE_LIBRARY) ||
      !TechMan->get_library_manager(INTERFACE_LIBRARY)->is_fu(ResourceName))
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level,
                     "-->Creating interface resource: " + INTERFACE_LIBRARY + ":" + ResourceName);
      structural_managerRef CM(new structural_manager(parameters));
      structural_type_descriptorRef module_type(new structural_type_descriptor(ResourceName));
      CM->set_top_info(ResourceName, module_type);
      const auto interface_top = CM->get_circ();
      /// add description and license
      GetPointerS<module>(interface_top)->set_description("Interface module for function: " + ResourceName);
      GetPointerS<module>(interface_top)->set_copyright(GENERATED_COPYRIGHT);
      GetPointerS<module>(interface_top)->set_authors("Component automatically generated by bambu");
      GetPointerS<module>(interface_top)->set_license(GENERATED_LICENSE);
      GetPointerS<module>(interface_top)->set_multi_unit_multiplicity(1U);
      const auto if_name = info.name == "ovalid" ? "none" : info.name;
      const auto is_unbounded = if_name == "valid" || if_name == "handshake" || if_name == "fifo" || if_name == "axis";

      const auto address_bitsize = HLSMgr->get_address_bitsize();
      structural_type_descriptorRef addrType(new structural_type_descriptor("bool", address_bitsize));
      structural_type_descriptorRef dataType(new structural_type_descriptor("bool", info.bitwidth));
      const auto out_bitsize = if_name == "fifo" ? (info.bitwidth + 1U) : info.bitwidth;
      structural_type_descriptorRef outType(new structural_type_descriptor("bool", out_bitsize));
      structural_type_descriptorRef bool_type(new structural_type_descriptor("bool", 0));
      if(is_unbounded)
      {
         CM->add_port(CLOCK_PORT_NAME, port_o::IN, interface_top, bool_type);
         CM->add_port(RESET_PORT_NAME, port_o::IN, interface_top, bool_type);
         CM->add_port_vector(DONE_PORT_NAME, port_o::OUT, 1U, interface_top, bool_type);
      }
      if(is_unbounded || info.name == "acknowledge")
      {
         CM->add_port_vector(START_PORT_NAME, port_o::IN, 1U, interface_top, bool_type);
      }
      const auto addrPort = CM->add_port("in1", port_o::IN, interface_top, addrType);
      GetPointerS<port_o>(addrPort)->set_is_addr_bus(true);
      GetPointerS<port_o>(addrPort)->set_port_alignment(info.alignment);
      CM->add_port("out1", port_o::OUT, interface_top, outType);

      std::string port_data_name = "_" + arg_name;
      port_o::port_interface port_if = port_o::port_interface::PI_RNONE;
      if(if_name == "axis")
      {
         port_data_name = "_s_axis_" + arg_name + "_TDATA";
         port_if = port_o::port_interface::PI_S_AXIS_TDATA;
      }
      else if(if_name == "fifo")
      {
         port_data_name += "_dout";
         port_if = port_o::port_interface::PI_FDOUT;
      }
      else if(IO_port)
      {
         port_data_name += "_i";
      }
      const auto inPort = CM->add_port(port_data_name, port_o::IN, interface_top, dataType);
      GetPointerS<port_o>(inPort)->set_port_interface(port_if);
      if(if_name == "acknowledge" || if_name == "handshake")
      {
         const auto inPort_o_ack =
             CM->add_port("_" + arg_name + (IO_port ? "_i" : "") + "_ack", port_o::OUT, interface_top, bool_type);
         GetPointerS<port_o>(inPort_o_ack)->set_port_interface(port_o::port_interface::PI_RACK);
      }
      if(if_name == "valid" || if_name == "handshake")
      {
         const auto inPort_o_vld =
             CM->add_port("_" + arg_name + (IO_port ? "_i" : "") + "_vld", port_o::IN, interface_top, bool_type);
         GetPointerS<port_o>(inPort_o_vld)->set_port_interface(port_o::port_interface::PI_RVALID);
      }
      if(if_name == "fifo")
      {
         const auto inPort_empty_n = CM->add_port("_" + arg_name + "_empty_n", port_o::IN, interface_top, bool_type);
         GetPointerS<port_o>(inPort_empty_n)->set_port_interface(port_o::port_interface::PI_EMPTY_N);
         const auto inPort_read = CM->add_port("_" + arg_name + "_read", port_o::OUT, interface_top, bool_type);
         GetPointerS<port_o>(inPort_read)->set_port_interface(port_o::port_interface::PI_READ);
      }
      if(if_name == "axis")
      {
         const auto inPort_empty_n =
             CM->add_port("_s_axis_" + arg_name + "_TVALID", port_o::IN, interface_top, bool_type);
         GetPointerS<port_o>(inPort_empty_n)->set_port_interface(port_o::port_interface::PI_S_AXIS_TVALID);
         const auto inPort_read =
             CM->add_port("_s_axis_" + arg_name + "_TREADY", port_o::OUT, interface_top, bool_type);
         GetPointerS<port_o>(inPort_read)->set_port_interface(port_o::port_interface::PI_S_AXIS_TREADY);
      }

      CM->add_NP_functionality(interface_top, NP_functionality::LIBRARY, "in1 out1");
      CM->add_NP_functionality(interface_top, NP_functionality::VERILOG_GENERATOR,
                               "Read_" + if_name + "ModuleGenerator");
      TechMan->add_resource(INTERFACE_LIBRARY, ResourceName, CM);
      for(const auto& fdName : operations)
      {
         TechMan->add_operation(INTERFACE_LIBRARY, ResourceName, fdName);
      }
      auto fu = GetPointerS<functional_unit>(TechMan->get_fu(ResourceName, INTERFACE_LIBRARY));
      fu->area_m = area_info::factory(parameters);
      fu->area_m->set_area_value(0);
      if(!is_unbounded)
      {
         fu->logical_type = functional_unit::COMBINATIONAL;
      }

      for(const auto& fdName : operations)
      {
         const auto op = GetPointer<operation>(fu->get_operation(fdName));
         op->time_m = time_info::factory(parameters);
         if(if_name == "fifo")
         {
            op->bounded = fdName.find("Async") != std::string::npos;
            const auto exec_time =
                (!op->bounded ? HLS_D->get_technology_manager()->CGetSetupHoldTime() : 0.0) + EPSILON;
            const auto cycles = op->bounded ? 1U : 0U;
            op->time_m->set_execution_time(exec_time, cycles);
         }
         else
         {
            op->bounded = !is_unbounded;
            const auto exec_time =
                (is_unbounded ? HLS_D->get_technology_manager()->CGetSetupHoldTime() : 0.0) + EPSILON;
            const auto cycles = if_name == "acknowledge" ? 1U : 0U;
            op->time_m->set_execution_time(exec_time, cycles);
         }
         op->time_m->set_synthesis_dependent(true);
      }
      HLSMgr->global_resource_constraints[std::make_pair(ResourceName, INTERFACE_LIBRARY)] = std::make_pair(1U, 1U);
      INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "<--Interface resource created");
   }
}

void InterfaceInfer::create_resource_Write_simple(const std::set<std::string>& operations, const std::string& arg_name,
                                                  const interface_info& info, bool IO_port) const
{
   if(operations.empty())
   {
      return;
   }
   const std::string ResourceName = ENCODE_FDNAME(arg_name, "_Write_", info.name);
   const auto HLSMgr = GetPointer<HLS_manager>(AppM);
   const auto HLS_D = HLSMgr->get_HLS_device();
   const auto TechMan = HLS_D->get_technology_manager();
   if(!operations.empty() && !(TechMan->is_library_manager(INTERFACE_LIBRARY) &&
                               TechMan->get_library_manager(INTERFACE_LIBRARY)->is_fu(ResourceName)))
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level,
                     "-->Creating interface resource: " + INTERFACE_LIBRARY + ":" + ResourceName);
      structural_managerRef CM(new structural_manager(parameters));
      structural_type_descriptorRef module_type(new structural_type_descriptor(ResourceName));
      CM->set_top_info(ResourceName, module_type);
      const auto interface_top = CM->get_circ();
      /// add description and license
      GetPointerS<module>(interface_top)->set_description("Interface module for function: " + ResourceName);
      GetPointerS<module>(interface_top)->set_copyright(GENERATED_COPYRIGHT);
      GetPointerS<module>(interface_top)->set_authors("Component automatically generated by bambu");
      GetPointerS<module>(interface_top)->set_license(GENERATED_LICENSE);
      GetPointerS<module>(interface_top)->set_multi_unit_multiplicity(1U);
      const auto if_name = info.name == "ovalid" ? "valid" : info.name;
      const auto is_unbounded =
          if_name == "acknowledge" || if_name == "handshake" || if_name == "fifo" || if_name == "axis";

      const auto address_bitsize = HLSMgr->get_address_bitsize();
      structural_type_descriptorRef addrType(new structural_type_descriptor("bool", address_bitsize));
      structural_type_descriptorRef dataType(new structural_type_descriptor("bool", info.bitwidth));
      const auto nbitDataSize = 64u - static_cast<unsigned>(__builtin_clzll(info.bitwidth));
      structural_type_descriptorRef rwsize(new structural_type_descriptor("bool", nbitDataSize));
      structural_type_descriptorRef rwtype(new structural_type_descriptor("bool", info.bitwidth));
      structural_type_descriptorRef bool_type(new structural_type_descriptor("bool", 0));
      if(is_unbounded || if_name == "none_registered")
      {
         CM->add_port(CLOCK_PORT_NAME, port_o::IN, interface_top, bool_type);
         CM->add_port(RESET_PORT_NAME, port_o::IN, interface_top, bool_type);
      }
      if(is_unbounded || if_name == "valid" || if_name == "none_registered")
      {
         CM->add_port_vector(START_PORT_NAME, port_o::IN, 1U, interface_top, bool_type);
      }
      if(is_unbounded)
      {
         CM->add_port_vector(DONE_PORT_NAME, port_o::OUT, 1U, interface_top, bool_type);
      }
      CM->add_port("in1", port_o::IN, interface_top, rwsize);
      CM->add_port("in2", port_o::IN, interface_top, rwtype);
      const auto addrPort = CM->add_port("in3", port_o::IN, interface_top, addrType);
      GetPointerS<port_o>(addrPort)->set_is_addr_bus(true);
      GetPointerS<port_o>(addrPort)->set_port_alignment(info.alignment);
      if(if_name == "fifo" || if_name == "axis")
      {
         CM->add_port("out1", port_o::OUT, interface_top, bool_type);
      }
      std::string port_data_name = "_" + arg_name;
      port_o::port_interface port_if = port_o::port_interface::PI_WNONE;
      if(if_name == "axis")
      {
         port_data_name = "_m_axis_" + arg_name + "_TDATA";
         port_if = port_o::port_interface::PI_M_AXIS_TDATA;
      }
      else if(if_name == "fifo")
      {
         port_data_name += "_din";
         port_if = port_o::port_interface::PI_FDIN;
      }
      else if(IO_port)
      {
         port_data_name += "_o";
      }
      const auto inPort_o = CM->add_port(port_data_name, port_o::OUT, interface_top, dataType);
      GetPointerS<port_o>(inPort_o)->set_port_interface(port_if);
      if(if_name == "acknowledge" || if_name == "handshake")
      {
         const auto inPort_o_ack =
             CM->add_port("_" + arg_name + (IO_port ? "_o" : "") + "_ack", port_o::IN, interface_top, bool_type);
         GetPointerS<port_o>(inPort_o_ack)->set_port_interface(port_o::port_interface::PI_WACK);
      }
      if(if_name == "valid" || if_name == "handshake")
      {
         const auto inPort_o_vld =
             CM->add_port("_" + arg_name + (IO_port ? "_o" : "") + "_vld", port_o::OUT, interface_top, bool_type);
         GetPointerS<port_o>(inPort_o_vld)->set_port_interface(port_o::port_interface::PI_WVALID);
      }
      if(if_name == "fifo")
      {
         const auto inPort_full_n = CM->add_port("_" + arg_name + "_full_n", port_o::IN, interface_top, bool_type);
         GetPointerS<port_o>(inPort_full_n)->set_port_interface(port_o::port_interface::PI_FULL_N);
         const auto inPort_read = CM->add_port("_" + arg_name + "_write", port_o::OUT, interface_top, bool_type);
         GetPointerS<port_o>(inPort_read)->set_port_interface(port_o::port_interface::PI_WRITE);
      }
      if(if_name == "axis")
      {
         const auto inPort_full_n =
             CM->add_port("_m_axis_" + arg_name + "_TREADY", port_o::IN, interface_top, bool_type);
         GetPointerS<port_o>(inPort_full_n)->set_port_interface(port_o::port_interface::PI_M_AXIS_TREADY);
         const auto inPort_read =
             CM->add_port("_m_axis_" + arg_name + "_TVALID", port_o::OUT, interface_top, bool_type);
         GetPointerS<port_o>(inPort_read)->set_port_interface(port_o::port_interface::PI_M_AXIS_TVALID);
      }

      CM->add_NP_functionality(interface_top, NP_functionality::LIBRARY, "in1 in2 in3");
      const auto writer = static_cast<HDLWriter_Language>(parameters->getOption<unsigned int>(OPT_writer_language));
      if((if_name == "none" || if_name == "none_registered") && writer == HDLWriter_Language::VHDL)
      {
         CM->add_NP_functionality(interface_top, NP_functionality::VHDL_GENERATOR,
                                  "Write_" + if_name + "ModuleGenerator");
      }
      else
      {
         CM->add_NP_functionality(interface_top, NP_functionality::VERILOG_GENERATOR,
                                  "Write_" + if_name + "ModuleGenerator");
      }
      TechMan->add_resource(INTERFACE_LIBRARY, ResourceName, CM);
      for(const auto& fdName : operations)
      {
         TechMan->add_operation(INTERFACE_LIBRARY, ResourceName, fdName);
      }
      auto fu = GetPointerS<functional_unit>(TechMan->get_fu(ResourceName, INTERFACE_LIBRARY));
      fu->area_m = area_info::factory(parameters);
      fu->area_m->set_area_value(0);
      if(!is_unbounded)
      {
         fu->logical_type = functional_unit::COMBINATIONAL;
      }

      for(const auto& fdName : operations)
      {
         const auto op_bounded = fdName.find("Async") != std::string::npos || !is_unbounded;
         const auto exec_time = (!op_bounded ? HLS_D->get_technology_manager()->CGetSetupHoldTime() : 0.0) + EPSILON;
         const auto cycles = [&]() {
            if(if_name == "none_registered")
            {
               return 2U;
            }
            else if(if_name == "none" || (if_name == "fifo" && op_bounded))
            {
               return 1U;
            }
            return 0U;
         }();

         const auto op = GetPointerS<operation>(fu->get_operation(fdName));
         op->time_m = time_info::factory(parameters);
         op->bounded = op_bounded;
         op->time_m->set_execution_time(exec_time, cycles);
         if(if_name == "none_registered")
         {
            op->time_m->set_stage_period(HLS_D->get_technology_manager()->CGetSetupHoldTime() + EPSILON);
            op->time_m->set_initiation_time(ControlStep(1U));
         }
         op->time_m->set_synthesis_dependent(true);
      }
      HLSMgr->global_resource_constraints[std::make_pair(ResourceName, INTERFACE_LIBRARY)] = std::make_pair(1U, 1U);
      INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "<--Interface resource created");
   }
}

void InterfaceInfer::create_resource_array(const std::set<std::string>& operationsR,
                                           const std::set<std::string>& operationsW, const std::string& bundle_name,
                                           const interface_info& info, unsigned long long arraySize) const
{
   const auto n_channels = parameters->getOption<unsigned int>(OPT_channels_number);
   const auto isDP = n_channels == 2;
   const auto n_resources = isDP ? 2U : 1U;
   const auto read_write_string = (isDP ? std::string("ReadWriteDP_") : std::string("ReadWrite_"));
   const auto ResourceName = ENCODE_FDNAME(bundle_name, "", "");
   const auto HLSMgr = GetPointerS<HLS_manager>(AppM);
   const auto HLS_D = HLSMgr->get_HLS_device();
   const auto TechMan = HLS_D->get_technology_manager();
   if(!TechMan->is_library_manager(INTERFACE_LIBRARY) ||
      !TechMan->get_library_manager(INTERFACE_LIBRARY)->is_fu(ResourceName))
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level,
                     "-->Creating interface resource: " + INTERFACE_LIBRARY + ":" + ResourceName +
                         " (multi: " + STR(n_resources) + ")");
      structural_objectRef interface_top;
      const structural_managerRef CM(new structural_manager(parameters));
      const structural_type_descriptorRef module_type(new structural_type_descriptor(ResourceName));
      CM->set_top_info(ResourceName, module_type);
      interface_top = CM->get_circ();
      /// add description and license
      GetPointerS<module>(interface_top)->set_description("Interface module for function: " + ResourceName);
      GetPointerS<module>(interface_top)->set_copyright(GENERATED_COPYRIGHT);
      GetPointerS<module>(interface_top)->set_authors("Component automatically generated by bambu");
      GetPointerS<module>(interface_top)->set_license(GENERATED_LICENSE);
      GetPointerS<module>(interface_top)->set_multi_unit_multiplicity(n_resources);
      const auto address_bitsize = HLSMgr->get_address_bitsize();
      const auto nbit = 64u - static_cast<unsigned long long>(__builtin_clzll(arraySize - 1U));
      const auto nbitDataSize = 64u - static_cast<unsigned long long>(__builtin_clzll(info.bitwidth));
      const structural_type_descriptorRef addrType(new structural_type_descriptor("bool", address_bitsize));
      const structural_type_descriptorRef address_interface_datatype(new structural_type_descriptor("bool", nbit));
      const structural_type_descriptorRef dataType(new structural_type_descriptor("bool", info.bitwidth));
      const structural_type_descriptorRef size1(new structural_type_descriptor("bool", 1));
      const structural_type_descriptorRef rwsize(new structural_type_descriptor("bool", nbitDataSize));
      const structural_type_descriptorRef rwtype(new structural_type_descriptor("bool", info.bitwidth));
      const structural_type_descriptorRef bool_type(new structural_type_descriptor("bool", 0));

      CM->add_port(CLOCK_PORT_NAME, port_o::IN, interface_top, bool_type);
      CM->add_port(RESET_PORT_NAME, port_o::IN, interface_top, bool_type);
      CM->add_port_vector(START_PORT_NAME, port_o::IN, n_resources, interface_top, bool_type);

      CM->add_port_vector("in1", port_o::IN, n_resources, interface_top,
                          size1); // when 0 is a read otherwise is a write
      CM->add_port_vector("in2", port_o::IN, n_resources, interface_top,
                          rwsize); // bit-width size of the written or read data
      const auto dataPort = CM->add_port_vector("in3", port_o::IN, n_resources, interface_top,
                                                rwtype); // value written when the first operand is 1, 0 otherwise
      const auto addrPort = CM->add_port_vector("in4", port_o::IN, n_resources, interface_top, addrType); // address
      GetPointerS<port_o>(addrPort)->set_is_addr_bus(true);
      GetPointerS<port_o>(addrPort)->set_port_alignment(info.alignment);

      CM->add_port_vector("out1", port_o::OUT, n_resources, interface_top, rwtype);

      const auto inPort_address =
          CM->add_port("_" + bundle_name + "_address0", port_o::OUT, interface_top, address_interface_datatype);
      GetPointerS<port_o>(inPort_address)->set_port_interface(port_o::port_interface::PI_ADDRESS);
      if(isDP)
      {
         const auto inPort_address1 =
             CM->add_port("_" + bundle_name + "_address1", port_o::OUT, interface_top, address_interface_datatype);
         GetPointerS<port_o>(inPort_address1)->set_port_interface(port_o::port_interface::PI_ADDRESS);
      }

      const auto inPort_ce = CM->add_port("_" + bundle_name + "_ce0", port_o::OUT, interface_top, bool_type);
      GetPointerS<port_o>(inPort_ce)->set_port_interface(port_o::port_interface::PI_CHIPENABLE);
      if(isDP)
      {
         const auto inPort_ce1 = CM->add_port("_" + bundle_name + "_ce1", port_o::OUT, interface_top, bool_type);
         GetPointerS<port_o>(inPort_ce1)->set_port_interface(port_o::port_interface::PI_CHIPENABLE);
      }

      if(!operationsW.empty())
      {
         const auto inPort_we = CM->add_port("_" + bundle_name + "_we0", port_o::OUT, interface_top, bool_type);
         GetPointerS<port_o>(inPort_we)->set_port_interface(port_o::port_interface::PI_WRITEENABLE);
         if(isDP)
         {
            const auto inPort_we1 = CM->add_port("_" + bundle_name + "_we1", port_o::OUT, interface_top, bool_type);
            GetPointerS<port_o>(inPort_we1)->set_port_interface(port_o::port_interface::PI_WRITEENABLE);
         }
      }
      if(!operationsR.empty())
      {
         const auto inPort_din = CM->add_port("_" + bundle_name + "_q0", port_o::IN, interface_top, dataType);
         GetPointerS<port_o>(inPort_din)->set_port_interface(port_o::port_interface::PI_DIN);
         if(isDP)
         {
            const auto inPort_din1 = CM->add_port("_" + bundle_name + "_q1", port_o::IN, interface_top, dataType);
            GetPointerS<port_o>(inPort_din1)->set_port_interface(port_o::port_interface::PI_DIN);
         }
      }
      if(!operationsW.empty())
      {
         const auto inPort_dout = CM->add_port("_" + bundle_name + "_d0", port_o::OUT, interface_top, dataType);
         GetPointerS<port_o>(inPort_dout)->set_port_interface(port_o::port_interface::PI_DOUT);
         if(isDP)
         {
            const auto inPort_dout1 = CM->add_port("_" + bundle_name + "_d1", port_o::OUT, interface_top, dataType);
            GetPointerS<port_o>(inPort_dout1)->set_port_interface(port_o::port_interface::PI_DOUT);
         }
      }

      CM->add_NP_functionality(interface_top, NP_functionality::LIBRARY, "in1 in2 in3 in4 out1");
      CM->add_NP_functionality(interface_top, NP_functionality::VERILOG_GENERATOR,
                               read_write_string + info.name + "ModuleGenerator");
      TechMan->add_resource(INTERFACE_LIBRARY, ResourceName, CM);
      const auto fu = GetPointerS<functional_unit>(TechMan->get_fu(ResourceName, INTERFACE_LIBRARY));
      fu->area_m = area_info::factory(parameters);
      fu->area_m->set_area_value(0);
      HLSMgr->global_resource_constraints[std::make_pair(ResourceName, INTERFACE_LIBRARY)] =
          std::make_pair(n_resources, n_resources);
      INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "<--Interface resource created");
   }
   for(const auto& fdName : operationsR)
   {
      TechMan->add_operation(INTERFACE_LIBRARY, ResourceName, fdName);
   }
   for(const auto& fdName : operationsW)
   {
      TechMan->add_operation(INTERFACE_LIBRARY, ResourceName, fdName);
   }
   const auto fu = GetPointerS<functional_unit>(TechMan->get_fu(ResourceName, INTERFACE_LIBRARY));
   const auto bram_f_unit = TechMan->get_fu(isDP ? ARRAY_1D_STD_BRAM_NN_SDS : ARRAY_1D_STD_BRAM_SDS, LIBRARY_STD_FU);
   const auto bram_fu = GetPointerS<functional_unit>(bram_f_unit);
   const auto load_op_node = bram_fu->get_operation("LOAD");
   const auto load_op = GetPointerS<operation>(load_op_node);
   const auto load_delay = load_op->time_m->get_execution_time();
   const auto load_cycles = load_op->time_m->get_cycles();
   const auto load_ii = load_op->time_m->get_initiation_time();
   const auto load_sp = load_op->time_m->get_stage_period();
   for(const auto& fdName : operationsR)
   {
      const auto op = GetPointerS<operation>(fu->get_operation(fdName));
      op->time_m = time_info::factory(parameters);
      op->bounded = true;
      op->time_m->set_execution_time(load_delay, load_cycles);
      op->time_m->set_initiation_time(load_ii);
      op->time_m->set_stage_period(load_sp);
      op->time_m->set_synthesis_dependent(true);
   }
   const auto store_op_node = bram_fu->get_operation("STORE");
   const auto store_op = GetPointerS<operation>(store_op_node);
   const auto store_delay = store_op->time_m->get_execution_time();
   const auto store_cycles = store_op->time_m->get_cycles();
   const auto store_ii = store_op->time_m->get_initiation_time();
   const auto store_sp = store_op->time_m->get_stage_period();
   for(const auto& fdName : operationsW)
   {
      const auto op = GetPointerS<operation>(fu->get_operation(fdName));
      op->time_m = time_info::factory(parameters);
      op->bounded = true;
      op->time_m->set_execution_time(store_delay, store_cycles);
      op->time_m->set_initiation_time(store_ii);
      op->time_m->set_stage_period(store_sp);
      op->time_m->set_synthesis_dependent(true);
   }
}

void InterfaceInfer::create_resource_m_axi(const std::set<std::string>& operationsR,
                                           const std::set<std::string>& operationsW, const std::string& arg_name,
                                           const std::string& bundle_name, const interface_info& info, m_axi_type mat,
                                           const std::map<interface_attributes, std::string>& bundle_attr_map) const
{
   const auto ResourceName = ENCODE_FDNAME(bundle_name, "", "");
   THROW_ASSERT(GetPointer<HLS_manager>(AppM), "");
   const auto HLSMgr = GetPointerS<HLS_manager>(AppM);
   const auto HLS_D = HLSMgr->get_HLS_device();
   const auto TechMan = HLS_D->get_technology_manager();
   unsigned long long way_lines = 0;

   if(!TechMan->is_library_manager(INTERFACE_LIBRARY) ||
      !TechMan->get_library_manager(INTERFACE_LIBRARY)->is_fu(ResourceName))
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level,
                     "-->Creating interface resource: " + INTERFACE_LIBRARY + ":" + ResourceName);
      const structural_managerRef CM(new structural_manager(parameters));
      const structural_type_descriptorRef module_type(new structural_type_descriptor(ResourceName));
      CM->set_top_info(ResourceName, module_type);
      const auto interface_top = CM->get_circ();
      /// add description and license
      GetPointerS<module>(interface_top)->set_description("Interface module for function: " + ResourceName);
      GetPointerS<module>(interface_top)->set_copyright(GENERATED_COPYRIGHT);
      GetPointerS<module>(interface_top)->set_authors("Component automatically generated by bambu");
      GetPointerS<module>(interface_top)->set_license(GENERATED_LICENSE);
      GetPointerS<module>(interface_top)->set_multi_unit_multiplicity(1U);

      const auto address_bitsize = HLSMgr->get_address_bitsize();
      const auto nbitDataSize = 64u - static_cast<unsigned>(__builtin_clzll(info.bitwidth));

      long long unsigned backEndBitsize = info.bitwidth;
      if(bundle_attr_map.find(attr_bus_size) != bundle_attr_map.end() && bundle_attr_map.at(attr_bus_size) != "")
      {
         backEndBitsize = std::stoull(bundle_attr_map.at(attr_bus_size));
      }

      const structural_type_descriptorRef address_interface_datatype(
          new structural_type_descriptor("bool", address_bitsize));
      const structural_type_descriptorRef size1(new structural_type_descriptor("bool", 1));
      const structural_type_descriptorRef rwsize(new structural_type_descriptor("bool", nbitDataSize));
      const structural_type_descriptorRef rwtypeIn(new structural_type_descriptor("bool", info.bitwidth));
      const structural_type_descriptorRef rwtypeOut(new structural_type_descriptor("bool", backEndBitsize));
      const structural_type_descriptorRef idType(new structural_type_descriptor("bool", 1));
      const structural_type_descriptorRef lenType(new structural_type_descriptor("bool", 8));
      const structural_type_descriptorRef sizeType(new structural_type_descriptor("bool", 3));
      const structural_type_descriptorRef burstType(new structural_type_descriptor("bool", 2));
      const structural_type_descriptorRef lockType(new structural_type_descriptor("bool", 1));
      const structural_type_descriptorRef cacheType(new structural_type_descriptor("bool", 4));
      const structural_type_descriptorRef protType(new structural_type_descriptor("bool", 3));
      const structural_type_descriptorRef qosType(new structural_type_descriptor("bool", 4));
      const structural_type_descriptorRef regionType(new structural_type_descriptor("bool", 4));
      const structural_type_descriptorRef userType(new structural_type_descriptor("bool", 1));
      const structural_type_descriptorRef strbType(new structural_type_descriptor("bool", backEndBitsize / 8ULL));
      const structural_type_descriptorRef respType(new structural_type_descriptor("bool", 2));
      const structural_type_descriptorRef bool_type(new structural_type_descriptor("bool", 0));

      CM->add_port(CLOCK_PORT_NAME, port_o::IN, interface_top, bool_type);
      CM->add_port(RESET_PORT_NAME, port_o::IN, interface_top, bool_type);
      CM->add_port_vector(START_PORT_NAME, port_o::IN, 1U, interface_top, bool_type);

      // when 0 is a read otherwise is a write
      CM->add_port("in1", port_o::IN, interface_top, size1);
      // bit-width size of the written or read data
      CM->add_port("in2", port_o::IN, interface_top, rwsize);
      // value written when the first operand is 1, 0 otherwise
      CM->add_port("in3", port_o::IN, interface_top, rwtypeIn);

      const auto addrPort = CM->add_port("in4", port_o::IN, interface_top, address_interface_datatype);
      GetPointerS<port_o>(addrPort)->set_is_addr_bus(true);

      const auto awready = CM->add_port("_m_axi_" + bundle_name + "_awready", port_o::IN, interface_top, bool_type);
      GetPointerS<port_o>(awready)->set_port_interface(port_o::port_interface::M_AXI_AWREADY);

      const auto wready = CM->add_port("_m_axi_" + bundle_name + "_wready", port_o::IN, interface_top, bool_type);
      GetPointerS<port_o>(wready)->set_port_interface(port_o::port_interface::M_AXI_WREADY);

      const auto bid = CM->add_port_vector("_m_axi_" + bundle_name + "_bid", port_o::IN, 1, interface_top, idType);
      GetPointerS<port_o>(bid)->set_port_interface(port_o::port_interface::M_AXI_BID);

      const auto bresp = CM->add_port("_m_axi_" + bundle_name + "_bresp", port_o::IN, interface_top, respType);
      GetPointerS<port_o>(bresp)->set_port_interface(port_o::port_interface::M_AXI_BRESP);

      const auto buser =
          CM->add_port_vector("_m_axi_" + bundle_name + "_buser", port_o::IN, 1, interface_top, userType);
      GetPointerS<port_o>(buser)->set_port_interface(port_o::port_interface::M_AXI_BUSER);

      const auto bvalid = CM->add_port("_m_axi_" + bundle_name + "_bvalid", port_o::IN, interface_top, bool_type);
      GetPointerS<port_o>(bvalid)->set_port_interface(port_o::port_interface::M_AXI_BVALID);

      const auto arready = CM->add_port("_m_axi_" + bundle_name + "_arready", port_o::IN, interface_top, bool_type);
      GetPointerS<port_o>(arready)->set_port_interface(port_o::port_interface::M_AXI_ARREADY);

      const auto rid = CM->add_port_vector("_m_axi_" + bundle_name + "_rid", port_o::IN, 1, interface_top, idType);
      GetPointerS<port_o>(rid)->set_port_interface(port_o::port_interface::M_AXI_RID);

      const auto rdata = CM->add_port("_m_axi_" + bundle_name + "_rdata", port_o::IN, interface_top, rwtypeOut);
      GetPointerS<port_o>(rdata)->set_port_interface(port_o::port_interface::M_AXI_RDATA);

      const auto rresp = CM->add_port("_m_axi_" + bundle_name + "_rresp", port_o::IN, interface_top, respType);
      GetPointerS<port_o>(rresp)->set_port_interface(port_o::port_interface::M_AXI_RRESP);

      const auto rlast = CM->add_port("_m_axi_" + bundle_name + "_rlast", port_o::IN, interface_top, bool_type);
      GetPointerS<port_o>(rlast)->set_port_interface(port_o::port_interface::M_AXI_RLAST);

      const auto ruser =
          CM->add_port_vector("_m_axi_" + bundle_name + "_ruser", port_o::IN, 1, interface_top, userType);
      GetPointerS<port_o>(ruser)->set_port_interface(port_o::port_interface::M_AXI_RUSER);

      const auto rvalid = CM->add_port("_m_axi_" + bundle_name + "_rvalid", port_o::IN, interface_top, bool_type);
      GetPointerS<port_o>(rvalid)->set_port_interface(port_o::port_interface::M_AXI_RVALID);

      CM->add_port(DONE_PORT_NAME, port_o::OUT, interface_top, bool_type);
      CM->add_port("out1", port_o::OUT, interface_top, rwtypeIn);

      const auto awid = CM->add_port_vector("_m_axi_" + bundle_name + "_awid", port_o::OUT, 1, interface_top, idType);
      GetPointerS<port_o>(awid)->set_port_interface(port_o::port_interface::M_AXI_AWID);

      const auto awaddr =
          CM->add_port("_m_axi_" + bundle_name + "_awaddr", port_o::OUT, interface_top, address_interface_datatype);
      GetPointerS<port_o>(awaddr)->set_port_interface(port_o::port_interface::M_AXI_AWADDR);

      const auto awlen = CM->add_port("_m_axi_" + bundle_name + "_awlen", port_o::OUT, interface_top, lenType);
      GetPointerS<port_o>(awlen)->set_port_interface(port_o::port_interface::M_AXI_AWLEN);

      const auto awsize = CM->add_port("_m_axi_" + bundle_name + "_awsize", port_o::OUT, interface_top, sizeType);
      GetPointerS<port_o>(awsize)->set_port_interface(port_o::port_interface::M_AXI_AWSIZE);

      const auto awburst = CM->add_port("_m_axi_" + bundle_name + "_awburst", port_o::OUT, interface_top, burstType);
      GetPointerS<port_o>(awburst)->set_port_interface(port_o::port_interface::M_AXI_AWBURST);

      const auto awlock =
          CM->add_port_vector("_m_axi_" + bundle_name + "_awlock", port_o::OUT, 1, interface_top, lockType);
      GetPointerS<port_o>(awlock)->set_port_interface(port_o::port_interface::M_AXI_AWLOCK);

      const auto awcache = CM->add_port("_m_axi_" + bundle_name + "_awcache", port_o::OUT, interface_top, cacheType);
      GetPointerS<port_o>(awcache)->set_port_interface(port_o::port_interface::M_AXI_AWCACHE);

      const auto awprot = CM->add_port("_m_axi_" + bundle_name + "_awprot", port_o::OUT, interface_top, protType);
      GetPointerS<port_o>(awprot)->set_port_interface(port_o::port_interface::M_AXI_AWPROT);

      const auto awqos = CM->add_port("_m_axi_" + bundle_name + "_awqos", port_o::OUT, interface_top, qosType);
      GetPointerS<port_o>(awqos)->set_port_interface(port_o::port_interface::M_AXI_AWQOS);

      const auto awregion = CM->add_port("_m_axi_" + bundle_name + "_awregion", port_o::OUT, interface_top, regionType);
      GetPointerS<port_o>(awregion)->set_port_interface(port_o::port_interface::M_AXI_AWREGION);

      const auto awuser =
          CM->add_port_vector("_m_axi_" + bundle_name + "_awuser", port_o::OUT, 1, interface_top, userType);
      GetPointerS<port_o>(awuser)->set_port_interface(port_o::port_interface::M_AXI_AWUSER);

      const auto awvalid = CM->add_port("_m_axi_" + bundle_name + "_awvalid", port_o::OUT, interface_top, bool_type);
      GetPointerS<port_o>(awvalid)->set_port_interface(port_o::port_interface::M_AXI_AWVALID);

      // BEAWARE: this is AXI3 spec, AXI4 does not have WID
      // const auto wid = CM->add_port_vector("_m_axi_" + bundle_name + "_wid", port_o::OUT, 1, interface_top, idType);
      // GetPointerS<port_o>(wid)->set_port_interface(port_o::port_interface::M_AXI_WID);

      const auto wdata = CM->add_port("_m_axi_" + bundle_name + "_wdata", port_o::OUT, interface_top, rwtypeOut);
      GetPointerS<port_o>(wdata)->set_port_interface(port_o::port_interface::M_AXI_WDATA);

      const auto wstrb =
          CM->add_port_vector("_m_axi_" + bundle_name + "_wstrb", port_o::OUT, 1, interface_top, strbType);
      GetPointerS<port_o>(wstrb)->set_port_interface(port_o::port_interface::M_AXI_WSTRB);

      const auto wlast = CM->add_port("_m_axi_" + bundle_name + "_wlast", port_o::OUT, interface_top, bool_type);
      GetPointerS<port_o>(wlast)->set_port_interface(port_o::port_interface::M_AXI_WLAST);

      const auto wuser =
          CM->add_port_vector("_m_axi_" + bundle_name + "_wuser", port_o::OUT, 1, interface_top, userType);
      GetPointerS<port_o>(wuser)->set_port_interface(port_o::port_interface::M_AXI_WUSER);

      const auto wvalid = CM->add_port("_m_axi_" + bundle_name + "_wvalid", port_o::OUT, interface_top, bool_type);
      GetPointerS<port_o>(wvalid)->set_port_interface(port_o::port_interface::M_AXI_WVALID);

      const auto bready = CM->add_port("_m_axi_" + bundle_name + "_bready", port_o::OUT, interface_top, bool_type);
      GetPointerS<port_o>(bready)->set_port_interface(port_o::port_interface::M_AXI_BREADY);

      const auto arid = CM->add_port_vector("_m_axi_" + bundle_name + "_arid", port_o::OUT, 1, interface_top, idType);
      GetPointerS<port_o>(arid)->set_port_interface(port_o::port_interface::M_AXI_ARID);

      const auto araddr =
          CM->add_port("_m_axi_" + bundle_name + "_araddr", port_o::OUT, interface_top, address_interface_datatype);
      GetPointerS<port_o>(araddr)->set_port_interface(port_o::port_interface::M_AXI_ARADDR);

      const auto arlen = CM->add_port("_m_axi_" + bundle_name + "_arlen", port_o::OUT, interface_top, lenType);
      GetPointerS<port_o>(arlen)->set_port_interface(port_o::port_interface::M_AXI_ARLEN);

      const auto arsize = CM->add_port("_m_axi_" + bundle_name + "_arsize", port_o::OUT, interface_top, sizeType);
      GetPointerS<port_o>(arsize)->set_port_interface(port_o::port_interface::M_AXI_ARSIZE);

      const auto arburst = CM->add_port("_m_axi_" + bundle_name + "_arburst", port_o::OUT, interface_top, burstType);
      GetPointerS<port_o>(arburst)->set_port_interface(port_o::port_interface::M_AXI_ARBURST);

      const auto arlock =
          CM->add_port_vector("_m_axi_" + bundle_name + "_arlock", port_o::OUT, 1, interface_top, lockType);
      GetPointerS<port_o>(arlock)->set_port_interface(port_o::port_interface::M_AXI_ARLOCK);

      const auto arcache = CM->add_port("_m_axi_" + bundle_name + "_arcache", port_o::OUT, interface_top, cacheType);
      GetPointerS<port_o>(arcache)->set_port_interface(port_o::port_interface::M_AXI_ARCACHE);

      const auto arprot = CM->add_port("_m_axi_" + bundle_name + "_arprot", port_o::OUT, interface_top, protType);
      GetPointerS<port_o>(arprot)->set_port_interface(port_o::port_interface::M_AXI_ARPROT);

      const auto arqos = CM->add_port("_m_axi_" + bundle_name + "_arqos", port_o::OUT, interface_top, qosType);
      GetPointerS<port_o>(arqos)->set_port_interface(port_o::port_interface::M_AXI_ARQOS);

      const auto arregion = CM->add_port("_m_axi_" + bundle_name + "_arregion", port_o::OUT, interface_top, regionType);
      GetPointerS<port_o>(arregion)->set_port_interface(port_o::port_interface::M_AXI_ARREGION);

      const auto aruser =
          CM->add_port_vector("_m_axi_" + bundle_name + "_aruser", port_o::OUT, 1, interface_top, userType);
      GetPointerS<port_o>(aruser)->set_port_interface(port_o::port_interface::M_AXI_ARUSER);

      const auto arvalid = CM->add_port("_m_axi_" + bundle_name + "_arvalid", port_o::OUT, interface_top, bool_type);
      GetPointerS<port_o>(arvalid)->set_port_interface(port_o::port_interface::M_AXI_ARVALID);

      const auto rready = CM->add_port("_m_axi_" + bundle_name + "_rready", port_o::OUT, interface_top, bool_type);
      GetPointerS<port_o>(rready)->set_port_interface(port_o::port_interface::M_AXI_RREADY);

      if(mat == m_axi_type::axi_slave)
      {
         const auto s_awvalid = CM->add_port("_s_axi_AXILiteS_AWVALID", port_o::IN, interface_top, bool_type);
         GetPointerS<port_o>(s_awvalid)->set_port_interface(port_o::port_interface::S_AXIL_AWVALID);
         const auto s_awaddr =
             CM->add_port("_s_axi_AXILiteS_AWADDR", port_o::IN, interface_top, address_interface_datatype);
         GetPointerS<port_o>(s_awaddr)->set_port_interface(port_o::port_interface::S_AXIL_AWADDR);
         const auto s_wvalid = CM->add_port("_s_axi_AXILiteS_WVALID", port_o::IN, interface_top, bool_type);
         GetPointerS<port_o>(s_wvalid)->set_port_interface(port_o::port_interface::S_AXIL_WVALID);
         const auto s_wdata = CM->add_port("_s_axi_AXILiteS_WDATA", port_o::IN, interface_top, rwtypeOut);
         GetPointerS<port_o>(s_wdata)->set_port_interface(port_o::port_interface::S_AXIL_WDATA);
         const auto s_wstrb = CM->add_port("_s_axi_AXILiteS_WSTRB", port_o::IN, interface_top, strbType);
         GetPointerS<port_o>(s_wstrb)->set_port_interface(port_o::port_interface::S_AXIL_WSTRB);
         const auto s_arvalid = CM->add_port("_s_axi_AXILiteS_ARVALID", port_o::IN, interface_top, bool_type);
         GetPointerS<port_o>(s_arvalid)->set_port_interface(port_o::port_interface::S_AXIL_ARVALID);
         const auto s_araddr =
             CM->add_port("_s_axi_AXILiteS_ARADDR", port_o::IN, interface_top, address_interface_datatype);
         GetPointerS<port_o>(s_araddr)->set_port_interface(port_o::port_interface::S_AXIL_ARADDR);
         const auto s_rready = CM->add_port("_s_axi_AXILiteS_RREADY", port_o::IN, interface_top, bool_type);
         GetPointerS<port_o>(s_rready)->set_port_interface(port_o::port_interface::S_AXIL_RREADY);
         const auto s_bready = CM->add_port("_s_axi_AXILiteS_BREADY", port_o::IN, interface_top, bool_type);
         GetPointerS<port_o>(s_bready)->set_port_interface(port_o::port_interface::S_AXIL_BREADY);

         const auto s_awready = CM->add_port("_s_axi_AXILiteS_AWREADY", port_o::OUT, interface_top, bool_type);
         GetPointerS<port_o>(s_awready)->set_port_interface(port_o::port_interface::S_AXIL_AWREADY);
         const auto s_wready = CM->add_port("_s_axi_AXILiteS_WREADY", port_o::OUT, interface_top, bool_type);
         GetPointerS<port_o>(s_wready)->set_port_interface(port_o::port_interface::S_AXIL_WREADY);
         const auto s_arready = CM->add_port("_s_axi_AXILiteS_ARREADY", port_o::OUT, interface_top, bool_type);
         GetPointerS<port_o>(s_arready)->set_port_interface(port_o::port_interface::S_AXIL_ARREADY);
         const auto s_rvalid = CM->add_port("_s_axi_AXILiteS_RVALID", port_o::OUT, interface_top, bool_type);
         GetPointerS<port_o>(s_rvalid)->set_port_interface(port_o::port_interface::S_AXIL_RVALID);
         const auto s_rdata = CM->add_port("_s_axi_AXILiteS_RDATA", port_o::OUT, interface_top, rwtypeIn);
         GetPointerS<port_o>(s_rdata)->set_port_interface(port_o::port_interface::S_AXIL_RDATA);
         const auto s_rresp = CM->add_port("_s_axi_AXILiteS_RRESP", port_o::OUT, interface_top, respType);
         GetPointerS<port_o>(s_rresp)->set_port_interface(port_o::port_interface::S_AXIL_RRESP);
         const auto s_bvalid = CM->add_port("_s_axi_AXILiteS_BVALID", port_o::OUT, interface_top, bool_type);
         GetPointerS<port_o>(s_bvalid)->set_port_interface(port_o::port_interface::S_AXIL_BVALID);
         const auto s_bresp = CM->add_port("_s_axi_AXILiteS_BRESP", port_o::OUT, interface_top, respType);
         GetPointerS<port_o>(s_bresp)->set_port_interface(port_o::port_interface::S_AXIL_BRESP);
      }

      CM->add_NP_functionality(interface_top, NP_functionality::LIBRARY, "in1 in2 in3 in4 out1");
      CM->add_NP_functionality(interface_top, NP_functionality::VERILOG_GENERATOR,
                               "ReadWrite_" + info.name + "ModuleGenerator");
      if(bundle_attr_map.find(attr_way_lines) != bundle_attr_map.end())
      {
         way_lines = std::stoull(bundle_attr_map.at(attr_way_lines));
      }
      /* Add the dependency to the IOB_cache module if there is a cache */
      if(way_lines > 0)
      {
         CM->add_NP_functionality(interface_top, NP_functionality::IP_COMPONENT, "IOB_cache_axi");
         auto mod = GetPointerS<module>(CM->get_circ());

         mod->AddParameter("WAY_LINES", STR(way_lines));
         if(bundle_attr_map.find(attr_line_size) != bundle_attr_map.end() && bundle_attr_map.at(attr_line_size) != "")
         {
            mod->AddParameter("LINE_SIZE", bundle_attr_map.at(attr_line_size));
         }
         if(bundle_attr_map.find(attr_bus_size) != bundle_attr_map.end() && bundle_attr_map.at(attr_bus_size) != "")
         {
            mod->AddParameter("BUS_SIZE", bundle_attr_map.at(attr_bus_size));
         }
         if(bundle_attr_map.find(attr_n_ways) != bundle_attr_map.end() && bundle_attr_map.at(attr_n_ways) != "")
         {
            mod->AddParameter("N_WAYS", bundle_attr_map.at(attr_n_ways));
         }
         if(bundle_attr_map.find(attr_buf_size) != bundle_attr_map.end() && bundle_attr_map.at(attr_buf_size) != "")
         {
            mod->AddParameter("BUF_SIZE", bundle_attr_map.at(attr_buf_size));
         }
         if(bundle_attr_map.find(attr_rep_pol) != bundle_attr_map.end() && bundle_attr_map.at(attr_rep_pol) != "")
         {
            mod->AddParameter("REP_POL", bundle_attr_map.at(attr_rep_pol) == "lru"  ? "0" :
                                         bundle_attr_map.at(attr_rep_pol) == "mru"  ? "1" :
                                         bundle_attr_map.at(attr_rep_pol) == "tree" ? "2" :
                                                                                      bundle_attr_map.at(attr_rep_pol));
         }

         if(bundle_attr_map.find(attr_wr_pol) != bundle_attr_map.end() && bundle_attr_map.at(attr_wr_pol) != "")
         {
            mod->AddParameter("WR_POL", bundle_attr_map.at(attr_wr_pol) == "wt" ? "0" :
                                        bundle_attr_map.at(attr_wr_pol) == "wb" ? "1" :
                                                                                  bundle_attr_map.at(attr_wr_pol));
         }
      }
      TechMan->add_resource(INTERFACE_LIBRARY, ResourceName, CM);

      const auto fu = GetPointerS<functional_unit>(TechMan->get_fu(ResourceName, INTERFACE_LIBRARY));
      fu->area_m = area_info::factory(parameters);
      fu->area_m->set_area_value(0);
      HLSMgr->global_resource_constraints[std::make_pair(ResourceName, INTERFACE_LIBRARY)] = std::make_pair(1U, 1U);
      INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "<--Interface resource created");
   }

   for(const auto& fdName : operationsR)
   {
      TechMan->add_operation(INTERFACE_LIBRARY, ResourceName, fdName);
   }
   for(const auto& fdName : operationsW)
   {
      TechMan->add_operation(INTERFACE_LIBRARY, ResourceName, fdName);
   }

   /* Flush Op */
   const auto flushName = ENCODE_FDNAME(bundle_name, "_Flush_", "m_axi");
   if(way_lines > 0)
   {
      TechMan->add_operation(INTERFACE_LIBRARY, ResourceName, flushName);
   }
   const auto fu = GetPointerS<functional_unit>(TechMan->get_fu(ResourceName, INTERFACE_LIBRARY));

   for(const auto& fdName : operationsR)
   {
      const auto op = GetPointerS<operation>(fu->get_operation(fdName));
      op->time_m = time_info::factory(parameters);
      op->bounded = false;
      op->time_m->set_execution_time(HLS_D->get_technology_manager()->CGetSetupHoldTime() + EPSILON, 0);
      op->time_m->set_synthesis_dependent(true);
   }
   for(const auto& fdName : operationsW)
   {
      const auto op = GetPointer<operation>(fu->get_operation(fdName));
      op->time_m = time_info::factory(parameters);
      op->bounded = false;
      op->time_m->set_execution_time(HLS_D->get_technology_manager()->CGetSetupHoldTime() + EPSILON, 0);
      op->time_m->set_synthesis_dependent(true);
   }

   if(way_lines > 0)
   {
      const auto op = GetPointer<operation>(fu->get_operation(flushName));
      op->time_m = time_info::factory(parameters);
      op->bounded = false;
      op->time_m->set_execution_time(HLS_D->get_technology_manager()->CGetSetupHoldTime() + EPSILON, 0);
      op->time_m->set_synthesis_dependent(true);
   }
   const auto address_bitsize = HLSMgr->get_address_bitsize();
   const structural_type_descriptorRef address_interface_datatype(
       new structural_type_descriptor("bool", address_bitsize));
   const auto interface_top = fu->CM->get_circ();
   const auto inPort_m_axi = fu->CM->add_port("_" + arg_name, port_o::IN, interface_top, address_interface_datatype);
   if(mat == m_axi_type::none || mat == m_axi_type::axi_slave)
   {
      GetPointerS<port_o>(inPort_m_axi)->set_port_interface(port_o::port_interface::PI_M_AXI_OFF);
   }
   else
   {
      GetPointerS<port_o>(inPort_m_axi)->set_port_interface(port_o::port_interface::PI_M_AXI_DIRECT);
   }
}

void InterfaceInfer::create_resource(const std::set<std::string>& operationsR, const std::set<std::string>& operationsW,
                                     const std::string& arg_name, const interface_info& info,
                                     const std::string& fname) const
{
   if(info.name == "none" || info.name == "none_registered" || info.name == "acknowledge" || info.name == "valid" ||
      info.name == "ovalid" || info.name == "handshake" || info.name == "fifo" || info.name == "axis")
   {
      THROW_ASSERT(!operationsR.empty() || !operationsW.empty(), "unexpected condition");
      const auto IO_P = !operationsR.empty() && !operationsW.empty();
      create_resource_Read_simple(operationsR, arg_name, info, IO_P);
      create_resource_Write_simple(operationsW, arg_name, info, IO_P);
   }
   else if(info.name == "array")
   {
      const auto HLSMgr = GetPointer<HLS_manager>(AppM);
      THROW_ASSERT(HLSMgr->design_attributes.find(fname) != HLSMgr->design_attributes.end() &&
                       HLSMgr->design_attributes.find(fname)->second.find(arg_name) !=
                           HLSMgr->design_attributes.find(fname)->second.end() &&
                       HLSMgr->design_attributes.find(fname)->second.find(arg_name)->second.find(attr_size) !=
                           HLSMgr->design_attributes.find(fname)->second.find(arg_name)->second.end(),
                   "unexpected condition");
      const auto arraySizeSTR = HLSMgr->design_attributes.at(fname).at(arg_name).at(attr_size);
      const auto arraySize = std::stoull(arraySizeSTR);
      if(arraySize == 0)
      {
         THROW_ERROR("array size equal to zero");
      }

      auto bundle_name = arg_name;
      if(HLSMgr->design_attributes.find(fname) != HLSMgr->design_attributes.end() &&
         HLSMgr->design_attributes.at(fname).find(arg_name) != HLSMgr->design_attributes.at(fname).end() &&
         HLSMgr->design_attributes.find(fname)->second.find(arg_name)->second.find(attr_bundle_name) !=
             HLSMgr->design_attributes.find(fname)->second.find(arg_name)->second.end())
      {
         bundle_name = HLSMgr->design_attributes.at(fname).at(arg_name).at(attr_bundle_name);
      }

      create_resource_array(operationsR, operationsW, bundle_name, info, arraySize);
   }
   else if(info.name == "m_axi")
   {
      auto mat = m_axi_type::none;
      const auto HLSMgr = GetPointerS<HLS_manager>(AppM);
      auto bundle_name = arg_name;

      std::map<interface_attributes, std::string> bundle_attr_map;

      if(HLSMgr->design_attributes.find(fname) != HLSMgr->design_attributes.end() &&
         HLSMgr->design_attributes.at(fname).find(arg_name) != HLSMgr->design_attributes.at(fname).end() &&
         HLSMgr->design_attributes.at(fname).at(arg_name).find(attr_offset) !=
             HLSMgr->design_attributes.at(fname).at(arg_name).end())
      {
         const auto& matString = HLSMgr->design_attributes.at(fname).at(arg_name).at(attr_offset);
         if(matString == "none")
         {
            mat = m_axi_type::none;
         }
         else if(matString == "direct")
         {
            mat = m_axi_type::direct;
            bundle_name = "gmem";
         }
         else if(matString == "axi_slave")
         {
            mat = m_axi_type::axi_slave;
            bundle_name = "gmem";
         }
         else
         {
            THROW_ERROR("non-supported m_axi attribute or malformed pragma");
         }
      }
      if(HLSMgr->design_attributes.find(fname) != HLSMgr->design_attributes.end() &&
         HLSMgr->design_attributes.at(fname).find(arg_name) != HLSMgr->design_attributes.at(fname).end())
      {
         if(HLSMgr->design_attributes.at(fname).at(arg_name).find(attr_bundle_name) !=
            HLSMgr->design_attributes.at(fname).at(arg_name).end())
         {
            bundle_name = HLSMgr->design_attributes.at(fname).at(arg_name).at(attr_bundle_name);
         }
         else
         {
            HLSMgr->design_attributes.at(fname).at(arg_name)[attr_bundle_name] = bundle_name;
         }
      }
      if(HLSMgr->design_attributes.find(fname) != HLSMgr->design_attributes.end())
      {
         for(auto& par : HLSMgr->design_attributes.at(fname))
         {
            if(par.second.find(attr_bundle_name) != par.second.end() && par.second.at(attr_bundle_name) == bundle_name)
            {
               /* Fill bundle attributes map. Always check that parameters of the same bundle are the same */

               if(bundle_attr_map.find(attr_way_lines) != bundle_attr_map.end())
               {
                  THROW_ASSERT(par.second.find(attr_way_lines) != par.second.end() &&
                                   par.second.at(attr_way_lines) == bundle_attr_map.at(attr_way_lines),
                               "Different cache lines for the same bundle");
               }
               else if(par.second.find(attr_way_lines) != par.second.end())
               {
                  bundle_attr_map[attr_way_lines] = par.second.at(attr_way_lines);
               }
               if(bundle_attr_map.find(attr_line_size) != bundle_attr_map.end())
               {
                  THROW_ASSERT(par.second.find(attr_line_size) != par.second.end() &&
                                   par.second.at(attr_line_size) == bundle_attr_map.at(attr_line_size),
                               "Different line sizes for the same bundle");
               }
               else if(par.second.find(attr_line_size) != par.second.end())
               {
                  bundle_attr_map[attr_line_size] = par.second.at(attr_line_size);
               }
               if(bundle_attr_map.find(attr_bus_size) != bundle_attr_map.end())
               {
                  THROW_ASSERT(par.second.find(attr_bus_size) != par.second.end() &&
                                   par.second.at(attr_bus_size) == bundle_attr_map.at(attr_bus_size),
                               "Different bus size for the same bundle");
               }
               else if(par.second.find(attr_bus_size) != par.second.end())
               {
                  bundle_attr_map[attr_bus_size] = par.second.at(attr_bus_size);
               }
               if(bundle_attr_map.find(attr_n_ways) != bundle_attr_map.end())
               {
                  THROW_ASSERT(par.second.find(attr_n_ways) != par.second.end() &&
                                   par.second.at(attr_n_ways) == bundle_attr_map.at(attr_n_ways),
                               "Different number of ways for the same bundle");
               }
               else if(par.second.find(attr_n_ways) != par.second.end())
               {
                  bundle_attr_map[attr_n_ways] = par.second.at(attr_n_ways);
               }
               if(bundle_attr_map.find(attr_buf_size) != bundle_attr_map.end())
               {
                  THROW_ASSERT(par.second.find(attr_buf_size) != par.second.end() &&
                                   par.second.at(attr_buf_size) == bundle_attr_map.at(attr_buf_size),
                               "Different buffer size for the same bundle");
               }
               else if(par.second.find(attr_buf_size) != par.second.end())
               {
                  bundle_attr_map[attr_buf_size] = par.second.at(attr_buf_size);
               }
               if(bundle_attr_map.find(attr_rep_pol) != bundle_attr_map.end())
               {
                  THROW_ASSERT(par.second.find(attr_rep_pol) != par.second.end() &&
                                   par.second.at(attr_rep_pol) == bundle_attr_map.at(attr_rep_pol),
                               "Different replacement policies for the same bundle");
               }
               else if(par.second.find(attr_rep_pol) != par.second.end())
               {
                  bundle_attr_map[attr_rep_pol] = par.second.at(attr_rep_pol);
               }
               if(bundle_attr_map.find(attr_wr_pol) != bundle_attr_map.end())
               {
                  THROW_ASSERT(par.second.find(attr_wr_pol) != par.second.end() &&
                                   par.second.at(attr_wr_pol) == bundle_attr_map.at(attr_wr_pol),
                               "Different write policies for the same bundle");
               }
               else if(par.second.find(attr_wr_pol) != par.second.end())
               {
                  bundle_attr_map[attr_wr_pol] = par.second.at(attr_wr_pol);
               }
            }
         }
      }
      create_resource_m_axi(operationsR, operationsW, arg_name, bundle_name, info, mat, bundle_attr_map);
   }
   else
   {
      THROW_ERROR("interface not supported: " + info.name);
   }
}
