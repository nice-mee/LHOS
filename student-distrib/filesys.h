/* filesys.h - Defines the read-only file system
 * vim:ts=4 noexpandtab
 */

#ifndef _FILESYS_H
#define _FILESYS_H

#include "types.h"

/* define basic constant for mp3 file system */
#define BLOCK_SIZE 4096     // 4kB per block
#define DIR_ENTRY_SIZE 64   // each directory entry takes 64 bytes
#define MAX_FILE_NUM 63     // max number of files supported as (BLOCK_SIZE / DIR_ENTRY_SIZE) - 1(statistics) = 63


/* define basic constant for directory entry */
#define MAX_FILE_NAME 32    // max bytes number of file name supported
#define RTC_FILE_TYPE 0     // unique number to represent RTC file type
#define DIR_FILE_TYPE 1     // unique number to represent directory file type
#define REGULAR_FILE_TYPE 2 // unique number to represent regular file type, only this type has meaningful index node (inode)

/* define basic constant for file descriptor */
#define IN_USE 1            // mark the flag field in file descriptor as being used
#define READY_TO_BE_USED 0  // mark the flag field in file descriptor as can be used


/* define data structure used by file system */
typedef struct dentry {
    uint8_t file_name[MAX_FILE_NAME];
    uint32_t file_type;
    uint32_t inode_index;   // only meaningful to regular file type
    uint8_t reserved[24];   // 24 reserved bytes, DIR_ENTRY_SIZE - MAX_FILE_NAME - 4(file_type) - 4(inode_index) = 24
} dentry_t;

typedef struct boot_block {
    uint32_t dir_entry_num;
    uint32_t inodes_num;
    uint32_t data_blocks_num;
    uint8_t reserved[52];   // 52 reserved bytes, DIR_ENTRY_SIZE - 4(dir_entry_num) - 4(inodes_num) - 4(data_blocks_num) = 52
    dentry_t dentries[MAX_FILE_NUM];
} boot_block_t;

typedef struct inode {
    uint32_t length;
    uint32_t data_block_index[(BLOCK_SIZE - 4) / 4];    // the rest of block all store data block index
} inode_t;

typedef struct data_block {
    uint8_t data[BLOCK_SIZE];
} data_block_t;

/* define basic function pointers for file descriptor */
typedef int32_t (*open_t)(const uint8_t* filename);
typedef int32_t (*close_t)(int32_t fd);
typedef int32_t (*read_t)(int32_t fd, void* buf, int32_t nbytes);
typedef int32_t (*write_t)(int32_t fd, const void* buf, int32_t nbytes);

/* define data structure used by file descriptor */
typedef struct operation_table {
    open_t open_operation;
    close_t close_operation;
    read_t read_operation;
    write_t write_operation;
} operation_table_t;

typedef struct file_descriptor {
    operation_table_t* operation_table; // file type-specific operation table
    uint32_t inode_index;               // only meaningful to regular file type, 0 for other types
    uint32_t file_position;             // keep track of where the user is currently reading from the file, updated each time after system call read
    uint32_t flags;                     // set to indicate this file descriptor is "in use"
} file_descriptor_t;


/* functions used by file system */

/* initialize the file system*/
void filesys_init(boot_block_t* in_memory_boot_block);

/* find the file by name and load that file into input dentry */
int32_t read_dentry_by_name(const uint8_t* fname, dentry_t* dentry);

/* find the file by index and load that file into input dentry */
int32_t read_dentry_by_index(uint32_t index, dentry_t* dentry);

/* read up to length bytes starting from position offset in the file with inode number inode */
int32_t read_data(uint32_t inode, uint32_t offset, uint8_t* buf, uint32_t length);


/* type-specific operations used in jump table in file descriptor */

/* for RTC operations is wrote in devices/rtc.h */

/* for directory operations */
int32_t dir_open(const uint8_t* id);
int32_t dir_close(int32_t id);
int32_t dir_read(int32_t fd, void* buf, int32_t nbytes);
int32_t dir_write(int32_t fd, const void* buf, int32_t nbytes);

/* for regular file operations */
int32_t fopen(const uint8_t* fd);
int32_t fclose(int32_t fd);
int32_t fread(int32_t fd, void* buf, int32_t nbytes);
int32_t fwrite(int32_t fd, const void* buf, int32_t nbytes);

extern operation_table_t file_operation_table;
extern operation_table_t dir_operation_table;

#endif /* _FILESYS_H */
