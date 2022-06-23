#include "fat32.h"
#include <stdio.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>
#include <sys/mman.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>  

#define NUMBLOCKS 100
#define NUMCMD 13
#define LENCMD 20
#define LENPATH 128


char* cmd[NUMCMD]= {
    "init","format","createFile",
    "ls","openFile","close",
    "write","read","seek",
    "cd","mkDir","rm","exit"
};

int ret;

//LIST OF FILEHANDLES


int main(int argc, char** argv) {
    if(argc<2) {
        printf("Inserire il nome del disco\n");
        return 0;
    }
    //READ THE DISK
    char filename[64];
    strcpy(filename,argv[1]);
    DiskDriver* disk=(DiskDriver*)malloc(sizeof(DiskDriver));
    fat32* fs=(fat32*)malloc(sizeof(fat32));
    DirectoryHandle* root= (DirectoryHandle*)malloc(sizeof(DirectoryHandle));
    disk->fd=open (filename,O_RDWR | O_SYNC,0666);
    disk->header=mmap (0, sizeof(DiskHeader), PROT_READ | PROT_WRITE, MAP_SHARED, disk->fd, 0);
    disk->fat =mmap(0,100*sizeof(int),PROT_READ | PROT_WRITE,MAP_SHARED,disk->fd,sysconf(_SC_PAGE_SIZE));
    fs->disk=disk;
    fs->cwd=(FirstDirectoryBlock*)malloc(BLOCK_SIZE);
    DiskDriver_readBlock(disk,fs->cwd,0);
    root->directory=NULL;
    root->dcb=fs->cwd;
    root->f=fs;
    /**********************************************************************/



    
    printf("Welcome to our fancy FileSystem, here's a list of what u can do:\n");
    for(int i=0;i<NUMCMD;i++) {
        printf("%s\t",cmd[i]);
        if(i!=0 && i%2==0) printf("\n");
    }
    char* path=malloc(LENPATH);
    strcpy(path,root->dcb->fcb.name);



    while(1) {
        printf("%s : ",path);
        char user_cmd[LENCMD];
       
        fgets(user_cmd, LENCMD, stdin);
        fflush(stdin);
        //remove new line
        if ((strlen(user_cmd) > 0) && (user_cmd[strlen (user_cmd) - 1] == '\n'))
        user_cmd[strlen (user_cmd) - 1] = '\0';

        char* CMD;
        char* ARG;

        CMD=strtok(user_cmd," ");
        ARG=strtok(NULL," ");
   
        
        if(CMD){
            if(!strcmp(CMD,"exit"))
            {
                break;
            }
            else if (!strcmp(CMD,"ls"))
            {
                char** names=filename_alloc();
                fat32_listDir(names,root);
                for(int i=0;i<root->dcb->num_entries;i++){
                    printf("%s\n",names[i]);
                }
                filename_dealloc(names);
            }
            else if (!strcmp(CMD,"cd"))
            {
                int len2=strlen(root->f->cwd->fcb.name)+1;
                ret=fat32_changeDir(root,ARG);
                if(ret!=-1){
                    if(!strcmp(ARG,"..")){
                        int len1=strlen(path);
                        int newlen=len1-len2;
                        char* s=(char*)malloc(64);
                        strncpy(s,path,newlen);
                        path=s;
                        
                    }
                    else{
                        strcat(path,root->f->cwd->fcb.name);
                        strcat(path,"/");
                        
                    }
                }
                else{
                    printf("cartella inesistente\n");
                }
            }
            else if (!strcmp(CMD,"mkDir"))
            {
            fat32_mkDir(root,ARG);
            }
            else if (!strcmp(CMD,"createFile"))
            {
            fat32_createFile(root,ARG);
            }
        

            
        }     
    }


    //FREE SECTION
    return 0;
}