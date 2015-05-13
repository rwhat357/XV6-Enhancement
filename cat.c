#include "types.h"
#include "stat.h"
#include "user.h"

char buf[512];

void
cat(int fd)
{
  int n;
  //int off=0;

  while((n = read(fd, buf, sizeof(buf))) > 0){
    //printf(0, "cat: offset:%d\n",off);
    write(1, buf, n);
    //off += n;
  }
  if(n < 0){
    printf(2, "cat: read error\n");
    exit();
  }
}

int
main(int argc, char *argv[])
{
  int fd, i;

  if(argc <= 1){
    cat(0);
    exit();
  }

  for(i = 1; i < argc; i++){
    if((fd = open(argv[i], 0)) < 0){
      printf(2, "cat: cannot open %s\n", argv[i]);
      exit();
    }
    cat(fd);
    close(fd);
  }
  exit();
}
