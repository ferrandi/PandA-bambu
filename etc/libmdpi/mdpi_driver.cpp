// Copyright (C) 2023-2026 Politecnico di Milano
//
// Part of the PandA/Bambu MDPI Library, under the Apache License v2.0 with LLVM Exceptions.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//    author Michele Fiorito <michele.fiorito@polimi.it>
//    author Jesun Firoz <jesun.firoz@pnnl.gov>
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
#include <iterator>
#include <limits>
#include <map>
#include <memory>
#include <queue>
#include <string>
#include <sys/wait.h>
#include <unistd.h>
#include <vector>

#ifdef BAMBU_CONCURRENT_COSIM
#include <pthread.h>
#endif

// Conditional Ramulator2 includes for memory simulation
#ifdef HAVE_RAMULATOR2
#include <climits>
#include <cmath>
#include <filesystem>
#include <stdexcept>
#include "base/config.h"
#include "frontend/frontend.h"
#include "memory_system/memory_system.h"
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
         error("Address " PTR_FORMAT " is not mapped to any memory space.\n", sim_addr);
         error("Nearest memory space is [" PTR_FORMAT ", " PTR_FORMAT "] -> [" BPTR_FORMAT ", " BPTR_FORMAT
               "] (%zu bytes).\n",
               mmu_base->first, mmu_it->first, bptr_to_int(mmu_base->second + mmu_base->first),
               bptr_to_int(mmu_base->second + mmu_it->first), static_cast<size_t>(mmu_it->first - mmu_base->first));
         return 0;
      }

      size_t region_avail(ptr_t sim_addr) override
      {
         auto region = std::prev(__m_mmu.upper_bound(sim_addr));
         if(region == __m_mmu.begin() || !region->second)
         {
            return 0;
         }
         const auto next = std::next(region);
         return next == __m_mmu.end() ? std::numeric_limits<size_t>::max() :
                                       static_cast<size_t>(next->first - sim_addr);
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
    protected:
      static bool valid_range(ptr_t addr, size_t bytes)
      {
         return addr && bytes && bytes - 1 <= std::numeric_limits<ptr_t>::max() - addr;
      }

      static bool mapped_range(ptr_t addr, size_t bytes)
      {
         if(!valid_range(addr, bytes))
         {
            return false;
         }
         for(size_t i = 0; i < bytes; ++i)
         {
            if(__m_mapper->region_avail(addr + i))
            {
               return true;
            }
         }
         return false;
      }

      static bool read_memory(bptr_t data, size_t bytes, ptr_t addr)
      {
         if(!valid_range(addr, bytes))
         {
            return false;
         }
         bool mapped = false;
         std::memset(data, 0, bytes);
         size_t offset = 0;
         while(offset < bytes)
         {
            const auto available = __m_mapper->region_avail(addr + offset);
            if(!available)
            {
               ++offset; // Unmapped padding in a wide bus/cache-line transfer.
               continue;
            }
            const auto count = std::min(bytes - offset, available);
            std::memcpy(data + offset, __m_mapper->addrmap(addr + offset), count);
            mapped = true;
            offset += count;
         }
         return mapped;
      }

      static void write_memory(const bptr_t data, uint16_t bitsize, ptr_t addr)
      {
         const auto bytes = static_cast<size_t>(bitsize / 8);
         size_t offset = 0;
         while(offset < bytes)
         {
            const auto available = __m_mapper->region_avail(addr + offset);
            if(!available)
            {
               ++offset;
               continue;
            }
            const auto count = std::min(bytes - offset, available);
            std::memcpy(__m_mapper->addrmap(addr + offset), data + offset, count);
            offset += count;
         }
         if(bitsize % 8 && __m_mapper->region_avail(addr + bytes))
         {
            auto* target = __m_mapper->addrmap(addr + bytes);
            const auto mask = static_cast<byte_t>((1U << (bitsize % 8)) - 1);
            *target = (*target & ~mask) | (data[bytes] & mask);
         }
      }

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
         assert((bitsize % 8) == 0 && "Expected byte-aligned memory size");
         const auto bytes = static_cast<size_t>(bitsize / 8);
         if(!read_memory(data, bytes, addr))
         {
            if_error("Read to non-mapped address " PTR_FORMAT ".\n", addr);
            return IF_ERROR;
         }
#if __M_OUT_LVL > 4
         std::vector<char> hex_data(bytes * 2 + 1, '\0');
         for(size_t i = 0; i < bytes; ++i)
         {
            snprintf(&hex_data[i * 2], 3, "%02X", data[i]);
         }
#endif
         if_debug("Read %u bytes at " PTR_FORMAT " (data: 0x%s)\n", bitsize / 8, addr, hex_data.data());
         return IF_OK;
      }

      int write(bptr_t data, uint16_t bitsize, ptr_t addr, int /*cmd*/) override
      {
         if(!mapped_range(addr, (bitsize + 7) / 8))
         {
            if_error("Write to non-mapped address " PTR_FORMAT ".\n", addr);
            return IF_ERROR;
         }
         write_memory(data, bitsize, addr);
#if __M_OUT_LVL > 4
         const auto bytes = static_cast<size_t>((bitsize + 7) / 8);
         std::vector<char> hex_data(bytes * 2 + 1, '\0');
         for(size_t i = 0; i < bytes; ++i)
         {
            snprintf(&hex_data[i * 2], 3, "%02X", data[i]);
         }
#endif
         if_debug("Write %u bits at " PTR_FORMAT " (data: 0x%s)\n", bitsize, addr, hex_data.data());
         return IF_OK;
      }
   };

#ifdef HAVE_RAMULATOR2
   class mem_interface_ramulator : public mem_interface
   {
    private:
      struct request_t
      {
         uint16_t bitsize;
         ptr_t addr;
         uint8_t channel;
         uint64_t cycle;
         std::vector<byte_t> data;
         bool completed = false;
      };

      // Entries live until the TB collects the completion, including their tags.
      std::map<int, request_t> requests;
      std::map<uint8_t, std::queue<int>> completions;
      std::unique_ptr<Ramulator::IFrontEnd> frontend;
      std::unique_ptr<Ramulator::IMemorySystem> memory;
      int next_tag = 1;
      uint64_t current_cycle = 0;
      uint64_t dram_ticks = 0;
      uint64_t reads_issued = 0;
      uint64_t reads_completed = 0;
      uint64_t writes_issued = 0;
      long double clock_ratio = 3.0L;
      long double clock_phase = 0.0L;

      void report(const char* format, ...) const
      {
#if __M_OUT_LVL >= 3
         // The simulator shares stdout. Keep each evidence record below the
         // minimum PIPE_BUF and emit it in one write, outside stdio buffering.
         char record[512];
         const auto prefix = std::snprintf(record, sizeof(record), "%s: Interface %u: ",
                                           mdpi_entity_str(__BAMBU_IPC_ENTITY), static_cast<unsigned>(_idx));
         va_list args;
         va_start(args, format);
         const auto body = std::vsnprintf(record + prefix, sizeof(record) - prefix, format, args);
         va_end(args);
         if(body < 0 || static_cast<size_t>(prefix + body) >= sizeof(record))
         {
            if_error("Invalid Ramulator diagnostic record.\n");
            std::abort();
         }
         std::fflush(stdout);
         ssize_t result;
         do
         {
            result = ::write(STDOUT_FILENO, record, prefix + body);
         } while(result < 0 && errno == EINTR);
#else
         (void)format;
#endif
      }

      static void resolve_configuration(YAML::Node node, const std::filesystem::path& directory,
                                        std::vector<std::filesystem::path>& stack)
      {
         if(node.IsScalar() && node.Tag() == "!include")
         {
            node = load_configuration(directory / node.as<std::string>(), stack);
         }
         else if(node.IsSequence())
         {
            for(size_t i = 0; i < node.size(); ++i)
            {
               resolve_configuration(node[i], directory, stack);
            }
         }
         else if(node.IsMap())
         {
            for(auto child : node)
            {
               resolve_configuration(child.second, directory, stack);
            }
         }
      }

      static YAML::Node load_configuration(const std::filesystem::path& path,
                                           std::vector<std::filesystem::path>& stack)
      {
         const auto absolute = std::filesystem::weakly_canonical(std::filesystem::absolute(path));
         if(!std::filesystem::is_regular_file(absolute))
         {
            throw std::runtime_error("Ramulator configuration is not a readable file: " + absolute.string());
         }
         if(std::find(stack.begin(), stack.end(), absolute) != stack.end())
         {
            throw std::runtime_error("Cyclic Ramulator configuration include: " + absolute.string());
         }
         stack.push_back(absolute);
         auto config = YAML::LoadFile(absolute.string());
         resolve_configuration(config, absolute.parent_path(), stack);
         stack.pop_back();
         return config;
      }

      static YAML::Node parse_configuration(const std::filesystem::path& path)
      {
         std::vector<std::filesystem::path> stack;
         return load_configuration(path, stack);
      }

      static YAML::Node configuration()
      {
         if(const auto* path = std::getenv("RAMULATOR2_CONFIG"))
         {
            if(*path)
            {
               // An explicit configuration must not silently select another model.
               return parse_configuration(path);
            }
         }
         const std::filesystem::path paths[] = {
             "ramulator2_config.yaml", std::filesystem::path(__FILE__).parent_path() / "configs/default_ddr4.yaml"};
         for(const auto& path : paths)
         {
            if(std::filesystem::is_regular_file(path))
            {
               return parse_configuration(path);
            }
         }
         // No shared /tmp filename: independent simulations must not race when
         // the installed configuration is absent.
         return YAML::Load(R"(
Frontend:
  impl: GEM5
MemorySystem:
  impl: GenericDRAM
  clock_ratio: 3
  DRAM:
    impl: DDR4
    org:
      preset: DDR4_8Gb_x8
      channel: 1
      rank: 1
    timing:
      preset: DDR4_2400R
  Controller:
    impl: Generic
    Scheduler:
      impl: FRFCFS
    RefreshManager:
      impl: AllBank
    RowPolicy:
      impl: ClosedRowPolicy
      cap: 4
  AddrMapper:
    impl: RoBaRaCoCh
)");
      }

      bool valid_size(uint16_t bitsize) const
      {
         return bitsize && bitsize <= sizeof(mdpi_op_interface_t::buffer) * 8;
      }

      void complete(int tag)
      {
         auto it = requests.find(tag);
         if(it == requests.end() || it->second.completed)
         {
            if_error("Unknown or duplicate Ramulator completion tag %d.\n", tag);
            std::abort();
         }
         auto& request = it->second;
         request.completed = true;
         completions[request.channel].push(tag);
         report("[MDPI-PROFILE] ID=%u,CYCLE=%llu,OP=READ,ADDR=" PTR_FORMAT ",BITSIZE=%u,LATENCY=%llu\n",
                 static_cast<unsigned>(_idx), static_cast<unsigned long long>(current_cycle), request.addr,
                 static_cast<unsigned>(request.bitsize),
                 static_cast<unsigned long long>(current_cycle - request.cycle));
      }

    public:
      mem_interface_ramulator(mdpi_idx_t idx, double clock_period_ns, uint16_t channels_number) : mem_interface(idx)
      {
         if(!std::isfinite(clock_period_ns) || clock_period_ns <= 0 || !channels_number)
         {
            throw std::invalid_argument("Invalid accelerator clock period or memory channel count");
         }
         auto config = configuration();
         auto system = config["MemorySystem"];
         if(system["clock_ratio"])
         {
            clock_ratio = system["clock_ratio"].as<long double>();
         }
         if(!std::isfinite(clock_ratio) || clock_ratio <= 0)
         {
            throw std::invalid_argument("MemorySystem.clock_ratio must be finite and positive");
         }
         // The external MDPI clock advances the model explicitly. Ramulator's
         // standalone scheduler expects an integer ratio; it is not used here.
         system["clock_ratio"] = 1;
         if(const auto* override_channels = std::getenv("RAMULATOR_OVERRIDE_CHANNELS"))
         {
            if(std::strcmp(override_channels, "1") == 0)
            {
               system["DRAM"]["org"]["channel"] = channels_number;
            }
            else if(std::strcmp(override_channels, "0") != 0 && *override_channels)
            {
               throw std::invalid_argument("RAMULATOR_OVERRIDE_CHANNELS must be 0 or 1");
            }
         }
         frontend.reset(Ramulator::Factory::create_frontend(config));
         memory.reset(Ramulator::Factory::create_memory_system(config));
         if(!frontend || !memory)
         {
            throw std::runtime_error("Ramulator factory did not create the frontend and memory system");
         }
         frontend->connect_memory_system(memory.get());
         memory->connect_frontend(frontend.get());
         report("Ramulator2 successfully initialized for interface %u\n", static_cast<unsigned>(_idx));
         report("Memory clock: %.9Lg DRAM ticks per accelerator cycle (%.9g ns).\n", clock_ratio, clock_period_ns);
      }

      ~mem_interface_ramulator() override
      {
         report("[MDPI-MEMORY] ID=%u,CYCLES=%llu,DRAM_TICKS=%llu,READS=%llu,COMPLETED=%llu,WRITES=%llu\n",
                 static_cast<unsigned>(_idx), static_cast<unsigned long long>(current_cycle),
                 static_cast<unsigned long long>(dram_ticks), static_cast<unsigned long long>(reads_issued),
                 static_cast<unsigned long long>(reads_completed), static_cast<unsigned long long>(writes_issued));
      }

      bool advance_to(uint64_t cycle)
      {
         if(cycle < current_cycle)
         {
            if_error("Memory clock moved backwards from %llu to %llu.\n",
                     static_cast<unsigned long long>(current_cycle), static_cast<unsigned long long>(cycle));
            return false;
         }
         while(current_cycle < cycle)
         {
            ++current_cycle;
            clock_phase += clock_ratio;
            while(clock_phase + 16 * std::numeric_limits<long double>::epsilon() >= 1.0L)
            {
               memory->tick();
               clock_phase -= 1.0;
               ++dram_ticks;
            }
         }
         return true;
      }

      int state(int /*data*/) override
      {
         return static_cast<int>(requests.size());
      }

      int read_request(uint16_t bitsize, ptr_t addr, int8_t channel)
      {
         if(!valid_size(bitsize))
         {
            if_error("Invalid asynchronous read size: %u bits.\n", bitsize);
            return IF_ERROR;
         }
         const auto initial_tag = next_tag;
         while(requests.count(next_tag))
         {
            next_tag = next_tag == INT_MAX ? 1 : next_tag + 1;
            if(next_tag == initial_tag)
            {
               return 0;
            }
         }
         const auto tag = next_tag;
         request_t request{bitsize, addr, static_cast<uint8_t>(channel), current_cycle,
                           std::vector<byte_t>((bitsize + 7) / 8, 0)};
         // Read data belongs to this transaction. A later accepted write must
         // not change an earlier read just because its DRAM callback is delayed.
         if(!read_memory(request.data.data(), request.data.size(), addr))
         {
            if_error("Read to non-mapped address " PTR_FORMAT ".\n", addr);
            return IF_ERROR;
         }
         if(bitsize % 8)
         {
            request.data.back() &= static_cast<byte_t>((1U << (bitsize % 8)) - 1);
         }
         requests.emplace(tag, std::move(request));
         if(!frontend->receive_external_requests(0, addr, 0, [this, tag](Ramulator::Request&) { complete(tag); }))
         {
            requests.erase(tag);
            return 0;
         }
         next_tag = tag == INT_MAX ? 1 : tag + 1;
         ++reads_issued;
         return tag;
      }

      int read_complete(bptr_t data, uint16_t& bitsize, uint8_t channel)
      {
         bitsize = 0;
         auto queue = completions.find(channel);
         if(queue == completions.end() || queue->second.empty())
         {
            return 0;
         }
         const auto tag = queue->second.front();
         queue->second.pop();
         const auto request = requests.find(tag);
         if(request == requests.end() || !request->second.completed)
         {
            if_error("Missing data for Ramulator completion tag %d.\n", tag);
            return IF_ERROR;
         }
         bitsize = request->second.bitsize;
         if(data)
         {
            std::memcpy(data, request->second.data.data(), request->second.data.size());
         }
         requests.erase(request);
         ++reads_completed;
         return tag;
      }

      int write_request(bptr_t data, uint16_t bitsize, ptr_t addr)
      {
         if(!data || !valid_size(bitsize))
         {
            if_error("Invalid asynchronous write size or data: %u bits.\n", bitsize);
            return IF_ERROR;
         }
         if(!mapped_range(addr, (bitsize + 7) / 8))
         {
            if_error("Write to non-mapped address " PTR_FORMAT ".\n", addr);
            return IF_ERROR;
         }
         if(!frontend->receive_external_requests(1, addr, 0, [](Ramulator::Request&) {}))
         {
            return 0;
         }
         write_memory(data, bitsize, addr);
         ++writes_issued;
         return 1;
      }
   };
#endif
} // namespace

void __m_interface_mem()
{
   __m_interface_set(new mem_interface(__m_interface_idx_next()));
}

static void __ipc_abort();

static void ramulator_initialization_error()
{
   // The simulator must publish READY before we send a terminal response;
   // otherwise it waits for a free IPC slot before issuing its first request.
   __ipc_wait(MDPI_IPC_STATE_REQUEST);
   __ipc_abort();
}

void __m_interface_mem_ramulator(double clock_period_ns, uint16_t channels_number)
{
#ifdef HAVE_RAMULATOR2
   try
   {
      __m_interface_set(new mem_interface_ramulator(__m_interface_idx_next(), clock_period_ns, channels_number));
   }
   catch(const std::exception& exception)
   {
      error("Ramulator2 initialization failed: %s\n", exception.what());
      ramulator_initialization_error();
   }
#else
   (void)clock_period_ns;
   (void)channels_number;
   error("Ramulator2 was requested, but the MDPI driver has no Ramulator support. "
         "Set RAMULATOR2_DIR to its installation prefix and rebuild the simulation.\n");
   ramulator_initialization_error();
#endif
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

void __m_sim_preflight_fail()
{
   debug("Waiting for simulator state report before preflight failure...\n");
   __ipc_wait(MDPI_IPC_STATE_REQUEST);
   assert(__m_ipc_operation.type == MDPI_OP_TYPE_STATE_CHANGE && "Unexpected simulator request.");
   assert(__m_ipc_operation.payload.sc.state == MDPI_STATE_READY && "Unexpected simulator state.");
   __m_exit(EXIT_FAILURE);
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
         case MDPI_OP_TYPE_MEM_TICK:
#ifdef HAVE_RAMULATOR2
            for(const auto& iface : __m_interfaces)
            {
               if(auto* memory = dynamic_cast<mem_interface_ramulator*>(iface.get()))
               {
                  if(!memory->advance_to(__m_ipc_operation.payload.interface.addr))
                  {
                     __ipc_abort();
                  }
               }
            }
#endif
            __ipc_response();
            break;
         case MDPI_OP_TYPE_IF_READ_REQUEST:
         case MDPI_OP_TYPE_IF_READ_COMPLETE:
         case MDPI_OP_TYPE_IF_WRITE_REQUEST:
         {
            const auto id = __m_ipc_operation.payload.interface.id;
#ifdef HAVE_RAMULATOR2
            auto* memory = id < __m_interfaces.size() ?
                               dynamic_cast<mem_interface_ramulator*>(__m_interfaces[id].get()) : nullptr;
            if(memory)
            {
               if(__m_ipc_operation.type == MDPI_OP_TYPE_IF_READ_REQUEST)
               {
                  __m_ipc_operation.payload.interface.info = memory->read_request(
                      __m_ipc_operation.payload.interface.bitsize, __m_ipc_operation.payload.interface.addr,
                      static_cast<int8_t>(__m_ipc_operation.payload.interface.info));
               }
               else if(__m_ipc_operation.type == MDPI_OP_TYPE_IF_READ_COMPLETE)
               {
                  const auto channel = static_cast<uint8_t>(__m_ipc_operation.payload.interface.info);
                  // The IPC record is packed: use a local for the output reference.
                  uint16_t bitsize = 0;
                  __m_ipc_operation.payload.interface.info =
                      memory->read_complete(__m_ipc_operation.payload.interface.buffer, bitsize, channel);
                  __m_ipc_operation.payload.interface.bitsize = bitsize;
               }
               else
               {
                  __m_ipc_operation.payload.interface.info = memory->write_request(
                      __m_ipc_operation.payload.interface.buffer, __m_ipc_operation.payload.interface.bitsize,
                      __m_ipc_operation.payload.interface.addr);
               }
            }
            else
#endif
            {
               error("Asynchronous memory operation on interface %u without a Ramulator backend.\n", id);
               __m_ipc_operation.payload.interface.info = interface::IF_ERROR;
               __m_ipc_operation.payload.interface.bitsize = 0;
            }
            __ipc_response();
            break;
         }
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
