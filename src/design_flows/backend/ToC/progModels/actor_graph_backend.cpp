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

/// Header include
#include "actor_graph_backend.hpp"

///. includes
#include "Parameter.hpp"

#include "string_manipulation.hpp" // for GET_CLASS

std::string ToString(ActorGraphBackend_Type actor_graph_backend_type)
{
   switch(actor_graph_backend_type)
   {
#if HAVE_MPPB
      case ActorGraphBackend_Type::BA_MPPB:
         return "MPPB";
#endif
      case ActorGraphBackend_Type::BA_NONE:
         return "NONE";
#if HAVE_GRAPH_PARTITIONING_BUILT
      case ActorGraphBackend_Type::BA_OPENMP:
         return "OpenMP";
      case ActorGraphBackend_Type::BA_PTHREAD:
         return "pthread";
#endif
      default:
         THROW_UNREACHABLE("");
   }
   return "";
}

ActorGraphBackend::ActorGraphBackend(const PartitioningManagerConstRef& _partitioning_manager, const CustomUnorderedMap<ActorGraph_Type, ActorGraphBackendRef>& _actor_graph_backends, const CWriterRef& _c_writer,
                                     const IndentedOutputStreamRef& _indented_output_stream, const ParameterConstRef& _parameters, const bool _verbose)
    : partitioning_manager(_partitioning_manager),
      actor_graph_backends(_actor_graph_backends),
      c_writer(_c_writer),
      indented_output_stream(_indented_output_stream),
      parameters(_parameters),
      verbose(_verbose),
      debug_level(parameters->get_class_debug_level(GET_CLASS(*this)))
{
}

ActorGraphBackend::~ActorGraphBackend() = default;
