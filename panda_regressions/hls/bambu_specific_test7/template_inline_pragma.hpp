#ifndef TEMPLATE_INLINE_PRAGMA_HPP
#define TEMPLATE_INLINE_PRAGMA_HPP

template <typename T>
T add_offset(T value, int offset)
{
#pragma HLS inline
   return value + offset;
}

template <typename T>
T unused_add_offset(T value, int offset)
{
#pragma HLS inline
   return value - offset;
}

template <typename T>
struct adjust
{
   static T add(T value, T increment)
   {
#pragma HLS inline
      return value + increment;
   }
};

template <typename T>
struct unused_adjust
{
   static T add(T value, T increment)
   {
#pragma HLS inline
      return value - increment;
   }
};

#endif
