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
 * @file schedule.hpp
 * @brief Data structure used to store the schedule of the operations.
 *
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 * @author Christian Pilato <pilato@elet.polimi.it>
 *
 */
#ifndef SCHEDULE_HPP
#define SCHEDULE_HPP

#include "custom_map.hpp"
#include "custom_set.hpp"
#include "hash_helper.hpp"
#include "op_graph.hpp"
#include "refcount.hpp"

#include <iosfwd>

class HLS_manager;
enum class FunctionFrontendFlowStep_Movable;
CONSTREF_FORWARD_DECL(AllocationInformation);
CONSTREF_FORWARD_DECL(FunctionBehavior);
CONSTREF_FORWARD_DECL(HLS_manager);
CONSTREF_FORWARD_DECL(Parameter);
CONSTREF_FORWARD_DECL(ir_manipulation);
REF_FORWARD_DECL(behavioral_manager);
REF_FORWARD_DECL(fu_binding);
REF_FORWARD_DECL(ir_manager);

/**
 * Absolute Control step
 * First field is the basic block
 * Second field is the relative control step
 */
struct AbsControlStep : std::pair<unsigned int, unsigned int>
{
 public:
   /// Constant used to specify unknown control step
   static constexpr unsigned int UNKNOWN = std::numeric_limits<unsigned int>::max();

   /**
    * Empty constructor
    */
   AbsControlStep();

   /**
    * Constructor
    * @param basic_block_index is the basic block index
    * @param control_step is the relative control step
    */
   AbsControlStep(const unsigned int basic_block_index, const unsigned int control_step);

   /**
    * Compare two scheduling step
    * @param other is the second step
    * @return true if this  is before other
    */
   bool operator<(const AbsControlStep& other) const;
};

struct loopPipelinedInfo
{
   unsigned int II{0};
};

/**
 * Class managing the schedule of the operations.
 */
class Schedule
{
 private:
   friend class parametric_list_based;
   friend class AllocationInformation;

   /// The HLS manager
   const Wrefcount<const HLS_manager> hls_manager;

   /// The IR manager
   ir_managerRef TM;

   /// The IR manipulation
   ir_manipulationConstRef ir_man;

   /// The allocation information
   AllocationInformationConstRef allocation_information;

   /// The index of the function
   const unsigned int function_index;

   /// total number of control steps
   unsigned int tot_csteps;

   /// map between the operation index and the clock cycle on which the operation starts its execution
   /// NOTE: it must be a map to know which is the last step
   CustomUnorderedMapUnstable<unsigned int, unsigned int> op_starting_cycle;

   /// The reverse of op_starting_cycle
   CustomMap<unsigned int, CustomSet<unsigned int>> starting_cycles_to_ops;

   /// map between the operation index and the clock cycle on which the operations ends its execution
   CustomUnorderedMapUnstable<unsigned int, unsigned int> op_ending_cycle;

   /// The absolute starting time of each operation as computed by the scheduling
   /// Key is the index of the statement
   CustomMap<unsigned int, double> starting_times;

   /// The absolute ending time of each operation as computed by the scheduling
   /// Key is the index of the statement
   CustomMap<unsigned int, double> ending_times;

   /// slack map
   std::map<gc_vertex_descriptor, double> op_slack;

   /// The operation graph (for scheduling purpose) (cannot be const because of = operator)
   const OpGraph op_graph;

   /// The set of input parameters
   const ParameterConstRef parameters;

   /// The debug level
   const int debug_level;

   /// store for each loop pipelined the resulting II and other infos ordered by BB index
   CustomOrderedMap<unsigned, loopPipelinedInfo> loopPipelinedMap;

   /**
    * Get when in a basic block an ssa is ready
    * @param ir_node_index is the index of the IR node
    * @param basic_block_index is the index of the basic block to be considered
    * @return the ending time of the defining statement
    */
   double GetReadyTime(const unsigned int ir_node_index, const unsigned int basic_block_index) const;

   /**
    * Return the ending time of a basic block (i.e., the ending time of the last ending operation
    * @param basic_block_index is the index of the basic block
    * @return the ending time of the last operation
    */
   double GetBBEndingTime(const unsigned int basic_block_index) const;

 public:
   /**
    * Constructor.
    */
   Schedule(const HLS_managerConstRef hls_manager, const unsigned int function_index,
            const ParameterConstRef parameters);

   /**
    * Initialize the data structure
    */
   void Initialize();

   /**
    * This method returns the number of control steps.
    * @return the number of control steps of the stored scheduling.
    */
   unsigned int get_csteps() const
   {
      return tot_csteps;
   }

   /**
    * This method sets the number of control steps.
    * @param cs is the number of control steps.
    */
   void set_csteps(unsigned int cs)
   {
      tot_csteps = cs;
   }

   /**
    * Function that prints the class schedule.
    */
   void print(fu_bindingRef Rfu = fu_bindingRef()) const;

   /**
    * Function that writes the dot file of the scheduling by using the AT&T direct graph representation.
    * @param file_name is the file name
    */
   void writeDot(const std::filesystem::path& file_name) const;

   void writeDot(const std::filesystem::path& file_name, const OpGraph& subgraph, const OpVertexSet& opSet) const;

   /**
    * Sets the starting clock cycle for the given operation
    * @param op the the operation vertex
    * @param c_step is an integer representing the clock cycle where the operation starts the computation
    */
   void set_execution(gc_vertex_descriptor op, unsigned int c_step);

   /**
    * Sets the ending clock cycle for the given operation
    * @param op the the operation vertex
    * @param c_step_end is an integer representing the clock cycle where the operation ends the computation
    */
   void set_execution_end(gc_vertex_descriptor op, unsigned int c_step_end);

   /**
    * Returns true if the given operation has been already scheduled, false otherwise
    */
   bool is_scheduled(gc_vertex_descriptor op) const;

   /**
    * Returns true if the given operation has been already scheduled, false otherwise
    */
   bool is_scheduled(const unsigned int statement_index) const;

   /**
    * Returns the clock cycle where the given operation has been scheduled
    * @param op is the vertex of the operation
    * @return an integer representing the clock cycle where it starts the execution
    */
   AbsControlStep get_cstep(gc_vertex_descriptor op) const;

   /**
    * Returns the clock cycle where the given operation has been scheduled
    * @param index is the index of the operation
    * @return an integer representing the clock cycle where it starts the execution
    */
   AbsControlStep get_cstep(const unsigned int index) const;

   /**
    * Return the last clock cycle in which the operation execute
    * @param op is the operation
    * @return the last clock cycle
    */
   AbsControlStep get_cstep_end(gc_vertex_descriptor op) const;

   /**
    * Return the last clock cycle in which the operation execute
    * @param statement_index is the index of the operation
    * @return the last clock cycle
    */
   AbsControlStep get_cstep_end(const unsigned int statement_index) const;

   /**
    * Return the starting time of the operation
    */
   double GetEndingTime(const unsigned int operation) const;

   /**
    * Return the starting time of the operation
    */
   double GetStartingTime(const unsigned int operation) const;

   /**
    * return the fan-out correction for a given edge
    * @param first_operation is the source operation of the connection
    * @param second_operation is the destination operation of the connection
    * @return the fan-out correction for the selected connection
    */
   double get_fo_correction(unsigned int first_operation, unsigned int second_operation) const;

   /**
    * Returns the number of scheduled operations
    */
   unsigned int num_scheduled() const;

   /**
    * Erases the current results
    */
   void clear();

   void remove_sched(gc_vertex_descriptor op);
   void remove_sched(const unsigned int operation_index);

   /**
    * set the slack associated with the vertex with respect to the clock period
    */
   void set_slack(gc_vertex_descriptor op, double v_slack)
   {
      op_slack[op] = v_slack;
   }

   double get_slack(gc_vertex_descriptor op) const
   {
      if(op_slack.find(op) != op_slack.end())
      {
         return op_slack.find(op)->second;
      }
      else
      {
         return 0.0;
      }
   }

   /**
    * Compute the starting and the ending time of a statement
    * @param operation_index is the index of the created statement
    * @param update_cs tells if control step information has to be updated
    */
   void UpdateTime(const unsigned int operation_index, bool update_cs = true);

   /**
    * Check if a statement can be moved at the end of a basic block
    * @param statement_index is the index of the statement
    * @param basic_block is the index of the basic block
    * @return true if it can be moved
    */
   FunctionFrontendFlowStep_Movable CanBeMoved(const unsigned int statement_index,
                                               const unsigned int basic_block) const;

   /**
    * Check if a further condition can be added to multi way if statement without increasing basic block latency
    * @param statement_index is the index of the multi_way_if_stmt
    * @param first_condition is the first condition
    * @param second_condition is the second condition
    * @param function_decl_nid is the function owning the statements being merged
    */
   bool EvaluateCondsMerging(const unsigned statement_index, const unsigned int first_condition,
                             const unsigned second_condition, unsigned int function_decl_nid) const;

   /**
    * Evaluate if two conditional statements can be merged to create a multi_way_if_stmt
    * @param first_statement_index is the index of the first statement to be merged
    * @param second_statement_index is the index of the second statement to be merged
    * @param function_decl_nid is the function owning the statements being merged
    * @return true if the latency of the basic block does not change
    */
   bool EvaluateMultiWayIfsMerging(const unsigned int first_statement_index, const unsigned int second_statement_index,
                                   unsigned int function_decl_nid) const;

   /**
    * Print the timing information about an operation
    */
   const std::string PrintTimingInformation(const unsigned int statement_index) const;

   /**
    * Compute the critical path inside a state
    * @param state_info is the state
    */
   template <typename StateInfo>
   CustomSet<unsigned int> ComputeCriticalPath(const StateInfo& state_info) const;

   /**
    * @brief AddLoopPipelinedInfor add info about the obtained II for a given BB
    * @param BB_index is the basic block index of the pipelined loop
    * @param II is the initiation interval
    */
   void AddLoopPipelinedInfor(unsigned BB_index, unsigned II)
   {
      loopPipelinedMap.insert({BB_index, {II}});
   }
   bool IsLoopPipelined(unsigned BB_index) const
   {
      return loopPipelinedMap.find(BB_index) != loopPipelinedMap.end();
   }
   unsigned GetLoopPipeliningII(unsigned BB_index) const
   {
      if(IsLoopPipelined(BB_index))
      {
         return loopPipelinedMap.find(BB_index)->second.II;
      }
      else
      {
         return 0;
      }
   }

   const CustomOrderedMap<unsigned, loopPipelinedInfo>& CGetLoopPipelinedInfo() const
   {
      return loopPipelinedMap;
   }
};
/// Refcount definition of the class
using ScheduleRef = refcount<Schedule>;
using ScheduleConstRef = refcount<const Schedule>;
#endif
