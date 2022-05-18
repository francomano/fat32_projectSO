#include "disk_driver.h"
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>  
#include <string.h>  
#include <errno.h>  
#include <signal.h>  
#include <fcntl.h>  
#include <ctype.h>  
#include <termios.h>  
#include <sys/types.h>  
#include <sys/mman.h> 

/**
   The blocks indices seen by the read/write functions 
   have to be calculated after the space occupied by the bitmap
*/

// opens the file (creating it if necessary_
// allocates the necessary space on the disk
// if the file was new
// compiles a disk header

//marco
void DiskDriver_init(DiskDriver* disk, const char* filename, int num_blocks){
   if ((disk->fd = open (filename, O_CREAT |O_RDWR | O_SYNC,0666)) == -1) { 
    perror("error in the opening of file");
  } 
  int ret=ftruncate(disk->fd,sizeof(DiskHeader)+num_blocks*sizeof(fatElem)+num_blocks*BLOCK_SIZE);


  disk->header = mmap (0, sizeof(DiskHeader), PROT_READ | PROT_WRITE, MAP_SHARED, disk->fd, 0); 
  if(disk->header==MAP_FAILED){
     perror("header MAP FAILED \n");
  }
  disk->fat =mmap(disk->header+sizeof(DiskHeader),num_blocks*sizeof(fatElem),PROT_READ | PROT_WRITE,MAP_SHARED,disk->fd,0);
  
  if(disk->fat==MAP_FAILED){
     perror("FAT MAP FAILED \n");
  }
   disk->fatDim=num_blocks;
}

// reads the block in position block_num
// returns -1 if the block is free accrding to the bitmap
// 0 otherwise
int DiskDriver_readBlock(DiskDriver* disk, void* dest, int block_num);

// writes a block in position block_num, and alters the bitmap accordingly
// returns -1 if operation not possible

//marco
int DiskDriver_writeBlock(DiskDriver* disk, void* src, int block_num){
   int fd=disk->fd;
   int ret=lseek(fd,sizeof(DiskHeader)+disk->fatDim*sizeof(fatElem)+BLOCK_SIZE*block_num,SEEK_SET);
   if(ret<0){
      perror("seek");
   }
   ret=write(fd,src,BLOCK_SIZE);


   

}

// frees a block in position block_num, and alters the bitmap accordingly
// returns -1 if operation not possible
int DiskDriver_freeBlock(DiskDriver* disk, int block_num);

// returns the first free blockin the disk from position (checking the bitmap)
int DiskDriver_getFreeBlock(DiskDriver* disk, int start);

// writes the data (flushing the mmaps)
int DiskDriver_flush(DiskDriver* disk);
