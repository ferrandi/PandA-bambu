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
 * @file time_model.hpp
 * @brief Class specification for time_model
 *
 * @author Christian Pilato <pilato@elet.polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *
 */
#ifndef _TIME_MODEL_HPP_
#define _TIME_MODEL_HPP_

#include <boost/version.hpp>

#if BOOST_VERSION >= 104600
#else
#include <tuple>
#define BOOST_TR1_TUPLE_HPP_INCLUDED
#endif
#include <boost/math/distributions.hpp>

#include "custom_map.hpp"
#include <string>
#include <vector>

#include "target_device.hpp"

/// HLS/scheduling include
#include "schedule.hpp"

/// utility includes
#include "refcount.hpp"
#include "utility.hpp"

/// refcount definition for the class
CONSTREF_FORWARD_DECL(Parameter);
UINT_STRONG_TYPEDEF_FORWARD_DECL(ControlStep);
REF_FORWARD_DECL(time_model);
class xml_element;

class time_model
{
 public:
   /// statistical delay for this type of operation on a given functional unit.
   boost::math::normal statistical_delay;

   /// map representing the pin-to-pin delay
   std::map<std::string, std::map<std::string, double>> pin_to_pin_delay;

   /// type of the timing path
   typedef enum
   {
      POST_SYNTHESIS = 1,
      POST_LAYOUT = 2
   } path_t;

 protected:
   /// class containing all the parameters
   const ParameterConstRef Param;

   /// map containing all the information about critical paths
   std::map<unsigned int, std::vector<std::string>> critical_paths;

   std::map<unsigned int, float> max_delay;

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
   explicit time_model(const ParameterConstRef Param);

   /**
    * Destructor.
    */
   virtual ~time_model();

   static time_modelRef create_model(TargetDevice_Type dv_type, const ParameterConstRef Param);

   virtual void xwrite(xml_element* pin_node, const std::string& output_pin) = 0;

   virtual unsigned int xload_timing_path(xml_element* node);

   std::vector<std::string> get_critical_path(unsigned int type) const;

   bool has_max_delay(unsigned int type) const;

   void set_max_delay(unsigned int type, float value);

   float get_max_delay(unsigned int type) const;

   void set_initiation_time(const ControlStep _initiation_time);

   ControlStep get_initiation_time() const;

   unsigned int get_cycles() const;

   double get_stage_period() const;

   void set_stage_period(double st_per);

   void set_execution_time(double execution_time, unsigned int cycles);

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
typedef refcount<time_model> time_modelRef;

#endif
