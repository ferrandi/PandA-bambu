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

EXTERN_C int m_cosim_main();

void* __m_cosim_main(void*)
{
   int retval = -1;
   debug("Thread started\n");

   enum mdpi_state sim_state = __m_wait_for(MDPI_ENTITY_COSIM);
   if(sim_state == MDPI_COSIM_INIT)
   {
      debug("Co-simulation started\n");
      retval = m_cosim_main();
      debug("Co-simulation finished\n");
   }
   else
   {
      error("Co-simulation startup failed. Unexpected state recived from simulator: %s\n", mdpi_state_str(sim_state));
   }

   __m_signal_to(MDPI_ENTITY_SIM, MDPI_COSIM_END);
   pthread_exit((void*)static_cast<ptr_t>((retval & 0xFF) << 8 | MDPI_COSIM_END));
   return NULL;
}

void __m_exit(int __status)
{
   enum mdpi_state state;
   debug("Exit called with value %d\n", __status);
   debug("Waiting for simulator to complete...\n");
   state = __m_wait_for(MDPI_ENTITY_COSIM);
   debug("Simulator reported state: %s\n", mdpi_state_str(state));
   __m_signal_to(MDPI_ENTITY_SIM, MDPI_COSIM_END);
   pthread_exit((void*)static_cast<ptr_t>((__status & 0xFF) << 8 | MDPI_COSIM_END));
}

void __m_abort()
{
   enum mdpi_state state;
   error("Co-simulation called abort\n");
   debug("Waiting for simulator to complete...\n");
   state = __m_wait_for(MDPI_ENTITY_COSIM);
   debug("Simulator reported state: %s\n", mdpi_state_str(state));
   __m_signal_to(MDPI_ENTITY_SIM, MDPI_COSIM_END);
   pthread_exit((void*)static_cast<ptr_t>(MDPI_COSIM_ABORT));
}
