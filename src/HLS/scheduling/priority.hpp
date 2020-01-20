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
 * @file priority.hpp
 * @brief This package is used to drive the list based algorithm with different type
 * of priority schemes.
 *
 * @defgroup Priority Priority Package
 * @ingroup HLS
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *
 */
#ifndef PRIORITY_HPP
#define PRIORITY_HPP

#include "Vertex.hpp"

#include "custom_map.hpp"
#include "refcount.hpp"

#include "op_graph.hpp"

/**
 * @name Forward declarations.
 */
//@{
REF_FORWARD_DECL(ASLAP);
//@}

/**
 * Base class used to define the priority associated
 * with each vertex of a list base scheduling problem.
 */
template <class dataType>
struct priority_data
{
   /**
    * this function updates the value of the priority at
    * the end of the control step analysis.
    */
   virtual bool update() = 0;
   /**
    * Return the priority associated with the vertex. Constant version
    * @param _a is the vertex
    */
   virtual dataType operator()(const vertex& _a) const
   {
      return priority_values(_a);
   }
   /**
    * return the priority associated with the vertex. Constant version
    * @param _a is the vertex
    */
   virtual dataType& operator[](const vertex& _a)
   {
      return priority_values[_a];
   }
   /**
    * Destructor.
    */
   virtual ~priority_data()
   {
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
   explicit priority_static_mobility(const ASLAPRef& aslap);

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
   priority_dynamic_mobility(const ASLAPRef& aslap, const OpVertexSet& _ready_nodes, unsigned int _ctrl_step_multiplier);

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
   explicit priority_fixed(const CustomUnorderedMapUnstable<vertex, int>& priority_value);

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
   bool operator()(const vertex& a, const vertex& b) const
   {
      return priority_values->operator()(a) < priority_values->operator()(b) || (priority_values->operator()(a) == priority_values->operator()(b) && a > b);
   }

   /**
    * Constructor
    * @param pri is the priority data structure which associate at each vertex a priority value of type Type.
    */
   explicit priority_compare_functor(const refcount<priority_data<Type>> pri) : priority_values(pri)
   {
   }

   /**
    * Copy assignment
    */
   priority_compare_functor& operator=(const priority_compare_functor& in)
   {
      priority_values = in.priority_values;
      return *this;
   }
   /**
    * Copy constructor
    */
   priority_compare_functor(const priority_compare_functor& in) : priority_values(in.priority_values)
   {
   }

   /**
    * Destructor
    */
   ~priority_compare_functor() = default;

 private:
   /// copy of the priority values
   const refcount<priority_data<Type>> priority_values;
};

#endif
