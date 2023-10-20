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

/* filesys_init
 *
 * initialize the file system with given in memory boot block
 * Inputs: in_memory_boot_block - the given boot block
 * Outputs: none
 * Side Effects: None
 */
void filesys_init(in_memory_boot_block){
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
    if(fname == NULL || strlen((int8_t*)fname) > 32) return -1;

    /* fail if dentry is invalid */
    if(dentry == NULL) return -1;

    /* search for the target dentry with the same name */
    for(i = 0; i < boot_block->dir_entry_num; i++){
        if(strncmp((int8_t*)fname, (int8_t*)dentries[i].file_name, MAX_FILE_NAME) == 0){
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
    uint32_t file_end = 0;      // check if file has reached the end
    uint32_t start_datablock;
    uint32_t end_datablock;     // closed interval
    uint32_t startbyte_index;
    uint32_t endbyte_index;     // closed interval
    data_block_t cur_datablock;
    inode_t cur_inode;
    /* fail if inode out of boundary or too much offset */
    if(inode >= boot_block->inodes_num || offset > inodes[inode].length) return -1;

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
        for(i = 0; i <= length; i++){
            buf[i] = cur_datablock.data[startbyte_index + i];
        }
        if(file_end == 1){
            return 0;
        } else {
            return length;
        }
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

    if(file_end == 1){
        return 0;
    } else {
        return length;
    }
}


/* dir_open
 *
 * allocate and initialize a directory stream
 * Inputs: fname - the name of directory to be open
 * Outputs: none
 * Return: file_stream_t* - the pointer to the directory stream if successfully
 *         NULL - if open fails
 * Side Effects: None
 */
file_stream_t* dir_open(const uint8_t* fname){
    dentry_t dentry;
    operation_table_t* table_pointer;
    file_stream_t* stream;

    /* check if it is directory */
    read_dentry_by_name(fname, &dentry);
    if(dentry.file_type == DIR_FILE_TYPE){
        /* allocate dynamic memory */
        stream = malloc(sizeof(file_stream_t));
        if(stream == NULL) return NULL;     // return NULL if allocate memory fails
        table_pointer = malloc(sizeof(operation_table_t));
        if(table_pointer == NULL){
            // return NULL if allocate memory fails
            free(stream);
            return NULL;
        }

        /* initialize the fields of stream */
        stream->operation_table = table_pointer;
        table_pointer->open_operation = dir_open;
        table_pointer->close_operation = dir_close;
        table_pointer->read_operation = dir_read;
        table_pointer->write_operation = dir_write;
        stream->inode_index = 0;
        stream->file_position = 0;
        stream->flags = 0;                          // ????? I do not know how to set this
        return stream;
    } else {
        // Open a file that is not directory
        return NULL;
    }
}

/* dir_close
 *
 * close and free the directory stream
 * Inputs: stream - the stream to be closed
 * Outputs: None
 * Return: 0 if successfully, -1 if fails
 * Side Effects:
 */
int32_t dir_close(file_stream_t* stream){
    /* if stream is NULL, return -1 to indicate fails */
    if(stream == NULL)  return -1;

    free(stream->operation_table);
    free(stream);
    return 0;
}

/* dir_read
 *
 * read the directory
 * Inputs:
 * Outputs:
 * Return:
 * Side Effects:
 */
int32_t dir_read(file_stream_t* stream, uint8_t* buf, uint32_t length){
    // nothing to read
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
int32_t dir_write(file_stream_t* stream, const uint8_t* buf, uint32_t length){
    return 0;
}


/* fopen
 *
 * allocate and initialize a file stream
 * Inputs: fname - the name of a file to be open
 * Outputs: none
 * Return: file_stream_t* - the pointer to the file stream if successful
 *         NULL - the open fails
 * Side Effects: None
 */
file_stream_t* fopen(const uint8_t* fname){
    dentry_t dentry;
    operation_table_t* table_pointer;
    file_stream_t* stream;

    /* check if it is file */
    read_dentry_by_name(fname, &dentry);
    if(dentry.file_type == REGULAR_FILE_TYPE){
        /* allocate dynamic memory */
        stream = malloc(sizeof(file_stream_t));
        if(stream == NULL) return NULL;     // return NULL if allocate memory fails
        table_pointer = malloc(sizeof(operation_table_t));
        if(table_pointer == NULL){
            // return NULL if allocate memory fails
            free(stream);
            return NULL;
        }

        /* initialize the fields of stream */
        stream->operation_table = table_pointer;
        table_pointer->open_operation = fopen;
        table_pointer->close_operation = fclose;
        table_pointer->read_operation = fread;
        table_pointer->write_operation = fwrite;
        stream->inode_index = dentry.inode_index;
        stream->file_position = 0;
        stream->flags = 0;                          // ????? I do not know how to set this
        return stream;
    } else {
        // Open a file that is not a regular file
        return NULL;
    }
}

/* fclose
 *
 * close and free the file stream
 * Inputs: stream - the stream to be closed
 * Outputs: None
 * Return: 0 if successfully, -1 if fails
 * Side Effects:
 */
int32_t fclose(file_stream_t* stream){
    /* if stream is NULL, return -1 to indicate fails */
    if(stream == NULL)  return -1;

    free(stream->operation_table);
    free(stream);
    return 0;
}

/* fread
 *
 * read the file
 * Inputs: stream - the file stream
 *         buf - the buffer to load the read data
 *         length - the length of bytes to be read
 * Outputs: None
 * Return: 0 if successful, -1 if fails
 * Side Effects: None
 */
int32_t fread(file_stream_t* stream, uint8_t* buf, uint32_t length){
    uint32_t bytes_read;
    /* if stream or buf is NULL, return -1 to indicate failure */
    if(stream == NULL || buf == NULL) return -1;
    
    /* read the data starting at the file_position */
    bytes_read = read_data(stream->inode_index, stream->file_position, buf, length);
    if(bytes_read == -1) return -1; // indicate reads fail
    if(bytes_read == 0){
        // reach the end, how to set file_position?????????????
    } else{
        stream->file_position += length;        // update file_position
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
int32_t fwrite(file_stream_t* stream, const uint8_t* buf, uint32_t length){
    return 0;
}
