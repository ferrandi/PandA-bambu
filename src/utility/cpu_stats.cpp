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
 * @file cpu_stats.cpp
 * @brief Utility managing CPU statistics.
 *
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *
 */
#include "cpu_stats.hpp"
#include "string_manipulation.hpp"
#ifdef _WIN32
#include <windows.h>

#include <psapi.h>
#include <winsock2.h>
#else
#include <sys/resource.h>
#include <sys/time.h>
#endif
#include <unistd.h>
#pragma GCC diagnostic ignored "-Wold-style-cast"
#pragma GCC diagnostic ignored "-Wmissing-field-initializers"

std::string PrintVirtualDataMemoryUsage()
{
#ifdef _WIN32
   return STR("unavailable");
#elif defined(__APPLE__)
   return STR("unavailable");
#else
   extern int end, etext, edata;
   long vm_init_data, vm_uninit_data, vm_sbrk_data;
   long int temp;
   /* Get the virtual memory sizes */
   temp = (long)(&edata) - (long)(&etext);
   vm_init_data = temp / 1024 + (((temp % 1024) > 512) ? 1 : 0);
   temp = (long)(&end) - (long)(&edata);
   vm_uninit_data = temp / 1024 + (((temp % 1024) > 512) ? 1 : 0);
   temp = (long)sbrk(0) - (long)(&end);
   vm_sbrk_data = temp / 1024 + (((temp % 1024) > 512) ? 1 : 0);
   return STR((vm_init_data + vm_uninit_data + vm_sbrk_data) / 1024) + "MB";
#endif
}

void util_print_cpu_stats(std::ostream& os)
{
#ifdef _WIN32
   char hostname[257];
   WSADATA wsaData;
   FILETIME creationTime, exitTime, kernelTime, userTime;
   double user, system;
   MEMORYSTATUSEX statex;
   size_t vm_limit;
   PROCESS_MEMORY_COUNTERS pmc;
   size_t peak_working_set;
   long page_faults;

   /* Get the hostname */
   WSAStartup(MAKEWORD(2, 2), &wsaData);
   (void)gethostname(hostname, sizeof(hostname));
   hostname[sizeof(hostname) - 1] = '\0'; /* just in case */
   WSACleanup();

   /* Get usage stats */
   if(GetProcessTimes(GetCurrentProcess(), &creationTime, &exitTime, &kernelTime, &userTime))
   {
      ULARGE_INTEGER integerSystemTime, integerUserTime;
      integerUserTime.u.LowPart = userTime.dwLowDateTime;
      integerUserTime.u.HighPart = userTime.dwHighDateTime;
      user = (double)integerUserTime.QuadPart * 1e-7;
      integerSystemTime.u.LowPart = kernelTime.dwLowDateTime;
      integerSystemTime.u.HighPart = kernelTime.dwHighDateTime;
      system = (double)integerSystemTime.QuadPart * 1e-7;
   }
   else
   {
      user = system = 0.0;
   }
   statex.dwLength = sizeof(statex);
   if(GlobalMemoryStatusEx(&statex))
   {
      vm_limit = (size_t)(statex.ullTotalVirtual / 1024.0 + 0.5);
   }
   else
   {
      vm_limit = 0;
   }
   if(GetProcessMemoryInfo(GetCurrentProcess(), &pmc, sizeof(pmc)))
   {
      peak_working_set = (size_t)(pmc.PeakWorkingSetSize / 1024.0 + 0.5);
      page_faults = (long)pmc.PageFaultCount;
   }
   else
   {
      peak_working_set = 0;
      page_faults = 0;
   }
   os << "Runtime Statistics\n";
   os << "------------------\n";
   os << "Machine name: " << hostname << std::endl;
   os << "User time   " << user << " seconds\n";
   os << "System time " << system << " seconds\n\n";
   os << "Maximum resident size            = ";
   if(peak_working_set == 0)
      os << "unavailable\n";
   else
      os << peak_working_set << "\n";
   os << "Virtual memory limit             = ";
   if(vm_limit == 0)
      os << "unavailable\n";
   else
      os << vm_limit << "\n";
   os << "Page faults       = " << page_faults << "\n";
#elif defined(__APPLE__)
   ; // do nothing
#else
   extern int end, etext, edata;
   struct rusage rusage
   {
   };
   struct rlimit rlp
   {
   };
   int text, data, stack;
   rlim_t vm_limit, vm_soft_limit;
   long double user, system, scale;
   long int temp;
   char hostname[257];
   long vm_text, vm_init_data, vm_uninit_data, vm_sbrk_data;

   /* Get the hostname */
   (void)gethostname(hostname, 256);
   hostname[256] = '\0'; /* just in case */

   /* Get the virtual memory sizes */
   temp = (long)(&etext);
   vm_text = temp / 1024 + (((temp % 1024) > 512) ? 1 : 0);
   temp = (long)(&edata) - (long)(&etext);
   vm_init_data = temp / 1024 + (((temp % 1024) > 512) ? 1 : 0);
   temp = (long)(&end) - (long)(&edata);
   vm_uninit_data = temp / 1024 + (((temp % 1024) > 512) ? 1 : 0);
   temp = (long)sbrk(0) - (long)(&end);
   vm_sbrk_data = temp / 1024 + (((temp % 1024) > 512) ? 1 : 0);

   /* Get virtual memory limits */
   (void)getrlimit(RLIMIT_DATA, &rlp);
   vm_limit = rlp.rlim_max / 1024 + (((rlp.rlim_max % 1024) > 512) ? 1 : 0);
   vm_soft_limit = rlp.rlim_cur / 1024 + (((rlp.rlim_cur % 1024) > 512) ? 1 : 0);

   /* Get usage stats */
   (void)getrusage(RUSAGE_SELF, &rusage);
   user = rusage.ru_utime.tv_sec + rusage.ru_utime.tv_usec / 1000000;
   system = rusage.ru_stime.tv_sec + rusage.ru_stime.tv_usec / 1000000;
   scale = (user + system) * 100.0L;
   if(scale == 0.0L)
   {
      scale = 0.001L;
   }

   os << "Runtime Statistics\n";
   os << "------------------\n";
   os << "Machine name: " << hostname << std::endl;
   os << "User time   " << user << " seconds\n";
   os << "System time " << system << " seconds\n\n";

   text = (int)(rusage.ru_ixrss / scale + 0.5L);
   data = (int)((rusage.ru_idrss) / scale + 0.5L);
   stack = (int)((rusage.ru_isrss) / scale + 0.5L);
   os << "Average resident text size       = " << text << "K\n";
   os << "Average resident data size = " << data << "K\n";
   os << "Average resident stack size = " << stack << "K\n";
   os << "Maximum resident size            = " << rusage.ru_maxrss / 2 << "K\n\n";
   os << "Virtual text size                = " << vm_text << "K\n";
   os << "Virtual data size                = " << vm_init_data + vm_uninit_data + vm_sbrk_data << "K\n";
   os << "    data size initialized        = " << vm_init_data << "K\n";
   os << "    data size uninitialized      = " << vm_uninit_data << "K\n";
   os << "    data size sbrk               = " << vm_sbrk_data << "K\n";
   os << "Virtual memory limit             = ";
   if(rlp.rlim_cur == RLIM_INFINITY)
   {
      os << "unlimited";
   }
   else
   {
      os << vm_soft_limit << "K";
   }
   os << " (";
   if(rlp.rlim_max == RLIM_INFINITY)
   {
      os << "unlimited";
   }
   else
   {
      os << vm_limit << "K";
   }
   os << ")\n\n";

   os << "Major page faults = " << rusage.ru_majflt << "\n";
   os << "Minor page faults = " << rusage.ru_minflt << "\n";
   os << "Swaps = " << rusage.ru_nswap << "\n";
   os << "Input blocks = " << rusage.ru_inblock << "\n";
   os << "Output blocks = " << rusage.ru_oublock << "\n";
   os << "Context switch (voluntary) = " << rusage.ru_nvcsw << "\n";
   os << "Context switch (involuntary) = " << rusage.ru_nivcsw << "\n";
#endif
}
