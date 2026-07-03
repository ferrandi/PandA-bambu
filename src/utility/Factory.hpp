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
