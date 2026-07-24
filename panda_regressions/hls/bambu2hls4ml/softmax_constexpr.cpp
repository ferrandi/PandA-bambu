#include <array>

#include "ac_fixed.h"

typedef ac_fixed<35, 15> softmax_data_t;
typedef ac_fixed<18, 8> softmax_table_t;

constexpr int ceillog2(unsigned value)
{
   int result = 0;
   while((1U << result) < value)
   {
      ++result;
   }
   return result;
}

template <unsigned TableSize>
constexpr float softmax_real_val_from_idx(unsigned index)
{
   constexpr int address_bits = ceillog2(TableSize);
   softmax_data_t value(0);
   value(value.width - 1, value.width - address_bits) = index;
   return static_cast<float>(value);
}

template <unsigned I>
constexpr softmax_table_t make_softmax_table_entry()
{
   return softmax_table_t(softmax_real_val_from_idx<16>(I));
}

template <unsigned... I>
constexpr std::array<softmax_table_t, sizeof...(I)> make_softmax_table(std::integer_sequence<unsigned, I...>)
{
   return {{make_softmax_table_entry<I>()...}};
}

static constexpr auto softmax_table = make_softmax_table(std::make_integer_sequence<unsigned, 16>{});

void softmax_constexpr(softmax_table_t output[16])
{
   for(unsigned i = 0; i < 16; ++i)
   {
      output[i] = softmax_table[i];
   }
}
