import sys
import os


indexer = 0
printstring = ""
baseslashnum = 0


def walkdirectory( curdir):
	global indexer
	global printstring
	#print(str(indexer + 1))
	dirlist = os.listdir(curdir)
	preppeddirname = curdir
	for num in range(0, baseslashnum + (indexer + 1)):
		preppeddirname = preppeddirname[preppeddirname.find('/') + 1::]
	printstring = printstring + indexer * "" + preppeddirname + "\n" + indexer * "" + "{\n" 
	indexer = indexer + 1
	backlog = []
	for item in dirlist:
		if(os.path.isdir(curdir + "/" + item) and not os.path.islink(curdir + "/" + item)):
			backlog.append(item)
		else:
			printstring = printstring + indexer * "" + curdir + "/" + item + " " + item + "\n"
	for item in backlog:
		walkdirectory(curdir + "/" + item)
	indexer = indexer - 1
	printstring = printstring + indexer * "" + "}\n"

os.chdir(sys.argv[1])
baseslashnum = os.getcwd().count('/')
#print(str(baseslashnum))
#print(str(len(os.listdir(os.getcwd()))))
walkdirectory(os.getcwd())
print(printstring[:-1])
