#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <ctype.h>
#include <errno.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>
#include <stdbool.h>


#define BLOCK_SIZE 1024
#define FREE_ARRAY_SIZE 251
#define INODE_SIZE 64

/*
===========================================================================================================
GROUP 15
TEAM MEMBERS:3
1. Sai Vijay Sathwik Reddy Gade ( NET ID: SXG200001 )
2. Mukesh Bachu ( NET ID: MXB190079 )
3. Riya Gupta  (NET ID: RXG210021)

===========================================================================================================

FILE NAME: modv6.c


To execute program use the following commands on Linux server:

First: gcc modv6.c -o modv6 -std=c99
Second: ./modv6

SUPPORTED COMMANDS: 
From the modified file system we developed the functions to provide for the following commands:

NOTE: openfs and initfs must be executed before any other operation respectively.
 
1. openfs v6FileSystem  	 			 // to open/create a v6 filesystem
2. initfs 7000 200  	        	    // to initialize the created filesystem with the total blocks and number of inode blocks
3. cpin externalfile.txt v6-file.txt   // trying copy an external file from outside into the file system
4. cpout v6-file.txt output.txt       // copying the internal v6-file to an external file
5. rm v6-file.txt                    // removing the v6-file
6. mkdir v6dir 		                // creating a new directory
7. cd v6dir                        // change directory
8. q                              // quit and close the file system 

===========================================================================================================
CONTRIBUTIONS BY:

1. Sai Vijay Sathwik Reddy Gade ( NET ID: SXG200001 )

	I)		writeToDataBlock
	II)		writeToBlockOffset
	III)	addFreeBlock
	IV)     cpout
	V)      main()
	VI)		openfs()
	
	
2. Mukesh Bachu ( NET ID: MXB190079 )

	I)		getInodeBlock
	II)		femptyInode
	III)	createRootDirectory
	IV)     makeDir
	V)      cpin
	VI) 	initfs()

3. Riya Gupta  (NET ID: RXG210021)

	I)		getFreeBlock
	II)		writetoInode
	III		addFreeInode
	IV) 	rm
	V)      cd
	VI)		quit()
	

NOTE: The final working source file has contributions from all team members 
      collectively through deliberate discussions about above functions.

================================================================================================================================================================
*/
/*************** super block structure**********************/
typedef struct {
		int isize;
		int fsize;
		int nfree;
		unsigned int free[251];
		char flock;
		char ilock;
		char fmod;
		unsigned int time;
} superBlock_type;

/****************inode structure ************************/
typedef struct {
		unsigned short flags;
		unsigned short nlinks;
		unsigned int uid;
		unsigned int gid;
		unsigned int size0;
		unsigned int size1;
		unsigned int addr[9];
		unsigned int actime;
		unsigned int modtime;
} Inode_type;

/****************directory structure ************************/
typedef struct{
		unsigned int inode;
		char filename[28];
}dir_type;


superBlock_type superBlock;
int fd;
char pd[100];
int curINodeNumber;
char fs_path[100];
int total_inodes_count;
unsigned int ninode;
unsigned short inode[FREE_ARRAY_SIZE];

//method to write contents to data blocks
void writeToDataBlock (int blockNumber, void * buffer, int nbytes)
{
        lseek(fd,BLOCK_SIZE * blockNumber, SEEK_SET);
        write(fd,buffer,nbytes);
}

//method to write to inodes as per the provided buffer
void writeToBlockOffset(int blockNumber, int offset, void * buffer, int nbytes)
{
        lseek(fd,(BLOCK_SIZE * blockNumber) + offset, SEEK_SET);
        write(fd,buffer,nbytes);
}

//method to add free data blocks to free[] array
void addFreeBlock(int blockNumber){
        if(superBlock.nfree == FREE_ARRAY_SIZE)
        {
                //write to the new block
                writeToDataBlock(blockNumber, superBlock.free, FREE_ARRAY_SIZE*4);
                superBlock.nfree = 0;
        }
        superBlock.free[superBlock.nfree] = blockNumber;
        superBlock.nfree++;
}

//method to get a free data block
int getFreeBlock(){
        if(superBlock.nfree == 0){
                int blockNumber = superBlock.free[0];
                lseek(fd,BLOCK_SIZE * blockNumber , SEEK_SET);
                read(fd,superBlock.free, FREE_ARRAY_SIZE * 2);
                superBlock.nfree = 100;
                return blockNumber;
        }
        superBlock.nfree--;
        return superBlock.free[superBlock.nfree];
}

//method to get a specific inode block when required
Inode_type getInodeBlock (int INumber){
        Inode_type iNode;
        int blockNumber =  2 ;
        int offset = ((INumber - 1) * INODE_SIZE) % BLOCK_SIZE;
        lseek(fd,(BLOCK_SIZE * blockNumber) + offset, SEEK_SET);
        read(fd,&iNode,INODE_SIZE);
        return iNode;
}

//method to add availabe inodes to inode array -- not necessarily needed
void addFreeInode(int iNumber){
        if(ninode == FREE_ARRAY_SIZE)
                return;
        inode[ninode] = iNumber;
        ninode++;
}

//method to write  contents to inode
void writetoInode(int INumber, Inode_type inode){
        int blockNumber =  2 ;
        int offset = ((INumber - 1) * INODE_SIZE) % BLOCK_SIZE;
        writeToBlockOffset(blockNumber, offset, &inode, sizeof(Inode_type));
}

//method to unallocate all the inodes and set the flag bit 15 to 0
void femptyInode(int INumber, Inode_type inode, int nbytes){
        int blockNumber =  2 ;
        int offset = ((INumber - 1) * INODE_SIZE) % BLOCK_SIZE;
        writeToBlockOffset(blockNumber, offset, &inode, nbytes);
}

//method to read contents from specific data block with offset
void readFromBlockOffset(int blockNumber, int offset, Inode_type * buffer, int nbytes)
{
        lseek(fd,(BLOCK_SIZE * blockNumber) + offset, SEEK_SET);
        read(fd,buffer,nbytes);
}

//method to read contents from a specific block
void readFromBlock(char *chararray, unsigned int blocknum)
{
		lseek(fd, blocknum*BLOCK_SIZE, 0);
		read(fd, chararray, BLOCK_SIZE);
}

// method to get free inode
int getFreeInode(){
	for (int iNumber=2; iNumber < total_inodes_count ; iNumber++){
		int blockNumber =  2 ;
        int offset = ((iNumber - 1) * INODE_SIZE) % BLOCK_SIZE;
		Inode_type freeInode;
		readFromBlockOffset(blockNumber,offset,&freeInode,INODE_SIZE);
        if (freeInode.flags == ( 0<<15) ){
			return iNumber;
		}
	}
}

//method to allocate root to inode number 1 and to store root contents to a data block
void createRootDirectory(){
        int blockNumber = getFreeBlock();
        dir_type directory[2];
        directory[0].inode = 1;
        strcpy(directory[0].filename,".");

        directory[1].inode = 1;
        strcpy(directory[1].filename,"..");

        writeToDataBlock(blockNumber, directory, 2*sizeof(dir_type));
		printf("\n root contents are stored in one of the data blocks  \n");

        Inode_type root;
        root.flags = 1<<14 | 1<<15; // setting flag bits 14  & 15 to 1 to allocate the  directory for the root
        root.nlinks = 1;
        root.uid = 0;
        root.gid = 0;
        root.size0 = 0;
		root.size1 = 2*sizeof(dir_type);
        root.addr[0] = blockNumber;
        root.actime = time(NULL);
        root.modtime = time(NULL);

        writetoInode(1,root);
		printf("\n root is allocated to inode number 1 \n");
        curINodeNumber = 1;
        strcpy(pd,"/");
}

//method to create new directories when mkdir call is made
void makeDir(char* dirName, unsigned int newInodeNum)
{

	char *givenPath = (char*)malloc(strlen(dirName));
	strcpy(givenPath, dirName);

	char* partialPath = strtok(givenPath, "/");

	char* finalFileName = (char *)malloc(sizeof(char) * 28);

	while (partialPath)
	{
		if (strlen(partialPath) < 28)
			strcpy(finalFileName, partialPath);
		else
			strncpy(finalFileName, partialPath, 28);

		partialPath = strtok(NULL, "/");
	}

	printf(" new directory name is %s\n", finalFileName);

	partialPath = strtok(dirName, "/");

	int parentInodeNum = 1;
	bool iamgroot = true;
	Inode_type tempInode;
	Inode_type tempInode_1;

	while (partialPath)
	{
		bool foundParent = false;
		if (iamgroot)
		{
			if (strcmp(finalFileName, partialPath) != 0)
			{
				tempInode = getInodeBlock(parentInodeNum);

				for (int addrIndex = 0; addrIndex < 9; addrIndex++)
				{
					unsigned int dataBlock = tempInode.addr[addrIndex];
					lseek(fd,BLOCK_SIZE * dataBlock, SEEK_SET);	
					//since block size is 1024 and dir size si 32
					for (int index = 0; index < 32; index++)
					{
						dir_type tempDir;

						read(fd, &tempDir, sizeof(dir_type));
						if (strcmp(tempDir.filename, partialPath) == 0)
						{
							parentInodeNum = tempDir.inode;
							foundParent = true;
							break;
						}
						else if (strcmp(tempDir.filename, "") == 0)
						{
							break;
						}
					}
				}

				if (!foundParent)
				{
					printf("\n The given path doesn't exist. \n");
					return;
				}
			}
			else
			{
				tempInode = getInodeBlock(parentInodeNum);

				for (int addrIndex = 0; addrIndex < 9; addrIndex++)
				{
					unsigned int dataBlock = tempInode.addr[addrIndex];
					lseek(fd,dataBlock * BLOCK_SIZE, SEEK_SET);

					for (int index = 0; index < 32; index++)
					{
						dir_type tempDir;

						read(fd, &tempDir, sizeof(dir_type));
						if (strcmp(tempDir.filename, finalFileName) == 0)
						{
							printf("\ndirectory name already exists\n");
							addrIndex = 9;
							break;
						}
						else if (strcmp(tempDir.filename, "") == 0)
						{
							unsigned int block_num = getFreeBlock();
							strcpy(tempDir.filename, finalFileName);
							tempDir.inode = newInodeNum;
							lseek(fd, (dataBlock * BLOCK_SIZE) + (index * (sizeof(tempDir))), 0);
							write(fd, &tempDir, sizeof(tempDir));

							// setting up this directory's inode and storing at inode number - newInodeNum
							tempInode_1.flags = 1<<14 | 1<<15; // setting 14th and 15th bit to 1, 15: allocated and 14: directory
							tempInode_1.nlinks = 2;
							tempInode_1.uid = 0;
							tempInode_1.gid = 0;
							tempInode_1.size0 = 0;
							tempInode_1.size1 = sizeof(dir_type);
							int i = 0;
							for (i = 1; i < 9; i++)
								tempInode_1.addr[i] = 0;
							tempInode_1.addr[0] = block_num;
							tempInode_1.actime = time(NULL);
							tempInode_1.modtime = time(NULL);
							
							writetoInode(newInodeNum,tempInode_1);
							

							strcpy(tempDir.filename, ".");
							tempDir.inode = newInodeNum;
							lseek(fd, (block_num * BLOCK_SIZE) + (0 * (sizeof(tempDir))), 0);
							write(fd, &tempDir, sizeof(tempDir));

							strcpy(tempDir.filename, "..");
							tempDir.inode = parentInodeNum;
							lseek(fd, (block_num * BLOCK_SIZE) + (1 * (sizeof(tempDir))), 0);
							write(fd, &tempDir, sizeof(tempDir));

							printf("\nDirectory created \n\n");

							addrIndex = 9; //to break out of outer loop
							break;
						}
					}
				}
				break;
			}

		}

		iamgroot = true;
		partialPath = strtok(NULL, "/");
	}
}


// to write to directory's data block
int writeToDirectory(Inode_type root, dir_type dir)
{

	int dup = 0; 
	unsigned short addrIndex = 0;
	char dirbuf[BLOCK_SIZE];
	int i = 0;
	dir_type tmpDir;
	for (addrIndex = 0; addrIndex < 9; addrIndex++)
	{
		lseek(fd, root.addr[addrIndex] * BLOCK_SIZE, 0);
		for (i = 0; i < 32; i++)
		{
			read(fd, &tmpDir, sizeof(dir_type));
			if (strcmp(tmpDir.filename, dir.filename) == 0)
			{
				printf("The directory name already exists.\n");
				dup = 1;
				break;
			}
		}
	}
	if (dup != 1)
	{
		for (addrIndex = 0; addrIndex < 9; addrIndex++)
		{
			readFromBlock(dirbuf, root.addr[addrIndex]);
			for (i = 0; i < 32; i++)	//to check all directory entries
			{

				if (dirbuf[32 * i] == 0) 
				{
					memcpy(dirbuf + 32 * i, &dir.inode, sizeof(dir.inode));
					memcpy(dirbuf + 32 * i + sizeof(dir.inode), &dir.filename, sizeof(dir.filename));
				    //storing contents of filename and inode number using using memcpy function
					writeToDataBlock(root.addr[addrIndex], dirbuf, BLOCK_SIZE);
					return dup;
				}
			}
		}
	}
	return dup;
}
//method to copy contents of external file to provided file in the file system
int cpin(char* source, char* dest)
{
	//printf("cpin start\n");
	char ReadBuffer[BLOCK_SIZE];
	int srcFD;
	unsigned long srcFileSize = 0;
	unsigned long totalBytesRead = 0;
	int CurrentBytesRead = 0;
	Inode_type tempInode;
	dir_type tmpDir;

	FILE *fp = fopen(source, "r");
	fseek(fp, 0, SEEK_END);
	srcFileSize = ftell(fp);
	fclose(fp);

	printf("\source file size is %ld\n", srcFileSize);

	//opening external file
	if ((srcFD = open(source, O_RDONLY)) == -1)
	{
		printf("\Error while opening the given Source file %s. \n", source);
		return -1;
	}
    //printf("\nsuccessfully opened the source file %s\n", source);

	unsigned int inumber = getFreeInode();
	//printf("inode no %d\n", inumber);
	if (inumber < 2) // 
	{
		printf("Error : no inodes avilable \n");
		return;
	}


	char* partialPath = strtok(dest, "/");

	char* finalFileName = (char *)malloc(sizeof(char) * 28);

	while (partialPath)
	{
		if (strlen(partialPath) < 28)
			strcpy(finalFileName, partialPath);
		else
			strncpy(finalFileName, partialPath, 28);

		partialPath = strtok(NULL, "/");
	}

	//printf("file name is %s\n", finalFileName);

	tmpDir.inode = inumber;
	if (strlen(finalFileName) < 28)
	{
		memcpy(tmpDir.filename, finalFileName, strlen(finalFileName));
	}
	else
	{
		memcpy(tmpDir.filename, dest, 28);
	}

	//New file - inode struct
	tempInode.flags = 0<<14 | 1<<15;
	tempInode.nlinks = 1;
	tempInode.uid = 0;
	tempInode.gid = 0;
	tempInode.size0 = 0;
	tempInode.size1 = 0;


	if (srcFileSize <= 27648)
	{
		//small file
		int index = 0;
		while (1)
		{
			memset(ReadBuffer, 0x00, BLOCK_SIZE);
			if ((CurrentBytesRead = read(srcFD, ReadBuffer, BLOCK_SIZE)) != 0 && index < 27)
			{
				int NewBlockNum = getFreeBlock();
				//printf("Blk_no is %d\n", NewBlockNum);
				writeToDataBlock(NewBlockNum, ReadBuffer, BLOCK_SIZE);
				tempInode.addr[index] = NewBlockNum;
				totalBytesRead += CurrentBytesRead;
				index++;
				//printf("Small file - current read %d, totalFileSize now %ld\n", CurrentBytesRead, totalBytesRead);
			}
			else
			{
				tempInode.size1 = totalBytesRead;
				//printf("Small file copied \n \n");
				//printf("External file Size %lu, Copied file size %lu\n", srcFileSize, totalBytesRead);
				break;
			}
		}
	}
	else{
		printf("\n No large files at all for this part of the project.\n");
	}


	tempInode.actime = time(NULL);
	tempInode.modtime = time(NULL);

	writetoInode(inumber, tempInode);
	lseek(fd, 2 * BLOCK_SIZE, 0);
	read(fd, &tempInode, INODE_SIZE);
	tempInode.nlinks++;
	writeToDirectory(tempInode, tmpDir);
	printf("\nFile copied\n\n");
	printf("Given External file Size is %lu, Copied file size is %lu\n\n", srcFileSize, totalBytesRead);
	return 0;
}

//method to copy contents of internal file to external file
void cpout(char* source, char* dest)
{
	char *newPartialPath = (char*)malloc(strlen(source));
	strcpy(newPartialPath, source);
	char* srcPath = strtok(newPartialPath, "/");
	char* FileName = (char *)malloc(sizeof(char) * 28);

	while (srcPath)
	{
		if (strlen(srcPath) < 28)
			strcpy(FileName , srcPath);
		else
			strncpy(FileName , srcPath, 28);

		srcPath = strtok(NULL, "/");
	}

	Inode_type tempInode;
	tempInode = getInodeBlock(1);

	int srcInode = 1;
	int found = 0;
	int end = 0;
	char ReadBuffer[BLOCK_SIZE];

    //going through all data blocks to find the source file name
	for (int addrIndex = 0; addrIndex < 9; addrIndex++)
	{
		unsigned int dataBlock = tempInode.addr[addrIndex];
		lseek(fd, dataBlock * BLOCK_SIZE, 0);
		//to go throiugh all directory entries in a block
		for (int index = 0; index < 32; index++)
		{
			dir_type tmpDir;

			read(fd, &tmpDir, sizeof(dir_type));
			if (strcmp(tmpDir.filename, FileName) == 0)
			{
				srcInode = tmpDir.inode;
				found = 1;
				break;
			}
			else if (strcmp(tmpDir.filename, "") == 0)
			{
				end = 0;
				break;
			}
		}
		if (found || end)
			break;
	}



	tempInode = getInodeBlock(srcInode);

	unsigned int TotalReadBytes = 0;
	unsigned int ReadBytes = 0;
	int finish = 0;
	int destFD = 0;
	if ((destFD = open(dest, O_RDWR | O_CREAT, 0600)) == -1)
	{
		printf("\n open() call for external file [%s] failed with error [%s]\n", dest, strerror(errno));
		return;
	}
	int i;
	//copying the exact number of blocks of source file
	for ( i = 0; i < tempInode.size1/BLOCK_SIZE; i++)
	{
		int written = 0;

		if (tempInode.addr[i] < 2)
		{
			finish = 1;
			break;
		}

		lseek(fd, tempInode.addr[i] * BLOCK_SIZE, 0);
		ReadBytes = read(fd, ReadBuffer, BLOCK_SIZE);


		if ((written = write(destFD, ReadBuffer, BLOCK_SIZE)) < 1)
		{
			printf("written %d", written);
		}
		TotalReadBytes += ReadBytes;
		if (ReadBytes < 1 )
		{
			finish = 1;
			break;
		}
	}
	//copying the partial part of the block used by the file
	lseek(fd, tempInode.addr[i] * BLOCK_SIZE, 0);
	ReadBytes = read(fd, ReadBuffer, tempInode.size1%BLOCK_SIZE);
	TotalReadBytes += ReadBytes;
	write(destFD, ReadBuffer, tempInode.size1%BLOCK_SIZE);
	printf("\nFile copied to the provided path\n\n");
	close(destFD);
	
}


//remove a file or a directory
void rm(char* fileName)
{
	char *newPartialPath = (char*)malloc(strlen(fileName));
	strcpy(newPartialPath, fileName);
	char* partialPath = strtok(newPartialPath, "/");
	char* finalFileName = (char *)malloc(sizeof(char) * 28);

	while (partialPath)
	{
		if (strlen(partialPath) < 28)
			strcpy(finalFileName, partialPath);
		else
			strncpy(finalFileName, partialPath, 28);

		partialPath = strtok(NULL, "/");
	}

	partialPath = strtok(fileName, "/");

	int parentInodeNum = 1;
	bool iamgroot = true;
	Inode_type tmpInode;

	while (partialPath)
	{
		bool foundParent = false;
		if (iamgroot)
		{
			if (strcmp(finalFileName, partialPath) != 0)
			{
				tmpInode = getInodeBlock(parentInodeNum);

				for (int addrIndex = 0; addrIndex < 9; addrIndex++)
				{
					unsigned int dataBlock = tmpInode.addr[addrIndex];
					lseek(fd, dataBlock * BLOCK_SIZE, 0);

					for (int index = 0; index < 32; index++)
					{
						dir_type tmpDir;

						read(fd, &tmpDir, sizeof(dir_type));
						if (strcmp(tmpDir.filename, partialPath) == 0)
						{
							parentInodeNum = tmpDir.inode;
							foundParent = true;

							strcpy(tmpDir.filename, "");
							tmpDir.inode = 0;

							lseek(fd, (dataBlock * BLOCK_SIZE) + (index * (sizeof(tmpDir))), 0);
							write(fd, &tmpDir, sizeof(tmpDir));
						}
						else if (strcmp(tmpDir.filename, "") == 0)
						{
							addrIndex = 9;
							break;
						}
						else if (foundParent)
						{
							lseek(fd, (dataBlock * BLOCK_SIZE) + ((index-1) * (sizeof(tmpDir))), 0);
							write(fd, &tmpDir, sizeof(tmpDir));
						}
					}
					if (foundParent)
						break;
				}

				if (!foundParent)
				{
					printf("\n The given path doesn't exist \n");
					return;
				}
			}
			else
			{ //final path 
				tmpInode = getInodeBlock(parentInodeNum);

				if (tmpInode.flags ==  1<<14)	//if it's directory
				{
					for (int addrIndex = 0;addrIndex < tmpInode.size1/BLOCK_SIZE; addrIndex++)
					{
						unsigned int dataBlock = tmpInode.addr[addrIndex];
						addFreeBlock(dataBlock);
					}
				}
				else		//if it's file
				{
					for (int index = 0; index < tmpInode.size1/BLOCK_SIZE; index++)
					{
						addFreeBlock(tmpInode.addr[index]);
					}
				}
				
				for (int i = 0; i < 9; i++)
					tmpInode.addr[i] = 0;
				tmpInode.flags = 0;
				
				writetoInode(parentInodeNum, tmpInode);
				break;
			}
		}

		iamgroot = true;
		partialPath = strtok(NULL, "/");
	}
	printf("\n file Deleted Successfully\n\n");
}

//to find the end of a string
int endOf(char str[100], char ch) {
    int i, pos=-1;
    for (i=0; i<100; i++) {
        if (str[i]==ch)
            pos = i;
    }
    return pos;
}
//to add the rest of the path to pd
void addPath(const char * str, char * buffer, size_t start, size_t end)
{
    size_t j = 0;
    size_t i;
    for ( i = start; i <= end; ++i ) {
        buffer[j++] = str[i];
    }
    buffer[j] = 0;
}
//method to change the directory
void changedir(char* dirName)
{
        Inode_type curINode = getInodeBlock(curINodeNumber);
        int blockNumber = curINode.addr[0];
        dir_type directory[100];
        int i;
        readFromBlockOffset(blockNumber,0,directory,curINode.size1);
        for(i = 0; i < curINode.size1/sizeof(dir_type); i++)
        {
                if(strcmp(dirName,directory[i].filename)==0){
                        Inode_type dir = getInodeBlock(directory[i].inode);
                        if(dir.flags ==( 1<<14 | 1<<15)){
                                if (strcmp(dirName, ".") == 0) {
                                        return;
                                }
                                else if (strcmp(dirName, "..") == 0) {
                                        curINodeNumber=directory[i].inode;
                                        int lastSlashPosition = endOf(pd, '/');
                                        char temp[100];
                                        addPath(pd, temp, 0, lastSlashPosition-1);
                                        strcpy(pd, temp);
                                }
                                else {
                                        curINodeNumber=directory[i].inode;
                                        strcat(pd, "/");
                                        strcat(pd, dirName);
                                } 
                        }
                        else{
                                printf("\n%s\n","NOT A DIRECTORY!");
                        }
                        return;
                }
        }
}
 // list directory contents
void ls(){ 
        Inode_type curINode = getInodeBlock(curINodeNumber);
        int blockNumber = curINode.addr[0];
        dir_type directory[100];
        int i;
        readFromBlockOffset(blockNumber,0,directory,curINode.size1);
        for(i = 0; i < curINode.size1/sizeof(dir_type); i++)
        {
                printf("%s\n",directory[i].filename);
        }
}

int openfs(const char *filename)
{
		char *path = filename;
	    //create file for File System if file doesn't exist
        if((fd = open(path,O_RDWR|O_CREAT,0600))== -1)
        {
                printf("\n file opening error [%s]\n",strerror(errno));
                return;
        }else{
			printf("The open() system call is successfully executed. \n");
        }
		
	strcpy(fs_path,path);
	lseek(fd,BLOCK_SIZE,SEEK_SET);
	//copying super block contents to superBlock struct
	read(fd,&superBlock,sizeof(superBlock));
	lseek(fd,2*BLOCK_SIZE,SEEK_SET);
	//copying inode number 1 contents to root struct 
    Inode_type root = getInodeBlock(1);
	read(fd,&root,sizeof(root));
	return 1;
}


void initfs( int total_blocks, int total_inode_blocks)
{
        printf("\n filesystem intialization started \n");
        total_inodes_count = total_inode_blocks*(BLOCK_SIZE/INODE_SIZE);
        char emptyBlock[BLOCK_SIZE] = {0}; //will write this emptyBlock to all data blocks to set them free
		Inode_type emptyInode;
		emptyInode.flags = 0<<15; //will write this emptyInode to all Inode to unallocate them
        int no_of_bytes,i,blockNumber,iNumber;

        // fsize
        superBlock.fsize = total_blocks;
		// isize
		superBlock.isize = total_inode_blocks;



        writeToDataBlock(total_blocks-1,emptyBlock,BLOCK_SIZE); // writing empty block to last block

        // adding all data blocks to the free array
        superBlock.nfree = 0;
        for (blockNumber= 1+superBlock.isize; blockNumber< total_blocks; blockNumber++)
                addFreeBlock(blockNumber);
		printf("\n All data blocks are set free and added to free[] array \n");
        // addding all free Inodes to inode array
        ninode = 0;
        for (iNumber=1; iNumber < total_inodes_count ; iNumber++)
                addFreeInode(iNumber);


        superBlock.flock = 'f';
        superBlock.ilock = 'i';
        superBlock.fmod = 'f';
        superBlock.time = 0;

        //write superBlock Block
        writeToDataBlock (1,&superBlock,BLOCK_SIZE);
		//printf("\n Super block is written to block number 1 \n");

        //allocating empty space for i-nodes
        for (i=1; i <= total_inodes_count; i++)
			femptyInode(i, emptyInode,INODE_SIZE );
		printf("\n All inodes are unallocated \n");

        createRootDirectory();
		printf("\n filesystem intialization completed \n");
}

void quit()
{
		if ( close(fd) ==0 ){
		printf("The close() system call is successfully executed. \n");
		}else{
			printf("\n file closing error [%s]\n",strerror(errno));
		}
        exit(0);
}



int main(int argc, char *argv[])
{
        char c;

        printf("\n Clearing screen \n");
        system("clear");

        unsigned int block_num =0, inode_num=0;
        char *fileSystem_path;
        char *arg1, *arg2;
        char *user_input, cmd[512];
        int fsinit = 0 ;
        unsigned short bytes_written;

        while(1)
        {
                printf("\n%s@%s>>>",fs_path,pd);
                scanf(" %[^\n]s", cmd);
                user_input = strtok(cmd," ");
                
		if(strcmp(user_input, "openfs")==0){
                        arg1 = strtok(NULL, " ");
						fileSystem_path = arg1;
                        openfs(arg1);
		}
		else if(strcmp(user_input, "initfs")==0)
		{

			arg1 = strtok(NULL, " ");
			arg2 = strtok(NULL, " ");
			if(access(fileSystem_path, X_OK) != -1)
			{
					printf("filesystem already exists. \n");
					printf("same file system will be used\n");
			}
			else
			{
					if (!arg1 || !arg2)
							printf(" insufficient arguments to proceed\n");
					else
					{
							block_num = atoi(arg1);
							inode_num = atoi(arg2);
							initfs(block_num, inode_num);
							fsinit =1;
					}
			}
			user_input = NULL;
		}
		else if(strcmp(user_input, "q")==0){
				quit();
		}
		else if (strcmp(user_input, "ls")==0) {
		
			ls();
			user_input=NULL;
		}
		else if (strcmp(user_input, "cd")==0) {
			char *path ;
			path = strtok(NULL, " ");
			changedir(path) ;
			user_input=NULL;
		}
		else if (strcmp(user_input, "cpin")==0)
		{
		   if (fsinit == 0)
		   { printf("\n The file system is not initialized yet");
		   return 0; }
		   char *source;
		   char *dest;
		   source= strtok(NULL," ");
		   dest= strtok(NULL," ");
		   if (!source || !dest)
		   { printf("\n src and dst not entered");}
		   else {cpin (source,dest);}
		
			user_input = NULL;
		}
		
		else if (strcmp(user_input, "cpout") == 0){
			//check if file system is initialised
			if (fsinit == 0)
			{
				printf("\n The file system has not been initialised yet. Please Initialise before trying out the other operations \n");
				return 0;
			}
			char *source;
			char *dest;

			//populate the expected fields from the command entered
			source = strtok(NULL, " ");
			dest = strtok(NULL, " ");

			if (!source || !dest)
			{
				printf("\n Source path and destination path are not entered \n");
			}
			else
			{
				//CheckFileExists(source);
				cpout(source, dest);
			}

			user_input = NULL;
		}
		else if (strcmp(user_input, "mkdir") == 0)
		{
			//check if file system is initialised
			if (fsinit == 0)
			{
				printf("\n The file system has not been initialised yet. Please Initialise before trying out the other operations \n");
				return 0;
			}
			char *dirName = strtok(NULL, " "); ;
			if (!dirName)
			{
				printf("Enter a directory name for mkdir\n");
				return 0;
			}
			//get a free inode
			unsigned int newInodeNum = getFreeInode();
			if (newInodeNum < 2)
			{
				printf("newInodeNum is %d \n",newInodeNum);
				printf("No free inodes remain\n");
				return 0;
			}

			makeDir(dirName, newInodeNum);

			user_input = NULL;
		}		
		else if (strcmp(user_input, "rm") == 0)
		{
			//check if file system is initialised
			if (fsinit == 0)
			{
				printf("\n The file system has not been initialised yet. Please Initialise before trying out the other operations \n");
				return 0;
			}

			char *fileName = strtok(NULL, " "); ;
			if (!fileName)
			{
				printf("Enter a directory name to remove\n");
				return 0;
			}

			rm(fileName);
			user_input = NULL;
		}else if(strcmp(user_input, "pd")==0){
						printf("%s\n",pd);
				}
		else
		{
			printf("\nInvalid command\n ");
		}
                
        }
        return 0;
}
