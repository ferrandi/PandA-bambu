// Copyright (C) 2026 Politecnico di Milano
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
#include <cstdint>

extern "C" std::uint64_t mask_zext2(std::uint32_t a, std::uint32_t b);

int main()
{
   return mask_zext2(5, 5) == 0x0000000100000001ULL && mask_zext2(5, 9) == 0x0000000100000000ULL ? 0 : 1;
}
