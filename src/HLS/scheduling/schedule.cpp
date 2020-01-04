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
 * @file schedule.cpp
 * @brief Class implementation of the schedule data structure.
 *
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 * @author Marco Lattuada <marco.lattuada@polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *
 */
#include "schedule.hpp"

#include <boost/filesystem/operations.hpp>
#include <ostream>

#include "behavioral_helper.hpp"
#include "behavioral_writer_helper.hpp"
#include "fu_binding.hpp"
#include "function_behavior.hpp"
#include "op_graph.hpp"

#include "Parameter.hpp"

/// frontend_analysis include
#include "function_frontend_flow_step.hpp"

/// hls includes
#include "hls.hpp"
#include "hls_constraints.hpp"
#include "hls_manager.hpp"
#include "hls_target.hpp"

/// hls/module_allocation
#include "allocation_information.hpp"

/// hls/stg include
#include "state_transition_graph.hpp"

/// STL include
#include <set>

/// technology include
#include "technology_manager.hpp"

/// tree include
#include "dbgPrintHelper.hpp" // for DEBUG_LEVEL_
#include "ext_tree_node.hpp"
#include "string_manipulation.hpp" // for GET_CLASS
#include "tree_basic_block.hpp"
#include "tree_helper.hpp"
#include "tree_manager.hpp"
#include "tree_manipulation.hpp"
#include "tree_node.hpp"
#include "tree_reindex.hpp"

AbsControlStep::AbsControlStep() : std::pair<unsigned int, ControlStep>(0, AbsControlStep::UNKNOWN)
{
}

AbsControlStep::AbsControlStep(const unsigned int basic_block_index, const ControlStep control_step) : std::pair<unsigned int, ControlStep>(basic_block_index, control_step)
{
}

const ControlStep AbsControlStep::UNKNOWN = ControlStep(std::numeric_limits<unsigned int>::max());

bool AbsControlStep::operator<(const AbsControlStep& other) const
{
   if(this->second != other.second)
      return this->second < other.second;
   return this->first < other.first;
}

Schedule::Schedule(const HLS_managerConstRef _hls_manager, const unsigned int _function_index, const OpGraphConstRef _op_graph, const ParameterConstRef _parameters)
    : hls_manager(_hls_manager),
      TM(_hls_manager->get_tree_manager()),
      tree_man(new tree_manipulation(TM, _parameters)),
      allocation_information(_hls_manager->get_HLS(_function_index)->allocation_information),
      function_index(_function_index),
      tot_csteps(0),
      op_graph(_op_graph),
      parameters(_parameters),
      debug_level(_parameters->get_class_debug_level(GET_CLASS(*this)))
{
}

Schedule::~Schedule() = default;

void Schedule::print(fu_bindingRef Rfu) const
{
   const OpGraphConstRef g = op_graph;
   std::map<AbsControlStep, OpVertexSet> csteps_partitions;
   VertexIterator sch_i, sch_end;
   for(boost::tie(sch_i, sch_end) = boost::vertices(*g); sch_i != sch_end; sch_i++)
   {
      const auto control_step = get_cstep(*sch_i);
      if(csteps_partitions.find(control_step) == csteps_partitions.end())
         csteps_partitions.insert(std::make_pair(control_step, OpVertexSet(g)));
      auto& control_step_ops = csteps_partitions.at(control_step);
      control_step_ops.insert(*sch_i);
   }
   for(const auto& control_step : csteps_partitions)
   {
      for(const auto op : control_step.second)
      {
         INDENT_OUT_MEX(0, 0,
                        "---Operation " + GET_NAME(g, op) + "(" + g->CGetOpNodeInfo(op)->GetOperation() + ")" + " scheduled at control step (" + STR(get_cstep(op).second) + "-" + STR(get_cstep_end(op).second) + ") " +
                            (Rfu ? ("on functional unit " + Rfu->get_fu_name(op)) : ""));
      }
   }
}

class ScheduleWriter : public GraphWriter
{
 private:
   /// The schedule to be printed
   const ScheduleConstRef sch;

 public:
   /**
    * Constructor
    * @param op_graph is the operation graph to be printed
    * @param _sch is the schedule
    */
   ScheduleWriter(const OpGraphConstRef op_graph, const ScheduleConstRef _sch) : GraphWriter(op_graph.get(), 0), sch(_sch)
   {
   }

   /**
    * Redifinition of operator()
    */
   void operator()(std::ostream& os) const override
   {
      os << "//Scheduling solution\n";
      os << "splines=polyline;\n";
      std::map<ControlStep, UnorderedSetStdStable<vertex>> inverse_relation;
      VertexIterator v, v_end;
      for(boost::tie(v, v_end) = boost::vertices(*printing_graph); v != v_end; v++)
      {
         inverse_relation[sch->get_cstep(*v).second].insert(*v);
      }
      for(ControlStep level = ControlStep(0u); level < sch->get_csteps(); ++level)
      {
         os << "//Control Step: " << level << std::endl;
         os << "CS" << level << " [style=plaintext]\n{rank=same; CS" << level << " ";
         for(const auto operation : inverse_relation[level])
         {
            os << boost::get(boost::vertex_index_t(), *printing_graph)[operation] << " ";
         }
         os << ";}\n";
      }
      for(ControlStep level = ControlStep(1u); level < sch->get_csteps(); ++level)
         os << "CS" << level - 1u << "-> CS" << level << ";\n";
   }
};

void Schedule::WriteDot(const std::string& file_name) const
{
   const BehavioralHelperConstRef helper = op_graph->CGetOpGraphInfo()->BH;
   std::string output_directory = parameters->getOption<std::string>(OPT_dot_directory) + "/" + helper->get_function_name() + "/";
   if(!boost::filesystem::exists(output_directory))
      boost::filesystem::create_directories(output_directory);
   const VertexWriterConstRef op_label_writer(new OpWriter(op_graph.get(), 0));
   const EdgeWriterConstRef op_edge_property_writer(new OpEdgeWriter(op_graph.get()));
   const GraphWriterConstRef graph_writer(new ScheduleWriter(op_graph, ScheduleConstRef(this, null_deleter())));
   op_graph->InternalWriteDot<const OpWriter, const OpEdgeWriter, const ScheduleWriter>(output_directory + file_name, op_label_writer, op_edge_property_writer, graph_writer);
}

void Schedule::set_execution(const vertex& op, ControlStep c_step)
{
   const auto operation_index = op_graph->CGetOpNodeInfo(op)->GetNodeId();
   if(op_starting_cycle.find(operation_index) == op_starting_cycle.end())
      op_starting_cycle.emplace(operation_index, c_step);
   else
      op_starting_cycle.at(operation_index) = c_step;
   starting_cycles_to_ops[c_step].insert(operation_index);
}

void Schedule::set_execution_end(const vertex& op, ControlStep c_step_end)
{
   const auto operation_index = op_graph->CGetOpNodeInfo(op)->GetNodeId();
   if(op_ending_cycle.find(operation_index) == op_ending_cycle.end())
      op_ending_cycle.emplace(operation_index, c_step_end);
   else
      op_ending_cycle.at(operation_index) = c_step_end;
}

bool Schedule::is_scheduled(const vertex& op) const
{
   const auto statement_index = op_graph->CGetOpNodeInfo(op)->GetNodeId();
   return is_scheduled(statement_index);
}

bool Schedule::is_scheduled(const unsigned int statement_index) const
{
   return op_starting_cycle.find(statement_index) != op_starting_cycle.end();
}

AbsControlStep Schedule::get_cstep(const vertex& op) const
{
   const auto operation_index = op_graph->CGetOpNodeInfo(op)->GetNodeId();
   THROW_ASSERT(is_scheduled(op), "Operation " + GET_NAME(op_graph, op) + " has not been scheduled");
   if(operation_index == ENTRY_ID)
      return AbsControlStep(BB_ENTRY, op_starting_cycle.at(operation_index));
   if(operation_index == EXIT_ID)
      return AbsControlStep(BB_EXIT, op_starting_cycle.at(operation_index));
   return AbsControlStep(GetPointer<const gimple_node>(TM->CGetTreeNode(operation_index))->bb_index, op_starting_cycle.at(operation_index));
}

AbsControlStep Schedule::get_cstep(const unsigned int operation_index) const
{
   THROW_ASSERT(op_starting_cycle.find(operation_index) != op_starting_cycle.end(), "Operation " + STR(operation_index) + " has not been scheduled");
   if(operation_index == ENTRY_ID)
      return AbsControlStep(BB_ENTRY, op_starting_cycle.at(operation_index));
   if(operation_index == EXIT_ID)
      return AbsControlStep(BB_EXIT, op_starting_cycle.at(operation_index));
   return AbsControlStep(GetPointer<const gimple_node>(TM->CGetTreeNode(operation_index))->bb_index, op_starting_cycle.at(operation_index));
}

AbsControlStep Schedule::get_cstep_end(const vertex& op) const
{
   const auto statement_index = op_graph->CGetOpNodeInfo(op)->GetNodeId();
   return get_cstep_end(statement_index);
}

AbsControlStep Schedule::get_cstep_end(const unsigned int statement_index) const
{
   THROW_ASSERT(is_scheduled(statement_index), "Operation " + STR(statement_index) + " has not been scheduled");
   THROW_ASSERT(op_ending_cycle.find(statement_index) != op_ending_cycle.end(), "Ending step not stored");
   if(statement_index == ENTRY_ID)
      return AbsControlStep(BB_ENTRY, op_ending_cycle.at(statement_index));
   if(statement_index == EXIT_ID)
      return AbsControlStep(BB_EXIT, op_ending_cycle.at(statement_index));
   return AbsControlStep(GetPointer<const gimple_node>(TM->CGetTreeNode(statement_index))->bb_index, op_ending_cycle.at(statement_index));
}

unsigned int Schedule::num_scheduled() const
{
   return static_cast<unsigned int>(op_starting_cycle.size());
}

void Schedule::clear()
{
   tot_csteps = ControlStep(0u);
   spec.clear();
   op_starting_cycle.clear();
   starting_cycles_to_ops.clear();
}

void Schedule::UpdateTime(const unsigned int operation_index, bool update_cs)
{
   if(operation_index == ENTRY_ID or operation_index == EXIT_ID)
   {
      return;
   }

   const auto current_starting_time = starting_times[operation_index];
   const auto current_ending_time = starting_times[operation_index];

   CustomOrderedSet<ssa_name*> rhs_ssa_uses;
   const auto tn = TM->get_tree_node_const(operation_index);
   const auto gn = GetPointer<const gimple_node>(tn);
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Computing ending time of new statement " + STR(gn->index) + ": " + gn->ToString());
   const auto hls = hls_manager.lock()->get_HLS(function_index);
   const auto clock_period = hls->HLS_C->get_clock_period() * hls->HLS_C->get_clock_period_resource_fraction();
   const auto margin = allocation_information->GetClockPeriodMargin();
   const auto curr_bb_index = gn->bb_index;
   THROW_ASSERT(curr_bb_index, "Basic block of " + gn->ToString() + " not set");
   auto starting_time = 0.0;

   /// The starting control step
   ControlStep starting_cs = ControlStep(0);
   if(not update_cs)
      starting_time = clock_period * from_strongtype_cast<double>(op_starting_cycle.at(operation_index));
   if(gn->get_kind() == gimple_assign_K)
   {
      tree_helper::compute_ssa_uses_rec_ptr(GetPointer<const gimple_assign>(tn)->op1, rhs_ssa_uses);
   }
   else if(gn->get_kind() == gimple_cond_K)
   {
      tree_helper::compute_ssa_uses_rec_ptr(GetPointer<const gimple_cond>(tn)->op0, rhs_ssa_uses);
   }
   else if(gn->get_kind() == gimple_multi_way_if_K)
   {
      const auto gmwi = GetPointer<const gimple_multi_way_if>(tn);
      for(auto cond : gmwi->list_of_cond)
         if(cond.first)
            tree_helper::compute_ssa_uses_rec_ptr(cond.first, rhs_ssa_uses);
   }
   else if(gn->get_kind() == gimple_phi_K or gn->get_kind() == gimple_nop_K or gn->get_kind() == gimple_label_K)
   {
   }
   else if(gn->get_kind() == gimple_return_K)
   {
      const auto gr = GetPointer<const gimple_return>(tn);
      if(gr->op)
         tree_helper::compute_ssa_uses_rec_ptr(gr->op, rhs_ssa_uses);
   }
   else if(gn->get_kind() == gimple_switch_K)
   {
      const auto gs = GetPointer<const gimple_switch>(tn);
      THROW_ASSERT(gs->op0, " Switch without operand");
      tree_helper::compute_ssa_uses_rec_ptr(gs->op0, rhs_ssa_uses);
   }
   else if(gn->get_kind() == gimple_call_K)
   {
      tree_helper::compute_ssa_uses_rec_ptr(tn, rhs_ssa_uses);
   }
   else if(gn->get_kind() == gimple_asm_K)
   {
      const auto ga = GetPointer<const gimple_asm>(tn);
      if(ga->in)
         tree_helper::compute_ssa_uses_rec_ptr(ga->in, rhs_ssa_uses);
      if(ga->clob)
         tree_helper::compute_ssa_uses_rec_ptr(ga->clob, rhs_ssa_uses);
   }
   else
   {
      THROW_UNREACHABLE("Compute new ending time of " + gn->ToString() + " (" + gn->get_kind_text() + ")");
   }
   for(const auto ssa_use : rhs_ssa_uses)
   {
      const auto def = ssa_use->CGetDefStmt();
      const auto def_gn = GetPointer<const gimple_node>(GET_NODE(def));
      if(def_gn->get_kind() == gimple_nop_K)
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Parameter");
         continue;
      }
      if(def_gn->get_kind() == gimple_assign_K and GetPointer<const gimple_assign>(GET_NODE(def))->clobber)
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Clobber");
         continue;
      }
      THROW_ASSERT(def_gn->bb_index, "Basic block of " + def_gn->ToString() + " which defines " + ssa_use->ToString() + " not set");
      if(def_gn->bb_index != curr_bb_index)
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Definition " + STR(def->index) + " - " + def->ToString() + " is in BB" + STR(curr_bb_index));
         continue;
      }
      if(ssa_use->virtual_flag and ending_times.find(def_gn->index) == ending_times.end())
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Definition " + STR(def->index) + " - " + def->ToString() + " not yet examined --> anti dependence?");
         continue;
      }
      if(!allocation_information->is_operation_bounded(def->index))
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Defined by an unbounded statement");
         continue;
      }
      THROW_ASSERT(ending_times.find(def_gn->index) != ending_times.end(), "Not possible because ending time of " + def_gn->ToString() + " (which defines " + ssa_use->ToString() + ") is unknown");
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Definition " + STR(def->index) + " - " + def->ToString() + " ends at " + STR(ending_times.at(def_gn->index)));

      /// Compute the control step
      if(get_cstep_end(def_gn->index).second > starting_cs)
         starting_cs = get_cstep_end(def_gn->index).second;

      /// Compute the remaining time
      const auto ending_time = ending_times.at(def_gn->index);
      const auto connection_time = allocation_information->GetConnectionTime(def_gn->index, operation_index, AbsControlStep(gn->bb_index, AbsControlStep::UNKNOWN));
      if(ending_time + connection_time >= starting_time)
         starting_time = ending_time + connection_time;
   }
   starting_times[operation_index] = starting_time;
   const auto cycles = allocation_information->GetCycleLatency(gn->index);
   if(cycles > 1)
   {
      /// Check if the first stage is across states; if so move a the beginning of the next state
      const auto first_stage_latency = allocation_information->is_operation_PI_registered(gn->index) ? 0.0 : allocation_information->GetTimeLatency(gn->index, fu_binding::UNKNOWN).second;
      auto first_stage_ending = starting_time + first_stage_latency;
      if(first_stage_ending != 0.0 and (floor((first_stage_ending + margin) / clock_period) != floor((starting_time) / clock_period)))
      {
         first_stage_ending = ((ceil((first_stage_ending + margin) / clock_period) + 1) * clock_period);
      }
      else
      {
         first_stage_ending = ((ceil((first_stage_ending + margin) / clock_period) * clock_period));
      }
      const auto other_cycles_latency = (cycles - 2) * clock_period + allocation_information->GetTimeLatency(gn->index, fu_binding::UNKNOWN, cycles - 1).second;
      ending_times[operation_index] = first_stage_ending + other_cycles_latency;
   }
   else
   {
      const auto latency = allocation_information->GetTimeLatency(gn->index, fu_binding::UNKNOWN).first;
      ending_times[operation_index] = starting_time + latency;
      /// Checking if we are crossing a state
      if(floor((ending_times[operation_index] + allocation_information->GetConnectionTime(operation_index, 0, AbsControlStep(gn->index, AbsControlStep::UNKNOWN)) + margin) / clock_period) != floor(starting_times[operation_index] / clock_period))
      {
         starting_times[operation_index] = ceil(starting_times[operation_index] / clock_period) * clock_period;
         ending_times[operation_index] = starting_times[operation_index] + latency;
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Fixed ending time is " + STR(ending_times[operation_index]));
      }
   }

   /// True if the execution time of this operation has been updated
   const bool updated_time = starting_times[operation_index] != current_starting_time or starting_times[operation_index] != current_ending_time;
   if(updated_time)
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                     "---Execution time updated from [" + STR(current_starting_time) + "---" + STR(current_ending_time) + "] to [" + STR(starting_times[operation_index]) + "---" + STR(ending_times[operation_index]) + "]");
   }

   if(update_cs)
   {
      /// Remove the old scheduling
      if(is_scheduled(operation_index))
         starting_cycles_to_ops[op_starting_cycle.at(operation_index)].erase(operation_index);
      auto valS = ControlStep(static_cast<unsigned int>(floor(starting_times[operation_index] / clock_period)));
      if(op_starting_cycle.find(operation_index) == op_starting_cycle.end())
         op_starting_cycle.emplace(operation_index, valS);
      else
         op_starting_cycle.at(operation_index) = valS;
      auto valE = ControlStep(static_cast<unsigned int>(floor(ending_times[operation_index] / clock_period)));
      if(op_ending_cycle.find(operation_index) == op_ending_cycle.end())
         op_ending_cycle.emplace(operation_index, valE);
      else
         op_ending_cycle.at(operation_index) = valE;
   }
   if(allocation_information->IsVariableExecutionTime(operation_index) and updated_time)
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Checking if simultaneous operations have to be rescheduled");
      const auto reschedule = [&]() -> bool {
         if(not is_scheduled(operation_index))
            return false;
         for(const auto simultaneous_operation : starting_cycles_to_ops[op_starting_cycle.at(operation_index)])
         {
            if(allocation_information->IsVariableExecutionTime(simultaneous_operation))
            {
               return true;
            }
         }
         return false;
      }();
      /// Reschedule operation according the order of control flow graph; in this way we are sure that topological order is followed;
      /// Skip operations prevoius to this control step since they have not to be rescheduled
      if(reschedule)
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Operations have to be rescheduled");
         const auto current_cs = op_starting_cycle.at(operation_index);
         const auto fd = GetPointer<const function_decl>(TM->CGetTreeNode(function_index));
         const auto sl = GetPointer<const statement_list>(GET_NODE(fd->body));
         for(const auto& stmt : sl->list_of_bloc.at(gn->bb_index)->CGetStmtList())
         {
            if(op_starting_cycle.at(stmt->index) >= current_cs and stmt->index != operation_index and starting_times.find(stmt->index) != starting_times.end())
            {
               UpdateTime(stmt->index, true);
            }
         }
      }
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Checked if simultaneous operations have to be rescheduled");
   }
   /// Performed in this point to avoid check in the previous cycle
   starting_cycles_to_ops[op_starting_cycle.at(operation_index)].insert(operation_index);
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                  "<--Scheduling is " + STR(starting_times.at(operation_index)) + "-" + STR(ending_times.at(operation_index)) + " Control steps: " + STR(op_starting_cycle.at(operation_index)) + "-" + STR(op_ending_cycle.at(operation_index)));
}

FunctionFrontendFlowStep_Movable Schedule::CanBeMoved(const unsigned int statement_index, const unsigned int basic_block_index) const
{
   THROW_ASSERT(basic_block_index, "Trying to move " + TM->get_tree_node_const(statement_index)->ToString() + " to BB0");
   const auto hls = hls_manager.lock()->get_HLS(function_index);
   const FunctionBehaviorConstRef FB = hls_manager.lock()->CGetFunctionBehavior(hls->functionId);
   const auto behavioral_helper = FB->CGetBehavioralHelper();
   if(not behavioral_helper->CanBeMoved(statement_index))
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--No because it is artifical and cannot be moved by default");
      return FunctionFrontendFlowStep_Movable::UNMOVABLE;
   }
   const auto clock_period = hls->HLS_C->get_clock_period() * hls->HLS_C->get_clock_period_resource_fraction();
   const auto clock_period_margin = allocation_information->GetClockPeriodMargin();
   double bb_ending_time = GetBBEndingTime(basic_block_index);
   auto current_cs = ControlStep(static_cast<unsigned int>(floor(bb_ending_time / clock_period)));
   const auto ga = GetPointer<const gimple_assign>(TM->get_tree_node_const(statement_index));
   THROW_ASSERT(ga, "Asking if " + TM->get_tree_node_const(statement_index)->ToString() + " can be moved");
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Checking if " + ga->ToString() + " can be moved");
   if(behavioral_helper->IsLoad(statement_index))
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--No because it is a load");
      return FunctionFrontendFlowStep_Movable::UNMOVABLE;
   }

   const bool unbounded_operation = not allocation_information->is_operation_bounded(statement_index);
   if(unbounded_operation)
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--No because it is unbounded");
      return FunctionFrontendFlowStep_Movable::UNMOVABLE;
   }
   const auto cycles = allocation_information->GetCycleLatency(ga->index);
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Cycles: " + STR(cycles));
   if(cycles > 1)
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--No because it is multicycle");
      return FunctionFrontendFlowStep_Movable::UNMOVABLE;
   }
   const auto latency = allocation_information->GetTimeLatency(ga->index, fu_binding::UNKNOWN).first;
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Latency: " + STR(latency));

   double new_ending_time = 0.0;
   CustomOrderedSet<ssa_name*> rhs_ssa_uses;
   tree_helper::compute_ssa_uses_rec_ptr(ga->op1, rhs_ssa_uses);
   for(const auto ssa_use : rhs_ssa_uses)
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Considering used ssa " + STR(ssa_use->index) + " - " + ssa_use->ToString());
      const auto def = GET_NODE(ssa_use->CGetDefStmt());
      const auto gn = GetPointer<const gimple_node>(def);
      if(gn->bb_index != basic_block_index)
      {
         /// Moved in a basic block after the definition - there cannot be problem of chaining
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Defined in a different basic block from the current (BB" + STR(gn->bb_index) + " BB" + STR(basic_block_index) + ")");
         continue;
      }
      THROW_ASSERT(ending_times.find(gn->index) != ending_times.end(), "Ending time of " + gn->ToString() + " is not available");
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Definition " + STR(def->index) + " - " + def->ToString() + " ends at " + STR(ending_times.at(gn->index)));
      if(!allocation_information->is_operation_bounded(def->index))
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Defined by an unbounded statement");
         continue;
      }
      /// Compute the remaining time
      const auto ending_time = ending_times.at(gn->index) + allocation_information->GetConnectionTime(gn->index, statement_index, AbsControlStep(basic_block_index, current_cs));
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Ending time + connection delay of predecessor is " + STR(ending_time));
      const auto operation_margin = (ceil(ending_time / clock_period) * clock_period) - ending_time - clock_period_margin;
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Connection time to phi is " + STR(allocation_information->GetConnectionTime(statement_index, 0, AbsControlStep(basic_block_index, current_cs))));
      if(operation_margin > latency + allocation_information->GetConnectionTime(statement_index, 0, AbsControlStep(basic_block_index, current_cs)))
      {
         if(ending_time + latency > new_ending_time)
            new_ending_time = ending_time + latency;
         continue;
      }
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                     "---Ending time of this operation: " + STR((ceil(ending_time / clock_period) * clock_period)) + " + " + STR(latency) + " + " + STR(clock_period) + " = " +
                         STR((ceil(ending_time / clock_period) * clock_period) + latency + clock_period_margin));
      /// Operation is scheduled at the beginning of the next control step
      if(((ceil(ending_time / clock_period) * clock_period) + latency + clock_period_margin) > new_ending_time)
      {
         new_ending_time = ((ceil(ending_time / clock_period) * clock_period) + latency);
      }
      /// Check if this operation ends not in the last control step
      if(ceil(bb_ending_time / clock_period) * clock_period < (ceil(new_ending_time / clock_period) * clock_period))
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--No because it can increase the latency of a previus BB: " + STR(bb_ending_time) + " vs. " + STR(new_ending_time));
         return FunctionFrontendFlowStep_Movable::TIMING;
      }
   }
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Yes because of chaining - New ending time is " + STR(new_ending_time));
   return FunctionFrontendFlowStep_Movable::MOVABLE;
}

#if 0
bool Schedule::CanBeChained(const vertex first_statement, const vertex second_statement) const
{
   const auto first_statement_index = op_graph->CGetOpNodeInfo(first_statement)->GetNodeId();
   const auto second_statement_index = op_graph->CGetOpNodeInfo(second_statement)->GetNodeId();
   const auto ret = CanBeChained(first_statement_index, second_statement_index);
   return ret;
}

bool Schedule::CanBeChained(const unsigned int first_statement_index, const unsigned int second_statement_index) const
{
   if(first_statement_index == ENTRY_ID or first_statement_index == EXIT_ID or second_statement_index == ENTRY_ID or second_statement_index == EXIT_ID)
   {
      return true;
   }
   const auto first_tree_node = TM->CGetTreeNode(first_statement_index);
   const auto second_tree_node = TM->CGetTreeNode(second_statement_index);
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Checking if (" + STR(second_statement_index) + ") " + STR(second_tree_node) + " can be chained with (" + STR(first_statement_index) + ") " + STR(first_tree_node));
   const auto first_operation = GetPointer<const gimple_node>(first_tree_node)->operation;
   const auto second_operation = GetPointer<const gimple_node>(second_tree_node)->operation;
   const auto hls = hls_manager.lock()->get_HLS(function_index);
   const auto behavioral_helper = hls->FB->CGetBehavioralHelper();
   if(behavioral_helper->IsStore(first_statement_index))
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--No because first is a store");
      return false;
   }
   else if(not allocation_information->is_operation_bounded(first_statement_index, allocation_information->GetFuType(first_statement_index)))
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--No because first is unbounded");
      return false;
   }
   ///Load/Store from distributed memory cannot be chained
   else if((behavioral_helper->IsLoad(second_statement_index) or behavioral_helper->IsStore(second_statement_index)) and allocation_information->is_one_cycle_direct_access_memory_unit(allocation_information->GetFuType(second_statement_index)))
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--No because second is a load from distributed memory");
      return false;
   }
   ///STORE cannot be executed in the same clock cycle of the condition which controls it
   else if ((first_tree_node->get_kind() == gimple_cond_K or first_tree_node->get_kind() == gimple_multi_way_if_K) and behavioral_helper->IsStore(second_statement_index))
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--No because stores cannot be executed in the same clock cycle of the condition which controls it");
      return false;
   }
   ///UNBOUNDED operations cannot be executed in the same clock cycle of the condition which controls it
   else if ((first_tree_node->get_kind() == gimple_cond_K or first_tree_node->get_kind() == gimple_multi_way_if_K) and not allocation_information->is_operation_bounded(second_statement_index, allocation_information->GetFuType(second_statement_index)))
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--No because unbounded operations cannot be executed in the same clock cycle of the condition which controls it");
      return false;
   }
   ///labels cannot be executed in the same clock cycle of the condition which controls it
   else if ((first_tree_node->get_kind() == gimple_cond_K or first_tree_node->get_kind() == gimple_multi_way_if_K) and (second_tree_node->get_kind() == gimple_label_K))
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--No because labels and nops cannot be executed in the same clock cycle of the condition which controls it");
      return false;
   }
   ///Operations with side effect cannot be executed in the same clock cycle of the control_step which controls them
   else if ((first_tree_node->get_kind() == gimple_cond_K or first_tree_node->get_kind() == gimple_multi_way_if_K) and (GetPointer<const gimple_node>(second_tree_node)->vdef))
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--No because operations with side effect cannot be executed in the same clock cycle of the condition which controls it");
      return false;
   }
   ///Load from bus cannot be chained with READ_COND and MULTI_READ_COND
   /*else if((((GET_TYPE(op_graph, other) & (TYPE_STORE | TYPE_LOAD)) != 0) and not allocation_information->is_direct_access_memory_unit(allocation_information->GetFuType(other))) and ((GET_TYPE(op_graph, current) & (TYPE_IF | TYPE_MULTIIF)) != 0))
     {
     constraint_to_be_added = true;
     }*/
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Yes");
   return true;
}
#endif

bool Schedule::EvaluateCondsMerging(const unsigned statement_index, const unsigned int first_condition, const unsigned second_condition) const
{
   if(not first_condition or not second_condition)
      return true;
   if(TM->CGetTreeNode(first_condition)->get_kind() == integer_cst_K or TM->CGetTreeNode(second_condition)->get_kind() == integer_cst_K)
      return true;
   const auto statement = GetPointer<const gimple_node>(TM->get_tree_node_const(statement_index));
   const auto or_result = tree_man->CreateOrExpr(TM->GetTreeReindex(first_condition), TM->GetTreeReindex(second_condition), blocRef());
   const auto or_ending_time = std::max(GetReadyTime(first_condition, statement->bb_index), GetReadyTime(second_condition, statement->bb_index)) +
                               allocation_information->GetTimeLatency(GetPointer<const ssa_name>(GET_NODE(or_result))->CGetDefStmt()->index, fu_binding::UNKNOWN).first;
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Checking if merging of conditions can be put at then end BB" + STR(statement->bb_index) + " (ending with " + statement->ToString());
   return or_ending_time < GetBBEndingTime(statement->bb_index);
}

bool Schedule::EvaluateMultiWayIfsMerging(const unsigned int first_statement_index, const unsigned int second_statement_index) const
{
   const auto hls = hls_manager.lock()->get_HLS(function_index);
   const auto basic_block_graph = hls_manager.lock()->CGetFunctionBehavior(function_index)->CGetBBGraph(FunctionBehavior::FBB);
   const auto list_of_block = GetPointer<statement_list>(GET_NODE(GetPointer<const function_decl>(TM->get_tree_node_const(function_index))->body))->list_of_bloc;
   const auto first_statement = TM->get_tree_node_const(first_statement_index);
   const auto second_statement = TM->get_tree_node_const(second_statement_index);
   const auto first_basic_block = GetPointer<const gimple_node>(first_statement)->bb_index;
   const auto first_block = list_of_block.at(first_basic_block);
   const auto second_basic_block = GetPointer<const gimple_node>(second_statement)->bb_index;
   /// Note that we compute ending times only for the longest newly created condition; existing condition can be worse
   const auto new_ending_time = [=]() -> double {
      if(first_statement->get_kind() == gimple_cond_K and second_statement->get_kind() == gimple_cond_K)
      {
         const auto first_gc = GetPointer<const gimple_cond>(first_statement);
         const auto first_gc_op = GET_NODE(first_gc->op0);
         THROW_ASSERT(first_gc_op->get_kind() == ssa_name_K, "Condition of the first gimple cond is " + first_gc_op->ToString());
         const auto first_gc_input_delay = GetReadyTime(first_gc_op->index, first_gc->bb_index);
         const auto second_gc = GetPointer<const gimple_cond>(second_statement);
         const auto second_gc_op = GET_NODE(second_gc->op0);
         THROW_ASSERT(second_gc_op->get_kind() == ssa_name_K, "Condition of the first gimple cond is " + second_gc_op->ToString());
         const auto second_gc_input_delay = GetReadyTime(second_gc_op->index, second_gc->bb_index);
         const auto second_block = list_of_block.at(second_basic_block);
         if(first_block->true_edge == second_basic_block)
         {
            const auto not_operation = tree_man->CreateNotExpr(second_gc->op0, blocRef());
            const auto not_ending_time = second_gc_input_delay + allocation_information->GetTimeLatency(not_operation->index, fu_binding::UNKNOWN).first;
            const auto and_operation = tree_man->CreateAndExpr(not_operation, first_gc->op0, blocRef());
            const auto and_ending_time = std::max(not_ending_time, first_gc_input_delay) + allocation_information->GetTimeLatency(and_operation->index, fu_binding::UNKNOWN).first;
            return and_ending_time;
         }
         else if(first_block->false_edge == second_basic_block)
         {
            const auto not_operation = tree_man->CreateNotExpr(first_gc->op0, blocRef());
            const auto not_ending_time = first_gc_input_delay + allocation_information->GetTimeLatency(not_operation->index, fu_binding::UNKNOWN).first;
            const auto and_operation = tree_man->CreateAndExpr(second_gc_op, not_operation, blocRef());
            const auto and_ending_time = std::max(not_ending_time, second_gc_input_delay) + allocation_information->GetTimeLatency(and_operation->index, fu_binding::UNKNOWN).first;
            return and_ending_time;
         }
         else
         {
            THROW_UNREACHABLE("BB" + STR(second_basic_block) + " is not on the true edge nor on the false edge of BB" + STR(first_basic_block));
         }
      }
      else if(first_statement->get_kind() == gimple_cond_K and second_statement->get_kind() == gimple_multi_way_if_K)
      {
         const auto first_gc = GetPointer<const gimple_cond>(first_statement);
         const auto second_gmwi = GetPointer<const gimple_multi_way_if>(second_statement);
         if(first_block->true_edge == second_basic_block)
         {
            const auto first_gc_op = GET_NODE(first_gc->op0);
            THROW_ASSERT(first_gc_op->get_kind() == ssa_name_K, "Condition of the first gimple cond is " + first_gc_op->ToString());
            const auto first_gc_input_delay = GetReadyTime(first_gc_op->index, first_gc->bb_index);
            auto current_condition = first_gc_op;
            auto current_ending_time = first_gc_input_delay;
            for(const auto& cond : second_gmwi->list_of_cond)
            {
               if(cond.first)
               {
                  const auto not_operation = tree_man->CreateNotExpr(cond.first, blocRef());
                  const auto cond_delay = GetReadyTime(cond.first->index, first_gc->bb_index);
                  const auto not_ending_time = cond_delay + allocation_information->GetTimeLatency(not_operation->index, fu_binding::UNKNOWN).first;
                  const auto and_operation = tree_man->CreateAndExpr(GetPointer<const gimple_assign>(GET_NODE(current_condition))->op0, GetPointer<const gimple_assign>(GET_NODE(not_operation))->op0, blocRef());
                  current_condition = and_operation;
                  current_ending_time = std::max(not_ending_time, current_ending_time) + allocation_information->GetTimeLatency(and_operation->index, fu_binding::UNKNOWN).first;
               }
            }
            return current_ending_time;
         }
         else if(first_block->false_edge == second_basic_block)
         {
            const auto first_gc_op = GET_NODE(first_gc->op0);
            THROW_ASSERT(first_gc_op->get_kind() == ssa_name_K, "Condition of the first gimple cond is " + first_gc_op->ToString());
            auto current_ending_time = 0.0;
            const auto not_operation = tree_man->CreateNotExpr(first_gc_op, blocRef());
            const auto not_ending_time = GetReadyTime(first_gc_op->index, first_basic_block) + allocation_information->GetTimeLatency(not_operation->index, fu_binding::UNKNOWN).first;
            for(const auto& cond : second_gmwi->list_of_cond)
            {
               const auto and_operation = tree_man->CreateAndExpr(GetPointer<const gimple_assign>(GET_NODE(not_operation))->op0, cond.first, blocRef());
               const auto and_ending_time = std::max(not_ending_time, GetReadyTime(cond.first->index, first_basic_block)) + allocation_information->GetTimeLatency(and_operation->index, fu_binding::UNKNOWN).first;
               current_ending_time = std::max(current_ending_time, and_ending_time);
            }
            return current_ending_time;
         }
         else
         {
            THROW_UNREACHABLE("BB" + STR(second_basic_block) + " is not on the true edge nor on the false edge of BB" + STR(first_basic_block));
         }
      }
      else if(first_statement->get_kind() == gimple_multi_way_if_K and second_statement->get_kind() == gimple_cond_K)
      {
         const auto first_gmwi = GetPointer<const gimple_multi_way_if>(first_statement);
         const auto second_gc = GetPointer<const gimple_cond>(second_statement);
         const auto second_gc_op = GET_NODE(second_gc->op0);

         /// The basic block on the default cond edge
         const auto default_basic_block = first_gmwi->list_of_cond.back().second;
         if(default_basic_block != second_basic_block)
         {
            for(const auto& cond : first_gmwi->list_of_cond)
            {
               if(cond.second == second_basic_block)
               {
                  auto const not_operation = tree_man->CreateNotExpr(second_gc_op, blocRef());
                  auto const not_ending_time = GetReadyTime(second_gc_op->index, first_basic_block) + allocation_information->GetTimeLatency(not_operation->index, fu_binding::UNKNOWN).first;
                  const auto and_operation = tree_man->CreateAndExpr(GetPointer<const gimple_assign>(GET_NODE(not_operation))->op0, cond.first, blocRef());
                  const auto and_ending_time = std::max(not_ending_time, GetReadyTime(cond.first->index, first_basic_block)) + allocation_information->GetTimeLatency(and_operation->index, fu_binding::UNKNOWN).first;
                  return and_ending_time;
               }
            }
            THROW_UNREACHABLE("");
         }
         else
         {
            auto current_condition = tree_nodeRef();
            auto current_ending_time = 0.0;
            for(const auto& cond : first_gmwi->list_of_cond)
            {
               if(cond.first)
               {
                  auto const not_operation = tree_man->CreateNotExpr(cond.first, blocRef());
                  const auto not_ending_time = GetReadyTime(cond.first->index, first_basic_block) + allocation_information->GetTimeLatency(not_operation->index, fu_binding::UNKNOWN).first;
                  const auto and_operation = current_condition ? tree_man->CreateAndExpr(current_condition, not_operation, blocRef()) : not_operation;
                  const auto and_ending_time = current_condition ? (std::max(not_ending_time, current_ending_time) + allocation_information->GetTimeLatency(and_operation->index, fu_binding::UNKNOWN).first) : not_ending_time;
                  current_condition = and_operation;
                  current_ending_time = and_ending_time;
               }
            }
            const auto and_operation = tree_man->CreateAndExpr(current_condition, second_gc_op, blocRef());
            const auto and_ending_time = std::max(current_ending_time, GetReadyTime(second_gc_op->index, first_basic_block)) + allocation_information->GetTimeLatency(and_operation->index, fu_binding::UNKNOWN).first;
            return and_ending_time;
         }
      }
      else if(first_statement->get_kind() == gimple_multi_way_if_K and second_statement->get_kind() == gimple_multi_way_if_K)
      {
         const auto first_gmwi = GetPointer<const gimple_multi_way_if>(first_statement);
         const auto second_gmwi = GetPointer<const gimple_multi_way_if>(second_statement);

         const auto default_basic_block = first_gmwi->list_of_cond.back().second;
         if(default_basic_block != second_basic_block)
         {
            for(const auto& first_cond : first_gmwi->list_of_cond)
            {
               if(first_cond.second == second_basic_block)
               {
                  auto current_condition = first_cond.first;
                  auto current_ending_time = GetReadyTime(first_cond.first->index, first_basic_block);
                  for(const auto& second_cond : second_gmwi->list_of_cond)
                  {
                     if(second_cond.first)
                     {
                        const auto not_operation = tree_man->CreateNotExpr(second_cond.first, blocRef());
                        const auto cond_delay = GetReadyTime(second_cond.first->index, first_basic_block);
                        const auto not_ending_time = cond_delay + allocation_information->GetTimeLatency(not_operation->index, fu_binding::UNKNOWN).first;
                        const auto and_operation = tree_man->CreateAndExpr(GetPointer<const gimple_assign>(GET_NODE(current_condition))->op0, GetPointer<const gimple_assign>(GET_NODE(not_operation))->op0, blocRef());
                        current_condition = and_operation;
                        current_ending_time = std::max(not_ending_time, current_ending_time) + allocation_information->GetTimeLatency(and_operation->index, fu_binding::UNKNOWN).first;
                     }
                  }
                  return current_ending_time;
               }
            }
            THROW_UNREACHABLE("");
         }
         else
         {
            auto current_condition = tree_nodeRef();
            auto current_ending_time = 0.0;
            for(const auto& cond : first_gmwi->list_of_cond)
            {
               if(cond.first)
               {
                  const auto not_operation = tree_man->CreateNotExpr(cond.first, blocRef());
                  const auto not_ending_time = GetReadyTime(cond.first->index, first_basic_block);
                  const auto and_operation = current_condition ? tree_man->CreateAndExpr(GetPointer<const gimple_assign>(GET_NODE(current_condition))->op0, cond.first, blocRef()) : not_operation;
                  const auto and_ending_time = current_condition ? std::max(not_ending_time, current_ending_time) + allocation_information->GetTimeLatency(and_operation->index, fu_binding::UNKNOWN).first : not_ending_time;
                  current_condition = and_operation;
                  current_ending_time = and_ending_time;
               }
            }
            for(const auto& cond : second_gmwi->list_of_cond)
            {
               if(cond.first)
               {
                  const auto and_operation = tree_man->CreateAndExpr(GetPointer<const gimple_assign>(GET_NODE(current_condition))->op0, cond.first, blocRef());
                  const auto and_ending_time = std::max(current_ending_time, GetReadyTime(cond.first->index, first_basic_block)) + allocation_information->GetTimeLatency(and_operation->index, fu_binding::UNKNOWN).first;
                  current_ending_time = std::max(and_ending_time, current_ending_time);
               }
            }
            return current_ending_time;
         }
      }
      else
      {
         THROW_UNREACHABLE("");
         return 0.0;
      }
      THROW_UNREACHABLE("");
      return 0.0;
   }();
   return new_ending_time < GetBBEndingTime(first_basic_block) ? true : false;
}

double Schedule::GetReadyTime(const unsigned int tree_node_index, const unsigned int basic_block_index) const
{
   const auto sn = GetPointer<const ssa_name>(TM->get_tree_node_const(tree_node_index));
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Computing ready time of " + sn->ToString());
   const auto def = GetPointer<gimple_node>(GET_NODE(sn->CGetDefStmt()));
   if(def->get_kind() == gimple_phi_K)
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--0.0");
      return 0.0;
   }
   if(def->get_kind() == gimple_assign_K)
   {
      if(def->bb_index != basic_block_index)
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--0.0");
         return 0.0;
      }
      THROW_ASSERT(ending_times.find(def->index) != ending_times.end(), "Ending time of " + def->ToString() + " not found");
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--" + STR(ending_times.at(def->index)));
      return ending_times.at(def->index);
   }
   THROW_UNREACHABLE("");
   return 0.0;
}

double Schedule::GetBBEndingTime(const unsigned int basic_block_index) const
{
   const auto hls = hls_manager.lock()->get_HLS(function_index);
   const auto clock_period = hls->HLS_C->get_clock_period() * hls->HLS_C->get_clock_period_resource_fraction();
   const auto margin = allocation_information->GetClockPeriodMargin();
   const auto tn = TM->get_tree_node_const(function_index);
   const auto fd = GetPointer<const function_decl>(tn);
   THROW_ASSERT(GetPointer<statement_list>(GET_NODE(fd->body))->list_of_bloc.find(basic_block_index) != GetPointer<statement_list>(GET_NODE(fd->body))->list_of_bloc.end(), "BB" + STR(basic_block_index) + " not found");
   const auto block = GetPointer<statement_list>(GET_NODE(fd->body))->list_of_bloc.at(basic_block_index);
   const auto stmt_list = block->CGetStmtList();
   if(stmt_list.size() == 0)
   {
      return 0.0;
   }
   const auto ending_time = std::max_element(stmt_list.begin(), stmt_list.end(), [=](const tree_nodeRef first, const tree_nodeRef second) { return ending_times.at(first->index) < ending_times.at(second->index); });
   THROW_ASSERT(ending_time != stmt_list.end(), "");
   return ceil((ending_times.at((*ending_time)->index) + margin) / clock_period) * clock_period;
}

double Schedule::GetEndingTime(const unsigned int operation_index) const
{
   if(operation_index == ENTRY_ID or operation_index == EXIT_ID)
      return 0.0;
   THROW_ASSERT(ending_times.find(operation_index) != ending_times.end(), "Ending time of operation " + STR(TM->CGetTreeNode(operation_index)) + " not found");
   return ending_times.at(operation_index);
}

double Schedule::GetStartingTime(const unsigned int operation_index) const
{
   if(operation_index == ENTRY_ID or operation_index == EXIT_ID)
      return 0.0;
   THROW_ASSERT(starting_times.find(operation_index) != starting_times.end(), "Starting time of operation " + STR(operation_index) + " not found");
   return starting_times.at(operation_index);
}

double Schedule::get_fo_correction(unsigned int first_operation, unsigned int second_operation) const
{
   const auto edge = std::pair<unsigned int, unsigned int>(first_operation, second_operation);
   if(connection_times.find(edge) != connection_times.end())
      return connection_times.at(edge);
   else
      return 0.0;
}

const std::string Schedule::PrintTimingInformation(const unsigned int statement_index) const
{
   return "[" + NumberToString(GetStartingTime(statement_index), 2, 7) + "---" + NumberToString(GetEndingTime(statement_index), 2, 7) + "(" + NumberToString(GetEndingTime(statement_index) - GetStartingTime(statement_index), 2, 7) + ")" + "]";
}

class StartingTimeSorter : std::binary_function<unsigned int, unsigned int, bool>
{
 protected:
   /// The starting time
   const CustomMap<unsigned int, double>& starting_times;

 public:
   /**
    * The constructor
    * @param starting_time is the starting time of each operation
    */
   explicit StartingTimeSorter(const CustomMap<unsigned int, double>& _starting_times) : starting_times(_starting_times)
   {
   }

   /**
    * Compare position of two operations
    * @param x is the index of the first operation
    * @param y is the index of the second operation
    * @return true if starting time of x is before starting time of y
    */
   bool operator()(const unsigned int x, const unsigned int y) const
   {
      if(starting_times.at(x) == starting_times.at(y))
      {
         return x < y;
      }
      else
      {
         return starting_times.at(x) < starting_times.at(y);
      }
   }
};

CustomSet<unsigned int> Schedule::ComputeCriticalPath(const StateInfoConstRef state_info) const
{
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Considering operations of state " + state_info->name);

   /// The set of operations which are in the critical path of the current basic block
   CustomSet<unsigned int> critical_paths;
   /// Computing ending time of state
   double ending_state_time = 0;
   for(const auto starting_operation : state_info->starting_operations)
   {
      const auto node_id = op_graph->CGetOpNodeInfo(starting_operation)->GetNodeId();
      if(node_id == ENTRY_ID or node_id == EXIT_ID)
         continue;
      const auto stmt = TM->get_tree_node_const(node_id);
      if(stmt->get_kind() == gimple_phi_K)
         continue;
      const bool found = std::find(state_info->ending_operations.begin(), state_info->ending_operations.end(), starting_operation) != state_info->ending_operations.end();
      const auto ending_time =
          found ? ending_times.at(stmt->index) : starting_times.at(stmt->index) + (allocation_information->is_operation_PI_registered(stmt->index) ? 0.0 : allocation_information->GetTimeLatency(stmt->index, fu_binding::UNKNOWN).second);
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Ending time of " + stmt->ToString() + " is " + STR(ending_time));
      if(ending_time > ending_state_time)
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Updating ending time to " + STR(ending_time) + " because of " + stmt->ToString());
         ending_state_time = ending_time;
      }
   }
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Ending time of state is " + STR(ending_state_time));

   std::set<unsigned int, StartingTimeSorter> to_be_processed = std::set<unsigned int, StartingTimeSorter>(StartingTimeSorter(starting_times));
   for(const auto starting_operation : state_info->starting_operations)
   {
      const auto node_id = op_graph->CGetOpNodeInfo(starting_operation)->GetNodeId();
      if(node_id == ENTRY_ID or node_id == EXIT_ID)
         continue;
      const auto stmt = TM->get_tree_node_const(node_id);
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Stmt " + stmt->ToString() + " ends at " + STR(ending_times.at(stmt->index)));
      const bool found = std::find(state_info->ending_operations.begin(), state_info->ending_operations.end(), starting_operation) != state_info->ending_operations.end();
      const auto ending_time =
          found ? ending_times.at(stmt->index) : starting_times.at(stmt->index) + (allocation_information->is_operation_PI_registered(stmt->index) ? 0.0 : allocation_information->GetTimeLatency(stmt->index, fu_binding::UNKNOWN).second);
      if(ending_time == ending_state_time)
      {
         to_be_processed.insert(stmt->index);
         critical_paths.insert(stmt->index);
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Adding " + stmt->ToString() + " to critical path");
      }
   }
   while(to_be_processed.size())
   {
      const auto last = *(to_be_processed.rbegin());
      const auto starting_time = starting_times.at(last);
      to_be_processed.erase(last);
      const auto stmt_tn = TM->get_tree_node_const(last);
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Processing " + stmt_tn->ToString());
      const auto gn = GetPointer<const gimple_node>(stmt_tn);
      CustomOrderedSet<ssa_name*> rhs_ssa_uses;
      if(gn->get_kind() == gimple_assign_K)
      {
         tree_helper::compute_ssa_uses_rec_ptr(GetPointer<const gimple_assign>(stmt_tn)->op1, rhs_ssa_uses);
      }
      else if(gn->get_kind() == gimple_cond_K)
      {
         tree_helper::compute_ssa_uses_rec_ptr(GetPointer<const gimple_cond>(stmt_tn)->op0, rhs_ssa_uses);
      }
      else if(gn->get_kind() == gimple_multi_way_if_K)
      {
         const auto gmwi = GetPointer<const gimple_multi_way_if>(stmt_tn);
         for(auto cond : gmwi->list_of_cond)
            if(cond.first)
               tree_helper::compute_ssa_uses_rec_ptr(cond.first, rhs_ssa_uses);
      }
      else if(gn->get_kind() == gimple_phi_K or gn->get_kind() == gimple_nop_K or gn->get_kind() == gimple_label_K)
      {
      }
      else if(gn->get_kind() == gimple_return_K)
      {
         const auto gr = GetPointer<const gimple_return>(stmt_tn);
         if(gr->op)
            tree_helper::compute_ssa_uses_rec_ptr(gr->op, rhs_ssa_uses);
      }
      else if(gn->get_kind() == gimple_switch_K)
      {
         const auto gs = GetPointer<const gimple_switch>(stmt_tn);
         THROW_ASSERT(gs->op0, " Switch without operand");
         tree_helper::compute_ssa_uses_rec_ptr(gs->op0, rhs_ssa_uses);
      }
      else if(gn->get_kind() == gimple_call_K)
      {
         tree_helper::compute_ssa_uses_rec_ptr(stmt_tn, rhs_ssa_uses);
      }
      else if(gn->get_kind() == gimple_asm_K)
      {
         tree_helper::compute_ssa_uses_rec_ptr(stmt_tn, rhs_ssa_uses);
      }
      else
      {
         THROW_UNREACHABLE("Computing critical path analyzing " + gn->ToString() + " (" + gn->get_kind_text() + ")");
      }
      for(const auto ssa_use : rhs_ssa_uses)
      {
         const auto def = GET_NODE(ssa_use->CGetDefStmt());
         const auto def_gn = GetPointer<const gimple_node>(def);
         if(def_gn->get_kind() == gimple_nop_K)
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Parameter");
            continue;
         }
         if(def_gn->get_kind() == gimple_pragma_K)
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Pragma");
            continue;
         }
         if(GetPointer<const gimple_assign>(def) and GetPointer<const gimple_assign>(def)->clobber)
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Clobber");
            continue;
         }
         THROW_ASSERT(def_gn->bb_index, "Basic block of " + def_gn->ToString() + " which defines " + ssa_use->ToString() + " not set");
         if(state_info->BB_ids.find(def_gn->bb_index) == state_info->BB_ids.end())
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Definition " + STR(def->index) + " - " + def->ToString() + " is in other state");
            continue;
         }
         if(ssa_use->virtual_flag and ending_times.find(def_gn->index) == ending_times.end())
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Definition " + STR(def->index) + " - " + def->ToString() + " not yet examined --> anti dependence?");
            continue;
         }
         THROW_ASSERT(ending_times.find(def_gn->index) != ending_times.end(), "Not possible because ending time of " + def_gn->ToString() + " (which defines " + ssa_use->ToString() + ") is unknown");
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Definition " + STR(def->index) + " - " + def->ToString() + " ends at " + STR(ending_times.at(def_gn->index)));
         /// Compute the remaining time
         const auto ending_time = ending_times.at(def_gn->index);
         if(ending_time + allocation_information->GetConnectionTime(def_gn->index, last, AbsControlStep(GetPointer<const gimple_node>(stmt_tn)->bb_index, op_starting_cycle.at(stmt_tn->index))) == starting_time)
         {
            to_be_processed.insert(def_gn->index);
            critical_paths.insert(def_gn->index);
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Adding " + def_gn->ToString() + " to critical path");
         }
      }
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Processed " + stmt_tn->ToString());
   }
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Computed critical path in state " + state_info->name);
   return critical_paths;
}

void Schedule::Initialize()
{
   TM = hls_manager.lock()->get_tree_manager();
   allocation_information = hls_manager.lock()->get_HLS(function_index)->allocation_information;
   tot_csteps = ControlStep(0);
   op_starting_cycle.clear();
   op_ending_cycle.clear();
   starting_times.clear();
   ending_times.clear();
   spec.clear();
   op_slack.clear();
   const FunctionBehaviorConstRef FB = hls_manager.lock()->CGetFunctionBehavior(function_index);
   op_graph = FB->CGetOpGraph(FunctionBehavior::FLSAODG);
}

void Schedule::AddConnectionTimes(unsigned int first_operation, unsigned int second_operation, const double value)
{
   connection_times[std::pair<unsigned int, unsigned int>(first_operation, second_operation)] = value;
}
