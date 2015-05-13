#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <assert.h>

#define stat xv6_stat  // avoid clash with host struct stat
#include "types.h"
#include "fs.h"
#include "stat.h"
#include "param.h"

#define static_assert(a, b) do { switch (0) case 0: case (a): ; } while (0)

int nblocks = (995-LOGSIZE);
int nlog = LOGSIZE;
int ninodes = 200;
int size = 1024 + 3; // +3 accounts for the added things to the i-node structure
//int size = 1024;

int dodebug = 0;//set to 0 for turning off debug messages

int fsfd; //file system file descriptor
struct superblock sb; //superblock
char zeroes[512];
uint freeblock;
uint usedblocks;
uint bitblocks;
uint freeinode = 1;

void balloc(int); //block alloc
void wsect(uint, void*); //write sector
void winode(uint, struct dinode*); //write inode
void rinode(uint inum, struct dinode *ip); //read inode
void rsect(uint sec, void *buf); //read sector
uint ialloc(ushort type); //alloc inode
void iappend(uint inum, void *p, int n); //append inode

char* readLine(FILE* file);
void mirrordir(FILE* file, uint rootino);
uint recursmirrordir(FILE* file, uint dirino);
void debug(char *message);

// convert to intel byte order
ushort //ensure little endianness?
xshort(ushort x)
{
  ushort y;
  uchar *a = (uchar*)&y;
  a[0] = x;
  a[1] = x >> 8;
  return y;
}

uint //ensure little endianness?
xint(uint x)
{
  uint y;
  uchar *a = (uchar*)&y;
  a[0] = x;
  a[1] = x >> 8;
  a[2] = x >> 16;
  a[3] = x >> 24;
  return y;
}

int
main(int argc, char *argv[])
{
  int i, cc, fd;
  uint rootino, inum, off;
  struct dirent de;
  char buf[512];
  struct dinode din;
  FILE *file;


  static_assert(sizeof(int) == 4, "Integers must be 4 bytes!"); //sanity check / make sure things are as we assume

  if(argc < 2){ //make sure there are actually files supplied to become the file system
    fprintf(stderr, "Usage: mkfs fs.img files...\n");
    exit(1);
  }


  assert(((512 + 64) % sizeof(struct dinode)) == 0); // +64 because inode got bigger
  //fprintf(1, "%lu",sizeof(struct dinode));
  //

  assert((512 % sizeof(struct dirent)) == 0);

  fsfd = open(argv[1], O_RDWR|O_CREAT|O_TRUNC, 0666);
  if(fsfd < 0){
    perror(argv[1]);
    exit(1);
  }

  sb.size = xint(size);
  sb.nblocks = xint(nblocks); // so whole disk is size sectors
  sb.ninodes = xint(ninodes);
  sb.nlog = xint(nlog);

  bitblocks = size/(512*8) + 1;
  usedblocks = ninodes / IPB + 3 + bitblocks;
  freeblock = usedblocks;

  printf("used %d (bit %d ninode %zu) free %u log %u total %d\n", usedblocks,
         bitblocks, ninodes/IPB + 1, freeblock, nlog, nblocks+usedblocks+nlog);

  assert(nblocks + usedblocks + nlog == size);

  for(i = 0; i < nblocks + usedblocks + nlog; i++)
    wsect(i, zeroes);//zero all the things

  memset(buf, 0, sizeof(buf));
  memmove(buf, &sb, sizeof(sb));
  wsect(1, buf);

  rootino = ialloc(T_DIR); // making root directory inode ?
  assert(rootino == ROOTINO);

  bzero(&de, sizeof(de));
  de.inum = xshort(rootino);
  strcpy(de.name, ".");
  iappend(rootino, &de, sizeof(de));

  bzero(&de, sizeof(de));
  de.inum = xshort(rootino);
  strcpy(de.name, "..");
  iappend(rootino, &de, sizeof(de));
  
  i = 2;
  if(strncmp(argv[i], "-d",14) == 0){
	  /*
	  ++i;
	  if(chdir(argv[i]) < -1){
		  perror(argv[i]);
		  exit(1);
	  }
	  
	  curDirINo = ialloc(T_DIR);
	  bzero(&de, sizeof(de));
	  de.inum = xshort(curDirINo);
	  strncpy(de.name, argv[i], DIRSIZ);
	  iappend(rootino, &de, sizeof(de));
	  */
	  ++i;
	  file = fopen(argv[i], "r");
	  if((file == 0))
	  {
		  perror(argv[i]);
		  exit(1);
	  }
	  /*
	  if((fd = open(argv[i], 0)) < 0)
	  {
		  perror(argv[i]);
		  exit(1);
	  }
	  */
	  mirrordir(file, rootino);
	  fclose(file);
  }	  
  else
  {
	  for(i = 2; i < argc; i++){
	    assert(index(argv[i], '/') == 0);
	    if((fd = open(argv[i], 0)) < 0){
	      perror(argv[i]);
	      exit(1);
	    }
	    
	    // Skip leading _ in name when writing to file system.
	    // The binaries are named _rm, _cat, etc. to keep the
	    // build operating system from trying to execute them
	    // in place of system binaries like rm and cat.
	    if(argv[i][0] == '_')
	      ++argv[i];

	    inum = ialloc(T_FILE);

	    bzero(&de, sizeof(de));
	    de.inum = xshort(inum);
	    strncpy(de.name, argv[i], DIRSIZ);
	    iappend(rootino, &de, sizeof(de));

	    while((cc = read(fd, buf, sizeof(buf))) > 0)
	      iappend(inum, buf, cc);

	    close(fd);
	  }
  }
  //MIGHT NEED THIS <- jsamp701
  // fix size of root inode dir
  rinode(rootino, &din);
  off = xint(din.size);
  off = ((off/BSIZE) + 1) * BSIZE;
  din.size = xint(off);
  winode(rootino, &din);

  balloc(usedblocks);

  exit(0);
}

void
wsect(uint sec, void *buf)
{
  if(lseek(fsfd, sec * 512L, 0) != sec * 512L){
    perror("lseek");
    exit(1);
  }
  if(write(fsfd, buf, 512) != 512){
    perror("write");
    exit(1);
  }
}

uint
i2b(uint inum)
{
  return (inum / IPB) + 2;
}

void
winode(uint inum, struct dinode *ip)
{
  char buf[512];
  uint bn;
  struct dinode *dip;

  bn = i2b(inum);
  rsect(bn, buf);
  dip = ((struct dinode*)buf) + (inum % IPB);
  *dip = *ip;
  wsect(bn, buf);
}

void
rinode(uint inum, struct dinode *ip)
{
  char buf[512];
  uint bn;
  struct dinode *dip;

  bn = i2b(inum);
  rsect(bn, buf);
  dip = ((struct dinode*)buf) + (inum % IPB);
  *ip = *dip;
}

void
rsect(uint sec, void *buf)
{
  if(lseek(fsfd, sec * 512L, 0) != sec * 512L){
    perror("lseek");
    exit(1);
  }
  if(read(fsfd, buf, 512) != 512){
    perror("read");
    exit(1);
  }
}

uint
ialloc(ushort type)
{
  uint inum = freeinode++;
  struct dinode din;

  bzero(&din, sizeof(din));
  din.type = xshort(type);
  din.nlink = xshort(1);
  din.size = xint(0);
  din.UID = 0;        // default to be owned by root
  din.permBit = 0x74; // default to be all access for owner and read for world
  winode(inum, &din);
  return inum;
}

void
balloc(int used)
{
  uchar buf[512];
  int i;

  printf("balloc: first %d blocks have been allocated\n", used);
  assert(used < 512*8);
  bzero(buf, 512);
  for(i = 0; i < used; i++){
    buf[i/8] = buf[i/8] | (0x1 << (i%8));
  }
  printf("balloc: write bitmap block at sector %zu\n", ninodes/IPB + 3);
  wsect(ninodes / IPB + 3, buf);
}

#define min(a, b) ((a) < (b) ? (a) : (b))

/*
A giant heaping helping of black magic.  I assume this function does something to the extent of storing an
inode's data in its data blocks.  80% certainty.  How to use

Directory:
	make the directory inode
	make a dirent for the directory inode
	add the directory dirent to whatever the previous directory inode is
	add all the data files (i.e., all the files) to the directory
*/
void
iappend(uint inum, void *xp, int n)
{
  char *p = (char*)xp;//to allow indexing of the data in xp
  uint fbn, off, n1;
  struct dinode din;
  char buf[512];
  uint indirect[NINDIRECT];
  uint x;

  rinode(inum, &din);

  off = xint(din.size);
  while(n > 0){
    fbn = off / 512;
    assert(fbn < MAXFILE);
    if(fbn < NDIRECT){
      if(xint(din.addrs[fbn]) == 0){
        din.addrs[fbn] = xint(freeblock++);
        usedblocks++;
      }
      x = xint(din.addrs[fbn]);
    } else {
      if(xint(din.addrs[NDIRECT]) == 0){
        // printf("allocate indirect block\n");
        din.addrs[NDIRECT] = xint(freeblock++);
        usedblocks++;
      }
      // printf("read indirect block\n");
      rsect(xint(din.addrs[NDIRECT]), (char*)indirect);
      if(indirect[fbn - NDIRECT] == 0){
        indirect[fbn - NDIRECT] = xint(freeblock++);
        usedblocks++;
        wsect(xint(din.addrs[NDIRECT]), (char*)indirect);
      }
      x = xint(indirect[fbn-NDIRECT]);
    }
    n1 = min(n, (fbn + 1) * 512 - off);
    rsect(x, buf);
    bcopy(p, buf + off - (fbn * 512), n1);
    wsect(x, buf);
    n -= n1;
    off += n1;
    p += n1;
  }
  din.size = xint(off);
  winode(inum, &din);
}

//takes the file pointer to the fs image text file and the inode number of the parent directory
//returns the inode number of the new directory
//leaves the file pointer pointing to the next line of the parent directory after the closing bracket of the new child directory
uint recursmirrordir(FILE *file, uint dirino)
{
	debug("GOT TO RECURSMIRRORDIR");
	
	uint retino, childino, off;
	struct dinode din;
	struct dirent de;
	char *dataline, *checkline, *path, *name;
	int lastfiletype, fd, cc;
	char buf[512];
	
	lastfiletype = 1;
	retino = ialloc(T_DIR);
	
	dataline = readLine(file);
	
	debug("INITIAL DATALINE");
	debug(dataline);

	bzero(&de, sizeof(de));
	de.inum = xshort(retino);
	strcpy(de.name, ".");
	iappend(retino, &de, sizeof(de));

	bzero(&de, sizeof(de));
	de.inum = xshort(dirino);
	strcpy(de.name, "..");
	iappend(retino, &de, sizeof(de));
	
	if(strncmp(dataline, "}", 3) != 0)
	{//directory is not empty
		checkline = readLine(file); //pull in second actual line
		
		debug("INITIAL CHECKLINE");
		debug(checkline);
		
		while(strncmp(checkline, "}",3) != 0){
			if(strchr(dataline, ' ') == 0)
			{//is a directory
				debug("DIR NAME");
				debug(dataline);
				
				lastfiletype = 0;
				puts(dataline);
				//file pointer is pointing to line after opening bracket of the child dir in the fs image txt file
				childino = recursmirrordir(file, retino);
				bzero(&de, sizeof(de));
				de.inum = xshort(childino);
				strcpy(de.name, dataline);
				iappend(retino, &de, sizeof(de));
			}
			else
			{//is a file
				lastfiletype = 1;
				path = strtok(dataline, " ");
				name = strtok(NULL, " ");
				if(name[0] == '_')
					++name;
				
				debug("FILE PATH");
				debug(path);
				debug("FILE NAME");
				debug(name);
				
				if((fd = open(path, 0)) < 0){
					perror(path);
					exit(1);
				}
				
				childino = ialloc(T_FILE);

				bzero(&de, sizeof(de));
				de.inum = xshort(childino);
				strncpy(de.name, name, DIRSIZ);
				iappend(retino, &de, sizeof(de));

				while((cc = read(fd, buf, sizeof(buf))) > 0)
					iappend(childino, buf, cc);
				
				close(fd);
			}
			free(dataline);
			if(lastfiletype == 0)
			{//was a dir
				debug("OLD CHECKLINE");
				debug(checkline);
				
				free(checkline);
				dataline = readLine(file);
				checkline = readLine(file);
				lastfiletype = 1;
				
				debug("NEW DATALINE");
				debug(dataline);
				debug("NEW CHECKLINE");
				debug(checkline);
			}
			else
			{
				debug("OLD CHECKLINE");
				debug(checkline);
				
				dataline = checkline;
				checkline = readLine(file);
				
				debug("NEW CHECKLINE");
				debug(checkline);
			}
		}
		
		if(lastfiletype == 1)
		{//process the last file
			path = strtok(dataline, " ");
			name = strtok(NULL, " ");
			if(name[0] == '_')
				++name;
			
			debug("FILE PATH");
			debug(path);
			debug("FILE NAME");
			debug(name);
			
			if((fd = open(path, 0)) < 0){
				perror(path);
				exit(1);
			}

			childino = ialloc(T_FILE);

			bzero(&de, sizeof(de));
			de.inum = xshort(childino);
			strncpy(de.name, name, DIRSIZ);
			iappend(retino, &de, sizeof(de));

			while((cc = read(fd, buf, sizeof(buf))) > 0)
				iappend(childino, buf, cc);

			close(fd);
		}
	}
	
	rinode(retino, &din);
	off = xint(din.size);
	off = ((off/BSIZE) + 1) * BSIZE;
	din.size = xint(off);
	winode(retino, &din);
	
	return retino;
}

void mirrordir(FILE *file, uint rootino)
{
	debug("GOT TO MIRRORDIR\n");
	char *dataline, *checkline, *path, *name;
	int lastfiletype; //0 for directory, 1 for file
	int fd, cc;
	char buf[512];
	
	struct dirent de;
	uint childino;
	
	dataline = readLine(file);//pull out line specifying first directory name
	free(dataline);
	
	dataline = readLine(file);//pull out line consisting of "{"
	free(dataline);
	
	dataline = readLine(file); //pull in first actual line
	
	debug("Initial dataline");
	debug(dataline);
	
	if(strncmp(dataline, "}",3) == 0){
		printf("Error: the directory image text file had an empty root directory. Aborting");
		exit(1);
	}
	
	lastfiletype = 1;
	
	checkline = readLine(file); //pull in second actual line
	
	debug("Initial checkline");
	debug(checkline);
	
	while(strncmp(checkline, "}",3) != 0 && checkline[0] != 0){
		if(strchr(dataline, ' ') == 0)
		{//is a directory
			debug("DIR NAME");
			debug(dataline);
			
			lastfiletype = 0;
			//file pointer is pointing to line after opening bracket of the child dir in the fs image txt file
			childino = recursmirrordir(file, rootino);
			bzero(&de, sizeof(de));
			de.inum = xshort(childino);
			strcpy(de.name, dataline);
			iappend(rootino, &de, sizeof(de));
		}
		else
		{//is a file
			lastfiletype = 1;
			path = strtok(dataline, " ");
			name = strtok(NULL, " ");
			
			debug("FILE PATH");
			debug(path);
			debug("FILE NAME");
			debug(name);
			
			if(name[0] == '_')
				++name;
			
			if((fd = open(path, 0)) < 0){
				perror(path);
				exit(1);
			}
			
			childino = ialloc(T_FILE);

			bzero(&de, sizeof(de));
			de.inum = xshort(childino);
			strncpy(de.name, name, DIRSIZ);
			iappend(rootino, &de, sizeof(de));

			while((cc = read(fd, buf, sizeof(buf))) > 0)
				iappend(childino, buf, cc);
			
			close(fd);
		}
		free(dataline);
		if(lastfiletype == 0)
		{//was a dir
			debug("OLD CHECKLINE");
			debug(checkline);
			
			free(checkline);
			dataline = readLine(file);
			checkline = readLine(file);
			
			debug("NEW DATALINE");
			debug(dataline);
			debug("NEW CHECKLINE");
			debug(checkline);
		}
		else
		{
			debug("OLD CHECKLINE");
			debug(checkline);
			
			dataline = checkline;
			checkline = readLine(file);
			
			debug("NEW CHECKLINE");
			debug(checkline);
		}
	}
	
	if(lastfiletype == 1)
	{//process the last file
		path = strtok(dataline, " ");
		name = strtok(NULL, " ");
		if(name[0] == '_')
			++name;
		
		debug("FILE PATH");
		debug(path);
		debug("FILE NAME");
		debug(name);
		
		if((fd = open(path, 0)) < 0){
			perror(path);
			exit(1);
		}

		childino = ialloc(T_FILE);

		bzero(&de, sizeof(de));
		de.inum = xshort(childino);
		strncpy(de.name, name, DIRSIZ);
		iappend(rootino, &de, sizeof(de));

		while((cc = read(fd, buf, sizeof(buf))) > 0)
			iappend(childino, buf, cc);

		close(fd);
	}
	
	free(dataline);
	free(checkline);
}

//taken from http://stackoverflow.com/questions/3501338/c-read-file-line-by-line
//does not keep newline character
//preserves whitespace
//returns dynamically allocated memory, so make sure to free the pointer returned once you are done =)
//const char *readLine(FILE *file) {
char *readLine(FILE *file) {

    if (file == NULL) {
        printf("Error: file pointer is null.");
        exit(1);
    }

    int maximumLineLength = 128;
    char *lineBuffer = (char *)malloc(sizeof(char) * maximumLineLength);

    if (lineBuffer == NULL) {
        printf("Error allocating memory for line buffer.");
        exit(1);
    }

    char ch = getc(file);
    int count = 0;

    while ((ch != '\n') && (ch != EOF)) {
        if (count == maximumLineLength) {
            maximumLineLength += 128;
            lineBuffer = realloc(lineBuffer, maximumLineLength);
            if (lineBuffer == NULL) {
                printf("Error reallocating space for line buffer.");
                exit(1);
            }
        }
        lineBuffer[count] = ch;
        count++;

        ch = getc(file);
    }
    if(count % 128 == 0)
	    lineBuffer = realloc(lineBuffer, count + 1);
    lineBuffer[count] = '\0';
    //lineBuffer = realloc(lineBuffer, count + 1);
    //char line[count + 1];
    //strncpy(line, lineBuffer, (count + 1));
    //free(lineBuffer);
    
    //const char *constLine = lineBuffer;
    return lineBuffer;
}

void debug(char *message)
{
	if(dodebug != 0)
		puts(message);
}
