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
 *              Copyright (C) 2022-2022 Politecnico di Milano
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
 *
 */
#include "InterfaceInfer.hpp"

#include "Parameter.hpp"

/// design_flows includes
#include "design_flow_graph.hpp"
#include "design_flow_manager.hpp"

/// design_flows/technology includes
#include "technology_flow_step.hpp"
#include "technology_flow_step_factory.hpp"

#include "application_manager.hpp"
#include "behavioral_helper.hpp"
#include "call_graph.hpp"
#include "call_graph_manager.hpp"
#include "function_behavior.hpp"
#include "hls_manager.hpp"
#include "hls_step.hpp"
#include "hls_target.hpp"

/// parser/compiler include
#include "token_interface.hpp"

/// tree includes
#include "dbgPrintHelper.hpp"      // for DEBUG_LEVEL_
#include "string_manipulation.hpp" // for GET_CLASS
#include "tree_basic_block.hpp"
#include "tree_helper.hpp"
#include "tree_manager.hpp"
#include "tree_manipulation.hpp"
#include "tree_node.hpp"
#include "tree_reindex.hpp"

/// XML includes used for writing and reading the configuration file
#include "polixml.hpp"
#include "xml_dom_parser.hpp"
#include "xml_helper.hpp"

#include "area_model.hpp"
#include "library_manager.hpp"
#include "technology_manager.hpp"
#include "technology_node.hpp"
#include "time_model.hpp"

#include "structural_manager.hpp"
#include "structural_objects.hpp"

#include "constant_strings.hpp"
#include "copyrights_strings.hpp"

#include "language_writer.hpp"

#include "hls_step.hpp" // for HLSFlowStep_Type

#include "config_PANDA_DATA_INSTALLDIR.hpp"
#include <boost/lexical_cast/try_lexical_convert.hpp>
#include <boost/regex.hpp>

#define EPSILON 0.000000001
#define ENCODE_FDNAME(arg_name, MODE, interfaceType) \
   ((arg_name) + STR_CST_interface_parameter_keyword + (MODE) + (interfaceType))

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

static const boost::regex signature_param_typename("((?:\\w+\\s*)+(?:<[^>]*>)?\\s*[\\*&]?\\s*)");

bool InterfaceInfer::HasToBeExecuted() const
{
   return !already_executed;
}

void InterfaceInfer::Initialize()
{
   const auto HLSMgr = GetPointer<HLS_manager>(AppM);
   THROW_ASSERT(HLSMgr, "");
   const auto parseInterfaceXML = [&](const std::string& XMLfilename)
   {
      if(boost::filesystem::exists(boost::filesystem::path(XMLfilename)))
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
                        std::string interfaceType;
                        std::string interfaceSize;
                        std::string interfaceAttribute2;
                        bool interfaceAttribute2_p = false;
                        std::string interfaceAttribute3;
                        bool interfaceAttribute3_p = false;
                        std::string interfaceTypename;
                        std::string interfaceTypenameOrig;
                        std::string interfaceTypenameInclude;
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
                              interfaceType = value;
                           }
                           if(key == "size")
                           {
                              interfaceSize = value;
                           }
                           if(key == "attribute2")
                           {
                              interfaceAttribute2 = value;
                              interfaceAttribute2_p = true;
                           }
                           if(key == "attribute3")
                           {
                              interfaceAttribute3 = value;
                              interfaceAttribute3_p = true;
                           }
                           if(key == "interface_typename")
                           {
                              interfaceTypename = value;
                              xml_node::convert_escaped(interfaceTypename);
                           }
                           if(key == "interface_typename_orig")
                           {
                              interfaceTypenameOrig = value;
                              xml_node::convert_escaped(interfaceTypenameOrig);
                           }
                           if(key == "interface_typename_include")
                           {
                              interfaceTypenameInclude = value;
                           }
                        }
                        if(argName == "")
                        {
                           THROW_ERROR("malformed interface file");
                        }
                        if(parameters->getOption<HLSFlowStep_Type>(OPT_interface_type) ==
                           HLSFlowStep_Type::INFERRED_INTERFACE_GENERATION)
                        {
                           if(interfaceType == "")
                           {
                              THROW_ERROR("malformed interface file");
                           }
                           INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level,
                                          "---|" + argName + "|" + interfaceType + "|\n");
                           HLSMgr->design_interface[fname][argName] = interfaceType;
                           if(interfaceType == "array")
                           {
                              HLSMgr->design_interface_arraysize[fname][argName] = interfaceSize;
                           }
                           if(interfaceType == "m_axi" && interfaceAttribute2_p)
                           {
                              HLSMgr->design_interface_attribute2[fname][argName] = interfaceAttribute2;
                           }
                           if((interfaceType == "m_axi" || interfaceType == "array") && interfaceAttribute3_p)
                           {
                              HLSMgr->design_interface_attribute3[fname][argName] = interfaceAttribute3;
                           }
                        }

                        HLSMgr->design_interface_typename[fname][argName] = interfaceTypename;
                        HLSMgr->design_interface_typename_signature[fname].push_back(interfaceTypename);
                        HLSMgr->design_interface_typename_orig_signature[fname].push_back(interfaceTypenameOrig);
                        if((interfaceTypenameOrig.find("ap_int<") != std::string::npos ||
                            interfaceTypenameOrig.find("ap_uint<") != std::string::npos) &&
                           interfaceTypenameInclude.find("ac_int.h") != std::string::npos)
                        {
                           boost::replace_all(interfaceTypenameInclude, "ac_int.h", "ap_int.h");
                        }
                        if((interfaceTypenameOrig.find("ap_fixed<") != std::string::npos ||
                            interfaceTypenameOrig.find("ap_ufixed<") != std::string::npos) &&
                           interfaceTypenameInclude.find("ac_fixed.h") != std::string::npos)
                        {
                           boost::replace_all(interfaceTypenameInclude, "ac_fixed.h", "ap_fixed.h");
                        }
                        HLSMgr->design_interface_typenameinclude[fname][argName] = interfaceTypenameInclude;
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
         const std::string leaf_name = source_file.second == "-" ? "stdin-" : GetLeafFileName(source_file.second);
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
   const auto add_to_modified = [&](const tree_nodeRef& tn)
   { modified.insert(GET_INDEX_CONST_NODE(GetPointer<gimple_node>(GET_CONST_NODE(tn))->scpe)); };
   for(const auto& top_id : top_functions)
   {
      const auto fnode = TM->CGetTreeNode(top_id);
      const auto fd = GetPointer<const function_decl>(fnode);
      std::string fname;
      tree_helper::get_mangled_fname(fd, fname);
      if(HLSMgr->design_interface_typename.find(fname) == HLSMgr->design_interface_typename.end())
      {
         const auto dfname = string_demangle(fname);
         if(!dfname.empty())
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Extracting interface from signature " + fname);
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Demangled as " + dfname);
            boost::sregex_token_iterator typename_it(dfname.begin(), dfname.end(), signature_param_typename, 0), end;
            ++typename_it; // First match is the function name
            auto& top_design_interface_typename = HLSMgr->design_interface_typename[fname];
            auto& top_design_interface_typename_signature = HLSMgr->design_interface_typename_signature[fname];
            auto& top_design_interface_typename_orig_signature =
                HLSMgr->design_interface_typename_orig_signature[fname];
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Iterating arguments:");
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->");
            for(const auto& arg : fd->list_of_args)
            {
               THROW_ASSERT(typename_it != end, "");
               const auto pname = [&]()
               {
                  std::stringstream ss;
                  ss << arg;
                  return ss.str();
               }();
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Argument " + pname);
               const std::string tname(*typename_it);
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Typename " + tname);
               top_design_interface_typename[pname] = tname;
               top_design_interface_typename_signature.push_back(tname);
               top_design_interface_typename_orig_signature.push_back(tname);
               if(tname.find("fixed<") != std::string::npos)
               {
                  HLSMgr->design_interface_typenameinclude[fname][pname] =
                      std::string(PANDA_DATA_INSTALLDIR "/panda/ac_types/include/" + tname.substr(0, 2) + "_fixed.h");
               }
               if(tname.find("int<") != std::string::npos)
               {
                  HLSMgr->design_interface_typenameinclude[fname][pname] =
                      std::string(PANDA_DATA_INSTALLDIR "/panda/ac_types/include/" + tname.substr(0, 2) + "_int.h");
               }
               ++typename_it;
            }
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--");
         }
      }

      if(parameters->getOption<HLSFlowStep_Type>(OPT_interface_type) == HLSFlowStep_Type::INFERRED_INTERFACE_GENERATION)
      {
         if(HLSMgr->design_interface.count(fname))
         {
            const tree_manipulationRef tree_man(new tree_manipulation(TM, parameters, AppM));
            /// pre-process the list of statements to bind parm_decl and ssa variables
            const auto sl = GetPointer<statement_list>(GET_NODE(fd->body));

            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Analyzing function " + fname);
            auto& DesignInterfaceArgs = HLSMgr->design_interface.at(fname);
            auto& DesignInterfaceTypenameArgs = HLSMgr->design_interface_typename.at(fname);
            for(const auto& arg : fd->list_of_args)
            {
               const auto arg_pd = GetPointerS<const parm_decl>(GET_CONST_NODE(arg));
               const auto arg_id = GET_INDEX_NODE(arg);
               const auto& arg_type = arg_pd->type;
               THROW_ASSERT(GetPointer<const identifier_node>(GET_CONST_NODE(arg_pd->name)), "unexpected condition");
               const auto& arg_name = GetPointerS<const identifier_node>(GET_CONST_NODE(arg_pd->name))->strg;
               INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "---Parameter @" + STR(arg_id) + " " + arg_name);
               THROW_ASSERT(DesignInterfaceArgs.count(arg_name), "Not matched parameter name: " + arg_name);
               auto interfaceType = DesignInterfaceArgs.at(arg_name);
               if(interfaceType != "default")
               {
                  const auto arg_ssa_id = AppM->getSSAFromParm(top_id, arg_id);
                  const auto arg_ssa = TM->GetTreeReindex(arg_ssa_id);
                  THROW_ASSERT(GET_CONST_NODE(arg_ssa)->get_kind() == ssa_name_K, "");
                  if(GetPointerS<const ssa_name>(GET_CONST_NODE(arg_ssa))->CGetUseStmts().empty())
                  {
                     THROW_WARNING("Parameter '" + arg_name + "' not used by any statement");
                     if(tree_helper::IsPointerType(arg_type))
                     {
                        DesignInterfaceArgs[arg_name] = "none";
                     }
                     else
                     {
                        THROW_ERROR("parameter not used: specified interface does not make sense - " + interfaceType);
                     }
                     continue;
                  }
                  if(interfaceType == "bus") /// TO BE FIXED
                  {
                     DesignInterfaceArgs[arg_name] = "default";
                     continue;
                  }
                  if(tree_helper::IsPointerType(arg_type))
                  {
                     INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "---Is a pointer type");
                     INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "---Parameter uses:");
                     const auto ptd_type = tree_helper::CGetPointedType(arg_type);
                     bool is_signed = tree_helper::IsSignedIntegerType(ptd_type);
                     bool is_fixed = false;
                     const auto acType_bw =
                         ac_type_bitwidth(DesignInterfaceTypenameArgs.at(arg_name), is_signed, is_fixed);
                     const auto is_acType = acType_bw != 0;
                     const auto input_bw = [&]()
                     {
                        if(is_acType)
                        {
                           return acType_bw;
                        }
                        else if(tree_helper::IsArrayType(ptd_type))
                        {
                           return tree_helper::GetArrayElementSize(ptd_type);
                        }
                        return tree_helper::Size(ptd_type);
                     }();
                     THROW_ASSERT(input_bw, "unexpected condition");
                     unsigned long long n_resources;
                     unsigned long long alignment;
                     ComputeResourcesAlignment(n_resources, alignment, input_bw, is_acType, is_signed, is_fixed);

                     std::list<tree_nodeRef> writeStmt;
                     std::list<tree_nodeRef> readStmt;
                     classifyArg(sl, arg_ssa, writeStmt, readStmt);
                     const auto isRead = !readStmt.empty();
                     const auto isWrite = !writeStmt.empty();

                     if(!isRead && !isWrite)
                     {
                        THROW_ERROR("Parameter '" + arg_name + "' cannot have interface type '" + interfaceType +
                                    "' (no load/store is associated with it)");
                     }

                     if(isRead && isWrite)
                     {
                        INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "---IO arg");
                        if(interfaceType == "ptrdefault")
                        {
                           if(parameters->IsParameter("none-ptrdefault") &&
                              parameters->GetParameter<int>("none-ptrdefault") == 1)
                           {
                              DesignInterfaceArgs[arg_name] = "none";
                              interfaceType = "none";
                           }
                           else if(parameters->IsParameter("none-registered-ptrdefault") &&
                                   parameters->GetParameter<int>("none-registered-ptrdefault") == 1)
                           {
                              DesignInterfaceArgs[arg_name] = "none_registered";
                              interfaceType = "none_registered";
                           }
                           else
                           {
                              DesignInterfaceArgs[arg_name] = "ovalid";
                              interfaceType = "ovalid";
                           }
                        }
                        else if(interfaceType == "fifo" || interfaceType == "axis")
                        {
                           THROW_ERROR("parameter " + arg_name + " cannot have interface " + interfaceType +
                                       " because it cannot be read and write at the same time");
                        }
                     }
                     else if(isRead)
                     {
                        INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "---I arg");
                        if(interfaceType == "ptrdefault")
                        {
                           DesignInterfaceArgs[arg_name] = "none";
                           interfaceType = "none";
                        }
                        else if(interfaceType == "ovalid")
                        {
                           THROW_ERROR("parameter " + arg_name + " cannot have interface " + interfaceType +
                                       " because it is read only");
                        }
                     }
                     else if(isWrite)
                     {
                        INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "---O arg");
                        if(interfaceType == "ptrdefault")
                        {
                           if(parameters->IsParameter("none-ptrdefault") &&
                              parameters->GetParameter<int>("none-ptrdefault") == 1)
                           {
                              DesignInterfaceArgs[arg_name] = "none";
                              interfaceType = "none";
                           }
                           else if(parameters->IsParameter("none-registered-ptrdefault") &&
                                   parameters->GetParameter<int>("none-registered-ptrdefault") == 1)
                           {
                              DesignInterfaceArgs[arg_name] = "none_registered";
                              interfaceType = "none_registered";
                           }
                           else
                           {
                              DesignInterfaceArgs[arg_name] = "valid";
                              interfaceType = "valid";
                           }
                        }
                     }

                     bool is_real = false;
                     bool isDiffSize = false;
                     unsigned long long rwsize = 1U;
                     std::set<std::string> operationsR, operationsW;
                     const auto commonRWSignature = interfaceType == "array" || interfaceType == "m_axi";
                     if(isRead)
                     {
                        unsigned int IdIndex = 0;
                        for(const auto& rs : readStmt)
                        {
                           const auto instanceFname = ENCODE_FDNAME(
                               arg_name, "_Read_" + (n_resources == 1 ? "" : (STR(IdIndex++) + "_")), interfaceType);
                           operationsR.insert(instanceFname);
                           const auto rs_node = GET_NODE(rs);
                           const auto rs_ga = GetPointer<gimple_assign>(rs_node);
                           const auto readType = GetPointer<mem_ref>(GET_NODE(rs_ga->op1))->type;
                           is_real = is_real || tree_helper::IsRealType(readType);
                           create_Read_function(rs, arg_name, instanceFname, arg_type, readType, tree_man, TM,
                                                commonRWSignature);
                           rwsize = std::max(rwsize, tree_helper::Size(readType));
                           add_to_modified(rs);
                        }
                     }
                     if(isWrite)
                     {
                        unsigned int IdIndex = 0;
                        unsigned long long WrittenSize = 0;
                        for(const auto& ws : writeStmt)
                        {
                           const auto instanceFname =
                               ENCODE_FDNAME(arg_name, "_Write_" + (n_resources == 1 ? "" : (STR(IdIndex++) + "_")),
                                             (interfaceType == "ovalid" ? "valid" : interfaceType));
                           operationsW.insert(instanceFname);
                           const auto ws_node = GET_NODE(ws);
                           const auto ws_ga = GetPointer<gimple_assign>(ws_node);
                           if(WrittenSize == 0)
                           {
                              WrittenSize = tree_helper::Size(ws_ga->op1);
                              if(WrittenSize < input_bw)
                              {
                                 isDiffSize = true;
                              }
                           }
                           else if(WrittenSize != tree_helper::Size(ws_ga->op1) || WrittenSize < input_bw)
                           {
                              isDiffSize = true;
                           }
                           rwsize = std::max(rwsize, tree_helper::Size(ws_ga->op1));
                           const auto writeType = GetPointerS<mem_ref>(GET_NODE(ws_ga->op0))->type;
                           is_real = is_real || tree_helper::IsRealType(writeType);
                           create_Write_function(arg_name, ws, instanceFname, ws_ga->op1, arg_type, writeType, tree_man,
                                                 TM, commonRWSignature);
                           add_to_modified(ws);
                        }
                     }

                     create_resource(operationsR, operationsW, arg_name, interfaceType, input_bw, isDiffSize, fname,
                                     n_resources, alignment, is_real, rwsize, top_id);
                  }
                  else if(interfaceType == "none")
                  {
                     THROW_ERROR("Interface type '" + interfaceType + "' for parameter '" + arg_name + "' unexpected");
                  }
                  else
                  {
                     THROW_ERROR("Interface type '" + interfaceType + "' for parameter '" + arg_name +
                                 "' is not supported");
                  }
               }
            }

            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Analyzed function " + fname);
         }
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

void InterfaceInfer::classifyArgRecurse(CustomOrderedSet<unsigned>& Visited, tree_nodeRef ssa_node,
                                        const statement_list* sl, std::list<tree_nodeRef>& writeStmt,
                                        std::list<tree_nodeRef>& readStmt)
{
   const auto ssa_var = GetPointer<const ssa_name>(GET_CONST_NODE(ssa_node));
   INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level,
                  "---SSA VARIABLE: " + ssa_var->ToString() + " with " + STR(ssa_var->CGetUseStmts().size()) +
                      " use statements");
   const auto propagate_arg_use =
       [&](tree_nodeRef fd_node, const std::vector<tree_nodeRef>& call_args, size_t use_count)
   {
      THROW_ASSERT(ssa_var->var, "unexpected condition");
      const auto orig_pd = GetPointerS<const parm_decl>(GET_CONST_NODE(ssa_var->var));
      const auto& orig_arg_name = [&]()
      {
         THROW_ASSERT(GetPointer<const identifier_node>(GET_CONST_NODE(orig_pd->name)), "unexpected condition");
         return GetPointerS<const identifier_node>(GET_CONST_NODE(orig_pd->name))->strg;
      }();
      const auto orig_fname = [&]()
      {
         const auto fun_node = GET_CONST_NODE(orig_pd->scpe);
         THROW_ASSERT(fun_node && fun_node->get_kind() == function_decl_K, "unexpected condition");
         const auto fd = GetPointerS<const function_decl>(fun_node);
         std::string fname;
         tree_helper::get_mangled_fname(fd, fname);
         return fname;
      }();
      THROW_ASSERT(GET_CONST_NODE(fd_node)->get_kind() == function_decl_K, "unexpected condition");
      const auto call_fd = GetPointerS<const function_decl>(GET_CONST_NODE(fd_node));
      THROW_ASSERT(call_fd->body, "unexpected condition");
      std::string call_fname;
      tree_helper::get_mangled_fname(call_fd, call_fname);
      size_t par_index = 0U;
      for(auto use_idx = 0U; use_idx < use_count; ++use_idx, ++par_index)
      {
         // look for the actual vs formal parameter binding
         par_index = [&](size_t start_idx)
         {
            for(auto idx = start_idx; idx < call_args.size(); ++idx)
            {
               if(GET_INDEX_CONST_NODE(call_args[idx]) == ssa_var->index)
               {
                  return idx;
               }
            }
            THROW_ERROR("Use of " + ssa_var->ToString() + " not found.");
            return static_cast<size_t>(-1);
         }(par_index);
         THROW_ASSERT(call_fd->list_of_args.size() > par_index, "unexpected condition");
         const auto call_arg_id = GET_INDEX_CONST_NODE(call_fd->list_of_args[par_index]);

         const auto call_arg_ssa_id = AppM->getSSAFromParm(call_fd->index, call_arg_id);
         const auto TM = AppM->get_tree_manager();
         const auto call_arg_ssa = TM->CGetTreeReindex(call_arg_ssa_id);
         THROW_ASSERT(GET_CONST_NODE(call_arg_ssa)->get_kind() == ssa_name_K, "");
         if(GetPointerS<const ssa_name>(GET_CONST_NODE(call_arg_ssa))->CGetUseStmts().size())
         {
            const auto call_arg_name = [&]()
            {
               const auto pd = GetPointerS<const parm_decl>(GET_CONST_NODE(call_fd->list_of_args[par_index]));
               THROW_ASSERT(GetPointer<const identifier_node>(GET_CONST_NODE(pd->name)), "unexpected condition");
               return GetPointerS<const identifier_node>(GET_CONST_NODE(pd->name))->strg;
            }();
            /// propagate design interfaces
            INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level,
                           "---Argument " + orig_arg_name + " from " + orig_fname + " forwarded to " + call_fname +
                               " as argument " + call_arg_name);
            const auto HLSMgr = GetPointer<HLS_manager>(AppM);
            if(HLSMgr->design_interface_arraysize.count(orig_fname) &&
               HLSMgr->design_interface_arraysize.at(orig_fname).count(orig_arg_name))
            {
               HLSMgr->design_interface_arraysize[call_fname][call_arg_name] =
                   HLSMgr->design_interface_arraysize.at(orig_fname).at(orig_arg_name);
            }
            if(HLSMgr->design_interface_attribute2.count(orig_fname) &&
               HLSMgr->design_interface_attribute2.at(orig_fname).count(orig_arg_name))
            {
               HLSMgr->design_interface_attribute2[call_fname][call_arg_name] =
                   HLSMgr->design_interface_attribute2.at(orig_fname).at(orig_arg_name);
            }
            if(HLSMgr->design_interface_attribute3.count(orig_fname) &&
               HLSMgr->design_interface_attribute3.at(orig_fname).count(orig_arg_name))
            {
               HLSMgr->design_interface_attribute3[call_fname][call_arg_name] =
                   HLSMgr->design_interface_attribute3.at(orig_fname).at(orig_arg_name);
            }
            const auto call_sl = GetPointerS<const statement_list>(GET_CONST_NODE(call_fd->body));
            classifyArgRecurse(Visited, call_arg_ssa, call_sl, writeStmt, readStmt);
            INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "---Sub-function done");
         }
      }
   };

   for(const auto& stmt_count : ssa_var->CGetUseStmts())
   {
      const auto use_stmt = GET_CONST_NODE(stmt_count.first);
      const auto& use_count = stmt_count.second;
      if(!Visited.insert(GET_INDEX_CONST_NODE(stmt_count.first)).second)
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "---SKIPPED STMT: " + use_stmt->ToString());
         continue;
      }
      INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "---STMT: " + use_stmt->ToString());
      if(const auto ga = GetPointer<const gimple_assign>(use_stmt))
      {
         if(GET_CONST_NODE(ga->op0)->get_kind() == mem_ref_K)
         {
            if(GET_CONST_NODE(ga->op1)->get_kind() == mem_ref_K)
            {
               THROW_ERROR("Pattern currently not supported: *x=*y; " + use_stmt->ToString());
            }
            else
            {
               THROW_ASSERT(GET_CONST_NODE(ga->op1)->get_kind() == ssa_name_K ||
                                GetPointer<const cst_node>(GET_CONST_NODE(ga->op1)),
                            "unexpected condition");
               if(GetPointer<const cst_node>(GET_CONST_NODE(ga->op1)) ||
                  GetPointer<const ssa_name>(GET_CONST_NODE(ga->op1)) != ssa_var)
               {
                  writeStmt.push_back(stmt_count.first);
               }
            }
         }
         else if(GET_CONST_NODE(ga->op1)->get_kind() == mem_ref_K)
         {
            readStmt.push_back(stmt_count.first);
         }
         else if(GET_CONST_NODE(ga->op1)->get_kind() == call_expr_K)
         {
            const auto ce = GetPointerS<const call_expr>(GET_CONST_NODE(ga->op1));
            const auto return_type = tree_helper::CGetType(ga->op0);
            if(tree_helper::IsPointerType(return_type))
            {
               THROW_ERROR("unexpected pattern");
            }
            propagate_arg_use(ce->fn, ce->args, use_count);
         }
         else if(GET_CONST_NODE(ga->op1)->get_kind() == nop_expr_K ||
                 GET_CONST_NODE(ga->op1)->get_kind() == view_convert_expr_K ||
                 GET_CONST_NODE(ga->op1)->get_kind() == ssa_name_K ||
                 GET_CONST_NODE(ga->op1)->get_kind() == pointer_plus_expr_K ||
                 GET_CONST_NODE(ga->op1)->get_kind() == cond_expr_K)
         {
            classifyArgRecurse(Visited, ga->op0, sl, writeStmt, readStmt);
         }
         else
         {
            THROW_ERROR("Pattern currently not supported: parameter used in a non-supported statement " +
                        use_stmt->ToString() + ":" + GET_CONST_NODE(ga->op1)->get_kind_text());
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
               propagate_arg_use(ae->op, gc->args, use_count);
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
         classifyArgRecurse(Visited, gp->res, sl, writeStmt, readStmt);
      }
      else
      {
         THROW_ERROR("USE PATTERN unexpected" + use_stmt->ToString());
      }
   }
}

void InterfaceInfer::classifyArg(statement_list* sl, tree_nodeRef ssa_var, std::list<tree_nodeRef>& writeStmt,
                                 std::list<tree_nodeRef>& readStmt)
{
   CustomOrderedSet<unsigned> Visited;
   classifyArgRecurse(Visited, ssa_var, sl, writeStmt, readStmt);
}

void InterfaceInfer::create_Read_function(tree_nodeRef origStmt, const std::string& arg_name, const std::string& fdName,
                                          tree_nodeRef aType, tree_nodeRef readType,
                                          const tree_manipulationRef tree_man, const tree_managerRef TM,
                                          bool commonRWSignature)
{
   THROW_ASSERT(origStmt, "expected a ref statement");
   THROW_ASSERT(GET_NODE(origStmt)->get_kind() == gimple_assign_K, "unexpected condition");
   const auto ga = GetPointer<gimple_assign>(GET_NODE(origStmt));
   unsigned int destBB = ga->bb_index;
   THROW_ASSERT(ga->scpe, "expected a scope");
   THROW_ASSERT(GET_NODE(ga->scpe)->get_kind() == function_decl_K, "expected a function_decl");
   const auto fd = GetPointer<function_decl>(GET_NODE(ga->scpe));
   THROW_ASSERT(fd->body, "expected a body");
   const auto sl = GetPointer<statement_list>(GET_NODE(fd->body));
   std::string fname;
   tree_helper::get_mangled_fname(fd, fname);
   /// create the function_decl
   std::vector<tree_nodeRef> argsT;
   tree_nodeRef bit_size_type;
   tree_nodeRef boolean_type;
   if(commonRWSignature)
   {
      boolean_type = tree_man->GetBooleanType();
      bit_size_type = tree_man->GetUnsignedIntegerType();
      argsT.push_back(boolean_type);
      argsT.push_back(bit_size_type);
      argsT.push_back(readType);
   }
   argsT.push_back(aType);
   const std::string srcp = fd->include_name + ":" + STR(fd->line_number) + ":" + STR(fd->column_number);
   auto function_decl_node = tree_man->create_function_decl(fdName, fd->scpe, argsT, readType, srcp, false);
   std::vector<tree_nodeRef> args;
   if(commonRWSignature)
   {
      const auto sel_value = TM->CreateUniqueIntegerCst(0, boolean_type);
      args.push_back(sel_value);
      const auto size_value = TM->CreateUniqueIntegerCst(tree_helper::Size(readType), bit_size_type);
      args.push_back(size_value);
      tree_nodeRef data_value;
      if(GET_NODE(readType)->get_kind() == integer_type_K || GET_NODE(readType)->get_kind() == enumeral_type_K ||
         GET_NODE(readType)->get_kind() == pointer_type_K || GET_NODE(readType)->get_kind() == reference_type_K)
      {
         data_value = TM->CreateUniqueIntegerCst(0, readType);
      }
      else if(tree_helper::IsRealType(readType))
      {
         data_value = TM->CreateUniqueRealCst(0.l, readType);
      }
      else
      {
         THROW_ERROR("unexpected data type");
      }
      args.push_back(data_value);
   }

   THROW_ASSERT(GET_NODE(ga->op1)->get_kind() == mem_ref_K, "unexpected condition");
   const auto mr = GetPointer<mem_ref>(GET_NODE(ga->op1));
   args.push_back(mr->op0);

   const auto call_expr_node = tree_man->CreateCallExpr(function_decl_node, args, srcp);
   const auto new_call = tree_man->CreateGimpleAssign(readType, tree_nodeRef(), tree_nodeRef(), call_expr_node,
                                                      GET_INDEX_NODE(ga->scpe), destBB, srcp);
   sl->list_of_bloc.at(destBB)->PushBefore(new_call, origStmt, AppM);

   const auto newGN = GetPointer<gimple_assign>(GET_NODE(new_call));
   const tree_nodeRef temp_ssa_var = newGN->op0;
   const auto ssaDefVar = GetPointer<ssa_name>(GET_NODE(ga->op0));
   THROW_ASSERT(ssaDefVar, "unexpected condition");
   const auto StmtUses = ssaDefVar->CGetUseStmts();
   for(const auto& used : StmtUses)
   {
      TM->ReplaceTreeNode(used.first, ga->op0, temp_ssa_var);
   }

   FixReadWriteCall(ga, newGN, tree_man, new_call, sl, TM, origStmt, destBB, fname, arg_name);
   INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "---LOAD STMT: " + new_call->ToString() + " in function " + fname);
}

void InterfaceInfer::create_Write_function(const std::string& arg_name, tree_nodeRef origStmt,
                                           const std::string& fdName, tree_nodeRef writeValue, tree_nodeRef aType,
                                           tree_nodeRef writeType, const tree_manipulationRef tree_man,
                                           const tree_managerRef TM, bool commonRWSignature)
{
   THROW_ASSERT(origStmt, "expected a ref statement");
   THROW_ASSERT(GET_NODE(origStmt)->get_kind() == gimple_assign_K, "unexpected condition");
   auto ga = GetPointer<gimple_assign>(GET_NODE(origStmt));
   THROW_ASSERT(GET_NODE(ga->op0)->get_kind() == mem_ref_K, "unexpected condition");
   unsigned int destBB = ga->bb_index;
   THROW_ASSERT(ga->scpe, "expected a scope");
   THROW_ASSERT(GET_NODE(ga->scpe)->get_kind() == function_decl_K, "expected a function_decl");
   auto fd = GetPointer<function_decl>(GET_NODE(ga->scpe));
   THROW_ASSERT(fd->body, "expected a body");
   const auto sl = GetPointerS<statement_list>(GET_NODE(fd->body));
   std::string fname;
   tree_helper::get_mangled_fname(fd, fname);
   tree_nodeRef boolean_type;
   const auto bit_size_type = tree_man->GetUnsignedIntegerType();
   const auto size_value = TM->CreateUniqueIntegerCst(tree_helper::Size(writeType), bit_size_type);

   /// create the function_decl
   std::vector<tree_nodeRef> argsT;
   if(commonRWSignature)
   {
      boolean_type = tree_man->GetBooleanType();
      argsT.push_back(boolean_type);
   }
   argsT.push_back(bit_size_type);
   if(tree_helper::IsSignedIntegerType(writeValue))
   {
      argsT.push_back(tree_man->CreateUnsigned(tree_helper::CGetType(writeValue)));
   }
   else
   {
      argsT.push_back(writeType);
   }
   argsT.push_back(aType);
   const std::string srcp = fd->include_name + ":" + STR(fd->line_number) + ":" + STR(fd->column_number);
   const auto function_decl_node =
       tree_man->create_function_decl(fdName, fd->scpe, argsT, tree_man->GetVoidType(), srcp, false);

   std::vector<tree_nodeRef> args;
   if(commonRWSignature)
   {
      const auto sel_value = TM->CreateUniqueIntegerCst(1, boolean_type);
      args.push_back(sel_value);
   }
   args.push_back(size_value);
   if(tree_helper::IsSignedIntegerType(writeValue))
   {
      const auto ga_nop =
          tree_man->CreateNopExpr(writeValue, tree_man->CreateUnsigned(tree_helper::CGetType(writeValue)), nullptr,
                                  nullptr, GET_INDEX_NODE(ga->scpe));
      sl->list_of_bloc.at(destBB)->PushBefore(ga_nop, origStmt, AppM);
      args.push_back(GetPointerS<gimple_assign>(GET_NODE(ga_nop))->op0);
   }
   else
   {
      args.push_back(writeValue);
   }
   const auto mr = GetPointerS<mem_ref>(GET_NODE(ga->op0));
   args.push_back(mr->op0);

   const auto new_call = tree_man->create_gimple_call(function_decl_node, args, GET_INDEX_NODE(ga->scpe), srcp, destBB);
   sl->list_of_bloc.at(destBB)->PushBefore(new_call, origStmt, AppM);

   const auto newGN = GetPointer<gimple_node>(GET_NODE(new_call));
   FixReadWriteCall(ga, newGN, tree_man, new_call, sl, TM, origStmt, destBB, fname, arg_name);
   INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level,
                  "---STORE STMT: " + new_call->ToString() + " in function " + fname);
}

void InterfaceInfer::create_resource_Read_simple(const std::set<std::string>& operations, const std::string& arg_name,
                                                 const std::string& interfaceType, unsigned long long input_bw,
                                                 bool IO_port, unsigned long long n_resources,
                                                 unsigned long long rwBWsize, unsigned int top_id) const
{
   const std::string ResourceName = ENCODE_FDNAME(arg_name, "_Read_", interfaceType);
   auto HLSMgr = GetPointer<HLS_manager>(AppM);
   auto HLS_T = HLSMgr->get_HLS_target();
   auto TechMan = HLS_T->get_technology_manager();
   if(!TechMan->is_library_manager(INTERFACE_LIBRARY) ||
      !TechMan->get_library_manager(INTERFACE_LIBRARY)->is_fu(ResourceName))
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level,
                     "-->Creating interface resource: " + INTERFACE_LIBRARY + ":" + ResourceName);
      structural_objectRef interface_top;
      structural_managerRef CM = structural_managerRef(new structural_manager(parameters));
      structural_type_descriptorRef module_type =
          structural_type_descriptorRef(new structural_type_descriptor(ResourceName));
      CM->set_top_info(ResourceName, module_type);
      interface_top = CM->get_circ();
      /// add description and license
      GetPointer<module>(interface_top)->set_description("Interface module for function: " + ResourceName);
      GetPointer<module>(interface_top)->set_copyright(GENERATED_COPYRIGHT);
      GetPointer<module>(interface_top)->set_authors("Component automatically generated by bambu");
      GetPointer<module>(interface_top)->set_license(GENERATED_LICENSE);
      const auto isMultipleResource = interfaceType == "acknowledge" || interfaceType == "valid" ||
                                      interfaceType == "handshake" || interfaceType == "fifo" ||
                                      interfaceType == "axis";
      if(isMultipleResource)
      {
         GetPointer<module>(interface_top)->set_multi_unit_multiplicity(n_resources);
      }

      auto address_bitsize = HLSMgr->get_address_bitsize();
      structural_type_descriptorRef addrType =
          structural_type_descriptorRef(new structural_type_descriptor("bool", address_bitsize));
      structural_type_descriptorRef dataType =
          structural_type_descriptorRef(new structural_type_descriptor("bool", input_bw));
      structural_type_descriptorRef bool_type =
          structural_type_descriptorRef(new structural_type_descriptor("bool", 0));
      structural_type_descriptorRef rwtype =
          structural_type_descriptorRef(new structural_type_descriptor("bool", rwBWsize));
      if(interfaceType == "valid" || interfaceType == "handshake" || interfaceType == "fifo" || interfaceType == "axis")
      {
         CM->add_port(CLOCK_PORT_NAME, port_o::IN, interface_top, bool_type);
         CM->add_port(RESET_PORT_NAME, port_o::IN, interface_top, bool_type);
      }
      if(isMultipleResource)
      {
         CM->add_port_vector(START_PORT_NAME, port_o::IN, n_resources, interface_top, bool_type);
      }
      structural_objectRef addrPort;
      if(isMultipleResource)
      {
         addrPort = CM->add_port_vector("in1", port_o::IN, n_resources, interface_top, addrType);
      }
      else
      {
         addrPort = CM->add_port("in1", port_o::IN, interface_top, addrType); // this port has a fixed name
      }
      GetPointer<port_o>(addrPort)->set_is_addr_bus(true);
      // GetPointer<port_o>(addrPort)->set_is_var_args(true); /// required to activate the module generation
      if(interfaceType == "valid" || interfaceType == "handshake" || interfaceType == "fifo" || interfaceType == "axis")
      {
         CM->add_port_vector(DONE_PORT_NAME, port_o::OUT, n_resources, interface_top, bool_type);
      }
      if(isMultipleResource)
      {
         CM->add_port_vector("out1", port_o::OUT, n_resources, interface_top, rwtype);
      }
      else
      {
         CM->add_port("out1", port_o::OUT, interface_top, rwtype);
      }

      std::string port_data_name;
      if(interfaceType == "axis")
      {
         port_data_name = "_s_axis_" + arg_name + "_TDATA";
      }
      else
      {
         port_data_name = "_" + arg_name + (interfaceType == "fifo" ? "_dout" : (IO_port ? "_i" : ""));
      }
      auto inPort = CM->add_port(port_data_name, port_o::IN, interface_top, dataType);
      GetPointer<port_o>(inPort)->set_port_interface((interfaceType == "axis" || interfaceType == "fifo") ?
                                                         port_o::port_interface::PI_FDOUT :
                                                         port_o::port_interface::PI_RNONE);
      if(interfaceType == "acknowledge" || interfaceType == "handshake")
      {
         auto inPort_o_ack =
             CM->add_port("_" + arg_name + (IO_port ? "_i" : "") + "_ack", port_o::OUT, interface_top, bool_type);
         GetPointer<port_o>(inPort_o_ack)->set_port_interface(port_o::port_interface::PI_RACK);
      }
      if(interfaceType == "valid" || interfaceType == "handshake")
      {
         auto inPort_o_vld =
             CM->add_port("_" + arg_name + (IO_port ? "_i" : "") + "_vld", port_o::IN, interface_top, bool_type);
         GetPointer<port_o>(inPort_o_vld)->set_port_interface(port_o::port_interface::PI_RVALID);
      }
      if(interfaceType == "fifo")
      {
         auto inPort_empty_n = CM->add_port("_" + arg_name + "_empty_n", port_o::IN, interface_top, bool_type);
         GetPointer<port_o>(inPort_empty_n)->set_port_interface(port_o::port_interface::PI_EMPTY_N);
         auto inPort_read = CM->add_port("_" + arg_name + "_read", port_o::OUT, interface_top, bool_type);
         GetPointer<port_o>(inPort_read)->set_port_interface(port_o::port_interface::PI_READ);
      }
      if(interfaceType == "axis")
      {
         auto inPort_empty_n = CM->add_port("_s_axis_" + arg_name + "_TVALID", port_o::IN, interface_top, bool_type);
         GetPointer<port_o>(inPort_empty_n)->set_port_interface(port_o::port_interface::PI_S_AXIS_TVALID);
         auto inPort_read = CM->add_port("_s_axis_" + arg_name + "_TREADY", port_o::OUT, interface_top, bool_type);
         GetPointer<port_o>(inPort_read)->set_port_interface(port_o::port_interface::PI_S_AXIS_TREADY);
      }

      CM->add_NP_functionality(interface_top, NP_functionality::LIBRARY, "in1 out1");
      CM->add_NP_functionality(interface_top, NP_functionality::VERILOG_GENERATOR,
                               "Read_" + interfaceType + "ModuleGenerator");
      TechMan->add_resource(INTERFACE_LIBRARY, ResourceName, CM);
      for(const auto& fdName : operations)
      {
         TechMan->add_operation(INTERFACE_LIBRARY, ResourceName, fdName);
      }
      auto* fu = GetPointer<functional_unit>(TechMan->get_fu(ResourceName, INTERFACE_LIBRARY));
      const target_deviceRef device = HLS_T->get_target_device();
      fu->area_m = area_model::create_model(device->get_type(), parameters);
      fu->area_m->set_area_value(0);
      if(!isMultipleResource)
      {
         fu->logical_type = functional_unit::COMBINATIONAL;
      }

      for(const auto& fdName : operations)
      {
         auto* op = GetPointer<operation>(fu->get_operation(fdName));
         op->time_m = time_model::create_model(device->get_type(), parameters);
         if(interfaceType == "valid" || interfaceType == "handshake" || interfaceType == "fifo" ||
            interfaceType == "axis")
         {
            op->bounded = false;
            op->time_m->set_execution_time(HLS_T->get_technology_manager()->CGetSetupHoldTime() + EPSILON, 0);
         }
         else
         {
            op->bounded = true;
            op->time_m->set_execution_time(EPSILON, 0);
            op->time_m->set_stage_period(0.0);
         }
         op->time_m->set_synthesis_dependent(true);
      }
      if(isMultipleResource)
      {
         HLSMgr->design_interface_constraints[top_id][INTERFACE_LIBRARY][ResourceName] = n_resources;
      }
      /// otherwise no constraints are required for this resource
      INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "<--Interface resource created");
   }
}

void InterfaceInfer::create_resource_Write_simple(const std::set<std::string>& operations, const std::string& arg_name,
                                                  const std::string& interfaceType, unsigned long long input_bw,
                                                  bool IO_port, bool isDiffSize, unsigned long long n_resources,
                                                  bool is_real, unsigned long long rwBWsize, unsigned int top_id) const
{
   const std::string ResourceName = ENCODE_FDNAME(arg_name, "_Write_", interfaceType);
   auto HLSMgr = GetPointer<HLS_manager>(AppM);
   auto HLS_T = HLSMgr->get_HLS_target();
   auto TechMan = HLS_T->get_technology_manager();
   if(!TechMan->is_library_manager(INTERFACE_LIBRARY) ||
      !TechMan->get_library_manager(INTERFACE_LIBRARY)->is_fu(ResourceName))
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level,
                     "-->Creating interface resource: " + INTERFACE_LIBRARY + ":" + ResourceName);
      structural_objectRef interface_top;
      structural_managerRef CM = structural_managerRef(new structural_manager(parameters));
      structural_type_descriptorRef module_type =
          structural_type_descriptorRef(new structural_type_descriptor(ResourceName));
      CM->set_top_info(ResourceName, module_type);
      interface_top = CM->get_circ();
      /// add description and license
      GetPointer<module>(interface_top)->set_description("Interface module for function: " + ResourceName);
      GetPointer<module>(interface_top)->set_copyright(GENERATED_COPYRIGHT);
      GetPointer<module>(interface_top)->set_authors("Component automatically generated by bambu");
      GetPointer<module>(interface_top)->set_license(GENERATED_LICENSE);
      const auto isAVH = interfaceType == "acknowledge" || interfaceType == "valid" || interfaceType == "handshake" ||
                         interfaceType == "fifo" || interfaceType == "axis" || interfaceType == "none_registered";
      const auto isMultipleResource = isDiffSize || isAVH;

      if(isMultipleResource)
      {
         GetPointer<module>(interface_top)->set_multi_unit_multiplicity(n_resources);
      }

      const auto address_bitsize = HLSMgr->get_address_bitsize();
      structural_type_descriptorRef addrType =
          structural_type_descriptorRef(new structural_type_descriptor("bool", address_bitsize));
      structural_type_descriptorRef dataType =
          structural_type_descriptorRef(new structural_type_descriptor("bool", input_bw));
      if(is_real)
      {
         dataType->type = structural_type_descriptor::REAL;
      }
      auto nbitDataSize = 32ull - static_cast<unsigned>(__builtin_clzll(rwBWsize));
      structural_type_descriptorRef rwsize =
          structural_type_descriptorRef(new structural_type_descriptor("bool", nbitDataSize));
      structural_type_descriptorRef rwtype =
          structural_type_descriptorRef(new structural_type_descriptor("bool", rwBWsize));
      structural_type_descriptorRef bool_type =
          structural_type_descriptorRef(new structural_type_descriptor("bool", 0));
      if(interfaceType == "none_registered" || interfaceType == "acknowledge" || interfaceType == "handshake" ||
         interfaceType == "fifo" || interfaceType == "axis")
      {
         CM->add_port(CLOCK_PORT_NAME, port_o::IN, interface_top, bool_type);
         CM->add_port(RESET_PORT_NAME, port_o::IN, interface_top, bool_type);
      }
      if(isMultipleResource)
      {
         CM->add_port_vector(START_PORT_NAME, port_o::IN, n_resources, interface_top, bool_type);
      }
      structural_objectRef addrPort;
      if(isMultipleResource)
      {
         CM->add_port_vector("in1", port_o::IN, n_resources, interface_top, rwsize);
         CM->add_port_vector("in2", port_o::IN, n_resources, interface_top, rwtype);
         addrPort = CM->add_port_vector("in3", port_o::IN, n_resources, interface_top, addrType);
      }
      else
      {
         CM->add_port("in1", port_o::IN, interface_top, rwsize);
         CM->add_port("in2", port_o::IN, interface_top, rwtype);
         addrPort = CM->add_port("in3", port_o::IN, interface_top, addrType);
      }
      GetPointer<port_o>(addrPort)->set_is_addr_bus(true);
      // GetPointer<port_o>(addrPort)->set_is_var_args(true); /// required to activate the module generation
      if(interfaceType == "none_registered" || interfaceType == "acknowledge" || interfaceType == "handshake" ||
         interfaceType == "fifo" || interfaceType == "axis")
      {
         CM->add_port_vector(DONE_PORT_NAME, port_o::OUT, n_resources, interface_top, bool_type);
      }
      std::string port_data_name;
      if(interfaceType == "axis")
      {
         port_data_name = "_m_axis_" + arg_name + "_TDATA";
      }
      else
      {
         port_data_name = "_" + arg_name + (interfaceType == "fifo" ? "_din" : (IO_port ? "_o" : ""));
      }
      auto inPort_o = CM->add_port(port_data_name, port_o::OUT, interface_top, dataType);
      GetPointer<port_o>(inPort_o)->set_port_interface((interfaceType == "axis" || interfaceType == "fifo") ?
                                                           port_o::port_interface::PI_FDIN :
                                                           port_o::port_interface::PI_WNONE);
      if(interfaceType == "acknowledge" || interfaceType == "handshake")
      {
         auto inPort_o_ack =
             CM->add_port("_" + arg_name + (IO_port ? "_o" : "") + "_ack", port_o::IN, interface_top, bool_type);
         GetPointer<port_o>(inPort_o_ack)->set_port_interface(port_o::port_interface::PI_WACK);
      }
      if(interfaceType == "valid" || interfaceType == "handshake")
      {
         auto inPort_o_vld =
             CM->add_port("_" + arg_name + (IO_port ? "_o" : "") + "_vld", port_o::OUT, interface_top, bool_type);
         GetPointer<port_o>(inPort_o_vld)->set_port_interface(port_o::port_interface::PI_WVALID);
      }
      if(interfaceType == "fifo")
      {
         auto inPort_full_n = CM->add_port("_" + arg_name + "_full_n", port_o::IN, interface_top, bool_type);
         GetPointer<port_o>(inPort_full_n)->set_port_interface(port_o::port_interface::PI_FULL_N);
         auto inPort_read = CM->add_port("_" + arg_name + "_write", port_o::OUT, interface_top, bool_type);
         GetPointer<port_o>(inPort_read)->set_port_interface(port_o::port_interface::PI_WRITE);
      }
      if(interfaceType == "axis")
      {
         auto inPort_full_n = CM->add_port("_m_axis_" + arg_name + "_TREADY", port_o::IN, interface_top, bool_type);
         GetPointer<port_o>(inPort_full_n)->set_port_interface(port_o::port_interface::PI_M_AXIS_TREADY);
         auto inPort_read = CM->add_port("_m_axis_" + arg_name + "_TVALID", port_o::OUT, interface_top, bool_type);
         GetPointer<port_o>(inPort_read)->set_port_interface(port_o::port_interface::PI_M_AXIS_TVALID);
      }

      CM->add_NP_functionality(interface_top, NP_functionality::LIBRARY, "in1 in2 in3");
      const auto writer = static_cast<HDLWriter_Language>(parameters->getOption<unsigned int>(OPT_writer_language));
      if((interfaceType == "none" || interfaceType == "none_registered") && !(isDiffSize && !isAVH) &&
         writer == HDLWriter_Language::VHDL)
      {
         CM->add_NP_functionality(interface_top, NP_functionality::VHDL_GENERATOR,
                                  "Write_" + interfaceType + ((isDiffSize && !isAVH) ? "DS" : "") + "ModuleGenerator");
      }
      else
      {
         CM->add_NP_functionality(interface_top, NP_functionality::VERILOG_GENERATOR,
                                  "Write_" + interfaceType + ((isDiffSize && !isAVH) ? "DS" : "") + "ModuleGenerator");
      }
      TechMan->add_resource(INTERFACE_LIBRARY, ResourceName, CM);
      for(const auto& fdName : operations)
      {
         TechMan->add_operation(INTERFACE_LIBRARY, ResourceName, fdName);
      }
      auto* fu = GetPointer<functional_unit>(TechMan->get_fu(ResourceName, INTERFACE_LIBRARY));
      const target_deviceRef device = HLS_T->get_target_device();
      fu->area_m = area_model::create_model(device->get_type(), parameters);
      fu->area_m->set_area_value(0);
      if(!isMultipleResource)
      {
         fu->logical_type = functional_unit::COMBINATIONAL;
      }

      for(const auto& fdName : operations)
      {
         auto* op = GetPointer<operation>(fu->get_operation(fdName));
         op->time_m = time_model::create_model(device->get_type(), parameters);
         if(interfaceType == "acknowledge" || interfaceType == "handshake" || interfaceType == "fifo" ||
            interfaceType == "axis")
         {
            op->bounded = false;
            op->time_m->set_execution_time(HLS_T->get_technology_manager()->CGetSetupHoldTime() + EPSILON, 0);
         }
         else if(interfaceType == "none_registered")
         {
            op->bounded = true;
            op->time_m->set_execution_time(EPSILON, 2);
            op->time_m->set_stage_period(HLS_T->get_technology_manager()->CGetSetupHoldTime() + EPSILON);
            const ControlStep ii_cs(1);
            op->time_m->set_initiation_time(ii_cs);
         }
         else
         {
            op->bounded = true;
            op->time_m->set_execution_time(EPSILON, 0);
            op->time_m->set_stage_period(0.0);
         }
         op->time_m->set_synthesis_dependent(true);
      }
      /// add constraint on resource
      HLSMgr->design_interface_constraints[top_id][INTERFACE_LIBRARY][ResourceName] = n_resources;
      INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "<--Interface resource created");
   }
}

void InterfaceInfer::create_resource_array(const std::set<std::string>& operationsR,
                                           const std::set<std::string>& operationsW, const std::string& bundle_name,
                                           const std::string& interfaceType, unsigned long long input_bw,
                                           unsigned int arraySize, unsigned n_resources, unsigned long long alignment,
                                           bool is_real, unsigned long long rwBWsize, unsigned int top_id) const
{
   const auto n_channels = parameters->getOption<unsigned int>(OPT_channels_number);
   const auto isDP = input_bw <= 64 && n_resources == 1 && n_channels == 2;
   const auto NResources = isDP ? 2 : n_resources;
   const auto read_write_string = (isDP ? std::string("ReadWriteDP_") : std::string("ReadWrite_"));
   const auto ResourceName = ENCODE_FDNAME(bundle_name, "", "");
   const auto HLSMgr = GetPointerS<HLS_manager>(AppM);
   const auto HLS_T = HLSMgr->get_HLS_target();
   const auto device_type = HLS_T->get_target_device()->get_type();
   const auto TechMan = HLS_T->get_technology_manager();
   if(!TechMan->is_library_manager(INTERFACE_LIBRARY) ||
      !TechMan->get_library_manager(INTERFACE_LIBRARY)->is_fu(ResourceName))
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level,
                     "-->Creating interface resource: " + INTERFACE_LIBRARY + ":" + ResourceName);
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
      GetPointerS<module>(interface_top)->set_multi_unit_multiplicity(NResources);

      const auto nbitAddres = 32ull - static_cast<unsigned>(__builtin_clzll(arraySize * alignment - 1));
      const auto address_bitsize = HLSMgr->get_address_bitsize();
      const auto nbit = 32ull - static_cast<unsigned>(__builtin_clzll(arraySize - 1));
      const auto nbitDataSize = 32ull - static_cast<unsigned>(__builtin_clzll(rwBWsize));
      const structural_type_descriptorRef addrType(new structural_type_descriptor("bool", address_bitsize));
      const structural_type_descriptorRef address_interface_type(new structural_type_descriptor("bool", nbit));
      const structural_type_descriptorRef dataType(new structural_type_descriptor("bool", input_bw));
      if(is_real)
      {
         dataType->type = structural_type_descriptor::REAL;
      }
      const structural_type_descriptorRef size1(new structural_type_descriptor("bool", 1));
      const structural_type_descriptorRef rwsize(new structural_type_descriptor("bool", nbitDataSize));
      const structural_type_descriptorRef rwtype(new structural_type_descriptor("bool", rwBWsize));
      const structural_type_descriptorRef bool_type(new structural_type_descriptor("bool", 0));

      CM->add_port(CLOCK_PORT_NAME, port_o::IN, interface_top, bool_type);
      CM->add_port(RESET_PORT_NAME, port_o::IN, interface_top, bool_type);
      CM->add_port_vector(START_PORT_NAME, port_o::IN, NResources, interface_top, bool_type);

      CM->add_port_vector("in1", port_o::IN, NResources, interface_top, size1); // when 0 is a read otherwise is a write
      CM->add_port_vector("in2", port_o::IN, NResources, interface_top,
                          rwsize); // bit-width size of the written or read data
      const auto dataPort = CM->add_port_vector("in3", port_o::IN, NResources, interface_top,
                                                rwtype); // value written when the first operand is 1, 0 otherwise
      const auto addrPort = CM->add_port_vector("in4", port_o::IN, NResources, interface_top, addrType); // address
      GetPointerS<port_o>(dataPort)->set_port_alignment(nbitAddres);

      GetPointerS<port_o>(addrPort)->set_is_addr_bus(true);
      // GetPointer<port_o>(addrPort)->set_is_var_args(true); /// required to activate the module generation

      CM->add_port_vector("out1", port_o::OUT, NResources, interface_top, rwtype);

      const auto inPort_address =
          CM->add_port("_" + bundle_name + "_address0", port_o::OUT, interface_top, address_interface_type);
      GetPointerS<port_o>(inPort_address)->set_port_interface(port_o::port_interface::PI_ADDRESS);
      GetPointerS<port_o>(inPort_address)->set_port_alignment(alignment);
      if(isDP)
      {
         const auto inPort_address1 =
             CM->add_port("_" + bundle_name + "_address1", port_o::OUT, interface_top, address_interface_type);
         GetPointerS<port_o>(inPort_address1)->set_port_interface(port_o::port_interface::PI_ADDRESS);
         GetPointerS<port_o>(inPort_address1)->set_port_alignment(alignment);
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
                               read_write_string + interfaceType + "ModuleGenerator");
      TechMan->add_resource(INTERFACE_LIBRARY, ResourceName, CM);
      const auto fu = GetPointerS<functional_unit>(TechMan->get_fu(ResourceName, INTERFACE_LIBRARY));
      fu->area_m = area_model::create_model(device_type, parameters);
      fu->area_m->set_area_value(0);

      /// add constraint on resource
      HLSMgr->design_interface_constraints[top_id][INTERFACE_LIBRARY][ResourceName] = NResources;
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
      op->time_m = time_model::create_model(device_type, parameters);
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
      op->time_m = time_model::create_model(device_type, parameters);
      op->bounded = true;
      op->time_m->set_execution_time(store_delay, store_cycles);
      op->time_m->set_initiation_time(store_ii);
      op->time_m->set_stage_period(store_sp);
      op->time_m->set_synthesis_dependent(true);
   }
}

void InterfaceInfer::create_resource_m_axi(const std::set<std::string>& operationsR,
                                           const std::set<std::string>& operationsW, const std::string& arg_name,
                                           const std::string& bundle_name, const std::string& interfaceType,
                                           unsigned long long input_bw, unsigned long long n_resources, m_axi_type mat,
                                           unsigned long long rwBWsize, unsigned int top_id) const
{
   const auto ResourceName = ENCODE_FDNAME(bundle_name, "", "");
   THROW_ASSERT(GetPointer<HLS_manager>(AppM), "");
   const auto HLSMgr = GetPointerS<HLS_manager>(AppM);
   const auto HLS_T = HLSMgr->get_HLS_target();
   const auto TechMan = HLS_T->get_technology_manager();
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
      GetPointerS<module>(interface_top)->set_multi_unit_multiplicity(n_resources);

      const auto address_bitsize = HLSMgr->get_address_bitsize();
      const auto nbitDataSize = 32ull - static_cast<unsigned>(__builtin_clzll(rwBWsize));
      const structural_type_descriptorRef address_interface_type(
          new structural_type_descriptor("bool", address_bitsize));
      const structural_type_descriptorRef Intype(new structural_type_descriptor("bool", input_bw));
      const structural_type_descriptorRef size1(new structural_type_descriptor("bool", 1));
      const structural_type_descriptorRef rwsize(new structural_type_descriptor("bool", nbitDataSize));
      const structural_type_descriptorRef rwtype(new structural_type_descriptor("bool", rwBWsize));
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
      const structural_type_descriptorRef strbType(new structural_type_descriptor("bool", input_bw / 8));
      const structural_type_descriptorRef respType(new structural_type_descriptor("bool", 2));
      const structural_type_descriptorRef bool_type(new structural_type_descriptor("bool", 0));
      std::string param_ports;

      CM->add_port(CLOCK_PORT_NAME, port_o::IN, interface_top, bool_type);
      CM->add_port(RESET_PORT_NAME, port_o::IN, interface_top, bool_type);
      CM->add_port_vector(START_PORT_NAME, port_o::IN, n_resources, interface_top, bool_type);

      // when 0 is a read otherwise is a write
      CM->add_port_vector("in1", port_o::IN, n_resources, interface_top, size1);
      // bit-width size of the written or read data
      CM->add_port_vector("in2", port_o::IN, n_resources, interface_top, rwsize);
      // value written when the first operand is 1, 0 otherwise
      CM->add_port_vector("in3", port_o::IN, n_resources, interface_top, rwtype);

      const auto addrPort = CM->add_port_vector("in4", port_o::IN, n_resources, interface_top, address_interface_type);
      GetPointerS<port_o>(addrPort)->set_is_addr_bus(true);

      const auto Port_awready =
          CM->add_port("_m_axi_" + bundle_name + "_AWREADY", port_o::IN, interface_top, bool_type);
      GetPointerS<port_o>(Port_awready)->set_port_interface(port_o::port_interface::M_AXI_AWREADY);

      const auto Port_wready = CM->add_port("_m_axi_" + bundle_name + "_WREADY", port_o::IN, interface_top, bool_type);
      GetPointerS<port_o>(Port_wready)->set_port_interface(port_o::port_interface::M_AXI_WREADY);

      const auto Port_bid = CM->add_port_vector("_m_axi_" + bundle_name + "_BID", port_o::IN, 1, interface_top, idType);
      GetPointerS<port_o>(Port_bid)->set_port_interface(port_o::port_interface::M_AXI_BID);

      const auto Port_bresp = CM->add_port("_m_axi_" + bundle_name + "_BRESP", port_o::IN, interface_top, respType);
      GetPointerS<port_o>(Port_bresp)->set_port_interface(port_o::port_interface::M_AXI_BRESP);

      const auto Port_buser =
          CM->add_port_vector("_m_axi_" + bundle_name + "_BUSER", port_o::IN, 1, interface_top, userType);
      GetPointerS<port_o>(Port_buser)->set_port_interface(port_o::port_interface::M_AXI_BUSER);

      const auto Port_bvalid = CM->add_port("_m_axi_" + bundle_name + "_BVALID", port_o::IN, interface_top, bool_type);
      GetPointerS<port_o>(Port_bvalid)->set_port_interface(port_o::port_interface::M_AXI_BVALID);

      const auto Port_arready =
          CM->add_port("_m_axi_" + bundle_name + "_ARREADY", port_o::IN, interface_top, bool_type);
      GetPointerS<port_o>(Port_arready)->set_port_interface(port_o::port_interface::M_AXI_ARREADY);

      const auto Port_rid = CM->add_port_vector("_m_axi_" + bundle_name + "_RID", port_o::IN, 1, interface_top, idType);
      GetPointerS<port_o>(Port_rid)->set_port_interface(port_o::port_interface::M_AXI_RID);

      const auto Port_rdata = CM->add_port("_m_axi_" + bundle_name + "_RDATA", port_o::IN, interface_top, Intype);
      GetPointerS<port_o>(Port_rdata)->set_port_interface(port_o::port_interface::M_AXI_RDATA);

      const auto Port_rresp = CM->add_port("_m_axi_" + bundle_name + "_RRESP", port_o::IN, interface_top, respType);
      GetPointerS<port_o>(Port_rresp)->set_port_interface(port_o::port_interface::M_AXI_RRESP);

      const auto Port_rlast = CM->add_port("_m_axi_" + bundle_name + "_RLAST", port_o::IN, interface_top, bool_type);
      GetPointerS<port_o>(Port_rlast)->set_port_interface(port_o::port_interface::M_AXI_RLAST);

      const auto Port_ruser =
          CM->add_port_vector("_m_axi_" + bundle_name + "_RUSER", port_o::IN, 1, interface_top, userType);
      GetPointerS<port_o>(Port_ruser)->set_port_interface(port_o::port_interface::M_AXI_RUSER);

      const auto Port_rvalid = CM->add_port("_m_axi_" + bundle_name + "_RVALID", port_o::IN, interface_top, bool_type);
      GetPointerS<port_o>(Port_rvalid)->set_port_interface(port_o::port_interface::M_AXI_RVALID);

      CM->add_port_vector(DONE_PORT_NAME, port_o::OUT, n_resources, interface_top, bool_type);
      CM->add_port_vector("out1", port_o::OUT, n_resources, interface_top, rwtype);

      const auto Port_awid =
          CM->add_port_vector("_m_axi_" + bundle_name + "_AWID", port_o::OUT, 1, interface_top, idType);
      GetPointerS<port_o>(Port_awid)->set_port_interface(port_o::port_interface::M_AXI_AWID);

      const auto Port_awaddr =
          CM->add_port("_m_axi_" + bundle_name + "_AWADDR", port_o::OUT, interface_top, address_interface_type);
      GetPointerS<port_o>(Port_awaddr)->set_port_interface(port_o::port_interface::M_AXI_AWADDR);

      const auto Port_awlen = CM->add_port("_m_axi_" + bundle_name + "_AWLEN", port_o::OUT, interface_top, lenType);
      GetPointerS<port_o>(Port_awlen)->set_port_interface(port_o::port_interface::M_AXI_AWLEN);

      const auto Port_awsize = CM->add_port("_m_axi_" + bundle_name + "_AWSIZE", port_o::OUT, interface_top, sizeType);
      GetPointerS<port_o>(Port_awsize)->set_port_interface(port_o::port_interface::M_AXI_AWSIZE);

      const auto Port_awburst =
          CM->add_port("_m_axi_" + bundle_name + "_AWBURST", port_o::OUT, interface_top, burstType);
      GetPointerS<port_o>(Port_awburst)->set_port_interface(port_o::port_interface::M_AXI_AWBURST);

      const auto Port_awlock =
          CM->add_port_vector("_m_axi_" + bundle_name + "_AWLOCK", port_o::OUT, 1, interface_top, lockType);
      GetPointerS<port_o>(Port_awlock)->set_port_interface(port_o::port_interface::M_AXI_AWLOCK);

      const auto Port_awcache =
          CM->add_port("_m_axi_" + bundle_name + "_AWCACHE", port_o::OUT, interface_top, cacheType);
      GetPointerS<port_o>(Port_awcache)->set_port_interface(port_o::port_interface::M_AXI_AWCACHE);

      const auto Port_awprot = CM->add_port("_m_axi_" + bundle_name + "_AWPROT", port_o::OUT, interface_top, protType);
      GetPointerS<port_o>(Port_awprot)->set_port_interface(port_o::port_interface::M_AXI_AWPROT);

      const auto Port_awqos = CM->add_port("_m_axi_" + bundle_name + "_AWQOS", port_o::OUT, interface_top, qosType);
      GetPointerS<port_o>(Port_awqos)->set_port_interface(port_o::port_interface::M_AXI_AWQOS);

      const auto Port_awregion =
          CM->add_port("_m_axi_" + bundle_name + "_AWREGION", port_o::OUT, interface_top, regionType);
      GetPointerS<port_o>(Port_awregion)->set_port_interface(port_o::port_interface::M_AXI_AWREGION);

      const auto Port_awuser =
          CM->add_port_vector("_m_axi_" + bundle_name + "_AWUSER", port_o::OUT, 1, interface_top, userType);
      GetPointerS<port_o>(Port_awuser)->set_port_interface(port_o::port_interface::M_AXI_AWUSER);

      const auto Port_awvalid =
          CM->add_port("_m_axi_" + bundle_name + "_AWVALID", port_o::OUT, interface_top, bool_type);
      GetPointerS<port_o>(Port_awvalid)->set_port_interface(port_o::port_interface::M_AXI_AWVALID);

      const auto Port_wid =
          CM->add_port_vector("_m_axi_" + bundle_name + "_WID", port_o::OUT, 1, interface_top, idType);
      GetPointerS<port_o>(Port_wid)->set_port_interface(port_o::port_interface::M_AXI_WID);

      const auto Port_wdata = CM->add_port("_m_axi_" + bundle_name + "_WDATA", port_o::OUT, interface_top, Intype);
      GetPointerS<port_o>(Port_wdata)->set_port_interface(port_o::port_interface::M_AXI_WDATA);

      const auto Port_wstrb =
          CM->add_port_vector("_m_axi_" + bundle_name + "_WSTRB", port_o::OUT, 1, interface_top, strbType);
      GetPointerS<port_o>(Port_wstrb)->set_port_interface(port_o::port_interface::M_AXI_WSTRB);

      const auto Port_wlast = CM->add_port("_m_axi_" + bundle_name + "_WLAST", port_o::OUT, interface_top, bool_type);
      GetPointerS<port_o>(Port_wlast)->set_port_interface(port_o::port_interface::M_AXI_WLAST);

      const auto Port_wuser =
          CM->add_port_vector("_m_axi_" + bundle_name + "_WUSER", port_o::OUT, 1, interface_top, userType);
      GetPointerS<port_o>(Port_wuser)->set_port_interface(port_o::port_interface::M_AXI_WUSER);

      const auto Port_wvalid = CM->add_port("_m_axi_" + bundle_name + "_WVALID", port_o::OUT, interface_top, bool_type);
      GetPointerS<port_o>(Port_wvalid)->set_port_interface(port_o::port_interface::M_AXI_WVALID);

      const auto Port_bready = CM->add_port("_m_axi_" + bundle_name + "_BREADY", port_o::OUT, interface_top, bool_type);
      GetPointerS<port_o>(Port_bready)->set_port_interface(port_o::port_interface::M_AXI_BREADY);

      const auto Port_arid =
          CM->add_port_vector("_m_axi_" + bundle_name + "_ARID", port_o::OUT, 1, interface_top, idType);
      GetPointerS<port_o>(Port_arid)->set_port_interface(port_o::port_interface::M_AXI_ARID);

      const auto Port_araddr =
          CM->add_port("_m_axi_" + bundle_name + "_ARADDR", port_o::OUT, interface_top, address_interface_type);
      GetPointerS<port_o>(Port_araddr)->set_port_interface(port_o::port_interface::M_AXI_ARADDR);

      const auto Port_arlen = CM->add_port("_m_axi_" + bundle_name + "_ARLEN", port_o::OUT, interface_top, lenType);
      GetPointerS<port_o>(Port_arlen)->set_port_interface(port_o::port_interface::M_AXI_ARLEN);

      const auto Port_arsize = CM->add_port("_m_axi_" + bundle_name + "_ARSIZE", port_o::OUT, interface_top, sizeType);
      GetPointerS<port_o>(Port_arsize)->set_port_interface(port_o::port_interface::M_AXI_ARSIZE);

      const auto Port_arburst =
          CM->add_port("_m_axi_" + bundle_name + "_ARBURST", port_o::OUT, interface_top, burstType);
      GetPointerS<port_o>(Port_arburst)->set_port_interface(port_o::port_interface::M_AXI_ARBURST);

      const auto Port_arlock =
          CM->add_port_vector("_m_axi_" + bundle_name + "_ARLOCK", port_o::OUT, 1, interface_top, lockType);
      GetPointerS<port_o>(Port_arlock)->set_port_interface(port_o::port_interface::M_AXI_ARLOCK);

      const auto Port_arcache =
          CM->add_port("_m_axi_" + bundle_name + "_ARCACHE", port_o::OUT, interface_top, cacheType);
      GetPointerS<port_o>(Port_arcache)->set_port_interface(port_o::port_interface::M_AXI_ARCACHE);

      const auto Port_arprot = CM->add_port("_m_axi_" + bundle_name + "_ARPROT", port_o::OUT, interface_top, protType);
      GetPointerS<port_o>(Port_arprot)->set_port_interface(port_o::port_interface::M_AXI_ARPROT);

      const auto Port_arqos = CM->add_port("_m_axi_" + bundle_name + "_ARQOS", port_o::OUT, interface_top, qosType);
      GetPointerS<port_o>(Port_arqos)->set_port_interface(port_o::port_interface::M_AXI_ARQOS);

      const auto Port_arregion =
          CM->add_port("_m_axi_" + bundle_name + "_ARREGION", port_o::OUT, interface_top, regionType);
      GetPointerS<port_o>(Port_arregion)->set_port_interface(port_o::port_interface::M_AXI_ARREGION);

      const auto Port_aruser =
          CM->add_port_vector("_m_axi_" + bundle_name + "_ARUSER", port_o::OUT, 1, interface_top, userType);
      GetPointerS<port_o>(Port_aruser)->set_port_interface(port_o::port_interface::M_AXI_ARUSER);

      const auto Port_arvalid =
          CM->add_port("_m_axi_" + bundle_name + "_ARVALID", port_o::OUT, interface_top, bool_type);
      GetPointerS<port_o>(Port_arvalid)->set_port_interface(port_o::port_interface::M_AXI_ARVALID);

      const auto Port_rready = CM->add_port("_m_axi_" + bundle_name + "_RREADY", port_o::OUT, interface_top, bool_type);
      GetPointerS<port_o>(Port_rready)->set_port_interface(port_o::port_interface::M_AXI_RREADY);

      if(mat == m_axi_type::axi_slave)
      {
         const auto Port_LSawvalid = CM->add_port("_s_axi_AXILiteS_AWVALID", port_o::IN, interface_top, bool_type);
         GetPointerS<port_o>(Port_LSawvalid)->set_port_interface(port_o::port_interface::S_AXIL_AWVALID);
         CM->add_port("_s_axi_AXILiteS_AWREADY", port_o::OUT, interface_top, bool_type);
         GetPointerS<port_o>(Port_LSawvalid)->set_port_interface(port_o::port_interface::S_AXIL_AWREADY);
         CM->add_port("_s_axi_AXILiteS_AWADDR", port_o::IN, interface_top, address_interface_type);
         GetPointerS<port_o>(Port_LSawvalid)->set_port_interface(port_o::port_interface::S_AXIL_AWADDR);
         CM->add_port("_s_axi_AXILiteS_WVALID", port_o::IN, interface_top, bool_type);
         CM->add_port("_s_axi_AXILiteS_WREADY", port_o::OUT, interface_top, bool_type);
         CM->add_port("_s_axi_AXILiteS_WDATA", port_o::IN, interface_top, Intype);
         CM->add_port("_s_axi_AXILiteS_WSTRB", port_o::IN, interface_top, strbType);
         CM->add_port("_s_axi_AXILiteS_ARVALID", port_o::IN, interface_top, bool_type);
         CM->add_port("_s_axi_AXILiteS_ARREADY", port_o::OUT, interface_top, bool_type);
         CM->add_port("_s_axi_AXILiteS_ARADDR", port_o::IN, interface_top, address_interface_type);
         CM->add_port("_s_axi_AXILiteS_RVALID", port_o::OUT, interface_top, bool_type);
         CM->add_port("_s_axi_AXILiteS_RREADY", port_o::IN, interface_top, bool_type);
         CM->add_port("_s_axi_AXILiteS_RDATA", port_o::OUT, interface_top, Intype);
         CM->add_port("_s_axi_AXILiteS_RRESP", port_o::OUT, interface_top, respType);
         CM->add_port("_s_axi_AXILiteS_BVALID", port_o::OUT, interface_top, bool_type);
         CM->add_port("_s_axi_AXILiteS_BREADY", port_o::IN, interface_top, bool_type);
         CM->add_port("_s_axi_AXILiteS_BRESP", port_o::OUT, interface_top, respType);
      }

      CM->add_NP_functionality(interface_top, NP_functionality::LIBRARY, "in1 in2 in3 in4 out1" + param_ports);
      CM->add_NP_functionality(interface_top, NP_functionality::VERILOG_GENERATOR,
                               "ReadWrite_" + interfaceType + "ModuleGenerator");
      TechMan->add_resource(INTERFACE_LIBRARY, ResourceName, CM);

      const auto fu = GetPointerS<functional_unit>(TechMan->get_fu(ResourceName, INTERFACE_LIBRARY));
      const auto device = HLS_T->get_target_device();
      fu->area_m = area_model::create_model(device->get_type(), parameters);
      fu->area_m->set_area_value(0);

      /// add constraint on resource
      HLSMgr->design_interface_constraints[top_id][INTERFACE_LIBRARY][ResourceName] = n_resources;
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
   const auto device = HLS_T->get_target_device();

   for(const auto& fdName : operationsR)
   {
      const auto op = GetPointerS<operation>(fu->get_operation(fdName));
      op->time_m = time_model::create_model(device->get_type(), parameters);
      op->bounded = false;
      op->time_m->set_execution_time(HLS_T->get_technology_manager()->CGetSetupHoldTime() + EPSILON, 0);
      op->time_m->set_synthesis_dependent(true);
   }
   for(const auto& fdName : operationsW)
   {
      const auto op = GetPointer<operation>(fu->get_operation(fdName));
      op->time_m = time_model::create_model(device->get_type(), parameters);
      op->bounded = false;
      op->time_m->set_execution_time(HLS_T->get_technology_manager()->CGetSetupHoldTime() + EPSILON, 0);
      op->time_m->set_synthesis_dependent(true);
   }
   const auto address_bitsize = HLSMgr->get_address_bitsize();
   const structural_type_descriptorRef address_interface_type(new structural_type_descriptor("bool", address_bitsize));
   const auto interface_top = fu->CM->get_circ();
   const auto inPort_m_axi = fu->CM->add_port("_" + arg_name, port_o::IN, interface_top, address_interface_type);
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
                                     const std::string& arg_name, const std::string& interfaceType,
                                     unsigned long long input_bw, bool isDiffSize, const std::string& fname,
                                     unsigned long long n_resources, unsigned long long alignment, bool isReal,
                                     unsigned long long rwBWsize, unsigned int top_id) const
{
   if(interfaceType == "none" || interfaceType == "none_registered" || interfaceType == "acknowledge" ||
      interfaceType == "valid" || interfaceType == "ovalid" || interfaceType == "handshake" ||
      interfaceType == "fifo" || interfaceType == "axis")
   {
      THROW_ASSERT(!operationsR.empty() || !operationsW.empty(), "unexpected condition");
      bool IO_P = !operationsR.empty() && !operationsW.empty();
      if(!operationsR.empty())
      {
         create_resource_Read_simple(operationsR, arg_name, (interfaceType == "ovalid" ? "none" : interfaceType),
                                     input_bw, IO_P, n_resources, rwBWsize, top_id);
      }
      if(!operationsW.empty())
      {
         create_resource_Write_simple(operationsW, arg_name, (interfaceType == "ovalid" ? "valid" : interfaceType),
                                      input_bw, IO_P, isDiffSize, n_resources, isReal, rwBWsize, top_id);
      }
   }
   else if(interfaceType == "array")
   {
      const auto HLSMgr = GetPointer<HLS_manager>(AppM);
      THROW_ASSERT(HLSMgr->design_interface_arraysize.find(fname) != HLSMgr->design_interface_arraysize.end() &&
                       HLSMgr->design_interface_arraysize.find(fname)->second.find(arg_name) !=
                           HLSMgr->design_interface_arraysize.find(fname)->second.end(),
                   "unexpected condition");
      const auto arraySizeSTR = HLSMgr->design_interface_arraysize.at(fname).at(arg_name);
      const auto arraySize = boost::lexical_cast<unsigned>(arraySizeSTR);
      if(arraySize == 0)
      {
         THROW_ERROR("array size equal to zero");
      }

      auto bundle_name = arg_name;
      if(HLSMgr->design_interface_attribute3.find(fname) != HLSMgr->design_interface_attribute3.end() &&
         HLSMgr->design_interface_attribute3.at(fname).find(arg_name) !=
             HLSMgr->design_interface_attribute3.at(fname).end())
      {
         bundle_name = HLSMgr->design_interface_attribute3.at(fname).at(arg_name);
      }

      create_resource_array(operationsR, operationsW, bundle_name, interfaceType, input_bw, arraySize, n_resources,
                            alignment, isReal, rwBWsize, top_id);
   }
   else if(interfaceType == "m_axi")
   {
      auto mat = m_axi_type::none;
      const auto HLSMgr = GetPointerS<HLS_manager>(AppM);
      auto bundle_name = arg_name;

      if(HLSMgr->design_interface_attribute2.find(fname) != HLSMgr->design_interface_attribute2.end() &&
         HLSMgr->design_interface_attribute2.at(fname).find(arg_name) !=
             HLSMgr->design_interface_attribute2.at(fname).end())
      {
         const auto& matString = HLSMgr->design_interface_attribute2.at(fname).at(arg_name);
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
      if(HLSMgr->design_interface_attribute3.find(fname) != HLSMgr->design_interface_attribute3.end() &&
         HLSMgr->design_interface_attribute3.at(fname).find(arg_name) !=
             HLSMgr->design_interface_attribute3.at(fname).end())
      {
         bundle_name = HLSMgr->design_interface_attribute3.at(fname).at(arg_name);
      }
      create_resource_m_axi(operationsR, operationsW, arg_name, bundle_name, interfaceType, input_bw, n_resources, mat,
                            rwBWsize, top_id);
   }
   else
   {
      THROW_ERROR("interface not supported: " + interfaceType);
   }
}

void InterfaceInfer::ComputeResourcesAlignment(unsigned long long& n_resources, unsigned long long& alignment,
                                               unsigned long long input_bw, bool is_acType, bool is_signed,
                                               bool is_fixed)
{
   n_resources = 1;
   if(input_bw > 64 && input_bw <= 128)
   {
      n_resources = 2;
   }
   else if(input_bw > 128)
   {
      n_resources = input_bw / 32 + (input_bw % 32 ? 1 : 0);
      if(!is_signed && input_bw % 32 == 0 && !is_fixed)
      {
         ++n_resources;
      }
   }
   /// compute alignment
   alignment = 1;
   if(input_bw <= 8)
   {
      alignment = !is_acType ? 1 : 4;
   }
   else if(input_bw <= 16)
   {
      alignment = !is_acType ? 2 : 4;
   }
   else if(input_bw <= 32)
   {
      alignment = 4;
   }
   else if(input_bw <= 64)
   {
      alignment = 8;
   }
   else if(input_bw <= 128)
   {
      alignment = 16;
   }
   else
   {
      alignment = (input_bw / 32) + 4 * (input_bw % 32 ? 1 : 0);
      if(!is_signed && input_bw % 32 == 0 && !is_fixed)
      {
         alignment += 4;
      }
   }
}

void InterfaceInfer::FixReadWriteCall(const gimple_assign* ga, gimple_node* newGN, const tree_manipulationRef tree_man,
                                      tree_nodeRef new_call, statement_list* sl, const tree_managerRef TM,
                                      tree_nodeRef origStmt, unsigned int destBB, const std::string& fname,
                                      const std::string& arg_name)
{
   newGN->memdef = ga->memdef;
   newGN->memuse = ga->memuse;
   if(ga->vdef)
   {
      auto ssaVDefVar = GetPointer<ssa_name>(GET_NODE(ga->vdef));
      THROW_ASSERT(ssaVDefVar, "unexpected condition");
      auto newSSAVdef =
          tree_man->create_ssa_name(ssaVDefVar->var, ssaVDefVar->type, tree_nodeRef(), tree_nodeRef(), false, true);
      newGN->vdef = newSSAVdef;
      GetPointerS<ssa_name>(GET_NODE(newGN->vdef))->SetDefStmt(new_call);
      const auto StmtVdefUses = ssaVDefVar->CGetUseStmts();
      for(const auto& used : StmtVdefUses)
      {
         TM->ReplaceTreeNode(used.first, ga->vdef, newSSAVdef);
      }
   }
   const auto StmtVusesUses = ga->vuses;
   for(const auto& vUse : StmtVusesUses)
   {
      auto sn = GetPointer<ssa_name>(GET_NODE(vUse));
      if(newGN->AddVuse(vUse))
      {
         sn->AddUseStmt(new_call);
      }
   }
   for(const auto& vOver : ga->vovers)
   {
      auto sn = GetPointer<ssa_name>(GET_NODE(vOver));
      if(newGN->AddVover(vOver))
      {
         sn->AddUseStmt(new_call);
      }
   }
   sl->list_of_bloc[destBB]->RemoveStmt(origStmt, AppM);
   GetPointer<HLS_manager>(AppM)->design_interface_loads[fname][destBB][arg_name].push_back(GET_INDEX_NODE(new_call));
}
