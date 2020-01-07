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
 * @file function_behavior.cpp
 * @brief A brief description of the C++ Source File
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 * @author Marco Lattuada <lattuada@elet.polimi.it>
 * @author Christian Pilato <pilato@elet.polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *
 */
#include "function_behavior.hpp"

#include "config_HAVE_EXPERIMENTAL.hpp"
#include "config_HAVE_HOST_PROFILING_BUILT.hpp"
#include "config_HAVE_POLIXML_BUILT.hpp"

#include "Dominance.hpp"                        // for dominance
#include "Parameter.hpp"                        // for ParameterConstRef
#include "application_manager.hpp"              // for application_manager
#include "basic_block.hpp"                      // for BBGraph, BBGraphInfo
#include "basic_blocks_graph_constructor.hpp"   // for BBGraphRef, BasicBl...
#include "behavioral_helper.hpp"                // for BehavioralHelper
#include "cdfg_edge_info.hpp"                   // for CFG_SELECTOR, CDG_S...
#include "config_HAVE_EXPERIMENTAL.hpp"         // for HAVE_EXPERIMENTAL
#include "config_HAVE_HOST_PROFILING_BUILT.hpp" // for HAVE_HOST_PROFILING...
#include "config_HAVE_POLIXML_BUILT.hpp"        // for HAVE_POLIXML_BUILT
#include "custom_set.hpp"                       // for CustomSet
#include "exceptions.hpp"                       // for THROW_ASSERT, THROW...
#include "graph.hpp"                            // for vertex, VertexIterator
#include "level_constructor.hpp"                // for level_constructor
#include "loop.hpp"                             // for LoopsRef
#include "loops.hpp"                            // for ProfilingInformatio...
#include "op_graph.hpp"                         // for OpGraph, OpGraphCon...
#include "operations_graph_constructor.hpp"     // for OpGraphRef, operati...
#include <boost/graph/adjacency_list.hpp>       // for adjacency_list
#include <boost/graph/filtered_graph.hpp>       // for filtered_graph<>::v...
#include <boost/iterator/filter_iterator.hpp>   // for filter_iterator
#include <boost/iterator/iterator_facade.hpp>   // for operator!=, operator++
#include <boost/tuple/tuple.hpp>                // for tie
#include <list>                                 // for list, _List_const_i...
#include <ostream>                              // for operator<<, basic_o...
#include <string>                               // for operator+, char_traits
#include <utility>                              // for pair
#if HAVE_HOST_PROFILING_BUILT
#include "profiling_information.hpp" // for BBGraphConstRef
#endif
#include "typed_node_info.hpp" // for GET_NAME

#if HAVE_EXPERIMENTAL
#include "epd_graph.hpp"
#include "extended_pdg_constructor.hpp"
#include "op_graph.hpp"
#include "parallel_regions_graph.hpp"
#include "parallel_regions_graph_constructor.hpp"
#endif

#if __GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 6)
#pragma GCC diagnostic push
#pragma GCC diagnostic error "-Weffc++"
#endif
memory_access::memory_access(unsigned int _node_id, unsigned int _base_address, unsigned int _offset) : node_id(_node_id), base_address(_base_address), offset(_offset)
{
}

FunctionBehavior::FunctionBehavior(const application_managerConstRef _AppM, const BehavioralHelperRef _helper, const ParameterConstRef _parameters)
    : helper(_helper),
      bb_graphs_collection(new BBGraphsCollection(BBGraphInfoRef(new BBGraphInfo(_AppM, _helper->get_function_index())), _parameters)),
      op_graphs_collection(new OpGraphsCollection(OpGraphInfoRef(new OpGraphInfo(helper)), _parameters)),
      bb(new BBGraph(bb_graphs_collection, CFG_SELECTOR)),
      extended_bb(new BBGraph(bb_graphs_collection, CFG_SELECTOR | ECFG_SELECTOR)),
      cdg_bb(new BBGraph(bb_graphs_collection, CDG_SELECTOR)),
      dj(new BBGraph(bb_graphs_collection, D_SELECTOR | J_SELECTOR)),
      dt(new BBGraph(bb_graphs_collection, D_SELECTOR)),
      fbb(new BBGraph(bb_graphs_collection, FCFG_SELECTOR)),
      pdt(new BBGraph(bb_graphs_collection, PD_SELECTOR)),
      ppg(new BBGraph(bb_graphs_collection, PP_SELECTOR | CFG_SELECTOR)),
      cfg(new OpGraph(op_graphs_collection, CFG_SELECTOR)),
      extended_cfg(new OpGraph(op_graphs_collection, CFG_SELECTOR | ECFG_SELECTOR)),
      fcfg(new OpGraph(op_graphs_collection, FCFG_SELECTOR)),
      adg(new OpGraph(op_graphs_collection, ADG_SELECTOR)),
      fadg(new OpGraph(op_graphs_collection, FADG_SELECTOR)),
      cdg(new OpGraph(op_graphs_collection, CDG_SELECTOR)),
      fcdg(new OpGraph(op_graphs_collection, FCDG_SELECTOR)),
      dfg(new OpGraph(op_graphs_collection, DFG_SELECTOR)),
      fdfg(new OpGraph(op_graphs_collection, FDFG_SELECTOR)),
      flg(new OpGraph(op_graphs_collection, FLG_SELECTOR)),
      odg(new OpGraph(op_graphs_collection, ODG_SELECTOR)),
      fodg(new OpGraph(op_graphs_collection, FODG_SELECTOR)),
      flaoddg(new OpGraph(op_graphs_collection, FLG_SELECTOR | ADG_SELECTOR | ODG_SELECTOR | DFG_SELECTOR)),
      fflaoddg(new OpGraph(op_graphs_collection, FLG_SELECTOR | FADG_SELECTOR | FODG_SELECTOR | FDFG_SELECTOR)),
      flsaodg(new OpGraph(op_graphs_collection, FLG_SELECTOR | SDG_SELECTOR | ADG_SELECTOR | ODG_SELECTOR)),
#ifndef NDEBUG
      flsaoddg(new OpGraph(op_graphs_collection, FLG_SELECTOR | SDG_SELECTOR | ADG_SELECTOR | ODG_SELECTOR | DEBUG_SELECTOR)),
#endif
      fflsaodg(new OpGraph(op_graphs_collection, FLG_SELECTOR | FSDG_SELECTOR | FADG_SELECTOR | FODG_SELECTOR)),
      saodg(new OpGraph(op_graphs_collection, SDG_SELECTOR | ADG_SELECTOR | ODG_SELECTOR)),
      fsaodg(new OpGraph(op_graphs_collection, FSDG_SELECTOR | FADG_SELECTOR | FODG_SELECTOR)),
      sdg(new OpGraph(op_graphs_collection, SDG_SELECTOR)),
      fsdg(new OpGraph(op_graphs_collection, FSDG_SELECTOR)),
#if HAVE_EXPERIMENTAL
      rpdg(new OpGraph(op_graphs_collection, RPDG_SELECTOR)),
#endif
      sg(new OpGraph(op_graphs_collection, SG_SELECTOR | FLG_SELECTOR)),
#if HAVE_EXPERIMENTAL
      epdg_bulk(new EpdGraphsCollection(saodg, _parameters)),
      cepdg(new EpdGraph(epdg_bulk, EpdEdgeInfo::EDGE_CONTROL /*| EpdEdgeInfo::EDGE_CONTROL_FLOW*/)),
      depdg(new EpdGraph(epdg_bulk, EpdEdgeInfo::EDGE_DATA_RAW | EpdEdgeInfo::EDGE_DATA_WAW_WAR)),
      cdepdg(new EpdGraph(epdg_bulk, EpdEdgeInfo::EDGE_DATA_RAW | EpdEdgeInfo::EDGE_DATA_WAW_WAR | EpdEdgeInfo::EDGE_CONTROL)),
      cdcfepdg(new EpdGraph(epdg_bulk, EpdEdgeInfo::EDGE_DATA_RAW | EpdEdgeInfo::EDGE_DATA_WAW_WAR | EpdEdgeInfo::EDGE_CONTROL | EpdEdgeInfo::EDGE_CONTROL_FLOW)),
      epdg(new EpdGraph(epdg_bulk, EpdEdgeInfo::EDGE_DATA_RAW | EpdEdgeInfo::EDGE_DATA_WAW_WAR | EpdEdgeInfo::EDGE_CONTROL | EpdEdgeInfo::EDGE_OUTPUT | EpdEdgeInfo::EDGE_CONTROL_FLOW)),
      fepdg(new EpdGraph(epdg_bulk, EpdEdgeInfo::EDGE_DATA_RAW | EpdEdgeInfo::EDGE_DATA_WAW_WAR | EpdEdgeInfo::EDGE_CONTROL | EpdEdgeInfo::EDGE_FEEDBACK | EpdEdgeInfo::EDGE_OUTPUT | EpdEdgeInfo::EDGE_CONTROL_FLOW)),
      afg(new EpdGraph(epdg_bulk, EpdEdgeInfo::EDGE_AF_STANDARD)),
      fafg(new EpdGraph(epdg_bulk, EpdEdgeInfo::EDGE_AF_STANDARD | EpdEdgeInfo::EDGE_AF_FEEDBACK)),
      prg_bulk(new ParallelRegionsGraphsCollection(_parameters)),
      prg(new ParallelRegionsGraph(prg_bulk, ParallelRegionsEdgeInfo::EDGE_CONTROL | ParallelRegionsEdgeInfo::EDGE_DATA)),
#endif
      agg_virtualg(new OpGraph(op_graphs_collection, DFG_AGG_SELECTOR | ADG_AGG_SELECTOR)),
#if HAVE_HOST_PROFILING_BUILT
      profiling_information(ProfilingInformationRef(new ProfilingInformation(bb))),
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
      pointer_type_conversion(false),
      unaligned_accesses(false),
      bb_version(1),
      bitvalue_version(1),
      has_globals(false),
      has_undefined_function_receiveing_pointers(false),
      state_variables(),
      pipelining_enabled(_parameters->isOption(OPT_pipelining) && _parameters->getOption<bool>(OPT_pipelining)),
      bb_reachability(),
      feedback_bb_reachability(),
      ogc(new operations_graph_constructor(op_graphs_collection)),
      bbgc(new BasicBlocksGraphConstructor(bb_graphs_collection)),
#if HAVE_EXPERIMENTAL
      epdgc(new extended_pdg_constructor(epdg_bulk, saodg)),
      prgc(new ParallelRegionsGraphConstructor(prg_bulk, _parameters)),
#endif
      lm(new level_constructor(map_levels, deque_levels)),
      bb_lm(new level_constructor(bb_map_levels, bb_deque_levels)),
      dominators(nullptr),
      post_dominators(nullptr),
      memory_info(),
      packed_vars(false)
{
}

FunctionBehavior::~FunctionBehavior()
{
   if(dominators)
      delete dominators;
   if(post_dominators)
      delete post_dominators;
}

OpGraphRef FunctionBehavior::GetOpGraph(FunctionBehavior::graph_type gt)
{
   switch(gt)
   {
      case CFG:
         return cfg;
      case ECFG:
         return extended_cfg;
      case FCFG:
         return fcfg;
      case CDG:
         return cdg;
      case FCDG:
         return fcdg;
      case DFG:
         return dfg;
      case FDFG:
         return fdfg;
      case ADG:
         return adg;
      case FADG:
         return fadg;
      case ODG:
         return odg;
      case FODG:
         return fodg;
      case SDG:
         return sdg;
      case FSDG:
         return fsdg;
      case SAODG:
         return saodg;
      case FSAODG:
         return fsaodg;
      case FLSAODG:
         return flsaodg;
#ifndef NDEBUG
      case FLSAODDG:
         return flsaoddg;
#endif
      case FFLSAODG:
         return fflsaodg;
      case FLAODDG:
         return flaoddg;
      case FFLAODDG:
         return fflaoddg;
      case FLG:
         return flg;
      case SG:
         return sg;
#if HAVE_EXPERIMENTAL
      case RPDG:
         return rpdg;
#endif
      case AGG_VIRTUALG:
         return agg_virtualg;
      default:
         THROW_UNREACHABLE("Not supported graph type");
   }
   return OpGraphRef();
}

const OpGraphConstRef FunctionBehavior::CGetOpGraph(FunctionBehavior::graph_type gt) const
{
   switch(gt)
   {
      case CFG:
         return cfg;
      case ECFG:
         return extended_cfg;
      case FCFG:
         return fcfg;
      case CDG:
         return cdg;
      case FCDG:
         return fcdg;
      case DFG:
         return dfg;
      case FDFG:
         return fdfg;
      case ADG:
         return adg;
      case FADG:
         return fadg;
      case ODG:
         return odg;
      case FODG:
         return fodg;
      case SDG:
         return sdg;
      case FSDG:
         return fsdg;
      case SAODG:
         return saodg;
      case FSAODG:
         return fsaodg;
      case FLSAODG:
         return flsaodg;
#ifndef NDEBUG
      case FLSAODDG:
         return flsaoddg;
#endif
      case FFLSAODG:
         return fflsaodg;
      case FLAODDG:
         return flaoddg;
      case FFLAODDG:
         return fflaoddg;
      case FLG:
         return flg;
      case SG:
         return sg;
#if HAVE_EXPERIMENTAL
      case RPDG:
         return rpdg;
#endif
      case AGG_VIRTUALG:
         return agg_virtualg;
      default:
         THROW_UNREACHABLE("Not supported graph type");
   }
   return OpGraphConstRef();
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
const OpGraphConstRef FunctionBehavior::CGetOpGraph(FunctionBehavior::graph_type gt, const OpVertexSet& statements) const
{
   /// This "transformation" is necessary because of graph constructor
   CustomUnorderedSet<vertex> subset;
   subset.insert(statements.begin(), statements.end());
   switch(gt)
   {
      case CFG:
         return OpGraphRef(new OpGraph(op_graphs_collection, CFG_SELECTOR, subset));

      case FCFG:
         return OpGraphRef(new OpGraph(op_graphs_collection, FCFG_SELECTOR, subset));

      case ECFG:
         return OpGraphRef(new OpGraph(op_graphs_collection, CFG_SELECTOR | ECFG_SELECTOR, subset));

      case CDG:
         return OpGraphRef(new OpGraph(op_graphs_collection, CDG_SELECTOR, subset));

      case FCDG:
         return OpGraphRef(new OpGraph(op_graphs_collection, FCDG_SELECTOR, subset));

      case DFG:
         return OpGraphRef(new OpGraph(op_graphs_collection, DFG_SELECTOR, subset));

      case FDFG:
         return OpGraphRef(new OpGraph(op_graphs_collection, FDFG_SELECTOR, subset));

      case ADG:
         return OpGraphRef(new OpGraph(op_graphs_collection, ADG_SELECTOR, subset));

      case FADG:
         return OpGraphRef(new OpGraph(op_graphs_collection, ADG_SELECTOR | FB_ADG_SELECTOR, subset));

      case ODG:
         return OpGraphRef(new OpGraph(op_graphs_collection, ODG_SELECTOR, subset));

      case FODG:
         return OpGraphRef(new OpGraph(op_graphs_collection, FODG_SELECTOR, subset));

      case SDG:
         return OpGraphRef(new OpGraph(op_graphs_collection, SDG_SELECTOR, subset));

      case FSDG:
         return OpGraphRef(new OpGraph(op_graphs_collection, FSDG_SELECTOR, subset));

      case SAODG:
         return OpGraphRef(new OpGraph(op_graphs_collection, SAODG_SELECTOR, subset));

      case FSAODG:
         return OpGraphRef(new OpGraph(op_graphs_collection, FSDG_SELECTOR | FADG_SELECTOR | FODG_SELECTOR | FDFG_SELECTOR, subset));

      case FLSAODG:
         return OpGraphRef(new OpGraph(op_graphs_collection, SAODG_SELECTOR | FLG_SELECTOR, subset));

#ifndef NDEBUG
      case FLSAODDG:
         return OpGraphRef(new OpGraph(op_graphs_collection, SAODG_SELECTOR | FLG_SELECTOR | DEBUG_SELECTOR, subset));
#endif

      case FFLSAODG:
         return OpGraphRef(new OpGraph(op_graphs_collection, FLG_SELECTOR | FSDG_SELECTOR | FADG_SELECTOR | FODG_SELECTOR, subset));

      case FLAODDG:
         return OpGraphRef(new OpGraph(op_graphs_collection, DFG_SELECTOR | ADG_SELECTOR | ODG_SELECTOR | FLG_SELECTOR, subset));

      case FFLAODDG:
         return OpGraphRef(new OpGraph(op_graphs_collection, FLG_SELECTOR | FADG_SELECTOR | FODG_SELECTOR | FDFG_SELECTOR));

      case FLG:
         return OpGraphRef(new OpGraph(op_graphs_collection, FLG_SELECTOR, subset));

      case SG:
         return OpGraphRef(new OpGraph(op_graphs_collection, SG_SELECTOR, subset));

#if HAVE_EXPERIMENTAL
      case RPDG:
         return OpGraphRef(new OpGraph(op_graphs_collection, SAODG_SELECTOR & RPDG_SELECTOR, subset));
#endif
      case AGG_VIRTUALG:
         return OpGraphRef(new OpGraph(op_graphs_collection, DFG_AGG_SELECTOR | ADG_AGG_SELECTOR, subset));
      default:
         THROW_UNREACHABLE("");
   }
   return OpGraphConstRef();
}

#if HAVE_EXPERIMENTAL

const EpdGraphRef FunctionBehavior::CGetEpdGraph(EpdGraph::Type type) const
{
   switch(type)
   {
      case EpdGraph::EPDG:
         return epdg;
      case EpdGraph::FEPDG:
         return fepdg;
      case EpdGraph::CEPDG:
         return cepdg;
      case EpdGraph::DEPDG:
         return depdg;
      case EpdGraph::CDEPDG:
         return cdepdg;
      case EpdGraph::CDCFEPDG:
         return cdcfepdg;
      case EpdGraph::AFG:
         return afg;
      case EpdGraph::FAFG:
         return fafg;
      default:
         THROW_UNREACHABLE("");
   }
   return EpdGraphRef();
}

const ParallelRegionsGraphRef FunctionBehavior::GetPrgGraph(ParallelRegionsGraph::Type gt) const
{
   switch(gt)
   {
      case ParallelRegionsGraph::PRG:
         return this->prg;
      default:
         THROW_UNREACHABLE("");
   }
   return ParallelRegionsGraphRef();
}
#endif

BBGraphRef FunctionBehavior::GetBBGraph(FunctionBehavior::bb_graph_type gt)
{
   switch(gt)
   {
      case BB:
         return bb;
      case FBB:
         return fbb;
      case EBB:
         return extended_bb;
      case CDG_BB:
         return cdg_bb;
      case DOM_TREE:
         return dt;
      case POST_DOM_TREE:
         return pdt;
      case PPG:
         return ppg;
      case DJ:
         return dj;
      default:
         THROW_UNREACHABLE("");
   }
   return BBGraphRef();
}

const BBGraphConstRef FunctionBehavior::CGetBBGraph(FunctionBehavior::bb_graph_type gt) const
{
   switch(gt)
   {
      case BB:
         return bb;
      case FBB:
         return fbb;
      case EBB:
         return extended_bb;
      case CDG_BB:
         return cdg_bb;
      case DOM_TREE:
         return dt;
      case POST_DOM_TREE:
         return pdt;
      case PPG:
         return ppg;
      case DJ:
         return dj;
      default:
         THROW_UNREACHABLE("");
   }
   return BBGraphRef();
}

void FunctionBehavior::print(std::ostream& os) const
{
   os << "Function " << helper->get_function_name() << std::endl;
   // os << "Bulk operation graph of " << og;
   // os << ", , ";
}

const std::deque<vertex>& FunctionBehavior::get_levels() const
{
   return deque_levels;
}

const std::deque<vertex>& FunctionBehavior::get_bb_levels() const
{
   return bb_deque_levels;
}

const std::map<vertex, unsigned int>& FunctionBehavior::get_map_levels() const
{
   return map_levels;
}

const std::map<vertex, unsigned int>& FunctionBehavior::get_bb_map_levels() const
{
   return bb_map_levels;
}

void FunctionBehavior::set_epp(EdgeDescriptor e, unsigned long long value)
{
   ppg->GetBBEdgeInfo(e)->set_epp_value(value);
}

const LoopsConstRef FunctionBehavior::CGetLoops() const
{
   return loops;
}

const LoopsRef FunctionBehavior::GetLoops() const
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

void FunctionBehavior::erase_dynamic_address(unsigned int node_id)
{
   dynamic_address.erase(node_id);
}

void FunctionBehavior::erase_all_dynamic_addresses()
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

const CustomOrderedSet<unsigned int>& FunctionBehavior::get_dynamic_address() const
{
   return dynamic_address;
}

const CustomOrderedSet<unsigned int>& FunctionBehavior::get_parm_decl_copied() const
{
   return parm_decl_copied;
}

const CustomOrderedSet<unsigned int>& FunctionBehavior::get_parm_decl_loaded() const
{
   return parm_decl_loaded;
}

const CustomOrderedSet<unsigned int>& FunctionBehavior::get_parm_decl_stored() const
{
   return parm_decl_stored;
}

CustomOrderedSet<unsigned int> FunctionBehavior::get_local_variables(const application_managerConstRef AppM) const
{
   CustomOrderedSet<unsigned int> vars;
   // I simply have to go over all the vertices and get the used variables;
   // the variables which have to be declared are all those variables but
   // the globals ones
   VertexIterator v, vEnd;
   for(boost::tie(v, vEnd) = boost::vertices(*cfg); v != vEnd; v++)
   {
      CustomSet<unsigned int> varsTemp = cfg->CGetOpNodeInfo(*v)->cited_variables;
      vars.insert(varsTemp.begin(), varsTemp.end());
   }
   const std::list<unsigned int>& funParams = helper->get_parameters();
   for(unsigned int funParam : funParams)
   {
      if(vars.find(funParam) != vars.end())
         vars.erase(funParam);
   }

   const CustomSet<unsigned int>& gblVariables = AppM->get_global_variables();
   for(unsigned int gblVariable : gblVariables)
   {
      if(vars.find(gblVariable) != vars.end())
         vars.erase(gblVariable);
   }
   return vars;
}

bool op_vertex_order_by_map::operator()(const vertex x, const vertex y) const
{
   THROW_ASSERT(ref.find(x) != ref.end(), "Vertex " + GET_NAME(g, x) + " is not in topological_sort");
   THROW_ASSERT(ref.find(y) != ref.end(), "Second " + GET_NAME(g, y) + " vertex is not in topological_sort");
   return ref.find(x)->second < ref.find(y)->second;
}

bool FunctionBehavior::CheckBBReachability(const vertex first_basic_block, const vertex second_basic_block) const
{
   if(bb_reachability.find(first_basic_block) != bb_reachability.end() and bb_reachability.find(first_basic_block)->second.find(second_basic_block) != bb_reachability.find(first_basic_block)->second.end())
   {
      return true;
   }
   else
   {
      return false;
   }
}

bool FunctionBehavior::CheckBBFeedbackReachability(const vertex first_basic_block, const vertex second_basic_block) const
{
   if(CheckBBReachability(first_basic_block, second_basic_block))
   {
      return true;
   }
   if(feedback_bb_reachability.find(first_basic_block) != feedback_bb_reachability.end() and feedback_bb_reachability.find(first_basic_block)->second.find(second_basic_block) != feedback_bb_reachability.find(first_basic_block)->second.end())
   {
      return true;
   }
   else
   {
      return false;
   }
}

bool FunctionBehavior::CheckReachability(const vertex first_operation, const vertex second_operation) const
{
   const CustomUnorderedMap<unsigned int, vertex>& bb_index_map = bb->CGetBBGraphInfo()->bb_index_map;
   const unsigned int first_bb_index = cfg->CGetOpNodeInfo(first_operation)->bb_index;
   const unsigned int second_bb_index = cfg->CGetOpNodeInfo(second_operation)->bb_index;
   const vertex first_bb_vertex = bb_index_map.find(first_bb_index)->second;
   const vertex second_bb_vertex = bb_index_map.find(second_bb_index)->second;
   if(CheckBBReachability(first_bb_vertex, second_bb_vertex))
   {
      return true;
   }
   if(first_bb_vertex == second_bb_vertex)
   {
      THROW_ASSERT(map_levels.size(), "");
      THROW_ASSERT(map_levels.find(first_operation) != map_levels.end(), "Level of " + GET_NAME(cfg, first_operation) + " not found");
      THROW_ASSERT(map_levels.find(second_operation) != map_levels.end(), "Level of " + GET_NAME(cfg, second_operation) + " not found");
      if(map_levels.find(first_operation)->second < map_levels.find(second_operation)->second)
      {
         return true;
      }
   }
   return false;
}

bool FunctionBehavior::CheckFeedbackReachability(const vertex first_operation, const vertex second_operation) const
{
   const CustomUnorderedMap<unsigned int, vertex>& bb_index_map = bb->CGetBBGraphInfo()->bb_index_map;
   const unsigned int first_bb_index = cfg->CGetOpNodeInfo(first_operation)->bb_index;
   const unsigned int second_bb_index = cfg->CGetOpNodeInfo(second_operation)->bb_index;
   const vertex first_bb_vertex = bb_index_map.find(first_bb_index)->second;
   const vertex second_bb_vertex = bb_index_map.find(second_bb_index)->second;
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

#if __GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 6)
#pragma GCC diagnostic pop
#endif
