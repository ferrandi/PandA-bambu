#include "leds_ctrl.h"
#include "sw_ctrl.h"
#include "btn_ctrl.h"
#include "sevensegments_ctrl.h"

unsigned char convert_char_digit_2_sseg(char c)
{
   static unsigned char table[] = {63/*0*/, 6/*1*/, 91/*2*/, 79/*3*/, 102/*4*/, 109/*5*/, 125/*6*/, 7/*7*/, 127/*8*/, 111/*9*/};
   return table[c-'0'];
}

void led_example()
{
  _Bool terminate = 0;
  char a, b, c, d, e, f, g;
  do
  {
     unsigned short val = sw_ctrl();
     unsigned long long sseg_val = ((unsigned long long)DOT_BITVALUE) << 4*8;///initialize with the middle dot

     short int low_byte = val & 255;
     short int high_byte = val >> 8;
     for ( a = '0' - 1; high_byte >= 0; high_byte -= 100   ) ++a;
     for ( b = '9' + 1; high_byte <  0; high_byte += 10    ) --b;
     c = high_byte + '0';
     sseg_val |= ((unsigned long long)convert_char_digit_2_sseg(c))<<4*8;
     sseg_val |= ((unsigned long long)convert_char_digit_2_sseg(b))<<5*8;
     sseg_val |= ((unsigned long long)convert_char_digit_2_sseg(a))<<6*8;
     ///now manage the fractional part of the number
     d = (low_byte * 10)>>8;
     low_byte = low_byte * 10 - d*256;
     d = '0' + d;
     e = (low_byte * 10)>>8;
     low_byte = low_byte * 10 - e*256;
     e = '0' + e;
     f = (low_byte * 10)>>8;
     low_byte = low_byte * 10 - f*256;
     f = '0' + f;
     g = (low_byte * 10)>>8;
     low_byte = low_byte * 10 - g*256;
     g = '0' + g;
     sseg_val |= ((unsigned long long)convert_char_digit_2_sseg(d))<<3*8;
     sseg_val |= ((unsigned long long)convert_char_digit_2_sseg(e))<<2*8;
     sseg_val |= ((unsigned long long)convert_char_digit_2_sseg(f))<<1*8;
     sseg_val |= ((unsigned long long)convert_char_digit_2_sseg(g))<<0*8;
     leds_ctrl(val);
     sevensegments_ctrl(sseg_val,~0ULL);
     terminate = (btn_ctrl() & BUTTON_UP) != 0;
  } while(!terminate);
}
