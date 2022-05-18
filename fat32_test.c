#include "fat32.h"
#include <stdio.h>


#include <string.h>
#include <sys/mman.h>
#include <stdlib.h>

int main(int agc, char** argv) {
 printf("FirstBlock size %ld\n", sizeof(FirstFileBlock));
  printf("DataBlock size %ld\n", sizeof(FileBlock));
  printf("FirstDirectoryBlock size %ld\n", sizeof(FirstDirectoryBlock));
  printf("DirectoryBlock size %ld\n", sizeof(DirectoryBlock));

  fatElem f [2];
  printf("fatElem size %ld\n",sizeof(f));

  DiskDriver* disk=(DiskDriver*)malloc(sizeof(DiskDriver));

  DiskDriver_init(disk, "./file.txt", 100);



  getchar();

  
}
