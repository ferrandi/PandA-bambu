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
 * @file augmented_vector.hpp
 * @brief class for representing augmented vector based on std::vector
 *
 * @author Marco Lattuada <lattuada@elet.polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *
 */

#ifndef AUGMENTED_VECTOR_HPP
#define AUGMENTED_VECTOR_HPP
#include "exceptions.hpp"         // for THROW_ASSERT
#include <boost/lexical_cast.hpp> // for lexical_cast
#include <cmath>                  // for sqrtl
#include <cstddef>                // for size_t, ptrdiff_t
#include <string>                 // for string
#include <vector>                 // for vector

template <class T>
class AugmentedVector
{
 private:
   /// The class which it logical extended by this (inheritance from template is not allowed)
   std::vector<T> internal_vector;

 public:
   /**
    * Empty Constructor
    */
   AugmentedVector<T>() = default;

   /**
    * Construct a vector composed by size element
    * @param size is the size of the vector
    * @param element is the element with which the vector is filled
    */
   AugmentedVector<T>(const size_t _size, T element) : internal_vector(_size, element)
   {
   }

   /**
    * Construct a vector of size size
    * @param size is the size of the vector
    */
   explicit AugmentedVector<T>(const size_t _size) : internal_vector(_size)
   {
   }

   /**
    * Constructor from std::vector
    * @param vector is the starting vector
    */
   explicit AugmentedVector<T>(const std::vector<T>& vector) : internal_vector(vector)
   {
   }

   /**
    * Redefinition of [] operator
    * @param position is the position of the element to get
    */
   T& operator[](size_t position)
   {
      return internal_vector[position];
   }

   /**
    * Redefinition of [] operator
    * @param position is the position of the element to get
    */
   const T& operator[](size_t position) const
   {
      return internal_vector[position];
   }

   /**
    * Redefinition of * operator as scalar multiplication
    * @param other is the second operand
    * @return the scalar multiplication
    */
   T operator*(const AugmentedVector<T>& other) const
   {
      /// Fixme this works only if exists conversion from 0 to T
      T return_value = 0;
      THROW_ASSERT(internal_vector.size() == other.size(), "Different size in operands: " + boost::lexical_cast<std::string>(internal_vector.size()) + " vs " + boost::lexical_cast<std::string>(other.size()));
      for(size_t i = 0; i < internal_vector.size(); i++)
      {
         return_value += internal_vector[i] * other[i];
      }
      return return_value;
   }

   /**
    * Redefinition of - operator as the difference of the single element
    * @param other is the second operand
    * @return a vector storing the difference of the corresponding elements
    */
   AugmentedVector<T> operator-(const AugmentedVector<T>& other) const
   {
      AugmentedVector<T> return_value;
      THROW_ASSERT(internal_vector.size() == other.size(), "Different size in operands: " + boost::lexical_cast<std::string>(internal_vector.size()) + " vs " + boost::lexical_cast<std::string>(other.size()));
      for(size_t i = 0; i < internal_vector.size(); i++)
      {
         return_value.internal_vector[i] = internal_vector[i] - other.internal_vector[i];
      }
      return return_value;
   }

   /**
    * Return the size of the vector
    * @return the size of the vector
    */
   size_t size() const
   {
      return internal_vector.size();
   }

   /**
    * Return the 2-norm of the vector
    * @return the 2-norm of the vector
    */
   T Norm2() const
   {
      /// Fixme this works only if exists conversion from 0 to T
      T return_value = 0;
      for(size_t i = 0; i < internal_vector.size(); i++)
      {
         return_value += internal_vector[i] * internal_vector[i];
      }
      return sqrtl(return_value);
   }

   /**
    * Return the index of mininum element of the vector
    * @param min is where the index of the mimimum element will be stored
    */
   void get_min(size_t& min) const
   {
      min = 0;
      const size_t _size = this->size();
      for(size_t index = 0; index < _size; index++)
      {
         if(this->internal_vector[index] < this->internal_vector[min])
            min = index;
      }
   }

   /**
    * Return the index of the minimum and maximum element of the vector
    * @param min is where the index of the minimum element will be stored
    * @param max is where the index of the maximum element will be stored
    */
   void get_min_max(size_t& min, size_t& max) const
   {
      min = 0;
      max = 0;
      const size_t _size = this->size();
      for(size_t index = 0; index < _size; index++)
      {
         if(this->internal_vector[index] > this->internal_vector[max])
            max = index;
         if(this->internal_vector[index] < this->internal_vector[min])
            min = index;
      }
   }

   /**
    * Return the sum of a row
    * @return the sum of the row
    */
   T get_sum() const
   {
      T return_value = 0;
      const size_t _size = this->size();
      for(size_t index = 0; index < _size; index++)
      {
         return_value += this->internal_vector[index];
      }
      return return_value;
   }

   /**
    * Return the mean of the vector
    * @return the mean
    */
   T get_mean() const
   {
      return this->get_sum() / this->size();
   }

   /**
    * Remove an element
    * @param index is the index of the element to be removed
    */
   void remove(const size_t index)
   {
      typename std::vector<T>::iterator it = internal_vector.begin();
      /// To avoid the cast, we should use a for with ++
      it += static_cast<ptrdiff_t>(index);
      internal_vector.erase(it);
   }

   /**
    * Erase all the elements
    */
   void clear()
   {
      this->internal_vector.clear();
   }

   /**
    * Inserts a new elment at the end
    * @param elem is the element to be inserted
    */
   void push_back(const T& elem)
   {
      this->internal_vector.push_back(elem);
   }

   /**
    * Inserts or erases elements at the end such that the size becomes n
    * @param n is the new size of the vector
    * @param t is the element to be inserted
    */
   void resize(size_t n, T t = T())
   {
      this->internal_vector.resize(n, t);
   }

   /**
    * Normalize the row
    */
   void normalize()
   {
      const T sum = get_sum();
      const size_t _size = this->size();
      for(size_t index = 0; index < _size; index++)
      {
         this->internal_vector[index] /= sum;
      }
   }

   /**
    * Erase all elements
    */
   void Clear()
   {
      this->internal_vector.clear();
   }
};
#endif
