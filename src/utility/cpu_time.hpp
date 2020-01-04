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
 * @file cpu_time.hpp
 * @brief Include a set of utilities used to manage CPU time measures.
 *
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *
 */
#ifndef CPU_TIME_HPP
#define CPU_TIME_HPP

#include "config_HAVE_OPENMP.hpp"

#if HAVE_OPENMP
#include "omp.h"
#endif

#include <boost/lexical_cast.hpp>

#ifdef _WIN32
#include <windows.h>
#undef IN
#undef OUT
#else
#include <sys/times.h>
#endif

#include "dbgPrintHelper.hpp"
#include <unistd.h>

#if defined(_SC_CLK_TCK)
#define TIMES_TICKS_PER_SEC sysconf(_SC_CLK_TCK)
#elif defined(CLK_TCK)
#define TIMES_TICKS_PER_SEC CLK_TCK
#elif defined(HZ)
#define TIMES_TICKS_PER_SEC HZ
#else // !CLK_TCK && !_SC_CLK_TCK && !HZ
#define TIMES_TICKS_PER_SEC 60
#endif // !CLK_TCK && !_SC_CLK_TCK && !HZ

/**
 * return a long which represents the elapsed processor
 * time in milliseconds since some constant reference
 */
inline long int p_cpu_time()
{
#ifdef _WIN32
   FILETIME creationTime, exitTime, kernelTime, userTime;
   if(GetProcessTimes(GetCurrentProcess(), &creationTime, &exitTime, &kernelTime, &userTime))
   {
      ULARGE_INTEGER integerTime;
      integerTime.u.LowPart = userTime.dwLowDateTime;
      integerTime.u.HighPart = userTime.dwHighDateTime;
      return (long)(integerTime.QuadPart / 10000);
   }
   else
      return 0;
#else
   long t;
   struct tms now;
   clock_t ret = times(&now);
   if(ret == static_cast<clock_t>(-1))
      // cppcheck-suppress unreadVariable
      now.tms_utime = now.tms_stime = now.tms_cutime = now.tms_cstime = ret = 0;
   // cppcheck-suppress ConfigurationNotChecked
   t = (long(now.tms_utime) * 1000) / (TIMES_TICKS_PER_SEC) + (long(now.tms_cutime) * 1000) / (TIMES_TICKS_PER_SEC);
   return t;
#endif
}

/**
 *  massage a long which represents a time interval in
 *  milliseconds, into a string suitable for output
 */
inline std::string print_cpu_time(long int t)
{
   std::string ost;
   ost = boost::lexical_cast<std::string>(t / 1000) + ".";
   long centisec = (t % 1000) / 10;
   if(centisec < 10)
      ost += "0" + boost::lexical_cast<std::string>(centisec);
   else
      ost += boost::lexical_cast<std::string>(centisec);
   return ost;
}

void inline dump_exec_time(const std::string& thing, long et)
{
   // cppcheck-suppress duplicateExpression
   INDENT_OUT_MEX(0, 0, thing + ": " + print_cpu_time(et) + " seconds;");
}

/// Macro used to store the start time into time_var
#define START_TIME(time_var) time_var = p_cpu_time()

/// Macro used to store the elapsed time into time_var
#define STOP_TIME(time_var) time_var = p_cpu_time() - (time_var)

/**
 * return a long which represents the elapsed wall processor
 * time in milliseconds since some constant reference
 */
inline long int p_cpu_wtime()
{
#if HAVE_OPENMP
   return static_cast<long int>(1000 * omp_get_wtime());
#else
   return p_cpu_time();
#endif
}

/// Macro used to store the start time into time_var
#define START_WTIME(time_var) time_var = p_cpu_wtime()

/// Macro used to store the elapsed time into time_var
#define STOP_WTIME(time_var) time_var = p_cpu_wtime() - (time_var)

#endif
