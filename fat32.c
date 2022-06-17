#include "fat32.h"
#include "string.h"
#include <stdio.h>
#include <math.h>


// initializes a file system on an already made disk
// returns a handle to the top level directory stored in the first block
//edo e marco
DirectoryHandle* fat32_init(fat32* fs, DiskDriver* disk){
    fs->disk=disk;
    FirstDirectoryBlock* fdb=(FirstDirectoryBlock*)malloc(sizeof(BLOCK_SIZE));
    fdb->num_entries=0;
    fdb->fcb.directory_block=0;
    fdb->fcb.block_in_disk=0;
    int size=(BLOCK_SIZE
		   -sizeof(FileControlBlock)
		    -sizeof(int))/sizeof(int);
    for(int i=0; i<size;i++){
        fdb->file_blocks[i]=-1;
    }
    strcpy(fdb->fcb.name,"/");
    fdb->fcb.size=0;
    fdb->fcb.is_dir=1;
    int ret=DiskDriver_writeBlock(fs->disk, fdb, 0);
    printf("write %d bytes for the root\n",ret);
    DirectoryHandle* root=(DirectoryHandle*)malloc(sizeof(DirectoryHandle));
    root->f=fs;
    root->dcb=fdb;
    root->directory=NULL;
    root->pos_in_dir=0;
    root->pos_in_block=0;
    fs->cwd=fdb;
    fs->disk->fat[0]=0;

    return root;
}

// creates the inital structures, the top level directory
// has name "/" and its control block is in the first position
// it also clears the bitmap of occupied blocks on the disk
// the current_directory_block is cached in the SimpleFS struct
// and set to the top level directory
void fat32_format(fat32* fs);

// creates an empty file in the directory d
// returns null on error (file existing, no free blocks)
// an empty file consists only of a block of type FirstBlock
//edoardo
FileHandle* fat32_createFile(DirectoryHandle* d, const char* filename) {
    if(d==NULL || filename==NULL){
        return 0;
    }
    int block_index=DiskDriver_getFreeBlock(d->f->disk,d->f->cwd->fcb.block_in_disk);
    printf("scriverò il blocco di questo nuovo file in pos %d\n",block_index);
    if(block_index==-1){
        return 0;
    }
    FileHandle* fh=(FileHandle*)malloc(sizeof(FileHandle));//inizializzo il file_handle
    fh->f=d->f;
    fh->pos_in_file=0;
    fh->directory=d->dcb;//parent dir 
    fh->ffb=(FirstFileBlock*)malloc(sizeof(BLOCK_SIZE));//inizializzo il first_file_block presente nel file_handle
    FirstFileBlock* ffb_copy = fh->ffb;
    ffb_copy->fcb.directory_block=d->dcb->fcb.block_in_disk;
    ffb_copy->fcb.block_in_disk=block_index;
    strcpy(ffb_copy->fcb.name,filename);
    ffb_copy->fcb.size=0;
    ffb_copy->fcb.is_dir=0;
    int ret=DiskDriver_writeBlock(d->f->disk,fh->ffb, block_index);
    printf("write %d bytes for the new file\n",ret);
    d->f->disk->fat[block_index]=block_index; //è in uso il blocco(valid) ma senza successori
    int max_dir_entries=(BLOCK_SIZE
		   -sizeof(FileControlBlock)
		    -sizeof(int))/sizeof(int);
    printf("la cartella %s ha %d entries %d\n",d->dcb->fcb.name,d->dcb->num_entries,max_dir_entries);
    if(d->dcb->num_entries<max_dir_entries){
        for(int i=0;i<max_dir_entries;i++){
            if(d->dcb->file_blocks[i]==-1){
                d->dcb->file_blocks[i]=block_index;
                break;
            }
        }
    }
    else{   //SE HO PIU ENTRIES DI QUANTE NE PUÒ CONTENERE IL PRIMO BLOCCO, HO SUCCESSORI OPPURE DEVO CREARLO
        int* fat=d->f->disk->fat;
        int first_succ_index=fat[d->dcb->fcb.block_in_disk];
        //int numero_blocchi_successori=(d->dcb->num_entries-first_size)/(BLOCK_SIZE/(sizeof(int)));
        if(first_succ_index!=fat[d->dcb->fcb.block_in_disk] && first_succ_index!=-1){
            while(first_succ_index!=fat[d->dcb->fcb.block_in_disk] && first_succ_index!=-1){
                first_succ_index=fat[first_succ_index];
            }
            DirectoryBlock* dirBlock=(DirectoryBlock*)malloc(sizeof(BLOCK_SIZE));
            int r=DiskDriver_readBlock(d->f->disk,dirBlock,first_succ_index);
            if(r==-1){
                return 0;
            }
            int flag=1; //se ho spazio nel blocco successore
            for(int z=0;z<BLOCK_SIZE/sizeof(int);z++){
                if(dirBlock->file_blocks[z]!=-1){
                    dirBlock->file_blocks[z]=block_index;
                    flag=0;
                    break;
                }
            }
            if(flag){
                //DirectoryBlock* dirBlock=(DirectoryBlock*)malloc(sizeof(BLOCK_SIZE));
                int index=DiskDriver_getFreeBlock(d->f->disk,d->dcb->fcb.block_in_disk);
                if(index==-1)
                    return 0;
                DiskDriver_writeBlock(d->f->disk, dirBlock, index);
                d->f->disk->fat[first_succ_index]=index; //successore del successore
                dirBlock->file_blocks[0]=block_index;
            }
        }
        else{
            DirectoryBlock* dirBlock=(DirectoryBlock*)malloc(sizeof(BLOCK_SIZE));
            int idx=DiskDriver_getFreeBlock(d->f->disk,d->dcb->fcb.block_in_disk);
            if(idx==-1)
                    return 0;
            DiskDriver_writeBlock(d->f->disk, dirBlock, idx);
            d->f->disk->fat[d->dcb->fcb.block_in_disk]=idx;
            dirBlock->file_blocks[0]=block_index;

            }
        }

    d->dcb->num_entries++;
    //AGGIORNARE LA SIZE DELLA CARTELLA E DEI GENITORI
    return fh;
        
}



// reads in the (preallocated) blocks array, the name of all files in a directory 
int fat32_listDir(char** names, DirectoryHandle* d);


// opens a file in the  directory d. The file should be exisiting
FileHandle* fat32_openFile(DirectoryHandle* d, const char* filename);


// closes a file handle (destroyes it)
int fat32_close(FileHandle* f);

// writes in the file, at current position for size bytes stored in data
// overwriting and allocating new space if necessary
// returns the number of bytes written
//marco
int fat32_write(FileHandle* f, void* data, int size){
    int bytes_written=0;
    int data_first_space=BLOCK_SIZE-sizeof(FileControlBlock);
    if(f->pos_in_file<BLOCK_SIZE-sizeof(FileControlBlock)){ //se il cursore sta nel primo blocco
        if(size<=data_first_space-f->pos_in_file){ //se c'è ancora spazio nel primo blocco dalla posizione in cui sono
            FirstFileBlock* first=f->ffb;
            memcpy(first->data+f->pos_in_file,data,size);
            DiskDriver_writeBlock(f->f->disk,first,f->ffb->fcb.block_in_disk);
            free(first);
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
             while(index!=-1 && index!=f->ffb->fcb.block_in_disk){ //esistono i successori
                printf("%d %d\n",index,f->f->disk->fat[index]);
                FileBlock* block=(FileBlock*)malloc(sizeof(BLOCK_SIZE));
                if(index==f->f->disk->fat[index] && blocchi_scritti+1==num_succ){ //ultimo blocco del file fin'ora
                    int last_size=size-bytes_written;
                    memcpy(block->data,data+size_to_write+blocchi_scritti*BLOCK_SIZE,last_size);
                    bytes_written+=last_size;
                    DiskDriver_writeBlock(f->f->disk,block,index);
                    free(block);
                    blocchi_scritti++;
                    printf("per ora ho scritto su %d blocchi che erano già presenti e ho finito\n",blocchi_scritti);
                    break;
                }
                else if(index==f->f->disk->fat[index] && blocchi_scritti+1<num_succ){ //non è l'ultimo da scrivere
                    memcpy(block->data,data+size_to_write+blocchi_scritti*BLOCK_SIZE,BLOCK_SIZE);
                    bytes_written+=BLOCK_SIZE;
                    DiskDriver_writeBlock(f->f->disk,block,index);
                    free(block);
                    blocchi_scritti++;
                    printf("per ora ho scritto su %d blocchi che erano già presenti e devo passare ad allocarne altri\n",blocchi_scritti);
                    break;
                }
                else{
                    memcpy(block->data,data+size_to_write+blocchi_scritti*BLOCK_SIZE,BLOCK_SIZE);
                    bytes_written+=BLOCK_SIZE;
                }
                    
                    DiskDriver_writeBlock(f->f->disk,block,index);
                    free(block);
                    blocchi_scritti++;
                    printf("per ora ho scritto su %d blocchi che erano già presenti\n",blocchi_scritti);
                    old_index=index;
                    index=f->f->disk->fat[old_index];
                


                }

            
             
            int rimasti=num_succ-blocchi_scritti;
            
            if(rimasti>0){
                int index=DiskDriver_getFreeBlock(f->f->disk,f->ffb->fcb.block_in_disk);
                
                
                for(int i=0;i<rimasti;i++){
                    f->f->disk->fat[old_index]=index;
                    FileBlock* block=(FileBlock*)malloc(sizeof(BLOCK_SIZE));
                    if(i!=rimasti-1){ //SE NON è L'ULTIMO CHE MI SERVE SCRIVO TUTTO IL BLOCCO
                        memcpy(block->data,data+size_to_write+i*BLOCK_SIZE,BLOCK_SIZE);
                        bytes_written+=BLOCK_SIZE;
                        f->ffb->fcb.size+=BLOCK_SIZE;
                        }
                        else{ //ultimo
                            int last_size=size-bytes_written;
                            memcpy(block->data,data+size_to_write+i*BLOCK_SIZE,last_size);
                            bytes_written+=last_size;
                            f->ffb->fcb.size+=last_size;
                        }
                        printf("ho dovuto allocare fin'ora %d blocchi\n",i+1);
                        DiskDriver_writeBlock(f->f->disk,block,index);
                        free(block);
                        old_index=index;
                        f->f->disk->fat[old_index]=index;
                        index=DiskDriver_getFreeBlock(f->f->disk,old_index);
                        

                    }
            }
        }
            
    }
    else{
        
        printf("il cursore non è nel primo blocco, alloco nuovi\n");
        int blocchi_da_scorrere=ceil((double)(f->pos_in_file-(BLOCK_SIZE-sizeof(FileControlBlock)))/(double)BLOCK_SIZE);
        int old_index=f->ffb->fcb.block_in_disk;
        int index=f->f->disk->fat[old_index]; 
        blocchi_da_scorrere--;
        while(blocchi_da_scorrere){
            old_index=index;
            index=f->f->disk->fat[old_index]; 
            blocchi_da_scorrere--;
        }
        int offset=f->pos_in_file%BLOCK_SIZE;
        int size_to_write=BLOCK_SIZE-offset;
        if(size<=size_to_write){
            FileBlock* block=(FileBlock*)malloc(sizeof(BLOCK_SIZE));
            memcpy(block->data+offset,data,size);
            bytes_written+=size;
            DiskDriver_writeBlock(f->f->disk,block,index);
            free(block);
        }
        else{
            FileBlock* block=(FileBlock*)malloc(sizeof(BLOCK_SIZE));
            memcpy(block->data,data,size_to_write);
            bytes_written+=size_to_write;
            DiskDriver_writeBlock(f->f->disk,block,index);
            free(block);
            int num_succ=ceil((double)(size-size_to_write)/(double)BLOCK_SIZE); //blocchi da scrivere
             printf("scriverò su %d blocchi\n",num_succ);
             int blocchi_scritti=0;
             if(index!=old_index){ //ce ne sono altri
                while(index!=-1){ 
                    FileBlock* block=(FileBlock*)malloc(sizeof(BLOCK_SIZE));
                    if(index==f->f->disk->fat[index] && blocchi_scritti+1==num_succ){ //ultimo blocco del file fin'ora
                        int last_size=size-bytes_written;
                        memcpy(block->data,data+size_to_write+blocchi_scritti*BLOCK_SIZE,last_size);
                        bytes_written+=last_size;
                        DiskDriver_writeBlock(f->f->disk,block,index);
                        free(block);
                        blocchi_scritti++;
                        printf("per ora ho scritto su %d blocchi che erano già presenti e ho finito\n",blocchi_scritti);
                        break;
                    }
                    else if(index==f->f->disk->fat[index] && blocchi_scritti+1<num_succ){ //non è l'ultimo da scrivere
                        memcpy(block->data,data+size_to_write+blocchi_scritti*BLOCK_SIZE,BLOCK_SIZE);
                        bytes_written+=BLOCK_SIZE;
                        DiskDriver_writeBlock(f->f->disk,block,index);
                        free(block);
                        blocchi_scritti++;
                        printf("per ora ho scritto su %d blocchi che erano già presenti e devo allocarne altri\n",blocchi_scritti);
                        break;

                    }
                    else{
                        memcpy(block->data,data+size_to_write+blocchi_scritti*BLOCK_SIZE,BLOCK_SIZE);
                        bytes_written+=BLOCK_SIZE;
                    }
                        
                        DiskDriver_writeBlock(f->f->disk,block,index);
                        free(block);
                        blocchi_scritti++;
                        printf("per ora ho scritto su %d blocchi che erano già presenti",blocchi_scritti);
                        old_index=index;
                        index=f->f->disk->fat[old_index];


                    }
             }


             
            int rimasti=num_succ-blocchi_scritti;
            printf("devo allocare ancora %d blocchi\n",rimasti);
            if(rimasti>0){
                int index=DiskDriver_getFreeBlock(f->f->disk,f->ffb->fcb.block_in_disk);
            
                
                for(int i=0;i<rimasti;i++){
                    f->f->disk->fat[old_index]=index;
                    FileBlock* block=(FileBlock*)malloc(sizeof(BLOCK_SIZE));
                    if(i!=rimasti-1){ //SE NON è L'ULTIMO CHE MI SERVE SCRIVO TUTTO IL BLOCCO
                        memcpy(block->data,data+size_to_write+i*BLOCK_SIZE,BLOCK_SIZE);
                        bytes_written+=BLOCK_SIZE;
                        f->ffb->fcb.size+=BLOCK_SIZE;
                        }
                        else{ //ultimo
                            int last_size=size-bytes_written;
                            memcpy(block->data,data+size_to_write+i*BLOCK_SIZE,last_size);
                            bytes_written+=last_size;
                            f->ffb->fcb.size+=last_size;
                        }
                        printf("ho dovuto allocare fin'ora %d blocchi\n",i+1);
                        DiskDriver_writeBlock(f->f->disk,block,index);
                        free(block);
                        old_index=index;
                        f->f->disk->fat[old_index]=index;
                        index=DiskDriver_getFreeBlock(f->f->disk,old_index);
                        
                            

                    }
            }
        }

    }
    f->pos_in_file+=bytes_written;
    return bytes_written;
}

// reads in the file, at current position size bytes stored in data
// returns the number of bytes read
int fat32_read(FileHandle* f, void* data, int size) {
    int bytes_read=0;
    if(f->pos_in_file<BLOCK_SIZE-sizeof(FileControlBlock)) {
        if(f->pos_in_file+size<BLOCK_SIZE-sizeof(FileControlBlock)) {
            memcpy(data,f->ffb->data+f->pos_in_file,size);
            bytes_read+=size;
        }
    }
    /*else {

    }*/
    return bytes_read;
}
// returns the number of bytes read (moving the current pointer to pos)
// returns pos on success
// -1 on error (file too short)
int fat32_seek(FileHandle* f, int pos);

// seeks for a directory in d. If dirname is equal to ".." it goes one level up
// 0 on success, negative value on error
// it does side effect on the provided handle
//marco
int fat32_changeDir(DirectoryHandle* d, char* dirname){
    if(!strcmp(dirname,"..")){
        FirstDirectoryBlock* aux=(FirstDirectoryBlock*)malloc(sizeof(BLOCK_SIZE));
        if(d->directory->fcb.block_in_disk==0){
             DiskDriver_readBlock(d->f->disk,aux,0);
             d->dcb=aux;
             d->directory=NULL;
        }
        else{
            DiskDriver_readBlock(d->f->disk,aux,d->directory->fcb.directory_block);
            d->dcb=d->directory;
            d->directory=aux;
        }
        
        
        
    
        d->f->cwd=d->dcb;
        return 0;
    }
    else{
        int entries_of_first_block=(BLOCK_SIZE
		   -sizeof(FileControlBlock)
		    -sizeof(int))/sizeof(int);
        int entries_of_others=BLOCK_SIZE/sizeof(int);
        int numero_blocchi_successori=(d->dcb->num_entries-entries_of_first_block)/entries_of_others;
        FirstDirectoryBlock* aux2=(FirstDirectoryBlock*)malloc(sizeof(BLOCK_SIZE));
        for(int i=0;i<entries_of_first_block;i++){
            int pos=d->dcb->file_blocks[i];
            if(DiskDriver_readBlock(d->f->disk,aux2,pos)==-1){
                return -1;
            }
            printf("%s\n",aux2->fcb.name);
            if(!strcmp(aux2->fcb.name,dirname)) {
                d->directory=d->dcb;
                d->dcb=aux2;
                d->f->cwd=d->dcb;
                return 0;
            }
        }
        for(int num=0;num<numero_blocchi_successori;num++){
            for(int j=0;j<entries_of_others;j++){
                int pos2=d->dcb->file_blocks[j];
                if(DiskDriver_readBlock(d->f->disk,aux2,pos2)==-1){
                return -1;
                }
                if(!strcmp(aux2->fcb.name,dirname)) {
                     d->directory=d->dcb;
                    d->dcb=aux2;
                    d->f->cwd=d->dcb;
                    return 0;
                }
            }
        }

    }
    return -1;
}

// creates a new directory in the current one (stored in fs->current_directory_block)
// 0 on success
// -1 on error
//marco
int fat32_mkDir(DirectoryHandle* d, char* dirname){
    //aggiornare le info della directory corrente
    //deve allocare blocco per quella nuova
    if(d==NULL || dirname==NULL){
        return -1;
    }
    int block_index=DiskDriver_getFreeBlock(d->f->disk,d->f->cwd->fcb.block_in_disk);

    //  printf("scriverò il blocco di questa nuova cartella in pos %d\n",block_index);
    if(block_index==-1){
        return -1;
    }
    
    int first_size=(BLOCK_SIZE
		   -sizeof(FileControlBlock)
		    -sizeof(int))/sizeof(int);
    
   
    
    //QUESTA PORZIONE CREA LA NUOVA CARTELLA
    FirstDirectoryBlock* fdb=(FirstDirectoryBlock*)malloc(sizeof(BLOCK_SIZE));
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
    //printf("write %d bytes for the new dir\n",ret);
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
        int first_succ_index=fat[d->dcb->fcb.block_in_disk];
        //int numero_blocchi_successori=(d->dcb->num_entries-first_size)/(BLOCK_SIZE/(sizeof(int)));
        if(first_succ_index!=fat[d->dcb->fcb.block_in_disk] && first_succ_index!=-1){
            while(first_succ_index!=fat[d->dcb->fcb.block_in_disk] && first_succ_index!=-1){
                first_succ_index=fat[first_succ_index];
            }
            DirectoryBlock* dirBlock=(DirectoryBlock*)malloc(sizeof(BLOCK_SIZE));
            int r=DiskDriver_readBlock(d->f->disk,dirBlock,first_succ_index);
            if(r==-1){
                return -1;
            }
            int flag=1; //PER CAPIRE SE I BLOCCHI SUCCESSORI HANNO SPAZIO PER ALTRE ENTRIES
            for(int z=0;z<BLOCK_SIZE/sizeof(int);z++){
                if(dirBlock->file_blocks[z]==-1){
                    dirBlock->file_blocks[z]=block_index;
                    DiskDriver_writeBlock(d->f->disk, dirBlock, first_succ_index);
                    flag=0;
                    break;
                }
            }
            if(flag){
                int index=DiskDriver_getFreeBlock(d->f->disk,d->dcb->fcb.block_in_disk);
                if(index==-1)
                    return -1;
                dirBlock->file_blocks[0]=block_index;
                DiskDriver_writeBlock(d->f->disk, dirBlock, index);
                d->f->disk->fat[first_succ_index]=index; //successore del successore
                

            }
        }
        else{
            DirectoryBlock* dirBlock=(DirectoryBlock*)malloc(sizeof(BLOCK_SIZE));
            int idx=DiskDriver_getFreeBlock(d->f->disk,d->dcb->fcb.block_in_disk);
            if(idx==-1)
                    return -1;
            dirBlock->file_blocks[0]=block_index;
            DiskDriver_writeBlock(d->f->disk, dirBlock, idx);
            d->f->disk->fat[d->dcb->fcb.block_in_disk]=idx;

        }
 
        }

    d->dcb->num_entries++;
    DiskDriver_writeBlock(d->f->disk, d->dcb, d->dcb->fcb.block_in_disk);



    return 0;


}

// removes the file in the current directory
// returns -1 on failure 0 on success
// if a directory, it removes recursively all contained files
int fat32_remove(fat32* fs, char* filename);


  