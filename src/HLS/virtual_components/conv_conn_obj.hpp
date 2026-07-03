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
 * @file conv_conn_obj.hpp
 * @brief Class implementation of the connection module converting the type and the size of connection objects
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 *
 */
#ifndef CONV_CONN_OBJ_HPP
#define CONV_CONN_OBJ_HPP

#include "generic_obj.hpp"

/**
 * @class uu_conv_conn_obj
 * This class is used to convert unsigned into unsigned int
 */
class uu_conv_conn_obj : public generic_obj
{
   /// number of bit of in/out ports
   unsigned int bitsize;

 public:
   uu_conv_conn_obj(const std::string& _name) : generic_obj(UU_CONV_CONN_OBJ, _name), bitsize(0)
   {
   }

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

/**
 * @class ui_conv_conn_obj
 * This class is used to convert unsigned into signed int
 */
class ui_conv_conn_obj : public generic_obj
{
   /// number of bit of in/out ports
   unsigned int bitsize;

 public:
   ui_conv_conn_obj(const std::string& _name) : generic_obj(UI_CONV_CONN_OBJ, _name), bitsize(0)
   {
   }

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

/**
 * @class iu_conv_conn_obj
 * This class is used to convert signed into unsigned int
 */
class iu_conv_conn_obj : public generic_obj
{
   /// number of bit of in/out ports
   unsigned int bitsize;

 public:
   iu_conv_conn_obj(const std::string& _name) : generic_obj(IU_CONV_CONN_OBJ, _name), bitsize(0)
   {
   }

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

/**
 * @class ii_conv_conn_obj
 * This class is used to convert signed into signed int
 */
class ii_conv_conn_obj : public generic_obj
{
   /// number of bit of in/out ports
   unsigned int bitsize;

 public:
   ii_conv_conn_obj(const std::string& _name) : generic_obj(II_CONV_CONN_OBJ, _name), bitsize(0)
   {
   }

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

/**
 * @class ff_conv_conn_obj
 * This class is used to convert real into real
 */
class ff_conv_conn_obj : public generic_obj
{
   /// number of bit of in ports
   unsigned int bitsize_in;
   /// number of bit of out ports
   unsigned int bitsize_out;

 public:
   ff_conv_conn_obj(const std::string& _name) : generic_obj(FF_CONV_CONN_OBJ, _name), bitsize_in(0), bitsize_out(0)
   {
   }

   /**
    * add a size in to the component
    */
   void add_bitsize_in(unsigned int _bitsize)
   {
      bitsize_in = _bitsize > bitsize_in ? _bitsize : bitsize_in;
   }

   /**
    * add a size in to the component
    */
   void add_bitsize_out(unsigned int _bitsize)
   {
      bitsize_out = _bitsize > bitsize_out ? _bitsize : bitsize_out;
   }

   /**
    * return the input bitsize associated with the component
    */
   unsigned int get_bitsize_in() const
   {
      return bitsize_in;
   }
   /**
    * return the output bitsize associated with the component
    */
   unsigned int get_bitsize_out() const
   {
      return bitsize_out;
   }
};

/**
 * @class i_assign_conn_obj
 * This class is used to specify the type of a connection object: INT
 */
class i_assign_conn_obj : public generic_obj
{
   /// number of bit of in/out ports
   unsigned int bitsize;

 public:
   i_assign_conn_obj(const std::string& _name) : generic_obj(I_ASSIGN_CONN_OBJ, _name), bitsize(0)
   {
   }

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

/**
 * @class u_assign_conn_obj
 * This class is used to specify the type of a connection object: UINT
 */
class u_assign_conn_obj : public generic_obj
{
   /// number of bit of in/out ports
   unsigned int bitsize;

 public:
   u_assign_conn_obj(const std::string& _name) : generic_obj(U_ASSIGN_CONN_OBJ, _name), bitsize(0)
   {
   }

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

/**
 * @class f_assign_conn_obj
 * This class is used to specify the type of a connection object: REAL
 */
class f_assign_conn_obj : public generic_obj
{
   /// number of bit of in/out ports
   unsigned int bitsize;

 public:
   f_assign_conn_obj(const std::string& _name) : generic_obj(F_ASSIGN_CONN_OBJ, _name), bitsize(0)
   {
   }

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

#endif // CONV_CONN_OBJ_HPP
