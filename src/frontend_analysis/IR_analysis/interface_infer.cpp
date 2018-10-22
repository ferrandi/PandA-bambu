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

DesignFlowStep_Status interface_infer::InternalExec()
{
   if (GetPointer<const HLS_manager>(AppM))
   {
      const auto top_functions = AppM->CGetCallGraphManager()->GetRootFunctions();
      bool is_top = top_functions.find(function_id) != top_functions.end();
      if (is_top)
      {
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
                              for (auto attrArg : EnodeArg->get_attributes())
                              {
                                 std::string key = attrArg->get_name();
                                 std::string value = attrArg->get_value();
                                 if (key == "id") argName = value;
                                 if (key == "interface_type") interfaceType = value;
                              }
                              if (argName == "") THROW_ERROR("malformed interface file");
                              if (interfaceType == "") THROW_ERROR("malformed interface file");
                              std::cerr << "|" << argName << "|" << interfaceType << "|\n";
                              GetPointer<HLS_manager>(AppM)->design_interface[fname][argName] = interfaceType;
                           }
                        }
                     }
                  }
               }
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--parsed file " + XMLfilename);
            }
         }

         bool modified = false;
         auto HLSMgr = GetPointer<HLS_manager>(AppM);
         const auto& DesignInterface = HLSMgr->design_interface;
         const auto TM = AppM->get_tree_manager();
         auto fnode = TM->get_tree_node_const(function_id);
         auto fname = tree_helper::name_function(TM, function_id);
         if (DesignInterface.find(fname) != DesignInterface.end())
         {
            const tree_manipulationRef tree_man = tree_manipulationRef(new tree_manipulation(TM, parameters));
            /// preprocess the list of statements to bind parm_decl and ssa variables
            std::map<unsigned, unsigned> par2ssa;
            auto fd = GetPointer<function_decl>(fnode);
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
            const auto& DesignInterfaceArgs = DesignInterface.find(fname)->second;
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
               if (interfaceType == "none")
               {
                  auto argTypeNode = GET_NODE(a->type);
                  if (tree_helper::is_a_pointer(TM, GET_INDEX_NODE(a->type)))
                  {
                     std::cerr << "is a pointer\n";
                     std::cerr << "list of statement that use this parameter\n";
                     THROW_ASSERT(par2ssa.find(arg_id) != par2ssa.end(), "unexpected condition");
                     auto argSSA_id = par2ssa.find(arg_id)->second;
                     auto argSSA_node = TM->get_tree_node_const(argSSA_id);
                     auto argSSA = GetPointer<ssa_name>(argSSA_node);
                     THROW_ASSERT(argSSA, "unexpected condition");
                     bool canBeMovedToBB2 = true;
                     bool isRead = false;
                     bool isWrite = false;
                     bool unkwown_pattern = false;
                     tree_nodeRef readType;
                     const auto inputBitWidth = tree_helper::size(TM, tree_helper::get_pointed_type(TM, GET_INDEX_NODE(a->type)));
                     std::list<tree_nodeRef> usedStmt_defs;
                     for (auto par_use : argSSA->CGetUseStmts())
                     {
                        auto use_stmt = GET_NODE(par_use.first);
                        std::cerr << "STMT: " << use_stmt->ToString() << std::endl;
                        auto gn = GetPointer<gimple_node>(use_stmt);
                        if (sl->list_of_bloc.find(gn->bb_index)->second->loop_id != 0) canBeMovedToBB2 = false;
                        if (auto ga = GetPointer<gimple_assign>(use_stmt))
                        {
                           if (GET_NODE(ga->op0)->get_kind() == mem_ref_K) isWrite = true;
                           if (GET_NODE(ga->op1)->get_kind() == mem_ref_K)
                           {
                              isRead = true;
                              if (readType && GET_INDEX_NODE(GetPointer<mem_ref>(GET_NODE(ga->op1))->type) != GET_INDEX_NODE(readType)) canBeMovedToBB2 = false; /// reading different objects
                              readType = GetPointer<mem_ref>(GET_NODE(ga->op1))->type;
                              usedStmt_defs.push_back(ga->op0);
                              if (ga->vdef)
                              {
                                 unkwown_pattern = true;
                                 THROW_WARNING("Pattern currently not supported: use of a volatile load" + use_stmt->ToString());
                              }
                           }
                        }
                        else
                        {
                           unkwown_pattern = true;
                           THROW_WARNING("USE PATTERN unexpected" + use_stmt->ToString());
                        }
                     }
                     if (unkwown_pattern)
                        std::cerr << "unknown pattern identified\n";
                     else
                     {
                        if (canBeMovedToBB2) std::cerr << "YES can be moved\n";
                        if (isRead && isWrite)
                           std::cerr << "IO arg\n";
                        else if (isRead)
                           std::cerr << "I arg\n";
                        else if (isWrite)
                           std::cerr << "O arg\n";
                        else
                           std::cerr << "unused arg\n";
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
                           /// create the function_decl
                           std::string fdName = argName_string + STR_CST_interface_parameter_keyword + "_Read_" + interfaceType;
                           std::vector<tree_nodeRef> argsT;
                           argsT.push_back(a->type);
                           const std::string srcp = fd->include_name + ":" + STR(fd->line_number) + ":" + STR(fd->column_number);
                           bool generateBody = false;
                           auto function_decl_node = tree_man->create_function_decl(fdName, fd->scpe, argsT, readType, srcp, generateBody);
                           auto fdCreated = GetPointer<function_decl>(GET_NODE(function_decl_node));
                           auto par = fdCreated->list_of_args.front();
                           if (generateBody)
                           {
                              /// add body
                              auto slCreated = GetPointer<statement_list>(GET_NODE(fdCreated->body));
                              std::vector<unsigned int> bb_list_of_pred;
                              std::vector<unsigned int> bb_list_of_succ;
                              bb_list_of_pred.push_back(BB_ENTRY);
                              bb_list_of_succ.push_back(BB_EXIT);
                              std::vector<tree_nodeRef> stmts;
                              /// create ssa for function decl parameter
                              tree_nodeRef ssa_par = tree_man->create_ssa_name(par, a->type);
                              /// create vuse var
                              auto void_type_node = tree_man->create_void_type();
                              tree_nodeRef ssa_vuse = tree_man->create_ssa_name(tree_nodeRef(), void_type_node, false, true);
                              std::map<TreeVocabularyTokenTypes_TokenEnum, std::string> nop_IR_schema;
                              const auto nop_id = TM->new_tree_node_id();
                              nop_IR_schema[TOK(TOK_SRCP)] = "<built-in>:0:0";
                              TM->create_tree_node(nop_id, gimple_nop_K, nop_IR_schema);
                              GetPointer<ssa_name>(GET_NODE(ssa_vuse))->AddDefStmt(TM->GetTreeReindex(nop_id));
                              GetPointer<ssa_name>(GET_NODE(ssa_vuse))->default_flag = true;
                              /// create load expression
                              auto offset = TM->CreateUniqueIntegerCst(0, GET_INDEX_NODE(a->type));
                              auto loadExpr = tree_man->create_binary_operation(readType, ssa_par, offset, srcp, mem_ref_K);
                              auto assignLoad = tree_man->CreateGimpleAssign(readType, loadExpr, 2, srcp);
                              stmts.push_back(assignLoad);
                              tree_nodeRef load_ssa_var = GetPointer<gimple_assign>(GET_NODE(assignLoad))->op0;
                              GetPointer<gimple_assign>(GET_NODE(assignLoad))->AddVuse(ssa_vuse);
                              auto gr = tree_man->create_gimple_return(readType, load_ssa_var, srcp, 2);
                              stmts.push_back(gr);
                              tree_man->create_basic_block(slCreated->list_of_bloc, bb_list_of_pred, bb_list_of_succ, stmts, 2);
                           }

                           std::vector<tree_nodeRef> args;
                           args.push_back(TM->GetTreeReindex(argSSA_id));
                           auto call_expr_node = tree_man->CreateCallExpr(function_decl_node, args, srcp);
                           auto new_assignment = tree_man->CreateGimpleAssign(readType, call_expr_node, destBB, srcp);
                           sl->list_of_bloc[destBB]->PushBack(new_assignment);
                           BehavioralHelperRef helper = BehavioralHelperRef(new BehavioralHelper(AppM, GET_INDEX_NODE(function_decl_node), generateBody, parameters));
                           FunctionBehaviorRef FB = FunctionBehaviorRef(new FunctionBehavior(AppM, helper, parameters));
                           AppM->GetCallGraphManager()->AddFunctionAndCallPoint(function_id, GET_INDEX_NODE(function_decl_node), new_assignment->index, FB, FunctionEdgeInfo::CallType::direct_call);
                           tree_nodeRef temp_ssa_var = GetPointer<gimple_assign>(GET_NODE(new_assignment))->op0;
                           for (auto defSSA : usedStmt_defs)
                           {
                              auto ssaDefVar = GetPointer<ssa_name>(GET_NODE(defSSA));
                              THROW_ASSERT(ssaDefVar, "unexpected condition");
                              std::list<tree_nodeRef> varUses;
                              for (auto used : ssaDefVar->CGetUseStmts()) { varUses.push_back(used.first); }
                              for (auto used : varUses) { TM->ReplaceTreeNode(used, defSSA, temp_ssa_var); }
                           }
                           modified = true;
                           auto HLS_T = HLSMgr->get_HLS_target();
                           auto TechMan = HLS_T->get_technology_manager();
                           if (!TechMan->is_library_manager(INTERFACE_LIBRARY) || !TechMan->get_library_manager(INTERFACE_LIBRARY)->is_fu(fdName))
                           {
                              INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "-->Creating interface resource: " + INTERFACE_LIBRARY + ":" + fdName);
                              structural_objectRef interface_top;
                              structural_managerRef CM = structural_managerRef(new structural_manager(parameters));
                              structural_type_descriptorRef module_type = structural_type_descriptorRef(new structural_type_descriptor(fdName));
                              CM->set_top_info(fdName, module_type);
                              interface_top = CM->get_circ();
                              /// add description and license
                              GetPointer<module>(interface_top)->set_description("Interface module for function: " + fdName);
                              GetPointer<module>(interface_top)->set_copyright(GENERATED_COPYRIGHT);
                              GetPointer<module>(interface_top)->set_authors("Component automatically generated by bambu");
                              GetPointer<module>(interface_top)->set_license(GENERATED_LICENSE);

                              unsigned int address_bitsize = HLSMgr->get_address_bitsize();
                              structural_type_descriptorRef word_bool_type = structural_type_descriptorRef(new structural_type_descriptor("bool", address_bitsize));
                              auto pd = GetPointer<parm_decl>(GET_NODE(par));
                              THROW_ASSERT(GetPointer<identifier_node>(GET_NODE(pd->name)), "unexpected condition");
                              auto addrPort = CM->add_port("in1", port_o::IN, interface_top, word_bool_type); // this port has a fixed name
                              GetPointer<port_o>(addrPort)->set_is_addr_bus(true);
                              GetPointer<port_o>(addrPort)->set_is_var_args(true); /// required to activate the module generation
                              structural_type_descriptorRef Intype = structural_type_descriptorRef(new structural_type_descriptor("bool", inputBitWidth));
                              auto ReadPort = CM->add_port("out1", port_o::OUT, interface_top, Intype);
                              auto inPort = CM->add_port("_"+argName_string, port_o::IN, interface_top, Intype);
                              GetPointer<port_o>(inPort)->set_port_interface(port_o::port_interface::PI_RNONE);

                              CM->add_NP_functionality(interface_top, NP_functionality::LIBRARY, "out1");
                              CM->add_NP_functionality(interface_top, NP_functionality::VERILOG_GENERATOR, "Read_" + interfaceType + ".cpp");
                              TechMan->add_resource(INTERFACE_LIBRARY, fdName, CM);
                              TechMan->add_operation(INTERFACE_LIBRARY, fdName, fdName);
                              auto* fu = GetPointer<functional_unit>(TechMan->get_fu(fdName, INTERFACE_LIBRARY));
                              const target_deviceRef device = HLS_T->get_target_device();
                              fu->area_m = area_model::create_model(device->get_type(), parameters);
                              fu->area_m->set_area_value(0);
                              fu->logical_type = functional_unit::COMBINATIONAL;

                              auto* op = GetPointer<operation>(fu->get_operation(fdName));
                              op->time_m = time_model::create_model(device->get_type(), parameters);
                              op->bounded = true;
                              op->time_m->set_execution_time(EPSILON, 0);
                              op->time_m->set_stage_period(0.0);
                              op->time_m->set_synthesis_dependent(true);

                              INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "<--Interface resource created: ");
                           }
                        }
                     }
                  }
                  else
                     THROW_ERROR("not yet supported input parameter type");
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
