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
 *              Copyright (C) 2004-2026 Politecnico di Milano
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 *
 *   This file is part of the PandA framework.
 *
 *   Licensed under the Apache License, Version 2.0, with BAMBU exceptions (the "License");
 *   you may not use this file except in compliance with the License.
 *   You may obtain a copy of the License at
 *
 *       http://www.apache.org/licenses/LICENSE-2.0
 *
 *   Unless required by applicable law or agreed to in writing, software
 *   distributed under the License is distributed on an "AS IS" BASIS,
 *   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *   See the License for the specific language governing permissions and
 *   limitations under the License.
 *
 */
/**
 * @file function_behavior.cpp
 * @brief A brief description of the C++ Source File
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 * @author Marco Lattuada <lattuada@elet.polimi.it>
 * @author Christian Pilato <pilato@elet.polimi.it>
 *
 */
#include "function_behavior.hpp"

#include "BasicBlockReachability.hpp"
#include "Parameter.hpp"
#include "SemiNCADominance.hpp"
#include "application_manager.hpp"
#include "basic_block.hpp"
#include "basic_blocks_graph_constructor.hpp"
#include "behavioral_helper.hpp"
#include "cdfg_edge_info.hpp"
#include "config_HAVE_HOST_PROFILING_BUILT.hpp"
#include "custom_set.hpp"
#include "exceptions.hpp"
#include "graph.hpp"
#include "ir_helper.hpp"
#include "ir_manager.hpp"
#include "ir_node.hpp"
#include "op_graph.hpp"
#include "operations_graph_constructor.hpp"
#include "typed_node_info.hpp"
#include "utility.hpp"
#if HAVE_HOST_PROFILING_BUILT
#include "profiling_information.hpp"
#endif
#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/filtered_graph.hpp>
#include <boost/iterator/filter_iterator.hpp>
#include <boost/iterator/iterator_facade.hpp>
#include <boost/tuple/tuple.hpp>
#include <list>
#include <ostream>
#include <string>
#include <utility>

#if __GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 6)
#pragma GCC diagnostic push
#pragma GCC diagnostic error "-Weffc++"
#endif
memory_access::memory_access(unsigned int _node_id, unsigned int _base_address, unsigned int _offset)
    : node_id(_node_id), base_address(_base_address), offset(_offset)
{
}

OMPInfo::OMPInfo(unsigned int _local_idx, unsigned int _context_count, unsigned int _core_id, unsigned int _ncore,
                 unsigned int _fork_call_id, const OMPInfoConstRef& _parent, unsigned int _kmp_t_nproc)
    : fork_call_id(_fork_call_id),
      ncore(_ncore),
      core_id(_core_id),
      kmp_t_nproc(_kmp_t_nproc),
      context_count(_context_count),
      mem_page_size(0ULL),
      critical(),
      local_idx(_local_idx),
      global_idx(make_global(_local_idx, _ncore, _parent)),
      parent(_parent)
{
}

unsigned int OMPInfo::make_global(unsigned int idx, unsigned int ncore, OMPInfoConstRef _parent)
{
   if(!idx)
   {
      return _parent ? _parent->global_idx : 0U;
   }
   if(_parent)
   {
      auto ancestor_idx = _parent->parent ? _parent->parent->local_idx : 0U;
      idx = (_parent->ncore * ancestor_idx + _parent->local_idx) * (ncore - 1U) + idx;
      unsigned int prev_idx = 0U;
      while(_parent->parent)
      {
         const auto ancestor_ncore = _parent->parent->ncore;
         prev_idx += (_parent->ncore - 1U) * ancestor_ncore;
         _parent = _parent->parent;
      }
      const auto root_ncore = _parent->ncore;
      return (root_ncore - 1U) + prev_idx + idx;
   }
   return idx;
}

FunctionBehavior::FunctionBehavior(const application_managerConstRef _AppM, const BehavioralHelperRef _helper,
                                   const ParameterConstRef _parameters)
    : helper(_helper),
      bb_graphs_collection(new BBGraphsCollection(BBGraphInfo(_AppM, _helper->get_function_index()))),
      op_graphs_collection(new OpGraphsCollection(OpGraphInfo(helper))),
#if HAVE_HOST_PROFILING_BUILT
      profiling_information(ProfilingInformationRef(new ProfilingInformation(bb_graphs_collection.get()))),
#endif
      map_levels(),
      bb_map_levels(),
      deque_levels(),
      bb_deque_levels(),
      loops(),
      mem_nodeID(),
      dynamic_address(),
      parm_decl_copied(),
      parm_decl_loaded(),
      parm_decl_stored(),
      parameters(_parameters),
      dereference_unknown_address(false),
      unaligned_accesses(false),
      bb_version(1),
      bitvalue_version(1),
      has_globals(false),
      has_undefined_function_receiveing_pointers(false),
      state_variables(),
      pipeline_enabled(false),
      is_stallable_pipelined_function(false),
      initiation_time(1),
      _channels_number(
          _parameters->isOption(OPT_channels_number) ? _parameters->getOption<unsigned int>(OPT_channels_number) : 0),
      _channels_type(_parameters->getOption<MemoryAllocation_ChannelsType>(OPT_channels_type)),
      _allocation_policy(_parameters->getOption<MemoryAllocation_Policy>(OPT_memory_allocation_policy)),
      _omp_core(false),
      _omp_info(nullptr),
      ogc(new operations_graph_constructor(*op_graphs_collection)),
      bbgc(new BasicBlocksGraphConstructor(*bb_graphs_collection)),
      dominators(nullptr),
      post_dominators(nullptr),
      memory_info(),
      packed_vars(false)
{
   const auto fnode = _AppM->get_ir_manager()->GetIRNode(_helper->get_function_index());
   THROW_ASSERT(fnode->get_kind() == function_val_node_K,
                "Called function_behavior on a node which is not a function_val_node");
   auto* fd = GetPointerS<function_val_node>(fnode);
   const auto fname = helper->GetFunctionName();
   const auto out_lvl = _parameters->getOption<int>(OPT_output_level);

   pipeline_enabled = fd->is_pipelined();
   is_stallable_pipelined_function = fd->get_pipeline_style() == function_val_node::STP_STYLE;
   initiation_time = fd->get_initiation_time();

   if(_parameters->isOption(OPT_pipelining))
   {
      auto tmp_string = _parameters->getOption<std::string>(OPT_pipelining);
      if(tmp_string.at(0) == '=')
      {
         tmp_string = tmp_string.substr(1);
      }
      if(tmp_string == "no-@ll")
      {
         // force no pipelining
      }
      else if(tmp_string == "@ll")
      {
         pipeline_enabled = true;
         initiation_time = 1;
      }
      else
      {
         const auto funcs_values = string_to_container<std::vector<std::string>>(tmp_string, ",");
         const auto demangle_fname = [&]() {
            std::string fsymbol = cxa_demangle(fname);
            return fsymbol.substr(0, fsymbol.find('('));
         }();
         for(const auto& fun_pipeline : funcs_values)
         {
            const auto fsymbol = fun_pipeline.substr(0, fun_pipeline.find('='));
            const auto ii_str = fun_pipeline.substr(fsymbol.size() + (fun_pipeline.size() > fsymbol.size()));
            if(fsymbol.size() && (fname == fsymbol || demangle_fname == fsymbol ||
                                  (fname.find("__float") == 0 && fname.find(fsymbol) == 0)))
            {
               pipeline_enabled = true;
               initiation_time = ii_str.size() ? static_cast<unsigned int>(std::stoul((ii_str))) : 1U;
            }
         }
      }
   }

   if(pipeline_enabled)
   {
      INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, out_lvl,
                     "Required pipelining with II=" + STR(initiation_time) + " for function: " + fname);
   }
}

const OpGraphsCollection& FunctionBehavior::GetOpGraphsCollection() const
{
   return *op_graphs_collection;
}

OpGraph FunctionBehavior::GetOpGraph(FunctionBehavior::graph_type gt) const
{
   switch(gt)
   {
      case CFG:
         return OpGraph(*op_graphs_collection, CFG_SELECTOR);
      case FCFG:
         return OpGraph(*op_graphs_collection, FCFG_SELECTOR);
      case CDG:
         return OpGraph(*op_graphs_collection, CDG_SELECTOR);
      case FCDG:
         return OpGraph(*op_graphs_collection, FCDG_SELECTOR);
      case DFG:
         return OpGraph(*op_graphs_collection, DFG_SELECTOR);
      case FDFG:
         return OpGraph(*op_graphs_collection, FDFG_SELECTOR);
      case ADG:
         return OpGraph(*op_graphs_collection, ADG_SELECTOR);
      case FADG:
         return OpGraph(*op_graphs_collection, FADG_SELECTOR);
      case ODG:
         return OpGraph(*op_graphs_collection, ODG_SELECTOR);
      case FODG:
         return OpGraph(*op_graphs_collection, FODG_SELECTOR);
      case SDG:
         return OpGraph(*op_graphs_collection, SDG_SELECTOR);
      case FSDG:
         return OpGraph(*op_graphs_collection, FSDG_SELECTOR);
      case SAODG:
         return OpGraph(*op_graphs_collection, SDG_SELECTOR | ADG_SELECTOR | ODG_SELECTOR);
      case FSAODG:
         return OpGraph(*op_graphs_collection, FSDG_SELECTOR | FADG_SELECTOR | FODG_SELECTOR);
      case FLSAODG:
         return OpGraph(*op_graphs_collection, FLG_SELECTOR | SDG_SELECTOR | ADG_SELECTOR | ODG_SELECTOR);
#ifndef NDEBUG
      case FLSAODDG:
         return OpGraph(*op_graphs_collection,
                        FLG_SELECTOR | SDG_SELECTOR | ADG_SELECTOR | ODG_SELECTOR | DEBUG_SELECTOR);
#endif
      case FFLSAODG:
         return OpGraph(*op_graphs_collection, FLG_SELECTOR | FSDG_SELECTOR | FADG_SELECTOR | FODG_SELECTOR);
      case FLAODDG:
         return OpGraph(*op_graphs_collection, FLG_SELECTOR | ADG_SELECTOR | ODG_SELECTOR | DFG_SELECTOR);
      case FFLAODDG:
         return OpGraph(*op_graphs_collection, FLG_SELECTOR | FADG_SELECTOR | FODG_SELECTOR | FDFG_SELECTOR);
      case FLG:
         return OpGraph(*op_graphs_collection, FLG_SELECTOR);
      case SG:
         return OpGraph(*op_graphs_collection, SG_SELECTOR | FLG_SELECTOR);
      case AGG_VIRTUALG:
         return OpGraph(*op_graphs_collection, DFG_AGG_SELECTOR | ADG_AGG_SELECTOR);
      default:
         break;
   }
   THROW_UNREACHABLE("Not supported graph type");
   return OpGraph(*op_graphs_collection, CFG_SELECTOR);
}

BehavioralHelperRef FunctionBehavior::GetBehavioralHelper()
{
   return helper;
}

const BehavioralHelperConstRef FunctionBehavior::CGetBehavioralHelper() const
{
   return helper;
}

/// optimization in case the subset is equal to the whole set of vertices is possible
OpGraph FunctionBehavior::GetOpGraph(FunctionBehavior::graph_type gt,
                                     const CustomUnorderedSet<OpGraph::vertex_descriptor>& subset) const
{
   if(subset.size() == op_graphs_collection->num_vertices())
   {
      return GetOpGraph(gt);
   }

   switch(gt)
   {
      case CFG:
         return OpGraph(*op_graphs_collection, CFG_SELECTOR, subset);

      case FCFG:
         return OpGraph(*op_graphs_collection, FCFG_SELECTOR, subset);

      case CDG:
         return OpGraph(*op_graphs_collection, CDG_SELECTOR, subset);

      case FCDG:
         return OpGraph(*op_graphs_collection, FCDG_SELECTOR, subset);

      case DFG:
         return OpGraph(*op_graphs_collection, DFG_SELECTOR, subset);

      case FDFG:
         return OpGraph(*op_graphs_collection, FDFG_SELECTOR, subset);

      case ADG:
         return OpGraph(*op_graphs_collection, ADG_SELECTOR, subset);

      case FADG:
         return OpGraph(*op_graphs_collection, ADG_SELECTOR | FB_ADG_SELECTOR, subset);

      case ODG:
         return OpGraph(*op_graphs_collection, ODG_SELECTOR, subset);

      case FODG:
         return OpGraph(*op_graphs_collection, FODG_SELECTOR, subset);

      case SDG:
         return OpGraph(*op_graphs_collection, SDG_SELECTOR, subset);

      case FSDG:
         return OpGraph(*op_graphs_collection, FSDG_SELECTOR, subset);

      case SAODG:
         return OpGraph(*op_graphs_collection, SAODG_SELECTOR, subset);

      case FSAODG:
         return OpGraph(*op_graphs_collection, FSDG_SELECTOR | FADG_SELECTOR | FODG_SELECTOR | FDFG_SELECTOR, subset);

      case FLSAODG:
         return OpGraph(*op_graphs_collection, SAODG_SELECTOR | FLG_SELECTOR, subset);

#ifndef NDEBUG
      case FLSAODDG:
         return OpGraph(*op_graphs_collection, SAODG_SELECTOR | FLG_SELECTOR | DEBUG_SELECTOR, subset);
#endif

      case FFLSAODG:
         return OpGraph(*op_graphs_collection, FLG_SELECTOR | FSDG_SELECTOR | FADG_SELECTOR | FODG_SELECTOR, subset);

      case FLAODDG:
         return OpGraph(*op_graphs_collection, DFG_SELECTOR | ADG_SELECTOR | ODG_SELECTOR | FLG_SELECTOR, subset);

      case FFLAODDG:
         return OpGraph(*op_graphs_collection, FLG_SELECTOR | FADG_SELECTOR | FODG_SELECTOR | FDFG_SELECTOR);

      case FLG:
         return OpGraph(*op_graphs_collection, FLG_SELECTOR, subset);

      case SG:
         return OpGraph(*op_graphs_collection, SG_SELECTOR, subset);

      case AGG_VIRTUALG:
         return OpGraph(*op_graphs_collection, DFG_AGG_SELECTOR | ADG_AGG_SELECTOR, subset);
      default:
         THROW_UNREACHABLE("");
   }
   return GetOpGraph(gt);
}

const BBGraphsCollection& FunctionBehavior::GetBBGraphsCollection() const
{
   return *bb_graphs_collection;
}

BBGraph FunctionBehavior::GetBBGraph(FunctionBehavior::bb_graph_type gt) const
{
   switch(gt)
   {
      case BB:
         return BBGraph(*bb_graphs_collection, CFG_SELECTOR);
      case FBB:
         return BBGraph(*bb_graphs_collection, FCFG_SELECTOR);
      case CDG_BB:
         return BBGraph(*bb_graphs_collection, CDG_SELECTOR);
      case DOM_TREE:
         return BBGraph(*bb_graphs_collection, D_SELECTOR);
      case POST_DOM_TREE:
         return BBGraph(*bb_graphs_collection, PD_SELECTOR);
      case DJ:
         return BBGraph(*bb_graphs_collection, D_SELECTOR | J_SELECTOR);
      default:
         break;
   }
   THROW_UNREACHABLE("");
   return BBGraph(*bb_graphs_collection, CFG_SELECTOR);
}

void FunctionBehavior::print(std::ostream& os) const
{
   os << "Function " << helper->GetFunctionName();
   // os << "Bulk operation graph of " << og;
   // os << ", , ";
}

void FunctionBehavior::add_level(gc_vertex_descriptor v, unsigned int index)
{
   map_levels[v] = index;
   deque_levels.push_back(v);
}

const std::deque<gc_vertex_descriptor>& FunctionBehavior::get_levels() const
{
   return deque_levels;
}

const std::map<gc_vertex_descriptor, unsigned int>& FunctionBehavior::get_map_levels() const
{
   return map_levels;
}

void FunctionBehavior::add_bb_level(gc_vertex_descriptor v, unsigned int index)
{
   bb_map_levels[v] = index;
   bb_deque_levels.push_back(v);
}

const std::deque<gc_vertex_descriptor>& FunctionBehavior::get_bb_levels() const
{
   return bb_deque_levels;
}

const std::map<gc_vertex_descriptor, unsigned int>& FunctionBehavior::get_bb_map_levels() const
{
   return bb_map_levels;
}

LoopsConstRef FunctionBehavior::getConstLoops() const
{
   return loops;
}

LoopsRef FunctionBehavior::getLoops() const
{
   return loops;
}

#if HAVE_HOST_PROFILING_BUILT
const ProfilingInformationConstRef FunctionBehavior::CGetProfilingInformation() const
{
   return profiling_information;
}
#endif

void FunctionBehavior::add_function_mem(unsigned int node_id)
{
   mem_nodeID.insert(node_id);
}

void FunctionBehavior::add_dynamic_address(unsigned int node_id)
{
   // std::cerr << "addr taken " << node_id << std::endl;
   dynamic_address.insert(node_id);
   /// the object may be written once you have the address
}

void FunctionBehavior::clean_dynamic_address()
{
   dynamic_address.clear();
}

bool FunctionBehavior::is_variable_mem(unsigned int node_id) const
{
   return mem_nodeID.find(node_id) != mem_nodeID.end();
}

void FunctionBehavior::add_parm_decl_copied(unsigned int node_id)
{
   parm_decl_copied.insert(node_id);
   dynamic_address.insert(node_id);
}

void FunctionBehavior::add_parm_decl_loaded(unsigned int node_id)
{
   parm_decl_loaded.insert(node_id);
   dynamic_address.insert(node_id);
}

void FunctionBehavior::add_parm_decl_stored(unsigned int node_id)
{
   parm_decl_stored.insert(node_id);
   dynamic_address.insert(node_id);
}

const CustomOrderedSet<unsigned int>& FunctionBehavior::get_function_mem() const
{
   return mem_nodeID;
}

void FunctionBehavior::clean_function_mem()
{
   mem_nodeID.clear();
}

const CustomOrderedSet<unsigned int>& FunctionBehavior::get_dynamic_address() const
{
   return dynamic_address;
}

const CustomOrderedSet<unsigned int>& FunctionBehavior::get_parm_decl_copied() const
{
   return parm_decl_copied;
}

void FunctionBehavior::clean_parm_decl_copied()
{
   parm_decl_copied.clear();
}

const CustomOrderedSet<unsigned int>& FunctionBehavior::get_parm_decl_loaded() const
{
   return parm_decl_loaded;
}

void FunctionBehavior::clean_parm_decl_loaded()
{
   parm_decl_loaded.clear();
}

const CustomOrderedSet<unsigned int>& FunctionBehavior::get_parm_decl_stored() const
{
   return parm_decl_stored;
}

void FunctionBehavior::clean_parm_decl_stored()
{
   parm_decl_stored.clear();
}

CustomOrderedSet<unsigned int> FunctionBehavior::get_local_variables(const application_managerConstRef AppM) const
{
   CustomOrderedSet<unsigned int> vars;
   // I simply have to go over all the vertices and get the used variables;
   // the variables which have to be declared are all those variables but
   // the globals ones
   const auto cfg = GetOpGraph(FunctionBehavior::CFG);
   for(const auto& v : cfg.vertices())
   {
      const auto& varsTemp = cfg.CGetNodeInfo(v).cited_variables;
      vars.insert(varsTemp.begin(), varsTemp.end());
   }
   for(const auto& funParam : helper->GetParameters())
   {
      vars.erase(funParam->index);
   }
   for(const auto& gblVariable : AppM->GetGlobalVariables())
   {
      vars.erase(gblVariable->index);
   }
   return vars;
}

bool FunctionBehavior::CheckBBReachability(BBGraph::vertex_descriptor first_basic_block,
                                           BBGraph::vertex_descriptor second_basic_block) const
{
   if(first_basic_block == second_basic_block)
   {
      return false;
   }
   const auto fcfg = GetBBGraph(FunctionBehavior::BB);
   return reachability::HasPath(fcfg, first_basic_block, second_basic_block);
}

bool FunctionBehavior::CheckBBFeedbackReachability(BBGraph::vertex_descriptor first_basic_block,
                                                   BBGraph::vertex_descriptor second_basic_block) const
{
   const auto fcfg = GetBBGraph(FunctionBehavior::FBB);
   return reachability::HasPath(fcfg, first_basic_block, second_basic_block);
}

bool FunctionBehavior::CheckReachability(OpGraph::vertex_descriptor first_operation,
                                         OpGraph::vertex_descriptor second_operation) const
{
   const auto& bb_index_map = bb_graphs_collection->CGetGraphInfo().bb_index_map;
   const unsigned int first_bb_index = op_graphs_collection->CGetNodeInfo(first_operation).bb_index;
   const unsigned int second_bb_index = op_graphs_collection->CGetNodeInfo(second_operation).bb_index;
   const auto first_bb_vertex = bb_index_map.find(first_bb_index)->second;
   const auto second_bb_vertex = bb_index_map.find(second_bb_index)->second;
   if(CheckBBReachability(first_bb_vertex, second_bb_vertex))
   {
      return true;
   }
   if(first_bb_vertex == second_bb_vertex)
   {
      THROW_ASSERT(map_levels.size(), "");
      THROW_ASSERT(map_levels.find(first_operation) != map_levels.end(),
                   "Level of " + op_graphs_collection->CGetNodeInfo(first_operation).vertex_name + " not found");
      THROW_ASSERT(map_levels.find(second_operation) != map_levels.end(),
                   "Level of " + op_graphs_collection->CGetNodeInfo(second_operation).vertex_name + " not found");
      if(map_levels.find(first_operation)->second < map_levels.find(second_operation)->second)
      {
         return true;
      }
   }
   return false;
}

bool FunctionBehavior::CheckFeedbackReachability(OpGraph::vertex_descriptor first_operation,
                                                 OpGraph::vertex_descriptor second_operation) const
{
   const auto& bb_index_map = bb_graphs_collection->CGetGraphInfo().bb_index_map;
   const auto first_bb_index = op_graphs_collection->CGetNodeInfo(first_operation).bb_index;
   const auto second_bb_index = op_graphs_collection->CGetNodeInfo(second_operation).bb_index;
   const auto first_bb_vertex = bb_index_map.find(first_bb_index)->second;
   const auto second_bb_vertex = bb_index_map.find(second_bb_index)->second;
   return CheckBBFeedbackReachability(first_bb_vertex, second_bb_vertex);
}

unsigned int FunctionBehavior::GetBBVersion() const
{
   return bb_version;
}

unsigned int FunctionBehavior::UpdateBBVersion()
{
   bb_version++;
   return bb_version;
}

unsigned int FunctionBehavior::GetBitValueVersion() const
{
   return bitvalue_version;
}

unsigned int FunctionBehavior::UpdateBitValueVersion()
{
   bitvalue_version++;
   return bitvalue_version;
}

unsigned int FunctionBehavior::GetChannelsNumber() const
{
   return _channels_number;
}

void FunctionBehavior::SetChannelsNumber(unsigned int val)
{
   _channels_number = val;
}

MemoryAllocation_ChannelsType FunctionBehavior::GetChannelsType() const
{
   return _channels_type;
}

void FunctionBehavior::SetChannelsType(MemoryAllocation_ChannelsType val)
{
   _channels_type = val;
}

MemoryAllocation_Policy FunctionBehavior::GetMemoryAllocationPolicy() const
{
   return _allocation_policy;
}

void FunctionBehavior::SetMemoryAllocationPolicy(MemoryAllocation_Policy val)
{
   _allocation_policy = val;
}

bool FunctionBehavior::IsOMPCore() const
{
   return _omp_core;
}

void FunctionBehavior::SetOMPCore(bool val)
{
   _omp_core = val;
}

OMPInfoRef FunctionBehavior::GetOMPInfo() const
{
   return _omp_info;
}

void FunctionBehavior::SetOMPInfo(OMPInfoRef info)
{
   THROW_ASSERT(info, "");
   _omp_info = info;
}

#if __GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 6)
#pragma GCC diagnostic pop
#endif

std::filesystem::path FunctionBehavior::GetDotPath() const
{
   THROW_ASSERT(parameters->getOption<bool>(OPT_print_dot), "unexpected condition");
   auto function_name = helper->GetFunctionName();
   if(function_name.size() > 256)
   {
      THROW_WARNING("Function name too long: " + function_name +
                    ".\nChanged to the the function index:" + STR(helper->get_function_index()));
      function_name = STR(helper->get_function_index());
   }
   return parameters->getOption<std::filesystem::path>(OPT_dot_directory) / function_name;
}
