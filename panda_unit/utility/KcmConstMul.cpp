#include "frontend_analysis/IR_analysis/kcm_constmul.hpp"

#include <boost/test/unit_test.hpp>

#include <cstdint>
#include <vector>

namespace
{
using kcm_constmul::kcmMulSigned;
using kcm_constmul::kcmMulUnsigned;
using kcm_constmul::truncSigned;
using kcm_constmul::truncUnsigned;
} // namespace

BOOST_AUTO_TEST_CASE(KcmConstMulUnsigned)
{
   const std::vector<unsigned> widths = {4, 6, 8};
   const std::vector<unsigned> alphas = {4, 6};
   const std::vector<uint64_t> coeffs = {0, 1, 2, 3, 5, 7, 9, 15, 31};
   const std::vector<uint64_t> samples = {0, 1, 2, 3, 5, 7, 9, 12, 15, 31, 63, 127, 255};

   for(const auto width : widths)
   {
      for(const auto alpha : alphas)
      {
         if(alpha > width)
         {
            continue;
         }
         for(const auto coeff : coeffs)
         {
            for(const auto x : samples)
            {
               const uint64_t ux = truncUnsigned(x, width);
               const uint64_t uc = truncUnsigned(coeff, width);
               const uint64_t ref = truncUnsigned(ux * uc, width);
               const uint64_t res = kcmMulUnsigned(ux, uc, width, alpha);
               BOOST_CHECK_EQUAL(res, ref);
            }
         }
      }
   }
}

BOOST_AUTO_TEST_CASE(KcmConstMulSigned)
{
   const std::vector<unsigned> widths = {6, 8};
   const std::vector<unsigned> alphas = {4, 6};
   const std::vector<int64_t> coeffs = {0, 1, -1, 3, -3, 5, -5, 8, -8, 15, -15};
   const std::vector<int64_t> samples = {0, 1, -1, 2, -2, 7, -7, 15, -16, 31, -32};

   for(const auto width : widths)
   {
      for(const auto alpha : alphas)
      {
         if(alpha > width)
         {
            continue;
         }
         for(const auto coeff : coeffs)
         {
            for(const auto x : samples)
            {
               const int64_t sx = truncSigned(x, width);
               const int64_t sc = truncSigned(coeff, width);
               const int64_t ref = truncSigned(sx * sc, width);
               const int64_t res = kcmMulSigned(sx, sc, width, alpha);
               BOOST_CHECK_EQUAL(res, ref);
            }
         }
      }
   }
}
