// Copyright (C) 2025-2026 Politecnico di Milano
//
// Part of the PandA/Bambu MDPI Library, under the Apache License v2.0 with LLVM Exceptions.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//    author Michele Fiorito <michele.fiorito@polimi.it>
//

#define __BAMBU_IPC_ENTITY MDPI_ENTITY_SIM

#if __M_OUT_LVL <= 4
#define NDEBUG
#endif

#include <mdpi/mdpi_debug.h>
#include <mdpi/mdpi_pp.h>

#include <assert.h>
#include <string.h>

#define MASK(bw) (0xffffffffffffffff >> (64 - bw))

#ifdef BAMBU_CSIM
#include <mdpi/mdpi_csim.h>

static_assert(sizeof(ptr_t) == sizeof(void*),
              "C simulation executable must be compiled using proper target architecture");

unsigned long long bambu_artificial_ParmMgr_Read(uint8_t idx, uint16_t bitsize, void* addr)
{
   assert(bitsize <= 64 && "Wider bitwidth not yet supported");
   unsigned long long val = 0;
   m_read(idx, (byte_t*)&val, bitsize, (ptr_t)addr, -1);
   val &= MASK(bitsize);
   return val;
}

void bambu_artificial_ParmMgr_Write(uint8_t idx, uint16_t bitsize, unsigned long long data, void* addr)
{
   assert(bitsize <= 64 && "Wider bitwidth not yet supported");
   unsigned long long val = data & MASK(bitsize);
   m_write(idx, (byte_t*)&val, bitsize, (ptr_t)addr, -1);
}
#else
#define ceil_bytes(bw) (bw / 8 + ((bw % 8) != 0))

unsigned long long bambu_artificial_ParmMgr_Read(uint8_t idx, uint16_t bitsize, void* addr)
{
   assert(bitsize <= 64 && "Wider bitwidth not yet supported");
   unsigned long long val = 0;
   memcpy(&val, addr, ceil_bytes(bitsize));
   val &= MASK(bitsize);
   return val;
}

void bambu_artificial_ParmMgr_Write(uint8_t idx, uint16_t bitsize, unsigned long long data, void* addr)
{
   assert(bitsize <= 64 && "Wider bitwidth not yet supported");
   unsigned long long val = data & MASK(bitsize);
   memcpy(addr, &val, ceil_bytes(bitsize));
}
#endif

unsigned long long bambu_artificial_ParmMgr(uint8_t idx, bool readWrite, uint16_t bitsize, unsigned long long wdata,
                                            void* addr)
{
   if(readWrite)
   {
      bambu_artificial_ParmMgr_Write(idx, bitsize, wdata, addr);
      return 0;
   }
   return bambu_artificial_ParmMgr_Read(idx, bitsize, addr);
}
