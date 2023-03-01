#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

int main()
{
   int fd = open("bin_test.dat", O_WRONLY|O_CREAT|O_TRUNC, S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH|S_IWOTH);
   int number, bytes_read = 0;
   if(fd != -1)
   {      
      bytes_read = read(fd, &number, sizeof(unsigned int));
      close(fd);
   }
   return number;
}