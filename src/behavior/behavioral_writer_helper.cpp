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
 * @file behavioral_writer_helper.cpp
 * @brief Implement all structs used to write a graph in the dot format
 *
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 * @author Marco Lattuada <lattuada@elet.polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *
 */
#include "behavioral_writer_helper.hpp"

#include "config_HAVE_HOST_PROFILING_BUILT.hpp"

#include "custom_map.hpp"                 // for _Rb_tree_const_iterator
#include <boost/lexical_cast.hpp>         // for lexical_cast
#include <boost/smart_ptr/shared_ptr.hpp> // for shared_ptr
#include <list>                           // for list
#include <string>                         // for string, operator<<
#include <utility>                        // for pair
#if HAVE_HLS_BUILT
#include "allocation_information.hpp" // for AllocationInformation
#endif
#include "application_manager.hpp" // for application_manager
#include "basic_block.hpp"         // for BBNodeInfoConstRef
#include "behavioral_helper.hpp"   // for BehavioralHelper
#include "cdfg_edge_info.hpp"      // for CDG_SELECTOR, CFG_SE...
#include "exceptions.hpp"          // for THROW_UNREACHABLE
#include "function_behavior.hpp"   // for tree_nodeRef, Functi...
#if HAVE_HLS_BUILT
#include "hls.hpp"         // for hls, AllocationInfor...
#include "hls_manager.hpp" // for HLS_manager, hlsRef
#endif
#include "op_graph.hpp" // for OpEdgeInfo, OpGraph
#if HAVE_HOST_PROFILING_BUILT
#include "profiling_information.hpp" // for ProfilingInformation
#endif
#if HAVE_HLS_BUILT
#include "schedule.hpp" // for Schedule, AbsControl...
#endif
#include "string_manipulation.hpp" // for STR
#include "tree_basic_block.hpp"    // for bloc, tree_nodeRef
#include "tree_common.hpp"         // for aggr_init_expr_K
#include "tree_node.hpp"           // for tree_node, CASE_BINA...
#include "tree_reindex.hpp"
#include "typed_node_info.hpp" // for GET_TYPE, GET_NAME
#include "var_pp_functor.hpp"  // for std_var_pp_functor

BBWriter::BBWriter(const BBGraph* _g, CustomUnorderedSet<vertex> _annotated)
    : VertexWriter(_g, 0),
      function_behavior(_g->CGetBBGraphInfo()->AppM->CGetFunctionBehavior(_g->CGetBBGraphInfo()->function_index)),
      helper(function_behavior->CGetBehavioralHelper()),
      annotated(std::move(_annotated))
#if HAVE_HLS_BUILT
      ,
      schedule(GetPointer<const HLS_manager>(_g->CGetBBGraphInfo()->AppM) and GetPointer<const HLS_manager>(_g->CGetBBGraphInfo()->AppM)->get_HLS(helper->get_function_index()) ?
                   GetPointer<const HLS_manager>(_g->CGetBBGraphInfo()->AppM)->get_HLS(helper->get_function_index())->Rsch :
                   ScheduleConstRef())
#endif
{
}

void BBWriter::operator()(std::ostream& out, const vertex& v) const
{
   const BBGraphInfoConstRef info = dynamic_cast<const BBGraph*>(printing_graph)->CGetBBGraphInfo();
   const BBNodeInfoConstRef bb_node_info = dynamic_cast<const BBGraph*>(printing_graph)->CGetBBNodeInfo(v);
   if(v == info->entry_vertex)
   {
      out << "[color=blue,shape=Msquare, ";
      if(annotated.find(v) != annotated.end())
         out << " style=filled, fillcolor=black, fontcolor=white,";
      out << "label=\"ENTRY";
   }
   else if(v == info->exit_vertex)
   {
      out << "[color=blue,shape=Msquare, ";
      if(annotated.find(v) != annotated.end())
      {
         out << " style=filled, fillcolor=black, fontcolor=white,";
      }
      out << "label=\"EXIT";
   }
   else
   {
      out << "[shape=box";
      if(annotated.find(v) != annotated.end())
         out << ", style=filled, fillcolor=black, fontcolor=white";
      if(bb_node_info and bb_node_info->block)
      {
         out << ", label=\"BB" << bb_node_info->block->number << " - GCCLI: " << bb_node_info->block->loop_id << " - HPL: " << bb_node_info->block->hpl << " - Cer: " << bb_node_info->cer;
         out << " - Loop " << bb_node_info->loop_id;
#if HAVE_HOST_PROFILING_BUILT
         out << " - Executions: " << function_behavior->CGetProfilingInformation()->GetBBExecutions(v);
#endif
      }
      if(bb_node_info and bb_node_info->block->CGetPhiList().size() and helper)
      {
         out << "\\l";
         for(const auto& phi : bb_node_info->block->CGetPhiList())
         {
            const var_pp_functorConstRef svpf(new std_var_pp_functor(helper));
            std::string res = STR(phi->index);
#if HAVE_HLS_BUILT
            if(schedule)
               res += " " + schedule->PrintTimingInformation(phi->index) + " ";
#endif
            res += " -> " + helper->print_node(phi->index, nullptr, svpf);
            std::string temp;
            for(char re : res)
            {
               if(re == '\"')
                  temp += "\\\"";
               else if(re != '\n')
                  temp += re;
            }
            out << temp << "\\l";
         }
      }
      if(bb_node_info and bb_node_info->block->CGetStmtList().size() and helper)
      {
         if(bb_node_info->block->CGetPhiList().empty())
            out << "\\n";
         for(const auto& statement : bb_node_info->block->CGetStmtList())
         {
            const var_pp_functorConstRef svpf(new std_var_pp_functor(helper));
            std::string res = STR(GET_INDEX_NODE(statement));
#if HAVE_HLS_BUILT
            if(schedule)
               res += " " + schedule->PrintTimingInformation(statement->index) + " ";
#endif
            res += " -> " + helper->print_node(statement->index, nullptr, svpf);
            const tree_nodeRef node = GET_NODE(statement);
            switch(node->get_kind())
            {
               case gimple_assign_K:
               case gimple_call_K:
               case gimple_asm_K:
               case gimple_goto_K:
               case gimple_predict_K:
               case gimple_resx_K:
                  res += ";";
                  break;
               case binfo_K:
               case block_K:
               case constructor_K:
               case call_expr_K:
               case aggr_init_expr_K:
               case case_label_expr_K:
               case gimple_bind_K:
               case gimple_cond_K:
               case gimple_for_K:
               case gimple_label_K:
               case gimple_multi_way_if_K:
               case gimple_nop_K:
               case gimple_phi_K:
               case gimple_pragma_K:
               case gimple_return_K:
               case gimple_switch_K:
               case gimple_while_K:
               case identifier_node_K:
               case ssa_name_K:
               case statement_list_K:
               case target_expr_K:
               case target_mem_ref_K:
               case target_mem_ref461_K:
               case tree_list_K:
               case tree_vec_K:
               case error_mark_K:
               case lut_expr_K:
               case CASE_BINARY_EXPRESSION:
               case CASE_CPP_NODES:
               case CASE_CST_NODES:
               case CASE_DECL_NODES:
               case CASE_FAKE_NODES:
               case CASE_PRAGMA_NODES:
               case CASE_QUATERNARY_EXPRESSION:
               case CASE_TERNARY_EXPRESSION:
               case CASE_TYPE_NODES:
               case CASE_UNARY_EXPRESSION:
               default:
                  break;
            }
            std::string temp;
            for(char re : res)
            {
               if(re == '\"')
                  temp += "\\\"";
               else if(re != '\n')
                  temp += re;
            }
            out << temp << "\\l";
         }
      }
   }
   out << "\"]";
}

OpEdgeWriter::OpEdgeWriter(const OpGraph* operation_graph) : EdgeWriter(operation_graph, 0), BH(operation_graph->CGetOpGraphInfo()->BH)
{
}

void OpEdgeWriter::operator()(std::ostream& out, const EdgeDescriptor& e) const
{
   if((FB_CFG_SELECTOR)&selector & printing_graph->GetSelector(e))
      out << "[color=gold";
   else if((CFG_SELECTOR)&selector & printing_graph->GetSelector(e))
      out << "[fontcolor=red3";
   else if((FB_CDG_SELECTOR)&selector & printing_graph->GetSelector(e) && FB_DFG_SELECTOR & selector & printing_graph->GetSelector(e))
      out << "[color=gold,style=dotted";
   else if((FB_CDG_SELECTOR)&selector & printing_graph->GetSelector(e))
      out << "[color=gold";
   else if(((CDG_SELECTOR)&selector & printing_graph->GetSelector(e)) && ((DFG_SELECTOR)&selector & printing_graph->GetSelector(e)))
      out << "[color=red3,style=dotted";
   else if((CDG_SELECTOR)&selector & printing_graph->GetSelector(e))
      out << "[color=red3";
   else if((FB_DFG_SELECTOR)&selector & printing_graph->GetSelector(e))
      out << "[color=lightblue";
   else if((DFG_SCA_SELECTOR)&selector & printing_graph->GetSelector(e))
      out << "[color=blue, style=dotted";
   else if((DFG_AGG_SELECTOR)&selector & printing_graph->GetSelector(e))
      out << "[color=blue";
   else if((FB_ADG_SELECTOR)&selector & printing_graph->GetSelector(e))
      out << "[color=lawngreen";
   else if((ADG_SELECTOR)&selector & printing_graph->GetSelector(e))
      out << "[color=green4";
   else if((FB_ODG_SELECTOR)&selector & printing_graph->GetSelector(e))
      out << "[color=lawngreen";
   else if((ODG_SELECTOR)&selector & printing_graph->GetSelector(e))
      out << "[color=green4";
   else if((CDG_SELECTOR)&selector & printing_graph->GetSelector(e))
      out << "[color=red3";
   else if((DFG_SELECTOR)&selector & printing_graph->GetSelector(e))
      out << "[color=blue";
   else if((FB_CDG_SELECTOR)&selector & printing_graph->GetSelector(e))
      out << "[color=gold";
   else if((FB_DFG_SELECTOR)&selector & printing_graph->GetSelector(e))
      out << "[color=lightblue";
   else if((FLG_SELECTOR)&selector & printing_graph->GetSelector(e))
      out << "[color=red3";
   else if((DEBUG_SELECTOR)&selector & printing_graph->GetSelector(e))
      out << "[color=red3";
   else if((CSG_SELECTOR)&selector & printing_graph->GetSelector(e) && DFG_SELECTOR & selector & printing_graph->GetSelector(e))
      out << "[color=pink,style=dotted";
   else if((CSG_SELECTOR)&selector & printing_graph->GetSelector(e))
      out << "[color=pink";
   else if((FB_FLG_SELECTOR)&selector & printing_graph->GetSelector(e))
      out << "[color=pink";

   const auto* edge_info = Cget_edge_info<OpEdgeInfo>(e, *printing_graph);

   if(edge_info)
   {
      out << ",label=\"";
      if(printing_graph->GetSelector(e) & selector & FCDG_SELECTOR)
      {
         if(edge_info->CdgEdgeT() || edge_info->CdgEdgeF() || edge_info->Switch())
         {
            out << edge_info->PrintLabels(CDG_SELECTOR, BH);
         }
      }
      if(printing_graph->GetSelector(e) & selector & FCFG_SELECTOR)
      {
         if(edge_info->CfgEdgeT() || edge_info->CfgEdgeF() || edge_info->Switch())
         {
            out << edge_info->PrintLabels(CFG_SELECTOR, BH);
         }
      }
      if(printing_graph->GetSelector(e) & selector & FLG_SELECTOR)
      {
         if(edge_info->FlgEdgeT() || edge_info->FlgEdgeF())
         {
            out << edge_info->PrintLabels(FLG_SELECTOR, BH);
         }
      }
      if(printing_graph->GetSelector(e) & selector & FDFG_SELECTOR)
      {
         out << edge_info->PrintLabels(DFG_SELECTOR, BH);
      }
      if(printing_graph->GetSelector(e) & selector & FADG_SELECTOR)
      {
         out << edge_info->PrintLabels(ADG_SELECTOR, BH);
      }
      if(printing_graph->GetSelector(e) & selector & FODG_SELECTOR)
      {
         out << edge_info->PrintLabels(ODG_SELECTOR, BH);
      }
      out << "\"";
   }
   out << "]";
}

BBEdgeWriter::BBEdgeWriter(const BBGraph* _g) : EdgeWriter(_g, 0), BH(_g->CGetBBGraphInfo()->AppM->CGetFunctionBehavior(_g->CGetBBGraphInfo()->function_index)->CGetBehavioralHelper())
{
}

void BBEdgeWriter::operator()(std::ostream& out, const EdgeDescriptor& e) const
{
   if(FB_CFG_SELECTOR & printing_graph->GetSelector(e))
      out << "[fontcolor=blue, color=gold";
   else if(CFG_SELECTOR & printing_graph->GetSelector(e))
      out << "[fontcolor=blue, color=red3";
   else if(CDG_SELECTOR & printing_graph->GetSelector(e))
      out << "[fontcolor=blue, color=red3";
   else if(D_SELECTOR & printing_graph->GetSelector(e))
      out << "[fontcolor=blue";
   else if(PD_SELECTOR & printing_graph->GetSelector(e))
      out << "[fontcolor=blue";
   else if(ECFG_SELECTOR & printing_graph->GetSelector(e))
      out << "[fontcolor=blue, color=gold";
   else if(J_SELECTOR & printing_graph->GetSelector(e))
      out << "[fontcolor=blue, color=gold";
   else if(PP_SELECTOR & printing_graph->GetSelector(e))
      out << "[fontcolor=blue, color=gold";
   else
      THROW_UNREACHABLE("Not supported graph type in printing: " + STR(printing_graph->GetSelector(e)) + " " + STR(PP_SELECTOR));
   const BBEdgeInfoConstRef bb_edge_info = dynamic_cast<const BBGraph*>(printing_graph)->CGetBBEdgeInfo(e);
   if(selector & PP_SELECTOR)
   {
      out << ",label=\"";
      out << boost::lexical_cast<std::string>(bb_edge_info->get_epp_value());
      out << "\"";
   }
   else if(selector & FCFG_SELECTOR)
   {
      if(bb_edge_info->CfgEdgeT() || bb_edge_info->CfgEdgeF() || bb_edge_info->Switch())
      {
         out << ",label=\"";
         out << bb_edge_info->PrintLabels(CFG_SELECTOR, BH);
         out << "\"";
      }
   }
   else if(selector & FCDG_SELECTOR)
   {
      if(bb_edge_info->CdgEdgeT() || bb_edge_info->CdgEdgeF() || bb_edge_info->Switch())
      {
         out << ",label=\"";
         out << bb_edge_info->PrintLabels(CDG_SELECTOR, BH);
         out << "\"";
      }
   }
   out << "]";
}

OpWriter::OpWriter(const OpGraph* operation_graph, const int _detail_level) : VertexWriter(operation_graph, _detail_level), helper(operation_graph->CGetOpGraphInfo()->BH)
{
}

void OpWriter::operator()(std::ostream& out, const vertex& v) const
{
   const auto* op_graph = dynamic_cast<const OpGraph*>(printing_graph);
   if(GET_TYPE(printing_graph, v) & (TYPE_IF | TYPE_SWITCH))
      out << "[color=red,shape=diamond,";
   else if(GET_TYPE(printing_graph, v) & TYPE_LOAD)
      out << "[color=green,shape=box,";
   else if(GET_TYPE(printing_graph, v) & TYPE_STORE)
      out << "[color=red,shape=box,";
   else if(GET_TYPE(printing_graph, v) & TYPE_EXTERNAL)
      out << "[color=green,shape=box,";
   else if(GET_TYPE(printing_graph, v) & TYPE_MEMCPY)
      out << "[color=burlywood,shape=diamond,";
   else if(GET_TYPE(printing_graph, v) & TYPE_GOTO)
      out << "[color=yellow,shape=box,";
   else if(GET_TYPE(printing_graph, v) & TYPE_ASSIGN)
      out << "[color=burlywood,shape=box,";
   else if(GET_TYPE(printing_graph, v) & (TYPE_ENTRY | TYPE_EXIT))
      out << "[color=blue,shape=Msquare,";
   else
      out << "[";
   out << "label=\"" << GET_NAME(printing_graph, v);
#if HAVE_HLS_BUILT
   out << " - " << op_graph->CGetOpNodeInfo(v)->GetOperation();
#endif
   const var_pp_functorConstRef svpf(new std_var_pp_functor(helper));
   if(op_graph->CGetOpNodeInfo(v)->node)
   {
      out << "\\n";
      null_deleter null_del;
      out << helper->print_vertex(OpGraphConstRef(dynamic_cast<const OpGraph*>(printing_graph), null_del), v, svpf, true);
      if(detail_level >= 1)
      {
         out << "\\n";
         op_graph->CGetOpNodeInfo(v)->Print(out, helper, true);
      }
   }
   out << "\"]";
}

#if HAVE_HLS_BUILT
TimedOpWriter::TimedOpWriter(const OpGraph* op_graph, const hlsConstRef _HLS, const CustomSet<unsigned int> _critical_paths) : OpWriter(op_graph, 0), HLS(_HLS), critical_paths(_critical_paths)
{
}

void TimedOpWriter::operator()(std::ostream& out, const vertex& v) const
{
   const auto schedule = HLS->Rsch;
   const auto* op_graph = dynamic_cast<const OpGraph*>(printing_graph);
   const unsigned node_id = op_graph->CGetOpNodeInfo(v)->GetNodeId();
   if(critical_paths.find(node_id) != critical_paths.end())
   {
      out << "[color=red,";
   }
   else
   {
      out << "[";
   }
   out << "label=\"";
   out << "[" << schedule->GetStartingTime(node_id) << "---" << schedule->GetEndingTime(node_id) << "]";
   out << " - " << GET_NAME(printing_graph, v);
   out << " - " << HLS->allocation_information->get_fu_name(HLS->allocation_information->GetFuType(node_id)).first;
   const var_pp_functorConstRef svpf(new std_var_pp_functor(helper));
   if(op_graph->CGetOpNodeInfo(v)->node)
   {
      out << "\\n";
      null_deleter null_del;
      out << helper->print_vertex(OpGraphConstRef(dynamic_cast<const OpGraph*>(printing_graph), null_del), v, svpf, true);
      if(detail_level >= 1)
      {
         out << "\\n";
         op_graph->CGetOpNodeInfo(v)->Print(out, helper, true);
      }
   }
   out << "\"]";
}

TimedOpEdgeWriter::TimedOpEdgeWriter(const OpGraph* _operation_graph, const hlsConstRef _HLS, CustomSet<unsigned int> _critical_paths) : OpEdgeWriter(_operation_graph), HLS(_HLS), critical_paths(std::move(_critical_paths))
{
}

void TimedOpEdgeWriter::operator()(std::ostream& out, const EdgeDescriptor& e) const
{
   const auto source = boost::source(e, *printing_graph);
   const auto target = boost::target(e, *printing_graph);
   const auto* op_graph = dynamic_cast<const OpGraph*>(printing_graph);
   const auto source_id = op_graph->CGetOpNodeInfo(source)->GetNodeId();
   const auto target_id = op_graph->CGetOpNodeInfo(target)->GetNodeId();
   out << "[";
   if(critical_paths.find(source_id) != critical_paths.end() and critical_paths.find(target_id) != critical_paths.end())
   {
      out << "color=red,";
   }
   const ControlStep u_control_step(AbsControlStep::UNKNOWN);
   out << "label=" << HLS->allocation_information->GetConnectionTime(source_id, target_id, AbsControlStep(op_graph->CGetOpNodeInfo(target)->bb_index, u_control_step));
   out << "]";
}
#endif
