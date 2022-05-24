#include "fat32.h"
#include <stdio.h>

#include <unistd.h>
#include <string.h>
#include <sys/mman.h>
#include <stdlib.h>
#include <unistd.h>
int main(int agc, char** argv) {
 printf("FirstBlock size %ld\n", sizeof(FirstFileBlock));
  printf("DataBlock size %ld\n", sizeof(FileBlock));
  printf("FirstDirectoryBlock size %ld\n", sizeof(FirstDirectoryBlock));
  printf("DirectoryBlock size %ld\n", sizeof(DirectoryBlock));


  DiskDriver* disk=(DiskDriver*)malloc(sizeof(DiskDriver));

  DiskDriver_init(disk, "./file.txt", 100);

  //printf("%d\n",disk->fat[0]);
  //printf("%d\n",disk->header->free_blocks);

  //printf("%d\n",sysconf(_SC_PAGE_SIZE));

 /* FileControlBlock FCB={
    .name="NAME\0"
  };
  FirstFileBlock* ff=(FirstFileBlock*)malloc(sizeof(FirstFileBlock));
  ff->fcb=FCB;

  int ret=DiskDriver_writeBlock(disk, ff, 0);
  printf("%d\n",ret);*/
  fat32* fs=(fat32*)malloc(sizeof(fat32));
  DirectoryHandle* root=fat32_init(fs,disk);
  printf("%d\n",root->pos_in_block);

  if(fat32_mkDir(root,"prova")){
    printf("errore mkdir\n");
  }
  
  FirstDirectoryBlock* fdb=(FirstDirectoryBlock*)malloc(sizeof(BLOCK_SIZE));
  DiskDriver_readBlock(disk,fdb,root->dcb->file_blocks[0]);
  printf("%s\n",fdb->fcb.name);

  if(fat32_mkDir(root,"prova2")){
    printf("errore mkdir\n");
  }

  getchar();

  
}
