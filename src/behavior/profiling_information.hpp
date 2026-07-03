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
 * @file profiling_information.hpp
 * @brief Class specification for storing profiling information.
 *
 * This structure stores information about loop and path profiling
 *
 * @author Marco Lattuada <marco.lattuada@polimi.it>
 *
 */
#ifndef PROFILING_INFORMATION_HPP
#define PROFILING_INFORMATION_HPP

#include "basic_block.hpp"
#include "custom_map.hpp"
#include "custom_set.hpp"
#include "refcount.hpp"

#include "algorithms/loops_detection/loop.hpp"

#include "config_HAVE_UNORDERED.hpp"

class xml_element;

/**
 * Map storing path profiling information
 */
#if HAVE_UNORDERED
class PathProfilingInformation
    : public CustomUnorderedMap<unsigned int, std::map<CustomOrderedSet<unsigned int>, long double>>
{
};
#else
class PathProfilingInformation : public std::map<unsigned int, std::map<CustomOrderedSet<unsigned int>, long double>>
{
};
#endif

/**
 * Map storing number of executions of each basic block
 */
#if HAVE_UNORDERED
class BBExecutions : public CustomUnorderedMap<BBGraphsCollection::vertex_descriptor, unsigned long long int>
{
 public:
   /**
    * Constructor
    * @param bb_graph is the basic block graph
    */
   explicit BBExecutions(const BBGraphsCollection* bb_graph);
};
#else
class BBExecutions : public std::map<BBGraphsCollection::vertex_descriptor, unsigned long long int, BBVertexSorter>
{
 public:
   /**
    * Constructor
    * @param bb_graph is the basic block graph
    */
   explicit BBExecutions(const BBGraphsCollection* bb_graph);
};
#endif

/**
 * Map storing number of executions of each basic block edge
 */
#if HAVE_UNORDERED
class BBEdgeExecutions : public CustomUnorderedMap<BBGraph::edge_descriptor, unsigned long long int>
{
 public:
   /**
    * Constructor
    * @param bb_graph is the basic block graph
    */
   explicit BBEdgeExecutions(const BBGraphsCollection* bb_graph);
};
#else
class BBEdgeExecutions : public std::map<BBGraph::edge_descriptor, unsigned long long int, BBEdgeSorter>
{
 public:
   /**
    * Constructor
    * @param bb_graph is the basic block graph
    */
   explicit BBEdgeExecutions(const BBGraphsCollection* bb_graph);
};
#endif

/**
 * Map storing number of average iterations
 */
#if HAVE_UNORDERED
class AvgIterations : public CustomUnorderedMap<unsigned int, long double>
{
};
#else
class AvgIterations : public std::map<unsigned int, long double>
{
};
#endif

/**
 * Map storing number of abs/max iterations
 */
#if HAVE_UNORDERED
class Iterations : public CustomUnorderedMap<unsigned int, unsigned long long int>
{
};
#else
class Iterations : public std::map<unsigned int, unsigned long long int>
{
};
#endif

/**
 * Definition of the profiling information class.
 */
class ProfilingInformation
{
 private:
   /// Friend defintion of profiling classes
   friend class BasicBlocksProfiling;
   friend class HostProfiling;

   /// map that represents, for each execution path of each loop (represented by the set of the executed control
   /// equivalent regions), the execution frequency
   PathProfilingInformation path_profiling;

   /// Absolute number of execution of each basic block
   BBExecutions bb_executions;

   /// Absolute number of running of edges
   BBEdgeExecutions edge_executions;

   /// number of average iterations for one loop execution
   AvgIterations avg_iterations;

   /// number of absolute execution (different from number of execution of the header); correspond to how many times a
   /// feedback edge of a loop is executed
   Iterations abs_iterations;

   /// Maximum number of iterations
   Iterations max_iterations;

 public:
   /**
    * Constructor
    * @param bb_graph is the basic block graph
    */
   explicit ProfilingInformation(const BBGraphsCollection* bb_graph);

   /**
    * Return the path profiling information
    * @return the path profiling information
    */
   const PathProfilingInformation& GetPathProfiling() const;

   /**
    * Return the absolute number of executions of a basic block
    * @param bb_vertex is the basic block
    * @return the absolute number of executions of a basic block
    */
   unsigned long long int GetBBExecutions(BBGraph::vertex_descriptor bb_vertex) const;

   /**
    * Return the absolute number of the executions of an edge
    * @param edge is the edge
    * @return the absolute number of executions of the edge
    */
   unsigned long long int GetEdgeExecutions(const BBGraph::edge_descriptor& edge) const;

   /**
    * Return number of average iterations of a loop
    * @param loop_id is the id of the loop
    * @return the number of average iterations of the loop
    */
   long double GetLoopAvgIterations(const unsigned int loop_id) const;

   /**
    * Return the maximum number of iterations of a loop
    * @param loop_id is the of the loop
    * @return the number of maximum iterations of the loop
    */
   unsigned long long int GetLoopMaxIterations(const unsigned int loop_id) const;

   /**
    * Return the number of absolute iterations of a loop
    * @param loop_id is the id of the loop
    * @return the number of average iterations of the loop
    */
   unsigned long long GetLoopAbsIterations(const unsigned int loop_id) const;

   /**
    * Return number of average iterations of a loop
    * @param loop is the loop
    * @return the number of average iterations of the loop
    */
   long double GetLoopAvgIterations(const LoopConstRef loop) const;

   /**
    * Return the maximum number of iterations of a loop
    * @param loop is the loop
    * @return the number of maximum iterations of the loop
    */
   unsigned long long int GetLoopMaxIterations(const LoopConstRef loop) const;

   /**
    * Return the number of absolute iterations of a loop
    * @param loop is the loop
    * @return the number of average iterations of the loop
    */
   unsigned long long GetLoopAbsIterations(const LoopConstRef loop) const;

   /**
    * Write to xml
    * @param root is the root xml node to which append the information contained in this profiling information
    * @param fcfg is the basic block graph of the function
    */
   void WriteToXml(xml_element* root, const BBGraph& fcfg) const;

   /**
    * Clear
    */
   void Clear();
};

#endif
