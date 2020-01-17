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

#include <boost/version.hpp>

/// null deleter
struct null_deleter
{
   /// deallocator
   void operator()(void const*) const
   {
   }
};

#if !defined(USE_REFCOUNT_POINTERS) && BOOST_VERSION >= 103300
#if __cplusplus > 199711L
#include <memory>
#define refcount std::shared_ptr
#define Wrefcount std::weak_ptr
#define RefcountCast std::dynamic_pointer_cast

#else
#include <boost/smart_ptr/shared_ptr.hpp> // for shared_ptr
#include <boost/smart_ptr/weak_ptr.hpp>   // for weak_ptr
#include <cstddef>                        // for size_t

#define refcount boost::shared_ptr
#define Wrefcount boost::weak_ptr
#define RefcountCast boost::dynamic_pointer_cast
#endif
#else
template <class T>
class Wrefcount;

/**
 * Template borrowed from the ANTLR library by Terence
 * Parr (http://www.jGuru.com - Software rights:
 * http://www.antlr.org/license.html). \c refcount is a simple
 * template class useful for passing references to function templates
 * (algorithms) that would usually take copies of their arguments and
 * to get transparent to the programmer all the allocation and
 * deallocation issues.
 * The original class has been extended to support refcount object that
 * does not deallocate the ptr object. This can be useful in case of
 * circular references. In particular, this implementation works
 * similarly to the smart_pointer object when a null deleter is passed
 * to the contructor.
 */
template <typename T>
struct refcount
{
 private:
   friend class Wrefcount<T>;
   struct Ref
   {
      T* const ptr;
      unsigned int count : 31;
      unsigned int deleter : 1;
      // cppcheck-suppress noExplicitConstructor
      Ref(T* p) : ptr(p), count(1), deleter(1)
      {
      }
      template <class null_deleter>
      Ref(T* p, null_deleter& d) : ptr(p), count(1), deleter(0)
      {
      }
      ~Ref()
      {
         if(deleter)
            delete ptr;
      }
      Ref* increment()
      {
         ++count;
         return this;
      }
      bool decrement()
      {
         return (--count == 0);
      }
   } * ref;

 public:
   explicit refcount(T* p = 0) : ref(p ? new Ref(p) : 0)
   {
   }
   template <class null_deleter>
   explicit refcount(T* p, null_deleter& d) : ref(p ? new Ref(p, d) : 0)
   {
   }
   // cppcheck-suppress noExplicitConstructor
   refcount(const refcount<T>& other) : ref(other.ref ? other.ref->increment() : 0)
   {
   }
   ~refcount()
   {
      if(ref && ref->decrement())
         delete ref;
   }
   // cppcheck-suppress operatorEqToSelf
   refcount<T>& operator=(const refcount<T>& other)
   {
      Ref* tmp = other.ref ? other.ref->increment() : 0;
      if(ref && ref->decrement())
         delete ref;
      ref = tmp;
      return *this;
   }
   bool operator==(const refcount<T>& other)
   {
      return this->get() == other.get();
   }
   operator T*() const
   {
      return ref ? ref->ptr : 0;
   }
   operator Wrefcount<T>() const
   {
      return Wrefcount<T>(get());
   }
   T* operator->() const
   {
      return ref ? ref->ptr : 0;
   }
   T* get() const
   {
      return ref ? ref->ptr : 0;
   }
   template <class newType>
   operator refcount<newType>()
   {
      return refcount<newType>(ref);
   }
};

template <typename T>
// cppcheck-suppress copyCtorAndEqOperator
struct Wrefcount
{
 private:
   T* ptr;

 public:
   Wrefcount() : ptr(0)
   {
   }
   // cppcheck-suppress noExplicitConstructor
   Wrefcount(Wrefcount<T> const& other) : ptr(other.ptr)
   {
   }
   // cppcheck-suppress noExplicitConstructor
   Wrefcount(refcount<T> const& other) : ptr(other.get())
   {
   }
   ~Wrefcount()
   {
   }
   Wrefcount<T>& operator=(Wrefcount<T> const& other)
   {
      ptr = other.ptr;
      return *this;
   }
   Wrefcount<T>& operator=(refcount<T> const& other)
   {
      ptr = other.get();
      return *this;
   }
   refcount<T> lock() const
   {
      null_deleter nullDel;
      return refcount<T>(ptr, nullDel);
   }
};

#endif
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

#include <functional>
#if !defined(USE_REFCOUNT_POINTERS) && BOOST_VERSION >= 103300 && __cplusplus > 199711L
#else
namespace std
{
   template <typename T>
   struct hash<refcount<T>> : public std::unary_function<refcount<T>, std::size_t>
   {
      std::size_t operator()(const refcount<T>& val) const
      {
         std::hash<const void*> hasher;
         return hasher(val.get());
      }
   };
} // namespace std
#endif

/**
 * The key comparison function for refcount
 */
template <typename T>
class RefCountOrder : std::binary_function<refcount<T>, refcount<T>, bool>
{
 public:
   /**
    * Constructor
    */
   RefCountOrder<T>() = default;

   /**
    * Compare of refcount object
    */
   bool operator()(const refcount<T> x, const refcount<T> y) const
   {
      return *(x.get()) < *(y.get());
   }
};
#endif
