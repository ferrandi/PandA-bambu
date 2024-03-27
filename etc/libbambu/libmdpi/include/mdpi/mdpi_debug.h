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

#include "mdpi_types.h"

#include <pthread.h>

#ifdef __cplusplus
#include <cstdio>
#else
#include <stdio.h>
#endif

#ifndef __LOCAL_ENTITY
#error Must define __LOCAL_ENTITY for debug prints
#endif

#ifndef __M_OUT_LVL
#define __M_OUT_LVL 4
#endif

#if __M_OUT_LVL >= 3
#define info(str, ...) fprintf(stdout, "%s: " str, mdpi_entity_str(__LOCAL_ENTITY), ##__VA_ARGS__)
#define info_append(str, ...) fprintf(stdout, str, ##__VA_ARGS__)
#else
#define info(...)
#define info_append(...)
#endif

#if __M_OUT_LVL > 4
#define debug(str, ...) fprintf(stdout, "%s %10s: " str, mdpi_entity_str(__LOCAL_ENTITY), __func__, ##__VA_ARGS__)
#define debug_append(str, ...) fprintf(stdout, str, ##__VA_ARGS__)
#define error(str, ...) debug("ERROR: " str, ##__VA_ARGS__)
#else
#define debug(...)
#define debug_append(...)
#define error(str, ...) fprintf(stderr, "ERROR: %s: " str, mdpi_entity_str(__LOCAL_ENTITY), ##__VA_ARGS__)
#endif

#endif // __MDPI_DEBUG_H