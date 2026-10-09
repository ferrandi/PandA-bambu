/* Copyright (C) 2026 Politecnico di Milano
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */
// Compile the real channel adapters with each supported Clang. In the legacy ABI,
// composite template arguments change substitution indices; BoolPayload<bool&>
// also puts bool& in the substitution table before either valid/dummy argument.
#include "ac_channel.h"

template <class T, class U>
struct Pair
{
   T first;
   U second;
};

template <class T>
struct BoolPayload
{
   int value;
};

template <class T>
void exercise(ac_channel<T>& input, ac_channel<T>& output)
{
   output.write(input.read());
   output.write(input.peek());
   T value{};
   if(input.nb_read(value))
   {
      output.write(value);
   }
   if(input.nb_peek(value))
   {
      output.write(value);
   }
}

void abi_int(ac_channel<int>& input, ac_channel<int>& output)
{
   exercise(input, output);
}

void abi_pair(ac_channel<Pair<int, int>>& input, ac_channel<Pair<int, int>>& output)
{
   exercise(input, output);
}

void abi_bool_reference(ac_channel<BoolPayload<bool&>>& input, ac_channel<BoolPayload<bool&>>& output)
{
   exercise(input, output);
}
