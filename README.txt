This readme file is for project2-part2

For this part of the project we are utilizing the previously created V6 file system from part-1.

Execution Instructions:
 
Use the following command to compile the source file in the unix server
 
 =====>   gcc modv6.c -o modv6 -std=c99
 =====>   ./modv6
 
The screenshots of execution of the program are included in the folder.
The external file used to copy to v6Filesystem is also attached.

From the modified file system we developed the functions to provide for the following commands:

NOTE: openfs and initfs must be executed before any other operation respectively.
 
1. openfs v6FileSystem  	         // to open/create a v6 filesystem
2. initfs 7000 200  	        	// to initialize the created filesystem with the total blocks and number of inode blocks
3. cpin externalfile.txt v6-file.txt   // trying copy an external file from outside into the file system
4. cpout v6-file.txt output.txt       // copying the internal v6-file to an external file
5. rm v6-file.txt                    // removing the v6-file
6. mkdir v6dir 		            // creating a new directory
7. cd v6dir                        // change directory
8. q                              // quit and close the file system 
 
