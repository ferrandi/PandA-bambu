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
 * @file mux_obj.hpp
 * @brief Base class for multiplexer into datapath
 *
 * @author Christian Pilato <pilato@elet.polimi.it>
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *
 */

#ifndef MUX_OBJ_HPP
#define MUX_OBJ_HPP

#include "refcount.hpp"
#include <boost/lexical_cast.hpp>

#include "generic_obj.hpp"
#include "graph.hpp"

/**
 * @class mux_obj
 * This class is a specialization of generic_obj class to represent a multiplexer into the datapath
 */
class mux_obj : public generic_obj
{
   /// number of bit of in/out ports
   unsigned int bitsize;

   /// reference to generic_obj associated with first input (when selector is TRUE, it's connected to out)
   const generic_objRef first;

   /// reference to generic_obj associated with second input (when selector is FALSE, it's connected to out)
   const generic_objRef second;

   /// reference to generic_obj associated with the multiplexer
   generic_objRef target;

   /// reference to generic_obj target of the mux tree's dataflow
   generic_objRef tree_target;

   /// selector object. It can be both a port or the root of a logic gate
   generic_objRef selector;

   /// depth level of the mux
   unsigned int level;

 public:
   /**
    * This is the constructor of the object class. It initializes type for generic_obj superclass
    * @param first is reference to first input
    * @param second is reference to second input
    * @param level is the mux level
    * @param name is the id
    * @param target is the overall mux tree target
    */
   mux_obj(const generic_objRef first, const generic_objRef second, unsigned int level, const std::string& name, const generic_objRef overall_target);

   /**
    * Destructor.
    */
   ~mux_obj() override;

   /**
    * Sets target object for multiplexer
    * @param tgt is reference to generic_obj where multiplexer output is connected
    */
   void set_target(const generic_objRef tgt);

   /**
    * Returns the object which will receive the mux tree result
    * @return the target of the mux tree
    */
   generic_objRef get_final_target();

   /**
    * Gets the selector
    * @return a reference to the component representing the selector
    */
   generic_objRef GetSelector() const;

   /**
    * Sets the element representing the selector
    * @param is the reference to the new selector for the multiplexer
    */
   void set_selector(const generic_objRef sel);

   /**
    * Return the level of the multiplexer
    */
   unsigned int get_level() const;

   /**
    * add a size to the component
    */
   void add_bitsize(unsigned int _bitsize)
   {
      bitsize = _bitsize > bitsize ? _bitsize : bitsize;
   }

   /**
    * return the maximum bitsize associated with the component
    */
   unsigned int get_bitsize() const
   {
      return bitsize;
   }
};

/// RefCount definition for the class
typedef refcount<mux_obj> mux_objRef;

#endif
