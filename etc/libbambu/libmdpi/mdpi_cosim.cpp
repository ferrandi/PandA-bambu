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
 *              Copyright (C) 2023 Politecnico di Milano
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
 * @file mdpi_cosim.cpp
 *
 * @author Michele Fiorito <michele.fiorito@polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *
 */
#include "mdpi_cosim.h"

#include "mdpi_debug.h"
#include "mdpi_wrapper.h"

#include <cassert>
#include <cstdio>
#include <pthread.h>

#ifdef __M_COSIM_ARGV
#include __M_COSIM_ARGV
#else
static const char* __m_cosim_argv[] = {"m_cosim_main"};
#endif

EXTERN_C int m_cosim_main(int argc, const char** argv);

void* __m_cosim_main(void*)
{
   int retval = -1;
   debug("Thread started\n");

   enum mdpi_state sim_state = __m_wait_for(MDPI_ENTITY_COSIM);
   if(sim_state == MDPI_COSIM_INIT)
   {
      info("Co-simulation started\n");
      retval = m_cosim_main(sizeof(__m_cosim_argv) / sizeof(*__m_cosim_argv), __m_cosim_argv);
      info("Co-simulation finished\n");
   }
   else
   {
      error("Co-simulation startup failed. Unexpected state recived from simulator: %s\n", mdpi_state_str(sim_state));
   }

   __m_signal_to(MDPI_ENTITY_SIM, MDPI_COSIM_END);
   return reinterpret_cast<void*>(static_cast<long>(((retval & 0xFF) << 8) | MDPI_COSIM_END));
}

void __m_exit(int __status)
{
   enum mdpi_state state;
   info("Exit called with value %d\n", __status);
   debug("Simulator reported state: %s\n", mdpi_state_str(state));
   __m_signal_to(MDPI_ENTITY_SIM, MDPI_COSIM_END);
   pthread_exit(reinterpret_cast<void*>(static_cast<long>(((__status & 0xFF) << 8) | MDPI_COSIM_END)));
}

void __m_abort()
{
   enum mdpi_state state;
   error("Co-simulation called abort\n");
   debug("Simulator reported state: %s\n", mdpi_state_str(state));
   __m_signal_to(MDPI_ENTITY_SIM, MDPI_COSIM_END);
   pthread_exit(reinterpret_cast<void*>(static_cast<long>(MDPI_COSIM_ABORT)));
}

void __m_assert_fail(const char* __assertion, const char* __file, unsigned int __line, const char* __function)
{
   error("%s: %d: %s: Assertion `%s' failed.\n", __file, __line, __function, __assertion);
   __m_abort();
}
