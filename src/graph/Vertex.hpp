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
 * @file Vertex.hpp
 * @brief Data structures used to manage set of vertexes.
 *
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *
 */
#ifndef VERTEX_HPP
#define VERTEX_HPP

#include "custom_map.hpp"
#include <iosfwd>

#include "graph.hpp"
#include "typed_node_info.hpp"

#define COLUMN_SIZE 30

/**
 * Class managing map of the vertexes on a generic object.
 */
template <class data_obj>
struct vertex2obj : public CustomUnorderedMapUnstable<vertex, data_obj>
{
   /**
    * Constructor.
    */
   vertex2obj() = default;

   /**
    * Destructor.
    */
   virtual ~vertex2obj()
   {
   }

   /**
    * Function that print the information associated with a vertex.
    * @param os is the output stream
    */
   virtual void print_el(std::ostream& os, const graph*, typename vertex2obj<data_obj>::const_iterator& it) const
   {
      os << "(" << it->second << ") ";
   }

   /**
    * Function that print the name and the operation performed by the vertex.
    * @param os is the output stream
    */
   virtual void print_rowHead(std::ostream& os, const graph* data, typename vertex2obj<data_obj>::const_iterator& it) const
   {
      if(data)
      {
         os << "Operation: ";
         os.width(COLUMN_SIZE);
         os.setf(std::ios_base::left, std::ios_base::adjustfield);
         ///         os << GET_NAME(data, it->first) + "(" + GET_OP(data, it->first) + ")";
         os.width(0);
      }
      else
         os << it->first;
   }
   /**
    * Function that prints the class vertex2obj.
    * @param os is the output stream
    */
   virtual void print(std::ostream& os, const graph* data = nullptr) const
   {
      auto i_end = this->end();
      for(auto i = this->begin(); i != i_end; ++i)
      {
         print_rowHead(os, data, i);
         print_el(os, data, i);
         if(data)
            os << std::endl;
         else
            os << " ";
      }
      if(!data)
         os << std::endl;
   }

   const data_obj operator()(const vertex& __k) const
   {
      THROW_ASSERT(this->find(__k) != this->end(), "expected a meaningful vertex");
      return this->find(__k)->second;
   }
   template <class Iterator, class data_type>
   void resize(Iterator left, Iterator right, data_type val)
   {
      for(; left != right; left++)
         this->operator[](*left) = val;
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
         s->print(os);
      return os;
   }
};

struct vertex2int : public vertex2obj<int>
{
};

struct vertex2float : public vertex2obj<double>
{
};

#endif
