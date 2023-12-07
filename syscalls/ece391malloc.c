#include <stdint.h>

#include "ece391support.h"
#include "ece391syscall.h"

int main ()
{
    void* ptr1;
    void* ptr2;
    void* ptr3;
    void* ptr4;

    ptr1 = ece391_malloc(4096);
    ptr2 = ece391_malloc(2048);
    ptr3 = ece391_malloc(4096);

    *((int32_t*) ptr1) = 1;
    *((int32_t*) ptr2) = 1;
    *((int32_t*) ptr3) = 1;

    ece391_free(ptr2);
    ptr2 = ece391_malloc(2048);
    *((int32_t*) ptr2) = 1;
    ece391_free(ptr2);
    ptr4 = ece391_malloc(4183952); // allocate the whole rest area
    *((int32_t*) ptr4) = 1;
    ptr2 = ece391_malloc(2048);
    *((int32_t*) ptr2) = 1;
    ece391_free(ptr3);
    ece391_free(ptr4);
    ece391_free(ptr2);
    ece391_free(ptr1);
    return 0;
}
