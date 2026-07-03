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
 * @file priority.hpp
 * @brief This package is used to drive the list based algorithm with different type
 * of priority schemes.
 *
 * @defgroup Priority Priority Package
 * @ingroup HLS
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 *
 */
#ifndef PRIORITY_HPP
#define PRIORITY_HPP

#include "Vertex.hpp"
#include "custom_map.hpp"
#include "op_graph.hpp"
#include "refcount.hpp"

class ASLAP;

/**
 * Base class used to define the priority associated
 * with each vertex of a list base scheduling problem.
 */
template <class dataType>
struct priority_data
{
   virtual ~priority_data() = default;

   /**
    * this function updates the value of the priority at
    * the end of the control step analysis.
    */
   virtual bool update() = 0;
   /**
    * Return the priority associated with the vertex. Constant version
    * @param _a is the vertex
    */

   virtual dataType operator()(OpGraph::vertex_descriptor _a) const
   {
      return priority_values(_a);
   }

   /**
    * return the priority associated with the vertex. Constant version
    * @param _a is the vertex
    */
   virtual dataType& operator[](OpGraph::vertex_descriptor _a)
   {
      return priority_values[_a];
   }

 private:
   /// data structure storing the priority values.
   vertex2obj<dataType> priority_values;
};

/**
 * This is a specialization based on mobility.
 * The update function does not change the priority at the end of the control step analysis.
 */
struct priority_static_mobility : public priority_data<int>
{
   /**
    * Constructor.
    */
   priority_static_mobility(const ASLAP& aslap);

   /**
    * This specialization does not update the priorities at the end of the control step.
    */
   bool update() override
   {
      return false;
   }
};

/**
 * This is a specialization based on mobility.
 * The update function does change the priority at the end of the control step analysis only of ready nodes.
 */
struct priority_dynamic_mobility : public priority_data<int>
{
   /**
    * Constructor.
    */
   priority_dynamic_mobility(const ASLAP& aslap, const OpVertexSet& _ready_nodes, unsigned int _ctrl_step_multiplier);

   /**
    * This specialization does update the priorities at the end of the control step only of ready nodes.
    */
   bool update() override;

 private:
   /// set of ready vertices.
   const OpVertexSet& ready_nodes;
   /// multiplier used to take into account chaining during asap/alap computation
   unsigned int ctrl_step_multiplier;
};

/**
 * This is a specialization based on a given fixed priority value.
 * The update function does not change the priority at the end of the control step analysis.
 */
struct priority_fixed : public priority_data<int>
{
   /**
    * Constructor.
    */
   priority_fixed(const CustomUnorderedMapUnstable<OpGraph::vertex_descriptor, int>& priority_value);

   /**
    * This specialization does not update the priorities at the end of the control step.
    */
   bool update() override
   {
      return false;
   }
};

/**
 * Functor used to compare two vertices with respect to a priority object
 */
template <class Type>
struct priority_compare_functor
{
   /**
    * functor function used to compare two vertices with respect to the priority data structure.
    * @param a is the first vertex
    * @param b is the second vertex
    * @return true when priority(a) < priority(b)
    */
   bool operator()(OpGraph::vertex_descriptor a, OpGraph::vertex_descriptor b) const
   {
      return priority_values->operator()(a) < priority_values->operator()(b) ||
             (priority_values->operator()(a) == priority_values->operator()(b) && a > b);
   }

   /**
    * Constructor
    * @param pri is the priority data structure which associate at each vertex a priority value of type Type.
    */
   priority_compare_functor(const refcount<priority_data<Type>>& pri) : priority_values(pri)
   {
   }

 private:
   /// copy of the priority values
   const refcount<priority_data<Type>> priority_values;
};

#endif
