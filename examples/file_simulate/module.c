#include <sys/types.h>
#include <sys/stat.h>
#include <assert.h>
#include <fcntl.h>
#include <unistd.h>

int check_printf(int a)
{
  int id = open("test.xml", O_RDONLY);
  assert(id != -1);
  int id_write = open("test_copied.xml", O_WRONLY|O_CREAT|O_TRUNC, S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH|S_IWOTH);
  assert(id_write != -1);
  ssize_t res;
  char buffer[256];
  int index, res_index;
  for(index=0; index<256; ++index)
    buffer[index]=0;
  printf("check %d\n", id);
  printf("check_write %d\n", id_write);
  res = read(id, buffer, 256);
  printf("res %d\n", res);
  for(index=0; index<res; ++index)
    printf("%c", buffer[index]);
  printf("\n");

  res_index = write(id_write, buffer, res);
  printf("n. bytes read=%d, n. bytes written=%d\n", res, res_index);
  close(id);
  close(id_write);
  return a;
}
