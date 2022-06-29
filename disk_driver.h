#pragma once


#define BLOCK_SIZE 512
// this is stored in the 1st block of the disk
typedef struct {
  int num_blocks;
  int free_blocks;     // free blocks
  int first_free_block;// first block index

} DiskHeader; 


typedef struct {
  DiskHeader* header; // mmapped
  int* fat; //mmapped
  int fd; // for us
} DiskDriver;




void DiskDriver_init(DiskDriver* disk, const char* filename, int num_blocks);


int DiskDriver_readBlock(DiskDriver* disk, void* dest, int block_num);


int DiskDriver_writeBlock(DiskDriver* disk, void* src, int block_num);


int DiskDriver_freeBlock(DiskDriver* disk, int block_num);


int DiskDriver_getFreeBlock(DiskDriver* disk, int start);


int DiskDriver_flush(DiskDriver* disk);
