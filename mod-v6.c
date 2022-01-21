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

FILE NAME: mod-v6.c


To execute program use the following commands on Linux server:

First: gcc -o modv6 mod-v6.c
Second: ./modv6

SUPPORTED COMMANDS: 
1. openfs <file_name.txt>
	SAMPLE: openfs test.txt
2. initfs <total_number_of_blocks> <total_number_of_blocks_for_inodes>
	SAMPLE: initfs 1000 300
	NOTE: total_number_of_blocks must be greater than total_number_of_blocks_for_inodes
3. q

===========================================================================================================
METHODS DEVELOPED BY:

1. Sai Vijay Sathwik Reddy Gade ( NET ID: SXG200001 )

	I)		writeToDataBlock
	II)		writeToBlockOffset
	III)	addFreeBlock
	
	
2. Mukesh Bachu ( NET ID: MXB190079 )

	I)		getInodeBlock
	II)		femptyInode
	III)	createRootDirectory

3. Riya Gupta  (NET ID: RXG210021)

	I)		getFreeBlock
	II)		writetoInode
	III		addFreeInode


NOTE: The methods openfs(), initfs(), quit() and main() have equal contributions from all the members.

================================================================================================================================================================
*/
/*************** super block structure**********************/
typedef struct {
        unsigned int isize;
        unsigned int fsize;
        unsigned int nfree;
        unsigned short free[251];
        unsigned short flock;
        unsigned short ilock;
        unsigned short fmod;
        unsigned int time[2];
} superBlock_type;

/****************inode structure ************************/
typedef struct {
        unsigned short flags;
        unsigned int nlinks;
        unsigned int uid;
        unsigned int gid;
        unsigned int size0;
		unsigned int size1;
        unsigned short addr[9];
        unsigned int actime;
        unsigned int modtime;
} Inode_type;


typedef struct
{
        unsigned short inode;
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
                writeToDataBlock(blockNumber, superBlock.free, FREE_ARRAY_SIZE * 2);
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

//method to get a inode block when required
Inode_type getInodeBlock (int INumber){
        Inode_type iNode;
        int blockNumber =  2 ;
        int offset = ((INumber - 1) * INODE_SIZE) % BLOCK_SIZE;
        lseek(fd,(BLOCK_SIZE * blockNumber) + offset, SEEK_SET);
        read(fd,&iNode,INODE_SIZE);
        return iNode;
}

//method to add availabe inodes to inode array
void addFreeInode(int iNumber){
        if(ninode == FREE_ARRAY_SIZE)
                return;
        inode[ninode] = iNumber;
        ninode++;
}

//method to write root contents to inode
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

//method to allocate root to inode number 1 and to store root contents to a data block
void createRootDirectory(){
        int blockNumber = getFreeBlock();
        dir_type directory[2];
        directory[0].inode = 0;
        strcpy(directory[0].filename,".");

        directory[1].inode = 0;
        strcpy(directory[1].filename,"..");

        writeToDataBlock(blockNumber, directory, 2*sizeof(dir_type));
		printf("\n root contents are stored in one of the data blocks  \n");

        Inode_type root;
        root.flags = 1<<14 | 1<<15; // setting flag bits 14  & 15 to 1 to allocate the  directory for the root
        root.nlinks = 1;
        root.uid = 0;
        root.gid = 0;
        root.size0 = sizeof(dir_type);
		root.size1 = sizeof(dir_type);
        root.addr[0] = blockNumber;
        root.actime = time(NULL);
        root.modtime = time(NULL);

        writetoInode(1,root);
		printf("\n root is allocated to inode number 1 \n");
        curINodeNumber = 1;
        strcpy(pd,"/");
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
void initfs( int total_blocks, int total_inodes)
{
        printf("\n filesystem intialization started \n");
        total_inodes_count = total_inodes;
        char emptyBlock[BLOCK_SIZE] = {0}; //will write this emptyBlock to all data blocks to set them free
		Inode_type emptyInode;
		emptyInode.flags = 0<<15; //will write this emptyInode to all Inode to unallocate them
        int no_of_bytes,i,blockNumber,iNumber;

        // isize (Number of blocks for inode)
        if(((total_inodes*INODE_SIZE)%BLOCK_SIZE) == 0)
                superBlock.isize = (total_inodes*INODE_SIZE)/BLOCK_SIZE;
        else
                superBlock.isize = (total_inodes*INODE_SIZE)/BLOCK_SIZE+1;

        // fsize
        superBlock.fsize = total_blocks;



        writeToDataBlock(total_blocks-1,emptyBlock,BLOCK_SIZE); // writing empty block to last block

        // adding all data blocks to the free array
        superBlock.nfree = 0;
        for (blockNumber= 1+superBlock.isize; blockNumber< total_blocks; blockNumber++)
                addFreeBlock(blockNumber);
		printf("\n All data blocks are set free and added to free[] array \n");
        // addding all free Inodes to inode array
        ninode = 0;
        for (iNumber=1; iNumber < total_inodes ; iNumber++)
                addFreeInode(iNumber);


        superBlock.flock = 'f';
        superBlock.ilock = 'i';
        superBlock.fmod = 'f';
        superBlock.time[0] = 0;
        superBlock.time[1] = 0;

        //write superBlock Block
        writeToDataBlock (1,&superBlock,BLOCK_SIZE);
		//printf("\n Super block is written to block number 1 \n");

        //allocating empty space for i-nodes
        for (i=1; i <= superBlock.isize; i++)
               // writeToDataBlock(i,emptyBlock,BLOCK_SIZE);
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

        while(1)
        {
                printf("\n%s@%s>>>",fs_path,pd);
                scanf(" %[^\n]s", cmd);
                user_input = strtok(cmd," ");
				if(strcmp(user_input, "openfs")==0){
                        arg1 = strtok(NULL, " ");
						fileSystem_path = arg1;
                        openfs(arg1);
                }else if(strcmp(user_input, "initfs")==0)
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
                                }
                        }
                        user_input = NULL;
                }else if(strcmp(user_input, "q")==0){
                        quit();
                }
        }
}
