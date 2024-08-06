/* filesys.h - Defines the read-only file system
 * vim:ts=4 noexpandtab
 */

#include "dynamic_alloc.h"
#include "lib.h"

static dynamic_allocation_node_t DA_head_node;
uint32_t block_used[PAGE_TBL_SIZE];         // used to record the corresponding PTE has now been used by how many allocation

/* dynamic_allocation_init - initialize the dynamic memory allocation structure for the specific pcd
 * Inputs: pid - the given pid to mark the specific pcd to initialize
 * Outputs: None
 * Return: None
 */
void dynamic_allocation_init(void){
    /* initialize the fields of dynamic_allocation_node_t */
    DA_head_node.prev = NULL;
    DA_head_node.next = NULL;
    DA_head_node.start_index = 0;
    DA_head_node.end_index = 0;
    DA_head_node.used_bytes_num = 0;
    DA_head_node.available_bytes_num = DYNAMIC_MEMORY_SIZE;
    DA_head_node.ptr = (uint8_t*)DYNAMIC_MEMORY_START;

    memset(block_used, 0, PAGE_TBL_SIZE);
}

/* malloc - dynamic allocate one memory area with input size
 * Inputs: size - the size of the dynamic allocate area
 * Outputs: None
 * Return: the pointer pointing to the target area if successfully
 *         NULL if allocate fails
 */
void* malloc(int32_t size){
    dynamic_allocation_node_t target_DA_node;
    dynamic_allocation_node_t* cur_DA_node = &DA_head_node;
    int32_t i;
    int32_t start_index = 0, end_index;

    /* if size is invalid, allocate fails */
    if(size <= 0) return NULL;

    while(cur_DA_node != NULL){
        /* check if current node has enough space, which includes size itself and the corresponding DA_node */
        if(cur_DA_node->available_bytes_num >= (size + sizeof(dynamic_allocation_node_t))){
            /* if has enough space, first turn on relevant paging structure */
            start_index += cur_DA_node->used_bytes_num + cur_DA_node->available_bytes_num - size - sizeof(dynamic_allocation_node_t);
            end_index = (start_index + size + sizeof(dynamic_allocation_node_t) - 1) / PAGE_SIZE;
            start_index = start_index / PAGE_SIZE;
            for(i = (start_index > 1 ? start_index - 1 : start_index) ; i <= (end_index <= 1022 ? end_index + 1 : end_index) ; i++){
                dynamic_tables[i].P = 1;
                block_used[i]++;
            }

            /* then update the target dynamic allocation node */
            target_DA_node.prev = cur_DA_node;
            target_DA_node.next = cur_DA_node->next;
            target_DA_node.start_index = start_index;
            target_DA_node.end_index = end_index;
            target_DA_node.used_bytes_num = size;
            target_DA_node.available_bytes_num = 0;
            target_DA_node.ptr = cur_DA_node->ptr + cur_DA_node->used_bytes_num + cur_DA_node->available_bytes_num - size;

            /* finally allocate the target space at the end of the available area inside cur_DA_node */
            memcpy(target_DA_node.ptr - sizeof(dynamic_allocation_node_t), &target_DA_node, sizeof(dynamic_allocation_node_t));

            /* update the current dynamic allocation node and linked list*/
            if(cur_DA_node->next != NULL) cur_DA_node->next->prev = (dynamic_allocation_node_t*)(target_DA_node.ptr - sizeof(dynamic_allocation_node_t));
            cur_DA_node->next = (dynamic_allocation_node_t*)(target_DA_node.ptr - sizeof(dynamic_allocation_node_t));
            cur_DA_node->available_bytes_num -= size + sizeof(dynamic_allocation_node_t);

            return target_DA_node.ptr;
        } else {
            if(cur_DA_node == &DA_head_node)    start_index += cur_DA_node->used_bytes_num + cur_DA_node->available_bytes_num;
            else    start_index += cur_DA_node->used_bytes_num + cur_DA_node->available_bytes_num + sizeof(dynamic_allocation_node_t);
            cur_DA_node = cur_DA_node->next;
        }
    }

    /* if no node has enough space, allocate fails */
    return NULL;
}

/* free - free the given area specified by the input ptr
 * Inputs: ptr - the size of the dynamic allocate area
 * Outputs: None
 * Return: 0 if free successfully, -1 otherwise
 */
int32_t free(void* ptr){
    dynamic_allocation_node_t* cur_DA_node = DA_head_node.next;
    dynamic_allocation_node_t* prev_node;
    dynamic_allocation_node_t* next_node;
    int32_t i, start_index, end_index;
    
    /* if ptr is invalid or only has head node (nothing has been allocated yet), free fails */
    if(ptr == NULL || cur_DA_node == NULL) return -1;

    while(cur_DA_node != NULL){
        /* find the area associated with the input ptr */
        if(cur_DA_node->ptr == ptr){
            /* once find it, first update previous node */
            cur_DA_node->prev->available_bytes_num += cur_DA_node->used_bytes_num + cur_DA_node->available_bytes_num + sizeof(dynamic_allocation_node_t);

            /* then remove the node from the linked list */
            prev_node = cur_DA_node->prev;
            next_node = cur_DA_node->next;
            prev_node->next = next_node;
            if(next_node != NULL) next_node->prev = prev_node;

            /* finally, turn off the relevant page structure */
            start_index = cur_DA_node->start_index;
            end_index = cur_DA_node->end_index;
            for(i = (start_index > 1 ? start_index - 1 : start_index) ; i <= (end_index <= 1022 ? end_index + 1 : end_index) ; i++){
                block_used[i]--;
                /* if no other allocation in this PTE, turn it off */
                if(block_used[i] == 0) dynamic_tables[i].P = 0;
            }

            return 0;
        } else {
            cur_DA_node = cur_DA_node->next;
        }
    }
    return -1;
}
