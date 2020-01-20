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
 *              Copyright (C) 2015-2020 Politecnico di Milano
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
 * @file hdl_function_decl_fix.cpp
 * @brief Pre-analysis step fixing names of functions which clash with signal names
 *
 * @author Marco Lattuada <marco.lattuada@polimi.it>
 *
 */

/// Header include
#include "hdl_function_decl_fix.hpp"

///. include
#include "Parameter.hpp"

/// behavior include
#include "application_manager.hpp"

/// design_flows/backend/ToHDL include
#include "language_writer.hpp"

/// HLS includes
#include "hls_manager.hpp"
#include "hls_target.hpp"

/// parser/treegcc include
#include "token_interface.hpp"

/// tree includes
#include "tree_manager.hpp"
#include "tree_node.hpp"
#include "tree_reindex.hpp"

#include "string_manipulation.hpp" // for GET_CLASS

HDLFunctionDeclFix::HDLFunctionDeclFix(const application_managerRef _AppM, const DesignFlowManagerConstRef _design_flow_manager, const ParameterConstRef _parameters)
    : ApplicationFrontendFlowStep(_AppM, HDL_FUNCTION_DECL_FIX, _design_flow_manager, _parameters)
{
   debug_level = parameters->get_class_debug_level(GET_CLASS(*this));
}

HDLFunctionDeclFix::~HDLFunctionDeclFix() = default;

DesignFlowStep_Status HDLFunctionDeclFix::Exec()
{
   bool changed_tree = false;
   const tree_managerRef TM = AppM->get_tree_manager();
   const auto hdl_writer_type = static_cast<HDLWriter_Language>(parameters->getOption<unsigned int>(OPT_writer_language));
   const auto hdl_writer = language_writer::create_writer(hdl_writer_type, GetPointer<HLS_manager>(AppM)->get_HLS_target()->get_technology_manager(), parameters);
   const auto hdl_reserved_names = hdl_writer->GetHDLReservedNames();
   std::remove_const<decltype(hdl_reserved_names)>::type found_names;
   if(hdl_writer_type == HDLWriter_Language::VHDL)
   {
      for(const auto& hdl_reserved_name : hdl_reserved_names)
      {
         found_names.insert(boost::to_upper_copy<std::string>(hdl_reserved_name));
      }
   }
   else
   {
      found_names = hdl_reserved_names;
   }

   for(const auto function : TM->GetAllFunctions())
   {
      auto fd = GetPointer<function_decl>(TM->get_tree_node_const(function));
      if(not fd->name)
         continue;
      auto in = GetPointer<identifier_node>(GET_NODE(fd->name));
      const auto identifier = hdl_writer_type == HDLWriter_Language::VHDL ? boost::to_upper_copy<std::string>(in->strg) : in->strg;
      if(found_names.find(identifier) != found_names.end())
      {
         std::map<TreeVocabularyTokenTypes_TokenEnum, std::string> IR_schema;
         unsigned int var_decl_name_nid_test;
         unsigned var_decl_unique_id = 0;
         do
         {
            IR_schema[TOK(TOK_STRG)] = in->strg + STR(var_decl_unique_id++);
            var_decl_name_nid_test = TM->find(identifier_node_K, IR_schema);
         } while(var_decl_name_nid_test);
         found_names.insert(in->strg + STR(var_decl_unique_id - 1));
         unsigned int var_decl_name_nid = TM->new_tree_node_id();
         TM->create_tree_node(var_decl_name_nid, identifier_node_K, IR_schema);
         IR_schema.clear();
         tree_nodeRef tr_new_id = TM->GetTreeReindex(var_decl_name_nid);
         fd->name = tr_new_id;
      }
      else
      {
         found_names.insert(identifier);
      }
   }
   return changed_tree ? DesignFlowStep_Status::SUCCESS : DesignFlowStep_Status::UNCHANGED;
}

const CustomUnorderedSet<std::pair<FrontendFlowStepType, FrontendFlowStep::FunctionRelationship>> HDLFunctionDeclFix::ComputeFrontendRelationships(const DesignFlowStep::RelationshipType relationship_type) const
{
   CustomUnorderedSet<std::pair<FrontendFlowStepType, FunctionRelationship>> relationships;
   switch(relationship_type)
   {
      case(DEPENDENCE_RELATIONSHIP):
      {
         relationships.insert(std::pair<FrontendFlowStepType, FunctionRelationship>(CREATE_TREE_MANAGER, WHOLE_APPLICATION));
         break;
      }
      case(INVALIDATION_RELATIONSHIP):
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
