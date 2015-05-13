//user program to test getuid syscall

#include "types.h"
#include "stat.h"
#include "user.h"
#include "fcntl.h"

int MAX_LEN = 200; // buffer lenght for username, password, and user id lengths
int NUM_OF_USERS = 4;
int STDIN    = 0;
int STDOUT = 1;
int STDERR = 2;

// represents each user
typedef struct user {
    char username[200];
    char userId[200];
} USER;



// removes all newlines (\n) from buffer
void removeNewlinesFromBuffer(char buf[], int size ) {

    char *src, *dst;
    
    for (src = dst = buf; *src != '\0'; src++) {
        *dst = *src;
        if (*dst != '\n') dst++;
    }
    *dst = '\0';
}

//  asumes a buffer initialized all slots to null values
//  reads a word from a file, appends \n to the word, and puts it into <buf>
//  retuns a pointer to <buf>
//  if EOF, return NULL ( aka '\0') in <buf[0]>
char*
readWord(int fd, char *buf, int max)
{
  int i, cc;
  char c;

  for(i=0; i+1 < max; ){
    cc = read(fd, &c, 1);
    if(cc < 1)
      break;
    buf[i++] = c;
    if(c == '\n' || c == '\r'  || c == ' ')
      break;
  }
  
  buf[i] = '\0';
  
  // if only one character long word, append \n
  if(i == 1)
      buf[i] = '\n';
  
  // if more than one character long word, append \n
  if(i > 1)
      buf[i-1] = '\n';
  
  return buf;
}


void zeroOutBuffer(char buf[], int size ){
    memset(buf, '\0', size);
}

// receives a filename to count the newlines from
// counts the number for newlines and returns that number
int getNumberOfNewlines(char filename[]) {

    int i = 0;
    int fd;
    int retFromRead;
    int counter = 0;
    char *ch;
    char buf[MAX_LEN];
    int bufSize = sizeof(buf);
    zeroOutBuffer(buf, bufSize);
    
    // open file
    if((fd = open(filename,  O_RDONLY)) < 0){
      printf(1, "login.c:getNumberOfNewlines: Error cannot open file '%s'\n", filename);
      exit();
    }
    
    retFromRead = read(fd, buf, sizeof(buf));
    if(retFromRead < 1){
        printf(1, "login.c:getNumberOfNewlines: Error cannot open file '%s'\n", filename);
    }
    
    // close file
    close(fd);
    
    for (ch = buf; i < bufSize; ch++, i++) {
        if (*ch == '\n') 
            counter++;
    }

    return counter;
}

      
void getUsrName(int procUID, char usr[], int size){
   char    etcUser[MAX_LEN] ;
   char    etcPass[MAX_LEN] ;
   char etcUserId[MAX_LEN] ;
   char filename[] = "/etc/passwd";
   int fd, i;
   
    if((fd = open(filename,  O_RDONLY)) < 0){
      printf(1, "login.c:getUsrName: Error cannot open file '%s'\n", filename);
      exit();
    }
    
    // compare the username and password given against each user and password in the file
    for(i  = 0; i < NUM_OF_USERS ; i++){
        
        // zero out buffers
        zeroOutBuffer(etcUser, sizeof(etcUser));
        zeroOutBuffer(etcPass, sizeof(etcPass));
        zeroOutBuffer(etcUserId, sizeof(etcUserId));
        
        // read one user info from file
       readWord(fd, etcUser, sizeof(etcUser));
       readWord(fd, etcPass, sizeof(etcPass));
       readWord(fd, etcUserId, sizeof(etcUserId));

        if(procUID == atoi(etcUserId)){
            strcpy(usr, etcUser);
        }

    }
    close(fd);
	
}

	 
int
main(void)
{
	int currProcUID = getuid();
	char tempUsr[MAX_LEN];
	
	zeroOutBuffer(tempUsr, sizeof(tempUsr));
	getUsrName(currProcUID, tempUsr, sizeof(tempUsr));
	
	printf(1, "Current process User: %s\n", tempUsr);
	
	exit();
}
