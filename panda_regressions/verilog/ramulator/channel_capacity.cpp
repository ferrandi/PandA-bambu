// Copyright (C) 2026 Politecnico di Milano
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
#define __BAMBU_IPC_ENTITY MDPI_ENTITY_DRIVER
#include <mdpi/mdpi_driver.h>

#include <cassert>
#include <climits>
#include <cstdio>

struct Data16
{
   unsigned long long values[16];
};

int main()
{
   // Exercise the signed boundary without allocating billions of channel elements.
   const unsigned int capacities[] = {1U, static_cast<unsigned int>(INT_MAX), static_cast<unsigned int>(INT_MAX) + 1U,
                                      UINT_MAX};
   for(const auto capacity : capacities)
   {
      ac_channel<int> output;
      channel_interface<int> writer(0, output, capacity);
      const int expected_before = capacity > static_cast<unsigned int>(INT_MAX) ? INT_MAX : capacity;
      const int expected_after = capacity - 1U > static_cast<unsigned int>(INT_MAX) ? INT_MAX : capacity - 1U;
      assert(writer.state(4) == expected_before);
      int value = 42;
      assert(writer.write(reinterpret_cast<bptr_t>(&value), 32, 0, -1) == expected_after);
      assert(writer.state(4) == expected_after);
      assert(output.size() == 1 && output.read() == value);
      if(capacity == 1)
      {
         assert(writer.write(reinterpret_cast<bptr_t>(&value), 32, 0, -1) == interface::IF_FULL);
      }

      ac_channel<int> input;
      input.write(value);
      channel_interface<int> reader(0, input, capacity);
      assert(reader.state(2) == expected_before);
      int received = 0;
      assert(reader.read(reinterpret_cast<bptr_t>(&received), 32, 0, -1) == expected_after);
      assert(received == value && input.empty());
      assert(reader.state(2) == expected_after);
      if(capacity == 1)
      {
         assert(reader.read(reinterpret_cast<bptr_t>(&received), 32, 0, -1) == interface::IF_EMPTY);
      }
   }

   ac_channel<Data16> unbounded;
   channel_interface<Data16> writer(0, unbounded);
   assert(writer.state(2) == 0);
   assert(writer.state(4) > 0);
   Data16 value = {};
   for(unsigned int i = 0; i < 2; ++i)
   {
      value.values[0] = i + 7;
      assert(writer.write(reinterpret_cast<bptr_t>(&value), sizeof(value) * CHAR_BIT, 0, -1) > 0);
      assert(writer.state(4) > 0);
      assert(writer.state(2) == static_cast<int>(i + 1));
   }
   assert(unbounded.read().values[0] == 7);
   assert(unbounded.read().values[0] == 8);
   assert(writer.state(2) == 0);
   assert(writer.state(8) == interface::IF_ERROR);
   puts("PASS MDPI channel capacity boundaries and unbounded writes");
}
