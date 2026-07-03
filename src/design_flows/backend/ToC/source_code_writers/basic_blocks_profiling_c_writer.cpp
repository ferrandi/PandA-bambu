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
 *              Copyright (C) 2015-2026 Politecnico di Milano
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
 * @file basic_blocks_profiling_c_writer.cpp
 * @brief This file contains the routines necessary to create a C executable function with instrumented edges for
 * profiling of executions of single basic blocks and cycle count
 *
 * @author Marco Lattuada <marco.lattuada@polimi.it>
 * @author Michele Fiorito <michele.fiorito@polimi.it>
 *
 */
#include "basic_blocks_profiling_c_writer.hpp"

#include "FSMInfo.hpp"
#include "Parameter.hpp"
#include "allocation_information.hpp"
#include "basic_block.hpp"
#include "behavioral_helper.hpp"
#include "call_graph_manager.hpp"
#include "constant_strings.hpp"
#include "dbgPrintHelper.hpp"
#include "fu_binding.hpp"
#include "function_behavior.hpp"
#include "hls.hpp"
#include "hls_manager.hpp"
#include "host_profiling_constants.hpp"
#include "indented_output_stream.hpp"
#include "instruction_writer.hpp"
#include "ir_basic_block.hpp"
#include "ir_helper.hpp"
#include "ir_manager.hpp"
#include "ir_node.hpp"
#include "schedule.hpp"
#include "string_manipulation.hpp"
#include "technology_node.hpp"
#include "var_pp_functor.hpp"

namespace
{
   std::tuple<unsigned, unsigned> compute_bb_csteps(const BBNodeInfo& bb_info, const ScheduleConstRef& sch)
   {
      unsigned max_cstep = 0u;
      unsigned min_cstep = std::numeric_limits<unsigned int>::max();
      for(auto op : bb_info.statements_list)
      {
         const auto cstep = sch->get_cstep(op);
         const auto cstep_end = sch->get_cstep_end(op);
         max_cstep = std::max(max_cstep, cstep_end.second);
         min_cstep = std::min(min_cstep, cstep.second);
      }
      return {min_cstep, max_cstep};
   }

   class WrapFunctionInstructionWriter : public InstructionWriter
   {
      const bool& enable;

    public:
      WrapFunctionInstructionWriter(const InstructionWriter& base, const bool& _enable)
          : InstructionWriter(base), enable(_enable)
      {
      }

      void write(const FunctionBehaviorConstRef function_behavior, gc_vertex_descriptor statement,
                 const std::unique_ptr<var_pp_functor>& varFunctor) override
      {
         if(!enable)
         {
            return InstructionWriter::write(function_behavior, statement, varFunctor);
         }

         const auto op_cfg = function_behavior->GetOpGraph(FunctionBehavior::CFG);
         const auto& op_info = op_cfg.CGetNodeInfo(statement);
         if(op_info.called.empty())
         {
            return InstructionWriter::write(function_behavior, statement, varFunctor);
         }
         THROW_ASSERT(op_info.called.size() == 1, "Multiple function calls in single statement not supported.");

         const auto caller_fid = function_behavior->CGetBehavioralHelper()->get_function_index();
         const auto caller_hls = GetPointerS<const HLS_manager>(AppM)->get_HLS(caller_fid);
         const auto is_bounded = [&](OpGraph::vertex_descriptor op) {
            const auto op_fu = caller_hls->allocation_information->get_fu(caller_hls->Rfu->get_assign(op));
            THROW_ASSERT(op_fu && op_fu->get_kind() == functional_unit_K, "");
            const auto op_op_node = GetPointerS<functional_unit>(op_fu)->get_operation(
                ir_helper::NormalizeTypename(op_cfg.CGetNodeInfo(op).GetOperation()));
            THROW_ASSERT(op_op_node && op_op_node->get_kind() == operation_K, "");
            return GetPointerS<operation>(op_op_node)->bounded;
         };
         if(is_bounded(statement))
         {
            return InstructionWriter::write(function_behavior, statement, varFunctor);
         }

         const auto called_fnode = AppM->get_ir_manager()->GetIRNode(*op_info.called.begin());
         const auto& bb_collection = function_behavior->GetBBGraphsCollection();
         const auto& bb_info =
             bb_collection.CGetNodeInfo(bb_collection.CGetGraphInfo().bb_index_map.at(op_info.bb_index));
         const auto& sch = caller_hls->Rsch;
         const auto get_owning_stage = [&](OpGraph::vertex_descriptor op) {
            auto cstep = sch->get_cstep(op).second;
            if(sch->IsLoopPipelined(op_info.bb_index))
            {
               cstep %= sch->GetLoopPipeliningII(op_info.bb_index);
            }
            return cstep;
         };
         const auto stmt_stage = get_owning_stage(statement);

         std::vector<OpGraph::vertex_descriptor> concurrent_ops;
         size_t statement_pos = 0;
         for(auto op : bb_info.statements_list)
         {
            const auto& oi = op_cfg.CGetNodeInfo(op);
            if(!oi.called.empty() && !is_bounded(op) && get_owning_stage(op) == stmt_stage)
            {
               concurrent_ops.push_back(op);
               if(op == statement)
               {
                  statement_pos = concurrent_ops.size();
               }
            }
         }
         THROW_ASSERT(statement_pos,
                      "Current statement not found in BB" + std::to_string(op_info.bb_index) + " statements' list.");

         if(statement_pos == 1)
         {
            indented_output_stream->Append("__bambu_csim_fork_init(" + std::to_string(caller_fid) + ", " +
                                           std::to_string(concurrent_ops.size()) + ");\n");
         }

         auto call_stmt = function_behavior->CGetBehavioralHelper()->print_vertex(op_cfg, statement, varFunctor);
         call_stmt.pop_back();
         call_stmt.pop_back();
         indented_output_stream->Append("__bambu_csim_fork_call(" + std::to_string(statement_pos - 1) + ", " +
                                        std::to_string(called_fnode->index) + ", 0, " + call_stmt + ");\n");

         if(statement_pos == concurrent_ops.size())
         {
            indented_output_stream->Append("__bambu_csim_fork_fini();\n");
         }
      }
   };

} // namespace

BasicBlocksProfilingCWriter::BasicBlocksProfilingCWriter(const HLS_managerConstRef _HLSMgr,
                                                         const InstructionWriterRef _instruction_writer,
                                                         const IndentedOutputStreamRef _indented_output_stream)
    : EdgeCWriter(_HLSMgr,
                  std::make_shared<WrapFunctionInstructionWriter>(*_instruction_writer, enable_instrumentation),
                  _indented_output_stream)
{
   debug_level = _HLSMgr->get_parameter()->get_class_debug_level(GET_CLASS(*this));
}

void BasicBlocksProfilingCWriter::print_loop_ending(unsigned fid, BBGraph::edge_descriptor e)
{
   print_edge(fid, e, 0);
}

void BasicBlocksProfilingCWriter::print_loop_escaping(unsigned fid, BBGraph::edge_descriptor e)
{
   print_edge(fid, e, 0);
}

void BasicBlocksProfilingCWriter::print_loop_starting(unsigned fid, BBGraph::edge_descriptor e)
{
   print_edge(fid, e, 0);
}

void BasicBlocksProfilingCWriter::print_edge(unsigned fid, BBGraph::edge_descriptor e, unsigned int)
{
   if(enable_instrumentation)
   {
      const auto function_behavior = HLSMgr->CGetFunctionBehavior(fid);
      const auto support_cfg = function_behavior->GetBBGraph(FunctionBehavior::BB);
      const auto src_bbi = support_cfg.CGetNodeInfo(support_cfg.source(e)).block->number;
      const auto tgt_bbi = support_cfg.CGetNodeInfo(support_cfg.target(e)).block->number;
      if(tgt_bbi != BB_EXIT)
      {
         const auto sch = HLSMgr->get_HLS(fid)->Rsch;
         int csteps = 0;
         if(tgt_bbi == src_bbi)
         {
            csteps = static_cast<int>(sch->GetLoopPipeliningII(tgt_bbi));
         }
         indented_output_stream->Append("__bambu_csim_trace(" + std::to_string(fid) + ", " + std::to_string(tgt_bbi) +
                                        ", " + std::to_string(csteps) + ");\n");
      }
   }
   dumped_edges.insert(e);
}

void BasicBlocksProfilingCWriter::print_loop_switching(unsigned fid, BBGraph::edge_descriptor e)
{
   print_edge(fid, e, 0);
}

void BasicBlocksProfilingCWriter::StartFunctionBody(const unsigned int function_id)
{
   const auto& fsm_info = HLSMgr->get_HLS(function_id)->fsm_info;
   const bool is_bounded = fsm_info->bounded;
   enable_instrumentation = HLSMgr->CGetCallGraphManager().GetRootFunctions().count(function_id) || !is_bounded;
   EdgeCWriter::StartFunctionBody(function_id);
}

void BasicBlocksProfilingCWriter::EndFunctionBody(unsigned int function_id)
{
   EdgeCWriter::EndFunctionBody(function_id);
   enable_instrumentation = false;
}

void BasicBlocksProfilingCWriter::InternalWriteHeader()
{
   EdgeCWriter::InternalWriteHeader();
   indented_output_stream->Append("#include <assert.h>\n\n");
   indented_output_stream->Append("#define __BAMBU_IPC_ENTITY MDPI_ENTITY_SIM\n");
   indented_output_stream->Append("#include <mdpi/mdpi_bbp.h>\n");
   indented_output_stream->Append("#include <mdpi/mdpi_debug.h>\n\n");
}

void BasicBlocksProfilingCWriter::InternalWriteGlobalDeclarations()
{
   indented_output_stream->Append("struct __bambu_csim_design_stats __bambu_csim_current_run = {\n");
   indented_output_stream->Append(".run_id = 0,\n");
   indented_output_stream->Append(".functions = (struct __bambu_csim_function_stats[]){\n");
   CustomOrderedSet<unsigned int> functions = HLSMgr->CGetCallGraphManager().GetReachedBodyFunctions();
   const auto lib_functions = HLSMgr->CGetCallGraphManager().GetReachedLibraryFunctions();
   functions.insert(lib_functions.begin(), lib_functions.end());
   size_t func_stat_label_idx = 0;
   std::string func_stat_labels;
   for(const auto fid : functions)
   {
      const auto fnode = TM->GetIRNode(fid);
      func_stat_labels += "#define __bambu_func_" + std::to_string(fid) + "_stats __bambu_csim_current_run.functions[" +
                          std::to_string(func_stat_label_idx++) + "]\n";
      indented_output_stream->Append("{\n.id = " + std::to_string(fid) + ",\n");
      if(ir_helper::IsFunctionImplemented(fnode))
      {
         const auto function_behavior = HLSMgr->CGetFunctionBehavior(fid);
         const auto& bb_graphs = function_behavior->GetBBGraphsCollection();
         const auto& bb_index_map = bb_graphs.CGetGraphInfo().bb_index_map;
         const auto fd = GetPointerS<const function_val_node>(fnode);
         const auto sl = GetPointerS<const statement_list_node>(fd->body);
         const auto max_bbi = sl->list_of_bloc.rbegin()->first + 1U;
         const auto hls = HLSMgr->get_HLS(fid);

         std::set<unsigned> dummy_bbi;
         for(const auto v : hls->fsm_info->vertices())
         {
            const auto& info = hls->fsm_info->getState(v);
            if(info.isDummy)
            {
               dummy_bbi.insert(info.bbId);
            }
         }

         indented_output_stream->Append(".bb_exec = (struct __bambu_csim_bb_stats[]){");
         for(unsigned bbi = 0; bbi < max_bbi; ++bbi)
         {
            unsigned csteps = 0U;
            if(auto it = sl->list_of_bloc.find(bbi); it != sl->list_of_bloc.end())
            {
               const auto& bb_info = bb_graphs.CGetNodeInfo(bb_index_map.at(bbi));
               const auto [min_cstep, max_cstep] = compute_bb_csteps(bb_info, hls->Rsch);
               auto cstep_fix = 0u;
               if(!dummy_bbi.count(bbi))
               {
                  cstep_fix += 1u;
               }
               csteps = max_cstep - min_cstep + cstep_fix;
            }
            indented_output_stream->Append("{0, " + std::to_string(csteps) + "},");
         }
         indented_output_stream->Append("},\n");
         indented_output_stream->Append(".bb_exec_size = " + std::to_string(max_bbi) + ",\n");
      }
      else
      {
         indented_output_stream->Append(".bb_exec = (struct __bambu_csim_bb_stats[]){},\n");
         indented_output_stream->Append(".bb_exec_size = 0,\n");
      }
      indented_output_stream->Append(".last_cycle = 0\n},\n");
   }
   indented_output_stream->Append("},\n");
   indented_output_stream->Append(".functions_size = " + std::to_string(functions.size()) + "\n");
   indented_output_stream->Append("};\n\n");
   indented_output_stream->Append(func_stat_labels);

   indented_output_stream->Append(R"(
#define __bambu_csim_func_stats(fid) (__bambu_func_##fid##_stats)
#define __bambu_csim_call_init(called_id, fix) __bambu_csim_module_push(&__bambu_csim_func_stats(called_id), fix)
#define __bambu_csim_call_fini() __bambu_csim_module_pop(1)

#define __bambu_csim_fork_init(fid, count)                                                         \
   do {                                                                                            \
      unsigned __bambu_csim_fork_i = 0;                                                            \
      struct __bambu_csim_function_stats* __bambu_csim_fork_stats = &__bambu_csim_func_stats(fid); \
      unsigned long long __bambu_csim_fork_last_cycle = __bambu_csim_fork_stats->last_cycle;       \
      unsigned long long __bambu_csim_fork[count] = {0,}

#define __bambu_csim_fork_call(idx, fid, fix, stmt)                                \
   __bambu_csim_call_init(fid, fix);                                               \
   stmt;                                                                           \
   __bambu_csim_call_fini();                                                       \
   __bambu_csim_fork[__bambu_csim_fork_i++] = __bambu_csim_fork_stats->last_cycle; \
   __bambu_csim_fork_stats->last_cycle = __bambu_csim_fork_last_cycle

#define __bambu_csim_fork_fini()                                  \
      for(unsigned i = 1; i < __bambu_csim_fork_i; ++i)           \
         if(__bambu_csim_fork[i] > __bambu_csim_fork[0])          \
            __bambu_csim_fork[0] = __bambu_csim_fork[i];          \
      __bambu_csim_fork_stats->last_cycle = __bambu_csim_fork[0]; \
   } while(0)
     

#define __bambu_csim_trace(fid, bbi, csteps) __bambu_csim_design_stats_trace(&__bambu_csim_func_stats(fid), bbi, csteps)

#ifdef __clang__
#pragma clang diagnostic ignored "-Wincompatible-pointer-types"
#else
#pragma GCC diagnostic ignored "-Wincompatible-pointer-types"
#endif

)");
   EdgeCWriter::InternalWriteGlobalDeclarations();
}
