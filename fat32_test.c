#include "fat32.h"
#include <stdio.h>

#include <unistd.h>
#include <string.h>
#include <sys/mman.h>
#include <stdlib.h>

int main(int agc, char** argv) {
 printf("FirstBlock size %ld\n", sizeof(FirstFileBlock));
  printf("DataBlock size %ld\n", sizeof(FileBlock));
  printf("FirstDirectoryBlock size %ld\n", sizeof(FirstDirectoryBlock));
  printf("DirectoryBlock size %ld\n", sizeof(DirectoryBlock));


  DiskDriver* disk=(DiskDriver*)malloc(sizeof(DiskDriver));

  DiskDriver_init(disk, "./file.txt", 100);

  printf("%d\n",disk->fat[0]);
  printf("%d\n",disk->header->free_blocks);

  printf("%d\n",sysconf(_SC_PAGE_SIZE));


  getchar();

  
}
