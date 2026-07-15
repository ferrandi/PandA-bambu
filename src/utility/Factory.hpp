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
 * @file Factory.hpp
 * @brief Generic factory class implementation with self-registering derived classes
 *
 * Base class of the factory should inherit from Factory and specify common
 * instantiation arguments, while derived classes should inherit from Base::Registrar
 * to be automatically registered in the factory internal registry.
 * Finally, derived classes can be instantiated through the Create method.
 *
 * @author Michele Fiorito <michele.fiorito@polimi.it>
 *
 */
#ifndef FACTORY_HPP
#define FACTORY_HPP

#include "custom_map.hpp"
#include "refcount.hpp"
#include "string_manipulation.hpp"

template <class Base, class... Args>
class Factory
{
 public:
   template <class... T>
   static refcount<Base> Create(const std::string& s, T&&... args)
   {
      const auto& registry = Registry();
      const auto it = registry.find(s);
      if(it != registry.end())
      {
         return it->second(std::forward<T>(args)...);
      }
      return nullptr;
   }

   template <class T>
   struct Registrar : Base
   {
      friend T;

      static bool RegisterType()
      {
         const auto name = GET_CLASS(T);
         Factory::Registry()[name] = [](Args... args) -> refcount<Base> {
            return refcount<T>(new T(std::forward<Args>(args)...));
         };
         return true;
      }
      static bool __registered;

    private:
      Registrar(Args... args) : Base(Key{}, std::forward<Args>(args)...)
      {
         (void)__registered;
      }
   };

   friend Base;

 private:
   class Key
   {
      Key() = default;

      template <class T>
      friend struct Registrar;
   };
   using FuncType = refcount<Base> (*)(Args...);
   Factory() = default;

   static auto& Registry()
   {
      static CustomUnorderedMap<std::string, FuncType> registry;
      return registry;
   }
};

template <class Base, class... Args>
template <class T>
bool Factory<Base, Args...>::Registrar<T>::__registered = Factory<Base, Args...>::Registrar<T>::RegisterType();

#endif
