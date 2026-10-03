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
 * @file refcount.hpp
 * @brief Template definition of refcount.
 *
 * This structure is used each time an auto-deallocating class is needed.
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
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
 * Template function used to hide dynamic_cast.
 * The template parameter T represents a type of an object having U as a base class.
 * @param t is a reference-counted handle containing the pointer to an object of type U
 * @return the pointer casted to the T class
 */
template <class T, class U>
inline T* GetPointer(const refcount<U>& t)
{
   return dynamic_cast<T*>(t.get());
}

/**
 * Template function used to hide static_cast.
 * The template parameter T represents a type of an object having U as a base class.
 * @param t is a reference-counted handle containing the pointer to an object of type U
 * @return the pointer casted to the T class
 */
template <class T, class U>
inline T* GetPointerS(const refcount<U>& t)
{
   return static_cast<T*>(t.get());
}

#endif // REFCOUNT_HPP
