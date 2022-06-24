#include "fat32.h"
#include <stdio.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>
#include <sys/mman.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>  
#include "linked_list.h"

#define NUMBLOCKS 100
#define NUMCMD 13
#define LENCMD 128
#define LENPATH 128
#define LINE 1024


char* cmd[NUMCMD]= {
    "init","format","createFile",
    "ls","openFile","close",
    "write","read","seek",
    "cd","mkDir","rm","exit"
};



//LIST OF FILEHANDLES


int main(int argc, char** argv) {
    if(argc<2) {
        printf("Inserire il nome del disco\n");
        return 0;
    }
    int ret;
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
    ret=DiskDriver_readBlock(disk,fs->cwd,0);
    assert(ret && "Errore lettura disco");
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


    ListHead* head=(ListHead*)malloc(sizeof(ListHead));
    List_init(head);
    FileHandle* fh;

    char user_cmd[LENCMD];
    while(1) {
        printf("%s : ",path);
        fflush(stdin);
        fgets(user_cmd, LENCMD, stdin);
        
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
                
            
                if(!strcmp(ARG,"..") && strcmp(root->dcb->fcb.name,"/")){
                    int len2=strlen(root->f->cwd->fcb.name)+1;
                    int len1=strlen(path);
                    int newlen=len1-len2;
                    char* s=(char*)malloc(LENPATH);
                    char*temp=path;
                    strncpy(s,path,newlen);                        
                    path=s;
                    free(temp);
                    ret=fat32_changeDir(root,ARG);
                    if(ret) {
                        printf("something went wrong\n");
                    }
                    
                        
                }
                else if(!strcmp(ARG,"..") && !strcmp(root->dcb->fcb.name,"/")){

                }
                else{
                    ret=fat32_changeDir(root,ARG);
                    if(ret) {
                        printf("something went wrong\n");
                    }
                    else{
                        strcat(path,root->f->cwd->fcb.name);                        
                        strcat(path,"/");
                    }
                        
                    }
                
            }
            else if (!strcmp(CMD,"mkDir"))
            {
            fat32_mkDir(root,ARG);
            }
            else if (!strcmp(CMD,"createFile"))
            {
                fh=fat32_createFile(root,ARG);
                if(fh) {
                    List_insert(head,NULL,(ListItem*)fh);
                    List_print(head);
                }
            }
            else if(!strcmp(CMD,"openFile")) {
                fh=fat32_openFile(root,ARG);
                if(fh) {
                    List_insert(head,NULL,(ListItem*)fh);
                    List_print(head);
                }
            }

            else if(!strcmp(CMD,"write")) {
                if(!ARG) {
                    printf("usage: write <nomefile>\n");
                    continue;
                }
                if((fh = (FileHandle*)List_find(head,ARG))==NULL) {
                    printf("File handle at :%p\n",fh);
                    printf("File non aperto o non creato\n");
                    continue;
                }
                printf("Inserire il text (0<=t<=%d)\n",LINE);
                char buff[LINE];
                fflush(stdin);
                fgets(buff,LINE,stdin);
                ret=fat32_write(fh,buff,strlen(buff));
                if(ret!=strlen(buff)) {
                    printf("Scrittura parziale del messaggio\n");
                }
                else printf("WRITE SUCCESS bytes_written: %d\n",ret);
            }
            else if(!strcmp(CMD,"seek")) {
                if(!ARG) {
                    printf("usage: seek <nomefile>\n");
                    continue;
                } 
                if((fh = (FileHandle*)List_find(head,ARG))==NULL) {
                    printf("File handle at :%p\n",fh);
                    printf("File non aperto o non creato\n");
                    continue;
                }
                printf("Inserire la posizione (0<=p<=%d)\n",fh->ffb->fcb.size);
                char p[LINE];
                fflush(stdin);
                fgets(p,LINE,stdin);
                int pos=atoi(p);
                ret=fat32_seek(fh,pos);
            } 
            else if(!strcmp(CMD,"read")) {
                if(!ARG) {
                    printf("usage: read <nomefile>\n");
                    continue;
                } 
                if((fh = (FileHandle*)List_find(head,ARG))==NULL) {
                    printf("File handle at :%p\n",fh);
                    printf("File non aperto o non creato\n");
                    continue;
                }
                printf("Inserire i bytes da leggere (0<=b<=%d)\n",fh->ffb->fcb.size);
                char p[LINE];
                fflush(stdin);
                fgets(p,LINE,stdin);
                int size=atoi(p);
                printf("size:%d\n",size);
                char buff[size];
                ret=fat32_read(fh,buff,size);
                printf("%s\n",buff);
                printf("bytes_read: %d\n",ret);
            }
            
        }     
    }



 
    //cleanup
    free(disk);
    free(fs->cwd);
    free(fs);
    free(root);
    free(path);
}