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
 * @file conv_conn_obj.hpp
 * @brief Class implementation of the connection module converting the type and the size of connection objects
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
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
   /**
    * Constructor
    */
   uu_conv_conn_obj(const std::string& _name) : generic_obj(UU_CONV_CONN_OBJ, _name), bitsize(0)
   {
   }

   /**
    * Destructor.
    */
   ~uu_conv_conn_obj() override = default;

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
   /**
    * Constructor
    */
   ui_conv_conn_obj(const std::string& _name) : generic_obj(UI_CONV_CONN_OBJ, _name), bitsize(0)
   {
   }

   /**
    * Destructor.
    */
   ~ui_conv_conn_obj() override = default;

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
   /**
    * Constructor
    */
   iu_conv_conn_obj(const std::string& _name) : generic_obj(IU_CONV_CONN_OBJ, _name), bitsize(0)
   {
   }

   /**
    * Destructor.
    */
   ~iu_conv_conn_obj() override = default;

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
   /**
    * Constructor
    */
   ii_conv_conn_obj(const std::string& _name) : generic_obj(II_CONV_CONN_OBJ, _name), bitsize(0)
   {
   }

   /**
    * Destructor.
    */
   ~ii_conv_conn_obj() override = default;

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
   /**
    * Constructor
    */
   ff_conv_conn_obj(const std::string& _name) : generic_obj(FF_CONV_CONN_OBJ, _name), bitsize_in(0), bitsize_out(0)
   {
   }

   /**
    * Destructor.
    */
   ~ff_conv_conn_obj() override = default;

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
   /**
    * Constructor
    */
   i_assign_conn_obj(const std::string& _name) : generic_obj(I_ASSIGN_CONN_OBJ, _name), bitsize(0)
   {
   }

   /**
    * Destructor.
    */
   ~i_assign_conn_obj() override = default;

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
   /**
    * Constructor
    */
   u_assign_conn_obj(const std::string& _name) : generic_obj(U_ASSIGN_CONN_OBJ, _name), bitsize(0)
   {
   }

   /**
    * Destructor.
    */
   ~u_assign_conn_obj() override = default;

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
 * @class vb_assign_conn_obj
 * This class is used to specify the type of a connection object: VECTOR_BOOL
 */
class vb_assign_conn_obj : public generic_obj
{
   /// number of bit of in/out ports
   unsigned int bitsize;

 public:
   /**
    * Constructor
    */
   vb_assign_conn_obj(const std::string& _name) : generic_obj(VB_ASSIGN_CONN_OBJ, _name), bitsize(0)
   {
   }

   /**
    * Destructor.
    */
   ~vb_assign_conn_obj() override = default;

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
   /**
    * Constructor
    */
   f_assign_conn_obj(const std::string& _name) : generic_obj(F_ASSIGN_CONN_OBJ, _name), bitsize(0)
   {
   }

   /**
    * Destructor.
    */
   ~f_assign_conn_obj() override = default;

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
