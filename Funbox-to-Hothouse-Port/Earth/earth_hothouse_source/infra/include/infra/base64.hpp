/*=============================================================================
   Copyright (c) 2016-2023 Joel de Guzman

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#if !defined(INFRA_BASE64_OCTOBER_31_2023)
#define INFRA_BASE64_OCTOBER_31_2023

#include <vector>
#include <string>
#include <array>
#include <cstddef>

namespace cycfi
{
   std::string             base64_encode(std::basic_string_view<std::byte> in);
   std::vector<std::byte>  base64_decode(std::string_view in);

///////////////////////////////////////////////////////////////////////////////
// Inline Implementation
///////////////////////////////////////////////////////////////////////////////
   constexpr char const base64_chars[]
      = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/"
      ;

   constexpr std::int8_t const base64_lu[] = {
      -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 62, -1, -1, -1, 63,
      52, 53, 54, 55, 56, 57, 58, 59, 60, 61, -1, -1, -1, -1, -1, -1,
      -1, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14,
      15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, -1, -1, -1, -1, -1,
      -1, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40,
      41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, -1, -1, -1, -1, -1,
   };

   inline std::string base64_encode(std::basic_string_view<std::byte> in)
   {
      std::string out;
      unsigned val = 0;
      int valb = -6;
      for (auto c : in)
      {
         val = (val << 8) + static_cast<int>(c);
         valb += 8;
         while (valb >= 0)
         {
            out.push_back(base64_chars[(val >> valb) & 0x3F]);
            valb -= 6;
         }
      }
      if (valb > -6)
         out.push_back(base64_chars[((val << 8) >> (valb + 8)) &0x3F]);
      while (out.size() % 4)
         out.push_back('=');
      return out;
   }

   inline std::vector<std::byte> base64_decode(std::string_view in)
   {
      std::vector<std::byte> out;
      unsigned val = 0;
      int valb = -8;
      for (auto c : in)
      {
         if (base64_lu[c] == -1)
            break;
         val = (val << 6) + base64_lu[c];
         valb += 6;
         if (valb >= 0)
         {
            out.push_back(static_cast<std::byte>((val >> valb) & 0xFF));
            valb -= 8;
         }
      }
      return out;
   }
}

#endif
