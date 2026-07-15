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
 *   Copyright (C) 2015-2026 Politecnico di Milano
 *
 * Part of the PandA Project, under the Apache License v2.0 with LLVM Exceptions.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 *
 */
/**
 * @file generic_obj.cpp
 * @brief Base class for all resources into datapath
 *
 *
 *
 * @author Christian Pilato <pilato@elet.polimi.it>
 * @author Marco Lattuada <marco.lattuada@polimi.it>
 *
 */
#include "generic_obj.hpp"

#include "exceptions.hpp"

#if !HAVE_UNORDERED
GenericObjSorter::GenericObjSorter() = default;

bool GenericObjSorter::operator()(const generic_objRef& x, const generic_objRef& y) const
{
   if(x == y)
   {
      return false;
   }
   THROW_ASSERT(x->get_string() != y->get_string() or x->get_string().find("CONSTANT") != std::string::npos,
                x->get_string());
   return x->get_string() < y->get_string();
}

GenericObjUnsignedIntSorter::GenericObjUnsignedIntSorter() = default;

bool GenericObjUnsignedIntSorter::operator()(const std::pair<generic_objRef, int>& x,
                                             const std::pair<generic_objRef, int>& y) const
{
   if(x.first == y.first)
   {
      return x.second < y.second;
   }
   THROW_ASSERT(x.first->get_string() != y.first->get_string(), x.first->get_string());
   return x.first->get_string() < y.first->get_string();
}

#endif

bool generic_obj::operator<(const generic_obj& other) const
{
   return get_string() < other.get_string();
}
