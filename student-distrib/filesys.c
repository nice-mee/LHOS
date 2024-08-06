/* filesys.c - implement the read-only file system
 * vim:ts=4 noexpandtab
 */

#include "lib.h"
#include "x86_desc.h"
#include "filesys.h"
#include "pcb.h"


/* global variables for file system */
boot_block_t* boot_block;
dentry_t* dentries;
inode_t* inodes;
data_block_t* datablocks;
int num_db;
int occupy_db[64];

operation_table_t file_operation_table = {
    .open_operation = fopen,
    .close_operation = fclose,
    .read_operation = fread,
    .write_operation = fwrite
};

operation_table_t dir_operation_table = {
    .open_operation = dir_open,
    .close_operation = dir_close,
    .read_operation = dir_read,
    .write_operation = dir_write
};

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
    if(fname == NULL || strlen((int8_t*)fname) > MAX_FILE_NAME) return -1;      // suggested by checkpoint 2, fail if fname too large

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
    data_block_t* cur_datablock;
    inode_t* cur_inode;
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
        length = inodes[inode].length - offset;
    }


    /* calculate the starting and ending datablock reading byte index */
    start_datablock = offset / BLOCK_SIZE;
    end_datablock = (offset + length - 1) / BLOCK_SIZE;
    startbyte_index = offset % BLOCK_SIZE;
    endbyte_index = (offset + length - 1) % BLOCK_SIZE;
    cur_inode = &(inodes[inode]);


    if(start_datablock == end_datablock){
        /* if only involves single datablock, read it and return */
        cur_datablock = &(datablocks[cur_inode->data_block_index[start_datablock]]);
        for(i = 0; i < length; i++){
            buf[i] = cur_datablock->data[startbyte_index + i];
        }
        return length;
    }

    /* if involves multiple datablocks, read the rest of start block */
    cur_datablock = &(datablocks[cur_inode->data_block_index[start_datablock]]);
    for(i = startbyte_index; i < BLOCK_SIZE; i++){
        buf[byte_read] = cur_datablock->data[i];
        byte_read++;
    }

    /* then read the middle blocks */
    for(i = start_datablock + 1; i < end_datablock; i++){
        cur_datablock = &(datablocks[cur_inode->data_block_index[i]]);
        for(j = 0; j < BLOCK_SIZE; j++){
            buf[byte_read] = cur_datablock->data[j];
            byte_read++;
        }
    }

    /* finally read the end blocks */
    cur_datablock = &(datablocks[cur_inode->data_block_index[end_datablock]]);
    for(i = 0; i <= endbyte_index; i++){
        buf[byte_read] = cur_datablock->data[i];
        byte_read++;
    }

    return length;
}


int32_t write_data(uint32_t inode, const uint8_t* buf, uint32_t length){
    uint32_t i;
    uint32_t j;
    uint32_t byte_written = 0;
    data_block_t* cur_datablock;
    inode_t* cur_inode = &(inodes[inode]);
    /* fail if inode out of boundary */
    if(inode >= boot_block->inodes_num) return -1;
    /* fail if buf is invalid */
    if(buf == NULL) return -1;

    /* free up all the current occupied data blocks */
    int32_t num_datablock = (cur_inode->length % BLOCK_SIZE == 0) ? cur_inode->length / BLOCK_SIZE : (cur_inode->length / BLOCK_SIZE + 1);
    for (i = 0; i < num_datablock; i++) {
        occupy_db[cur_inode->data_block_index[i]] = 0;
    }
    memset(cur_inode->data_block_index, 0, sizeof(uint32_t) * num_datablock);
    cur_inode->length = 0;

    /* allocate new data blocks */
    num_datablock = (length % BLOCK_SIZE == 0) ? length / BLOCK_SIZE : (length / BLOCK_SIZE + 1);
    for (i = 0; i < num_datablock; i++) {
        for (j = 0; j < 64; j++) {
            if (occupy_db[j] == 0) {
                occupy_db[j] = 1;
                cur_inode->data_block_index[i] = j;
                break;
            }
        }
        if (j == 64) {
            return -1;
        }
    }

    while (byte_written < length) {
        cur_datablock = &(datablocks[cur_inode->data_block_index[byte_written / BLOCK_SIZE]]);
        if (length - byte_written < BLOCK_SIZE) {
            memcpy(cur_datablock->data, buf + byte_written, length - byte_written);
            byte_written += length - byte_written;
        } else {
            memcpy(cur_datablock->data, buf + byte_written, BLOCK_SIZE);
            byte_written += BLOCK_SIZE;
        }
    }

    cur_inode->length = byte_written;

    return byte_written;
}


/* dir_open
 *
 * open a directory if the fd_array has empty sapce and initialize it
 * Inputs: id - meaningless
 * Outputs: none
 * Return: file descriptor if successfully, -1 if fail
 * Side Effects: None
 */
int32_t dir_open(const uint8_t* id){
    int32_t i;
    pcb_t* cur_pcb = get_current_pcb();
    file_descriptor_t* cur_fd;
    for(i = 2; i < NUM_FILES; i++){     // 2 as stdin and stdout already been used
        cur_fd = &(cur_pcb->fd_array[i]);
        if(cur_fd->flags == READY_TO_BE_USED){
            /* if there exists empty file descriptor, assign it */
            cur_fd->operation_table = &dir_operation_table;
            cur_fd->inode_index = 0;
            cur_fd->file_position = 0;
            cur_fd->flags = IN_USE;
            return i;
        }
    }
    return -1;  // if no empty, open fail
}

/* dir_close
 *
 * close a directory and free that file descriptor
 * Inputs: id - the file descriptor to be closed
 * Outputs: None
 * Return: 0 if successfully, -1 if fails
 * Side Effects: None
 */
int32_t dir_close(int32_t id){
    pcb_t* cur_pcb = get_current_pcb();
    file_descriptor_t* cur_fd;
    /* if if out of boundary, close fail */
    if(id < 2 || id >= NUM_FILES) return -1;

    cur_fd = &(cur_pcb->fd_array[id]);
    /* if that id is invalid, close fail */
    if(cur_fd->flags != IN_USE || cur_fd->inode_index != 0) return -1;

    /* free that file descriptor if every thing all right */
    cur_fd->flags = READY_TO_BE_USED;
    return 0;
}

/* dir_read
 *
 * read the directory
 * Inputs: fd - the file descriptor associated with the directory
 *         buf - the buffer to load the read data
 *         nbytes - the number of bytes read, normally should be a multype of 32
 * Outputs: None
 * Return: number of bytes read if read successfully, 0 if reach the end, -1 if fails
 * Side Effects: None
 */
int32_t dir_read(int32_t fd, void* buf, int32_t nbytes){
    int32_t i,j;
    dentry_t dentry;
    int32_t length = nbytes;
    int32_t dentry_read_num = (nbytes % 32 == 0) ? nbytes / 32 : (nbytes / 32 + 1);     // this equal to the smallest integer that is larger or equal to bytes / 4
    int32_t bytes_read = 0;
    pcb_t* cur_pcb = get_current_pcb();
    file_descriptor_t* cur_fd;
    /* if buf is null or fd is invalid, read fails */
    if(buf == NULL || fd < 2 || fd >= NUM_FILES || nbytes < 0) return -1;

    if(nbytes == 0) return 0;

    cur_fd = &(cur_pcb->fd_array[fd]);
    /* if read reach end, return 0 directly */
    if(cur_fd->file_position == boot_block->dir_entry_num) return 0;

    for(i = 0; i < dentry_read_num; i++){
        /* check if reach the end */
        if(cur_fd->file_position + i >= boot_block->dir_entry_num){
            cur_fd->file_position = boot_block->dir_entry_num;
            return bytes_read;
        }
        /* read the dentry by index, index is recorded in file_position */
        if(read_dentry_by_index(cur_fd->file_position + i, &dentry) == -1) return -1;
        /* then copy the target dentry's file name into the buffer */
        for(j = 0; j < 32 && j < length; j++){               // 32 as max file name 32 bytes
            ((char*)buf)[bytes_read + j] = dentry.file_name[j];
        }
        length -= 32;
        bytes_read += j;
    }

    /* update dentry position */
    cur_fd->file_position += dentry_read_num;

    return bytes_read;
}

/* dir_write
 *
 * not been implemented yet
 * Inputs: fd - directory associated with file descriptor to be wrote
 *         buf - the buffer holding the content to write
 *         nbytes - the length of bytes to write
 * Outputs: None
 * Return: -1 as not implemented
 * Side Effects: None
 */
int32_t dir_write(int32_t fd, const void* buf, int32_t nbytes){
    return -1;
}


/* fopen
 *
 * open a file if there exists empty in fd_array, then assign file descriptor to fd_array
 * Inputs: fname - the name of file to be opened
 * Outputs: none
 * Return: file descriptor if successfully, -1 if open fails
 * Side Effects: None
 */
int32_t fopen(const uint8_t* fname){
    int32_t i;
    pcb_t* cur_pcb = get_current_pcb();
    file_descriptor_t* cur_fd;
    dentry_t dentry;

    /* check if the file exists first */
    if(read_dentry_by_name(fname, &dentry) == -1) return -1;
    /* if this is not a regular file, open fail */
    if(dentry.file_type != REGULAR_FILE_TYPE) return -1;

    for(i = 2; i < NUM_FILES; i++){         // 2 as stdin and stdout already been used
        cur_fd = &(cur_pcb->fd_array[i]);
        if(cur_fd->flags == READY_TO_BE_USED){
            /* if there exists empty file descriptor, assign it */
            cur_fd->operation_table = &file_operation_table;
            cur_fd->inode_index = dentry.inode_index;
            cur_fd->file_position = 0;
            cur_fd->flags = IN_USE;
            return i;
        }
    }
    return -1;  // if no empty, open fail
}

/* fclose
 *
 * close a file and free that file descriptor
 * Inputs: fd - the file associated with inode to be closed
 * Outputs: None
 * Return: 0 if successfully, -1 if fails
 * Side Effects: None
 */
int32_t fclose(int32_t fd){
    pcb_t* cur_pcb = get_current_pcb();
    file_descriptor_t* cur_fd;
    /* if if out of boundary, close fail */
    if(fd < 2 || fd >= NUM_FILES) return -1;

    cur_fd = &(cur_pcb->fd_array[fd]);
    /* if that id is invalid, close fail */
    if(cur_fd->flags != IN_USE) return -1;

    /* free that file descriptor if every thing all right */
    cur_fd->flags = READY_TO_BE_USED;
    return 0;
}

/* fread
 *
 * read the file
 * Inputs: fd - file associated with file descriptor to be read
 *         buf - the buffer to load the read data
 *         nbytes - the length of bytes to be read
 * Outputs: None
 * Return: number of bytes read if successful, 0 if reach the end, -1 if fails
 * Side Effects: None
 */
int32_t fread(int32_t fd, void* buf, int32_t nbytes){
    uint32_t bytes_read;
    pcb_t* cur_pcb = get_current_pcb();
    file_descriptor_t* cur_fd;
    /* if buf is null or fd is invalid or nbytes is invalid, read fails */
    if(buf == NULL || fd < 0 || fd >= NUM_FILES || nbytes < 0) return -1;

    cur_fd = &(cur_pcb->fd_array[fd]);
    /* read the file starting at the file_position */
    bytes_read = read_data(cur_fd->inode_index, cur_fd->file_position, buf, nbytes);

    /* if reach the end, return 0*/
    if(bytes_read == 0) return 0;
    
    /* else, need to update file_position */
    if(bytes_read < nbytes){
        cur_fd->file_position = inodes[cur_fd->inode_index].length;       // file end has been reached
    } else {
        cur_fd->file_position += nbytes;
    }

    return bytes_read;
}

/* fwrite
 *
 * not been implemented yet
 * Inputs: fd - file associated with file descriptor to be wrote
 *         buf - the buffer holding the content to write
 *         nbytes - the length of bytes to write
 * Outputs: None
 * Return: -1 as not implemented
 * Side Effects: None
 */
int32_t fwrite(int32_t fd, const void* buf, int32_t nbytes){
    uint32_t bytes_written;
    pcb_t* cur_pcb = get_current_pcb();
    file_descriptor_t* cur_fd;
    /* if buf is null or fd is invalid or nbytes is invalid, read fails */
    if(buf == NULL || fd < 0 || fd >= NUM_FILES || nbytes < 0) return -1;    
    cur_fd = &(cur_pcb->fd_array[fd]);
    /* read the file starting at 0 
       this is mandatory as text editor requires to overwrite the entire file */
    bytes_written = write_data(cur_fd->inode_index, buf, nbytes);
    if(bytes_written == nbytes)
        return bytes_written;
    return -1;
}
