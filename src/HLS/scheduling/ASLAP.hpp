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
 * @file ASLAP.hpp
 * @brief Class specifying ALAP and ASAP algorithms.
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *
 */
#ifndef ASLAP_HPP
#define ASLAP_HPP

#include <deque>
#include <iosfwd>

#include "graph.hpp"
#include "refcount.hpp"

/// HLS/scheduling include
#include "schedule.hpp"

/**
 * @name Forward declarations.
 */
//@{
CONSTREF_FORWARD_DECL(AllocationInformation);
REF_FORWARD_DECL(HLS_constraints);
CONSTREF_FORWARD_DECL(Schedule);
REF_FORWARD_DECL(Schedule);
REF_FORWARD_DECL(hls);
CONSTREF_FORWARD_DECL(HLS_manager);
CONSTREF_FORWARD_DECL(OpGraph);
CONSTREF_FORWARD_DECL(Parameter);
class graph;
class OpVertexSet;
enum class Allocation_MinMax;
//@}

/**
 * Class managing ALAP and ASAP algorithms.
 */
class ASLAP
{
 private:
   /// Array storing the ASAP values of the nodes in the graph.
   ScheduleRef ASAP;

   /// Array storing the ALAP values of the nodes in the graph.
   ScheduleRef ALAP;

   /// variable storing the minimum number of control steps required to schedule the graph.
   ControlStep min_tot_csteps;

   /// variable storing the maximum number of control steps required to schedule the graph.
   ControlStep max_tot_csteps;

   /// the graph to be scheduled
   OpGraphConstRef beh_graph;

   /// constant variable storing the reference to the array of vertexes sorted by topological order associated with
   /// the SDG(it can be used also for SG).
   std::deque<vertex> levels;

   /// is true if the beh_graph has at least one branching block
   bool has_branching_blocks;

   /// The allocation data structure
   const AllocationInformationConstRef allocation_information;

   /// speculation
   bool speculation;

   /// The clock period
   const double clock_period;

   /// debug level
   int debug_level;

   /// multiplier used to take into account chaining during asap/alap computation
   unsigned int ctrl_step_multiplier;
   /**
    * Modify the ALAP scheduling taking into account also technology constraints.
    * @param ALL is the allocation manager.
    */
   void add_constraints_to_ALAP();

   /**
    * Modify the ASAP scheduling taking into account also technology constraints.
    * Currently this function does not correcty work when alternative paths are present.
    * @param ALL is the allocation manager.
    */
   void add_constraints_to_ASAP();

   /**
    * Compute the standard ALAP scheduling with uncontrained resource
    * @param feasible is where feasibility of calculated ASAP & ALAP will be put; if used it must be setted initially
    *                 to true
    */
   void compute_ALAP_fast(bool* feasible = nullptr);

   /**
    * Compute the ALAP scheduling with uncontrained resources and with the minumum execution time. Considering the
    * minimum execution time is possible when it is already computed the max_tot_csteps. The standard ALAP algorithms
    * should use the maximum execution time since it does not know the total number of control steps thus requiring
    * a worst estimation of the ALAP time for each operation.
    * @param ALL is the allocation manager.
    */
   void compute_ALAP_LB();

   /**
    * Compute ALAP using list based euristic to identify the total number of control steps.
    * @param ALL is the allocation manager.
    */
   void compute_ALAP_worst_case();

   /**
    * This method returns the minimum number of control steps to schedule the graph.
    * @return the best-case estimation of control steps.
    */
   ControlStep get_min_csteps() const
   {
      return min_tot_csteps;
   }

   /**
    * This method returns the maximum number of control steps induced
    * by the specification and by the technology constraints.
    * @return a worst-case estimation of number of control steps.
    */
   ControlStep get_max_csteps() const
   {
      return max_tot_csteps;
   }

   /**
    * Return the number of cycles required to execute an operation
    * @param operation is the operation
    * @param minmax specifies if the minimum or the maximum execution time must be returned
    * @return the number of cycles
    */
   ControlStep GetCycleLatency(const vertex operation, Allocation_MinMax minmax) const;

 public:
   /**
    * Constructor.
    * @param hls_manager is the HLS manager
    * @param HLS is the whole HLS data structure.
    * @param speculation tells if speculation could be performed
    * @param beh_graph is the graph on which asap and alap is computed.
    * @param parameters is the set of input parameters
    */
   ASLAP(const HLS_managerConstRef hls_manager, const hlsRef HLS, const bool speculation, const OpVertexSet& operations, const ParameterConstRef parameters, unsigned int _ctrl_step_multiplier);

   /**
    * Destructor.
    */
   ~ASLAP() = default;

   /**
    * Possible type of ALAP method currently implemented.
    *  - \b ALAP_fast compute ALAP with unlimited constraints (O(n)), but max_ctotstep is calculated considering
    *                 the fastest functional units
    *  - \b ALAP_worst_case compute ALAP assuming only one resource for each type (O(n)).
    *  - \b ALAP_with_upper compute ALAP using partial_schedule to identify the total number of control
    *                       steps (O(n)).
    *  - \b ALAP_with_upper_minus_one compute ALAP using partial_schedule to identify the total number of control
    *                       steps minus one (O(n)).
    *  - \b ALAP_with_partial_scheduling compute ALAP using partial_schedule when is possible (O(n)).
    *
    */
   enum ALAP_method
   {
      ALAP_fast,
      ALAP_worst_case,
      ALAP_with_upper,
      ALAP_with_upper_minus_one,
      ALAP_with_partial_scheduling
   };

   /**
    * Function that prints the class ASLAP.
    * @param os is the stream where the class has to be printed
    */
   void print(std::ostream& os) const;

   /**
    * Function that computes the ASAP scheduling of the graph.
    * @param ALL is the allocation manager.
    * @param partial_schedule is a partial schedule that has to be taken into account
    */
   void compute_ASAP(const ScheduleConstRef partial_schedule = ScheduleConstRef());

   /**
    * Returns the ASAP vector of the vertices.
    * @return a map containing the ASAP values for each operation vertex
    */
   const ScheduleConstRef CGetASAP() const
   {
      return ASAP;
   }

   ScheduleRef GetASAP()
   {
      return ASAP;
   }

   /**
    * Function that computes the ALAP scheduling of the graph.
    * @param met is the method adopted to compute the ALAP scheduling
    *  - \b ALAP_fast compute ALAP with unlimited constraints (O(n)), but max_ctotstep is calculated considering
    *                 the fastest functional units
    *  - \b ALAP_worst_case compute ALAP assuming only one resource for each type (O(n)).
    *  - \b ALAP_with_upper compute ALAP using partial_schedule to identify the total number of control
    *                       steps (O(n)).
    *  - \b ALAP_with_upper_minus_one compute ALAP using partial_schedule to identify the total number of control
    *                       steps minus one (O(n)).
    *  - \b ALAP_with_partial_scheduling compute ALAP using partial_schedule when is possible (O(n)).
    *
    * @param partial_schedule is a partial schedule that has to be considered
    * @param feasible is where feasibility of calculated ASAP & ALAP will be put; if used it must be setted initially
    *                 to true
    * @param  est_upper_bound is an upper estimation of the control steps (used by ).
    */
   void compute_ALAP(ALAP_method met, const ScheduleConstRef partial_schedule = ScheduleConstRef(), bool* feasible = nullptr, const ControlStep = ControlStep(0u));

   /**
    * Returns the ALAP vector of the vertices.
    * @return a map containing the ALAP values for each operation vertex
    */
   const ScheduleConstRef CGetALAP() const
   {
      return ALAP;
   }

   /**
    * Return the ALAP (modifiable)
    */
   const ScheduleRef GetALAP()
   {
      return ALAP;
   }

   /**
    * Update the ALAP by using maxc as upper bound.
    * This method modifies the maximum number of control steps and updates the ALAP scheduling if it possible.
    * @param maxc is the maximum value of the control steps.
    * @param feasible is where feasibility of calculated ASAP & ALAP will be put; if used it must be setted initially
    *                 to true
    * @param partial_schedule is a partial schedule that has to be taken into account
    */
   void update_ALAP(const ControlStep maxc, bool* feasible = nullptr, const ScheduleConstRef partial_schedule = ScheduleConstRef());

   /**
    * Friend definition of the << operator.
    */
   friend std::ostream& operator<<(std::ostream& os, const ASLAP& s)
   {
      s.print(os);
      return os;
   }

   /**
    * Gets graph
    */
   const OpGraphConstRef CGetOpGraph() const;
};

typedef refcount<ASLAP> ASLAPRef;

#endif
