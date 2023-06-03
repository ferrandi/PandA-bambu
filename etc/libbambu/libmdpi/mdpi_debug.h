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
 * @file mdpi_debug.h
 *
 * @author Michele Fiorito <michele.fiorito@polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *
 */
#ifndef __MDPI_DEBUG_H
#define __MDPI_DEBUG_H

#include <pthread.h>

#ifdef __cplusplus
extern "C"
#else
extern
#endif
    volatile pthread_t __m_main_tid;

#ifndef NDEBUG
#ifdef __cplusplus
#include <cstdio>
#else
#include <stdio.h>
#endif

#define debug(str, ...) \
   fprintf(stdout, "%s %10s: " str, __m_main_tid == pthread_self() ? "Sim" : "Co-sim", __func__, ##__VA_ARGS__)
#define error(str, ...) \
   fprintf(stdout, "ERROR: %s %10s: " str, __m_main_tid == pthread_self() ? "Sim" : "Co-sim", __func__, ##__VA_ARGS__)
#else
#define debug(...)
#define error(str, ...) \
   fprintf(stderr, "ERROR: %s %10s: " str, __m_main_tid == pthread_self() ? "Sim" : "Co-sim", __func__, ##__VA_ARGS__)
#endif

#endif // __MDPI_DEBUG_H