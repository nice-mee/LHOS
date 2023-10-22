/* filesys.c - implement the read-only file system
 * vim:ts=4 noexpandtab
 */

#include "lib.h"
#include "x86_desc.h"
#include "filesys.h"


/* global variables for file system */
boot_block_t* boot_block;
dentry_t* dentries;
inode_t* inodes;
data_block_t* datablocks;
uint32_t dentry_position = 0;   // used to track the read position for dir_read
uint32_t file_position = 0;     // used to track the read position for fread
uint32_t cur_file_inode = 0;    // initialized after fopen
uint32_t file_end = 0;

/* filesys_init
 *
 * initialize the file system with given in memory boot block
 * Inputs: in_memory_boot_block - the given boot block
 * Outputs: none
 * Side Effects: None
 */
void filesys_init(boot_block_t* in_memory_boot_block){
    boot_block = in_memory_boot_block;
    dentries = boot_block->dentries;
    inodes = (inode_t*)(boot_block + 1);    // inodes following the boot_block
    datablocks = (data_block_t*)(inodes + boot_block->inodes_num); // data blocks following the inodes
    return;
}

/* read_dentry_by_name
 *
 * find the file by name and load that file into input dentry
 * Inputs: fname - the given file name to be found
 *         dentry - the dentry to be filled with the found file's fields
 * Outputs: -1 if file does not exist
 *          0 if file can be found
 * Side Effects: change the input dentry
 */
int32_t read_dentry_by_name(const uint8_t* fname, dentry_t* dentry){
    uint32_t i;
    /* fail if fname invalid */
    if(fname == NULL) return -1;

    /* fail if dentry is invalid */
    if(dentry == NULL) return -1;

    /* search for the target dentry with the same name */
    for(i = 0; i < boot_block->dir_entry_num; i++){
        if(strncmp((const char*)fname, (const char*)dentries[i].file_name, MAX_FILE_NAME) == 0){
            /* if found, call read_dentry_by_index to copy them */
            read_dentry_by_index(i, dentry);
            return 0;
        }
    }
    return -1;      // if not found, return -1
}

/* read_dentry_by_index
 *
 * find the file by index and load that file into input dentry
 * Inputs: index- the given index of file to be found
 *         dentry - the dentry to be filled with the found file's fields
 * Outputs: -1 if the index out of reange
 *          0 if file can be found
 * Side Effects: change the input dentry
 */
int32_t read_dentry_by_index(uint32_t index, dentry_t* dentry){
    /* fail if index out of boundary */
    if(index >= boot_block->dir_entry_num) return -1;

    /* fail if dentry is invalid */
    if(dentry == NULL) return -1;

    /* copy the target dentry into the input dentry */
    memcpy(dentry->file_name, dentries[index].file_name, MAX_FILE_NAME);
    dentry->file_type = dentries[index].file_type;
    dentry->inode_index = dentries[index].inode_index;
    return 0;
}

/* read_dentry_by_index
 *
 * read up to length bytes starting from position offset in the file with inode number inode
 * Inputs: inode- the inode index in the inodes
 *         offset - the offset position in the file to be read
 *         buf - the buffer the load the read data
 *         length - the length of bytes to be read
 * Outputs: -1 if input inode number is invalid
 *          0 if the end of the file has been reached
 *          number of bytes read if read successfully without reaching the end of the file
 * Side Effects: change the input buf
 */
int32_t read_data(uint32_t inode, uint32_t offset, uint8_t* buf, uint32_t length){
    uint32_t i;
    uint32_t j;
    uint32_t byte_read = 0;     // record the index to load
    uint32_t start_datablock;
    uint32_t end_datablock;     // closed interval
    uint32_t startbyte_index;
    uint32_t endbyte_index;     // closed interval
    data_block_t cur_datablock;
    inode_t cur_inode;
    /* fail if inode out of boundary */
    if(inode >= boot_block->inodes_num) return -1;

    /* return 0 if reach the end */
    if(offset >= inodes[inode].length) return 0;

    /* fail if buf is invalid */
    if(buf == NULL) return -1;

    /* if read nothing return 0 directly */
    if(length == 0) return 0;

    /* check if read will touch the end of the file */
    if(length + offset > inodes[inode].length){
        file_end = 1;
        length = inodes[inode].length - offset;
    }


    /* calculate the starting and ending datablock reading byte index */
    start_datablock = offset / BLOCK_SIZE;
    end_datablock = (offset + length - 1) / BLOCK_SIZE;
    startbyte_index = offset % BLOCK_SIZE;
    endbyte_index = (offset + length - 1) % BLOCK_SIZE;
    cur_inode = inodes[inode];


    if(start_datablock == end_datablock){
        /* if only involves single datablock, read it and return */
        cur_datablock = datablocks[cur_inode.data_block_index[start_datablock]];
        for(i = 0; i < length; i++){
            buf[i] = cur_datablock.data[startbyte_index + i];
        }
        return length;
    }

    /* if involves multiple datablocks, read the rest of start block */
    cur_datablock = datablocks[cur_inode.data_block_index[start_datablock]];
    for(i = startbyte_index; i < BLOCK_SIZE; i++){
        buf[byte_read] = cur_datablock.data[i];
        byte_read++;
    }

    /* then read the middle blocks */
    for(i = start_datablock + 1; i < end_datablock; i++){
        cur_datablock = datablocks[cur_inode.data_block_index[i]];
        for(j = 0; j < BLOCK_SIZE; j++){
            buf[byte_read] = cur_datablock.data[j];
            byte_read++;
        }
    }

    /* finally read the end blocks */
    cur_datablock = datablocks[cur_inode.data_block_index[end_datablock]];
    for(i = 0; i <= endbyte_index; i++){
        buf[byte_read] = cur_datablock.data[i];
        byte_read++;
    }

    return length;
}


/* dir_open
 *
 * open a directory
 * Inputs: fname - the name of directory to be open
 * Outputs: none
 * Return: 0 if successfully, -1 if open fails
 * Side Effects: None
 */
int32_t dir_open(const uint8_t* fname){
    dentry_position = 0;
    return 0;       // as directory always here
}

/* dir_close
 *
 * close a directory
 * Inputs: inode - the file associated with inode to be closed
 * Outputs: None
 * Return: 0 if successfully, -1 if fails
 * Side Effects: None
 */
int32_t dir_close(uint32_t inode){
    return 0;       // nothing to do here
}

/* dir_read
 *
 * read the directory
 * Inputs: inode - meaningless here
 *         buf - the buffer to load the read data
 *         index - meaningless here
 * Outputs: None
 * Return: 0 if successfully, -1 if fails
 * Side Effects: None
 */
int32_t dir_read(uint32_t inode, uint8_t* buf, uint32_t index){
    dentry_t dentry;
    /* if buf is null, read fails */
    if(buf == NULL) return -1;

    /* read the dentry by index, index is recorded in dentry_position */
    if(read_dentry_by_index(dentry_position, &dentry) == -1) return -1;

    /* then copy the target dentry's file name into the buffer */
    memcpy(buf, dentry.file_name, MAX_FILE_NAME);

    /* update dentry position */
    dentry_position++;                                          // return -1 if reaches the end
    return 0;
}

/* dir_write
 *
 * not been implemented yet (checkpoint 3)
 * Inputs:
 * Outputs:
 * Return:
 * Side Effects:
 */
int32_t dir_write(uint32_t inode, const uint8_t* buf, uint32_t count){
    return -1;
}


/* fopen
 *
 * open a file
 * Inputs: fname - the name of a file to be open
 * Outputs: none
 * Return: 0 if successfully, -1 otherwise
 * Side Effects: None
 */
int32_t fopen(const uint8_t* fname){
    dentry_t dentry;
    /* check if file exists, only check FILE_MAX_NAME size */
    if(read_dentry_by_name(fname, &dentry) == -1) return -1;
    if(dentry.file_type != REGULAR_FILE_TYPE) return -1;

    /* initialize variable */
    cur_file_inode = dentry.inode_index;
    file_position = 0;
    file_end = 0;
    return 0;
}

/* fclose
 *
 * close the file associated with inode
 * Inputs: inode - the file associated with inode to be closed
 * Outputs: None
 * Return: 0 if successfully, -1 if fails
 * Side Effects:
 */
int32_t fclose(uint32_t inode){
    return 0;   // nothing to do here
}

/* fread
 *
 * read the file
 * Inputs: inode - meaningless here
 *         buf - the buffer to load the read data
 *         count - the length of bytes to be read
 * Outputs: None
 * Return: 0 if successful, -1 if fails
 * Side Effects: None
 */
int32_t fread(uint32_t inode, uint8_t* buf, uint32_t count){
    uint32_t bytes_read;
    /* if inode out of boundary or buf is NULL, return -1 to indicate failure */
    if(cur_file_inode >= boot_block->inodes_num || buf == NULL) return -1;

    /* read the data starting at the file_position */
    bytes_read = read_data(cur_file_inode, file_position, buf, count);
    if(bytes_read == -1) return -1; // indicate reads fail
    if(file_end == 1){
        file_position = inodes[cur_file_inode].length;       // file end has been reached
    } else {
        file_position += count;        // update file_position
    }
    return 0;
}

/* fwrite
 *
 * not been implemented yet (checkpoint 3)
 * Inputs:
 * Outputs:
 * Return:
 * Side Effects:
 */
int32_t fwrite(uint32_t inode, const uint8_t* buf, uint32_t count){
    return -1;
}
