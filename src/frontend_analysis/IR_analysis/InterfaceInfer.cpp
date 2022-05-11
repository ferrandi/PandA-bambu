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

#include "application_manager.hpp"
#include "hls_manager.hpp"
#include "hls_step.hpp" // for HLSFlowStep_Type

/// parser/compiler include
#include "token_interface.hpp"

/// XML includes used for writing and reading the configuration file
#include "polixml.hpp"
#include "xml_dom_parser.hpp"
#include "xml_helper.hpp"

#include "dbgPrintHelper.hpp" // for DEBUG_LEVEL_
#include "string_manipulation.hpp"

InterfaceInfer::InterfaceInfer(const application_managerRef _AppM, const DesignFlowManagerConstRef _design_flow_manager,
                               const ParameterConstRef _parameters)
    : ApplicationFrontendFlowStep(_AppM, INTERFACE_INFER, _design_flow_manager, _parameters)
{
   debug_level = parameters->get_class_debug_level(GET_CLASS(*this));

   const auto HLSMgr = GetPointer<HLS_manager>(AppM);
   THROW_ASSERT(HLSMgr, "");
   const auto parseInterfaceXML = [&](const std::string& XMLfilename) {
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

InterfaceInfer::~InterfaceInfer() = default;

const CustomUnorderedSet<std::pair<FrontendFlowStepType, FrontendFlowStep::FunctionRelationship>>
InterfaceInfer::ComputeFrontendRelationships(const DesignFlowStep::RelationshipType /* relationship_type */) const
{
   CustomUnorderedSet<std::pair<FrontendFlowStepType, FunctionRelationship>> relationships;
   return relationships;
}

DesignFlowStep_Status InterfaceInfer::Exec()
{
   return DesignFlowStep_Status::UNCHANGED;
}
