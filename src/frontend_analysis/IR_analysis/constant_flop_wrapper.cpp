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
 * @file constant_flop_wrapper.cpp
 * @brief Step that recognizes when there's a floating point operation
 *        with a constant and optimize it.
 *
 * @author Nicolas Tagliabue
 * @author Lorenzo Porro
 */

/// header include
#include "constant_flop_wrapper.hpp"

/// design_flows include
#include "design_flow_graph.hpp"
#include "design_flow_manager.hpp"
#include "design_flow_step.hpp"

/// frontend_analysis
#include "symbolic_application_frontend_flow_step.hpp"

/// Behavior include
#include "application_manager.hpp"
#include "behavioral_helper.hpp"
#include "call_graph.hpp"
#include "call_graph_manager.hpp"
#include "function_behavior.hpp"

/// Graph include
#include "basic_block.hpp"
#include "basic_blocks_graph_constructor.hpp"

/// Parameter include
#include "Parameter.hpp"

/// STD include
#include <boost/filesystem/fstream.hpp>
#include <fstream>

/// STL include
#include "custom_set.hpp"
#include <algorithm>
#include <set>
#include <string>

/// Tree include
#include "ext_tree_node.hpp"
#include "gcc_wrapper.hpp"
#include "tree_basic_block.hpp"
#include "tree_helper.hpp"
#include "tree_manager.hpp"
#include "tree_manipulation.hpp"
#include "tree_node.hpp"
#include "tree_reindex.hpp"

/// Utility include
#include "dbgPrintHelper.hpp"
#include "exceptions.hpp"

/// SoftFloat functions include
#include "config_PANDA_INCLUDE_INSTALLDIR.hpp"
#include "string_manipulation.hpp" // for GET_CLASS

static std::set<std::string> functions = {"__float32_addif", "__float32_subif", "__float32_mulif", "__float32_divif", "__float64_addif", "__float64_subif", "__float64_mulif", "__float64_divif"};

CustomOrderedSet<std::string> constant_flop_wrapper::operations;

constant_flop_wrapper::constant_flop_wrapper(const ParameterConstRef _parameters, const application_managerRef _AppM, unsigned int _function_id, const DesignFlowManagerConstRef _design_flow_manager)
    : FunctionFrontendFlowStep(_AppM, _function_id, CONSTANT_FLOP_WRAPPER, _design_flow_manager, _parameters), TreeM(_AppM->get_tree_manager()), tree_man(new tree_manipulation(TreeM, parameters))
{
   debug_level = parameters->get_class_debug_level(GET_CLASS(*this), DEBUG_LEVEL_NONE);
}

constant_flop_wrapper::~constant_flop_wrapper() = default;

const CustomUnorderedSet<std::pair<FrontendFlowStepType, FrontendFlowStep::FunctionRelationship>> constant_flop_wrapper::ComputeFrontendRelationships(const DesignFlowStep::RelationshipType relationship_type) const
{
   CustomUnorderedSet<std::pair<FrontendFlowStepType, FunctionRelationship>> relationships;
   switch(relationship_type)
   {
      case(DEPENDENCE_RELATIONSHIP):
      {
         relationships.insert(std::pair<FrontendFlowStepType, FunctionRelationship>(SOFT_FLOAT_CG_EXT, SAME_FUNCTION));
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

DesignFlowStep_Status constant_flop_wrapper::InternalExec()
{
   bool changed = false;
   const tree_nodeRef curr_tn = TreeM->GetTreeNode(function_id);
   auto* this_fd = GetPointer<function_decl>(curr_tn);
   auto* sl = GetPointer<statement_list>(GET_NODE(this_fd->body));
   CustomSet<std::pair<std::string, tree_nodeConstRef>> functions_to_be_created;
   for(const auto& block : sl->list_of_bloc)
   {
      for(const auto& stmt : block.second->CGetStmtList())
      {
#ifndef NDEBUG
         if(not AppM->ApplyNewTransformation())
         {
            break;
         }
#endif
         auto ga = GetPointer<gimple_assign>(GET_NODE(stmt));
         if(not ga)
         {
            continue;
         }
         const call_expr* ce = GetPointer<call_expr>(GET_NODE(ga->op1));
         if(not ce)
         {
            continue;
         }
         const tree_nodeRef f = GET_NODE(ce->fn);
         const addr_expr* ae = GetPointer<addr_expr>(f);
         const tree_nodeRef o = GET_NODE(ae->op);
         const function_decl* fd = GetPointer<function_decl>(o);
         const tree_nodeRef i = GET_NODE(fd->name);
         const identifier_node* in = GetPointer<identifier_node>(i);

         // the following three lines check if the functions written by this class are being analyzed multiple times (to avoid infinite loop)
         bool function_already_scanned = operations.find(in->strg) != operations.end();
         if(function_already_scanned)
         {
            THROW_ASSERT(function_already_scanned, "Inconsistent behaviour: the same function is being analyzed multiple times.");
         }

         if(functions.find(in->strg) != functions.end())
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Function is a soft float");
            THROW_ASSERT(ce->args.size() == 2, STR(ce->args.size()));
            const auto arg0 = GET_NODE(ce->args[0]);
            const auto arg1 = GET_NODE(ce->args[1]);
            if(arg0->get_kind() == real_cst_K)
            {
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Skipped because both the args are constant");
               continue;
            }
            if(arg1->get_kind() != real_cst_K)
            {
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Skipped because without constant arguments");
               continue;
            }
            functions_to_be_created.insert(std::pair<std::string, tree_nodeRef>(in->strg, arg1));
         }
      }
   }
   SoftFloatWriter(functions_to_be_created);
   for(const auto& block : sl->list_of_bloc)
   {
      for(const auto& stmt : block.second->CGetStmtList())
      {
#ifndef NDEBUG
         if(not AppM->ApplyNewTransformation())
         {
            break;
         }
#endif
         auto ga = GetPointer<gimple_assign>(GET_NODE(stmt));
         if(not ga)
         {
            continue;
         }
         const call_expr* ce = GetPointer<call_expr>(GET_NODE(ga->op1));
         if(not ce)
         {
            continue;
         }
         const tree_nodeRef f = GET_NODE(ce->fn);
         const addr_expr* ae = GetPointer<addr_expr>(f);
         const tree_nodeRef o = GET_NODE(ae->op);
         const function_decl* fd = GetPointer<function_decl>(o);
         const tree_nodeRef i = GET_NODE(fd->name);
         const identifier_node* in = GetPointer<identifier_node>(i);

         // the following three lines check if the functions written by this class are being analyzed multiple times (to avoid infinite loop)
         bool function_already_scanned = operations.find(in->strg) != operations.end();
         if(function_already_scanned)
         {
            THROW_ASSERT(function_already_scanned, "Inconsistent behaviour: the same function is being analyzed multiple times.");
         }

         if(functions.find(in->strg) != functions.end())
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Function is a soft float");
            THROW_ASSERT(ce->args.size() == 2, STR(ce->args.size()));
            const auto arg0 = GET_NODE(ce->args[0]);
            const auto arg1 = GET_NODE(ce->args[1]);
            if(arg0->get_kind() == real_cst_K)
            {
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Skipped because both the args are constant");
               continue;
            }
            if(arg1->get_kind() != real_cst_K)
            {
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Skipped because without constant arguments");
               continue;
            }
            const auto fu_name = GenerateFunctionName(in->strg, arg1);
            unsigned int called_function_id = TreeM->function_index(fu_name);
            THROW_ASSERT(called_function_id, "The library miss this function " + fu_name);
            std::vector<tree_nodeRef> argvs;
            argvs.push_back(TreeM->GetTreeReindex(arg0->index));
            TreeM->ReplaceTreeNode(stmt, ga->op1, tree_man->CreateCallExpr(TreeM->GetTreeReindex(called_function_id), argvs, ce->include_name + ":" + STR(ce->line_number) + ":" + STR(ce->column_number)));
            changed = true;
            bool body = true;
            if(TreeM->get_implementation_node(called_function_id))
               called_function_id = TreeM->get_implementation_node(called_function_id);
            else
               body = false;
#ifndef NDEBUG
            AppM->RegisterTransformation(GetName(), stmt);
#endif

            BehavioralHelperRef helper = BehavioralHelperRef(new BehavioralHelper(AppM, called_function_id, body, parameters));
            FunctionBehaviorRef FB = FunctionBehaviorRef(new FunctionBehavior(AppM, helper, parameters));
            AppM->GetCallGraphManager()->AddFunctionAndCallPoint(function_id, called_function_id, stmt->index, FB, FunctionEdgeInfo::CallType::direct_call);
            AppM->GetCallGraphManager()->RemoveCallPoint(function_id, fd->index, stmt->index);
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Removing call of " + STR(fd->index) + " (" + AppM->CGetFunctionBehavior(fd->index)->CGetBehavioralHelper()->get_function_name() + ") in " + STR(stmt->index));

            tree_nodeRef fun = TreeM->get_tree_node_const(called_function_id);
            auto* fud = GetPointer<function_decl>(fun);

            if(fud->scpe && GET_NODE(fud->scpe)->get_kind() == function_decl_K)
            {
               THROW_ERROR_CODE(NESTED_FUNCTIONS_EC, "Nested functions not yet supported " + boost::lexical_cast<std::string>(function_id));
            }
#ifndef NDEBUG
            for(const auto p : helper->get_parameters())
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Parameter " + STR(p));
#endif
            if(body)
            {
               INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "---Implementation node of this function is " + boost::lexical_cast<std::string>(TreeM->get_implementation_node(function_id)));
               THROW_ASSERT(fd->body, "Expected the function body");
            }
            else
            {
               INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "No body");
            }
            INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "<--Completed recursive analysis of not main function " + fu_name);
         }
      }
#ifndef NDEBUG
      if(not AppM->ApplyNewTransformation())
      {
         break;
      }
#endif
   }

   if(changed)
   {
      function_behavior->UpdateBBVersion();
   }
   return changed ? DesignFlowStep_Status::SUCCESS : DesignFlowStep_Status::UNCHANGED;
}

void constant_flop_wrapper::SoftFloatWriter(CustomSet<std::pair<std::string, tree_nodeConstRef>> function_to_be_generateds)
{
   bool generate_source_code = false;
   boost::filesystem::path full_path = parameters->getOption<std::string>(OPT_output_temporary_directory) + "/float_operations_" + function_behavior->CGetBehavioralHelper()->get_function_name() + ".c";
   boost::filesystem::ofstream new_file;

   new_file.open(full_path, boost::filesystem::ofstream::app);
   new_file << "#include \"" << PANDA_INCLUDE_INSTALLDIR << "/softfloat.c\"" << std::endl;
   for(const auto& function_to_be_generated : function_to_be_generateds)
   {
      const std::string function_header = GenerateFunctionName(function_to_be_generated.first, function_to_be_generated.second);
      if(operations.find(function_header) == operations.end())
      {
         operations.insert(function_header);
         generate_source_code = true;
         unsigned int bitsize = tree_helper::size(TreeM, function_to_be_generated.second->index);
         if(bitsize <= 32)
         {
            new_file << "float " << function_header << "(float variable){" << std::endl;
         }
         if(bitsize > 32 && bitsize <= 64)
         {
            new_file << "double " << function_header << "(double variable){" << std::endl;
         }
         if(bitsize > 64)
         {
            THROW_UNREACHABLE("");
         }
         new_file << "   return " << function_to_be_generated.first << "(variable, " << function_behavior->CGetBehavioralHelper()->print_constant(function_to_be_generated.second->index) << ");" << std::endl;
         new_file << "};" << std::endl;
      }
   }
   new_file.close();
   if(generate_source_code)
   {
      AppM->input_files.insert(std::pair<std::string, std::string>(full_path.string(), full_path.string()));
      std::map<std::string, std::string> input_file_operations;
      input_file_operations.insert(std::pair<std::string, std::string>(full_path.string(), full_path.string()));
      const GccWrapper_CompilerTarget compiler_target = parameters->getOption<GccWrapper_CompilerTarget>(OPT_default_compiler);
      const GccWrapper_OptimizationSet optimization_set = GccWrapper_OptimizationSet::OSF;
      const GccWrapperRef gcc_wrapper = GccWrapperRef(new GccWrapper(parameters, compiler_target, optimization_set));
      gcc_wrapper->FillTreeManager(TreeM, input_file_operations);
   }
}

std::string constant_flop_wrapper::GenerateFunctionName(const std::string& function_name, const tree_nodeConstRef constant)
{
   const auto* rc = GetPointer<const real_cst>(constant);

   std::string function_header = function_name + std::string("_") + rc->valr;
   std::replace(function_header.begin(), function_header.end(), '.', '_');
   std::replace(function_header.begin(), function_header.end(), '+', 'p');
   std::replace(function_header.begin(), function_header.end(), '-', 'm');
   return function_header;
}
