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
 *              Copyright (c) 2018 Politecnico di Milano
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
 * @file interface_infer.cpp
 * @brief Restructure the top level function to adhere the specified interface.
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 *
 */

/// Header include
#include "interface_infer.hpp"

///. include
#include "Parameter.hpp"

/// design_flows includes
#include "design_flow_graph.hpp"
#include "design_flow_manager.hpp"

/// design_flows/technology includes
#include "technology_flow_step.hpp"
#include "technology_flow_step_factory.hpp"

/// behavior include
#include "application_manager.hpp"
#include "behavioral_helper.hpp"
#include "call_graph.hpp"
#include "call_graph_manager.hpp"
#include "function_behavior.hpp"
#include "hls_manager.hpp"
#include "hls_target.hpp"

/// parser/treegcc include
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

#define EPSILON 0.000000001
#define ENCODE_FDNAME(argName_string,MODE,interfaceType) argName_string + STR_CST_interface_parameter_keyword + MODE + interfaceType

const std::unordered_set<std::pair<FrontendFlowStepType, FrontendFlowStep::FunctionRelationship>> interface_infer::ComputeFrontendRelationships(const DesignFlowStep::RelationshipType relationship_type) const
{
   std::unordered_set<std::pair<FrontendFlowStepType, FunctionRelationship>> relationships;
   switch (relationship_type)
   {
      case (DEPENDENCE_RELATIONSHIP):
      {
         relationships.insert(std::make_pair(IR_LOWERING, SAME_FUNCTION));
         relationships.insert(std::make_pair(USE_COUNTING, SAME_FUNCTION));
         break;
      }
      case (INVALIDATION_RELATIONSHIP):
      {
         break;
      }
      case (PRECEDENCE_RELATIONSHIP):
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

void interface_infer::ComputeRelationships(DesignFlowStepSet& relationship, const DesignFlowStep::RelationshipType relationship_type)
{
   switch (relationship_type)
   {
      case (PRECEDENCE_RELATIONSHIP):
      {
         break;
      }
      case DEPENDENCE_RELATIONSHIP:
      {
         const DesignFlowGraphConstRef design_flow_graph = design_flow_manager.lock()->CGetDesignFlowGraph();
         const auto* technology_flow_step_factory = GetPointer<const TechnologyFlowStepFactory>(design_flow_manager.lock()->CGetDesignFlowStepFactory("Technology"));
         const std::string technology_flow_signature = TechnologyFlowStep::ComputeSignature(TechnologyFlowStep_Type::LOAD_TECHNOLOGY);
         const vertex technology_flow_step = design_flow_manager.lock()->GetDesignFlowStep(technology_flow_signature);
         const DesignFlowStepRef technology_design_flow_step =
             technology_flow_step ? design_flow_graph->CGetDesignFlowStepInfo(technology_flow_step)->design_flow_step : technology_flow_step_factory->CreateTechnologyFlowStep(TechnologyFlowStep_Type::LOAD_TECHNOLOGY);
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
   FunctionFrontendFlowStep::ComputeRelationships(relationship, relationship_type);
}

interface_infer::interface_infer(const application_managerRef _AppM, unsigned int _function_id, const DesignFlowManagerConstRef _design_flow_manager, const ParameterConstRef _parameters)
    : FunctionFrontendFlowStep(_AppM, _function_id, INTERFACE_INFER, _design_flow_manager, _parameters)
{
   debug_level = parameters->get_class_debug_level(GET_CLASS(*this), DEBUG_LEVEL_NONE);
}

interface_infer::~interface_infer() = default;

void interface_infer::classifyArgRecurse(std::set<unsigned>&Visited, ssa_name*argSSA, unsigned int destBB, statement_list*sl, bool &canBeMovedToBB2, bool &isRead, bool &isWrite, bool &unkwown_pattern, std::list<tree_nodeRef> &writeStmt, std::list<tree_nodeRef> &readStmt)
{
   tree_nodeRef readType;
   for (auto par_use : argSSA->CGetUseStmts())
   {
      auto use_stmt = GET_NODE(par_use.first);
      if(Visited.find(GET_INDEX_NODE(par_use.first)) != Visited.end()) continue;
      Visited.insert(GET_INDEX_NODE(par_use.first));
      std::cerr << "STMT: " << use_stmt->ToString() << std::endl;
      auto gn = GetPointer<gimple_node>(use_stmt);
      if (auto ga = GetPointer<gimple_assign>(use_stmt))
      {
         if (GET_NODE(ga->op0)->get_kind() == mem_ref_K)
         {
            isWrite = true;
            writeStmt.push_back(par_use.first);
            if (GET_NODE(ga->op1)->get_kind() == mem_ref_K)
            {
               unkwown_pattern = true;
               THROW_WARNING("Pattern currently not supported: *x=*y; " + use_stmt->ToString());
            }
         }
         else if (GET_NODE(ga->op1)->get_kind() == mem_ref_K)
         {
            if (sl->list_of_bloc.find(gn->bb_index)->second->loop_id != 0) canBeMovedToBB2 = false;
            isRead = true;
            if (readType && GET_INDEX_NODE(GetPointer<mem_ref>(GET_NODE(ga->op1))->type) != GET_INDEX_NODE(readType)) canBeMovedToBB2 = false; /// reading different objects
            readType = GetPointer<mem_ref>(GET_NODE(ga->op1))->type;
            readStmt.push_back(par_use.first);
            if (ga->vdef)
            {
               unkwown_pattern = true;
               canBeMovedToBB2 = false;
               THROW_WARNING("Pattern currently not supported: use of a volatile load " + use_stmt->ToString());
            }
            if(ga->bb_index == destBB)
               canBeMovedToBB2 = false;
         }
         else if (GET_NODE(ga->op1)->get_kind() == call_expr_K)
         {
            unkwown_pattern = true;
            canBeMovedToBB2 = false;
            THROW_WARNING("Pattern currently not supported: parameter passed as a parameter to another function " + use_stmt->ToString());
         }
         else if(GET_NODE(ga->op1)->get_kind() == nop_expr_K || GET_NODE(ga->op1)->get_kind() == ssa_name_K)
         {
            canBeMovedToBB2 = false;
            auto op0SSA = GetPointer<ssa_name>(GET_NODE(ga->op0));
            THROW_ASSERT(argSSA, "unexpected condition");
            classifyArgRecurse(Visited, op0SSA, destBB, sl, canBeMovedToBB2, isRead, isWrite, unkwown_pattern, writeStmt, readStmt);
         }
         else
            THROW_ERROR("Pattern currently not supported: parameter used in a non-supported statement " + use_stmt->ToString() + ":" + GET_NODE(ga->op1)->get_kind_text());
      }
      else
      {
         unkwown_pattern = true;
         canBeMovedToBB2 = false;
         THROW_WARNING("USE PATTERN unexpected" + use_stmt->ToString());
      }
   }
}
void interface_infer::classifyArg(statement_list*sl, tree_nodeRef argSSANode, bool &canBeMovedToBB2, bool &isRead, bool &isWrite, bool &unkwown_pattern, std::list<tree_nodeRef> &writeStmt, std::list<tree_nodeRef> &readStmt)
{
   unsigned int destBB = bloc::ENTRY_BLOCK_ID;
   for (auto bb_succ : sl->list_of_bloc[bloc::ENTRY_BLOCK_ID]->list_of_succ)
   {
      if (bb_succ == bloc::EXIT_BLOCK_ID) continue;
      if (destBB == bloc::ENTRY_BLOCK_ID)
         destBB = bb_succ;
      else
         THROW_ERROR("unexpected pattern");
   }
   THROW_ASSERT(destBB != bloc::ENTRY_BLOCK_ID, "unexpected condition");
   auto argSSA = GetPointer<ssa_name>(GET_NODE(argSSANode));
   THROW_ASSERT(argSSA, "unexpected condition");
   std::set<unsigned>Visited;
   classifyArgRecurse(Visited, argSSA, destBB, sl, canBeMovedToBB2, isRead, isWrite, unkwown_pattern, writeStmt, readStmt);
}

void interface_infer::create_Read_function(tree_nodeRef origStmt, unsigned int destBB, statement_list*sl, function_decl* fd, const std::string &fdName, tree_nodeRef argSSANode, parm_decl* a, tree_nodeRef readType, const std::list<tree_nodeRef> &usedStmt_defs, const tree_manipulationRef tree_man, const tree_managerRef TM)
{
   /// create the function_decl
   std::vector<tree_nodeRef> argsT;
   argsT.push_back(a->type);
   const std::string srcp = fd->include_name + ":" + STR(fd->line_number) + ":" + STR(fd->column_number);
   auto function_decl_node = tree_man->create_function_decl(fdName, fd->scpe, argsT, readType, srcp, false);

   std::vector<tree_nodeRef> args;
   if(origStmt)
   {
      THROW_ASSERT(GET_NODE(origStmt)->get_kind()==gimple_assign_K, "unexpected condition");
      auto ga = GetPointer<gimple_assign>(GET_NODE(origStmt));
      THROW_ASSERT(GET_NODE(ga->op1)->get_kind()==mem_ref_K, "unexpected condition");
      auto mr = GetPointer<mem_ref>(GET_NODE(ga->op1));
      args.push_back(mr->op0);
   }
   else
      args.push_back(argSSANode);
   auto call_expr_node = tree_man->CreateCallExpr(function_decl_node, args, srcp);
   auto new_assignment = tree_man->CreateGimpleAssign(readType, call_expr_node, destBB, srcp);
   tree_nodeRef temp_ssa_var = GetPointer<gimple_assign>(GET_NODE(new_assignment))->op0;
   for (auto defSSA : usedStmt_defs)
   {
      auto ssaDefVar = GetPointer<ssa_name>(GET_NODE(defSSA));
      THROW_ASSERT(ssaDefVar, "unexpected condition");
      std::list<tree_nodeRef> varUses;
      for (auto used : ssaDefVar->CGetUseStmts()) { varUses.push_back(used.first); }
      for (auto used : varUses) { TM->ReplaceTreeNode(used, defSSA, temp_ssa_var); }
   }
   if(origStmt)
      sl->list_of_bloc[destBB]->PushBefore(new_assignment, origStmt);
   else
      sl->list_of_bloc[destBB]->PushBack(new_assignment);
   BehavioralHelperRef helper = BehavioralHelperRef(new BehavioralHelper(AppM, GET_INDEX_NODE(function_decl_node), false, parameters));
   FunctionBehaviorRef FB = FunctionBehaviorRef(new FunctionBehavior(AppM, helper, parameters));
   AppM->GetCallGraphManager()->AddFunctionAndCallPoint(function_id, GET_INDEX_NODE(function_decl_node), new_assignment->index, FB, FunctionEdgeInfo::CallType::direct_call);
}

void interface_infer::create_Write_function(tree_nodeRef origStmt, unsigned int destBB, statement_list*sl, function_decl* fd, const std::string &fdName, tree_nodeRef argSSANode, tree_nodeRef writeValue, parm_decl* a, tree_nodeRef writeType, const tree_manipulationRef tree_man, const tree_managerRef TM)
{
   const auto size_value_id = TM->new_tree_node_id();
   const auto bit_size_type = tree_man->create_bit_size_type();
   const auto size_value = tree_man->CreateIntegerCst(bit_size_type, tree_helper::Size(writeType), size_value_id);

   /// create the function_decl
   std::vector<tree_nodeRef> argsT;
   argsT.push_back(bit_size_type);
   argsT.push_back(writeType);
   argsT.push_back(a->type);
   const std::string srcp = fd->include_name + ":" + STR(fd->line_number) + ":" + STR(fd->column_number);
   auto function_decl_node = tree_man->create_function_decl(fdName, fd->scpe, argsT, tree_man->create_void_type(), srcp, false);

   std::vector<tree_nodeRef> args;
   args.push_back(size_value);
   args.push_back(writeValue);
   if(origStmt)
   {
      THROW_ASSERT(GET_NODE(origStmt)->get_kind()==gimple_assign_K, "unexpected condition");
      auto ga = GetPointer<gimple_assign>(GET_NODE(origStmt));
      THROW_ASSERT(GET_NODE(ga->op0)->get_kind()==mem_ref_K, "unexpected condition");
      auto mr = GetPointer<mem_ref>(GET_NODE(ga->op0));
      args.push_back(mr->op0);
   }
   else
      args.push_back(argSSANode);
   tree_nodeRef new_readwritecall;

   new_readwritecall = tree_man->create_gimple_call(function_decl_node, args, srcp, destBB);

   THROW_ASSERT(origStmt, "unexpected condition");
   sl->list_of_bloc[destBB]->PushBefore(new_readwritecall, origStmt);
   addGimpleNOPxVirtual(origStmt,sl,TM);
   BehavioralHelperRef helper = BehavioralHelperRef(new BehavioralHelper(AppM, GET_INDEX_NODE(function_decl_node), false, parameters));
   FunctionBehaviorRef FB = FunctionBehaviorRef(new FunctionBehavior(AppM, helper, parameters));
   AppM->GetCallGraphManager()->AddFunctionAndCallPoint(function_id, GET_INDEX_NODE(function_decl_node), new_readwritecall->index, FB, FunctionEdgeInfo::CallType::direct_call);
}



void interface_infer::create_resource_Read_none(std::vector<std::string> & operations, const std::string& argName_string, const std::string &interfaceType, unsigned int inputBitWidth, bool IO_port)
{
   const std::string ResourceName = ENCODE_FDNAME(argName_string,"_Read_",interfaceType);
   auto HLSMgr = GetPointer<HLS_manager>(AppM);
   auto HLS_T = HLSMgr->get_HLS_target();
   auto TechMan = HLS_T->get_technology_manager();
   if (!TechMan->is_library_manager(INTERFACE_LIBRARY) || !TechMan->get_library_manager(INTERFACE_LIBRARY)->is_fu(ResourceName))
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "-->Creating interface resource: " + INTERFACE_LIBRARY + ":" + ResourceName);
      structural_objectRef interface_top;
      structural_managerRef CM = structural_managerRef(new structural_manager(parameters));
      structural_type_descriptorRef module_type = structural_type_descriptorRef(new structural_type_descriptor(ResourceName));
      CM->set_top_info(ResourceName, module_type);
      interface_top = CM->get_circ();
      /// add description and license
      GetPointer<module>(interface_top)->set_description("Interface module for function: " + ResourceName);
      GetPointer<module>(interface_top)->set_copyright(GENERATED_COPYRIGHT);
      GetPointer<module>(interface_top)->set_authors("Component automatically generated by bambu");
      GetPointer<module>(interface_top)->set_license(GENERATED_LICENSE);

      unsigned int address_bitsize = HLSMgr->get_address_bitsize();
      structural_type_descriptorRef word_bool_type = structural_type_descriptorRef(new structural_type_descriptor("bool", address_bitsize));
      auto addrPort = CM->add_port("in1", port_o::IN, interface_top, word_bool_type); // this port has a fixed name
      GetPointer<port_o>(addrPort)->set_is_addr_bus(true);
      GetPointer<port_o>(addrPort)->set_is_var_args(true); /// required to activate the module generation
      structural_type_descriptorRef Intype = structural_type_descriptorRef(new structural_type_descriptor("bool", inputBitWidth));
      auto ReadPort = CM->add_port("out1", port_o::OUT, interface_top, Intype);
      auto inPort = CM->add_port("_"+argName_string+(IO_port?"_i":""), port_o::IN, interface_top, Intype);
      GetPointer<port_o>(inPort)->set_port_interface(port_o::port_interface::PI_RNONE);

      CM->add_NP_functionality(interface_top, NP_functionality::LIBRARY, "out1");
      CM->add_NP_functionality(interface_top, NP_functionality::VERILOG_GENERATOR, "Read_" + interfaceType + ".cpp");
      TechMan->add_resource(INTERFACE_LIBRARY, ResourceName, CM);
      for(auto fdName: operations)
         TechMan->add_operation(INTERFACE_LIBRARY, ResourceName, fdName);
      auto* fu = GetPointer<functional_unit>(TechMan->get_fu(ResourceName, INTERFACE_LIBRARY));
      const target_deviceRef device = HLS_T->get_target_device();
      fu->area_m = area_model::create_model(device->get_type(), parameters);
      fu->area_m->set_area_value(0);
      fu->logical_type = functional_unit::COMBINATIONAL;

      for(auto fdName: operations)
      {
         auto* op = GetPointer<operation>(fu->get_operation(fdName));
         op->time_m = time_model::create_model(device->get_type(), parameters);
         op->bounded = true;
         op->time_m->set_execution_time(EPSILON, 0);
         op->time_m->set_stage_period(0.0);
         op->time_m->set_synthesis_dependent(true);
      }
      /// add constraint on resource
      HLSMgr->design_interface_constraints[function_id][INTERFACE_LIBRARY][ResourceName]=1;
      INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "<--Interface resource created: ");
   }
}

void interface_infer::create_resource_Write_none(std::vector<std::string> & operations, const std::string& argName_string, const std::string &interfaceType, unsigned int inputBitWidth, bool IO_port)
{
   const std::string ResourceName = ENCODE_FDNAME(argName_string,"_Write_",interfaceType);
   auto HLSMgr = GetPointer<HLS_manager>(AppM);
   auto HLS_T = HLSMgr->get_HLS_target();
   auto TechMan = HLS_T->get_technology_manager();
   if (!TechMan->is_library_manager(INTERFACE_LIBRARY) || !TechMan->get_library_manager(INTERFACE_LIBRARY)->is_fu(ResourceName))
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "-->Creating interface resource: " + INTERFACE_LIBRARY + ":" + ResourceName);
      structural_objectRef interface_top;
      structural_managerRef CM = structural_managerRef(new structural_manager(parameters));
      structural_type_descriptorRef module_type = structural_type_descriptorRef(new structural_type_descriptor(ResourceName));
      CM->set_top_info(ResourceName, module_type);
      interface_top = CM->get_circ();
      /// add description and license
      GetPointer<module>(interface_top)->set_description("Interface module for function: " + ResourceName);
      GetPointer<module>(interface_top)->set_copyright(GENERATED_COPYRIGHT);
      GetPointer<module>(interface_top)->set_authors("Component automatically generated by bambu");
      GetPointer<module>(interface_top)->set_license(GENERATED_LICENSE);

      unsigned int address_bitsize = HLSMgr->get_address_bitsize();
      structural_type_descriptorRef word_bool_type = structural_type_descriptorRef(new structural_type_descriptor("bool", address_bitsize));
      structural_type_descriptorRef Intype = structural_type_descriptorRef(new structural_type_descriptor("bool", inputBitWidth));
      structural_type_descriptorRef rwsize = structural_type_descriptorRef(new structural_type_descriptor("bool", 1));
      structural_type_descriptorRef rwtype = structural_type_descriptorRef(new structural_type_descriptor("bool", 1));
      auto rwPort = CM->add_port("in1", port_o::IN, interface_top, rwsize);
      auto writePort = CM->add_port("in2", port_o::IN, interface_top, rwtype);
      auto addrPort = CM->add_port("in3", port_o::IN, interface_top, word_bool_type);
      GetPointer<port_o>(addrPort)->set_is_addr_bus(true);
      GetPointer<port_o>(addrPort)->set_is_var_args(true); /// required to activate the module generation
      auto inPort_o = CM->add_port("_"+argName_string+(IO_port?"_o":""), port_o::OUT, interface_top, Intype);
      GetPointer<port_o>(inPort_o)->set_port_interface(port_o::port_interface::PI_WNONE);

      CM->add_NP_functionality(interface_top, NP_functionality::LIBRARY, "in1 in2");
      CM->add_NP_functionality(interface_top, NP_functionality::VERILOG_GENERATOR, "Write_" + interfaceType + ".cpp");
      TechMan->add_resource(INTERFACE_LIBRARY, ResourceName, CM);
      for(auto fdName: operations)
         TechMan->add_operation(INTERFACE_LIBRARY, ResourceName, fdName);
      auto* fu = GetPointer<functional_unit>(TechMan->get_fu(ResourceName, INTERFACE_LIBRARY));
      const target_deviceRef device = HLS_T->get_target_device();
      fu->area_m = area_model::create_model(device->get_type(), parameters);
      fu->area_m->set_area_value(0);
      fu->logical_type = functional_unit::COMBINATIONAL;

      for(auto fdName: operations)
      {
         auto* op = GetPointer<operation>(fu->get_operation(fdName));
         op->time_m = time_model::create_model(device->get_type(), parameters);
         op->bounded = true;
         op->time_m->set_execution_time(EPSILON, 0);
         op->time_m->set_stage_period(0.0);
         op->time_m->set_synthesis_dependent(true);
      }
      /// add constraint on resource
      HLSMgr->design_interface_constraints[function_id][INTERFACE_LIBRARY][ResourceName]=1;
      INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "<--Interface resource created: ");
   }
}
void interface_infer::create_resource(std::vector<std::string> & operationsR, std::vector<std::string> & operationsW, const std::string& argName_string, const std::string &interfaceType, unsigned int inputBitWidth)
{
   if(interfaceType=="none")
   {
      THROW_ASSERT(!operationsR.empty() || !operationsW.empty(), "unexpected condition");
      bool IO_P = !operationsR.empty() && !operationsW.empty();
      if(!operationsR.empty())
         create_resource_Read_none(operationsR, argName_string, interfaceType, inputBitWidth, IO_P);
      if(!operationsW.empty())
            create_resource_Write_none(operationsW, argName_string, interfaceType, inputBitWidth, IO_P);
   }
   else
      THROW_ERROR("interface not supported: " + interfaceType);

}


void interface_infer::addGimpleNOPxVirtual(tree_nodeRef origStmt, statement_list*sl, const tree_managerRef TM)
{
   std::map<TreeVocabularyTokenTypes_TokenEnum, std::string> gimple_nop_schema;
   gimple_nop_schema[TOK(TOK_SRCP)] = "<built-in>:0:0";
   const auto gimple_node_id = TM->new_tree_node_id();
   TM->create_tree_node(gimple_node_id, gimple_nop_K, gimple_nop_schema);
   auto gimple_nop_Node = TM->GetTreeReindex(gimple_node_id);
   auto newGN = GetPointer<gimple_node>(GET_NODE(gimple_nop_Node));
   auto origGN = GetPointer<gimple_node>(GET_NODE(origStmt));
   newGN->memdef=origGN->memdef;
   newGN->memuse=origGN->memuse;
   for(auto vUse : origGN->vuses)
   {
      auto sn = GetPointer<ssa_name>(GET_NODE(vUse));
      newGN->AddVuse(vUse);
      sn->AddUseStmt(gimple_nop_Node);
   }
   for(auto vOver : origGN->vovers)
      newGN->AddVover(vOver);
   if(origGN->vdef)
   {
      auto snDef = GetPointer<ssa_name>(GET_NODE(origGN->vdef));
      newGN->vdef=origGN->vdef;
      snDef->SetDefStmt(gimple_nop_Node);
   }
   sl->list_of_bloc[origGN->bb_index]->PushBefore(gimple_nop_Node, origStmt);
   sl->list_of_bloc[origGN->bb_index]->RemoveStmt(origStmt);
}


DesignFlowStep_Status interface_infer::InternalExec()
{
   if (GetPointer<const HLS_manager>(AppM))
   {
      const auto top_functions = AppM->CGetCallGraphManager()->GetRootFunctions();
      bool is_top = top_functions.find(function_id) != top_functions.end();
      if (is_top)
      {
         auto HLSMgr = GetPointer<HLS_manager>(AppM);
         /// load xml interface specification file
         for (auto source_file : AppM->input_files)
         {
            const std::string output_temporary_directory = parameters->getOption<std::string>(OPT_output_temporary_directory);
            std::string leaf_name = source_file.second == "-" ? "stdin-" : GetLeafFileName(source_file.second);
            auto XMLfilename = output_temporary_directory + "/" + leaf_name + ".interface.xml";
            if ((boost::filesystem::exists(boost::filesystem::path(XMLfilename))))
            {
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->parsing " + XMLfilename);
               XMLDomParser parser(XMLfilename);
               parser.Exec();
               if (parser)
               {
                  // Walk the tree:
                  const xml_element* node = parser.get_document()->get_root_node(); // deleted by DomParser.
                  for (const auto& iter : node->get_children())
                  {
                     const auto* Enode = GetPointer<const xml_element>(iter);
                     if (!Enode) continue;
                     if (Enode->get_name() == "function")
                     {
                        std::string fname;
                        for (auto attr : Enode->get_attributes())
                        {
                           std::string key = attr->get_name();
                           std::string value = attr->get_value();
                           if (key == "id") fname = value;
                        }
                        if (fname == "") THROW_ERROR("malformed interface file");
                        for (const auto& iterArg : Enode->get_children())
                        {
                           const auto* EnodeArg = GetPointer<const xml_element>(iterArg);
                           if (!EnodeArg) continue;
                           if (EnodeArg->get_name() == "arg")
                           {
                              std::string argName;
                              std::string interfaceType;
                              std::string interfaceTypename;
                              std::string interfaceTypenameInclude;
                              for (auto attrArg : EnodeArg->get_attributes())
                              {
                                 std::string key = attrArg->get_name();
                                 std::string value = attrArg->get_value();
                                 if (key == "id") argName = value;
                                 if (key == "interface_type") interfaceType = value;
                                 if (key == "interface_typename") interfaceTypename = value;
                                 if (key == "interface_typename_include") interfaceTypenameInclude = value;
                              }
                              if (argName == "") THROW_ERROR("malformed interface file");
                              if (interfaceType == "") THROW_ERROR("malformed interface file");
                              std::cerr << "|" << argName << "|" << interfaceType << "|\n";
                              HLSMgr->design_interface[fname][argName] = interfaceType;
                              HLSMgr->design_interface_typename[fname][argName] = interfaceTypename;
                              if((interfaceTypename.find("ap_int<") != std::string::npos || interfaceTypename.find("ap_uint<") != std::string::npos) && interfaceTypenameInclude.find("ac_int.h") != std::string::npos)
                                 boost::replace_all(interfaceTypenameInclude, "ac_int.h", "ap_int.h");
                              if((interfaceTypename.find("ap_fixed<") != std::string::npos || interfaceTypename.find("ap_ufixed<") != std::string::npos) && interfaceTypenameInclude.find("ac_fixed.h") != std::string::npos)
                                 boost::replace_all(interfaceTypenameInclude, "ac_fixed.h", "ap_fixed.h");
                              HLSMgr->design_interface_typenameinclude[fname][argName] = interfaceTypenameInclude;
                           }
                        }
                     }
                  }
               }
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--parsed file " + XMLfilename);
            }
         }

         bool modified = false;
         auto& DesignInterface = HLSMgr->design_interface;
         auto& DesignInterfaceTypename = HLSMgr->design_interface_typename;
         const auto TM = AppM->get_tree_manager();
         auto fnode = TM->get_tree_node_const(function_id);
         auto fd = GetPointer<function_decl>(fnode);
         std::string fname;
         tree_helper::get_mangled_fname(fd, fname);
         if (DesignInterface.find(fname) != DesignInterface.end())
         {
            const tree_manipulationRef tree_man = tree_manipulationRef(new tree_manipulation(TM, parameters));
            /// pre-process the list of statements to bind parm_decl and ssa variables
            std::map<unsigned, unsigned> par2ssa;
            auto* sl = GetPointer<statement_list>(GET_NODE(fd->body));
            for (auto block : sl->list_of_bloc)
            {
               for (const auto& stmt : block.second->CGetStmtList())
               {
                  TreeNodeMap<size_t> used_ssa = tree_helper::ComputeSsaUses(stmt);
                  for (const auto& s : used_ssa)
                  {
                     const tree_nodeRef ssa_tn = GET_NODE(s.first);
                     const auto* ssa = GetPointer<const ssa_name>(ssa_tn);
                     if (ssa->var != nullptr and GET_NODE(ssa->var)->get_kind() == parm_decl_K)
                     {
                        auto par_id = GET_INDEX_NODE(ssa->var);
                        if (par2ssa.find(par_id) != par2ssa.end()) { THROW_ASSERT(par2ssa.find(par_id)->second == GET_INDEX_NODE(s.first), "unexpected condition"); }
                        else
                           par2ssa[par_id] = GET_INDEX_NODE(s.first);
                     }
                  }
               }
            }

            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Analyzing function " + fname);
            auto& DesignInterfaceArgs = DesignInterface.find(fname)->second;
            auto& DesignInterfaceTypenameArgs = DesignInterfaceTypename.find(fname)->second;
            for (auto arg : fd->list_of_args)
            {
               auto arg_id = GET_INDEX_NODE(arg);
               INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "---parm_decl id: " + STR(arg_id));
               auto a = GetPointer<parm_decl>(GET_NODE(arg));
               auto argName = GET_NODE(a->name);
               THROW_ASSERT(GetPointer<identifier_node>(argName), "unexpected condition");
               const std::string& argName_string = GetPointer<identifier_node>(argName)->strg;
               INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "---parm_decl name: " + argName_string);
               THROW_ASSERT(DesignInterfaceArgs.find(argName_string) != DesignInterfaceArgs.end(), "unexpected condition");
               auto interfaceType = DesignInterfaceArgs.find(argName_string)->second;
               auto interfaceTypename = DesignInterfaceTypenameArgs.find(argName_string)->second;
               if (interfaceType != "default")
               {                  
                  if(par2ssa.find(arg_id) == par2ssa.end())
                  {
                     THROW_WARNING("parameter not used by any statement");
                     if (tree_helper::is_a_pointer(TM, GET_INDEX_NODE(a->type)))
                        DesignInterfaceArgs[argName_string]="none";
                     else
                        THROW_ERROR("parameter not used: specified interface does not make sense - " + interfaceType);
                     continue;
                  }
                  if (tree_helper::is_a_pointer(TM, GET_INDEX_NODE(a->type)))
                  {
                     std::cerr << "is a pointer\n";
                     std::cerr << "list of statement that use this parameter\n";
                     auto inputBitWidth = tree_helper::size(TM, tree_helper::get_pointed_type(TM, GET_INDEX_NODE(a->type)));
                     if(interfaceTypename.find("ac_int<") == 0)
                     {
                        auto subtypeArg=interfaceTypename.substr(std::string("ac_int<").size());
                        auto sizeString = subtypeArg.substr(0,subtypeArg.find_first_of(",> "));
                        inputBitWidth=boost::lexical_cast<unsigned>(sizeString);
                     }
                     else if(interfaceTypename.find("ac_fixed<") == 0)
                     {
                        auto subtypeArg=interfaceTypename.substr(std::string("ac_fixed<").size());
                        auto sizeString = subtypeArg.substr(0,subtypeArg.find_first_of(",> "));
                        inputBitWidth=boost::lexical_cast<unsigned>(sizeString);
                     }
                     else if(interfaceTypename.find("ap_int<") == 0)
                     {
                        auto subtypeArg=interfaceTypename.substr(std::string("ap_int<").size());
                        auto sizeString = subtypeArg.substr(0,subtypeArg.find_first_of(",> "));
                        inputBitWidth=boost::lexical_cast<unsigned>(sizeString);
                     }
                     else if(interfaceTypename.find("ap_uint<") == 0)
                     {
                        auto subtypeArg=interfaceTypename.substr(std::string("ap_uint<").size());
                        auto sizeString = subtypeArg.substr(0,subtypeArg.find_first_of(",> "));
                        inputBitWidth=boost::lexical_cast<unsigned>(sizeString);
                     }
                     else if(interfaceTypename.find("ap_fixed<") == 0)
                     {
                        auto subtypeArg=interfaceTypename.substr(std::string("ap_fixed<").size());
                        auto sizeString = subtypeArg.substr(0,subtypeArg.find_first_of(",> "));
                        inputBitWidth=boost::lexical_cast<unsigned>(sizeString);
                     }
                     else if(interfaceTypename.find("ap_ufixed<") == 0)
                     {
                        auto subtypeArg=interfaceTypename.substr(std::string("ap_ufixed<").size());
                        auto sizeString = subtypeArg.substr(0,subtypeArg.find_first_of(",> "));
                        inputBitWidth=boost::lexical_cast<unsigned>(sizeString);
                     }
                     THROW_ASSERT(inputBitWidth, "unexpected condition");

                     auto argSSANode = TM->GetTreeReindex(par2ssa.find(arg_id)->second);
                     bool canBeMovedToBB2 = true;
                     bool isRead = false;
                     bool isWrite = false;
                     bool unkwown_pattern = false;
                     std::list<tree_nodeRef> writeStmt;
                     std::list<tree_nodeRef> readStmt;

                     classifyArg(sl, argSSANode, canBeMovedToBB2, isRead, isWrite, unkwown_pattern, writeStmt, readStmt);

                     if (unkwown_pattern)
                        std::cerr << "unknown pattern identified\n";
                     else
                     {
                        if (canBeMovedToBB2 && isRead) std::cerr << "YES can be moved\n";
                        if (isRead && isWrite)
                        {
                           std::cerr << "IO arg\n";
                           if(interfaceType == "ptrdefault")
                           {
                              DesignInterfaceArgs[argName_string]="valid";
                              interfaceType="valid";
                           }
                        }
                        else if (isRead)
                        {
                           if(interfaceType == "ptrdefault")
                           {
                              DesignInterfaceArgs[argName_string]="none";
                              interfaceType="none";
                           }
                           std::cerr << "I arg\n";
                        }
                        else if (isWrite)
                        {
                           std::cerr << "O arg\n";
                           if(interfaceType == "ptrdefault")
                           {
                              DesignInterfaceArgs[argName_string]="valid";
                              interfaceType="valid";
                           }
                        }
                        else
                           THROW_ERROR("pattern not yet supported: unused arg");
                        if (canBeMovedToBB2 && isRead && !isWrite)
                        {
                           unsigned int destBB = bloc::ENTRY_BLOCK_ID;
                           for (auto bb_succ : sl->list_of_bloc[bloc::ENTRY_BLOCK_ID]->list_of_succ)
                           {
                              if (bb_succ == bloc::EXIT_BLOCK_ID) continue;
                              if (destBB == bloc::ENTRY_BLOCK_ID)
                                 destBB = bb_succ;
                              else
                                 THROW_ERROR("unexpected pattern");
                           }
                           THROW_ASSERT(destBB != bloc::ENTRY_BLOCK_ID, "unexpected condition");
                           std::string fdName = ENCODE_FDNAME(argName_string,"_Read_",interfaceType);
                           std::vector<std::string> operationsR,operationsW;
                           operationsR.push_back(fdName);
                           std::list<tree_nodeRef> usedStmt_defs;
                           tree_nodeRef readType;
                           for(auto rs : readStmt)
                           {
                              auto rs_node = GET_NODE(rs);
                              auto rs_ga = GetPointer<gimple_assign>(rs_node);
                              usedStmt_defs.push_back(rs_ga->op0);
                              if(!readType)
                                 readType=GetPointer<mem_ref>(GET_NODE(rs_ga->op1))->type;
                           }
                           create_Read_function(tree_nodeRef(), destBB, sl, fd, fdName, argSSANode, a, readType, usedStmt_defs, tree_man, TM);
                           for(auto rs: readStmt)
                              addGimpleNOPxVirtual(rs,sl,TM);
                           create_resource(operationsR, operationsW, argName_string, interfaceType, inputBitWidth);
                           modified = true;
                        }
                        else if(isRead && !isWrite)
                        {
                           std::string fdName = ENCODE_FDNAME(argName_string,"_Read_",interfaceType);
                           std::vector<std::string> operationsR,operationsW;
                           unsigned int loadIdIndex=0;
                           for(auto rs : readStmt)
                           {
                              std::list<tree_nodeRef> usedStmt_defs;
                              auto rs_node = GET_NODE(rs);
                              auto rs_ga = GetPointer<gimple_assign>(rs_node);
                              usedStmt_defs.push_back(rs_ga->op0);
                              std::string instanceFname = fdName+STR(loadIdIndex);
                              operationsR.push_back(instanceFname);
                              create_Read_function(rs, rs_ga->bb_index, sl, fd, instanceFname, argSSANode, a, GetPointer<mem_ref>(GET_NODE(rs_ga->op1))->type, usedStmt_defs, tree_man, TM);
                              addGimpleNOPxVirtual(rs,sl,TM);
                              usedStmt_defs.clear();
                              ++loadIdIndex;
                           }
                           create_resource(operationsR, operationsW, argName_string, interfaceType, inputBitWidth);
                           modified = true;
                        }
                        else if (canBeMovedToBB2 && isRead && isWrite)
                        {
                           unsigned int destBB = bloc::ENTRY_BLOCK_ID;
                           for (auto bb_succ : sl->list_of_bloc[bloc::ENTRY_BLOCK_ID]->list_of_succ)
                           {
                              if (bb_succ == bloc::EXIT_BLOCK_ID) continue;
                              if (destBB == bloc::ENTRY_BLOCK_ID)
                                 destBB = bb_succ;
                              else
                                 THROW_ERROR("unexpected pattern");
                           }
                           THROW_ASSERT(destBB != bloc::ENTRY_BLOCK_ID, "unexpected condition");
                           std::string fdName = ENCODE_FDNAME(argName_string,"_Read_",interfaceType);
                           std::vector<std::string> operationsR,operationsW;
                           operationsR.push_back(fdName);
                           std::list<tree_nodeRef> usedStmt_defs;
                           tree_nodeRef readType;
                           for(auto rs : readStmt)
                           {
                              auto rs_node = GET_NODE(rs);
                              auto rs_ga = GetPointer<gimple_assign>(rs_node);
                              usedStmt_defs.push_back(rs_ga->op0);
                              if(!readType)
                                 readType=GetPointer<mem_ref>(GET_NODE(rs_ga->op1))->type;
                           }
                           create_Read_function(tree_nodeRef(), destBB, sl, fd, fdName, argSSANode, a, readType, usedStmt_defs, tree_man, TM);
                           for(auto rs: readStmt)
                              addGimpleNOPxVirtual(rs,sl,TM);
                           unsigned int IdIndex=0;
                           fdName = ENCODE_FDNAME(argName_string,"_Write_",interfaceType);
                           for(auto ws : writeStmt)
                           {
                              auto ws_node = GET_NODE(ws);
                              auto ws_ga = GetPointer<gimple_assign>(ws_node);
                              std::string instanceFname = fdName+STR(IdIndex);
                              operationsW.push_back(instanceFname);
                              create_Write_function(ws, ws_ga->bb_index, sl, fd, instanceFname, argSSANode, ws_ga->op1, a, GetPointer<mem_ref>(GET_NODE(ws_ga->op0))->type, tree_man, TM);
                              ++IdIndex;
                           }
                           create_resource(operationsR, operationsW, argName_string, interfaceType, inputBitWidth);
                           modified = true;
                        }
                        else if(isRead && isWrite)
                        {
                           std::string fdName = ENCODE_FDNAME(argName_string,"_Read_",interfaceType);
                           std::vector<std::string> operationsR,operationsW;
                           unsigned int IdIndex=0;
                           std::list<tree_nodeRef> usedStmt_defs;
                           for(auto rs : readStmt)
                           {
                              auto rs_node = GET_NODE(rs);
                              auto rs_ga = GetPointer<gimple_assign>(rs_node);
                              usedStmt_defs.push_back(rs_ga->op0);
                              std::string instanceFname = fdName+STR(IdIndex);
                              operationsR.push_back(instanceFname);
                              create_Read_function(rs, rs_ga->bb_index, sl, fd, instanceFname, argSSANode, a, GetPointer<mem_ref>(GET_NODE(rs_ga->op1))->type, usedStmt_defs, tree_man, TM);
                              addGimpleNOPxVirtual(rs,sl,TM);
                              usedStmt_defs.clear();
                              ++IdIndex;
                           }
                           IdIndex=0;
                           fdName = ENCODE_FDNAME(argName_string,"_Write_",interfaceType);
                           for(auto ws : writeStmt)
                           {
                              auto ws_node = GET_NODE(ws);
                              auto ws_ga = GetPointer<gimple_assign>(ws_node);
                              std::string instanceFname = fdName+STR(IdIndex);
                              operationsW.push_back(instanceFname);
                              create_Write_function(ws, ws_ga->bb_index, sl, fd, instanceFname, argSSANode, ws_ga->op1, a, GetPointer<mem_ref>(GET_NODE(ws_ga->op0))->type, tree_man, TM);
                              ++IdIndex;
                           }
                           create_resource(operationsR, operationsW, argName_string, interfaceType, inputBitWidth);
                           modified = true;
                        }
                        else if(!isRead && isWrite)
                        {
                           std::vector<std::string> operationsR,operationsW;
                           unsigned int IdIndex=0;
                           std::string fdName = ENCODE_FDNAME(argName_string,"_Write_",interfaceType);;
                           for(auto ws : writeStmt)
                           {
                              auto ws_node = GET_NODE(ws);
                              auto ws_ga = GetPointer<gimple_assign>(ws_node);
                              std::string instanceFname = fdName+STR(IdIndex);
                              operationsW.push_back(instanceFname);
                              create_Write_function(ws, ws_ga->bb_index, sl, fd, instanceFname, argSSANode, ws_ga->op1, a, GetPointer<mem_ref>(GET_NODE(ws_ga->op0))->type, tree_man, TM);
                              ++IdIndex;
                           }
                           create_resource(operationsR, operationsW, argName_string, interfaceType, inputBitWidth);
                           modified = true;
                        }
                        else
                           THROW_ERROR("pattern not yet supported");
                     }
                  }
                  else if(interfaceType == "none")
                  {
                     THROW_ERROR("unexpected interface ("+interfaceType+") for parameter " + argName_string);
                  }
                  else
                  {
                     THROW_ERROR("not yet supported interface ("+interfaceType+") for parameter " + argName_string);
                  }
               }
            }

            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Analyzed function " + fname);
         }
         if (modified) function_behavior->UpdateBBVersion();
         return modified ? DesignFlowStep_Status::SUCCESS : DesignFlowStep_Status::UNCHANGED;
      }
   }
   return DesignFlowStep_Status::UNCHANGED;
}

void interface_infer::Initialize() { FunctionFrontendFlowStep::Initialize(); }

bool interface_infer::HasToBeExecuted() const { return bb_version == 0; }
