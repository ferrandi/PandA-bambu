/*
 *
 *                   _/_/_/    _/_/   _/    _/ _/_/_/    _/_/
 *                  _/   _/ _/    _/ _/_/  _/ _/   _/ _/    _/
 *                 _/_/_/  _/_/_/_/ _/  _/_/ _/   _/ _/_/_/_/
 *                _/      _/    _/ _/    _/ _/   _/ _/    _/
 *               _/      _/    _/ _/    _/ _/_/_/  _/    _/
 *
 *             ***********************************************
 *                              PandA Project
 *                     URL: http://panda.dei.polimi.it
 *                       Politecnico di Milano - DEIB
 *                        System Architectures Group
 *             ***********************************************
 *              Copyright (C) 2004-2020 Politecnico di Milano
 *
 *   This file is part of the PandA framework.
 *
 *   The PandA framework is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */
/**
 * @file strong_typedef.hpp
 * @brief This class macros for the definition of strong typedef.
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 * @author Marco Lattuada <lattuada@elet.polimi.it>
 *
 */
#ifndef STRONG_TYPEDEF_HPP
#define STRONG_TYPEDEF_HPP

#ifdef NDEBUG
#define STRONG_TYPEDEF(original_type, new_type) typedef original_type new_type
#define STRONG_TYPEDEF_FORWARD_DECL(original_type, new_type) typedef original_type new_type

template <typename Dest, typename Source>
inline Dest from_strongtype_cast(Source source)
{
   return static_cast<Dest>(source);
}

#if 0
template<typename Dest, typename Source>
inline Dest to_strongtype_cast(Source source)
{
   return static_cast<Dest>(source);
}
#endif

#else
#include <boost/functional/hash/hash.hpp>
template <typename Dest, typename Source>
inline Dest from_strongtype_cast(Source source)
{
   return static_cast<Dest>(source.GetContent());
}

#if 0
template<typename Dest, typename Source>
inline Dest to_strongtype_cast(Source source)
{
   return Dest(source);
}
#endif

#define STRONG_TYPEDEF_FORWARD_DECL(OriginalType, NewType) class NewType
/// This is quite similar to boost strong typedef, but implicit conversion are disabled
#define STRONG_TYPEDEF(OriginalType, NewType)                                   \
   class NewType                                                                \
   {                                                                            \
    private:                                                                    \
      /*The actual value of the object*/                                        \
      OriginalType content;                                                     \
                                                                                \
    public:                                                                     \
      /* Explicit constructor */                                                \
      explicit NewType(const OriginalType _content) : content(_content)         \
      {                                                                         \
      }                                                                         \
                                                                                \
      friend std::ostream& operator<<(std::ostream& os, const NewType element); \
                                                                                \
      inline OriginalType GetContent() const                                    \
      {                                                                         \
         return content;                                                        \
      }                                                                         \
                                                                                \
      /* Overloading of -- */                                                   \
      NewType& operator--()                                                     \
      {                                                                         \
         content--;                                                             \
         return *this;                                                          \
      }                                                                         \
                                                                                \
      /* Overloading of ++ */                                                   \
      NewType& operator++()                                                     \
      {                                                                         \
         content++;                                                             \
         return *this;                                                          \
      }                                                                         \
                                                                                \
      /* Overloading of ++ */                                                   \
      NewType operator++(int)                                                   \
      {                                                                         \
         content++;                                                             \
         return NewType(content - 1);                                           \
      }                                                                         \
                                                                                \
      /* Overloading of + */                                                    \
      NewType operator+(const NewType& other) const                             \
      {                                                                         \
         return NewType(content + other.content);                               \
      }                                                                         \
      NewType operator+(const OriginalType& other) const                        \
      {                                                                         \
         return NewType(content + other);                                       \
      }                                                                         \
                                                                                \
      /* Overloading of += */                                                   \
      NewType operator+=(const NewType& other)                                  \
      {                                                                         \
         content += other.content;                                              \
         return *this;                                                          \
      }                                                                         \
      NewType operator+=(const OriginalType& other)                             \
      {                                                                         \
         content += other;                                                      \
         return *this;                                                          \
      }                                                                         \
                                                                                \
      /* Overloading of - */                                                    \
      NewType operator-(const NewType& other) const                             \
      {                                                                         \
         return NewType(content - other.content);                               \
      }                                                                         \
      NewType operator-(const OriginalType& other) const                        \
      {                                                                         \
         return NewType(content - other);                                       \
      }                                                                         \
                                                                                \
      /* Overloading of - */                                                    \
      NewType operator-() const                                                 \
      {                                                                         \
         return NewType(-content);                                              \
      }                                                                         \
                                                                                \
      /* Overloading of * */                                                    \
      NewType operator*(const NewType& other) const                             \
      {                                                                         \
         return NewType(content * other.content);                               \
      }                                                                         \
      NewType operator*(const OriginalType& other) const                        \
      {                                                                         \
         return NewType(content * other);                                       \
      }                                                                         \
                                                                                \
      /* Overloading of < */                                                    \
      bool operator<(const NewType& other) const                                \
      {                                                                         \
         return content < other.content;                                        \
      }                                                                         \
      bool operator<(const OriginalType& other) const                           \
      {                                                                         \
         return content < other;                                                \
      }                                                                         \
                                                                                \
      /* Overloading of <= */                                                   \
      bool operator<=(const NewType& other) const                               \
      {                                                                         \
         return content <= other.content;                                       \
      }                                                                         \
                                                                                \
      /* Overloading of > */                                                    \
      bool operator>(const NewType& other) const                                \
      {                                                                         \
         return content > other.content;                                        \
      }                                                                         \
      bool operator>(const OriginalType& other) const                           \
      {                                                                         \
         return content > other;                                                \
      }                                                                         \
                                                                                \
      /* Overloading of >= */                                                   \
      bool operator>=(const NewType& other) const                               \
      {                                                                         \
         return content >= other.content;                                       \
      }                                                                         \
                                                                                \
      /* Overloading of == */                                                   \
      bool operator==(const NewType& other) const                               \
      {                                                                         \
         return content == other.content;                                       \
      }                                                                         \
      bool operator==(const OriginalType& other) const                          \
      {                                                                         \
         return content == other;                                               \
      }                                                                         \
                                                                                \
      /* Overloading of != */                                                   \
      bool operator!=(const NewType& other) const                               \
      {                                                                         \
         return content != other.content;                                       \
      }                                                                         \
      bool operator!=(const OriginalType& other) const                          \
      {                                                                         \
         return content != other;                                               \
      }                                                                         \
   };                                                                           \
   inline std::ostream& operator<<(std::ostream& os, const NewType element)     \
   {                                                                            \
      os << element.content;                                                    \
      return os;                                                                \
   }                                                                            \
                                                                                \
   namespace std                                                                \
   {                                                                            \
      template <>                                                               \
      struct hash<NewType> : public unary_function<NewType, size_t>             \
      {                                                                         \
         size_t operator()(NewType var) const                                   \
         {                                                                      \
            hash<int> hasher;                                                   \
            return hasher(static_cast<int>(var.GetContent()));                  \
         }                                                                      \
      };                                                                        \
   }                                                                            \
   /* Workaround for ;;*/                                                       \
   class NewType

#endif

#define UINT_STRONG_TYPEDEF(new_type) STRONG_TYPEDEF(unsigned int, new_type)
#define UINT_STRONG_TYPEDEF_FORWARD_DECL(new_type) STRONG_TYPEDEF_FORWARD_DECL(unsigned int, new_type)

#endif
