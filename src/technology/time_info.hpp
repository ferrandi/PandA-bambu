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
 *              Copyright (C) 2023-2024 Politecnico di Milano
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
 * @file time_info.hpp
 * @brief Collect information about resource performance
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 *
 */
#ifndef TIME_INFO_HPP
#define TIME_INFO_HPP

#include "refcount.hpp"
#include "schedule.hpp"

/// refcount definition for the class
CONSTREF_FORWARD_DECL(Parameter);
UINT_STRONG_TYPEDEF_FORWARD_DECL(ControlStep);
REF_FORWARD_DECL(time_info);
class xml_element;

class time_info
{
   /// class containing all the parameters
   const ParameterConstRef Param;
   /// initiation time, in terms of cycle_units, for this type of operation on a given functional unit.
   ControlStep initiation_time;
   /// number of cycles required to complete the computation
   unsigned int cycles;
   /// flag to check if the number of cycles are dependent on the synthesis or not
   bool synthesis_dependent;
   /// critical timing execution path, in term of ns, of a potentially pipelined operation.
   double stage_period;
   /// execution time, in terms of ns, for this type of operation on a given functional unit.
   double execution_time;

 public:
   /**
    * Constructor.
    */
   explicit time_info(const ParameterConstRef _Param);

   /**
    * Destructor.
    */
   ~time_info();

   static time_infoRef factory(const ParameterConstRef Param);

   void set_initiation_time(const ControlStep _initiation_time);

   ControlStep get_initiation_time() const;

   unsigned int get_cycles() const;

   double get_stage_period() const;

   void set_stage_period(double st_per);

   void set_execution_time(double execution_time, unsigned int cycles = time_info::cycles_time_DEFAULT);

   void set_synthesis_dependent(bool value);

   bool get_synthesis_dependent() const;

   double get_execution_time() const;

 public:
   /**
    * @name Default values associated with the members of the operation class.
    */
   //@{
   static const double execution_time_DEFAULT /*= 0.0*/;
   static const ControlStep initiation_time_DEFAULT /*= 0*/;
   static const unsigned int cycles_time_DEFAULT /*=0*/;
   static const double stage_period_DEFAULT /*= 0.0*/;
   //@}
};
/// refcount definition of the class
using time_infoRef = refcount<time_info>;

#endif
