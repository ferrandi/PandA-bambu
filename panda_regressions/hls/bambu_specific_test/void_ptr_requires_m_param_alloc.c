#include <stdint.h>

int void_ptr_requires_m_param_alloc(const void *input, unsigned int len)
{
   const uint8_t *bytes = (const uint8_t *)input;
   return len ? bytes[0] : 0;
}
