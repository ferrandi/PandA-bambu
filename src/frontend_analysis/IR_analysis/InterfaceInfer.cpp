/*
 *
 *        _/_/_/    _/_/   _/    _/ _/_/_/    _/_/
 *       _/   _/ _/    _/ _/_/  _/ _/   _/ _/    _/
 *      _/_/_/  _/_/_/_/ _/  _/_/ _/   _/ _/_/_/_/
 *     _/      _/    _/ _/    _/ _/   _/ _/    _/
 *    _/      _/    _/ _/    _/ _/_/_/  _/    _/
 *
 *  ***********************************************
 *                   PandA Project
 *   URL: https://github.com/ferrandi/PandA-bambu
 *            Politecnico di Milano - DEIB
 *             System Architectures Group
 *  ***********************************************
 *   Copyright (C) 2022-2026 Politecnico di Milano
 *
 * Part of the PandA Project, under the Apache License v2.0 with LLVM Exceptions.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
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

#include "CompilerWrapper.hpp"
#include "IR/function_ordered_instructions.hpp"
#include "Parameter.hpp"
#include "application_manager.hpp"
#include "area_info.hpp"
#include "behavioral_helper.hpp"
#include "call_graph.hpp"
#include "call_graph_manager.hpp"
#include "constant_strings.hpp"
#include "copyrights_strings.hpp"
#include "dbgPrintHelper.hpp"
#include "design_flow_graph.hpp"
#include "design_flow_manager.hpp"
#include "function_behavior.hpp"
#include "hls_device.hpp"
#include "hls_manager.hpp"
#include "hls_step.hpp"
#include "ir_basic_block.hpp"
#include "ir_helper.hpp"
#include "ir_manager.hpp"
#include "ir_manipulation.hpp"
#include "ir_node.hpp"
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
#include "var_pp_functor.hpp"

#include <algorithm>
#include <limits>
#include <memory>
#include <regex>
#include <sstream>

#include "config_PANDA_INCLUDE_INSTALLDIR.hpp"

#define EPSILON 0.000000001
#define ENCODE_FDNAME(arg_name, MODE, interface_type) \
   ((arg_name) + STR_CST_interface_parameter_keyword + (MODE) + (interface_type))

namespace
{
   using OrderedInstructionsCache = std::map<unsigned int, std::unique_ptr<FunctionOrderedInstructions>>;

   bool isPointerDerivation(const ir_nodeRef& stmt, const ir_nodeRef& pointer)
   {
      if(stmt->get_kind() != assign_stmt_K)
      {
         return false;
      }
      const auto ga = GetPointerS<const assign_stmt>(stmt);
      if(ga->op0->get_kind() != ssa_node_K || !ir_helper::IsPointerType(ga->op0))
      {
         return false;
      }
      if(ga->op1->get_kind() == bitcast_node_K || ga->op1->get_kind() == nop_node_K)
      {
         return GetPointerS<const unary_node>(ga->op1)->op->index == pointer->index;
      }
      return ga->op1->get_kind() == gep_node_K && GetPointerS<const binary_node>(ga->op1)->op0->index == pointer->index;
   }

   bool collectAggregateMemoryAccesses(const ir_nodeRef& pointer, const ir_nodeRef& interface_stmt,
                                       const bool collect_stores, std::vector<ir_nodeRef>& accesses,
                                       std::vector<ir_nodeRef>& pointer_derivations,
                                       std::set<unsigned int>& visited_pointers)
   {
      if(!visited_pointers.insert(pointer->index).second)
      {
         return true;
      }
      for(const auto& use : GetPointerS<const ssa_node>(pointer)->CGetUseStmts())
      {
         const auto use_stmt = use.first;
         if(use_stmt->index == interface_stmt->index)
         {
            continue;
         }
         if(isPointerDerivation(use_stmt, pointer))
         {
            pointer_derivations.push_back(use_stmt);
            if(!collectAggregateMemoryAccesses(GetPointerS<const assign_stmt>(use_stmt)->op0, interface_stmt,
                                               collect_stores, accesses, pointer_derivations, visited_pointers))
            {
               return false;
            }
            continue;
         }
         if(use_stmt->get_kind() != assign_stmt_K)
         {
            return false;
         }
         const auto ga = GetPointerS<const assign_stmt>(use_stmt);
         const auto memory_operand = collect_stores ? ga->op0 : ga->op1;
         if(memory_operand->get_kind() != mem_access_node_K ||
            GetPointerS<const mem_access_node>(memory_operand)->op->index != pointer->index)
         {
            return false;
         }
         accesses.push_back(use_stmt);
      }
      return true;
   }

   bool getConstantPointerBitOffset(const ir_nodeRef& pointer, const ir_nodeRef& expected_base,
                                    unsigned long long& bit_offset)
   {
      std::vector<ir_nodeRef> offsets;
      const auto base = ir_helper::GetBaseVariable(pointer, &offsets);
      if(!base || base->index != expected_base->index)
      {
         return false;
      }
      unsigned long long byte_offset = 0;
      for(const auto& offset : offsets)
      {
         if(offset->get_kind() != constant_int_val_node_K)
         {
            return false;
         }
         const auto current_offset = static_cast<unsigned long long>(ir_helper::GetConstValue(offset));
         if(current_offset > (std::numeric_limits<unsigned long long>::max() - byte_offset))
         {
            return false;
         }
         byte_offset += current_offset;
      }
      if(byte_offset > (std::numeric_limits<unsigned long long>::max() / 8))
      {
         return false;
      }
      bit_offset = byte_offset * 8;
      return true;
   }

   bool hasRegularAggregateElementLayout(std::vector<std::pair<unsigned long long, unsigned long long>> element_ranges,
                                         const unsigned long long payload_size)
   {
      std::sort(element_ranges.begin(), element_ranges.end());
      if(element_ranges.size() == 1)
      {
         return element_ranges.front().first == 0 && element_ranges.front().second == payload_size;
      }
      if(element_ranges.size() < 2 || element_ranges.front().first != 0)
      {
         return false;
      }
      const auto element_size = element_ranges.front().second - element_ranges.front().first;
      const auto element_stride = element_ranges.at(1).first;
      if(!element_size || element_stride < element_size || payload_size % element_stride != 0 ||
         element_ranges.size() != payload_size / element_stride)
      {
         return false;
      }
      for(size_t element_index = 0; element_index < element_ranges.size(); ++element_index)
      {
         const auto expected_begin = element_index * element_stride;
         if(element_ranges.at(element_index).first != expected_begin ||
            element_ranges.at(element_index).second != expected_begin + element_size)
         {
            return false;
         }
      }
      return true;
   }

   void removeDeadAggregateTemporary(const ir_nodeRef& data_ptr, const std::vector<ir_nodeRef>& pointer_derivations,
                                     const statement_list_node* sl, const application_managerRef& AppM)
   {
      for(auto it = pointer_derivations.rbegin(); it != pointer_derivations.rend(); ++it)
      {
         const auto derivation = *it;
         const auto result = GetPointerS<const assign_stmt>(derivation)->op0;
         if(GetPointerS<const ssa_node>(result)->CGetUseStmts().empty())
         {
            const auto derivation_bb = sl->list_of_bloc.at(GetPointerS<const node_stmt>(derivation)->bb_index);
            derivation_bb->RemoveStmt(derivation, AppM);
         }
      }
      const auto data_ptr_ssa = GetPointerS<const ssa_node>(data_ptr);
      if(data_ptr_ssa->CGetUseStmts().empty())
      {
         const auto data_ptr_def = data_ptr_ssa->GetDefStmt();
         if(data_ptr_def && data_ptr_def->get_kind() == assign_stmt_K &&
            GetPointerS<const assign_stmt>(data_ptr_def)->op1->get_kind() == addr_node_K)
         {
            const auto data_ptr_def_bb = sl->list_of_bloc.at(GetPointerS<const node_stmt>(data_ptr_def)->bb_index);
            data_ptr_def_bb->RemoveStmt(data_ptr_def, AppM);
         }
      }
   }

   bool isScalarPointerProtocolWithoutReadback(const std::string& interface_name)
   {
      return interface_name == "none" || interface_name == "valid" || interface_name == "ovalid" ||
             interface_name == "handshake" || interface_name == "acknowledge";
   }

   bool hasWriteBeforeRead(const std::list<ir_nodeRef>& writeStmt, const std::list<ir_nodeRef>& readStmt,
                           const application_managerRef& AppM, OrderedInstructionsCache& ordered_instructions)
   {
      const auto get_ordered_instructions = [&](const ir_nodeRef& stmt) -> FunctionOrderedInstructions& {
         const auto stmt_node = GetPointerS<const node_stmt>(stmt);
         THROW_ASSERT(stmt_node->parent && stmt_node->parent->get_kind() == function_val_node_K,
                      "expected a function_val_node scope");
         const auto fd = GetPointerS<const function_val_node>(stmt_node->parent);
         THROW_ASSERT(fd->body, "expected a body");
         auto oi_it = ordered_instructions.find(fd->index);
         if(oi_it == ordered_instructions.end())
         {
            oi_it =
                ordered_instructions.emplace(fd->index, std::make_unique<FunctionOrderedInstructions>(AppM, fd->index))
                    .first;
         }
         return *oi_it->second;
      };

      for(const auto& write_stmt : writeStmt)
      {
         const auto write_stmt_node = GetPointerS<const node_stmt>(write_stmt);
         for(const auto& read_stmt : readStmt)
         {
            const auto read_stmt_node = GetPointerS<const node_stmt>(read_stmt);
            if(write_stmt_node->parent == read_stmt_node->parent &&
               get_ordered_instructions(write_stmt).dominates(write_stmt, read_stmt))
            {
               return true;
            }
         }
      }
      return false;
   }

   std::string getHDLGeneratorNameToken(const std::string& interface_name)
   {
      if(interface_name == "DP_array")
      {
         return "DPArray";
      }
      if(interface_name == "m_axi")
      {
         return "MAXI";
      }

      std::string token;
      std::stringstream ss(interface_name);
      std::string chunk;
      while(std::getline(ss, chunk, '_'))
      {
         token += capitalize(chunk);
      }
      return token;
   }

   void addVirtualUseToStatement(const ir_nodeRef& stmt, const ir_nodeRef& vssa)
   {
      if(!vssa)
      {
         return;
      }
      const auto stmt_node = GetPointerS<node_stmt>(stmt);
      if(stmt_node->AddVuse(vssa))
      {
         GetPointerS<ssa_node>(vssa)->AddUseStmt(stmt);
      }
   }

   void addVirtualOverToStatement(const ir_nodeRef& stmt, const ir_nodeRef& vssa)
   {
      if(!vssa)
      {
         return;
      }
      const auto stmt_node = GetPointerS<node_stmt>(stmt);
      if(stmt_node->AddVover(vssa))
      {
         GetPointerS<ssa_node>(vssa)->AddUseStmt(stmt);
      }
   }

   void serializeInterfaceAccess(const ir_nodeRef& stmt, const std::string& interface_fname,
                                 std::map<std::string, ir_nodeRef>& last_interface_vdefs,
                                 const ir_manipulationRef& ir_man)
   {
      const auto stmt_node = GetPointerS<node_stmt>(stmt);
      THROW_ASSERT(stmt_node, "Expected statement for interface serialization");
      const auto last_vdef = last_interface_vdefs.find(interface_fname);
      if(last_vdef != last_interface_vdefs.end())
      {
         addVirtualUseToStatement(stmt, last_vdef->second);
      }
      const auto new_vdef = ir_man->create_ssa_name(ir_nodeRef(), ir_man->GetPointerType(ir_man->GetVoidType()),
                                                    ir_nodeRef(), ir_nodeRef(), true);
      stmt_node->SetVdef(new_vdef);
      GetPointerS<ssa_node>(new_vdef)->SetDefStmt(stmt);
      last_interface_vdefs[interface_fname] = new_vdef;
   }

   void preserveBlockingInterfaceAccess(const ir_nodeRef& stmt, const ir_nodeRef& original_stmt,
                                        const std::string& interface_fname,
                                        std::map<std::string, ir_nodeRef>& last_interface_vdefs,
                                        const ir_manipulationRef& ir_man)
   {
      const auto stmt_node = GetPointerS<node_stmt>(stmt);
      const auto original_stmt_node = GetPointerS<node_stmt>(original_stmt);
      THROW_ASSERT(stmt_node, "Expected statement for blocking interface serialization");
      THROW_ASSERT(original_stmt_node, "Expected original statement for blocking interface serialization");

      if(original_stmt_node->memdef && !stmt_node->memdef)
      {
         stmt_node->memdef = original_stmt_node->memdef;
         GetPointerS<ssa_node>(stmt_node->memdef)->SetDefStmt(stmt);
      }
      if(original_stmt_node->memuse && !stmt_node->memuse)
      {
         stmt_node->memuse = original_stmt_node->memuse;
         GetPointerS<ssa_node>(stmt_node->memuse)->AddUseStmt(stmt);
      }
      for(const auto& vuse : original_stmt_node->vuses)
      {
         addVirtualUseToStatement(stmt, vuse);
      }
      for(const auto& vover : original_stmt_node->vovers)
      {
         addVirtualOverToStatement(stmt, vover);
      }

      const auto last_vdef = last_interface_vdefs.find(interface_fname);
      if(last_vdef != last_interface_vdefs.end())
      {
         addVirtualUseToStatement(stmt, last_vdef->second);
      }

      ir_nodeRef stmt_vdef = stmt_node->vdef;
      if(!stmt_vdef)
      {
         stmt_vdef = ir_man->create_ssa_name(ir_nodeRef(), ir_man->GetPointerType(ir_man->GetVoidType()), ir_nodeRef(),
                                             ir_nodeRef(), true);
         stmt_node->SetVdef(stmt_vdef);
         GetPointerS<ssa_node>(stmt_vdef)->SetDefStmt(stmt);
      }
      last_interface_vdefs[interface_fname] = stmt_vdef;
   }

   bool isSimpleStreamInterface(const application_managerRef& AppM, unsigned int function_id,
                                const std::string& bundle_name)
   {
      const auto HLSMgr = GetPointerS<HLS_manager>(AppM);
      THROW_ASSERT(HLSMgr, "Expected HLS manager");
      const auto top_fid = HLSMgr->CGetCallGraphManager().GetRootFunction(function_id);
      const auto top_fname = HLSMgr->CGetFunctionBehavior(top_fid)->CGetBehavioralHelper()->GetFunctionName();
      const auto& ifaces = HLSMgr->module_arch->GetArchitecture(top_fname)->ifaces;
      const auto iface_it = ifaces.find(bundle_name);
      if(iface_it == ifaces.end())
      {
         return false;
      }
      const auto mode_it = iface_it->second.find(FunctionArchitecture::iface_mode);
      return mode_it != iface_it->second.end() && (mode_it->second == "axis" || mode_it->second == "fifo");
   }
} // namespace

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
   bool BW1less;

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
         BW1less(false),
         parm_attrs(_parm_attrs),
         iface_attrs(_iface_attrs)
   {
   }

   void update(const ir_nodeRef& tn, const std::string& _type_name, ParameterConstRef parameters)
   {
      auto computeBWandAlgn = [&](unsigned long long& bitwidth0) -> unsigned {
         const auto ptd_type = ir_helper::CGetType(tn);
         if(ir_helper::IsStructType(ptd_type))
         {
            bitwidth0 = ir_helper::Size(ptd_type);
         }
         else if(ir_helper::IsArrayEquivType(ptd_type))
         {
            const auto elt_type = ir_helper::CGetArrayBaseType(ptd_type);
            if(ir_helper::IsStructType(elt_type))
            {
               if(!ir_helper::IsArrayEquivType(elt_type))
               {
                  THROW_ERROR_USAGE("Struct type not supported for interfacing: " + STR(elt_type) +
                                    ". Use a supported array/scalar interface representation.");
               }
               bitwidth0 = ir_helper::Size(ir_helper::CGetArrayBaseType(elt_type));
            }
            else
            {
               bitwidth0 = ir_helper::Size(elt_type);
            }
         }
         else if(ir_helper::IsPointerType(ptd_type))
         {
            bitwidth0 = static_cast<unsigned long long>(CompilerWrapper::CGetPointerSize(parameters));
         }
         else
         {
            bitwidth0 = ir_helper::Size(ptd_type);
         }
         if(BW1less)
         {
            --bitwidth0;
         }
         return static_cast<unsigned>(get_aligned_bitsize(bitwidth0) >> 3);
      };
      auto computeStorageAlignment = [&]() -> unsigned {
         auto storage_type = ir_helper::CGetType(tn);
         if(ir_helper::IsPointerType(storage_type))
         {
            storage_type = ir_helper::CGetPointedType(storage_type);
         }
         if(ir_helper::IsArrayEquivType(storage_type))
         {
            storage_type = ir_helper::CGetArrayBaseType(storage_type);
         }
         const auto storage_bits = ir_helper::SizeAlloc(storage_type);
         return static_cast<unsigned>(std::max(1ULL, (storage_bits + 7ULL) / 8ULL));
      };
      if(type != datatype::ac_type && type != datatype::bool_type)
      {
         bool is_signed, is_fixed;
         const auto type_name =
             std::regex_replace(_type_name, std::regex("(ac_channel|stream|hls::stream)<(.*)>"), "$2");
         static const std::regex exact_ac_type_def(
             R"(^\s*(?:const\s+)?(?:(?:\w+)::)*a[cp]_(?:u)?(?:\w+)<\s*\d+\s*,?\s*\d*\,?\s*\w*[^>]*>\s*$)");
         const auto ac_bitwidth =
             std::regex_match(type_name, exact_ac_type_def) ? ac_type_bitwidth(type_name, is_signed, is_fixed) : 0ULL;
         auto _type = ac_bitwidth != 0ULL ? datatype::ac_type : datatype::generic;
         unsigned long long _bitwidth = 0;
         if(_type == datatype::ac_type)
         {
            _bitwidth = ac_bitwidth;
            type = datatype::ac_type;
            alignment = std::max(alignment, computeStorageAlignment());
         }
         else if(_type_name.empty())
         {
            const auto _alignment = computeBWandAlgn(_bitwidth);
            alignment = std::max(alignment, _alignment);
         }
         else if(std::regex_search(_type_name, std::regex("(_B|b)ool[&*]")))
         {
            _bitwidth = 1;
            type = datatype::bool_type;
            alignment = 1;
         }

         if(_fixed_size && bitwidth && bitwidth != _bitwidth)
         {
            THROW_ERROR_USAGE("Unaligned access is not allowed for the required interface. "
                              "Align the data type/interface bitwidth or relax the required interface.");
         }
         bitwidth = std::max(bitwidth, _bitwidth);
      }
      else if(type == datatype::ac_type)
      {
         unsigned long long Lbitwidth = 0;
         alignment = std::max(computeBWandAlgn(Lbitwidth), computeStorageAlignment());
         if(bitwidth > 64ULL)
         {
            // For _BitInt(N) with N>64, get_aligned_bitsize rounds up to the next
            // power-of-two (e.g. 128 for N=66), but the actual sizeof is rounded to
            // the target pointer-size boundary.
            // On 32-bit targets (ptrBytes=4): W=66 -> ceil(9/4)*4=12.
            // On 64-bit targets (ptrBytes=8): W=66 -> ceil(9/8)*8=16.
            const auto ptrBytes = CompilerWrapper::CGetPointerSize(parameters) / 8;
            const auto byteSize = (bitwidth + 7) / 8;
            alignment = static_cast<unsigned>(((byteSize + ptrBytes - 1) / ptrBytes) * ptrBytes);
         }
      }
   }
};

static const std::regex signature_param_typename("((?:\\w+\\s*)+(?:<[^>]*>)?\\s*[\\*&\\/]*\\s*)");

static std::string get_decl_name(ir_nodeRef tn)
{
   const auto dn = GetPointer<const decl_node>(tn);
   THROW_ASSERT(dn, "Expected decl_node.");
   THROW_ASSERT(dn->name, "Expected declaration name.");
   return GetPointer<const identifier_node>(dn->name)->strg;
}

static std::vector<unsigned int> GetSortedRoots(const CallGraphManager& CGM)
{
   std::vector<unsigned int> sorted_roots;

   const auto& CG = CGM.GetCallGraph();
   std::deque<CallGraph::vertex_descriptor> sorted_cg;
   CG.ReverseTopologicalSort(sorted_cg);
   auto root_fid = CGM.GetRootFunctions();
   for(auto v : sorted_cg)
   {
      const auto fid = CGM.get_function(v);
      auto r_it = root_fid.find(fid);
      if(r_it != root_fid.end())
      {
         root_fid.erase(r_it);
         sorted_roots.push_back(fid);
      }
   }

   return sorted_roots;
}

static std::tuple<unsigned int, unsigned int> GetCallStmt(const CallGraphManager& CGM, unsigned int fid)
{
   const auto fv = CGM.GetVertex(fid);
   const auto& CG = CGM.GetCallGraph();
   if(CG.in_degree(fv) == 1)
   {
      const auto ie = CG.in_edges(fv).front();
      const auto& edge_info = CG.CGetEdgeInfo(ie);
      if(edge_info.direct_call_points.size() == 1)
      {
         const auto call_id = *edge_info.direct_call_points.begin();
         return {CGM.get_function(CG.source(ie)), call_id};
      }
   }
   return {0, 0};
}

static std::vector<ir_nodeRef> GetCallArgs(ir_nodeRef stmt)
{
   if(const auto ga = GetPointer<const assign_stmt>(stmt))
   {
      const auto ce = GetPointer<const call_node>(ga->op1);
      return ce->args;
   }
   else if(const auto gc = GetPointer<const call_stmt>(stmt))
   {
      return gc->args;
   }
   THROW_UNREACHABLE("Unexpected call statement.");
   return std::vector<ir_nodeRef>();
}

InterfaceInfer::InterfaceInfer(const application_managerRef _AppM, const DesignFlowManager& _design_flow_manager,
                               const ParameterConstRef _parameters)
    : ApplicationFrontendFlowStep(_AppM, INTERFACE_INFER, _design_flow_manager, _parameters), already_executed(false)
{
   debug_level = parameters->get_class_debug_level(GET_CLASS(*this));
}

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
      case(DEPENDENCE_RELATIONSHIP):
      {
         const auto design_flow_graph = design_flow_manager.CGetDesignFlowGraph();
         const auto technology_flow_step_factory = GetPointer<const TechnologyFlowStepFactory>(
             design_flow_manager.CGetDesignFlowStepFactory(DesignFlowStep::TECHNOLOGY));
         const auto technology_flow_signature =
             TechnologyFlowStep::ComputeSignature(TechnologyFlowStep_Type::LOAD_TECHNOLOGY);
         const auto technology_flow_step = design_flow_manager.GetDesignFlowStep(technology_flow_signature);
         const auto technology_design_flow_step =
             technology_flow_step != DesignFlowGraph::null_vertex() ?
                 design_flow_graph->CGetNodeInfo(technology_flow_step)->design_flow_step :
                 technology_flow_step_factory->CreateTechnologyFlowStep(TechnologyFlowStep_Type::LOAD_TECHNOLOGY);
         relationship.insert(technology_design_flow_step);
         break;
      }
      case(INVALIDATION_RELATIONSHIP):
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
   const auto TM = AppM->get_ir_manager();
   const auto& CGM = AppM->CGetCallGraphManager();
   const auto sorted_roots = GetSortedRoots(CGM);

   std::set<unsigned int> modified;
   const auto add_to_modified = [&](const ir_nodeRef& tn) {
      modified.insert(GetPointer<node_stmt>(tn)->parent->index);
   };

   // Remove interface information for non interfaced functions to avoid issues with aggressive IR optimizations
   // (signature modification, SROA, ...)
   for(auto it = HLSMgr->module_arch->cbegin(); it != HLSMgr->module_arch->cend();)
   {
      const auto is_original_csroa_func =
          it->second->attrs.find(FunctionArchitecture::func_original) != it->second->attrs.end();
      const auto fnode = TM->GetFunction(it->first);
      if(!is_original_csroa_func &&
         (!fnode || (it->second->attrs.size() <= 2 &&
                     std::find(sorted_roots.begin(), sorted_roots.end(), fnode->index) == sorted_roots.end())))
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

   for(const auto root_id : sorted_roots)
   {
      const auto fnode = TM->GetIRNode(root_id);
      const auto fd = GetPointer<const function_val_node>(fnode);
      const auto fsymbol = ir_helper::GetFunctionName(fnode);
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
               return ir_helper::PrintType(fd->type, true);
            }
            return cxa_fname;
         }();
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Extracting interface from signature " + fsymbol);
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Demangled as " + fsign);
         size_t parm_index = 0;
         std::sregex_token_iterator typename_it(fsign.begin(), fsign.end(), signature_param_typename, 0), end;
         func_arch->attrs[FunctionArchitecture::func_symbol] = fsymbol;
         func_arch->attrs[FunctionArchitecture::func_name] = ir_helper::GetFunctionName(fnode);
         ++typename_it; // First match is the function name/pointer type
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Iterating arguments:");
         for(const auto& arg : fd->list_of_args)
         {
            THROW_ASSERT(typename_it != end, "");
            const auto parm_name = [&]() {
               const auto* arg_decl = GetPointerS<argument_val_node>(arg);
               return GetPointerS<identifier_node>(arg_decl->name)->strg;
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
                   STR(";" PANDA_INCLUDE_INSTALLDIR "/" + tname.substr(0, 2) + "_fixed.h");
            }
            if(tname.find("_int<") != std::string::npos)
            {
               parm_attrs[FunctionArchitecture::parm_includes] +=
                   STR(";" PANDA_INCLUDE_INSTALLDIR "/" + tname.substr(0, 2) + "_int.h");
            }
            if(tname.find("ac_channel<") != std::string::npos)
            {
               parm_attrs[FunctionArchitecture::parm_includes] += STR(";" PANDA_INCLUDE_INSTALLDIR "/ac_channel.h");
            }
            ++typename_it;
            ++parm_index;
         }
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--");
         HLSMgr->module_arch->AddArchitecture(fsymbol, func_arch);
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--");
      }

      const ir_manipulationRef ir_man(new ir_manipulation(TM, parameters, AppM));
      const auto is_dataflow_module =
          func_arch->attrs.find(FunctionArchitecture::func_dataflow_module) != func_arch->attrs.end() &&
          (func_arch->attrs.find(FunctionArchitecture::func_dataflow_module)->second == "1");
      if(is_dataflow_module)
      {
         const auto [caller_id, call_id] = GetCallStmt(CGM, root_id);
         THROW_ASSERT(call_id, "Expected unique call point for dataflow module " + fsymbol + ".");
         const auto call_stmt = TM->GetIRNode(call_id);
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Call point " + STR(call_stmt));
         const auto& args = GetCallArgs(call_stmt);

         size_t idx = 0;
         for(const auto& arg : args)
         {
            if(ir_helper::IsPointerType(arg))
            {
               std::vector<ir_nodeRef> field_offset;
               const auto [base_var, owner_id] = ir_helper::ResolvePointerAlias(CGM, TM, arg, caller_id, field_offset);
               const auto parm_attr = std::find_if(func_arch->parms.begin(), func_arch->parms.end(), [&](auto& it) {
                  return it.second.at(FunctionArchitecture::parm_index) == std::to_string(idx);
               });
               THROW_ASSERT(parm_attr != func_arch->parms.end(), "Expected parameter index " + std::to_string(idx));
               auto& current_bundle = parm_attr->second.at(FunctionArchitecture::parm_bundle);
               std::string bundle_name;
               if(base_var->get_kind() == argument_val_node_K)
               {
                  func_arch->ifaces[current_bundle].at(FunctionArchitecture::iface_mode) = "default";
                  INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, output_level,
                                 "---Parameter " + parm_attr->second.at(FunctionArchitecture::parm_port) +
                                     " forwarded from caller function");
                  ++idx;
                  continue;
               }
               if(ir_helper::IsConstant(base_var))
               {
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                 "---Parameter " + parm_attr->second.at(FunctionArchitecture::parm_port) +
                                     " is constant at this call point");
                  ++idx;
                  continue;
               }
               if(const auto dn = GetPointer<const decl_node>(base_var))
               {
                  const auto base_type = ir_helper::CGetType(base_var);
                  if(ir_helper::IsArrayType(base_type))
                  {
                     func_arch->ifaces[current_bundle].at(FunctionArchitecture::iface_mode) = "array";
                     parm_attr->second[FunctionArchitecture::parm_elem_count] =
                         STR(ir_helper::GetArrayTotalSize(base_type));
                  }
                  unsigned int offset = 0;
                  for(const auto& fld : field_offset)
                  {
                     if(!ir_helper::IsConstant(fld))
                     {
                        THROW_ERROR("Non-constant-offset for DATAFLOW bundle");
                     }
                     offset += static_cast<unsigned>(ir_helper::GetConstValue(fld));
                  }
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                 "---Offset equal to " + std::to_string(offset));
                  bundle_name = "DF_bambu_" + std::to_string(owner_id) + "_" + std::to_string(base_var->index) + "FO" +
                                std::to_string(offset);
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
               auto pd = GetPointerS<argument_val_node>(fd->list_of_args.at(idx));
               pd->name = ir_man->create_identifier_node(bundle_name);
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
      }

      std::map<std::string, IRNodeSet> bundle_vdefs;
      std::map<std::string, ir_nodeRef> channel_read_vdefs;
      std::map<std::string, ir_nodeRef> channel_write_vdefs;
      OrderedInstructionsCache ordered_instructions;
      std::map<std::string, unsigned long long> bundle_max_storage_bitwidth;
      for(const auto& arg : fd->list_of_args)
      {
         const auto arg_name = get_decl_name(arg);
         const auto arg_type = ir_helper::CGetType(arg);
         if(func_arch->parms.find(arg_name) == func_arch->parms.end() || !ir_helper::IsPointerType(arg_type))
         {
            continue;
         }
         const auto& parm_attrs = func_arch->parms.at(arg_name);
         if(parm_attrs.find(FunctionArchitecture::parm_bundle) == parm_attrs.end())
         {
            continue;
         }
         const auto iface_it = func_arch->ifaces.find(parm_attrs.at(FunctionArchitecture::parm_bundle));
         if(iface_it == func_arch->ifaces.end() ||
            iface_it->second.find(FunctionArchitecture::iface_mode) == iface_it->second.end() ||
            iface_it->second.at(FunctionArchitecture::iface_mode) != "m_axi")
         {
            continue;
         }
         auto storage_type = ir_helper::CGetPointedType(arg_type);
         if(ir_helper::IsArrayEquivType(storage_type))
         {
            storage_type = ir_helper::CGetArrayBaseType(storage_type);
         }
         const auto storage_bitwidth = std::max(8ULL, ceil_pow2(ir_helper::SizeAlloc(storage_type)));
         auto& max_storage_bitwidth = bundle_max_storage_bitwidth[parm_attrs.at(FunctionArchitecture::parm_bundle)];
         max_storage_bitwidth = std::max(max_storage_bitwidth, storage_bitwidth);
      }
      for(const auto& arg : fd->list_of_args)
      {
         const auto arg_id = arg->index;
         const auto arg_name = get_decl_name(arg);
         const auto arg_type = ir_helper::CGetType(arg);
         INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, output_level, "---Parameter @" + STR(arg_id) + " " + arg_name);
         THROW_ASSERT(func_arch->parms.find(arg_name) != func_arch->parms.end(),
                      "Not matched parameter name: " + arg_name);
         auto& parm_attrs = func_arch->parms.at(arg_name);
         THROW_ASSERT(parm_attrs.find(FunctionArchitecture::parm_bundle) != parm_attrs.end(),
                      "Missing parameter bundle name");
         parm_attrs.emplace(FunctionArchitecture::parm_offset, "off");
         if(ir_helper::IsPointerType(arg_type))
         {
            const auto ptd_type = ir_helper::CGetPointedType(arg_type);
            const auto array_size = ir_helper::IsArrayType(ptd_type) ? ir_helper::GetArrayTotalSize(ptd_type) : 1ULL;
            auto parm_size_in_bytes = array_size * (get_aligned_bitsize(ir_helper::Size(arg_type)) >> 3);
            parm_attrs.emplace(FunctionArchitecture::parm_size_in_bytes, std::to_string(parm_size_in_bytes));
         }
         THROW_ASSERT(func_arch->ifaces.find(parm_attrs.at(FunctionArchitecture::parm_bundle)) !=
                          func_arch->ifaces.end(),
                      "Missing parameter bundle: " + parm_attrs.at(FunctionArchitecture::parm_bundle));
         auto& iface_attrs = func_arch->ifaces.at(parm_attrs.at(FunctionArchitecture::parm_bundle));
         iface_attrs[FunctionArchitecture::iface_direction] = port_o::GetString(port_o::IN);
         const auto is_pointer_type = ir_helper::IsPointerType(arg_type);
         const auto raw_interface_type = iface_attrs.at(FunctionArchitecture::iface_mode);
         const auto default_iface_bitwidth =
             is_pointer_type ?
                 ((raw_interface_type == "bus" || raw_interface_type == "default") ? ir_helper::Size(arg_type) : 0ULL) :
                 ir_helper::Size(arg_type);
         auto& iface_bitwidth = iface_attrs[FunctionArchitecture::iface_bitwidth];
         iface_bitwidth =
             STR(std::max(default_iface_bitwidth, iface_bitwidth.empty() ? 0ULL : std::stoull(iface_bitwidth)));
         iface_attrs[FunctionArchitecture::iface_alignment] =
             std::to_string(get_aligned_bitsize(ir_helper::Size(arg_type)) >> 3);
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
            const auto arg_ssa = TM->GetIRNode(arg_ssa_id);
            THROW_ASSERT(arg_ssa->get_kind() == ssa_node_K, "");
            bool unused_port = false;
            if(GetPointerS<const ssa_node>(arg_ssa)->CGetUseStmts().empty())
            {
               THROW_WARNING("Parameter '" + arg_name + "' not used by any statement");
               if(ir_helper::IsPointerType(arg_type))
               {
                  unused_port = true;
               }
               else
               {
                  THROW_ERROR_USAGE("Parameter '" + arg_name + "' is unused, so requested interface '" +
                                    interface_type +
                                    "' is not meaningful. Remove the interface constraint or use the parameter.");
               }
            }
            if(ir_helper::IsPointerType(arg_type))
            {
               INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "---Is a pointer type");
               interface_info info(arg_name, fsymbol, interface_type != "m_axi", parm_attrs, iface_attrs);
               info.update(arg_ssa, parm_attrs.at(FunctionArchitecture::parm_typename), parameters);
               if(interface_type == "m_axi" &&
                  iface_attrs.find(FunctionArchitecture::iface_cache_line_count) != iface_attrs.end())
               {
                  auto storage_type = ir_helper::CGetPointedType(arg_type);
                  if(ir_helper::IsArrayEquivType(storage_type))
                  {
                     storage_type = ir_helper::CGetArrayBaseType(storage_type);
                  }
                  const auto cache_word_bitsize =
                      ceil_pow2(std::max(info.bitwidth, ir_helper::SizeAlloc(storage_type)));
                  auto& cache_word_size = iface_attrs[FunctionArchitecture::iface_cache_word_size];
                  cache_word_size = std::to_string(
                      std::max(cache_word_size.empty() ? 0ULL : std::stoull(cache_word_size), cache_word_bitsize));
               }

               std::list<ir_nodeRef> writeStmt;
               std::list<ir_nodeRef> readStmt;
               ChasePointerInterface(arg_ssa, writeStmt, readStmt, info);
               const auto is_csroa_ac_type = [&]() {
                  const auto type_attr = parm_attrs.find(FunctionArchitecture::parm_original_typename);
                  if(type_attr == parm_attrs.end())
                  {
                     return false;
                  }
                  static const std::regex ac_type_def(R"(a[cp]_(?:u)?(?:\w+)<)");
                  return std::regex_search(type_attr->second, ac_type_def);
               }();
               if((info.type == datatype::ac_type || is_csroa_ac_type) &&
                  parm_attrs.find(FunctionArchitecture::parm_size_in_bytes) != parm_attrs.end())
               {
                  const auto storage_elem_count = [&]() {
                     const auto elem_count_attr = parm_attrs.find(FunctionArchitecture::parm_elem_count);
                     if(elem_count_attr != parm_attrs.end())
                     {
                        return std::max(1ULL, std::stoull(elem_count_attr->second));
                     }
                     const auto ptd_type = ir_helper::CGetPointedType(arg_type);
                     return std::max(1ULL,
                                     ir_helper::IsArrayType(ptd_type) ? ir_helper::GetArrayTotalSize(ptd_type) : 1ULL);
                  }();
                  const auto storage_alignment = std::max(
                      1ULL, std::stoull(parm_attrs.at(FunctionArchitecture::parm_size_in_bytes)) / storage_elem_count);
                  info.alignment =
                      static_cast<unsigned>(std::max<unsigned long long>(info.alignment, storage_alignment));
               }
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
                        return interface_type;
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
                           interface_type = "none";
                        }
                        else if(parameters->IsParameter("none-registered-ptrdefault") &&
                                parameters->GetParameter<int>("none-registered-ptrdefault") == 1)
                        {
                           iface_attrs[FunctionArchitecture::iface_register] = "";
                           interface_type = "none";
                        }
                        interface_type = "ovalid";
                     }
                     else if(interface_type == "fifo" || interface_type == "axis")
                     {
                        THROW_ERROR_USAGE("Parameter '" + arg_name + "' cannot use interface '" + interface_type +
                                          "' because it is both read and written. Use an interface that supports I/O.");
                     }
                     THROW_ASSERT(interface_type != "ptrdefault", "unexpected condition");
                     if(isScalarPointerProtocolWithoutReadback(interface_type) &&
                        hasWriteBeforeRead(writeStmt, readStmt, AppM, ordered_instructions))
                     {
                        THROW_ERROR_USAGE("Parameter '" + arg_name + "' in function '" + fsymbol +
                                          "' uses inferred scalar pointer interface '" + interface_type +
                                          "' but the pointer is written before it is read. Scalar protocols 'none', "
                                          "'handshake', 'acknowledge', 'valid', and 'ovalid' do not support pointer "
                                          "read-after-write semantics. Use 'array', 'bus', or 'm_axi'.");
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
                        THROW_ERROR_USAGE("Parameter '" + arg_name + "' cannot use interface '" + interface_type +
                                          "' because it is read-only. Use a read-compatible interface.");
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
               const auto& parm_bundle = parm_attrs.at(FunctionArchitecture::parm_bundle);
               const auto bundle_max_bitwidth =
                   interface_type == "m_axi" &&
                           bundle_max_storage_bitwidth.find(parm_bundle) != bundle_max_storage_bitwidth.end() ?
                       bundle_max_storage_bitwidth.at(parm_bundle) :
                       0ULL;
               iface_attrs[FunctionArchitecture::iface_bitwidth] =
                   std::to_string(std::max(std::max(info.bitwidth, bundle_max_bitwidth),
                                           std::stoull(iface_attrs[FunctionArchitecture::iface_bitwidth])));
               iface_attrs[FunctionArchitecture::iface_alignment] = std::to_string(info.alignment);
               if(interface_type == "fifo" || interface_type == "axis")
               {
                  iface_attrs.emplace(FunctionArchitecture::iface_depth, "0");
               }
               interface_type = info.name;

               THROW_ASSERT(info.bitwidth, "Expected non-zero bitwidth");

               std::set<std::string> operationsR, operationsW;
               const auto interface_datatype = ir_man->GetCustomIntegerType(info.bitwidth, true);
               const auto& bundle_name = iface_attrs.at(FunctionArchitecture::iface_name);
               const auto require_flush =
                   interface_type == "m_axi" &&
                   iface_attrs.find(FunctionArchitecture::iface_cache_line_count) != iface_attrs.end();
               const auto store_vdef = [&](const ir_nodeRef& stmt) {
                  if(require_flush)
                  {
                     const auto& vdef = GetPointerS<const node_stmt>(stmt)->vdef;
                     if(vdef)
                     {
                        bundle_vdefs[bundle_name].insert(vdef);
                     }
                  }
               };
               for(const auto& stmt : readStmt)
               {
                  setReadInterface(stmt, bundle_name, operationsR, interface_datatype, channel_read_vdefs, ir_man, TM);
                  add_to_modified(stmt);
                  store_vdef(stmt);
               }
               for(const auto& stmt : writeStmt)
               {
                  setWriteInterface(stmt, bundle_name, operationsW, interface_datatype, channel_write_vdefs, ir_man,
                                    TM);
                  add_to_modified(stmt);
                  store_vdef(stmt);
               }
               create_resource(operationsR, operationsW, info, func_arch, unused_port, root_id);
            }
            else if(interface_type == "none")
            {
               THROW_ERROR_USAGE("Interface type '" + interface_type + "' for parameter '" + arg_name +
                                 "' is not valid in this context.");
            }
            else
            {
               THROW_ERROR_USAGE("Interface type '" + interface_type + "' for parameter '" + arg_name +
                                 "' is not supported. Select a supported interface mode.");
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
         if(parm_attrs.find(FunctionArchitecture::parm_size_in_bytes) != parm_attrs.end())
         {
            if(parm_attrs.at(FunctionArchitecture::parm_size_in_bytes) == "0")
            {
               const auto has_void_typename =
                   parm_attrs.find(FunctionArchitecture::parm_typename) != parm_attrs.end() &&
                   parm_attrs.at(FunctionArchitecture::parm_typename).find("void") != std::string::npos;
               const auto is_void_pointer =
                   has_void_typename ||
                   (ir_helper::IsPointerType(arg_type) && ir_helper::IsVoidType(ir_helper::CGetPointedType(arg_type)));
               if(is_void_pointer)
               {
                  THROW_ERROR_USAGE("Unable to infer the byte size of parameter '" + arg_name + "' in function '" +
                                    fsymbol +
                                    "': a top-level void* parameter has no element-size information for co-simulation. "
                                    "Use a typed pointer (e.g., uint8_t*) or use a C/C++ testbench and call "
                                    "m_param_alloc(<parameter_position>, <size_in_bytes>) before invoking the top "
                                    "function.");
               }
               THROW_ERROR_USAGE(
                   "Unable to infer the size in bytes of parameter '" + arg_name + "' in function '" + fsymbol +
                   "'. For co-simulation you must provide the pointed-data size with a C/C++ testbench by "
                   "calling m_param_alloc(<parameter_position>, <size_in_bytes>) before the top-function "
                   "call, also when the parameter type is not void*.");
            }
            INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, output_level,
                           "---bytes     : " + parm_attrs.at(FunctionArchitecture::parm_size_in_bytes));
         }
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
            std::vector<ir_nodeConstRef> argsT;
            argsT.push_back(ir_man->GetBooleanType());
            argsT.push_back(ir_man->GetUnsignedIntegerType());
            const auto fini_fd = ir_man->create_function_decl(fini_fname, fd->parent, argsT, ir_man->GetVoidType(),
                                                              BUILTIN_LOCINFO, false);

            // Cache flush is indicated by a write of size 0.
            std::vector<ir_nodeRef> args;
            args.push_back(TM->CreateUniqueIntegerCst(1, argsT.at(0)));
            args.push_back(TM->CreateUniqueIntegerCst(0, argsT.at(1)));
            const auto fini_call = ir_man->create_call_stmt(fini_fd, args, fd->parent->index, BUILTIN_LOCINFO);
            const auto fini_node = GetPointerS<node_stmt>(fini_call);
            for(const auto& vdef : vdefs)
            {
               fini_node->AddVuse(vdef);
            }
            return fini_call;
         };

         const auto sl = GetPointerS<const statement_list_node>(fd->body);
         for(const auto& bbi_bb : sl->list_of_bloc)
         {
            const auto& bb = bbi_bb.second;
            const auto is_last = std::count(bb->list_of_succ.cbegin(), bb->list_of_succ.cend(), BB_EXIT);
            if(bb->number != BB_ENTRY && is_last)
            {
               const auto fini_call = generate_fini_call();
               THROW_ASSERT(bb->CGetStmtList().size(), "BB" + STR(bb->number) + " should not be empty");
               const auto last_stmt = bb->CGetStmtList().back();
               if(GetPointer<const return_stmt>(last_stmt))
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

void InterfaceInfer::ChasePointerInterfaceRecurse(CustomOrderedSet<unsigned>& Visited, ir_nodeRef ssa_ref,
                                                  std::list<ir_nodeRef>& writeStmt, std::list<ir_nodeRef>& readStmt,
                                                  interface_info& info)
{
   const auto TM = AppM->get_ir_manager();
   const auto ir_man = ir_manipulationRef(new ir_manipulation(TM, parameters, AppM));
   enum call_type
   {
      ct_forward,
      ct_read,
      ct_peek,
      ct_write
   };
   const auto propagate_arg_use = [&](ir_nodeRef arg_var, size_t use_count, ir_nodeRef fd_node,
                                      const std::vector<ir_nodeRef>& call_args, ir_nodeRef& ssa_var) -> call_type {
      THROW_ASSERT(arg_var && fd_node, "unexpected condition");
      ssa_var = arg_var;
      const auto call_fd = [&]() {
         const auto fd_kind = fd_node->get_kind();
         auto& fn = fd_node;
         if(fd_kind == addr_node_K)
         {
            fn = GetPointerS<const addr_node>(fd_node)->op;
         }
         THROW_ASSERT(fn->get_kind() == function_val_node_K, "unexpected condition: " + fn->get_kind_text());
         return GetPointerS<const function_val_node>(fn);
      }();
      if(!call_fd->body)
      {
         const auto called_fname = [&]() {
            const auto fname = ir_helper::GetFunctionName(fd_node);
            const auto demangled = cxa_demangle(fname);
            return demangled.size() ? demangled : fname;
         }();
         if(called_fname.find(STR_CST_interface_parameter_keyword) != std::string::npos)
         {
            const auto bundle_id = called_fname.substr(0, called_fname.find(STR_CST_interface_parameter_keyword));
            if(bundle_id != info.arg_id && bundle_id != info.parm_attrs.at(FunctionArchitecture::parm_bundle))
            {
               THROW_ERROR_USAGE("Pattern not supported with required I/O interface: parameters '" + bundle_id +
                                 "' and '" + info.arg_id +
                                 "' share a memory operation. Split/shared accesses or relax interface constraints.");
            }
            return call_type::ct_forward;
         }
         else if(called_fname.find("ac_channel") != std::string::npos)
         {
            if(called_fname.find("_read_bambu_internal") != std::string::npos ||
               called_fname.find("_peek_bambu_internal") != std::string::npos)
            {
               if(call_fd->list_of_args.size() >= 1 && call_fd->list_of_args.size() <= 4)
               {
                  auto ret_type = call_fd->type;
                  if(ret_type->get_kind() == function_ty_node_K)
                  {
                     const auto ft = GetPointerS<const function_ty_node>(ret_type);
                     if(ft->retn->get_kind() != void_ty_node_K)
                     {
                        info.BW1less = call_fd->list_of_args.size() == 2;
                        ssa_var = ft->retn;
                     }
                     else
                     {
                        info.BW1less = call_fd->list_of_args.size() == 3;
                        THROW_ASSERT(call_fd->list_of_args.size() >= 2, "unexpected condition");
                        THROW_ASSERT(call_args.size() >= 2, "unexpected condition");
                        auto first_arg = call_args.at(0);
                        THROW_ASSERT(ir_helper::IsPointerType(first_arg), "unexpected condition");
                        ssa_var = ir_helper::GetBaseVariable(first_arg);
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
               return called_fname.find("_peek_bambu_internal") != std::string::npos ? call_type::ct_peek :
                                                                                       call_type::ct_read;
            }
            else if(called_fname.find("_write_bambu_internal") != std::string::npos)
            {
               if(call_fd->list_of_args.size() >= 2)
               {
                  auto ret_type = call_fd->type;
                  if(ret_type->get_kind() == void_ty_node_K)
                  {
                     THROW_ERROR("unexpected condition");
                  }
                  const auto payload_index = call_fd->list_of_args.size() - 1;
                  if(call_fd->list_of_args.size() > 2)
                  {
                     unsigned long long payload_bits = 0;
                     for(size_t payload_idx = 1; payload_idx < call_fd->list_of_args.size(); ++payload_idx)
                     {
                        payload_bits += ir_helper::Size(call_fd->list_of_args.at(payload_idx));
                     }
                     ssa_var = ir_man->GetCustomIntegerType(payload_bits, false);
                  }
                  else if(ir_helper::IsPointerType(call_fd->list_of_args.at(payload_index)))
                  {
                     auto second_arg = call_args.at(payload_index);
                     ssa_var = ir_helper::GetBaseVariable(second_arg);
                     THROW_ASSERT(ssa_var != second_arg, "unexpected condition");
                  }
                  else
                  {
                     ssa_var = call_fd->list_of_args.at(payload_index);
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
         const auto call_arg_ssa = TM->GetIRNode(call_arg_ssa_id);
         THROW_ASSERT(call_arg_ssa->get_kind() == ssa_node_K, "");
         if(GetPointerS<const ssa_node>(call_arg_ssa)->CGetUseStmts().size())
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
   const auto push_stmt = [&](ir_nodeRef stmt, call_type ct, ir_nodeRef val) {
      if(ct == call_type::ct_forward)
      {
         return;
      }
      if(ct == call_type::ct_read || ct == call_type::ct_peek)
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
   if(!Visited.insert(ssa_ref->index).second)
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level,
                     "---SKIPPED FUNCTION: already visited through argument " + ssa_ref->ToString());
      return;
   }

   std::queue<ir_nodeRef> chase_ssa;
   chase_ssa.push(ssa_ref);
   while(chase_ssa.size())
   {
      const auto current_ssa_ref = chase_ssa.front();
      const auto ssa_var = GetPointer<const ssa_node>(current_ssa_ref);
      chase_ssa.pop();
      INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level,
                     "-->SSA VARIABLE: " + ssa_var->ToString() + " with " + STR(ssa_var->CGetUseStmts().size()) +
                         " use statements");
      for(const auto& stmt_count : ssa_var->CGetUseStmts())
      {
         const auto use_stmt = stmt_count.first;
         const auto& use_count = stmt_count.second;
         INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "---STMT: " + use_stmt->ToString());
         if(const auto ga = GetPointer<const assign_stmt>(use_stmt))
         {
            const auto op0_kind = ga->op0->get_kind();
            const auto op1_kind = ga->op1->get_kind();
            if(op0_kind == mem_access_node_K)
            {
               if(op1_kind == mem_access_node_K)
               {
                  THROW_ERROR("Pattern currently not supported: *x=*y; " + use_stmt->ToString());
               }
               else
               {
                  THROW_ASSERT(op1_kind == ssa_node_K || GetPointer<const cst_node>(ga->op1), "unexpected condition");
                  if(GetPointer<const cst_node>(ga->op1) || GetPointer<const ssa_node>(ga->op1) != ssa_var)
                  {
                     push_stmt(stmt_count.first, call_type::ct_write, ga->op1);
                  }
               }
            }
            else if(op1_kind == mem_access_node_K)
            {
               push_stmt(stmt_count.first, call_type::ct_read, ga->op0);
               if(ir_helper::IsPointerType(ga->op0))
               {
                  INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "---Pointer to pointer interface propagation");
                  chase_ssa.push(ga->op0);
               }
            }
            else if(op1_kind == call_node_K)
            {
               const auto ce = GetPointerS<const call_node>(ga->op1);
               const auto return_type = ir_helper::CGetType(ga->op0);
               if(ir_helper::IsPointerType(return_type))
               {
                  THROW_ERROR("unexpected pattern");
               }
               ir_nodeRef ssa_val;
               const auto ct = propagate_arg_use(current_ssa_ref, use_count, ce->fn, ce->args, ssa_val);
               push_stmt(stmt_count.first, ct, ssa_val);
            }
            else if(op1_kind == eq_node_K || op1_kind == ne_node_K || op1_kind == gt_node_K || op1_kind == lt_node_K ||
                    op1_kind == ge_node_K || op1_kind == le_node_K)
            {
               THROW_WARNING("Pattern potentially not supported: pointer parameter is used in a compare statement: " +
                             use_stmt->ToString() + ":" + ga->op1->get_kind_text());
            }
            else
            {
               if(!ir_helper::IsPointerType(ga->op0))
               {
                  THROW_WARNING("Pattern potentially not supported: parameter converted to non-pointer type: " +
                                use_stmt->ToString() + ":" + ga->op1->get_kind_text());
               }
               else if(op1_kind != nop_node_K && op1_kind != bitcast_node_K && op1_kind != ssa_node_K &&
                       op1_kind != gep_node_K && op1_kind != select_node_K)
               {
                  THROW_WARNING("Pattern potentially not supported: parameter used in a non-supported statement: " +
                                use_stmt->ToString() + ":" + ga->op1->get_kind_text());
               }
               ChasePointerInterfaceRecurse(Visited, ga->op0, writeStmt, readStmt, info);
            }
         }
         else if(const auto gc = GetPointer<const call_stmt>(use_stmt))
         {
            THROW_ASSERT(gc->fn, "unexpected condition");
            const auto fn_node = gc->fn;
            if(fn_node->get_kind() == addr_node_K)
            {
               const auto ae = GetPointerS<const addr_node>(fn_node);
               const auto ae_op = ae->op;
               if(ae_op->get_kind() == function_val_node_K)
               {
                  ir_nodeRef ssa_val;
                  const auto ct = propagate_arg_use(current_ssa_ref, use_count, ae->op, gc->args, ssa_val);
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
         else if(const auto gp = GetPointer<const phi_stmt>(use_stmt))
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

void InterfaceInfer::ChasePointerInterface(ir_nodeRef ssa_node, std::list<ir_nodeRef>& writeStmt,
                                           std::list<ir_nodeRef>& readStmt, interface_info& info)
{
   CustomOrderedSet<unsigned> Visited;
   INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "-->Parameter uses:");
   ChasePointerInterfaceRecurse(Visited, ssa_node, writeStmt, readStmt, info);
   INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "<--");
}

void InterfaceInfer::forwardInterface(const ir_nodeRef& fnode, const ir_nodeRef& parm_node, const interface_info& info)
{
   const auto fsymbol = ir_helper::GetFunctionName(fnode);
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

void InterfaceInfer::setReadInterface(ir_nodeRef stmt, const std::string& arg_name, std::set<std::string>& operationsR,
                                      ir_nodeConstRef interface_datatype,
                                      std::map<std::string, ir_nodeRef>& channel_read_vdefs,
                                      const ir_manipulationRef ir_man, const ir_managerRef TM)
{
   const auto gn = GetPointerS<node_stmt>(stmt);
   THROW_ASSERT(gn->parent && gn->parent->get_kind() == function_val_node_K, "expected a function_val_node scope");
   const auto fd = GetPointerS<function_val_node>(gn->parent);
   const auto fname = ir_helper::GetFunctionName(gn->parent);
   INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "-->LOAD from " + fname + ":");
   INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "---BEFORE: " + stmt->ToString());

   THROW_ASSERT(fd->body, "expected a body");
   const auto sl = GetPointerS<statement_list_node>(fd->body);
   const auto curr_bb = sl->list_of_bloc.at(gn->bb_index);
   const auto ret_call =
       stmt->get_kind() == assign_stmt_K && GetPointerS<assign_stmt>(stmt)->op1->get_kind() == call_node_K;
   const auto ref_call = stmt->get_kind() == call_stmt_K;
   if(ret_call || ref_call)
   {
      const auto is_peek_call = [&]() {
         ir_nodeRef fnode;
         if(ret_call)
         {
            const auto ga = GetPointerS<const assign_stmt>(stmt);
            fnode = GetPointerS<const call_node>(ga->op1)->fn;
         }
         else
         {
            fnode = GetPointerS<const call_stmt>(stmt)->fn;
         }
         const auto raw_fname = ir_helper::GetFunctionName(fnode);
         const auto demangled = cxa_demangle(raw_fname);
         return (demangled.size() ? demangled : raw_fname).find("_peek_bambu_internal") != std::string::npos;
      }();
      bool valid_var;
      ir_nodeRef valid_ptr;
      ir_nodeConstRef data_type;
      ir_nodeRef stmt_op0;
      ir_nodeRef data_ptr;
      bool oldAsyncSignature;
      if(ret_call)
      {
         const auto ga = GetPointerS<const assign_stmt>(stmt);
         stmt_op0 = ga->op0;
         const auto ce = GetPointerS<const call_node>(ga->op1);
         valid_var = ce->args.size() >= 2;
         oldAsyncSignature = ce->args.size() == 3;
         data_type = ir_helper::CGetType(ga->op0);
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
         const auto gc = GetPointerS<const call_stmt>(stmt);
         valid_var = gc->args.size() >= 3;
         oldAsyncSignature = gc->args.size() == 4;
         auto first_arg = gc->args.at(0);
         THROW_ASSERT(ir_helper::IsPointerType(first_arg), "unexpected condition");
         data_ptr = first_arg;
         auto data_obj = ir_helper::GetBaseVariable(first_arg);
         THROW_ASSERT(data_obj != first_arg, "unexpected condition");
         data_type = ir_helper::CGetType(data_obj);
         if(valid_var)
         {
            valid_ptr = gc->args.at(2);
         }
         else
         {
            THROW_ASSERT(gc->args.size() == 1 || gc->args.size() == 2, "unexpected condition");
         }
      }
      THROW_ASSERT(!valid_ptr || ir_helper::IsPointerType(valid_ptr), "Valid type must be bool pointer");
      THROW_ASSERT(!gn->memdef && !gn->memuse, "");

      const auto data_size = ir_helper::Size(interface_datatype);
      const auto sel_type = ir_man->GetBooleanType();
      const auto ret_type = ir_man->GetCustomIntegerType(data_size + (valid_var ? 1 : 0), true);
      const auto out_ptr_type = ir_man->GetPointerType(interface_datatype);
      const auto interface_fname = ENCODE_FDNAME(
          arg_name, valid_var ? (is_peek_call ? "_PeekAsync" : "_ReadAsync") : (is_peek_call ? "_Peek" : "_Read"),
          "Channel");
      const auto preserve_blocking_dependences = !valid_var;
      const auto fdecl_node = [&]() {
         operationsR.insert(interface_fname);
         std::vector<ir_nodeConstRef> argsT;
         argsT.push_back(sel_type);
         argsT.push_back(sel_type);
         return ir_man->create_function_decl(interface_fname, fd->parent, argsT, ret_type, BUILTIN_LOCINFO, false);
      }();

      std::vector<ir_nodeRef> args;
      args.push_back(TM->CreateUniqueIntegerCst(!is_peek_call, sel_type));
      args.push_back(TM->CreateUniqueIntegerCst(valid_var, sel_type));
      const auto new_ce = ir_man->CreateCallExpr(fdecl_node, args, BUILTIN_LOCINFO);
      const auto new_call = ir_man->CreateAssignStmt(ret_type, nullptr, nullptr, new_ce, fd->index, BUILTIN_LOCINFO);
      curr_bb->PushBefore(new_call, stmt, AppM);
      if(preserve_blocking_dependences)
      {
         preserveBlockingInterfaceAccess(new_call, stmt, interface_fname, channel_read_vdefs, ir_man);
      }
      else
      {
         serializeInterfaceAccess(new_call, interface_fname, channel_read_vdefs, ir_man);
      }
      const auto retval = GetPointerS<const assign_stmt>(new_call)->op0;
      INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "--- AFTER: " + new_call->ToString());

      std::map<unsigned int, unsigned long long> load_bit_offsets;
      std::vector<ir_nodeRef> read_pointer_derivations;
      auto enableOptFun = [&]() -> std::pair<bool, std::vector<ir_nodeRef>> {
         std::pair<bool, std::vector<ir_nodeRef>> res;
         res.first = true;
         INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "---data_ptr->" + data_ptr->ToString());
         THROW_ASSERT(data_ptr->get_kind() == ssa_node_K, "unexpected case");
         const auto data_ptr_base = ir_helper::GetBaseVariable(data_ptr);
         if(!data_ptr_base)
         {
            res.first = false;
            return res;
         }
         std::set<unsigned int> visited_pointers;
         if(!collectAggregateMemoryAccesses(data_ptr, stmt, false, res.second, read_pointer_derivations,
                                            visited_pointers))
         {
            res.first = false;
            return res;
         }
         const FunctionOrderedInstructions ordered_instructions(AppM, fd->index);
         std::vector<std::pair<unsigned long long, unsigned long long>> loaded_ranges;
         for(const auto& load_stmt : res.second)
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "---user-> " + load_stmt->ToString());
            const auto user_ga = GetPointerS<const assign_stmt>(load_stmt);
            const auto load_ptr = GetPointerS<const mem_access_node>(user_ga->op1)->op;
            unsigned long long bit_offset = 0;
            const auto load_size = ir_helper::Size(user_ga->op0);
            if(!ordered_instructions.dominates(stmt, load_stmt) ||
               !getConstantPointerBitOffset(load_ptr, data_ptr_base, bit_offset) || bit_offset > data_size ||
               load_size > (data_size - bit_offset))
            {
               res.first = false;
               return res;
            }
            load_bit_offsets.emplace(load_stmt->index, bit_offset);
            loaded_ranges.emplace_back(bit_offset, bit_offset + load_size);
         }
         if(!hasRegularAggregateElementLayout(loaded_ranges, data_size))
         {
            res.first = false;
         }
         if(valid_var && (res.second.size() != 1 || load_bit_offsets.at(res.second.front()->index) != 0 ||
                          ir_helper::Size(GetPointerS<const assign_stmt>(res.second.front())->op0) != data_size + 1))
         {
            res.first = false;
         }
         res.first = res.first && !res.second.empty();
         return res;
      };
      std::pair<bool, std::vector<ir_nodeRef>> enableOpt;
      if(!oldAsyncSignature)
      {
         if(!ret_call)
         {
            enableOpt = enableOptFun();
         }
         else if(ret_call && valid_var)
         {
            enableOpt.first = true;
         }
      }
      if(enableOpt.first)
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "---Do the optimization");
         if(ret_call)
         {
            if(valid_var)
            {
               THROW_ASSERT(ir_helper::Size(stmt_op0) == (data_size + 1),
                            "Size ret var=" + std::to_string(ir_helper::Size(stmt_op0)) +
                                " payload size=" + std::to_string(data_size));
               TM->ReplaceIRNode(new_call, retval, stmt_op0);
            }
            else
            {
               THROW_ASSERT(ir_helper::Size(stmt_op0) == data_size,
                            "Size ret var=" + std::to_string(ir_helper::Size(stmt_op0)) +
                                " payload size=" + std::to_string(data_size));
               auto ga_nop = ir_man->CreateNopExpr(retval, interface_datatype, nullptr, nullptr, fd->index);
               curr_bb->PushBefore(ga_nop, stmt, AppM);
               // Cast read data
               INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "---  NOP: " + ga_nop->ToString());
               auto data_nop = GetPointerS<const assign_stmt>(ga_nop)->op0;
               TM->ReplaceIRNode(ga_nop, data_nop, stmt_op0);
            }
         }
         else
         {
            for(auto& used_stmt : enableOpt.second)
            {
               INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "--- LOAD to be removed " + used_stmt->ToString());
               auto user_ga = GetPointerS<assign_stmt>(used_stmt);
               if(valid_var)
               {
                  THROW_ASSERT(ir_helper::Size(user_ga->op0) == (data_size + 1),
                               "Size ret var=" + std::to_string(ir_helper::Size(user_ga->op0)) +
                                   " payload size=" + std::to_string(data_size));
                  TM->ReplaceIRNode(new_call, retval, user_ga->op0);
                  INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "--- NEW-CALL-POST: " + new_call->ToString());
               }
               else
               {
                  const auto load_bb = sl->list_of_bloc.at(GetPointerS<const node_stmt>(used_stmt)->bb_index);
                  ir_nodeRef field_value = retval;
                  const auto bit_offset = load_bit_offsets.at(used_stmt->index);
                  if(bit_offset)
                  {
                     const auto shift_value = TM->CreateUniqueIntegerCst(bit_offset, ret_type);
                     const auto shift_expr =
                         ir_man->create_binary_operation(ret_type, retval, shift_value, BUILTIN_LOCINFO, shr_node_K);
                     const auto shift_stmt =
                         ir_man->CreateAssignStmt(ret_type, nullptr, nullptr, shift_expr, fd->index, BUILTIN_LOCINFO);
                     load_bb->PushBefore(shift_stmt, used_stmt, AppM);
                     field_value = GetPointerS<const assign_stmt>(shift_stmt)->op0;
                  }
                  const auto field_type = ir_helper::CGetType(user_ga->op0);
                  const auto truncate_type = ir_helper::IsRealType(field_type) ?
                                                 ir_man->GetCustomIntegerType(ir_helper::Size(field_type), false) :
                                                 field_type;
                  const auto truncate_expr =
                      ir_man->create_unary_operation(truncate_type, field_value, BUILTIN_LOCINFO, nop_node_K);
                  ir_nodeRef replacement;
                  if(ir_helper::IsRealType(field_type))
                  {
                     const auto truncated_value = ir_man->create_ssa_name(nullptr, truncate_type, nullptr, nullptr);
                     const auto truncate_stmt =
                         ir_man->create_assign_stmt(truncated_value, truncate_expr, fd->index, BUILTIN_LOCINFO);
                     load_bb->PushBefore(truncate_stmt, used_stmt, AppM);
                     const auto bitcast_expr =
                         ir_man->create_unary_operation(field_type, truncated_value, BUILTIN_LOCINFO, bitcast_node_K);
                     replacement = ir_man->create_assign_stmt(user_ga->op0, bitcast_expr, fd->index, BUILTIN_LOCINFO);
                  }
                  else
                  {
                     replacement = ir_man->create_assign_stmt(user_ga->op0, truncate_expr, fd->index, BUILTIN_LOCINFO);
                  }
                  load_bb->Replace(used_stmt, replacement, true, AppM);
               }
               if(valid_var)
               {
                  const auto load_bb = sl->list_of_bloc.at(GetPointerS<const node_stmt>(used_stmt)->bb_index);
                  load_bb->RemoveStmt(used_stmt, AppM);
               }
            }
         }
         curr_bb->RemoveStmt(stmt, AppM);
         if(!ret_call)
         {
            removeDeadAggregateTemporary(data_ptr, read_pointer_derivations, sl, AppM);
         }
      }
      else
      {
         if(ret_call && !valid_var && ir_helper::IsStructType(data_type))
         {
            THROW_ASSERT(ir_helper::Size(data_type) == data_size, "Aggregate payload size mismatch");
            const auto aggregate_ssa = GetPointerS<const ssa_node>(stmt_op0);
            const auto aggregate_uses = aggregate_ssa->CGetUseStmts();
            std::map<size_t, unsigned long long> field_sizes;
            for(const auto& use_stmt_count : aggregate_uses)
            {
               const auto use_ga = GetPointerS<const assign_stmt>(use_stmt_count.first);
               THROW_ASSERT(use_ga->op1->get_kind() == extractvalue_node_K,
                            "Aggregate channel read must be consumed through extractvalue operations: " +
                                use_stmt_count.first->ToString());
               const auto extract_value = GetPointerS<const extractvalue_node>(use_ga->op1);
               THROW_ASSERT(extract_value->op0->index == stmt_op0->index, "Unexpected aggregate extraction source");
               const auto field_index = static_cast<size_t>(ir_helper::GetConstValue(extract_value->op1));
               const auto field_size = ir_helper::Size(use_ga->op0);
#if HAVE_ASSERTS
               const auto [field_it, inserted] =
#endif
                   field_sizes.emplace(field_index, field_size);
               THROW_ASSERT(inserted || field_it->second == field_size, "Inconsistent aggregate extraction type");
            }
            for(const auto& use_stmt_count : aggregate_uses)
            {
               const auto use_stmt = use_stmt_count.first;
               const auto use_ga = GetPointerS<const assign_stmt>(use_stmt);
               const auto extract_value = GetPointerS<const extractvalue_node>(use_ga->op1);
               const auto field_index = static_cast<size_t>(ir_helper::GetConstValue(extract_value->op1));
               unsigned long long field_offset = 0;
               for(const auto& [index, size] : field_sizes)
               {
                  if(index >= field_index)
                  {
                     break;
                  }
                  field_offset += size;
               }
               const auto use_bb = sl->list_of_bloc.at(GetPointerS<const node_stmt>(use_stmt)->bb_index);
               ir_nodeRef field_value = retval;
               if(field_offset)
               {
                  const auto shift_value = TM->CreateUniqueIntegerCst(field_offset, ret_type);
                  const auto shift_expr =
                      ir_man->create_binary_operation(ret_type, retval, shift_value, BUILTIN_LOCINFO, shr_node_K);
                  const auto shift_stmt =
                      ir_man->CreateAssignStmt(ret_type, nullptr, nullptr, shift_expr, fd->index, BUILTIN_LOCINFO);
                  use_bb->PushBefore(shift_stmt, use_stmt, AppM);
                  field_value = GetPointerS<const assign_stmt>(shift_stmt)->op0;
               }
               const auto field_type = ir_helper::CGetType(use_ga->op0);
               const auto truncate_expr =
                   ir_man->create_unary_operation(field_type, field_value, BUILTIN_LOCINFO, nop_node_K);
               const auto replacement =
                   ir_man->create_assign_stmt(use_ga->op0, truncate_expr, fd->index, BUILTIN_LOCINFO);
               use_bb->Replace(use_stmt, replacement, true, AppM);
            }
            curr_bb->RemoveStmt(stmt, AppM);
         }
         else
         {
            auto ga_mask = ir_man->CreateNopExpr(retval, interface_datatype, nullptr, nullptr, fd->index);
            curr_bb->PushBefore(ga_mask, stmt, AppM);
            // Mask and cast read data
            INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "---  MASK: " + ga_mask->ToString());
            auto data_mask = GetPointerS<const assign_stmt>(ga_mask)->op0;
            if(ir_helper::IsRealType(data_type))
            {
               const auto bitcast_expr =
                   ir_man->create_unary_operation(data_type, data_mask, BUILTIN_LOCINFO, bitcast_node_K);
               ga_mask =
                   ir_man->CreateAssignStmt(data_type, nullptr, nullptr, bitcast_expr, fd->index, BUILTIN_LOCINFO);
               curr_bb->PushBefore(ga_mask, stmt, AppM);
               INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "--- BITCAST: " + ga_mask->ToString());
               data_mask = GetPointerS<const assign_stmt>(ga_mask)->op0;
            }

            ir_nodeRef last_stmt;
            if(ret_call)
            {
               INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "---Do the optimization");
               TM->ReplaceIRNode(ga_mask, data_mask, stmt_op0);
               INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "---   FIX: " + ga_mask->ToString());
               last_stmt = ga_mask;
            }
            else
            {
               INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "---Not optimized");
               const auto data_ptr_type = ir_helper::CGetType(data_ptr);
               const auto data_ref =
                   ir_man->create_unary_operation(interface_datatype, data_ptr, BUILTIN_LOCINFO, mem_access_node_K);
               const auto ga_store = ir_man->create_assign_stmt(data_ref, data_mask, fd->index, BUILTIN_LOCINFO);
               if(valid_var)
               {
                  const auto vdef = ir_man->create_ssa_name(ir_nodeRef(), ir_man->GetPointerType(ir_man->GetVoidType()),
                                                            ir_nodeRef(), ir_nodeRef(), true);
                  GetPointerS<assign_stmt>(ga_store)->SetVdef(vdef);
                  for(const auto& vuse : GetPointerS<assign_stmt>(stmt)->vuses)
                  {
                     if(GetPointerS<node_stmt>(ga_store)->AddVuse(vuse))
                     {
                        GetPointerS<ssa_node>(vuse)->AddUseStmt(ga_store);
                     }
                  }
                  THROW_ASSERT(gn->vdef, "Expected virtual ssa definition on store operation: " + gn->ToString());
                  THROW_ASSERT(gn->vdef->get_kind() == ssa_node_K, "unexpected condition");
                  auto vdef_ssa = GetPointerS<ssa_node>(gn->vdef);
                  for(const auto& usingStmt : vdef_ssa->CGetUseStmts())
                  {
                     if(GetPointerS<node_stmt>(usingStmt.first)->AddVuse(vdef))
                     {
                        GetPointerS<ssa_node>(vdef)->AddUseStmt(usingStmt.first);
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
               const auto be_vshift = ir_man->create_binary_operation(
                   ret_type, retval, TM->CreateUniqueIntegerCst(data_size, ret_type), BUILTIN_LOCINFO, shr_node_K);
               const auto ga_vshift =
                   ir_man->CreateAssignStmt(ret_type, nullptr, nullptr, be_vshift, fd->index, BUILTIN_LOCINFO);
               curr_bb->PushBefore(ga_vshift, stmt, AppM);
               INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "---VSHIFT: " + ga_vshift->ToString());
               const auto v_shift = GetPointerS<const assign_stmt>(ga_vshift)->op0;
               const auto valid_ptd_type = ir_man->GetBooleanType();
               const auto valid_type = ir_man->GetPointerType(valid_ptd_type);
               const auto ga_valid = ir_man->CreateNopExpr(v_shift, valid_ptd_type, nullptr, nullptr, fd->index);
               curr_bb->PushBefore(ga_valid, stmt, AppM);
               INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "--- VALID: " + ga_valid->ToString());
               const auto valid_ref = GetPointerS<const assign_stmt>(ga_valid)->op0;
               const auto valid_memref =
                   ir_man->create_unary_operation(valid_ptd_type, valid_ptr, BUILTIN_LOCINFO, mem_access_node_K);
               const auto ga_valid_store =
                   ir_man->create_assign_stmt(valid_memref, valid_ref, fd->index, BUILTIN_LOCINFO);
               curr_bb->Replace(stmt, ga_valid_store, true, AppM);
               if(!ret_call)
               {
                  auto ssaDef = GetPointerS<assign_stmt>(last_stmt)->vdef;
                  if(GetPointerS<node_stmt>(ga_valid_store)->AddVuse(ssaDef))
                  {
                     GetPointerS<ssa_node>(ssaDef)->AddUseStmt(ga_valid_store);
                  }
               }
               INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "--- VALID STORE: " + ga_valid_store->ToString());
            }
            else if(ret_call)
            {
               curr_bb->RemoveStmt(stmt, AppM);
            }
         }
      }
   }
   else
   {
      THROW_ASSERT(stmt && stmt->get_kind() == assign_stmt_K, "unexpected condition");
      const auto ga = GetPointerS<assign_stmt>(stmt);
      THROW_ASSERT(ga->op1->get_kind() == mem_access_node_K, "unexpected condition");

      /// create the function_val_node
      const auto actual_type = ir_helper::CGetType(ga->op0);
      const auto bit_size_type = ir_man->GetUnsignedIntegerType();
      const auto boolean_type = ir_man->GetBooleanType();
      const auto is_simple_stream_interface = isSimpleStreamInterface(AppM, fd->index, arg_name);
      const auto fdecl_node = [&]() {
         const auto interface_fname = ENCODE_FDNAME(arg_name, "_Read", "");
         operationsR.insert(interface_fname);
         std::vector<ir_nodeConstRef> argsT;
         argsT.push_back(boolean_type);
         argsT.push_back(bit_size_type);
         argsT.push_back(interface_datatype);
         argsT.push_back(ir_helper::CGetType(ga->op1));
         return ir_man->create_function_decl(interface_fname, fd->parent, argsT, interface_datatype, BUILTIN_LOCINFO,
                                             false);
      }();
      std::vector<ir_nodeRef> args;
      const auto sel_value = TM->CreateUniqueIntegerCst(is_simple_stream_interface ? 1 : 0, boolean_type);
      const auto size_value = TM->CreateUniqueIntegerCst(
          is_simple_stream_interface ? 0 : static_cast<long long>(ir_helper::Size(actual_type)), bit_size_type);

      const auto data_value = TM->CreateUniqueIntegerCst(0, interface_datatype);
      args.push_back(sel_value);
      args.push_back(size_value);
      args.push_back(data_value);

      THROW_ASSERT(ga->op1->get_kind() == mem_access_node_K, "unexpected condition");
      const auto mr = GetPointerS<const mem_access_node>(ga->op1);
      args.push_back(mr->op);

      const auto ce = ir_man->CreateCallExpr(fdecl_node, args, BUILTIN_LOCINFO);
      if(ir_helper::IsSameType(interface_datatype, actual_type))
      {
         TM->ReplaceIRNode(stmt, ga->op1, ce);
         CustomUnorderedSet<unsigned int> AV;
         CallGraphManager::addCallPointAndExpand(AV, AppM, ga->parent->index, fdecl_node->index, stmt->index,
                                                 FunctionEdgeInfo::CallType::direct_call, DEBUG_LEVEL_NONE);
         GetPointer<HLS_manager>(AppM)->design_interface_io[fname][ga->bb_index][arg_name].push_back(stmt->index);
         INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "--- AFTER: " + stmt->ToString());
      }
      else
      {
         const auto is_real = ir_helper::IsRealType(actual_type);
         const auto tmp_type =
             is_real ? ir_man->GetCustomIntegerType(ir_helper::Size(actual_type), true) : interface_datatype;
         const auto tmp_ssa = ir_man->create_ssa_name(nullptr, tmp_type, nullptr, nullptr);
         const auto gc = ir_man->create_assign_stmt(tmp_ssa, ce, fd->index, BUILTIN_LOCINFO);
         curr_bb->Replace(stmt, gc, true, AppM);
         const auto cast_expr = ir_man->create_unary_operation(actual_type, tmp_ssa, BUILTIN_LOCINFO,
                                                               is_real ? bitcast_node_K : nop_node_K);
         const auto cast = ir_man->create_assign_stmt(ga->op0, cast_expr, fd->index, BUILTIN_LOCINFO);
         curr_bb->PushAfter(cast, gc, AppM);
         GetPointer<HLS_manager>(AppM)->design_interface_io[fname][curr_bb->number][arg_name].push_back(gc->index);

         INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "--- AFTER: " + gc->ToString());
         INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "---   NOP: " + stmt->ToString());
      }
   }
   INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "<--");
}

void InterfaceInfer::setWriteInterface(ir_nodeRef stmt, const std::string& arg_name, std::set<std::string>& operationsW,
                                       ir_nodeConstRef interface_datatype,
                                       std::map<std::string, ir_nodeRef>& channel_write_vdefs,
                                       const ir_manipulationRef ir_man, const ir_managerRef TM)
{
   const auto gn = GetPointerS<node_stmt>(stmt);
   THROW_ASSERT(gn->parent && gn->parent->get_kind() == function_val_node_K, "expected a function_val_node scope");
   const auto fd = GetPointerS<function_val_node>(gn->parent);
   const auto fname = ir_helper::GetFunctionName(gn->parent);
   INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "-->STORE from " + fname + ":");
   INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "---BEFORE: " + stmt->ToString());

   THROW_ASSERT(fd->body, "expected a body");
   const auto sl = GetPointerS<statement_list_node>(fd->body);
   const auto curr_bb = sl->list_of_bloc.at(gn->bb_index);
   const auto ret_call =
       stmt->get_kind() == assign_stmt_K && GetPointerS<assign_stmt>(stmt)->op1->get_kind() == call_node_K;
   const auto ref_call = stmt->get_kind() == call_stmt_K;
   if(ret_call || ref_call)
   {
      ir_nodeRef data_obj;
      ir_nodeRef data_ptr;
      ir_nodeConstRef ret_type;
      bool valid_var;
      bool isAStruct = false;
      if(ret_call)
      {
         const auto ga = GetPointerS<const assign_stmt>(stmt);
         const auto ce = GetPointerS<const call_node>(ga->op1);
         THROW_ASSERT(ce->args.size() == 2, "unexpected condition");
         data_obj = ce->args.at(1);
         if(ir_helper::IsPointerType(data_obj))
         {
            data_ptr = data_obj;
            data_obj = ir_helper::GetBaseVariable(data_obj);
            THROW_ASSERT(data_obj != data_ptr, "unexpected condition");
            isAStruct = true;
         }
         valid_var = true;
         ret_type = ir_helper::CGetType(ga->op0);
      }
      else
      {
         const auto gc = GetPointerS<const call_stmt>(stmt);
         THROW_ASSERT(gc->args.size() >= 2, "unexpected condition");
         data_obj = gc->args.back();
         if(gc->args.size() > 2)
         {
            const auto packed_type = ir_man->GetCustomIntegerType(ir_helper::Size(interface_datatype), false);
            ir_nodeRef packed_value;
            unsigned long long bit_offset = 0;
            for(size_t payload_idx = 1; payload_idx < gc->args.size(); ++payload_idx)
            {
               const auto fragment = gc->args.at(payload_idx);
               const auto fragment_type = ir_helper::CGetType(fragment);
               const auto fragment_value =
                   ir_man->create_unary_operation(packed_type, fragment, BUILTIN_LOCINFO, nop_node_K);
               const auto fragment_ssa = ir_man->create_ssa_name(nullptr, packed_type, nullptr, nullptr);
               curr_bb->PushBefore(ir_man->create_assign_stmt(fragment_ssa, fragment_value, fd->index, BUILTIN_LOCINFO),
                                   stmt, AppM);
               ir_nodeRef shifted_fragment = fragment_ssa;
               if(bit_offset)
               {
                  const auto shift = TM->CreateUniqueIntegerCst(bit_offset, packed_type);
                  const auto shift_expr =
                      ir_man->create_binary_operation(packed_type, fragment_ssa, shift, BUILTIN_LOCINFO, shl_node_K);
                  const auto shift_ssa = ir_man->create_ssa_name(nullptr, packed_type, nullptr, nullptr);
                  curr_bb->PushBefore(ir_man->create_assign_stmt(shift_ssa, shift_expr, fd->index, BUILTIN_LOCINFO),
                                      stmt, AppM);
                  shifted_fragment = shift_ssa;
               }
               if(!packed_value)
               {
                  packed_value = shifted_fragment;
               }
               else
               {
                  const auto packed_expr = ir_man->create_binary_operation(packed_type, packed_value, shifted_fragment,
                                                                           BUILTIN_LOCINFO, or_node_K);
                  const auto packed_ssa = ir_man->create_ssa_name(nullptr, packed_type, nullptr, nullptr);
                  curr_bb->PushBefore(ir_man->create_assign_stmt(packed_ssa, packed_expr, fd->index, BUILTIN_LOCINFO),
                                      stmt, AppM);
                  packed_value = packed_ssa;
               }
               bit_offset += ir_helper::Size(fragment_type);
            }
            data_obj = packed_value;
         }
         if(ir_helper::IsPointerType(data_obj))
         {
            data_ptr = data_obj;
            data_obj = ir_helper::GetBaseVariable(data_obj);
            THROW_ASSERT(data_obj != data_ptr, "unexpected condition");
            isAStruct = true;
         }
         valid_var = false;
         ret_type = ir_man->GetVoidType();
      }

      const auto data_type = ir_helper::CGetType(data_obj);
      const auto data_size = ir_helper::Size(interface_datatype);
      const auto boolean_type = ir_man->GetBooleanType();
      const auto bit_size_type = ir_man->GetUnsignedIntegerType();
      const auto out_ptr_type = ir_man->GetPointerType(interface_datatype);
      const auto interface_fname = ENCODE_FDNAME(arg_name, valid_var ? "_WriteAsync" : "_Write", "Channel");
      const auto preserve_blocking_dependences = !valid_var;
      const auto fdecl_node = [&]() {
         operationsW.insert(interface_fname);
         std::vector<ir_nodeConstRef> argsT;
         argsT.push_back(boolean_type);
         argsT.push_back(bit_size_type);
         argsT.push_back(data_type);
         argsT.push_back(boolean_type);
         return ir_man->create_function_decl(interface_fname, fd->parent, argsT, ret_type, BUILTIN_LOCINFO, false);
      }();

      ir_nodeRef lastStmt;
      ir_nodeRef forwarded_data_ptr;
      std::vector<ir_nodeRef> write_pointer_derivations;
      if(isAStruct)
      {
         std::map<unsigned int, unsigned long long> store_bit_offsets;
         auto enableOpt = [&]() -> std::pair<bool, std::vector<ir_nodeRef>> {
            std::pair<bool, std::vector<ir_nodeRef>> res;
            res.first = true;
            INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "---data_ptr->" + data_ptr->ToString());
            THROW_ASSERT(data_ptr->get_kind() == ssa_node_K, "unexpected case");
            const auto data_ptr_base = ir_helper::GetBaseVariable(data_ptr);
            if(!data_ptr_base)
            {
               res.first = false;
               return res;
            }
            std::set<unsigned int> visited_pointers;
            if(!collectAggregateMemoryAccesses(data_ptr, stmt, true, res.second, write_pointer_derivations,
                                               visited_pointers))
            {
               res.first = false;
               return res;
            }
            const FunctionOrderedInstructions ordered_instructions(AppM, fd->index);
            std::vector<std::pair<unsigned long long, unsigned long long>> stored_ranges;
            for(const auto& store_stmt : res.second)
            {
               INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "---data_ptr_stmt_i " + store_stmt->ToString());
               const auto user_ga = GetPointerS<const assign_stmt>(store_stmt);
               const auto store_ptr = GetPointerS<const mem_access_node>(user_ga->op0)->op;
               const auto store_size = ir_helper::Size(user_ga->op1);
               unsigned long long bit_offset = 0;
               if(GetPointerS<const node_stmt>(store_stmt)->bb_index != gn->bb_index ||
                  !ordered_instructions.dominates(store_stmt, stmt) ||
                  (!ir_helper::IsConstant(user_ga->op1) && user_ga->op1->get_kind() != ssa_node_K) ||
                  !getConstantPointerBitOffset(store_ptr, data_ptr_base, bit_offset) || bit_offset > data_size ||
                  store_size > (data_size - bit_offset))
               {
                  res.first = false;
                  return res;
               }
               for(const auto& [range_begin, range_end] : stored_ranges)
               {
                  if(bit_offset < range_end && range_begin < bit_offset + store_size)
                  {
                     res.first = false;
                     return res;
                  }
               }
               stored_ranges.emplace_back(bit_offset, bit_offset + store_size);
               store_bit_offsets.emplace(store_stmt->index, bit_offset);
            }
            res.first = res.first && hasRegularAggregateElementLayout(stored_ranges, data_size);
            return res;
         }();
         if(enableOpt.first)
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "---Do the optimization");
            std::sort(enableOpt.second.begin(), enableOpt.second.end(), [&](const auto& lhs, const auto& rhs) {
               return store_bit_offsets.at(lhs->index) < store_bit_offsets.at(rhs->index);
            });
            ir_nodeRef packed_value;
            for(auto& used_stmt : enableOpt.second)
            {
               INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "--- STORE to be removed " + used_stmt->ToString());
               const auto user_ga = GetPointerS<const assign_stmt>(used_stmt);
               const auto fragment_size = ir_helper::Size(user_ga->op1);
               const auto fragment_type = ir_man->GetCustomIntegerType(fragment_size, false);
               const auto fragment_expr =
                   ir_man->create_unary_operation(fragment_type, user_ga->op1, BUILTIN_LOCINFO,
                                                  ir_helper::IsRealType(user_ga->op1) ? bitcast_node_K : nop_node_K);
               const auto fragment_stmt =
                   ir_man->CreateAssignStmt(fragment_type, nullptr, nullptr, fragment_expr, fd->index, BUILTIN_LOCINFO);
               curr_bb->PushBefore(fragment_stmt, stmt, AppM);
               const auto fragment = GetPointerS<const assign_stmt>(fragment_stmt)->op0;
               const auto extend_expr =
                   ir_man->create_unary_operation(interface_datatype, fragment, BUILTIN_LOCINFO, nop_node_K);
               const auto extend_stmt = ir_man->CreateAssignStmt(interface_datatype, nullptr, nullptr, extend_expr,
                                                                 fd->index, BUILTIN_LOCINFO);
               curr_bb->PushBefore(extend_stmt, stmt, AppM);
               ir_nodeRef packed_fragment = GetPointerS<const assign_stmt>(extend_stmt)->op0;
               const auto bit_offset = store_bit_offsets.at(used_stmt->index);
               if(bit_offset)
               {
                  const auto shift_value = TM->CreateUniqueIntegerCst(bit_offset, interface_datatype);
                  const auto shift_expr = ir_man->create_binary_operation(interface_datatype, packed_fragment,
                                                                          shift_value, BUILTIN_LOCINFO, shl_node_K);
                  const auto shift_stmt = ir_man->CreateAssignStmt(interface_datatype, nullptr, nullptr, shift_expr,
                                                                   fd->index, BUILTIN_LOCINFO);
                  curr_bb->PushBefore(shift_stmt, stmt, AppM);
                  packed_fragment = GetPointerS<const assign_stmt>(shift_stmt)->op0;
               }
               if(packed_value)
               {
                  const auto packed_expr = ir_man->create_binary_operation(interface_datatype, packed_value,
                                                                           packed_fragment, BUILTIN_LOCINFO, or_node_K);
                  const auto packed_stmt = ir_man->CreateAssignStmt(interface_datatype, nullptr, nullptr, packed_expr,
                                                                    fd->index, BUILTIN_LOCINFO);
                  curr_bb->PushBefore(packed_stmt, stmt, AppM);
                  packed_value = GetPointerS<const assign_stmt>(packed_stmt)->op0;
               }
               else
               {
                  packed_value = packed_fragment;
               }
               curr_bb->RemoveStmt(used_stmt, AppM);
            }
            data_obj = packed_value;
            forwarded_data_ptr = data_ptr;
            isAStruct = false;
         }
         else
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "---Not optimized");
            const auto ga_ptr = ir_man->CreateNopExpr(data_ptr, out_ptr_type, nullptr, nullptr, fd->index);
            curr_bb->PushBefore(ga_ptr, stmt, AppM);
            INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "--- PCAST: " + ga_ptr->ToString());
            const auto out_data_ptr = GetPointerS<const assign_stmt>(ga_ptr)->op0;
            const auto data_ref =
                ir_man->create_unary_operation(interface_datatype, out_data_ptr, BUILTIN_LOCINFO, mem_access_node_K);
            const auto ga_load =
                ir_man->CreateAssignStmt(interface_datatype, nullptr, nullptr, data_ref, fd->index, BUILTIN_LOCINFO);
            curr_bb->Replace(stmt, ga_load, true, AppM);
            INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "---  LOAD: " + ga_load->ToString());
            data_obj = GetPointerS<const assign_stmt>(ga_load)->op0;
            lastStmt = ga_load;
         }
      }
      else if(ir_helper::IsRealType(data_type))
      {
         const auto bitcast_expr = ir_man->create_unary_operation(data_type, data_obj, BUILTIN_LOCINFO, bitcast_node_K);
         const auto ga_bitcast =
             ir_man->CreateAssignStmt(interface_datatype, nullptr, nullptr, bitcast_expr, fd->index, BUILTIN_LOCINFO);
         curr_bb->PushBefore(ga_bitcast, stmt, AppM);
         INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "--- BITCAST: " + ga_bitcast->ToString());
         data_obj = GetPointerS<const assign_stmt>(ga_bitcast)->op0;
      }

      std::vector<ir_nodeRef> args;
      args.push_back(TM->CreateUniqueIntegerCst(1, boolean_type));
      args.push_back(TM->CreateUniqueIntegerCst(ir_helper::Size(interface_datatype), bit_size_type));
      args.push_back(data_obj);
      args.push_back(TM->CreateUniqueIntegerCst(valid_var, boolean_type));
      if(valid_var)
      {
         const auto ga_stmt = GetPointerS<const assign_stmt>(stmt);
         const auto ce = ir_man->CreateCallExpr(fdecl_node, args, BUILTIN_LOCINFO);
         const auto ga_call = ir_man->create_assign_stmt(ga_stmt->op0, ce, fd->index, BUILTIN_LOCINFO);
         if(isAStruct)
         {
            curr_bb->PushAfter(ga_call, lastStmt, AppM);
         }
         else
         {
            curr_bb->Replace(stmt, ga_call, preserve_blocking_dependences, AppM);
         }
         if(preserve_blocking_dependences)
         {
            preserveBlockingInterfaceAccess(ga_call, stmt, interface_fname, channel_write_vdefs, ir_man);
         }
         else
         {
            serializeInterfaceAccess(ga_call, interface_fname, channel_write_vdefs, ir_man);
         }
         INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "--- AFTER: " + ga_call->ToString());
      }
      else
      {
         const auto gc = ir_man->create_call_stmt(fdecl_node, args, fd->index, BUILTIN_LOCINFO);
         if(isAStruct)
         {
            curr_bb->PushAfter(gc, lastStmt, AppM);
         }
         else
         {
            curr_bb->Replace(stmt, gc, preserve_blocking_dependences, AppM);
         }
         if(preserve_blocking_dependences)
         {
            preserveBlockingInterfaceAccess(gc, stmt, interface_fname, channel_write_vdefs, ir_man);
         }
         else
         {
            serializeInterfaceAccess(gc, interface_fname, channel_write_vdefs, ir_man);
         }
         INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "--- AFTER: " + gc->ToString());
      }
      if(forwarded_data_ptr)
      {
         removeDeadAggregateTemporary(forwarded_data_ptr, write_pointer_derivations, sl, AppM);
         THROW_ASSERT(GetPointerS<const ssa_node>(forwarded_data_ptr)->CGetUseStmts().empty(),
                      "Expected the forwarded channel temporary to be unused: " + forwarded_data_ptr->ToString());
      }
   }
   else
   {
      THROW_ASSERT(stmt && stmt->get_kind() == assign_stmt_K, "unexpected condition");
      const auto ga = GetPointerS<assign_stmt>(stmt);
      THROW_ASSERT(ga->op0->get_kind() == mem_access_node_K, "unexpected condition");

      auto value_node = ga->op1;
      auto actual_type = ir_helper::CGetType(value_node);
      if(ir_helper::IsSameType(interface_datatype, actual_type))
      {
         ir_nodeRef nop;
         if(ir_helper::IsRealType(actual_type))
         {
            const auto int_type = ir_man->GetCustomIntegerType(ir_helper::Size(actual_type), true);
            const auto bitcast_expr =
                ir_man->create_unary_operation(int_type, value_node, BUILTIN_LOCINFO, bitcast_node_K);
            value_node = ir_man->create_ssa_name(nullptr, int_type, nullptr, nullptr);
            nop = ir_man->create_assign_stmt(value_node, bitcast_expr, fd->index, BUILTIN_LOCINFO);
         }
         else
         {
            nop = ir_man->CreateNopExpr(value_node, interface_datatype, nullptr, nullptr, ga->parent->index);
            value_node = GetPointerS<const assign_stmt>(nop)->op0;
         }
         curr_bb->PushBefore(nop, stmt, AppM);
         INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "---   NOP: " + nop->ToString());
      }
      const auto boolean_type = ir_man->GetBooleanType();
      const auto bit_size_type = ir_man->GetUnsignedIntegerType();
      const auto is_simple_stream_interface = isSimpleStreamInterface(AppM, fd->index, arg_name);

      /// create the function_val_node
      const auto fdecl_node = [&]() {
         const auto interface_fname = ENCODE_FDNAME(arg_name, "_Write", "");
         operationsW.insert(interface_fname);
         std::vector<ir_nodeConstRef> argsT;
         argsT.push_back(boolean_type);
         argsT.push_back(bit_size_type);
         argsT.push_back(interface_datatype);
         argsT.push_back(ir_helper::CGetType(ga->op0));
         return ir_man->create_function_decl(interface_fname, fd->parent, argsT, ir_man->GetVoidType(), BUILTIN_LOCINFO,
                                             false);
      }();

      std::vector<ir_nodeRef> args;
      args.push_back(TM->CreateUniqueIntegerCst(1, boolean_type));
      args.push_back(TM->CreateUniqueIntegerCst(
          is_simple_stream_interface ? 0 : static_cast<long long>(ir_helper::Size(actual_type)), bit_size_type));
      args.push_back(value_node);
      args.push_back(
          is_simple_stream_interface ?
              TM->CreateUniqueIntegerCst(0, ir_helper::CGetType(GetPointerS<const mem_access_node>(ga->op0)->op)) :
              GetPointerS<const mem_access_node>(ga->op0)->op);

      const auto gc = ir_man->create_call_stmt(fdecl_node, args, ga->parent->index, BUILTIN_LOCINFO);
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
      GetPointerS<module_o>(interface_top)->set_description("Interface module for function: " + ResourceName);
      GetPointerS<module_o>(interface_top)->set_copyright(GENERATED_COPYRIGHT);
      GetPointerS<module_o>(interface_top)->set_authors("Component automatically generated by bambu");
      GetPointerS<module_o>(interface_top)->set_license(GENERATED_LICENSE);
      GetPointerS<module_o>(interface_top)->set_multi_unit_multiplicity(1U);
      const auto if_name = info.name == "ovalid" ? "none" : info.name;
      const auto is_unbounded = if_name == "valid" || if_name == "handshake" || if_name == "fifo" || if_name == "axis";

      const auto address_bitsize = HLSMgr->get_address_bitsize();
      const auto nbitDataSize = 64u - static_cast<unsigned>(__builtin_clzll(info.bitwidth));
      const auto out_bitsize = if_name == "fifo" ? (info.bitwidth + 1U) : info.bitwidth;
      const structural_type_descriptorRef size1(new structural_type_descriptor("bool", 1));
      const structural_type_descriptorRef rwsize(new structural_type_descriptor("bool", nbitDataSize));
      const structural_type_descriptorRef dataType(new structural_type_descriptor("bool", info.bitwidth));
      const structural_type_descriptorRef addrType(new structural_type_descriptor("bool", address_bitsize));
      const structural_type_descriptorRef outType(new structural_type_descriptor("bool", out_bitsize));
      const structural_type_descriptorRef bool_type(new structural_type_descriptor("bool", 0));
      CM->add_port(CLOCK_PORT_NAME, port_o::IN, interface_top, bool_type);
      CM->add_port(RESET_PORT_NAME, port_o::IN, interface_top, bool_type);
      CM->add_port_vector(START_PORT_NAME, port_o::IN, 1U, interface_top, bool_type);
      if(is_unbounded)
      {
         CM->add_port_vector(DONE_PORT_NAME, port_o::OUT, 1U, interface_top, bool_type);
      }

      // in1, in2, and in3 are not used by read interfaces, but are necessary to keep ParmMgr signature consistent
      CM->add_port("in1", port_o::IN, interface_top, size1);
      CM->add_port("in2", port_o::IN, interface_top, rwsize);
      CM->add_port("in3", port_o::IN, interface_top, dataType);
      const auto addrPort = CM->add_port("in4", port_o::IN, interface_top, addrType);
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

      CM->add_NP_functionality(interface_top, NP_functionality::LIBRARY, "in1 in2 in3 in4 out1");
      CM->add_NP_functionality(interface_top, NP_functionality::VERILOG_GENERATOR,
                               "Read" + getHDLGeneratorNameToken(if_name) + "HDLGenerator");
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
      fu->area_m = std::make_shared<area_info>();
      fu->area_m->resources[area_info::AREA] = 0;
      if(!is_unbounded)
      {
         fu->logical_type = functional_unit::COMBINATIONAL;
      }

      for(const auto& fdName : operations)
      {
         const auto op = GetPointer<operation>(fu->get_operation(fdName));
         op->time_m = std::make_shared<time_info>();
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
      GetPointerS<module_o>(interface_top)->set_description("Interface module for function: " + ResourceName);
      GetPointerS<module_o>(interface_top)->set_copyright(GENERATED_COPYRIGHT);
      GetPointerS<module_o>(interface_top)->set_authors("Component automatically generated by bambu");
      GetPointerS<module_o>(interface_top)->set_license(GENERATED_LICENSE);
      GetPointerS<module_o>(interface_top)->set_multi_unit_multiplicity(1U);
      const auto if_name = info.name == "ovalid" ? "valid" : info.name;
      const auto is_unbounded =
          if_name == "acknowledge" || if_name == "handshake" || if_name == "fifo" || if_name == "axis";

      const auto address_bitsize = HLSMgr->get_address_bitsize();
      const auto nbitDataSize = 64u - static_cast<unsigned>(__builtin_clzll(info.bitwidth));
      const structural_type_descriptorRef size1(new structural_type_descriptor("bool", 1));
      const structural_type_descriptorRef rwsize(new structural_type_descriptor("bool", nbitDataSize));
      const structural_type_descriptorRef dataType(new structural_type_descriptor("bool", info.bitwidth));
      const structural_type_descriptorRef addrType(new structural_type_descriptor("bool", address_bitsize));
      const structural_type_descriptorRef bool_type(new structural_type_descriptor("bool", 0));

      CM->add_port(CLOCK_PORT_NAME, port_o::IN, interface_top, bool_type);
      CM->add_port(RESET_PORT_NAME, port_o::IN, interface_top, bool_type);
      CM->add_port_vector(START_PORT_NAME, port_o::IN, 1U, interface_top, bool_type);
      if(is_unbounded)
      {
         CM->add_port_vector(DONE_PORT_NAME, port_o::OUT, 1U, interface_top, bool_type);
      }
      // in1 is not used by write interfaces, but is necessary to keep ParmMgr signature consistent
      CM->add_port("in1", port_o::IN, interface_top, size1);
      CM->add_port("in2", port_o::IN, interface_top, rwsize);
      CM->add_port("in3", port_o::IN, interface_top, dataType);
      const auto addrPort = CM->add_port("in4", port_o::IN, interface_top, addrType);
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

      CM->add_NP_functionality(interface_top, NP_functionality::LIBRARY, "in1 in2 in3 in4");
      const auto writer = parameters->getOption<HDLWriter_Language>(OPT_writer_language);
      if(if_name == "none" && writer == HDLWriter_Language::VHDL)
      {
         CM->add_NP_functionality(interface_top, NP_functionality::VHDL_GENERATOR,
                                  "Write" + getHDLGeneratorNameToken(if_name) + "HDLGenerator");
      }
      else
      {
         CM->add_NP_functionality(interface_top, NP_functionality::VERILOG_GENERATOR,
                                  "Write" + getHDLGeneratorNameToken(if_name) + "HDLGenerator");
      }
      TechMan->add_resource(INTERFACE_LIBRARY, ResourceName, CM);
      for(const auto& fdName : operations)
      {
         TechMan->add_operation(INTERFACE_LIBRARY, ResourceName, fdName);
      }
      auto fu = GetPointerS<functional_unit>(TechMan->get_fu(ResourceName, INTERFACE_LIBRARY));
      fu->area_m = std::make_shared<area_info>();
      fu->area_m->resources[area_info::AREA] = 0;
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
         op->time_m = std::make_shared<time_info>();
         op->bounded = op_bounded;
         op->time_m->set_execution_time(exec_time, cycles);
         if(op_bounded && is_registered)
         {
            op->time_m->set_stage_period(HLS_D->get_technology_manager()->CGetSetupHoldTime() + EPSILON);
            op->time_m->set_initiation_time(1U);
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
   const auto read_write_string = (isDP ? std::string("ReadWriteDP") : std::string("ReadWrite"));
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
      GetPointerS<module_o>(interface_top)->set_description("Interface module for function: " + ResourceName);
      GetPointerS<module_o>(interface_top)->set_copyright(GENERATED_COPYRIGHT);
      GetPointerS<module_o>(interface_top)->set_authors("Component automatically generated by bambu");
      GetPointerS<module_o>(interface_top)->set_license(GENERATED_LICENSE);
      GetPointerS<module_o>(interface_top)->set_multi_unit_multiplicity(n_resources);
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
                               read_write_string + getHDLGeneratorNameToken(info.name) + "HDLGenerator");
      TechMan->add_resource(INTERFACE_LIBRARY, ResourceName, CM);
      if(unused_port)
      {
         HLSMgr->unused_interfaces[root_id].insert(std::make_pair(INTERFACE_LIBRARY, ResourceName));
      }
      const auto fu = GetPointerS<functional_unit>(TechMan->get_fu(ResourceName, INTERFACE_LIBRARY));
      fu->area_m = std::make_shared<area_info>();
      fu->area_m->resources[area_info::AREA] = 0;
      HLSMgr->global_resource_constraints[std::make_pair(ResourceName, INTERFACE_LIBRARY)] =
          std::make_pair(n_resources, n_resources);
      INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "<--Interface resource created");
   }
   else
   {
      const auto fu = GetPointerS<functional_unit>(TechMan->get_fu(ResourceName, INTERFACE_LIBRARY));
      THROW_ASSERT(fu->CM && fu->CM->get_circ(),
                   "Missing structural description for interface resource '" + ResourceName + "'");
      const auto interface_top = fu->CM->get_circ();
      const auto module = GetPointerS<module_o>(interface_top);

      // Validate that the existing resource is compatible with this invocation.
#if HAVE_ASSERTS
      const auto existing_channel_count = module->get_multi_unit_multiplicity();
#endif
      THROW_ASSERT(existing_channel_count == 0U || existing_channel_count == n_resources,
                   "Incompatible channel count for array bundle '" + bundle_name + "' resource '" + ResourceName +
                       "': existing resource has " + STR(existing_channel_count) + " channels but " + STR(n_resources) +
                       " expected. The bundle must be used with consistent DP/SP mode across all dataflow modules.");
      // Validate bit-width: if a data port already exists its type must match.
      if(const auto existing_q = module->find_member("_" + bundle_name + "_q0", port_o_K, interface_top))
      {
#if HAVE_ASSERTS
         const auto existing_bitwidth = STD_GET_SIZE(GetPointerS<const port_o>(existing_q)->get_typeRef());
#endif
         THROW_ASSERT(existing_bitwidth == info.bitwidth,
                      "Incompatible data width for array bundle '" + bundle_name + "' resource '" + ResourceName +
                          "': existing resource has " + STR(existing_bitwidth) + "-bit data port but " +
                          STR(info.bitwidth) + " expected.");
      }
      // Validate global resource constraint consistency.
      const auto constraint_it =
          HLSMgr->global_resource_constraints.find(std::make_pair(ResourceName, INTERFACE_LIBRARY));
      if(constraint_it != HLSMgr->global_resource_constraints.end())
      {
         THROW_ASSERT(constraint_it->second.first == n_resources && constraint_it->second.second == n_resources,
                      "Incompatible resource constraint for array bundle '" + bundle_name + "' resource '" +
                          ResourceName + "': existing constraint " + STR(constraint_it->second.first) + "/" +
                          STR(constraint_it->second.second) + " differs from expected " + STR(n_resources));
      }
      // Validate HDL generator type: the existing resource must use the same generator.
      const auto expected_gen = read_write_string + getHDLGeneratorNameToken(info.name) + "HDLGenerator";
      const auto np_func = module->get_NP_functionality();
      THROW_ASSERT(np_func && np_func->exist_NP_functionality(NP_functionality::VERILOG_GENERATOR),
                   "Existing interface resource '" + ResourceName + "' for bundle '" + bundle_name +
                       "' is missing VERILOG_GENERATOR NP_functionality.");
      const auto existing_gen = np_func->get_NP_functionality(NP_functionality::VERILOG_GENERATOR);
      THROW_ASSERT(existing_gen == expected_gen,
                   "Incompatible HDL generator for array bundle '" + bundle_name + "' resource '" + ResourceName +
                       "': existing generator '" + existing_gen + "' differs from expected '" + expected_gen +
                       "'. The bundle must use a consistent interface type across all dataflow modules.");
      // Validate read/write operation set: any operations from this function that are already
      // registered in the existing resource must be consistent (same signature). Operations from
      // different functions using the same bundle are allowed and will be appended to the resource.
      // Validate write-port bit-width: if _d0 already exists its type must match.
      if(const auto existing_d = module->find_member("_" + bundle_name + "_d0", port_o_K, interface_top))
      {
#if HAVE_ASSERTS
         const auto existing_d_bitwidth = STD_GET_SIZE(GetPointerS<const port_o>(existing_d)->get_typeRef());
#endif
         THROW_ASSERT(existing_d_bitwidth == info.bitwidth,
                      "Incompatible data width for array bundle '" + bundle_name + "' resource '" + ResourceName +
                          "': existing _d0 port has " + STR(existing_d_bitwidth) + "-bit data port but " +
                          STR(info.bitwidth) + " expected.");
      }
      // Validate address port interface: check that the existing address port has the expected interface.
      if(const auto existing_addr = module->find_member("_" + bundle_name + "_address0", port_o_K, interface_top))
      {
#if HAVE_ASSERTS
         const auto addr_port = GetPointerS<const port_o>(existing_addr);
#endif
         THROW_ASSERT(addr_port->get_port_interface() == port_o::port_interface::PI_ADDRESS,
                      "Existing address port '" + bundle_name + "_address0" + "' in resource '" + ResourceName +
                          "' for bundle '" + bundle_name + "' has unexpected port interface " +
                          STR(addr_port->get_port_interface()) + " (expected PI_ADDRESS).");
      }
      // Validate direction and interface for existing ports.
      const auto check_port_interface = [&](const std::string& port_name, port_o::port_interface expected_if) {
         (void)expected_if;
         if(const auto port_k = module->find_member(port_name, port_o_K, interface_top))
         {
#if HAVE_ASSERTS
            const auto port = GetPointerS<const port_o>(port_k);
#endif
            THROW_ASSERT(port->get_port_interface() == expected_if,
                         "Incompatible port interface for '" + port_name + "' in resource '" + ResourceName +
                             "' (bundle '" + bundle_name + "): expected '" + STR(static_cast<unsigned>(expected_if)) +
                             "' but got '" + STR(static_cast<unsigned>(port->get_port_interface())) + "'.");
         }
      };
      check_port_interface("_" + bundle_name + "_address0", port_o::port_interface::PI_ADDRESS);
      if(isDP)
      {
         check_port_interface("_" + bundle_name + "_address1", port_o::port_interface::PI_ADDRESS);
      }
      check_port_interface("_" + bundle_name + "_ce0", port_o::port_interface::PI_CHIPENABLE);
      if(isDP)
      {
         check_port_interface("_" + bundle_name + "_ce1", port_o::port_interface::PI_CHIPENABLE);
      }
      if(!operationsR.empty() || unused_port)
      {
         check_port_interface("_" + bundle_name + "_q0", port_o::port_interface::PI_DIN);
         if(isDP)
         {
            check_port_interface("_" + bundle_name + "_q1", port_o::port_interface::PI_DIN);
         }
      }
      if(!operationsW.empty())
      {
         check_port_interface("_" + bundle_name + "_we0", port_o::port_interface::PI_WRITEENABLE);
         check_port_interface("_" + bundle_name + "_d0", port_o::port_interface::PI_DOUT);
         if(isDP)
         {
            check_port_interface("_" + bundle_name + "_we1", port_o::port_interface::PI_WRITEENABLE);
            check_port_interface("_" + bundle_name + "_d1", port_o::port_interface::PI_DOUT);
         }
      }

      const auto dataType = structural_type_descriptorRef(new structural_type_descriptor("bool", info.bitwidth));
      const auto bool_type = structural_type_descriptorRef(new structural_type_descriptor("bool", 0));

      const auto ensure_port = [&](const std::string& port_name, port_o::port_direction dir,
                                   const structural_type_descriptorRef& type_descr,
                                   const port_o::port_interface port_if) {
         if(!module->has_port(port_name))
         {
            const auto port = fu->CM->add_port(port_name, dir, interface_top, type_descr);
            GetPointerS<port_o>(port)->set_port_interface(port_if);
         }
      };

      if(!operationsR.empty() || unused_port)
      {
         ensure_port("_" + bundle_name + "_q0", port_o::IN, dataType, port_o::port_interface::PI_DIN);
         if(isDP)
         {
            ensure_port("_" + bundle_name + "_q1", port_o::IN, dataType, port_o::port_interface::PI_DIN);
         }
      }
      if(!operationsW.empty())
      {
         ensure_port("_" + bundle_name + "_we0", port_o::OUT, bool_type, port_o::port_interface::PI_WRITEENABLE);
         ensure_port("_" + bundle_name + "_d0", port_o::OUT, dataType, port_o::port_interface::PI_DOUT);
         if(isDP)
         {
            ensure_port("_" + bundle_name + "_we1", port_o::OUT, bool_type, port_o::port_interface::PI_WRITEENABLE);
            ensure_port("_" + bundle_name + "_d1", port_o::OUT, dataType, port_o::port_interface::PI_DOUT);
         }
      }
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
   const auto bram_f_unit =
       TechMan->get_fu(isDP ? ARRAY_1D_STD_BRAM_NN_SDS :
                              (HLSMgr->UseSinglePortSdsMemory() ? ARRAY_1D_STD_BRAM_SDS1 : ARRAY_1D_STD_BRAM_SDS),
                       LIBRARY_STD_FU);
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
      op->time_m = std::make_shared<time_info>();
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
      op->time_m = std::make_shared<time_info>();
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
      GetPointerS<module_o>(interface_top)->set_description("Interface module for function: " + ResourceName);
      GetPointerS<module_o>(interface_top)->set_copyright(GENERATED_COPYRIGHT);
      GetPointerS<module_o>(interface_top)->set_authors("Component automatically generated by bambu");
      GetPointerS<module_o>(interface_top)->set_license(GENERATED_LICENSE);
      GetPointerS<module_o>(interface_top)->set_multi_unit_multiplicity(1U);

      const auto address_bitsize = HLSMgr->get_address_bitsize();
      const auto interface_bitwidth = iface_attrs.find(FunctionArchitecture::iface_bitwidth) != iface_attrs.end() ?
                                          std::stoull(iface_attrs.find(FunctionArchitecture::iface_bitwidth)->second) :
                                          info.bitwidth;
      const auto nbitDataSize = 64u - static_cast<unsigned>(__builtin_clzll(interface_bitwidth));
      const auto backEndBitsize =
          iface_attrs.find(FunctionArchitecture::iface_cache_bus_size) != iface_attrs.end() ?
              std::stoull(iface_attrs.find(FunctionArchitecture::iface_cache_bus_size)->second) :
              interface_bitwidth;

      const structural_type_descriptorRef address_interface_datatype(
          new structural_type_descriptor("bool", address_bitsize));
      const structural_type_descriptorRef size1(new structural_type_descriptor("bool", 1));
      const structural_type_descriptorRef rwsize(new structural_type_descriptor("bool", nbitDataSize));
      const structural_type_descriptorRef rwtypeIn(new structural_type_descriptor("bool", interface_bitwidth));
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

      const auto cache_reset_port = CM->add_port(CACHE_RESET_PORT_NAME, port_o::IN, interface_top, bool_type);
      GetPointerS<port_o>(cache_reset_port)->set_port_interface(port_o::port_interface::CACHE_RESET);

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
         THROW_ERROR_USAGE("Only 'direct' AXI interfaces are supported for this configuration.");
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
                               "ReadWrite" + getHDLGeneratorNameToken(info.name) + "HDLGenerator");

      TechMan->add_resource(INTERFACE_LIBRARY, ResourceName, CM);
      if(unused_port)
      {
         HLSMgr->unused_interfaces[root_id].insert(std::make_pair(INTERFACE_LIBRARY, ResourceName));
      }
      const auto fu = GetPointerS<functional_unit>(TechMan->get_fu(ResourceName, INTERFACE_LIBRARY));
      fu->area_m = std::make_shared<area_info>();
      fu->area_m->resources[area_info::AREA] = 0;
      if(iface_attrs.find(FunctionArchitecture::iface_cache_line_count) != iface_attrs.end())
      {
         const auto flushName = ENCODE_FDNAME(bundle_name, "_Flush_", "m_axi");
         TechMan->add_operation(INTERFACE_LIBRARY, ResourceName, flushName);
         const auto op = GetPointer<operation>(fu->get_operation(flushName));
         op->time_m = std::make_shared<time_info>();
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
      op->time_m = std::make_shared<time_info>();
      op->bounded = false;
      op->time_m->set_execution_time(HLS_D->get_technology_manager()->CGetSetupHoldTime() + EPSILON, 0);
      op->time_m->set_synthesis_dependent(true);
   }
   for(const auto& fdName : operationsW)
   {
      const auto op = GetPointer<operation>(fu->get_operation(fdName));
      op->time_m = std::make_shared<time_info>();
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
      THROW_ERROR_USAGE("Interface not supported: " + info.name + ". Select a supported interface type.");
   }
}
