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
 * @file schedule.hpp
 * @brief Data structure used to store the schedule of the operations.
 *
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 * @author Christian Pilato <pilato@elet.polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *
 */
#ifndef SCHEDULE_HPP
#define SCHEDULE_HPP

/// graph include
#include "graph.hpp"

/// STD include
#include <iosfwd>

/// utility includes
#include "refcount.hpp"
#include "strong_typedef.hpp"

CONSTREF_FORWARD_DECL(AllocationInformation);
REF_FORWARD_DECL(behavioral_manager);
UINT_STRONG_TYPEDEF(ControlStep);
/// This define cannot be included in the previous since it is not possible to have define inside define
#ifdef NDEBUG
#define ControlStep(value) value
#endif
CONSTREF_FORWARD_DECL(FunctionBehavior);
enum class FunctionFrontendFlowStep_Movable;
class HLS_manager;
CONSTREF_FORWARD_DECL(HLS_manager);
CONSTREF_FORWARD_DECL(OpGraph);
CONSTREF_FORWARD_DECL(Parameter);
REF_FORWARD_DECL(tree_manager);
CONSTREF_FORWARD_DECL(StateInfo);
CONSTREF_FORWARD_DECL(tree_manipulation);
REF_FORWARD_DECL(fu_binding);
//@}

#include "custom_map.hpp"
#include "custom_set.hpp"
#include "graph.hpp"
#include "hash_helper.hpp"
#include <iosfwd>

/**
 * Absolute Control step
 * First field is the basic block
 * Second field is the relative control step
 */
struct AbsControlStep : std::pair<unsigned int, ControlStep>
{
 public:
   /// Constant used to specify unknown control step
   static const ControlStep UNKNOWN;

   /**
    * Empty constructor
    */
   AbsControlStep();

   /**
    * Constructor
    * @param basic_block_index is the basic block index
    * @param control_step is the relative control step
    */
   AbsControlStep(const unsigned int basic_block_index, const ControlStep control_step);

   /**
    * Compare two scheduling step
    * @param other is the second step
    * @return true if this  is before other
    */
   bool operator<(const AbsControlStep& other) const;
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

   /// The tree manager
   tree_managerRef TM;

   /// The tree manipulation
   tree_manipulationConstRef tree_man;

   /// The allocation information
   AllocationInformationConstRef allocation_information;

   /// The index of the function
   const unsigned int function_index;

   /// total number of control steps
   ControlStep tot_csteps;

   /// map between the operation index and the clock cycle on which the operation starts its execution
   /// NOTE: it must be a map to know which is the last step
   CustomUnorderedMapUnstable<unsigned int, ControlStep> op_starting_cycle;

   /// The reverse of op_starting_cycle
   CustomMap<ControlStep, CustomSet<unsigned int>> starting_cycles_to_ops;

   /// map between the operation index and the clock cycle on which the operations ends its execution
   CustomUnorderedMapUnstable<unsigned int, ControlStep> op_ending_cycle;

   /// The absolute starting time of each operation as computed by the scheduling
   /// Key is the index of the gimple operation
   CustomMap<unsigned int, double> starting_times;

   /// The absolute ending time of each operation as computed by the scheduling
   /// Key is the index of the gimple operation
   CustomMap<unsigned int, double> ending_times;

   /// Connection times
   /// The key is an operation graph edge
   CustomMap<std::pair<unsigned int, unsigned int>, double> connection_times;

   /// Map for speculation property of each operation vertex. If true, it means that vertex is speculative executed,
   /// false otherwise
   CustomUnorderedMap<vertex, bool> spec;

   /// slack map
   std::map<vertex, double> op_slack;

   /// The operation graph (for scheduling purpose) (cannot be const because of = operator)
   OpGraphConstRef op_graph;

   /// The set of input parameters
   const ParameterConstRef parameters;

   /// The debug level
   const int debug_level;

   /**
    * Get when in a basic block an ssa is ready
    * @param tree_node_index is the index of the tree node
    * @param basic_block_index is the index of the basic block to be considered
    * @return the ending time of the definition gimple
    */
   double GetReadyTime(const unsigned int tree_node_index, const unsigned int basic_block_index) const;

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
   Schedule(const HLS_managerConstRef hls_manager, const unsigned int function_index, const OpGraphConstRef op_graph, const ParameterConstRef parameters);

   /**
    * Destructor.
    */
   ~Schedule();

   /**
    * Initialize the data structure
    */
   void Initialize();

   /**
    * This method returns the number of control steps.
    * @return the number of control steps of the stored scheduling.
    */
   ControlStep get_csteps() const
   {
      return tot_csteps;
   }

   /**
    * This method sets the number of control steps.
    * @param cs is the number of control steps.
    */
   void set_csteps(ControlStep cs)
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
   void WriteDot(const std::string& file_name) const;

   /**
    * Sets the speculation map
    */
   void set_spec(const CustomUnorderedMap<vertex, bool>& spec_map)
   {
      spec = spec_map;
   }

   /**
    * Returns the speculation map
    */
   CustomUnorderedMap<vertex, bool> get_spec() const
   {
      return spec;
   }

   /**
    * Sets the starting clock cycle for the given operation
    * @param op the the operation vertex
    * @param c_step is an integer representing the clock cycle where the operation starts the computation
    */
   void set_execution(const vertex& op, ControlStep c_step);

   /**
    * Sets the ending clock cycle for the given operation
    * @param op the the operation vertex
    * @param c_step_end is an integer representing the clock cycle where the operation ends the computation
    */
   void set_execution_end(const vertex& op, ControlStep c_step_end);

   /**
    * Returns true if the given operation has been already scheduled, false otherwise
    */
   bool is_scheduled(const vertex& op) const;

   /**
    * Returns true if the given operation has been already scheduled, false otherwise
    */
   bool is_scheduled(const unsigned int statement_index) const;

   /**
    * Returns the clock cycle where the given operation has been scheduled
    * @param op is the vertex of the operation
    * @return an integer representing the clock cycle where it starts the execution
    */
   AbsControlStep get_cstep(const vertex& op) const;

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
   AbsControlStep get_cstep_end(const vertex& op) const;

   /**
    * Return the last clock cycle in which the operation execute
    * @param op is the operation
    * @return the last clock cycle
    */
   AbsControlStep get_cstep_end(const unsigned int op) const;

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
    * @param e is the edge
    * @return return the fan-out correction for e
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

   /**
    * set the slack associated with the vertex with respect to the clock period
    */
   void set_slack(vertex op, double v_slack)
   {
      op_slack[op] = v_slack;
   }

   double get_slack(vertex op) const
   {
      if(op_slack.find(op) != op_slack.end())
         return op_slack.find(op)->second;
      else
         return 0.0;
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
   FunctionFrontendFlowStep_Movable CanBeMoved(const unsigned int statement_index, const unsigned int basic_block) const;

   /**
    * Check if a further condition can be added to gimple multi way if without increasing basic block latency
    * @param statement_index is the index of the gimple_multi_way_if
    * @param first_condition is the first condition
    * @param second_condition is the second condition
    */
   bool EvaluateCondsMerging(const unsigned statement_index, const unsigned int first_condition, const unsigned second_condition) const;

   /**
    * Evaluate if two conditional statements can be merged to create a gimple_multi_way_if
    * @param first_statement_index is the index of the first statement to be merged
    * @param second_statement_index is the index of the second statement to be merged
    * @return true if the latency of the basic block does not change
    */
   bool EvaluateMultiWayIfsMerging(const unsigned int first_statement_index, const unsigned int second_statement_index) const;

   /**
    * Print the timing information about an operation
    */
   const std::string PrintTimingInformation(const unsigned int statement_index) const;

   /**
    * Compute the critical path inside a state
    * @param state_info is the state
    */
   CustomSet<unsigned int> ComputeCriticalPath(const StateInfoConstRef state_info) const;

   /**
    * Add fan out correction time
    * @param edge is the edge to which correction time is associated
    * @param value is the value of the correction
    */
   void AddConnectionTimes(unsigned int first_operation, unsigned int second_operation, const double value);
};
/// Refcount definition of the class
typedef refcount<Schedule> ScheduleRef;
typedef refcount<const Schedule> ScheduleConstRef;
#endif
