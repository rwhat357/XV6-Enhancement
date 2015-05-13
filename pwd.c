#include "types.h"
#include "stat.h"
#include "user.h"
#include "fs.h"




int main(int argc, char *argv[])
{
    char *workdir = malloc(256);
    workdir = getcwd(workdir,256);
    printf(1,"%s\n",workdir);
	free(workdir);
    exit();
}
