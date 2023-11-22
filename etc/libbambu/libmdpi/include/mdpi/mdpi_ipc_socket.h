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
 * @file mdpi_ipc_socket.h
 *
 * @author Michele Fiorito <michele.fiorito@polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *
 */

/*
 * Never include this file directly; use <mdpi/mdpi_ipc.h> instead.
 */

#ifndef __MDPI_IPC_SOCKET_H
#define __MDPI_IPC_SOCKET_H

#ifndef __cplusplus
#include <assert.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#else
#include <cassert>
#include <cerrno>
#include <cstdlib>
#include <cstring>
#endif
#include <fcntl.h>
#include <sys/socket.h>
#include <unistd.h>

#define __USE_MISC
#include <sys/un.h>

typedef struct
{
   int socket;
   mdpi_op_t operation;
} mdpi_ipc_file_t;

static mdpi_ipc_file_t __m_ipc_file;

#define __m_ipc_operation (__m_ipc_file.operation)

static void __ipc_wait(__attribute__((unused)) mdpi_ipc_state_t state)
{
   int size;
   size = recv(__m_ipc_file.socket, &__m_ipc_operation, sizeof(mdpi_op_t), 0);
   if(size == -1)
   {
      error("Unable to receive data.\n");
      perror("recv failed");
      exit(EXIT_FAILURE);
   }
}

static void __ipc_reserve()
{
}

static void __ipc_request()
{
   size_t op_size = (size_t)(&((mdpi_op_t*)0)->payload);
   switch(__m_ipc_operation.type)
   {
      case MDPI_OP_TYPE_MEM_WRITE:
         op_size += __m_ipc_operation.payload.mem.size;
         // fall through
      case MDPI_OP_TYPE_MEM_READ:
         op_size += (size_t)(((mdpi_op_mem_t*)0)->buffer);
         break;
      case MDPI_OP_TYPE_ARG_WRITE:
         op_size += __m_ipc_operation.payload.arg.bitsize / 8 + ((__m_ipc_operation.payload.arg.bitsize % 8) != 0);
         // fall through
      case MDPI_OP_TYPE_ARG_READ:
         op_size += (size_t)(((mdpi_op_arg_t*)0)->buffer);
         break;
      case MDPI_OP_TYPE_PARAM_INFO:
         op_size += sizeof(mdpi_op_param_t);
         break;
      case MDPI_OP_TYPE_STATE_CHANGE:
         op_size += sizeof(mdpi_op_state_change_t);
         break;
      case MDPI_OP_TYPE_NONE:
      default:
         break;
   }
   if(send(__m_ipc_file.socket, &__m_ipc_operation, op_size, 0) == -1)
   {
      error("Unable to send request data.\n");
      perror("send failed");
      exit(EXIT_FAILURE);
   }
}

static void __ipc_response()
{
   size_t op_size = (size_t)(&((mdpi_op_t*)0)->payload);
   switch(__m_ipc_operation.type)
   {
      case MDPI_OP_TYPE_MEM_READ:
         op_size += __m_ipc_operation.payload.mem.size;
         // fall through
      case MDPI_OP_TYPE_MEM_WRITE:
         op_size += (size_t)(((mdpi_op_mem_t*)0)->buffer);
         break;
      case MDPI_OP_TYPE_ARG_READ:
         op_size += __m_ipc_operation.payload.arg.bitsize / 8 + ((__m_ipc_operation.payload.arg.bitsize % 8) != 0);
         // fall through
      case MDPI_OP_TYPE_ARG_WRITE:
         op_size += (size_t)(((mdpi_op_arg_t*)0)->buffer);
         break;
      case MDPI_OP_TYPE_PARAM_INFO:
         op_size += sizeof(mdpi_op_param_t);
         break;
      case MDPI_OP_TYPE_STATE_CHANGE:
         op_size += sizeof(mdpi_op_state_change_t);
         break;
      case MDPI_OP_TYPE_NONE:
      default:
         break;
   }
   if(send(__m_ipc_file.socket, &__m_ipc_operation, op_size, 0) == -1)
   {
      error("Unable to send response data.\n");
      perror("send failed");
      exit(EXIT_FAILURE);
   }
}

static void __ipc_release()
{
}

static void __ipc_exit(mdpi_ipc_state_t ipc_state, mdpi_state_t state, uint8_t retval)
{
   __m_ipc_operation.type = MDPI_OP_TYPE_STATE_CHANGE;
   __m_ipc_operation.payload.sc.state = state;
   __m_ipc_operation.payload.sc.retval = retval;
   __ipc_request();
}

static void __ipc_set_socket_buffer_size(int socket_fd)
{
   int error;
   int buffer_size = sizeof(mdpi_op_t);
   if(setsockopt(socket_fd, SOL_SOCKET, SO_SNDBUF, &buffer_size, sizeof buffer_size) == -1)
   {
      error("Error setting socket send buffer size.\n");
      perror("setsockopt failed");
      exit(EXIT_FAILURE);
   }
   if(setsockopt(socket_fd, SOL_SOCKET, SO_RCVBUF, &buffer_size, sizeof buffer_size) == -1)
   {
      error("Error setting socket receive buffer size.\n");
      perror("setsockopt failed");
      exit(EXIT_FAILURE);
   }
}

static void __ipc_init(mdpi_entity_t init)
{
   __m_ipc_file.socket = socket(AF_UNIX, SOCK_STREAM, 0);
   if(__m_ipc_file.socket == -1)
   {
      error("Error opening IPC socket.\n");
      perror("socket failed");
      exit(EXIT_FAILURE);
   }
   debug("IPC socket file %s\n", __M_IPC_FILENAME);
   struct sockaddr_un address;
   memset(&address, 0, sizeof(struct sockaddr_un));
   address.sun_family = AF_UNIX;
   strcpy(address.sun_path, __M_IPC_FILENAME);

   if(init == MDPI_ENTITY_DRIVER)
   {
      remove(address.sun_path);
      if(bind(__m_ipc_file.socket, (struct sockaddr*)&address, SUN_LEN(&address)) == -1)
      {
         error("Unable to bind testbench-side socket.\n");
         perror("bind failed");
         exit(EXIT_FAILURE);
      }
      if(listen(__m_ipc_file.socket, 1) == -1)
      {
         error("Unable to listen of testbench-side socket.\n");
         perror("listen failed");
         exit(EXIT_FAILURE);
      }
      debug("IPC socket listener running...\n");
   }
   else
   {
      __ipc_set_socket_buffer_size(__m_ipc_file.socket);
      if(connect(__m_ipc_file.socket, (struct sockaddr*)&address, SUN_LEN(&address)) == -1)
      {
         error("Unable to connect simulator-side socket.\n");
         perror("connect failed");
         exit(EXIT_FAILURE);
      }
      debug("IPC socket connection completed.\n");
   }

   mdpi_op_init(&__m_ipc_operation);
}

static void __ipc_init1()
{
   struct sockaddr_un client;
   int server_socket = __m_ipc_file.socket;
   socklen_t length = sizeof client;

   __m_ipc_file.socket = accept(server_socket, (struct sockaddr*)&client, &length);
   if(__m_ipc_file.socket == -1)
   {
      error("Unable to accept socket at testbench side.\n");
      perror("accept failed");
      exit(EXIT_FAILURE);
   }
   __ipc_set_socket_buffer_size(__m_ipc_file.socket);
   close(server_socket);
   debug("IPC socket connection completed.\n");
}

static void __ipc_fini(mdpi_entity_t init)
{
   close(__m_ipc_file.socket);
   if(init == MDPI_ENTITY_DRIVER)
   {
      remove(__M_IPC_FILENAME);
   }
}

#endif // __MDPI_IPC_SOCKET_H