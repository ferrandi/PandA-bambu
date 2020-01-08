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
 *              Copyright (C) 2018-2019 Politecnico di Milano
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
 * @file pipeline_infer.cpp
 * @brief Recognize pipelining requirements set in the source file
 *
 * @author Luca Ezio Pozzoni <lucaezio.pozzoni@mail.polimi.it>
 *
 */

// include header class
#include "pipeline_infer.hpp"

// include from src/
#include "Parameter.hpp"

// XML includes used for writing and reading the configuration file
#include "polixml.hpp"
#include "xml_dom_parser.hpp"

#include "dbgPrintHelper.hpp"      // for DEBUG_LEVEL_
#include "string_manipulation.hpp" // for GET_CLASS

#include "function_behavior.hpp"
#include "hls_manager.hpp"
#include "tree_manager.hpp"

pipelineInfer::pipelineInfer(const application_managerRef _AppM, const DesignFlowManagerConstRef _design_flow_manager, const ParameterConstRef _parameters) : ApplicationFrontendFlowStep(_AppM, PIPELINE_INFER, _design_flow_manager, _parameters)
{
   debug_level = parameters->get_class_debug_level(GET_CLASS(*this));
}

pipelineInfer::~pipelineInfer() = default;

const CustomUnorderedSet<std::pair<FrontendFlowStepType, FrontendFlowStep::FunctionRelationship>> pipelineInfer::ComputeFrontendRelationships(const DesignFlowStep::RelationshipType relationship_type) const
{
   CustomUnorderedSet<std::pair<FrontendFlowStepType, FunctionRelationship>> relationships;
   switch(relationship_type)
   {
      case(DEPENDENCE_RELATIONSHIP):
      {
         relationships.insert(std::pair<FrontendFlowStepType, FrontendFlowStep::FunctionRelationship>(CREATE_TREE_MANAGER, WHOLE_APPLICATION));
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

DesignFlowStep_Status pipelineInfer::Exec()
{
   if(GetPointer<HLS_manager>(AppM))
   {
      auto HLSMgr = GetPointer<HLS_manager>(AppM);
      const auto TM = AppM->get_tree_manager();
      for(auto source_file : AppM->input_files)
      {
         const std::string output_temporary_directory = parameters->getOption<std::string>(OPT_output_temporary_directory);
         std::string leaf_name = source_file.second == "-" ? "stdin-" : GetLeafFileName(source_file.second);
         auto XMLfilename = output_temporary_directory + "/" + leaf_name + ".pipeline.xml";
         if((boost::filesystem::exists(boost::filesystem::path(XMLfilename))))
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->parsing " + XMLfilename);
            XMLDomParser parser(XMLfilename);
            parser.Exec();
            if(parser)
            {
               const xml_element* node = parser.get_document()->get_root_node();
               for(const auto& iter : node->get_children())
               {
                  const auto* Enode = GetPointer<const xml_element>(iter);
                  if(!Enode)
                     continue;
                  if(Enode->get_name() == "function")
                  {
                     std::string fname;
                     std::string is_pipelined = "null";
                     for(auto attr : Enode->get_attributes())
                     {
                        std::string key = attr->get_name();
                        std::string value = attr->get_value();
                        if(key == "id")
                           fname = value;
                        if(key == "is_pipelined")
                           is_pipelined = value;
                     }
                     if(fname == "")
                        THROW_ERROR("malformed pipeline infer file");
                     if(is_pipelined.compare("yes") && is_pipelined.compare("no"))
                        THROW_ERROR("malformed pipeline infer file");
                     for(const auto& iterArg : Enode->get_children())
                     {
                        const auto* EnodeArg = GetPointer<const xml_element>(iterArg);
                        if(EnodeArg)
                           THROW_ERROR("malformed pipeline infer file");
                     }
                     auto findex = TM->function_index(fname);
                     std::cout << "The function " << fname << " has parameter is_pipelined=\"" << is_pipelined << "\"\n";
                     std::cout << "Tree retrieved index for the function is " << std::to_string(findex) << "\n\n";
                     if(is_pipelined.compare("yes"))
                        HLSMgr->GetFunctionBehavior(findex)->set_pipelining_enabled(true);
                     else
                        HLSMgr->GetFunctionBehavior(findex)->set_pipelining_enabled(false);
                  }
               }
            }
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--parsed file " + XMLfilename);
         }
      }
      return DesignFlowStep_Status::SUCCESS;
   }
   THROW_ERROR("AppM is not an HLS_manager");
   return DesignFlowStep_Status::ABORTED;
}
