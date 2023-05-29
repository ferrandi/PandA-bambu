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
 * @file mdpi.h
 *
 * @author Michele Fiorito <michele.fiorito@polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *
 */
#ifndef __MDPI_H
#define __MDPI_H

#include "mdpi_types.h"
#include <svdpi.h>

EXTERN_C EXPORT void m_init();
EXTERN_C EXPORT unsigned int m_next(unsigned int state);
EXTERN_C EXPORT int m_fini();

EXTERN_C EXPORT void m_getarg(svLogicVecVal* data, unsigned int index);
EXTERN_C EXPORT void m_setarg(CONSTARG svLogicVecVal* data, unsigned int index);
EXTERN_C EXPORT unsigned int m_getptrargsize(unsigned int index);

EXTERN_C EXPORT void m_read8(svLogicVecVal* data, ptr_t addr);
EXTERN_C EXPORT void m_read16(svLogicVecVal* data, ptr_t addr);
EXTERN_C EXPORT void m_read32(svLogicVecVal* data, ptr_t addr);
EXTERN_C EXPORT void m_read64(svLogicVecVal* data, ptr_t addr);
EXTERN_C EXPORT void m_read128(svLogicVecVal* data, ptr_t addr);
EXTERN_C EXPORT void m_read256(svLogicVecVal* data, ptr_t addr);
EXTERN_C EXPORT void m_read512(svLogicVecVal* data, ptr_t addr);
EXTERN_C EXPORT void m_read1024(svLogicVecVal* data, ptr_t addr);

EXTERN_C EXPORT void m_write8(unsigned short size, CONSTARG svLogicVecVal* data, ptr_t addr);
EXTERN_C EXPORT void m_write16(unsigned short size, CONSTARG svLogicVecVal* data, ptr_t addr);
EXTERN_C EXPORT void m_write32(unsigned short size, CONSTARG svLogicVecVal* data, ptr_t addr);
EXTERN_C EXPORT void m_write64(unsigned short size, CONSTARG svLogicVecVal* data, ptr_t addr);
EXTERN_C EXPORT void m_write128(unsigned short size, CONSTARG svLogicVecVal* data, ptr_t addr);
EXTERN_C EXPORT void m_write256(unsigned short size, CONSTARG svLogicVecVal* data, ptr_t addr);
EXTERN_C EXPORT void m_write512(unsigned short size, CONSTARG svLogicVecVal* data, ptr_t addr);
EXTERN_C EXPORT void m_write1024(unsigned short size, CONSTARG svLogicVecVal* data, ptr_t addr);

#endif // __MDPI_H
