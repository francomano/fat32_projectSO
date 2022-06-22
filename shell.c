#include "fat32.h"
#include <stdio.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>
#include <sys/mman.h>
#include <stdlib.h>
#include <unistd.h>

char* cmd[13]= {
    "init","format","createFile",
    "ls","openFile","close",
    "write","read","seek",
    "cd","mkDir","rm","exit"
};



int main(int argc, char** argv) {
    if(argc<2) {
        printf("Inserire il numero di blocchi");
        return 0;
    }
    int num_blocks=atoi(argv[1]);
    DiskDriver* disk=(DiskDriver*)malloc(sizeof(DiskDriver));
    DiskDriver_init(disk, "./file.txt",num_blocks);
    fat32* fs=(fat32*)malloc(sizeof(fat32));
    DirectoryHandle* root=fat32_init(fs,disk);
    printf("Welcome to our fancy FileSystem, here's a list of what u can do:\n");
    for(int i=0;i<13;i++) {
        printf("%s\t",cmd[i]);
        if(i!=0 && i%2==0) printf("\n");
    }
    while(1) {
        char user_cmd[10];
        scanf("%s",user_cmd);
        if(!strcmp(user_cmd,"exit"))break;
    }
    return 0;
}