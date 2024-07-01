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
 *              Copyright (C) 2022-2024 Politecnico di Milano
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
#include "dbgPrintHelper.hpp"
#include "design_flow_graph.hpp"
#include "design_flow_manager.hpp"
#include "function_behavior.hpp"
#include "hls_device.hpp"
#include "hls_manager.hpp"
#include "hls_step.hpp"
#include "language_writer.hpp"
#include "library_manager.hpp"
#include "math_function.hpp"
#include "string_manipulation.hpp"
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
   ac_type,
   bool_type
};

struct InterfaceInfer::interface_info
{
   bool _fixed_size;

 public:
   std::string name;
   const std::string arg_id;
   const std::string interface_fname;
   unsigned alignment;
   unsigned long long bitwidth;
   unsigned long long factor;
   datatype type;

   FunctionArchitecture::parm_attrs& parm_attrs;
   FunctionArchitecture::iface_attrs& iface_attrs;

   interface_info(const std::string& _arg_id, const std::string& _interface_fname, bool fixed_size,
                  FunctionArchitecture::parm_attrs& _parm_attrs, FunctionArchitecture::iface_attrs& _iface_attrs)
       : _fixed_size(fixed_size),
         name(""),
         arg_id(_arg_id),
         interface_fname(_interface_fname),
         alignment(1U),
         bitwidth(0ULL),
         factor(1ULL),
         type(datatype::generic),
         parm_attrs(_parm_attrs),
         iface_attrs(_iface_attrs)
   {
   }

   void update(const tree_nodeRef& tn, const std::string& _type_name, ParameterConstRef parameters)
   {
      if(type != datatype::ac_type && type != datatype::bool_type)
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

         else if(std::regex_search(_type_name, std::regex("(_B|b)ool[&*]")))
         {
            _bitwidth = 1;
            type = datatype::bool_type;
            alignment = 8;
         }

         if(_fixed_size && bitwidth && bitwidth != _bitwidth)
         {
            THROW_ERROR("Unaligned access not allowed for required interface!");
         }
         bitwidth = std::max(bitwidth, _bitwidth);
      }
   }
};

static const std::regex signature_param_typename("((?:\\w+\\s*)+(?:<[^>]*>)?\\s*[\\*&\\/]*\\s*)");

static std::string get_decl_name(tree_nodeRef tn)
{
   const auto dn = GetPointer<const decl_node>(tn);
   THROW_ASSERT(dn, "Expected decl_node.");
   THROW_ASSERT(dn->name, "Expected declaration name.");
   return GetPointer<const identifier_node>(dn->name)->strg;
}

static std::vector<unsigned int> GetSortedRoots(const CallGraphManagerConstRef& CGM)
{
   std::vector<unsigned int> sorted_roots;

   const auto CG = CGM->CGetCallGraph();
   std::vector<vertex> sorted_cg;
   sorted_cg.reserve(boost::num_vertices(*CG));
   boost::topological_sort(*CG, std::back_inserter(sorted_cg));
   auto root_fid = CGM->GetRootFunctions();
   for(auto v : sorted_cg)
   {
      const auto fid = CGM->get_function(v);
      auto r_it = root_fid.find(fid);
      if(r_it != root_fid.end())
      {
         root_fid.erase(r_it);
         sorted_roots.push_back(fid);
      }
   }

   return sorted_roots;
}

static std::tuple<unsigned int, unsigned int> GetCallStmt(const CallGraphManagerConstRef& CGM, unsigned int fid)
{
   const auto fv = CGM->GetVertex(fid);
   const auto CG = CGM->CGetCallGraph();
   const auto [calls, calls_end] = boost::in_edges(fv, *CG);
   if(std::distance(calls, calls_end) == 1)
   {
      const auto edge_info = CG->CGetFunctionEdgeInfo(*calls);
      if(edge_info->direct_call_points.size() == 1)
      {
         const auto call_id = *edge_info->direct_call_points.begin();
         return {CGM->get_function(boost::source(*calls, *CG)), call_id};
      }
   }
   return {0, 0};
}

static std::vector<tree_nodeRef> GetCallArgs(tree_nodeRef stmt)
{
   if(const auto ga = GetPointer<const gimple_assign>(stmt))
   {
      const auto ce = GetPointer<const call_expr>(ga->op1);
      return ce->args;
   }
   else if(const auto gc = GetPointer<const gimple_call>(stmt))
   {
      return gc->args;
   }
   THROW_UNREACHABLE("Unexpected call statement.");
   return std::vector<tree_nodeRef>();
}

static std::pair<tree_nodeRef, unsigned int> ResolvePointerAlias(const CallGraphManagerConstRef& CGM,
                                                                 const tree_managerConstRef& TM,
                                                                 const tree_nodeRef& var, unsigned int fid,
                                                                 std::vector<tree_nodeRef>* field_offset)
{
   const auto base_var = tree_helper::GetBaseVariable(var, field_offset);
   if(const auto pd = GetPointer<const parm_decl>(base_var))
   {
      const auto [caller_id, call_id] = GetCallStmt(CGM, fid);
      if(caller_id)
      {
         const auto fd = GetPointer<const function_decl>(TM->GetTreeNode(fid));
         const auto parm_idx = static_cast<size_t>(std::distance(
             fd->list_of_args.begin(), std::find_if(fd->list_of_args.begin(), fd->list_of_args.end(),
                                                    [&](const auto& tn) { return tn->index == base_var->index; })));
         THROW_ASSERT(parm_idx < fd->list_of_args.size(), "Parameter not found.");
         const auto call_args = GetCallArgs(TM->GetTreeNode(call_id));
         THROW_ASSERT(call_args.size() == fd->list_of_args.size(),
                      "Expected formal and actual parameters' count match.");
         auto retVal = ResolvePointerAlias(CGM, TM, call_args.at(parm_idx), caller_id, field_offset);
         if(parm_idx == 0)
         {
            retVal.second = fid;
         }
         return retVal;
      }
   }
   return {base_var, fid};
}

InterfaceInfer::InterfaceInfer(const application_managerRef _AppM, const DesignFlowManagerConstRef _design_flow_manager,
                               const ParameterConstRef _parameters)
    : ApplicationFrontendFlowStep(_AppM, INTERFACE_INFER, _design_flow_manager, _parameters), already_executed(false)
{
   debug_level = parameters->get_class_debug_level(GET_CLASS(*this));
}

InterfaceInfer::~InterfaceInfer() = default;

CustomUnorderedSet<std::pair<FrontendFlowStepType, FrontendFlowStep::FunctionRelationship>>
InterfaceInfer::ComputeFrontendRelationships(const DesignFlowStep::RelationshipType relationship_type) const
{
   CustomUnorderedSet<std::pair<FrontendFlowStepType, FunctionRelationship>> relationships;
   switch(relationship_type)
   {
      case(DEPENDENCE_RELATIONSHIP):
      {
         relationships.insert(std::make_pair(DATAFLOW_CG_EXT, ALL_FUNCTIONS));
         relationships.insert(std::make_pair(FUNCTION_ANALYSIS, WHOLE_APPLICATION));
         relationships.insert(std::make_pair(IR_LOWERING, ALL_FUNCTIONS));
         relationships.insert(std::make_pair(PARM2SSA, ALL_FUNCTIONS));
         relationships.insert(std::make_pair(USE_COUNTING, ALL_FUNCTIONS));
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
             design_flow_manager.lock()->CGetDesignFlowStepFactory(DesignFlowStep::TECHNOLOGY));
         const auto technology_flow_signature =
             TechnologyFlowStep::ComputeSignature(TechnologyFlowStep_Type::LOAD_TECHNOLOGY);
         const auto technology_flow_step = design_flow_manager.lock()->GetDesignFlowStep(technology_flow_signature);
         const auto technology_design_flow_step =
             technology_flow_step != DesignFlowGraph::null_vertex() ?
                 design_flow_graph->CGetNodeInfo(technology_flow_step)->design_flow_step :
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

bool InterfaceInfer::HasToBeExecuted() const
{
   return !already_executed;
}

DesignFlowStep_Status InterfaceInfer::Exec()
{
   const auto HLSMgr = GetPointer<HLS_manager>(AppM);
   THROW_ASSERT(HLSMgr, "");
   const auto TM = AppM->get_tree_manager();
   const auto CGM = AppM->CGetCallGraphManager();
   const auto CG = CGM->CGetCallGraph();
   const auto sorted_roots = GetSortedRoots(CGM);

   std::set<unsigned int> modified;
   const auto add_to_modified = [&](const tree_nodeRef& tn) {
      modified.insert(GetPointer<gimple_node>(tn)->scpe->index);
   };

   for(const auto root_id : sorted_roots)
   {
      const auto fnode = TM->GetTreeNode(root_id);
      const auto fd = GetPointer<const function_decl>(fnode);
      const auto fsymbol = tree_helper::GetMangledFunctionName(fd);
      INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, output_level, "-->Analyzing function " + fsymbol);

      /* Check if there is a typename corresponding to fname */
      auto func_arch = HLSMgr->module_arch->GetArchitecture(fsymbol);
      if(!func_arch)
      {
         func_arch = refcount<FunctionArchitecture>(new FunctionArchitecture());

         const auto fsign = [&]() {
            const auto cxa_fname = cxa_demangle(fsymbol);
            if(cxa_fname.empty())
            {
               return tree_helper::PrintType(TM, fd->type, false, true);
            }
            return cxa_fname;
         }();
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Extracting interface from signature " + fsymbol);
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Demangled as " + fsign);
         size_t parm_index = 0;
         std::sregex_token_iterator typename_it(fsign.begin(), fsign.end(), signature_param_typename, 0), end;
         func_arch->attrs[FunctionArchitecture::func_symbol] = fsymbol;
         func_arch->attrs[FunctionArchitecture::func_name] = tree_helper::GetFunctionName(TM, fnode);
         ++typename_it; // First match is the function name/pointer type
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Iterating arguments:");
         for(const auto& arg : fd->list_of_args)
         {
            THROW_ASSERT(typename_it != end, "");
            const auto parm_name = [&]() {
               std::stringstream ss;
               ss << arg;
               return ss.str();
            }();
            auto& iface_attrs = func_arch->ifaces[parm_name];
            iface_attrs[FunctionArchitecture::iface_name] = parm_name;
            iface_attrs[FunctionArchitecture::iface_mode] = "default";
            auto& parm_attrs = func_arch->parms[parm_name];
            const std::string tname(*typename_it);
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Argument " + parm_name + " : " + tname);
            parm_attrs[FunctionArchitecture::parm_port] = parm_name;
            parm_attrs[FunctionArchitecture::parm_typename] = parm_attrs[FunctionArchitecture::parm_original_typename] =
                tname;
            parm_attrs[FunctionArchitecture::parm_index] = std::to_string(parm_index);
            parm_attrs[FunctionArchitecture::parm_bundle] = parm_name;
            if(tname.find("_fixed<") != std::string::npos)
            {
               parm_attrs[FunctionArchitecture::parm_includes] +=
                   STR(";" PANDA_DATA_INSTALLDIR "/panda/ac_types/include/" + tname.substr(0, 2) + "_fixed.h");
            }
            if(tname.find("_int<") != std::string::npos)
            {
               parm_attrs[FunctionArchitecture::parm_includes] +=
                   STR(";" PANDA_DATA_INSTALLDIR "/panda/ac_types/include/" + tname.substr(0, 2) + "_int.h");
            }
            if(tname.find("ac_channel<") != std::string::npos)
            {
               parm_attrs[FunctionArchitecture::parm_includes] +=
                   STR(";" PANDA_DATA_INSTALLDIR "/panda/ac_types/include/ac_channel.h");
            }
            ++typename_it;
            ++parm_index;
         }
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--");
         HLSMgr->module_arch->AddArchitecture(fsymbol, func_arch);
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--");
      }

      const tree_manipulationRef tree_man(new tree_manipulation(TM, parameters, AppM));
      const auto is_dataflow_module =
          func_arch->attrs.find(FunctionArchitecture::func_dataflow_module) != func_arch->attrs.end() &&
          (func_arch->attrs.find(FunctionArchitecture::func_dataflow_module)->second == "1");
      if(is_dataflow_module)
      {
         const auto [caller_id, call_id] = GetCallStmt(CGM, root_id);
         THROW_ASSERT(call_id, "Expected unique call point for dataflow module " + fsymbol + ".");
         const auto call_stmt = TM->GetTreeNode(call_id);
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Call point " + STR(call_stmt));
         const auto& args = GetCallArgs(call_stmt);

         size_t idx = 0;
         for(const auto& arg : args)
         {
            if(tree_helper::IsPointerType(arg))
            {
               std::vector<tree_nodeRef> field_offset;
               unsigned int offset = 0;
               const auto [base_var, owner_id] = ResolvePointerAlias(CGM, TM, arg, caller_id, &field_offset);
               if(!field_offset.empty())
               {
                  for(const auto& fld : field_offset)
                  {
                     auto offsetNode = fld;
                     if(offsetNode->get_kind() == integer_cst_K)
                     {
                        auto offsetValue = GetPointerS<const integer_cst>(offsetNode);
                        offset += static_cast<unsigned>(offsetValue->value);
                     }
                     else
                     {
                        THROW_ERROR("Non-constant-offset for DATAFLOW bundle");
                     }
                  }
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Offset equal to " + STR(offset));
               }
               const auto parm_attr = std::find_if(func_arch->parms.begin(), func_arch->parms.end(), [&](auto& it) {
                  return it.second.at(FunctionArchitecture::parm_index) == std::to_string(idx);
               });
               THROW_ASSERT(parm_attr != func_arch->parms.end(), "Expected parameter index " + std::to_string(idx));
               auto& current_bundle = parm_attr->second.at(FunctionArchitecture::parm_bundle);
               std::string bundle_name;
               if(base_var->get_kind() == parm_decl_K)
               {
                  func_arch->ifaces[current_bundle].at(FunctionArchitecture::iface_mode) = "default";
                  INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, output_level,
                                 "---Parameter " + parm_attr->second.at(FunctionArchitecture::parm_port) +
                                     " forwarded from caller function");
                  ++idx;
                  continue;
               }
               else if(GetPointer<const cst_node>(base_var))
               {
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                 "---Parameter " + parm_attr->second.at(FunctionArchitecture::parm_port) +
                                     " is constant at this call point");
                  ++idx;
                  continue;
               }
               else if(const auto dn = GetPointer<const decl_node>(base_var))
               {
                  bundle_name = "DF_bambu_" + std::to_string(owner_id) + "_" + std::to_string(base_var->index) + "FO" +
                                STR(offset);
               }
               else
               {
                  THROW_UNREACHABLE("Unexpected declaration.");
               }
               func_arch->ifaces[bundle_name] = func_arch->ifaces.at(current_bundle);
               func_arch->ifaces[bundle_name].at(FunctionArchitecture::iface_name) = bundle_name;
               func_arch->ifaces.erase(current_bundle);
               current_bundle = bundle_name;
               THROW_ASSERT(fd->list_of_args.size() > idx, "Unexpected function parameters count");
               auto pd = GetPointerS<parm_decl>(fd->list_of_args.at(idx));
               pd->name = tree_man->create_identifier_node(bundle_name);
               INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, output_level,
                              "---Parameter " + parm_attr->second.at(FunctionArchitecture::parm_port) +
                                  " renamed and bound to internal interface " + STR(base_var) + " - " + bundle_name);
               parm_attr->second.at(FunctionArchitecture::parm_port) = bundle_name;
               func_arch->parms[bundle_name] = parm_attr->second;
               func_arch->parms.erase(parm_attr);
            }
            ++idx;
         }
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--");

         INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, output_level, "---Generating control flow loop for dataflow module");
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->");
         const auto sl = GetPointer<statement_list>(fd->body);
         THROW_ASSERT(sl->list_of_bloc.count(BB_ENTRY) && sl->list_of_bloc.count(BB_EXIT),
                      "Expected entry and exit basic blocks for function " + fsymbol);
         const auto& bb_entry = sl->list_of_bloc.at(BB_ENTRY);
         THROW_ASSERT(bb_entry->list_of_succ.size() == 1, "Expected single entry basic block for function " + fsymbol);
         const auto& bb_first = sl->list_of_bloc.at(bb_entry->list_of_succ.front());
         for(auto& [bbi, bb] : sl->list_of_bloc)
         {
            if(std::find(bb->list_of_succ.begin(), bb->list_of_succ.end(), BB_EXIT) != bb->list_of_succ.end())
            {
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                              "---BB" + std::to_string(bbi) + " loop-back to entry BB" +
                                  std::to_string(bb_first->number));
               bb->list_of_succ.clear();
               bb->list_of_succ.push_back(bb_first->number);
               bb_first->list_of_pred.push_back(bbi);
               const auto return_stmt = bb->CGetStmtList().back();
               THROW_ASSERT(GetPointer<const gimple_return>(return_stmt),
                            "Expected return statement as last statement");
               bb->RemoveStmt(return_stmt, AppM);
            }
         }
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--");
      }

      std::map<std::string, TreeNodeSet> bundle_vdefs;
      for(const auto& arg : fd->list_of_args)
      {
         const auto arg_id = arg->index;
         const auto arg_name = get_decl_name(arg);
         const auto arg_type = tree_helper::CGetType(arg);
         INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, output_level, "---Parameter @" + STR(arg_id) + " " + arg_name);
         THROW_ASSERT(func_arch->parms.find(arg_name) != func_arch->parms.end(),
                      "Not matched parameter name: " + arg_name);
         auto& parm_attrs = func_arch->parms.at(arg_name);
         THROW_ASSERT(parm_attrs.find(FunctionArchitecture::parm_bundle) != parm_attrs.end(),
                      "Missing parameter bundle name");
         parm_attrs.emplace(FunctionArchitecture::parm_offset, "off");
         if(tree_helper::IsPointerType(arg_type))
         {
            const auto ptd_type = tree_helper::CGetPointedType(arg_type);
            const auto array_size =
                tree_helper::IsArrayType(ptd_type) ? tree_helper::GetArrayTotalSize(ptd_type) : 1ULL;
            parm_attrs.emplace(FunctionArchitecture::parm_size_in_bytes,
                               std::to_string(array_size * (get_aligned_bitsize(tree_helper::Size(arg_type)) >> 3)));
         }
         THROW_ASSERT(func_arch->ifaces.find(parm_attrs.at(FunctionArchitecture::parm_bundle)) !=
                          func_arch->ifaces.end(),
                      "Missing parameter bundle: " + parm_attrs.at(FunctionArchitecture::parm_bundle));
         auto& iface_attrs = func_arch->ifaces.at(parm_attrs.at(FunctionArchitecture::parm_bundle));
         iface_attrs[FunctionArchitecture::iface_direction] = port_o::GetString(port_o::IN);
         iface_attrs[FunctionArchitecture::iface_bitwidth] = STR(tree_helper::Size(arg_type));
         iface_attrs[FunctionArchitecture::iface_alignment] =
             std::to_string(get_aligned_bitsize(tree_helper::Size(arg_type)) >> 3);
         auto& interface_type = iface_attrs[FunctionArchitecture::iface_mode];
         THROW_ASSERT(parameters->getOption<HLSFlowStep_Type>(OPT_interface_type) !=
                              HLSFlowStep_Type::INFERRED_INTERFACE_GENERATION ||
                          interface_type.size(),
                      "Expected interface type for parameter '" + arg_name + "'");
         if(parameters->getOption<HLSFlowStep_Type>(OPT_interface_type) !=
                HLSFlowStep_Type::INFERRED_INTERFACE_GENERATION ||
            interface_type == "bus")
         {
            interface_type = "default";
         }
         else if(interface_type != "default")
         {
            const auto arg_ssa_id = AppM->getSSAFromParm(root_id, arg_id);
            const auto arg_ssa = TM->GetTreeNode(arg_ssa_id);
            THROW_ASSERT(arg_ssa->get_kind() == ssa_name_K, "");
            bool unused_port = false;
            if(GetPointerS<const ssa_name>(arg_ssa)->CGetUseStmts().empty())
            {
               THROW_WARNING("Parameter '" + arg_name + "' not used by any statement");
               if(tree_helper::IsPointerType(arg_type))
               {
                  unused_port = true;
               }
               else
               {
                  THROW_ERROR("parameter not used: specified interface does not make sense - " + interface_type);
               }
            }
            if(tree_helper::IsPointerType(arg_type))
            {
               INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "---Is a pointer type");
               interface_info info(arg_name, fsymbol, interface_type != "m_axi", parm_attrs, iface_attrs);
               info.update(arg_ssa, parm_attrs.at(FunctionArchitecture::parm_typename), parameters);

               std::list<tree_nodeRef> writeStmt;
               std::list<tree_nodeRef> readStmt;
               ChasePointerInterface(arg_ssa, writeStmt, readStmt, info);
               const auto isRead = !readStmt.empty();
               const auto isWrite = !writeStmt.empty();

               if(!isRead && !isWrite)
               {
                  if(starts_with(arg_name, "DF_bambu_"))
                  {
                     continue;
                  }
                  if(!unused_port)
                  {
                     unused_port = true;
                     THROW_WARNING("Parameter '" + arg_name + "' not used by any statement");
                  }
               }
               if(unused_port && info.type == datatype::generic)
               {
                  info.bitwidth = 8ULL * std::stoull(parm_attrs.at(FunctionArchitecture::parm_size_in_bytes));
               }

               info.factor = std::max(
                   info.type == datatype::generic ?
                       (8ULL * std::stoull(parm_attrs.at(FunctionArchitecture::parm_size_in_bytes))) / info.bitwidth :
                       1ULL,
                   1ULL);
               info.name = [&]() -> std::string {
                  if(isRead && isWrite)
                  {
                     iface_attrs[FunctionArchitecture::iface_direction] = port_o::GetString(port_o::IO);
                     INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "---I/O interface");
                     if(interface_type == "array")
                     {
                        if(info.factor > 1)
                        {
                           parm_attrs[FunctionArchitecture::parm_elem_count] = STR(info.factor);
                        }
                        return "array";
                     }
                     else if(interface_type == "ptrdefault")
                     {
                        if(info.factor > 1)
                        {
                           parm_attrs[FunctionArchitecture::parm_elem_count] = STR(info.factor);
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
                           iface_attrs[FunctionArchitecture::iface_register] = "";
                           return "none";
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
                     iface_attrs[FunctionArchitecture::iface_direction] = port_o::GetString(port_o::IN);
                     INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "---Read-only interface");
                     if(interface_type == "array")
                     {
                        if(info.factor > 1)
                        {
                           parm_attrs[FunctionArchitecture::parm_elem_count] = STR(info.factor);
                        }
                        return "array";
                     }
                     else if(interface_type == "ptrdefault")
                     {
                        if(info.factor > 1)
                        {
                           parm_attrs[FunctionArchitecture::parm_elem_count] = STR(info.factor);
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
                     iface_attrs[FunctionArchitecture::iface_direction] = port_o::GetString(port_o::OUT);
                     INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "---Write-only interface");
                     if(interface_type == "array")
                     {
                        if(info.factor > 1)
                        {
                           parm_attrs[FunctionArchitecture::parm_elem_count] = std::to_string(info.factor);
                        }
                        return "array";
                     }
                     else if(interface_type == "ptrdefault")
                     {
                        if(info.factor > 1)
                        {
                           parm_attrs[FunctionArchitecture::parm_elem_count] = std::to_string(info.factor);
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
                           iface_attrs[FunctionArchitecture::iface_register] = "";
                           return "none";
                        }
                        return "valid";
                     }
                  }
                  else if(unused_port && interface_type == "ptrdefault")
                  {
                     return "none";
                  }
                  return interface_type;
               }();
               iface_attrs[FunctionArchitecture::iface_bitwidth] = std::to_string(info.bitwidth);
               iface_attrs[FunctionArchitecture::iface_alignment] = std::to_string(info.alignment);
               if(interface_type == "fifo" || interface_type == "axis")
               {
                  iface_attrs.emplace(FunctionArchitecture::iface_depth, "0");
               }
               interface_type = info.name;

               THROW_ASSERT(info.bitwidth, "Expected non-zero bitwidth");

               std::set<std::string> operationsR, operationsW;
               const auto interface_datatype = tree_man->GetCustomIntegerType(info.bitwidth, true);
               const auto commonRWSignature = interface_type == "array" || interface_type == "m_axi";
               const auto& bundle_name = iface_attrs.at(FunctionArchitecture::iface_name);
               const auto require_flush =
                   interface_type == "m_axi" &&
                   iface_attrs.find(FunctionArchitecture::iface_cache_line_count) != iface_attrs.end();
               const auto store_vdef = [&](const tree_nodeRef& stmt) {
                  if(require_flush)
                  {
                     const auto& vdef = GetPointerS<const gimple_node>(stmt)->vdef;
                     if(vdef)
                     {
                        bundle_vdefs[bundle_name].insert(vdef);
                     }
                  }
               };
               for(const auto& stmt : readStmt)
               {
                  setReadInterface(stmt, bundle_name, operationsR, commonRWSignature, interface_datatype, tree_man, TM);
                  add_to_modified(stmt);
                  store_vdef(stmt);
               }
               for(const auto& stmt : writeStmt)
               {
                  setWriteInterface(stmt, bundle_name, operationsW, commonRWSignature, interface_datatype, tree_man,
                                    TM);
                  add_to_modified(stmt);
                  store_vdef(stmt);
               }
               create_resource(operationsR, operationsW, info, func_arch, unused_port, root_id);
            }
            else if(interface_type == "none")
            {
               THROW_ERROR("Interface type '" + interface_type + "' for parameter '" + arg_name + "' unexpected");
            }
            else
            {
               THROW_ERROR("Interface type '" + interface_type + "' for parameter '" + arg_name + "' is not supported");
            }
         }
         INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, output_level, "-->Interface specification:");
         INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, output_level, "---Parameter : " + arg_name);
         INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, output_level, "---Protocol  : " + interface_type);
         INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, output_level,
                        "---Direction : " + iface_attrs.at(FunctionArchitecture::iface_direction));
         INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, output_level,
                        "---Bundle    : " + parm_attrs.at(FunctionArchitecture::parm_bundle));
         INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, output_level,
                        "---Bitwidth  : " + iface_attrs.at(FunctionArchitecture::iface_bitwidth));
         if(parm_attrs.find(FunctionArchitecture::parm_elem_count) != parm_attrs.end())
         {
            INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, output_level,
                           "---Size      : " + parm_attrs.at(FunctionArchitecture::parm_elem_count));
         }
         INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, output_level,
                        "---Alignment : " + iface_attrs.at(FunctionArchitecture::iface_alignment));
         INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, output_level, "<--");
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
            const auto fini_call = tree_man->create_gimple_call(fini_fd, args, fd->scpe->index, BUILTIN_SRCP);
            const auto fini_node = GetPointerS<gimple_node>(fini_call);
            for(const auto& vdef : vdefs)
            {
               fini_node->AddVuse(vdef);
            }
            return fini_call;
         };

         const auto sl = GetPointerS<const statement_list>(fd->body);
         for(const auto& bbi_bb : sl->list_of_bloc)
         {
            const auto& bb = bbi_bb.second;
            const auto is_last = std::count(bb->list_of_succ.cbegin(), bb->list_of_succ.cend(), BB_EXIT);
            if(bb->number != BB_ENTRY && is_last)
            {
               const auto fini_call = generate_fini_call();
               THROW_ASSERT(bb->CGetStmtList().size(), "BB" + STR(bb->number) + " should not be empty");
               const auto last_stmt = bb->CGetStmtList().back();
               if(GetPointer<const gimple_return>(last_stmt))
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

      INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, output_level, "<--Analyzed function " + fsymbol);
   }

   // Remove interface information for non interfaced functions to avoid issues with aggressive IR optimizations
   // (signature modification, SROA, ...)
   for(auto it = HLSMgr->module_arch->cbegin(); it != HLSMgr->module_arch->cend();)
   {
      const auto fnode = TM->GetFunction(it->first);
      if(!fnode || (it->second->attrs.size() <= 2 &&
                    std::find(sorted_roots.begin(), sorted_roots.end(), fnode->index) == sorted_roots.end()))
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                        "Erase function architecture for function " + it->first);
         it = HLSMgr->module_arch->erase(it);
      }
      else
      {
         ++it;
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
         const auto fd_kind = fd_node->get_kind();
         auto& fn = fd_node;
         if(fd_kind == addr_expr_K)
         {
            fn = GetPointerS<const addr_expr>(fd_node)->op;
         }
         THROW_ASSERT(fn->get_kind() == function_decl_K, "unexpected condition: " + fn->get_kind_text());
         return GetPointerS<const function_decl>(fn);
      }();
      if(!call_fd->body)
      {
         const auto called_fname = [&]() {
            const auto fname = tree_helper::print_function_name(TM, call_fd);
            const auto demangled = cxa_demangle(fname);
            return demangled.size() ? demangled : fname;
         }();
         if(called_fname.find(STR_CST_interface_parameter_keyword) != std::string::npos)
         {
            const auto bundle_id = called_fname.substr(0, called_fname.find(STR_CST_interface_parameter_keyword));
            if(bundle_id != info.arg_id && bundle_id != info.parm_attrs.at(FunctionArchitecture::parm_bundle))
            {
               THROW_ERROR("Pattern not supported with required I/O interface: parameters '" + bundle_id + "' and '" +
                           info.arg_id + "' share a memory operation");
            }
            return call_type::ct_forward;
         }
         else if(called_fname.find("ac_channel") != std::string::npos)
         {
            if(called_fname.find("::_read") != std::string::npos)
            {
               if(call_fd->list_of_args.size() >= 1 && call_fd->list_of_args.size() <= 3)
               {
                  auto ret_type = call_fd->type;
                  if(ret_type->get_kind() == function_type_K || ret_type->get_kind() == method_type_K)
                  {
                     const auto ft = GetPointerS<const function_type>(ret_type);
                     if(ft->retn->get_kind() != void_type_K)
                     {
                        ssa_var = ft->retn;
                     }
                     else
                     {
                        THROW_ASSERT(call_fd->list_of_args.size() >= 2, "unexpected condition");
                        THROW_ASSERT(call_args.size() >= 2, "unexpected condition");
                        auto first_arg = call_args.at(0);
                        THROW_ASSERT(tree_helper::IsPointerType(first_arg), "unexpected condition");
                        ssa_var = tree_helper::GetBaseVariable(first_arg);
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
                  auto ret_type = call_fd->type;
                  if(ret_type->get_kind() == void_type_K)
                  {
                     THROW_ERROR("unexpected condition");
                  }
                  if(tree_helper::IsPointerType(call_fd->list_of_args.at(1)))
                  {
                     auto second_arg = call_args.at(1);
                     ssa_var = tree_helper::GetBaseVariable(second_arg);
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
         THROW_UNREACHABLE("Hardware function " + called_fname + " interfacing not supported.");
      }

      size_t par_index = 0U;
      for(auto use_idx = 0U; use_idx < use_count; ++use_idx, ++par_index)
      {
         // look for the actual vs formal parameter binding
         par_index = [&](size_t start_idx) {
            for(auto idx = start_idx; idx < call_args.size(); ++idx)
            {
               if(call_args[idx]->index == arg_var->index)
               {
                  return idx;
               }
            }
            THROW_ERROR("Use of " + arg_var->ToString() + " not found.");
            return static_cast<size_t>(-1);
         }(par_index);
         THROW_ASSERT(call_fd->list_of_args.size() > par_index, "unexpected condition");
         const auto called_param = call_fd->list_of_args.at(par_index);

         const auto call_arg_ssa_id = AppM->getSSAFromParm(call_fd->index, called_param->index);
         const auto call_arg_ssa = TM->GetTreeNode(call_arg_ssa_id);
         THROW_ASSERT(call_arg_ssa->get_kind() == ssa_name_K, "");
         if(GetPointerS<const ssa_name>(call_arg_ssa)->CGetUseStmts().size())
         {
            forwardInterface(fd_node, called_param, info);

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
   if(!Visited.insert(ssa_node->index).second)
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
      const auto ssa_var = GetPointer<const ssa_name>(ssa_node);
      chase_ssa.pop();
      INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level,
                     "-->SSA VARIABLE: " + ssa_var->ToString() + " with " + STR(ssa_var->CGetUseStmts().size()) +
                         " use statements");
      for(const auto& stmt_count : ssa_var->CGetUseStmts())
      {
         const auto use_stmt = stmt_count.first;
         const auto& use_count = stmt_count.second;
         INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "---STMT: " + use_stmt->ToString());
         if(const auto ga = GetPointer<const gimple_assign>(use_stmt))
         {
            const auto op0_kind = ga->op0->get_kind();
            const auto op1_kind = ga->op1->get_kind();
            if(op0_kind == mem_ref_K)
            {
               if(op1_kind == mem_ref_K)
               {
                  THROW_ERROR("Pattern currently not supported: *x=*y; " + use_stmt->ToString());
               }
               else
               {
                  THROW_ASSERT(op1_kind == ssa_name_K || GetPointer<const cst_node>(ga->op1), "unexpected condition");
                  if(GetPointer<const cst_node>(ga->op1) || GetPointer<const ssa_name>(ga->op1) != ssa_var)
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
               const auto ce = GetPointerS<const call_expr>(ga->op1);
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
                             use_stmt->ToString() + ":" + ga->op1->get_kind_text());
            }
            else
            {
               if(!tree_helper::IsPointerType(ga->op0))
               {
                  THROW_WARNING("Pattern potentially not supported: parameter converted to non-pointer type: " +
                                use_stmt->ToString() + ":" + ga->op1->get_kind_text());
               }
               else if(op1_kind != nop_expr_K && op1_kind != view_convert_expr_K && op1_kind != ssa_name_K &&
                       op1_kind != pointer_plus_expr_K && op1_kind != cond_expr_K)
               {
                  THROW_WARNING("Pattern potentially not supported: parameter used in a non-supported statement: " +
                                use_stmt->ToString() + ":" + ga->op1->get_kind_text());
               }
               ChasePointerInterfaceRecurse(Visited, ga->op0, writeStmt, readStmt, info);
            }
         }
         else if(const auto gc = GetPointer<const gimple_call>(use_stmt))
         {
            THROW_ASSERT(gc->fn, "unexpected condition");
            const auto fn_node = gc->fn;
            if(fn_node->get_kind() == addr_expr_K)
            {
               const auto ae = GetPointerS<const addr_expr>(fn_node);
               const auto ae_op = ae->op;
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

void InterfaceInfer::forwardInterface(const tree_nodeRef& fnode, const tree_nodeRef& parm_node,
                                      const interface_info& info)
{
   const auto fd = GetPointer<const function_decl>(fnode);
   const auto fsymbol = tree_helper::GetMangledFunctionName(fd);
   const auto func_arch = GetPointerS<HLS_manager>(AppM)->module_arch->GetArchitecture(fsymbol);
   if(func_arch)
   {
      // Update function architecture information on called function
      const auto pname = get_decl_name(parm_node);
      auto bname = func_arch->parms.at(pname).at(FunctionArchitecture::parm_bundle);
#if HAVE_ASSERTS
      const auto ecount =
#endif
          func_arch->parms.erase(pname);
      THROW_ASSERT(ecount, "Expected parameter information for parameter " + pname + " in function " + fsymbol);
      const auto is_bundle_used =
          std::any_of(func_arch->parms.begin(), func_arch->parms.end(), [&](const auto& name_attrs) {
             return name_attrs.second.at(FunctionArchitecture::parm_bundle) == bname;
          });
      if(!is_bundle_used)
      {
         func_arch->ifaces.erase(bname);
      }
      THROW_ASSERT(func_arch->parms.find(pname) == func_arch->parms.end(),
                   "Duplicate parameter name " + pname + " in " + fsymbol + ".");
      bname = info.iface_attrs.at(FunctionArchitecture::iface_name);
      func_arch->parms[pname] = info.parm_attrs;
      func_arch->parms[pname].at(FunctionArchitecture::parm_port) = pname;
      func_arch->parms[pname].at(FunctionArchitecture::parm_bundle) = bname;
      if(func_arch->ifaces.find(bname) == func_arch->ifaces.end())
      {
         func_arch->ifaces[bname] = info.iface_attrs;
      }
      else
      {
         THROW_ASSERT(func_arch->ifaces.find(bname)->second.at(FunctionArchitecture::iface_mode) ==
                          info.iface_attrs.at(FunctionArchitecture::iface_mode),
                      "Duplicate interface bundle name " + bname + " in " + fsymbol + ".");
      }
   }
}

void InterfaceInfer::setReadInterface(tree_nodeRef stmt, const std::string& arg_name,
                                      std::set<std::string>& operationsR, bool commonRWSignature,
                                      tree_nodeConstRef interface_datatype, const tree_manipulationRef tree_man,
                                      const tree_managerRef TM)
{
   const auto gn = GetPointerS<gimple_node>(stmt);
   THROW_ASSERT(gn->scpe && gn->scpe->get_kind() == function_decl_K, "expected a function_decl scope");
   const auto fd = GetPointerS<function_decl>(gn->scpe);
   const auto fname = tree_helper::GetMangledFunctionName(fd);
   INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "-->LOAD from " + fname + ":");
   INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "---BEFORE: " + stmt->ToString());

   THROW_ASSERT(fd->body, "expected a body");
   const auto sl = GetPointerS<statement_list>(fd->body);
   const auto curr_bb = sl->list_of_bloc.at(gn->bb_index);
   const auto ret_call =
       stmt->get_kind() == gimple_assign_K && GetPointerS<gimple_assign>(stmt)->op1->get_kind() == call_expr_K;
   const auto ref_call = stmt->get_kind() == gimple_call_K;
   if(ret_call || ref_call)
   {
      bool valid_var;
      tree_nodeRef valid_ptr;
      tree_nodeConstRef data_type;
      tree_nodeRef stmt_op0;
      tree_nodeRef data_ptr;
      if(ret_call)
      {
         const auto ga = GetPointerS<const gimple_assign>(stmt);
         stmt_op0 = ga->op0;
         const auto ce = GetPointerS<const call_expr>(ga->op1);
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
         const auto gc = GetPointerS<const gimple_call>(stmt);
         valid_var = gc->args.size() == 3;
         auto first_arg = gc->args.at(0);
         THROW_ASSERT(tree_helper::IsPointerType(first_arg), "unexpected condition");
         data_ptr = first_arg;
         auto data_obj = tree_helper::GetBaseVariable(first_arg);
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
      THROW_ASSERT(!valid_ptr || tree_helper::IsPointerType(valid_ptr), "Valid type must be bool pointer");
      THROW_ASSERT(!gn->memdef && !gn->memuse, "");

      const auto data_size = tree_helper::Size(interface_datatype);
      const auto sel_type = tree_man->GetBooleanType();
      const auto ret_type = tree_man->GetCustomIntegerType(data_size + 1, true);
      const auto out_ptr_type = tree_man->GetPointerType(interface_datatype);
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
      const auto retval = GetPointerS<const gimple_assign>(new_call)->op0;
      auto ga_mask = tree_man->CreateNopExpr(retval, interface_datatype, nullptr, nullptr, fd->index);
      curr_bb->PushBefore(ga_mask, stmt, AppM);
      INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "---  MASK: " + ga_mask->ToString());
      auto data_mask = GetPointerS<const gimple_assign>(ga_mask)->op0;
      if(tree_helper::IsRealType(data_type))
      {
         const auto vc = tree_man->create_unary_operation(data_type, data_mask, BUILTIN_SRCP, view_convert_expr_K);
         ga_mask = tree_man->CreateGimpleAssign(data_type, nullptr, nullptr, vc, fd->index, BUILTIN_SRCP);
         curr_bb->PushBefore(ga_mask, stmt, AppM);
         INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "---  VIEW: " + ga_mask->ToString());
         data_mask = GetPointerS<const gimple_assign>(ga_mask)->op0;
      }

      tree_nodeRef last_stmt;
      if(ret_call)
      {
         TM->ReplaceTreeNode(ga_mask, data_mask, stmt_op0);
         INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "---   FIX: " + ga_mask->ToString());
         last_stmt = ga_mask;
      }
      else
      {
         const auto data_ptr_type = tree_helper::CGetType(data_ptr);
         const auto data_ref = tree_man->create_binary_operation(
             interface_datatype, data_ptr, TM->CreateUniqueIntegerCst(0, data_ptr_type), BUILTIN_SRCP, mem_ref_K);
         const auto ga_store = tree_man->create_gimple_modify_stmt(data_ref, data_mask, fd->index, BUILTIN_SRCP);
         if(valid_var)
         {
            const auto vdef = tree_man->create_ssa_name(tree_nodeRef(), tree_man->GetVoidType(), tree_nodeRef(),
                                                        tree_nodeRef(), false, true);
            GetPointerS<gimple_assign>(ga_store)->SetVdef(vdef);
            for(const auto& vuse : GetPointerS<gimple_assign>(stmt)->vuses)
            {
               if(GetPointerS<gimple_node>(ga_store)->AddVuse(vuse))
               {
                  GetPointerS<ssa_name>(vuse)->AddUseStmt(ga_store);
               }
            }
            THROW_ASSERT(gn->vdef, "Expected virtual ssa definition on store operation: " + gn->ToString());
            auto vdef_ssa = GetPointerS<ssa_name>(gn->vdef);
            for(const auto& usingStmt : vdef_ssa->CGetUseStmts())
            {
               if(GetPointerS<gimple_node>(usingStmt.first)->AddVuse(vdef))
               {
                  GetPointerS<ssa_name>(vdef)->AddUseStmt(usingStmt.first);
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
         const auto v_shift = GetPointerS<const gimple_assign>(ga_vshift)->op0;
         const auto valid_ptd_type = tree_man->GetBooleanType();
         const auto valid_type = tree_man->GetPointerType(valid_ptd_type);
         const auto ga_valid = tree_man->CreateNopExpr(v_shift, valid_ptd_type, nullptr, nullptr, fd->index);
         curr_bb->PushBefore(ga_valid, stmt, AppM);
         INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "--- VALID: " + ga_valid->ToString());
         const auto valid_ref = GetPointerS<const gimple_assign>(ga_valid)->op0;
         const auto valid_memref = tree_man->create_binary_operation(
             valid_ptd_type, valid_ptr, TM->CreateUniqueIntegerCst(0, valid_type), BUILTIN_SRCP, mem_ref_K);
         const auto ga_valid_store =
             tree_man->create_gimple_modify_stmt(valid_memref, valid_ref, fd->index, BUILTIN_SRCP);
         curr_bb->Replace(stmt, ga_valid_store, true, AppM);
         if(!ret_call)
         {
            auto ssaDef = GetPointerS<gimple_assign>(last_stmt)->vdef;
            if(GetPointerS<gimple_node>(ga_valid_store)->AddVuse(ssaDef))
            {
               GetPointerS<ssa_name>(ssaDef)->AddUseStmt(ga_valid_store);
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
      THROW_ASSERT(stmt && stmt->get_kind() == gimple_assign_K, "unexpected condition");
      const auto ga = GetPointerS<gimple_assign>(stmt);
      THROW_ASSERT(ga->op1->get_kind() == mem_ref_K, "unexpected condition");

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

      THROW_ASSERT(ga->op1->get_kind() == mem_ref_K, "unexpected condition");
      const auto mr = GetPointerS<const mem_ref>(ga->op1);
      args.push_back(mr->op0);

      const auto ce = tree_man->CreateCallExpr(fdecl_node, args, BUILTIN_SRCP);
      if(tree_helper::IsSameType(interface_datatype, actual_type))
      {
         TM->ReplaceTreeNode(stmt, ga->op1, ce);
         CustomUnorderedSet<unsigned int> AV;
         CallGraphManager::addCallPointAndExpand(AV, AppM, ga->scpe->index, fdecl_node->index, stmt->index,
                                                 FunctionEdgeInfo::CallType::direct_call, DEBUG_LEVEL_NONE);
         GetPointer<HLS_manager>(AppM)->design_interface_io[fname][ga->bb_index][arg_name].push_back(stmt->index);
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
         GetPointer<HLS_manager>(AppM)->design_interface_io[fname][curr_bb->number][arg_name].push_back(gc->index);

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
   const auto gn = GetPointerS<gimple_node>(stmt);
   THROW_ASSERT(gn->scpe && gn->scpe->get_kind() == function_decl_K, "expected a function_decl scope");
   const auto fd = GetPointerS<function_decl>(gn->scpe);
   const auto fname = tree_helper::GetMangledFunctionName(fd);
   INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "-->STORE from " + fname + ":");
   INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "---BEFORE: " + stmt->ToString());

   THROW_ASSERT(fd->body, "expected a body");
   const auto sl = GetPointerS<statement_list>(fd->body);
   const auto curr_bb = sl->list_of_bloc.at(gn->bb_index);
   const auto ret_call =
       stmt->get_kind() == gimple_assign_K && GetPointerS<gimple_assign>(stmt)->op1->get_kind() == call_expr_K;
   const auto ref_call = stmt->get_kind() == gimple_call_K;
   if(ret_call || ref_call)
   {
      tree_nodeRef data_obj;
      tree_nodeRef data_ptr;
      tree_nodeConstRef ret_type;
      bool valid_var;
      bool isAStruct = false;
      if(ret_call)
      {
         const auto ga = GetPointerS<const gimple_assign>(stmt);
         const auto ce = GetPointerS<const call_expr>(ga->op1);
         THROW_ASSERT(ce->args.size() == 2, "unexpected condition");
         data_obj = ce->args.at(1);
         if(tree_helper::IsPointerType(data_obj))
         {
            data_ptr = data_obj;
            data_obj = tree_helper::GetBaseVariable(data_obj);
            THROW_ASSERT(data_obj != data_ptr, "unexpected condition");
            isAStruct = true;
         }
         valid_var = true;
         ret_type = tree_helper::CGetType(ga->op0);
      }
      else
      {
         const auto gc = GetPointerS<const gimple_call>(stmt);
         THROW_ASSERT(gc->args.size() == 2, "unexpected condition");
         data_obj = gc->args.at(1);
         if(tree_helper::IsPointerType(data_obj))
         {
            data_ptr = data_obj;
            data_obj = tree_helper::GetBaseVariable(data_obj);
            THROW_ASSERT(data_obj != data_ptr, "unexpected condition");
            isAStruct = true;
         }
         valid_var = false;
         ret_type = tree_man->GetVoidType();
      }

      const auto data_type = tree_helper::CGetType(data_obj);
      const auto sel_type = tree_man->GetBooleanType();
      const auto out_ptr_type = tree_man->GetPointerType(interface_datatype);
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
         const auto out_data_ptr = GetPointerS<const gimple_assign>(ga_ptr)->op0;
         const auto data_ref = tree_man->create_binary_operation(
             interface_datatype, out_data_ptr, TM->CreateUniqueIntegerCst(0, out_ptr_type), BUILTIN_SRCP, mem_ref_K);
         const auto ga_load =
             tree_man->CreateGimpleAssign(interface_datatype, nullptr, nullptr, data_ref, fd->index, BUILTIN_SRCP);
         curr_bb->Replace(stmt, ga_load, true, AppM);
         INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "---  LOAD: " + ga_load->ToString());
         data_obj = GetPointerS<const gimple_assign>(ga_load)->op0;
         lastStmt = ga_load;
      }
      else if(tree_helper::IsRealType(data_type))
      {
         const auto vc = tree_man->create_unary_operation(data_type, data_obj, BUILTIN_SRCP, view_convert_expr_K);
         const auto ga_vc =
             tree_man->CreateGimpleAssign(interface_datatype, nullptr, nullptr, vc, fd->index, BUILTIN_SRCP);
         curr_bb->PushBefore(ga_vc, stmt, AppM);
         INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "---  VIEW: " + ga_vc->ToString());
         data_obj = GetPointerS<const gimple_assign>(ga_vc)->op0;
      }

      std::vector<tree_nodeRef> args;
      args.push_back(TM->CreateUniqueIntegerCst(valid_var, sel_type));
      args.push_back(data_obj);
      if(valid_var)
      {
         const auto ga_stmt = GetPointerS<const gimple_assign>(stmt);
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
      THROW_ASSERT(stmt && stmt->get_kind() == gimple_assign_K, "unexpected condition");
      const auto ga = GetPointerS<gimple_assign>(stmt);
      THROW_ASSERT(ga->op0->get_kind() == mem_ref_K, "unexpected condition");

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
            nop = tree_man->CreateNopExpr(value_node, interface_datatype, nullptr, nullptr, ga->scpe->index);
            value_node = GetPointerS<const gimple_assign>(nop)->op0;
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
      const auto mr = GetPointerS<const mem_ref>(ga->op0);
      args.push_back(mr->op0);

      const auto gc = tree_man->create_gimple_call(fdecl_node, args, ga->scpe->index, BUILTIN_SRCP);
      curr_bb->Replace(stmt, gc, true, AppM);
      GetPointer<HLS_manager>(AppM)->design_interface_io[fname][curr_bb->number][arg_name].push_back(gc->index);

      INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "--- AFTER: " + gc->ToString());
   }
   INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "<--");
}

void InterfaceInfer::create_resource_Read_simple(const std::set<std::string>& operations, const interface_info& info,
                                                 FunctionArchitectureRef func_arch, bool IO_port, bool unused_port,
                                                 unsigned root_id) const
{
   if(operations.empty() && !unused_port)
   {
      return;
   }
   const auto& parm_attrs = func_arch->parms.at(info.arg_id);
   const auto& bundle_name = parm_attrs.at(FunctionArchitecture::parm_bundle);
   const std::string ResourceName = ENCODE_FDNAME(bundle_name, "_Read_", info.name);
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
      CM->add_port(CLOCK_PORT_NAME, port_o::IN, interface_top, bool_type);
      CM->add_port(RESET_PORT_NAME, port_o::IN, interface_top, bool_type);
      CM->add_port_vector(START_PORT_NAME, port_o::IN, 1U, interface_top, bool_type);
      if(is_unbounded)
      {
         CM->add_port_vector(DONE_PORT_NAME, port_o::OUT, 1U, interface_top, bool_type);
      }
      const auto addrPort = CM->add_port("in1", port_o::IN, interface_top, addrType);
      GetPointerS<port_o>(addrPort)->set_is_addr_bus(true);
      GetPointerS<port_o>(addrPort)->set_port_alignment(info.alignment);
      CM->add_port("out1", port_o::OUT, interface_top, outType);

      std::string port_data_name = "_" + info.arg_id;
      port_o::port_interface port_if = port_o::port_interface::PI_RNONE;
      if(if_name == "axis")
      {
         port_data_name = "_" + info.arg_id + "_TDATA";
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
             CM->add_port("_" + info.arg_id + (IO_port ? "_i" : "") + "_ack", port_o::OUT, interface_top, bool_type);
         GetPointerS<port_o>(inPort_o_ack)->set_port_interface(port_o::port_interface::PI_RACK);
      }
      if(if_name == "valid" || if_name == "handshake")
      {
         const auto inPort_o_vld =
             CM->add_port("_" + info.arg_id + (IO_port ? "_i" : "") + "_vld", port_o::IN, interface_top, bool_type);
         GetPointerS<port_o>(inPort_o_vld)->set_port_interface(port_o::port_interface::PI_RVALID);
      }
      if(if_name == "fifo")
      {
         const auto inPort_empty_n = CM->add_port("_" + info.arg_id + "_empty_n", port_o::IN, interface_top, bool_type);
         GetPointerS<port_o>(inPort_empty_n)->set_port_interface(port_o::port_interface::PI_EMPTY_N);
         const auto inPort_read = CM->add_port("_" + info.arg_id + "_read", port_o::OUT, interface_top, bool_type);
         GetPointerS<port_o>(inPort_read)->set_port_interface(port_o::port_interface::PI_READ);
      }
      if(if_name == "axis")
      {
         const auto inPort_empty_n = CM->add_port("_" + info.arg_id + "_TVALID", port_o::IN, interface_top, bool_type);
         GetPointerS<port_o>(inPort_empty_n)->set_port_interface(port_o::port_interface::PI_S_AXIS_TVALID);
         const auto inPort_read = CM->add_port("_" + info.arg_id + "_TREADY", port_o::OUT, interface_top, bool_type);
         GetPointerS<port_o>(inPort_read)->set_port_interface(port_o::port_interface::PI_S_AXIS_TREADY);
      }

      CM->add_NP_functionality(interface_top, NP_functionality::LIBRARY, "in1 out1");
      CM->add_NP_functionality(interface_top, NP_functionality::VERILOG_GENERATOR,
                               "Read_" + if_name + "ModuleGenerator");
      TechMan->add_resource(INTERFACE_LIBRARY, ResourceName, CM);
      if(unused_port)
      {
         HLSMgr->unused_interfaces[root_id].insert(std::make_pair(INTERFACE_LIBRARY, ResourceName));
      }
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

void InterfaceInfer::create_resource_Write_simple(const std::set<std::string>& operations, const interface_info& info,
                                                  FunctionArchitectureRef func_arch, bool IO_port) const
{
   if(operations.empty())
   {
      return;
   }
   const auto& parm_attrs = func_arch->parms.at(info.arg_id);
   const auto& bundle_name = parm_attrs.at(FunctionArchitecture::parm_bundle);
   const std::string ResourceName = ENCODE_FDNAME(bundle_name, "_Write_", info.name);
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

      CM->add_port(CLOCK_PORT_NAME, port_o::IN, interface_top, bool_type);
      CM->add_port(RESET_PORT_NAME, port_o::IN, interface_top, bool_type);
      CM->add_port_vector(START_PORT_NAME, port_o::IN, 1U, interface_top, bool_type);
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
      std::string port_data_name = "_" + info.arg_id;
      port_o::port_interface port_if = port_o::port_interface::PI_WNONE;
      if(if_name == "axis")
      {
         port_data_name = "_" + info.arg_id + "_TDATA";
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
             CM->add_port("_" + info.arg_id + (IO_port ? "_o" : "") + "_ack", port_o::IN, interface_top, bool_type);
         GetPointerS<port_o>(inPort_o_ack)->set_port_interface(port_o::port_interface::PI_WACK);
      }
      if(if_name == "valid" || if_name == "handshake")
      {
         const auto inPort_o_vld =
             CM->add_port("_" + info.arg_id + (IO_port ? "_o" : "") + "_vld", port_o::OUT, interface_top, bool_type);
         GetPointerS<port_o>(inPort_o_vld)->set_port_interface(port_o::port_interface::PI_WVALID);
      }
      if(if_name == "fifo")
      {
         const auto inPort_full_n = CM->add_port("_" + info.arg_id + "_full_n", port_o::IN, interface_top, bool_type);
         GetPointerS<port_o>(inPort_full_n)->set_port_interface(port_o::port_interface::PI_FULL_N);
         const auto inPort_read = CM->add_port("_" + info.arg_id + "_write", port_o::OUT, interface_top, bool_type);
         GetPointerS<port_o>(inPort_read)->set_port_interface(port_o::port_interface::PI_WRITE);
      }
      if(if_name == "axis")
      {
         const auto inPort_full_n = CM->add_port("_" + info.arg_id + "_TREADY", port_o::IN, interface_top, bool_type);
         GetPointerS<port_o>(inPort_full_n)->set_port_interface(port_o::port_interface::PI_M_AXIS_TREADY);
         const auto inPort_read = CM->add_port("_" + info.arg_id + "_TVALID", port_o::OUT, interface_top, bool_type);
         GetPointerS<port_o>(inPort_read)->set_port_interface(port_o::port_interface::PI_M_AXIS_TVALID);
      }

      CM->add_NP_functionality(interface_top, NP_functionality::LIBRARY, "in1 in2 in3");
      const auto writer = parameters->getOption<HDLWriter_Language>(OPT_writer_language);
      if(if_name == "none" && writer == HDLWriter_Language::VHDL)
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

      const auto& iface_attrs = func_arch->ifaces.at(bundle_name);
      const auto is_registered = iface_attrs.find(FunctionArchitecture::iface_register) != iface_attrs.end();
      for(const auto& fdName : operations)
      {
         const auto op_bounded = fdName.find("Async") != std::string::npos || !is_unbounded;
         const auto exec_time = (op_bounded ? 0.0 : HLS_D->get_technology_manager()->CGetSetupHoldTime()) + EPSILON;
         const auto cycles = op_bounded ? (1U + is_registered) : 0U;

         const auto op = GetPointerS<operation>(fu->get_operation(fdName));
         op->time_m = time_info::factory(parameters);
         op->bounded = op_bounded;
         op->time_m->set_execution_time(exec_time, cycles);
         if(op_bounded && is_registered)
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
                                           const std::set<std::string>& operationsW, const interface_info& info,
                                           FunctionArchitectureRef func_arch, bool unused_port, unsigned root_id) const
{
   const auto n_channels = parameters->getOption<unsigned int>(OPT_channels_number);
   const auto isDP = n_channels == 2;
   const auto n_resources = isDP ? 2U : 1U;
   const auto read_write_string = (isDP ? std::string("ReadWriteDP_") : std::string("ReadWrite_"));
   const auto HLSMgr = GetPointerS<HLS_manager>(AppM);
   THROW_ASSERT(func_arch->parms.find(info.arg_id) != func_arch->parms.end(),
                "Missing parameter '" + info.arg_id + "' from function architecute.");
   const auto& parm_attrs = func_arch->parms.at(info.arg_id);
   const auto& bundle_name = parm_attrs.at(FunctionArchitecture::parm_bundle);
   const auto ResourceName = ENCODE_FDNAME(bundle_name, "", "");
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
      THROW_ASSERT(parm_attrs.find(FunctionArchitecture::parm_elem_count) != parm_attrs.end(),
                   "Missing parameter element count for parameter '" + info.arg_id + "'.");
      const auto arraySize = std::stoull(parm_attrs.at(FunctionArchitecture::parm_elem_count));
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
      if(!operationsR.empty() || unused_port)
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
      if(unused_port)
      {
         HLSMgr->unused_interfaces[root_id].insert(std::make_pair(INTERFACE_LIBRARY, ResourceName));
      }
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
                                           const std::set<std::string>& operationsW, const interface_info& info,
                                           FunctionArchitectureRef func_arch, bool unused_port,
                                           unsigned int root_id) const
{
   THROW_ASSERT(GetPointer<HLS_manager>(AppM), "");
   const auto HLSMgr = GetPointerS<HLS_manager>(AppM);
   THROW_ASSERT(func_arch->parms.find(info.arg_id) != func_arch->parms.end(),
                "Missing parameter '" + info.arg_id + "' from function architecute.");
   const auto& parm_attrs = func_arch->parms.at(info.arg_id);
   const auto& bundle_name = parm_attrs.at(FunctionArchitecture::parm_bundle);
   const auto ResourceName = ENCODE_FDNAME(bundle_name, "", "");
   const auto HLS_D = HLSMgr->get_HLS_device();
   const auto TechMan = HLS_D->get_technology_manager();

   if(!TechMan->is_library_manager(INTERFACE_LIBRARY) ||
      !TechMan->get_library_manager(INTERFACE_LIBRARY)->is_fu(ResourceName))
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level,
                     "-->Creating interface resource: " + INTERFACE_LIBRARY + ":" + ResourceName);
      const auto& iface_attrs = func_arch->ifaces.at(bundle_name);
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
      const auto backEndBitsize =
          iface_attrs.find(FunctionArchitecture::iface_cache_bus_size) != iface_attrs.end() ?
              std::stoull(iface_attrs.find(FunctionArchitecture::iface_cache_bus_size)->second) :
              info.bitwidth;

      const structural_type_descriptorRef address_interface_datatype(
          new structural_type_descriptor("bool", address_bitsize));
      const structural_type_descriptorRef size1(new structural_type_descriptor("bool", 1));
      const structural_type_descriptorRef rwsize(new structural_type_descriptor("bool", nbitDataSize));
      const structural_type_descriptorRef rwtypeIn(new structural_type_descriptor("bool", info.bitwidth));
      const structural_type_descriptorRef rwtypeOut(new structural_type_descriptor("bool", backEndBitsize));
      const structural_type_descriptorRef idType(new structural_type_descriptor("bool", 6));
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

      bool has_slave = false;
      bool has_direct = false;
      for(auto& p : func_arch->parms)
      {
         if(p.second.at(FunctionArchitecture::parm_bundle) == bundle_name)
         {
            const auto& parm_offset = p.second.at(FunctionArchitecture::parm_offset);
            if(parm_offset == "slave")
            {
               has_slave = true;
            }
            else if(parm_offset == "direct")
            {
               has_direct = true;
               const auto offset_port =
                   CM->add_port("_" + p.first, port_o::IN, interface_top, address_interface_datatype);
               GetPointerS<port_o>(offset_port)->set_port_interface(port_o::port_interface::PI_M_AXI_DIRECT);
            }
         }
      }
      if(!has_direct)
      {
         THROW_ERROR("only 'direct' axi interfaces are supported");
      }
      if(has_slave)
      {
         const auto s_awvalid = CM->add_port("_s_axi_AXILiteS_AWVALID", port_o::IN, interface_top, bool_type);
         GetPointerS<port_o>(s_awvalid)->set_port_interface(port_o::port_interface::S_AXIL_AWVALID);
         const auto s_awaddr =
             CM->add_port("_s_axi_AXILiteS_AWADDR", port_o::IN, interface_top, address_interface_datatype);
         GetPointerS<port_o>(s_awaddr)->set_port_interface(port_o::port_interface::S_AXIL_AWADDR);
         const auto s_wvalid = CM->add_port("_s_axi_AXILiteS_WVALID", port_o::IN, interface_top, bool_type);
         GetPointerS<port_o>(s_wvalid)->set_port_interface(port_o::port_interface::S_AXIL_WVALID);
         const auto s_wdata =
             CM->add_port("_s_axi_AXILiteS_WDATA", port_o::IN, interface_top, address_interface_datatype);
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
         const auto s_rdata =
             CM->add_port("_s_axi_AXILiteS_RDATA", port_o::OUT, interface_top, address_interface_datatype);
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

      TechMan->add_resource(INTERFACE_LIBRARY, ResourceName, CM);
      if(unused_port)
      {
         HLSMgr->unused_interfaces[root_id].insert(std::make_pair(INTERFACE_LIBRARY, ResourceName));
      }
      const auto fu = GetPointerS<functional_unit>(TechMan->get_fu(ResourceName, INTERFACE_LIBRARY));
      fu->area_m = area_info::factory(parameters);
      fu->area_m->set_area_value(0);
      if(iface_attrs.find(FunctionArchitecture::iface_cache_line_count) != iface_attrs.end())
      {
         const auto flushName = ENCODE_FDNAME(bundle_name, "_Flush_", "m_axi");
         TechMan->add_operation(INTERFACE_LIBRARY, ResourceName, flushName);
         const auto op = GetPointer<operation>(fu->get_operation(flushName));
         op->time_m = time_info::factory(parameters);
         op->bounded = false;
         op->time_m->set_execution_time(HLS_D->get_technology_manager()->CGetSetupHoldTime() + EPSILON, 0);
         op->time_m->set_synthesis_dependent(true);
      }
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
}

void InterfaceInfer::create_resource(const std::set<std::string>& operationsR, const std::set<std::string>& operationsW,
                                     const interface_info& info, FunctionArchitectureRef func_arch, bool unused_port,
                                     unsigned root_id) const
{
   if(info.name == "none" || info.name == "acknowledge" || info.name == "valid" || info.name == "ovalid" ||
      info.name == "handshake" || info.name == "fifo" || info.name == "axis")
   {
      THROW_ASSERT(!operationsR.empty() || !operationsW.empty() || unused_port, "unexpected condition");
      const auto IO_P = !operationsR.empty() && !operationsW.empty();
      create_resource_Read_simple(operationsR, info, func_arch, IO_P, unused_port, root_id);
      create_resource_Write_simple(operationsW, info, func_arch, IO_P);
   }
   else if(info.name == "array")
   {
      create_resource_array(operationsR, operationsW, info, func_arch, unused_port, root_id);
   }
   else if(info.name == "m_axi")
   {
      create_resource_m_axi(operationsR, operationsW, info, func_arch, unused_port, root_id);
   }
   else
   {
      THROW_ERROR("interface not supported: " + info.name);
   }
}
