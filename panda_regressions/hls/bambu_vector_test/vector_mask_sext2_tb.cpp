// Copyright (C) 2026 Politecnico di Milano
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
#include <cstdint>

extern "C" std::uint64_t mask_sext2(std::uint32_t a, std::uint32_t b);

int main()
{
   return mask_sext2(5, 5) == 0xffffffffffffffffULL && mask_sext2(5, 9) == 0xffffffff00000000ULL ? 0 : 1;
}
