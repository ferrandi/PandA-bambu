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
 *   Copyright (C) 2023-2026 Politecnico di Milano
 *
 * Part of the PandA Project, under the Apache License v2.0 with LLVM Exceptions.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
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

class time_info
{
 public:
   /// zero means that the operation is not pipelined
   static constexpr unsigned int initiation_time_DEFAULT{0u};

   /// zero means that the operation last in ceil(execution_time/clock_period)
   static constexpr unsigned int cycles_DEFAULT{0u};

   /// zero means a non-pipelined operation
   static constexpr double stage_period_DEFAULT{0.0};

   static constexpr double execution_time_DEFAULT{0.0};

 private:
   /// initiation time, in terms of cycle_units, for this type of operation on a given functional unit.
   unsigned int initiation_time{initiation_time_DEFAULT};
   /// number of cycles required to complete the computation
   unsigned int cycles{cycles_DEFAULT};
   /// flag to check if the number of cycles are dependent on the synthesis or not
   bool synthesis_dependent{false};
   /// critical timing execution path, in term of ns, of a potentially pipelined operation.
   double stage_period{stage_period_DEFAULT};
   /// execution time, in terms of ns, for this type of operation on a given functional unit.
   double execution_time{execution_time_DEFAULT};

 public:
   void set_initiation_time(unsigned int _initiation_time)
   {
      initiation_time = _initiation_time;
   }

   unsigned int get_initiation_time() const
   {
      return initiation_time;
   }

   unsigned int get_cycles() const
   {
      return cycles;
   }

   double get_stage_period() const
   {
      return stage_period;
   }

   void set_stage_period(double st_per)
   {
      stage_period = st_per;
   }

   void set_execution_time(double _execution_time, unsigned int _cycles = time_info::cycles_DEFAULT)
   {
      execution_time = _execution_time;
      cycles = _cycles;
   }

   double get_execution_time() const
   {
      return execution_time;
   }

   void set_synthesis_dependent(bool value)
   {
      synthesis_dependent = value;
   }

   bool get_synthesis_dependent() const
   {
      return synthesis_dependent;
   }
};
using time_infoRef = refcount<time_info>;

#endif
