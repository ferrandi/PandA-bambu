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
 *              Copyright (C) 2004-2024 Politecnico di Milano
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
 * @file refcount.hpp
 * @brief Template definition of refcount.
 *
 * This structure is used each time an auto-deallocating class is needed.
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *
 */
#ifndef REFCOUNT_HPP
#define REFCOUNT_HPP
#include <memory>

/// null deleter
struct null_deleter
{
   /// deallocator
   void operator()(void const*) const
   {
   }
};

template <typename T>
using refcount = std::shared_ptr<T>;

template <typename T>
using Wrefcount = std::weak_ptr<T>;

#define RefcountCast std::dynamic_pointer_cast

/**
 * Macro used to forward declared a Refcount type
 */
#define REF_FORWARD_DECL(obj) \
   class obj;                 \
   typedef refcount<obj> obj##Ref
#define CONSTREF_FORWARD_DECL(obj) \
   class obj;                      \
   typedef refcount<const obj> obj##ConstRef

/**
 * Template function used to hide dynamic_cast
 * The template parameter T represents a type of an object having as a base class U.
 * @param t is a refcount<U> containing the pointer
 * @return the pointer casted to the T class
 */
template <class T, class U>
inline T* GetPointer(const refcount<U>& t)
{
   return dynamic_cast<T*>(t.get());
}

/**
 * Template function used to hide static_cast
 * The template parameter T represents a type of an object having as a base class U.
 * @param t is a refcount<U> containing the pointer
 * @return the pointer casted to the T class
 */
template <class T, class U>
inline T* GetPointerS(const refcount<U>& t)
{
   return static_cast<T*>(t.get());
}

#endif // REFCOUNT_HPP
