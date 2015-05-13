#include "types.h"
#include "stat.h"
#include "user.h"
#include "fs.h"

int
main(int argc, char *argv[])
{

  //struct stat st;

  if(argc < 3){
    printf(2, "Usage: mv originalfilepath destfilepath...\n");
    printf(2, "       mv originalfilepath destdirectory...\n");
    exit();
  }

  int result;
  result = fMoveFile(argv[1],argv[2]);



  if(result == -1){
    printf(2, "mv: '%s' could not be found\n", argv[1]);
    exit();
  }
  if(result == -2){
    printf(2, "mv: '%s' is a directory\n", argv[1]);
    exit();
  }
  if(result == -3){
    printf(2, "mv: '%s' already exists\n", argv[2]);
    exit();
  }
  if(result == -4){
    printf(2, "mv: the destination link could not be created\n");
    exit();
  }
  if(result == -5){
    printf(2, "mv: '%s' has no parent\n", argv[1]);
    exit();
  }
  if(result == -6){
    printf(2, "mv: '.' and '..' are built-in directory links and should not be moved\n");
    exit();
  }
  if(result == -7){
    printf(2, "mv: source and destination cannot be the same\n");
    exit();
  }
  if(result == -8){
    printf(2, "mv: destination path '%s' does not exist\n", argv[2]);
    exit();
  }
  if(result == -9){
    printf(2, "mv: destination path '%s/%s' already exists\n", argv[2], argv[1]);
    exit();
  }
  if(result < -9){
    printf(2, "mv: failed to move '%s' to '%s'\n", argv[1], argv[2]);
    exit();
  }
  printf(1, "mv: successfully moved '%s' to '%s'\n", argv[1], argv[2]);
  exit();

  //if(fstat(argv[1],&st) < 0){
  //  printf(2, "mv: '%s' doesn't exist\n", argv[1]);
  //  exit();
  //}

  //if(fstat(argv[2],&st) == 0){
  //  //destination exists. is it a directory?
  //  if (st.type == T_DIR){
  //    
  //  }else{
  //    printf(2, "mv: '%s' already exists\n", argv[2]);
  //    exit();
  //  }
  //}

  //if(link(argv[1],argv[2]) < 0){
  //  printf(2, "mv: failed to move '%s' to '%s'\n", argv[1], argv[2]);
  //  exit();
  //}
  //unlink(argv[1]);

  exit();
}
