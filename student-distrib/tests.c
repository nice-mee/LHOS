#include "tests.h"
#include "x86_desc.h"
#include "lib.h"
#include "devices/rtc.h"
#include "filesys.h"
#include "pcb.h"
#include "syscall_task.h"

#define PASS 1
#define FAIL 0

/* global variable for tests */
uint8_t buf1[4096] = {'f', 'i', 'l', 'e', ' ', 'r', 'e', 'a', 'c', 'h', ' ', 't', 'h', 'e', ' ', 'e', 'n', 'd', '\n', 0};
uint8_t buf2[4096] = {'f', 'i', 'l', 'e', ' ', 'r', 'e', 'a', 'c', 'h', ' ', 't', 'h', 'e', ' ', 'e', 'n', 'd', '\n', 0};
uint8_t buf3[4096] = {'f', 'i', 'l', 'e', ' ', 'r', 'e', 'a', 'c', 'h', ' ', 't', 'h', 'e', ' ', 'e', 'n', 'd', '\n', 0};
uint8_t buf4[4096] = {'f', 'i', 'l', 'e', ' ', 'r', 'e', 'a', 'c', 'h', ' ', 't', 'h', 'e', ' ', 'e', 'n', 'd', '\n', 0};
uint8_t buf5[4096] = {'f', 'i', 'l', 'e', ' ', 'r', 'e', 'a', 'c', 'h', ' ', 't', 'h', 'e', ' ', 'e', 'n', 'd', '\n', 0};
uint8_t buf6[4096] = {'f', 'i', 'l', 'e', ' ', 'r', 'e', 'a', 'c', 'h', ' ', 't', 'h', 'e', ' ', 'e', 'n', 'd', '\n', 0};

/* format these macros as you see fit */
#define TEST_HEADER 	\
	printf("[TEST %s] Running %s at %s:%d\n", __FUNCTION__, __FUNCTION__, __FILE__, __LINE__)
#define TEST_OUTPUT(name, result)	\
	printf("[TEST %s] Result = %s\n", name, (result) ? "PASS" : "FAIL");

static inline void assertion_failure(){
	/* Use exception #15 for assertions, otherwise
	   reserved by Intel */
	asm volatile("int $15");
}


/* Checkpoint 1 tests */

/* IDT Test - Example
 * 
 * Asserts that first 10 IDT entries are not NULL
 * Inputs: None
 * Outputs: PASS/FAIL
 * Side Effects: None
 * Coverage: Load IDT, IDT definition
 * Files: x86_desc.h/S
 */
int idt_test(){
	TEST_HEADER;

	int i;
	int result = PASS;
	for (i = 0; i < 20; ++i){
		if (i == 15) continue; // reserved by Intel
		if ((idt[i].offset_15_00 == NULL) && 
			(idt[i].offset_31_16 == NULL)){
			assertion_failure();
			result = FAIL;
		}
	}

	return result;
}

/* div_by_zero
 *
 * Try to trigger divided by zero exception.
 * Inputs: none
 * Outputs: none
 * Side Effects: fault should freeze the kernel
 */
int div_by_zero() {
	TEST_HEADER;

	int a = 0;
	int b = 1 / a; // This should raise an exception
	(void)b;
	return FAIL; // Shall not get here
}

/* invalid_opcode
 *
 * Try to trigger invalid opcode exception.
 * Inputs: none
 * Outputs: none
 * Side Effects: fault should freeze the kernel
 */
int invalid_opcode() {
	TEST_HEADER;

	asm volatile("ud2"); // This should generate an invalid opcode
	return FAIL; // Shall not get here
}

/* deref_null
 *
 * Try to dereference NULL.
 * Inputs: none
 * Outputs: none
 * Side Effects: fault should freeze the kernel
 */
int deref_null() {
	TEST_HEADER;

	uint8_t test;
	uint8_t* ptr = NULL;
	test = *(ptr);		// dereference NULL
	return FAIL;		// shouldn't reach here
}

/* deref_video_mem_upperbound
 *
 * Try to dereference the address at upperbound of video mem
 * Inputs: none
 * Outputs: none
 * Side Effects: fault should freeze the kernel
 */
int deref_video_mem_upperbound() {
	TEST_HEADER;

	uint8_t test;
	uint8_t* ptr = (uint8_t*) 0xB9000;	// upperbound of the video mem
	test = *(ptr);
	return FAIL;		// shouldn't reach here
}

/* deref_video_mem_lowerbound
 *
 * Try to dereference the address at upperbound of video mem
 * Inputs: none
 * Outputs: none
 * Side Effects: fault should freeze the kernel
 */
int deref_video_mem_lowerbound() {
	TEST_HEADER;

	uint8_t test;
	uint8_t* ptr = (uint8_t*) 0xB7FFF;	// lowerbound of the video mem
	test = *(ptr);
	return FAIL;		// shouldn't reach here
}

/* deref_ker_mem_upperbound
 *
 * Try to dereference the address at upperbound of kernel mem
 * Inputs: none
 * Outputs: none
 * Side Effects: fault should freeze the kernel
 */
int deref_ker_mem_upperbound() {
	TEST_HEADER;

	uint8_t test;
	uint8_t* ptr = (uint8_t*) 0x800000;	// upperbound of the kernel mem
	test = *(ptr);
	return FAIL;		// shouldn't reach here
}

/* deref_kernel_mem_lowerbound
 *
 * Try to dereference the address at upperbound of kernel mem
 * Inputs: none
 * Outputs: none
 * Side Effects: fault should freeze the kernel
 */
int deref_ker_mem_lowerbound() {
	TEST_HEADER;

	uint8_t test;
	uint8_t* ptr = (uint8_t*) 0x3FFFFF;	// lowerbound of the kernel mem
	test = *(ptr);
	return FAIL;		// shouldn't reach here
}

/* deref_video_mem
 *
 * Try to dereference every linear address in the range of video memory
 * Inputs: None
 * Outputs: PASS or FAIL
 * Side Effects: None
 */
int deref_video_mem() {
	TEST_HEADER;

	/* starting and ending addressed for the video memory*/
	uint8_t* start = (uint8_t*) 0xB8000;
	uint8_t* end   = (uint8_t*) 0xB9000;
	uint8_t* i;
	uint8_t test;
	
	/* derefencing every video mem address */
	for (i = start; i < end; ++i) {
		test = (*i);
	}

	return PASS;		// should reach here
}

/* deref_ker_mem
 *
 * Try to dereference every linear address in the range of kernel memory
 * Inputs: None
 * Outputs: PASS or FAIL
 * Side Effects: None
 */
int deref_ker_mem() {
	TEST_HEADER;

	/* starting and ending addressed for the kernel memory*/
	uint8_t* start = (uint8_t*) 0x400000;
	uint8_t* end   = (uint8_t*) 0x800000;
	uint8_t* i;
	uint8_t test;
	
	/* derefencing every kernel mem address */
	for (i = start; i < end; ++i) {
		test = (*i);
	}

	return PASS;		// should reach here
}

// add more tests here

/* Checkpoint 2 tests */

int terminal_read_test() {
	TEST_HEADER;

	char buf[INPUT_BUF_SIZE + 1]; // +1 for '\0'
	char mem_fence[26] = "This shall not be printed\n";
	(void) mem_fence; // to avoid warning
	int i;
	for (i = 0; i < 4; i++) {
		printf("Please enter something:");
		int32_t ret = vt_read(0, buf, INPUT_BUF_SIZE);
		buf[ret] = '\0'; // add '\0' to the end of the string
		printf("Content of input      :%s", buf);
		printf("Return value: %d\n", ret);
	}
	return PASS;
}

int terminal_write_test() {
	TEST_HEADER;

	char test1[128] = "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Morbi nec eros placerat, pretium justo eget, porta leo. Duis eget in.\n"; // 128 bytes including '\0'
	char test2[128] = "Lorem ipsum dolor sit amet, consectetur adipiscing elit fusce.\n"; // 64 bytes including first '\0'

	printf("Printing test1 with 128 bytes\n");
	vt_write(1, test1, 128);
	printf("Printing test2 with 128 bytes\n");
	vt_write(1, test2, 128);
	printf("Printing test1 with 64 bytes\n");
	vt_write(1, test1, 64);
	printf("Printing test2 with 32 bytes\n");
	vt_write(1, test2, 32);
	return PASS;
}

int read_dentry_by_name_test(const uint8_t* fname){
	TEST_HEADER;

	dentry_t dentry;
	int32_t result;

	result = read_dentry_by_name(fname, &dentry);
	if(result == -1 || strncmp((int8_t*)dentry.file_name, (int8_t*)fname, MAX_FILE_NAME) != 0) return FAIL;
	printf("The dentry's filename is ");
	vt_write(1, dentry.file_name, MAX_FILE_NAME);
	printf("\n");
	printf("The dentry's filetype is %u\n", dentry.file_type);
	printf("The dentry's inode index is %u\n", dentry.inode_index);
	return PASS;
}

int read_dentry_by_index_test(uint32_t index){
	TEST_HEADER;

	dentry_t dentry;
	int32_t result;

	if(read_dentry_by_index(63, &dentry) != -1) return FAIL;
	if(read_dentry_by_index(0, &dentry) == -1) return FAIL;

	result = read_dentry_by_index(index, &dentry);
	if(result == -1) return FAIL;
	printf("The dentry's filename is ");
	vt_write(1, dentry.file_name, MAX_FILE_NAME);
	printf("\n");
	printf("The dentry's filetype is %u\n", dentry.file_type);
	printf("The dentry's inode index is %u\n", dentry.inode_index);		// index is not the inode index
	return PASS;
}

int dir_read_test(){
	TEST_HEADER;
	// can not be used in checkpoint 3
	uint32_t i;
	uint8_t buf[MAX_FILE_NAME];
	int32_t result;

	dir_open(NULL);
	if(dir_read(0, NULL, 0) != -1) return FAIL;
	dir_close(0);

	dir_open(NULL);
	for(i = 0; i < MAX_FILE_NUM + 1; i++){
		result = dir_read(0, buf, 32);
		if(result == -1) return FAIL;
		if(result == 0) return PASS;
		printf("The %u read get file name ", i);
		vt_write(1, buf, MAX_FILE_NAME);
		printf("\n");
	}
	return FAIL;
}

int fread_test(const uint8_t* fname){
	TEST_HEADER;

	if(fopen(fname) == -1) return FAIL;
	if(fread(0, buf1, 1024) == -1) return FAIL;
	printf("First reading result:\n");
	vt_write(1, buf1, 1024);
	printf("\n");
	if(fread(0, buf2, 3090) == -1) return FAIL;
	printf("Second reading result:\n");
	vt_write(1, buf2, 3090);
	printf("\n");
	if(fread(0, buf3, 2048) == -1) return FAIL;
	printf("Third reading result:\n");
	vt_write(1, buf3, 2048);
	printf("\n");
	if(fread(0, buf4, 4096) == -1) return FAIL;
	printf("Forth reading result:\n");
	vt_write(1, buf4, 4096);
	printf("\n");
	fclose(0);

	/*
	fopen(fname);
	if(fread(0, buf4, 4096) == -1) return FAIL;
	printf("First reading result:\n");
	vt_write(1, buf4, 4096);
	printf("\n");
	fclose(0);
	*/

	return PASS;
}


int RTC_change_freq() {
	TEST_HEADER;

	int32_t freq = 2; // corresponding to rate "15"
	int32_t i;
	while(freq <= RTC_BASE_FREQ) {
		printf("current freq:%d\n", freq);
		RTC_write(0, &freq, 4); // let pid = 0
		
		for (i = 0; i < freq; ++i) {
			RTC_read(0, NULL, 0); 
			printf("1");
		}
		freq *= 2; //change to next rate
		printf("\n");
	}
	return PASS;
}


/* Checkpoint 3 tests */

int pcb_tests() {
	int32_t pid = get_current_pid();
	pcb_t* pcb = get_current_pcb();
	printf("pid: %d\n", pid);
	printf("pcb: %x\n", pcb);
	if (pid != 0) return FAIL;
	if (pcb != (pcb_t*)0x7FE000) return FAIL;
	return PASS;
}

int regular_file_syscall_test(){
	TEST_HEADER;
	
	int32_t fd;

	fd = __syscall_open((const uint8_t*)"frame0.txt");
	if(fd == -1) return FAIL;
	if(__syscall_read(fd, buf1, 20) == -1) return FAIL;
	printf("First reading result: File Descriptor%d:\n", fd);
	vt_write(1, buf1, 20);
	printf("\n");
	if(__syscall_read(fd, buf2, 20) == -1) return FAIL;
	printf("Second reading result:\n");
	vt_write(1, buf2, 20);
	printf("\n");
	if(__syscall_read(fd, buf3, 2048) == -1) return FAIL;
	printf("Third reading result:\n");
	vt_write(1, buf3, 2048);
	printf("\n");
	if(__syscall_read(fd, buf4, 4096) == -1) return FAIL;
	printf("Forth reading result:\n");
	vt_write(1, buf4, 4096);
	printf("\n");
	if(__syscall_write(fd, buf1, 5) != -1)	return FAIL;
	if(__syscall_close(fd) == -1) return FAIL;
	return PASS;
}

int execute_file_syscall_test(){
	TEST_HEADER;
	
	int32_t fd;

	fd = __syscall_open((const uint8_t*)"hello");
	if(fd == -1) return FAIL;
	if(__syscall_read(fd, buf1, 20) == -1) return FAIL;
	printf("First reading result: File Descriptor%d:\n", fd);
	vt_write(fd, buf1, 20);
	printf("\n");
	if(__syscall_read(fd, buf2, 20) == -1) return FAIL;
	printf("Second reading result:\n");
	vt_write(1, buf2, 20);
	printf("\n");
	if(__syscall_read(fd, buf3, 2048) == -1) return FAIL;
	printf("Third reading result:\n");
	vt_write(1, buf3, 2048);
	printf("\n");
	if(__syscall_read(fd, buf4, 4096) == -1) return FAIL;
	printf("Forth reading result:\n");
	vt_write(1, buf4, 4096);
	printf("\n");
	if(__syscall_write(fd, buf1, 5) != -1)	return FAIL;
	if(__syscall_close(fd) == -1) return FAIL;
	return PASS;
}


int long_name_file_syscall_test(){
	TEST_HEADER;
	
	int32_t fd;

	fd = __syscall_open((const uint8_t*)"verylargetextwithverylongname.txt");
	if(fd == -1) return PASS;
	return FAIL;
}


int directory_syscall_test(){
	TEST_HEADER;
	
	int32_t fd;
	uint32_t i;
	uint8_t buf[MAX_FILE_NAME];
	uint8_t buf1[5] = {'I', ' ', 'G', '\n', 0};
	int32_t result;

	fd = __syscall_open((const uint8_t*)".");
	printf("fd value: %d\n", fd);
	if(fd == -1) return -1;
	if(__syscall_read(fd, NULL, 2) != -1) return FAIL;
	if(__syscall_close(fd) == -1) return FAIL;

	fd = __syscall_open((const uint8_t*)".");
	if(__syscall_write(fd, buf1, 5) != -1) return FAIL;
	for(i = 0; i < MAX_FILE_NUM + 1; i++){
		result = __syscall_read(fd, buf, MAX_FILE_NAME);
		if(result == -1) return FAIL;
		if(result == 0){
			if(__syscall_close(fd) == -1) return FAIL;
			return PASS;
		}
		printf("The %u read get file name ", i);
		vt_write(1, buf, MAX_FILE_NAME);
		printf("\n");
	}
	return FAIL;
}


int rtc_syscall_test(){
	TEST_HEADER;

	int32_t freq = 2; // corresponding to rate "15"
	int32_t i;
	int32_t fd;

	fd = __syscall_open((const uint8_t*)"rtc");
	printf("fd value: %d\n", fd);
	if(fd == -1) return FAIL;
	while(freq <= RTC_BASE_FREQ) {
		printf("current freq:%d\n", freq);
		if(__syscall_write(fd, &freq, 4) == -1) return FAIL;
		
		for (i = 0; i < freq; ++i) {
			if(__syscall_read(fd, NULL, 0) == -1) return FAIL; 
			printf("1");
		}
		freq *= 2; //change to next rate
		printf("\n");
	}
	if(__syscall_close(fd) == -1) return FAIL;
	return PASS;
}

int keyboard_read_syscall_test(){
	TEST_HEADER;

	char buf[INPUT_BUF_SIZE + 1]; // +1 for '\0'
	char mem_fence[26] = "This shall not be printed\n";
	(void) mem_fence; // to avoid warning
	int i;
	for (i = 0; i < 4; i++) {
		printf("Please enter something:");
		int32_t ret = __syscall_read(0, buf, INPUT_BUF_SIZE);
		if(ret == -1) return FAIL;
		buf[ret] = '\0'; // add '\0' to the end of the string
		printf("Content of input      :%s", buf);
		printf("Return value: %d\n", ret);
	}
	return PASS;
}

int keyboard_write_syscall_test(){
	TEST_HEADER;

	char test1[128] = "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Morbi nec eros placerat, pretium justo eget, porta leo. Duis eget in.\n"; // 128 bytes including '\0'
	char test2[128] = "Lorem ipsum dolor sit amet, consectetur adipiscing elit fusce.\n"; // 64 bytes including first '\0'

	printf("Printing test1 with 128 bytes\n");
	if(__syscall_write(1, test1, 128) == -1) return FAIL;
	printf("Printing test2 with 128 bytes\n");
	if(__syscall_write(1, test2, 128) == -1) return FAIL;
	printf("Printing test1 with 64 bytes\n");
	if(__syscall_write(1, test1, 64) == -1) return FAIL;
	printf("Printing test2 with 32 bytes\n");
	if(__syscall_write(1, test2, 32) == -1) return FAIL;
	return PASS;
}

int heavy_load_syscall_test(){
	TEST_HEADER;
	
	int32_t fd1;
	int32_t fd2;
	int32_t fd3;
	int32_t fd4;
	int32_t fd5;
	int32_t fd6;
	int32_t fd7;

	fd1 = __syscall_open((const uint8_t*)"frame0.txt");
	fd2 = __syscall_open((const uint8_t*)"cat");
	fd3 = __syscall_open((const uint8_t*)"ls");
	fd4 = __syscall_open((const uint8_t*)"rtc");
	fd5 = __syscall_open((const uint8_t*)".");
	fd6 = __syscall_open((const uint8_t*)"frame0.txt");
	fd7 = __syscall_open((const uint8_t*)"hello");
	if(fd1 == -1) return FAIL;
	if(fd2 == -1) return FAIL;
	if(fd3 == -1) return FAIL;
	if(fd4 == -1) return FAIL;
	if(fd5 == -1) return FAIL;
	if(fd6 == -1) return FAIL;
	if(fd7 != -1) return FAIL;

	if(__syscall_read(fd1, buf1, 50) == -1) return FAIL;
	printf("First reading result: File Descriptor%d:\n", fd1);
	vt_write(1, buf1, 50);
	printf("\n");
	if(__syscall_read(fd2, buf2, 500) == -1) return FAIL;
	printf("Second reading result: File Descriptor%d:\n", fd2);
	vt_write(1, buf2, 500);
	printf("\n");
	if(__syscall_read(fd4, buf4, 50) == -1) return FAIL;
	printf("Third reading result: File Descriptor%d:\n", fd4);
	vt_write(1, buf4, 32);
	printf("\n");
	if(__syscall_read(fd5, buf5, 32) == -1) return FAIL;
	printf("Forth reading result: File Descriptor%d:\n", fd5);
	vt_write(1, buf5, 32);
	printf("\n");
	if(__syscall_read(fd6, buf6, 50) == -1) return FAIL;
	printf("Fifth reading result: File Descriptor%d:\n", fd6);
	vt_write(1, buf6, 50);
	printf("\n");

	if(__syscall_close(fd1) == -1) return FAIL;
	fd1 = __syscall_open((const uint8_t*)"frame1.txt");
	if(fd1 == -1) return FAIL;
	if(__syscall_read(fd1, buf1, 50) == -1) return FAIL;
	printf("Sixth reading result: File Descriptor%d:\n", fd1);
	vt_write(1, buf1, 50);
	printf("\n");

	if(__syscall_close(fd1) == -1) return FAIL;
	if(__syscall_close(fd2) == -1) return FAIL;
	if(__syscall_close(fd3) == -1) return FAIL;
	if(__syscall_close(fd4) == -1) return FAIL;
	if(__syscall_close(fd5) == -1) return FAIL;
	if(__syscall_close(fd6) == -1) return FAIL;
	if(__syscall_close(fd7) != -1) return FAIL;
	return PASS;
}

int syscall_edge_test(){
	if(__syscall_open((uint8_t *)"BYDBYD") != -1) return FAIL;
	if(__syscall_read(-1, buf1, 100) != -1) return FAIL;
	if(__syscall_write(-1, buf2, 100) != -1) return FAIL;
	if((__syscall_close(-1) != -1) || (__syscall_close(0) != -1) || (__syscall_close(1) != -1)) return FAIL;
	return PASS;
}

/* Checkpoint 4 tests */
/* Checkpoint 5 tests */


/* Test suite entry point */
void launch_tests(){
	/* Checkpoint 1 Tests*/
	// TEST_OUTPUT("idt_test", idt_test());
	// TEST_OUTPUT("div_by_zero", div_by_zero());
	// TEST_OUTPUT("invalid_opcode", invalid_opcode());
	// TEST_OUTPUT("deref_null", deref_null());
	// TEST_OUTPUT("deref_page_nonexist", deref_page_nonexist());
	// TEST_OUTPUT("deref_video_mem_upperbound", deref_video_mem_upperbound());
	// TEST_OUTPUT("deref_video_mem_lowerbound", deref_video_mem_lowerbound());
	// TEST_OUTPUT("deref_ker_mem_upperbound", deref_ker_mem_upperbound());
	// TEST_OUTPUT("deref_ker_mem_lowerbound", deref_ker_mem_lowerbound())
	// TEST_OUTPUT("deref_video_mem", deref_video_mem());
	// TEST_OUTPUT("deref_ker_mem", deref_ker_mem());

	/* Checkpoint 2 Tests*/
	// TEST_OUTPUT("terminal_read_test", terminal_read_test());
	// TEST_OUTPUT("terminal_write_test", terminal_write_test());
	// TEST_OUTPUT("read_dentry_by_index_test", read_dentry_by_index_test(0)); // this one is derictory
	// TEST_OUTPUT("read_dentry_by_index_test", read_dentry_by_index_test(1));	// this one is regular file
	// TEST_OUTPUT("read_dentry_by_index_test", read_dentry_by_index_test(5));	// this one is rtc
	// TEST_OUTPUT("read_dentry_by_name_test", read_dentry_by_name_test("BYDBYDBYD"));
	// TEST_OUTPUT("read_dentry_by_name_test", read_dentry_by_name_test("."));
	// TEST_OUTPUT("read_dentry_by_index_test", read_dentry_by_index_test(10));
	// TEST_OUTPUT("read_dentry_by_name_test", read_dentry_by_name_test("frame0.txt"));
	// TEST_OUTPUT("read_dentry_by_name_test", read_dentry_by_name_test("verylargetextwithverylongname.txt"));
	// TEST_OUTPUT("dir_read_test", dir_read_test());
	// TEST_OUTPUT("fread_test", fread_test("frame0.txt"));
	// TEST_OUTPUT("fread_test", fread_test("verylargetextwithverylongname.txt"));
	// TEST_OUTPUT("fread_test", fread_test("hello"));
	// TEST_OUTPUT("RTC_change_freq", RTC_change_freq());
	// TEST_OUTPUT("pcb_tests", pcb_tests());
	// TEST_OUTPUT("regular_file_syscall_test", regular_file_syscall_test());
	// TEST_OUTPUT("execute_file_syscall_test", execute_file_syscall_test());
	// TEST_OUTPUT("long_name_file_syscall_test", long_name_file_syscall_test());
	// TEST_OUTPUT("directory_syscall_test", directory_syscall_test());
	// TEST_OUTPUT("rtc_syscall_test", rtc_syscall_test());
	// TEST_OUTPUT("keyboard_read_syscall_test", keyboard_read_syscall_test());
	// TEST_OUTPUT("keyboard_write_syscall_test", keyboard_write_syscall_test());
	// TEST_OUTPUT("heavy_load_syscall_test", heavy_load_syscall_test());
	// TEST_OUTPUT("syscall_edge_test", syscall_edge_test());
}
