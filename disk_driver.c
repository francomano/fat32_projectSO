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
  int ret=ftruncate(disk->fd,sizeof(DiskHeader)+num_blocks*sizeof(int)+num_blocks*BLOCK_SIZE);
   if(ret==-1){
      perror("ftruncate");
   }

  disk->header = mmap (0, sizeof(DiskHeader), PROT_READ | PROT_WRITE, MAP_SHARED, disk->fd, 0); 
  if(disk->header==MAP_FAILED){
     perror("header MAP FAILED \n");
  }
  disk->header->num_blocks=num_blocks;
  disk->header->free_blocks=num_blocks;
  disk->header->first_free_block=0;
  printf("%d\n",disk->header->free_blocks);

   /*if(msync(disk->header,sizeof(DiskHeader),MS_SYNC)==-1){
      perror("MSYNC");
   }*/

  disk->fat =mmap(0,num_blocks*sizeof(int),PROT_READ | PROT_WRITE,MAP_SHARED,disk->fd,sysconf(_SC_PAGE_SIZE));
  //printf("%d %d\n",disk->fat,MAP_FAILED);


  if(disk->fat==MAP_FAILED){
     perror("FAT MAP FAILED \n");
  }

   for(int i=0;i<num_blocks;i++){
      disk->fat[i]=-1;

   }
   

  /*if(msync(disk->fat,sizeof(int)*num_blocks,MS_SYNC)==-1){
      perror("MSYNC FAT");
   }*/
   
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
   int ret=lseek(fd,sizeof(DiskHeader)+disk->header->num_blocks*sizeof(int)+BLOCK_SIZE*block_num,SEEK_SET);
   printf("ret seek = %d\n",ret);
   if(ret<0){
      perror("seek");
      return -1;
   }
   ret=write(fd,src,BLOCK_SIZE);
   if(ret==-1){
      perror("write block error");
      return -1;
   }

   return ret;

   

}

// frees a block in position block_num, and alters the bitmap accordingly
// returns -1 if operation not possible
int DiskDriver_freeBlock(DiskDriver* disk, int block_num);

// returns the first free blockin the disk from position (checking the bitmap)
int DiskDriver_getFreeBlock(DiskDriver* disk, int start);

// writes the data (flushing the mmaps)
int DiskDriver_flush(DiskDriver* disk);
