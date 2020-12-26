#include "types.h"
#include "mmap.h"

uint64
sys_mmap(){
    return MAP_FAIL_ADDR;
}

uint64
sys_munmap(){
    return -1;
}