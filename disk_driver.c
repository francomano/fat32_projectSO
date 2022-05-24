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
  int ret=ftruncate(disk->fd,sysconf(_SC_PAGE_SIZE)+num_blocks*sizeof(int)+num_blocks*BLOCK_SIZE);
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
//edoardo
int DiskDriver_readBlock(DiskDriver* disk, void* dest, int block_num) {
      int n_blocks = disk->header->num_blocks;
      if(block_num<0 || block_num>=n_blocks) {
         fprintf(stderr,"Block %d out of range (0<=block<=%d)",block_num,n_blocks);
         return -1;
      }
      int fd = disk->fd;
      int ret = lseek(fd,sysconf(_SC_PAGE_SIZE)+ n_blocks*sizeof(int)+BLOCK_SIZE*block_num,SEEK_SET);
      if(ret<0) {
         fprintf(stderr,"Error seeking block %d",block_num);
         return -1;
      }
      ret = read(fd,dest,BLOCK_SIZE);
      if(ret==-1) {
         fprintf(stderr,"Error reading block %d",block_num);
         return -1;
      }
      return ret;
}


// writes a block in position block_num, and alters the bitmap accordingly
// returns -1 if operation not possible

//marco
int DiskDriver_writeBlock(DiskDriver* disk, void* src, int block_num){
   if(block_num==disk->header->first_free_block) //ATOMIC
      disk->header->first_free_block=DiskDriver_getFreeBlock(disk,disk->header->first_free_block);
   int fd=disk->fd;
   int ret=lseek(fd,sysconf(_SC_PAGE_SIZE)+disk->header->num_blocks*sizeof(int)+BLOCK_SIZE*block_num,SEEK_SET);
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
   disk->header->free_blocks--;
   return ret;

   

}

// frees a block in position block_num, and alters the bitmap accordingly
// returns -1 if operation not possible
//edoardo
int DiskDriver_freeBlock(DiskDriver* disk, int block_num) {
   int n_blocks = disk->header->num_blocks;
   if(block_num<0 || block_num>=n_blocks) {
      fprintf(stderr,"Block %d out of range (0<=block<=%d)",block_num,n_blocks);
      return -1;
   }
   int fd = disk->fd;
   int ret = lseek(fd,sysconf(_SC_PAGE_SIZE)+ n_blocks*sizeof(int)+BLOCK_SIZE*block_num,SEEK_SET);
   if(ret<0) {
      fprintf(stderr,"Error seeking block %d",block_num);
      return -1;
   }
   int zero_block[BLOCK_SIZE] = {0};
   ret = write(fd,zero_block,BLOCK_SIZE);
   if(ret<0) {
      fprintf(stderr,"Error freeing block %d",block_num);
      return -1;
   }
   disk->fat[block_num] = -1;
   disk->header->free_blocks++;
   if(block_num<disk->header->first_free_block)  disk->header->first_free_block = block_num;
   return block_num;
}

// returns the first free blockin the disk from position (checking the bitmap)
int DiskDriver_getFreeBlock(DiskDriver* disk, int start){
   int n_blocks = disk->header->num_blocks;
   if(disk->header->free_blocks==0){
      fprintf(stderr,"Memory error out of disk space");
      return -1;
   }
   if(start<0 || start>n_blocks) {
      fprintf(stderr,"Start index %d out of range (0<=block<=%d)",start,n_blocks);
      return -1;
   }
   //if(disk->header->first_free_block>start) return disk->header->first_free_block;
   for(int i=start;i<n_blocks;i++) {
      if(disk->fat[i]==-1) {
         disk->header->first_free_block = i;
         return i;
      }
   }
   return disk->header->first_free_block;
}

// writes the data (flushing the mmaps)
int DiskDriver_flush(DiskDriver* disk);
