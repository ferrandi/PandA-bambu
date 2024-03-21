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
 * @file mdpi_driver.h
 *
 * @author Michele Fiorito <michele.fiorito@polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *
 */
#ifndef __MDPI_DRIVER_H
#define __MDPI_DRIVER_H

#include "mdpi_types.h"

#include <stddef.h>

#if defined(__cplusplus) && __cplusplus >= 201103L
#include <ac_channel.h>
#endif

#define MDPI_MEMMAP_DEVICE 0
#define MDPI_MEMMAP_SHARED 1

EXTERN_C void __m_interface_port(uint8_t idx, void* bits, uint16_t bitsize);
EXTERN_C void __m_interface_ptr(uint8_t idx, bptr_t* bits, uint16_t bitsize);
EXTERN_C void __m_interface_array(uint8_t idx, void* base, uint16_t bitsize, uint8_t align, uint64_t size);
EXTERN_C void __m_interface_fifo(uint8_t idx, void* base, uint16_t bitsize, uint8_t align, uint64_t size);
EXTERN_C void __m_interface_mem(uint8_t idx);
EXTERN_C void __m_interface_fini();

EXTERN_C void __m_memmap_init(int map_mode);
EXTERN_C int __m_memmap(ptr_t dst, void* src, size_t bytes);
EXTERN_C void __m_param_alloc(uint8_t idx, size_t size);
EXTERN_C size_t __m_param_size(uint8_t idx);

EXTERN_C void __m_init();
EXTERN_C void __m_sim_start();
EXTERN_C unsigned int __m_sim_end();

EXTERN_C void __m_exit(int __status);
EXTERN_C void __m_abort(void);
EXTERN_C void __m_assert_fail(const char* __assertion, const char* __file, unsigned int __line, const char* __function);

#if defined(__cplusplus) && __cplusplus >= 201103L

#define if_error(str, ...) error("Interface %d: " str, (int)_idx, ##__VA_ARGS__)
#define if_debug(str, ...) debug("Interface %d: " str, (int)_idx, ##__VA_ARGS__)
#define if_info(str, ...) info("Interface %d: " str, (int)_idx, ##__VA_ARGS__)

class interface
{
 protected:
   const uint8_t _idx;

 public:
   enum state
   {
      IF_OK = 0,
      IF_ERROR = -1,
      IF_FULL = -2,
      IF_EMPTY = -3
   };

   interface(uint8_t idx);

   virtual ~interface() = default;

   virtual int read(bptr_t data, uint16_t bitsize, ptr_t addr, bool shift) = 0;

   virtual int write(bptr_t data, uint16_t bitsize, ptr_t addr, bool shift) = 0;

   virtual int state(int data);
};

template <typename T>
class channel_interface : public interface
{
   ac_channel<T>& _chan;
   unsigned int _count;
   const unsigned int _max_size;

   unsigned int _read_size()
   {
      return _max_size ? (_max_size - _count) : _chan.size();
   }

   unsigned int _write_size()
   {
      return _max_size ? (_max_size - _count) : _chan.num_free();
   }

 public:
   channel_interface(uint8_t idx, ac_channel<T>& chan, unsigned int max_size = 0)
       : interface(idx), _chan(chan), _count(0), _max_size(max_size)
   {
      debug("Interface channel %d with %u/%u read/write elements.\n", (int)_idx, _read_size(), _write_size());
   }

   int read(bptr_t data, uint16_t /*bitsize*/, ptr_t /*addr*/, bool shift) override
   {
      if(!_read_size())
      {
         if_error("Read on empty channel.\n");
         return IF_EMPTY;
      }
      if(shift)
      {
         *reinterpret_cast<T*>(data) = _chan.read();
         ++_count;
      }
      else
      {
         *reinterpret_cast<T*>(data) = _chan[0];
      }
      return _read_size();
   }

   int write(bptr_t data, uint16_t /*bitsize*/, ptr_t /*addr*/, bool shift) override
   {
      if(!_write_size())
      {
         if_error("Write on full channel.\n");
         return IF_FULL;
      }
      if(shift)
      {
         _chan.write(*reinterpret_cast<T*>(data));
         ++_count;
      }
      else
      {
         _chan[_chan.size() - 1] = *reinterpret_cast<T*>(data);
      }
      return _write_size();
   }

   int state(int data)
   {
      if(data == (1 << 1))
      {
         return _read_size();
      }
      else if(data == (1 << 2))
      {
         return _write_size();
      }
      return IF_ERROR;
   }
};

template <typename T>
void __m_interface_channel(uint8_t id, ac_channel<T>& chan, unsigned int max_size = 0)
{
   __m_interface_set(id, new channel_interface<T>(id, chan, max_size));
}

void __m_interface_set(uint8_t id, interface* if_manager);
#endif

#endif // __MDPI_DRIVER_H