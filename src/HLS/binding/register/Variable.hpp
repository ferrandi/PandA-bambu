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
 * @file Variable.hpp
 * @brief Data structures used to manage set of variables.
 *
 *
 * @author Christian Pilato <pilato@elet.polimi.it>
 *
 */
#ifndef VARIABLE_HPP
#define VARIABLE_HPP

#include "custom_map.hpp"
#include "graph.hpp"
#include "refcount.hpp"

#include <iosfwd>

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

   virtual ~variable2obj() = default;

   /**
    * Function that prints the information associated with a variable.
    * @param it iterator pointing at the entry to print.
    */
   virtual void print_el(typename variable2obj<data_obj>::const_iterator& it) const = 0;

   /**
    * Function that prints the class variable2obj.
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
      {
         this->operator[](*left) = val;
      }
   }

   /**
    * Friend definition of the << operator.
    * @param os is the output stream
    * @param s is the object being printed
    */
   friend std::ostream& operator<<(std::ostream& os, variable2obj& s)
   {
      s.print(os);
      return os;
   }

   /**
    * Friend definition of the << operator. Pointer version.
    * @param os is the output stream
    * @param s is the object pointer to print
    */
   friend std::ostream& operator<<(std::ostream& os, const variable2obj* s)
   {
      if(s)
      {
         s->print(os);
      }
      return os;
   }
};

#endif
