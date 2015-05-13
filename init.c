
// init: The initial user-level program

#include "types.h"
#include "stat.h"
#include "user.h"
#include "fcntl.h" 
 
char *argv[] = { "login", 0 };

int
main(void)
{
  //fd 0 (stdin)
  if(open("/dev/console", O_RDWR) < 0){
    mknod("/dev/console", 1, 1);
    open("/dev/console", O_RDWR);
  }
  //fd 1 (stdout) duplicate of stdin=0
  dup(0);
  //fd 2 (stderr) duplicate of stdin=0
  dup(0);
  
  // Begin device additions

  //fd 3 (null)
  if(open("/dev/null", O_RDWR) < 0){
    mknod("/dev/null", 2, 1);
  }else{
    close(3);
  }
  //fd 3 (zero)
  if(open("/dev/zero", O_RDWR) < 0){
    mknod("/dev/zero", 3, 1);
  }else{
    close(3);
  }
  //fd 3 (urandom)
  if(open("/dev/urandom", O_RDWR) < 0){
    mknod("/dev/urandom", 4, 1);
  }else{
    close(3);
  }

  printf(1, "init: starting login prompt\n");

  // execute the login program, which forks & executes a shell
  exec("/bin/login", argv);
  printf(1, "init: exec login failed\n");

  //if we reach this point, the kernel will panic, by design
  exit();

}
