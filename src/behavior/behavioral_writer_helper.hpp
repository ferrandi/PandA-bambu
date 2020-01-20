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
 * @file behavioral_writer_helper.hpp
 * @brief Collect all structs used to write a graph in the dot format
 *
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 * @author Marco Lattuada <lattuada@elet.polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *
 */
#ifndef BEHAVIORAL_WRITER_HELPER_HPP
#define BEHAVIORAL_WRITER_HELPER_HPP

/// Autoheader includs
#include "config_HAVE_HLS_BUILT.hpp"
#include "config_HAVE_HOST_PROFILING_BUILT.hpp"

/// Graph include
#include "graph.hpp"

/// utility include
#include "custom_set.hpp"
#include "refcount.hpp"

/// STD include
#include <ostream>

/**
 * @name forward declarations
 */
//@{
CONSTREF_FORWARD_DECL(BBGraph);
CONSTREF_FORWARD_DECL(BehavioralHelper);
CONSTREF_FORWARD_DECL(FunctionBehavior);
CONSTREF_FORWARD_DECL(hls);
CONSTREF_FORWARD_DECL(OpGraph);
#if HAVE_HOST_PROFILING_BUILT
CONSTREF_FORWARD_DECL(ProfilingInformation);
#endif
CONSTREF_FORWARD_DECL(Schedule);
//@}

/**
 * Edge writer for operation graph
 */
class OpEdgeWriter : public EdgeWriter
{
 private:
   /// The helper used to print the labels
   const BehavioralHelperConstRef BH;

 public:
   /**
    * Constructor
    * @param _g is the operation graph
    */
   explicit OpEdgeWriter(const OpGraph* operation_graph);

   /**
    * Operator which print label of an EdgeDescriptor
    * @param out is the stream
    * @param e is the edge
    */
   void operator()(std::ostream& out, const EdgeDescriptor& e) const override;
};

/**
 * Class which prints the edge of a basic block graph in dot format
 */
class BBEdgeWriter : public EdgeWriter
{
 private:
   /// The helper used to print the labels
   const BehavioralHelperConstRef BH;

 public:
   /**
    * Constructor
    * @param g is the bb_graph to be printed
    */
   explicit BBEdgeWriter(const BBGraph* g);

   /**
    * Operator used to print an edge
    * @param out is the stream on which edge should be printed
    * @param e is the edge to be printed
    */
   void operator()(std::ostream& out, const EdgeDescriptor& e) const override;
};

class OpWriter : public VertexWriter
{
 protected:
   /// The behavioral helper
   const BehavioralHelperConstRef helper;

 public:
   /**
    * Constructor
    * @param op_graph is the operation graph
    * @param detail_level is the level of detail:
    *  0 - print operation
    *  1 - print operation and accessed variables
    */
   OpWriter(const OpGraph* operation_graph, const int detail_level);

   /**
    * Functor to print the label of a vertex
    * @param out is the stream where label has to be printed
    * @param v is the vertex whose label has to be printed
    */
   void operator()(std::ostream& out, const vertex& v) const override;
};

class BBWriter : public VertexWriter
{
 private:
   /// The function behavior
   const FunctionBehaviorConstRef function_behavior;

   /// The helper
   const BehavioralHelperConstRef helper;

   /// The set of vertices to be annotated
   CustomUnorderedSet<vertex> annotated;

#if HAVE_HLS_BUILT
   /// The scheduling solution
   const ScheduleConstRef schedule;
#endif

 public:
   /**
    * The constructor
    * @param g is the graph to be printed
    * @param annotated is the set of the vertices to be annotated
    */
   BBWriter(const BBGraph* g, CustomUnorderedSet<vertex> annotated = CustomUnorderedSet<vertex>());

   /**
    * Operator used to print the label of a vertex
    * @param out is the stream where label has to printed
    * @param v is the vertex to be printed
    */
   void operator()(std::ostream& out, const vertex& v) const override;
};

#if HAVE_HLS_BUILT
class TimedOpWriter : public OpWriter
{
 protected:
   /// The HLS data structure
   const hlsConstRef HLS;

   /// The set of operations belonging to critical_paths
   CustomSet<unsigned int> critical_paths;

 public:
   /**
    * Constructor
    * @param op_graph is the operation graph to be printed
    * @param HLS is the HLS data structure
    * @param critical_paths is the set of operations belonging to critical paths
    */
   TimedOpWriter(const OpGraph* op_graph, const hlsConstRef HLS, CustomSet<unsigned int> critical_paths);

   /**
    * Operator which print label of an EdgeDescriptor
    * @param out is the stream
    * @param e is the edge
    */
   void operator()(std::ostream& out, const vertex& v) const override;
};

class TimedOpEdgeWriter : public OpEdgeWriter
{
 protected:
   /// The HLS data structure
   const hlsConstRef HLS;

   /// The set of operations belonging to critical_paths
   CustomSet<unsigned int> critical_paths;

 public:
   /**
    * Constructor
    * @param _g is the operation graph
    * @param HLS is the HLS data structure
    * @param critical_paths is the set of operations belonging to critical paths
    */
   TimedOpEdgeWriter(const OpGraph* operation_graph, const hlsConstRef HLS, CustomSet<unsigned int> critical_paths);

   /**
    * Operator which print label of an EdgeDescriptor
    * @param out is the stream
    * @param e is the edge
    */
   void operator()(std::ostream& out, const EdgeDescriptor& e) const override;
};
#endif

#endif
