#include "types.h"
#include "stat.h"
#include "user.h"
#include "fs.h"

int
main(int argc, char *argv[])
{

  if(argc < 3){
    printf(2, "Usage: cp sourcefile destfile...\n");
    exit();
  }

  int result;
  result = fCopyFile(argv[1],argv[2]);

  if(result == -1){
    printf(2, "cp: '%s' could not be found\n", argv[1]);
    exit();
  }
  if(result == -2){
    printf(2, "cp: '%s' could not be created\n", argv[2]);
    exit();
  }
  if(result == -3){
    printf(2, "cp: '%s' already exists\n", argv[2]);
    exit();
  }
  if(result == -4){
    printf(2, "cp: source and destination cannot be the same\n");
    exit();
  }
  if(result < -4){
    printf(2, "cp: failed to copy '%s' to '%s'\n", argv[1], argv[2]);
    exit();
  }
  printf(1, "cp: successfully copied '%s' to '%s'\n", argv[1], argv[2]);
  exit();
}
