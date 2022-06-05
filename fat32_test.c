#include "fat32.h"
#include <stdio.h>
#include <assert.h>
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
  printf("%s\n",root->dcb->fcb.name);
  FirstDirectoryBlock* fdb=(FirstDirectoryBlock*)malloc(sizeof(BLOCK_SIZE));
  DiskDriver_readBlock(disk,fdb,root->dcb->file_blocks[0]);
  printf("%s\n",fdb->fcb.name);

  if(fat32_mkDir(root,"prova2")){
    printf("errore mkdir\n");
  }
  if(fat32_changeDir(root,"prova")){
    printf("errore nella changeDir\n");
  }
  printf("ora sono in %s\n",root->dcb->fcb.name);

   if(fat32_mkDir(root,"prova3")){
    printf("errore mkdir\n");
  }
  if(fat32_changeDir(root,"prova3")){
    printf("errore nella changeDir\n");
  }
   printf("ora sono in %s\n",root->dcb->fcb.name);
  if(fat32_changeDir(root,"..")){
    printf("errore nella changeDir\n");
  }
  if(fat32_changeDir(root,"..")){
    printf("errore nella changeDir\n");
  }
  printf("%s",root->dcb->fcb.name);
  FileHandle* fh=fat32_createFile(root,"file_test.txt");
  FirstFileBlock* ffb=(FirstFileBlock*)malloc(sizeof(FirstFileBlock));
  if(!fh) printf("Error  creating file");
  DiskDriver_readBlock(disk,ffb,fh->ffb->fcb.block_in_disk);
  printf("%s\n",ffb->fcb.name);

  int ret=fat32_write(fh,"ciao",4);
  printf("%d\n",ret);
  char buf[5];
  ret=fat32_read(fh,buf,4);
  printf("%s",buf);
  getchar();
 
  
}
