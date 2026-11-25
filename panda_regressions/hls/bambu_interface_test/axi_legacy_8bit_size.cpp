#include <stdint.h>

#ifdef __BAMBU_SIM__
#include <mdpi/mdpi_user.h>
#endif

#pragma HLS interface port=data mode=m_axi offset=direct bundle=shared
extern "C" void __attribute__((noinline)) copy_bytes(uint8_t *data)
{
   for(unsigned i = 0; i < 4; ++i)
      data[i] = static_cast<uint8_t>(data[i] + 1);
}

int main()
{
   uint8_t data[4] = {0x10, 0x21, 0x32, 0x43};
#ifdef __BAMBU_SIM__
   m_param_alloc(0, sizeof(data));
#endif
   copy_bytes(data);
   return data[0] == 0x11 && data[1] == 0x22 && data[2] == 0x33 && data[3] == 0x44 ? 0 : 1;
}
