#include "frontend_analysis/IR_analysis/constdiv_magic.hpp"

#include <boost/test/unit_test.hpp>

#include <cstdint>
#include <vector>

namespace
{
using constdiv_magic::SDivMagic64;
using constdiv_magic::UDivMagic64;
using constdiv_magic::computeSDivMagic64;
using constdiv_magic::computeUDivMagic64;
using constdiv_magic::maskForWidth;
using constdiv_magic::signExtend;

uint64_t trunc_u64(uint64_t value, unsigned width)
{
   return value & maskForWidth(width);
}

int64_t trunc_s64(int64_t value, unsigned width)
{
   return signExtend(static_cast<uint64_t>(value), width);
}

uint64_t mulhu(uint64_t x, uint64_t y, unsigned width)
{
   const __uint128_t prod = static_cast<__uint128_t>(x) * static_cast<__uint128_t>(y);
   return trunc_u64(static_cast<uint64_t>(prod >> width), width);
}

int64_t mulhs(int64_t x, int64_t y, unsigned width)
{
   const __int128 prod = static_cast<__int128>(x) * static_cast<__int128>(y);
   return trunc_s64(static_cast<int64_t>(prod >> width), width);
}

uint64_t udiv_magic(uint64_t numerator, uint64_t divisor, unsigned width, unsigned leading_zeros)
{
   const UDivMagic64 magic = computeUDivMagic64(divisor, width, leading_zeros, true);
   uint64_t q = trunc_u64(numerator, width);
   if(magic.preShift != 0)
   {
      q >>= magic.preShift;
   }
   q = mulhu(q, magic.magic, width);
   if(magic.isAdd)
   {
      uint64_t npq = trunc_u64(numerator - q, width);
      npq >>= 1;
      q = trunc_u64(npq + q, width);
   }
   if(magic.postShift != 0)
   {
      q >>= magic.postShift;
   }
   return trunc_u64(q, width);
}

int64_t sdiv_magic(int64_t numerator, int64_t divisor, unsigned width)
{
   const SDivMagic64 magic = computeSDivMagic64(divisor, width);
   const int64_t n = trunc_s64(numerator, width);
   const int64_t m = trunc_s64(static_cast<int64_t>(magic.magic), width);
   int64_t q = mulhs(n, m, width);
   if(magic.numeratorFactor == 1)
   {
      q = trunc_s64(q + n, width);
   }
   else if(magic.numeratorFactor == -1)
   {
      q = trunc_s64(q - n, width);
   }
   if(magic.shift != 0)
   {
      q = trunc_s64(q >> magic.shift, width);
   }
   uint64_t uq = trunc_u64(static_cast<uint64_t>(q), width);
   uint64_t t = (uq >> (width - 1)) & maskForWidth(width);
   const uint64_t shift_mask = trunc_u64(static_cast<uint64_t>(magic.shiftMask), width);
   t &= shift_mask;
   q = trunc_s64(static_cast<int64_t>(uq + t), width);
   return q;
}

std::vector<uint64_t> sample_values(unsigned width)
{
   std::vector<uint64_t> values = {0, 1, 2, 3, 5, 7, 10, 15, 16, 31, 32, 63, 64, 127, 128, 255, 257, 511};
   const uint64_t maxv = maskForWidth(width);
   values.push_back(maxv);
   values.push_back(maxv >> 1);
   for(auto& v : values)
   {
      v &= maxv;
   }
   return values;
}
} // namespace

BOOST_AUTO_TEST_CASE(ConstDivMagicUnsigned)
{
   const std::vector<unsigned> widths = {8, 16, 32, 64};
   const std::vector<uint64_t> divisors = {3, 5, 7, 10, 12, 255, 1024, 15, 17};
   for(const auto width : widths)
   {
      for(const auto d : divisors)
      {
         if(d == 0 || d == 1)
         {
            continue;
         }
         const uint64_t divisor = trunc_u64(d, width);
         if(divisor == 0 || divisor == 1)
         {
            continue;
         }
         const auto values = sample_values(width);
         for(const auto v : values)
         {
            const uint64_t num = trunc_u64(v, width);
            const uint64_t ref = trunc_u64(num / divisor, width);
            const uint64_t res = udiv_magic(num, divisor, width, 0);
            BOOST_CHECK_EQUAL(res, ref);
         }
      }
   }
}

BOOST_AUTO_TEST_CASE(ConstDivMagicSigned)
{
   const std::vector<unsigned> widths = {8, 16, 32, 64};
   const std::vector<int64_t> divisors = {3, -3, 5, -5, 7, -7, 9, -9};
   for(const auto width : widths)
   {
      for(const auto d : divisors)
      {
         if(d == 0 || d == 1 || d == -1)
         {
            continue;
         }
         const auto values = sample_values(width);
         for(const auto v : values)
         {
            const int64_t num = trunc_s64(static_cast<int64_t>(v), width);
            const int64_t div = trunc_s64(d, width);
            const int64_t min_val = trunc_s64(static_cast<int64_t>(1ULL << (width - 1)), width);
            if(num == min_val && div == -1)
            {
               continue;
            }
            const int64_t ref = trunc_s64(num / div, width);
            const int64_t res = sdiv_magic(num, div, width);
            BOOST_CHECK_EQUAL(res, ref);
         }
      }
   }
}
