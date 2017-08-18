#include "get_ticks.h"
#include <sys/times.h>
#include <unistd.h>

#if defined(_SC_CLK_TCK)
#define TIMES_TICKS_PER_SEC sysconf(_SC_CLK_TCK)
#elif defined(CLK_TCK)
#define TIMES_TICKS_PER_SEC CLK_TCK
#elif defined(HZ)
#define TIMES_TICKS_PER_SEC HZ
#else // !CLK_TCK && !_SC_CLK_TCK && !HZ
#define TIMES_TICKS_PER_SEC 60
#endif // !CLK_TCK && !_SC_CLK_TCK && !HZ

/**
 * return an unsigned int which represents the elapsed processor
 * time in milliseconds since some constant reference
*/
unsigned int get_ticks(unsigned char restart_value)
{
   unsigned int t;
   struct tms now;
   clock_t    ret = times(&now);
   if (ret == (clock_t)(-1))
      now.tms_utime = now.tms_stime = now.tms_cutime = now.tms_cstime = ret = 0;
   t = ((unsigned int)(now.tms_utime) * 1000) / (TIMES_TICKS_PER_SEC) + ((unsigned int)(now.tms_cutime) * 1000) / (TIMES_TICKS_PER_SEC);
   return t;
}

