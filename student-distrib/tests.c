#include "tests.h"
#include "x86_desc.h"
#include "lib.h"
#include "filesys.h"

#define PASS 1
#define FAIL 0

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

	char buf[128];
	int i;
	for (i = 0; i < 4; i++) {
		memset(buf, '\0', 128);
		printf("Please enter something:");
		int32_t ret = vt_read(0, buf, 128);
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
	/*
	const uint8_t* test_fname = "BYDBYDBYD";

	if(read_dentry_by_name(test_fname, &dentry) != -1) return FAIL;
	*/

	result = read_dentry_by_name(fname, &dentry);
	if(result == -1 || strncmp((int8_t*)dentry.file_name, (int8_t*)fname, MAX_FILE_NAME) != 0) return FAIL;
	printf("The dentry's filename is %s\n", dentry.file_name);
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
	printf("The dentry's filename is %s\n", dentry.file_name);
	printf("The dentry's filetype is %u\n", dentry.file_type);
	printf("The dentry's inode index is %u\n", dentry.inode_index);		// index is not the inode index
	return PASS;
}

int dir_read_test(){
	TEST_HEADER;
	
	uint32_t i;
	uint8_t buf[MAX_FILE_NAME];

	dir_open(NULL);
	if(dir_read(0, NULL, 0) != -1) return FAIL;
	dir_close(0);

	dir_open(NULL);
	for(i = 0; i < MAX_FILE_NUM; i++){
		if(dir_read(0, buf, 0) == -1) return PASS;
		printf("The %u dentry has file name %s\n", i, buf);
	}
	return FAIL;
}

int fread_test(const uint8_t* fname){
	TEST_HEADER;
	
	uint8_t buf1[1024];
	uint8_t buf2[4096] = {'f', 'i', 'l', 'e', ' ', 'r', 'e', 'a', 'c', 'h', ' ', 't', 'h', 'e', ' ', 'e', 'n', 'd', '\n', 0};
	uint8_t buf3[2048] = {'f', 'i', 'l', 'e', ' ', 'r', 'e', 'a', 'c', 'h', ' ', 't', 'h', 'e', ' ', 'e', 'n', 'd', '\n', 0};
	uint8_t buf4[4096 + 1024] = {'f', 'i', 'l', 'e', ' ', 'r', 'e', 'a', 'c', 'h', ' ', 't', 'h', 'e', ' ', 'e', 'n', 'd', '\n', 0};


	if(fopen(fname) == -1) return FAIL;
	if(fread(0, buf1, 5) == -1) return FAIL;
	printf("First reading result:\n%s\n", buf1);
	if(fread(0, buf2, 19) == -1) return FAIL;
	printf("Second reading result:\n%s\n", buf2);
	if(fread(0, buf3, 2048) == -1) return FAIL;
	printf("Third reading result:\n%s\n", buf3);
	if(fread(0, buf4, 2048) == -1) return FAIL;
	printf("Forth reading result:\n%s\n", buf4);
	fclose(0);


	/*
	fopen(fname);
	if(fread(0, buf4, 4096 + 1024) == -1) return FAIL;
	printf("First reading result:\n%s\n", buf4);
	fclose(0);
	*/

	return PASS;
}

/* Checkpoint 3 tests */
/* Checkpoint 4 tests */
/* Checkpoint 5 tests */


/* Test suite entry point */
void launch_tests(){
	// TEST_OUTPUT("idt_test", idt_test());
	// TEST_OUTPUT("div_by_zero", div_by_zero());
	// TEST_OUTPUT("invalid_opcode", invalid_opcode());
	// TEST_OUTPUT("deref_null", deref_null());
	// TEST_OUTPUT("deref_page_nonexist", deref_page_nonexist());
	// TEST_OUTPUT("deref_video_mem_upperbound", deref_video_mem_upperbound());
	// TEST_OUTPUT("deref_video_mem_lowerbound", deref_video_mem_lowerbound());
	// TEST_OUTPUT("deref_ker_mem_upperbound", deref_ker_mem_upperbound());
	// TEST_OUTPUT("deref_ker_mem_lowerbound", deref_ker_mem_lowerbound())
	//TEST_OUTPUT("deref_video_mem", deref_video_mem());
	//TEST_OUTPUT("deref_ker_mem", deref_ker_mem());
	//TEST_OUTPUT("terminal_read_test", terminal_read_test());
	//TEST_OUTPUT("terminal_write_test", terminal_write_test());

	// launch your tests here
	/*
	TEST_OUTPUT("read_dentry_by_index_test", read_dentry_by_index_test(0)); // this one is derictory
	TEST_OUTPUT("read_dentry_by_index_test", read_dentry_by_index_test(1));	// this one is regular file
	TEST_OUTPUT("read_dentry_by_index_test", read_dentry_by_index_test(5));	// this one is rtc
	*/
	/*
	TEST_OUTPUT("read_dentry_by_name_test", read_dentry_by_name_test("BYDBYDBYD"));
	TEST_OUTPUT("read_dentry_by_name_test", read_dentry_by_name_test("."));
	TEST_OUTPUT("read_dentry_by_index_test", read_dentry_by_index_test(10));
	TEST_OUTPUT("read_dentry_by_name_test", read_dentry_by_name_test("frame0.txt"));
	TEST_OUTPUT("read_dentry_by_name_test", read_dentry_by_name_test("verylargetextwithverylongname.txt"));
	*/
	// TEST_OUTPUT("dir_read_test", dir_read_test());
	TEST_OUTPUT("fread_test", fread_test("frame0.txt"));
	//TEST_OUTPUT("fread_test", fread_test("verylargetextwithverylongname.txt"));
}
