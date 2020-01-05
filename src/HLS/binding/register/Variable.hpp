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
 * @file Variable.hpp
 * @brief Data structures used to manage set of variables.
 *
 *
 * @author Christian Pilato <pilato@elet.polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *
 */
#ifndef VARIABLE_HPP
#define VARIABLE_HPP

#include "custom_map.hpp"
#include <iosfwd>

#include "graph.hpp"

#include "refcount.hpp"

#define VARIABLE_COLUMN_SIZE 25

/**
 * Class managing map of the storage values on a generic object.
 */
template <class data_obj>
struct variable2obj : public std::map<unsigned int, data_obj>
{
   /**
    * Constructor.
    */
   variable2obj() = default;

   /**
    * Destructor.
    */
   virtual ~variable2obj()
   {
   }

   /**
    * Function that print the information associated with a variable
    * @param os is the output stream
    */
   virtual void print_el(typename variable2obj<data_obj>::const_iterator& it) const = 0;

   /**
    * Function that prints the class variable2obj.
    * @param os is the output stream
    */
   virtual void print() const
   {
      auto i_end = this->end();
      for(auto i = this->begin(); i != i_end; ++i)
      {
         print_el(i);
      }
   }

   const data_obj operator()(const unsigned int& __k) const
   {
      THROW_ASSERT(this->find(__k) != this->end(), "Impossible to find variable " + std::to_string(__k));
      return this->find(__k)->second;
   }

   template <class Iterator>
   void resize(Iterator left, Iterator right, int val)
   {
      for(; left != right; left++)
         this->operator[](*left) = val;
   }

   /**
    * Friend definition of the << operator.
    * @param os is the output stream
    */
   friend std::ostream& operator<<(std::ostream& os, variable2obj& s)
   {
      s.print(os);
      return os;
   }

   /**
    * Friend definition of the << operator. Pointer version.
    * @param os is the output stream
    */
   friend std::ostream& operator<<(std::ostream& os, const variable2obj* s)
   {
      if(s)
         s->print(os);
      return os;
   }
};

#endif
