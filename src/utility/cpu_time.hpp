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
 * @file cpu_time.hpp
 * @brief Include a set of utilities used to manage CPU time measures.
 *
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 *
 */
#ifndef CPU_TIME_HPP
#define CPU_TIME_HPP

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
   {
      // cppcheck-suppress unreadVariable
      now.tms_utime = now.tms_stime = now.tms_cutime = now.tms_cstime = 0;
   }
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
   ost = std::to_string(t / 1000) + ".";
   long centisec = (t % 1000) / 10;
   if(centisec < 10)
   {
      ost += "0" + std::to_string(centisec);
   }
   else
   {
      ost += std::to_string(centisec);
   }
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
   return p_cpu_time();
}

/// Macro used to store the start time into time_var
#define START_WTIME(time_var) time_var = p_cpu_wtime()

/// Macro used to store the elapsed time into time_var
#define STOP_WTIME(time_var) time_var = p_cpu_wtime() - (time_var)

#endif
