// Copyright (C) 2023-2026 Politecnico di Milano
//
// Part of the PandA/Bambu MDPI Library, under the Apache License v2.0 with LLVM Exceptions.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//    author Michele Fiorito <michele.fiorito@polimi.it>
//    author Jesun Firoz <jesun.firoz@pnnl.gov>
//

#ifndef __MDPI_MEMMAP_H
#define __MDPI_MEMMAP_H

#include "mdpi_types.h"

#include <cstddef>

class memmap
{
 protected:
   memmap() = default;

 public:
   virtual ~memmap() = default;

   virtual void check() = 0;

   virtual int map(ptr_t dst, void* src, size_t bytes) = 0;

   virtual bptr_t addrmap(ptr_t sim_addr) = 0;

   virtual ptr_t mapaddr(const bptr_t addr) = 0;

   // Bytes writable at sim_addr before crossing into the next mapped region.
   // Used to clamp writes so a transaction addressed inside one buffer can
   // never spill into a physically-adjacent buffer. Default: no clamp.
   virtual size_t region_avail(ptr_t /*sim_addr*/)
   {
      return static_cast<size_t>(-1);
   }
};

#endif // __MDPI_MEMMAP_H
