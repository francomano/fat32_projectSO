#pragma once
#include "disk_driver.h"
#include <stdlib.h>
#include "linked_list.h"

/*these are structures stored on disk*/



// this is in the first block of a chain, after the header
typedef struct {
  int directory_block; // first block of the parent directory
  int block_in_disk;   // repeated position of the block on the disk
  char name[64];
  int  size;
  int is_dir;          // 0 for file, 1 for dir
} FileControlBlock;

// this is the first physical block of a file
// it has a header
// an FCB storing file infos
// and can contain some data

/******************* stuff on disk BEGIN *******************/
typedef struct {
  FileControlBlock fcb;
  char data[BLOCK_SIZE-sizeof(FileControlBlock)] ;
} FirstFileBlock;

// this is one of the next physical blocks of a file
typedef struct {
  char  data[BLOCK_SIZE];
} FileBlock;

// this is the first physical block of a directory
typedef struct {
  FileControlBlock fcb;
  int num_entries;
  int file_blocks[ (BLOCK_SIZE
		   -sizeof(FileControlBlock)
		    -sizeof(int))/sizeof(int) ];
} FirstDirectoryBlock;

// this is remainder block of a directory
typedef struct {
  int file_blocks[ (BLOCK_SIZE)/sizeof(int) ]; //array di indici dei primi blocchi di ogni file che la cartella contiene
} DirectoryBlock;
/******************* stuff on disk END *******************/




  
typedef struct {
  DiskDriver* disk;
  FirstDirectoryBlock* cwd;
  // add more fields if needed
} fat32;

// this is a file handle, used to refer to open files
typedef struct {
  ListItem item;
  fat32* f;                   // pointer to memory file system structure
  FirstFileBlock* ffb;             // pointer to the first block of the file(read it)
  FirstDirectoryBlock* directory  ;  // pointer to the directory where the file is stored
  int pos_in_file;                 // position of the cursor
} FileHandle;

typedef struct {
  fat32* f;                   // pointer to memory file system structure
  FirstDirectoryBlock* dcb;        // pointer to the first block of the directory(read it)
  FirstDirectoryBlock* directory;  // pointer to the parent directory (null if top level)
  int pos_in_dir;                  // absolute position of the cursor in the directory
  int pos_in_block;                // relative position of the cursor in the block
} DirectoryHandle;

// initializes a file system on an already made disk
// returns a handle to the top level directory stored in the first block
DirectoryHandle* fat32_init(fat32* fs, DiskDriver* disk);

// creates the inital structures, the top level directory
// has name "/" and its control block is in the first position
// it also clears the bitmap of occupied blocks on the disk
// the current_directory_block is cached in the SimpleFS struct
// and set to the top level directory
void fat32_format(fat32* fs);

// creates an empty file in the directory d
// returns null on error (file existing, no free blocks)
// an empty file consists only of a block of type FirstBlock
FileHandle* fat32_createFile(DirectoryHandle* d, const char* filename);

char** filename_alloc(void);

void filename_dealloc(char** names);


int fat32_listDir(char** names, DirectoryHandle* d);



// opens a file in the  directory d. The file should be exisiting
FileHandle* fat32_openFile(DirectoryHandle* d, const char* filename);


// closes a file handle (destroyes it)
int fat32_close(FileHandle* f);

// writes in the file, at current position for size bytes stored in data
// overwriting and allocating new space if necessary
// returns the number of bytes written
int fat32_write(FileHandle* f, void* data, int size);

// writes in the file, at current position size bytes stored in data
// overwriting and allocating new space if necessary
// returns the number of bytes read
int fat32_read(FileHandle* f, void* data, int size);

// returns the number of bytes read (moving the current pointer to pos)
// returns pos on success
// -1 on error (file too short)
int fat32_seek(FileHandle* f, int pos);

// seeks for a directory in d. If dirname is equal to ".." it goes one level up
// 0 on success, negative value on error
// it does side effect on the provided handle
 int fat32_changeDir(DirectoryHandle* d, char* dirname);

int fat32_update_size(DirectoryHandle* d,int num);
// creates a new directory in the current one (stored in fs->current_directory_block)
// 0 on success
// -1 on error
int fat32_mkDir(DirectoryHandle* d, char* dirname);

// removes the file in the current directory
// returns -1 on failure 0 on success
// if a directory, it removes recursively all contained files
int fat32_remove(DirectoryHandle* d, char* filename);


  

