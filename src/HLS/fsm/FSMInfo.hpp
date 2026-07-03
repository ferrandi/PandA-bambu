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
 *              Copyright (C) 2025-2026 Politecnico di Milano
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
 * @file FSMInfo.cpp
 * @brief Container for FSM state metadata associated with FSM states and edges
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 *
 */

#ifndef FSMINFO_HPP
#define FSMINFO_HPP

#include <algorithm>
#include <iterator>
#include <limits>
#include <list>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "behavior/op_graph.hpp"
#include "refcount.hpp"

REF_FORWARD_DECL(hls);
CONSTREF_FORWARD_DECL(FunctionBehavior);
REF_FORWARD_DECL(structural_object);
REF_FORWARD_DECL(generic_obj);
REF_FORWARD_DECL(HLS_manager);
REF_FORWARD_DECL(Schedule);

enum transitionType : int
{
   noEdgeCondition = 0,
   elseifEdgeCondition,
   doneVariableLatencyOpEdgeCondition,
   runningVariableLatencyOpEdgeCondition
};

using stateTransitionType = bool;
inline constexpr stateTransitionType stEdgeNormal{false};
inline constexpr stateTransitionType stEdgeFeedback{true};

class FSMInfo
{
 public:
   /// Identifier used to reference FSM states.
   using state_descriptor = unsigned int;
   /// Vertex descriptor in the operation graph used to reference scheduled operations.
   using operation_descriptor = OpGraph::vertex_descriptor;
   /// Unique key for transitions, pairing source and destination states.
   using edge_key = std::pair<state_descriptor, state_descriptor>;
   inline static constexpr state_descriptor invalidState = std::numeric_limits<state_descriptor>::max();

   struct edgeData;

   using EdgeMap = std::map<state_descriptor, edgeData>;

 private:
   // Outgoing edges: source state -> (target state -> edge metadata)
   std::map<state_descriptor, EdgeMap> edgeDataMap;
   std::map<state_descriptor, std::set<state_descriptor>> reverseEdgeMap;

 public:
   static edgeData buildUnboundedCondition(transitionType t, const std::vector<operation_descriptor>& ops,
                                           state_descriptor referenceState, stateTransitionType type);
   static edgeData buildElseIfCondition(operation_descriptor op, const CustomOrderedSet<unsigned>& labels,
                                        bool isElseEdge, stateTransitionType type);

   struct stateData
   {
      /// Human-readable state name emitted in reports and DOT dumps.
      std::string name;
      // Operations that execute while the FSM resides in this state.
      std::list<operation_descriptor> executingOperations;
      // Operations whose execution begins in this state.
      std::list<operation_descriptor> startingOperations;
      // Operations that complete in this state.
      std::list<operation_descriptor> endingOperations;
      // Basic block contributing control flow to this state.
      unsigned int bbId{0};
      // Scheduling step at which each dependency enters the state.
      std::map<operation_descriptor, unsigned> stepIn;
      // True when this is the last real state before the FSM exit.
      bool isLastState{false};
      // Initiation interval assigned by loop pipelining (if any).
      unsigned lpII{0};
      // Highest scheduling step scheduled for this state.
      unsigned maxStep{0};
      // Marks states introduced only to hold place and not part of original behavior.
      bool isDummy{false};
      // True when the state belongs to a pipelined controller.
      bool isPipelinedState{false};
      // States involved in prologue execution for pipelined kernels.
      std::set<operation_descriptor> isPrologue;
   };

   /// Metadata associated with an FSM edge (transition).
   struct edgeData
   {
      /// Control condition associated with the transition.
      transitionType edgeFSMType{noEdgeCondition};
      /// True when the edge represents a default branch of a switch.
      bool isElseEdge{false};
      /// Reference state used for multi-operation checks (e.g., ALL_FINISHED).
      state_descriptor referenceState{invalidState};
      /// Operations referenced by the transition condition.
      std::vector<operation_descriptor> edgeOperations;
      /// Guard labels associated with CASE transitions.
      CustomOrderedSet<unsigned> edgeConditions;

      stateTransitionType edgeSelector{stEdgeNormal};

      edgeData() = default;
   };

   /// Synthetic entry vertex used as the FSM root.
   state_descriptor entryNode{invalidState};
   /// Synthetic exit vertex used as the FSM sink.
   state_descriptor exitNode{invalidState};

   /// true when the FSM has cycles
   bool isADag{true};

   /// true when at least one state is a dummy state
   bool hasDummyState{false};

   /// true in case the FSM is for a fixed latency function
   bool bounded{false};

   /// number of stages in case the function is pipelined
   unsigned nStages{0};

   /// in case of a dag it is possible to compute the minimum number of cycles
   unsigned int minCycles{0};

   /// maximum number of cycles
   unsigned int maxCycles{0};

   // Highest scheduling step scheduled for a given BB
   std::map<unsigned int, unsigned int> BB2MaxStep;

   /// store in which states and an operation is terminating its execution
   std::map<operation_descriptor, std::set<state_descriptor>> operationEndingStates;
   /// store in which states and an operation is running
   std::map<operation_descriptor, std::set<state_descriptor>> operationExecutingStates;

   std::map<state_descriptor,
            std::map<operation_descriptor, std::map<unsigned int, CustomOrderedSet<state_descriptor>>>>
       variableSourceStates;

   std::map<operation_descriptor, unsigned> opStepOut;

   FSMInfo() = default;

   // Lightweight ranges to traverse successors using edgeDataMap
   struct successorsRange
   {
      struct const_iterator
      {
         EdgeMap::const_iterator it;
         EdgeMap::const_iterator end;
         bool skip;
         const_iterator& operator++()
         {
            ++it;
            if(skip)
            {
               while(it != end && it->second.edgeSelector == stEdgeFeedback)
               {
                  ++it;
               }
            }
            return *this;
         }
         bool operator!=(const const_iterator& o) const
         {
            return it != o.it;
         }
         state_descriptor operator*() const
         {
            return it->first;
         }
      };
      successorsRange(const EdgeMap* map, bool skip = false) : m(map), skip_feedback(skip)
      {
      }
      const_iterator begin() const
      {
         EdgeMap::const_iterator b = m ? m->cbegin() : EdgeMap::const_iterator{};
         EdgeMap::const_iterator e = m ? m->cend() : EdgeMap::const_iterator{};
         if(skip_feedback)
         {
            while(b != e && b->second.edgeSelector == stEdgeFeedback)
            {
               ++b;
            }
         }
         return const_iterator{b, e, skip_feedback};
      }
      const_iterator end() const
      {
         EdgeMap::const_iterator e = m ? m->cend() : EdgeMap::const_iterator{};
         return const_iterator{e, e, skip_feedback};
      }

    private:
      const EdgeMap* m;
      bool skip_feedback{false};
   };
   struct successorsWithDataRange
   {
      struct value_type
      {
         state_descriptor target;
         const edgeData& data;
      };
      struct const_iterator
      {
         const_iterator(EdgeMap::const_iterator it, EdgeMap::const_iterator end, bool skip)
             : m_it(it), m_end(end), m_skip(skip)
         {
            if(m_skip)
            {
               advance();
            }
         }

         const_iterator& operator++()
         {
            ++m_it;
            if(m_skip)
            {
               advance();
            }
            return *this;
         }

         bool operator!=(const const_iterator& o) const
         {
            return m_it != o.m_it;
         }

         value_type operator*() const
         {
            return value_type{m_it->first, m_it->second};
         }

       private:
         void advance()
         {
            while(m_it != m_end && m_it->second.edgeSelector == stEdgeFeedback)
            {
               ++m_it;
            }
         }

         EdgeMap::const_iterator m_it;
         EdgeMap::const_iterator m_end;
         bool m_skip;
      };

      successorsWithDataRange(const EdgeMap* map, bool skip) : m(map), skip_feedback(skip)
      {
      }

      const_iterator begin() const
      {
         EdgeMap::const_iterator b = m ? m->cbegin() : EdgeMap::const_iterator{};
         EdgeMap::const_iterator e = m ? m->cend() : EdgeMap::const_iterator{};
         return const_iterator{b, e, skip_feedback};
      }

      const_iterator end() const
      {
         EdgeMap::const_iterator e = m ? m->cend() : EdgeMap::const_iterator{};
         return const_iterator{e, e, skip_feedback};
      }

    private:
      const EdgeMap* m;
      bool skip_feedback{false};
   };
   successorsRange successors(state_descriptor src) const;
   successorsWithDataRange successorsWithData(state_descriptor src) const;
   successorsRange successors(state_descriptor src, bool skip_feedback) const;
   successorsWithDataRange successorsWithData(state_descriptor src, bool skip_feedback) const;

   // Lightweight ranges to traverse predecessors (computed on demand)
   struct predecessorsRange
   {
      struct const_iterator
      {
         using set_iterator = std::set<state_descriptor>::const_iterator;

         const_iterator(const FSMInfo* owner, state_descriptor dst, set_iterator it, set_iterator end, bool skip)
             : m_owner(owner), m_dst(dst), m_it(it), m_end(end), m_skip(skip)
         {
            if(m_owner && m_skip)
            {
               advance();
            }
         }

         const_iterator& operator++()
         {
            ++m_it;
            if(m_owner && m_skip)
            {
               advance();
            }
            return *this;
         }

         bool operator!=(const const_iterator& o) const
         {
            return m_it != o.m_it;
         }

         bool operator==(const const_iterator& o) const
         {
            return m_it == o.m_it;
         }

         state_descriptor operator*() const
         {
            return *m_it;
         }

       private:
         void advance()
         {
            while(m_it != m_end)
            {
               const auto src = *m_it;
               const auto out_it = m_owner->edgeDataMap.find(src);
               if(out_it != m_owner->edgeDataMap.end())
               {
                  auto edge_it = out_it->second.find(m_dst);
                  if(edge_it != out_it->second.end() && edge_it->second.edgeSelector == stEdgeFeedback)
                  {
                     ++m_it;
                     continue;
                  }
               }
               break;
            }
         }

         const FSMInfo* m_owner{nullptr};
         state_descriptor m_dst{0};
         set_iterator m_it;
         set_iterator m_end;
         bool m_skip{false};
      };

      predecessorsRange(const FSMInfo* owner, state_descriptor dst, const std::set<state_descriptor>* sources,
                        bool skip)
          : m_owner(owner), m_dst(dst), m_sources(sources), m_skip(skip)
      {
      }

      const_iterator begin() const
      {
         return const_iterator{m_owner, m_dst, m_sources ? m_sources->cbegin() : set_iterator{},
                               m_sources ? m_sources->cend() : set_iterator(), m_skip};
      }

      const_iterator end() const
      {
         return const_iterator{m_owner, m_dst, m_sources ? m_sources->cend() : set_iterator{},
                               m_sources ? m_sources->cend() : set_iterator(), m_skip};
      }

    private:
      using set_iterator = std::set<state_descriptor>::const_iterator;

      const FSMInfo* m_owner;
      state_descriptor m_dst;
      const std::set<state_descriptor>* m_sources;
      bool m_skip{false};
   };
   predecessorsRange predecessors(state_descriptor dst) const;
   predecessorsRange predecessors(state_descriptor dst, bool skip_feedback) const;

   state_descriptor createState(const std::list<operation_descriptor>& exec_op,
                                const std::list<operation_descriptor>& start_op,
                                const std::list<operation_descriptor>& end_op, unsigned int BB_id,
                                const std::map<operation_descriptor, unsigned>& step_in,
                                const std::map<operation_descriptor, unsigned>& step_out, unsigned vertex_LP_II,
                                unsigned max_steps, bool is_last_state, bool isPipelined, const OpGraph& data,
                                const char* custom_name = nullptr);

   bool hasState(state_descriptor state) const;

   const stateData& getState(state_descriptor state) const;

   stateData& getState(state_descriptor state);

   unsigned getStateId(state_descriptor s) const;

   unsigned GetStep(const OpGraph& data, state_descriptor v, operation_descriptor op, unsigned int var, bool in,
                    bool var_register_compatible) const;
   unsigned GetStepWrite(const OpGraph& data, operation_descriptor def_op) const;
   unsigned GetStepIn(const OpGraph& data, unsigned int BB_index, unsigned int var) const;
   unsigned GetStepPhiIn(const OpGraph& data, operation_descriptor op, unsigned int var, unsigned int BB_src,
                         unsigned BB_src_state, state_descriptor src_state, const ScheduleRef& schedule) const;
   unsigned GetStepPhiOut(const OpGraph& data, operation_descriptor op, unsigned int var,
                          const ScheduleRef& schedule) const;
   unsigned GetStepOut(const OpGraph& data, unsigned int var) const;
   std::pair<bool, unsigned> GetPrevStep(const OpGraph& data, unsigned int BB_index, unsigned int var,
                                         unsigned curr_step, unsigned int offset, bool var_register_compatible) const;
   unsigned GetStepOp(const OpGraph& data, state_descriptor v, operation_descriptor exec_op) const;

   // Iterate over all FSM states
   struct statesRange;
   statesRange vertices() const;

   EdgeMap::size_type outDegree(state_descriptor state) const
   {
      auto it = edgeDataMap.find(state);
      if(it == edgeDataMap.end())
      {
         return 0;
      }
      return it->second.size();
   }

   struct statesRangeWithData
   {
      using value_type = std::pair<state_descriptor, const stateData&>;
      struct const_iterator
      {
         using vector_iterator = std::vector<stateData>::const_iterator;

         const_iterator(vector_iterator it, vector_iterator begin) : m_it(it), m_begin(begin)
         {
         }

         const_iterator& operator++()
         {
            ++m_it;
            return *this;
         }

         bool operator!=(const const_iterator& o) const
         {
            return m_it != o.m_it;
         }

         value_type operator*() const
         {
            const auto offset = static_cast<state_descriptor>(std::distance(m_begin, m_it));
            return value_type{offset, *m_it};
         }

       private:
         vector_iterator m_it;
         vector_iterator m_begin;
      };

      statesRangeWithData(const std::vector<stateData>* map) : m(map)
      {
      }
      const_iterator begin() const
      {
         return const_iterator{m->cbegin(), m->cbegin()};
      }
      const_iterator end() const
      {
         return const_iterator{m->cend(), m->cbegin()};
      }

    private:
      const std::vector<stateData>* m;
   };

   statesRangeWithData verticesWithData() const;

   // Range over only state descriptors
   struct statesRange
   {
      struct const_iterator
      {
         using vector_iterator = std::vector<stateData>::const_iterator;

         const_iterator(vector_iterator it, vector_iterator begin) : m_it(it), m_begin(begin)
         {
         }

         const_iterator& operator++()
         {
            ++m_it;
            return *this;
         }

         bool operator!=(const const_iterator& o) const
         {
            return m_it != o.m_it;
         }

         state_descriptor operator*() const
         {
            return static_cast<state_descriptor>(std::distance(m_begin, m_it));
         }

       private:
         vector_iterator m_it;
         vector_iterator m_begin;
      };

      statesRange(const std::vector<stateData>* map) : m(map)
      {
      }
      const_iterator begin() const
      {
         return const_iterator{m->cbegin(), m->cbegin()};
      }
      const_iterator end() const
      {
         return const_iterator{m->cend(), m->cbegin()};
      }
      auto size() const
      {
         return m->size();
      }

    private:
      const std::vector<stateData>* m;
   };

   bool notSameStep(state_descriptor state, operation_descriptor def, operation_descriptor op) const;

   edgeData& createEdge(state_descriptor source, state_descriptor target, const edgeData& data)
   {
      if(data.edgeSelector == stEdgeFeedback)
      {
         isADag = false;
      }
      auto oit = edgeDataMap.find(source);
      if(oit == edgeDataMap.end())
      {
         oit = edgeDataMap.emplace(source, EdgeMap()).first;
      }
      auto& out_map = oit->second;
      auto [it, inserted] = out_map.try_emplace(target, data);
      it->second = data;
      reverseEdgeMap[target].insert(source);
      return it->second;
   }

   const edgeData& getEdge(state_descriptor source, state_descriptor target) const
   {
      auto oit = edgeDataMap.find(source);
      THROW_ASSERT(oit != edgeDataMap.end(), "Edge data not found");
      auto iit = oit->second.find(target);
      THROW_ASSERT(iit != oit->second.end(), "Edge data not found");
      return iit->second;
   }

   void writeDot(const std::filesystem::path& file_name, FunctionBehaviorConstRef FB, hlsRef HLS,
                 const int detail_level = 0) const;

   void addMultiUnboundedObj(state_descriptor s, const std::vector<operation_descriptor>& ops);

   void specialiseMu(structural_objectRef& mu_mod, generic_objRef mu, bool is_function_pipelined) const;

   const std::map<state_descriptor, generic_objRef>& getMuCtrls() const
   {
      return multiUnboundedTable;
   }

   unsigned int getNumberOfStates(bool is_function_pipelined) const;

   void addToSM(structural_objectRef clock_port, structural_objectRef reset_port, hlsRef HLS,
                bool is_function_pipelined) const;

   void topologicalOrder(std::list<state_descriptor>& order) const;

   void reverseTopologicalOrder(std::list<state_descriptor>& order) const;

   /**
    * Compute depth buckets for FSM states using an optional feedback skip flag.
    * @param skip_feedback When true, feedback edges are ignored to ensure a DAG ordering.
    * @return Vector grouping states by depth level.
    */
   std::vector<std::set<state_descriptor>> depthMap(bool skip_feedback = true) const;

   inline static constexpr char stateNamePrefix[] = "S_";

   /**
    * @brief Retrieve the upstream states that feed the variable for the operation in the state.
    */
   const CustomOrderedSet<state_descriptor>&
   getVariableSourceStates(state_descriptor state, operation_descriptor operation, unsigned int variable) const;

   /**
    * @brief Record that the variable used by the operation in the state originates
    *        from the source state.
    */
   void addVariableSourceState(state_descriptor state, operation_descriptor operation, unsigned int variable,
                               state_descriptor sourceState);

   void finalizeFSMInfo(const OpGraph& data, const HLS_managerRef HLSMgr);

 private:
   unsigned satStep(unsigned BB_index, unsigned step) const;
   unsigned getStepInternal(const OpGraph& data, state_descriptor v, operation_descriptor op, unsigned int var,
                            bool in) const;
   std::vector<stateData> states;
   std::map<state_descriptor, generic_objRef> multiUnboundedTable;
   unsigned nextStateId{0};
   unsigned nextStateNameIndex{0};
   CustomOrderedSet<state_descriptor> emptyStateSet;
};

inline bool FSMInfo::hasState(state_descriptor state) const
{
   return state < states.size();
}

inline const FSMInfo::stateData& FSMInfo::getState(state_descriptor state) const
{
   THROW_ASSERT(state < states.size(), "Requested FSM state metadata not available");
   return states[state];
}

inline FSMInfo::stateData& FSMInfo::getState(state_descriptor state)
{
   THROW_ASSERT(state < states.size(), "Requested FSM state metadata not available");
   return states[state];
}

inline bool FSMInfo::notSameStep(state_descriptor state, operation_descriptor def, operation_descriptor op) const
{
   const auto& state_data = getState(state);
   if(!state_data.stepIn.empty())
   {
      const auto def_it = opStepOut.find(def);
      const auto op_it = state_data.stepIn.find(op);
      return def_it != opStepOut.end() && op_it != state_data.stepIn.end() && def_it->second != op_it->second;
   }
   return false;
}

inline FSMInfo::statesRange FSMInfo::vertices() const
{
   return statesRange{&states};
}

inline FSMInfo::statesRangeWithData FSMInfo::verticesWithData() const
{
   return statesRangeWithData{&states};
}

#endif
