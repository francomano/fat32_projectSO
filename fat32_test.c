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
printf("root num entries %d\n",root->dcb->num_entries);
  if(fat32_mkDir(root,"prova")){
    printf("errore mkdir\n");
  }
  printf("root num entries dopo aver creato prova %d\n",root->dcb->num_entries);

  if(fat32_mkDir(root,"prova2")){
    printf("errore mkdir\n");
  }
  printf("root num entries dopo aver creato prova2 %d\n",root->dcb->num_entries);
  if(fat32_changeDir(root,"prova")){
    printf("errore nella changeDir\n");
  }
 printf("ora sono in %s e ho %d entries e il mio parent ne ha %d\n",root->dcb->fcb.name,root->dcb->num_entries,root->directory->num_entries);
  

   if(fat32_mkDir(root,"prova3")){
    printf("errore mkdir\n");
  }
  if(fat32_changeDir(root,"prova3")){
    printf("errore nella changeDir\n");
  }
  printf("ora sono in %s e ho %d entries e il mio parent ne ha %d\n",root->dcb->fcb.name,root->dcb->num_entries,root->directory->num_entries);
  
 

   
  if(fat32_changeDir(root,"..")){
    printf("errore nella changeDir\n");
  }
  if(fat32_changeDir(root,"..")){
    printf("errore nella changeDir\n");
  }
  printf("%s",root->dcb->fcb.name);
  printf("root num entries dopo aver fatto avanti e indietro nelle cartelle %d\n",root->dcb->num_entries);
  FileHandle* fh=fat32_createFile(root,"file_test.txt");
  FirstFileBlock* ffb=(FirstFileBlock*)malloc(sizeof(FirstFileBlock));
  if(!fh) printf("Error  creating file");
  DiskDriver_readBlock(disk,ffb,fh->ffb->fcb.block_in_disk);
  printf("%s\n",ffb->fcb.name);
  //write&read testing
  int ret=fat32_write(fh,"Hell0 W0rld! ;)",16);
  printf("%d\n",ret);
  char buf[16];
  ret=fat32_read(fh,buf,16);
  printf("%s",buf);
  
 
  
}
