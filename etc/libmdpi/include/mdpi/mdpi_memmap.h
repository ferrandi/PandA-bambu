// Copyright (C) 2023-2026 Politecnico di Milano
//
// Part of the PandA/Bambu MDPI Library, under the Apache License v2.0 with LLVM Exceptions.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//    author Michele Fiorito <michele.fiorito@polimi.it>
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
};

#endif // __MDPI_MEMMAP_H