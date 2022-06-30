#pragma once
#include "disk_driver.h"
#include <stdlib.h>
#include "linked_list.h"

/*these are structures stored on disk*/




typedef struct {
  int directory_block;
  int block_in_disk;   
  char name[64];
  int  size;
  int is_dir;        
} FileControlBlock;



/******************* stuff on disk BEGIN *******************/
typedef struct {
  FileControlBlock fcb;
  char data[BLOCK_SIZE-sizeof(FileControlBlock)] ;
} FirstFileBlock;


typedef struct {
  char  data[BLOCK_SIZE];
} FileBlock;


typedef struct {
  FileControlBlock fcb;
  int num_entries;
  int file_blocks[ (BLOCK_SIZE
		   -sizeof(FileControlBlock)
		    -sizeof(int))/sizeof(int) ];
} FirstDirectoryBlock;

typedef struct {
  int file_blocks[ (BLOCK_SIZE)/sizeof(int) ];
} DirectoryBlock;
/******************* stuff on disk END *******************/




  
typedef struct {
  DiskDriver* disk;
  FirstDirectoryBlock* cwd;
} fat32;

typedef struct {
  ListItem item;
  fat32* f;                   
  FirstFileBlock* ffb;             
  FirstDirectoryBlock* directory;  
  int pos_in_file;                 
} FileHandle;

typedef struct {
  fat32* f;                   
  FirstDirectoryBlock* dcb;      
  FirstDirectoryBlock* directory;
} DirectoryHandle;


DirectoryHandle* fat32_init(fat32* fs, DiskDriver* disk);


void fat32_format(fat32* fs);


FileHandle* fat32_createFile(DirectoryHandle* d, const char* filename);

char** filename_alloc(void);

void filename_dealloc(char** names);


int fat32_listDir(char** names, DirectoryHandle* d);




FileHandle* fat32_openFile(DirectoryHandle* d, const char* filename);



int fat32_close(FileHandle* f);


int fat32_write(FileHandle* f, void* data, int size);


int fat32_read(FileHandle* f, void* data, int size);


int fat32_seek(FileHandle* f, int pos);


 int fat32_changeDir(DirectoryHandle* d, char* dirname);

void fat32_update_size(DirectoryHandle* d,int num);

int fat32_mkDir(DirectoryHandle* d, char* dirname);


int fat32_remove(DirectoryHandle* d, char* filename);


  

