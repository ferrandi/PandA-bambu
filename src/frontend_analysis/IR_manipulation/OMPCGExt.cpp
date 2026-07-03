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
 *              Copyright (C) 2023-2026 Politecnico di Milano
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
 * @file OMPCGExt.cpp
 *
 * @author Michele Fiorito <michele.fiorito@polimi.it>
 *
 */

#include "OMPCGExt.hpp"

#include "Parameter.hpp"
#include "Range.hpp"
#include "SemiNCADominance.hpp"
#include "application_manager.hpp"
#include "area_info.hpp"
#include "basic_block.hpp"
#include "basic_blocks_graph_constructor.hpp"
#include "behavioral_helper.hpp"
#include "bit_lattice.hpp"
#include "call_graph_manager.hpp"
#include "dbgPrintHelper.hpp"
#include "function_behavior.hpp"
#include "functions.hpp"
#include "hls_device.hpp"
#include "hls_manager.hpp"
#include "ir_basic_block.hpp"
#include "ir_helper.hpp"
#include "ir_manager.hpp"
#include "ir_manipulation.hpp"
#include "ir_node.hpp"
#include "library_manager.hpp"
#include "op_graph.hpp"
#include "string_manipulation.hpp"
#include "structural_manager.hpp"
#include "structural_objects.hpp"
#include "technology_manager.hpp"
#include "technology_node.hpp"
#include "time_info.hpp"
#include "var_pp_functor.hpp"

#include "kmp_bambu_names.h"

#define VAL(str) #str
#define TOSTRING(str) VAL(str)

#define EPSILON 0.000000001

/// Track the number of threads currently required in case of a kmp_bambu_fork_call function call
static unsigned int __kmp_bambu_t_nproc = 0U;

static time_infoRef get_assign_time_info(const application_managerRef& AppM)
{
   const auto HLSMgr = GetPointer<HLS_manager>(AppM);
   const auto HLS_D = HLSMgr->get_HLS_device();
   const auto TechM = HLS_D->get_technology_manager();
   auto time_m = std::make_shared<time_info>();
   time_m->set_execution_time(TechM->CGetSetupHoldTime() + EPSILON, 0);
   time_m->set_synthesis_dependent(true);
   return time_m;
}

static std::string get_cs_fu(const std::string& fu_name)
{
   return "KMP_BAMBU_CS" + fu_name.substr(sizeof("KMP_BAMBU") - 1U);
}

CustomMap<unsigned int, std::vector<OMPInfoRef>> OMPCGExt::fork_infos;
unsigned int OMPCGExt::_thread_context_count = 0;

OMPCGExt::OMPCGExt(const ParameterConstRef _parameters, const application_managerRef _AppM, unsigned int _function_id,
                   const FrontendFlowStepType _frontend_flow_step_type, const DesignFlowManager& _design_flow_manager)
    : FunctionFrontendFlowStep(_AppM, _function_id, _frontend_flow_step_type, _design_flow_manager, _parameters),
      _assign_time_info(get_assign_time_info(AppM)),
      TM(_AppM->get_ir_manager()),
      ir_man(new ir_manipulation(_AppM->get_ir_manager(), _parameters, _AppM))
{
   if(!__kmp_bambu_t_nproc)
   {
      __kmp_bambu_t_nproc = _parameters->getOption<unsigned int>(OPT_num_accelerators);
      _thread_context_count =
          _parameters->isOption(OPT_context_switch) ? _parameters->getOption<unsigned int>(OPT_context_switch) : 1U;
   }
}

OMPCGExt::OMPCGExt(const ParameterConstRef _parameters, const application_managerRef _AppM, unsigned int _function_id,
                   const DesignFlowManager& _design_flow_manager)
    : OMPCGExt(_parameters, _AppM, _function_id, OMP_CG_EXT, _design_flow_manager)
{
   debug_level = _parameters->get_class_debug_level(GET_CLASS(*this), DEBUG_LEVEL_NONE);
}

CustomUnorderedSet<std::pair<FrontendFlowStepType, FunctionFrontendFlowStep::FunctionRelationship>>
OMPCGExt::ComputeFrontendRelationships(const DesignFlowStep::RelationshipType relationship_type) const
{
   CustomUnorderedSet<std::pair<FrontendFlowStepType, FunctionRelationship>> relationships;
   switch(relationship_type)
   {
      case(DEPENDENCE_RELATIONSHIP):
      {
         relationships.insert(std::make_pair(IR_LOWERING, SAME_FUNCTION));
         relationships.insert(std::make_pair(OMP_CG_EXT, CALLING_FUNCTIONS));
         break;
      }
      case(PRECEDENCE_RELATIONSHIP):
      {
         break;
      }
      case(INVALIDATION_RELATIONSHIP):
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

bool OMPCGExt::HasToBeExecuted() const
{
   return bb_version == 0 && FunctionFrontendFlowStep::HasToBeExecuted();
}

DesignFlowStep_Status OMPCGExt::InternalExec()
{
   if(fork_infos.count(function_id))
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Generated fork call lowering not needed");
      return DesignFlowStep_Status::UNCHANGED;
   }

   const auto omp_info = function_behavior->GetOMPInfo();
   if(omp_info)
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Fork call id : " + STR(omp_info->fork_call_id));
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Context count: " + STR(omp_info->context_count));
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Local index  : " + STR(omp_info->local_idx));
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Global index : " + STR(omp_info->global_idx));
   }
   else
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Missing OMP info");
   }
   auto& CGM = AppM->GetCallGraphManager();
   const auto& CG = CGM.GetCallGraph();
   const auto extract_fnode = [](const ir_nodeRef& tn) {
      if(tn->get_kind() == addr_node_K)
      {
         return GetPointerS<const unary_node>(tn)->op;
      }
      THROW_ASSERT(tn->get_kind() == function_val_node_K, "");
      return tn;
   };
   const auto remove_address_edge = [&](unsigned int source, unsigned int dest) {
      const auto source_v = CGM.GetVertex(source);
      const auto dest_v = CGM.GetVertex(dest);
      if(CG.ExistsEdge(source_v, dest_v))
      {
         const auto edge = CG.CGetEdge(source_v, dest_v);
         const auto& edge_info = CG.CGetEdgeInfo(edge);
         THROW_ASSERT(edge_info.direct_call_points.empty() && edge_info.indirect_call_points.empty(),
                      "Unexpected call points.");
         const auto to_remove = edge_info.function_addresses;
         for(const auto& call_id : to_remove)
         {
            CGM.RemoveCallPoint(edge, call_id);
         }
      }
   };
   const auto set_t_nproc = [&](unsigned int t_nproc) {
      if(omp_info)
      {
         omp_info->kmp_t_nproc = t_nproc;
      }
      else
      {
         __kmp_bambu_t_nproc = t_nproc;
      }
   };
   const auto get_t_nproc = [&]() { return omp_info ? omp_info->kmp_t_nproc : __kmp_bambu_t_nproc; };

   bool modified = false;
   CustomUnorderedSet<unsigned int> already_visited;
   const auto bb_topological = DominatorTopologicalSort(function_id, AppM);
   for(const auto& BB : bb_topological)
   {
      const auto stmt_list = BB->CGetStmtList();
      for(const auto& stmt : stmt_list)
      {
         if(stmt->get_kind() == call_stmt_K)
         {
            const auto gc = GetPointerS<const call_stmt>(stmt);
            const auto called_fnode = extract_fnode(gc->fn);
            const auto called_fname = ir_helper::GetFunctionName(called_fnode);
            if(called_fname == TOSTRING(KMP_TH_SET_NPROC))
            {
               THROW_ASSERT(gc->args.size() >= 2, "Expected at least two arguments for " TOSTRING(KMP_TH_SET_NPROC));
               const auto nthread = gc->args.at(1);
               const auto ic = GetPointer<const constant_int_val_node>(nthread);
               THROW_ASSERT(ic, "Expected an integer constant: " + nthread->get_kind_text() + " " + STR(nthread));
               set_t_nproc(static_cast<unsigned int>(ic->value));
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                              "---OpenMP required " + STR(get_t_nproc()) + " threads");
               BB->RemoveStmt(stmt, AppM);
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Removed " + STR(stmt));
               modified = true;
            }
            else if(called_fname == TOSTRING(KMP_FORK_CALL))
            {
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Expanding " + STR(stmt));
               THROW_ASSERT(gc->args.size() >= 2, "Expected at least two arguments for " TOSTRING(KMP_FORK_CALL));
               const auto outlined_fnode = ir_helper::GetBaseVariable(gc->args.at(1));
               THROW_ASSERT(outlined_fnode->get_kind() == function_val_node_K, "");

               const auto version_fnode = [&]() {
                  const auto fname = ir_helper::GetFunctionName(called_fnode);
                  const auto version_suffix = "_" + STR(function_id) + "_" + STR(outlined_fnode->index);
                  auto fnode = TM->GetFunction(fname + version_suffix);
                  if(!fnode)
                  {
                     fnode = ir_man->CloneFunction(called_fnode, version_suffix);

                     THROW_WARNING_ASSERT(
                         __kmp_bambu_t_nproc > 1U,
                         "OpenMP threads count is set to 1. Please remove -fopenmp command line option "
                         "to avoid the OpenMP overhead.");
                     ExpandForkCall(fnode, outlined_fnode, get_t_nproc());
                  }
                  return fnode;
               }();

               // BEAWARE: call point removal is done through the following instead of simply using stmt id since
               // call graph expansion on partially modified functions leads to inaccurate placing of addressed call
               // points on generic statements which are not actual call statements (because of function call type
               // cleanup introducing casts and view converts on call arguments)
               remove_address_edge(function_id, outlined_fnode->index);
               const auto version_call =
                   ir_man->create_call_stmt(version_fnode, gc->args, function_id, BUILTIN_LOCINFO);
               BB->Replace(stmt, version_call, true, AppM);
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Expanded " + STR(version_call));
               modified = true;
            }
         }
      }
   }
   if(modified)
   {
      AppM->GetFunctionBehavior(function_id)->UpdateBBVersion();
      return DesignFlowStep_Status::SUCCESS;
   }
   return DesignFlowStep_Status::UNCHANGED;
}

unsigned int OMPCGExt::ComputeCurrentThreads(unsigned int local_idx, unsigned int nthread, unsigned int ncontext)
{
   return (local_idx || (nthread % ncontext == 0)) ? ncontext : nthread % ncontext;
}

void OMPCGExt::ExpandForkCall(const ir_nodeRef& version_fnode, const ir_nodeRef& outlined_fnode,
                              const unsigned int nthread) const
{
   auto& CGM = AppM->GetCallGraphManager();
   const auto fork_call_id = version_fnode->index;
   GetPointerS<HLS_manager>(AppM)
       ->global_resource_constraints[std::make_pair(ir_helper::GetFunctionName(version_fnode), WORK_LIBRARY)] =
       std::make_pair(1U, 1U);

   // Fix function type
   const auto version_fd = GetPointerS<function_val_node>(version_fnode);
   const auto version_ftype = GetPointerS<function_ty_node>(version_fd->type);
   version_ftype->varargs_flag = false;

   const auto outlined_fd = GetPointerS<const function_val_node>(outlined_fnode);
   std::vector<ir_nodeRef> fork_param_ssa;
   for(auto it = outlined_fd->list_of_args.begin(); it != outlined_fd->list_of_args.end(); ++it)
   {
      const auto pd = GetPointerS<const argument_val_node>(*it);

      // Add parameter to function decl
      const auto fork_parm =
          ir_man->create_parm_decl(pd->name, pd->type, version_fnode, nullptr, BUILTIN_LOCINFO, pd->readonly_flag);
      version_fd->AddArg(fork_parm);
      version_ftype->AddArgType(pd->type);

      // Add parameter ssa declaration
      const auto param_ssa = ir_man->create_ssa_name(fork_parm, pd->type, ir_nodeConstRef(), ir_nodeConstRef());
      const auto gn_nid = TM->new_ir_node_id();
      TM->AddIRNode(ir_nodeRef(new nop_stmt(gn_nid)));
      GetPointerS<ssa_node>(param_ssa)->SetDefStmt(TM->GetIRNode(gn_nid));
      fork_param_ssa.push_back(param_ssa);
   }

   // Add basic block structure
   const auto version_sl_nid = TM->new_ir_node_id();
   const auto sl = new statement_list_node(version_sl_nid);
   TM->AddIRNode(ir_nodeRef(sl));
   version_fd->body = TM->GetIRNode(version_sl_nid);
   blocRef entry(new bloc(bloc::ENTRY_BLOCK_ID));
   blocRef exit(new bloc(bloc::EXIT_BLOCK_ID));
   blocRef bb(new bloc(2U));
   entry->list_of_succ.push_back(exit->number);
   exit->list_of_pred.push_back(entry->number);
   entry->list_of_succ.push_back(bb->number);
   bb->list_of_pred.push_back(entry->number);
   bb->list_of_succ.push_back(exit->number);
   exit->list_of_pred.push_back(bb->number);
   sl->add_bloc(entry);
   sl->add_bloc(bb);
   sl->add_bloc(exit);

   // Add multiple omp outlined function calls
   const auto outlined_id = outlined_fnode->index;
   const auto outlined_parm_type = ir_helper::GetParameterTypes(outlined_fnode);
   const auto reached_from_outlined = AppM->CGetCallGraphManager().GetReachedFunctionsFrom(outlined_id, false);
   const auto ncontext = [&]() {
      const auto fork_call = TM->GetFunction(TOSTRING(KMP_FORK_CALL))->index;
      return reached_from_outlined.count(fork_call) ? 1U : _thread_context_count;
   }();
   const auto ncore = nthread / ncontext + (nthread % ncontext != 0);
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                  "---Required " + STR(nthread) + " OMP threads with " + STR(ncontext) + " context cores");
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Generating " + STR(ncore) + " OMP cores.");
   THROW_ASSERT(ncore, "At least one OMP core must be there.");
   const auto parent_info = function_behavior->GetOMPInfo();
   if(parent_info)
   {
      // Nested fork call must have limited memory channels
      SetSimpleMemFunction(fork_call_id, parent_info);
   }
   const auto gr = ir_man->create_return_stmt(ir_man->GetVoidType(), nullptr, fork_call_id, BUILTIN_LOCINFO);
   std::vector<OMPInfoRef> cores(ncore, nullptr);
   for(auto core_idx = 0U; core_idx < ncore; ++core_idx)
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Generating OMP core " + STR(core_idx) + "...");
      const auto suffix =
          core_idx || (parent_info && parent_info->global_idx) ? OMPForkSuffix(fork_call_id, core_idx) : "";
      const auto outlined_clone_fnode = ir_man->CloneFunction(outlined_fnode, suffix);
      const auto outlined_clone_id = outlined_clone_fnode->index;
      const auto current_threads = ComputeCurrentThreads(core_idx, nthread, ncontext);
      THROW_ASSERT(current_threads, "At least one thread per core must be there.");
      CGM.SetOMPThreadsCount(outlined_clone_id, current_threads);
      OMPInfoRef omp_info(new OMPInfo(core_idx, current_threads, outlined_clone_id, ncore, fork_call_id, parent_info,
                                      __kmp_bambu_t_nproc));
      SetSimpleMemFunction(outlined_clone_id, omp_info);
      AppM->GetFunctionBehavior(outlined_clone_id)->SetOMPCore(true);
      cores[core_idx] = omp_info;
      const auto outlined_call =
          ir_man->create_call_stmt(outlined_clone_fnode, fork_param_ssa, fork_call_id, BUILTIN_LOCINFO);
      const auto call_vssa = ir_man->create_ssa_name(nullptr, ir_man->GetVoidType(), nullptr, nullptr, true);
      GetPointerS<node_stmt>(outlined_call)->SetVdef(call_vssa);
      GetPointerS<node_stmt>(gr)->AddVuse(call_vssa);
      bb->PushBack(outlined_call, AppM);
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Added " + STR(outlined_call));
   }
   bb->PushBack(gr, AppM);

   if(ncore != nthread)
   {
      GenerateContextSwitchInterface(cores);
   }

   const auto kmp_get_reduce_data = TM->GetFunction(TOSTRING(KMP_GET_REDUCE_DATA));
   if(kmp_get_reduce_data && reached_from_outlined.count(kmp_get_reduce_data->index))
   {
      GenerateThreadLocalDataAccessorsHW(cores);
   }

   const auto kmp_barrier_reached = TM->GetFunction(TOSTRING(KMP_BARRIER_REACHED));
   if(kmp_barrier_reached && reached_from_outlined.count(kmp_barrier_reached->index))
   {
      GenerateBarrierAccessors(cores);
   }

   const auto kmp_tid_from_gtid = TM->GetFunction(TOSTRING(KMP_GET_TID_FROM_GTID));
   if(kmp_tid_from_gtid && reached_from_outlined.count(kmp_tid_from_gtid->index))
   {
      GenerateThreadIDAccessors(cores, nthread, ncontext);
   }

   CustomUnorderedSet<unsigned int> AV;
   for(const auto& omp_info : cores)
   {
      CallGraphManager::expandCallGraphFromFunction(AV, AppM, omp_info->core_id, DEBUG_LEVEL_NONE);
   }
   fork_infos[fork_call_id] = std::move(cores);
}

void OMPCGExt::GenerateThreadIDAccessors(const std::vector<OMPInfoRef>& cores, const unsigned int nthread,
                                         const unsigned int ncontext) const
{
   const auto fork_call_id = cores.at(0)->fork_call_id;
   const auto ncore = static_cast<unsigned int>(cores.size());
   const auto kmp_tid_from_gtid = TM->GetFunction(TOSTRING(KMP_GET_TID_FROM_GTID));
   THROW_ASSERT(kmp_tid_from_gtid, "Expected " TOSTRING(KMP_GET_TID_FROM_GTID) " signature to be present.");
   const auto tid_from_gtid_fnode = ir_man->CloneFunction(kmp_tid_from_gtid, OMPForkSuffix(fork_call_id, 0));
   {
      auto tid_from_gtid_fd = GetPointerS<function_val_node>(tid_from_gtid_fnode);

      // Add basic block structure
      const auto version_sl_nid = TM->new_ir_node_id();
      const auto sl = new statement_list_node(version_sl_nid);
      TM->AddIRNode(ir_nodeRef(sl));
      tid_from_gtid_fd->body = TM->GetIRNode(version_sl_nid);
      blocRef entry(new bloc(bloc::ENTRY_BLOCK_ID));
      blocRef exit(new bloc(bloc::EXIT_BLOCK_ID));
      blocRef bb(new bloc(2U));
      entry->list_of_succ.push_back(exit->number);
      exit->list_of_pred.push_back(entry->number);
      entry->list_of_succ.push_back(bb->number);
      bb->list_of_pred.push_back(entry->number);
      sl->add_bloc(entry);
      sl->add_bloc(bb);
      sl->add_bloc(exit);

      // Create multi way if for local data variable selection
      const auto get_mwi_nid = TM->new_ir_node_id();
      const auto get_mwi = new multi_way_if_stmt(get_mwi_nid);
      TM->AddIRNode(ir_nodeRef(get_mwi));

      THROW_ASSERT(tid_from_gtid_fd->list_of_args.size(),
                   "At least one argument expected for function " TOSTRING(KMP_GET_TID_FROM_GTID));
      const auto gtid_parmssa = ir_man->create_ssa_name(tid_from_gtid_fd->list_of_args.at(0),
                                                        ir_helper::CGetType(tid_from_gtid_fd->list_of_args.at(0)),
                                                        ir_nodeConstRef(), ir_nodeConstRef());
      {
         const auto gn_nid = TM->new_ir_node_id();
         TM->AddIRNode(ir_nodeRef(new nop_stmt(gn_nid)));
         GetPointerS<ssa_node>(gtid_parmssa)->SetDefStmt(TM->GetIRNode(gn_nid));
      }

      const auto tid_type = ir_helper::GetFunctionReturnType(tid_from_gtid_fnode);
      const auto parent_info = cores.at(0)->parent;

      // Generate a branch for each local data variable in the multi way if
      auto min_tid = 0U;
      for(auto core_idx = 0U; core_idx < cores.size(); ++core_idx)
      {
         const auto current_threads = ComputeCurrentThreads(core_idx, nthread, ncontext);
         const auto max_tid = min_tid + current_threads;
         for(auto tid = min_tid; tid < max_tid; ++tid)
         {
            const auto gtid = OMPInfo::make_global(tid, ncore, parent_info);

            // Create new basic block
            blocRef ret_bb(new bloc(bb->number + tid + 1U));
            bb->list_of_succ.push_back(ret_bb->number);
            ret_bb->list_of_pred.push_back(bb->number);
            ret_bb->list_of_succ.push_back(exit->number);
            exit->list_of_pred.push_back(ret_bb->number);
            sl->add_bloc(ret_bb);

            // Define switch condition
            const auto cmp = ir_man->CreateAssignStmt(
                ir_man->GetBooleanType(), nullptr, nullptr,
                ir_man->CreateEqExpr(gtid_parmssa, TM->CreateUniqueIntegerCst(gtid, ir_helper::CGetType(gtid_parmssa)),
                                     bb, tid_from_gtid_fd->index),
                tid_from_gtid_fd->index, BUILTIN_LOCINFO);
            bb->PushBack(cmp, AppM);
            get_mwi->add_cond(GetPointerS<const assign_stmt>(cmp)->op0, ret_bb->number);

            const auto gr = ir_man->create_return_stmt(tid_type, TM->CreateUniqueIntegerCst(tid, tid_type),
                                                       tid_from_gtid_fd->index, BUILTIN_LOCINFO);
            ret_bb->PushBack(gr, AppM);
         }
         min_tid = max_tid;
      }

      // Add default branch to multi way if
      blocRef ret_default(new bloc(bb->number + nthread + 1U));
      bb->list_of_succ.push_back(ret_default->number);
      ret_default->list_of_pred.push_back(bb->number);
      ret_default->list_of_succ.push_back(exit->number);
      exit->list_of_pred.push_back(ret_default->number);
      sl->add_bloc(ret_default);
      ret_default->PushBack(ir_man->create_return_stmt(tid_type, TM->CreateUniqueIntegerCst(0ULL, tid_type),
                                                       tid_from_gtid_fd->index, BUILTIN_LOCINFO),
                            AppM);
      get_mwi->add_cond(nullptr, ret_default->number);
      bb->PushBack(TM->GetIRNode(get_mwi_nid), AppM);
      // SetSimpleMemFunction(tid_from_gtid_fnode->index);
   }
   for(auto i = 0U; i < ncore; ++i)
   {
      const auto replica_fnode = ir_man->CloneFunction(tid_from_gtid_fnode, OMPProcSuffix(cores.at(i)->core_id));
      SetSimpleMemFunction(replica_fnode->index);
   }
}

void OMPCGExt::GenerateThreadLocalDataAccessorsHW(const std::vector<OMPInfoRef>& cores) const
{
   static const std::vector<std::pair<const char*, const char*>> hw_functions = {
       {TOSTRING(KMP_GET_REDUCE_DATA), "KMP_BAMBU_GET_TH_LOCAL_REDUCE_DATA_FU"},
       {TOSTRING(KMP_SET_REDUCE_DATA), "KMP_BAMBU_SET_TH_LOCAL_REDUCE_DATA_FU"},
   };

   for(const auto& fname_fu : hw_functions)
   {
      const auto& fname = fname_fu.first;
      const auto& fu_name = fname_fu.second;
      auto freplicas = GenerateOMPReplicas(fname, cores);
      const auto fork_call_id = cores.at(0)->fork_call_id;
      auto i = 0U;
      for(const auto& replica : freplicas)
      {
         const auto replica_fu_name = fu_name + OMPForkSuffix(fork_call_id, cores.at(i++)->core_id);
         CloneFU(fu_name, replica_fu_name);
         const auto HLSMgr = GetPointer<HLS_manager>(AppM);
         const auto HLS_D = HLSMgr->get_HLS_device();
         const auto TechM = HLS_D->get_technology_manager();

         const auto fname_replica = ir_helper::GetFunctionName(replica);
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                        "---Registered operation " + fname_replica + " on FU " + replica_fu_name);
         std::string fu_lib;
         const auto fu = TechM->get_fu(replica_fu_name, &fu_lib);
         const auto op_node = TechM->add_operation(fu_lib, replica_fu_name, fname_replica);
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Timed operation: " + fname_replica);
         const auto op = GetPointerS<operation>(op_node);
         op->bounded = true;
         auto& time_m = op->time_m;
         if(time_m)
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                           "TIME_M already initialized: " + time_m->get_cycles());
         }
         else
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "TIME_M not initialized: ");
         }
         time_m = std::make_shared<time_info>();
         time_m->set_execution_time(0.1, 2U);
         time_m->set_stage_period(1.1110000000000001);
         time_m->set_initiation_time(1U);
         time_m->set_synthesis_dependent(true);
      }
   }
}

void OMPCGExt::GenerateBarrierAccessors(const std::vector<OMPInfoRef>& cores) const
{
   const auto fork_call_id = cores.at(0)->fork_call_id;

   static const std::vector<std::tuple<const char*, const char*, bool>> hw_functions = {
       {TOSTRING(KMP_BARRIER_REACHED), "KMP_BAMBU_BARRIER_REACHED_FU", true},
       {TOSTRING(KMP_WAIT_ALL_THREADS), "KMP_BAMBU_WAIT_ALL_THREADS_FU", false}};

   for(const auto& fname_fu : hw_functions)
   {
      const auto& fname = std::get<0>(fname_fu);
      const auto& fu_name = std::get<1>(fname_fu);
      const auto& fu_bounded = std::get<2>(fname_fu);
      if(TM->GetFunction(fname))
      {
         const auto freplicas = GenerateOMPReplicas(fname, cores);
         auto i = 0U;
         for(const auto& replica : freplicas)
         {
            const auto local_fu_name = (!fu_bounded && cores.at(i)->context_count > 1U) ? get_cs_fu(fu_name) : fu_name;
            const auto replica_fu_name = local_fu_name + OMPForkSuffix(fork_call_id, cores.at(i)->core_id);
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                           "Cloning " + STR(local_fu_name) + " to " + STR(replica_fu_name));
            CloneFU(local_fu_name, replica_fu_name);
            BindFunction(replica_fu_name, replica, _assign_time_info, fu_bounded);
            i += 1U;
         }
      }
   }
}

void OMPCGExt::GenerateCriticalAccessors(const unsigned int loc_id, const std::vector<OMPInfoRef>& cores) const
{
   static const std::vector<std::tuple<const char*, const char*, bool>> hw_functions = {
       {TOSTRING(KMP_CRITICAL), "KMP_BAMBU_CRITICAL_FU", false},
       {TOSTRING(KMP_END_CRITICAL), "KMP_BAMBU_END_CRITICAL_FU", true},
   };
   THROW_ASSERT(TM->GetFunction(TOSTRING(KMP_CRITICAL)), "Function " TOSTRING(KMP_CRITICAL) " not defined.");
   const auto kmp_scope = GetPointerS<const function_val_node>(TM->GetFunction(TOSTRING(KMP_CRITICAL)))->parent;
   const auto fork_call_id = cores.at(0)->fork_call_id;

   for(const auto& fname_fu : hw_functions)
   {
      const auto fname = STR(std::get<0>(fname_fu)) + "_" + STR(loc_id);
      const auto& fu_name = std::get<1>(fname_fu);
      const auto& fu_bounded = std::get<2>(fname_fu);
      // Generated critical accessor has no arguments since location pointer is embedded in FU name
      const auto fnode = ir_man->create_function_decl(fname, kmp_scope, std::vector<ir_nodeConstRef>(),
                                                      ir_man->GetVoidType(), BUILTIN_LOCINFO, false);

      const auto freplicas = GenerateOMPReplicas(fname, cores);
      auto i = 0U;
      for(const auto& replica : freplicas)
      {
         const auto replica_fu_name = fu_name + OMPForkSuffix(fork_call_id, cores.at(i)->core_id);
         CloneFU(fu_name, replica_fu_name);
         BindFunction(replica_fu_name, replica, _assign_time_info, fu_bounded);
         i += 1U;
      }
   }
}

void OMPCGExt::GenerateContextSwitchInterface(const std::vector<OMPInfoRef>& cores) const
{
   const auto tid_replicas = GenerateOMPReplicasFU(TOSTRING(KMP_CS_GET_TID), "KMP_BAMBU_CS_GET_TID_FU", true, cores);
   const auto gtid_replicas = GenerateOMPReplicasFU(TOSTRING(KMP_CS_GET_GTID), "KMP_BAMBU_CS_GET_GTID_FU", true, cores);
   const auto tid_size = static_cast<Range::bw_t>(ir_helper::Size(ir_helper::CGetType(tid_replicas.at(0))));
   const auto gtid_size = static_cast<Range::bw_t>(ir_helper::Size(ir_helper::CGetType(gtid_replicas.at(0))));

   const auto parent_info = cores.at(0)->parent;
   const auto ncore = static_cast<unsigned int>(cores.size());
   auto tid = 0U;
   auto i = 0U;
   for(const auto& core : cores)
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                     "-->OMP core " + STR(i) + " (" + STR(core->core_id) + ") context:");
      auto tid_fd = GetPointer<function_val_node>(tid_replicas.at(i));
      tid_fd->range = Range(RangeType::Regular, tid_size, tid, tid + core->context_count - 1U);
      tid_fd->bit_values = bitstring_to_string(tid_fd->range.getBitValues(false));
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                     "---Local  thread id range: " + tid_fd->range.ToString() + "<" + tid_fd->bit_values + ">");

      const auto max_tid = static_cast<unsigned>(tid_fd->range.getUnsignedMax());
      const auto gtid = OMPInfo::make_global(tid, ncore, parent_info);
      const auto max_gtid = OMPInfo::make_global(max_tid, ncore, parent_info);

      auto gtid_fd = GetPointer<function_val_node>(tid_replicas.at(i));
      gtid_fd->range = Range(RangeType::Regular, gtid_size, gtid, max_gtid);
      gtid_fd->bit_values = bitstring_to_string(gtid_fd->range.getBitValues(false));
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                     "---Global thread id range: " + gtid_fd->range.ToString() + "<" + gtid_fd->bit_values + ">");
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--");

      tid = max_tid + 1U;
      i += 1U;
   }
}

void OMPCGExt::SetSimpleMemFunction(const unsigned int fid, OMPInfoRef omp_info) const
{
   auto& CGM = AppM->GetCallGraphManager();
   FunctionBehaviorRef FB;
   if(!CGM.IsVertex(fid))
   {
      const auto fnode = AppM->get_ir_manager()->GetIRNode(fid);
      THROW_ASSERT(fnode, "");
      FB = std::make_shared<FunctionBehavior>(AppM, std::make_shared<BehavioralHelper>(AppM, fid, parameters),
                                              parameters);
      CGM.AddFunction(fid, FB);
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                     "---Added definition for " +
                         ir_helper::PrintType(fnode, false, false, nullptr,
                                              std::make_unique<std_var_pp_functor>(FB->CGetBehavioralHelper())));
   }
   else
   {
      FB = AppM->GetFunctionBehavior(fid);
   }
   FB->SetChannelsNumber(1);
   FB->SetChannelsType(static_cast<MemoryAllocation_ChannelsType>(0));
   FB->SetMemoryAllocationPolicy(static_cast<MemoryAllocation_Policy>(3));
   if(omp_info)
   {
      FB->SetOMPInfo(omp_info);
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                     "---Added OMP info to " + FB->CGetBehavioralHelper()->GetFunctionName());
   }
}

ir_nodeRef OMPCGExt::CloneFunction(const ir_nodeRef& base_fnode, const OMPInfoRef& omp_info) const
{
   const auto suffix = OMPProcSuffix(omp_info->global_idx);
   auto clone_fnode = TM->GetFunction(ir_helper::GetFunctionName(base_fnode) + suffix);
   if(!clone_fnode)
   {
      clone_fnode = ir_man->CloneFunction(base_fnode, suffix);
      const auto has_body = GetPointerS<const function_val_node>(base_fnode)->body != nullptr;
      if(!has_body)
      {
         const auto fu_name = functions::GetFUName(base_fnode->index, RefcountCast<HLS_manager>(AppM));
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                        "Cloning functional unit " + STR(fu_name) + " for operation " + STR(fu_name));
         const auto clone_fu_name = fu_name + suffix;
         CloneFU(fu_name, clone_fu_name);
         BindFunction(clone_fu_name, clone_fnode);
      }
   }
   SetSimpleMemFunction(clone_fnode->index, omp_info);
   return clone_fnode;
}

void OMPCGExt::BindFunction(const std::string& fu_name, const ir_nodeRef& fnode, time_infoRef tinfo,
                            bool op_bounded) const
{
   const auto HLSMgr = GetPointer<HLS_manager>(AppM);
   const auto HLS_D = HLSMgr->get_HLS_device();
   const auto TechM = HLS_D->get_technology_manager();

   const auto fname = ir_helper::GetFunctionName(fnode);
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Registered operation " + fname + " on FU " + fu_name);
   std::string fu_lib;
   const auto fu = TechM->get_fu(fu_name, &fu_lib);
   const auto op_node = TechM->add_operation(fu_lib, fu_name, fname);
   if(!tinfo)
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Operation not timed: " + fname);
      THROW_ASSERT(fu->get_kind() == functional_unit_K, "");
      if(GetPointerS<functional_unit>(fu)->get_operations().size())
      {
         const auto& first_op = GetPointerS<const functional_unit>(fu)->get_operations().front();
         tinfo = GetPointerS<const operation>(first_op)->time_m;
         op_bounded = GetPointerS<const operation>(first_op)->bounded;
      }
   }
   if(tinfo)
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Timed operation: " + fname);
      const auto op = GetPointerS<operation>(op_node);
      op->bounded = op_bounded;
      auto& time_m = op->time_m;
      time_m = std::make_shared<time_info>();
      time_m->set_execution_time(tinfo->get_execution_time(), tinfo->get_cycles());
      time_m->set_initiation_time(tinfo->get_initiation_time());
      time_m->set_stage_period(tinfo->get_stage_period());
      time_m->set_synthesis_dependent(true);
   }
}

std::vector<ir_nodeRef> OMPCGExt::GenerateOMPReplicas(const std::string& fname,
                                                      const std::vector<OMPInfoRef>& cores) const
{
   const auto fork_call_id = cores.at(0)->fork_call_id;
   std::vector<ir_nodeRef> replicas(cores.size(), nullptr);
   const auto fnode = TM->GetFunction(fname);
   THROW_ASSERT(fnode, "Expected " + fname + " signature to be present.");
   for(auto i = 0U; i < cores.size(); ++i)
   {
      replicas[i] = ir_man->CloneFunction(fnode, OMPForkSuffix(fork_call_id, cores.at(i)->core_id));
      SetSimpleMemFunction(replicas[i]->index, cores.at(i));
   }
   return replicas;
}

void OMPCGExt::CloneFU(const std::string& fu_name, const std::string& clone_name) const
{
   const auto HLSMgr = GetPointer<HLS_manager>(AppM);
   const auto HLS_D = HLSMgr->get_HLS_device();
   const auto TechM = HLS_D->get_technology_manager();
   std::string fu_lib;
   const auto base_node = TechM->get_fu(fu_name, &fu_lib);
   THROW_ASSERT(base_node->get_kind() == functional_unit_K, "Functional unit is not a functional unit");
   const auto base_fu = GetPointerS<const functional_unit>(base_node);
   const auto clone_node = TechM->add_resource(fu_lib, clone_name, base_fu->CM);
   auto clone_fu = GetPointerS<functional_unit>(clone_node);
   clone_fu->set_clock_period_resource_fraction(base_fu->get_clock_period_resource_fraction());
   clone_fu->area_m = std::make_shared<area_info>();
   clone_fu->area_m->resources[area_info::AREA] = 0;
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                  "---Added functional unit replica " + clone_name + " from base " + fu_name);
}

std::vector<ir_nodeRef> OMPCGExt::GenerateOMPReplicasFU(const std::string& fname, const std::string& fu_name,
                                                        const bool fu_bounded,
                                                        const std::vector<OMPInfoRef>& cores) const
{
   auto freplicas = GenerateOMPReplicas(fname, cores);
   const auto fork_call_id = cores.at(0)->fork_call_id;
   auto i = 0U;
   for(const auto& replica : freplicas)
   {
      const auto replica_fu_name = fu_name + OMPForkSuffix(fork_call_id, cores.at(i++)->core_id);
      CloneFU(fu_name, replica_fu_name);
      BindFunction(replica_fu_name, replica, _assign_time_info, fu_bounded);
   }
   return freplicas;
}

std::vector<blocRef> OMPCGExt::DominatorTopologicalSort(const unsigned int function_id,
                                                        const application_managerRef& AppM)
{
   const auto fnode = AppM->get_ir_manager()->GetIRNode(function_id);
   const auto fd = GetPointerS<const function_val_node>(fnode);
   const auto sl = GetPointerS<const statement_list_node>(fd->body);
   BBGraphsCollection bb_graphs_collection(BBGraphInfo(AppM, function_id));
   BBGraph cfg(bb_graphs_collection, CFG_SELECTOR);
   CustomUnorderedMap<unsigned int, BBGraph::vertex_descriptor> inverse_vertex_map;
   /// add vertices
   for(const auto& block : sl->list_of_bloc)
   {
      inverse_vertex_map.insert(std::make_pair(block.first, bb_graphs_collection.AddVertex(BBNodeInfo(block.second))));
   }

   /// add edges
   for(const auto& curr_bb_pair : sl->list_of_bloc)
   {
      const auto curr_bbi = curr_bb_pair.first;
      const auto curr_bb = curr_bb_pair.second;
      for(const auto& lop : curr_bb->list_of_pred)
      {
         THROW_ASSERT(static_cast<bool>(inverse_vertex_map.count(lop)),
                      "BB" + STR(lop) + " (successor of BB" + STR(curr_bbi) + ") does not exist");
         bb_graphs_collection.AddEdge(inverse_vertex_map.at(lop), inverse_vertex_map.at(curr_bbi), CFG_SELECTOR);
      }

      for(const auto& los : curr_bb->list_of_succ)
      {
         if(los == bloc::EXIT_BLOCK_ID)
         {
            bb_graphs_collection.AddEdge(inverse_vertex_map.at(curr_bbi), inverse_vertex_map.at(los), CFG_SELECTOR);
         }
      }

      if(curr_bb->list_of_succ.empty())
      {
         bb_graphs_collection.AddEdge(inverse_vertex_map.at(curr_bbi), inverse_vertex_map.at(bloc::EXIT_BLOCK_ID),
                                      CFG_SELECTOR);
      }
   }
   /// add a connection between entry and exit thus avoiding problems with non terminating code
   bb_graphs_collection.AddEdge(inverse_vertex_map.at(bloc::ENTRY_BLOCK_ID), inverse_vertex_map.at(bloc::EXIT_BLOCK_ID),
                                CFG_SELECTOR);
   dominance<BBGraph> bb_dominators(cfg, inverse_vertex_map.at(bloc::ENTRY_BLOCK_ID),
                                    inverse_vertex_map.at(bloc::EXIT_BLOCK_ID));

   BBGraph dt(bb_graphs_collection, D_SELECTOR);
   bb_dominators.forEachDominanceRelation(
       [&](const BBGraph::vertex_descriptor child, const BBGraph::vertex_descriptor dom_vertex) {
          if(child != inverse_vertex_map.at(bloc::ENTRY_BLOCK_ID))
          {
             bb_graphs_collection.AddEdge(dom_vertex, child, D_SELECTOR);
          }
       });
   dt.GetGraphInfo().bb_index_map = std::move(inverse_vertex_map);

   std::list<BBGraph::vertex_descriptor> v_topological;
   dt.TopologicalSort(v_topological);
   THROW_ASSERT(v_topological.size(), "");
   std::vector<blocRef> bb_topological;
   std::transform(v_topological.begin(), v_topological.end(), std::back_inserter(bb_topological),
                  [&](BBGraph::vertex_descriptor v) { return dt.CGetNodeInfo(v).block; });
   return bb_topological;
}

std::vector<OMPInfoRef> OMPCGExt::GetOMPForkInfo(unsigned int fork_id)
{
   THROW_ASSERT(fork_infos.count(fork_id), "Required fork call id not present.");
   return fork_infos.at(fork_id);
}

std::string OMPCGExt::OMPProcSuffix(unsigned int local_idx)
{
   return local_idx ? "_" + STR(local_idx) : "";
}

std::string OMPCGExt::OMPForkSuffix(unsigned int fork_call_id, unsigned int local_idx)
{
   return "_" + STR(fork_call_id) + OMPProcSuffix(local_idx);
}
