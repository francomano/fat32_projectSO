#include "fat32.h"
#include <stdio.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>
#include <sys/mman.h>
#include <stdlib.h>
#include <unistd.h>
int main(int agc, char** argv) {

  DiskDriver* disk=(DiskDriver*)malloc(sizeof(DiskDriver));

  DiskDriver_init(disk, "./file.txt", 100);
  fat32* fs=(fat32*)malloc(sizeof(fat32));
  DirectoryHandle* root=fat32_init(fs,disk);
  printf("root num entries %d\n",root->dcb->num_entries);

  //SEZIONE CREA CARTELLE

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
  printf("prova3 dirsize: %d\n",root->dcb->fcb.size);

  //printf("root num entries dopo aver fatto avanti e indietro nelle cartelle %d\n",root->dcb->num_entries);


  //SEZIONE CREA FILE 
  FileHandle* fh=fat32_createFile(root,"file_test.txt");
  if(!fh) printf("Error  creating file");
  printf("prova3 dirsize dopo che ho creato il file: %d\n",root->dcb->fcb.size);


  //sezione WRITE
  char*buff=malloc(1000);
  memset(buff,1,1000);
  //memcpy(buff,"Hello W0rld!",13);
  int ret=fat32_write(fh,buff,1000);
  printf("prova3 dirsize dopo la prima write: %d\n",root->dcb->fcb.size);
  printf("%d\n",ret);
  fh->pos_in_file=0;
  memset(buff,0,1000);
  printf("root dirsize: %d\n",root->dcb->fcb.size);
  ret=fat32_write(fh,buff,1000);
   printf("prova3 dirsize dopo la seconda write: %d\n",root->dcb->fcb.size);
  printf("%d\n",ret);
  printf("root dirsize: %d\n",root->dcb->fcb.size);
  ret=fat32_write(fh,buff,1000);
   printf("prova3 dirsize dopo la terza write: %d\n",root->dcb->fcb.size);
  printf("%d\n",ret);
  printf("prova3 dirsize: %d\n",root->dcb->fcb.size);
  


 //SEZIONE READ
 
  char buf[1000];
  fh->pos_in_file=0;
  ret=fat32_read(fh,buf,1000);
  printf("nella prima lettura ho letto %d\n",ret);

  ret=fat32_read(fh,buf,1000);
  printf("nella seconda lettura ho letto %d\n",ret);
  ret=fat32_read(fh,buf,1000);
  printf("nella terza lettura ho letto %d\n",ret);
  printf("filesize: %d\n",fh->ffb->fcb.size);
  printf("cursore a %d\n",fh->pos_in_file);




  //TORNO ALLA HOME
  if(fat32_changeDir(root,"..")){
    printf("errore nella changeDir\n");
  }
  if(fat32_changeDir(root,"..")){
    printf("errore nella changeDir\n");
  }
  if(fat32_changeDir(root,"prova")){
    printf("errore nella changeDir\n");
  }
   if(fat32_changeDir(root,"prova3")){
    printf("errore nella changeDir\n");
  }

  FileHandle* fh1=fat32_openFile(root,"hello.txt");
  assert(!fh&&"FILE INESISTENTE");
  printf("Ho aperto il file: %s\n",fh1->ffb->fcb.name);
 
 
  
}
