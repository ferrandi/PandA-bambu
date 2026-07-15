/*
 *
 *        _/_/_/    _/_/   _/    _/ _/_/_/    _/_/
 *       _/   _/ _/    _/ _/_/  _/ _/   _/ _/    _/
 *      _/_/_/  _/_/_/_/ _/  _/_/ _/   _/ _/_/_/_/
 *     _/      _/    _/ _/    _/ _/   _/ _/    _/
 *    _/      _/    _/ _/    _/ _/_/_/  _/    _/
 *
 *  ***********************************************            
 *                   PandA Project
 *   URL: https://github.com/ferrandi/PandA-bambu
 *            Politecnico di Milano - DEIB
 *             System Architectures Group
 *  ***********************************************
 *   Copyright (C) 2004-2026 Politecnico di Milano
 *
 * Part of the PandA Project, under the Apache License v2.0 with LLVM Exceptions.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 *
 */
/**
 * @file Vertex.hpp
 * @brief Data structures used to manage set of vertexes.
 *
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 *
 */
#ifndef VERTEX_HPP
#define VERTEX_HPP

#include "custom_map.hpp"
#include "graph.hpp"

#include <iosfwd>

#define COLUMN_SIZE 30

/**
 * Class managing map of the vertexes on a generic object.
 */
template <class data_obj, typename vertex = gc_vertex_descriptor>
struct vertex2obj : public CustomUnorderedMapUnstable<vertex, data_obj>
{
   virtual ~vertex2obj() = default;

   /**
    * Function that print the information associated with a vertex.
    * @param os is the output stream
    * @param it is the iterator pointing to the vertex to print
    */
   virtual void print_el(std::ostream& os, typename vertex2obj<data_obj>::const_iterator& it) const
   {
      os << "(" << it->second << ") ";
   }

   /**
    * Function that print the name and the operation performed by the vertex.
    * @param os is the output stream
    * @param it is the iterator pointing to the vertex to print
    */
   virtual void print_rowHead(std::ostream& os, typename vertex2obj<data_obj>::const_iterator& it) const
   {
      os << it->first;
   }
   /**
    * Function that prints the class vertex2obj.
    * @param os is the output stream
    */
   virtual void print(std::ostream& os) const
   {
      auto i_end = this->end();
      for(auto i = this->begin(); i != i_end; ++i)
      {
         print_rowHead(os, i);
         print_el(os, i);
         os << " ";
      }
      os << std::endl;
   }

   const data_obj& operator()(const vertex& __k) const
   {
      THROW_ASSERT(this->find(__k) != this->end(), "expected a meaningful vertex");
      return this->find(__k)->second;
   }

   template <class Iterator, class data_type>
   void resize(Iterator left, Iterator right, data_type val)
   {
      for(; left != right; left++)
      {
         this->operator[](*left) = val;
      }
   }

   /**
    * Friend definition of the << operator.
    * @param os is the output stream
    */
   friend std::ostream& operator<<(std::ostream& os, const vertex2obj& s)
   {
      s.print(os);
      return os;
   }

   /**
    * Friend definition of the << operator. Pointer version.
    * @param os is the output stream
    */
   friend std::ostream& operator<<(std::ostream& os, const vertex2obj* s)
   {
      if(s)
      {
         s->print(os);
      }
      return os;
   }
};

template <typename vertex = gc_vertex_descriptor>
using vertex2int = vertex2obj<int, vertex>;

template <typename vertex = gc_vertex_descriptor>
using vertex2float = vertex2obj<double, vertex>;

#endif
