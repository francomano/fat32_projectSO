#include "fat32.h"
#include "string.h"
#include <stdio.h>
#include <math.h>



//edo e marco
DirectoryHandle* fat32_init(fat32* fs, DiskDriver* disk){
    fs->disk=disk;
    FirstDirectoryBlock* fdb=(FirstDirectoryBlock*)malloc(sizeof(FirstDirectoryBlock));
    fdb->num_entries=0;
    fdb->fcb.directory_block=0;
    fdb->fcb.block_in_disk=0;
    int size=(BLOCK_SIZE
		   -sizeof(FileControlBlock)
		    -sizeof(int))/sizeof(int);
    for(int i=0; i<size;i++){
        fdb->file_blocks[i]=-1;
    }
    strcpy(fdb->fcb.name,"/\0");
    fdb->fcb.size=0;
    fdb->fcb.is_dir=1;
    DiskDriver_writeBlock(fs->disk, fdb, 0);
    DirectoryHandle* root=(DirectoryHandle*)malloc(sizeof(DirectoryHandle));
    root->f=fs;
    root->dcb=fdb;
    root->directory=NULL;
    fs->cwd=fdb;
    fs->disk->fat[0]=0;
    return root;
}


void Initialize_file_blocks(int* file_blocks) {
    for(int i=0;i<BLOCK_SIZE/sizeof(int);i++) {
        file_blocks[i]=-1;
    }
}

//edoardo
FileHandle* fat32_createFile(DirectoryHandle* d, const char* filename) {
    if(d==NULL || filename==NULL){
        return 0;
    }
    int block_index=DiskDriver_getFreeBlock(d->f->disk,d->dcb->fcb.block_in_disk);
    printf("scriverò il blocco di questo nuovo file in pos %d\n",block_index);
    if(block_index==-1){
        return 0;
    }
    int entries=d->dcb->num_entries;
    int i=0,valid_entries=0;
    int max_entries=(BLOCK_SIZE-sizeof(FileControlBlock)-sizeof(int))/sizeof(int);
    FirstFileBlock* aux=(FirstFileBlock*)malloc(sizeof(FirstFileBlock));
    while(i<max_entries) {
        int index=d->dcb->file_blocks[i];
        if(index!=-1){
            DiskDriver_readBlock(d->f->disk,aux,index);
            if(!strcmp(aux->fcb.name,filename)) {
                free(aux);
                printf("File già esistente\n");
                return 0;

            }
        valid_entries++;
        }
        i++;
    }
    entries-=valid_entries;
    int* fat=d->f->disk->fat;
    int first_succ=fat[d->dcb->fcb.block_in_disk];
    DirectoryBlock* buf=(DirectoryBlock*)malloc(sizeof(DirectoryBlock));
    DiskDriver_readBlock(d->f->disk,buf,first_succ);
    while(entries>0) {
        i=0;
        while(i<BLOCK_SIZE/sizeof(int)){
            int index=buf->file_blocks[i];
            if(index!=-1) {
                DiskDriver_readBlock(d->f->disk,aux,index);
                if(!strcmp(aux->fcb.name,filename)) {
                    free(aux);
                    printf("File già esistente\n");
                    return 0;
                }
            valid_entries++;
            }
        }
        entries-=valid_entries;
    }
    free(aux);
    FileHandle* fh=(FileHandle*)malloc(sizeof(FileHandle));//inizializzo il file_handle
    ((ListItem*)fh)->next=0;
    ((ListItem*)fh)->prev=0;
    fh->f=d->f;
    fh->pos_in_file=0;
    fh->directory=d->dcb;//parent dir 
    fh->ffb=(FirstFileBlock*)malloc(BLOCK_SIZE);//inizializzo il first_file_block presente nel file_handle
    FirstFileBlock* ffb_copy = fh->ffb;
    ffb_copy->fcb.directory_block=d->dcb->fcb.block_in_disk;
    ffb_copy->fcb.block_in_disk=block_index;
    strcpy(ffb_copy->fcb.name,filename);
    ffb_copy->fcb.size=0;
    ffb_copy->fcb.is_dir=0;
    DiskDriver_writeBlock(d->f->disk,fh->ffb, block_index);
    d->f->disk->fat[block_index]=block_index; //è in uso il blocco(valid) ma senza successori
    //printf("la cartella %s ha %d entries %d\n",d->dcb->fcb.name,d->dcb->num_entries,max_dir_entries);
    if(d->dcb->num_entries<max_entries){
        for(int i=0;i<max_entries;i++){
            if(d->dcb->file_blocks[i]==-1){
                d->dcb->file_blocks[i]=block_index;
                break;
            }
        }
    }
    else{   //SE HO PIU ENTRIES DI QUANTE NE PUÒ CONTENERE IL PRIMO BLOCCO, HO SUCCESSORI OPPURE DEVO CREARLO
        int old_index=d->dcb->fcb.block_in_disk;
        int first_succ_index=fat[old_index];
        //int numero_blocchi_successori=(d->dcb->num_entries-first_size)/(BLOCK_SIZE/(sizeof(int)));
        if(first_succ_index!=old_index && first_succ_index!=-1){ //ho successori
            int flag;
            DirectoryBlock* dirBlock=(DirectoryBlock*)malloc(sizeof(BLOCK_SIZE));
            while(first_succ_index!=old_index){ //scorro i successori
                DiskDriver_readBlock(d->f->disk,dirBlock,first_succ_index);
                flag=1; //se ho spazio nel blocco successore
                for(int z=0;z<BLOCK_SIZE/sizeof(int);z++){
                    if(dirBlock->file_blocks[z]==-1){
                        dirBlock->file_blocks[z]=block_index;
                        flag=0;
                        DiskDriver_writeBlock(d->f->disk, dirBlock, first_succ_index);
                        free(dirBlock);
                        break;
                    }
            }
 
            old_index=first_succ_index;
            first_succ_index=fat[old_index];
            }
            if(flag){ //erano tutti occupati
                int index=DiskDriver_getFreeBlock(d->f->disk,d->dcb->fcb.block_in_disk);
                if(index==-1)
                    return 0;
                d->f->disk->fat[first_succ_index]=index; //successore del successore
                Initialize_file_blocks(dirBlock->file_blocks);
                dirBlock->file_blocks[0]=block_index;
                DiskDriver_writeBlock(d->f->disk, dirBlock, index);
                free(dirBlock);
            }
        }
        
        else{
            DirectoryBlock* dirBlock=(DirectoryBlock*)malloc(sizeof(BLOCK_SIZE));
            DiskDriver_readBlock(d->f->disk,dirBlock,first_succ_index);
            int idx=DiskDriver_getFreeBlock(d->f->disk,d->dcb->fcb.block_in_disk);
            if(idx==-1) return 0;
            d->f->disk->fat[d->dcb->fcb.block_in_disk]=idx;
            Initialize_file_blocks(dirBlock->file_blocks);
            dirBlock->file_blocks[0]=block_index;
            DiskDriver_writeBlock(d->f->disk, dirBlock, idx);
            free(dirBlock);

            }
        }

    d->dcb->num_entries++;
    DiskDriver_writeBlock(d->f->disk, d->dcb, d->dcb->fcb.block_in_disk);
    return fh;
        
}



//marco
int fat32_listDir(char** names, DirectoryHandle* d){
    int i=0;
    int color_bitmap=0;
    int entries=d->dcb->num_entries;
    int entries_of_first_block=(BLOCK_SIZE
		   -sizeof(FileControlBlock)
		    -sizeof(int))/sizeof(int);
    int entries_of_others=BLOCK_SIZE/sizeof(int);
    int valid_entries=0;
    FirstDirectoryBlock* buf=(FirstDirectoryBlock*)malloc(sizeof(FirstDirectoryBlock));
    int mask;
    while(entries_of_first_block){
        if(d->dcb->file_blocks[i]!=-1){
            DiskDriver_readBlock(d->f->disk,buf,d->dcb->file_blocks[i]);
            if(buf->fcb.is_dir) {
                mask=(1<<i);
                color_bitmap|=mask;
            }
            strcpy(names[valid_entries],buf->fcb.name);
            valid_entries++;
        }
        entries_of_first_block--;
        i++;
        
    }
    entries-=valid_entries;
    while(entries){ //finchè ce ne sono ancora negli altri blocchi
        int*fat=d->f->disk->fat;
        int succ=fat[d->dcb->fcb.block_in_disk];
        DirectoryBlock* buf3=(DirectoryBlock*)malloc(sizeof(DirectoryBlock));
        DiskDriver_readBlock(d->f->disk,buf3,succ);
        i=0;
        FirstDirectoryBlock* buf2=(FirstDirectoryBlock*)malloc(sizeof(FirstDirectoryBlock));
        while(entries_of_others){
            if(buf3->file_blocks[i]!=-1){
                DiskDriver_readBlock(d->f->disk,buf2,buf3->file_blocks[i]);
                if(buf->fcb.is_dir) {
                mask=(1<<i);
                color_bitmap|=mask;
                }
                strcpy(names[valid_entries],buf2->fcb.name);
                valid_entries++;
                entries--;
            }
            entries_of_others--;
            i++;
        }
        succ=fat[succ];
        free(buf2);
        free(buf3);
    }
    free(buf);
    return color_bitmap;
}

char** filename_alloc() {
    char**names=(char**)malloc(sizeof(char*)*100);
    for(int i=0;i<100;i++){
        names[i]=(char*)malloc(64);
    }
    return names;
}
void filename_dealloc(char** names) {
    for(int i=0;i<100;i++){
        free(names[i]);
    }
    free(names);
}

FileHandle* fat32_openFile(DirectoryHandle* d, const char* filename) {
    if(d==NULL || filename==NULL) return NULL;
    int entries=d->dcb->num_entries;
    int max_entries=(BLOCK_SIZE-sizeof(FileControlBlock)-sizeof(int))/sizeof(int);
    int i=0,valid_entries=0;
    FileHandle* fh;
    while(i<max_entries){
        int index=d->dcb->file_blocks[i];
        if(index!=-1) {
            FirstFileBlock* fdb=(FirstFileBlock*)malloc(sizeof(FirstFileBlock));
            DiskDriver_readBlock(d->f->disk,fdb,index);
            if(!strcmp(fdb->fcb.name,filename)) {
                fh=(FileHandle*)malloc(sizeof(FileHandle));
                fh->directory=d->dcb;
                fh->f=d->f;
                fh->ffb=fdb;
                fh->pos_in_file=0;
                ((ListItem*)fh)->next=0;
                ((ListItem*)fh)->prev=0;
                return fh;
            }
            valid_entries++;
        }
        i++;
    }
    entries-=valid_entries;
    while(entries>0) {
        int*fat=d->f->disk->fat;
        int succ=fat[d->dcb->fcb.block_in_disk];
        DirectoryBlock* buf=(DirectoryBlock*)malloc(sizeof(DirectoryBlock));
        DiskDriver_readBlock(d->f->disk,buf,succ);
        i=0;
        while(i<BLOCK_SIZE){
            int index=buf->file_blocks[i];
            if(index!=-1){
                FirstFileBlock* fdb=(FirstFileBlock*)malloc(sizeof(FirstFileBlock));
                DiskDriver_readBlock(d->f->disk,fdb,index);
                if(!(strcmp(fdb->fcb.name,filename))) {
                    fh=(FileHandle*)malloc(sizeof(FileHandle));
                    fh->directory=d->dcb;
                    fh->f=d->f;
                    fh->ffb=fdb;
                    fh->pos_in_file=0;
                    ((ListItem*)fh)->next=0;
                    ((ListItem*)fh)->prev=0;
                    return fh;
                }
                valid_entries++;
                entries--;
            }
            i++;
        }
        succ=fat[succ];
        free(buf);
    }
    printf("File %s non presente nella cwd\n",filename);
    return NULL;
}



int fat32_close(FileHandle* f) {
    if(f==NULL) {
        printf("FileHandle inesistente");
        return -1;
    }
    //free(f->directory);
    free(f->ffb);
    free(f);
    return 0;
}


//marco
int fat32_write(FileHandle* f, void* data, int size){
    int bytes_written=0;
    int added_size=0;
    int data_first_space=BLOCK_SIZE-sizeof(FileControlBlock);
    int s=f->pos_in_file+size;
    if(f->ffb->fcb.size<s) {
        printf("fcb size: %d\n",f->ffb->fcb.size);
        added_size+=s-f->ffb->fcb.size;
        f->ffb->fcb.size+=added_size;
        printf("Added_size: %d\n",added_size);
    }
    if(f->pos_in_file<=BLOCK_SIZE-sizeof(FileControlBlock)){ //se il cursore sta nel primo blocco
        if(size<=data_first_space-f->pos_in_file){ //se c'è ancora spazio nel primo blocco dalla posizione in cui sono
            FirstFileBlock* first=f->ffb;
            memcpy(first->data+f->pos_in_file,data,size);
            DiskDriver_writeBlock(f->f->disk,first,f->ffb->fcb.block_in_disk);
            bytes_written+=size;
        }
        else{ //scrivo un po' nel primo ed il resto nei successori
            printf("stiamo nel primo blocco e dobbiamo scrivere un po qua un po dopo\n");
             FirstFileBlock* first=f->ffb;
             int size_to_write=data_first_space-f->pos_in_file; //size da scrivere nel primo
             memcpy(first->data+f->pos_in_file,data,size_to_write);
             DiskDriver_writeBlock(f->f->disk,first,f->ffb->fcb.block_in_disk);
             bytes_written+=size_to_write;
             int num_succ=ceil((double)(size-size_to_write)/(double)BLOCK_SIZE); //blocchi da scrivere
             printf("scriverò su %d blocchi oltre al primo\n",num_succ);
             int old_index=f->ffb->fcb.block_in_disk;
             int index=f->f->disk->fat[old_index]; //prendo successore dalla fat
             int blocchi_scritti=0;
             while(index!=-1 && index!=old_index){ //esistono i successori
                //printf("%d %d\n",index,f->f->disk->fat[index]);
                FileBlock* block=(FileBlock*)malloc(sizeof(FileBlock));
                if(index==f->f->disk->fat[index] && blocchi_scritti+1==num_succ){ //ultimo blocco del file fin'ora
                    int last_size=size-bytes_written;
                    DiskDriver_readBlock(f->f->disk,block,index);
                    memcpy(block->data,data+size_to_write+blocchi_scritti*BLOCK_SIZE,last_size);
                    bytes_written+=last_size;
                }
                else if(index==f->f->disk->fat[index] && blocchi_scritti+1<num_succ){ //non è l'ultimo da scrivere
                    DiskDriver_readBlock(f->f->disk,block,index);
                    memcpy(block->data,data+size_to_write+blocchi_scritti*BLOCK_SIZE,BLOCK_SIZE);
                    bytes_written+=BLOCK_SIZE;
                    
                }
                else{
                    DiskDriver_readBlock(f->f->disk,block,index);
                    memcpy(block->data,data+size_to_write+blocchi_scritti*BLOCK_SIZE,BLOCK_SIZE);
                    bytes_written+=BLOCK_SIZE;
                }
                    
                    
                    DiskDriver_writeBlock(f->f->disk,block,index);
                    free(block);
                    blocchi_scritti++;
                    //printf("per ora ho scritto su %d blocchi che erano già presenti\n",blocchi_scritti);
                    old_index=index;
                    index=f->f->disk->fat[old_index];
                


                }

            
             
            int rimasti=num_succ-blocchi_scritti;
            
            if(rimasti>0){
                int index;
                
                
                for(int i=0;i<rimasti;i++){
                    index=DiskDriver_getFreeBlock(f->f->disk,old_index);
                    f->f->disk->fat[old_index]=index;
                    FileBlock* block=(FileBlock*)malloc(sizeof(FileBlock));
                    if(i!=rimasti-1){ //SE NON è L'ULTIMO CHE MI SERVE SCRIVO TUTTO IL BLOCCO
                        memcpy(block->data,data+size_to_write+blocchi_scritti*BLOCK_SIZE+i*BLOCK_SIZE,BLOCK_SIZE);
                        DiskDriver_writeBlock(f->f->disk,block,index);
                        bytes_written+=BLOCK_SIZE;
                       
                        //printf("ora la size è %d\n",f->ffb->fcb.size);
                        }
                    else{ //ultimo
                        printf("scrivo sull'ultimo blocco allocato\n");
                        int last_size=size-bytes_written;
                        memcpy(block->data,data+size_to_write+blocchi_scritti*BLOCK_SIZE+i*BLOCK_SIZE,last_size);
                        DiskDriver_writeBlock(f->f->disk,block,index);
                        bytes_written+=last_size;
                            
                        //printf("ora la size è %d\n",f->ffb->fcb.size);
                    }
                        printf("ho dovuto allocare fin'ora %d blocchi\n",i+1);
                        free(block);
                        old_index=index;
                        f->f->disk->fat[old_index]=index;
                        
                        

                    }
            }
        }
            
    }
    else{
        
        printf("il cursore non è nel primo blocco\n");
        int blocchi_da_scorrere=ceil((double)(f->pos_in_file-(BLOCK_SIZE-sizeof(FileControlBlock)))/(double)BLOCK_SIZE);
        int old_index=f->ffb->fcb.block_in_disk;
        int index=f->f->disk->fat[old_index]; 
        if(blocchi_da_scorrere>0) blocchi_da_scorrere--;
        while(blocchi_da_scorrere){ 
            old_index=index;
            index=f->f->disk->fat[old_index]; 
            blocchi_da_scorrere--;
        }
        int offset=(f->pos_in_file-(BLOCK_SIZE-sizeof(FileControlBlock)))%BLOCK_SIZE;
        printf("offset %d\n",offset);
        int size_to_write=BLOCK_SIZE-offset;
        if(size<=size_to_write){
            FileBlock* block=(FileBlock*)malloc(sizeof(FileBlock));
            memcpy(block->data+offset,data,size);
            bytes_written+=size;
            DiskDriver_writeBlock(f->f->disk,block,index);
            free(block);
            
        }
        else{
            printf("scrivo un po' nel blocco corrente e il resto nei successori\n");
            FileBlock* block=(FileBlock*)malloc(sizeof(FileBlock));
            DiskDriver_readBlock(f->f->disk,block,index);
            memcpy(block->data+offset,data,size_to_write);
            bytes_written+=size_to_write;
            DiskDriver_writeBlock(f->f->disk,block,index);
            free(block);
            int num_succ=ceil((double)(size-size_to_write)/(double)BLOCK_SIZE); //blocchi da scrivere
            printf("scriverò su %d blocchi\n",num_succ);
             int blocchi_scritti=0;
             old_index=index;
             index=f->f->disk->fat[index];
             if(index!=old_index){ //ce ne sono altri
                while(index!=-1 && index!=old_index){ 
                    FileBlock* block=(FileBlock*)malloc(sizeof(FileBlock));
                    if(index==f->f->disk->fat[index] && blocchi_scritti+1==num_succ){ //ultimo blocco del file fin'ora
                        int last_size=size-bytes_written;
                        DiskDriver_readBlock(f->f->disk,block,index);
                        memcpy(block->data,data+size_to_write+blocchi_scritti*BLOCK_SIZE,last_size);
                        bytes_written+=last_size;
                    }
                    else if(index==f->f->disk->fat[index] && blocchi_scritti+1<num_succ){ //non è l'ultimo da scrivere
                        DiskDriver_readBlock(f->f->disk,block,index);
                        memcpy(block->data,data+size_to_write+blocchi_scritti*BLOCK_SIZE,BLOCK_SIZE);
                        bytes_written+=BLOCK_SIZE;

                    }
                    else{
                        DiskDriver_readBlock(f->f->disk,block,index);
                        memcpy(block->data,data+size_to_write+blocchi_scritti*BLOCK_SIZE,BLOCK_SIZE);
                        bytes_written+=BLOCK_SIZE;
                    }
                        
                        DiskDriver_writeBlock(f->f->disk,block,index);
                        free(block);
                        blocchi_scritti++;
                        //printf("per ora ho scritto su %d blocchi che erano già presenti",blocchi_scritti);
                        old_index=index;
                        index=f->f->disk->fat[old_index];


                    }
             }


            old_index=index;
            int rimasti=num_succ-blocchi_scritti;
            printf("devo allocare ancora %d blocchi\n",rimasti);
            if(rimasti>0){  
                for(int i=0;i<rimasti;i++){
                    index=DiskDriver_getFreeBlock(f->f->disk,old_index);
                    //printf("%d %d\n",old_index,index);
                    f->f->disk->fat[old_index]=index;
                    FileBlock* block=(FileBlock*)malloc(sizeof(FileBlock));
                    if(i!=rimasti-1){ //SE NON è L'ULTIMO CHE MI SERVE SCRIVO TUTTO IL BLOCCO
                        memcpy(block->data,data+size_to_write+blocchi_scritti*BLOCK_SIZE+i*BLOCK_SIZE,BLOCK_SIZE);
                        bytes_written+=BLOCK_SIZE;
                        }
                        else{ //ultimo
                            int last_size=size-bytes_written;
                            memcpy(block->data,data+size_to_write+blocchi_scritti*BLOCK_SIZE+i*BLOCK_SIZE,last_size);
                            bytes_written+=last_size;
                        }
                        DiskDriver_writeBlock(f->f->disk,block,index);
                        free(block);
                        old_index=index;
                        f->f->disk->fat[old_index]=index;
                        
                        
                            

                    }
            }
        }

    }
    f->pos_in_file+=bytes_written;
    DirectoryHandle* fakedir=(DirectoryHandle*)malloc(sizeof(DirectoryHandle));
    fakedir->dcb=f->f->cwd;
    fakedir->directory=NULL;
    fakedir->f=f->f;
    
    if(strcmp(f->f->cwd->fcb.name,"/\0")){
        fakedir->directory=(FirstDirectoryBlock*)malloc(sizeof(FirstDirectoryBlock));
        DiskDriver_readBlock(f->f->disk,fakedir->directory,f->f->cwd->fcb.directory_block);
    }
    fat32_update_size(fakedir,added_size);
    free(fakedir);
    return bytes_written;
}

//edoardo
int fat32_read(FileHandle* f, void* data, int size) {
    size=(size<=f->ffb->fcb.size)? size : f->ffb->fcb.size;
    int bytes_read=0;
    int* fat=f->f->disk->fat;
    int bytes_left_within_block;
    if(f->pos_in_file<=BLOCK_SIZE-sizeof(FileControlBlock)) { //siamo col cursore nel primo
        if(f->pos_in_file+size<=BLOCK_SIZE-sizeof(FileControlBlock)) {
            if(size+f->pos_in_file<=f->ffb->fcb.size) {
                memcpy(data,f->ffb->data+f->pos_in_file,size);
                bytes_read+=size;
            }
            else {
                int left=f->ffb->fcb.size-f->pos_in_file;
                memcpy(data,f->ffb->data+f->pos_in_file,left);
                bytes_read+=left;
            }
        }   
        else {
            int pos_offset=f->pos_in_file;
            int final_offset=(f->ffb->fcb.size-(BLOCK_SIZE-sizeof(FileControlBlock)))%BLOCK_SIZE;
            //printf("final_offset: %d\n",final_offset);
            int old_index=f->ffb->fcb.block_in_disk;
            int first_succ=fat[f->ffb->fcb.block_in_disk];
            
            if(first_succ==f->ffb->fcb.block_in_disk){ //non ha successori
                int diff=f->ffb->fcb.size-pos_offset;
                memcpy(data,f->ffb->data+f->pos_in_file,diff);
                bytes_read+=diff;
                printf("Lettura oltre EOF\n");
                
            }
            else{ //leggiamo il resto del primo e andiamo a leggere i successori
                printf("sto qua\n");
                bytes_left_within_block = BLOCK_SIZE-sizeof(FileControlBlock)-f->pos_in_file;
                int num_fileblocks=ceil((double)((size-bytes_left_within_block))/BLOCK_SIZE);
                printf("num_fileblocks %d\n",num_fileblocks);
                //leggo i rimanenti bytes del ffb
                memcpy(data,f->ffb->data+f->pos_in_file,bytes_left_within_block);
                //num_fileblocks--;
                bytes_read+=bytes_left_within_block;
                //printf("ho letto nel primo blocco%d\n",bytes_read);
                //printf("%d %d\n",first_succ,fat[first_succ]);
                
                
                //buffer di ausilio 
                char buff[BLOCK_SIZE];
                //leggo prima i fileblocks interi
                //num_fileblocks--;
                printf("prima del while fileblocks %d\n",num_fileblocks);
                while(num_fileblocks>1 && first_succ!=old_index) {
                    DiskDriver_readBlock(f->f->disk,data+bytes_read,first_succ);
                    bytes_read+=BLOCK_SIZE;
                    old_index=first_succ;
                    first_succ=fat[old_index];
                    num_fileblocks--;
                }
                //leggo i bytes rimanenti dell'ultimo blocco 
                DiskDriver_readBlock(f->f->disk,buff,first_succ);
                
                if(first_succ==fat[first_succ] && size-bytes_read<=final_offset ){ //siamo ancora dentro il file nell'ultimo blocco
                    memcpy(data+bytes_read,buff,size-bytes_read);
                    bytes_read+=size-bytes_read;
                }
                else if(first_succ==fat[first_succ] && size-bytes_read>final_offset){ //arriviamo a EOF
                    memcpy(data+bytes_read,buff,final_offset); //leggo tutto ciò che posso
                    bytes_read+=final_offset;
                    printf("lettura oltre EOF\n");
                }
                else{
                    memcpy(data+bytes_read,buff,size-bytes_read);
                    bytes_read+=size-bytes_read;
                }
                
               
                
            }
        }
    }
    else {
        int current_block=ceil((double)(f->pos_in_file-(BLOCK_SIZE-sizeof(FileControlBlock)))/(double)BLOCK_SIZE);
        //printf("da scorrere per arrivare al current %d\n",current_block);
        int current_block_index=fat[f->ffb->fcb.block_in_disk];
        char buff[BLOCK_SIZE];
        
        int pos_offset=(f->pos_in_file-(BLOCK_SIZE-sizeof(FileControlBlock)))%BLOCK_SIZE;
        
        int final_offset=(f->ffb->fcb.size-(BLOCK_SIZE-sizeof(FileControlBlock)))%BLOCK_SIZE;
        //printf("final offset=%d\n",final_offset);

        if(current_block>0) current_block--;
        while(current_block>0) {
            current_block_index=fat[current_block_index];
            current_block--;
        }
        DiskDriver_readBlock(f->f->disk,buff,current_block_index);


        if(pos_offset+size<=BLOCK_SIZE && f->pos_in_file+size<f->ffb->fcb.size) {
            memcpy(data,buff,size);
            bytes_read+=size;
        }
        else { //non entra tutto nel blocco il messaggio
           // printf("current=%d e first_succ=%d\n",current_block_index,fat[current_block_index]);
            //leggo i rimanenti bytes del current_block
            if(fat[current_block_index]==current_block_index){ //non ha successori
                int diff=final_offset-pos_offset;
                memcpy(data,buff+pos_offset,diff);
                bytes_read+=diff;
                printf("Lettura oltre EOF\n");
            }
            else { //ha successori, quindi leggo tutto questo blocco e vado avanti
                int num_fileblocks=ceil((size-bytes_read)/sizeof(FileBlock));
                bytes_left_within_block=BLOCK_SIZE-pos_offset; //qua offset rappresenta pos in file
                memcpy(data,buff+pos_offset,bytes_left_within_block);
                bytes_read+=bytes_left_within_block;
                //calcolo quanti  fileblocks dovrò leggere
                num_fileblocks--;
                //leggo prima i fileblocks interi
                int first_succ=fat[current_block_index];
                while(num_fileblocks>1 && first_succ!=fat[first_succ]) {
                    DiskDriver_readBlock(f->f->disk,data+bytes_read,first_succ);
                    bytes_read+=BLOCK_SIZE;
                    first_succ=fat[first_succ];
                    //printf("%d\n",first_succ);
                    num_fileblocks--;
                }
                //leggo i bytes rimanenti dell'ultimo blocco 
                DiskDriver_readBlock(f->f->disk,buff,first_succ);
                if(first_succ==fat[first_succ] && size-bytes_read<=final_offset ){ //siamo ancora dentro il file nell'ultimo blocco
                    memcpy(data+bytes_read,buff,size-bytes_read);
                    bytes_read+=size-bytes_read;
                }
                else if(first_succ==fat[first_succ] && size-bytes_read>final_offset){ //arriviamo a EOF
                    memcpy(data+bytes_read,buff,final_offset); //leggo tutto ciò che posso
                    bytes_read+=final_offset;
                    printf("lettura oltre EOF\n");
                }
                else{
                    memcpy(data+bytes_read,buff,size-bytes_read);
                    bytes_read+=size-bytes_read;
                }
                    
            }
            
        }

    }
    
    f->pos_in_file+=bytes_read;
    return bytes_read;
}
//recursively updates size of directories when a write operation is performed
//edoardo
void fat32_update_size(DirectoryHandle* d,int num) {
    if(!(strcmp(d->dcb->fcb.name,"/\0"))) {
        //printf("aggiorno di %d\n",num);
        //printf("prima è %d\n",d->dcb->fcb.size);
        d->dcb->fcb.size+=num;
        DiskDriver_writeBlock(d->f->disk,d->dcb,d->dcb->fcb.block_in_disk);
        //printf("ora è %d\n",d->dcb->fcb.size);
        return;
    }
    d->dcb->fcb.size+=num;
    DiskDriver_writeBlock(d->f->disk,d->dcb,d->dcb->fcb.block_in_disk);
    FirstDirectoryBlock* aux=(FirstDirectoryBlock*)malloc(sizeof(FirstDirectoryBlock));
    DiskDriver_readBlock(d->f->disk,aux,d->directory->fcb.directory_block);
    d->dcb=d->directory;
    d->directory=aux;
    fat32_update_size(d,num);
    return;
}

int fat32_seek(FileHandle* f, int pos){
    if(pos<f->ffb->fcb.size){
        f->pos_in_file=pos;
        return pos;
    }
    return -1;
}


//marco
int fat32_changeDir(DirectoryHandle* d, char* dirname){
    if(!strcmp(dirname,"..")){
        if(!strcmp(d->dcb->fcb.name,"/\0")) return 0;
        if(d->directory->fcb.block_in_disk==0){
             DiskDriver_readBlock(d->f->disk,d->dcb,0);
             d->directory=NULL;
        }
        else{
            FirstDirectoryBlock* aux=(FirstDirectoryBlock*)malloc(sizeof(FirstDirectoryBlock));
            DiskDriver_readBlock(d->f->disk,aux,d->directory->fcb.directory_block);
            FirstDirectoryBlock*temp=d->dcb;
            d->dcb=d->directory;
            d->directory=aux;
            free(temp);
        }
        
        
        
    
        d->f->cwd=d->dcb;
        return 0;
    }
    else{
        int entries_of_first_block=(BLOCK_SIZE
		   -sizeof(FileControlBlock)
		    -sizeof(int))/sizeof(int);
        int entries_of_others=BLOCK_SIZE/sizeof(int);
        FirstDirectoryBlock* aux2=(FirstDirectoryBlock*)malloc(sizeof(FirstDirectoryBlock));
        for(int i=0;i<entries_of_first_block;i++){
            int pos=d->dcb->file_blocks[i];
            if(pos!=-1){
                DiskDriver_readBlock(d->f->disk,aux2,pos);
                if(!strcmp(aux2->fcb.name,dirname)) {
                    FirstDirectoryBlock*temp=d->directory;
                    d->directory=d->dcb;
                    d->dcb=aux2;
                    free(temp);
                    d->f->cwd=d->dcb;
                    return 0;
                }
            }
        }
        int*fat=d->f->disk->fat;
        int old_index=fat[d->dcb->fcb.block_in_disk];
        int first_succ=fat[old_index];
        FirstDirectoryBlock* buf=(FirstDirectoryBlock*)malloc(sizeof(FirstDirectoryBlock));
        while(first_succ!=old_index){
            DiskDriver_readBlock(d->f->disk,buf,first_succ);
            for(int j=0;j<entries_of_others;j++){
                int pos2=buf->file_blocks[j];
                if(pos2!=-1){
                    DiskDriver_readBlock(d->f->disk,aux2,pos2);
                    if(!strcmp(aux2->fcb.name,dirname)) {
                        FirstDirectoryBlock*temp=d->directory;
                        d->directory=d->dcb;
                        d->dcb=aux2;
                        free(temp);
                        d->f->cwd=d->dcb;
                        return 0;
                    }
                }
            }
        }
        old_index=first_succ;
        first_succ=fat[old_index];
    }
    return -1;
}


//marco
int fat32_mkDir(DirectoryHandle* d, char* dirname){
    //aggiornare le info della directory corrente
    //deve allocare blocco per quella nuova
    if(d==NULL || dirname==NULL){
        return -1;
    }
    int first_size=(BLOCK_SIZE
		   -sizeof(FileControlBlock)
		    -sizeof(int))/sizeof(int);
    
    int entries=d->dcb->num_entries;
    int i=0,valid_entries=0;
    FirstFileBlock* aux=(FirstFileBlock*)malloc(sizeof(FirstFileBlock));
    while(i<first_size) {
        int index=d->dcb->file_blocks[i];
        if(index!=-1){
            DiskDriver_readBlock(d->f->disk,aux,index);
            if(!strcmp(aux->fcb.name,dirname)) {
                free(aux);
                printf("Directory gia esistente\n");
                return 0;

            }
            valid_entries++;
        }
        i++;
    }
    entries-=valid_entries;
    int* fat=d->f->disk->fat;
    int first_succ=fat[d->dcb->fcb.block_in_disk];
    DirectoryBlock* buf=(DirectoryBlock*)malloc(sizeof(DirectoryBlock));
    DiskDriver_readBlock(d->f->disk,buf,first_succ);
    while(entries>0) {
        i=0;
        while(i<BLOCK_SIZE/sizeof(int)){
            int index=buf->file_blocks[i];
            if(index!=-1) {
                DiskDriver_readBlock(d->f->disk,aux,index);
                if(!strcmp(aux->fcb.name,dirname)) {
                    free(aux);
                    printf("Directory già esistente\n");
                    return 0;
                }
            valid_entries++;
            }
        }
        entries-=valid_entries;
    }
    free(aux);
    int block_index=DiskDriver_getFreeBlock(d->f->disk,d->dcb->fcb.block_in_disk);

    //printf("scriverò il blocco di questa nuova cartella in pos %d\n",block_index);
    if(block_index==-1){
        return -1;
    }
    
   
    
    //QUESTA PORZIONE CREA LA NUOVA CARTELLA
    FirstDirectoryBlock* fdb=(FirstDirectoryBlock*)malloc(sizeof(FirstDirectoryBlock));
    fdb->num_entries=0;
    fdb->fcb.directory_block=d->dcb->fcb.block_in_disk;
    fdb->fcb.block_in_disk=block_index;
    strcpy(fdb->fcb.name,dirname);
    fdb->fcb.size=0;
    fdb->fcb.is_dir=1;
     for(int i=0; i<first_size;i++){
        fdb->file_blocks[i]=-1;
    }
    DiskDriver_writeBlock(d->f->disk, fdb, block_index);
    free(fdb);
    d->f->disk->fat[block_index]=block_index; //se la fat ha stesso numero dell'indice è in uso il blocco(valid) ma senza successori
        
    if(d->dcb->num_entries<first_size){
        for(int i=0;i<first_size;i++){
            if(d->dcb->file_blocks[i]==-1){
                d->dcb->file_blocks[i]=block_index;
                break;
            }
        }
    }
    else{   //SE HO PIU ENTRIES DI QUANTE NE PUò CONTENERE IL PRIMO BLOCCO, HO SUCCESSORI OPPURE DEVO CREARLO
        int* fat=d->f->disk->fat;
        int old_index=d->dcb->fcb.block_in_disk;
        int first_succ_index=fat[old_index];
        //int numero_blocchi_successori=(d->dcb->num_entries-first_size)/(BLOCK_SIZE/(sizeof(int)));
        if(first_succ_index!=old_index && first_succ_index!=-1){
            int flag;
            DirectoryBlock* dirBlock=(DirectoryBlock*)malloc(sizeof(BLOCK_SIZE));
            while(first_succ_index!=old_index){ //scorro i successori
                DiskDriver_readBlock(d->f->disk,dirBlock,first_succ_index);
                flag=1; //se ho spazio nel blocco successore
                for(int z=0;z<BLOCK_SIZE/sizeof(int);z++){
                    if(dirBlock->file_blocks[z]==-1){
                        dirBlock->file_blocks[z]=block_index;
                        flag=0;
                        DiskDriver_writeBlock(d->f->disk, dirBlock, first_succ_index);
                        free(dirBlock);
                        break;
                    }
            } 
            old_index=first_succ_index;
            first_succ_index=fat[old_index];
            }
            if(flag){ //erano tutti occupati
                int index=DiskDriver_getFreeBlock(d->f->disk,d->dcb->fcb.block_in_disk);
                if(index==-1)
                    return 0;
                d->f->disk->fat[first_succ_index]=index; //successore del successore
                Initialize_file_blocks(dirBlock->file_blocks);
                dirBlock->file_blocks[0]=block_index;
                DiskDriver_writeBlock(d->f->disk, dirBlock, index);
            }
        }
        
        else{
            DirectoryBlock* dirBlock=(DirectoryBlock*)malloc(sizeof(BLOCK_SIZE));
            DiskDriver_readBlock(d->f->disk,dirBlock,first_succ_index);
            int idx=DiskDriver_getFreeBlock(d->f->disk,d->dcb->fcb.block_in_disk);
            if(idx==-1) return 0;
            d->f->disk->fat[d->dcb->fcb.block_in_disk]=idx;
            Initialize_file_blocks(dirBlock->file_blocks);
            dirBlock->file_blocks[0]=block_index;
            DiskDriver_writeBlock(d->f->disk, dirBlock, idx);
            free(dirBlock);

            }
        }
    d->dcb->num_entries++;
    DiskDriver_writeBlock(d->f->disk, d->dcb, d->dcb->fcb.block_in_disk);
   
    return 0;


}
int freeFile(fat32*f,int index){
    if(index==-1) return -1;
    if(f->disk->fat[index]==index){
        printf("ho eliminato il blocco %d\n",index);
        return DiskDriver_freeBlock(f->disk,index);
    }
    int new_index=f->disk->fat[index];
    freeFile(f,new_index);
    printf("ho eliminato il blocco %d\n",index);
    return DiskDriver_freeBlock(f->disk,index);
 
}
int free_dir(DirectoryHandle* d) {
    int max_entries=(BLOCK_SIZE-sizeof(FileControlBlock)-sizeof(int))/sizeof(int);
    int i=0;
    int* fat=d->f->disk->fat;
    if(d->dcb->num_entries==0)  {
        freeFile(d->f,d->dcb->fcb.block_in_disk);
        return 0;
    }
    while(d->dcb->num_entries>0 && i<max_entries){
            int index=d->dcb->file_blocks[i];
            if(index!=-1) {
                //leggo il ffb per vedere se è una directory
                FirstFileBlock* ffb=(FirstFileBlock*)malloc(sizeof(FirstFileBlock));
                DiskDriver_readBlock(d->f->disk,ffb,index);
                if(ffb->fcb.is_dir) {
                    printf("entro nell'if\n");
                    DirectoryHandle* dh=(DirectoryHandle*)malloc(sizeof(DirectoryHandle));
                    dh->f=d->f;
                    dh->dcb=(FirstDirectoryBlock*)ffb;
                    dh->directory=d->dcb;
                    free_dir(dh);
                    d->dcb->file_blocks[i]=-1;
                    d->dcb->num_entries--;
                    DiskDriver_writeBlock(d->f->disk,d->dcb,d->dcb->fcb.block_in_disk);
                    printf("sono in %s e ho num_entries: %d\n",d->dcb->fcb.name,d->dcb->num_entries);
                    free(dh);
                }
                else{    
                freeFile(d->f,index);
                d->dcb->file_blocks[i]=-1;
                d->dcb->num_entries--;
                DiskDriver_writeBlock(d->f->disk,d->dcb,d->dcb->fcb.block_in_disk);
                printf("sono in %s e ho num_entries: %d\n",d->dcb->fcb.name,d->dcb->num_entries);
                }
            }
             i++;
            //printf("sto iterando e i vale: %d\n",i);
    }
    
    if(d->dcb->num_entries>0) {
        i=0;
        int first_succ=fat[d->dcb->fcb.block_in_disk];
        DirectoryBlock* buf=(DirectoryBlock*)malloc(sizeof(DirectoryBlock));
        DiskDriver_readBlock(d->f->disk,buf,first_succ);
        while(d->dcb->num_entries>0) {
            while(i<BLOCK_SIZE/sizeof(int)){
                    int index=buf->file_blocks[i];
                    if(index!=-1) {
                        //leggo il ffb per vedere se è una directory
                        FirstFileBlock* ffb=(FirstFileBlock*)malloc(sizeof(FirstFileBlock));
                        DiskDriver_readBlock(d->f->disk,ffb,index);
                        if(ffb->fcb.is_dir) {
                            DirectoryHandle* dh=(DirectoryHandle*)malloc(sizeof(DirectoryHandle));
                            dh->f=d->f;
                            dh->dcb=(FirstDirectoryBlock*)ffb;
                            dh->directory=d->dcb;
                            free_dir(dh);
                            buf->file_blocks[i]=-1;
                            d->dcb->num_entries--;
                            DiskDriver_writeBlock(d->f->disk,d->dcb,d->dcb->fcb.block_in_disk);
                            DiskDriver_writeBlock(d->f->disk,buf,first_succ);
                            free(dh);
                            return 0;
                        }
                        else {
                            freeFile(d->f,index);
                            buf->file_blocks[i]=-1;
                            d->dcb->num_entries--;
                            DiskDriver_writeBlock(d->f->disk,d->dcb,d->dcb->fcb.block_in_disk);
                            DiskDriver_writeBlock(d->f->disk,buf,first_succ);
                            return 0;
                        }
                    }
                }
                i++;
            }
            first_succ=fat[first_succ];
    }
    freeFile(d->f,d->dcb->fcb.block_in_disk);
    return 0;
}

int fat32_remove(DirectoryHandle* d, char* filename) {
    if(d==NULL) return -1;
    printf("sono in %s e ho num_entries: %d\n",d->dcb->fcb.name,d->dcb->num_entries);
    int* fat=d->f->disk->fat;
    int max_entries=(BLOCK_SIZE-sizeof(FileControlBlock)-sizeof(int))/sizeof(int);
    int i=0;
    int entries=d->dcb->num_entries;
    int valid_entries=0;
    DirectoryHandle* fakedir=(DirectoryHandle*)malloc(sizeof(DirectoryHandle));
    fakedir->dcb=d->dcb;
    fakedir->directory=NULL;
    fakedir->f=d->f;
    if(strcmp(d->dcb->fcb.name,"/\0")){
        fakedir->directory=(FirstDirectoryBlock*)malloc(sizeof(FirstDirectoryBlock));
        DiskDriver_readBlock(d->f->disk,fakedir->directory,fakedir->dcb->fcb.directory_block);
    }
    int size_to_remove=0;
    while(i<max_entries){
        int index=d->dcb->file_blocks[i];
        if(index!=-1) {
            //printf("trovato e index=%d\n",index);
                //leggo il ffb per vedere se è una directory
            FirstFileBlock* ffb=(FirstFileBlock*)malloc(sizeof(FirstFileBlock));
            DiskDriver_readBlock(d->f->disk,ffb,index);
            if(!strcmp(ffb->fcb.name,filename)){
                if(ffb->fcb.is_dir) {
                    DirectoryHandle* dh=(DirectoryHandle*)malloc(sizeof(DirectoryHandle));
                    dh->f=d->f;
                    dh->dcb=(FirstDirectoryBlock*)ffb;
                    dh->directory=d->dcb;
                    size_to_remove-=ffb->fcb.size;
                    free_dir(dh);
                    d->dcb->file_blocks[i]=-1;
                    d->dcb->num_entries--;
                    DiskDriver_writeBlock(d->f->disk,d->dcb,d->dcb->fcb.block_in_disk);
                    free(dh);
                    fat32_update_size(fakedir,size_to_remove);
                    free(fakedir);
                    free(ffb);
                    return 0;
                }
                else {
                    freeFile(d->f,index);
                    d->dcb->file_blocks[i]=-1;
                    d->dcb->num_entries--;
                    DiskDriver_writeBlock(d->f->disk,d->dcb,d->dcb->fcb.block_in_disk);
                    size_to_remove-=ffb->fcb.size;
                    fat32_update_size(fakedir,size_to_remove);
                    free(fakedir);
                    free(ffb);
                    return 0;
                }
            }
            valid_entries++;   
        }
        i++;
    }
    entries-=valid_entries;
       
    //se ho ancora entries scorro la fat rimuovendo i Directory_Blocks
    if(entries>0) {
        i=0;
        int first_succ=fat[d->dcb->fcb.block_in_disk];
        DirectoryBlock* buf=(DirectoryBlock*)malloc(sizeof(DirectoryBlock));
        DiskDriver_readBlock(d->f->disk,buf,first_succ);
        while(entries>0) {
            while(i<BLOCK_SIZE/sizeof(int)){
                int index=buf->file_blocks[i];
                if(index!=-1) {
                    FirstFileBlock* ffb=(FirstFileBlock*)malloc(sizeof(FirstFileBlock));
                    DiskDriver_readBlock(d->f->disk,ffb,index);
                    if(!strcmp(ffb->fcb.name,filename)) {
                        if(ffb->fcb.is_dir) {
                            DirectoryHandle* dh=(DirectoryHandle*)malloc(sizeof(DirectoryHandle));
                            dh->f=d->f;
                            dh->dcb=(FirstDirectoryBlock*)ffb;
                            dh->directory=d->dcb;
                            free_dir(dh);
                            buf->file_blocks[i]=-1;
                            d->dcb->num_entries--;
                            DiskDriver_writeBlock(d->f->disk,d->dcb,d->dcb->fcb.block_in_disk);
                            DiskDriver_writeBlock(d->f->disk,buf,first_succ);
                            free(dh);
                            size_to_remove-=ffb->fcb.size;
                            fat32_update_size(fakedir,size_to_remove);
                            free(fakedir);
                            free(ffb);
                            return 0;
                        }
                        else {
                            freeFile(d->f,index);
                            buf->file_blocks[i]=-1;
                            d->dcb->num_entries--;
                            DiskDriver_writeBlock(d->f->disk,d->dcb,d->dcb->fcb.block_in_disk);
                            DiskDriver_writeBlock(d->f->disk,buf,first_succ);
                            size_to_remove-=ffb->fcb.size;
                            fat32_update_size(fakedir,size_to_remove);
                            free(fakedir);
                            free(ffb);
                            return 0;
                        }
                        valid_entries++;
                        entries--;
                    }
                }
                i++;
            }
            first_succ=fat[first_succ];
        }
        
       
    }
    printf("File %s non presente nella cwd\n",filename);
    return -1;
}


  