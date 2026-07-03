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

#define __BAMBU_IPC_ENTITY MDPI_ENTITY_DRIVER

#include <mdpi/mdpi_debug.h>

#if __M_OUT_LVL <= 4 && !defined(NDEBUG)
#define NDEBUG
#endif

#ifndef NDEBUG
#define DEBUG_PARAM(p) p
#else
#define DEBUG_PARAM(p)
#endif

#include <mdpi/mdpi_driver.h>
#include <mdpi/mdpi_ipc.h>
#include <mdpi/mdpi_memmap.h>
#include <mdpi/mdpi_types.h>
#include <mdpi/mdpi_user.h>

#include <algorithm>
#include <cassert>
#include <cerrno>
#include <csignal>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <map>
#include <memory>
#include <sys/wait.h>
#include <unistd.h>
#include <vector>

#ifdef BAMBU_CONCURRENT_COSIM
#include <pthread.h>
#endif

#define FORCE_INLINE __attribute__((always_inline)) inline

#define byte_offset(i) ((i & 3) << 3)

#if(__WORDSIZE == 32 && !defined(__M64)) || (__WORDSIZE == 64 && defined(__M64))
#define ENABLE_SHARED_MEMORY 1
#else
#define ENABLE_SHARED_MEMORY 0
#endif

static void* __m_driver_loop(void*);

static pid_t __m_sim_pid = 0;
static pthread_t __m_ipc_driver = 0;

static std::map<uint8_t, size_t> __m_params_size;
static std::unique_ptr<memmap> __m_mapper;

static std::vector<std::unique_ptr<interface>> __m_interfaces;

static char __m_sigsegv_error_msg[128];

void __m_interface_set(interface* if_manager)
{
   __m_interfaces.push_back(std::unique_ptr<interface>(if_manager));
}

mdpi_idx_t __m_interface_idx_next()
{
   assert(__m_interfaces.size() <= UINT16_MAX && "Interface index overflow.");
   return static_cast<mdpi_idx_t>(__m_interfaces.size());
}

void __m_interface_fini()
{
   debug("Finalize interface list.\n");
   __m_interfaces.clear();
}

static __attribute__((noinline)) void __m_memcheck(bptr_t base, size_t size, const char* format, ...)
{
   va_list va;
   byte_t val;
   volatile bptr_t begin = base;
   debug("Check memory location at " BPTR_FORMAT " for %zu bytes.\n", bptr_to_int(base), size);
   va_start(va, format);
   vsnprintf(__m_sigsegv_error_msg, sizeof(__m_sigsegv_error_msg) / sizeof(__m_sigsegv_error_msg[0]), format, va);
   va_end(va);
   val = *begin;
   asm volatile("" ::"r"(val) : "memory");
   val = *(begin + size - 1);
   asm volatile("" ::"r"(val) : "memory");
   __m_sigsegv_error_msg[0] = '\0';
}

namespace
{
   class device_memmap : public memmap
   {
    private:
      std::map<ptr_t, bptr_t> __m_mmu;

    public:
      device_memmap()
      {
         __m_mmu[0] = NULL;
      }

      void check() override
      {
      }

      int map(ptr_t dst, void* src, size_t bytes) override
      {
         bptr_t bits = reinterpret_cast<bptr_t>(src);
         info("Address " BPTR_FORMAT " mapped at " PTR_FORMAT " (%zu bytes)\n", bptr_to_int(bits), dst, bytes);

         const std::pair<std::map<ptr_t, bptr_t>::iterator, bool> base =
             __m_mmu.insert(std::make_pair(dst, bits - dst));
         const std::pair<std::map<ptr_t, bptr_t>::iterator, bool> top =
             __m_mmu.insert(std::make_pair<ptr_t, bptr_t>(dst + bytes, 0));
         std::map<ptr_t, bptr_t>::iterator front = base.first, prev = base.first;
         --prev;
         std::map<ptr_t, bptr_t>::iterator back = top.first, next = top.first;
         ++next;
         if(!base.second)
         {
            if(!front->second)
            {
               front->second = bits - dst;
               if(prev->second == front->second)
               {
                  __m_mmu.erase(front);
                  front = prev;
                  --prev;
               }
            }
            else if(front->second != (bits - dst))
            {
               error("Uncorrelated memory spaces overlap: " PTR_FORMAT "(" BPTR_FORMAT ") over " PTR_FORMAT
                     "(" BPTR_FORMAT ").\n",
                     dst, bptr_to_int(bits + dst), front->first, bptr_to_int(front->second + front->first));
               return 1;
            }
         }
         else if(prev->second)
         {
            if(prev->second != front->second)
            {
               error("Uncorrelated memory spaces overlap: " PTR_FORMAT "(" BPTR_FORMAT ") over " PTR_FORMAT
                     "(" BPTR_FORMAT ").\n",
                     front->first, bptr_to_int(front->second + front->first), prev->first,
                     bptr_to_int(prev->second + prev->first));
               return 1;
            }
            front = prev;
         }
         if(top.second && next != __m_mmu.end() && !next->second)
         {
            back = next;
         }
         next = front;
         ++next;
         if(next != back)
         {
            std::map<ptr_t, bptr_t>::iterator it = next;
            for(; it != back; ++it)
            {
               if(it->second && it->second != front->second)
               {
                  error("Uncorrelated memory spaces overlap: " PTR_FORMAT "(" BPTR_FORMAT ") over " PTR_FORMAT
                        "(" BPTR_FORMAT ").\n",
                        front->first, bptr_to_int(front->second + front->first), it->first,
                        bptr_to_int(it->second + it->first));
                  return 1;
               }
            }
            __m_mmu.erase(next, back);
         }
         return 0;
      }

      bptr_t addrmap(ptr_t sim_addr) override
      {
         std::map<ptr_t, bptr_t>::const_iterator mmu_it = --__m_mmu.upper_bound(sim_addr);
         if(mmu_it != __m_mmu.begin() && mmu_it->second)
         {
            bptr_t addr = mmu_it->second + sim_addr;
            return addr;
         }
         std::map<ptr_t, bptr_t>::const_iterator mmu_base;
         if(mmu_it == __m_mmu.begin())
         {
            mmu_base = ++mmu_it;
            ++mmu_it;
         }
         else
         {
            mmu_base = mmu_it;
            --mmu_base;
         }
         error("Nearest memory space is [" PTR_FORMAT ", " PTR_FORMAT "] -> [" BPTR_FORMAT ", " BPTR_FORMAT
               "] (%zu bytes).\n",
               mmu_base->first, mmu_it->first, bptr_to_int(mmu_base->second + mmu_base->first),
               bptr_to_int(mmu_base->second + mmu_it->first), static_cast<size_t>(mmu_it->first - mmu_base->first));
         return 0;
      }

      ptr_t mapaddr(const bptr_t addr) override
      {
         std::map<ptr_t, bptr_t>::const_iterator curr = __m_mmu.begin(), prev;
         do
         {
            if(curr->second)
            {
               prev = curr++;
            }
            else
            {
               prev = ++curr;
               ++curr;
            }
         } while(prev != __m_mmu.end() &&
                 (addr < (prev->first + prev->second) || addr >= (curr->first + prev->second)));
         if(prev == __m_mmu.end())
         {
            return 0;
         }
         return addr - prev->second;
      }
   };

#if ENABLE_SHARED_MEMORY
   class shared_memmap : public memmap
   {
    public:
      void check() override
      {
      }

      int map(ptr_t dst, void* src, size_t bytes) override
      {
         return 0;
      }

      bptr_t addrmap(ptr_t sim_addr) override
      {
         return ptr_to_bptr(sim_addr);
      }

      ptr_t mapaddr(const bptr_t addr) override
      {
         return reinterpret_cast<ptr_t>(addr);
      }
   };
#endif

   class port_interface : public interface
   {
      const uint16_t _bitsize;
      const uint64_t _size;
      const uint8_t _align;
      const bptr_t _data;

    public:
      port_interface(mdpi_idx_t idx, void* data, uint16_t bitsize, uint8_t align)
          : interface(idx),
            _data(reinterpret_cast<bptr_t>(data)),
            _bitsize(bitsize),
            _size(bitsize / 8 + ((bitsize % 8) ? 1 : 0)),
            _align(align > _size ? align : 0)
      {
         if_debug("Port interface for " BPTR_FORMAT " %u bits.\n", bptr_to_int(_data), _bitsize);
      }

      void check() override
      {
         __m_memcheck(_data, _align ? _align : _size,
                      "Invalid memory space set for parameter %u (reported size: %zu bytes).", _idx,
                      _align ? _align : _size);
      }

      int read(bptr_t data, uint16_t bitsize, ptr_t /*addr*/, int /*cmd*/) override
      {
         assert(bitsize == _bitsize && "Bitsize mismatch");
         memcpy(data, _data, _size);
         return 1;
      }

      int write(bptr_t data, uint16_t bitsize, ptr_t /*addr*/, int /*cmd*/) override
      {
         assert(bitsize == _bitsize && "Bitsize mismatch");
         memcpy(_data, data, _size);
         if(_align)
         {
            memset(_data + _size, 0, _align - _size);
         }
         return 1;
      }

      int state(int data) override
      {
         if(data == MDPI_OP_TYPE_IF_READ || data == MDPI_OP_TYPE_IF_WRITE)
         {
            return interface::IF_OK;
         }
         return interface::state(data);
      }
   };
} // namespace

void __m_interface_port(void* bits, uint16_t bitsize, uint8_t align)
{
   __m_interface_set(new port_interface(__m_interface_idx_next(), bits, bitsize, align));
}

void __m_interface_ptr(bptr_t* bits, uint16_t bitsize, size_t ptd_size)
{
   const bptr_t addr = *bits;
   const ptr_t dst = __m_mapper->mapaddr(addr);
   if(dst)
   {
      interface* ptr_port;
      info("Pointer parameter " BPTR_FORMAT " mapped at " PTR_FORMAT "\n", bptr_to_int(addr), dst);
      assert((bitsize == PTR_SIZE || dst < (UINT64_MAX >> (64 - PTR_SIZE))) && "Pointer value overflow.");
      *bits = ptr_to_bptr(dst);
      ptr_port = new port_interface(__m_interface_idx_next(), bits, bitsize, 0);
      __m_memcheck(addr, ptd_size, "Invalid memory space set for parameter (reported size: %zu bytes).", ptd_size);
      return __m_interface_set(ptr_port);
   }
   error("Unknown parameter address mapping for " BPTR_FORMAT "\n", bptr_to_int(addr));
   // TODO: report to simulator
   exit(EXIT_FAILURE);
}

namespace
{
   class array_interface : public interface
   {
    protected:
      const bptr_t _base;
      const uint16_t _bitsize;
      const uint8_t _align;
      const uint16_t _esize;
      const uint64_t _size;

    public:
      array_interface(mdpi_idx_t idx, void* data, uint16_t bitsize, uint8_t align, uint64_t size)
          : interface(idx),
            _base(reinterpret_cast<bptr_t>(data)),
            _bitsize(bitsize),
            _align(align),
            _esize(bitsize / 8 + ((bitsize % 8 ? 1 : 0))),
            _size(size)
      {
         if_debug("Array interface for " BPTR_FORMAT ", %" PRIu64 " elements of %u bits aligned at %u bytes.\n",
                  bptr_to_int(_base), _size, _bitsize, _align);
      }

      virtual ~array_interface() = default;

      void check() override
      {
         __m_memcheck(_base, _size, "Invalid memory space set for parameter %u (reported size: %zu bytes).", _idx,
                      _size);
      }

      int read(bptr_t data, uint16_t DEBUG_PARAM(bitsize), ptr_t addr, int /*cmd*/) override
      {
         assert(bitsize == _bitsize && "Bitsize mismatch");
         if(addr >= _size)
         {
            if_error("Access out of bounds: " PTR_FORMAT " > %" PRIu64 "\n", addr, _size);
            return IF_ERROR;
         }
         memcpy(data, _base + (_align * addr), _esize);
         return IF_OK;
      }

      int write(bptr_t data, uint16_t DEBUG_PARAM(bitsize), ptr_t addr, int /*cmd*/) override
      {
         assert(bitsize == _bitsize && "Bitsize mismatch");
         if(addr >= _size)
         {
            if_error("Access out of bounds: " PTR_FORMAT " > %" PRIu64 "\n", addr, _size);
            return IF_ERROR;
         }
         memcpy(_base + (_align * addr), data, _esize);
         return IF_OK;
      }
   };
} // namespace

void __m_interface_array(void* base, uint16_t bitsize, uint8_t align, uint64_t size)
{
   __m_interface_set(new array_interface(__m_interface_idx_next(), base, bitsize, align, size));
}

namespace
{
   class array_interface_csroa : public array_interface
   {
      class csroa_iterator
      {
         const uint64_t _part_idx;
         const uint64_t _dim_count;
         const partition_desc_t* _part_desc;
         const uint64_t* _dim_sizes;
         uint64_t* _strides;
         uint64_t* _indices;
         uint64_t* _start_indices;
         bool _end;

         uint64_t get_linear_addr()
         {
            uint64_t addr = 0;
            for(uint64_t i = 0; i < _dim_count; i++)
            {
               addr += _indices[i] * _strides[i];
            }
            return addr;
         }

         void next_indices()
         {
            bool done = false;
            for(int64_t i = _dim_count - 1; i >= 0 && !done; i--)
            {
               done = true;
               switch(_part_desc[i].kind)
               {
                  case partition_kind_t::block:
                  {
                     const uint64_t block_size = _dim_sizes[i] / _part_desc[i].factor;
                     _indices[i] += 1;
                     if(_indices[i] >= (_start_indices[i] + block_size))
                     {
                        _indices[i] = _start_indices[i];
                        done = false;
                     }
                     break;
                  }
                  case partition_kind_t::cyclic:
                  {
                     _indices[i] += _part_desc[i].factor;
                     if(_indices[i] >= _dim_sizes[i])
                     {
                        _indices[i] = _start_indices[i];
                        done = false;
                     }
                     break;
                  }
                  case partition_kind_t::complete:
                  {
                     done = false;
                     break;
                  }
                  case partition_kind_t::none:
                  {
                     _indices[i] += 1;
                     if(_indices[i] >= _dim_sizes[i])
                     {
                        _indices[i] = 0;
                        done = false;
                     }
                     break;
                  }
                  default:
                     assert(0 && "Unexpected partition kind.");
               }
            }

            if(!done)
            {
               _end = true;
            }
         }

         void init_indices()
         {
            // Decode partition digits in row-major order:
            // dim0 is the most-significant partitioned dimension.
            uint64_t* suffix_partitions = new uint64_t[_dim_count + 1];
            suffix_partitions[_dim_count] = 1;
            for(int64_t i = _dim_count - 1; i >= 0; --i)
            {
               uint64_t factor = 1;
               if(_part_desc[i].kind != partition_kind_t::none)
               {
                  factor = _part_desc[i].factor;
               }
               suffix_partitions[i] = suffix_partitions[i + 1] * factor;
            }

            for(uint64_t i = 0; i < _dim_count; ++i)
            {
               switch(_part_desc[i].kind)
               {
                  case partition_kind_t::block:
                  {
                     const uint64_t factor = _part_desc[i].factor;
                     const uint64_t dim_idx = (_part_idx / suffix_partitions[i + 1]) % factor;
                     const uint64_t block_size = _dim_sizes[i] / _part_desc[i].factor;
                     _indices[i] = dim_idx * block_size;
                     _start_indices[i] = _indices[i];
                     break;
                  }
                  case partition_kind_t::cyclic:
                  {
                     const uint64_t factor = _part_desc[i].factor;
                     const uint64_t dim_idx = (_part_idx / suffix_partitions[i + 1]) % factor;
                     _indices[i] = dim_idx;
                     _start_indices[i] = _indices[i];
                     break;
                  }
                  case partition_kind_t::complete:
                  {
                     const uint64_t factor = _part_desc[i].factor;
                     const uint64_t dim_idx = (_part_idx / suffix_partitions[i + 1]) % factor;
                     _indices[i] = dim_idx;
                     _start_indices[i] = _indices[i];
                     break;
                  }
                  case partition_kind_t::none:
                  {
                     _indices[i] = 0;
                     _start_indices[i] = 0;
                     break;
                  }
                  default:
                     info("Unexpected partition kind\n");
                     assert(0 && "Unexpected partition kind.");
               }
            }
            delete[] suffix_partitions;
         }

         void init_strides()
         {
            _strides[_dim_count - 1] = 1;
            for(int64_t i = _dim_count - 2; i >= 0; i--)
            {
               _strides[i] = _strides[i + 1] * _dim_sizes[i + 1];
            }
         }

       public:
         csroa_iterator(const partition_desc_t* part_desc, const uint64_t* dim_sizes, uint64_t dim_count,
                        uint64_t part_idx)
             : _dim_count(dim_count), _part_desc(part_desc), _dim_sizes(dim_sizes), _part_idx(part_idx), _end(false)
         {
            _indices = new uint64_t[dim_count];
            _strides = new uint64_t[dim_count];
            _start_indices = new uint64_t[dim_count];
            init_indices();
            init_strides();
         }

         ~csroa_iterator()
         {
            delete[] _indices;
            delete[] _strides;
            delete[] _start_indices;
         }

         uint64_t next()
         {
            if(_end)
            {
               assert(0 && "Iterator out of bounds.");
            }
            uint64_t ret = get_linear_addr();
            next_indices();
            return ret;
         }
      };

      bptr_t _original_data;
      const partition_desc_t* _part_desc;
      const uint64_t _dim_count;
      const uint64_t _part_idx;
      const uint64_t* _dim_sizes;

      void* _transfer_data(bptr_t data, uint8_t align, uint64_t size, uint64_t esize, uint64_t dim_count,
                           uint64_t part_idx, const uint64_t* dim_sizes, const partition_desc_t* part_desc)
      {
         bptr_t base = reinterpret_cast<bptr_t>(malloc(align * size));
         csroa_iterator iter(part_desc, dim_sizes, dim_count, part_idx);

         for(uint64_t i = 0; i < size; i++)
         {
            memcpy(base + (align * i), data + (align * iter.next()), esize);
         }

         return base;
      }

    public:
      array_interface_csroa(mdpi_idx_t idx, void* data, uint16_t bitsize, uint8_t align, uint64_t size,
                            partition_desc_t* part_desc, uint64_t* dim_sizes, uint64_t dim_count, uint64_t part_idx)
          : array_interface(idx,
                            _transfer_data(reinterpret_cast<bptr_t>(data), align, size,
                                           bitsize / 8 + ((bitsize % 8 ? 1 : 0)), dim_count, part_idx, dim_sizes,
                                           part_desc),
                            bitsize, align, size),
            _original_data(reinterpret_cast<bptr_t>(data)),
            _part_desc(part_desc),
            _dim_count(dim_count),
            _part_idx(part_idx),
            _dim_sizes(dim_sizes)
      {
         if_debug("Array interface csroa for " BPTR_FORMAT ", %" PRIu64 " elements of %u bits aligned at %u bytes.\n",
                  bptr_to_int(_base), _size, _bitsize, _align);
      }

      ~array_interface_csroa()
      {
         // copy back to original memory
         csroa_iterator iter(_part_desc, _dim_sizes, _dim_count, _part_idx);
         for(uint64_t i = 0; i < _size; i++)
         {
            memcpy(_original_data + (_align * iter.next()), _base + (_align * i), _esize);
         }
         free(_base);
      }

      int read(bptr_t data, uint16_t bitsize, ptr_t addr, int cmd) override
      {
         const auto ret = array_interface::read(data, bitsize, addr, cmd);
         return ret < 0 ? ret : 1;
      }

      int write(bptr_t data, uint16_t bitsize, ptr_t addr, int cmd) override
      {
         const auto ret = array_interface::write(data, bitsize, addr, cmd);
         return ret < 0 ? ret : 1;
      }

      int state(int data) override
      {
         if(data == MDPI_OP_TYPE_IF_READ || data == MDPI_OP_TYPE_IF_WRITE)
         {
            return 1;
         }
         return interface::state(data);
      }
   };
} // namespace

void __m_interface_array_csroa(void* base, uint16_t bitsize, uint8_t align, uint64_t size, partition_desc_t* part_desc,
                               uint64_t* dim_sizes, uint64_t dim_count)
{
   size_t part_count = 1;
   for(size_t i = 0; i < dim_count; i++)
   {
      if(part_desc[i].kind != partition_kind_t::none)
      {
         part_count *= part_desc[i].factor;
      }
   }

   for(size_t i = 0; i < part_count; i++)
   {
      __m_interface_set(new array_interface_csroa(__m_interface_idx_next(), base, bitsize, align, size / part_count,
                                                  part_desc, dim_sizes, dim_count, i));
   }
}

namespace
{
   class fifo_interface : public interface
   {
      bptr_t _base;
      const bptr_t _end;
      const uint16_t _bitsize;
      const uint8_t _align;
      const uint16_t _esize;

      inline int _size()
      {
         return std::distance(_base, _end) / _align;
      }

    public:
      fifo_interface(mdpi_idx_t idx, void* data, uint16_t bitsize, uint8_t align, uint64_t size)
          : interface(idx),
            _base(reinterpret_cast<bptr_t>(data)),
            _end(_base + (align * size)),
            _bitsize(bitsize),
            _align(align),
            _esize(bitsize / 8 + ((bitsize % 8 ? 1 : 0)))
      {
         if_debug("FIFO interface for " BPTR_FORMAT ", %" PRIu64 " elements of %u bits aligned at %u bytes.\n",
                  bptr_to_int(_base), size, _bitsize, _align);
      }

      void check() override
      {
         __m_memcheck(_base, std::distance(_base, _end),
                      "Invalid memory space set for parameter %u (reported size: %zu bytes).", _idx,
                      std::distance(_base, _end));
      }

      int read(bptr_t data, uint16_t DEBUG_PARAM(bitsize), ptr_t /*addr*/, int cmd) override
      {
         assert(bitsize == _bitsize && "Bitsize mismatch");
         assert((cmd == 0 || cmd == 1 || cmd == -1) && "Unexpected command value.");
         if(_base == _end)
         {
            if(cmd)
            {
               if_error("Read on empty FIFO.\n");
               return IF_EMPTY;
            }
            return 0;
         }
         if(cmd == 1)
         {
            _base += _align;
            if_debug("Item pop (%u left).\n", _size());
         }
         memcpy(data, _base, _esize);
         if(cmd == -1)
         {
            _base += _align;
            if_debug("Item pop (%u left).\n", _size());
         }
         return _size();
      }

      int write(bptr_t data, uint16_t DEBUG_PARAM(bitsize), ptr_t /*addr*/, int cmd) override
      {
         assert(bitsize == _bitsize && "Bitsize mismatch");
         assert((cmd == 0 || cmd == -1) && "Unexpected command value.");
         if(_base == _end)
         {
            if_error("Write on full FIFO.\n");
            return IF_FULL;
         }
         memcpy(_base, data, _esize);
         if(cmd == -1)
         {
            _base += _align;
            if_debug("Item push (%u free).\n", _size());
         }
         return _size();
      }

      int state(int data) override
      {
         if(data == MDPI_OP_TYPE_IF_READ || data == MDPI_OP_TYPE_IF_WRITE)
         {
            return _size();
         }
         return interface::state(data);
      }
   };
} // namespace

void __m_interface_fifo(void* base, uint16_t bitsize, uint8_t align, uint64_t size)
{
   __m_interface_set(new fifo_interface(__m_interface_idx_next(), base, bitsize, align, size));
}

namespace
{
   class mem_interface : public interface
   {
    public:
      mem_interface(mdpi_idx_t idx) : interface(idx)
      {
         if_debug("Memory interface.\n");
      }

      void check() override
      {
         __m_mapper->check();
      }

      int read(bptr_t data, uint16_t bitsize, ptr_t addr, int /*cmd*/) override
      {
         assert((bitsize % 8) == 0 && "Expected byte-aligned memory address");
         bptr_t __addr = __m_mapper->addrmap(addr);
         if(__addr)
         {
#if __M_OUT_LVL > 4
            std::vector<char> hex_data(bitsize / 4, '\0');
            for(uint16_t i = 0; i < bitsize / 8; ++i)
            {
               snprintf(&hex_data[i * 2], 3, "%02X", __addr[i]);
            }
#endif
            if_debug("Read %u bytes at " PTR_FORMAT "->" BPTR_FORMAT " (data: 0x%s)\n", bitsize / 8, addr,
                     bptr_to_int(__addr), hex_data.data());
            memcpy(data, __addr, bitsize / 8);
         }
         else
         {
            if_error("Read to non-mapped address " PTR_FORMAT ".\n", addr);
            return IF_ERROR;
         }
         return IF_OK;
      }

      int write(bptr_t data, uint16_t bitsize, ptr_t addr, int /*cmd*/) override
      {
         bptr_t __addr = __m_mapper->addrmap(addr);
         if(__addr)
         {
            const auto floor_bytes = bitsize / 8;
#if __M_OUT_LVL > 4
            std::vector<char> hex_data(floor_bytes * 2 + 2, '\0');
            for(uint16_t i = 0; i < floor_bytes; ++i)
            {
               snprintf(&hex_data[i * 2], 3, "%02X", data[i]);
            }
#endif
            memcpy(__addr, data, floor_bytes);
            auto spare = bitsize % 8;
            if(spare)
            {
               const byte_t mask = 0xFF << spare;
#if __M_OUT_LVL > 4
               snprintf(&hex_data[floor_bytes * 2], 3, "%02X", data[floor_bytes] & ~mask);
#endif
               __addr[floor_bytes] = (__addr[floor_bytes] & mask) | (data[floor_bytes] & ~mask);
            }
            if_debug("Write %u bits at " PTR_FORMAT "->" BPTR_FORMAT " (data: 0x%s)\n", bitsize, addr,
                     bptr_to_int(__addr), hex_data.data());
         }
         else
         {
            if_error("Write to non-mapped address " PTR_FORMAT ".\n", addr);
            return IF_ERROR;
         }
         return IF_OK;
      }
   };
} // namespace

void __m_interface_mem()
{
   __m_interface_set(new mem_interface(__m_interface_idx_next()));
}

static FORCE_INLINE void __ipc_exit(mdpi_state_t state, uint8_t retval)
{
   static bool exited = false;
   if(!exited)
   {
      exited = true;
      __ipc_exit(MDPI_IPC_STATE_RESPONSE, state, retval);
   }
}

static void __ipc_abort()
{
   __ipc_exit(MDPI_STATE_ABORT, EXIT_FAILURE);
   exit(EXIT_FAILURE);
}

void __m_sig_handler(int __sig, siginfo_t* info, void* ucontext)
{
   if(__sig == SIGCHLD)
   {
      int status;
      if(info->si_pid != __m_sim_pid)
      {
         return;
      }
      __m_sim_pid = 0;
      if(wait(&status) == -1)
      {
         error("Error waiting for simulation process.\n");
         perror("wait failed");
      }
      else
      {
         if(WIFEXITED(status) && !WEXITSTATUS(status))
         {
            debug("Simulation process exited with code %d.\n", WEXITSTATUS(status));
            return;
         }
         error("Simulation process terminated with error.\n");
      }
   }
   else
   {
      error("Abrupt exception: %s\n", strsignal(__sig));
      if(strlen(__m_sigsegv_error_msg))
      {
         error("%s\n", __m_sigsegv_error_msg);
      }
      if(__m_sim_pid)
      {
         pid_t sim_pid = __m_sim_pid;
         __m_sim_pid = 0;
         debug("Sending abort report to simulation process.\n");
         __ipc_exit(MDPI_STATE_ABORT, EXIT_FAILURE);
         sleep(2);
         do
         {
            info("Killing simulation process (PID: %d).\n", sim_pid);
            if(kill(sim_pid, SIGTERM) == -1)
            {
               if(errno == EAGAIN || errno == EINTR)
                  continue;
               error("Unable to kill simulation process (PID: %d).\n", sim_pid);
               perror("kill failed");
            }
            break;
         } while(1);
         if(wait(NULL) == -1)
         {
            error("Error waiting for simulation process.\n");
            perror("wait failed");
         }
      }
   }
   fflush(stdout);
#if __M_OUT_LVL < 4
   fflush(stderr);
#endif
   exit(EXIT_FAILURE);
}

void __m_exit(int __status)
{
   info("Exit called with value %d\n", __status);
   __ipc_exit(MDPI_STATE_END, __status);
   _exit(__status);
}

void __m_abort()
{
   error("Co-simulation called abort\n");
   __ipc_abort();
}

void __m_assert_fail(const char* __assertion, const char* __file, unsigned int __line, const char* __function)
{
#if __M_OUT_LVL > 4
   fprintf(stdout,
#else
   fprintf(stderr,
#endif
           "%s: %d: %s: Assertion `%s' failed.\n", __file, __line, __function, __assertion);
   __m_abort();
}

void __attribute__((constructor)) __mdpi_driver_init()
{
   static const int __sigs[] = {SIGINT, SIGABRT, SIGSEGV, SIGCHLD};
   int error;
   size_t i;
   struct sigaction driver_sa;
#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wwrite-strings"
#else
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wwrite-strings"
#endif
   char* const sim_argv[] = {"bash", "-c", getenv(__M_IPC_SIM_CMD_ENV), NULL};
#ifdef __clang__
#pragma clang diagnostic pop
#else
#pragma GCC diagnostic pop
#endif

   debug("Loading MDPI library...\n");

   memset(__m_sigsegv_error_msg, 0, sizeof(__m_sigsegv_error_msg) / sizeof(__m_sigsegv_error_msg[0]));
   memset(&driver_sa, 0, sizeof(driver_sa));
   sigemptyset(&driver_sa.sa_mask);
   driver_sa.sa_flags = SA_SIGINFO;
   driver_sa.sa_sigaction = __m_sig_handler;

   __ipc_init(__BAMBU_IPC_ENTITY);

   for(i = 0; i < (sizeof(__sigs) / sizeof(*__sigs)); ++i)
   {
      struct sigaction oldsa;
      memset(&oldsa, 0, sizeof(struct sigaction));
      sigaction(__sigs[i], NULL, &oldsa);
      if(((oldsa.sa_flags & SA_SIGINFO) && oldsa.sa_sigaction) || oldsa.sa_handler)
      {
         warn("Signal handler for \"%s\" already registered.\n", strsignal(__sigs[i]));
      }
      else
      {
         sigaction(__sigs[i], &driver_sa, NULL);
         debug("Signal handler registered for \"%s\".\n", strsignal(__sigs[i]));
      }
   }

   if(sim_argv[2] && strlen(sim_argv[2]))
   {
      fflush(stdout);
      __m_sim_pid = fork();
      if(__m_sim_pid == -1)
      {
         error("Error forking simulation process.\n");
         perror("fork");
         exit(EXIT_FAILURE);
      }
      else if(!__m_sim_pid)
      {
         char* ipc_filename_abs;
         // Unset LD_PRELOAD to prevent the new process from loading mdpi_driver.so
         error = unsetenv("LD_PRELOAD");
         if(error)
         {
            error("Failed to unset LD_PRELOAD.\n");
            perror("unsetenv");
            _exit(EXIT_FAILURE);
         }
         debug("Simulation process command line: \"%s\"\n", sim_argv[2]);
         ipc_filename_abs = realpath(__ipc_filename(), NULL);
         if(!ipc_filename_abs)
         {
            perror("realpath");
            _exit(EXIT_FAILURE);
         }
         error = setenv(__M_IPC_FILENAME_ENV, ipc_filename_abs, true);
         if(error)
         {
            error("Failed to set " __M_IPC_FILENAME_ENV ".\n");
            perror("setenv");
            _exit(EXIT_FAILURE);
         }
         error = execvp("bash", sim_argv);
         error("Failed to launch simulation process.\n");
         perror("execv");
         _exit(EXIT_FAILURE);
      }
      debug("Launched simulation process with PID %d.\n", __m_sim_pid);
   }
   else
   {
      info("Simulation command line found empty, expecting simulator to be launched by others.\n");
   }

   __ipc_init1();

   debug("Loading completed.\n");
}

void __attribute__((destructor)) __mdpi_driver_fini()
{
   __ipc_exit(MDPI_STATE_END, EXIT_SUCCESS);
   __ipc_fini(__BAMBU_IPC_ENTITY);
   if(__m_sim_pid)
   {
      int status;
      __m_sim_pid = 0;
      if(wait(&status) == -1)
      {
         error("Error waiting for simulation process.\n");
         perror("wait failed");
      }
      else if(WIFEXITED(status) && WEXITSTATUS(status))
      {
         error("Simulation process terminated with non-zero value: %d\n", WEXITSTATUS(status));
      }
      else if(WIFSIGNALED(status))
      {
         error("Simulation process terminated by signal: %s", strsignal(WTERMSIG(status)));
      }
   }
   debug("Finalization completed.\n");
}

void __m_sim_start()
{
   debug("Parameters memory space check...\n");
   for(size_t id = 0; id < __m_interfaces.size(); ++id)
   {
      debug("Checking parameter %zu memory space\n", id);
      __m_interfaces[id]->check();
   }
   debug("Waiting for simulator state report...\n");
   __ipc_wait(MDPI_IPC_STATE_REQUEST);
   assert(__m_ipc_operation.type == MDPI_OP_TYPE_STATE_CHANGE && "Unexpected simulator request.");
   assert(__m_ipc_operation.payload.sc.state == MDPI_STATE_READY && "Unexpected simulator state.");
   __m_ipc_operation.payload.sc.state = MDPI_STATE_SETUP;
   __ipc_response();
   debug("Simulator state: %s (%u)\n", mdpi_state_str(__m_ipc_operation.payload.sc.state),
         __m_ipc_operation.payload.sc.retval);
   debug("Launch simulation\n");

#ifdef BAMBU_CONCURRENT_COSIM
   int error = pthread_create(&__m_ipc_driver, NULL, __m_driver_loop, NULL);
   if(error)
   {
      error("An error occurred on co-simulation thread creation.\n");
      errno = error;
      perror("pthread_create");
      __ipc_abort();
   }
#else
   __m_driver_loop(NULL);
#endif
}

unsigned int __m_sim_end()
{
   unsigned int retval;

   debug("Waiting for simulator state report...\n");
#ifdef BAMBU_CONCURRENT_COSIM
   int error = pthread_join(__m_ipc_driver, NULL);
   if(error)
   {
      error("An error occurred on co-simulation thread join.\n");
      errno = error;
      perror("pthread_join");
      exit(EXIT_FAILURE);
   }
#endif

   assert(__m_ipc_operation.type == MDPI_OP_TYPE_STATE_CHANGE && "Unexpected simulator request.");
   retval = __m_ipc_operation.payload.sc.retval;
   debug("Simulator state: %s (%u)\n", mdpi_state_str(__m_ipc_operation.payload.sc.state), retval);
   return retval;
}

void __m_memmap_init(int map_mode)
{
   debug("Initializing co-simulation MMU\n");
   if(map_mode == MDPI_MEMMAP_DEVICE)
   {
      __m_mapper = std::unique_ptr<memmap>(new device_memmap());
   }
#if ENABLE_SHARED_MEMORY
   else if(map_mode == MDPI_MEMMAP_SHARED)
   {
      __m_mapper = std::unique_ptr<memmap>(new shared_memmap());
   }
#endif
   else
   {
      error("Unsupported memory mapping mode '%d'.", map_mode);
      exit(EXIT_FAILURE);
   }
}

int __m_memmap(ptr_t dst, void* src, size_t bytes)
{
   return __m_mapper->map(dst, src, bytes);
}

size_t __m_param_size(uint8_t idx)
{
   const std::map<uint8_t, size_t>::iterator mps_it = __m_params_size.find(idx);
   if(mps_it != __m_params_size.end())
   {
      return mps_it->second;
   }
   error("Parameter size for parameter %u has not been set.\n", idx);
   exit(EXIT_FAILURE);
   return 0;
}

void __m_param_alloc(uint8_t idx, size_t size)
{
   if(!__m_params_size.count(idx))
   {
      __m_params_size[idx] = size;
      debug("Memory size for parameter %u set to %zu bytes.\n", idx, size);
   }
}

static void* __m_driver_loop(void*)
{
   debug("IPC thread started.\n");

   while(true)
   {
      __ipc_wait(MDPI_IPC_STATE_REQUEST);
      switch(__m_ipc_operation.type)
      {
         case MDPI_OP_TYPE_STATE_CHANGE:
            return NULL;
         case MDPI_OP_TYPE_IF_READ:
         case MDPI_OP_TYPE_IF_WRITE:
         case MDPI_OP_TYPE_IF_INFO:
            if(__m_interfaces.empty())
            {
               error("Operation on uninitialized interfaces' list.\n");
               __m_ipc_operation.payload.interface.id = MDPI_IF_IDX_EMPTY;
            }
            else if(__m_interfaces.size() <= __m_ipc_operation.payload.interface.id)
            {
               error("Interface id out of bounds: %u.\n", __m_ipc_operation.payload.interface.id);
               __m_ipc_operation.payload.interface.id = MDPI_IF_IDX_OUT_OF_BOUNDS;
            }
            else
            {
               debug("Interface %u operation: ", __m_ipc_operation.payload.interface.id);
               if(__m_ipc_operation.type & MDPI_OP_TYPE_IF_READ)
               {
                  debug_append("read %u bits at " PTR_FORMAT ".\n", __m_ipc_operation.payload.interface.bitsize,
                               __m_ipc_operation.payload.interface.addr);
                  __m_ipc_operation.payload.interface.info =
                      __m_interfaces.at(__m_ipc_operation.payload.interface.id)
                          ->read(__m_ipc_operation.payload.interface.buffer,
                                 __m_ipc_operation.payload.interface.bitsize, __m_ipc_operation.payload.interface.addr,
                                 __m_ipc_operation.payload.interface.info);
               }
               else if(__m_ipc_operation.type & MDPI_OP_TYPE_IF_WRITE)
               {
                  debug_append("write %u bits at " PTR_FORMAT ".\n", __m_ipc_operation.payload.interface.bitsize,
                               __m_ipc_operation.payload.interface.addr);
                  __m_ipc_operation.payload.interface.info =
                      __m_interfaces.at(__m_ipc_operation.payload.interface.id)
                          ->write(__m_ipc_operation.payload.interface.buffer,
                                  __m_ipc_operation.payload.interface.bitsize, __m_ipc_operation.payload.interface.addr,
                                  __m_ipc_operation.payload.interface.info);
               }
               else
               {
                  debug_append("state (data: %u).\n", __m_ipc_operation.payload.interface.info);
                  __m_ipc_operation.payload.interface.info = __m_interfaces.at(__m_ipc_operation.payload.interface.id)
                                                                 ->state(__m_ipc_operation.payload.interface.info);
               }
            }
            __ipc_response();
            break;
         case MDPI_OP_TYPE_IF_EXIT:
            assert(__m_interfaces.size() <= 2 && "Expected single interface on builtin exit/abort.");
            __m_interfaces.at(0)->write(reinterpret_cast<bptr_t>(&__m_ipc_operation.payload.interface.info),
                                        sizeof(__m_ipc_operation.payload.interface.info) * 8, 0, 0);
            __ipc_exit(MDPI_STATE_END, EXIT_SUCCESS);
            return NULL;
         case MDPI_OP_TYPE_NONE:
         default:
            error("Unexpected transaction type: %u\n", __m_ipc_operation.type);
            __ipc_abort();
            break;
      }
   }
   debug("IPC thread completed.\n");
   return NULL;
}

void m_param_alloc(uint8_t idx, size_t size)
{
   info("Memory size for parameter %u set to %zu bytes.\n", idx, size);
   __m_params_size[idx] = size;
}

EXTERN_C float m_float_distancef(float a, float b)
{
   return m_float_distance<float>(a, b);
}

EXTERN_C double m_float_distance(double a, double b)
{
   return m_float_distance<double>(a, b);
}

EXTERN_C long double m_float_distancel(long double a, long double b)
{
   return m_float_distance<long double>(a, b);
}

EXTERN_C float m_floats_distancef(float a, float b)
{
   return m_floats_distance<float>(a, b);
}

EXTERN_C double m_floats_distance(double a, double b)
{
   return m_floats_distance<double>(a, b);
}

EXTERN_C long double m_floats_distancel(long double a, long double b)
{
   return m_floats_distance<long double>(a, b);
}
