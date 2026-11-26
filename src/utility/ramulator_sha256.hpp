// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
#pragma once
#include <array>
#include <cstdint>
#include <iomanip>
#include <sstream>
#include <string>

// FIPS 180-4 SHA-256, used for reproducible model identity without a runtime
// command or an additional cryptographic library dependency.
inline std::string ramulator_sha256(const std::string& input)
{
   constexpr std::array<uint32_t, 64> k{{
       0x428a2f98,0x71374491,0xb5c0fbcf,0xe9b5dba5,0x3956c25b,0x59f111f1,0x923f82a4,0xab1c5ed5,
       0xd807aa98,0x12835b01,0x243185be,0x550c7dc3,0x72be5d74,0x80deb1fe,0x9bdc06a7,0xc19bf174,
       0xe49b69c1,0xefbe4786,0x0fc19dc6,0x240ca1cc,0x2de92c6f,0x4a7484aa,0x5cb0a9dc,0x76f988da,
       0x983e5152,0xa831c66d,0xb00327c8,0xbf597fc7,0xc6e00bf3,0xd5a79147,0x06ca6351,0x14292967,
       0x27b70a85,0x2e1b2138,0x4d2c6dfc,0x53380d13,0x650a7354,0x766a0abb,0x81c2c92e,0x92722c85,
       0xa2bfe8a1,0xa81a664b,0xc24b8b70,0xc76c51a3,0xd192e819,0xd6990624,0xf40e3585,0x106aa070,
       0x19a4c116,0x1e376c08,0x2748774c,0x34b0bcb5,0x391c0cb3,0x4ed8aa4a,0x5b9cca4f,0x682e6ff3,
       0x748f82ee,0x78a5636f,0x84c87814,0x8cc70208,0x90befffa,0xa4506ceb,0xbef9a3f7,0xc67178f2}};
   std::array<uint32_t, 8> h{{0x6a09e667,0xbb67ae85,0x3c6ef372,0xa54ff53a,
                             0x510e527f,0x9b05688c,0x1f83d9ab,0x5be0cd19}};
   const auto rotate = [](uint32_t x, unsigned n) { return (x >> n) | (x << (32 - n)); };
   std::string padded = input;
   padded.push_back(static_cast<char>(0x80));
   while(padded.size() % 64 != 56) padded.push_back(0);
   const auto bits = static_cast<uint64_t>(input.size()) * 8;
   for(int i = 7; i >= 0; --i) padded.push_back(static_cast<char>(bits >> (8 * i)));
   for(size_t block = 0; block < padded.size(); block += 64)
   {
      std::array<uint32_t, 64> w{};
      for(unsigned i = 0; i < 16; ++i)
         for(unsigned j = 0; j < 4; ++j)
            w[i] = (w[i] << 8) | static_cast<unsigned char>(padded[block + 4 * i + j]);
      for(unsigned i = 16; i < 64; ++i)
      {
         const auto s0 = rotate(w[i-15],7) ^ rotate(w[i-15],18) ^ (w[i-15] >> 3);
         const auto s1 = rotate(w[i-2],17) ^ rotate(w[i-2],19) ^ (w[i-2] >> 10);
         w[i] = w[i-16] + s0 + w[i-7] + s1;
      }
      auto a=h[0], b=h[1], c=h[2], d=h[3], e=h[4], f=h[5], g=h[6], z=h[7];
      for(unsigned i = 0; i < 64; ++i)
      {
         const auto s1 = rotate(e,6) ^ rotate(e,11) ^ rotate(e,25);
         const auto ch = (e & f) ^ (~e & g);
         const auto t1 = z + s1 + ch + k[i] + w[i];
         const auto s0 = rotate(a,2) ^ rotate(a,13) ^ rotate(a,22);
         const auto maj = (a & b) ^ (a & c) ^ (b & c);
         const auto t2 = s0 + maj;
         z=g; g=f; f=e; e=d+t1; d=c; c=b; b=a; a=t1+t2;
      }
      h[0]+=a; h[1]+=b; h[2]+=c; h[3]+=d; h[4]+=e; h[5]+=f; h[6]+=g; h[7]+=z;
   }
   std::ostringstream output;
   output << std::hex << std::setfill('0');
   for(auto value : h) output << std::setw(8) << value;
   return output.str();
}
