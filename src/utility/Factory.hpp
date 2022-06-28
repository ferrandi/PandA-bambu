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
 *              Copyright (C) 2004-2022 Politecnico di Milano
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
 * @file Factory.hpp
 * @brief Generic factory class implementation with self-registering derived classes
 *
 * Base class of the factory should inherit from Factory and specify common
 * instanciation arguments, while derived classes should inherit from Base::Registrar
 * to be automatically registered in the factory internal registry.
 * Finally, derived classes can be instantiated through the Create method.
 *
 * @author Michele Fiorito <michele.fiorito@polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
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
      Key(){};
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