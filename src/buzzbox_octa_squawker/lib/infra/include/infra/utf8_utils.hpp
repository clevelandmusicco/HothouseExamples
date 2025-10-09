/*=============================================================================
   Copyright (c) 2016-2023 Joel de Guzman

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#if !defined(INFRA_UTF8_UTILS_MAY_22_2016)
#define INFRA_UTF8_UTILS_MAY_22_2016

#include <string>
#include <string_view>
#include <cctype>

namespace cycfi
{
   ////////////////////////////////////////////////////////////////////////////
   bool              is_space(char32_t cp);
   bool              is_newline(char32_t cp);
   bool              is_punctuation(char32_t cp);

   bool              is_space(char const* utf8);
   bool              is_newline(char const* utf8);
   bool              is_punctuation(char const* utf8);

   std::string       codepoint_to_utf8(char32_t codepoint);
   char32_t          decode_utf8(char32_t& state, char32_t& codepoint, char32_t byte);
   char const*       next_utf8(char const* utf8, char const* last);
   char const*       prev_utf8(char const* utf8, char const* first);
   char32_t          codepoint(char const*& utf8);
   std::string       codepoint_to_utf8(char32_t codepoint);
   std::string       to_utf8(std::u32string_view utf32);
   std::u32string    to_utf32(std::string_view s);
   bool              is_valid_utf8(std::string_view s);

   ////////////////////////////////////////////////////////////////////////////
   // Inlines
   ////////////////////////////////////////////////////////////////////////////
   namespace detail
   {
      inline char const* codepoint_to_utf8(char32_t cp, char str[8])
      {
         int n = 0;
         if (cp < 0x80) n = 1;
         else if (cp < 0x800) n = 2;
         else if (cp < 0x10000) n = 3;
         else if (cp < 0x200000) n = 4;
         else if (cp < 0x4000000) n = 5;
         else if (cp <= 0x7fffffff) n = 6;
         str[n] = '\0';

         switch (n)
         {
            case 6: str[5] = 0x80 | (cp & 0x3f); cp = cp >> 6; cp |= 0x4000000; [[fallthrough]];
            case 5: str[4] = 0x80 | (cp & 0x3f); cp = cp >> 6; cp |= 0x200000;  [[fallthrough]];
            case 4: str[3] = 0x80 | (cp & 0x3f); cp = cp >> 6; cp |= 0x10000;   [[fallthrough]];
            case 3: str[2] = 0x80 | (cp & 0x3f); cp = cp >> 6; cp |= 0x800;     [[fallthrough]];
            case 2: str[1] = 0x80 | (cp & 0x3f); cp = cp >> 6; cp |= 0xc0;      [[fallthrough]];
            case 1: str[0] = cp;
         }
         return str;
      }
   }

   inline std::string codepoint_to_utf8(char32_t codepoint)
   {
      char cache[8];
      return detail::codepoint_to_utf8(codepoint, cache);
   }

   inline std::string to_utf8(std::u32string_view utf32)
   {
      std::string utf8;
      for (auto cp : utf32)
      {
         char str[8];
         detail::codepoint_to_utf8(cp, str);
         utf8 += str;
      }
      return utf8;
   }

   inline bool is_space(char32_t codepoint)
   {
      switch (codepoint)
      {
         case 9:        // \t
         case 11:       // \v
         case 12:       // \f
         case 32:       // space
			case 10:		   // \n
			case 13:		   // \r
         case 0xA0:     // NBSP
            return true;
         default:
            return false;
      }
   }

   // Check if codepoint is a new line
   inline bool is_newline(char32_t codepoint)
   {
      switch (codepoint)
      {
			case 10:		   // \n
			case 13:		   // \r
			case 0x85:	   // NEL
            return true;
         default:
            return false;
      }
   }

   // Check if codepoint is a punctuation
   inline bool is_punctuation(char32_t codepoint)
   {
      return (codepoint < 0x80 && std::ispunct(codepoint))
         || (codepoint >= 0xA0 && codepoint <= 0xBF)
         || (codepoint >= 0x2000 && codepoint <= 0x206F)
         ;
   }

   inline bool is_space(char const* utf8)
   {
      return is_space(codepoint(utf8));
   }

   inline bool is_newline(char const* utf8)
   {
      return is_newline(codepoint(utf8));
   }

   inline bool is_punctuation(char const* utf8)
   {
      return is_newline(codepoint(utf8));
   }

   ////////////////////////////////////////////////////////////////////////////
   // Decoding utf8
   //
   // Copyright (c) 2008-2010 Bjoern Hoehrmann <bjoern@hoehrmann.de>
   // See http://bjoern.hoehrmann.de/utf-8/decoder/dfa/ for details.
   ////////////////////////////////////////////////////////////////////////////

   enum
   {
      utf8_accept = 0,
      utf8_reject = 12
   };

   inline char32_t decode_utf8(char32_t& state, char32_t& codepoint, uint8_t byte)
   {
      static constexpr uint8_t utf8d[] =
      {
         // The first part of the table maps bytes to character classes that
         // to reduce the size of the transition table and create bitmasks.
         0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,       0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
         0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,       0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
         0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,       0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
         0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,       0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
         1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,       9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,
         7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,       7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
         8,8,2,2,2,2,2,2,2,2,2,2,2,2,2,2,       2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
         10,3,3,3,3,3,3,3,3,3,3,3,3,4,3,3,      11,6,6,6,5,8,8,8,8,8,8,8,8,8,8,8,

         // The second part is a transition table that maps a combination
         // of a state of the automaton and a character class to a state.
         0,12,24,36,60,96,84,12,12,12,48,72,    12,12,12,12,12,12,12,12,12,12,12,12,
         12, 0,12,12,12,12,12, 0,12, 0,12,12,   12,24,12,12,12,12,12,24,12,24,12,12,
         12,12,12,12,12,12,12,24,12,12,12,12,   12,24,12,12,12,12,12,12,12,24,12,12,
         12,12,12,12,12,12,12,36,12,36,12,12,   12,36,12,12,12,12,12,36,12,36,12,12,
         12,36,12,12,12,12,12,12,12,12,12,12,
      };

      char32_t type = utf8d[byte];

      codepoint = (state != utf8_accept) ?
         (byte & 0x3fu) | (codepoint << 6) :
         (0xff >> type) & (byte)
         ;

      state = utf8d[256 + state + type];
      return state;
   }

   ////////////////////////////////////////////////////////////////////////////
   // utf8 Iteration. See A code point iterator adapter for C++ strings in
   // UTF-8 by Ángel José Riesgo: http://www.nubaria.com/en/blog/?p=371
   ////////////////////////////////////////////////////////////////////////////
   struct utf8_mask
   {
      static uint8_t const first    = 128;   // 1000000
      static uint8_t const second   = 64;    // 0100000
      static uint8_t const third    = 32;    // 0010000
      static uint8_t const fourth   = 16;    // 0001000
   };

   inline char const* next_utf8(char const* utf8, char const* last)
   {
      char c = *utf8;
      std::size_t offset = 1;

      if (c & utf8_mask::first)
         offset =
            (c & utf8_mask::third)?
               ((c & utf8_mask::fourth)? 4 : 3) : 2
         ;

      utf8 += offset;
      if (utf8 > last)
         utf8 = last;
      return utf8;
   }

   inline char const* prev_utf8(char const* utf8, char const* first)
   {
      if (*--utf8 & utf8_mask::first)
      {
         if ((*--utf8 & utf8_mask::second) == 0)
         {
            if ((*--utf8 & utf8_mask::second) == 0)
               --utf8;
         }
      }
      if (utf8 < first)
         utf8 = first;
      return utf8;
   }

   ////////////////////////////////////////////////////////////////////////////
   // Extracting codepoints from utf8
   ////////////////////////////////////////////////////////////////////////////
   inline char32_t codepoint(char const*& utf8)
   {
      char32_t state = 0;
      char32_t cp;
      while (decode_utf8(state, cp, uint8_t(*utf8)))
         utf8++;
      ++utf8; // one past the last byte
      return cp;
   }

   ////////////////////////////////////////////////////////////////////////////
   // Converting utf8 to u32string
   ////////////////////////////////////////////////////////////////////////////
   inline std::u32string to_utf32(std::string_view s)
   {
      std::u32string s32;
      char const* last = s.data() + s.size();
      char32_t state = 0;
      char32_t cp;
      for (char const* i = s.data(); i != last; ++i)
      {
         while (decode_utf8(state, cp, uint8_t(*i)))
            i++;
         s32.push_back(cp);
      }
      if (state == utf8_reject)
         throw std::runtime_error{"Error: Invalid utf8."};
      return s32;
   }

   ////////////////////////////////////////////////////////////////////////////
   // Check for valid utf8
   ////////////////////////////////////////////////////////////////////////////
   inline bool is_valid_utf8(std::string_view s)
   {
      char const* last = s.data() + s.size();
      char32_t state = 0;
      char32_t cp;
      for (char const* i = s.data(); i != last; ++i)
      {
         while (decode_utf8(state, cp, uint8_t(*i)))
            i++;
      }
      return state != utf8_reject;
   }
}

#endif
