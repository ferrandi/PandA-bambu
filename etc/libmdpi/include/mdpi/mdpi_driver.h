//    Copyright (C) 2023-2026 Politecnico di Milano
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//    This file is part of the PandA/Bambu MDPI Library.
//
//    author Michele Fiorito <michele.fiorito@polimi.it>
//
// Licensed under the Apache License, Version 2.0, with BAMBU exceptions (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
#ifndef __MDPI_DRIVER_H
#define __MDPI_DRIVER_H

#include "mdpi_debug.h"
#include "mdpi_types.h"

#include <stddef.h>

#if defined(__cplusplus) && __cplusplus >= 201103L
#include <ac_channel.h>
#endif

#define MDPI_MEMMAP_DEVICE 0
#define MDPI_MEMMAP_SHARED 1

#ifdef __cplusplus
extern "C"
{
#endif

   void __m_interface_port(void* bits, uint16_t bitsize, uint8_t align = 0);
   void __m_interface_ptr(bptr_t* bits, uint16_t bitsize, size_t ptd_size);
   void __m_interface_array(void* base, uint16_t bitsize, uint8_t align, uint64_t size);
   void __m_interface_array_csroa(void* base, uint16_t bitsize, uint8_t align, uint64_t size,
                                  partition_desc_t* part_desc, uint64_t* dim_sizes, uint64_t dim_count);
   void __m_interface_fifo(void* base, uint16_t bitsize, uint8_t align, uint64_t size);
   void __m_interface_mem();
   void __m_interface_fini();

   void __m_memmap_init(int map_mode);
   int __m_memmap(ptr_t dst, void* src, size_t bytes);
   void __m_param_alloc(uint8_t idx, size_t size);
   size_t __m_param_size(uint8_t idx);

   void __m_sim_start();
   unsigned int __m_sim_end();

   void __m_exit(int __status);
   void __m_abort(void);
   void __m_assert_fail(const char* __assertion, const char* __file, unsigned int __line, const char* __function);

#ifdef __cplusplus
}
#endif

#if defined(__cplusplus) && __cplusplus >= 201103L

#define if_error(str, ...) error("Interface %d: " str, (int)_idx, ##__VA_ARGS__)
#define if_debug(str, ...) debug("Interface %d: " str, (int)_idx, ##__VA_ARGS__)
#define if_info(str, ...) info("Interface %d: " str, (int)_idx, ##__VA_ARGS__)

class interface
{
 protected:
   const mdpi_idx_t _idx;

 public:
   enum state
   {
      IF_OK = 0,
      IF_ERROR = -1,
      IF_FULL = -2,
      IF_EMPTY = -3
   };

   interface(mdpi_idx_t idx) : _idx(idx)
   {
   }

   virtual ~interface() = default;

   virtual void check()
   {
   }

   virtual int read(bptr_t data, uint16_t bitsize, ptr_t addr, int cmd) = 0;

   virtual int write(bptr_t data, uint16_t bitsize, ptr_t addr, int cmd) = 0;

   virtual int state(int data)
   {
      if_error("Unknown data required: %d.\n", data);
      return interface::IF_ERROR;
   }
};

void __m_interface_set(interface* if_manager);
mdpi_idx_t __m_interface_idx_next();

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
   channel_interface(mdpi_idx_t idx, ac_channel<T>& chan, unsigned int max_size = 0)
       : interface(idx), _chan(chan), _count(0), _max_size(max_size)
   {
      if_debug("Channel interface with %u/%u read/write elements.\n", _read_size(), _write_size());
   }

   int read(bptr_t data, uint16_t /*bitsize*/, ptr_t /*addr*/, int cmd) override
   {
      assert((cmd == 0 || cmd == 1 || cmd == -1) && "Unexpected command value.");
      if(!_read_size())
      {
         if(cmd)
         {
            if_error("Read on empty channel.\n");
            return IF_EMPTY;
         }
         return 0;
      }
      if(cmd == 1)
      {
         _chan.read();
         ++_count;
         if_debug("Item pop (%u left).\n", _read_size());
      }
      *reinterpret_cast<T*>(data) = _chan[0];
      if(cmd == -1)
      {
         _chan.read();
         ++_count;
         if_debug("Item pop (%u left).\n", _read_size());
      }
      return _read_size();
   }

   int write(bptr_t data, uint16_t /*bitsize*/, ptr_t /*addr*/, int cmd) override
   {
      assert((cmd == 0 || cmd == -1) && "Unexpected command value.");
      if(!_write_size())
      {
         if_error("Write on full channel.\n");
         return IF_FULL;
      }
      if(cmd == -1)
      {
         _chan.write(*reinterpret_cast<T*>(data));
         ++_count;
         if_debug("Item push (%u free).\n", _write_size());
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
void __m_interface_channel(ac_channel<T>& chan, unsigned int max_size = 0)
{
   __m_interface_set(new channel_interface<T>(__m_interface_idx_next(), chan, max_size));
}
#endif

#endif // __MDPI_DRIVER_H
