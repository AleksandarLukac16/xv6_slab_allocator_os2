#include "types.h"
#define BUDDY_TREE_LEVEL 14

#define MEM_SIZE 128*1024*1024


void *buddy_kalloc(uint16 order);

int buddy_kfree(void *adr,uint16 order);

int binit();