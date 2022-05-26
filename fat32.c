#include "fat32.h"
#include "string.h"
#include <stdio.h>


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
    strcpy(fdb->fcb.name,"/\0");
    fdb->fcb.size=0;
    fdb->fcb.is_dir=1;
    int ret=DiskDriver_writeBlock(fs->disk, fdb, 0);
    printf("write %d bytes for the root\n",ret);
    DirectoryHandle* root=(DirectoryHandle*)malloc(sizeof(DirectoryHandle));
    root->f=fs;
    root->dcb=fdb;
    root->directory=fdb;
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
FileHandle* fat32_createFile(DirectoryHandle* d, const char* filename);

// reads in the (preallocated) blocks array, the name of all files in a directory 
int fat32_listDir(char** names, DirectoryHandle* d);


// opens a file in the  directory d. The file should be exisiting
FileHandle* fat32_openFile(DirectoryHandle* d, const char* filename);


// closes a file handle (destroyes it)
int fat32_close(FileHandle* f);

// writes in the file, at current position for size bytes stored in data
// overwriting and allocating new space if necessary
// returns the number of bytes written
int fat32_write(FileHandle* f, void* data, int size);

// writes in the file, at current position size bytes stored in data
// overwriting and allocating new space if necessary
// returns the number of bytes read
int fat32_read(FileHandle* f, void* data, int size);

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
        DiskDriver_readBlock(d->f->disk,aux,d->directory->fcb.directory_block);
        d->dcb=d->directory;
        d->directory=aux;
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

    printf("scriverò il blocco di questa nuova cartella in pos %d\n",block_index);
    if(block_index==-1){
        return -1;
    }
    int first_size=(BLOCK_SIZE
		   -sizeof(FileControlBlock)
		    -sizeof(int))/sizeof(int);
    printf("il primo blocco contiene %d entries\n",first_size);
   
    
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
    int ret=DiskDriver_writeBlock(d->f->disk, fdb, block_index);
    printf("write %d bytes for the new dir\n",ret);
    d->f->disk->fat[block_index]=block_index; //se la fat ha stesso numero dell'indice è in uso il blocco(valid) ma senza successori
        
    if(d->dcb->num_entries<first_size){
        for(int i=0;i<first_size;i++){
            if(d->dcb->file_blocks[i]==-1){
                d->dcb->file_blocks[i]=block_index;
            }
        }
    }
    else{   //SE HO PIU ENTRIES DI QUANTE NE PUò CONTENER EIL PRIMO BLOCCO, HO SUCCESSORI OPPURE DEVO CREARLO
        int* fat=d->f->disk->fat;
        int first_succ_index=fat[d->dcb->fcb.block_in_disk];
        int numero_blocchi_successori=(d->dcb->num_entries-first_size)/(BLOCK_SIZE/(sizeof(int)));
        if(numero_blocchi_successori>0){
            while(numero_blocchi_successori>0){
                first_succ_index=fat[first_succ_index];
                numero_blocchi_successori--;
            }
            DirectoryBlock* dirBlock=(DirectoryBlock*)malloc(sizeof(BLOCK_SIZE));
            int r=DiskDriver_readBlock(d->f->disk,dirBlock,first_succ_index);
            if(r==-1){
                return -1;
            }
            int flag=1; //PER CAPIRE SE I BLOCCHI SUCCESSORI HANNO SPAZIO PER ALTRE ENTRIES
            for(int z=0;z<BLOCK_SIZE/sizeof(int);z++){
                if(d->dcb->file_blocks[z]!=-1){
                    d->dcb->file_blocks[z]=block_index;
                    flag=0;
                }
            }
            if(flag){
                //DirectoryBlock* dirBlock=(DirectoryBlock*)malloc(sizeof(BLOCK_SIZE));
                int index=DiskDriver_getFreeBlock(d->f->disk,d->dcb->fcb.block_in_disk);
                if(index==-1)
                    return -1;
                DiskDriver_writeBlock(d->f->disk, dirBlock, index);
                d->f->disk->fat[first_succ_index]=index; //successore del successore
                dirBlock->file_blocks[0]=block_index;

            }
        }
        else{
            DirectoryBlock* dirBlock=(DirectoryBlock*)malloc(sizeof(BLOCK_SIZE));
            int idx=DiskDriver_getFreeBlock(d->f->disk,d->dcb->fcb.block_in_disk);
            if(idx==-1)
                    return -1;
            DiskDriver_writeBlock(d->f->disk, dirBlock, idx);
            d->f->disk->fat[d->dcb->fcb.block_in_disk]=idx;
            dirBlock->file_blocks[0]=block_index;

        }
 
        }

    d->dcb->num_entries++;

    return 0;


}

// removes the file in the current directory
// returns -1 on failure 0 on success
// if a directory, it removes recursively all contained files
int fat32_remove(fat32* fs, char* filename);


  