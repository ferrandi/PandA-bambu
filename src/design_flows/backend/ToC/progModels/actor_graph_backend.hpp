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
 * @file actor_graph_backend.hpp
 * @brief Abstract class to write an actor graphs
 *
 * @author Marco Lattuada <lattuada@elet.polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *
 */

#ifndef ACTOR_GRAPH_BACKEND_HPP
#define ACTOR_GRAPH_BACKEND_HPP

/// Autoheader includes
#include "config_HAVE_GRAPH_PARTITIONING_BUILT.hpp"
#include "config_HAVE_MPPB.hpp"

/// intermediate_representations/actor_graphs include
#include "actor_graph_backend.hpp"

/// utility includes
#include "refcount.hpp"

/// STL include
#include "custom_map.hpp"

REF_FORWARD_DECL(ActorGraphBackend);
REF_FORWARD_DECL(ActorGraphWriter);
CONSTREF_FORWARD_DECL(ActorGraphManager);
REF_FORWARD_DECL(CWriter);
REF_FORWARD_DECL(IndentedOutputStream);
CONSTREF_FORWARD_DECL(Parameter);
CONSTREF_FORWARD_DECL(PartitioningManager);
enum class ActorGraph_Type;

/**
 * C thread backend to be used
 */
enum class ActorGraphBackend_Type
{
#if HAVE_MPPB
   BA_MPPB, /**< Mppb backend */
#endif
   BA_NONE, /**< Plain backend */
#if HAVE_GRAPH_PARTITIONING_BUILT
   BA_OPENMP, /**< Openmp backend */
   BA_PTHREAD /**< Pthread backend */
#endif
};

std::string ToString(ActorGraphBackend_Type actor_graph_backend_type);

class ActorGraphBackend
{
 protected:
   /// The partitioning manager
   const PartitioningManagerConstRef partitioning_manager;

   /// The map containing all the actor graph writers
   const CustomUnorderedMap<ActorGraph_Type, ActorGraphBackendRef>& actor_graph_backends;

   /// The writer used to print the content of an actor
   const CWriterRef c_writer;

   /// The output stream
   const IndentedOutputStreamRef indented_output_stream;

   /// The set of input parameters
   const ParameterConstRef parameters;

   /// True if comments have to be printed
   const bool verbose;

   /// The debug_level
   int debug_level;

 public:
   /**
    * Constructor
    * @param partitioning_manager is the partitioning manager
    * @param actor_graph_backends are the backend used to print graphs;
    * NOTE: this parameters has to be passed by reference; it is actually allocated in ParallelCWriter
    * @param c_writer is the writer used to print content of the actor
    * @param indented_output_stream is the output stream
    * @param parameters is the set of input parameters
    * @param verbose specifies if comments have to be printed
    */
   ActorGraphBackend(const PartitioningManagerConstRef& partitioning_manager, const CustomUnorderedMap<ActorGraph_Type, ActorGraphBackendRef>& actor_graph_backends, const CWriterRef& c_writer, const IndentedOutputStreamRef& indented_output_stream,
                     const ParameterConstRef& parameters, const bool verbose);

   /**
    * Destructor
    */
   virtual ~ActorGraphBackend();

   /**
    * Write the implementation of the body of a loop associated with an actor graph
    * @param actor_graph_manager is the actor graph
    */
   virtual void WriteBodyLoop(const ActorGraphManagerConstRef actor_graph_manager) = 0;

   /**
    * Write the implementation of an actor graph
    * @param actor_graph_manager is the actor graph manager to be created
    */
   virtual void WriteActorGraph(const ActorGraphManagerConstRef actor_graph_manager) = 0;

   /**
    * @return the associated writer
    */
   virtual const ActorGraphWriterRef GetActorGraphWriter() = 0;
};
typedef refcount<ActorGraphBackend> ActorGraphBackendRef;
#endif
