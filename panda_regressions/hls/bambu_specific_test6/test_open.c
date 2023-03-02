#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

int main()
{
   int fd = open("bin_test.dat", O_RDONLY, S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH|S_IWOTH);
   int number = 0, bytes_read = 0;
   long long int number2=0;
   if(fd == -1)
   {   
      abort();   
      close(fd);
   }
   bytes_read = read(fd, &number, sizeof(number));
   printf("%d\n", number);
   if(number != 2 || bytes_read != 4)
   {
      abort();
   }
   bytes_read = read(fd, &number, 1);
   printf("%d\n", number);
   if(number != 2 || bytes_read != 1)
   {
      abort();
   }
   bytes_read = read(fd, &number2,sizeof(number2));
   printf("%lld\n", number2);
   if(number2 != 18383834416414720LL || bytes_read != 8)
   {
      abort();
   }
   return 0;
}