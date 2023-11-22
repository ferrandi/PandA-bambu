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
 * @file mdpi_ipc.h
 *
 * @author Michele Fiorito <michele.fiorito@polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *
 */
#ifndef __MDPI_IPC_H
#define __MDPI_IPC_H

#ifndef __M_IPC_FILENAME
#define __M_IPC_FILENAME "/tmp/panda_ipc_mmap"
#endif

#ifndef __M_IPC_SIM_CMD_ENV
#define __M_IPC_SIM_CMD_ENV "M_IPC_SIM_CMD"
#endif

#define IPC_STRUCT_ATTR __attribute__((aligned(8), packed))

#include "mdpi_debug.h"
#include "mdpi_types.h"

typedef enum
{
   MDPI_IPC_STATE_FREE = 0,
   MDPI_IPC_STATE_WRITING,
   MDPI_IPC_STATE_REQUEST,
   MDPI_IPC_STATE_RESPONSE
} mdpi_ipc_state_t;

// clang-format off
typedef enum
{
   MDPI_STATE_UNDEFINED = 0,
   MDPI_STATE_READY     = 1,
   MDPI_STATE_SETUP     = 1 << 1,
   MDPI_STATE_RUNNING   = 1 << 2,
   MDPI_STATE_END       = 1 << 3,
   MDPI_STATE_ERROR     = 1 << 4,
   MDPI_STATE_ABORT     = 1 << 5,
} mdpi_state_t;

#define mdpi_state_str(s)                 \
    s == MDPI_STATE_READY   ? "READY" :   \
   (s == MDPI_STATE_SETUP   ? "SETUP" :   \
   (s == MDPI_STATE_RUNNING ? "RUNNING" : \
   (s == MDPI_STATE_END     ? "END" :     \
   (s == MDPI_STATE_ERROR   ? "ERROR" :   \
   (s == MDPI_STATE_ABORT   ? "ABORT" :   \
                              "UNDEFINED")))))
// clang-format on

typedef enum
{
   MDPI_OP_TYPE_NONE = 0,
   MDPI_OP_TYPE_STATE_CHANGE = 1,
   MDPI_OP_TYPE_MEM_READ,
   MDPI_OP_TYPE_MEM_WRITE,
   MDPI_OP_TYPE_ARG_READ,
   MDPI_OP_TYPE_ARG_WRITE,
   MDPI_OP_TYPE_PARAM_INFO
} mdpi_op_type_t;

typedef struct
{
   ptr_t addr;
   uint16_t size; // Size in bytes
   byte_t buffer[512];
} IPC_STRUCT_ATTR mdpi_op_mem_t;

#define MDPI_ARG_IDX_OUT_OF_BOUNDS (UINT8_MAX)
#define MDPI_ARG_IDX_EMPTY (UINT8_MAX - 1)

typedef struct
{
   uint8_t index;
   uint16_t bitsize;
   byte_t buffer[512];
} IPC_STRUCT_ATTR mdpi_op_arg_t;

typedef struct
{
   uint8_t index;
   uint64_t size;
} IPC_STRUCT_ATTR mdpi_op_param_t;

typedef struct
{
   mdpi_state_t state;
   uint8_t retval;
} IPC_STRUCT_ATTR mdpi_op_state_change_t;

typedef struct
{
   mdpi_op_type_t type;
   union
   {
      mdpi_op_state_change_t sc;
      mdpi_op_param_t param;
      mdpi_op_mem_t mem;
      mdpi_op_arg_t arg;
   } __attribute__((aligned(8))) payload;
} __attribute__((aligned(8))) mdpi_op_t;

static inline __attribute__((always_inline)) void mdpi_op_init(mdpi_op_t* op)
{
   op->type = MDPI_OP_TYPE_NONE;
}

static void __ipc_wait(mdpi_ipc_state_t state);

static void __ipc_reserve();

static void __ipc_request();

static void __ipc_response();

static void __ipc_release();

static void __ipc_exit(mdpi_ipc_state_t ipc_state, mdpi_state_t state, uint8_t retval);

static void __ipc_init(mdpi_entity_t init);

static void __ipc_init1();

static void __ipc_fini(mdpi_entity_t init);

#define __M_IPC_BACKEND_ATOMIC 0
#define __M_IPC_BACKEND_SOCKET 1
#define __M_IPC_BACKEND_MUTEX 2

#ifndef __M_IPC_BACKEND
#define __M_IPC_BACKEND __M_IPC_BACKEND_SOCKET
#endif

#if __M_IPC_BACKEND == __M_IPC_BACKEND_ATOMIC
#include "mdpi_ipc_atomic.h"
#endif

#if __M_IPC_BACKEND == __M_IPC_BACKEND_SOCKET
#include "mdpi_ipc_socket.h"
#endif

#if __M_IPC_BACKEND == __M_IPC_BACKEND_MUTEX
#include "mdpi_ipc_mutex.h"
#endif

#endif // __MDPI_IPC_H